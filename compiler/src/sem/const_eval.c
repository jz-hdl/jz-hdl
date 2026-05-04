/**
 * @file const_eval.c
 * @brief Integer constant-expression evaluation for semantic analysis.
 */

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "sem.h"
#include "util.h"

/* Simple recursive-descent expression parser for integer constant expressions.
 * Grammar (roughly, with standard precedence):
 *
 *   expr        := logical_or
 *   logical_or  := logical_and ( "||" logical_and )*
 *   logical_and := equality ( "&&" equality )*
 *   equality    := relational ( ("==" | "!=") relational )*
 *   relational  := shift ( ("<" | "<=" | ">" | ">=") shift )*
 *   shift       := additive ( ("<<" | ">>" | ">>>") additive )*
 *   additive    := multiplicative ( ("+" | "-") multiplicative )*
 *   multiplicative := unary ( ("*" | "/" | "%") unary )*
 *   unary       := ("+" | "-") unary
 *                | primary
 *   primary     := INTEGER
 *                | IDENT
 *                | "clog2" "(" expr ")"
 *                | "(" expr ")"
 */

/**
 * @enum TokenKind
 * @brief Lexer token kinds for the standalone constant-expression parser.
 */
typedef enum TokenKind {
    TK_EOF = 0, /**< End of input. */
    TK_INT,     /**< Decimal integer literal. */
    TK_IDENT,   /**< Identifier or builtin name. */
    TK_PLUS,    /**< `+`. */
    TK_MINUS,   /**< `-`. */
    TK_STAR,    /**< `*`. */
    TK_SLASH,   /**< `/`. */
    TK_PERCENT, /**< `%`. */
    TK_LPAREN,  /**< `(`. */
    TK_RPAREN,  /**< `)`. */
    TK_EQEQ,    /**< `==`. */
    TK_NEQ,     /**< `!=`. */
    TK_LT,      /**< `<`. */
    TK_LE,      /**< `<=`. */
    TK_GT,      /**< `>`. */
    TK_GE,      /**< `>=`. */
    TK_SHL,     /**< `<<`. */
    TK_SHR,     /**< `>>`. */
    TK_ASHR,    /**< `>>>`. */
    TK_AND_AND, /**< `&&`. */
    TK_OR_OR    /**< `||`. */
} TokenKind;

/**
 * @struct Token
 * @brief One token emitted by the constant-expression lexer.
 */
typedef struct Token {
    TokenKind kind;      /**< Token category. */
    long long int_val;   /**< Integer literal value when `kind == TK_INT`. */
    char      ident[64]; /**< Small fixed-size identifier buffer. */
} Token;

/**
 * @struct Lexer
 * @brief Cursor state for the standalone constant-expression lexer.
 */
typedef struct Lexer {
    const char *p; /**< Current source pointer. */
} Lexer;

static void lex_init(Lexer *lx, const char *src)
{
    lx->p = src ? src : "";
}

static void skip_ws(Lexer *lx)
{
    while (*lx->p && isspace((unsigned char)*lx->p)) {
        lx->p++;
    }
}

static int is_ident_start(int c)
{
    return isalpha(c) || c == '_';
}

static int is_ident_char(int c)
{
    return isalnum(c) || c == '_';
}

static Token lex_one(Lexer *lx)
{
    Token t;
    memset(&t, 0, sizeof(t));

    skip_ws(lx);
    char c = *lx->p;
    if (!c) {
        t.kind = TK_EOF;
        return t;
    }

    if (isdigit((unsigned char)c)) {
        long long val = 0;
        /* Parse a decimal integer, allowing embedded underscores (e.g.,
         * 27_000_000) which are ignored for the numeric value.
         */
        while (isdigit((unsigned char)*lx->p) || *lx->p == '_') {
            if (*lx->p == '_') {
                /* Skip visual separators inside decimal literals. */
                lx->p++;
                continue;
            }
            int d = *lx->p - '0';
            if (val > (LLONG_MAX - d) / 10) {
                /* Clamp on overflow; semantic layer will treat as error. */
                val = LLONG_MAX;
            } else {
                val = val * 10 + d;
            }
            lx->p++;
        }
        t.kind = TK_INT;
        t.int_val = val;
        return t;
    }

    if (is_ident_start((unsigned char)c)) {
        size_t len = 0;
        while (is_ident_char((unsigned char)*lx->p) && len + 1 < sizeof(t.ident)) {
            t.ident[len++] = *lx->p++;
        }
        t.ident[len] = '\0';
        /* Skip any remaining identifier characters beyond our buffer. */
        while (is_ident_char((unsigned char)*lx->p)) {
            lx->p++;
        }
        t.kind = TK_IDENT;
        return t;
    }

    lx->p++;
    switch (c) {
    case '"':
        /* Skip to closing quote — the primary handler will emit a clear error. */
        while (*lx->p && *lx->p != '"') lx->p++;
        if (*lx->p == '"') lx->p++;
        t.kind = TK_IDENT;
        strncpy(t.ident, "__string__", sizeof(t.ident) - 1);
        return t;
    case '+': t.kind = TK_PLUS; break;
    case '-': t.kind = TK_MINUS; break;
    case '*': t.kind = TK_STAR; break;
    case '/': t.kind = TK_SLASH; break;
    case '%': t.kind = TK_PERCENT; break;
    case '(': t.kind = TK_LPAREN; break;
    case ')': t.kind = TK_RPAREN; break;
    case '<':
        if (*lx->p == '<') {
            /* "<<" */
            lx->p++;
            t.kind = TK_SHL;
        } else if (*lx->p == '=') {
            lx->p++;
            t.kind = TK_LE;
        } else {
            t.kind = TK_LT;
        }
        break;
    case '>':
        if (*lx->p == '>') {
            /* Either ">>" or ">>>". */
            lx->p++;
            if (*lx->p == '>') {
                lx->p++;
                t.kind = TK_ASHR;
            } else {
                t.kind = TK_SHR;
            }
        } else if (*lx->p == '=') {
            lx->p++;
            t.kind = TK_GE;
        } else {
            t.kind = TK_GT;
        }
        break;
    case '=':
        if (*lx->p == '=') {
            lx->p++;
            t.kind = TK_EQEQ;
        } else {
            /* Bare '=' isn't expected in expressions; treat as EOF. */
            t.kind = TK_EOF;
        }
        break;
    case '!':
        if (*lx->p == '=') {
            lx->p++;
            t.kind = TK_NEQ;
        } else {
            t.kind = TK_EOF;
        }
        break;
    case '&':
        if (*lx->p == '&') {
            lx->p++;
            t.kind = TK_AND_AND;
        } else {
            t.kind = TK_EOF;
        }
        break;
    case '|':
        if (*lx->p == '|') {
            lx->p++;
            t.kind = TK_OR_OR;
        } else {
            t.kind = TK_EOF;
        }
        break;
    default:
        t.kind = TK_EOF;
        break;
    }

    return t;
}

typedef struct Parser {
    Lexer lx;
    Token cur;
    const JZConstEvalOptions *opts;
    int   error;
    unsigned expr_depth;
} Parser;

static void parser_init(Parser *p, const char *src, const JZConstEvalOptions *opts)
{
    lex_init(&p->lx, src);
    p->cur = lex_one(&p->lx);
    p->opts = opts;
    p->error = 0;
    p->expr_depth = 0;
}

static void parser_diag(Parser *p, const char *msg)
{
    if (p->error) return;
    p->error = 1;
    if (p->opts && p->opts->diagnostics) {
        JZLocation loc;
        loc.filename = p->opts->filename;
        loc.line = 1;
        loc.column = 1;
        jz_diagnostic_report(p->opts->diagnostics, loc, JZ_SEVERITY_ERROR,
                             "CONST001", msg);
    }
}

static void parser_diag_rule(Parser *p, const char *code, const char *msg)
{
    if (p->error) return;
    p->error = 1;
    if (p->opts && p->opts->diagnostics) {
        JZLocation loc;
        loc.filename = p->opts->filename;
        loc.line = 1;
        loc.column = 1;
        (void)jz_diagnostic_report_rule(p->opts->diagnostics, loc, code, msg);
    }
}

static long long eval_clog2_positive(unsigned long long u)
{
    /* Spec §5.5.5 defines clog2(1) as 1 instead of the usual 0. */
    unsigned result = 1;
    unsigned long long v = u - 1;
    while (v > 1) {
        v >>= 1;
        result++;
    }
    return (long long)result;
}

static void advance(Parser *p)
{
    if (p->cur.kind != TK_EOF) {
        p->cur = lex_one(&p->lx);
    }
}

static long long parse_expr(Parser *p);
static long long parse_shift(Parser *p);

static int parser_enter_expr_depth(Parser *p)
{
    if (jz_depth_enter_checked(&p->expr_depth, JZ_LIMIT_CONST_EVAL_DEPTH) != 0) {
        parser_diag_rule(p,
                         "CONST_EVAL_DEPTH_LIMIT_EXCEEDED",
                         "constant expression nesting exceeds the compiler safety limit");
        return -1;
    }
    return 0;
}

static void parser_leave_expr_depth(Parser *p)
{
    jz_depth_leave(&p->expr_depth);
}

static long long parse_primary(Parser *p)
{
    if (p->error) return 0;

    if (p->cur.kind == TK_INT) {
        long long v = p->cur.int_val;
        advance(p);
        return v;
    }

    if (p->cur.kind == TK_IDENT) {
        /* Builtin clog2(x) */
        if (strcmp(p->cur.ident, "clog2") == 0) {
            advance(p);
            if (p->cur.kind != TK_LPAREN) {
                parser_diag(p, "expected '(' after clog2");
                return 0;
            }
            advance(p);
            long long arg = parse_expr(p);
            if (p->error) return 0;
            if (p->cur.kind != TK_RPAREN) {
                parser_diag(p, "expected ')' after clog2 argument");
                return 0;
            }
            advance(p);
            if (arg <= 0) {
                parser_diag(p, "clog2 argument must be positive");
                return 0;
            }
            return eval_clog2_positive((unsigned long long)arg);
        }

        /* For now, identifiers other than clog2 are not resolved here; they
         * will be handled in jz_const_eval_all where we know the environment.
         */
        parser_diag(p, "bare identifiers are not allowed in anonymous expressions");
        return 0;
    }

    if (p->cur.kind == TK_LPAREN) {
        advance(p);
        long long v = parse_expr(p);
        if (p->error) return 0;
        if (p->cur.kind != TK_RPAREN) {
            parser_diag(p, "expected ')' after expression");
            return 0;
        }
        advance(p);
        return v;
    }

    parser_diag(p, "expected primary expression");
    return 0;
}

static long long parse_unary(Parser *p)
{
    if (p->error) return 0;

    if (p->cur.kind == TK_PLUS) {
        advance(p);
        if (parser_enter_expr_depth(p) != 0) return 0;
        long long v = parse_unary(p);
        parser_leave_expr_depth(p);
        return v;
    }
    if (p->cur.kind == TK_MINUS) {
        advance(p);
        if (parser_enter_expr_depth(p) != 0) return 0;
        long long v = parse_unary(p);
        parser_leave_expr_depth(p);
        return -v;
    }
    return parse_primary(p);
}

static long long parse_multiplicative(Parser *p)
{
    long long left = parse_unary(p);
    while (!p->error &&
           (p->cur.kind == TK_STAR || p->cur.kind == TK_SLASH || p->cur.kind == TK_PERCENT)) {
        TokenKind op = p->cur.kind;
        advance(p);
        long long right = parse_unary(p);
        if (p->error) return 0;
        if (op == TK_STAR) {
            left = left * right;
        } else if (op == TK_SLASH) {
            if (right == 0) {
                parser_diag(p, "division by zero in constant expression");
                return 0;
            }
            left = left / right;
        } else if (op == TK_PERCENT) {
            if (right == 0) {
                parser_diag(p, "modulo by zero in constant expression");
                return 0;
            }
            left = left % right;
        }
    }
    return left;
}

static long long parse_additive(Parser *p)
{
    long long left = parse_multiplicative(p);
    while (!p->error && (p->cur.kind == TK_PLUS || p->cur.kind == TK_MINUS)) {
        TokenKind op = p->cur.kind;
        advance(p);
        long long right = parse_multiplicative(p);
        if (p->error) return 0;
        if (op == TK_PLUS) left += right;
        else left -= right;
    }
    return left;
}

static long long parse_shift(Parser *p)
{
    long long left = parse_additive(p);
    while (!p->error &&
           (p->cur.kind == TK_SHL || p->cur.kind == TK_SHR || p->cur.kind == TK_ASHR)) {
        TokenKind op = p->cur.kind;
        advance(p);
        long long right = parse_additive(p);
        if (p->error) return 0;
        if (right < 0) {
            parser_diag(p, "shift amount must be non-negative in constant expression");
            return 0;
        }
        if (right > 63) {
            parser_diag(p, "shift amount too large in constant expression");
            return 0;
        }
        unsigned sh = (unsigned)right;
        if (op == TK_SHL) {
            left = left << sh;
        } else if (op == TK_SHR) {
            left = (long long)((unsigned long long)left >> sh);
        } else { /* TK_ASHR */
            left = left >> sh;
        }
    }
    return left;
}

static long long parse_relational(Parser *p)
{
    long long left = parse_shift(p);
    while (!p->error &&
           (p->cur.kind == TK_LT || p->cur.kind == TK_LE ||
            p->cur.kind == TK_GT || p->cur.kind == TK_GE)) {
        TokenKind op = p->cur.kind;
        advance(p);
        long long right = parse_shift(p);
        if (p->error) return 0;
        long long res = 0;
        switch (op) {
        case TK_LT: res = (left < right); break;
        case TK_LE: res = (left <= right); break;
        case TK_GT: res = (left > right); break;
        case TK_GE: res = (left >= right); break;
        default: break;
        }
        left = res;
    }
    return left;
}

static long long parse_equality(Parser *p)
{
    long long left = parse_relational(p);
    while (!p->error && (p->cur.kind == TK_EQEQ || p->cur.kind == TK_NEQ)) {
        TokenKind op = p->cur.kind;
        advance(p);
        long long right = parse_relational(p);
        if (p->error) return 0;
        long long res = 0;
        if (op == TK_EQEQ) res = (left == right);
        else res = (left != right);
        left = res;
    }
    return left;
}

static long long parse_logical_and(Parser *p)
{
    long long left = parse_equality(p);
    while (!p->error && p->cur.kind == TK_AND_AND) {
        advance(p);
        long long right = parse_equality(p);
        if (p->error) return 0;
        left = (left && right) ? 1 : 0;
    }
    return left;
}

static long long parse_logical_or(Parser *p)
{
    long long left = parse_logical_and(p);
    while (!p->error && p->cur.kind == TK_OR_OR) {
        advance(p);
        long long right = parse_logical_and(p);
        if (p->error) return 0;
        left = (left || right) ? 1 : 0;
    }
    return left;
}

static long long parse_expr(Parser *p)
{
    long long value = 0;
    if (parser_enter_expr_depth(p) != 0) return 0;
    value = parse_logical_or(p);
    parser_leave_expr_depth(p);
    return value;
}

int jz_const_eval_expr(const char *expr,
                       const JZConstEvalOptions *options,
                       long long *out_value)
{
    if (!expr || !out_value) {
        return -1;
    }

    Parser p;
    parser_init(&p, expr, options);
    long long value = parse_expr(&p);

    if (!p.error && p.cur.kind != TK_EOF) {
        parser_diag(&p, "unexpected trailing tokens in constant expression");
    }
    if (p.error) {
        return -1;
    }
    if (value < 0) {
        Parser tmp = p; /* reuse diagnostic machinery */
        parser_diag(&tmp, "constant expression must be non-negative");
        return -1;
    }

    *out_value = value;
    return 0;
}

/* Dependency-tracking evaluator for named CONST/CONFIG entries. */

typedef enum EvalState {
    EVAL_NOT_VISITED = 0,
    EVAL_VISITING,
    EVAL_DONE
} EvalState;

typedef struct EvalEnv {
    const JZConstDef          *defs;
    size_t                     count;
    long long                 *values;
    EvalState                 *state;
    const JZConstEvalOptions  *opts;
} EvalEnv;

static int find_def_index(const EvalEnv *env, const char *name)
{
    if (!name || !*name) return -1;
    for (size_t i = 0; i < env->count; ++i) {
        if (env->defs[i].name && strcmp(env->defs[i].name, name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

/* Variant of the parser that can resolve identifiers through EvalEnv. */

typedef struct EnvParser {
    Lexer   lx;
    Token   cur;
    EvalEnv *env;
    size_t  current_index;
    int     error;
    unsigned expr_depth;
} EnvParser;

static void env_parser_diag(EnvParser *p, const char *msg)
{
    if (p->error) return;
    p->error = 1;
    if (p->env && p->env->opts && p->env->opts->diagnostics) {
        JZLocation loc;
        loc.filename = p->env->opts->filename;
        loc.line = 1;
        loc.column = 1;
        jz_diagnostic_report(p->env->opts->diagnostics, loc, JZ_SEVERITY_ERROR,
                             "CONST002", msg);
    }
}

static void env_parser_diag_rule(EnvParser *p, const char *code, const char *msg)
{
    if (p->error) return;
    p->error = 1;
    if (p->env && p->env->opts && p->env->opts->diagnostics) {
        JZLocation loc;
        loc.filename = p->env->opts->filename;
        loc.line = 1;
        loc.column = 1;
        (void)jz_diagnostic_report_rule(p->env->opts->diagnostics, loc, code, msg);
    }
}

static void env_parser_init(EnvParser *p, const char *src,
                            EvalEnv *env, size_t index)
{
    lex_init(&p->lx, src);
    p->cur = lex_one(&p->lx);
    p->env = env;
    p->current_index = index;
    p->error = 0;
    p->expr_depth = 0;
}

static void env_advance(EnvParser *p)
{
    if (p->cur.kind != TK_EOF) {
        p->cur = lex_one(&p->lx);
    }
}

static int eval_one(EvalEnv *env, size_t index);
static long long env_parse_expr(EnvParser *p);
static long long env_parse_shift(EnvParser *p);

static int env_parser_enter_expr_depth(EnvParser *p)
{
    if (jz_depth_enter_checked(&p->expr_depth, JZ_LIMIT_CONST_EVAL_DEPTH) != 0) {
        env_parser_diag_rule(p,
                             "CONST_EVAL_DEPTH_LIMIT_EXCEEDED",
                             "constant expression nesting exceeds the compiler safety limit");
        return -1;
    }
    return 0;
}

static void env_parser_leave_expr_depth(EnvParser *p)
{
    jz_depth_leave(&p->expr_depth);
}

static long long env_parse_primary(EnvParser *p)
{
    if (p->error) return 0;

    if (p->cur.kind == TK_INT) {
        long long v = p->cur.int_val;
        env_advance(p);
        return v;
    }

    if (p->cur.kind == TK_IDENT) {
        if (strcmp(p->cur.ident, "clog2") == 0) {
            env_advance(p);
            if (p->cur.kind != TK_LPAREN) {
                env_parser_diag(p, "expected '(' after clog2");
                return 0;
            }
            env_advance(p);
            long long arg = env_parse_expr(p);
            if (p->error) return 0;
            if (p->cur.kind != TK_RPAREN) {
                env_parser_diag(p, "expected ')' after clog2 argument");
                return 0;
            }
            env_advance(p);
            if (arg <= 0) {
                env_parser_diag(p, "clog2 argument must be positive");
                return 0;
            }
            return eval_clog2_positive((unsigned long long)arg);
        }

        int idx = find_def_index(p->env, p->cur.ident);
        if (idx < 0) {
            env_parser_diag(p, "unknown constant name in expression");
            return 0;
        }

        if (p->env->state[idx] == EVAL_VISITING) {
            p->error = 2; /* circular dependency — distinct from generic error (1) */
            return 0;
        }
        if (p->env->state[idx] == EVAL_DONE) {
            long long v = p->env->values[idx];
            env_advance(p);
            return v;
        }

        /* Need to evaluate this dependency now. */
        int eval_rc = eval_one(p->env, (size_t)idx);
        if (eval_rc != 0) {
            p->error = (eval_rc == -2) ? 2 : 1;
            return 0;
        }
        env_advance(p);
        return p->env->values[idx];
    }

    if (p->cur.kind == TK_LPAREN) {
        env_advance(p);
        long long v = env_parse_expr(p);
        if (p->error) return 0;
        if (p->cur.kind != TK_RPAREN) {
            env_parser_diag(p, "expected ')' after expression");
            return 0;
        }
        env_advance(p);
        return v;
    }

    env_parser_diag(p, "expected primary expression");
    return 0;
}

static long long env_parse_unary(EnvParser *p)
{
    if (p->error) return 0;
    if (p->cur.kind == TK_PLUS) {
        env_advance(p);
        if (env_parser_enter_expr_depth(p) != 0) return 0;
        long long v = env_parse_unary(p);
        env_parser_leave_expr_depth(p);
        return v;
    }
    if (p->cur.kind == TK_MINUS) {
        env_advance(p);
        if (env_parser_enter_expr_depth(p) != 0) return 0;
        long long v = env_parse_unary(p);
        env_parser_leave_expr_depth(p);
        return -v;
    }
    return env_parse_primary(p);
}

static long long env_parse_multiplicative(EnvParser *p)
{
    long long left = env_parse_unary(p);
    while (!p->error &&
           (p->cur.kind == TK_STAR || p->cur.kind == TK_SLASH || p->cur.kind == TK_PERCENT)) {
        TokenKind op = p->cur.kind;
        env_advance(p);
        long long right = env_parse_unary(p);
        if (p->error) return 0;
        if (op == TK_STAR) {
            left = left * right;
        } else if (op == TK_SLASH) {
            if (right == 0) {
                env_parser_diag(p, "division by zero in constant expression");
                return 0;
            }
            left = left / right;
        } else if (op == TK_PERCENT) {
            if (right == 0) {
                env_parser_diag(p, "modulo by zero in constant expression");
                return 0;
            }
            left = left % right;
        }
    }
    return left;
}

static long long env_parse_additive(EnvParser *p)
{
    long long left = env_parse_multiplicative(p);
    while (!p->error && (p->cur.kind == TK_PLUS || p->cur.kind == TK_MINUS)) {
        TokenKind op = p->cur.kind;
        env_advance(p);
        long long right = env_parse_multiplicative(p);
        if (p->error) return 0;
        if (op == TK_PLUS) left += right;
        else left -= right;
    }
    return left;
}

static long long env_parse_shift(EnvParser *p)
{
    long long left = env_parse_additive(p);
    while (!p->error &&
           (p->cur.kind == TK_SHL || p->cur.kind == TK_SHR || p->cur.kind == TK_ASHR)) {
        TokenKind op = p->cur.kind;
        env_advance(p);
        long long right = env_parse_additive(p);
        if (p->error) return 0;
        if (right < 0) {
            env_parser_diag(p, "shift amount must be non-negative in constant expression");
            return 0;
        }
        if (right > 63) {
            env_parser_diag(p, "shift amount too large in constant expression");
            return 0;
        }
        unsigned sh = (unsigned)right;
        if (op == TK_SHL) {
            left = left << sh;
        } else if (op == TK_SHR) {
            left = (long long)((unsigned long long)left >> sh);
        } else { /* TK_ASHR */
            left = left >> sh;
        }
    }
    return left;
}

static long long env_parse_relational(EnvParser *p)
{
    long long left = env_parse_shift(p);
    while (!p->error &&
           (p->cur.kind == TK_LT || p->cur.kind == TK_LE ||
            p->cur.kind == TK_GT || p->cur.kind == TK_GE)) {
        TokenKind op = p->cur.kind;
        env_advance(p);
        long long right = env_parse_shift(p);
        if (p->error) return 0;
        long long res = 0;
        switch (op) {
        case TK_LT: res = (left < right); break;
        case TK_LE: res = (left <= right); break;
        case TK_GT: res = (left > right); break;
        case TK_GE: res = (left >= right); break;
        default: break;
        }
        left = res;
    }
    return left;
}

static long long env_parse_equality(EnvParser *p)
{
    long long left = env_parse_relational(p);
    while (!p->error && (p->cur.kind == TK_EQEQ || p->cur.kind == TK_NEQ)) {
        TokenKind op = p->cur.kind;
        env_advance(p);
        long long right = env_parse_relational(p);
        if (p->error) return 0;
        long long res = 0;
        if (op == TK_EQEQ) res = (left == right);
        else res = (left != right);
        left = res;
    }
    return left;
}

static long long env_parse_logical_and(EnvParser *p)
{
    long long left = env_parse_equality(p);
    while (!p->error && p->cur.kind == TK_AND_AND) {
        env_advance(p);
        long long right = env_parse_equality(p);
        if (p->error) return 0;
        left = (left && right) ? 1 : 0;
    }
    return left;
}

static long long env_parse_logical_or(EnvParser *p)
{
    long long left = env_parse_logical_and(p);
    while (!p->error && p->cur.kind == TK_OR_OR) {
        env_advance(p);
        long long right = env_parse_logical_and(p);
        if (p->error) return 0;
        left = (left || right) ? 1 : 0;
    }
    return left;
}

static long long env_parse_expr(EnvParser *p)
{
    long long value = 0;
    if (env_parser_enter_expr_depth(p) != 0) return 0;
    value = env_parse_logical_or(p);
    env_parser_leave_expr_depth(p);
    return value;
}

static int eval_one(EvalEnv *env, size_t index)
{
    if (index >= env->count) return -1;
    if (env->state[index] == EVAL_DONE) return 0;
    if (env->state[index] == EVAL_VISITING) {
        if (env->opts && env->opts->diagnostics) {
            JZLocation loc;
            loc.filename = env->opts->filename;
            loc.line = 1;
            loc.column = 1;
            jz_diagnostic_report(env->opts->diagnostics, loc, JZ_SEVERITY_ERROR,
                                 "CONST_CIRCULAR_DEP", "circular dependency in CONST/CONFIG definitions");
        }
        return -2; /* distinct code for circular dependency */
    }

    env->state[index] = EVAL_VISITING;

    const char *expr = env->defs[index].expr ? env->defs[index].expr : "0";
    EnvParser p;
    env_parser_init(&p, expr, env, index);

    long long value = env_parse_expr(&p);
    if (!p.error && p.cur.kind != TK_EOF) {
        env_parser_diag(&p, "unexpected trailing tokens in constant expression");
    }
    if (p.error) {
        env->state[index] = EVAL_NOT_VISITED;
        return (p.error == 2) ? -2 : -1;
    }
    if (value < 0) {
        env_parser_diag(&p, "constant expression must be non-negative");
        env->state[index] = EVAL_NOT_VISITED;
        return -1;
    }

    env->values[index] = value;
    env->state[index] = EVAL_DONE;
    return 0;
}

int jz_const_eval_all(const JZConstDef *defs,
                      size_t count,
                      const JZConstEvalOptions *options,
                      long long *out_values)
{
    if (!defs || !out_values) return -1;
    if (count == 0) return 0;

    EvalState *state = (EvalState *)calloc(count, sizeof(EvalState));
    if (!state) return -1;

    EvalEnv env;
    env.defs = defs;
    env.count = count;
    env.values = out_values;
    env.state = state;
    env.opts = options;

    int result = 0;
    for (size_t i = 0; i < count; ++i) {
        if (state[i] == EVAL_NOT_VISITED) {
            int rc = eval_one(&env, i);
            if (rc != 0) {
                result = rc; /* preserve -2 for circular dep */
                break;
            }
        }
    }

    free(state);
    return result;
}
