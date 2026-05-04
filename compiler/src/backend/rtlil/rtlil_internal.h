/**
 * @file rtlil_internal.h
 * @brief Internal cross-translation-unit declarations for the RTLIL backend.
 */
#ifndef JZ_HDL_RTLIL_INTERNAL_H
#define JZ_HDL_RTLIL_INTERNAL_H

#include <stdio.h>
#include <stdbool.h>

#include "ir.h"
#include "diagnostic.h"

/**
 * @brief Return the next backend-generated identifier.
 * @return Next monotonically increasing auto-ID value.
 */
int rtlil_next_id(void);

/**
 * @brief Reset the backend-generated identifier counter.
 */
void rtlil_reset_id(void);

/**
 * @brief Return the current backend-generated identifier without incrementing.
 * @return Current auto-ID value.
 */
int rtlil_current_id(void);

/**
 * @brief Report an RTLIL backend I/O failure.
 * @param diagnostics Diagnostic sink that receives the emitted error.
 * @param input_filename Source filename used for the diagnostic location.
 * @param message Diagnostic message text.
 */
void rtlil_backend_io_error(JZDiagnosticList *diagnostics,
                            const char *input_filename,
                            const char *message);

/**
 * @brief Write RTLIL indentation.
 * @param out Destination stream.
 * @param level Indentation depth in two-space units.
 */
void rtlil_indent(FILE *out, int level);

/**
 * @brief Emit an IR literal as an RTLIL constant.
 * @param out Destination stream.
 * @param lit Literal value to emit. A null pointer emits `1'0`.
 */
void rtlil_emit_const(FILE *out, const IR_Literal *lit);

/**
 * @brief Emit a raw integer value as an RTLIL constant.
 * @param out Destination stream.
 * @param width Bit width to emit.
 * @param value Unsigned value to encode.
 */
void rtlil_emit_const_val(FILE *out, int width, uint64_t value);

/**
 * @brief Emit an all-zero RTLIL constant.
 * @param out Destination stream.
 * @param width Bit width to emit.
 */
void rtlil_emit_zero(FILE *out, int width);

/**
 * @brief Emit the sigspec for a whole signal.
 * @param out Destination stream.
 * @param mod Module used to resolve the signal.
 * @param signal_id IR signal identifier to emit.
 */
void rtlil_emit_sigspec_signal(FILE *out, const IR_Module *mod, int signal_id);

/**
 * @brief Emit the sigspec for one selected signal bit.
 * @param out Destination stream.
 * @param mod Module used to resolve the signal.
 * @param signal_id IR signal identifier to emit.
 * @param bit Zero-based bit index to select.
 */
void rtlil_emit_sigspec_bit(FILE *out, const IR_Module *mod, int signal_id,
                            int bit);

/**
 * @brief Emit the sigspec for a contiguous signal slice.
 * @param out Destination stream.
 * @param mod Module used to resolve the signal.
 * @param signal_id IR signal identifier to emit.
 * @param offset Least-significant bit index of the slice.
 * @param width Width of the slice in bits.
 */
void rtlil_emit_sigspec_range(FILE *out, const IR_Module *mod, int signal_id,
                              int offset, int width);

/**
 * @brief Look up a signal after alias canonicalization.
 * @param mod Module that owns the signal.
 * @param signal_id IR signal identifier to resolve.
 * @return Canonical signal for the identifier, or `NULL` if none exists.
 */
const IR_Signal *rtlil_find_signal_by_id(const IR_Module *mod, int signal_id);

/**
 * @brief Look up a signal without alias canonicalization.
 * @param mod Module that owns the signal.
 * @param signal_id IR signal identifier to resolve.
 * @return Matching raw signal, or `NULL` if none exists.
 */
const IR_Signal *rtlil_find_signal_by_id_raw(const IR_Module *mod, int signal_id);

/**
 * @brief Look up a clock domain by identifier.
 * @param mod Module that owns the clock domain.
 * @param clock_id IR clock-domain identifier to resolve.
 * @return Matching clock domain, or `NULL` if none exists.
 */
const IR_ClockDomain *rtlil_find_clock_domain_by_id(const IR_Module *mod,
                                                    int clock_id);

/** Maximum size, in bytes, of scratch sigspec buffers used by the backend. */
#define RTLIL_SIGSPEC_MAX 4096

/**
 * @brief Lower an expression into RTLIL cells and return its result sigspec.
 * @param out Destination stream for emitted wires and cells.
 * @param mod Module that owns the expression.
 * @param expr Expression to lower.
 * @param out_sigspec Buffer that receives the resulting sigspec string.
 * @param sigspec_size Size of `out_sigspec` in bytes.
 * @return `0` after writing a valid fallback or computed sigspec.
 */
int rtlil_emit_expr(FILE *out, const IR_Module *mod, const IR_Expr *expr,
                    char *out_sigspec, int sigspec_size);

/**
 * @brief Emit the module's asynchronous statement block as an RTLIL process.
 * @param out Destination stream.
 * @param mod Module whose asynchronous logic is emitted.
 */
void rtlil_emit_async_block(FILE *out, const IR_Module *mod);

/**
 * @brief Emit RTLIL processes for every clock domain in a module.
 * @param out Destination stream.
 * @param mod Module whose synchronous logic is emitted.
 */
void rtlil_emit_clock_domains(FILE *out, const IR_Module *mod);

/**
 * @brief Emit the opening RTLIL lines for one module.
 * @param out Destination stream.
 * @param mod Module to open.
 * @param is_top Nonzero when the emitted module should receive the `\\top`
 * attribute.
 */
void rtlil_emit_module_header(FILE *out, const IR_Module *mod, int is_top);

/**
 * @brief Emit RTLIL wire declarations for the module signal table.
 * @param out Destination stream.
 * @param mod Module whose signal declarations are emitted.
 */
void rtlil_emit_wires(FILE *out, const IR_Module *mod);

/**
 * @brief Emit RTLIL memory declarations for a module.
 * @param out Destination stream.
 * @param mod Module whose memory declarations are emitted.
 */
void rtlil_emit_memories(FILE *out, const IR_Module *mod);

/**
 * @brief Emit RTLIL `connect` statements for alias assignments.
 * @param out Destination stream.
 * @param mod Module whose alias assignments are emitted.
 */
void rtlil_emit_alias_connects(FILE *out, const IR_Module *mod);

/**
 * @brief Emit RTLIL cell instances for user-defined module instantiations.
 * @param out Destination stream.
 * @param design Design that owns the referenced child modules.
 * @param mod Parent module whose instances are emitted.
 */
void rtlil_emit_instances(FILE *out, const IR_Design *design,
                          const IR_Module *mod);

/**
 * @brief Emit RTLIL memory initialization and write-port cells.
 * @param out Destination stream.
 * @param mod Module whose memory support cells are emitted.
 */
void rtlil_emit_memory_cells(FILE *out, const IR_Module *mod);

/**
 * @brief Clear the memory-emission error counter.
 */
void rtlil_reset_memory_emit_errors(void);

/**
 * @brief Return the number of memory-emission errors seen so far.
 * @return Count of memory-related backend emission failures.
 */
int rtlil_memory_emit_errors(void);

/**
 * @brief Emit the project-level wrapper module.
 * @param out Destination stream.
 * @param design Design whose project wrapper is emitted.
 */
void rtlil_emit_project_wrapper(FILE *out, const IR_Design *design);

/**
 * @brief Emit all RTLIL modules in backend-selected order.
 * @param out Destination stream.
 * @param design Design whose modules are emitted.
 */
void rtlil_emit_module_order(FILE *out, const IR_Design *design);

/**
 * @brief Check whether a design already contains a module with a given name.
 * @param design Design to search.
 * @param name Module name to look for.
 * @return Nonzero when any module in `design` matches `name`.
 */
int rtlil_design_has_module_named(const IR_Design *design, const char *name);

#endif /* JZ_HDL_RTLIL_INTERNAL_H */
