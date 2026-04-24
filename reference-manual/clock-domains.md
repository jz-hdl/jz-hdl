---
url: /jz-hdl/reference-manual/clock-domains.md
---

# Clock Domains

## Overview

Clock domains are explicit in JZ-HDL. Registers have a single home domain (the clock they are synchronized to). Direct use of a register across different clock domains is forbidden. The `CDC` block provides explicit, designer‑declared crossings that create safe, compiler‑understood synchronized views of register values between domains.

Key goals:

* Make cross‑domain behavior explicit and auditable.
* Prevent accidental multi‑domain register usage.
* Let the compiler enforce domain locality and generate appropriate synchronizer structures.

## Syntax

CDC entries appear inside a module body in a `CDC { ... }` block.

Basic form:

```text
CDC {
  <cdc_type>[n_stages] <source_reg> (<src_clk>) => <dest_alias> (<dest_clk>);
  ...
}
```

* `cdc_type` is one of: `BIT`, `BUS`, `FIFO`, `HANDSHAKE`, `PULSE`, `MCP`, `RAW`
* `[n_stages]` optional positive integer; default = 2 (must NOT be provided for `RAW`)
* `source_reg`: a `REGISTER` identifier defined in the same module (plain name, no slices/concat)
* `(src_clk)`: clock identifier for the source/home domain (must match a `SYNCHRONOUS` block `CLK` or a top-level clock input)
* `=> dest_alias (dest_clk)`: creates a read‑only alias visible in the destination clock domain

Examples:

```text
CDC {
  BIT    status_sync (clk_io) => cpu_status (clk_cpu);
  BUS[3] flags_bus    (clk_a)  => flags_view (clk_b);
  FIFO[4] payload     (clk_prod) => consumer_view (clk_cons);
}
```

## Semantics

* The CDC entry sets the **home clock domain** of the source register to `src_clk`.
* The dest alias is a read‑only signal that represents the synchronized view of the source register after approximately `n_stages` cycles in the destination clock.
* The source register may only be read/written in `SYNCHRONOUS` blocks whose `CLK` equals `src_clk` (its home domain).
* The dest alias may only be read in `SYNCHRONOUS` blocks whose `CLK` equals `dest_clk`. It can also be read combinationally in `ASYNCHRONOUS` blocks, but that combinational use must respect domain semantics (see "Usage Notes").
* The compiler elaborates the `CDC` entry into appropriate synchronization hardware:
  * `BIT[n]`: N-stage single-bit flip-flop synchronizer. The source register **must have width \[1]** — using BIT with a multi-bit register is a compile error (`CDC_BIT_WIDTH_NOT_1`).
  * `BUS[n]`: Multi‑bit synchronized path; intended only when the source follows Gray‑code or single‑bit change discipline.
  * `FIFO[n]`: Asynchronous FIFO for arbitrary multi‑bit transfers (handles wide or multi‑bit simultaneous changes safely).
  * `HANDSHAKE[n]`: Req/ack handshake protocol for infrequent multi‑bit transfers.
  * `PULSE[n]`: Toggle-based pulse synchronizer. The source register **must have width \[1]** — using PULSE with a multi-bit register is a compile error (`CDC_PULSE_WIDTH_NOT_1`).
  * `MCP[n]`: Multi‑cycle path formulation for stable multi‑bit data.
  * `RAW`: Direct unsynchronized wire connection (no crossing logic).

## CDC Types and Intended Use

* BIT\[n\_stages]
  * Use for single‑bit control/status signals.
  * Result `dest_alias` width: 1.
  * `n_stages` typically 2 or 3 in practice.
  * Latency: exactly `n_stages` cycles of `dest_clk`.

* BUS\[n\_stages]
  * Use for multi‑bit values that change in a safe, bounded way (e.g., Gray‑encoded counters, handshaked state encodings where only one bit flips at a time).
  * Not safe for arbitrary parallel changes (multi‑bit updates).
  * If source can change arbitrarily, prefer `FIFO`.
  * Latency: exactly `n_stages` cycles of `dest_clk`.

* FIFO\[n\_stages]
  * Use for arbitrary multi‑bit transfers (data buses, registers updated with new values every cycle).
  * Produces a staged, safe transfer; semantics approximates a buffer or asynchronous FIFO depending on implementation.
  * Latency: between 1 cycle (existing data) and `n_stages + 2` cycles (fresh write).

* HANDSHAKE\[n\_stages]
  * Use for infrequent multi‑bit transfers where throughput is not critical.
  * Source latches data and asserts request; destination syncs request, latches data, and asserts ack; source syncs ack and deasserts request.
  * Safe for arbitrary data widths.
  * Latency: variable depending on clock ratio; transfer completes after full req/ack handshake.

* PULSE\[n\_stages]
  * Use for single‑bit pulse events (width == 1 only).
  * Source toggles a register on each pulse; destination syncs the toggle and XOR‑detects edges to produce output pulses.
  * Latency: `n_stages` cycles for pulse detection; one output pulse per input pulse.

* MCP\[n\_stages]
  * Multi‑cycle path formulation for stable multi‑bit data transfers.
  * Source holds data stable and asserts an enable; destination syncs the enable and samples data when the enable is seen.
  * Uses the same req/ack protocol as HANDSHAKE for safe handoff.
  * Latency: variable, similar to HANDSHAKE; data held stable during transfer.

* RAW
  * Direct unsynchronized view — no crossing logic is inserted.
  * The destination alias is a direct wire connection (`assign`) to the source register.
  * The `[n_stages]` parameter must NOT be provided (compile error if present).
  * Any register width is allowed.
  * Latency: 0 cycles (direct connection).
  * Use only when the designer knows the signals are safe (e.g., quasi‑static configuration registers, signals with external synchronization, or signals that are stable at the time of sampling).
  * **Warning:** RAW explicitly opts out of CDC safety guarantees. Metastability protection is the designer's responsibility.

## Validation Rules (Compiler Enforced)

* Source register must be a `REGISTER` declared in the containing module.
* Source register identifier must be a plain name (no slices, concatenations, instance-qualified names).
* The `CDC` entry establishes the home domain for the source register; that register:
  * May only be assigned (in `SYNCHRONOUS`) inside a `SYNCHRONOUS` block whose `CLK` equals `src_clk`.
  * May only be read inside `SYNCHRONOUS` blocks whose `CLK` equals `src_clk`.
  * Any attempt to use the source register in a `SYNCHRONOUS` block with a different `CLK` is a DOMAIN\_CONFLICT error.
* The dest alias:
  * Is created by the compiler as a read‑only signal name (not a register the designer assigns).
  * May only be read in `SYNCHRONOUS` blocks whose `CLK` equals `dest_clk`.
  * Attempting to assign to the dest alias is a compile error.
* A module may have at most one `SYNCHRONOUS` block per unique clock signal. (See SYNCHRONOUS block rules.)
* Multiple CDC entries may declare crossings between the same pair of clocks or different clocks; each source register's home domain is set by its CORESCDC entry.
* If a register is referenced by a CDC entry, that CDC entry must appear before any `SYNCHRONOUS` block that uses the alias or the source register (tool-specific ordering rule for resolvability).
* For `BUS[n_stages]`, compiler may generate warnings if it detects potential multi‑bit simultaneous changes that violate Gray‑code assumptions (static/flow analysis best‑effort).

## Usage Notes

* The `CDC` block does not replace proper CDC design discipline; it documents intent and enables the toolchain to synthesize correct synchronizers and raise violations.
* The destination alias is an explicit name you should use in the destination domain's synchronous logic, not the source register name.
  * Correct: `IF (cpu_status) { ... }` where `cpu_status` is `dest_alias`.
  * Incorrect: reading `status_reg` inside `CLK=cpu_clk` when `status_reg` is home to `clk_io`.
* `dest_alias` is visible combinationally in `ASYNCHRONOUS` blocks, but be careful: combinational logic that mixes `dest_alias` with signals from the destination clock domain should not be used to create cross‑domain control paths (avoid asynchronous handshakes without explicit synchronizers).
* If you need to sample a multi‑bit value that changes arbitrarily, use `FIFO` to avoid metastability and data corruption.
* A single register may have multiple destination aliases to different clocks (fan‑out synchronization to multiple domains).

## Examples

### Single‑bit synchronizer (BIT, 2 stages default)

A 1‑bit `event_flag` is written in the `clk_a` domain and read via the synchronized alias `event_flag_sync` in the `clk_b` domain. The compiler inserts a 2‑stage flip‑flop chain.

::: code-group

```il \[Generated RTLIL]
# Generated by jz-hdl RTLIL backend
# jz-hdl version: Version 0.1.7 (c6d52fc)

attribute \src "unknown:1"
module \JZHDL_LIB_CDC_BIT
  wire width 1 input 1 \clk_dest
  wire width 1 input 2 \data_in
  wire width 1 output 3 \data_out
  wire width 1 \sync_ff1
  wire width 1 \sync_ff2
  connect \data_out \sync_ff2
  wire $0\sync_ff1[0:0]
  wire $0\sync_ff2[0:0]
  process $proc$clk0$1
    assign $0\sync_ff1[0:0] \sync_ff1
    assign $0\sync_ff2[0:0] \sync_ff2
    assign $0\sync_ff1[0:0] \data_in
    assign $0\sync_ff2[0:0] \sync_ff1
    sync posedge \clk_dest
      update \sync_ff1 $0\sync_ff1[0:0]
      update \sync_ff2 $0\sync_ff2[0:0]
  end
end

attribute \src "jz-hdl:6"
module \cdc_bit
  wire width 1 input 1 \clk_a
  wire width 1 input 2 \clk_b
  wire width 1 input 3 \rst_n
  wire width 1 input 4 \trigger
  wire width 1 output 5 \led
  wire width 1 \event_flag
  wire width 1 \cpu_seen
  wire width 1 \event_flag_sync
  cell \JZHDL_LIB_CDC_BIT \u_cdc_bit_event_flag_sync
    connect \clk_dest \clk_b
    connect \data_in \event_flag
    connect \data_out \event_flag_sync
  end
  wire $0\led[0:0]
  process $proc$async$2
    assign $0\led[0:0] \cpu_seen
    sync always
      update \led $0\led[0:0]
  end
  wire $0\event_flag[0:0]
  wire width 1 $auto$3
  cell $logic_not $auto$4
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \rst_n
    connect \Y $auto$3
  end
  process $proc$clk0$5
    assign $0\event_flag[0:0] \event_flag
    switch $auto$3
      case 1'1
        assign $0\event_flag[0:0] 1'0
      case
        switch \trigger
          case 1'1
            assign $0\event_flag[0:0] 1'1
          case
            assign $0\event_flag[0:0] 1'0
        end
    end
    sync posedge \clk_a
      update \event_flag $0\event_flag[0:0]
  end
  wire $0\cpu_seen[0:0]
  wire width 1 $auto$6
  cell $logic_not $auto$7
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \rst_n
    connect \Y $auto$6
  end
  process $proc$clk1$8
    assign $0\cpu_seen[0:0] \cpu_seen
    switch $auto$6
      case 1'1
        assign $0\cpu_seen[0:0] 1'0
      case
        switch \event_flag_sync
          case 1'1
            assign $0\cpu_seen[0:0] 1'1
          case
            assign $0\cpu_seen[0:0] 1'0
        end
    end
    sync posedge \clk_b
      update \cpu_seen $0\cpu_seen[0:0]
  end
end

attribute \top 1
module \top
  wire width 1 input 1 \SCLK
  wire width 1 input 2 \DONE
  wire width 2 input 3 \KEY
  wire width 1 output 4 \LED
  wire width 1 \CLK_FAST
  wire width 1 \jz_unused_pll_LOCK_cg0_u0
  wire width 1 \jz_unused_pll_PHASE_cg0_u0
  wire width 1 \jz_unused_pll_DIV_cg0_u0
  wire width 1 \jz_unused_pll_DIV3_cg0_u0
  cell \rPLL $auto$0_0
  parameter \DEVICE "GW1N-9C"
  parameter \FCLKIN "26.998"
  parameter \IDIV_SEL 2
  parameter \FBDIV_SEL 7
  parameter \ODIV_SEL 8
  parameter \PSDA_SEL "0000"
  parameter \DUTYDA_SEL "1000"
  parameter \DYN_IDIV_SEL "FALSE"
  parameter \DYN_FBDIV_SEL "FALSE"
  parameter \DYN_ODIV_SEL "FALSE"
  parameter \DYN_DA_EN "FALSE"
  parameter \DYN_SDIV_SEL 2
  parameter \CLKOUT_FT_DIR 1
  parameter \CLKOUTP_FT_DIR 1
  parameter \CLKOUT_DLY_STEP 0
  parameter \CLKOUTP_DLY_STEP 0
  parameter \CLKFB_SEL "internal"
  parameter \CLKOUT_BYPASS "FALSE"
  parameter \CLKOUTP_BYPASS "FALSE"
  parameter \CLKOUTD_BYPASS "FALSE"
  parameter \CLKOUTD_SRC "CLKOUT"
  parameter \CLKOUTD3_SRC "CLKOUT"
  connect \CLKIN \SCLK
  connect \CLKOUT \CLK_FAST
  connect \LOCK \jz_unused_pll_LOCK_cg0_u0
  connect \CLKOUTP \jz_unused_pll_PHASE_cg0_u0
  connect \CLKOUTD \jz_unused_pll_DIV_cg0_u0
  connect \CLKOUTD3 \jz_unused_pll_DIV3_cg0_u0
  connect \RESET 1'0
  connect \RESET_P 1'0
  connect \CLKFB 1'0
end
  wire \jz_inv_led
  cell $not $auto$9
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \jz_inv_led
    connect \Y \LED [0]
  end
  cell \cdc_bit \u_top
    connect \clk_a \SCLK
    connect \clk_b \CLK_FAST
    connect \rst_n \KEY [0]
    connect \trigger \KEY [1]
    connect \led \jz_inv_led
  end
end


```

:::

### Multi‑bit Gray‑code bus (BUS, 3 stages)

An 8‑bit `gray_ptr` register crosses from `clk_a` to `clk_b` using a 3‑stage BUS synchronizer. This is safe only when the source follows Gray‑code or single‑bit‑change discipline.

::: code-group

```il \[Generated RTLIL]
# Generated by jz-hdl RTLIL backend
# jz-hdl version: Version 0.1.7 (c6d52fc)

attribute \src "unknown:1"
module \JZHDL_LIB_CDC_BUS__W8
  wire width 1 input 1 \clk_src
  wire width 1 input 2 \clk_dest
  wire width 8 input 3 \data_in
  wire width 8 output 4 \data_out
  wire width 8 \gray_src
  wire width 8 \gray_sync1
  wire width 8 \gray_sync2
  wire width 8 $0\data_out[7:0]
  wire width 1 $auto$1
  cell $xor $auto$2
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \B_SIGNED 0
    parameter \B_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \data_out [7]
    connect \B \gray_sync2 [6]
    connect \Y $auto$1
  end
  wire width 1 $auto$3
  cell $xor $auto$4
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \B_SIGNED 0
    parameter \B_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \data_out [6]
    connect \B \gray_sync2 [5]
    connect \Y $auto$3
  end
  wire width 1 $auto$5
  cell $xor $auto$6
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \B_SIGNED 0
    parameter \B_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \data_out [5]
    connect \B \gray_sync2 [4]
    connect \Y $auto$5
  end
  wire width 1 $auto$7
  cell $xor $auto$8
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \B_SIGNED 0
    parameter \B_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \data_out [4]
    connect \B \gray_sync2 [3]
    connect \Y $auto$7
  end
  wire width 1 $auto$9
  cell $xor $auto$10
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \B_SIGNED 0
    parameter \B_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \data_out [3]
    connect \B \gray_sync2 [2]
    connect \Y $auto$9
  end
  wire width 1 $auto$11
  cell $xor $auto$12
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \B_SIGNED 0
    parameter \B_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \data_out [2]
    connect \B \gray_sync2 [1]
    connect \Y $auto$11
  end
  wire width 1 $auto$13
  cell $xor $auto$14
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \B_SIGNED 0
    parameter \B_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \data_out [1]
    connect \B \gray_sync2 [0]
    connect \Y $auto$13
  end
  process $proc$async$15
    assign $0\data_out[7:0] [7] \gray_sync2 [7]
    assign $0\data_out[7:0] [6] $auto$1
    assign $0\data_out[7:0] [5] $auto$3
    assign $0\data_out[7:0] [4] $auto$5
    assign $0\data_out[7:0] [3] $auto$7
    assign $0\data_out[7:0] [2] $auto$9
    assign $0\data_out[7:0] [1] $auto$11
    assign $0\data_out[7:0] [0] $auto$13
    sync always
      update \data_out $0\data_out[7:0]
  end
  wire width 8 $0\gray_src[7:0]
  wire width 8 $auto$16
  cell $shr $auto$17
    parameter \A_SIGNED 0
    parameter \A_WIDTH 8
    parameter \B_SIGNED 0
    parameter \B_WIDTH 8
    parameter \Y_WIDTH 8
    connect \A \data_in
    connect \B 8'00000001
    connect \Y $auto$16
  end
  wire width 8 $auto$18
  cell $xor $auto$19
    parameter \A_SIGNED 0
    parameter \A_WIDTH 8
    parameter \B_SIGNED 0
    parameter \B_WIDTH 8
    parameter \Y_WIDTH 8
    connect \A $auto$16
    connect \B \data_in
    connect \Y $auto$18
  end
  process $proc$clk0$20
    assign $0\gray_src[7:0] \gray_src
    assign $0\gray_src[7:0] $auto$18
    sync posedge \clk_src
      update \gray_src $0\gray_src[7:0]
  end
  wire width 8 $0\gray_sync1[7:0]
  wire width 8 $0\gray_sync2[7:0]
  process $proc$clk1$21
    assign $0\gray_sync1[7:0] \gray_sync1
    assign $0\gray_sync2[7:0] \gray_sync2
    assign $0\gray_sync1[7:0] \gray_src
    assign $0\gray_sync2[7:0] \gray_sync1
    sync posedge \clk_dest
      update \gray_sync1 $0\gray_sync1[7:0]
      update \gray_sync2 $0\gray_sync2[7:0]
  end
end

attribute \src "jz-hdl:7"
module \cdc_bus
  wire width 1 input 1 \clk_a
  wire width 1 input 2 \clk_b
  wire width 1 input 3 \rst_n
  wire width 6 output 4 \leds
  wire width 8 \gray_ptr
  wire width 8 \read_ptr
  wire width 8 \gray_ptr_sync
  cell \JZHDL_LIB_CDC_BUS__W8 \u_cdc_bus_gray_ptr_sync
    connect \clk_src \clk_a
    connect \clk_dest \clk_b
    connect \data_in \gray_ptr
    connect \data_out \gray_ptr_sync
  end
  wire width 6 $0\leds[5:0]
  process $proc$async$22
    assign $0\leds[5:0] \read_ptr [5:0]
    sync always
      update \leds $0\leds[5:0]
  end
  wire width 8 $0\gray_ptr[7:0]
  wire width 1 $auto$23
  cell $logic_not $auto$24
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \rst_n
    connect \Y $auto$23
  end
  wire width 8 $auto$25
  cell $add $auto$26
    parameter \A_SIGNED 0
    parameter \A_WIDTH 8
    parameter \B_SIGNED 0
    parameter \B_WIDTH 8
    parameter \Y_WIDTH 8
    connect \A \gray_ptr
    connect \B 8'00000001
    connect \Y $auto$25
  end
  process $proc$clk0$27
    assign $0\gray_ptr[7:0] \gray_ptr
    switch $auto$23
      case 1'1
        assign $0\gray_ptr[7:0] 8'00000000
      case
        assign $0\gray_ptr[7:0] $auto$25
    end
    sync posedge \clk_a
      update \gray_ptr $0\gray_ptr[7:0]
  end
  wire width 8 $0\read_ptr[7:0]
  wire width 1 $auto$28
  cell $logic_not $auto$29
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \rst_n
    connect \Y $auto$28
  end
  process $proc$clk1$30
    assign $0\read_ptr[7:0] \read_ptr
    switch $auto$28
      case 1'1
        assign $0\read_ptr[7:0] 8'00000000
      case
        assign $0\read_ptr[7:0] \gray_ptr_sync
    end
    sync posedge \clk_b
      update \read_ptr $0\read_ptr[7:0]
  end
end

attribute \top 1
module \top
  wire width 1 input 1 \SCLK
  wire width 1 input 2 \DONE
  wire width 1 input 3 \KEY
  wire width 6 output 4 \LED
  wire width 1 \CLK_FAST
  wire width 1 \jz_unused_pll_LOCK_cg0_u0
  wire width 1 \jz_unused_pll_PHASE_cg0_u0
  wire width 1 \jz_unused_pll_DIV_cg0_u0
  wire width 1 \jz_unused_pll_DIV3_cg0_u0
  cell \rPLL $auto$0_0
  parameter \DEVICE "GW1N-9C"
  parameter \FCLKIN "26.998"
  parameter \IDIV_SEL 2
  parameter \FBDIV_SEL 7
  parameter \ODIV_SEL 8
  parameter \PSDA_SEL "0000"
  parameter \DUTYDA_SEL "1000"
  parameter \DYN_IDIV_SEL "FALSE"
  parameter \DYN_FBDIV_SEL "FALSE"
  parameter \DYN_ODIV_SEL "FALSE"
  parameter \DYN_DA_EN "FALSE"
  parameter \DYN_SDIV_SEL 2
  parameter \CLKOUT_FT_DIR 1
  parameter \CLKOUTP_FT_DIR 1
  parameter \CLKOUT_DLY_STEP 0
  parameter \CLKOUTP_DLY_STEP 0
  parameter \CLKFB_SEL "internal"
  parameter \CLKOUT_BYPASS "FALSE"
  parameter \CLKOUTP_BYPASS "FALSE"
  parameter \CLKOUTD_BYPASS "FALSE"
  parameter \CLKOUTD_SRC "CLKOUT"
  parameter \CLKOUTD3_SRC "CLKOUT"
  connect \CLKIN \SCLK
  connect \CLKOUT \CLK_FAST
  connect \LOCK \jz_unused_pll_LOCK_cg0_u0
  connect \CLKOUTP \jz_unused_pll_PHASE_cg0_u0
  connect \CLKOUTD \jz_unused_pll_DIV_cg0_u0
  connect \CLKOUTD3 \jz_unused_pll_DIV3_cg0_u0
  connect \RESET 1'0
  connect \RESET_P 1'0
  connect \CLKFB 1'0
end
  cell \cdc_bus \u_top
    connect \clk_a \SCLK
    connect \clk_b \CLK_FAST
    connect \rst_n \KEY [0]
    connect \leds { \LED [5] \LED [4] \LED [3] \LED [2] \LED [1] \LED [0] }
  end
end


```

:::

### Wide arbitrary data via FIFO (FIFO, 4 stages)

A 64‑bit `packet_word` register crosses from `clk_a` to `clk_b` using a 4‑stage FIFO synchronizer. Unlike BUS, FIFO handles arbitrary multi‑bit changes safely.

::: code-group

```il \[Generated RTLIL]
# Generated by jz-hdl RTLIL backend
# jz-hdl version: Version 0.1.7 (c6d52fc)

attribute \src "unknown:1"
module \JZHDL_LIB_CDC_FIFO__W64
  wire width 1 input 1 \clk_wr
  wire width 1 input 2 \clk_rd
  wire width 1 input 3 \rst_wr
  wire width 1 input 4 \rst_rd
  wire width 64 input 5 \data_in
  wire width 1 input 6 \write_en
  wire width 1 input 7 \read_en
  wire width 64 output 8 \data_out
  wire width 1 output 9 \full
  wire width 1 output 10 \empty
  wire width 5 \wr_ptr_bin
  wire width 5 \wr_ptr_gray
  wire width 5 \rd_ptr_bin
  wire width 5 \rd_ptr_gray
  wire width 5 \wr_ptr_gray_sync1
  wire width 5 \wr_ptr_gray_sync2
  wire width 5 \rd_ptr_gray_sync1
  wire width 5 \rd_ptr_gray_sync2
  memory width 64 size 16 \mem
  wire width 64 $auto$1
  cell $memrd_v2 $auto$2
    parameter \MEMID "\\mem"
    parameter \ABITS 4
    parameter \WIDTH 64
    parameter \CLK_ENABLE 0
    parameter \CLK_POLARITY 1
    parameter \TRANSPARENCY_MASK 0
    parameter \COLLISION_X_MASK 0
    parameter \CE_OVER_SRST 0
    parameter \ARST_VALUE 64'0000000000000000000000000000000000000000000000000000000000000000
    parameter \SRST_VALUE 64'0000000000000000000000000000000000000000000000000000000000000000
    parameter \INIT_VALUE 64'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    connect \ADDR \rd_ptr_bin [3:0]
    connect \DATA $auto$1
    connect \EN 1'1
    connect \ARST 1'0
    connect \SRST 1'0
    connect \CLK 1'0
  end
  connect \data_out $auto$1
  wire width 2 $auto$3
  cell $not $auto$4
    parameter \A_SIGNED 0
    parameter \A_WIDTH 2
    parameter \Y_WIDTH 2
    connect \A \rd_ptr_gray_sync2 [4:3]
    connect \Y $auto$3
  end
  wire width 1 $auto$5
  cell $eq $auto$6
    parameter \A_SIGNED 0
    parameter \A_WIDTH 5
    parameter \B_SIGNED 0
    parameter \B_WIDTH 5
    parameter \Y_WIDTH 1
    connect \A \wr_ptr_gray
    connect \B { $auto$3 \rd_ptr_gray_sync2 [2:0] }
    connect \Y $auto$5
  end
  connect \full $auto$5
  wire width 1 $auto$7
  cell $eq $auto$8
    parameter \A_SIGNED 0
    parameter \A_WIDTH 5
    parameter \B_SIGNED 0
    parameter \B_WIDTH 5
    parameter \Y_WIDTH 1
    connect \A \rd_ptr_gray
    connect \B \wr_ptr_gray_sync2
    connect \Y $auto$7
  end
  connect \empty $auto$7
  wire width 1 $auto$9
  cell $not $auto$10
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \full
    connect \Y $auto$9
  end
  wire width 1 $auto$11
  cell $logic_and $auto$12
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \B_SIGNED 0
    parameter \B_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \write_en
    connect \B $auto$9
    connect \Y $auto$11
  end
  cell $memwr_v2 $auto$13
    parameter \MEMID "\\mem"
    parameter \ABITS 4
    parameter \WIDTH 64
    parameter \CLK_ENABLE 1
    parameter \CLK_POLARITY 1
    parameter \PORTID 0
    parameter \PRIORITY_MASK 0
    connect \ADDR \wr_ptr_bin [3:0]
    connect \DATA \data_in
    connect \EN { $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 }
    connect \CLK \clk_wr
  end
  wire width 5 $0\wr_ptr_bin[4:0]
  wire width 5 $0\wr_ptr_gray[4:0]
  wire width 1 $auto$14
  cell $not $auto$15
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \full
    connect \Y $auto$14
  end
  wire width 1 $auto$16
  cell $logic_and $auto$17
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \B_SIGNED 0
    parameter \B_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \write_en
    connect \B $auto$14
    connect \Y $auto$16
  end
  wire width 5 $auto$18
  cell $add $auto$19
    parameter \A_SIGNED 0
    parameter \A_WIDTH 5
    parameter \B_SIGNED 0
    parameter \B_WIDTH 5
    parameter \Y_WIDTH 5
    connect \A \wr_ptr_bin
    connect \B 5'00001
    connect \Y $auto$18
  end
  wire width 5 $auto$20
  cell $add $auto$21
    parameter \A_SIGNED 0
    parameter \A_WIDTH 5
    parameter \B_SIGNED 0
    parameter \B_WIDTH 5
    parameter \Y_WIDTH 5
    connect \A \wr_ptr_bin
    connect \B 5'00001
    connect \Y $auto$20
  end
  wire width 5 $auto$22
  cell $shr $auto$23
    parameter \A_SIGNED 0
    parameter \A_WIDTH 5
    parameter \B_SIGNED 0
    parameter \B_WIDTH 5
    parameter \Y_WIDTH 5
    connect \A $auto$20
    connect \B 5'00001
    connect \Y $auto$22
  end
  wire width 5 $auto$24
  cell $add $auto$25
    parameter \A_SIGNED 0
    parameter \A_WIDTH 5
    parameter \B_SIGNED 0
    parameter \B_WIDTH 5
    parameter \Y_WIDTH 5
    connect \A \wr_ptr_bin
    connect \B 5'00001
    connect \Y $auto$24
  end
  wire width 5 $auto$26
  cell $xor $auto$27
    parameter \A_SIGNED 0
    parameter \A_WIDTH 5
    parameter \B_SIGNED 0
    parameter \B_WIDTH 5
    parameter \Y_WIDTH 5
    connect \A $auto$22
    connect \B $auto$24
    connect \Y $auto$26
  end
  process $proc$clk0$28
    assign $0\wr_ptr_bin[4:0] \wr_ptr_bin
    assign $0\wr_ptr_gray[4:0] \wr_ptr_gray
    switch \rst_wr
      case 1'1
        assign $0\wr_ptr_bin[4:0] 5'00000
        assign $0\wr_ptr_gray[4:0] 5'00000
      case
        switch $auto$16
          case 1'1
            # memory write: mem (handled by $memwr_v2 cell)
            assign $0\wr_ptr_bin[4:0] $auto$18
            assign $0\wr_ptr_gray[4:0] $auto$26
          case
        end
    end
    sync posedge \clk_wr
      update \wr_ptr_bin $0\wr_ptr_bin[4:0]
      update \wr_ptr_gray $0\wr_ptr_gray[4:0]
    sync high \rst_wr
      update \wr_ptr_bin 5'00000
      update \wr_ptr_gray 5'00000
  end
  wire width 5 $0\rd_ptr_bin[4:0]
  wire width 5 $0\rd_ptr_gray[4:0]
  wire width 1 $auto$29
  cell $not $auto$30
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \empty
    connect \Y $auto$29
  end
  wire width 1 $auto$31
  cell $logic_and $auto$32
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \B_SIGNED 0
    parameter \B_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \read_en
    connect \B $auto$29
    connect \Y $auto$31
  end
  wire width 5 $auto$33
  cell $add $auto$34
    parameter \A_SIGNED 0
    parameter \A_WIDTH 5
    parameter \B_SIGNED 0
    parameter \B_WIDTH 5
    parameter \Y_WIDTH 5
    connect \A \rd_ptr_bin
    connect \B 5'00001
    connect \Y $auto$33
  end
  wire width 5 $auto$35
  cell $add $auto$36
    parameter \A_SIGNED 0
    parameter \A_WIDTH 5
    parameter \B_SIGNED 0
    parameter \B_WIDTH 5
    parameter \Y_WIDTH 5
    connect \A \rd_ptr_bin
    connect \B 5'00001
    connect \Y $auto$35
  end
  wire width 5 $auto$37
  cell $shr $auto$38
    parameter \A_SIGNED 0
    parameter \A_WIDTH 5
    parameter \B_SIGNED 0
    parameter \B_WIDTH 5
    parameter \Y_WIDTH 5
    connect \A $auto$35
    connect \B 5'00001
    connect \Y $auto$37
  end
  wire width 5 $auto$39
  cell $add $auto$40
    parameter \A_SIGNED 0
    parameter \A_WIDTH 5
    parameter \B_SIGNED 0
    parameter \B_WIDTH 5
    parameter \Y_WIDTH 5
    connect \A \rd_ptr_bin
    connect \B 5'00001
    connect \Y $auto$39
  end
  wire width 5 $auto$41
  cell $xor $auto$42
    parameter \A_SIGNED 0
    parameter \A_WIDTH 5
    parameter \B_SIGNED 0
    parameter \B_WIDTH 5
    parameter \Y_WIDTH 5
    connect \A $auto$37
    connect \B $auto$39
    connect \Y $auto$41
  end
  process $proc$clk1$43
    assign $0\rd_ptr_bin[4:0] \rd_ptr_bin
    assign $0\rd_ptr_gray[4:0] \rd_ptr_gray
    switch \rst_rd
      case 1'1
        assign $0\rd_ptr_bin[4:0] 5'00000
        assign $0\rd_ptr_gray[4:0] 5'00000
      case
        switch $auto$31
          case 1'1
            assign $0\rd_ptr_bin[4:0] $auto$33
            assign $0\rd_ptr_gray[4:0] $auto$41
          case
        end
    end
    sync posedge \clk_rd
      update \rd_ptr_bin $0\rd_ptr_bin[4:0]
      update \rd_ptr_gray $0\rd_ptr_gray[4:0]
    sync high \rst_rd
      update \rd_ptr_bin 5'00000
      update \rd_ptr_gray 5'00000
  end
  wire width 5 $0\rd_ptr_gray_sync1[4:0]
  wire width 5 $0\rd_ptr_gray_sync2[4:0]
  process $proc$clk2$44
    assign $0\rd_ptr_gray_sync1[4:0] \rd_ptr_gray_sync1
    assign $0\rd_ptr_gray_sync2[4:0] \rd_ptr_gray_sync2
    switch \rst_wr
      case 1'1
        assign $0\rd_ptr_gray_sync1[4:0] 5'00000
        assign $0\rd_ptr_gray_sync2[4:0] 5'00000
      case
        assign $0\rd_ptr_gray_sync1[4:0] \rd_ptr_gray
        assign $0\rd_ptr_gray_sync2[4:0] \rd_ptr_gray_sync1
    end
    sync posedge \clk_wr
      update \rd_ptr_gray_sync1 $0\rd_ptr_gray_sync1[4:0]
      update \rd_ptr_gray_sync2 $0\rd_ptr_gray_sync2[4:0]
    sync high \rst_wr
      update \rd_ptr_gray_sync1 5'00000
      update \rd_ptr_gray_sync2 5'00000
  end
  wire width 5 $0\wr_ptr_gray_sync1[4:0]
  wire width 5 $0\wr_ptr_gray_sync2[4:0]
  process $proc$clk3$45
    assign $0\wr_ptr_gray_sync1[4:0] \wr_ptr_gray_sync1
    assign $0\wr_ptr_gray_sync2[4:0] \wr_ptr_gray_sync2
    switch \rst_rd
      case 1'1
        assign $0\wr_ptr_gray_sync1[4:0] 5'00000
        assign $0\wr_ptr_gray_sync2[4:0] 5'00000
      case
        assign $0\wr_ptr_gray_sync1[4:0] \wr_ptr_gray
        assign $0\wr_ptr_gray_sync2[4:0] \wr_ptr_gray_sync1
    end
    sync posedge \clk_rd
      update \wr_ptr_gray_sync1 $0\wr_ptr_gray_sync1[4:0]
      update \wr_ptr_gray_sync2 $0\wr_ptr_gray_sync2[4:0]
    sync high \rst_rd
      update \wr_ptr_gray_sync1 5'00000
      update \wr_ptr_gray_sync2 5'00000
  end
end

attribute \src "jz-hdl:7"
module \cdc_fifo
  wire width 1 input 1 \clk_a
  wire width 1 input 2 \clk_b
  wire width 1 input 3 \rst_n
  wire width 6 output 4 \leds
  wire width 64 \packet_word
  wire width 64 \data_out
  wire width 64 \packet_view
  cell \JZHDL_LIB_CDC_FIFO__W64 \u_cdc_fifo_packet_view
    connect \clk_wr \clk_a
    connect \clk_rd \clk_b
    connect \rst_wr \rst_n
    connect \rst_rd \rst_n
    connect \data_in \packet_word
    connect \write_en 1'1
    connect \read_en 1'1
    connect \data_out \packet_view
  end
  wire width 6 $0\leds[5:0]
  process $proc$async$46
    assign $0\leds[5:0] \data_out [5:0]
    sync always
      update \leds $0\leds[5:0]
  end
  wire width 64 $0\packet_word[63:0]
  wire width 1 $auto$47
  cell $logic_not $auto$48
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \rst_n
    connect \Y $auto$47
  end
  wire width 64 $auto$49
  cell $add $auto$50
    parameter \A_SIGNED 0
    parameter \A_WIDTH 64
    parameter \B_SIGNED 0
    parameter \B_WIDTH 64
    parameter \Y_WIDTH 64
    connect \A \packet_word
    connect \B 64'0000000000000000000000000000000000000000000000000000000000000001
    connect \Y $auto$49
  end
  process $proc$clk0$51
    assign $0\packet_word[63:0] \packet_word
    switch $auto$47
      case 1'1
        assign $0\packet_word[63:0] 64'0000000000000000000000000000000000000000000000000000000000000000
      case
        assign $0\packet_word[63:0] $auto$49
    end
    sync posedge \clk_a
      update \packet_word $0\packet_word[63:0]
  end
  wire width 64 $0\data_out[63:0]
  wire width 1 $auto$52
  cell $logic_not $auto$53
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \rst_n
    connect \Y $auto$52
  end
  process $proc$clk1$54
    assign $0\data_out[63:0] \data_out
    switch $auto$52
      case 1'1
        assign $0\data_out[63:0] 64'0000000000000000000000000000000000000000000000000000000000000000
      case
        assign $0\data_out[63:0] \packet_view
    end
    sync posedge \clk_b
      update \data_out $0\data_out[63:0]
  end
end

attribute \top 1
module \top
  wire width 1 input 1 \SCLK
  wire width 1 input 2 \DONE
  wire width 1 input 3 \KEY
  wire width 6 output 4 \LED
  wire width 1 \CLK_FAST
  wire width 1 \jz_unused_pll_LOCK_cg0_u0
  wire width 1 \jz_unused_pll_PHASE_cg0_u0
  wire width 1 \jz_unused_pll_DIV_cg0_u0
  wire width 1 \jz_unused_pll_DIV3_cg0_u0
  cell \rPLL $auto$0_0
  parameter \DEVICE "GW1N-9C"
  parameter \FCLKIN "26.998"
  parameter \IDIV_SEL 2
  parameter \FBDIV_SEL 7
  parameter \ODIV_SEL 8
  parameter \PSDA_SEL "0000"
  parameter \DUTYDA_SEL "1000"
  parameter \DYN_IDIV_SEL "FALSE"
  parameter \DYN_FBDIV_SEL "FALSE"
  parameter \DYN_ODIV_SEL "FALSE"
  parameter \DYN_DA_EN "FALSE"
  parameter \DYN_SDIV_SEL 2
  parameter \CLKOUT_FT_DIR 1
  parameter \CLKOUTP_FT_DIR 1
  parameter \CLKOUT_DLY_STEP 0
  parameter \CLKOUTP_DLY_STEP 0
  parameter \CLKFB_SEL "internal"
  parameter \CLKOUT_BYPASS "FALSE"
  parameter \CLKOUTP_BYPASS "FALSE"
  parameter \CLKOUTD_BYPASS "FALSE"
  parameter \CLKOUTD_SRC "CLKOUT"
  parameter \CLKOUTD3_SRC "CLKOUT"
  connect \CLKIN \SCLK
  connect \CLKOUT \CLK_FAST
  connect \LOCK \jz_unused_pll_LOCK_cg0_u0
  connect \CLKOUTP \jz_unused_pll_PHASE_cg0_u0
  connect \CLKOUTD \jz_unused_pll_DIV_cg0_u0
  connect \CLKOUTD3 \jz_unused_pll_DIV3_cg0_u0
  connect \RESET 1'0
  connect \RESET_P 1'0
  connect \CLKFB 1'0
end
  cell \cdc_fifo \u_top
    connect \clk_a \SCLK
    connect \clk_b \CLK_FAST
    connect \rst_n \KEY [0]
    connect \leds { \LED [5] \LED [4] \LED [3] \LED [2] \LED [1] \LED [0] }
  end
end


```

:::

### Raw unsynchronized view (RAW, quasi‑static config)

A 16‑bit `config_word` register crosses unsynchronized via RAW. No CDC logic is inserted — the designer guarantees the value is stable when read.

::: code-group

```il \[Generated RTLIL]
# Generated by jz-hdl RTLIL backend
# jz-hdl version: Version 0.1.7 (c6d52fc)

attribute \src "jz-hdl:7"
module \cdc_raw
  wire width 1 input 1 \clk_a
  wire width 1 input 2 \clk_b
  wire width 1 input 3 \rst_n
  wire width 6 output 4 \leds
  wire width 16 \config_word
  wire width 16 \local_config
  wire width 16 \config_view
  connect \config_view \config_word
  wire width 6 $0\leds[5:0]
  process $proc$async$1
    assign $0\leds[5:0] \local_config [5:0]
    sync always
      update \leds $0\leds[5:0]
  end
  wire width 16 $0\config_word[15:0]
  wire width 1 $auto$2
  cell $logic_not $auto$3
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \rst_n
    connect \Y $auto$2
  end
  process $proc$clk0$4
    assign $0\config_word[15:0] \config_word
    switch $auto$2
      case 1'1
        assign $0\config_word[15:0] 16'1010010110100101
      case
        assign $0\config_word[15:0] \config_word
    end
    sync posedge \clk_a
      update \config_word $0\config_word[15:0]
  end
  wire width 16 $0\local_config[15:0]
  wire width 1 $auto$5
  cell $logic_not $auto$6
    parameter \A_SIGNED 0
    parameter \A_WIDTH 1
    parameter \Y_WIDTH 1
    connect \A \rst_n
    connect \Y $auto$5
  end
  process $proc$clk1$7
    assign $0\local_config[15:0] \local_config
    switch $auto$5
      case 1'1
        assign $0\local_config[15:0] 16'0000000000000000
      case
        assign $0\local_config[15:0] \config_view
    end
    sync posedge \clk_b
      update \local_config $0\local_config[15:0]
  end
end

attribute \top 1
module \top
  wire width 1 input 1 \SCLK
  wire width 1 input 2 \DONE
  wire width 1 input 3 \KEY
  wire width 6 output 4 \LED
  wire width 1 \CLK_FAST
  wire width 1 \jz_unused_pll_LOCK_cg0_u0
  wire width 1 \jz_unused_pll_PHASE_cg0_u0
  wire width 1 \jz_unused_pll_DIV_cg0_u0
  wire width 1 \jz_unused_pll_DIV3_cg0_u0
  cell \rPLL $auto$0_0
  parameter \DEVICE "GW1N-9C"
  parameter \FCLKIN "26.998"
  parameter \IDIV_SEL 2
  parameter \FBDIV_SEL 7
  parameter \ODIV_SEL 8
  parameter \PSDA_SEL "0000"
  parameter \DUTYDA_SEL "1000"
  parameter \DYN_IDIV_SEL "FALSE"
  parameter \DYN_FBDIV_SEL "FALSE"
  parameter \DYN_ODIV_SEL "FALSE"
  parameter \DYN_DA_EN "FALSE"
  parameter \DYN_SDIV_SEL 2
  parameter \CLKOUT_FT_DIR 1
  parameter \CLKOUTP_FT_DIR 1
  parameter \CLKOUT_DLY_STEP 0
  parameter \CLKOUTP_DLY_STEP 0
  parameter \CLKFB_SEL "internal"
  parameter \CLKOUT_BYPASS "FALSE"
  parameter \CLKOUTP_BYPASS "FALSE"
  parameter \CLKOUTD_BYPASS "FALSE"
  parameter \CLKOUTD_SRC "CLKOUT"
  parameter \CLKOUTD3_SRC "CLKOUT"
  connect \CLKIN \SCLK
  connect \CLKOUT \CLK_FAST
  connect \LOCK \jz_unused_pll_LOCK_cg0_u0
  connect \CLKOUTP \jz_unused_pll_PHASE_cg0_u0
  connect \CLKOUTD \jz_unused_pll_DIV_cg0_u0
  connect \CLKOUTD3 \jz_unused_pll_DIV3_cg0_u0
  connect \RESET 1'0
  connect \RESET_P 1'0
  connect \CLKFB 1'0
end
  cell \cdc_raw \u_top
    connect \clk_a \SCLK
    connect \clk_b \CLK_FAST
    connect \rst_n \KEY [0]
    connect \leds { \LED [5] \LED [4] \LED [3] \LED [2] \LED [1] \LED [0] }
  end
end


```

:::

## Common Errors and Diagnostics

* DOMAIN\_CONFLICT
  * Cause: Using a `REGISTER` in a synchronous block whose `CLK` differs from the register's home domain (as set by CDC or by where the register is first assigned).
  * Fix: Move the register usage to the correct `SYNCHRONOUS` block or add a CDC entry that creates an alias for cross‑domain use.

* DUPLICATE\_CDC\_ENTRY
  * Cause: Two CDC entries attempt to set home domain for the same source register inconsistently.
  * Fix: Consolidate CDC entries; each register should have a single definitive home domain.

* INVALID\_CDC\_TARGET
  * Cause: The source register is not a plain register identifier (slice, concat, or undefined).
  * Fix: Use the plain register name; if you need to cross slices or fields, create separate registers or use FIFO.

* INVALID\_CDC\_TYPE
  * Cause: Using `BIT` for multi‑bit sources or `BUS` for sources that change arbitrarily.
  * Fix: Use the correct CDC type. Use `BIT` for width==1, `BUS` only when Gray‑code discipline is followed, otherwise `FIFO`.

* UNSAFE\_BUS\_WARNING (warning)
  * Cause: Static/heuristic analysis detects that a `BUS` source may change multiple bits simultaneously.
  * Fix: Use `FIFO` or redesign the producer to follow Gray‑code or single‑bit change discipline.

* MISSING\_CDC\_FOR\_CROSS\_DOMAIN\_USE
  * Cause: A register is referenced in a different domain without a CDC entry.
  * Fix: Add a CDC entry or redesign to avoid cross‑domain reads.

* MULTI\_CLK\_ASSIGN / REGISTER\_LOCALITY\_VIOLATION
  * Cause: Assigning the same register in more than one `SYNCHRONOUS` block for different clocks.
  * Fix: Ensure a register is written only in its home domain; use CDC to observe it elsewhere.

## Best Practices

* Be explicit and conservative:
  * Prefer `FIFO` for multi‑bit transfers unless you can guarantee single‑bit changes (Gray code) and understand the implications.
  * Use `BIT[2]` or `BIT[3]` for status/control signals; 2 stages is common, 3 for safety in noisy environments.

* Name aliases clearly:
  * Use systematic naming like `reg_sync_destclk` or `src_to_dst_signal` so intent is obvious.

* One synchronous block per clock:
  * Place all logic for a given clock in the same `SYNCHRONOUS(CLK=...)` block to satisfy Synchronous Block Uniqueness.

* Keep CDC block near register declarations:
  * Place `CDC { ... }` entries close to the `REGISTER` declarations for readability and to help tools resolve names.

* Document constraints:
  * If a `BUS` CDC requires Gray‑coding, document the producer's requirement in comments and add static checks or assertions when possible.

* Verify with static checks and timing:
  * Run CDC-specific static checks and, where possible, formal checks to ensure no combinational paths create control hazards across domains.
  * Ensure timing constraints for synchronizers are included in downstream SDC (synthesis/timing) flows.

## Anti‑patterns (What to avoid)

* Reading a source register directly in another clock domain without CDC — causes DOMAIN\_CONFLICT and metastability.
* Using `BUS` for wide registers that can change every cycle with arbitrary bit patterns — leads to data corruption.
* Mixing alias names and source register names across domains (ambiguous intent).
* Trying to synchronize slices of a multi‑bit register via separate `BIT` synchronizers without ensuring atomic update semantics on the producer side.

## Checklist for Adding a CDC Crossing

* Confirm the source is a `REGISTER` and has a single, clear producer domain.
* Decide the appropriate CDC type:
  * BIT → single‑bit flag.
  * BUS → multi‑bit Gray‑code or single‑bit change guaranteed.
  * FIFO → arbitrary multi‑bit transfers.
  * RAW → quasi‑static or externally synchronized signals (no CDC logic inserted).
* Choose `n_stages` (default 2). For safety or noisy inputs, increase to 3.
* Add the `CDC` entry near the register declaration.
* Replace any cross‑domain uses with reads of the `dest_alias`.
* Run static checks; address compiler warnings/errors.
* Add documentation/comments explaining invariants (e.g., Gray‑code property).

## Synthesis and Implementation Notes

* The compiler will lower `CDC` entries into synthesizable primitives:
  * BIT → chain of flip‑flops with optional meta‑stable handling.
  * BUS → bank of flip‑flops and optional handshaking or gating depending on implementation.
  * FIFO → dual‑clock FIFO or asynchronous FIFO implementation using pointers and synchronizers.
* Downstream tools should map these to:
  * Vendor synchronizer primitives or hand‑optimized flops.
  * FIFO IP blocks for `FIFO` CDC entries when available.
* Ensure timing constraints (SDC) include created synchronizer registers so STA correctly analyzes setup/hold for destination domain.
