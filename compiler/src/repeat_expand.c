/**
 * @file repeat_expand.c
 * @brief Pre-parser @repeat expansion for JZ-HDL.
 *
 * Expands @repeat N ... @end blocks in raw source text before lexing.
 * The body between @repeat N and @end is duplicated N times, with
 * each standalone occurrence of IDX replaced by the iteration index
 * (0 through N-1). Nesting is supported.
 *
 * This runs on raw text before lexing, so it works in any context:
 * @testbench, @simulation, or any future construct.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "../include/repeat_expand.h"
#include "../include/diagnostic.h"
#include "../include/util.h"

/**
 * @struct StrBuf
 * @brief Growable string buffer used during raw-text expansion.
 */
typedef struct {
    char  *data;  /**< Heap-allocated buffer contents. */
    size_t len;   /**< Number of bytes currently written, excluding the terminator. */
    size_t cap;   /**< Allocated capacity in bytes. */
} StrBuf;

/**
 * @struct RepeatExpandContext
 * @brief Shared expansion state for diagnostics and hard limits.
 */
typedef struct {
    const char        *full_src;    /**< Full unexpanded source text. */
    const char        *filename;    /**< Diagnostic filename associated with the source. */
    JZDiagnosticList  *diagnostics; /**< Diagnostic sink for expansion failures. */
    JZExpansionLimits  limits;      /**< Effective expansion limits in force. */
} RepeatExpandContext;

/**
 * @struct RepeatFrame
 * @brief Stack frame used while iteratively expanding nested repeat regions.
 */
typedef struct {
    const char *region_end;        /**< Exclusive end pointer of the current region. */
    const char *p;                 /**< Current scan position within the region. */
    StrBuf      nested_out;        /**< Owned output buffer for nested expansion results. */
    StrBuf     *out;               /**< Buffer that receives output for this frame. */
    int         owns_out;          /**< Non-zero when `nested_out` storage is owned here. */
    int         pending_repeat;    /**< Non-zero when a child repeat body is being accumulated. */
    size_t      pending_count;     /**< Iteration count for the pending repeat. */
    const char *pending_repeat_at; /**< Pointer to the pending @repeat directive. */
    const char *pending_end_at;    /**< Pointer to the matching @end directive. */
} RepeatFrame;

/**
 * @struct RepeatFrameStack
 * @brief Dynamic stack of active repeat-expansion frames.
 */
typedef struct {
    RepeatFrame *data; /**< Stack storage. */
    size_t       len;  /**< Number of active frames. */
    size_t       cap;  /**< Allocated frame capacity. */
} RepeatFrameStack;

static int count_line(const char *src, const char *pos);

static void sb_init(StrBuf *sb)
{
    sb->cap = 4096;
    sb->data = malloc(sb->cap);
    sb->len = 0;
    if (sb->data) {
        sb->data[0] = '\0';
    }
}

static void report_repeat_rule(RepeatExpandContext *ctx,
                               const char *at,
                               const char *rule_id,
                               const char *message)
{
    JZLocation loc;

    if (!ctx || !ctx->diagnostics || !rule_id || !message) return;

    loc.filename = ctx->filename;
    loc.line = count_line(ctx->full_src, at ? at : ctx->full_src);
    loc.column = 1;

    jz_diagnostic_report(ctx->diagnostics, loc, JZ_SEVERITY_ERROR, rule_id, message);
}

static void report_repeat_internal_failure(RepeatExpandContext *ctx,
                                           const char *at,
                                           const char *message)
{
    report_repeat_rule(ctx, at, "RPT_INTERNAL", message);
}

static int sb_append_limited(StrBuf *sb,
                             const char *s,
                             size_t n,
                             RepeatExpandContext *ctx,
                             const char *limit_at)
{
    size_t needed = 0;

    if (n == 0) return 0;
    if (!sb || !ctx) return 1;

    if (sb->len > ctx->limits.repeat_max_bytes ||
        n > ctx->limits.repeat_max_bytes - sb->len) {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "@repeat expansion exceeds the configured expanded-size limit of %zu byte(s)",
                 ctx->limits.repeat_max_bytes);
        report_repeat_rule(ctx, limit_at, "RPT_EXPANDED_SIZE_LIMIT_EXCEEDED", msg);
        return 1;
    }

    if (sb->len > SIZE_MAX - n - 1) {
        report_repeat_internal_failure(ctx, limit_at,
                                       "internal error: @repeat expansion size overflow");
        return 1;
    }

    if (jz_size_add_checked(sb->len, n, &needed) != 0 ||
        jz_size_add_checked(needed, 1, &needed) != 0) {
        report_repeat_internal_failure(ctx, limit_at,
                                       "internal error: @repeat expansion size overflow");
        return 1;
    }
    while (needed > sb->cap) {
        size_t new_cap = 0;
        char *new_data = NULL;
        if (jz_size_grow_doubling_checked(sb->cap, needed, 4096, &new_cap) != 0) {
            report_repeat_internal_failure(ctx, limit_at,
                                           "internal error: @repeat expansion size overflow");
            return 1;
        }

        new_data = realloc(sb->data, new_cap);
        if (!new_data) {
            report_repeat_internal_failure(ctx, limit_at,
                                           "out of memory during @repeat expansion");
            return 1;
        }
        sb->data = new_data;
        sb->cap = new_cap;
    }

    memcpy(sb->data + sb->len, s, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
    return 0;
}


/* ── Helpers ────────────────────────────────────────────────────── */

/* Check if character is a word boundary (not alphanumeric or underscore). */
static int is_word_char(char c)
{
    return isalnum((unsigned char)c) || c == '_';
}

static int has_remaining(const char *p, const char *end, size_t need)
{
    return p && end && p <= end && (size_t)(end - p) >= need;
}

static int matches_directive(const char *p, const char *end,
                             const char *directive, size_t directive_len)
{
    if (!has_remaining(p, end, directive_len)) {
        return 0;
    }
    return strncmp(p, directive, directive_len) == 0;
}

static int directive_boundary_ok(const char *p, const char *end, size_t directive_len)
{
    if (!has_remaining(p, end, directive_len + 1)) {
        return 1;
    }
    return !is_word_char(p[directive_len]);
}

/* Count the line number at position `pos` within `src`. */
static int count_line(const char *src, const char *pos)
{
    int line = 1;
    for (const char *p = src; p < pos; p++) {
        if (*p == '\n') line++;
    }
    return line;
}

/* Skip whitespace (spaces and tabs only, not newlines). */
static const char *skip_hspace(const char *p, const char *end)
{
    while (p < end && (*p == ' ' || *p == '\t')) p++;
    return p;
}

/**
 * Find the matching @end for a @repeat, handling nesting.
 * Returns pointer to the '@' of @end, or NULL if not found.
 */
static const char *find_matching_end(const char *body_start, const char *src_end)
{
    int depth = 1;
    const char *p = body_start;

    while (p < src_end && depth > 0) {
        /* Skip single-line comments */
        if (*p == '/' && p + 1 < src_end && p[1] == '/') {
            while (p < src_end && *p != '\n') p++;
            continue;
        }
        /* Skip string literals */
        if (*p == '"') {
            p++;
            while (p < src_end && *p != '"' && *p != '\n') {
                if (*p == '\\' && p + 1 < src_end) p++;
                p++;
            }
            if (p < src_end) p++;
            continue;
        }
        if (*p == '@') {
            /* Check for @repeat (nested) */
            if (matches_directive(p, src_end, "@repeat", 7) &&
                directive_boundary_ok(p, src_end, 7)) {
                depth++;
                p += 7;
                continue;
            }
            /* Check for @end — but NOT @endmod, @endtb, @endsim, etc. */
            if (matches_directive(p, src_end, "@end", 4) &&
                directive_boundary_ok(p, src_end, 4)) {
                depth--;
                if (depth == 0) {
                    return p;
                }
                p += 4;
                continue;
            }
        }
        p++;
    }
    return NULL;
}

/**
 * Substitute IDX with a decimal integer in the body text.
 * Only replaces standalone IDX (word boundaries on both sides).
 */
static int substitute_idx(StrBuf *sb,
                          const char *body,
                          size_t body_len,
                          size_t idx,
                          RepeatExpandContext *ctx,
                          const char *repeat_at)
{
    char idx_str[16];
    snprintf(idx_str, sizeof(idx_str), "%zu", idx);
    size_t idx_str_len = strlen(idx_str);

    const char *p = body;
    const char *end = body + body_len;

    while (p < end) {
        /* Look for "IDX" */
        const char *found = NULL;
        for (const char *s = p; s + 3 <= end; s++) {
            if (s[0] == 'I' && s[1] == 'D' && s[2] == 'X') {
                /* Check word boundaries */
                int left_ok = (s == body) || !is_word_char(s[-1]);
                int right_ok = (s + 3 >= end) || !is_word_char(s[3]);
                if (left_ok && right_ok) {
                    found = s;
                    break;
                }
            }
        }

        if (found) {
            /* Append everything before IDX */
            if (sb_append_limited(sb, p, (size_t)(found - p), ctx, repeat_at) != 0) {
                return 1;
            }
            /* Append the index value */
            if (sb_append_limited(sb, idx_str, idx_str_len, ctx, repeat_at) != 0) {
                return 1;
            }
            p = found + 3;
        } else {
            /* No more IDX, append the rest */
            if (sb_append_limited(sb, p, (size_t)(end - p), ctx, repeat_at) != 0) {
                return 1;
            }
            break;
        }
    }

    return 0;
}

static void repeat_frame_release(RepeatFrame *frame)
{
    if (!frame) {
        return;
    }
    if (frame->owns_out) {
        free(frame->nested_out.data);
        frame->nested_out.data = NULL;
        frame->nested_out.len = 0;
        frame->nested_out.cap = 0;
    }
}

static void repeat_frame_stack_free(RepeatFrameStack *stack)
{
    if (!stack) {
        return;
    }
    for (size_t i = 0; i < stack->len; ++i) {
        repeat_frame_release(&stack->data[i]);
    }
    free(stack->data);
    stack->data = NULL;
    stack->len = 0;
    stack->cap = 0;
}

static int repeat_frame_stack_push(RepeatFrameStack *stack, const RepeatFrame *frame)
{
    if (!stack || !frame) {
        return -1;
    }
    if (stack->len == stack->cap) {
        size_t new_cap = stack->cap ? stack->cap : 8;
        size_t new_bytes = 0;
        if (stack->cap != 0) {
            if (new_cap > SIZE_MAX / 2) {
                return -1;
            }
            new_cap *= 2;
        }
        if (jz_size_mul_checked(new_cap, sizeof(RepeatFrame), &new_bytes) != 0) {
            return -1;
        }
        RepeatFrame *new_data = (RepeatFrame *)realloc(stack->data, new_bytes);
        if (!new_data) {
            return -1;
        }
        stack->data = new_data;
        stack->cap = new_cap;
    }
    stack->data[stack->len] = *frame;
    if (stack->data[stack->len].owns_out) {
        stack->data[stack->len].out = &stack->data[stack->len].nested_out;
    }
    stack->len++;
    return 0;
}

/**
 * Expand @repeat blocks in [start, src_end).
 * Appends expanded text to `out`.
 * Returns 0 on success, non-zero on error.
 */
static int expand_region(const char *start,
                         const char *src_end,
                         RepeatExpandContext *ctx,
                         StrBuf *out)
{
    RepeatFrameStack stack = {0};
    RepeatFrame root;

    memset(&root, 0, sizeof(root));
    root.region_end = src_end;
    root.p = start;
    root.out = out;

    if (repeat_frame_stack_push(&stack, &root) != 0) {
        report_repeat_internal_failure(ctx, start,
                                       "out of memory during @repeat expansion");
        return 1;
    }

    while (stack.len > 0) {
        RepeatFrame *frame = &stack.data[stack.len - 1];
        const char *p = frame->p;

        if (p >= frame->region_end) {
            if (stack.len == 1) {
                stack.len = 0;
                break;
            }

            RepeatFrame child = stack.data[stack.len - 1];
            RepeatFrame *parent = &stack.data[stack.len - 2];
            stack.len--;

            if (!parent->pending_repeat) {
                repeat_frame_stack_free(&stack);
                report_repeat_internal_failure(ctx, parent->p,
                                               "internal error: missing pending @repeat state");
                repeat_frame_release(&child);
                return 1;
            }

            for (size_t i = 0; i < parent->pending_count; ++i) {
                if (substitute_idx(parent->out,
                                   child.out->data,
                                   child.out->len,
                                   i,
                                   ctx,
                                   parent->pending_repeat_at) != 0) {
                    repeat_frame_release(&child);
                    repeat_frame_stack_free(&stack);
                    return 1;
                }
            }
            repeat_frame_release(&child);

            parent->p = parent->pending_end_at + 4;
            parent->p = skip_hspace(parent->p, parent->region_end);
            if (parent->p < parent->region_end && *parent->p == '\n') {
                parent->p++;
            }
            parent->pending_repeat = 0;
            parent->pending_count = 0;
            parent->pending_repeat_at = NULL;
            parent->pending_end_at = NULL;
            continue;
        }

        /* Skip single-line comments */
        if (*p == '/' && has_remaining(p, frame->region_end, 2) && p[1] == '/') {
            const char *eol = p;
            while (eol < frame->region_end && *eol != '\n') eol++;
            if (sb_append_limited(frame->out, p, (size_t)(eol - p), ctx, p) != 0) {
                repeat_frame_stack_free(&stack);
                return 1;
            }
            frame->p = eol;
            continue;
        }

        /* Skip string literals */
        if (*p == '"') {
            const char *q = p + 1;
            while (q < frame->region_end && *q != '"' && *q != '\n') {
                if (*q == '\\' && has_remaining(q, frame->region_end, 2)) q++;
                q++;
            }
            if (q < frame->region_end && *q == '"') q++;
            if (sb_append_limited(frame->out, p, (size_t)(q - p), ctx, p) != 0) {
                repeat_frame_stack_free(&stack);
                return 1;
            }
            frame->p = q;
            continue;
        }

        /* Scan for @repeat */
        if (*p == '@' &&
            matches_directive(p, frame->region_end, "@repeat", 7) &&
            directive_boundary_ok(p, frame->region_end, 7)) {
            const char *repeat_at = p;
            const char *body_start = NULL;
            const char *end_at = NULL;
            RepeatFrame child;
            memset(&child, 0, sizeof(child));
            p += 7;

            /* Skip whitespace after @repeat */
            p = skip_hspace(p, frame->region_end);

            /* Parse the count */
            if (p >= frame->region_end || !isdigit((unsigned char)*p)) {
                report_repeat_rule(ctx, repeat_at,
                                   "RPT_COUNT_INVALID",
                                   "S7.4/S4.6 RPT-001 @repeat requires a positive integer count");
                repeat_frame_stack_free(&stack);
                return 1;
            }

            size_t count = 0;
            int overflow = 0;
            while (p < frame->region_end && isdigit((unsigned char)*p)) {
                unsigned digit = (unsigned)(*p - '0');
                if (!overflow) {
                    if (count > (SIZE_MAX - digit) / 10) {
                        overflow = 1;
                    } else {
                        count = count * 10 + digit;
                    }
                }
                p++;
            }

            if (count == 0) {
                report_repeat_rule(ctx, repeat_at,
                                   "RPT_COUNT_INVALID",
                                   "S7.4/S4.6 RPT-001 @repeat requires a positive integer count");
                repeat_frame_stack_free(&stack);
                return 1;
            }

            if (overflow || count > ctx->limits.repeat_max_count) {
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "@repeat count exceeds the configured limit of %zu iteration(s)",
                         ctx->limits.repeat_max_count);
                report_repeat_rule(ctx, repeat_at,
                                   "RPT_COUNT_LIMIT_EXCEEDED",
                                   msg);
                repeat_frame_stack_free(&stack);
                return 1;
            }

            /* Skip to end of line (the body starts on the next line or after whitespace) */
            p = skip_hspace(p, frame->region_end);
            if (p < frame->region_end && *p == '\n') p++;

            /* Find matching @end */
            end_at = find_matching_end(p, frame->region_end);
            if (!end_at) {
                report_repeat_rule(ctx, repeat_at,
                                   "RPT_NO_MATCHING_END",
                                   "RPT-002 @repeat without matching @end");
                repeat_frame_stack_free(&stack);
                return 1;
            }

            body_start = p;
            sb_init(&child.nested_out);
            if (!child.nested_out.data) {
                report_repeat_internal_failure(ctx, repeat_at,
                                               "out of memory during @repeat expansion");
                repeat_frame_stack_free(&stack);
                return 1;
            }
            child.region_end = end_at;
            child.p = body_start;
            child.out = &child.nested_out;
            child.owns_out = 1;

            frame->pending_repeat = 1;
            frame->pending_count = count;
            frame->pending_repeat_at = repeat_at;
            frame->pending_end_at = end_at;

            if (repeat_frame_stack_push(&stack, &child) != 0) {
                repeat_frame_release(&child);
                report_repeat_internal_failure(ctx, repeat_at,
                                               "out of memory during @repeat expansion");
                repeat_frame_stack_free(&stack);
                return 1;
            }

        } else {
            /* Regular character — just copy */
            if (sb_append_limited(frame->out, p, 1, ctx, p) != 0) {
                repeat_frame_stack_free(&stack);
                return 1;
            }
            frame->p = p + 1;
        }
    }

    repeat_frame_stack_free(&stack);
    return 0;
}

/* ── Public API ─────────────────────────────────────────────────── */

char *jz_repeat_expand(const char *source,
                       const char *filename,
                       JZDiagnosticList *diagnostics,
                       const JZExpansionLimits *limits)
{
    JZExpansionLimits effective_limits = JZ_EXPANSION_LIMITS_DEFAULT_INIT;
    RepeatExpandContext ctx;

    if (!source) return NULL;
    if (limits) {
        effective_limits = *limits;
    }

    /* Quick check: if no @repeat in source, return a copy as-is */
    if (!strstr(source, "@repeat")) {
        return strdup(source);
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.full_src = source;
    ctx.filename = filename;
    ctx.diagnostics = diagnostics;
    ctx.limits = effective_limits;

    size_t src_len = strlen(source);
    StrBuf out;
    sb_init(&out);
    if (!out.data) {
        report_repeat_internal_failure(&ctx, source,
                                       "out of memory during @repeat expansion");
        return NULL;
    }

    if (expand_region(source, source + src_len, &ctx, &out) != 0) {
        free(out.data);
        return NULL;
    }

    return out.data;
}
