# jz-viewer

Native waveform viewer for JZ-HDL `.jzw` (JZW) simulation output. Built on
SDL3 + Dear ImGui with a direct SQLite reader — no intermediate conversion.

Supports **live reload**: open a `.jzw` while the simulator is still writing
to it and watch waveforms stream in.

## Build

The root CMake build fetches bundled dependencies via `FetchContent`:

- **SDL3** (`release-3.2.14`, static)
- **SQLite3** (amalgamation `3.49.01`, built as shared target `jz_sqlite3_lib`)
- **Dear ImGui** (`v1.91.8-docking`) with the SDL3 + SDLRenderer3 backends

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The resulting binary is `build/viewer/jz-viewer`.

Requires a C++17 compiler and CMake >= 3.16. No system packages are required
beyond the normal build toolchain.

## Usage

```sh
jz-viewer <file.jzw>
```

The file must be produced by the JZ-HDL simulator with the `--jzw` flag:

```sh
jz-hdl --simulate sim_file.jz --jzw
jz-viewer sim_file.jzw
```

`--jzw` can be combined with `--vcd` / `--fst` to produce multiple formats
in one run (see `specification/jzw-specification.md` §7).

Example using an in-tree test artifact:

```sh
jz-hdl --simulate compiler/tests/validation/some_sim.jz --jzw -o /tmp/out.jzw
build/viewer/jz-viewer /tmp/out.jzw
```

### Live reload

The viewer opens the database `READWRITE` so it joins the simulator's WAL
session as a concurrent reader. While the simulation is running, the viewer
polls for new rows every ~250 ms and, when **live-follow** is enabled,
auto-scrolls the viewport so the most recent activity stays visible. Pressing
any horizontal navigation key disables follow; `End` re-enables it.

`READWRITE` mode is required by SQLite WAL for a cooperating reader — the
viewer never issues `INSERT`/`UPDATE`/`DELETE` against the database.

## Features

### Signal tree

- Scopes grouped as `clocks`, `wires`, and instance hierarchy from `TAP`
  (matches the JZW `signals.scope` convention).
- Per-signal visibility toggle.
- Drag-reorder of visible signals.
- Multi-bit signals can be expanded to show individual bits.
- Hover tooltip reports signal id, width, type, and end time.

### Waveform display

- Discrete zoom ladder with presets from sub-ns up to seconds-per-division.
- Single-bit signals render as logic-level traces; buses render as hex-value
  transitions.
- Hovering a bus segment pops a tooltip with hex / decimal / binary value
  plus the segment start, end, and duration.
- High-impedance (`z`) bits from the spec's value encoding are rendered
  distinctly from driven levels.

### Cursors

Four independent cursors (**C1–C4**) can be placed anywhere on the timeline.
Hovering the cursor label shows the delta between paired cursors
(`C1↔C2`, `C3↔C4`) formatted as a human-readable time.

### Annotations

All four annotation types defined in the JZW spec §5 are rendered:

| Type     | Rendering                                                  |
| :------- | :--------------------------------------------------------- |
| `mark`   | Full-height vertical line in the annotation color          |
| `alert`  | Full-height vertical line; tooltip lists condition + msg   |
| `select` | Colored range highlight on the associated signal row       |
| `trace`  | Visual gap / dimmed region for `@trace(state=off)` windows |

Color names map to the spec's fixed palette
(`RED`, `ORANGE`, `YELLOW`, `GREEN`, `BLUE`, `PURPLE`, `CYAN`, `WHITE`).

### Clock info dialog

A toolbar button opens a panel sourced from the JZW `clocks` table (spec §3.5),
showing per-clock period, frequency (MHz), phase (ps / degrees), peak-to-peak
jitter, and drift (max ppm, actual ppm, drifted period). Fields that are
zero / unconfigured are shown dimmed as "disabled".

### Toolbar

Filename + top-level module name, live-status indicator ("Watching" when the
simulation is still running), zoom readout, and zoom / fit / clock-info
buttons.

## Keyboard shortcuts

| Key                | Action                                           |
| :----------------- | :----------------------------------------------- |
| `+` / `=`          | Zoom in (centered on active cursor or viewport)  |
| `-`                | Zoom out                                         |
| `F`                | Fit entire simulation to view                    |
| `←` / `→`          | Scroll horizontally                              |
| `Shift` + `←`/`→`  | Scroll horizontally, 5× step                     |
| `↑` / `↓`          | Scroll signal list                               |
| `Home`             | Jump to simulation start                         |
| `End`              | Jump to simulation end + re-enable live-follow   |
| `Esc`              | Clear cursors (last placed first); deselect tool |
| `Ctrl` + wheel     | Zoom centered on mouse position                  |
| Wheel              | Scroll signal list                               |
| `Shift` + wheel    | Scroll timeline horizontally                     |

Shortcuts are suppressed while a text-input widget has focus.

## JZW format

The viewer is a direct consumer of the JZW format. See
`specification/jzw-specification.md` for the full schema.

Tables read by the viewer:

| Table         | Purpose                                                   |
| :------------ | :-------------------------------------------------------- |
| `meta`        | `module_name`, `sim_end_time`, `tick_ps`, etc.            |
| `signals`     | Signal catalog (`id`, `scope`, `name`, `width`, `type`)   |
| `changes`     | Value changes keyed by `(signal_id, time)`                |
| `annotations` | `mark` / `alert` / `select` / `trace` entries             |
| `clocks`      | Per-clock period / phase / jitter / drift (optional)      |

The `clocks` table is treated as optional for backward compatibility with
older `.jzw` files (spec §3.5).

## Architecture

Single-file implementation in `src/main.cpp`, organized as:

- **Data model** (`Signal`, `ValueChange`, `Annotation`, `ClockInfo`, `JZWFile`)
  — plain structs plus a `std::map<int, std::vector<ValueChange>>` keyed by
  signal id.
- **`JZWFile::load`** — one-shot initial read of all tables.
- **`JZWFile::poll`** — incremental read using `max_loaded_time` and
  `max_annotation_id` watermarks; appends new rows without rescanning.
- **Rendering** — ImGui immediate-mode; waveform lanes are drawn via
  `ImDrawList` primitives for performance on dense traces.
- **Main loop** — event-driven idle (`SDL_WaitEventTimeout`) when static,
  continuous 60 fps when `watching` or `sim_running` is true.

## Limitations / TODO

- **No file-open dialog.** The `.jzw` path must be passed as a CLI argument.
- **No persistence.** Signal visibility, ordering, zoom level, and cursor
  positions are not saved between runs.
- **Single source file.** `src/main.cpp` holds the full implementation
  (~2.5k lines); it has not yet been split into modules.
- **No search / filter** on the signal tree.
- **No VCD / FST support.** JZW only — use GTKWave for those formats.
