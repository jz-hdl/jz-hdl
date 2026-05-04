#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "../../include/ast.h"
#include "../../include/util.h"

#define INITIAL_CHILD_CAPACITY 4

/*
 * Helper to duplicate a string while trimming leading and trailing
 * whitespace. Used for AST identifier names and free-form text so
 * that parser-constructed lexemes like "foo " or "expr ... " do not
 * carry trailing spaces into later phases (JSON, diagnostics, IR).
 */
static char *jz_strdup_trim(const char *s)
{
    if (!s) return NULL;

    const char *start = s;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    const char *end = s + strlen(s);
    while (end > start && isspace((unsigned char)*(end - 1))) {
        end--;
    }

    size_t len = (size_t)(end - start);
    size_t alloc_size = 0;
    char *copy = NULL;
    if (jz_size_add_checked(len, 1, &alloc_size) != 0) return NULL;
    copy = (char *)malloc(alloc_size);
    if (!copy) return NULL;
    if (len > 0) {
        memcpy(copy, start, len);
    }
    copy[len] = '\0';
    return copy;
}

JZASTNode *jz_ast_new(JZASTNodeType type, JZLocation loc) {
    JZASTNode *node = (JZASTNode *)calloc(1, sizeof(JZASTNode));
    if (!node) return NULL;
    node->type = type;
    node->loc = loc;
    return node;
}

void jz_ast_set_name(JZASTNode *node, const char *name) {
    if (!node) return;
    free(node->name);
    node->name = name ? jz_strdup_trim(name) : NULL;
}

void jz_ast_set_block_kind(JZASTNode *node, const char *kind) {
    if (!node) return;
    free(node->block_kind);
    node->block_kind = kind ? jz_strdup_trim(kind) : NULL;
}

void jz_ast_set_text(JZASTNode *node, const char *text) {
    if (!node) return;
    free(node->text);
    node->text = text ? jz_strdup_trim(text) : NULL;
}

void jz_ast_set_width(JZASTNode *node, const char *width) {
    if (!node) return;
    free(node->width);
    node->width = width ? jz_strdup_trim(width) : NULL;
}

int jz_ast_add_child(JZASTNode *parent, JZASTNode *child) {
    if (!parent || !child) return -1;
    if (parent->child_count == parent->child_capacity) {
        size_t new_cap = 0;
        size_t new_bytes = 0;
        if (jz_size_grow_doubling_checked(parent->child_capacity,
                                          parent->child_count + 1,
                                          INITIAL_CHILD_CAPACITY,
                                          &new_cap) != 0) return -1;
        if (jz_size_mul_checked(new_cap, sizeof(JZASTNode *), &new_bytes) != 0) return -1;
        JZASTNode **new_children = (JZASTNode **)realloc(parent->children, new_bytes);
        if (!new_children) return -1;
        parent->children = new_children;
        parent->child_capacity = new_cap;
    }
    if (parent->child_count >= parent->child_capacity) return -1;
    parent->children[parent->child_count++] = child;
    return 0;
}

static void jz_ast_free_recursive_unbounded(JZASTNode *node)
{
    if (!node) return;
    for (size_t i = 0; i < node->child_count; ++i) {
        jz_ast_free_recursive_unbounded(node->children[i]);
    }
    free(node->children);
    free(node->name);
    free(node->block_kind);
    free(node->text);
    free(node->width);
    free(node);
}

void jz_ast_free(JZASTNode *node) {
    JZBuffer stack = {0};
    JZASTNode *cur = NULL;

    if (!node) return;
    if (jz_buf_append(&stack, &node, sizeof(node)) != 0) {
        jz_ast_free_recursive_unbounded(node);
        return;
    }

    while (stack.len >= sizeof(cur)) {
        memcpy(&cur, stack.data + stack.len - sizeof(cur), sizeof(cur));
        stack.len -= sizeof(cur);
        if (!cur) continue;

        for (size_t i = 0; i < cur->child_count; ++i) {
            JZASTNode *child = cur->children[i];
            if (!child) continue;
            if (jz_buf_append(&stack, &child, sizeof(child)) != 0) {
                for (size_t j = i; j < cur->child_count; ++j) {
                    jz_ast_free_recursive_unbounded(cur->children[j]);
                }
                free(cur->children);
                free(cur->name);
                free(cur->block_kind);
                free(cur->text);
                free(cur->width);
                free(cur);
                while (stack.len >= sizeof(cur)) {
                    memcpy(&cur, stack.data + stack.len - sizeof(cur), sizeof(cur));
                    stack.len -= sizeof(cur);
                    jz_ast_free_recursive_unbounded(cur);
                }
                jz_buf_free(&stack);
                return;
            }
        }

        free(cur->children);
        free(cur->name);
        free(cur->block_kind);
        free(cur->text);
        free(cur->width);
        free(cur);
    }

    jz_buf_free(&stack);
}
