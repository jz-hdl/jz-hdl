/**
 * @file sim_perf.h
 * @brief Performance tracking for the simulation engine.
 *
 * Enabled by compiling with -DTRACK_PERF.
 * Tracks wall-clock time and call counts for key simulation phases,
 * plus current/future state statistics (NBA pending, signal changes).
 */

#ifndef JZ_SIM_PERF_H
#define JZ_SIM_PERF_H

#ifdef TRACK_PERF

#include <stdint.h>
#include <time.h>

/* ---- Timer / counter IDs ---- */

/**
 * @brief Identifiers for instrumented simulation timers.
 */
typedef enum {
    PERF_PROPAGATE_INPUTS,     /**< Time spent copying testbench inputs into the DUT. */
    PERF_PROPAGATE_OUTPUTS,    /**< Time spent copying DUT outputs back to testbench wires. */
    PERF_SETTLE_COMBINATIONAL, /**< Time spent running delta-cycle settling. */
    PERF_SETTLE_HIERARCHY_ONCE,/**< Time spent in one hierarchical settle pass. */
    PERF_RESOLVE_INOUT_Z,      /**< Time spent restoring non-`z` values on inout nets. */
    PERF_FIRE_DOMAINS,         /**< Time spent dispatching matching clock domains. */
    PERF_EXEC_SYNC_DOMAIN,     /**< Time spent executing synchronous domain statements. */
    PERF_APPLY_NBA,            /**< Time spent applying queued non-blocking assignments. */
    PERF_EXEC_STMT,            /**< Count-only bucket for IR statement execution. */
    PERF_EVAL_EXPR,            /**< Count-only bucket for IR expression evaluation. */
    PERF_WAVEFORM_DUMP,        /**< Time spent writing waveform value changes. */
    PERF_FULL_SETTLE,          /**< Time spent in the full input-settle-output pipeline. */
    PERF_CLOCK_TOGGLE,         /**< Time spent processing simulation clock toggles. */
    PERF__COUNT                /**< Sentinel count; must remain last. */
} PerfTimerId;

/* ---- Per-timer accumulator ---- */

/**
 * @brief Accumulated wall-clock statistics for one timer bucket.
 */
typedef struct {
    uint64_t calls;    /**< Number of times the timer or counter was recorded. */
    uint64_t total_ns; /**< Total accumulated wall-clock time in nanoseconds. */
    uint64_t max_ns;   /**< Longest single recorded duration in nanoseconds. */
} PerfTimer;

/* ---- State tracking counters ---- */

/**
 * @brief Aggregate counters for key simulator state transitions.
 */
typedef struct {
    uint64_t nba_applies;       /**< Number of `sim_ctx_apply_nba()` calls. */
    uint64_t nba_pending_total; /**< Total pending-register count observed across NBA applies. */
    uint64_t nba_pending_max;   /**< Largest pending-register count seen in a single NBA apply. */
    uint64_t settle_iterations; /**< Total delta-cycle iterations executed. */
    uint64_t settle_max_iters;  /**< Largest iteration count seen in a single settle call. */
    uint64_t signals_changed;   /**< Total number of changed-state observations during settling. */
    uint64_t tick_count;        /**< Total simulation ticks processed. */
} PerfStateCounters;

/* ---- Global perf context ---- */

/**
 * @brief Global performance snapshot for one simulator run.
 */
typedef struct {
    PerfTimer timers[PERF__COUNT]; /**< Per-phase timer accumulators. */
    PerfStateCounters state;       /**< Cross-phase simulation state counters. */
} PerfContext;

/* Single global instance (defined in sim_perf.c) */
extern PerfContext g_perf;

/* ---- Timing helpers ---- */

/**
 * @brief Read the current monotonic time in nanoseconds.
 *
 * @return Current monotonic clock timestamp in nanoseconds.
 */
static inline uint64_t perf_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ---- Macros for instrumentation ---- */

#define PERF_TIMER_START(id) \
    uint64_t _perf_start_##id = perf_now_ns()

#define PERF_TIMER_STOP(id) \
    do { \
        uint64_t _perf_elapsed = perf_now_ns() - _perf_start_##id; \
        g_perf.timers[id].calls++; \
        g_perf.timers[id].total_ns += _perf_elapsed; \
        if (_perf_elapsed > g_perf.timers[id].max_ns) \
            g_perf.timers[id].max_ns = _perf_elapsed; \
    } while (0)

#define PERF_COUNT(id) \
    do { g_perf.timers[id].calls++; } while (0)

#define PERF_STATE_NBA_PENDING(n) \
    do { \
        g_perf.state.nba_applies++; \
        g_perf.state.nba_pending_total += (uint64_t)(n); \
        if ((uint64_t)(n) > g_perf.state.nba_pending_max) \
            g_perf.state.nba_pending_max = (uint64_t)(n); \
    } while (0)

#define PERF_STATE_SETTLE_ITER(iters, changed) \
    do { \
        g_perf.state.settle_iterations += (uint64_t)(iters); \
        if ((uint64_t)(iters) > g_perf.state.settle_max_iters) \
            g_perf.state.settle_max_iters = (uint64_t)(iters); \
        g_perf.state.signals_changed += (uint64_t)(changed); \
    } while (0)

#define PERF_STATE_TICK() \
    do { g_perf.state.tick_count++; } while (0)

/* ---- API ---- */

/**
 * @brief Clear all accumulated performance counters.
 */
void perf_reset(void);

/**
 * @brief Print the current performance summary to standard error.
 */
void perf_print_summary(void);

#else /* !TRACK_PERF */

/* No-op stubs when TRACK_PERF is not defined */
#define PERF_TIMER_START(id)              ((void)0)
#define PERF_TIMER_STOP(id)               ((void)0)
#define PERF_COUNT(id)                    ((void)0)
#define PERF_STATE_NBA_PENDING(n)         ((void)0)
#define PERF_STATE_SETTLE_ITER(iters, changed) ((void)0)
#define PERF_STATE_TICK()                 ((void)0)

/**
 * @brief No-op stub used when performance tracking is disabled.
 */
static inline void perf_reset(void) {}
/**
 * @brief No-op stub used when performance tracking is disabled.
 */
static inline void perf_print_summary(void) {}

#endif /* TRACK_PERF */

#endif /* JZ_SIM_PERF_H */
