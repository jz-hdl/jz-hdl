/**
 * @file lsp_io.c
 * @brief JSON-RPC message I/O over stdio for the LSP server.
 */

#include "lsp/lsp_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#ifndef _WIN32
#include <sys/select.h>
#include <unistd.h>
#endif

#define LSP_MAX_CONTENT_LENGTH (16u * 1024u * 1024u)
#define LSP_BODY_READ_TIMEOUT_MS 250

/**
 * @brief Convert a hexadecimal digit character into its numeric value.
 * @param c ASCII digit to decode.
 * @return Hex digit value in the range 0-15, or -1 if invalid.
 */
static int hex_digit(char c);
static int lsp_read_body_exact(char *body, size_t content_length);
#ifndef _WIN32
static int lsp_wait_stdin_readable(int timeout_ms);
#endif

/**
 * @brief Write a formatted debug log line for the LSP server.
 * @param fmt `printf`-style format string.
 * @param ... Format arguments.
 */
void lsp_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[jz-hdl-lsp] ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    fflush(stderr);
    va_end(ap);
}

char *lsp_io_read_message(size_t *out_len) {
    /* Read headers until we find Content-Length and a blank line. */
    size_t content_length = 0;
    int have_content_length = 0;
    static int stdio_unbuffered = 0;

    if (!stdio_unbuffered) {
        (void)setvbuf(stdin, NULL, _IONBF, 0);
        stdio_unbuffered = 1;
    }

    for (;;) {
        char header[256];
        if (!fgets(header, sizeof(header), stdin)) {
            return NULL; /* EOF */
        }

        /* Blank line (just \r\n or \n) marks end of headers. */
        if (strcmp(header, "\r\n") == 0 || strcmp(header, "\n") == 0) {
            break;
        }

        if (strncmp(header, "Content-Length:", 15) == 0) {
            const char *value = header + 15;
            while (*value == ' ' || *value == '\t') ++value;

            errno = 0;
            char *end = NULL;
            unsigned long parsed = strtoul(value, &end, 10);
            while (end && (*end == ' ' || *end == '\t' ||
                           *end == '\r' || *end == '\n')) {
                ++end;
            }

            if (value == end || errno == ERANGE ||
                parsed > LSP_MAX_CONTENT_LENGTH ||
                (end && *end != '\0')) {
                lsp_log("invalid Content-Length header");
                return NULL;
            }

            content_length = (size_t)parsed;
            have_content_length = 1;
        }
        /* Ignore other headers (e.g., Content-Type). */
    }

    if (!have_content_length) {
        return NULL;
    }

    char *body = malloc(content_length + 1);
    if (!body) return NULL;

    if (lsp_read_body_exact(body, content_length) != 0) {
        free(body);
        return NULL;
    }
    body[content_length] = '\0';

    if (out_len) *out_len = (size_t)content_length;
    return body;
}

void lsp_io_write_message(const char *json, size_t len) {
    fprintf(stdout, "Content-Length: %zu\r\n\r\n", len);
    fwrite(json, 1, len, stdout);
    fflush(stdout);
}

/* ------------------------------------------------------------------ */
/*  URI helpers                                                       */
/* ------------------------------------------------------------------ */

static int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int lsp_uri_to_path(const char *uri, char *out, size_t out_cap) {
    /* Expect file:///path or file://localhost/path. */
    if (strncmp(uri, "file://", 7) != 0) return -1;
    const char *p = uri + 7;
    /* Skip optional localhost. */
    if (strncmp(p, "localhost", 9) == 0) p += 9;
    /* On unix, path starts with /. On windows, skip the leading /
     * before the drive letter (e.g., /C:/...). We handle unix only. */

    size_t i = 0;
    while (*p && i + 1 < out_cap) {
        if (*p == '%' && p[1] && p[2]) {
            int h = hex_digit(p[1]);
            int l = hex_digit(p[2]);
            if (h >= 0 && l >= 0) {
                out[i++] = (char)(h * 16 + l);
                p += 3;
                continue;
            }
        }
        out[i++] = *p++;
    }
    out[i] = '\0';
    return 0;
}

#ifndef _WIN32
static int lsp_wait_stdin_readable(int timeout_ms)
{
    fd_set rfds;
    struct timeval tv;
    int fd = fileno(stdin);

    if (fd < 0) return -1;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    return select(fd + 1, &rfds, NULL, NULL, &tv);
}
#endif

static int lsp_read_body_exact(char *body, size_t content_length)
{
    size_t total = 0;

    if (!body) return -1;

    while (total < content_length) {
#ifndef _WIN32
        int ready = lsp_wait_stdin_readable(LSP_BODY_READ_TIMEOUT_MS);
        if (ready <= 0) {
            lsp_log("timed out waiting for LSP body bytes (%zu/%zu read)",
                    total, content_length);
            return -1;
        }
#endif
        int ch = fgetc(stdin);
        if (ch == EOF) {
            lsp_log("unexpected EOF in LSP message body (%zu/%zu read)",
                    total, content_length);
            return -1;
        }
        body[total++] = (char)ch;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Document store                                                    */
/* ------------------------------------------------------------------ */

void lsp_docstore_init(LspDocStore *store) {
    memset(store, 0, sizeof(*store));
}

void lsp_docstore_free(LspDocStore *store) {
    for (size_t i = 0; i < store->count; ++i) {
        free(store->docs[i].uri);
        free(store->docs[i].content);
    }
    store->count = 0;
}

LspDocument *lsp_docstore_open(LspDocStore *store, const char *uri,
                               const char *content, int version) {
    if (store->count >= LSP_MAX_DOCUMENTS) return NULL;
    char *uri_dup = strdup(uri);
    char *content_dup = strdup(content);
    if (!uri_dup || !content_dup) {
        free(uri_dup);
        free(content_dup);
        return NULL;
    }
    LspDocument *doc = &store->docs[store->count++];
    doc->uri = uri_dup;
    doc->content = content_dup;
    doc->version = version;
    return doc;
}

LspDocument *lsp_docstore_find(LspDocStore *store, const char *uri) {
    for (size_t i = 0; i < store->count; ++i) {
        if (strcmp(store->docs[i].uri, uri) == 0) {
            return &store->docs[i];
        }
    }
    return NULL;
}

int lsp_docstore_update(LspDocument *doc, const char *content, int version) {
    char *dup = strdup(content);
    if (!dup) return -1;
    free(doc->content);
    doc->content = dup;
    doc->version = version;
    return 0;
}

void lsp_docstore_close(LspDocStore *store, const char *uri) {
    for (size_t i = 0; i < store->count; ++i) {
        if (strcmp(store->docs[i].uri, uri) == 0) {
            free(store->docs[i].uri);
            free(store->docs[i].content);
            /* Shift remaining entries down. */
            for (size_t k = i; k + 1 < store->count; ++k) {
                store->docs[k] = store->docs[k + 1];
            }
            --store->count;
            return;
        }
    }
}
