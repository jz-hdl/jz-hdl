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

#include "../../include/ast.h"
#include "../../include/diagnostic.h"
#include "../../include/rules.h"

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
                                       const JZASTNode *test,
                                       JZDiagnosticList *diagnostics)
{
    if (!tb || !test) return;

    for (size_t i = 0; i < test->child_count; ++i) {
        const JZASTNode *child = test->children[i];
        if (!child) continue;

        if (child->type == JZ_AST_TB_SETUP || child->type == JZ_AST_TB_UPDATE) {
            check_stimulus_clock_assignments(tb, child, diagnostics);
        }
    }
}

static void validate_testbench(JZASTNode *tb, JZASTNode *root,
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
            check_test_block_semantics(tb, child, diagnostics);
        }
    }

    if (test_count == 0) {
        tb_report_rule(diagnostics, tb->loc, "TB_NO_TEST_BLOCKS",
                       "@testbench has no TEST blocks; add at least one TEST { ... } block\n"
                       "containing a @new instantiation and @setup");
    }
}

int jz_sem_run_testbench(JZASTNode *root, JZDiagnosticList *diagnostics)
{
    if (!root) return 0;

    for (size_t i = 0; i < root->child_count; ++i) {
        JZASTNode *child = root->children[i];
        if (!child) continue;

        if (child->type == JZ_AST_TESTBENCH) {
            validate_testbench(child, root, diagnostics);
        }
    }

    return 0;
}
