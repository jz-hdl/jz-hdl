/**
 * @file ast_json.h
 * @brief JSON serialization for the AST.
 *
 * Provides a function to emit the abstract syntax tree as pretty-printed
 * JSON, used by the --ast CLI mode.
 */

#ifndef JZ_HDL_AST_JSON_H
#define JZ_HDL_AST_JSON_H

#include <stdio.h>
#include "ast.h"
#include "diagnostic.h"

/**
 * @brief Print an AST as pretty-printed JSON.
 * @param out  Output stream (e.g., stdout).
 * @param root Root AST node to serialize. Must not be NULL.
 */
int jz_ast_print_json(FILE *out, const JZASTNode *root, JZDiagnosticList *diagnostics);

#endif /* JZ_HDL_AST_JSON_H */
