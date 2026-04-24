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

/* ── Dynamic string buffer ──────────────────────────────────────── */

typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} StrBuf;

typedef struct {
    const char        *full_src;
    const char        *filename;
    JZDiagnosticList  *diagnostics;
    JZExpansionLimits  limits;
} RepeatExpandContext;

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

    needed = sb->len + n + 1;
    while (needed > sb->cap) {
        size_t new_cap = sb->cap ? sb->cap : 4096;
        char *new_data = NULL;

        while (needed > new_cap) {
            if (new_cap > SIZE_MAX / 2) {
                new_cap = needed;
                break;
            }
            new_cap *= 2;
        }

        new_data = realloc(sb->data, new_cap);
        if (!new_data) return 1;
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
static const char *skip_hspace(const char *p)
{
    while (*p == ' ' || *p == '\t') p++;
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
            if (strncmp(p, "@repeat", 7) == 0 && !is_word_char(p[7])) {
                depth++;
                p += 7;
                continue;
            }
            /* Check for @end — but NOT @endmod, @endtb, @endsim, etc. */
            if (strncmp(p, "@end", 4) == 0 && !is_word_char(p[4])) {
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

/* ── Recursive expansion ────────────────────────────────────────── */

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
    const char *p = start;

    while (p < src_end) {
        /* Skip single-line comments */
        if (*p == '/' && p + 1 < src_end && p[1] == '/') {
            const char *eol = p;
            while (eol < src_end && *eol != '\n') eol++;
            if (sb_append_limited(out, p, (size_t)(eol - p), ctx, p) != 0) {
                return 1;
            }
            p = eol;
            continue;
        }

        /* Skip string literals */
        if (*p == '"') {
            const char *q = p + 1;
            while (q < src_end && *q != '"' && *q != '\n') {
                if (*q == '\\' && q + 1 < src_end) q++;
                q++;
            }
            if (q < src_end && *q == '"') q++;
            if (sb_append_limited(out, p, (size_t)(q - p), ctx, p) != 0) {
                return 1;
            }
            p = q;
            continue;
        }

        /* Scan for @repeat */
        if (*p == '@' && strncmp(p, "@repeat", 7) == 0 && !is_word_char(p[7])) {
            const char *repeat_at = p;
            p += 7;

            /* Skip whitespace after @repeat */
            p = skip_hspace(p);

            /* Parse the count */
            if (!isdigit((unsigned char)*p)) {
                report_repeat_rule(ctx, repeat_at,
                                   "RPT_COUNT_INVALID",
                                   "RPT-001 @repeat requires a positive integer count");
                return 1;
            }

            size_t count = 0;
            int overflow = 0;
            while (isdigit((unsigned char)*p)) {
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
                                   "RPT-001 @repeat count must be a positive integer");
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
                return 1;
            }

            /* Skip to end of line (the body starts on the next line or after whitespace) */
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '\n') p++;

            /* Find matching @end */
            const char *end_at = find_matching_end(p, src_end);
            if (!end_at) {
                report_repeat_rule(ctx, repeat_at,
                                   "RPT_NO_MATCHING_END",
                                   "RPT-002 @repeat without matching @end");
                return 1;
            }

            const char *body_start = p;
            StrBuf nested;

            sb_init(&nested);
            if (!nested.data) return 1;
            if (expand_region(body_start, end_at, ctx, &nested) != 0) {
                free(nested.data);
                return 1;
            }

            /* For each iteration, recursively expand nested @repeats,
             * then substitute IDX. */
            for (size_t i = 0; i < count; i++) {
                if (substitute_idx(out, nested.data, nested.len, i, ctx, repeat_at) != 0) {
                    free(nested.data);
                    return 1;
                }
            }
            free(nested.data);

            /* Skip past @end */
            p = end_at + 4; /* skip "@end" */
            /* Skip trailing whitespace and newline */
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '\n') p++;

        } else {
            /* Regular character — just copy */
            if (sb_append_limited(out, p, 1, ctx, p) != 0) {
                return 1;
            }
            p++;
        }
    }

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
    if (!out.data) return NULL;

    if (expand_region(source, source + src_len, &ctx, &out) != 0) {
        free(out.data);
        return NULL;
    }

    return out.data;
}
