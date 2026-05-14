# jz-hdl-dev 1.0.0 Release Readiness

**Status:** Review complete
**Date:** 2026-05-11
**Reviewer:** Codex (GPT-5)

## Exclusions
- `datasheets/`
- `.git/`
- `build/`, `target/`, `dist/`, `out/`, `node_modules/`
- `compiler/build/`, `viewer/build/`, `docs/.vitepress/dist/`, `vscode-ext/out/`
- generated example outputs including emitted `*.jzw` files and reports

## Top 1.0.0 Blockers (cross-cutting)
- Release framing is still visibly pre-1.0: the core specifications still say `State: Beta — Version: 0.2.0`, the docs site republishes that framing, and at least one CLI example still shows `Version 0.2.0`.
- Clean-build reproducibility is not release-grade yet: the root build fetches `sqlite3`, the viewer fetches SDL3 and ImGui, and the docs publish path still depends on non-lockfile `npm install`.
- Simulator artifacts are not yet reproducible: VCD, FST, and JZW outputs embed wall-clock timestamps, and all three waveform writers still impose a fixed 4096-signal ceiling.
- The examples tree over-promises in a few places: multiple roots advertise `simulate` targets without a local simulation harness, and some copied clean rules do not match actual outputs.
- Chip-data validation is not yet strict enough for a 1.0.0 contract: duplicate `clock_gen` entries can exist in built-ins and legacy compatibility paths still widen the accepted schema.
- The viewer and VS Code extension are functional, but their release packaging stories are still incomplete: network-coupled viewer builds, thin marketplace assets for the extension, and limited UI-level regression coverage.

## Overall Score: 7 / 10
The project is solidly above “usable but rough” and below “production-ready with only minor rough edges.” I weighted the compiler frontend, IR, backends, diagnostics, simulator, and verification stack most heavily, because those are the critical-path 1.0.0 promises; that core is strong, generally scores 7 to 8, and looks materially closer to release than the surrounding tooling. The overall score stays at 7 because the remaining blockers are concentrated in release polish rather than core language capability: stale beta framing, nondeterministic simulator artifacts, non-reproducible build and docs paths, and avoidable example and chip-data hygiene issues. The viewer and VS Code extension pull less weight than the compiler core, but both still reinforce the same conclusion: the project already feels real, yet it has not fully crossed the line into a crisp, deterministic, audited 1.0.0 release system. The top three blockers are release framing and version drift, reproducible builds and publication, and simulator artifact determinism.

---

## Groups

### 1. Language & Specification — Score: 7 / 10
**Paths owned:** `specification/`, `pipeline/`
**Criteria scored against:**
- completeness versus implemented language, simulation, testbench, chip-info, and waveform behavior
- consistency across specifications and rule-coverage documents
- clarity for a new reader and suitability as 1.0.0 reference material
- stale content, dead references, and versioning/release framing
**Last reviewed:** 2026-05-11
**Rationale:**
The core specs are broad and mostly implementation-aligned: the simulator and CLI already support the major documented surfaces, including `@simulation`, `@testbench`, `@run_until`, `@run_while`, `@trace`, `@mark_if`, `@alert_if`, `--jzw`, jitter/drift, and JZW clock metadata. The main gap is not feature coverage but polish and consistency: all five spec docs still carry a Beta 0.2.0 framing, and there are several stale or misnumbered internal references in the language and testbench docs. That puts this comfortably above “usable but rough” but still below 1.0.0 reference quality.

**Key measurements:**
- 6 specification files, totaling 10,398 lines.
- 18 pipeline files, totaling 3,313 lines, including a committed `pipeline/__pycache__/run_pipeline.cpython-314.pyc` artifact.
- 5/5 spec front-matter banners still say `State: Beta — Version: 0.2.0`.
- 9 literal `TODO`/`FIXME`/`XXX`/`HACK`/`stub`/`unimplemented` matches in `pipeline/`, all in prompt/config text rather than implementation code.
- 0 literal marker hits in the spec documents themselves.
- At least 4 obvious stale or misnumbered self-references in the core spec set, including `Section 4.10`, `Section 4.9`, and `Section 6.8` references that no longer point cleanly at the intended material.
- The simulator spec and the implementation agree on the important runtime surface, including waveform formats, per-clock jitter/drift, and monitor/trace behavior.

**Needed before 1.0.0:**
- Replace the Beta 0.2.0 front matter across all five spec docs with 1.0.0-ready release framing, or explicitly document why the project is still intentionally pre-1.0.
- Audit and fix the stale section references in `jz-hdl-specification.md`, `testbench-specification.md`, and `simulation-specification.md` so the docs read cleanly as a single authoritative set.
- Add a concise release-status or compatibility note that tells new readers what is guaranteed stable at 1.0.0 and what is intentionally frozen.
- Decide whether `pipeline/__pycache__/run_pipeline.cpython-314.pyc` belongs in the repository; if not, remove it before release and ignore it going forward.
- Normalize the `simulation-specification.md` section order and cross-references so the structure matches the narrative flow without requiring the reader to mentally correct numbering drift.

**Surprising findings:**
- The implementation already covers most of the documented simulation and waveform surface, so the remaining issue is documentation quality, not missing runtime capability.

### 2. Compiler — Frontend — Score: 8 / 10
**Paths owned:** `compiler/src/lexer.c`, `compiler/src/parser/`, `compiler/src/ast/`, `compiler/src/sem/`, `compiler/src/repeat_expand.c`, `compiler/src/compiler.c`, `compiler/include/`
**Criteria scored against:**
- feature completeness versus the core language spec
- parser and semantic correctness on edge cases
- diagnostic quality, source locations, and recovery behavior
- test coverage, TODO density, and obvious memory-safety or UB risks
**Last reviewed:** 2026-05-11
**Rationale:**
The frontend is broadly feature-complete for the core language surface: the parser and semantic passes cover modules, projects, declarations, control flow, feature guards, templates, instantiation, testbenches, simulation, and MEM `@file()` handling, with rule-based diagnostics and source locations threaded through the stack. The codebase also looks mature from a safety standpoint, with no TODO/FIXME/HACK/abort-style markers in the owned tree and no obvious memory-safety shortcuts in the main control paths. It still lands short of 9 because recovery and test coverage are uneven, and a few areas rely on compatibility-specific parsing rather than a uniformly polished grammar.

**Key measurements:**
- 74 owned files totaling 53,078 LOC.
- 19 parser files totaling 11,981 LOC.
- 26 semantic-analysis files totaling 35,172 LOC.
- 2 AST source files totaling 455 LOC.
- 24 public headers totaling 3,721 LOC.
- 97 AST node kinds in `compiler/include/ast.h`.
- 119 token kinds in `compiler/include/lexer.h`.
- 3 frontend unit-test files under `compiler/src/sem/` and 0 parser unit-test files in the owned tree.
- 0 TODO/FIXME/XXX/HACK/assert(0)/abort(/not implemented/stub hits in the owned tree.

**Needed before 1.0.0:**
- Add parser-focused regression coverage for malformed directives, unterminated and nested blocks, recovery after bad tokens, and edge cases around `@feature`, `@apply`, `@new`, and `@file()`.
- Tighten recovery in the largest parser paths so one bad construct does not collapse the rest of a file.
- Normalize the remaining lexer/parser compatibility wrinkles, especially around `@file`, so there is one clearly intended tokenization path.
- Add a release-grade frontend test matrix that covers both positive spec conformance and diagnostic/source-location quality.

**Surprising findings:**
- `@file()` is already supported end to end, including a compatibility path that accepts both `@` plus `file` tokens and a single `@file` lexeme.
- The frontend is larger and more complete than it reads at first glance: rule-driven diagnostics are already wired through lexer, parser, and semantics rather than being bolted on later.

### 3. Compiler — IR & Middle-end — Score: 8 / 10
**Paths owned:** `compiler/src/ir/`
**Criteria scored against:**
- IR completeness for all frontend constructs that must lower cleanly
- transformation correctness and invariants preservation
- serialization/debuggability and internal API cohesion
- test coverage, TODO density, and obvious correctness risks
**Last reviewed:** 2026-05-11
**Rationale:**
The IR middle-end is broadly feature-complete and internally coherent: it lowers the core frontend constructs into a rich IR, runs dedicated passes for tri-state elimination, memory initialization, differential metadata, dead-module elimination, and division-guard analysis, and serializes the result with depth protection. The tree also looks mature operationally, with no TODO/FIXME/XXX/HACK/stub markers in `compiler/src/ir/` and broad validation coverage around the main transform paths. It still lands below 9 because there are a few real rough edges, notably fixed-size instance-collection buffers, best-effort memory binding, and some unsupported lowering fallbacks that should be closed or rejected earlier before 1.0.0.

**Key measurements:**
- `compiler/src/ir/` contains 16 source and header files totaling 23,539 LOC.
- The pass surface is compact and cohesive: design build, tri-state transform, differential lowering, memory init lowering, div-guard checks, dead-module elimination, cloning, library-module generation, and JSON serialization.
- A literal scan of `compiler/src/ir/` found 0 TODO/FIXME/XXX/HACK/assert(0)/abort(/not implemented/stub/unimplemented markers.
- Validation coverage exists for the main IR surfaces, including div-guard, tri-state transform, memory handling, instance specialization, CDC lowering, and assignment/statement lowering.
- The JSON serializer covers the major IR shapes end to end, including signals, expressions, statements, memories, clock domains, instances, CDC crossings, and project metadata, with depth-limit protection.

**Needed before 1.0.0:**
- Replace the fixed 512-entry instance-collection buffers in specialization and elaboration with dynamic growth or explicit overflow diagnostics.
- Decide whether complex assignment targets such as concatenation LHS should be supported in IR; if yes, lower them, and if not, move the rejection into semantics and add a regression test.
- Make memory-port binding deterministic by reporting unresolved bindings or proving them earlier, rather than silently treating it as best effort.
- Remove or justify the remaining “unsupported for now” lowering fallbacks that can still reach release paths, especially around expression forms that are only preserved indirectly for backends.
- Reframe the serialized IR versioning if the JSON output is intended to be a stable 1.0.0 debug artifact.

**Surprising findings:**
- The middle-end is more complete than the file layout suggests: the IR builder already handles BUS accesses, memory reads and writes, CDC metadata, clock-domain reset wrapping, and post-build transforms in one pipeline.
- The JSON exporter still hardcodes an `ir_version` of `0.1.0`, so the internal wire format is not yet being presented as release-stable even though the implementation is otherwise mature.

### 4. Compiler — Backends — Score: 8 / 10
**Paths owned:** `compiler/src/backend/`
**Criteria scored against:**
- feature completeness versus supported synthesis targets
- output correctness and conformance for Verilog-2005 and RTLIL
- constraint and wrapper generation quality
- test coverage, TODO density, and backend-specific failure handling
**Last reviewed:** 2026-05-11
**Rationale:**
This backend layer is already production-capable for the supported release targets: Verilog-2005, RTLIL, and the board-constraint formats (`SDC`, `XDC`, `PCF`, `CST`). The emitters are not thin wrappers; they handle alias resolution, module ordering, project wrappers, chip-data-driven differential I/O, clock-generator expansion, memory initialization, and safety-checked file output. Output hygiene is also solid: the Verilog backend emits Yosys-oriented headers and escapes strings and names carefully, while the RTLIL backend resets IDs, enforces memory-init limits, and reports I/O failures through diagnostics.

It still stops short of a 9 because there are a few release-quality gaps rather than broad missing subsystems. The owned tree still has one literal `TODO` and a couple of `unsupported` emission paths, the RTLIL wrapper still has an open note for multi-bit inverted ports, and some chip-data-dependent cases fall back to generic primitives or comments when templates are unavailable. Those are bounded gaps, but they are real polish and conformance risks for a 1.0.0 synthesis story.

**Key measurements:**
- `compiler/src/backend/` contains 20 files totaling 13,354 LOC: 11 Verilog-2005 files and 9 RTLIL files.
- Marker scan found 1 `TODO`, 2 `unsupported` references, and no `FIXME`, `HACK`, `stub`, or `unimplemented` hits in the owned tree.
- `compiler/tests/golden/` includes 71 `.jz` inputs, 60 Verilog outputs, 57 RTLIL outputs, and 5 constraint outputs (`1 .sdc`, `2 .xdc`, `1 .pcf`, `1 .cst`).
- There are 24 shell-based golden/backend checks, including dedicated coverage for RTLIL memory-init limits and constraint escaping.
- `compiler/tests/run_validation.sh` already includes Yosys parse and Verilog-vs-RTLIL equivalence checks for backend outputs.

**Needed before 1.0.0:**
- Close the remaining RTLIL wrapper TODO, especially multi-bit inverted-port handling, instead of leaving it as a known gap.
- Replace or harden fallback emission paths that silently downgrade unsupported chip-data cases to comments or generic primitives.
- Add explicit regression coverage for backend failure paths that are still only exercised indirectly, especially around wrapper generation and RTLIL unsupported-statement handling.
- Decide whether the backend-specific golden coverage should stay concentrated in `compiler/tests/golden/` or also be mirrored into `tests/validation/` for release-gate visibility.

**Surprising findings:**
- `compiler/tests/validation/` has no committed `.v` or `.il` golden files, so the Verilog/RTLIL parse and equivalence checks in `run_validation.sh` are effectively backed by `compiler/tests/golden/`, not the main lint corpus.
- The wrapper and constraint side is more mature than the file layout suggests: the Verilog backend already emits and escapes four constraint dialects and handles differential I/O and clock-gen templates with chip-data-aware logic.

### 5. Simulator — Score: 7 / 10
**Paths owned:** `compiler/src/sim/`, `specification/simulation-specification.md`, `specification/jzw-specification.md`
**Criteria scored against:**
- simulation-spec and waveform-spec conformance
- correctness, determinism, and runtime behavior on realistic designs
- waveform output quality across `vcd`, `fst`, and `jzw`
- performance sanity, test coverage, and obvious stub or error-path gaps
**Last reviewed:** 2026-05-11
**Rationale:**
The simulator is broadly implemented and covers the major documented runtime surface: `@simulation`, `@run`, `@run_until`, `@run_while`, `@update`, `@trace`, `@mark`, `@alert`, `MONITOR`, jitter/drift, JZW metadata, and waveform emission for VCD, FST, and JZW. The implementation is not a stub and the validation corpus shows real coverage for directive syntax, timing conversion, annotation rules, and select-chain behavior. The release blocker is determinism and polish, not basic feature existence: VCD, FST, and JZW all stamp wall-clock date information, so identical source and seed do not yet guarantee bit-identical waveform artifacts. There is also a fixed 4096-signal ceiling in all three waveform writers, which is a real limit for larger designs and should be surfaced as an explicit release constraint or removed before 1.0.0.

**Key measurements:**
- `compiler/src/sim/` contains 20 source and header files totaling 9,419 LOC.
- Validation coverage includes 73 simulator and JZW fixtures under `compiler/tests/validation/`, plus 8 focused runtime fixtures under `compiler/tests/simulation/`.
- JZW-specific coverage exists for select chains, select scope resolution, annotation color validation, trace toggles, exact picosecond conversion, and wrong-tool gating.
- A literal scan of `compiler/src/sim/` found no `TODO`/`FIXME`/`XXX`/`HACK`/`stub`/`unimplemented` markers in the core simulator paths; the only hits are deliberate no-op perf stubs in `sim_perf.h`.
- The JZW backend implements the documented schema surface, including `meta`, `signals`, `changes`, `annotations`, and `clocks`, plus prepared-statement batching.
- The runtime engine already has deterministic PRNG seeding, event-driven clock scheduling, exact picosecond conversion checks, monitor evaluation, and runtime timeout and error paths.

**Needed before 1.0.0:**
- Remove wall-clock date stamping from VCD, FST, and JZW output paths, or document and isolate it so identical inputs produce bit-identical artifacts.
- Add an explicit regression test that compares simulator output across repeated runs with the same seed to prove determinism.
- Replace the fixed 4096-signal cap in the waveform backends with dynamic growth or a hard diagnostic that is documented as a supported release limit.
- Add direct tests for large-design waveform emission and for failure behavior when signal registration or annotation tables approach capacity.
- Audit the VCD and FST timestamp handling against the ps-resolution engine so sub-ns activity is either preserved or intentionally documented as lossy.

**Surprising findings:**
- The simulator is materially more complete than the file list suggests: the same engine already drives both `@testbench` and `@simulation`, and the waveform backends are not thin wrappers.
- The main release risk is not missing features but nondeterministic artifact generation, which is a stronger blocker than the feature surface itself.

### 6. Testbench & Verification — Score: 8 / 10
**Paths owned:** `compiler/src/parser/parser_testbench.c`, `compiler/src/sem/driver_testbench.c`, `compiler/tests/`
**Criteria scored against:**
- completeness versus the testbench specification
- quality and isolation of validation fixtures and expected outputs
- breadth of automated verification for compiler behavior
- maintainability of the validation harness and test signal-to-noise
**Last reviewed:** 2026-05-11
**Rationale:**
The testbench stack is broadly release-ready. The parser and semantic driver cover the documented `@testbench` surface end to end: `CLOCK`, `WIRE`, file-level and in-block `@import`, BUS declarations and BUS shorthand bindings, `@new`, `@setup`, `@update`, `@clock`, `@expect_equal`, `@expect_not_equal`, `@expect_tristate`, `@print`, and `@print_if`. The verification corpus is large and intentional rather than ad hoc: fixtures are named around specific rules, expected outputs are usually minimal, and the harness already exercises lint-style validation, simulation, golden backend outputs, Yosys parse checks, backend equivalence, testbench smoke runs, simulation smoke runs, and cross-mode rejection. It stays at 8 rather than 9 because the verification story is still heavily fixture- and shell-script-driven, helper inputs are mixed into the same validation tree as executable cases, and the main runner is a sizable convention-based script rather than a smaller layered harness.

**Key measurements:**
- The owned implementation is compact relative to the coverage surface: `parser_testbench.c` is 914 LOC and `driver_testbench.c` is 1,172 LOC.
- The main validation corpus has 1,505 `.jz` fixtures and 1,468 expected `.out` files under `compiler/tests/validation/`.
- The validation tree is broad by category: 93 `TB_*`, 60 `SIM_*`, and 13 `JZW_*` validation cases, plus 38 helper inputs that intentionally have no `.out`.
- Golden coverage is substantial: 71 golden `.jz` inputs, 60 Verilog outputs, 57 RTLIL outputs, and 24 `test.sh`-driven golden scripts.
- `compiler/tests/run_validation.sh` currently reports 1,889 passes, 0 failures, and 38 skips, including 114 Yosys parse checks, 56 equivalence checks, 5 testbench smoke tests, 5 simulation smoke tests, and 5 cross-mode rejection checks.
- A literal marker scan found no `TODO`, `FIXME`, `XXX`, `HACK`, `stub`, `unimplemented`, `assert(0)`, or `abort(` hits in the owned parser and semantics files.

**Needed before 1.0.0:**
- Split or factor the validation harness enough that the core flows are easier to understand and extend without following filename conventions through a large shell script.
- Make the skipped helper fixtures more explicit in the corpus layout so the 38 skips are clearly intentional instead of just implicit by filename.
- Add a few more focused negative testbench fixtures for parser and semantic edge cases that are still mostly exercised indirectly through the larger regression matrix.
- Reduce the remaining `test.sh`-only golden cases where a declarative `.jz` plus `.out` fixture would express the same behavior without shell-side logic.

**Surprising findings:**
- `@repeat` is handled as a pre-parser expansion step, so the testbench and simulation parser never sees the raw directive structure; that keeps the parser simpler, but it also means repeat regressions live in the expansion harness rather than the parser.
- The validation runner already normalizes volatile waveform paths and filters version lines before comparison, which is why the golden outputs stay stable across machines even though the underlying tools emit environment-specific text.

### 7. Diagnostics & Reports — Score: 8 / 10
**Paths owned:** `compiler/src/diagnostic.c`, `compiler/src/rules.c`, `compiler/src/report/`
**Criteria scored against:**
- diagnostic clarity, consistency, and explainability
- lint-rule surface area and report usefulness
- source mapping, grouping, warning-policy behavior, and output polish
- test coverage and obvious message-quality gaps
**Last reviewed:** 2026-05-11
**Rationale:**
This is a strong, mostly release-ready diagnostics layer. The core renderer already does the right hard things: rule-aware severity mapping, stable sorting, file and line grouping, duplicate suppression on same-location output, project-relative filename normalization, `--explain` support, colorized rendering, and warning-policy filtering by group. The rule table is broad and well-structured, with clear groupings and readable messages, and the dedicated report emitters for aliasing, tri-state analysis, and memory resource reporting are substantive rather than cosmetic.

It stays at 8 instead of 9 because the direct regression surface is still thinner than the implementation surface. The code supports user-facing knobs like `--warn-as-error`, `--info`, `--color`, `--explain`, `--Wno-group`, `--Eno-group`, and `--lint-rules`, but the tests are still weighted toward semantic fixtures and a few report goldens rather than explicit coverage of every rendering and policy path. The result is good output behavior with a small but real risk of UI-regression drift.

**Key measurements:**
- 6 owned source files totaling 6,355 LOC across diagnostics, rules, and report emitters.
- `compiler/src/rules.c` contains 200-plus rule entries organized into named groups, including `REPORTS`, `GENERAL_WARNINGS`, `TESTBENCH`, `SIMULATION`, and `TRISTATE_TRANSFORM`.
- 1,505 validation fixtures and 1,468 expected `.out` files under `compiler/tests/validation/`.
- 3 dedicated report golden suites (`alias_report`, `memory_report`, `tristate_report`) plus 23 shell-based golden scripts across diagnostics-adjacent features.
- Marker scan found 0 `TODO`, `FIXME`, `XXX`, `HACK`, `stub`, or `unimplemented` hits in the owned tree.
- `run_validation.sh` already exercises the normal lint path with `--info --lint`, while report goldens cover `--alias-report`, `--memory-report`, and `--tristate-report`.

**Needed before 1.0.0:**
- Add direct regression coverage for diagnostic rendering flags, especially `--warn-as-error`, `--explain`, `--color`, and `--info`.
- Add explicit tests for warning-policy group overrides (`--Wno-group` and `--Eno-group`) so suppression behavior is locked down independently of semantic fixtures.
- Add a small `--lint-rules` golden or snapshot test so rule-table output is stable as the table evolves.
- Expand report-mode coverage with at least one negative-path fixture for report traversal limits or depth diagnostics, not just happy-path output.
- Review the rule descriptions for a few remaining wording inconsistencies and ensure the most user-facing messages stay concise and explanatory.

**Surprising findings:**
- The renderer already has release-grade sorting and grouping semantics: same-file and same-line diagnostics are prioritized, deduplicated, and rendered deterministically.
- The report emitters are more useful than their names suggest: alias reporting includes identifier indexing and cross-module summaries, tri-state reporting includes proof obligations, and memory reporting is chip-aware.

### 8. Chip Data & Vendor Support — Score: 7 / 10
**Paths owned:** `compiler/data/`, `compiler/src/chip_data.c`, `compiler/src/chip_data_internal.h`, `specification/chip-info-specification.md`
**Criteria scored against:**
- chip-info spec conformance and schema correctness
- vendor coverage and whether declared resources look usable end to end
- completeness of pin/resource modeling and special-case handling
- validation depth, data hygiene, and drift risk between JSON and compiler expectations
**Last reviewed:** 2026-05-11
**Rationale:**
The chip-data layer is broadly usable and clearly not an afterthought. The built-in JSON set covers four vendor families end to end, and the compiler consumes that data in multiple places: chip-info reporting, memory reporting, semantic validation, backend wrapper generation, and differential I/O lowering. The schema validator is also reasonably strict for the top-level structure and the nested `clock_gen`, `differential`, memory, and fixed-pin shapes, including exhaustive and disjoint validation for `variants`.

It stops short of a 1.0.0-ready 8 because there are still real hygiene and drift risks. The loader keeps legacy compatibility paths for some memory and differential shapes, which is fine for transition but weakens the release contract. More importantly, the implementation does not enforce a few semantic invariants the spec and consumers rely on, and the iCE40 chip files currently contain duplicate `osc` entries with the same `type` and `mode`, making one definition unreachable through the current lookup API. That is the kind of data issue that can sit quietly until a backend or report path asks for the wrong entry.

**Key measurements:**
- 9 built-in chip JSON files are present, spanning Gowin, iCE40, ECP5, and Xilinx 7-series devices.
- 8 of 9 built-ins include a `differential` section; `gw5a-lv25-mg121-c1-i0` intentionally omits it.
- 40 chip-data-focused validation fixtures exist, covering malformed JSON, missing required sections, bad differential keys and types, `map` versus `variants` exclusivity, variant coverage, and clock-gen parameter errors.
- The owned chip-data code path is marker-clean in the scanned files: no `TODO`, `FIXME`, `XXX`, `HACK`, `stub`, or `unimplemented` hits in `compiler/src/chip_data.c` or the chip-info spec.
- The chip-data API is used directly by `compiler/src/sem/driver_project_hw.c`, `compiler/src/report/chip_report.c`, `compiler/src/report/memory_report.c`, and both backend wrapper emitters.

**Needed before 1.0.0:**
- Enforce the semantic invariants the compiler already depends on, especially rejecting duplicate `clock_gen` entries with the same `type` and `mode` and validating required outputs such as `BASE`.
- Remove or split the duplicate `osc` definitions in both iCE40 chip JSON files so every declared clock generator is reachable by lookup.
- Add end-to-end regression coverage for each built-in chip JSON so report output, clock-gen lookup, differential handling, and fixed-pin metadata stay aligned with the compiler’s expectations.
- Tighten the loader contract around legacy compatibility paths so only intentionally supported JSON shapes remain accepted.

**Surprising findings:**
- Chip data is not just for synthesis; the same payload drives user-facing reports, semantic checks, backend generation, and vendor-specific constraints.
- `variants` coverage is already validated exhaustively and disjointly at load time, which is stronger than the rest of the chip JSON validation surface.

### 9. CLI & LSP — Score: 7 / 10
**Paths owned:** `compiler/src/main.c`, `compiler/src/cli_*.c`, `compiler/src/path_security.c`, `compiler/src/lsp/`
**Criteria scored against:**
- CLI surface completeness and stability for 1.0.0 users
- path-safety, IO robustness, and mode dispatch correctness
- LSP feature completeness and resilience on malformed input
- test coverage, packaging ergonomics, and user-facing error handling
**Last reviewed:** 2026-05-11
**Rationale:**
The CLI surface is mostly complete and the docs are already aligned with it: the entrypoint exposes the expected modes, `--chip-info`, `--lint-rules`, path-safety flags, expansion limits, and `--lsp`, and the README plus CLI usage docs describe the same surfaces. The LSP server is also real rather than skeletal: it handles initialize and shutdown, diagnostics, hover, completion, definition, project selection, project discovery, and cached clock metadata. The score stops at 7 because the release-hardening story is still uneven. Top-level dispatch has only indirect coverage for some standalone flags, malformed LSP payloads are mostly dropped or truncated rather than turned into explicit protocol errors, and diagnostics still collapse file identity to basenames during publish, which is risky in multi-file workspaces with duplicate filenames.

**Key measurements:**
- 10 owned source and header files totaling 5,538 LOC.
- 3 LSP-specific shell goldens and 23 path-security validation cases.
- The validation runner already exercises lint, golden backends, Yosys parse and equivalence, testbench, simulation, and cross-mode rejection paths.
- `README.md` and `docs/getting-started/cli-usage.md` both document `--lsp`, the major modes, path-safety flags, and editor integration.
- The LSP transport layer caps message bodies at 16 MiB and uses a 250 ms read timeout; the server also keeps fixed-capacity stores for 128 open docs, 32 discovered projects, and 32 sandbox roots.

**Needed before 1.0.0:**
- Add direct CLI smoke coverage for standalone dispatch and error paths, especially `--help`, `--version`, `--lint-rules`, mode-conflict rejection, and unknown-option handling.
- Return explicit JSON-RPC errors for malformed LSP requests instead of silently ignoring bad params or relying on fixed-size intermediate buffers.
- Preserve canonical file identity when publishing diagnostics so files with the same basename in different directories do not collide.
- Decide whether the fixed caps on open documents, discovered projects, and sandbox roots are supported release limits or should be replaced with dynamic growth plus explicit diagnostics.

**Surprising findings:**
- The LSP server already exposes more than the typical minimum editor surface, including a custom `jz-hdl/projectInfo` notification and project override flow.
- Path sandboxing is stricter than the CLI docs alone suggest: absolute paths, traversal, and symlink escapes all have dedicated diagnostics and dedicated validation fixtures.

### 10. Waveform Viewer — Score: 6 / 10
**Paths owned:** `viewer/`
**Criteria scored against:**
- clean-build and run readiness on a fresh machine
- feature completeness versus emitted waveform formats
- packaging and release readiness for 1.0.0 distribution
- error handling, UX rough edges, and obvious maintenance risks
**Last reviewed:** 2026-05-11
**Rationale:**
The viewer is a capable JZW-native UI: it handles live reload, signal expansion, annotations, clocks, cursors, and waveform rendering without obvious stub paths, and the SQLite loading code is careful about text bounds and quota enforcement. It is not 1.0.0-ready yet because the build is network-coupled at configure time (`FetchContent` pulls SQLite, SDL3, and ImGui from upstream URLs), the release surface is intentionally narrow (`.jzw` only, with VCD and FST explicitly out of scope), and the interactive UI has no dedicated regression coverage beyond a `--validate` smoke path in CI. The initial open path also insists on `SQLITE_OPEN_READWRITE`, which blocks read-only archives and makes the viewer less robust on packaged or locked-down installs. The monolithic `viewer/src/main.cpp` is a maintenance hotspot.

**Key measurements:**
- `viewer/src/main.cpp` is 3,125 LOC and owns the full runtime and UI path.
- `viewer/CMakeLists.txt` is 60 LOC, but it fetches SDL3 and ImGui from GitHub at configure time, while the root build also fetches sqlite3 from sqlite.org.
- CI has one `viewer-smoke` job, and it exercises `--validate` only, not the interactive GUI or live-reload UI.
- `viewer/README.md` documents explicit limitations: no file-open dialog, no persistence, no modularization, and no VCD or FST support.

**Needed before 1.0.0:**
- Add an offline or vendored dependency path so a clean build does not depend on live network fetches during configure.
- Decide whether JZW-only support is the release boundary; if it is, document that clearly as the supported scope, otherwise add support for the other emitted waveform formats.
- Add at least one GUI-level smoke or screenshot regression test for the interactive viewer, not just `--validate`.
- Relax the initial `SQLITE_OPEN_READWRITE` load path or add a read-only fallback so archived traces on read-only media still open.
- Split `viewer/src/main.cpp` or isolate loading, state, and rendering into separate units before release.

**Surprising findings:**
- The viewer feature set itself is stronger than the file layout suggests: live reload, four cursors, signal-tree expansion, annotation rendering, and a clock dialog are already implemented.
- The strongest release risk is packaging and portability, not missing core waveform rendering logic.

### 11. VS Code Extension — Score: 7 / 10
**Paths owned:** `vscode-ext/`
**Criteria scored against:**
- install/build/package readiness and editor integration quality
- LSP client behavior, configuration ergonomics, and fallback behavior
- documentation, smoke coverage, and marketplace/release readiness
- stale build artifacts, dependency hygiene, and obvious UX gaps
**Last reviewed:** 2026-05-11
**Rationale:**
The extension is functional and reasonably well integrated: it compiles to `out/`, packages a VSIX, activates on `jz-hdl` files and `.jz` workspaces, starts an LSP client against either `jz-hdl` on `PATH` or a configured binary, and exposes a project picker plus status-bar state. The smoke path is real rather than symbolic: it packages the extension, installs the VSIX into a downloaded VS Code, and exercises startup without LSP, missing and non-file binary failures, project discovery, project selection, go-to-definition, and diagnostics against fixture workspaces.

It still lands at 7 because release packaging is thin and a few polish gaps remain. There is no `README.md`, `CHANGELOG.md`, icon, or other marketplace-facing content under `vscode-ext/`, the package version is still `0.2.0`, the automated coverage is concentrated in one end-to-end smoke suite plus a small test harness, and binary selection is still mostly settings-driven rather than discoverable from the UI. I did not find marker debt in the owned source, but the release story is not yet complete enough to call polished 1.0.0 material.

**Key measurements:**
- 2 extension source files (`src/extension.ts` and `src/test/suite/index.ts`), 1 smoke script, and 5 smoke fixtures under `test-fixtures/smoke-workspace/`.
- `package.json` defines `compile`, `package:vsix`, `smoke`, `test`, and a `vscode:prepublish` hook.
- The extension contributes one language, one grammar, one command, and three user-facing settings (`binaryPath`, `lsp.enabled`, and two hover toggles).
- Smoke coverage spans startup-disabled-LSP, missing binary, non-file binary, ambiguous project selection, project switching, go-to-definition, and diagnostics.
- Literal marker scan found 0 `TODO`/`FIXME`/`XXX`/`HACK`/`stub`/`unimplemented` hits in `vscode-ext/src/` and `vscode-ext/scripts/`.

**Needed before 1.0.0:**
- Add marketplace-facing release assets and docs: `README.md`, changelog or release notes, and an icon or banner if the extension is meant to ship publicly.
- Bump the extension package version from `0.2.0` and verify the VSIX metadata matches the release train.
- Add at least one focused regression test for settings persistence or restart behavior so config-driven LSP fallback is covered beyond the current smoke path.
- Reduce reliance on manual settings for binary selection by adding a clearer UI affordance or quick-pick path for `jz-hdl.binaryPath`.
- Audit whether any generated release artifacts are intentionally tracked; if not, keep build outputs out of the published tree.

**Surprising findings:**
- The extension already handles the main failure modes cleanly: it validates configured binaries up front, falls back to `PATH` when unset, and emits targeted startup diagnostics instead of failing silently.
- The smoke suite is more than a launch check; it verifies project discovery and editor features end to end against real workspace fixtures.

### 12. Examples — Score: 7 / 10
**Paths owned:** `examples/` source trees and per-example `Makefile`s
**Criteria scored against:**
- buildability and simulation usefulness from clean source inputs
- representativeness of the language and platform feature set
- code quality and whether examples teach good patterns
- explanation quality, maintenance burden, and stale-output risk
**Last reviewed:** 2026-05-11
**Rationale:**
The examples tree is broadly strong and genuinely useful as a release corpus. It covers the language from small, teachable designs (`counter`, `latch`) through clocking and multi-domain examples (`domains`, `pll`) into larger integration demos (`dvi`, `dvi_audio`, `uart_audio`, `terminal`, `cpu`, `soc`, `ascon`, `uart_echo`, `lcd`). The source quality is generally good, and the best examples do teach solid patterns with clear comments and readable structure.

It is not 1.0.0-clean yet. The build story is inconsistent across roots: only 5 example trees actually ship a local `src/simulation.jz` harness (`counter`, `domains`, `dvi`, `latch`, `pll`), while the other 8 root `Makefile`s still expose `simulate` targets that point at a missing file. Maintenance hygiene is also uneven: `examples/cpu/Makefile` cleans the wrong outputs, `examples/soc/Makefile` leaves `reports/` behind, and several docs read like working notes or stale snapshots rather than release guidance (`examples/soc/bios.md`, `examples/status.md`, `examples/terminal/tools/escape commands.md`). The result is useful and representative, but still rough enough to keep it below 8.

**Key measurements:**
- 13 example roots and 14 `Makefile`s cover 138 `.jz` source files.
- Only 5 roots have a dedicated `src/simulation.jz` harness; 8 root `Makefile`s still advertise `simulate` without a matching local source file.
- 6 Markdown docs and notes live under `examples/`, including `examples/coverage.md` and a generated status snapshot dated `2026-05-06`.
- Tracked report-like outputs exist in `examples/cpu/reports/` and `examples/soc/reports/`, plus generated data files such as `examples/terminal/font/font.json`.
- Marker scans over `examples/` found no `TODO`/`FIXME`/`XXX`/`HACK`/`stub`/`unimplemented` hits.

**Needed before 1.0.0:**
- Add or remove `simulate` targets so every advertised simulation path has a source-backed harness, or gate those targets per example and document the limitation clearly.
- Fix the clean rules so they remove the outputs the example actually generates, especially `examples/cpu/Makefile` and `examples/soc/Makefile`.
- Separate checked-in generated artifacts from hand-written documentation, or mark them explicitly as generated snapshots and add freshness or regeneration checks.
- Rewrite the drafty notes into release-grade example documentation, especially the BIOS notes, terminal escape reference, and status snapshot.
- Add a top-level examples index that explains which trees are build-only, simulation-ready, or board-specific so users do not have to infer behavior from copied `Makefile` templates.

**Surprising findings:**
- The corpus is more representative than the directory names suggest: it spans simple counters, latches, DVI and video, UART and audio, terminal I/O, crypto, and a small SoC with software payload generation.
- The weakest part is not RTL quality but hygiene and documentation drift, and the copy-pasted `Makefile` template has replicated the same `simulate` and `clean` problems into multiple roots.

### 13. Documentation Site — Score: 7 / 10
**Paths owned:** `docs/`, `README.md`
**Criteria scored against:**
- completeness and navigability for new users
- consistency with the authoritative specifications and current tooling
- build/publish readiness and dead-link/stale-content risk
- onboarding quality for install, build, and first success
**Last reviewed:** 2026-05-11
**Rationale:**
The docs surface is already solid for a 1.0.0 release: `README.md`, `docs/getting-started/*`, `docs/examples/*`, and the reference manual form a coherent path from install to first compile, and the VitePress nav mirrors that structure cleanly. The site is not content-starved, and the onboarding flow is materially better than a typical project README plus a pile of reference pages.

The main gaps are release polish and publish reproducibility, not missing coverage. The only stale marker found in owned docs content is the `Version 0.2.0` example in CLI usage, but the published spec PDFs still inherit beta-era framing from `specification/`, which makes the site feel pre-release even though the current tooling and docs layout are otherwise release-shaped. The build path is also not yet self-contained: `scripts/build-docs-site` goes through a top-level CMake configure that fetches `sqlite3` from the network, and the docs build uses `npm install` without a `docs/package-lock.json`, so the publish path is not fully deterministic.

**Key measurements:**
- 57 owned `docs/` and `README.md` files, including 37 markdown pages under `docs/` (`4` getting-started, `22` reference-manual, `10` examples).
- `docs/.vitepress/config.mts` already sets the GitHub Pages base path, wires the main nav and sidebar, and links the PDF specifications.
- `scripts/build-docs-site` stages 5 spec PDFs into the site and verifies they exist in the final output.
- One stale version marker remains in owned docs and README: `docs/getting-started/cli-usage.md` still shows `Version 0.2.0 (abc1234)`.
- The publish build failed in this environment because the root CMake `FetchContent` step tried to download `sqlite3` from `www.sqlite.org`, and DNS resolution was unavailable.

**Needed before 1.0.0:**
- Replace the remaining `0.2.0` CLI example and add a short release-status note that tells readers what is guaranteed stable at 1.0.0.
- Make the docs publish path deterministic and offline-safe by removing the network fetch from the docs build path and switching `scripts/build-docs-site` to lockfile-driven dependency installation.
- Add a minimal site smoke or link-check step so internal nav and published PDF targets are validated before deployment.
- Regenerate and republish the spec PDFs after the release framing is updated so the site stops shipping mixed beta-era language.

**Surprising findings:**
- The onboarding path is better than the raw file count suggests: `README.md`, `docs/getting-started/*`, and `docs/examples/*` already give new users a mostly linear path to install, build, and run.
- The bigger risk is build reproducibility, not documentation breadth.

### 14. Build & CI Infrastructure — Score: 7 / 10
**Paths owned:** `CMakeLists.txt`, `compiler/CMakeLists.txt`, `specification/CMakeLists.txt`, `viewer/CMakeLists.txt`, `.github/`, `scripts/`, `VERSION`
**Criteria scored against:**
- reproducible clean builds from checkout
- CI breadth against the project’s stated release gates
- release packaging/version stamping and artifact story
- contributor ergonomics and automation gaps
**Last reviewed:** 2026-05-11
**Rationale:**
The build and CI story is solidly usable and covers most of the release path end to end: the root CMake project wires versioning, packaging, and subprojects together; the compiler CMake config generates version headers and installs the binary; the docs target builds PDFs when `pandoc` is present; and the viewer build is integrated with its third-party dependencies. The single GitHub Actions workflow also covers the stated 1.0.0 gates from `README.md`: compiler build and test and validation, docs build, VS Code extension compile and smoke, and viewer smoke, with an extra macOS viewer matrix.

It stops at 7 because reproducibility and release publication are not fully deterministic yet. The build still fetches third-party source at configure time from live URLs and tags, `scripts/build-docs-site` uses `npm install` even though the docs tree has a lockfile, and the release path is split across multiple scripts rather than one publishable, CI-verified artifact pipeline. The versioning flow exists and works, but it is still more release helper than release system.

**Key measurements:**
- 1 GitHub Actions workflow file with 4 jobs: `compiler-test`, `docs-site`, `vscode-extension`, and `viewer-smoke`.
- The viewer smoke job is the only matrixed build, and it covers 2 OSes: Ubuntu and macOS.
- `CMakeLists.txt` defines 3 CPack components (`Compiler`, `Viewer`, `Docs`) and both binary and source TGZ packaging.
- `scripts/release` already runs the full release sequence: version sync, CMake build, CTest, `compiler/tests/run_validation.sh`, viewer smoke, VS Code extension smoke, docs build, tagging, and `cpack`.
- `scripts/sync_version.py` updates 5 versioned touchpoints: `VERSION`, `compiler/include/version.h`, `docs/getting-started/cli-usage.md`, `vscode-ext/package.json`, and `vscode-ext/package-lock.json`.
- There are 3 external `FetchContent` fetches in the build graph: SQLite in the root CMake and SDL3 and ImGui in `viewer/CMakeLists.txt`.

**Needed before 1.0.0:**
- Pin or vendor the remaining network-fetched dependencies so a clean checkout builds without depending on mutable upstream availability or tag state.
- Make the docs-site build deterministic by using lockfile-driven install behavior instead of `npm install`.
- Add CI coverage for release packaging itself, not just the build-and-test steps that precede it.
- Consolidate the release publication story so docs deployment, package generation, version stamping, and tagging are exercised through one obvious path.
- Remove the remaining hardcoded version example drift in `docs/getting-started/cli-usage.md` if it is intended to be release-quality.
- Decide whether broader platform build smoke beyond the viewer’s Ubuntu and macOS matrix is part of the 1.0.0 support promise; if it is, add it now.

**Surprising findings:**
- The repository already has a more complete release helper than the CI surface suggests: `scripts/release` is effectively a manual release pipeline.
- Docs publication is integrated more tightly than it first appears, because the docs build rebuilds the PDFs and verifies they appear in the final VitePress output.
