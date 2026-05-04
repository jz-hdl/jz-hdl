/**
 * @file cli_frontend.h
 * @brief Front-end driver for source loading, expansion, lexing, and parsing.
 */

#ifndef JZ_HDL_CLI_FRONTEND_H
#define JZ_HDL_CLI_FRONTEND_H

#include <stdio.h>
#include <time.h>

#include "compiler.h"
#include "expansion_limits.h"

/**
 * @brief Measure elapsed wall-clock time from a saved `clock()` sample.
 *
 * @param start Start timestamp returned by `clock()`.
 * @return Elapsed time in milliseconds.
 */
double jz_cli_elapsed_ms(clock_t start);

/**
 * @brief Run the compiler front-end pipeline for one input file.
 *
 * Reads the source file, applies expansion passes, lexes and parses the AST,
 * then either prints the AST JSON or runs semantic analysis.
 *
 * @param compiler      Compiler context receiving AST state and diagnostics.
 * @param filename      Source filename to compile.
 * @param print_ast_json Non-zero to print the AST JSON instead of running semantics.
 * @param ast_out       Output stream for AST JSON, or NULL to use `stdout`.
 * @param test_mode     Non-zero when `@testbench` blocks are allowed.
 * @param simulate_mode Non-zero when `@simulation` blocks are allowed.
 * @param verbose       Non-zero to emit timing diagnostics to `stderr`.
 * @param limits        Expansion safety limits to enforce during preprocessing.
 * @return 0 on success, non-zero on failure.
 */
int jz_cli_run_frontend(JZCompiler *compiler,
                        const char *filename,
                        int print_ast_json,
                        FILE *ast_out,
                        int test_mode,
                        int simulate_mode,
                        int verbose,
                        const JZExpansionLimits *limits);

#endif /* JZ_HDL_CLI_FRONTEND_H */
