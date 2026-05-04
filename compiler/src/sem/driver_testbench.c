/**
 * @file driver_testbench.c
 * @brief Semantic validation for @testbench blocks.
 *
 * Phase 1 subset of TB-001 through TB-020.
 * This validates the structural correctness of testbench constructs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../../include/ast.h"
#include "../../include/diagnostic.h"
#include "../../include/sem.h"
#include "../../include/rules.h"
#include "driver_internal.h"

/**
 * @brief Report a testbench rule diagnostic with the rule-configured severity.
 * @param diagnostics Diagnostic sink to append to.
 * @param loc Source location for the diagnostic.
 * @param rule_id Rule identifier to emit.
 * @param fallback Fallback message when the rule table has no explanation text.
 */
static void tb_report_rule(JZDiagnosticList *diagnostics,
                           JZLocation loc,
                           const char *rule_id,
                           const char *fallback)
{
    if (!diagnostics || !rule_id) return;
    const JZRuleInfo *rule = jz_rule_lookup(rule_id);
    JZSeverity sev = JZ_SEVERITY_ERROR;
    if (rule) {
        switch (rule->mode) {
        case JZ_RULE_MODE_WRN: sev = JZ_SEVERITY_WARNING; break;
        case JZ_RULE_MODE_INF: sev = JZ_SEVERITY_NOTE; break;
        default: break;
        }
    }
    /* Store the caller's explanation as d->message so that --explain can
     * show it underneath the rule description on the main diagnostic line. */
    const char *msg = fallback ? fallback : rule_id;
    jz_diagnostic_report(diagnostics, loc, sev, rule_id, msg);
}

/**
 * @brief Count value-consuming conversion specifiers in a `@print` format string.
 * @param fmt Format string to inspect.
 * @return Number of `%h`, `%d`, and `%b` placeholders.
 */
static int tb_count_print_value_specifiers(const char *fmt)
{
    int count = 0;

    if (!fmt) return 0;

    for (const char *p = fmt; *p; ++p) {
        if (*p != '%') continue;

        if (strncmp(p, "%tick", 5) == 0) {
            p += 4;
            continue;
        }
        if (strncmp(p, "%ms", 3) == 0) {
            p += 2;
            continue;
        }
        if (p[1] == 'h' || p[1] == 'd' || p[1] == 'b') {
            ++count;
            ++p;
        }
    }

    return count;
}

static void check_print_directive_args(const JZASTNode *node,
                                       JZDiagnosticList *diagnostics)
{
    int value_spec_count;
    int arg_offset;
    int arg_count;
    char msg[512];

    if (!node || (node->type != JZ_AST_PRINT && node->type != JZ_AST_PRINT_IF)) {
        return;
    }

    value_spec_count = tb_count_print_value_specifiers(node->text);
    arg_offset = (node->type == JZ_AST_PRINT_IF) ? 1 : 0;
    arg_count = (int)node->child_count - arg_offset;
    if (arg_count < 0) arg_count = 0;

    if (value_spec_count == arg_count) {
        return;
    }

    snprintf(msg, sizeof(msg),
             "@print format string consumes %d argument%s but %d argument%s %s provided;\n"
             "match each %%h/%%d/%%b with one argument and do not count %%tick/%%ms",
             value_spec_count, value_spec_count == 1 ? "" : "s",
             arg_count, arg_count == 1 ? "" : "s",
             arg_count == 1 ? "is" : "are");
    tb_report_rule(diagnostics, node->loc, "PRT_ARG_COUNT_MISMATCH", msg);
}

static void check_print_directives_recursive(const JZASTNode *node,
                                             JZDiagnosticList *diagnostics)
{
    if (!node) return;

    check_print_directive_args(node, diagnostics);

    for (size_t i = 0; i < node->child_count; ++i) {
        check_print_directives_recursive(node->children[i], diagnostics);
    }
}

static int tb_has_decl(const JZASTNode *tb,
                       JZASTNodeType block_type,
                       JZASTNodeType decl_type,
                       const char *name)
{
    if (!tb || !name) return 0;

    for (size_t i = 0; i < tb->child_count; ++i) {
        const JZASTNode *block = tb->children[i];
        if (!block || block->type != block_type) continue;

        for (size_t j = 0; j < block->child_count; ++j) {
            const JZASTNode *decl = block->children[j];
            if (!decl || decl->type != decl_type || !decl->name) continue;
            if (strcmp(decl->name, name) == 0) {
                return 1;
            }
        }
    }

    return 0;
}

static int tb_has_clock_decl(const JZASTNode *tb, const char *name)
{
    return tb_has_decl(tb, JZ_AST_TB_CLOCK_BLOCK, JZ_AST_TB_CLOCK_DECL, name);
}

static int tb_has_wire_decl(const JZASTNode *tb, const char *name)
{
    return tb_has_decl(tb, JZ_AST_TB_WIRE_BLOCK, JZ_AST_TB_WIRE_DECL, name);
}

static const JZASTNode *tb_find_decl_node(const JZASTNode *tb,
                                          JZASTNodeType block_type,
                                          JZASTNodeType decl_type,
                                          const char *name)
{
    if (!tb || !name) return NULL;

    for (size_t i = 0; i < tb->child_count; ++i) {
        const JZASTNode *block = tb->children[i];
        if (!block || block->type != block_type) continue;

        for (size_t j = 0; j < block->child_count; ++j) {
            const JZASTNode *decl = block->children[j];
            if (!decl || decl->type != decl_type || !decl->name) continue;
            if (strcmp(decl->name, name) == 0) {
                return decl;
            }
        }
    }

    return NULL;
}

static const JZASTNode *tb_find_wire_decl_node(const JZASTNode *tb, const char *name)
{
    return tb_find_decl_node(tb, JZ_AST_TB_WIRE_BLOCK, JZ_AST_TB_WIRE_DECL, name);
}

static const JZASTNode *tb_find_module_node(const JZASTNode *root, const char *name)
{
    if (!root || !name) return NULL;

    for (size_t i = 0; i < root->child_count; ++i) {
        const JZASTNode *child = root->children[i];
        if (!child || !child->name) continue;
        if (child->type != JZ_AST_MODULE && child->type != JZ_AST_BLACKBOX) continue;
        if (strcmp(child->name, name) == 0) {
            return child;
        }
    }

    return NULL;
}

static void tb_collect_feature_decl_symbols(const JZASTNode *node,
                                            JZModuleScope *scope,
                                            JZASTNodeType decl_type,
                                            JZSymbolKind kind,
                                            JZDiagnosticList *diagnostics)
{
    if (!node || !scope) return;

    if (node->type == decl_type && node->name) {
        (void)module_scope_add_symbol(scope, kind, node->name, (JZASTNode *)node, diagnostics);
        return;
    }

    if (node->type != JZ_AST_FEATURE_GUARD) return;

    for (size_t i = 0; i < node->child_count; ++i) {
        tb_collect_feature_decl_symbols(node->children[i], scope, decl_type, kind, diagnostics);
    }
}

static int tb_build_module_scope(const JZASTNode *module,
                                 JZModuleScope *scope,
                                 JZDiagnosticList *diagnostics)
{
    if (!module || !scope) return 0;

    memset(scope, 0, sizeof(*scope));
    scope->name = module->name;
    scope->node = (JZASTNode *)module;

    for (size_t i = 0; i < module->child_count; ++i) {
        const JZASTNode *child = module->children[i];
        if (!child) continue;

        switch (child->type) {
        case JZ_AST_CONST_BLOCK:
            for (size_t j = 0; j < child->child_count; ++j) {
                tb_collect_feature_decl_symbols(child->children[j], scope,
                                                JZ_AST_CONST_DECL, JZ_SYM_CONST,
                                                diagnostics);
            }
            break;
        case JZ_AST_PORT_BLOCK:
            for (size_t j = 0; j < child->child_count; ++j) {
                tb_collect_feature_decl_symbols(child->children[j], scope,
                                                JZ_AST_PORT_DECL, JZ_SYM_PORT,
                                                diagnostics);
            }
            break;
        case JZ_AST_WIRE_BLOCK:
            for (size_t j = 0; j < child->child_count; ++j) {
                tb_collect_feature_decl_symbols(child->children[j], scope,
                                                JZ_AST_WIRE_DECL, JZ_SYM_WIRE,
                                                diagnostics);
            }
            break;
        case JZ_AST_REGISTER_BLOCK:
            for (size_t j = 0; j < child->child_count; ++j) {
                tb_collect_feature_decl_symbols(child->children[j], scope,
                                                JZ_AST_REGISTER_DECL, JZ_SYM_REGISTER,
                                                diagnostics);
            }
            break;
        case JZ_AST_LATCH_BLOCK:
            for (size_t j = 0; j < child->child_count; ++j) {
                tb_collect_feature_decl_symbols(child->children[j], scope,
                                                JZ_AST_LATCH_DECL, JZ_SYM_LATCH,
                                                diagnostics);
            }
            break;
        case JZ_AST_MODULE_INSTANCE:
            if (child->name) {
                (void)module_scope_add_symbol(scope, JZ_SYM_INSTANCE,
                                              child->name, (JZASTNode *)child,
                                              diagnostics);
            }
            break;
        default:
            break;
        }
    }

    return 1;
}

static void tb_free_module_scope(JZModuleScope *scope)
{
    if (!scope) return;
    jz_buf_free(&scope->symbols);
    jz_buf_free(&scope->bus_signal_decls);
}

static int tb_eval_decl_width(const JZASTNode *decl,
                              const JZModuleScope *scope,
                              const JZBuffer *project_symbols,
                              unsigned *out_width)
{
    if (!decl || !decl->width || !out_width) return 0;

    if (eval_simple_positive_decl_int(decl->width, out_width) == 1) {
        return 1;
    }

    if (scope && sem_eval_width_expr_at_loc(decl->width,
                                            scope,
                                            project_symbols,
                                            out_width,
                                            decl->loc) == 0) {
        return 1;
    }

    if (!scope) {
        long long width_value = 0;
        if (sem_eval_const_expr_in_project(decl->width, project_symbols, &width_value) == 0 &&
            width_value > 0) {
            *out_width = (unsigned)width_value;
            return 1;
        }
    }

    return 0;
}

static int tb_eval_lit_call_width(const char *expr,
                                  const JZBuffer *project_symbols,
                                  unsigned *out_width)
{
    if (!expr || !project_symbols || !out_width) return 0;

    const char *p = expr;
    while (*p && isspace((unsigned char)*p)) ++p;
    if (strncmp(p, "lit", 3) != 0) return 0;
    p += 3;

    while (*p && isspace((unsigned char)*p)) ++p;
    if (*p != '(') return 0;
    ++p;

    {
        int depth = 0;
        const char *arg_start = p;
        const char *comma = NULL;
        const char *end = NULL;
        const char *w_start;
        const char *w_end;
        size_t w_len;
        char *w_expr;
        long long width_value = 0;

        for (; *p; ++p) {
            if (*p == '(') {
                depth++;
            } else if (*p == ')') {
                if (depth == 0) {
                    end = p;
                    break;
                }
                depth--;
            } else if (*p == ',' && depth == 0 && !comma) {
                comma = p;
            }
        }

        if (!comma || !end) return 0;

        w_start = arg_start;
        w_end = comma;
        while (w_start < w_end && isspace((unsigned char)*w_start)) ++w_start;
        while (w_end > w_start && isspace((unsigned char)w_end[-1])) --w_end;
        if (w_start >= w_end) return 0;

        w_len = (size_t)(w_end - w_start);
        w_expr = (char *)malloc(w_len + 1);
        if (!w_expr) return 0;
        memcpy(w_expr, w_start, w_len);
        w_expr[w_len] = '\0';

        if (sem_eval_const_expr_in_project(w_expr, project_symbols, &width_value) != 0 ||
            width_value <= 0) {
            free(w_expr);
            return 0;
        }

        free(w_expr);
        *out_width = (unsigned)width_value;
        return 1;
    }
}

static int tb_resolve_global_const_width(const char *name,
                                         const JZBuffer *project_symbols,
                                         unsigned *out_width)
{
    const char *dot;
    char namespace_name[256];
    size_t ns_len;
    const JZSymbol *glob_sym;

    if (!name || !project_symbols || !out_width) return 0;

    dot = strchr(name, '.');
    if (!dot || dot == name || dot[1] == '\0') return 0;

    ns_len = (size_t)(dot - name);
    if (ns_len >= sizeof(namespace_name)) return 0;
    memcpy(namespace_name, name, ns_len);
    namespace_name[ns_len] = '\0';

    glob_sym = project_lookup(project_symbols, namespace_name, JZ_SYM_GLOBAL);
    if (!glob_sym || !glob_sym->node) return 0;

    for (size_t i = 0; i < glob_sym->node->child_count; ++i) {
        const JZASTNode *decl = glob_sym->node->children[i];
        const char *tick;
        size_t prefix_len;
        char width_expr[128];
        long long width_value = 0;

        if (!decl || decl->type != JZ_AST_CONST_DECL || !decl->name || !decl->text) continue;
        if (strcmp(decl->name, dot + 1) != 0) continue;

        tick = strchr(decl->text, '\'');
        if (!tick) {
            return tb_eval_lit_call_width(decl->text, project_symbols, out_width);
        }

        prefix_len = (size_t)(tick - decl->text);
        if (prefix_len == 0 || prefix_len >= sizeof(width_expr)) return 0;
        memcpy(width_expr, decl->text, prefix_len);
        width_expr[prefix_len] = '\0';

        if (sem_eval_const_expr_in_project(width_expr, project_symbols, &width_value) != 0 ||
            width_value <= 0) {
            return 0;
        }

        *out_width = (unsigned)width_value;
        return 1;
    }

    return 0;
}

static int tb_resolve_module_path_width(const JZASTNode *root,
                                        const JZASTNode *module,
                                        const char *path,
                                        const JZBuffer *project_symbols,
                                        unsigned *out_width)
{
    if (!root || !module || !path || !*path || !out_width) return 0;

    JZModuleScope scope;
    const JZSymbol *sym = NULL;
    int ok = 0;
    const char *dot = strchr(path, '.');

    if (!tb_build_module_scope(module, &scope, NULL)) {
        return 0;
    }

    if (!dot) {
        sym = module_scope_lookup_kind(&scope, path, JZ_SYM_PORT);
        if (!sym) sym = module_scope_lookup_kind(&scope, path, JZ_SYM_WIRE);
        if (!sym) sym = module_scope_lookup_kind(&scope, path, JZ_SYM_REGISTER);
        if (!sym) sym = module_scope_lookup_kind(&scope, path, JZ_SYM_LATCH);
        if (sym && sym->node) {
            ok = tb_eval_decl_width(sym->node, &scope, project_symbols, out_width);
        }
        tb_free_module_scope(&scope);
        return ok;
    }

    {
        char inst_name[256];
        size_t inst_len = (size_t)(dot - path);
        if (inst_len == 0 || inst_len >= sizeof(inst_name)) {
            tb_free_module_scope(&scope);
            return 0;
        }

        memcpy(inst_name, path, inst_len);
        inst_name[inst_len] = '\0';

        sym = module_scope_lookup_kind(&scope, inst_name, JZ_SYM_INSTANCE);
        if (!sym || !sym->node || !sym->node->text) {
            tb_free_module_scope(&scope);
            return 0;
        }

        {
            const JZASTNode *child_module = tb_find_module_node(root, sym->node->text);
            tb_free_module_scope(&scope);
            return tb_resolve_module_path_width(root,
                                                child_module,
                                                dot + 1,
                                                project_symbols,
                                                out_width);
        }
    }
}

static const JZASTNode *tb_find_test_instance(const JZASTNode *test, const char *name)
{
    if (!test || !name) return NULL;

    for (size_t i = 0; i < test->child_count; ++i) {
        const JZASTNode *child = test->children[i];
        if (!child || child->type != JZ_AST_MODULE_INSTANCE || !child->name) continue;
        if (strcmp(child->name, name) == 0) {
            return child;
        }
    }

    return NULL;
}

static int tb_resolve_signal_width(const JZASTNode *root,
                                   const JZASTNode *tb,
                                   const JZASTNode *test,
                                   const char *name,
                                   const JZBuffer *project_symbols,
                                   unsigned *out_width)
{
    if (!tb || !name || !out_width) return 0;

    if (!strchr(name, '.')) {
        const JZASTNode *wire_decl = tb_find_wire_decl_node(tb, name);
        if (wire_decl) {
            return tb_eval_decl_width(wire_decl, NULL, project_symbols, out_width);
        }

        if (tb_has_clock_decl(tb, name)) {
            *out_width = 1;
            return 1;
        }
        return 0;
    }

    if (!root || !test) return 0;

    {
        const char *dot = strchr(name, '.');
        char inst_name[256];
        size_t inst_len = (size_t)(dot - name);
        const JZASTNode *inst;
        const JZASTNode *module;

        if (inst_len == 0 || inst_len >= sizeof(inst_name)) return 0;
        memcpy(inst_name, name, inst_len);
        inst_name[inst_len] = '\0';

        inst = tb_find_test_instance(test, inst_name);
        if (!inst || !inst->text) return 0;

        module = tb_find_module_node(root, inst->text);
        return tb_resolve_module_path_width(root, module, dot + 1, project_symbols, out_width);
    }
}

static int tb_literal_width(const JZASTNode *node,
                            const JZBuffer *project_symbols,
                            unsigned *out_width)
{
    JZBitvecType ty;

    if (!node || node->type != JZ_AST_EXPR_LITERAL || !out_width) return 0;

    infer_literal_type((JZASTNode *)node, NULL, &ty);
    if (ty.width > 0) {
        *out_width = ty.width;
        return 1;
    }

    if (node->text) {
        const char *tick = strchr(node->text, '\'');
        if (tick && tick != node->text) {
            size_t prefix_len = (size_t)(tick - node->text);
            char width_expr[128];
            long long width_value = 0;

            if (prefix_len >= sizeof(width_expr)) return 0;
            memcpy(width_expr, node->text, prefix_len);
            width_expr[prefix_len] = '\0';

            if (sem_eval_const_expr_in_project(width_expr, project_symbols, &width_value) == 0 &&
                width_value > 0) {
                *out_width = (unsigned)width_value;
                return 1;
            }
        }
    }

    return 0;
}

static int tb_map_unary_op(const char *op, JZUnaryOp *out)
{
    if (!op || !out) return 0;

    if (strcmp(op, "POS") == 0) {
        *out = JZ_UNARY_PLUS;
        return 1;
    }
    if (strcmp(op, "NEG") == 0) {
        *out = JZ_UNARY_MINUS;
        return 1;
    }
    if (strcmp(op, "BIT_NOT") == 0) {
        *out = JZ_UNARY_BIT_NOT;
        return 1;
    }
    if (strcmp(op, "LOG_NOT") == 0) {
        *out = JZ_UNARY_LOGICAL_NOT;
        return 1;
    }

    return 0;
}

static int tb_map_binary_op(const char *op, JZBinaryOp *out)
{
    if (!op || !out) return 0;

    if (strcmp(op, "ADD") == 0)      *out = JZ_BIN_ADD;
    else if (strcmp(op, "SUB") == 0) *out = JZ_BIN_SUB;
    else if (strcmp(op, "MUL") == 0) *out = JZ_BIN_MUL;
    else if (strcmp(op, "DIV") == 0) *out = JZ_BIN_DIV;
    else if (strcmp(op, "MOD") == 0) *out = JZ_BIN_MOD;
    else if (strcmp(op, "BIT_AND") == 0) *out = JZ_BIN_BIT_AND;
    else if (strcmp(op, "BIT_OR") == 0)  *out = JZ_BIN_BIT_OR;
    else if (strcmp(op, "BIT_XOR") == 0) *out = JZ_BIN_BIT_XOR;
    else if (strcmp(op, "LOG_AND") == 0) *out = JZ_BIN_LOG_AND;
    else if (strcmp(op, "LOG_OR") == 0)  *out = JZ_BIN_LOG_OR;
    else if (strcmp(op, "EQ") == 0)      *out = JZ_BIN_EQ;
    else if (strcmp(op, "NEQ") == 0)     *out = JZ_BIN_NE;
    else if (strcmp(op, "LT") == 0)      *out = JZ_BIN_LT;
    else if (strcmp(op, "LE") == 0)      *out = JZ_BIN_LE;
    else if (strcmp(op, "GT") == 0)      *out = JZ_BIN_GT;
    else if (strcmp(op, "GE") == 0)      *out = JZ_BIN_GE;
    else if (strcmp(op, "SHL") == 0)     *out = JZ_BIN_SHL;
    else if (strcmp(op, "SHR") == 0)     *out = JZ_BIN_SHR;
    else if (strcmp(op, "ASHR") == 0)    *out = JZ_BIN_ASHR;
    else return 0;

    return 1;
}

static int tb_infer_expr_type(const JZASTNode *root,
                              const JZASTNode *tb,
                              const JZASTNode *test,
                              const JZASTNode *expr,
                              const JZBuffer *project_symbols,
                              JZBitvecType *out)
{
    if (!expr || !out) return 0;
    memset(out, 0, sizeof(*out));

    switch (expr->type) {
    case JZ_AST_EXPR_LITERAL: {
        unsigned width = 0;
        if (!tb_literal_width(expr, project_symbols, &width) || width == 0) {
            return 0;
        }
        jz_type_scalar(width, 0, out);
        return 1;
    }

    case JZ_AST_EXPR_IDENTIFIER:
    case JZ_AST_EXPR_QUALIFIED_IDENTIFIER: {
        unsigned width = 0;
        if (!expr->name) {
            return 0;
        }
        if (expr->type == JZ_AST_EXPR_QUALIFIED_IDENTIFIER &&
            tb_resolve_global_const_width(expr->name, project_symbols, &width)) {
            jz_type_scalar(width, 0, out);
            return 1;
        }
        if (!tb_resolve_signal_width(root, tb, test, expr->name, project_symbols, &width)) {
            return 0;
        }
        jz_type_scalar(width, 0, out);
        return 1;
    }

    case JZ_AST_EXPR_UNARY: {
        JZBitvecType operand;
        JZUnaryOp op;
        if (expr->child_count < 1 ||
            !tb_infer_expr_type(root, tb, test, expr->children[0], project_symbols, &operand) ||
            !tb_map_unary_op(expr->block_kind, &op) ||
            jz_type_unary(op, &operand, out) != 0) {
            return 0;
        }
        return 1;
    }

    case JZ_AST_EXPR_BINARY: {
        JZBitvecType lhs;
        JZBitvecType rhs;
        JZBinaryOp op;
        if (expr->child_count < 2 ||
            !tb_infer_expr_type(root, tb, test, expr->children[0], project_symbols, &lhs) ||
            !tb_infer_expr_type(root, tb, test, expr->children[1], project_symbols, &rhs) ||
            !tb_map_binary_op(expr->block_kind, &op) ||
            jz_type_binary(op, &lhs, &rhs, out) != 0) {
            return 0;
        }
        return 1;
    }

    case JZ_AST_EXPR_TERNARY: {
        JZBitvecType cond;
        JZBitvecType on_true;
        JZBitvecType on_false;
        if (expr->child_count < 3 ||
            !tb_infer_expr_type(root, tb, test, expr->children[0], project_symbols, &cond) ||
            !tb_infer_expr_type(root, tb, test, expr->children[1], project_symbols, &on_true) ||
            !tb_infer_expr_type(root, tb, test, expr->children[2], project_symbols, &on_false) ||
            jz_type_ternary(&cond, &on_true, &on_false, out) != 0) {
            return 0;
        }
        return 1;
    }

    case JZ_AST_EXPR_CONCAT: {
        JZBitvecType elems[64];
        size_t count = expr->child_count;
        if (count == 0 || count > sizeof(elems) / sizeof(elems[0])) return 0;

        for (size_t i = 0; i < count; ++i) {
            if (!tb_infer_expr_type(root, tb, test, expr->children[i], project_symbols, &elems[i])) {
                return 0;
            }
        }

        return jz_type_concat(elems, count, out) == 0;
    }

    case JZ_AST_EXPR_SLICE: {
        JZBitvecType base;
        long msb = 0;
        long lsb = 0;
        if (expr->child_count < 3 ||
            !tb_infer_expr_type(root, tb, test, expr->children[0], project_symbols, &base) ||
            !sem_try_const_eval_ast_expr(expr->children[1], &msb) ||
            !sem_try_const_eval_ast_expr(expr->children[2], &lsb) ||
            msb < 0 || lsb < 0) {
            return 0;
        }
        return jz_type_slice(&base, (unsigned)msb, (unsigned)lsb, out) == 0;
    }

    default:
        return 0;
    }
}

static void check_expect_widths(const JZASTNode *root,
                                const JZASTNode *tb,
                                const JZASTNode *test,
                                const JZASTNode *expect_node,
                                const JZBuffer *project_symbols,
                                JZDiagnosticList *diagnostics)
{
    JZBitvecType signal_ty;
    JZBitvecType value_ty;
    const JZASTNode *signal_node;
    const JZASTNode *value_node;
    const char *directive_name;
    char msg[512];

    if (!root || !tb || !test || !expect_node || expect_node->child_count < 2) return;

    signal_node = expect_node->children[0];
    value_node = expect_node->children[1];
    directive_name = expect_node->type == JZ_AST_TB_EXPECT_EQ
        ? "@expect_equal"
        : "@expect_not_equal";

    if (!tb_infer_expr_type(root, tb, test, signal_node, project_symbols, &signal_ty) ||
        !tb_infer_expr_type(root, tb, test, value_node, project_symbols, &value_ty)) {
        return;
    }

    if (signal_ty.width == 0 || value_ty.width == 0 || signal_ty.width == value_ty.width) {
        return;
    }

    snprintf(msg, sizeof(msg),
             "%s value width %u does not match signal width %u",
             directive_name,
             value_ty.width,
             signal_ty.width);
    tb_report_rule(diagnostics, value_node->loc, "TB_EXPECT_WIDTH_MISMATCH", msg);
}

static void check_clock_directive(const JZASTNode *tb,
                                  const JZASTNode *clock_adv,
                                  const JZBuffer *project_symbols,
                                  JZDiagnosticList *diagnostics)
{
    if (!tb || !clock_adv) return;

    if (clock_adv->name && !tb_has_clock_decl(tb, clock_adv->name)) {
        char msg[512];
        snprintf(msg, sizeof(msg),
                 "@clock references `%s`, but that identifier is not declared in the testbench\n"
                 "CLOCK block",
                 clock_adv->name);
        tb_report_rule(diagnostics, clock_adv->loc, "TB_CLOCK_NOT_DECLARED", msg);
    }

    if (clock_adv->text) {
        long long cycles = 0;
        int rc = sem_eval_const_expr_in_project(clock_adv->text, project_symbols, &cycles);
        if (rc != 0 || cycles <= 0) {
            char msg[512];
            snprintf(msg, sizeof(msg),
                     "@clock cycle count `%s` must evaluate to a positive integer",
                     clock_adv->text);
            tb_report_rule(diagnostics, clock_adv->loc, "TB_CLOCK_CYCLE_NOT_POSITIVE", msg);
        }
    }
}

static const JZASTNode *tb_find_bus_def(const JZASTNode *root, const char *bus_name)
{
    if (!root || !bus_name) return NULL;

    for (size_t i = 0; i < root->child_count; ++i) {
        const JZASTNode *child = root->children[i];
        if (child && child->type == JZ_AST_BUS_BLOCK && child->name &&
            strcmp(child->name, bus_name) == 0) {
            return child;
        }
    }

    for (size_t i = 0; i < root->child_count; ++i) {
        const JZASTNode *child = root->children[i];
        if (!child ||
            (child->type != JZ_AST_TESTBENCH && child->type != JZ_AST_SIMULATION)) {
            continue;
        }

        for (size_t j = 0; j < child->child_count; ++j) {
            const JZASTNode *grandchild = child->children[j];
            if (grandchild && grandchild->type == JZ_AST_BUS_BLOCK &&
                grandchild->name && strcmp(grandchild->name, bus_name) == 0) {
                return grandchild;
            }
        }
    }

    return NULL;
}

static void check_tb_bus_wire_declarations(const JZASTNode *tb,
                                           const JZASTNode *root,
                                           JZDiagnosticList *diagnostics)
{
    if (!tb || !root) return;

    for (size_t i = 0; i < tb->child_count; ++i) {
        const JZASTNode *block = tb->children[i];
        if (!block || block->type != JZ_AST_TB_WIRE_BLOCK) continue;

        for (size_t j = 0; j < block->child_count; ++j) {
            const JZASTNode *decl = block->children[j];
            char msg[512];

            if (!decl || decl->type != JZ_AST_TB_WIRE_DECL ||
                !decl->block_kind || strcmp(decl->block_kind, "BUS") != 0) {
                continue;
            }

            if (tb_find_bus_def(root, decl->text)) {
                continue;
            }

            snprintf(msg, sizeof(msg),
                     "testbench BUS wire `%s` references unknown BUS definition `%s`;\n"
                     "declare BUS `%s` at file scope or inside a @testbench before the WIRE block",
                     decl->name ? decl->name : "?",
                     decl->text ? decl->text : "?",
                     decl->text ? decl->text : "?");
            tb_report_rule(diagnostics, decl->loc, "TB_BUS_NOT_FOUND", msg);
        }
    }
}

static void check_tb_bus_instance_bindings(const JZASTNode *test,
                                           const JZASTNode *root,
                                           JZDiagnosticList *diagnostics)
{
    if (!test || !root) return;

    for (size_t i = 0; i < test->child_count; ++i) {
        const JZASTNode *inst = test->children[i];
        if (!inst || inst->type != JZ_AST_MODULE_INSTANCE) continue;

        for (size_t j = 0; j < inst->child_count; ++j) {
            const JZASTNode *decl = inst->children[j];
            char msg[512];

            if (!decl || decl->type != JZ_AST_PORT_DECL ||
                !decl->block_kind || strcmp(decl->block_kind, "BUS") != 0) {
                continue;
            }

            if (tb_find_bus_def(root, decl->text)) {
                continue;
            }

            snprintf(msg, sizeof(msg),
                     "testbench @new BUS binding for `%s` references unknown BUS definition `%s`;\n"
                     "declare BUS `%s` at file scope or inside a @testbench before the TEST block",
                     decl->name ? decl->name : "?",
                     decl->text ? decl->text : "?",
                     decl->text ? decl->text : "?");
            tb_report_rule(diagnostics, decl->loc, "TB_BUS_NOT_FOUND", msg);
        }
    }
}

static void check_stimulus_clock_assignments(const JZASTNode *tb,
                                             const JZASTNode *block,
                                             JZDiagnosticList *diagnostics)
{
    const char *rule_id;
    const char *directive_name;

    if (!tb || !block) return;

    if (block->type == JZ_AST_TB_SETUP) {
        rule_id = "TB_SETUP_CLOCK_ASSIGN";
        directive_name = "@setup";
    } else if (block->type == JZ_AST_TB_UPDATE) {
        rule_id = "TB_UPDATE_CLOCK_ASSIGN";
        directive_name = "@update";
    } else {
        return;
    }

    for (size_t i = 0; i < block->child_count; ++i) {
        const JZASTNode *stmt = block->children[i];
        const JZASTNode *lhs;
        char msg[512];

        if (!stmt || stmt->type != JZ_AST_STMT_ASSIGN || stmt->child_count < 1) {
            continue;
        }

        lhs = stmt->children[0];
        if (!lhs || lhs->type != JZ_AST_EXPR_IDENTIFIER || !lhs->name) {
            continue;
        }

        if (block->type == JZ_AST_TB_UPDATE && !tb_has_wire_decl(tb, lhs->name)) {
            if (tb_has_clock_decl(tb, lhs->name)) {
                snprintf(msg, sizeof(msg),
                         "%s may not assign clock signal `%s`; clocks are driven exclusively\n"
                         "by @clock directives",
                         directive_name, lhs->name);
                tb_report_rule(diagnostics, lhs->loc, rule_id, msg);
            } else {
                snprintf(msg, sizeof(msg),
                         "@update may only assign testbench WIRE identifier `%s`;\n"
                         "declare `%s` in the WIRE block or assign a declared testbench wire",
                         lhs->name, lhs->name);
                tb_report_rule(diagnostics, lhs->loc, "TB_UPDATE_NOT_WIRE", msg);
            }
            continue;
        }

        if (!tb_has_clock_decl(tb, lhs->name)) {
            continue;
        }

        snprintf(msg, sizeof(msg),
                 "%s may not assign clock signal `%s`; clocks are driven exclusively\n"
                 "by @clock directives",
                 directive_name, lhs->name);
        tb_report_rule(diagnostics, lhs->loc, rule_id, msg);
    }
}

/**
 * @brief Check that a @testbench's module name refers to a module defined
 *        as a sibling child of the root.
 */
static void check_tb_module_exists(JZASTNode *tb, JZASTNode *root,
                                   JZDiagnosticList *diagnostics)
{
    if (!tb || !tb->name) return;

    for (size_t i = 0; i < root->child_count; ++i) {
        JZASTNode *child = root->children[i];
        if (child && child->type == JZ_AST_MODULE &&
            child->name && strcmp(child->name, tb->name) == 0) {
            return; /* found */
        }
    }

    {
        char msg[512];
        snprintf(msg, sizeof(msg),
                 "@testbench references module `%s` but no @module with that name was found;\n"
                 "check spelling or ensure the module is defined or @imported",
                 tb->name ? tb->name : "?");
        tb_report_rule(diagnostics, tb->loc, "TB_MODULE_NOT_FOUND", msg);
    }
}

/**
 * @brief Validate a single TEST block inside a @testbench.
 *
 * Checks:
 * - TB-013: exactly one @new
 * - TB-005: exactly one @setup, after @new, before other directives
 */
static void check_test_block(JZASTNode *test, JZDiagnosticList *diagnostics)
{
    if (!test) return;

    int new_count = 0;
    int setup_count = 0;
    int saw_new = 0;
    int saw_setup = 0;

    for (size_t i = 0; i < test->child_count; ++i) {
        JZASTNode *child = test->children[i];
        if (!child) continue;

        if (child->type == JZ_AST_INSTANTIATION ||
            child->type == JZ_AST_MODULE_INSTANCE) {
            new_count++;
            saw_new = 1;
        } else if (child->type == JZ_AST_TB_SETUP) {
            setup_count++;
            if (!saw_new) {
                tb_report_rule(diagnostics, child->loc, "TB_SETUP_POSITION",
                               "@setup must appear after @new; declare the DUT instance first,\n"
                               "then configure it in @setup");
            }
            saw_setup = 1;
        } else {
            /* Any directive after @new but before @setup */
            if (saw_new && !saw_setup &&
                child->type != JZ_AST_TB_SETUP) {
                /* This is a directive before @setup — only report if there's
                 * no @setup at all (will be caught below).
                 */
            }
        }
    }

    if (new_count == 0) {
        tb_report_rule(diagnostics, test->loc, "TB_MULTIPLE_NEW",
                       "TEST block is missing a @new instantiation; each TEST must create\n"
                       "exactly one DUT instance with @new");
    } else if (new_count > 1) {
        {
            char msg[512];
            snprintf(msg, sizeof(msg),
                     "TEST block contains %d @new instantiations but exactly one is required;\n"
                     "remove the extra @new statements", new_count);
            tb_report_rule(diagnostics, test->loc, "TB_MULTIPLE_NEW", msg);
        }
    }

    if (setup_count == 0) {
        tb_report_rule(diagnostics, test->loc, "TB_SETUP_POSITION",
                       "TEST block is missing a @setup block; each TEST must contain\n"
                       "exactly one @setup to configure the DUT after @new");
    } else if (setup_count > 1) {
        {
            char msg[512];
            snprintf(msg, sizeof(msg),
                     "TEST block contains %d @setup blocks but exactly one is allowed;\n"
                     "merge your setup logic into a single @setup block", setup_count);
            tb_report_rule(diagnostics, test->loc, "TB_SETUP_POSITION", msg);
        }
    }
}

/**
 * @brief Validate a @testbench block.
 */
static void check_test_block_semantics(const JZASTNode *tb,
                                       const JZASTNode *root,
                                       const JZASTNode *test,
                                       const JZBuffer *project_symbols,
                                       JZDiagnosticList *diagnostics)
{
    if (!tb || !root || !test) return;

    check_tb_bus_instance_bindings(test, root, diagnostics);

    for (size_t i = 0; i < test->child_count; ++i) {
        const JZASTNode *child = test->children[i];
        if (!child) continue;

        if (child->type == JZ_AST_TB_SETUP || child->type == JZ_AST_TB_UPDATE) {
            check_stimulus_clock_assignments(tb, child, diagnostics);
        } else if (child->type == JZ_AST_TB_CLOCK_ADV) {
            check_clock_directive(tb, child, project_symbols, diagnostics);
        } else if (child->type == JZ_AST_TB_EXPECT_EQ ||
                   child->type == JZ_AST_TB_EXPECT_NEQ) {
            check_expect_widths(root, tb, test, child, project_symbols, diagnostics);
        }
    }

    check_print_directives_recursive(test, diagnostics);
}

static void check_simulation_semantics(const JZASTNode *sim,
                                       const JZASTNode *root,
                                       JZDiagnosticList *diagnostics)
{
    if (!sim || !root) return;

    check_tb_bus_wire_declarations(sim, root, diagnostics);
    check_print_directives_recursive(sim, diagnostics);
}

static void validate_testbench(JZASTNode *tb, JZASTNode *root,
                               const JZBuffer *project_symbols,
                               JZDiagnosticList *diagnostics)
{
    if (!tb) return;

    /* TB-001: module must exist */
    check_tb_module_exists(tb, root, diagnostics);
    check_tb_bus_wire_declarations(tb, root, diagnostics);

    /* TB-012: must contain at least one TEST */
    int test_count = 0;
    for (size_t i = 0; i < tb->child_count; ++i) {
        JZASTNode *child = tb->children[i];
        if (!child) continue;

        if (child->type == JZ_AST_TB_TEST) {
            test_count++;
            check_test_block(child, diagnostics);
            check_test_block_semantics(tb, root, child, project_symbols, diagnostics);
        }
    }

    if (test_count == 0) {
        tb_report_rule(diagnostics, tb->loc, "TB_NO_TEST_BLOCKS",
                       "@testbench has no TEST blocks; add at least one TEST { ... } block\n"
                       "containing a @new instantiation and @setup");
    }
}

int jz_sem_run_testbench(JZASTNode *root,
                         const JZBuffer *project_symbols,
                         JZDiagnosticList *diagnostics)
{
    if (!root) return 0;

    for (size_t i = 0; i < root->child_count; ++i) {
        JZASTNode *child = root->children[i];
        if (!child) continue;

        if (child->type == JZ_AST_TESTBENCH) {
            validate_testbench(child, root, project_symbols, diagnostics);
        } else if (child->type == JZ_AST_SIMULATION) {
            check_simulation_semantics(child, root, diagnostics);
        }
    }

    return 0;
}
