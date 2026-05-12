/**
 * @file parser_blocks.c
 * @brief Block dispatch and shared block-parsing helpers.
 *
 * This file implements the top-level block dispatcher (parse_block) that
 * routes to block-type-specific parsers, as well as shared helpers for
 * SYNCHRONOUS header parsing, ASYNCHRONOUS/SYNCHRONOUS body wrappers,
 * raw-text block parsing, and CONST/CONFIG block body parsing.
 *
 * Block-type-specific parsers live in their own files:
 *   parser_port.c           - PORT block
 *   parser_wire.c           - WIRE block
 *   parser_register.c       - REGISTER and LATCH blocks
 *   parser_mem.c            - MEM block
 *   parser_mux.c            - MUX block
 *   parser_cdc.c            - CDC block
 *   parser_project_blocks.c - CLOCKS, PIN, MAP, CLOCK_GEN blocks
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser_internal.h"

/**
 * @brief Parse the parameter list attached to a `SYNCHRONOUS(...)` block header.
 * @param p Active parser.
 * @param block Block node that receives parsed sync-parameter children.
 * @return 0 on success or -1 on parse failure.
 */
static int parse_synchronous_header(Parser *p, JZASTNode *block);

/**
 * @brief Parse a braced block into raw-text child items.
 * @param p Active parser.
 * @param parent Parent AST node that receives the raw-text items.
 * @param unterminated_msg Error text to use when the closing brace is missing.
 * @return 0 on success or -1 on parse failure.
 */
static int parse_braced_raw_items(Parser *p, JZASTNode *parent, const char *unterminated_msg);

/**
 * @brief Parse the body of an `ASYNCHRONOUS` block.
 * @param p Active parser.
 * @param parent Block node that receives parsed statements.
 * @return 0 on success or -1 on parse failure.
 */
static int parse_asynchronous_block_body(Parser *p, JZASTNode *parent);

/**
 * @brief Parse the body of a `SYNCHRONOUS` block.
 * @param p Active parser.
 * @param parent Block node that receives parsed statements.
 * @return 0 on success or -1 on parse failure.
 */
static int parse_synchronous_block_body(Parser *p, JZASTNode *parent);

/**
 * @brief Parse the parameter header of a SYNCHRONOUS block.
 *
 * The SYNCHRONOUS header has the form:
 *   SYNCHRONOUS(key = value, ...)
 *
 * Each key/value pair is parsed and attached to the block as a
 * JZ_AST_SYNC_PARAM child node.
 *
 * @param p     Active parser
 * @param block SYNCHRONOUS block AST node
 * @return 0 on success, -1 on error
 */
static int parse_synchronous_header(Parser *p, JZASTNode *block) {
    if (!match(p, JZ_TOK_LPAREN)) {
        parser_error(p, "expected '(' after SYNCHRONOUS");
        return -1;
    }

    for (;;) {
        const JZToken *t = peek(p);
        if (t->type == JZ_TOK_RPAREN) {
            advance(p); /* consume ')' */
            break;
        }
        if (t->type == JZ_TOK_EOF) {
            parser_error(p, "unterminated SYNCHRONOUS header (missing ')')");
            return -1;
        }

        if (t->type != JZ_TOK_IDENTIFIER || !t->lexeme) {
            parser_error(p, "expected parameter name in SYNCHRONOUS header");
            return -1;
        }
        const JZToken *key_tok = t;
        advance(p);

        if (!match(p, JZ_TOK_OP_ASSIGN)) {
            parser_error(p, "expected '=' after parameter name in SYNCHRONOUS header");
            return -1;
        }

        JZASTNode *value = parse_expression(p);
        if (!value) {
            return -1;
        }

        JZASTNode *param = jz_ast_new(JZ_AST_SYNC_PARAM, key_tok->loc);
        if (!param) {
            jz_ast_free(value);
            return -1;
        }
        jz_ast_set_name(param, key_tok->lexeme);
        if (jz_ast_add_child(param, value) != 0) {
            jz_ast_free(param);
            jz_ast_free(value);
            return -1;
        }
        if (jz_ast_add_child(block, param) != 0) {
            jz_ast_free(param);
            return -1;
        }

        /* Whitespace/newlines are already skipped by the lexer; the next token
           is either another identifier (next parameter) or ')'. */
    }

    return 0;
}

static int filter_decl_block_apply_tokens(Parser *p,
                                          const char *kind,
                                          JZToken **out_tokens,
                                          size_t *out_count,
                                          size_t *out_original_pos)
{
    size_t start = p->pos;
    size_t cap = (p->count > start) ? (p->count - start + 1) : 1;
    JZToken *filtered = (JZToken *)malloc(cap * sizeof(JZToken));
    size_t out_len = 0;
    size_t pos = start;
    int depth = 1;
    int found_apply = 0;

    if (!filtered) return -1;

    while (pos < p->count) {
        const JZToken *t = &p->tokens[pos];

        if (t->type == JZ_TOK_KW_APPLY) {
            char msg[256];
            const char *block_kind = kind ? kind : "declaration";

            found_apply = 1;
            snprintf(msg, sizeof(msg),
                     "@apply found inside %s block; @apply may only appear "
                     "inside ASYNCHRONOUS or SYNCHRONOUS blocks",
                     block_kind);
            parser_report_rule(p, t, "TEMPLATE_APPLY_OUTSIDE_BLOCK", msg);

            pos++;
            while (pos < p->count &&
                   p->tokens[pos].type != JZ_TOK_EOF &&
                   p->tokens[pos].type != JZ_TOK_SEMICOLON &&
                   p->tokens[pos].type != JZ_TOK_RBRACE &&
                   p->tokens[pos].type != JZ_TOK_KW_FEATURE_ELSE &&
                   p->tokens[pos].type != JZ_TOK_KW_ENDFEAT) {
                pos++;
            }
            if (pos < p->count && p->tokens[pos].type == JZ_TOK_SEMICOLON) {
                pos++;
            }
            continue;
        }

        filtered[out_len++] = *t;

        if (t->type == JZ_TOK_LBRACE) {
            depth++;
        } else if (t->type == JZ_TOK_RBRACE) {
            depth--;
            pos++;
            if (depth == 0) break;
            continue;
        } else if (t->type == JZ_TOK_EOF) {
            pos++;
            break;
        }

        pos++;
    }

    if (!found_apply) {
        free(filtered);
        *out_tokens = NULL;
        *out_count = 0;
        *out_original_pos = start;
        return 0;
    }

    if (out_len == 0 || filtered[out_len - 1].type != JZ_TOK_EOF) {
        if (p->count > 0) {
            filtered[out_len++] = p->tokens[p->count - 1];
        }
    }

    *out_tokens = filtered;
    *out_count = out_len;
    *out_original_pos = pos;
    return 0;
}

static int parse_decl_block_body_with_apply_recovery(Parser *p,
                                                     JZASTNode *parent,
                                                     const char *kind,
                                                     int (*body_fn)(Parser *p, JZASTNode *parent))
{
    JZToken *filtered_tokens = NULL;
    size_t filtered_count = 0;
    size_t original_pos_after_body = p->pos;

    if (filter_decl_block_apply_tokens(p, kind, &filtered_tokens, &filtered_count,
                                       &original_pos_after_body) != 0) {
        return -1;
    }

    if (!filtered_tokens) {
        return body_fn(p, parent);
    }

    const JZToken *saved_tokens = p->tokens;
    size_t saved_count = p->count;
    p->tokens = filtered_tokens;
    p->count = filtered_count;
    p->pos = 0;

    int rc = body_fn(p, parent);

    p->tokens = saved_tokens;
    p->count = saved_count;
    p->pos = original_pos_after_body;
    free(filtered_tokens);

    return rc;
}

int parser_recover_decl_block_bad_token(Parser *p, const char *block_kind)
{
    const JZToken *t = peek(p);
    const char *kind = block_kind ? block_kind : "declaration";

    if (!t) return 0;

    if (t->type == JZ_TOK_KW_APPLY) {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "@apply found inside %s block; @apply may only appear "
                 "inside ASYNCHRONOUS or SYNCHRONOUS blocks",
                 kind);
        parser_report_rule(p, t, "TEMPLATE_APPLY_OUTSIDE_BLOCK", msg);
    } else if (t->type == JZ_TOK_KW_CHECK ||
               t->type == JZ_TOK_KW_NEW ||
               t->type == JZ_TOK_KW_PROJECT ||
               t->type == JZ_TOK_KW_ENDPROJ ||
               t->type == JZ_TOK_KW_MODULE ||
               t->type == JZ_TOK_KW_ENDMOD ||
               t->type == JZ_TOK_KW_BLACKBOX ||
               t->type == JZ_TOK_KW_IMPORT ||
               t->type == JZ_TOK_KW_GLOBAL ||
               t->type == JZ_TOK_KW_ENDGLOB) {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "structural directive is not allowed inside %s block",
                 kind);
        parser_report_rule(p, t, "DIRECTIVE_INVALID_CONTEXT", msg);
    } else {
        return 0;
    }

    advance(p);

    if (t->type == JZ_TOK_KW_NEW) {
        int depth = 0;
        while (peek(p)->type != JZ_TOK_EOF &&
               peek(p)->type != JZ_TOK_RBRACE &&
               peek(p)->type != JZ_TOK_KW_FEATURE_ELSE &&
               peek(p)->type != JZ_TOK_KW_ENDFEAT) {
            if (peek(p)->type == JZ_TOK_LBRACE) {
                depth++;
            } else if (peek(p)->type == JZ_TOK_RBRACE) {
                if (depth == 0) break;
                depth--;
            } else if (peek(p)->type == JZ_TOK_SEMICOLON && depth == 0) {
                advance(p);
                break;
            }
            advance(p);
        }
        return 1;
    }

    while (peek(p)->type != JZ_TOK_EOF &&
           peek(p)->type != JZ_TOK_SEMICOLON &&
           peek(p)->type != JZ_TOK_RBRACE &&
           peek(p)->type != JZ_TOK_KW_FEATURE_ELSE &&
           peek(p)->type != JZ_TOK_KW_ENDFEAT) {
        advance(p);
    }
    if (peek(p)->type == JZ_TOK_SEMICOLON) {
        advance(p);
    }
    return 1;
}

/**
 * @brief Parse a generic braced block as raw text items.
 *
 * This is used for blocks whose internal syntax is not structurally parsed
 * by the front end.
 *
 * @param p                Active parser
 * @param parent           Parent AST node
 * @param unterminated_msg Error message for unterminated blocks
 * @return 0 on success, -1 on error
 */
static int parse_braced_raw_items(Parser *p, JZASTNode *parent, const char *unterminated_msg) {
    size_t item_start = p->pos;
    int depth = 1;

    while (p->pos < p->count && depth > 0) {
        const JZToken *t = &p->tokens[p->pos++];
        if (t->type == JZ_TOK_LBRACE) {
            depth++;
        } else if (t->type == JZ_TOK_RBRACE) {
            depth--;
            if (depth == 0) {
                break;
            }
        }

        if (depth == 1 && t->type == JZ_TOK_SEMICOLON) {
            size_t item_end = p->pos - 1; /* include ';' in raw text */
            JZASTNode *raw = make_raw_text_node(p, item_start, item_end);
            if (!raw) return -1;
            if (jz_ast_add_child(parent, raw) != 0) {
                jz_ast_free(raw);
                return -1;
            }
            item_start = p->pos;
        }
    }

    if (depth != 0) {
        parser_error(p, unterminated_msg);
        return -1;
    }

    /* Capture any trailing tokens before the closing '}' as a final raw-text item. */
    size_t block_end = p->pos - 1; /* index of '}' */
    if (item_start < block_end) {
        JZASTNode *raw = make_raw_text_node(p, item_start, block_end);
        if (raw) {
            if (jz_ast_add_child(parent, raw) != 0) {
                jz_ast_free(raw);
                return -1;
            }
        }
    }

    return 0;
}

/**
 * @brief Parse the body of an ASYNCHRONOUS block.
 *
 * @param p      Active parser
 * @param parent ASYNCHRONOUS block AST node
 * @return 0 on success, -1 on error
 */
static int parse_asynchronous_block_body(Parser *p, JZASTNode *parent) {
    /* Opening '{' has already been consumed by parse_block. */
    return parse_statement_list(p, parent, JZ_TOK_RBRACE, 0);
}

/**
 * @brief Parse the body of a SYNCHRONOUS block.
 *
 * Syntax is identical to ASYNCHRONOUS blocks; semantics differ later.
 *
 * @param p      Active parser
 * @param parent SYNCHRONOUS block AST node
 * @return 0 on success, -1 on error
 */
static int parse_synchronous_block_body(Parser *p, JZASTNode *parent) {
    /* Body syntax is identical; semantics differ in later passes. */
    return parse_statement_list(p, parent, JZ_TOK_RBRACE, 1);
}

/**
 * @brief Parse a structured block.
 *
 * Dispatches to the appropriate block-body parser based on node type
 * and block kind.
 *
 * @param p         Active parser
 * @param block_kw  Block keyword token
 * @param kind      Block kind string
 * @param node_type AST node type to create
 * @return Block AST node, or NULL on error
 */
JZASTNode *parse_block(Parser *p, const JZToken *block_kw, const char *kind, JZASTNodeType node_type) {
    JZLocation loc = block_kw->loc;
    JZASTNode *node = jz_ast_new(node_type, loc);
    if (!node) return NULL;
    jz_ast_set_block_kind(node, kind);

    /* For SYNCHRONOUS blocks, parse the header arguments into SyncParam children
       before consuming the '{' that begins the body. */
    if (kind && strcmp(kind, "SYNCHRONOUS") == 0) {
        if (parse_synchronous_header(p, node) != 0) {
            jz_ast_free(node);
            return NULL;
        }
    }

    /* MEM blocks support an optional header attribute list: MEM(TYPE=...){...} or
       MEM(type=...){...}. Capture the raw attribute text between '(' and ')' on
       the MemBlock node so later semantic passes can interpret TYPE. */
    if (node_type == JZ_AST_MEM_BLOCK && match(p, JZ_TOK_LPAREN)) {
        size_t attr_start = p->pos;
        int depth = 1;
        while (p->pos < p->count && depth > 0) {
            const JZToken *tok = &p->tokens[p->pos++];
            if (tok->type == JZ_TOK_LPAREN) {
                depth++;
            } else if (tok->type == JZ_TOK_RPAREN) {
                depth--;
                if (depth == 0) {
                    break;
                }
            }
        }
        if (depth != 0) {
            parser_error(p, "unterminated MEM header (missing ')')");
            jz_ast_free(node);
            return NULL;
        }

        size_t attr_end = p->pos - 1; /* index of ')' */
        if (attr_start < attr_end) {
            char *buf = parser_join_token_lexemes_spaced(p, attr_start, attr_end, 1);
            if (!buf) {
                jz_ast_free(node);
                return NULL;
            }
            jz_ast_set_text(node, buf);
            free(buf);
        }
    }

    if (!match(p, JZ_TOK_LBRACE)) {
        parser_error(p, "expected '{' after block keyword");
        jz_ast_free(node);
        return NULL;
    }

    if (node_type == JZ_AST_CONST_BLOCK) {
        if (parse_decl_block_body_with_apply_recovery(p, node, kind,
                                                      parse_const_block_body) != 0) {
            jz_ast_free(node);
            return NULL;
        }
    } else if (node_type == JZ_AST_PORT_BLOCK) {
        if (parse_decl_block_body_with_apply_recovery(p, node, kind,
                                                      parse_port_block_body) != 0) {
            jz_ast_free(node);
            return NULL;
        }
    } else if (node_type == JZ_AST_WIRE_BLOCK) {
        if (parse_decl_block_body_with_apply_recovery(p, node, kind,
                                                      parse_wire_block_body) != 0) {
            jz_ast_free(node);
            return NULL;
        }
    } else if (node_type == JZ_AST_REGISTER_BLOCK) {
        if (parse_decl_block_body_with_apply_recovery(p, node, kind,
                                                      parse_register_block_body) != 0) {
            jz_ast_free(node);
            return NULL;
        }
    } else if (node_type == JZ_AST_LATCH_BLOCK) {
        if (parse_latch_block_body(p, node) != 0) {
            jz_ast_free(node);
            return NULL;
        }
    } else if (node_type == JZ_AST_MEM_BLOCK) {
        if (parse_mem_block_body(p, node) != 0) {
            jz_ast_free(node);
            return NULL;
        }
    } else if (node_type == JZ_AST_MUX_BLOCK) {
        if (parse_mux_block_body(p, node) != 0) {
            jz_ast_free(node);
            return NULL;
        }
    } else if (kind && strcmp(kind, "CDC") == 0 && node_type == JZ_AST_BLOCK) {
        if (parse_cdc_block_body(p, node) != 0) {
            jz_ast_free(node);
            return NULL;
        }
    } else if (node_type == JZ_AST_CONFIG_BLOCK) {
        if (parse_const_block_body(p, node) != 0) {
            jz_ast_free(node);
            return NULL;
        }
    } else if (node_type == JZ_AST_CLOCKS_BLOCK) {
        if (parse_clocks_block_body(p, node) != 0) {
            jz_ast_free(node);
            return NULL;
        }
    } else if (node_type == JZ_AST_IN_PINS_BLOCK) {
        if (parse_pins_block_body(p, node, "IN_PINS") != 0) {
            jz_ast_free(node);
            return NULL;
        }
    } else if (node_type == JZ_AST_OUT_PINS_BLOCK) {
        if (parse_pins_block_body(p, node, "OUT_PINS") != 0) {
            jz_ast_free(node);
            return NULL;
        }
    } else if (node_type == JZ_AST_INOUT_PINS_BLOCK) {
        if (parse_pins_block_body(p, node, "INOUT_PINS") != 0) {
            jz_ast_free(node);
            return NULL;
        }
    } else if (node_type == JZ_AST_MAP_BLOCK) {
        if (parse_map_block_body(p, node) != 0) {
            jz_ast_free(node);
            return NULL;
        }
    } else if (node_type == JZ_AST_BLOCK && kind && strcmp(kind, "ASYNCHRONOUS") == 0) {
        if (parse_asynchronous_block_body(p, node) != 0) {
            jz_ast_free(node);
            return NULL;
        }
    } else if (node_type == JZ_AST_BLOCK && kind && strcmp(kind, "SYNCHRONOUS") == 0) {
        if (parse_synchronous_block_body(p, node) != 0) {
            jz_ast_free(node);
            return NULL;
        }
    } else {
        if (parse_braced_raw_items(p, node, "unterminated block (missing '}' )") != 0) {
            jz_ast_free(node);
            return NULL;
        }
    }

    return node;
}

/**
 * @brief Parse the body of a CONST block.
 *
 * CONST blocks define named constant expressions.
 *
 * @param p      Active parser
 * @param parent CONST block AST node
 * @return 0 on success, -1 on error
 */
int parse_const_block_body(Parser *p, JZASTNode *parent) {
    for (;;) {
        const JZToken *t = peek(p);
        if (t->type == JZ_TOK_RBRACE) {
            advance(p); /* consume '}' */
            return 0;
        }
        if (t->type == JZ_TOK_EOF) {
            parser_error(p, "unterminated CONST block (missing '}' )");
            return -1;
        }

        /* Feature guard sentinel — stop when inside a feature body */
        if (t->type == JZ_TOK_KW_FEATURE_ELSE || t->type == JZ_TOK_KW_ENDFEAT) {
            return 0;
        }
        /* Feature guard — parse it */
        if (t->type == JZ_TOK_KW_FEATURE) {
            if (parse_feature_guard_in_block(p, parent, parse_const_block_body) != 0)
                return -1;
            continue;
        }
        if (parser_recover_decl_block_bad_token(p, "CONST")) {
            continue;
        }

        const JZToken *name_tok = peek(p);
        if (!is_decl_identifier_token(name_tok)) {
            parser_error_id_syntax_or_parse(p, "expected identifier in CONST block");
            return -1;
        }
        advance(p);

        if (!match(p, JZ_TOK_OP_ASSIGN)) {
            parser_error_id_syntax_or_parse(p, "expected '=' after CONST name");
            return -1;
        }

        /* String literal value: CONST/CONFIG name = "string"; */
        if (peek(p)->type == JZ_TOK_STRING) {
            const JZToken *str_tok = peek(p);
            advance(p); /* consume string */

            if (!match(p, JZ_TOK_SEMICOLON)) {
                parser_error(p, "expected ';' after string value");
                return -1;
            }

            JZASTNode *decl = jz_ast_new(JZ_AST_CONST_DECL, name_tok->loc);
            if (!decl) return -1;
            jz_ast_set_name(decl, name_tok->lexeme);
            jz_ast_set_text(decl, str_tok->lexeme);
            jz_ast_set_block_kind(decl, "STRING");

            if (jz_ast_add_child(parent, decl) != 0) {
                jz_ast_free(decl);
                return -1;
            }
            continue;
        }

        size_t expr_start = p->pos;
        while (peek(p)->type != JZ_TOK_EOF &&
               peek(p)->type != JZ_TOK_SEMICOLON &&
               peek(p)->type != JZ_TOK_RBRACE) {
            advance(p);
        }

        const JZToken *semi = peek(p);
        if (semi->type != JZ_TOK_SEMICOLON) {
            parser_error(p, "expected ';' after CONST expression");
            return -1;
        }

        size_t expr_end = p->pos; /* tokens [expr_start, expr_end) form the expression */
        advance(p); /* consume ';' */

        JZASTNode *decl = jz_ast_new(JZ_AST_CONST_DECL, name_tok->loc);
        if (!decl) return -1;
        jz_ast_set_name(decl, name_tok->lexeme);

        if (expr_start < expr_end) {
            char *buf = parser_join_token_lexemes_spaced(p, expr_start, expr_end, 1);
            if (!buf) {
                jz_ast_free(decl);
                return -1;
            }
            jz_ast_set_text(decl, buf);
            free(buf);
        }

        if (jz_ast_add_child(parent, decl) != 0) {
            jz_ast_free(decl);
            return -1;
        }
    }
}
