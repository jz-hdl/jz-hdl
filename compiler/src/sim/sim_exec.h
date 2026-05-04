/**
 * @file sim_exec.h
 * @brief IR statement executor for simulation.
 */

#ifndef JZ_SIM_EXEC_H
#define JZ_SIM_EXEC_H

#include "sim_state.h"

/** Maximum delta cycles for combinational settling before reporting
 *  a combinational loop runtime error (SE-001). */
#define SIM_SETTLE_MAX_ITERS 100

/**
 * @brief Execute an IR statement.
 *
 * @param ctx Simulation context to mutate.
 * @param stmt IR statement to execute.
 * @param is_nba Non-zero to queue register writes in NBA state, zero for immediate updates.
 */
void sim_exec_stmt(SimContext *ctx, const IR_Stmt *stmt, int is_nba);

/**
 * @brief Repeatedly evaluate asynchronous logic until the hierarchy quiesces.
 *
 * @param ctx Root simulation context to settle.
 * @param max_iters Maximum delta-cycle iterations to attempt before reporting oscillation.
 * @return `0` when settling converges, or `-1` when the iteration limit is reached.
 */
int sim_settle_combinational(SimContext *ctx, int max_iters);

/**
 * @brief Execute one synchronous clock domain in NBA mode.
 *
 * @param ctx Simulation context that owns the clock domain.
 * @param domain_idx Index of the clock domain in `ctx->module->clock_domains`.
 */
void sim_exec_sync_domain(SimContext *ctx, int domain_idx);

/**
 * @brief Execute a synchronous clock domain and matching child domains.
 *
 * @param ctx Simulation context that owns the parent clock domain.
 * @param domain_idx Index of the parent clock domain in `ctx->module->clock_domains`.
 * @param reset_active Non-zero when the parent domain is currently being held in reset.
 */
void sim_exec_sync_domain_with_children(SimContext *ctx, int domain_idx,
                                        int reset_active);

#endif /* JZ_SIM_EXEC_H */
