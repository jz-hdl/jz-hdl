---
url: /jz-hdl/examples/pll.md
---

# PLL Blink

Five LEDs blink at different rates, each driven by an independent clock domain. The 27 MHz board oscillator feeds the first counter directly and drives a PLL that generates four additional clocks. The visible blink differences confirm that all PLL outputs are active and running at the expected frequencies.

## Clock Tree

```text
27 MHz crystal (SCLK)
  └─ PLL (IDIV=2, FBDIV=24, ODIV=4)
       ├─ OUT BASE   CLK_A = 225 MHz
       ├─ OUT PHASE  CLK_B = 225 MHz (phase-shifted)
       ├─ OUT DIV    CLK_C = 56.25 MHz (CLKOUTD_DIV=4)
       └─ OUT DIV3   CLK_D = 75 MHz
```

PLL math: VCO = 27 × (24+1) × 4 / (2+1) = 900 MHz. Base output = 900/4 = 225 MHz. Divided outputs: 225/4 = 56.25 MHz and 225/3 = 75 MHz.

Each LED blinks at `clock_freq / 2^27` — roughly 0.2 Hz for the 27 MHz counter, up to 1.7 Hz for the 225 MHz counters.

## Modules

### `blinkers`

Five clock inputs (`clk1` through `clk5`), a POR input, a reset button, and 5 LED outputs.

The `ASYNCHRONOUS` block combines `por & rst_n` into an active-low `reset` wire and drives each LED from the inverted MSB of its corresponding counter (`leds[N] <= ~counter_X[26]`).

Five `SYNCHRONOUS` blocks each declare their own clock (`CLK=clk1` through `CLK=clk5`) with `RESET_TYPE=Clocked` for proper synchronous reset deassertion within each domain. Each block increments its 27-bit counter by 1.

### Project Files

Both Tang Nano 20K and 9K project files use identical PLL configuration. The `CLOCK_GEN` block declares the PLL with all four outputs (BASE, PHASE, DIV, DIV3) and configuration parameters. The `@top` instance maps all five clocks and the reset.

::: code-group

```jz
@module blinkers
    PORT {
        IN  [1] clk1;
        IN  [1] clk2;
        IN  [1] clk3;
        IN  [1] clk4;
        IN  [1] clk5;
        IN  [1] por;
        IN  [1] rst_n;
        OUT [5] leds;
    }

    WIRE {
        reset [1];
    }

    REGISTER {
        counter_a [27] = 27'b1;
        counter_b [27] = 27'b1;
        counter_c [27] = 27'b1;
        counter_d [27] = 27'b1;
        counter_e [27] = 27'b1;
    }

    ASYNCHRONOUS {
        reset <= por & rst_n;
        leds[0] <= ~counter_a[26];
        leds[1] <= ~counter_b[26];
        leds[2] <= ~counter_c[26];
        leds[3] <= ~counter_d[26];
        leds[4] <= ~counter_e[26];
    }

    SYNCHRONOUS(CLK=clk1 RESET=reset RESET_ACTIVE=Low RESET_TYPE=Clocked) {
        counter_a <= counter_a + 27'b1;
    }

    SYNCHRONOUS(CLK=clk2 RESET=reset RESET_ACTIVE=Low RESET_TYPE=Clocked) {
        counter_b <= counter_b + 27'b1;
    }

    SYNCHRONOUS(CLK=clk3 RESET=reset RESET_ACTIVE=Low RESET_TYPE=Clocked) {
        counter_c <= counter_c + 27'b1;
    }

    SYNCHRONOUS(CLK=clk4 RESET=reset RESET_ACTIVE=Low RESET_TYPE=Clocked) {
        counter_d <= counter_d + 27'b1;
    }

    SYNCHRONOUS(CLK=clk5 RESET=reset RESET_ACTIVE=Low RESET_TYPE=Clocked) {
        counter_e <= counter_e + 27'b1;
    }
@endmod

```

:::

## JZ-HDL Language Features

**PLL in the source.** The `CLOCK_GEN` block declares the PLL's input, outputs, and divider parameters directly in the project file. The compiler validates VCO range and divider ratios against the target chip's constraints. Traditional FPGA flows configure PLLs through vendor GUI tools that generate opaque wrapper modules disconnected from the HDL.

**Multi-clock enforcement.** Each `SYNCHRONOUS` block names its clock explicitly. The compiler tracks which registers belong to which domain. Reading a register from `clk1` inside a `clk2` synchronous block would be a compile-time error, catching cross-domain violations before they become metastability bugs on hardware. Verilog has no such check — nothing prevents reading a `clk1` register inside an `always @(posedge clk2)` block.
