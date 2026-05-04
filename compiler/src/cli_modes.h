/**
 * @file cli_modes.h
 * @brief CLI backend-mode entry points after front-end success.
 */

#ifndef JZ_HDL_CLI_MODES_H
#define JZ_HDL_CLI_MODES_H

#include "compiler.h"
#include "cli_options.h"

/**
 * @brief Run lint-mode IR construction and validation.
 *
 * @param compiler Compiler context containing the verified AST.
 * @param opts     Parsed CLI options for the current invocation.
 * @return 0 on success, non-zero on failure.
 */
int jz_cli_run_lint_ir(JZCompiler *compiler, const JZCLIOptions *opts);

/**
 * @brief Serialize the IR as JSON for `--ir` mode.
 *
 * @param compiler Compiler context containing the verified AST.
 * @param opts     Parsed CLI options for the current invocation.
 * @return 0 on success, non-zero on failure.
 */
int jz_cli_run_ir_emit(JZCompiler *compiler, const JZCLIOptions *opts);

/**
 * @brief Run the Verilog backend and any requested constraint emitters.
 *
 * @param compiler Compiler context containing the verified AST.
 * @param opts     Parsed CLI options for the current invocation.
 * @return 0 on success, non-zero on failure.
 */
int jz_cli_run_verilog(JZCompiler *compiler, const JZCLIOptions *opts);

/**
 * @brief Run the RTLIL backend and any requested constraint emitters.
 *
 * @param compiler Compiler context containing the verified AST.
 * @param opts     Parsed CLI options for the current invocation.
 * @return 0 on success, non-zero on failure.
 */
int jz_cli_run_rtlil(JZCompiler *compiler, const JZCLIOptions *opts);

/**
 * @brief Execute `@testbench` blocks for `--test` mode.
 *
 * @param compiler Compiler context containing the verified AST and IR.
 * @param opts     Parsed CLI options for the current invocation.
 * @return 0 on success, non-zero on failure.
 */
int jz_cli_run_test(JZCompiler *compiler, const JZCLIOptions *opts);

/**
 * @brief Execute `@simulation` blocks and emit a waveform.
 *
 * @param compiler Compiler context containing the verified AST and IR.
 * @param opts     Parsed CLI options for the current invocation.
 * @return 0 on success, non-zero on failure.
 */
int jz_cli_run_simulate(JZCompiler *compiler, const JZCLIOptions *opts);

#endif /* JZ_HDL_CLI_MODES_H */
