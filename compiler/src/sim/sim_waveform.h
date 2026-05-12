/**
 * @file sim_waveform.h
 * @brief Backend-neutral waveform writer interface.
 */

#ifndef JZ_SIM_WAVEFORM_H
#define JZ_SIM_WAVEFORM_H

#include <stdint.h>
#include "sim_value.h"

/**
 * @enum SimWaveFormat
 * @brief Waveform backend formats supported by @ref SimWaveWriter.
 */
typedef enum {
    SIM_WAVE_VCD = 0, /**< Write IEEE VCD text traces. */
    SIM_WAVE_FST = 1, /**< Write GTKWave FST traces. */
    SIM_WAVE_JZW = 2  /**< Write JZ-HDL SQLite waveform traces. */
} SimWaveFormat;

/**
 * @struct SimWaveWriter
 * @brief Opaque wrapper around one concrete waveform backend.
 */
typedef struct SimWaveWriter SimWaveWriter;

/**
 * @brief Open a waveform writer for the requested backend format.
 * @param filename Output file path.
 * @param timescale_ps Trace timescale in picoseconds.
 * @param format Backend format to create.
 * @return Waveform writer handle, or `NULL` on failure.
 */
SimWaveWriter *sim_wave_open(const char *filename, uint64_t timescale_ps,
                             SimWaveFormat format);

/**
 * @brief Add a metadata key/value pair to the trace.
 * @param w Waveform writer.
 * @param key Metadata key string.
 * @param value Metadata value string.
 */
void sim_wave_set_meta(SimWaveWriter *w, const char *key, const char *value);

/**
 * @brief Record one clock description for backends that support it.
 * @param w Waveform writer.
 * @param name Clock signal name.
 * @param period_ps Nominal clock period in picoseconds.
 * @param phase_ps Initial phase offset in picoseconds.
 * @param jitter_pp_ps Peak-to-peak jitter in picoseconds.
 * @param jitter_sigma_ps Jitter sigma in picoseconds.
 * @param drift_max_ppm Maximum drift in parts per million.
 * @param drift_actual_ppm Actual applied drift in parts per million.
 * @param drifted_period_ps Effective drifted period in picoseconds.
 */
void sim_wave_add_clock(SimWaveWriter *w, const char *name, uint64_t period_ps,
                        uint64_t phase_ps, uint64_t jitter_pp_ps,
                        double jitter_sigma_ps, double drift_max_ppm,
                        double drift_actual_ppm, double drifted_period_ps);

/**
 * @brief Register a signal in the waveform definition section.
 * @param w Waveform writer.
 * @param scope Hierarchical scope name.
 * @param name Signal name within the scope.
 * @param width Signal width in bits.
 * @param type Signal classification string used by JZW backends.
 * @return Zero-based signal ID, or `-1` on failure.
 */
int sim_wave_add_signal(SimWaveWriter *w, const char *scope, const char *name,
                        int width, const char *type);

/**
 * @brief Finalize signal definitions before time/value emission begins.
 * @param w Waveform writer.
 */
void sim_wave_end_definitions(SimWaveWriter *w);

/**
 * @brief Set the current simulation timestamp for subsequent value dumps.
 * @param w Waveform writer.
 * @param time_ps Current simulation time in picoseconds.
 */
void sim_wave_set_time(SimWaveWriter *w, uint64_t time_ps);

/**
 * @brief Emit a value change for one signal.
 * @param w Waveform writer.
 * @param sig_id Signal identifier returned by sim_wave_add_signal().
 * @param value Full simulator value to encode.
 */
void sim_wave_dump_value(SimWaveWriter *w, int sig_id, SimValue value);

/**
 * @brief Record an annotation event for backends that support annotations.
 * @param w Waveform writer.
 * @param time_ps Annotation start time in picoseconds.
 * @param type Annotation type string.
 * @param signal_id Related signal ID, or a backend-specific global marker.
 * @param message Annotation message text.
 * @param color Annotation color name.
 * @param end_time Annotation end time in picoseconds.
 */
void sim_wave_add_annotation(SimWaveWriter *w, uint64_t time_ps,
                             const char *type, int signal_id,
                             const char *message, const char *color,
                             uint64_t end_time);

/**
 * @brief Close the writer and release its backend resources.
 * @param w Waveform writer to close. Passing `NULL` is allowed.
 */
void sim_wave_close(SimWaveWriter *w);

#endif /* JZ_SIM_WAVEFORM_H */
