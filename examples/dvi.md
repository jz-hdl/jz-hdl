---
url: /jz-hdl/examples/dvi.md
---

# DVI Color Bars

A 1280x720 @ 30Hz DVI display with selectable test patterns: horizontal color bars, vertical color bars, and an optional animated starfield. A debounced button cycles through the patterns. The design runs on both Tang Nano 20K and 9K boards.

## Clock Tree

```text
27 MHz crystal (SCLK)
  └─ PLL (IDIV=7, FBDIV=54, ODIV=4)
       └─ 185.625 MHz serial_clk
            └─ CLKDIV (DIV_MODE=5)
                 └─ 37.125 MHz pixel_clk
```

Both generators appear in a single `CLOCK_GEN` block. The CLKDIV's input references the PLL's output — the compiler resolves the chain and validates that the VCO (742.5 MHz) is within range and the serial-to-pixel ratio is 5:1 for 10-bit DDR serialization.

## Modules

### `video_timing`

CEA-861 compliant 1280x720 timing generator. Constants define the full timing: H\_ACTIVE=1280, H\_FRONT=110, H\_SYNC=40, H\_BACK=220 (H\_TOTAL=1650); V\_ACTIVE=720, V\_FRONT=5, V\_SYNC=5, V\_BACK=20 (V\_TOTAL=750). Two counters (`h_cnt`, `v_cnt`) free-run and wrap. Hsync and vsync are positive polarity. Outputs include pixel coordinates (`x_pos`, `y_pos`) and `display_enable`.

### `tmds_encoder`

DVI-specification TMDS 8b/10b encoder. The encoding pipeline:

1. Popcount the 8-bit input via an adder tree.
2. Select XOR or XNOR mode based on whether the popcount exceeds 4.
3. Build a 9-bit transition-minimized word by chaining XOR/XNOR of adjacent bits.
4. Popcount the result, track running disparity with a 5-bit signed counter, and conditionally invert the output for DC balance.

During blanking (`display_enable == 0`), the encoder outputs fixed control tokens based on `c0`/`c1` and resets disparity to zero.

### `hbars` / `vbars`

Purely combinational pattern generators. `hbars` maps `x_pos` to 5 horizontal bars of 256 pixels each (Red, Green, Blue, White, Black). `vbars` maps `y_pos` to 5 vertical bars of 144 pixels each in the same color order.

### `warp`

Animated starfield with 30 stars. Each star is a 2x2 white pixel that accelerates radially outward from screen center (640, 360). Stars that leave the screen respawn near center with positions randomized by a 32-bit Galois LFSR (taps at bits 31, 21, 1, 0). Updates occur every 4th vsync via a frame counter.

Three `@template` blocks handle per-star computation:

* **STAR\_DIST** — Absolute distance from center: `dx = |sx - 640|`, `dy = |sy - 360|`.
* **STAR\_HIT** — Pixel hit-test against the star's 2x2 bounding box.
* **STAR\_NEXT** — Radial movement with velocity = `distance/32 + 1`, or respawn if offscreen.

Each template is expanded 30 times by `@apply [NUM_STARS]` with `IDX` substitution.

The warp module is conditionally compiled via `@feature CONFIG.warp == 1`. The 9K project sets `warp = 0` to fit the smaller device; the 20K project sets `warp = 1`.

### `debounce`

2-stage metastability synchronizer followed by a counter-based debouncer. When the synchronized input disagrees with the stable state, a 20-bit counter increments until it reaches 742,499 (~20ms at 37.125 MHz), then latches the new state. A falling-edge detector produces a single-cycle `btn_press` pulse.

### `por`

Power-on reset timer. After the FPGA's `DONE` signal goes high, a 20-bit counter counts to 1,048,575 (~28ms) before releasing the active-low `por_n` output. This delay ensures the PLL has locked before the design starts running.

### `dvi_top`

Top-level integration. Instantiates all submodules and handles:

* **Pattern selection**: A 2-bit `pattern_sel` register increments on vsync rising edge when `pattern_pending` is set by the debounced button. Color outputs are muxed from hbars, vbars, or warp based on this register.
* **TMDS output pipeline**: A 2-stage register pipeline (pre-mux → output register) ensures a clean FF-to-OSER10 timing path. The TMDS clock channel drives a fixed `10'b1111100000` pattern.
* **Heartbeat**: A 25-bit counter toggles an LED.

## Differential Output

TMDS pins are declared with `mode=DIFFERENTIAL`, `standard=LVDS25`, and serialization clock bindings (`fclk = serial_clk`, `pclk = pixel_clk`). The compiler generates the OSER10 serializer and TLVDS\_OBUF differential buffer from these attributes. Each channel shifts out a 10-bit word per pixel clock using DDR at 185.625 MHz.

::: code-group

```jz
@module por
    PORT {
        IN  [1] clk;
        IN  [1] done;
        OUT [1] por_n;
    }

    CONST {
        POR_CYCLES   = 1_048_576;  // ~28ms at 37.125MHz — wait for PLL lock
        POR_CNT_BITS = clog2(POR_CYCLES);
        POR_MAX      = POR_CYCLES - 1;
    }

    REGISTER {
        por_reg [1] = 1'b0;
        cnt     [POR_CNT_BITS] = POR_CNT_BITS'b0;
    }

    ASYNCHRONOUS {
        por_n <= por_reg;
    }

    SYNCHRONOUS(CLK=clk) {
        IF (done == 1'b0) {
            por_reg <= 1'b0;
            cnt <= POR_CNT_BITS'b0;
        } ELIF (cnt == lit(POR_CNT_BITS, POR_MAX)) {
            por_reg <= 1'b1;
            cnt <= cnt;
        } ELSE {
            por_reg <= 1'b0;
            cnt <= cnt + POR_CNT_BITS'b1;
        }
    }
@endmod

```

:::

## JZ-HDL Language Features

**Clock chain validation.** The `CLOCK_GEN` block declares PLL and CLKDIV together with their input/output relationships. The compiler verifies VCO range, divider ratios, and serializer clock compatibility end-to-end. Traditional flows configure these separately in vendor GUI tools with no cross-validation.

**Templates.** `@template` and `@apply` eliminate repetitive per-instance logic. The 30-star warp module would require 30 copies of identical code in Verilog — or a `generate` block with synthesis-tool-dependent behavior.

**Conditional compilation.** `@feature` / `@endfeat` directives gate entire module instantiations and their associated wiring on project-level `CONFIG` values, enabling single-source multi-target builds.

**Integrated differential I/O.** Pin declarations with `mode=DIFFERENTIAL` and clock bindings replace manual instantiation of vendor serializer primitives (OSER10, TLVDS\_OBUF) and their associated constraint files.
