/**
 * @file sim_value.h
 * @brief Four-state bit-vector values used by the simulator.
 *
 * Encoding per bit:
 *   val=0, xmask=0, zmask=0 -> logic 0
 *   val=1, xmask=0, zmask=0 -> logic 1
 *   xmask=1                 -> x (unknown)
 *   zmask=1                 -> z (high-impedance)
 *
 * Maximum supported width: 256 bits (SIM_VAL_WORDS * 64).
 */

#ifndef JZ_SIM_VALUE_H
#define JZ_SIM_VALUE_H

#include <stdint.h>

/** Number of 64-bit words per SimValue (supports up to 256 bits). */
#define SIM_VAL_WORDS 4

/**
 * @struct SimValue
 * @brief Fixed-capacity four-state bit-vector value.
 */
typedef struct SimValue {
    uint64_t val[SIM_VAL_WORDS];   /**< Packed known 0/1 bits, least-significant word first. */
    uint64_t xmask[SIM_VAL_WORDS]; /**< Per-bit unknown-state mask. */
    uint64_t zmask[SIM_VAL_WORDS]; /**< Per-bit high-impedance-state mask. */
    int      width;                /**< Logical value width in bits. */
} SimValue;

/**
 * @brief Construct an all-zero value of the requested width.
 * @param width Logical width in bits.
 * @return Zero-initialized simulator value.
 */
SimValue sim_val_zero(int width);
/**
 * @brief Construct an all-one value of the requested width.
 * @param width Logical width in bits.
 * @return Value whose active bits are all set to `1`.
 */
SimValue sim_val_ones(int width);
/**
 * @brief Construct a value from a 64-bit unsigned integer.
 * @param v Source integer bits.
 * @param width Logical width in bits.
 * @return Simulator value containing the low-width bits of @p v.
 */
SimValue sim_val_from_uint(uint64_t v, int width);
/**
 * @brief Construct a value from raw 64-bit words.
 * @param words Source word array in least-significant-word-first order.
 * @param num_words Number of words available in @p words.
 * @param width Logical width in bits.
 * @return Simulator value masked down to @p width bits.
 */
SimValue sim_val_from_words(const uint64_t *words, int num_words, int width);
/**
 * @brief Construct an all-unknown value of the requested width.
 * @param width Logical width in bits.
 * @return Simulator value whose active bits are all `x`.
 */
SimValue sim_val_all_x(int width);
/**
 * @brief Construct an all-high-impedance value of the requested width.
 * @param width Logical width in bits.
 * @return Simulator value whose active bits are all `z`.
 */
SimValue sim_val_all_z(int width);

/**
 * @brief Clear any bits stored above the logical width of a value.
 * @param v Value to normalize.
 * @return Copy of @p v with inactive bits cleared.
 */
SimValue sim_val_mask(SimValue v);

/**
 * @brief Read one value bit without consulting x/z masks.
 * @param v Value to inspect.
 * @param bit Zero-based bit index.
 * @return Bit value `0` or `1`, or `0` when the index is out of range.
 */
int sim_val_get_bit(SimValue v, int bit);
/**
 * @brief Write one value bit without changing x/z masks.
 * @param v Mutable value to update.
 * @param bit Zero-based bit index.
 * @param b Non-zero to set the bit, or zero to clear it.
 */
void sim_val_set_bit(SimValue *v, int bit, int b);

/**
 * @brief Test whether any active bit is `x` or `z`.
 * @param v Value to inspect.
 * @return Non-zero when the value contains any unknown or high-impedance bits.
 */
int sim_val_has_xz(SimValue v);
/**
 * @brief Test whether every active bit is `z`.
 * @param v Value to inspect.
 * @return Non-zero when all active bits are high impedance.
 */
int sim_val_is_all_z(SimValue v);
/**
 * @brief Evaluate a value in logical truth context.
 * @param v Value to inspect.
 * @return `1` for true, `0` for false, or `-1` when any active bit is `x`/`z`.
 */
int sim_val_is_true(SimValue v);
/**
 * @brief Compare two values exactly, including x/z masks.
 * @param a Left operand.
 * @param b Right operand.
 * @return Non-zero when both values have identical width, bits, and masks.
 */
int sim_val_equal(SimValue a, SimValue b);

/**
 * @brief Add two values using unsigned low-word arithmetic.
 * @param a Left operand.
 * @param b Right operand.
 * @return Sum with result width equal to the wider operand, or all `x` if either input contains `x`/`z`.
 */
SimValue sim_val_add(SimValue a, SimValue b);
/**
 * @brief Subtract one value from another using unsigned low-word arithmetic.
 * @param a Left operand.
 * @param b Right operand.
 * @return Difference with result width equal to the wider operand, or all `x` if either input contains `x`/`z`.
 */
SimValue sim_val_sub(SimValue a, SimValue b);
/**
 * @brief Multiply two values using unsigned low-word arithmetic.
 * @param a Left operand.
 * @param b Right operand.
 * @return Product with result width equal to the wider operand, or all `x` if either input contains `x`/`z`.
 */
SimValue sim_val_mul(SimValue a, SimValue b);
/**
 * @brief Divide one value by another using unsigned low-word arithmetic.
 * @param a Dividend.
 * @param b Divisor.
 * @return Quotient with result width equal to the wider operand, or all `x` for x/z inputs or division by zero.
 */
SimValue sim_val_div(SimValue a, SimValue b);
/**
 * @brief Compute the remainder of unsigned low-word division.
 * @param a Dividend.
 * @param b Divisor.
 * @return Remainder with result width equal to the wider operand, or all `x` for x/z inputs or division by zero.
 */
SimValue sim_val_mod(SimValue a, SimValue b);
/**
 * @brief Two's-complement negate a value using low-word arithmetic.
 * @param a Operand to negate.
 * @return Negated value, or all `x` when the input contains `x`/`z`.
 */
SimValue sim_val_neg(SimValue a);

/**
 * @brief Compute bitwise AND with four-state propagation rules.
 * @param a Left operand.
 * @param b Right operand.
 * @return Bitwise AND result.
 */
SimValue sim_val_and(SimValue a, SimValue b);
/**
 * @brief Compute bitwise OR with four-state propagation rules.
 * @param a Left operand.
 * @param b Right operand.
 * @return Bitwise OR result.
 */
SimValue sim_val_or(SimValue a, SimValue b);
/**
 * @brief Compute bitwise XOR with four-state propagation rules.
 * @param a Left operand.
 * @param b Right operand.
 * @return Bitwise XOR result.
 */
SimValue sim_val_xor(SimValue a, SimValue b);
/**
 * @brief Compute bitwise NOT with four-state propagation rules.
 * @param a Operand to invert.
 * @return Bitwise complement of @p a.
 */
SimValue sim_val_not(SimValue a);

/**
 * @brief Compare two values for equality.
 * @param a Left operand.
 * @param b Right operand.
 * @return One-bit true/false result, or `x` when either input contains `x`/`z`.
 */
SimValue sim_val_eq(SimValue a, SimValue b);
/**
 * @brief Compare two values for inequality.
 * @param a Left operand.
 * @param b Right operand.
 * @return One-bit true/false result, or `x` when either input contains `x`/`z`.
 */
SimValue sim_val_neq(SimValue a, SimValue b);
/**
 * @brief Compare whether one value is less than another.
 * @param a Left operand.
 * @param b Right operand.
 * @return One-bit true/false result, or `x` when either input contains `x`/`z`.
 */
SimValue sim_val_lt(SimValue a, SimValue b);
/**
 * @brief Compare whether one value is greater than another.
 * @param a Left operand.
 * @param b Right operand.
 * @return One-bit true/false result, or `x` when either input contains `x`/`z`.
 */
SimValue sim_val_gt(SimValue a, SimValue b);
/**
 * @brief Compare whether one value is less than or equal to another.
 * @param a Left operand.
 * @param b Right operand.
 * @return One-bit true/false result, or `x` when either input contains `x`/`z`.
 */
SimValue sim_val_lte(SimValue a, SimValue b);
/**
 * @brief Compare whether one value is greater than or equal to another.
 * @param a Left operand.
 * @param b Right operand.
 * @return One-bit true/false result, or `x` when either input contains `x`/`z`.
 */
SimValue sim_val_gte(SimValue a, SimValue b);

/**
 * @brief Compute logical AND using three-valued truth semantics.
 * @param a Left operand.
 * @param b Right operand.
 * @return One-bit logical result.
 */
SimValue sim_val_logical_and(SimValue a, SimValue b);
/**
 * @brief Compute logical OR using three-valued truth semantics.
 * @param a Left operand.
 * @param b Right operand.
 * @return One-bit logical result.
 */
SimValue sim_val_logical_or(SimValue a, SimValue b);
/**
 * @brief Compute logical NOT using three-valued truth semantics.
 * @param a Operand to negate logically.
 * @return One-bit logical result.
 */
SimValue sim_val_logical_not(SimValue a);

/**
 * @brief Shift a value left by the amount stored in another value.
 * @param a Value to shift.
 * @param b Shift amount.
 * @return Shifted value, or all `x` when either input contains `x`/`z`.
 */
SimValue sim_val_shl(SimValue a, SimValue b);
/**
 * @brief Shift a value right logically by the amount stored in another value.
 * @param a Value to shift.
 * @param b Shift amount.
 * @return Shifted value, or all `x` when either input contains `x`/`z`.
 */
SimValue sim_val_shr(SimValue a, SimValue b);
/**
 * @brief Shift a value right arithmetically by the amount stored in another value.
 * @param a Value to shift.
 * @param b Shift amount.
 * @return Shifted value with sign extension, or all `x` when either input contains `x`/`z`.
 */
SimValue sim_val_ashr(SimValue a, SimValue b);

/**
 * @brief Concatenate multiple values into one wider value.
 * @param vals Array of values ordered from most-significant chunk to least-significant chunk.
 * @param count Number of entries in @p vals.
 * @return Concatenated result.
 */
SimValue sim_val_concat(const SimValue *vals, int count);
/**
 * @brief Extract a contiguous bit slice from a value.
 * @param v Source value.
 * @param msb Inclusive most-significant bit index.
 * @param lsb Inclusive least-significant bit index.
 * @return Extracted slice value.
 */
SimValue sim_val_slice(SimValue v, int msb, int lsb);

/**
 * @brief Evaluate a ternary select between two values.
 * @param cond Condition value.
 * @param t Result selected when @p cond is true.
 * @param f Result selected when @p cond is false.
 * @return Selected result, or a merged/x-propagated result when @p cond is unknown.
 */
SimValue sim_val_ternary(SimValue cond, SimValue t, SimValue f);

/**
 * @brief Zero-extend a value to a wider width.
 * @param v Value to extend.
 * @param new_width Target width in bits.
 * @return Zero-extended value.
 */
SimValue sim_val_zext(SimValue v, int new_width);
/**
 * @brief Sign-extend a value to a wider width.
 * @param v Value to extend.
 * @param new_width Target width in bits.
 * @return Sign-extended value.
 */
SimValue sim_val_sext(SimValue v, int new_width);

/**
 * @brief Format a value as a hexadecimal text string.
 * @param v Value to format.
 * @param buf Destination buffer.
 * @param buflen Size of @p buf in bytes.
 * @return @p buf.
 */
char *sim_val_to_hex(SimValue v, char *buf, int buflen);
/**
 * @brief Format a value as a binary text string.
 * @param v Value to format.
 * @param buf Destination buffer.
 * @param buflen Size of @p buf in bytes.
 * @return @p buf.
 */
char *sim_val_to_bin(SimValue v, char *buf, int buflen);
/**
 * @brief Format a value as an unsigned decimal text string.
 * @param v Value to format.
 * @param buf Destination buffer.
 * @param buflen Size of @p buf in bytes.
 * @return @p buf.
 */
char *sim_val_to_dec(SimValue v, char *buf, int buflen);

/**
 * @brief Format a value as a sized HDL literal.
 * @param v Value to format.
 * @param buf Destination buffer.
 * @param buflen Size of @p buf in bytes.
 * @return @p buf.
 */
char *sim_val_format_literal(SimValue v, char *buf, int buflen);

#endif /* JZ_SIM_VALUE_H */
