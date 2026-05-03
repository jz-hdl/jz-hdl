/**
 * @file parser_import.c
 * @brief Support for @import directives and imported source file handling.
 *
 * This file implements the logic required to load, lex, and parse external
 * source files referenced by @import directives inside a @project block.
 * Imported modules, blackboxes, and global blocks are merged into the host
 * project AST.
 *
 * To ensure diagnostic stability, both the original import spelling used for
 * diagnostics and the resolved filesystem path used for nested resolution are
 * retained for the lifetime of parsing and freed only when parsing is fully
 * complete.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <limits.h>
#endif

#include "parser_internal.h"
#include "path_security.h"

#define JZ_IMPORT_MAX_DEPTH_DEFAULT (64u)
#define JZ_IMPORT_MAX_RETAINED_SOURCE_BYTES_DEFAULT (16u * 1024u * 1024u)
#define JZ_IMPORT_MAX_RETAINED_TOKEN_BYTES_DEFAULT  (64u * 1024u * 1024u)

/* Global storage for imported path lifetime management. */
char  **g_imported_filenames      = NULL;
size_t  g_imported_filenames_len  = 0;
size_t  g_imported_filenames_cap  = 0;
char  **g_imported_resolved_paths      = NULL;
size_t  g_imported_resolved_paths_len  = 0;
size_t  g_imported_resolved_paths_cap  = 0;

static size_t g_import_active_depth = 0;
static size_t g_import_active_source_bytes = 0;
static size_t g_import_active_token_bytes = 0;

typedef struct ImportBudgetGuard {
    int    entered;
    size_t source_bytes;
    size_t token_bytes;
} ImportBudgetGuard;

static void report_import_limit(const Parser *parent,
                                const JZToken *import_token,
                                const char *rule_id,
                                const char *message,
                                const char *path_hint)
{
    if (parent && parent->diagnostics && import_token) {
        parser_report_rule(parent, import_token, rule_id, message);
        return;
    }
    if (import_token) {
        fprintf(stderr,
                "%s:%d:%d: import error: %s\n",
                import_token->loc.filename ? import_token->loc.filename : "<input>",
                import_token->loc.line,
                import_token->loc.column,
                message);
        return;
    }
    fprintf(stderr, "%s:1:1: import error: %s\n",
            path_hint ? path_hint : "<input>",
            message);
}

static int import_budget_enter(ImportBudgetGuard *guard,
                               const Parser *parent,
                               const JZToken *import_token,
                               const char *path_hint)
{
    size_t next_depth = 0;

    if (!guard) {
        return -1;
    }
    if (jz_size_add_checked(g_import_active_depth, 1, &next_depth) != 0 ||
        next_depth > JZ_IMPORT_MAX_DEPTH_DEFAULT) {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "nested @import depth exceeds the compiler safety limit of %u file(s)",
                 (unsigned)JZ_IMPORT_MAX_DEPTH_DEFAULT);
        report_import_limit(parent, import_token, "IMPORT_DEPTH_LIMIT_EXCEEDED", msg, path_hint);
        return -1;
    }
    memset(guard, 0, sizeof(*guard));
    guard->entered = 1;
    g_import_active_depth = next_depth;
    return 0;
}

static int import_budget_add_bytes(size_t *global_total,
                                   size_t *guard_total,
                                   size_t add_bytes,
                                   size_t limit_bytes,
                                   const Parser *parent,
                                   const JZToken *import_token,
                                   const char *path_hint,
                                   const char *resource_name)
{
    size_t new_total = 0;
    size_t new_guard_total = 0;

    if (jz_size_add_checked(*global_total, add_bytes, &new_total) != 0 ||
        new_total > limit_bytes ||
        jz_size_add_checked(*guard_total, add_bytes, &new_guard_total) != 0) {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "nested @import retained %s exceeds the compiler safety limit of %zu byte(s)",
                 resource_name, limit_bytes);
        report_import_limit(parent, import_token, "IMPORT_MEMORY_LIMIT_EXCEEDED", msg, path_hint);
        return -1;
    }

    *global_total = new_total;
    *guard_total = new_guard_total;
    return 0;
}

static void import_budget_leave(ImportBudgetGuard *guard)
{
    if (!guard || !guard->entered) {
        return;
    }
    if (g_import_active_depth > 0) {
        g_import_active_depth--;
    }
    g_import_active_source_bytes =
        (g_import_active_source_bytes >= guard->source_bytes)
            ? (g_import_active_source_bytes - guard->source_bytes)
            : 0;
    g_import_active_token_bytes =
        (g_import_active_token_bytes >= guard->token_bytes)
            ? (g_import_active_token_bytes - guard->token_bytes)
            : 0;
    guard->entered = 0;
    guard->source_bytes = 0;
    guard->token_bytes = 0;
}

/**
 * @brief Record an imported filename for lifetime management.
 *
 * Imported files allocate their filename strings dynamically. These pointers
 * are copied into token locations and AST nodes, so they must remain valid
 * until parsing and diagnostics are complete.
 *
 * This function appends the filename pointer to a global list that is later
 * released by jz_parser_free_imported_filenames().
 *
 * @param path Allocated filename string to retain
 * @return 0 on success, -1 on allocation failure
 */
static int remember_imported_path(char ***paths,
                                  size_t *len,
                                  size_t *cap,
                                  char *path)
{
    if (!path) return 0;
    if (*len == *cap) {
        size_t new_cap = *cap ? *cap * 2 : 8;
        char **new_arr = (char **)realloc(*paths, new_cap * sizeof(char *));
        if (!new_arr) {
            /* If we fail here, we must not drop the pointer, otherwise it would
             * leak without being tracked. Just fall back to leaking this one
             * path; callers can still free previously remembered ones. */
            return -1;
        }
        *paths = new_arr;
        *cap = new_cap;
    }
    (*paths)[(*len)++] = path;
    return 0;
}

static int remember_imported_filename(char *path)
{
    return remember_imported_path(&g_imported_filenames,
                                  &g_imported_filenames_len,
                                  &g_imported_filenames_cap,
                                  path);
}

static int remember_imported_resolved_path(char *path)
{
    return remember_imported_path(&g_imported_resolved_paths,
                                  &g_imported_resolved_paths_len,
                                  &g_imported_resolved_paths_cap,
                                  path);
}

static int imported_name_collides(const JZASTNode *proj, const char *name)
{
    if (!proj || !name) return 0;

    for (size_t i = 0; i < proj->child_count; ++i) {
        JZASTNode *existing = proj->children[i];
        if (!existing || !existing->name) continue;
        if (existing->type != JZ_AST_MODULE &&
            existing->type != JZ_AST_BLACKBOX) {
            continue;
        }
        if (strcmp(existing->name, name) == 0) {
            return 1;
        }
    }

    return 0;
}

static void report_imported_name_collision(const Parser *parent,
                                           const JZASTNode *decl)
{
    if (!parent || !parent->diagnostics || !decl) return;

    JZToken fake;
    memset(&fake, 0, sizeof(fake));
    fake.loc = decl->loc;

    char dup_msg[512];
    snprintf(dup_msg, sizeof(dup_msg),
             "imported module/blackbox `%s` has the same name as an existing\n"
             "definition in the project; rename one to avoid the conflict",
             decl->name ? decl->name : "?");
    parser_report_rule(parent,
                       &fake,
                       "IMPORT_DUP_MODULE_OR_BLACKBOX",
                       dup_msg);
}

static int add_imported_module_like(const Parser *parent,
                                    JZASTNode *proj,
                                    JZASTNode *decl)
{
    if (!proj || !decl) return -1;

    decl->is_imported = 1;

    if (imported_name_collides(proj, decl->name)) {
        report_imported_name_collision(parent, decl);
        jz_ast_free(decl);
        return 0;
    }

    if (jz_ast_add_child(proj, decl) != 0) {
        jz_ast_free(decl);
        return -1;
    }

    return 0;
}

/**
 * @brief Import modules and globals from an external source file.
 *
 * This function resolves a relative or absolute path, loads the source file,
 * lexes it, and parses its top-level constructs. Imported modules, blackboxes,
 * and global blocks are attached directly to the target project AST.
 *
 * Rules enforced:
 * - Each resolved file path may only be imported once per project
 * - Imported files must not contain their own @project blocks
 * - Imported module/blackbox names must not collide with existing ones
 *
 * @param parent       Parser performing the import
 * @param proj         Target project AST node
 * @param rel_path     Path string from the @import directive
 * @param import_token Token corresponding to the @import keyword
 * @return 0 on success, -1 on error
 */
int import_modules_from_path(const Parser *parent,
                                    JZASTNode *proj,
                                    const char *rel_path,
                                    const JZToken *import_token) {
    ImportBudgetGuard budget = {0};
    char base_dir[512];
    char *full_path = NULL;
    char *display_path = NULL;
    char *source = NULL;
    JZTokenStream tokens = {0};
    int result = -1;
    size_t size = 0;

    if (!proj || !rel_path) return -1;
    if (import_budget_enter(&budget, parent, import_token, rel_path) != 0) {
        return -1;
    }

    /* Validate the import path against security policy. */
    base_dir[0] = '\0';
    const char *base_filename =
        (parent && parent->resolved_filename) ? parent->resolved_filename :
        (parent ? parent->filename : NULL);
    if (base_filename) {
        const char *slash = strrchr(base_filename, '/');
        if (slash) {
            size_t dir_len = (size_t)(slash - base_filename);
            if (dir_len >= sizeof(base_dir)) dir_len = sizeof(base_dir) - 1;
            memcpy(base_dir, base_filename, dir_len);
            base_dir[dir_len] = '\0';
        }
    }

    JZLocation import_loc = import_token ? import_token->loc :
        (JZLocation){ parent->filename, 1, 1 };

    /* Validate and canonicalize the import path. */
    if (parent->diagnostics) {
        full_path = jz_path_validate(rel_path, base_dir[0] ? base_dir : NULL,
                                     import_loc, parent->diagnostics);
        if (!full_path) goto cleanup;
    } else {
        char *joined = NULL;
        if (rel_path[0] == '/') {
            joined = jz_strdup(rel_path);
        } else if (base_dir[0]) {
            size_t dir_len = strlen(base_dir);
            size_t path_len = strlen(rel_path);
            size_t joined_len = 0;
            if (jz_size_add_checked(dir_len, 1, &joined_len) != 0 ||
                jz_size_add_checked(joined_len, path_len, &joined_len) != 0 ||
                jz_size_add_checked(joined_len, 1, &joined_len) != 0) {
                goto cleanup;
            }
            joined = (char *)malloc(joined_len);
            if (!joined) goto cleanup;
            memcpy(joined, base_dir, dir_len);
            joined[dir_len] = '/';
            memcpy(joined + dir_len + 1, rel_path, path_len + 1);
        } else {
            joined = jz_strdup(rel_path);
        }
        if (!joined) goto cleanup;

        full_path = realpath(joined, NULL);
        if (!full_path) {
            full_path = joined;
        } else {
            free(joined);
        }
    }

    for (size_t i = 0; i < g_imported_resolved_paths_len; ++i) {
        const char *seen = g_imported_resolved_paths[i];
        if (seen && strcmp(seen, full_path) == 0) {
            if (parent && parent->diagnostics && import_token) {
                parser_report_rule(parent,
                                   import_token,
                                   "IMPORT_FILE_MULTIPLE_TIMES",
                                   "this file has already been imported into the current project;\n"
                                   "remove the duplicate @import directive");
            } else if (import_token) {
                fprintf(stderr,
                        "%s:%d:%d: import error: same source file imported more than once into a single project\n",
                        import_token->loc.filename ? import_token->loc.filename : "<input>",
                        import_token->loc.line,
                        import_token->loc.column);
            } else {
                fprintf(stderr,
                        "%s:1:1: import error: same source file imported more than once into a single project\n",
                        full_path);
            }
            goto cleanup;
        }
    }

    display_path = jz_strdup(rel_path);
    if (!display_path) goto cleanup;

    if (remember_imported_filename(display_path) != 0) goto cleanup;
    display_path = NULL;
    if (remember_imported_resolved_path(full_path) != 0) goto cleanup;
    full_path = NULL;

    source = jz_read_entire_file(g_imported_resolved_paths[g_imported_resolved_paths_len - 1], &size);
    if (!source) {
        fprintf(stderr, "%s:1:1: import error: failed to read imported file '%s'\n",
                g_imported_resolved_paths[g_imported_resolved_paths_len - 1], rel_path);
        goto cleanup;
    }
    {
        size_t retained_source_bytes = 0;
        if (jz_size_add_checked(size, 1, &retained_source_bytes) != 0 ||
            import_budget_add_bytes(&g_import_active_source_bytes,
                                    &budget.source_bytes,
                                    retained_source_bytes,
                                    JZ_IMPORT_MAX_RETAINED_SOURCE_BYTES_DEFAULT,
                                    parent,
                                    import_token,
                                    rel_path,
                                    "source buffers") != 0) {
            goto cleanup;
        }
    }

    if (jz_lex_source(g_imported_filenames[g_imported_filenames_len - 1], source, &tokens, NULL) != 0) {
        fprintf(stderr, "%s:1:1: import error: lexing failed for imported file\n",
                g_imported_filenames[g_imported_filenames_len - 1]);
        goto cleanup;
    }
    {
        size_t retained_token_bytes = 0;
        if (jz_size_mul_checked(tokens.count, sizeof(JZToken), &retained_token_bytes) != 0 ||
            import_budget_add_bytes(&g_import_active_token_bytes,
                                    &budget.token_bytes,
                                    retained_token_bytes,
                                    JZ_IMPORT_MAX_RETAINED_TOKEN_BYTES_DEFAULT,
                                    parent,
                                    import_token,
                                    rel_path,
                                    "token streams") != 0) {
            goto cleanup;
        }
    }

    Parser ip;
    ip.filename = g_imported_filenames[g_imported_filenames_len - 1];
    ip.resolved_filename = g_imported_resolved_paths[g_imported_resolved_paths_len - 1];
    ip.tokens = tokens.tokens;
    ip.count = tokens.count;
    ip.pos = 0;
    ip.diagnostics = parent->diagnostics;

    int saw_project = 0;

    while (peek(&ip)->type != JZ_TOK_EOF) {
        const JZToken *t = peek(&ip);
        if (t->type == JZ_TOK_KW_MODULE) {
            advance(&ip);
            JZASTNode *mod = parse_module(&ip);
            if (!mod) goto cleanup;

            if (add_imported_module_like(parent, proj, mod) != 0) goto cleanup;
        } else if (t->type == JZ_TOK_KW_BLACKBOX) {
            advance(&ip);
            const JZToken *name = peek(&ip);
            if (!is_decl_identifier_token(name)) {
                parser_error(&ip, "expected identifier after @blackbox");
                goto cleanup;
            }
            advance(&ip);

            JZASTNode *bb = jz_ast_new(JZ_AST_BLACKBOX, t->loc);
            if (!bb) goto cleanup;
            jz_ast_set_name(bb, name->lexeme);
            if (!match(&ip, JZ_TOK_LBRACE)) {
                parser_error(&ip, "expected '{' after @blackbox name");
                jz_ast_free(bb);
                goto cleanup;
            }

            if (parse_blackbox_body(&ip, bb) != 0) {
                jz_ast_free(bb);
                goto cleanup;
            }

            if (add_imported_module_like(parent, proj, bb) != 0) goto cleanup;
        } else if (t->type == JZ_TOK_KW_GLOBAL) {
            advance(&ip);
            JZASTNode *glob = parse_global(&ip);
            if (!glob) goto cleanup;

            if (jz_ast_add_child(proj, glob) != 0) {
                jz_ast_free(glob);
                goto cleanup;
            }
        } else if (t->type == JZ_TOK_KW_IMPORT) {
            advance(&ip);
            const JZToken *path_tok = peek(&ip);
            if (path_tok->type != JZ_TOK_STRING || !path_tok->lexeme) {
                parser_error(&ip, "expected string after @import");
                goto cleanup;
            }
            const char *path = path_tok->lexeme;
            advance(&ip);
            if (import_modules_from_path(&ip, proj, path, t) != 0) goto cleanup;
            match(&ip, JZ_TOK_SEMICOLON);
        } else if (t->type == JZ_TOK_KW_PROJECT) {
            if (!saw_project) {
                saw_project = 1;
                if (parent && parent->diagnostics) {
                    parser_report_rule(parent,
                                       t,
                                       "IMPORT_FILE_HAS_PROJECT",
                                       "the imported file contains a @project block, which is forbidden;\n"
                                       "imported files should only contain @module, @blackbox, or @global definitions");
                } else {
                    fprintf(stderr,
                            "%s:%d:%d: import error: imported files may not contain @project\n",
                            t->loc.filename ? t->loc.filename : ip.resolved_filename,
                            t->loc.line, t->loc.column);
                }
            }
            advance(&ip);
            JZASTNode *bad_proj = parse_project(&ip);
            if (bad_proj) jz_ast_free(bad_proj);
            goto cleanup;
        } else {
            advance(&ip);
        }
    }

    result = 0;

cleanup:
    jz_token_stream_free(&tokens);
    free(source);
    free(display_path);
    free(full_path);
    import_budget_leave(&budget);
    return result;
}

/**
 * @brief Free all retained imported filename strings.
 *
 * This function must be called once parsing and all diagnostics are complete.
 * It releases all filename strings retained for imported source files.
 */
void jz_parser_free_imported_filenames(void)
{
    for (size_t i = 0; i < g_imported_filenames_len; ++i) {
        free(g_imported_filenames[i]);
    }
    free(g_imported_filenames);
    g_imported_filenames = NULL;
    g_imported_filenames_len = 0;
    g_imported_filenames_cap = 0;

    for (size_t i = 0; i < g_imported_resolved_paths_len; ++i) {
        free(g_imported_resolved_paths[i]);
    }
    free(g_imported_resolved_paths);
    g_imported_resolved_paths = NULL;
    g_imported_resolved_paths_len = 0;
    g_imported_resolved_paths_cap = 0;
    g_import_active_depth = 0;
    g_import_active_source_bytes = 0;
    g_import_active_token_bytes = 0;
}
