/**
 * @file sim_state.h
 * @brief Simulation context storage for DUT state, memories, and testbench bindings.
 */

#ifndef JZ_SIM_STATE_H
#define JZ_SIM_STATE_H

#include "sim_value.h"
#include "../../include/ir.h"
#include "../../include/diagnostic.h"

/**
 * @struct SimSignalEntry
 * @brief Stores the current and pending value for one simulated signal.
 */
typedef struct SimSignalEntry {
    int      signal_id;   /**< IR signal identifier for this slot. */
    SimValue current;     /**< Current committed signal value. */
    SimValue next;        /**< Staged non-blocking assignment value. */
    int      has_pending; /**< Non-zero when @ref next contains a pending update. */
} SimSignalEntry;

/**
 * @struct SimMemEntry
 * @brief Stores one simulated memory array.
 */
typedef struct SimMemEntry {
    int       mem_id;      /**< IR memory identifier for this entry. */
    SimValue *cells;       /**< Array of memory cells with @ref depth elements. */
    int       word_width;  /**< Width of each memory word in bits. */
    int       depth;       /**< Number of addressable words. */
} SimMemEntry;

/**
 * @struct SimPortMapping
 * @brief Precomputed parent-to-child or child-to-parent signal mapping.
 */
typedef struct SimPortMapping {
    SimSignalEntry *parent_entry; /**< Direct pointer into the parent context signal table. */
    SimSignalEntry *child_entry;  /**< Direct pointer into the child context signal table. */
    int             parent_msb;   /**< Parent slice MSB, or `-1` for a full-width mapping. */
    int             parent_lsb;   /**< Parent slice LSB, or `-1` for a full-width mapping. */
    int             child_width;  /**< Child port width used for width adjustment. */
    int             is_inout;     /**< Non-zero when the mapped child port is inout. */
} SimPortMapping;

/**
 * @struct SimAsyncChunk
 * @brief One dependency-analyzed asynchronous statement chunk.
 */
typedef struct SimAsyncChunk {
    const IR_Stmt *stmt;          /**< Statement executed for this chunk. */
    int           *read_indices;  /**< Signal-table indices read by @ref stmt. */
    int            num_reads;     /**< Number of entries in @ref read_indices. */
    int           *write_indices; /**< Signal-table indices written by @ref stmt. */
    int            num_writes;    /**< Number of entries in @ref write_indices. */
} SimAsyncChunk;

/**
 * @struct SimChildInstance
 * @brief Simulator state for one instantiated child module.
 */
typedef struct SimChildInstance {
    struct SimContext  *ctx;             /**< Child simulation context. */
    const IR_Instance  *inst;            /**< IR instance descriptor for this child. */
    const IR_Module    *child_module;    /**< IR module definition for the child. */
    SimPortMapping     *input_maps;      /**< Precomputed parent-to-child port mappings. */
    int                 num_input_maps;  /**< Number of entries in @ref input_maps. */
    SimPortMapping     *output_maps;     /**< Precomputed child-to-parent port mappings. */
    int                 num_output_maps; /**< Number of entries in @ref output_maps. */
} SimChildInstance;

/**
 * @struct SimContext
 * @brief Complete simulator state for one module instance.
 */
typedef struct SimContext {
    const IR_Module   *module;           /**< IR module currently being simulated. */
    const IR_Design   *design;           /**< Design database used for child lookup. */
    SimSignalEntry    *signals;          /**< Per-signal storage for this context. */
    int                num_signals;      /**< Number of entries in @ref signals. */
    int               *sig_id_map;       /**< Maps IR signal IDs to indices in @ref signals. */
    int                max_sig_id;       /**< Largest valid signal ID in @ref sig_id_map. */
    SimMemEntry       *memories;         /**< Simulated memories owned by this context. */
    int                num_memories;     /**< Number of entries in @ref memories. */
    SimChildInstance  *children;         /**< Child-instance simulator contexts. */
    int                num_children;     /**< Number of entries in @ref children. */
    uint32_t           rng_state;        /**< Internal xorshift32 state. */
    int                runtime_error;    /**< Non-zero after a runtime simulation error. */
    int                settle_dirty;     /**< Non-zero when a settle pass changed a signal. */
    SimAsyncChunk     *async_chunks;     /**< Dependency-analyzed async statement chunks. */
    int                num_async_chunks; /**< Number of entries in @ref async_chunks. */
    uint8_t           *sig_dirty;        /**< Per-signal dirty flags indexed like @ref signals. */
} SimContext;

/**
 * @struct SimTbWire
 * @brief One named testbench-visible wire value.
 */
typedef struct SimTbWire {
    const char *name;      /**< Wire name presented to the testbench. */
    SimValue    value;     /**< Current wire value. */
    int         width;     /**< Declared wire width in bits. */
    int         is_clock;  /**< Non-zero when the wire is driven as a clock. */
    int         owns_name; /**< Non-zero when @ref name must be freed by the owner. */
} SimTbWire;

/**
 * @struct SimPortBinding
 * @brief Binds one DUT port signal to one testbench wire slot.
 */
typedef struct SimPortBinding {
    int port_signal_id; /**< IR signal ID for the bound module port. */
    int tb_wire_index;  /**< Index of the bound wire in `tb_wires`. */
} SimPortBinding;

/**
 * @struct SimTestState
 * @brief Shared execution state used by simulator-driven testbench features.
 */
typedef struct SimTestState {
    SimTbWire       *tb_wires;           /**< Testbench wire table. */
    int              num_tb_wires;       /**< Number of entries in @ref tb_wires. */
    SimPortBinding  *bindings;           /**< Port-to-wire binding table. */
    int              num_bindings;       /**< Number of entries in @ref bindings. */
    SimContext      *dut;                /**< DUT simulation context. */
    int              verbose;            /**< Non-zero when verbose test output is enabled. */
    int              runtime_error;      /**< Non-zero after a testbench runtime error. */
    JZDiagnosticList *diagnostics;       /**< Diagnostic sink for testbench failures. */
    const char      *filename;           /**< Source file associated with the running test. */
    const char      *dut_instance_name;  /**< Instance name used when reporting DUT failures. */
    int              num_failed;         /**< Number of failed assertions or checks. */
    char           **failure_msgs;       /**< Collected failure message strings. */
    int              num_failure_msgs;   /**< Number of entries in @ref failure_msgs. */
    int              cap_failure_msgs;   /**< Allocated capacity of @ref failure_msgs. */
    uint64_t         current_time_ps;    /**< Current simulation time in picoseconds. */
    uint64_t         tick_ps;            /**< Tick size in picoseconds. */
    int              test_passed;        /**< Non-zero after the test declares success. */
    int              num_expects;        /**< Number of expected assertions evaluated. */
    int              num_passed;         /**< Number of passed assertions. */
    uint64_t         cycle_count;        /**< Number of executed simulation cycles. */
} SimTestState;

/**
 * @brief Create a simulation context for one IR module instance.
 * @param module IR module to instantiate.
 * @param design Design database used to resolve child instances.
 * @param rng_seed Initial seed for the simulator PRNG.
 * @return Newly allocated simulation context, or `NULL` on failure.
 */
SimContext *sim_ctx_create(const IR_Module *module, const IR_Design *design,
                           uint32_t rng_seed);
/**
 * @brief Destroy a simulation context and all owned allocations.
 * @param ctx Simulation context to destroy. Passing `NULL` is allowed.
 */
void       sim_ctx_destroy(SimContext *ctx);

/**
 * @brief Look up one signal entry by IR signal ID.
 * @param ctx Simulation context to search.
 * @param signal_id IR signal identifier to resolve.
 * @return Matching signal entry, or `NULL` when the ID is not present.
 */
SimSignalEntry *sim_ctx_lookup(SimContext *ctx, int signal_id);
/**
 * @brief Look up one memory entry by memory name.
 * @param ctx Simulation context to search.
 * @param name Memory name to match.
 * @return Matching memory entry, or `NULL` when no memory has that name.
 */
SimMemEntry    *sim_ctx_lookup_mem(SimContext *ctx, const char *name);
/**
 * @brief Commit all pending non-blocking assignments in a context.
 * @param ctx Simulation context whose staged updates should be applied.
 */
void            sim_ctx_apply_nba(SimContext *ctx);
/**
 * @brief Clear per-signal dirty flags and settle state for a context.
 * @param ctx Simulation context to reset.
 */
void            sim_ctx_clear_dirty(SimContext *ctx);

/**
 * @brief Advance the simulator xorshift32 pseudo-random generator.
 * @param state Mutable PRNG state value.
 * @return Next pseudo-random 32-bit value.
 */
uint32_t sim_rng_next(uint32_t *state);

#endif /* JZ_SIM_STATE_H */
