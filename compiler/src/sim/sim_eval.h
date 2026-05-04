/**
 * @file sim_eval.h
 * @brief IR expression evaluator for simulation.
 */

#ifndef JZ_SIM_EVAL_H
#define JZ_SIM_EVAL_H

#include "sim_state.h"

/**
 * @brief Evaluate an IR expression against the current simulation state.
 *
 * @param ctx Simulation context that supplies signal and memory state.
 * @param expr IR expression tree to evaluate.
 * @return Simulated value for the expression.
 */
SimValue sim_eval_expr(SimContext *ctx, const IR_Expr *expr);

#endif /* JZ_SIM_EVAL_H */
