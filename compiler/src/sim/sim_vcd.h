/**
 * @file sim_vcd.h
 * @brief VCD waveform writer API for simulator traces.
 */

#ifndef JZ_SIM_VCD_H
#define JZ_SIM_VCD_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @struct VCDWriter
 * @brief Opaque writer for Value Change Dump output.
 */
typedef struct VCDWriter VCDWriter;

/**
 * @brief Open a new VCD file for writing.
 *
 * @param filename     Output file path.
 * @param timescale_ps Timescale in picoseconds (e.g., 1000 for 1ns).
 * @return Writer handle, or NULL on failure.
 */
VCDWriter *vcd_open(const char *filename, uint64_t timescale_ps);

/**
 * @brief Register a signal that should appear in the VCD output.
 *
 * Must be called before vcd_end_definitions().
 *
 * @param w     VCD writer.
 * @param scope Hierarchical scope (e.g., "dut", "tb").
 * @param name  Signal name.
 * @param width Signal width in bits.
 * @return Signal ID (0-based index), or -1 on failure.
 */
int vcd_add_signal(VCDWriter *w, const char *scope, const char *name, int width);

/**
 * @brief Finalize the VCD header and variable definitions.
 *
 * Must be called after all vcd_add_signal() calls and before any value dumps.
 *
 * @param w VCD writer.
 */
void vcd_end_definitions(VCDWriter *w);

/**
 * @brief Advance the current timestamp used for subsequent dumps.
 *
 * Only emits a time marker if the time has changed since the last call.
 *
 * @param w       VCD writer.
 * @param time_ps Current time in picoseconds.
 */
void vcd_set_time(VCDWriter *w, uint64_t time_ps);

/**
 * @brief Emit a value change record for one signal.
 *
 * @param w      VCD writer.
 * @param sig_id Signal ID returned by vcd_add_signal().
 * @param value  Signal value (up to 64 bits).
 * @param width Declared signal width in bits. This backend uses the width already registered for the signal.
 */
void vcd_dump_value(VCDWriter *w, int sig_id, uint64_t value, int width);

/**
 * @brief Close the VCD file and free all writer resources.
 *
 * @param w VCD writer.
 */
void vcd_close(VCDWriter *w);

#endif /* JZ_SIM_VCD_H */
