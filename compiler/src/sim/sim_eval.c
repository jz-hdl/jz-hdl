/**
 * @file sim_eval.c
 * @brief Recursive IR expression evaluator for cycle-accurate simulation.
 */

#include "sim_eval.h"
#include "sim_perf.h"
#include <string.h>
#include <stdio.h>

static int sim_value_to_index(SimValue v, int *out);
static SimValue sim_value_from_index(int value, int width);
static SimValue sim_value_abs(SimValue src, int result_w);
static SimValue sim_value_signed_choice(SimValue a, SimValue b, int pick_min,
                                        int result_w);

static int sim_value_to_index(SimValue v, int *out) {
    int nw = (v.width + 63) / 64;
    uint64_t raw = 0;
    if (nw <= 0) nw = 1;
    if (sim_val_has_xz(v)) return -1;
    for (int i = 1; i < nw; i++) {
        if (v.val[i] != 0) return -1;
    }
    raw = v.val[0];
    if (raw > (uint64_t)INT32_MAX) return -1;
    *out = (int)raw;
    return 0;
}

static SimValue sim_value_from_index(int value, int width) {
    SimValue r = sim_val_zero(width);
    uint32_t raw = (uint32_t)value;
    int bit = 0;
    while (raw != 0) {
        if (raw & 1U) sim_val_set_bit(&r, bit, 1);
        raw >>= 1;
        bit++;
    }
    return sim_val_mask(r);
}

static SimValue sim_value_abs(SimValue src, int result_w) {
    int src_w = src.width > 0 ? src.width : 1;
    SimValue magnitude = sim_val_get_bit(src, src_w - 1) ? sim_val_neg(src) : src;
    magnitude = sim_val_zext(magnitude, result_w > src_w ? result_w : src_w);
    magnitude.width = result_w;
    magnitude = sim_val_mask(magnitude);

    if (sim_val_get_bit(src, src_w - 1)) {
        SimValue most_neg = sim_val_zero(src_w);
        sim_val_set_bit(&most_neg, src_w - 1, 1);
        if (sim_val_equal(sim_val_mask(src), most_neg) && result_w > 0)
            sim_val_set_bit(&magnitude, result_w - 1, 1);
    }
    return sim_val_mask(magnitude);
}

static SimValue sim_value_signed_choice(SimValue a, SimValue b, int pick_min,
                                        int result_w)
{
    int width = a.width > b.width ? a.width : b.width;
    int sign_a = sim_val_get_bit(a, a.width - 1);
    int sign_b = sim_val_get_bit(b, b.width - 1);
    int pick_a = 0;

    a = sim_val_sext(a, width);
    b = sim_val_sext(b, width);

    if (sign_a != sign_b) {
        pick_a = pick_min ? sign_a : !sign_a;
    } else {
        int a_lt_b = sim_val_is_true(sim_val_lt(a, b)) == 1;
        pick_a = pick_min ? a_lt_b : !a_lt_b;
    }

    if (pick_a) {
        a.width = result_w;
        return sim_val_mask(a);
    }
    b.width = result_w;
    return sim_val_mask(b);
}

SimValue sim_eval_expr(SimContext *ctx, const IR_Expr *expr) {
    if (!expr) return sim_val_all_x(1);
    PERF_COUNT(PERF_EVAL_EXPR);

    switch (expr->kind) {

    case EXPR_LITERAL: {
        SimValue v = sim_val_from_words(expr->u.literal.literal.words,
                                         IR_LIT_WORDS,
                                         expr->u.literal.literal.width);
        if (expr->u.literal.literal.is_z)
            v = sim_val_all_z(expr->u.literal.literal.width);
        if (v.width == 0) v.width = expr->width > 0 ? expr->width : 1;
        return v;
    }

    case EXPR_SIGNAL_REF: {
        SimSignalEntry *e = sim_ctx_lookup(ctx, expr->u.signal_ref.signal_id);
        if (!e) return sim_val_all_x(expr->width > 0 ? expr->width : 1);
        return e->current;
    }

    /* ---- Unary ---- */
    case EXPR_UNARY_NOT: {
        SimValue op = sim_eval_expr(ctx, expr->u.unary.operand);
        SimValue r = sim_val_not(op);
        r.width = expr->width > 0 ? expr->width : r.width;
        return r;
    }
    case EXPR_UNARY_NEG: {
        SimValue op = sim_eval_expr(ctx, expr->u.unary.operand);
        SimValue r = sim_val_neg(op);
        r.width = expr->width > 0 ? expr->width : r.width;
        return r;
    }
    case EXPR_LOGICAL_NOT: {
        SimValue op = sim_eval_expr(ctx, expr->u.unary.operand);
        return sim_val_logical_not(op);
    }

    /* ---- Binary arithmetic ---- */
    case EXPR_BINARY_ADD: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        SimValue res = sim_val_add(l, r);
        res.width = expr->width > 0 ? expr->width : res.width;
        return sim_val_mask(res);
    }
    case EXPR_BINARY_SUB: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        SimValue res = sim_val_sub(l, r);
        res.width = expr->width > 0 ? expr->width : res.width;
        return sim_val_mask(res);
    }
    case EXPR_BINARY_MUL: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        SimValue res = sim_val_mul(l, r);
        res.width = expr->width > 0 ? expr->width : res.width;
        return sim_val_mask(res);
    }
    case EXPR_BINARY_DIV: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        SimValue res = sim_val_div(l, r);
        res.width = expr->width > 0 ? expr->width : res.width;
        return sim_val_mask(res);
    }
    case EXPR_BINARY_MOD: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        SimValue res = sim_val_mod(l, r);
        res.width = expr->width > 0 ? expr->width : res.width;
        return sim_val_mask(res);
    }

    /* ---- Bitwise ---- */
    case EXPR_BINARY_AND: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        SimValue res = sim_val_and(l, r);
        res.width = expr->width > 0 ? expr->width : res.width;
        return res;
    }
    case EXPR_BINARY_OR: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        SimValue res = sim_val_or(l, r);
        res.width = expr->width > 0 ? expr->width : res.width;
        return res;
    }
    case EXPR_BINARY_XOR: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        SimValue res = sim_val_xor(l, r);
        res.width = expr->width > 0 ? expr->width : res.width;
        return res;
    }

    /* ---- Shifts ---- */
    case EXPR_BINARY_SHL: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        SimValue res = sim_val_shl(l, r);
        res.width = expr->width > 0 ? expr->width : res.width;
        return sim_val_mask(res);
    }
    case EXPR_BINARY_SHR: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        SimValue res = sim_val_shr(l, r);
        res.width = expr->width > 0 ? expr->width : res.width;
        return sim_val_mask(res);
    }
    case EXPR_BINARY_ASHR: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        SimValue res = sim_val_ashr(l, r);
        res.width = expr->width > 0 ? expr->width : res.width;
        return sim_val_mask(res);
    }

    /* ---- Comparisons ---- */
    case EXPR_BINARY_EQ: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        return sim_val_eq(l, r);
    }
    case EXPR_BINARY_NEQ: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        return sim_val_neq(l, r);
    }
    case EXPR_BINARY_LT: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        return sim_val_lt(l, r);
    }
    case EXPR_BINARY_GT: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        return sim_val_gt(l, r);
    }
    case EXPR_BINARY_LTE: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        return sim_val_lte(l, r);
    }
    case EXPR_BINARY_GTE: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        return sim_val_gte(l, r);
    }

    /* ---- Logical ---- */
    case EXPR_LOGICAL_AND: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        return sim_val_logical_and(l, r);
    }
    case EXPR_LOGICAL_OR: {
        SimValue l = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue r = sim_eval_expr(ctx, expr->u.binary.right);
        return sim_val_logical_or(l, r);
    }

    /* ---- Ternary ---- */
    case EXPR_TERNARY: {
        SimValue cond = sim_eval_expr(ctx, expr->u.ternary.condition);
        SimValue tv = sim_eval_expr(ctx, expr->u.ternary.true_val);
        SimValue fv = sim_eval_expr(ctx, expr->u.ternary.false_val);
        /* z in ternary condition is a runtime error (SE-008).
         * x is not a runtime value; z in a non-tristate expression
         * means z leaked past compile-time structural checks. */
        if (sim_val_has_xz(cond) && !ctx->runtime_error) {
            ctx->runtime_error = 1;
            fprintf(stderr, "RUNTIME ERROR: z reached ternary condition "
                    "(SE-008)\n");
        }
        return sim_val_ternary(cond, tv, fv);
    }

    /* ---- Concat ---- */
    case EXPR_CONCAT: {
        int n = expr->u.concat.num_operands;
        SimValue parts[64];
        if (n > 64) n = 64;
        for (int i = 0; i < n; i++)
            parts[i] = sim_eval_expr(ctx, expr->u.concat.operands[i]);
        return sim_val_concat(parts, n);
    }

    /* ---- Slice ---- */
    case EXPR_SLICE: {
        SimValue base;
        if (expr->u.slice.base_expr) {
            base = sim_eval_expr(ctx, expr->u.slice.base_expr);
        } else {
            SimSignalEntry *e = sim_ctx_lookup(ctx, expr->u.slice.signal_id);
            if (!e) return sim_val_all_x(expr->width > 0 ? expr->width : 1);
            base = e->current;
        }
        return sim_val_slice(base, expr->u.slice.msb, expr->u.slice.lsb);
    }

    /* ---- Memory read ---- */
    case EXPR_MEM_READ: {
        SimMemEntry *me = sim_ctx_lookup_mem(ctx, expr->u.mem_read.memory_name);
        if (!me) return sim_val_all_x(expr->width > 0 ? expr->width : 1);
        SimValue addr = sim_eval_expr(ctx, expr->u.mem_read.address);
        int idx = 0;
        if (sim_val_has_xz(addr)) return sim_val_all_x(me->word_width);
        if (sim_value_to_index(addr, &idx) != 0 || idx < 0 || idx >= me->depth)
            return sim_val_all_x(me->word_width);
        return me->cells[idx];
    }

    /* ---- Intrinsics ---- */
    case EXPR_INTRINSIC_UADD: {
        SimValue src = sim_eval_expr(ctx, expr->u.intrinsic.source);
        SimValue idx = sim_eval_expr(ctx, expr->u.intrinsic.index);
        SimValue res = sim_val_add(src, idx);
        res.width = expr->width > 0 ? expr->width : res.width;
        return sim_val_mask(res);
    }
    case EXPR_INTRINSIC_SADD: {
        SimValue src = sim_eval_expr(ctx, expr->u.intrinsic.source);
        SimValue idx = sim_eval_expr(ctx, expr->u.intrinsic.index);
        SimValue res = sim_val_add(src, idx);
        res.width = expr->width > 0 ? expr->width : res.width;
        return sim_val_mask(res);
    }
    case EXPR_INTRINSIC_UMUL: {
        SimValue src = sim_eval_expr(ctx, expr->u.intrinsic.source);
        SimValue idx = sim_eval_expr(ctx, expr->u.intrinsic.index);
        SimValue res = sim_val_mul(src, idx);
        res.width = expr->width > 0 ? expr->width : res.width;
        return sim_val_mask(res);
    }
    case EXPR_INTRINSIC_SMUL: {
        SimValue src = sim_eval_expr(ctx, expr->u.intrinsic.source);
        SimValue idx = sim_eval_expr(ctx, expr->u.intrinsic.index);
        SimValue res = sim_val_mul(src, idx);
        res.width = expr->width > 0 ? expr->width : res.width;
        return sim_val_mask(res);
    }
    case EXPR_INTRINSIC_GBIT: {
        SimValue src = sim_eval_expr(ctx, expr->u.intrinsic.source);
        SimValue idx = sim_eval_expr(ctx, expr->u.intrinsic.index);
        int bit = 0;
        if (sim_val_has_xz(idx)) return sim_val_all_x(1);
        if (sim_value_to_index(idx, &bit) != 0) return sim_val_zero(1);
        if (bit < 0 || bit >= src.width) return sim_val_zero(1);
        return sim_val_slice(src, bit, bit);
    }
    case EXPR_INTRINSIC_SBIT: {
        SimValue src = sim_eval_expr(ctx, expr->u.intrinsic.source);
        SimValue idx = sim_eval_expr(ctx, expr->u.intrinsic.index);
        SimValue val = sim_eval_expr(ctx, expr->u.intrinsic.value);
        int bit = 0;
        if (sim_val_has_xz(idx)) return sim_val_all_x(src.width);
        if (sim_value_to_index(idx, &bit) != 0) return src;
        if (bit < 0 || bit >= src.width) return src;
        {
            int wi = bit / 64;
            int bi = bit % 64;
            uint64_t mask = (uint64_t)1 << bi;
            SimValue one_bit = sim_val_slice(val, 0, 0);
            src.val[wi] = (src.val[wi] & ~mask) |
                          ((uint64_t)sim_val_get_bit(one_bit, 0) << bi);
            src.xmask[wi] = (src.xmask[wi] & ~mask) |
                            (((one_bit.xmask[0] & 1U) ? UINT64_C(1) : UINT64_C(0)) << bi);
            src.zmask[wi] = (src.zmask[wi] & ~mask) |
                            (((one_bit.zmask[0] & 1U) ? UINT64_C(1) : UINT64_C(0)) << bi);
        }
        return src;
    }
    case EXPR_INTRINSIC_GSLICE: {
        SimValue src = sim_eval_expr(ctx, expr->u.intrinsic.source);
        SimValue idx = sim_eval_expr(ctx, expr->u.intrinsic.index);
        int ew = expr->u.intrinsic.element_width;
        int elem_idx = 0;
        if (ew <= 0) ew = expr->width > 0 ? expr->width : 1;
        if (sim_val_has_xz(idx))
            return sim_val_all_x(ew);
        /* Index is an element index; multiply by element width for bit offset */
        if (sim_value_to_index(idx, &elem_idx) != 0) return sim_val_all_x(ew);
        int lo = elem_idx * ew;
        int hi = lo + ew - 1;
        if (lo < 0 || hi >= src.width)
            return sim_val_all_x(ew);
        return sim_val_slice(src, hi, lo);
    }
    case EXPR_INTRINSIC_SSLICE: {
        SimValue src = sim_eval_expr(ctx, expr->u.intrinsic.source);
        SimValue idx = sim_eval_expr(ctx, expr->u.intrinsic.index);
        SimValue val = sim_eval_expr(ctx, expr->u.intrinsic.value);
        int lo = 0;
        if (sim_val_has_xz(idx)) return sim_val_all_x(src.width);
        if (sim_value_to_index(idx, &lo) != 0) return src;
        int ew = expr->u.intrinsic.element_width;
        int hi = lo + ew - 1;
        if (lo < 0 || hi >= src.width) return src;
        /* Use slice-based approach for multi-word support */
        SimValue slice_val = sim_val_slice(val, ew - 1, 0);
        /* Build result by shifting slice into position */
        SimValue shift_amt = sim_val_from_uint((uint64_t)lo, 32);
        SimValue shifted_val = sim_val_shl(sim_val_zext(slice_val, src.width), shift_amt);
        /* Build position mask */
        SimValue ones_ew = sim_val_ones(ew);
        SimValue shifted_mask = sim_val_shl(sim_val_zext(ones_ew, src.width), shift_amt);
        SimValue inv_mask = sim_val_not(shifted_mask);
        SimValue cleared = sim_val_and(src, inv_mask);
        return sim_val_or(cleared, shifted_val);
    }

    case EXPR_INTRINSIC_OH2B: {
        SimValue src = sim_eval_expr(ctx, expr->u.intrinsic.source);
        if (sim_val_has_xz(src)) return sim_val_all_x(expr->width > 0 ? expr->width : 1);
        int result_w = expr->width > 0 ? expr->width : 1;
        int result = 0;
        for (int i = 0; i < src.width; i++) {
            if (sim_val_get_bit(src, i)) result = i;
        }
        return sim_value_from_index(result, result_w);
    }

    case EXPR_INTRINSIC_B2OH: {
        SimValue idx = sim_eval_expr(ctx, expr->u.intrinsic.source);
        int bit = 0;
        if (sim_val_has_xz(idx)) return sim_val_all_x(expr->width > 0 ? expr->width : 1);
        int result_w = expr->width > 0 ? expr->width : 1;
        if (sim_value_to_index(idx, &bit) == 0 && bit >= 0 && bit < result_w) {
            SimValue r = sim_val_zero(result_w);
            sim_val_set_bit(&r, bit, 1);
            return r;
        }
        return sim_val_zero(result_w);
    }

    case EXPR_INTRINSIC_PRIENC: {
        SimValue src = sim_eval_expr(ctx, expr->u.intrinsic.source);
        if (sim_val_has_xz(src)) return sim_val_all_x(expr->width > 0 ? expr->width : 1);
        int result_w = expr->width > 0 ? expr->width : 1;
        int result = 0;
        for (int i = src.width - 1; i >= 0; i--) {
            if (sim_val_get_bit(src, i)) { result = i; break; }
        }
        return sim_value_from_index(result, result_w);
    }

    case EXPR_INTRINSIC_LZC: {
        SimValue src = sim_eval_expr(ctx, expr->u.intrinsic.source);
        if (sim_val_has_xz(src)) return sim_val_all_x(expr->width > 0 ? expr->width : 1);
        int result_w = expr->width > 0 ? expr->width : 1;
        int count = src.width;
        for (int i = src.width - 1; i >= 0; i--) {
            if (sim_val_get_bit(src, i)) { count = src.width - 1 - i; break; }
        }
        return sim_value_from_index(count, result_w);
    }

    case EXPR_INTRINSIC_USUB:
    case EXPR_INTRINSIC_SSUB: {
        SimValue a = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue b = sim_eval_expr(ctx, expr->u.binary.right);
        SimValue res = sim_val_sub(a, b);
        res.width = expr->width > 0 ? expr->width : res.width;
        return sim_val_mask(res);
    }

    case EXPR_INTRINSIC_ABS: {
        SimValue src = sim_eval_expr(ctx, expr->u.intrinsic.source);
        if (sim_val_has_xz(src)) return sim_val_all_x(expr->width > 0 ? expr->width : 1);
        int result_w = expr->width > 0 ? expr->width : 1;
        return sim_value_abs(src, result_w);
    }

    case EXPR_INTRINSIC_UMIN:
    case EXPR_INTRINSIC_UMAX: {
        SimValue a = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue b = sim_eval_expr(ctx, expr->u.binary.right);
        if (sim_val_has_xz(a) || sim_val_has_xz(b))
            return sim_val_all_x(expr->width > 0 ? expr->width : 1);
        int result_w = expr->width > 0 ? expr->width : 1;
        {
            int a_lt_b = sim_val_is_true(sim_val_lt(a, b)) == 1;
            SimValue chosen = ((expr->kind == EXPR_INTRINSIC_UMIN) ? a_lt_b : !a_lt_b) ? a : b;
            chosen.width = result_w;
            return sim_val_mask(chosen);
        }
    }

    case EXPR_INTRINSIC_SMIN:
    case EXPR_INTRINSIC_SMAX: {
        SimValue a = sim_eval_expr(ctx, expr->u.binary.left);
        SimValue b = sim_eval_expr(ctx, expr->u.binary.right);
        if (sim_val_has_xz(a) || sim_val_has_xz(b))
            return sim_val_all_x(expr->width > 0 ? expr->width : 1);
        int result_w = expr->width > 0 ? expr->width : 1;
        return sim_value_signed_choice(a, b,
                                       expr->kind == EXPR_INTRINSIC_SMIN,
                                       result_w);
    }

    case EXPR_INTRINSIC_POPCOUNT: {
        SimValue src = sim_eval_expr(ctx, expr->u.intrinsic.source);
        if (sim_val_has_xz(src)) return sim_val_all_x(expr->width > 0 ? expr->width : 1);
        int result_w = expr->width > 0 ? expr->width : 1;
        int count = 0;
        for (int i = 0; i < src.width; i++) {
            if (sim_val_get_bit(src, i)) count++;
        }
        return sim_value_from_index(count, result_w);
    }

    case EXPR_INTRINSIC_REVERSE: {
        SimValue src = sim_eval_expr(ctx, expr->u.intrinsic.source);
        if (sim_val_has_xz(src)) return sim_val_all_x(expr->width > 0 ? expr->width : 1);
        int w = src.width > 0 ? src.width : 1;
        SimValue result = sim_val_zero(w);
        for (int i = 0; i < w; i++) {
            if (sim_val_get_bit(src, i))
                sim_val_set_bit(&result, w - 1 - i, 1);
        }
        return result;
    }

    case EXPR_INTRINSIC_BSWAP: {
        SimValue src = sim_eval_expr(ctx, expr->u.intrinsic.source);
        if (sim_val_has_xz(src)) return sim_val_all_x(expr->width > 0 ? expr->width : 1);
        int w = src.width > 0 ? src.width : 8;
        int num_bytes = w / 8;
        SimValue result = sim_val_zero(w);
        for (int i = 0; i < num_bytes; i++) {
            for (int b = 0; b < 8; b++) {
                if (sim_val_get_bit(src, i * 8 + b))
                    sim_val_set_bit(&result, (num_bytes - 1 - i) * 8 + b, 1);
            }
        }
        return result;
    }

    case EXPR_INTRINSIC_REDUCE_AND: {
        SimValue src = sim_eval_expr(ctx, expr->u.intrinsic.source);
        if (sim_val_has_xz(src)) return sim_val_all_x(1);
        /* Check if all bits are 1 */
        SimValue ones = sim_val_ones(src.width);
        return sim_val_eq(src, ones);
    }

    case EXPR_INTRINSIC_REDUCE_OR: {
        SimValue src = sim_eval_expr(ctx, expr->u.intrinsic.source);
        if (sim_val_has_xz(src)) return sim_val_all_x(1);
        SimValue zero = sim_val_zero(src.width);
        return sim_val_neq(src, zero);
    }

    case EXPR_INTRINSIC_REDUCE_XOR: {
        SimValue src = sim_eval_expr(ctx, expr->u.intrinsic.source);
        if (sim_val_has_xz(src)) return sim_val_all_x(1);
        int w = src.width > 0 ? src.width : 1;
        uint64_t result = 0;
        for (int i = 0; i < w; i++) {
            result ^= (uint64_t)sim_val_get_bit(src, i);
        }
        return sim_val_from_uint(result, 1);
    }

    } /* end switch */

    /* Unreachable for known expression kinds */
    return sim_val_all_x(expr->width > 0 ? expr->width : 1);
}
