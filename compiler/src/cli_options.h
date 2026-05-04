/**
 * @file cli_options.h
 * @brief Parsed CLI option state for compiler entry-point dispatch.
 */

#ifndef JZ_HDL_CLI_OPTIONS_H
#define JZ_HDL_CLI_OPTIONS_H

#include <stddef.h>
#include <stdint.h>

#include "diagnostic.h"
#include "expansion_limits.h"
#include "sim/sim_engine.h"

/**
 * @struct JZCLIOptions
 * @brief Parsed command-line options for a single compiler invocation.
 */
typedef struct JZCLIOptions {
    const char *input_filename;                  /**< Primary input source file, or NULL when absent. */
    const char *output_filename;                 /**< Requested output filename, or NULL to use mode defaults. */
    const char *mode;                            /**< Selected mode flag such as `--lint` or `--verilog`. */
    const char *sdc_filename;                    /**< Optional SDC constraint output path. */
    const char *xdc_filename;                    /**< Optional XDC constraint output path. */
    const char *pcf_filename;                    /**< Optional PCF constraint output path. */
    const char *cst_filename;                    /**< Optional CST constraint output path. */
    int lint_rules;                              /**< Non-zero to print lint-rule metadata instead of compiling input. */
    int warn_as_error;                           /**< Non-zero to promote warnings to errors. */
    int show_info;                               /**< Non-zero to include informational diagnostics in output. */
    int use_color;                               /**< Non-zero to enable ANSI color in diagnostic rendering. */
    int alias_report;                            /**< Non-zero to emit the alias report. */
    int memory_report;                           /**< Non-zero to emit the memory report. */
    int tristate_report;                         /**< Non-zero to emit the tri-state lowering report. */
    int test_mode;                               /**< Non-zero when `--test` mode is active. */
    int simulate_mode;                           /**< Non-zero when `--simulate` mode is active. */
    int verbose;                                 /**< Non-zero to print per-phase timing and progress output. */
    int show_explain;                            /**< Non-zero to print expanded diagnostic explanations. */
    int allow_absolute_paths;                    /**< Non-zero to allow absolute paths in source-controlled file access. */
    int allow_traversal;                         /**< Non-zero to allow `..` path traversal in source-controlled file access. */
    int chip_info;                               /**< Non-zero to run chip-info mode instead of normal compilation. */
    const char *chip_info_id;                    /**< Optional chip identifier argument for chip-info mode. */
    int tristate_default;                        /**< Tri-state default selection: 0 unset, 1 GND, 2 VCC. */
    int sim_format_fst;                          /**< Non-zero to emit FST waveforms for simulation output. */
    int sim_format_jzw;                          /**< Non-zero to emit JZW waveforms for simulation output. */
    uint32_t test_seed;                          /**< Parsed simulation or test seed value. */
    int test_seed_set;                           /**< Non-zero when `test_seed` came from the command line. */
    const char *sandbox_roots[16];              /**< Additional permitted filesystem roots. */
    size_t sandbox_root_count;                   /**< Number of valid entries in `sandbox_roots`. */
    JZWarningGroupOverride group_overrides[16];  /**< Explicit warning-group enable or disable overrides. */
    size_t group_override_count;                 /**< Number of valid entries in `group_overrides`. */
    SimJitterConfig jitter_configs[16];          /**< Parsed clock-jitter simulation overrides. */
    int num_jitter;                              /**< Number of valid jitter entries. */
    SimDriftConfig drift_configs[16];            /**< Parsed clock-drift simulation overrides. */
    int num_drift;                               /**< Number of valid drift entries. */
    JZExpansionLimits expansion_limits;          /**< Expansion safety limits for repeat and apply processing. */
} JZCLIOptions;

/**
 * @brief Parse command-line arguments into a `JZCLIOptions` structure.
 *
 * @param opts Receives the parsed option state.
 * @param argc Argument count from `main()`.
 * @param argv Argument vector from `main()`.
 * @return 0 on success, 1 on parse error, or -1 when the program should exit successfully.
 */
int jz_cli_parse_options(JZCLIOptions *opts, int argc, char **argv);

/**
 * @brief Print the command-line usage summary to `stderr`.
 *
 * @param prog Program name to include in the usage text.
 */
void jz_cli_print_usage(const char *prog);

/**
 * @brief Print the compiler version string to `stdout`.
 */
void jz_cli_print_version(void);

#endif /* JZ_HDL_CLI_OPTIONS_H */
