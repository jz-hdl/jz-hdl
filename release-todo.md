# jz-hdl-dev 1.0.0 Release Readiness

**Status:** Review complete
**Date:** 2026-05-09
**Reviewer:** Codex (GPT-5)

## Exclusions
- `datasheets/`
- `.git/`
- build/output folders including `compiler/build/`, `viewer/build/`, `docs/.vitepress/dist/`, `docs/node_modules/`, `vscode-ext/node_modules/`, `vscode-ext/out/`, example `build/`, `out/`, and `reports/` directories
- generated compiler outputs inside examples including emitted `*.jzw` files and memory-init artifacts
- Python cache and transient files such as `pipeline/__pycache__/`

## Top 1.0.0 Blockers (cross-cutting)
- Restore a clean release gate: `compiler/tests/run_validation.sh` currently depends on a missing `compiler/tests/path_security_escape_target.jz`, and standalone testbench/simulation smoke tests are not part of the enforced gate.
- Fix release-critical correctness mismatches in shipped semantics: `@repeat` ignores block comments incorrectly, hierarchical `TAP` lookup is leaf-name only, and `@run_until` / `@run_while` do not short-circuit on already-satisfied entry conditions.
- Unify versioning and release stamping across the project: specs still present as beta `0.1.8`, the VS Code extension is `0.1.0`, and LSP `serverInfo.version` is hardcoded to `0.1.0`.
- Raise build and packaging infrastructure from internal-tool quality to release quality: viewer is not built in CI, dependency fetches are not checksum-locked, docs generation dirties the source tree, and there is no full install/package flow for shipped artifacts.
- Fix LSP file attribution so diagnostics are matched by canonical path/URI instead of basename-only filtering.
- Decide and document the supported chip-data surface: embed or intentionally exclude `gw5a-lv25-mg121-c1-i0.json`, align differential type vocabulary with the spec, and tighten schema validation expectations.
- Bring examples and docs up to release teaching quality: fix the README quick example, replace placeholder install instructions, and close the most visible feature/example coverage gaps.
- Decide the shipping scope for the viewer and editor tooling, then either package/persist/polish them or explicitly position them as preview-tier companion tools rather than 1.0.0-complete deliverables.

## Overall Score: 6 / 10
The weighted picture is stronger than the release story alone suggests: the core language specification, frontend, IR, backends, simulator, diagnostics, and chip modeling mostly land in the `7–8/10` range, which means the heart of the compiler is already solid and close to production quality. The overall score is pulled down by groups that matter disproportionately for a 1.0.0 release claim: the validation gate is not currently clean, build/package/CI infrastructure is only `5/10`, and several user-facing surfaces still carry inconsistent versioning, onboarding gaps, or correctness edge cases. The top three blockers are the broken release gate, the undercooked packaging/versioning story, and the remaining correctness bugs in repeat expansion, simulation TAP/timeout semantics, and LSP file attribution. Weighting favored compiler correctness, validation trustworthiness, and release reproducibility over companion tooling polish, but even with that weighting the project is not yet at “production-ready with minor rough edges.” No group scores were revised during calibration; the initial per-group spread was internally consistent.

---

## Groups

### 1. Language Specification & Rule Catalog — Score: 7 / 10
**Paths owned:** `specification/`, `pipeline/`
**Criteria scored against:**
- Completeness versus implemented language, simulation, testbench, and chip-data behavior
- Consistency across specification documents and rule-catalog material
- Clarity and auditability for a new reader tracing rule intent to tests
- Staleness, broken references, and release-readiness of process docs
**Last reviewed:** 2026-05-09
**Rationale:**
The spec corpus is strong: the five specification documents are detailed, internally consistent, and cover the major language, simulation, testbench, waveform, and chip-data behaviors that the implementation and validation corpus exercise. Cross-document references I spot-checked are live and line up with real headings, and the owned paths are free of actual unfinished-code markers; the only `TODO`/`FIXME` hits in `pipeline/` are in the review-plan prompt text itself. The main reason this does not reach 8/10 is release maturity, not coverage depth: every spec front matter still says `State: Beta — Version: 0.1.8`, and a few sections still carry explicit deferred/future-feature language (`phase offset` in simulation, reserved future `input.<NAME>.type` in chip data). The pipeline files are usable and coherent, but they read as internal operator instructions rather than polished 1.0.0 process docs.

**Key measurements:**
- 5 spec files and 17 pipeline files in scope, totaling 13,299 lines across the owned paths.
- No actual `TODO`/`FIXME`/`XXX`/`HACK`/`assert(0)`/`abort(`/`not implemented`/`stub`/`unimplemented` markers in the spec docs themselves; the only grep hits in `pipeline/` were instruction text in `pipeline/project_ready/prompts/1-review-plan.md`.
- Cross-reference scan found the expected live anchors: `1.6.3`, `1.6.4`, `1.6.6`, `1.6.7`, `5.2`, `6.2`, `7.3`, and `12.2`, and the corresponding headings exist in the source docs.
- The spec set is broadly aligned on core semantics like `@testbench` vs. `@simulation`, tri-state handling, and combinational-loop behavior.

**Needed before 1.0.0:**
- Update the spec front matter from `Beta — Version: 0.1.8` to a release-stamped 1.0.0 pass after one final consistency review.
- Decide whether the remaining deferred items are intentionally post-1.0.0 (`phase offset` in simulation, reserved future `input.<NAME>.type` in chip data) or must be implemented and documented before release.
- Add a concise release-oriented traceability pass for the spec/rule pipeline so a new reader can map rule intent to tests without reading the internal review prompt first.

**Surprising findings:**
- The core spec corpus is much closer to release-ready than the version banners suggest; the biggest gap is maturity labeling and release packaging, not missing language coverage.

### 2. Compiler Frontend — Score: 8 / 10
**Paths owned:** `compiler/include/{ast.h,ast_json.h,lexer.h,parser.h,sem.h,sem_driver.h,repeat_expand.h,template_expand.h,expansion_limits.h}`, `compiler/src/ast/`, `compiler/src/lexer.c`, `compiler/src/parser/`, `compiler/src/sem/`, `compiler/src/repeat_expand.c`
**Criteria scored against:**
- Feature completeness versus the language and testbench specifications
- Diagnostic quality, source locations, and parse/semantic recovery behavior
- Test coverage depth for parsing, typing, constant evaluation, templates, CDC, and memory semantics
- Memory safety and absence of obvious undefined behavior in C implementation
**Last reviewed:** 2026-05-09
**Rationale:**
This frontend is broadly feature-complete for the shipped language surface. The lexer, recursive-descent parser, semantic drivers, constant-eval helpers, and expansion passes collectively cover modules, projects, blackboxes, imports, testbenches, simulations, templates, CDC, memory semantics, `widthof()`, `clog2()`, tristate behavior, and the main recovery paths the specs describe. Diagnostics are generally strong: most errors are rule-linked, source-located, and designed to survive recovery instead of collapsing into generic parse failures.

What keeps this at 8 instead of 9 is polish, not basic capability. The clearest concrete defect is in `compiler/src/repeat_expand.c`: the raw `@repeat` prepass ignores line comments and strings, but not block comments, even though the spec says `@repeat` inside comments is ignored. That is a real language mismatch and a missing regression. I also see a few release-readiness rough edges: template scratch naming is time-seeded and therefore nondeterministic, and some pre-lexing diagnostics are coarse compared with the rest of the frontend. None of that looks like a structural blocker, but it is enough to keep this out of “spec-complete and polished.”

**Key measurements:**
- 48,318 lines across the owned frontend headers and sources I inspected.
- The validation corpus in scope is broad: 260 frontend-related validation artifacts across HDL/TB/SIM, with dense coverage for templates, `@repeat`, `widthof()`, `clog2()`, CDC, memory, tristate, and simulation/testbench directives.
- No actual `TODO`/`FIXME`/`XXX`/`HACK`/`assert(0)`/`abort(`/`not implemented`/`stub`/`unimplemented` markers showed up in the owned frontend paths.
- The codebase already has focused validation for the critical semantic buckets this group owns, including template expansion, constant evaluation, identifier rules, memory semantics, and tri-state analysis.

**Needed before 1.0.0:**
- Fix `@repeat` expansion so block comments are treated the same as line comments and strings, and add a regression fixture for `/* @repeat ... */`.
- Decide whether the time-seeded scratch suffix generation in `template_expand.c` is acceptable for a 1.0.0 release; if reproducible output matters, make it deterministic.
- Tighten any remaining pre-lexing diagnostic locality if you want repeat-expansion failures to match the rest of the frontend’s source-location quality.

**Surprising findings:**
- The parser/semantics layer is much more mature than the raw `@repeat` prepass suggests; the main gap I found was a pre-lexing corner case, not a missing core language feature.

### 3. Compiler IR & Middle-end — Score: 8 / 10
**Paths owned:** `compiler/include/{ir.h,ir_builder.h,ir_mem_bind.h,ir_serialize.h}`, `compiler/src/ir/`
**Criteria scored against:**
- Correctness and completeness of lowering from semantic model to IR
- Soundness of transformation passes and invariants between passes
- Serialization/debuggability for users and maintainers
- Testability and evidence that IR features are exercised by validation or example flows
**Last reviewed:** 2026-05-09
**Rationale:**
This middle-end is broadly production-ready. The core build pipeline is coherent and ordered correctly: `jz_ir_build_design()` constructs the design, binds memory ports while semantic scope is still available, lowers memory writes, materializes CDC library modules, and then eliminates dead modules. The tri-state pass is also disciplined: it clones the design, transforms in place, and rolls back on any failure instead of mutating the caller’s IR into a half-lowered state.

The implementation coverage is strong across the supported IR surface. I found real lowering for signals, expressions, assignments, control-flow, memories, module instances, CDC crossings, differential pin metadata, dead-module elimination, and division-guard analysis. The owned paths have no actual `TODO`/`FIXME`/`XXX`/`HACK`/`assert(0)`/`abort(`/`not implemented`/`stub`/`unimplemented` markers. What keeps this from 9 is release polish and a few fidelity gaps: the JSON serializer still stamps `ir_version` as `0.1.0`, does not surface some internal middle-end metadata like `eliminated`, `is_blackbox`, or `port_alias_groups`, and a few lowering paths still rely on explicit fallback behavior for unsupported forms rather than fully normalized IR.

**Key measurements:**
- 20 owned files in scope, totaling 24,562 lines.
- 57 IR golden outputs are present, and the validation tree contains 2,936 files overall.
- The IR-focused coverage is broad: `cdc_*`, `tristate_*`, `memory_*`, `mem_file_init_*`, `intrinsic_*`, `top_concatenation_binding`, `override_module_specialization`, `serializer_reset`, `clock_domain_isolation_and_cdc_crossing`, and `path-exclusive_determinism` all exercise the serializer or the lowered IR.
- Spot-checked goldens show the serializer captures the major structures users need for debugging: modules, signals, clock domains, memories, instances, CDC crossings, project pins/mappings, and top bindings.

**Needed before 1.0.0:**
- Re-stamp the IR JSON format from `0.1.0` to a release version and decide whether the serializer should expose `eliminated`, `is_blackbox`, and `port_alias_groups` for debugging.
- Add or tighten regressions around the remaining fallback lowering paths so unsupported selector/LHS forms fail explicitly and consistently.
- Keep the existing IR validation coverage as a release gate, especially for memory lowering, CDC lowering, tri-state transforms, and the `--ir` output path.

**Surprising findings:**
- The lowering pipeline is much closer to shipped quality than the serializer versioning suggests; the weakest part is release-facing metadata, not the core IR transformations.

### 4. Compiler Backends — Score: 8 / 10
**Paths owned:** `compiler/include/{verilog_backend.h,rtlil_backend.h}`, `compiler/src/backend/`
**Criteria scored against:**
- Correctness and completeness of Verilog and RTLIL generation
- Constraint/output support for supported FPGA flows
- Stability of emitted code for real examples and downstream toolchains
- Coverage of backend-specific edge cases, diagnostics, and unsupported constructs
**Last reviewed:** 2026-05-09
**Rationale:**
This backend layer is broadly production-ready. The Verilog emitter and RTLIL emitter both cover the expected release surface: module emission, instances, memory lowering, aliasing, clock-gen wrappers, differential I/O, and project-level wrappers, plus SDC/XDC/PCF/CST sidecar generation for the supported FPGA flows. The output paths are exercised by a substantial golden corpus and by `run_validation.sh`, which checks both backend diffs and downstream Yosys parsing, and for fixtures that have both formats it also checks Verilog vs. RTLIL equivalence.

What keeps this at 8 instead of 9 is polish and a small number of explicit fallback paths. I found one concrete backend `TODO` in RTLIL wrapper emission for multi-bit inverted ports, and both backends still contain “unsupported stmt kind” fallback comments rather than fully surfaced diagnostics for unreachable shapes. Those are real rough edges, but they are isolated and do not look like release blockers given the current coverage and the fact that the main emitted flows are already stable against Yosys and the golden fixtures.

**Key measurements:**
- 60 Verilog goldens, 57 RTLIL goldens, and 21 backend-sidecar golden scripts are present in scope.
- `run_validation.sh` verifies generated `.v` and `.il` files with Yosys parse checks, and runs Verilog/RTLIL equivalence checks for fixtures that carry both outputs.
- Constraint coverage is real, not incidental: the corpus includes SDC/XDC/PCF/CST generation and escaping-focused fixtures.
- The owned paths have no broad forest of unfinished-code markers; the only explicit backend `TODO` I found was the RTLIL multi-bit inverted-port note.

**Needed before 1.0.0:**
- Decide whether RTLIL multi-bit inverted top bindings should be implemented before release, or explicitly documented as unsupported if they are truly out of scope.
- Replace the remaining generic “unsupported stmt kind” fallback comments with explicit diagnostics if those code paths can be reached by valid inputs.
- Keep the existing Verilog/RTLIL/Yosys validation as a release gate, especially for memory emission, differential I/O, clock-gen wrappers, and constraint sidecars.

**Surprising findings:**
- The strongest signal here is not code volume but verification depth: the backend outputs are already cross-checked by golden diffs, parser validation, and equivalence checks, which is a good sign for downstream toolchain stability.

### 5. Simulator & Waveforms — Score: 8 / 10
**Paths owned:** `compiler/src/sim/`
**Criteria scored against:**
- Conformance to simulation and waveform specifications
- Correctness of event execution, value semantics, clocks, and waveform emission
- Performance sanity and absence of obvious pathological behavior
- Coverage of simulation-specific error handling and format interoperability
**Last reviewed:** 2026-05-09
**Rationale:**
This is broadly production-ready. The simulator has a coherent event-driven core with deterministic clock ordering, 1ps internal time, exact ps conversion checks, reset/setup sequencing, combinational settling, NBA updates, `@run`/`@run_until`/`@run_while`, `@trace`, `@mark`, `@alert`, `@monitor`, TAP dumping, and three waveform backends. The waveform layer is also solid: VCD, FST, and JZW are all wired through a shared interface, and JZW carries the expected SQLite schema, metadata, clock records, change-only storage, and annotations.

What keeps this at 8 instead of 9 is a small set of real fidelity gaps. The most concrete one is TAP resolution: `compiler/src/sim/sim_engine.c` resolves `TAP dut.child.signal` by leaf signal name only, so hierarchical taps can misbind if two signals share a leaf name. I also found a semantic gap in `@run_until`/`@run_while`: the loop does not check whether the condition is already satisfied at entry, so those directives do not short-circuit immediately on a pre-existing true/false condition. Those are not release blockers, but they are correctness issues that matter for a 1.0.0 claim.

**Key measurements:**
- The owned simulator tree is compact but complete: 18 source/header files in `compiler/src/sim/`.
- Validation and examples are substantial: 54 simulation validation artifacts, 5 standalone simulation tests, plus golden coverage for waveform formats and `@run_until`/`@run_while`.
- I found no actual unfinished-code markers in the owned sim sources; the only hits were benign no-op perf stubs in `sim_perf.h`.
- The exercised surface includes wide-value emission, trace toggling, JZW metadata, TAPs, multi-clock scheduling, and the main simulation directives.

**Needed before 1.0.0:**
- Fix TAP lookup to resolve the full hierarchical path, not just the leaf signal name, so nested `TAP` entries cannot misbind.
- Make `@run_until` and `@run_while` short-circuit when the condition is already satisfied or violated at directive entry.
- Add regressions for hierarchical TAP collisions and zero-wait `@run_until`/`@run_while` cases, and keep the existing simulation/golden corpus as the release gate.

**Surprising findings:**
- The simulator is farther along than the directory size suggests: the hard parts are already in place, and the remaining work is mostly precision and edge-case correctness rather than missing subsystems.

### 6. Testbench & Validation Suite — Score: 7 / 10
**Paths owned:** `compiler/tests/`, `compiler/src/parser/parser_testbench.c`, `compiler/src/parser/parser_simulation.c`, `compiler/src/sem/driver_testbench.c`
**Criteria scored against:**
- Isolation and correctness of validation fixtures against intended rule coverage
- Breadth and maintainability of simulation, testbench, and golden-output coverage
- Drift risk between specs, compiler diagnostics, and golden files
- Ability of the suite to catch release-blocking regressions before 1.0.0
**Last reviewed:** 2026-05-09
**Rationale:**
The suite is broad and mostly well structured. The parser/semantic split for `@testbench` and `@simulation` is explicit, the validation corpus covers the main rule families for TB/SIM plus repeat expansion and path-security, and the golden tests exercise runtime behaviors like waveform output, `@run_until`/`@run_while`, `@expect_tristate`, and hierarchical `@expect_equal`. The main reason this does not reach 8 is that the release gate is not clean: `compiler/tests/run_validation.sh` currently fails on `HDL_12_4_ADDITIONAL_SANDBOX_ROOT-happy_path.jz` because it imports a missing `compiler/tests/path_security_escape_target.jz`, and the auxiliary `compiler/tests/testbenches/` and `compiler/tests/simulation/` fixtures are not wired into CI.

**Key measurements:**
- 47 `TB_*.jz` validation fixtures and 27 `SIM_*.jz` validation fixtures are present, plus 5 standalone testbench smoke tests, 5 standalone simulation smoke tests, and 91 golden assets.
- `run_validation.sh` covers `tests/validation` and `tests/golden`; it skips any validation `.jz` without a sibling `.out`, which is fine for helper libraries but creates drift risk if a real happy-path fixture is added without an expected output.
- Coverage is strong for declaration and diagnostic rules, but there are still gaps in release-blocking runtime regressions, especially the zero-wait `@run_until`/`@run_while` cases and a TAP collision case for same-leaf hierarchical names.
- The tree already has good positive coverage for hierarchical `@expect_equal`, `@expect_tristate`, `MONITOR`, `TAP`, `@print_if`, and repeat expansion, so the core semantics are exercised rather than only syntax.

**Needed before 1.0.0:**
- Fix or restore the missing path-security import target so `run_validation.sh` passes end to end again.
- Add a regression for `@run_until`/`@run_while` when the condition is already satisfied or violated at directive entry.
- Add a regression for hierarchical `TAP` name collisions, and wire the standalone `compiler/tests/testbenches/` and `compiler/tests/simulation/` smoke tests into the release gate or a documented test job.
- Consider making the validation harness fail on unexpected skipped happy-path fixtures rather than silently ignoring `.jz` files with no `.out`, to reduce fixture drift.

**Surprising findings:**
- The suite is healthier than the failed gate suggests: most of the coverage is disciplined and targeted, but a single missing imported fixture is enough to make the current release gate unusable.

### 7. Diagnostics & Reports — Score: 8 / 10
**Paths owned:** `compiler/include/{diagnostic.h,rules.h}`, `compiler/src/diagnostic.c`, `compiler/src/rules.c`, `compiler/src/report/`
**Criteria scored against:**
- Quality, precision, and consistency of diagnostics and rule metadata
- Usefulness and maturity of alias, memory, chip, and tristate reports
- Consistency between documented rules, emitted diagnostics, and tests
- Stability of outputs users would rely on in a 1.0.0 release
**Last reviewed:** 2026-05-09
**Rationale:**
This group is close to release-ready. The diagnostic core is disciplined: buffered storage, rule-linked severities, warning policy handling, deterministic sorting/deduping, and a renderer that keeps source location, rule code, and message text aligned. The rule catalog is large and coherent at 446 entries, and the public report surfaces are real products, not placeholders: alias, memory, tristate, and chip-info all have dedicated golden suites with exact output diffs.

I kept this at 8 instead of 9 because there are still a few release-polish gaps. The public `--lint-rules` listing is part of the contract, but I did not find a dedicated snapshot test for it. The report emitters in `compiler/src/report/` are useful, but they still rely on fixed-width stack buffers and hand-formatted tables, which is fine today but a little brittle if chip databases or descriptions grow. The generic diagnostic printer also has one small one-off branch for `RPT_COUNT_INVALID` rather than a fully uniform metadata path.

**Key measurements:**
- 446 rules are defined in `compiler/src/rules.c`.
- The owned files total 1,534 lines across `diagnostic.h`, `rules.h`, `diagnostic.c`, `rules.c`, and `compiler/src/report/`.
- There are 4 dedicated report golden suites: `alias_report`, `memory_report`, `tristate_report`, and `chip_info`, and 2,937 validation artifacts overall.
- I did not find unfinished-code markers in the owned paths.

**Needed before 1.0.0:**
- Add a dedicated snapshot for `--lint-rules` so the public rule catalog is pinned like the other report commands.
- Add at least one stress case for long chip/memory labels and descriptions, or switch the report tables to safer bounded formatting if larger chip data is expected.
- Decide whether `REPORT_DEPTH_LIMIT_EXCEEDED` should get an explicit regression, since the rule exists and the code path is present but is not visibly pinned by a golden case.

**Surprising findings:**
- The report layer is much more mature than the directory layout suggests: the four user-facing report commands are already backed by exact golden outputs, and the chip-info command is detailed enough to be a real release feature rather than a debugging dump.

### 8. Chip Data & Vendor Support — Score: 7 / 10
**Paths owned:** `compiler/data/`, `compiler/include/chip_data.h`, `compiler/src/chip_data.c`, `compiler/src/chip_data_internal.h`
**Criteria scored against:**
- Correctness versus the chip-info specification and vendor modeling needs
- Coverage and completeness of supported FPGA parts and resource data
- Validation, error handling, and resistance to malformed or inconsistent chip definitions
- End-to-end readiness of chip data for backend and reporting workflows
**Last reviewed:** 2026-05-09
**Rationale:**
This is solid and close to release-ready for the parts it actually models. The database spans Gowin, Lattice iCE40, Lattice ECP5, and AMD/Xilinx Artix-7; the chip records carry real resources, memory tables, clock generators, differential I/O, latch/DSP metadata, and fixed pins; and the loader is not a dumb parser. It enforces built-in/local path safety, token/nesting limits, variant exhaustiveness/disjointness, and basic shape checks for memory and clock-gen sections, which keeps malformed vendor data from turning into silent bad output.

It still lands below an 8 because the shipping surface is narrower and less schema-strict than the spec implies. There are 9 JSON definitions in `compiler/data/`, but only 8 are embedded in `k_builtin_chips`; `gw5a-lv25-mg121-c1-i0.json` exists as source data without being part of the built-in database. The chip-info spec is also not fully aligned with the data vocabulary, because the iCE40 differential entries use `type: "pseudo"` while the spec documents only `true` or `emulated`. On the validation side, the loader accepts a lot of partially malformed structure by skipping unknown or missing fields instead of rejecting them, so custom chip definitions can fail open unless they hit the narrow set of explicit checks.

**Key measurements:**
- 9 chip JSON definitions exist under `compiler/data/`.
- 8 of those are embedded as built-ins in `compiler/src/chip_data.c`.
- The built-in set covers 4 vendor families: Gowin, Lattice iCE40, Lattice ECP5, and AMD/Xilinx Artix-7.
- Across the data set, I saw 5 memory types (`SDRAM`, `DISTRIBUTED`, `BLOCK`, `SPRAM`, `FLASH`) and 7 distinct clock-gen types (`osc`, `pll`, `pll2`, `clkdiv`, `clkdiv2`, `buf`, `buf2`).
- The `chip_info` golden coverage is narrow: one list snapshot and one detailed chip snapshot are pinned.
- The loader does validate higher-risk cases like variant coverage, path safety, and token/nesting limits, so the failure mode is controlled rather than arbitrary.

**Needed before 1.0.0:**
- Decide whether `gw5a-lv25-mg121-c1-i0.json` is intentionally external-only or needs to be embedded and listed as a supported built-in chip.
- Normalize the differential type vocabulary in the spec and data (`pseudo` vs. `true`/`emulated`) so the contract is explicit.
- Add stricter schema validation for required top-level sections and unknown or malformed nested objects if custom chip JSON is expected to be supported.
- Expand report and test coverage beyond one detailed `--chip-info` snapshot so changes to non-Gowin vendor data are pinned before release.

**Surprising findings:**
- The modeled hardware surface is broader than the built-in list suggests, but the main risk is release packaging and schema discipline, not missing primitive categories.

### 9. CLI & LSP — Score: 7 / 10
**Paths owned:** `compiler/include/{compiler.h,lsp.h,path_security.h,util.h,version.h}`, `compiler/src/{main.c,compiler.c,util.c,arena.c,path_security.c,cli_frontend.c,cli_frontend.h,cli_modes.c,cli_modes.h,cli_options.c,cli_options.h}`, `compiler/src/lsp/`
**Criteria scored against:**
- CLI surface completeness, coherence, and user-facing stability
- LSP capability depth and robustness for real editor use
- Path security, file handling, and failure-mode quality
- Packaging/versioning maturity for a 1.0.0 command-line tool
**Last reviewed:** 2026-05-09
**Rationale:**
The command-line surface is broad and coherent: lint, AST, IR, Verilog, RTLIL, test, simulate, report commands, chip-info, lint-rules, and the stdio LSP entrypoint are all wired through a single parser and mode dispatcher. Path security is also materially strong for a 1.0.0 tool: it canonicalizes paths, rejects absolute/traversal by default, supports explicit sandbox roots, checks symlink escapes, and revalidates open-after-validate on POSIX.

What keeps this at 7 is release-facing polish and one editor-grade correctness bug. The version banner is still stamped as `0.1.8`, the LSP initialize response hardcodes `serverInfo.version` to `0.1.0`, and the only dedicated LSP regression coverage I found is two golden suites. More importantly, both diagnostic publish paths filter by basename only, so same-named files in different directories can be misattributed in real workspaces. That is the kind of failure mode that will show up quickly in a large editor project.

**Key measurements:**
- 21 owned CLI/LSP source/header files in scope.
- 2 dedicated LSP golden suites.
- 28 path-security validation fixtures in `compiler/tests/validation/`.
- The CLI usage surface already advertises the release-oriented modes and commands, including `--chip-info`, `--lint-rules`, and `--lsp`.

**Needed before 1.0.0:**
- Replace basename-only LSP diagnostic filtering with canonical path or URI matching so same-leaf files cannot collide.
- Re-stamp the release-facing version surfaces (`version.h`, LSP `serverInfo.version`, and any generated version metadata) to 1.0.0.
- Expand dedicated LSP regression coverage beyond the current sandbox and resource tests, especially for hover, definition, and completion on real multi-file projects.

**Surprising findings:**
- The sandbox layer is stronger than the directory name suggests; the bigger gap is not path validation but release stamping and editor-facing correctness.

### 10. Build, Packaging & CI Infrastructure — Score: 5 / 10
**Paths owned:** `compiler/CMakeLists.txt`, `compiler/cmake/`, `viewer/CMakeLists.txt`, `.github/`, `scripts/`
**Criteria scored against:**
- Reproducible clean builds for compiler, viewer, and supporting tools
- CI coverage breadth and likelihood of catching release regressions
- Version stamping, release automation, and packaging hygiene
- Contributor and release-engineering ergonomics for 1.0.0
**Last reviewed:** 2026-05-09
**Rationale:**
The build system is functional, but it is not yet release-grade. The compiler has a conventional CMake build, generated version stamping, and CI does run build, CTest, and validation with warnings-as-errors. But the release surface is still narrow: CI only exercises the compiler on one Ubuntu job, the viewer is not built or tested in CI, and there is no real install/export/package layer beyond a single `install(TARGETS jz-hdl ...)` rule.

Reproducibility is also incomplete. Both compiler and viewer pull third-party dependencies through `FetchContent` with pinned tags or versioned URLs, but not with checksum-locked artifacts, so a clean build still depends on upstream availability. In `compiler/CMakeLists.txt`, the `docs` target is `ALL` and copies generated PDFs back into `docs/public/pdf/`, which dirties the source tree during a normal build. That is poor packaging hygiene for 1.0.0 and makes “clean build” less clean than it should be.

`scripts/release` is useful, but it behaves more like an internal release helper than a shippable packaging workflow. It is macOS-specific (`sed -i ''`), edits multiple source files directly, validates only the compiler path, and archives source trees rather than producing a reproducible compiled viewer artifact. `scripts/gitpages-update` is similarly serviceable, but `npm install` makes the docs deployment less deterministic than a locked install flow.

**Key measurements:**
- 1 CI workflow in `.github/workflows/ci.yml`, with a single Ubuntu job.
- 5 owned support/build scripts or CMake helpers outside the two main CMakeLists.
- Only the compiler install target is defined in the reviewed paths.
- Both compiler and viewer use network-fetched third-party dependencies during clean builds.

**Needed before 1.0.0:**
- Add viewer build/test coverage in CI, plus at least one matrix dimension that catches release-relevant variation beyond a single Ubuntu compiler job.
- Stop writing generated PDFs back into the source tree during ordinary builds; stage them in the build tree or a dedicated packaging output path instead.
- Add a real install/package flow for shipped artifacts, including the viewer, and centralize version stamping so the release script does not have to patch multiple files.
- Replace the ad hoc release helper with cross-platform, deterministic packaging steps and pinned dependency installs.

**Surprising findings:**
- The strongest part of the current release story is compiler validation coverage, not packaging; the automation looks polished on the surface, but it still behaves like internal tooling rather than 1.0.0 distribution infrastructure.

### 11. Waveform Viewer — Score: 6 / 10
**Paths owned:** `viewer/` excluding `viewer/build/`
**Criteria scored against:**
- Buildability and runtime completeness on a clean machine
- Fidelity and usability for inspecting JZW waveforms
- Error handling, persistence, and scalability expectations for practical use
- Packaging/distribution readiness as a shipped companion tool
**Last reviewed:** 2026-05-09
**Rationale:**
The viewer is functionally solid for its core job. `src/main.cpp` already loads JZW traces directly from SQLite, supports live reload against WAL, renders scalar and bus waveforms, shows annotations and clock metadata, and provides the expected interaction set for a waveform browser: zoom, pan, cursors, signal visibility, drag reorder, and per-bit expansion. Error handling is also materially better than a prototype; the loader enforces explicit caps on signals, changes, annotations, clocks, and resident text, and it reports malformed trace data with readable failures instead of undefined behavior.

What keeps this out of 7/10 is release maturity. The clean-machine build story depends on network fetches in `CMakeLists.txt` for SDL3, SQLite, and ImGui, and there is no install or packaging target to turn the tool into a distributable companion app. The usability surface is still thin for a shipped viewer: there is no file-open dialog, no search/filter, and no persistence for layout, zoom, or cursor state. The live-reload path is also not fully robust, because polling advances by `time > max_loaded_time`, which can miss late-arriving rows at an already-seen timestamp.

**Key measurements:**
- 3 owned files in scope, totaling 3,296 lines; `src/main.cpp` alone is 3,050 lines.
- The build pulls 3 upstream dependencies with `FetchContent`: SDL3 `release-3.2.14`, SQLite amalgamation `3.49.0100`, and Dear ImGui `v1.91.8-docking`.
- The implementation has explicit in-memory caps for 200,000 signals, 4,000,000 changes, 200,000 annotations, 4,096 clocks, and 256 MiB of resident text.
- The runtime surface already covers the major JZW viewer features: signal tree browsing, waveform drawing, four cursors, annotations, clock metadata, and live-follow.

**Needed before 1.0.0:**
- Add an offline-capable build path and a real packaging/install target so the viewer can be shipped without relying on live source fetches.
- Fix live-reload change ingestion so same-timestamp rows cannot be skipped.
- Add file-open and search/filter support, or explicitly defer them if the intended release scope is a minimal companion viewer.
- Add persistence for viewport, signal ordering, visibility, and cursors if the tool is meant to feel like a finished desktop app rather than a transient trace inspector.

**Surprising findings:**
- The rendering and trace-loading core are farther along than the project layout suggests; the main gap is distribution and polish, not the waveform engine itself.

### 12. VS Code Extension — Score: 6 / 10
**Paths owned:** `vscode-ext/` excluding `vscode-ext/node_modules/` and `vscode-ext/out/`
**Criteria scored against:**
- Install/build/run readiness and extension packaging quality
- Quality of editor integration with the CLI/LSP
- Robustness on missing binaries, bad paths, and failure states
- Feature completeness relative to what a 1.0.0 editor integration should expose
**Last reviewed:** 2026-05-09
**Rationale:**
This is a usable extension, but not yet a finished 1.0.0 editor integration. The manifest, language configuration, grammar, and compiled entrypoint are all present, and `npm run compile` succeeds cleanly. The client can launch `jz-hdl --lsp` or a configured binary, expose hover toggles, surface a project picker, and show a user-facing error if startup fails.

What keeps it at 6 instead of 7 is editor-fidelity and release polish. The extension version is still `0.1.0`, activation is only `onLanguage:jz-hdl`, and the LSP client only targets `file` documents, so unsaved or non-file buffers do not get the integration path. The project-info cache is also global rather than per active document or URI, which means switching between multiple open `.jz` files can leave the status bar and picker state stale or misattributed. Error handling for missing or bad binaries is present, but it is generic and stops at a single message instead of offering a clearer degraded mode or binary validation flow. The published-package story is also thin: `.vscodeignore` keeps the payload lean, but there is no in-repo VSIX packaging or smoke-test flow beyond TypeScript compilation.

**Key measurements:**
- 5 owned files in scope, totaling 500 lines.
- The extension contributes 1 language, 1 grammar, 1 command, and 4 user-facing settings.
- `npm run compile` passes in the extension directory.
- Packaging is controlled by `.vscodeignore`, which excludes `src/**`, `node_modules/**`, `.gitignore`, and `tsconfig.json` from the shipped extension payload.

**Needed before 1.0.0:**
- Stamp the extension-facing version surfaces to `1.0.0` instead of `0.1.0`.
- Track project info per active editor or URI so status-bar state and project selection do not bleed across multiple open files.
- Broaden activation and document targeting if unsaved or remote `.jz` buffers are expected to work in real editor use.
- Add explicit packaging and smoke-test coverage for install, startup, missing-binary, and bad-path cases, not just a TypeScript compile check.

**Surprising findings:**
- The code is smaller and cleaner than the feature surface suggests; the main gap is not complexity, it is state handling and editor reach.

### 13. Examples — Score: 6 / 10
**Paths owned:** `examples/` excluding generated outputs
**Criteria scored against:**
- Buildability and simulation sanity from clean source state
- Representativeness of language, chip, and tooling features
- Code quality and whether examples teach good patterns
- Documentation/comments sufficient for users to learn from them
**Last reviewed:** 2026-05-09
**Rationale:**
The examples corpus is useful and broadly healthy, but it is not yet a polished 1.0.0 showcase. Most example roots have a conventional `build`/`synthesis`/`simulate` flow that shells out to `../../compiler/build/jz-hdl`, and the better-written samples such as `terminal` and `soc` are heavily commented and do teach real patterns: module composition, buses, clocks, memories, reports, simulation, and board-specific configuration. The two top-level docs also help: `examples/status.md` gives a current synthesis matrix and `examples/coverage.md` is a useful feature map for the whole corpus.

The gap is completeness and consistency. The status matrix still shows one explicit board failure (`latch` on `pa35t-edu`) and several blank board/example combinations, so the set is not uniformly build-verified across its advertised targets. More importantly, the coverage matrix itself documents missing or underrepresented language features that matter for a release claim, including `=z`/`<=z`, `=s`/`<=s`, hardware `*` and `/`, alternate reset/clock modes, write-mode variants, and several intrinsic functions. Documentation quality also varies: `examples/soc/bios.md` reads like working notes rather than release-facing guidance, and some Makefiles are copy-paste heavy or board-specific in ways that make the corpus less teachable than it could be.

**Key measurements:**
- 13 example roots are present: `ascon`, `counter`, `cpu`, `domains`, `dvi`, `dvi_audio`, `latch`, `lcd`, `pll`, `soc`, `terminal`, `uart_audio`, and `uart_echo`.
- I counted 180 tracked non-generated files in the owned example tree, including source, docs, and helper scripts.
- `examples/status.md` tracks 13 examples across 4 board targets and shows one explicit `FAIL` plus several unfilled cells.
- `examples/coverage.md` confirms broad coverage for modules, memories, CDC, clocks, simulation, and configuration, but also lists several intentionally uncovered core features.
- The strongest teaching examples are the large, commented designs: `terminal`, `soc`, `dvi`, and `uart_audio`.

**Needed before 1.0.0:**
- Add at least one small, focused tri-state example and one width-mismatch/sign-extension example so the coverage gaps in `examples/coverage.md` stop being the default story for the corpus.
- Normalize the build/sim story across the example roots so the simplest demos are as reproducible and discoverable as the larger board-oriented projects.
- Turn the roughest prose docs, especially `examples/soc/bios.md`, into release-quality guidance or reclassify them as internal notes.

**Surprising findings:**
- The corpus is better documented than the directory name suggests: `coverage.md` is a genuinely useful feature map, and the best examples already show real design patterns instead of toy snippets.

### 14. Documentation Site & Project Docs — Score: 6 / 10
**Paths owned:** `docs/` excluding generated and dependency folders, `README.md`
**Criteria scored against:**
- Accuracy and completeness of user-facing documentation versus implementation
- Information architecture, onboarding quality, and release usability
- Staleness, generated-asset drift, and broken-reference risk
- Adequacy of installation/build/run guidance for external users
**Last reviewed:** 2026-05-09
**Rationale:**
The docs surface is structurally solid. The VitePress site already has a sensible Quick Start / Reference Manual / Verification / Examples / Specifications split, and the README covers build, test, CLI usage, examples, and editor support in a way an external user can mostly follow.

What keeps this from 7/10 is concrete release-facing polish. The README quick example references `reset` without declaring it, so copy-paste users will hit a compile error. `docs/getting-started/installation.md` still uses `git clone <repository-url>`, which is not actionable for a release user. The CLI docs also omit `--Wgroup=NAME`, even though the compiler accepts it. The shipped PDFs under `docs/public/pdf/` are useful, but they are checked-in generated artifacts and there is no documented regeneration path in the docs package, so drift risk remains.

**Key measurements:**
- 37 Markdown source pages in `docs/`, plus `README.md`.
- 5 shipped PDF specification artifacts under `docs/public/pdf/`.
- 38 HTML pages in `docs/.vitepress/dist`, so the site is already broad enough for a 1.0.0 docs set.
- The site config already separates Quick Start, Reference Manual, Verification, Specifications, and Examples.

**Needed before 1.0.0:**
- Replace the placeholder clone instruction in `docs/getting-started/installation.md` with the actual repository URL and a platform-accurate build note.
- Fix the invalid README quick example so every referenced signal is declared, or remove the undeclared reset reference.
- Document all accepted diagnostic-group flags, including `--Wgroup=NAME`, in the README and CLI docs.
- Add a documented regeneration path for the shipped PDF specs, or move them out of the hand-edited docs tree to reduce drift risk.
- Tighten cross-platform installation guidance if you want the build output path and toolchain notes to be truly external-user friendly.

**Surprising findings:**
- The information architecture is already release-shaped; the main gap is trustworthiness of specific examples and onboarding instructions, not missing topical coverage.
