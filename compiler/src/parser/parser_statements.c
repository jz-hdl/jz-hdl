/**
 * @file parser_statements.c
 * @brief Parsing of executable statements, lvalues, control flow, and feature guards.
 *
 * This file implements parsing of executable statements inside ASYNCHRONOUS
 * and SYNCHRONOUS blocks, including assignments, IF/ELIF/ELSE chains,
 * SELECT/CASE statements, @feature guards, and statement lists.
 *
 * The parser is intentionally permissive in certain cases to allow semantic
 * analysis to issue precise diagnostics instead of low-level parse errors.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser_internal.h"

static unsigned g_parser_stmt_depth = 0;
static unsigned g_parser_lvalue_depth = 0;

/**
 * @brief Parse the primary identifier form of an assignment lvalue.
 *
 * @param p Active parser
 * @return Lvalue AST node, or NULL on error
 */
static JZASTNode *parse_lvalue_primary(Parser *p);

/**
 * @brief Parse a full assignment lvalue expression.
 *
 * @param p Active parser
 * @return Lvalue AST node, or NULL on error
 */
static JZASTNode *parse_lvalue(Parser *p);

/**
 * @brief Parse an assignment statement inside an executable block.
 *
 * @param p       Active parser
 * @param is_sync Nonzero when parsing a SYNCHRONOUS block
 * @return Assignment AST node, or NULL on error
 */
static JZASTNode *parse_assignment_stmt(Parser *p, int is_sync);

/**
 * @brief Parse an `IF` / `ELIF` / `ELSE` chain.
 *
 * @param p       Active parser
 * @param parent  Parent AST node receiving parsed branch nodes
 * @param is_sync Nonzero when parsing a SYNCHRONOUS block
 * @return 0 on success, -1 on error
 */
static int parse_if_chain(Parser *p, JZASTNode *parent, int is_sync);

/**
 * @brief Parse a `SELECT` statement and its `CASE` branches.
 *
 * @param p       Active parser
 * @param parent  Parent AST node receiving the parsed SELECT node
 * @param is_sync Nonzero when parsing a SYNCHRONOUS block
 * @return 0 on success, -1 on error
 */
static int parse_select_stmt(Parser *p, JZASTNode *parent, int is_sync);

/**
 * @brief Parse the statements inside a feature-guard branch.
 *
 * @param p            Active parser
 * @param parent       Branch AST node receiving parsed statements
 * @param is_sync      Nonzero when parsing a SYNCHRONOUS block
 * @param out_saw_else Output flag set when `@feature_else` is encountered
 * @return 0 on success, -1 on error
 */
static int parse_feature_guard_body(Parser *p,
                                    JZASTNode *parent,
                                    int is_sync,
                                    int *out_saw_else);

/**
 * @brief Parse an executable `@feature` guard statement.
 *
 * @param p       Active parser
 * @param parent  Parent AST node receiving the parsed feature guard
 * @param is_sync Nonzero when parsing a SYNCHRONOUS block
 * @return 0 on success, -1 on error
 */
static int parse_feature_guard_stmt(Parser *p, JZASTNode *parent, int is_sync);

static JZASTNode *parse_lvalue_primary(Parser *p) {
    const JZToken *t = peek(p);
    if (!is_decl_identifier_token(t) && t->type != JZ_TOK_KW_CONFIG) {
        parser_error(p, "expected identifier in assignment left-hand side");
        return NULL;
    }

    /* Reuse the qualified-identifier logic from parse_primary_expr but
     * restricted to identifier-like tokens (IDENTIFIER, CONFIG, or
     * reserved keywords so semantic analysis can report
     * KEYWORD_AS_IDENTIFIER precisely).
     */
    JZLocation loc = t->loc;
    char *buf = NULL;
    size_t buf_sz = 0;
    for (;;) {
        const JZToken *id = peek(p);
        if ((!is_decl_identifier_token(id) && id->type != JZ_TOK_KW_CONFIG) ||
            !id->lexeme) {
            break;
        }
        size_t len = strlen(id->lexeme);
        size_t new_size = 0;
        char *new_buf = NULL;
        if (jz_size_add_checked(buf_sz, len, &new_size) != 0 ||
            jz_size_add_checked(new_size, 2, &new_size) != 0) {
            free(buf);
            return NULL;
        }
        new_buf = (char *)realloc(buf, new_size);
        if (!new_buf) {
            free(buf);
            return NULL;
        }
        buf = new_buf;
        memcpy(buf + buf_sz, id->lexeme, len);
        buf_sz += len;
        buf[buf_sz] = '\0';
        advance(p);
        if (!match(p, JZ_TOK_DOT)) {
            break;
        }
        if (jz_size_add_checked(buf_sz, 2, &new_size) != 0) {
            free(buf);
            return NULL;
        }
        char *new_buf2 = (char *)realloc(buf, new_size);
        if (!new_buf2) {
            free(buf);
            return NULL;
        }
        buf = new_buf2;
        buf[buf_sz++] = '.';
        buf[buf_sz] = '\0';
    }

    if (!buf) {
        parser_error(p, "expected identifier in lvalue");
        return NULL;
    }

    JZASTNodeType type = (strchr(buf, '.') != NULL) ?
        JZ_AST_EXPR_QUALIFIED_IDENTIFIER : JZ_AST_EXPR_IDENTIFIER;
    JZASTNode *id_node = jz_ast_new(type, loc);
    if (!id_node) {
        free(buf);
        return NULL;
    }
    jz_ast_set_name(id_node, buf);
    free(buf);
    return id_node;
}

static JZASTNode *parse_lvalue(Parser *p) {
    if (jz_depth_enter_checked(&g_parser_lvalue_depth,
                               JZ_LIMIT_PARSER_EXPR_DEPTH) != 0) {
        parser_report_rule(p,
                           peek(p),
                           "PARSER_EXPR_DEPTH_LIMIT_EXCEEDED",
                           "lvalue nesting exceeds the parser safety limit");
        return NULL;
    }
    if (match(p, JZ_TOK_LBRACE)) {
        /* Concatenation lvalue: { lhs1, lhs2, ... } */
        JZASTNode *concat = jz_ast_new(JZ_AST_EXPR_CONCAT, peek(p)->loc);
        if (!concat) return NULL;

        if (peek(p)->type != JZ_TOK_RBRACE) {
            for (;;) {
                JZASTNode *elem = parse_lvalue(p);
                if (!elem) {
                    jz_ast_free(concat);
                    return NULL;
                }
                if (jz_ast_add_child(concat, elem) != 0) {
                    jz_ast_free(elem);
                    jz_ast_free(concat);
                    return NULL;
                }
                if (!match(p, JZ_TOK_COMMA)) break;
            }
        }
        if (!match(p, JZ_TOK_RBRACE)) {
            jz_ast_free(concat);
            parser_error(p, "expected '}' at end of concatenation left-hand side");
            jz_depth_leave(&g_parser_lvalue_depth);
            return NULL;
        }
        jz_depth_leave(&g_parser_lvalue_depth);
        return concat;
    }

    JZASTNode *base = parse_lvalue_primary(p);
    if (!base) {
        jz_depth_leave(&g_parser_lvalue_depth);
        return NULL;
    }

    /* Optional slice chains on lvalue. */
    for (;;) {
        if (!match(p, JZ_TOK_LBRACKET)) break;
        int is_wildcard = 0;
        JZASTNode *msb = NULL;
        if (peek(p)->type == JZ_TOK_OP_STAR) {
            is_wildcard = 1;
            advance(p);
        } else {
            msb = parse_simple_index_expr(p);
            if (!msb) {
                jz_ast_free(base);
                jz_depth_leave(&g_parser_lvalue_depth);
                return NULL;
            }
        }

        JZASTNode *lsb = NULL;
        int is_slice = 0;
        if (!is_wildcard && match(p, JZ_TOK_OP_COLON)) {
            /* Full slice [msb:lsb]. */
            is_slice = 1;
            lsb = parse_simple_index_expr(p);
            if (!lsb) {
                jz_ast_free(base);
                jz_ast_free(msb);
                jz_depth_leave(&g_parser_lvalue_depth);
                return NULL;
            }
        }

        if (!match(p, JZ_TOK_RBRACKET)) {
            jz_ast_free(base);
            jz_ast_free(msb);
            if (lsb) jz_ast_free(lsb);
            parser_error(p, "expected ']' after lvalue slice/index");
            jz_depth_leave(&g_parser_lvalue_depth);
            return NULL;
        }

        if (!is_slice && match(p, JZ_TOK_DOT)) {
            const JZToken *member_tok = peek(p);
            if (!is_decl_identifier_token(member_tok) || !member_tok->lexeme) {
                jz_ast_free(base);
                if (msb) jz_ast_free(msb);
                parser_error(p, "expected BUS member identifier after '.'");
                jz_depth_leave(&g_parser_lvalue_depth);
                return NULL;
            }

            if (!base || base->type != JZ_AST_EXPR_IDENTIFIER || !base->name) {
                jz_ast_free(base);
                if (msb) jz_ast_free(msb);
                parser_error(p, "expected BUS port identifier before member access");
                jz_depth_leave(&g_parser_lvalue_depth);
                return NULL;
            }

            JZASTNode *member = jz_ast_new(JZ_AST_EXPR_INDEXED_MEMBER_ACCESS, base->loc);
            if (!member) {
                jz_ast_free(base);
                if (msb) jz_ast_free(msb);
                jz_depth_leave(&g_parser_lvalue_depth);
                return NULL;
            }
            jz_ast_set_name(member, base->name);
            jz_ast_set_text(member, member_tok->lexeme);
            if (is_wildcard) {
                jz_ast_set_block_kind(member, "WILDCARD");
            } else if (msb) {
                if (jz_ast_add_child(member, msb) != 0) {
                    jz_ast_free(member);
                    jz_ast_free(msb);
                    jz_ast_free(base);
                    jz_depth_leave(&g_parser_lvalue_depth);
                    return NULL;
                }
            }
            advance(p); /* consume member identifier */
            jz_ast_free(base);
            base = member;
            continue;
        }

        if (is_wildcard) {
            jz_ast_free(base);
            parser_error(p, "wildcard index is only valid for BUS member access");
            jz_depth_leave(&g_parser_lvalue_depth);
            return NULL;
        }

        if (!is_slice) {
            /* Single index [idx]: treat as [idx:idx] but with two distinct nodes. */
            lsb = jz_ast_new(msb->type, msb->loc);
            if (!lsb) {
                jz_ast_free(base);
                jz_ast_free(msb);
                jz_depth_leave(&g_parser_lvalue_depth);
                return NULL;
            }
            if (msb->type == JZ_AST_EXPR_IDENTIFIER) {
                if (msb->name) jz_ast_set_name(lsb, msb->name);
            } else if (msb->type == JZ_AST_EXPR_LITERAL) {
                if (msb->text) jz_ast_set_text(lsb, msb->text);
            }
        }

        JZASTNode *slice = jz_ast_new(JZ_AST_EXPR_SLICE, base->loc);
        if (!slice) {
            jz_ast_free(base);
            jz_ast_free(msb);
            jz_ast_free(lsb);
            jz_depth_leave(&g_parser_lvalue_depth);
            return NULL;
        }
        if (jz_ast_add_child(slice, base) != 0 ||
            jz_ast_add_child(slice, msb) != 0 ||
            jz_ast_add_child(slice, lsb) != 0) {
            jz_ast_free(slice);
            jz_ast_free(base);
            jz_ast_free(msb);
            jz_ast_free(lsb);
            jz_depth_leave(&g_parser_lvalue_depth);
            return NULL;
        }
        base = slice;
    }

    jz_depth_leave(&g_parser_lvalue_depth);
    return base;
}

static JZASTNode *parse_assignment_stmt(Parser *p, int is_sync) {
    JZLocation loc = peek(p)->loc;
    (void)is_sync;
    JZASTNode *lhs = parse_lvalue(p);
    if (!lhs) return NULL;

    const JZToken *op_tok = peek(p);
    const char *op_kind = NULL;
    switch (op_tok->type) {
    case JZ_TOK_OP_ASSIGN:    op_kind = "ALIAS"; break;
    case JZ_TOK_OP_ASSIGN_Z:  op_kind = "ALIAS_Z"; break;
    case JZ_TOK_OP_ASSIGN_S:  op_kind = "ALIAS_S"; break;
    case JZ_TOK_OP_DRIVE:     op_kind = "DRIVE"; break;
    case JZ_TOK_OP_DRIVE_Z:   op_kind = "DRIVE_Z"; break;
    case JZ_TOK_OP_DRIVE_S:   op_kind = "DRIVE_S"; break;
    case JZ_TOK_OP_LE:        op_kind = "RECEIVE"; break;
    case JZ_TOK_OP_LE_Z:      op_kind = "RECEIVE_Z"; break;
    case JZ_TOK_OP_LE_S:      op_kind = "RECEIVE_S"; break;
    default:
        jz_ast_free(lhs);
        parser_error(p, "expected assignment operator (=, =>, <= and variants)");
        return NULL;
    }
    JZTokenType op_type = op_tok->type;
    advance(p); /* consume operator */

    /* Parse the first RHS expression. For most assignments this is the full
     * RHS; for guarded latch assignments in ASYNCHRONOUS blocks it is the
     * enable expression that precedes ':'.
     */
    JZASTNode *first_rhs = parse_expression(p);
    if (!first_rhs) {
        jz_ast_free(lhs);
        return NULL;
    }

    JZASTNode *rhs = first_rhs;

    /* LATCH guarded assignment syntax (Section 4.8):
     *   <latch_name> <= <enable_expr> : <data_expr>;
     * is represented syntactically so semantic analysis can enforce placement
     * rules. We represent this as an assignment with children:
     *   [0] = lhs (latch), [1] = data_expr, [2] = enable_expr,
     * and a block_kind that still begins with "RECEIVE" so existing
     * directional semantics continue to apply.
     */
    int is_plain_le = (op_type == JZ_TOK_OP_LE);
    JZASTNode *enable_expr = NULL;
    if (is_plain_le && peek(p)->type == JZ_TOK_OP_COLON) {
        /* Treat first_rhs as the enable expression and parse data after ':'. */
        enable_expr = first_rhs;
        advance(p); /* consume ':' */
        rhs = parse_expression(p); /* data expression */
        if (!rhs) {
            jz_ast_free(lhs);
            jz_ast_free(enable_expr);
            return NULL;
        }
        /* Refine op_kind so semantic passes can distinguish latch guards while
         * still recognizing this as a RECEIVE-family operator.
         */
        if (op_kind && strcmp(op_kind, "RECEIVE") == 0) {
            op_kind = "RECEIVE_LATCH";
        }
    }

    if (!match(p, JZ_TOK_SEMICOLON)) {
        jz_ast_free(lhs);
        jz_ast_free(rhs);
        if (enable_expr) jz_ast_free(enable_expr);
        parser_error(p, "expected ';' after assignment");
        return NULL;
    }

    JZASTNode *stmt = jz_ast_new(JZ_AST_STMT_ASSIGN, loc);
    if (!stmt) {
        jz_ast_free(lhs);
        jz_ast_free(rhs);
        if (enable_expr) jz_ast_free(enable_expr);
        return NULL;
    }
    if (op_kind) jz_ast_set_block_kind(stmt, op_kind);
    if (jz_ast_add_child(stmt, lhs) != 0 ||
        jz_ast_add_child(stmt, rhs) != 0) {
        jz_ast_free(stmt);
        jz_ast_free(lhs);
        jz_ast_free(rhs);
        if (enable_expr) jz_ast_free(enable_expr);
        return NULL;
    }
    if (enable_expr) {
        if (jz_ast_add_child(stmt, enable_expr) != 0) {
            jz_ast_free(stmt);
            jz_ast_free(enable_expr);
            return NULL;
        }
    }
    return stmt;
}

static int parse_if_chain(Parser *p, JZASTNode *parent, int is_sync) {
    (void)is_sync;

    const JZToken *if_tok = peek(p);
    advance(p); /* consume IF */

    if (!match(p, JZ_TOK_LPAREN)) {
        parser_report_rule(p,
                           if_tok,
                           "IF_COND_MISSING_PARENS",
                           "expected '(' after IF; condition must be enclosed in parentheses\n"
                           "like: IF (expr) { ... }");
        return -1;
    }
    JZASTNode *cond = parse_expression(p);
    if (!cond) return -1;
    if (!match(p, JZ_TOK_RPAREN)) {
        jz_ast_free(cond);
        parser_report_rule(p,
                           if_tok,
                           "IF_COND_MISSING_PARENS",
                           "expected ')' after IF condition; the condition must be enclosed\n"
                           "in parentheses like: IF (expr) { ... }");
        return -1;
    }
    if (!match(p, JZ_TOK_LBRACE)) {
        jz_ast_free(cond);
        parser_error(p, "expected '{' to begin IF body");
        return -1;
    }

    JZASTNode *if_node = jz_ast_new(JZ_AST_STMT_IF, if_tok->loc);
    if (!if_node) {
        jz_ast_free(cond);
        return -1;
    }
    if (jz_ast_add_child(if_node, cond) != 0) {
        jz_ast_free(if_node);
        jz_ast_free(cond);
        return -1;
    }
    if (parse_statement_list(p, if_node, JZ_TOK_RBRACE, is_sync) != 0) {
        jz_ast_free(if_node);
        return -1;
    }
    if (jz_ast_add_child(parent, if_node) != 0) {
        jz_ast_free(if_node);
        return -1;
    }

    /* Zero or more ELIF, optional ELSE. */
    for (;;) {
        const JZToken *t = peek(p);
        if (t->type == JZ_TOK_KW_ELIF) {
            const JZToken *elif_tok = t;
            advance(p);
            if (!match(p, JZ_TOK_LPAREN)) {
                parser_report_rule(p,
                                   elif_tok,
                                   "IF_COND_MISSING_PARENS",
                                   "expected '(' after ELIF; condition must be enclosed in parentheses\n"
                                   "like: ELIF (expr) { ... }");
                return -1;
            }
            JZASTNode *elif_cond = parse_expression(p);
            if (!elif_cond) return -1;
            if (!match(p, JZ_TOK_RPAREN)) {
                jz_ast_free(elif_cond);
                parser_report_rule(p,
                                   elif_tok,
                                   "IF_COND_MISSING_PARENS",
                                   "expected ')' after ELIF condition; the condition must be enclosed\n"
                                   "in parentheses like: ELIF (expr) { ... }");
                return -1;
            }
            if (!match(p, JZ_TOK_LBRACE)) {
                jz_ast_free(elif_cond);
                parser_error(p, "expected '{' to begin ELIF body");
                return -1;
            }
            JZASTNode *elif_node = jz_ast_new(JZ_AST_STMT_ELIF, t->loc);
            if (!elif_node) {
                jz_ast_free(elif_cond);
                return -1;
            }
            if (jz_ast_add_child(elif_node, elif_cond) != 0) {
                jz_ast_free(elif_node);
                jz_ast_free(elif_cond);
                return -1;
            }
            if (parse_statement_list(p, elif_node, JZ_TOK_RBRACE, is_sync) != 0) {
                jz_ast_free(elif_node);
                return -1;
            }
            if (jz_ast_add_child(parent, elif_node) != 0) {
                jz_ast_free(elif_node);
                return -1;
            }
            continue;
        } else if (t->type == JZ_TOK_KW_ELSE) {
            advance(p);
            if (!match(p, JZ_TOK_LBRACE)) {
                parser_error(p, "expected '{' to begin ELSE body");
                return -1;
            }
            JZASTNode *else_node = jz_ast_new(JZ_AST_STMT_ELSE, t->loc);
            if (!else_node) return -1;
            if (parse_statement_list(p, else_node, JZ_TOK_RBRACE, is_sync) != 0) {
                jz_ast_free(else_node);
                return -1;
            }
            if (jz_ast_add_child(parent, else_node) != 0) {
                jz_ast_free(else_node);
                return -1;
            }
            break; /* ELSE must be last */
        } else {
            break;
        }
    }

    return 0;
}

static int parse_select_stmt(Parser *p, JZASTNode *parent, int is_sync) {
    (void)is_sync;

    const JZToken *sel_tok = peek(p);
    advance(p); /* SELECT */

    if (!match(p, JZ_TOK_LPAREN)) {
        parser_error(p, "expected '(' after SELECT");
        return -1;
    }
    JZASTNode *expr = parse_expression(p);
    if (!expr) return -1;
    if (!match(p, JZ_TOK_RPAREN)) {
        jz_ast_free(expr);
        parser_error(p, "expected ')' after SELECT expression");
        return -1;
    }
    if (!match(p, JZ_TOK_LBRACE)) {
        jz_ast_free(expr);
        parser_error(p, "expected '{' to begin SELECT body");
        return -1;
    }

    JZASTNode *sel = jz_ast_new(JZ_AST_STMT_SELECT, sel_tok->loc);
    if (!sel) {
        jz_ast_free(expr);
        return -1;
    }
    if (jz_ast_add_child(sel, expr) != 0) {
        jz_ast_free(sel);
        jz_ast_free(expr);
        return -1;
    }

    int saw_default = 0;
    for (;;) {
        const JZToken *t = peek(p);
        if (t->type == JZ_TOK_RBRACE) {
            advance(p);
            break;
        }
        if (t->type == JZ_TOK_EOF) {
            jz_ast_free(sel);
            parser_error(p, "unterminated SELECT block (missing '}' )");
            return -1;
        }
        if (t->type == JZ_TOK_SEMICOLON) {
            advance(p);
            continue;
        }

        if (t->type == JZ_TOK_KW_CASE) {
            advance(p);
            JZASTNode *val = parse_expression(p);
            if (!val) {
                jz_ast_free(sel);
                return -1;
            }
            JZASTNode *case_node = jz_ast_new(JZ_AST_STMT_CASE, t->loc);
            if (!case_node) {
                jz_ast_free(sel);
                jz_ast_free(val);
                return -1;
            }
            if (jz_ast_add_child(case_node, val) != 0) {
                jz_ast_free(sel);
                jz_ast_free(case_node);
                jz_ast_free(val);
                return -1;
            }

            if (match(p, JZ_TOK_LBRACE)) {
                if (parse_statement_list(p, case_node, JZ_TOK_RBRACE, is_sync) != 0) {
                    jz_ast_free(sel);
                    jz_ast_free(case_node);
                    return -1;
                }
            } else {
                /* Naked CASE label: optional ';' and fall-through to next case/default. */
                match(p, JZ_TOK_SEMICOLON);
            }

            if (jz_ast_add_child(sel, case_node) != 0) {
                jz_ast_free(sel);
                jz_ast_free(case_node);
                return -1;
            }
            continue;
        }

        if (t->type == JZ_TOK_KW_DEFAULT) {
            if (saw_default) {
                jz_ast_free(sel);
                parser_error(p, "multiple DEFAULT labels in SELECT block");
                return -1;
            }
            saw_default = 1;
            advance(p);
            if (!match(p, JZ_TOK_LBRACE)) {
                jz_ast_free(sel);
                parser_error(p, "expected '{' to begin DEFAULT body");
                return -1;
            }
            JZASTNode *def = jz_ast_new(JZ_AST_STMT_DEFAULT, t->loc);
            if (!def) {
                jz_ast_free(sel);
                return -1;
            }
            if (parse_statement_list(p, def, JZ_TOK_RBRACE, is_sync) != 0) {
                jz_ast_free(sel);
                jz_ast_free(def);
                return -1;
            }
            if (jz_ast_add_child(sel, def) != 0) {
                jz_ast_free(sel);
                jz_ast_free(def);
                return -1;
            }
            continue;
        }

        parser_error(p, "expected CASE or DEFAULT inside SELECT block");
        jz_ast_free(sel);
        return -1;
    }

    if (jz_ast_add_child(parent, sel) != 0) {
        jz_ast_free(sel);
        return -1;
    }
    return 0;
}

static int parse_feature_guard_body(Parser *p,
                                      JZASTNode *parent,
                                      int is_sync,
                                      int *out_saw_else)
{
    if (out_saw_else) {
        *out_saw_else = 0;
    }

    for (;;) {
        const JZToken *t = peek(p);
        if (t->type == JZ_TOK_KW_FEATURE_ELSE) {
            if (out_saw_else) {
                *out_saw_else = 1;
            }
            return 0; /* caller will consume @else */
        }
        if (t->type == JZ_TOK_KW_ENDFEAT) {
            return 0; /* caller will consume @endfeat */
        }
        if (t->type == JZ_TOK_EOF || t->type == JZ_TOK_RBRACE) {
            parser_error(p, "unterminated @feature block (missing @endfeat)");
            return -1;
        }
        if (t->type == JZ_TOK_SEMICOLON) {
            advance(p);
            continue; /* empty statement */
        }
        if (t->type == JZ_TOK_KW_FEATURE) {
            /* Nested feature guards are not allowed by the spec. */
            parser_report_rule(p,
                               t,
                               "FEATURE_NESTED",
                               "@feature cannot be nested inside another @feature guard;\n"
                               "use a single @feature with a combined condition instead");
            advance(p);
            return -1;
        }
        if (t->type == JZ_TOK_KW_CHECK) {
            /* @check is not allowed inside @feature guarded bodies. */
            parser_report_rule(p,
                               t,
                               "DIRECTIVE_INVALID_CONTEXT",
                               "@check is not allowed inside a @feature guard body;\n"
                               "move it to module or project scope");
            advance(p);
            return -1;
        }
        if (t->type == JZ_TOK_KW_NEW ||
            t->type == JZ_TOK_KW_PROJECT ||
            t->type == JZ_TOK_KW_ENDPROJ ||
            t->type == JZ_TOK_KW_MODULE ||
            t->type == JZ_TOK_KW_ENDMOD ||
            t->type == JZ_TOK_KW_BLACKBOX ||
            t->type == JZ_TOK_KW_IMPORT ||
            t->type == JZ_TOK_KW_GLOBAL ||
            t->type == JZ_TOK_KW_ENDGLOB) {
            /* Structural directives are not allowed inside @feature bodies. */
            parser_report_rule(p, t,
                               "DIRECTIVE_INVALID_CONTEXT",
                               "structural directive is not allowed inside a @feature guard body;\n"
                               "move @new and other structural directives to module scope");
            advance(p);
            return -1;
        }

        if (t->type == JZ_TOK_KW_IF) {
            if (parse_if_chain(p, parent, is_sync) != 0) {
                return -1;
            }
            continue;
        }
        if (t->type == JZ_TOK_KW_SELECT) {
            if (parse_select_stmt(p, parent, is_sync) != 0) {
                return -1;
            }
            continue;
        }
        if (t->type == JZ_TOK_KW_CASE || t->type == JZ_TOK_KW_DEFAULT) {
            parser_report_rule(p,
                               t,
                               "CONTROL_FLOW_OUTSIDE_BLOCK",
                               "CASE/DEFAULT must appear inside a SELECT (...) { ... } block;\n"
                               "they cannot be used standalone in a @feature body");
            advance(p);
            return -1;
        }

        JZASTNode *stmt = parse_assignment_stmt(p, is_sync);
        if (!stmt) return -1;
        if (jz_ast_add_child(parent, stmt) != 0) {
            jz_ast_free(stmt);
            return -1;
        }
    }
}

static int parse_feature_guard_stmt(Parser *p, JZASTNode *parent, int is_sync)
{
    const JZToken *feat_tok = peek(p);
    advance(p); /* consume @feature */

    /* Parse the feature condition expression. */
    JZASTNode *cond = parse_expression(p);
    if (!cond) {
        return -1;
    }

    JZASTNode *guard = jz_ast_new(JZ_AST_FEATURE_GUARD, feat_tok->loc);
    if (!guard) {
        jz_ast_free(cond);
        return -1;
    }
    jz_ast_set_block_kind(guard, is_sync ? "FEATURE_SYNC" : "FEATURE_ASYNC");
    if (jz_ast_add_child(guard, cond) != 0) {
        jz_ast_free(cond);
        jz_ast_free(guard);
        return -1;
    }

    /* THEN body */
    JZASTNode *then_block = jz_ast_new(JZ_AST_BLOCK, feat_tok->loc);
    if (!then_block) {
        jz_ast_free(guard);
        return -1;
    }
    jz_ast_set_block_kind(then_block, "FEATURE_THEN");

    int saw_else = 0;
    if (parse_feature_guard_body(p, then_block, is_sync, &saw_else) != 0) {
        jz_ast_free(then_block);
        jz_ast_free(guard);
        return -1;
    }
    if (jz_ast_add_child(guard, then_block) != 0) {
        jz_ast_free(then_block);
        jz_ast_free(guard);
        return -1;
    }

    /* Optional @else body */
    if (saw_else) {
        const JZToken *else_tok = peek(p);
        advance(p); /* consume @else */

        JZASTNode *else_block = jz_ast_new(JZ_AST_BLOCK, else_tok->loc);
        if (!else_block) {
            jz_ast_free(guard);
            return -1;
        }
        jz_ast_set_block_kind(else_block, "FEATURE_ELSE");

        int dummy = 0;
        if (parse_feature_guard_body(p, else_block, is_sync, &dummy) != 0) {
            jz_ast_free(else_block);
            jz_ast_free(guard);
            return -1;
        }
        if (jz_ast_add_child(guard, else_block) != 0) {
            jz_ast_free(else_block);
            jz_ast_free(guard);
            return -1;
        }
    }

    /* Expect closing @endfeat */
    if (peek(p)->type != JZ_TOK_KW_ENDFEAT) {
        parser_error(p, "missing @endfeat for @feature block");
        jz_ast_free(guard);
        return -1;
    }
    advance(p); /* consume @endfeat */

    if (jz_ast_add_child(parent, guard) != 0) {
        jz_ast_free(guard);
        return -1;
    }

    return 0;
}

int parse_feature_guard_in_block(Parser *p, JZASTNode *parent,
                                  int (*body_fn)(Parser *p, JZASTNode *parent))
{
    if (jz_depth_enter_checked(&g_parser_stmt_depth,
                               JZ_LIMIT_PARSER_STATEMENT_DEPTH) != 0) {
        parser_report_rule(p,
                           peek(p),
                           "PARSER_STMT_DEPTH_LIMIT_EXCEEDED",
                           "feature nesting exceeds the parser safety limit");
        return -1;
    }
    const JZToken *feat_tok = peek(p);
    advance(p); /* consume @feature */

    /* Parse the feature condition expression. */
    JZASTNode *cond = parse_expression(p);
    if (!cond) {
        jz_depth_leave(&g_parser_stmt_depth);
        return -1;
    }

    JZASTNode *guard = jz_ast_new(JZ_AST_FEATURE_GUARD, feat_tok->loc);
    if (!guard) {
        jz_ast_free(cond);
        jz_depth_leave(&g_parser_stmt_depth);
        return -1;
    }
    jz_ast_set_block_kind(guard, "FEATURE_DECL");
    if (jz_ast_add_child(guard, cond) != 0) {
        jz_ast_free(cond);
        jz_ast_free(guard);
        jz_depth_leave(&g_parser_stmt_depth);
        return -1;
    }

    /* THEN body */
    JZASTNode *then_block = jz_ast_new(JZ_AST_BLOCK, feat_tok->loc);
    if (!then_block) {
        jz_ast_free(guard);
        jz_depth_leave(&g_parser_stmt_depth);
        return -1;
    }
    jz_ast_set_block_kind(then_block, "FEATURE_THEN");

    if (body_fn(p, then_block) != 0) {
        jz_ast_free(then_block);
        jz_ast_free(guard);
        jz_depth_leave(&g_parser_stmt_depth);
        return -1;
    }
    if (jz_ast_add_child(guard, then_block) != 0) {
        jz_ast_free(then_block);
        jz_ast_free(guard);
        jz_depth_leave(&g_parser_stmt_depth);
        return -1;
    }

    /* Optional @else body */
    if (peek(p)->type == JZ_TOK_KW_FEATURE_ELSE) {
        const JZToken *else_tok = peek(p);
        advance(p); /* consume @else */

        JZASTNode *else_block = jz_ast_new(JZ_AST_BLOCK, else_tok->loc);
        if (!else_block) {
            jz_ast_free(guard);
            jz_depth_leave(&g_parser_stmt_depth);
            return -1;
        }
        jz_ast_set_block_kind(else_block, "FEATURE_ELSE");

        if (body_fn(p, else_block) != 0) {
            jz_ast_free(else_block);
            jz_ast_free(guard);
            jz_depth_leave(&g_parser_stmt_depth);
            return -1;
        }
        if (jz_ast_add_child(guard, else_block) != 0) {
            jz_ast_free(else_block);
            jz_ast_free(guard);
            jz_depth_leave(&g_parser_stmt_depth);
            return -1;
        }
    }

    /* Expect closing @endfeat */
    if (peek(p)->type != JZ_TOK_KW_ENDFEAT) {
        parser_error(p, "missing @endfeat for @feature block");
        jz_ast_free(guard);
        jz_depth_leave(&g_parser_stmt_depth);
        return -1;
    }
    advance(p); /* consume @endfeat */

    if (jz_ast_add_child(parent, guard) != 0) {
        jz_ast_free(guard);
        jz_depth_leave(&g_parser_stmt_depth);
        return -1;
    }

    jz_depth_leave(&g_parser_stmt_depth);
    return 0;
}

int parse_statement_list(Parser *p, JZASTNode *parent, JZTokenType terminator, int is_sync) {
    if (jz_depth_enter_checked(&g_parser_stmt_depth, JZ_LIMIT_PARSER_STATEMENT_DEPTH) != 0) {
        parser_report_rule(p,
                           peek(p),
                           "PARSER_STMT_DEPTH_LIMIT_EXCEEDED",
                           "statement nesting exceeds the parser safety limit");
        return -1;
    }

    for (;;) {
        const JZToken *t = peek(p);
        if (t->type == terminator) {
            advance(p); /* consume terminator */
            break;
        }
        if (t->type == JZ_TOK_EOF) {
            parser_error(p, "unterminated block (missing closing terminator)");
            goto fail;
        }
        if (t->type == JZ_TOK_SEMICOLON) {
            advance(p);
            continue; /* empty statement */
        }

        if (t->type == JZ_TOK_KW_IF) {
            if (parse_if_chain(p, parent, is_sync) != 0) {
                goto fail;
            }
            continue;
        }
        if (t->type == JZ_TOK_KW_SELECT) {
            if (parse_select_stmt(p, parent, is_sync) != 0) {
                goto fail;
            }
            continue;
        }
        if (t->type == JZ_TOK_KW_CASE || t->type == JZ_TOK_KW_DEFAULT) {
            /* CASE or DEFAULT appearing here means it is not nested inside a
             * SELECT block body.
             */
            parser_report_rule(p,
                               t,
                               "CONTROL_FLOW_OUTSIDE_BLOCK",
                               "CASE/DEFAULT must appear inside a SELECT (...) { ... } block;\n"
                               "they cannot be used outside of a SELECT statement");
            advance(p);
            goto fail;
        }
        if (t->type == JZ_TOK_KW_APPLY) {
            advance(p);
            JZASTNode *apply = parse_apply_stmt(p);
            if (!apply) goto fail;
            if (jz_ast_add_child(parent, apply) != 0) {
                jz_ast_free(apply);
                goto fail;
            }
            continue;
        }
        if (t->type == JZ_TOK_KW_SCRATCH) {
            /* @scratch outside template body. */
            parser_report_rule(p, t, "TEMPLATE_SCRATCH_OUTSIDE",
                               "@scratch found inside an executable block; @scratch declares temporary\n"
                               "wires and may only be used inside a @template body");
            /* Skip past the semicolon to recover */
            advance(p);
            while (peek(p)->type != JZ_TOK_EOF &&
                   peek(p)->type != JZ_TOK_SEMICOLON &&
                   peek(p)->type != terminator) {
                advance(p);
            }
            if (peek(p)->type == JZ_TOK_SEMICOLON) advance(p);
            continue;
        }
        if (t->type == JZ_TOK_KW_FEATURE) {
            if (parse_feature_guard_stmt(p, parent, is_sync) != 0) {
                goto fail;
            }
            continue;
        }
        if (t->type == JZ_TOK_KW_FEATURE_ELSE || t->type == JZ_TOK_KW_ENDFEAT) {
            /* @else or @endfeat without matching @feature in this block. */
            parser_error(p, "unexpected feature directive without matching @feature");
            advance(p);
            goto fail;
        }
        if (t->type == JZ_TOK_KW_CHECK) {
            /* @check is not allowed inside ASYNCHRONOUS/SYNCHRONOUS blocks. */
            parser_report_rule(p,
                               t,
                               "DIRECTIVE_INVALID_CONTEXT",
                               "@check is not allowed inside ASYNCHRONOUS/SYNCHRONOUS blocks;\n"
                               "move it to module scope (directly inside @module...@endmod)");
            /* Skip past the semicolon to avoid cascading parse errors from
             * the @check argument list (e.g. "(1, \"msg\");"). */
            advance(p);
            while (peek(p)->type != JZ_TOK_EOF &&
                   peek(p)->type != JZ_TOK_SEMICOLON &&
                   peek(p)->type != terminator) {
                advance(p);
            }
            if (peek(p)->type == JZ_TOK_SEMICOLON) advance(p);
            continue;
        }
        if (t->type == JZ_TOK_KW_NEW ||
            t->type == JZ_TOK_KW_PROJECT ||
            t->type == JZ_TOK_KW_ENDPROJ ||
            t->type == JZ_TOK_KW_MODULE ||
            t->type == JZ_TOK_KW_ENDMOD ||
            t->type == JZ_TOK_KW_BLACKBOX ||
            t->type == JZ_TOK_KW_IMPORT ||
            t->type == JZ_TOK_KW_GLOBAL ||
            t->type == JZ_TOK_KW_ENDGLOB) {
            /* Structural directives are not allowed inside blocks. */
            parser_report_rule(p, t,
                               "DIRECTIVE_INVALID_CONTEXT",
                               "structural directive is not allowed inside ASYNCHRONOUS/SYNCHRONOUS blocks;\n"
                               "move @new and other structural directives to module scope");
            JZTokenType kw = t->type;
            advance(p);
            /* @new has "name Type { ... };" syntax; skip past its trailing
             * semicolon (respecting brace nesting) to avoid cascading parse
             * errors. Other structural directives are single tokens or
             * terminators that the main parser recovers from naturally. */
            if (kw == JZ_TOK_KW_NEW) {
                int depth = 0;
                while (peek(p)->type != JZ_TOK_EOF) {
                    if (depth == 0 && peek(p)->type == terminator) {
                        break;
                    }
                    if (peek(p)->type == JZ_TOK_LBRACE) {
                        depth++;
                    } else if (peek(p)->type == JZ_TOK_RBRACE) {
                        depth--;
                    } else if (peek(p)->type == JZ_TOK_SEMICOLON && depth <= 0) {
                        advance(p);
                        break;
                    }
                    advance(p);
                }
            }
            continue;
        }

        /* Fallback: assignment statement. */
        JZASTNode *stmt = parse_assignment_stmt(p, is_sync);
        if (!stmt) goto fail;
        if (jz_ast_add_child(parent, stmt) != 0) {
            jz_ast_free(stmt);
            goto fail;
        }
    }

    jz_depth_leave(&g_parser_stmt_depth);
    return 0;

fail:
    jz_depth_leave(&g_parser_stmt_depth);
    return -1;
}
