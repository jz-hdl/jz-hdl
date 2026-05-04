# jz-hdl-dev 1.0.0 Release Readiness

**Status:** Review complete
**Date:** 2026-04-29
**Reviewer:** Codex (GPT-5)

## Exclusions
- `datasheets/`
- `.git/`
- `compiler/build/`
- `viewer/build/`
- `docs/.vitepress/dist/`
- `docs/node_modules/`
- `vscode-ext/node_modules/`
- `vscode-ext/out/`
- `build/`, `target/`, `dist/`, `out/` directories anywhere else in the tree
- Generated compiler outputs inside examples, including emitted `*.jzw` files and reports

## Top 1.0.0 Blockers (cross-cutting)
- Fix simulator correctness before release: runtime failures in `--simulate` must return nonzero, and the simulator/waveform path must stop truncating multi-bit values to 64-bit or `val[0]`.
- Close the testbench/runtime contract gap: either implement the broader testbench semantics implied by the public docs or narrow the documented surface to the current supported subset.
- Reconcile public versioning and docs with reality: move specs/docs/tooling off beta-era `0.1.x` markers, fix stale onboarding commands, and remove the obsolete "`--fst` not supported" claim.
- Make diagnostics part of a stable public contract by eliminating or formally registering fallback surfaces like `PARSE000` and adding direct golden coverage for report-mode outputs.
- Fix chip database consistency so the built-in chip inventory matches the checked-in JSON set, especially the missing `GW5A-LV25-MG121-C1-I0` entry.
- Deepen automated runtime coverage for simulation, testbench, FST/JZW, report modes, and LSP behavior instead of relying mainly on compile-and-smoke success.
- Harden the LSP/editor path: remove fragile fixed-size assumptions, decide whether `.jzhdl-lsp.rc` workspace mutation is part of the product contract, and add direct tests.
- Replace the maintainer-only release flow with a repeatable release pipeline and broaden CI beyond `compiler/` so shipped tooling is actually built in automation.

## Overall Score: 6 / 10
The project is well past "prototype" status: the language/spec surface is broad, the core compiler pipeline is real, and most of the compiler-facing groups landed in the 7-8 range rather than the 5-and-below range. Weighting those core groups more heavily than the viewer and VS Code extension keeps the overall score above the midpoint, because the main compiler, semantic, IR, and backend surfaces already look usable for serious work. The score stays at 6 instead of 7 because the remaining blockers sit on critical release paths rather than optional edges: simulator correctness and width handling are not 1.0-safe, runtime and testbench coverage are materially weaker than compile-time coverage, the public docs/spec/versioning story is still inconsistent, and release engineering is still maintainer-oriented instead of productized.

The three biggest blockers are the simulator/runtime correctness issues, the mismatch between the documented/public contract and the current shipped surfaces, and the weak release/CI pipeline around non-compiler artifacts. Those are weighted more heavily than the viewer and editor-tooling gaps because they directly affect whether users can trust outputs and whether maintainers can ship repeatable releases. Calibration note: the final pass did not require per-group score revisions; the existing spread of `8` for strong reference surfaces, `7` for solid compiler subsystems, `6` for usable but under-hardened tooling/runtime areas, and `5` for release engineering remained consistent.

---

## Groups

### 1. Language & Formal Specifications — Score: 8 / 10
**Paths owned:** `specification/`
**Criteria scored against:**
- Completeness against the implemented language, simulation, testbench, chip-info, and waveform surfaces
- Internal consistency across specification documents
- Clarity and precision for a new reader implementing or using the language
- Versioning and release-readiness of the documents as 1.0.0 references
- Stale references, contradictions, and underspecified behavior
**Last reviewed:** 2026-04-29
**Rationale:**
The spec set is broad and mostly coherent: the language, testbench, simulation, chip-data, and waveform formats are all documented at a usable level, and the implementation surface already supports the major directives and waveform backends these docs describe. The remaining issues are mostly release-polish and consistency, not missing core coverage: all owned specs still carry the Beta 0.1.8 banner, and at least one stale claim in the simulation docs conflicts with implemented FST support. I would not call these 1.0.0 reference-ready until the versioning and cross-document wording are cleaned up.

**Key measurements:**
- 5 owned spec files are present and together cover the core language, simulation, testbench, chip-info, and JZW surfaces.
- No `TODO`/`FIXME`/`XXX`/`HACK`/`assert(0)`/`abort(`/`not implemented`/`stub`/`unimplemented` markers were found under `specification/`.
- `specification/simulation-specification.md` still advertises `--fst` as "not yet supported" even though the codebase already includes FST writer support and CLI wiring.
- Every owned specification front matter still declares `State: Beta — Version: 0.1.8`, which is the clearest 1.0.0-readiness gap.

**Needed before 1.0.0:**
- Bump the spec front matter and release language from Beta 0.1.8 to 1.0.0.
- Reconcile waveform-format wording so simulation, JZW, and CLI docs agree on VCD/FST/JZW support.
- Do one final pass for section/link consistency after the version freeze.

**Surprising findings:**
- The implementation already supports FST output, so the FST issue is stale documentation rather than missing code.
- The JZW implementation writes richer per-clock metadata than the spec currently calls out explicitly.

### 2. Documentation & Project Guides — Score: 7 / 10
**Paths owned:** `docs/`, `README.md`, `LICENSE.md`
**Criteria scored against:**
- Accuracy against the current compiler, simulator, and tooling behavior
- Coverage of installation, usage, examples, and error-model guidance
- Information architecture and onboarding quality for a first-time user
- Broken, stale, or misleading instructions
- Release polish for a public 1.0.0 surface
**Last reviewed:** 2026-04-29
**Rationale:**
The documentation set is broad, well organized, and mostly accurate. The error model, reference manual, and worked examples are strong, but the first-run path still has a few release-critical correctness issues: the installation guide uses the wrong source/build paths and a nonexistent test file, the README has a typo in its test build command, and the simulation docs still claim `--fst` is unsupported even though the CLI already exposes it. This is usable for 1.0.0, but not yet polished enough to call fully release-ready without fixing the onboarding mismatches.

**Key measurements:**
- No `TODO`/`FIXME`/`XXX`/`HACK`/`assert(0)`/`abort(`/`not implemented`/`stub`/`unimplemented` markers were found in the owned documentation files.
- The docs cover the major user journeys: installation, CLI usage, getting started, reference manual, migration, diagnostics/error model, examples, editor support, and licensing.
- `docs/getting-started/installation.md` points at `cmake -S jz-hdl -B jz-hdl/build` and `jz-hdl/tests/blink.jz`, which do not match the repository layout or test locations.
- `README.md` has a build/test typo (`cmake -S compilerz-hdl -B compiler/build -DBUILD_TESTING=ON`) that would fail for a new user.
- `docs/reference-manual/simulation.md` says `--fst` is "not yet supported", but the current CLI parser and usage text already accept `--fst`.
- The CLI exposes expansion-limit controls in `compiler/src/cli_options.h` and `--tristate-default` on more modes than the onboarding docs currently explain.

**Needed before 1.0.0:**
- Fix the installation and verification instructions to use the actual repo paths, build directory, binary path, and test targets.
- Reconcile the docs with the current CLI surface: remove the stale `--fst` note, document expansion-limit controls, and explain where `--tristate-default` applies.
- Do one final pass over top-level links and examples after the onboarding commands are corrected.

**Surprising findings:**
- The error-model docs are more complete than the onboarding docs, which is the right direction for a 1.0.0 reference surface.
- `LICENSE.md` is already clean and explicit about the licensing split, so it does not look like a release risk.

### 3. Compiler Frontend — Score: 7 / 10
**Paths owned:** `compiler/src/lexer.c`, `compiler/src/parser/`, `compiler/src/ast/`, `compiler/src/repeat_expand.c`
**Criteria scored against:**
- Grammar and syntax coverage against the language specification
- Parse error quality, recovery behavior, and source locations
- AST completeness and maintainability
- Test coverage for accepted and rejected syntax forms
- Obvious robustness risks, undefined behavior, and unfinished paths
**Last reviewed:** 2026-04-29
**Rationale:**
The frontend is broad and clearly beyond "usable but rough": the lexer, recursive-descent parser, AST layer, and pre-parse `@repeat` expander together cover the main language, project, template, simulation, and testbench surfaces with substantial validation coverage behind them. The main release-readiness drag is diagnostic polish and recovery behavior, not missing core grammar: generic `PARSE000` is still a common fallback, several validation fixtures explicitly document cascading parse errors after template-related invalid constructs, and one parser TODO still marks an incomplete `@top` BUS-binding path. This is solid enough for a 1.0.0 compiler core, but not yet polished enough to call spec-complete and parser-hardened.

**Key measurements:**
- The owned frontend surface is 20 source files and 12,987 lines across `lexer.c`, `repeat_expand.c`, `parser/*.c`, and `ast/*.c`.
- Marker scan found 1 owned `TODO` and no owned `FIXME`/`XXX`/`HACK`/`assert(0)`/`abort(`/`not implemented`/`stub`/`unimplemented` hits; the remaining TODO is in `compiler/src/parser/parser_project.c` for non-trivial BUS targets in project `@top` bindings.
- The validation suite has at least 91 frontend-adjacent `.jz` fixtures covering parser, repeat, template, and syntax surfaces, plus dedicated `misc_RPT*.jz` fixtures for `@repeat` errors and limits.
- Multiple validation fixtures explicitly note cascading `PARSE000` behavior, concentrated around template-forbidden-content recovery paths.
- `compiler/src/parser/parser_core.c` still emits generic `PARSE000` text for unrecovered syntax failures, while newer paths already use rule-based diagnostics to avoid that fallback.
- `compiler/src/repeat_expand.c` is better hardened than a typical text preprocessor: it enforces count and expanded-size limits, handles nesting, and has dedicated diagnostics for invalid count, missing `@end`, and limit overflow.

**Needed before 1.0.0:**
- Reduce generic `PARSE000` fallback coverage, especially in template-forbidden-content and malformed-structure paths, so invalid programs produce the intended rule diagnostics without cascades.
- Finish or deliberately constrain the incomplete project `@top` BUS-binding parser path instead of shipping a TODO-backed partial behavior.
- Do a parser recovery pass on known bad-context constructs so one invalid directive or block header does not poison the remainder of the file.
- Audit duplicated expression-cloning and token-to-string assembly logic across parser files; it works, but it is a maintainability risk for post-1.0 grammar changes.

**Surprising findings:**
- The `@repeat` preprocessor is more production-ready than the ordinary parse-error surface; it already has hard expansion limits and focused diagnostics, while the parser still falls back to `PARSE000` in too many places.
- The parser is intentionally permissive in several declaration contexts to let semantic analysis own better diagnostics, which is the right architectural choice for 1.0.0 even though the recovery polish is not finished.

### 4. Compiler Semantic Analysis — Score: 7 / 10
**Paths owned:** `compiler/src/sem/`, `compiler/src/compiler.c`, `compiler/src/arena.c`, `compiler/src/util.c`
**Criteria scored against:**
- Enforcement of language invariants and type/driver/clock rules
- Feature completeness against the specification
- Diagnostic precision at semantic-analysis time
- Validation coverage for rule enforcement and regression resistance
- Memory-safety and maintainability risks in core compiler logic
**Last reviewed:** 2026-04-29
**Rationale:**
The semantic core is broad and materially stronger than "usable but rough": `jz_sem_run()` stages project checks, name resolution, expression typing, memory/resource validation, net-graph construction, exclusivity analysis, dead-code checks, and clock-domain enforcement in a coherent pass order, and the owned code directly implements most of the width, driver, tristate, CDC, memory, and project-structure rules the specs describe. The main release-readiness gaps are not missing core RTL semantics, but uneven completeness around adjacent surfaces and engineering polish: the dedicated testbench semantic path still advertises itself as a Phase 1 subset and currently validates only a small slice of the TB rule table, and template scratch-wire expansion is still nondeterministic because names are salted with `rand()` seeded from wall-clock time. Diagnostics are generally specific and rule-based, but the subsystem is large and monolithic enough that maintainability and regression resistance still depend more on broad golden tests than on fine-grained unit coverage.

**Key measurements:**
- The owned surface is 29 files and 32,427 lines across `compiler/src/sem/`, `compiler/src/compiler.c`, `compiler/src/arena.c`, and `compiler/src/util.c`.
- Marker scan found 0 owned `TODO`/`FIXME`/`XXX`/`HACK`/`assert(0)`/`abort(`/`not implemented`/`stub`/`unimplemented` hits in the owned code.
- `jz_sem_run()` currently executes 9 major semantic stages after parse/template work: lexical identifier checks, symbol-table build, project checks, name resolution, expression/type checks, memory-resource checks, net-graph checks, exclusive-assignment/dead-code checks, and clock-domain checks.
- The validation suite contains a large semantic surface, but only 3 direct semantic unit-test source files exist under `compiler/src/sem/`: `const_eval_test.c`, `literal_test.c`, and `type_test.c`.
- `compiler/src/sem/driver_testbench.c` explicitly describes itself as a "Phase 1 subset" and, on direct read, only implements structural checks around module existence and `TEST`/`@new`/`@setup` shape rather than the full TB rule table documented in the spec.
- `compiler/src/sem/template_expand.c` still seeds `rand()` with `time(NULL)` and uses random suffixes for scratch-wire renaming, making expanded internal names vary across runs.

**Needed before 1.0.0:**
- Either complete the promised testbench semantic coverage to match the published TB rule table or narrow the spec/docs so the implemented subset is what 1.0.0 actually claims.
- Replace time-seeded random scratch-wire suffixing with a deterministic naming scheme so expansion output is reproducible across builds and easier to diff/debug.
- Add more focused regression coverage around semantic helper boundaries, especially name resolution, width evaluation, and tristate/CDC interactions that currently rely mostly on end-to-end validation fixtures.
- Break up or better isolate some of the largest semantic translation units, especially `driver.c`, to reduce the risk of future rule changes causing cross-pass regressions.

**Surprising findings:**
- The RTL semantic engine is substantially more complete than the testbench semantic path; the maturity gap is inside the same owned subsystem rather than between separate top-level components.
- Memory-safety hygiene in the support code is better than average for a C compiler of this size: `util.c` uses checked size growth, `arena.c` is simple and disciplined, and the semantic driver does explicit cleanup on symbol-table build failure.

### 5. Compiler IR & Middle-end — Score: 7 / 10
**Paths owned:** `compiler/src/ir/`
**Criteria scored against:**
- Faithfulness of lowering from frontend semantics into IR
- Transform correctness and preservation of invariants
- Serialization and inspection support for debugging and regressions
- Testability and observability of middle-end behavior
- Signs of brittle lowering logic or partially implemented transforms
**Last reviewed:** 2026-04-29
**Rationale:**
The IR layer is substantial and already doing real compiler work rather than acting as a thin handoff: it lowers expressions, statements, memories, clocks, instances, specializations, CDC library insertion, tri-state elimination, differential-output prep, division-guard analysis, and memory-init lowering, with JSON serialization good enough to support golden diffing. The main 1.0.0 concerns are architectural brittleness and transform complexity, not missing core capability: `jz_ir_build_design()` re-derives symbol tables and net graphs instead of consuming a canonical semantic artifact, and the largest mutating pass (`ir_tristate_transform.c`) is complex enough that it depends on clone-and-rollback safety plus a growing matrix of transform-specific validation cases. This is solid and production-leaning, but not yet polished enough to call deeply hardened middle-end infrastructure.

**Key measurements:**
- The owned IR surface is 16 files and a little over 22k lines across `compiler/src/ir/`.
- Marker scan found 0 owned `TODO`/`FIXME`/`XXX`/`HACK`/`assert(0)`/`abort(`/`not implemented`/`stub`/`unimplemented` hits.
- The golden suite currently contains dozens of checked-in `test.ir` artifacts, giving broad snapshot coverage of emitted IR structure.
- Validation coverage is strongest around mutating passes, especially tri-state transform, division-guard, serializer, and memory-init rule families.
- `jz_ir_build_design()` explicitly rebuilds symbol tables and net graphs for IR construction instead of lowering from an already-materialized semantic result, which increases drift risk between semantic and IR phases.
- `jz_ir_tristate_transform()` clones the whole design, transforms a working copy, and emits `TRISTATE_TRANSFORM_ROLLBACK` on failure; that is a good safety valve, but it is also evidence that this pass is complex enough to need transactional behavior.
- `compiler/src/ir/ir_serialize.c` still stamps JSON as `\"ir_version\": \"0.1.0\"` and falls back to generic strings for unknown node kinds instead of enforcing stronger exhaustiveness.

**Needed before 1.0.0:**
- Reduce semantic/IR drift risk by either lowering from canonical semantic outputs or adding tighter regression coverage around places where IR rebuilds semantic context independently.
- Harden the tri-state transform further, especially around OE extraction and rollback-triggering edge cases, so more failures are prevented earlier instead of recovered after partial transform work.
- Tighten IR observability and release polish: update the serialized IR versioning story and make serializer coverage/exhaustiveness stricter so new IR node shapes cannot silently degrade debug output.
- Add more focused IR-level regression tests for library insertion, memory-init lowering, and transform postconditions rather than relying mostly on end-to-end backend or validation outcomes.

**Surprising findings:**
- The most mature part of this subsystem is its safety posture around risky transforms: clone-and-rollback is already in place for tri-state rewriting.
- The biggest design risk is not a visible TODO in the IR code; it is that the IR builder intentionally mirrors part of semantic analysis instead of consuming a single semantic source of truth.

### 6. Compiler Backends — Score: 7 / 10
**Paths owned:** `compiler/src/backend/`
**Criteria scored against:**
- Correctness and completeness of Verilog and RTLIL emission
- Constraint and wrapper generation quality
- Backend-specific diagnostics and unsupported-case handling
- Golden-test coverage and determinism of emitted artifacts
- Synthesis-facing release readiness for supported targets
**Last reviewed:** 2026-04-29
**Rationale:**
The backend layer is substantial and already doing production-shaped work: both Verilog-2005 and RTLIL emitters cover modules, ports, memories, instances, async and synchronous logic, top-level wrappers, differential I/O, and chip-template-driven clock-generator insertion, and both main drivers emit in deterministic module-id order with atomic temp-file replacement. The main 1.0.0 gaps are around hardening and observability rather than missing the core output paths: constraint emitters have no checked-in golden artifacts, some backend failure modes still surface as comments or stderr-side text instead of structured diagnostics, and one RTLIL wrapper TODO still marks incomplete multi-bit inverted-port support. This is solid enough to ship behind the core compiler, but not yet polished enough to call synthesis-hardened across every supported target/output combination.

**Key measurements:**
- The owned backend surface is 20 files and roughly 12k lines across `verilog-2005/` and `rtlil/`.
- Marker scan found 1 owned `TODO` and no owned `FIXME`/`XXX`/`HACK`/`assert(0)`/`abort(`/`not implemented`/`stub`/`unimplemented` hits; the remaining TODO is in `compiler/src/backend/rtlil/emit_wrapper.c` for multi-bit inverted ports.
- The golden suite contains a broad set of checked-in `test.v` and `test.il` artifacts, giving strong snapshot coverage for both emitted HDL forms.
- I found 0 checked-in golden `.sdc`, `.xdc`, `.pcf`, or `.cst` artifacts under `compiler/tests/golden`, so constraint generation is implemented but not snapshot-tested the way Verilog and RTLIL are.
- Unsupported statement kinds are still emitted as comments (`/* unsupported stmt kind %d */` in Verilog and `# unsupported stmt kind %d` in RTLIL) rather than causing a structured backend diagnostic.
- The backend writers use temporary output files and rename into place, which is a strong determinism and failure-containment signal.

**Needed before 1.0.0:**
- Add direct golden coverage for generated constraint files (`.sdc`, `.xdc`, `.pcf`, `.cst`), especially around differential pins, array flattening, and chip-specific IO-standard and clock handling.
- Replace comment and stderr fallback behavior for unsupported backend cases with structured diagnostics or hard failures so invalid backend states cannot silently leak into emitted artifacts.
- Finish or explicitly constrain the RTLIL wrapper's incomplete multi-bit inverted-port path before calling wrapper generation fully release-ready.
- Do a synthesis-facing verification pass across the supported chip families for wrapper and clock-generator template expansion, since those paths are more lightly exercised than plain module emission.

**Surprising findings:**
- The strongest release signal here is determinism: both backends intentionally emit in stable order and write through temporary files before rename.
- The weakest gap is not Verilog or RTLIL body emission itself; it is that constraints and a few wrapper edge cases are less test-hardened than the main HDL outputs.

### 7. Simulator & Waveforms — Score: 6 / 10
**Paths owned:** `compiler/src/sim/`
**Criteria scored against:**
- Conformance to the simulator and waveform specifications
- Correctness of execution, state updates, and waveform capture
- Performance sanity and obvious algorithmic pathologies
- Robustness of output formats and error handling
- Coverage through simulation fixtures and examples
**Last reviewed:** 2026-04-29
**Rationale:**
The simulator is doing real work and is clearly beyond a prototype: it has a unified engine for `@testbench` and `@simulation`, hierarchical combinational settling, deferred-NBA simulation semantics, runtime checks for non-convergence and illegal z-propagation, and three waveform backends. The main reason this does not score alongside the other solid compiler subsystems is that there are still release-level correctness and hardening gaps inside the owned code itself, not just missing polish. Most importantly, `--simulate` runtime failures do not currently propagate to a nonzero return, waveform dumping truncates values to `val[0]` even though the simulator advertises up to 256-bit `SimValue`s, and a large share of arithmetic and intrinsic execution in `sim_value.c` is still effectively 64-bit-only. That leaves the simulator usable and featureful, but not yet trustworthy enough to call production-ready for a 1.0.0 HDL toolchain core.

**Key measurements:**
- The owned simulator surface is 20 files and roughly 7.4k lines across execution, state, value semantics, performance hooks, and the VCD, FST, and JZW waveform backends.
- Marker scan found no owned `TODO`/`FIXME`/`XXX`/`HACK`/`assert(0)`/`abort(`/`not implemented`/`unimplemented` hits; the only match was a benign "no-op stubs" comment in `sim_perf.h`.
- Direct simulation coverage is still thin: `compiler/tests/simulation/` contains 5 `.jz` simulation fixtures, `compiler/tests/testbenches/` contains 4 `.jz` testbench fixtures, and only a small number of golden shell tests are simulation-focused.
- All checked-in waveform reference artifacts under `compiler/tests/simulation/` are VCDs; I found no direct golden coverage for FST or JZW output structure.
- `wave_dump_all()` passes only `value.val[0]` into the waveform layer, and all three backend dump APIs take `uint64_t value`, so waveform capture is effectively truncated to 64 bits despite `SimValue` supporting wider values.
- `compiler/src/sim/sim_value.c` has some multiword support for concat, slice, and masking, but core arithmetic, division/modulus, shifts, and several intrinsic paths still operate on `val[0]`.
- `jz_sim_run_simulations()` only sees failures if `sim_run_simulation()` returns nonzero, but `sim_run_simulation()` currently returns success even after timeout or runtime-error reporting, so `--simulate` mode can print failure text while still exiting successfully.

**Needed before 1.0.0:**
- Fix simulation failure propagation so runtime errors and timeout conditions in `--simulate` produce a nonzero command exit, not just stderr or stdout text.
- Make waveform dumping width-correct for the full supported `SimValue` range instead of silently truncating captured values to 64 bits.
- Audit and complete wider-than-64-bit execution semantics in `sim_value.c`, especially arithmetic, shifts, comparisons, and intrinsic helpers that currently collapse to `val[0]`.
- Add direct regression coverage for FST and JZW outputs, not just VCD and command-success smoke checks.
- Strengthen simulation-mode tests so they validate failure behavior and waveform correctness, not only successful command completion.

**Surprising findings:**
- The simulator's control-flow and event model are more mature than its data-path width handling; the architecture looks 1.0-shaped, but wide-value correctness is still behind it.
- The biggest release risk is not a visible TODO in the codebase; it is that `--simulate` can report a runtime error and still appear successful to automation.

### 8. Diagnostics, Reports & Rules Surface — Score: 6 / 10
**Paths owned:** `compiler/src/diagnostic.c`, `compiler/src/rules.c`, `compiler/src/report/`
**Criteria scored against:**
- Clarity, consistency, and usefulness of diagnostics and reports
- Coverage and maintainability of lint-rule metadata
- Output stability for user-facing report formats
- Traceability between rules, emitted messages, and documented behavior
- Gaps that would make debugging or adoption materially harder at 1.0.0
**Last reviewed:** 2026-04-29
**Rationale:**
This surface has the right overall architecture for 1.0.0: diagnostics are centralized, rule metadata is explicit and extensive, same-line prioritization is deterministic, and the report family covers aliasing, memory mapping, tri-state analysis, and chip introspection. The release-readiness problem is traceability and hardening, not absence of machinery. There are still meaningful mismatches between documented behavior and actual output policy, `PARSE000` remains a real unregistered user-visible code path, several rule IDs are known to be unreachable or misleading in practice, and I found no direct golden coverage for the report outputs themselves. That leaves the surface useful and broad, but not yet polished enough to be the debugging interface users will rely on at 1.0.0.

**Key measurements:**
- The owned surface is 6 files and roughly 6k lines across `diagnostic.c`, `rules.c`, and 4 report emitters under `compiler/src/report/`.
- Marker scan found 0 owned `TODO`/`FIXME`/`XXX`/`HACK`/`assert(0)`/`abort(`/`not implemented`/`stub`/`unimplemented` hits.
- `compiler/src/rules.c` defines a large rule table across dozens of groups, with explicit priorities and severities feeding deterministic same-line filtering in `jz_diagnostic_print_all()`.
- `jz_diagnostic_apply_warning_policy()` only suppresses diagnostics whose runtime severity is `WARNING`, but the docs currently describe `--Wno-group=NAME` more broadly than that implementation.
- `PARSE000` is still a real emitted code path referenced by spec, docs, and tests, but it is not registered in `rules.c`, which weakens `--lint-rules` completeness and rule-to-doc traceability.
- I found 0 direct golden tests for `--alias-report`, `--memory-report`, `--tristate-report`, or `--chip-info` output formats under `compiler/tests/`.
- Report output stability is mixed: the tri-state report prints a local-time `Generated:` timestamp, while alias and memory reports do not emit equivalent metadata.

**Needed before 1.0.0:**
- Decide whether `PARSE000` is a supported public diagnostic code; if yes, register and document it consistently, and if not, eliminate it from user-visible output paths.
- Reconcile CLI and docs with implementation for diagnostic controls, especially `--Wno-group` semantics and warning/info behavior.
- Fix misleading or unreachable rule surfaces around known cases where current emitted messages do not match the nominal rule name.
- Add direct golden coverage for report modes (`alias`, `memory`, `tristate`, `chip-info`) so user-facing report formats can evolve safely.
- Remove or gate nondeterministic report fields such as the tri-state report's local-time generation stamp if these outputs are meant to be CI-stable artifacts.
- Close the known missing-coverage gaps for rule families that are already in the public table, especially chip-variant, testbench, and simulation entries.

**Surprising findings:**
- The main diagnostics printer is more deterministic than the surrounding rule surface: tie handling and same-line filtering are intentionally stable, but the public rule catalog still has real emitted codes outside the catalog.
- Small severity mismatches between docs and `rules.c` are already present, which is exactly the kind of inconsistency that becomes user-visible once the rule surface is treated as contractual.

### 9. Chip Data & Vendor Support — Score: 6 / 10
**Paths owned:** `compiler/data/`, `compiler/src/chip_data.c`, `compiler/src/chip_data_internal.h`, `compiler/src/report/chip_report.c`
**Criteria scored against:**
- Correctness and completeness of bundled chip definitions
- Conformance to the chip-info specification
- End-to-end usability of supported vendor targets
- Validation of special resources, fixed pins, and constraint data
- Maintainability and safety of the chip data loading path
**Last reviewed:** 2026-04-29
**Rationale:**
This surface is already doing meaningful end-to-end work: chip JSON drives memory-capacity checks, clock-generator parameter and range validation, differential I/O lowering, wrapper emission, and constraint generation across several vendor families. The main release-readiness problem is that the support story is not yet internally consistent or fully hardened. Most notably, `compiler/data/` now contains 9 bundled chip JSON files but `compiler/src/chip_data.c` only embeds 8 of them, so `GW5A-LV25-MG121-C1-I0` is present in-repo yet unavailable through the built-in database and `--chip-info`; beyond that, loader conformance is narrower than the published chip-info spec, and the report surface has essentially no direct golden coverage.

**Key measurements:**
- The owned surface is 12 files: 9 chip JSON files plus 3 code files (`chip_data.c`, `chip_data_internal.h`, `chip_report.c`).
- The bundled JSON set covers 9 device variants across Gowin, Lattice iCE40, Lattice ECP5, and AMD/Xilinx Artix-7 lines.
- Marker scan found 0 owned `TODO`/`FIXME`/`XXX`/`HACK`/`assert(0)`/`abort(`/`not implemented`/`stub`/`unimplemented` hits.
- `compiler/CMakeLists.txt` glob-embeds every `compiler/data/*.json`, but `compiler/src/chip_data.c` manually includes only 8 generated headers and omits `gw5a-lv25-mg121-c1-i0.h`; the built-in chip table also has only 8 entries.
- All 9 JSON files contain the required `resources`, `clock_gen`, and `fixed_pins` keys, but `JZChipData` only materializes a subset of the full chip-info schema while other fields are parsed only by the report printer.
- I found no dedicated golden tests for `--chip-info` output formatting or completeness, and no tests that would catch the missing built-in `GW5A` entry from the standalone CLI path.

**Needed before 1.0.0:**
- Fix the built-in database mismatch so every JSON under `compiler/data/` that is intended to ship is actually embedded and discoverable through `jz_chip_builtin_count()`, `jz_chip_builtin_id()`, and `--chip-info`.
- Decide whether the chip-info specification is a full schema contract or just a reporting format; then either validate the required top-level sections during load or narrow the spec so it matches the compiler's actual consumed subset.
- Add direct golden coverage for `--chip-info`, especially builtin listing, prefix lookup, representative per-vendor reports, and fixed-pin, clock-gen, and differential sections.
- Reduce drift between loader and reporter by centralizing more chip parsing in one canonical structure instead of maintaining two parallel JSON-walking implementations.
- Do a final vendor-support audit on special-resource paths that depend on chip data indirectly: clock-gen constraints and chaining, differential serializer ratios, fixed pins, and board-pin-sensitive constraint generation.

**Surprising findings:**
- The biggest support gap is not bad JSON data; it is that one apparently complete bundled chip (`GW5A-LV25-MG121-C1-I0`) is silently missing from the built-in lookup table.
- The data files are richer than the core loader surface: `fixed_pins`, `boards`, and DSP and resource metadata are documented and printable, but most of that information is not represented in `JZChipData` itself.

### 10. CLI, Path Security & LSP — Score: 6 / 10
**Paths owned:** `compiler/src/main.c`, `compiler/src/cli_frontend.c`, `compiler/src/cli_frontend.h`, `compiler/src/cli_modes.c`, `compiler/src/cli_modes.h`, `compiler/src/cli_options.c`, `compiler/src/cli_options.h`, `compiler/src/path_security.c`, `compiler/src/lsp/`
**Criteria scored against:**
- CLI ergonomics, mode coverage, and argument validation
- Path sandboxing correctness and failure behavior
- LSP feature completeness and protocol robustness
- Stability of user-facing interfaces for a 1.0.0 release
- Handling of malformed input, I/O failures, and integration edge cases
**Last reviewed:** 2026-04-29
**Rationale:**
The CLI and path-security surfaces are solid enough to be useful in production: mode dispatch is clear, option parsing rejects many invalid flag combinations up front, and the sandbox implementation has real coverage for absolute-path, traversal, symlink-escape, and additional-root behavior. The score drops because the owned surface also includes the LSP, and that part is materially less hardened than the rest of the compiler: it is a hand-rolled JSON-RPC server with fixed-size extraction buffers, no direct regression suite, a checked-in cache-file workflow that mutates user workspaces, and a capability and version story that still reads like a pre-1.0 tool. This is release-usable, but not yet stable enough to be a polished 1.0.0 user interface layer.

**Key measurements:**
- The owned surface is 13 files and roughly 4.9k lines: 8 CLI and path-security files plus 5 LSP files.
- Marker scan found 0 owned `TODO`/`FIXME`/`XXX`/`HACK`/`assert(0)`/`abort(`/`not implemented`/`stub`/`unimplemented` hits.
- Path-security and wrong-tool validation coverage is present, including fixtures for absolute-path, traversal, outside-sandbox, symlink-escape, additional sandbox roots, and wrong-tool cases.
- I found no direct LSP regression tests under `compiler/tests/`; the only `lsp`-related files checked in there are `.jzhdl-lsp.rc` cache artifacts.
- `main.c` special-cases `--lsp` before normal option parsing, so `jz-hdl --lsp ...other flags...` silently hands control to the server instead of validating conflicting CLI input.
- The LSP transport accepts large messages at the I/O layer, but request handling still copies key JSON objects and document text into fixed local buffers such as `char text[65536]`, `char params[4096]`, and `char td[2048]`.
- The server advertises only full document sync and implements a small capability set: hover, keyword-only completion, and same-file definition.
- `lsp_project_discovery.c` reads, writes, and deletes `.jzhdl-lsp.rc` files in user directories as part of normal operation, which is functional but a rough workspace-mutating contract for a 1.0.0 language server.

**Needed before 1.0.0:**
- Harden the LSP message parser and document handling so oversized files and large JSON payloads fail explicitly instead of being truncated or ignored by fixed-size buffers.
- Add direct LSP regression coverage for initialize, open, change, save, hover, completion, definition, plus project-discovery and selected-project behavior; the current surface is effectively untested compared with the compiler proper.
- Revisit the `.jzhdl-lsp.rc` cache-file design or at least document and constrain it clearly; shipping a language server that writes and removes hidden files in user workspaces needs a firmer contract.
- Make the version surface consistent across CLI, LSP, and docs so `--version`, LSP `serverInfo`, and release documentation all describe the same product state.
- Tighten edge-case CLI validation around mode ownership and flag limits, especially cases like `--lsp` mixed with other flags and silently ignored excess repeated options.
- Expand editor-facing capability coverage or explicitly narrow the 1.0.0 promise to basic diagnostics, hover, completion, and definition only.

**Surprising findings:**
- The path sandbox is more mature than the LSP around it; the risky filesystem surface has focused rule coverage, while the editor integration still relies on ad hoc JSON parsing and cache files.
- The LSP already accepts custom project-selection notifications from the VS Code extension, so the architecture is more specialized and extension-coupled than the generic `jz-hdl --lsp` docs imply.

### 11. Test Infrastructure & Validation Assets — Score: 6 / 10
**Paths owned:** `compiler/tests/`, `pipeline/`, `audit/`, `security-audit/`
**Criteria scored against:**
- Coverage breadth across validation, golden, simulation, and targeted tests
- Reliability and maintainability of the test runners and configs
- Alignment between tests, rule surfaces, and declared project readiness checks
- Evidence that failures are actionable and not noisy
- Missing automation that would weaken a 1.0.0 release gate
**Last reviewed:** 2026-04-29
**Rationale:**
The test surface is broad enough to catch a large class of regressions: the validation corpus is huge, the golden suite covers AST, IR, Verilog, and RTLIL outputs, and the main runner also does Yosys parse checks, backend equivalence checks, testbench runs, and basic simulation smoke tests. The release-readiness problem is that the gate is still uneven and partly manual. `compiler/tests/issues.md` and `audit/runner.log` both document known missing or non-actionable coverage, simulation and testbench checks are much shallower than lint and golden coverage, and the auxiliary `pipeline/`, `audit/`, and `security-audit/` assets are review workflows and snapshots rather than an automated release gate wired into the normal test path. This is a solid engineering base, but not yet a fully trustworthy 1.0.0 quality bar.

**Key measurements:**
- `compiler/tests/validation/` currently contains 1,401 `.jz` fixtures and 1,375 matching `.out` files, and `compiler/tests/golden/` contains 58 `test.jz` cases with broad AST, IR, Verilog, and RTLIL artifact coverage plus custom `test.sh` checks.
- Direct runtime coverage is much thinner than lint and backend coverage: `compiler/tests/simulation/` has 5 `.jz` files and a few checked-in VCD references, while `compiler/tests/testbenches/` has 4 `.jz` files with mostly pass or fail smoke execution.
- `compiler/tests/run_validation.sh` runs validation, golden diffs, Yosys parse verification, Verilog-vs-RTLIL equivalence, testbench smoke tests, simulation smoke tests, and a cross-mode rejection pass, but it exits on the first validation mismatch and the runtime-mode sections generally assert command success more than detailed semantics.
- `compiler/tests/issues.md` still records multiple `compiler-bug`, `parser-recovery`, `missing-coverage`, `missing-happy-path`, and `test-quality` items, which is too much known test debt for a clean 1.0.0 gate.
- The audit and security side is lightweight and manual: `pipeline/`, `audit/`, and `security-audit/` exist, but they are not wired into the normal CTest path alongside `lint_validation`.
- `compiler/tests/update_golden.sh` still carries stale path examples and skips golden directories with custom `test.sh`, leaving part of the golden estate outside the normal regeneration path.

**Needed before 1.0.0:**
- Close the known high-value gaps in `compiler/tests/issues.md`, especially testbench and simulation rule coverage that is currently missing even where the rules are already public.
- Strengthen runtime-mode assertions: simulation tests should verify expected failure behavior and waveform or output semantics, and testbench tests should check expected output rather than only zero exit status.
- Fix the release-gate blind spot where `--simulate` smoke tests can pass on exit-code success alone; with the current simulator failure-propagation bug elsewhere in the codebase, this harness can false-pass runtime failures.
- Improve failure batching and maintenance ergonomics in `compiler/tests/run_validation.sh` and `compiler/tests/update_golden.sh`, including removing stale path examples and making golden regeneration cover the custom-script cases more explicitly.
- Decide whether `pipeline/`, `audit/`, and `security-audit/` are part of the 1.0.0 quality gate or advisory tooling; if they matter for release readiness, wire them into a repeatable automated workflow instead of leaving them as manual snapshots.
- Add direct golden or structured checks for report-mode, FST and JZW, and cross-mode rejection surfaces that are currently documented as missing, not-testable, or only partially exercised.

**Surprising findings:**
- The biggest weakness here is not raw test count; it is that the strongest coverage is concentrated in lint and backend artifact generation, while runtime simulation and testbench behavior is comparatively lightly asserted.
- The `pipeline/` directory no longer contains the large `test_*.md` rule-plan corpus described elsewhere; what exists today is a small prompt and config runner plus generated audit outputs, which makes the audit infrastructure look more ad hoc than the project description suggests.

### 12. Waveform Viewer — Score: 6 / 10
**Paths owned:** `viewer/`
**Criteria scored against:**
- Buildability and portability from a clean checkout
- Feature completeness against the advertised waveform-viewing story
- Input robustness and failure behavior on malformed or large traces
- Packaging and release readiness for end users
- Signs of stubbed or experimental functionality
**Last reviewed:** 2026-04-29
**Rationale:**
The viewer is already a real product surface rather than a placeholder: it has a native UI, live JZW reload support, cursoring, annotations, and clock metadata, and the README is much more complete than most secondary tools in the repo. The score stays at 6 because it is still a single large translation unit with explicit feature limitations, no visible automated test coverage, and no packaging or release flow beyond local CMake builds. That makes it promising and usable, but not yet a hardened 1.0.0 companion tool.

**Key measurements:**
- The owned surface is 3 files and about 2.7k lines across `viewer/src/main.cpp`, `viewer/README.md`, and `viewer/CMakeLists.txt`.
- `viewer/src/main.cpp` is a single-file implementation at 2,466 lines.
- `viewer/README.md` has an explicit `Limitations / TODO` section listing no file-open dialog, no persistence, no search or filter, single-file implementation, and JZW-only support.
- I found no dedicated viewer tests, CI jobs, or packaging artifacts in the repo.
- The build pulls SDL3, SQLite, and Dear ImGui via `FetchContent`, which is convenient for local builds but still leaves portability and dependency pinning as part of the release risk surface.

**Needed before 1.0.0:**
- Split the viewer into smaller modules or at least isolate file I/O, JZW parsing, and rendering paths so bugs in one area are easier to test and fix.
- Add at least smoke-level automated coverage around JZW load, poll, and annotation rendering semantics.
- Decide on the 1.0.0 packaging story: binary distribution, platform support, and whether JZW-only is the intended public contract.
- Close the highest-visibility UX gaps called out in the README, especially file-open ergonomics and signal search or filtering.

**Surprising findings:**
- The viewer README is one of the more polished documents in the repo, which makes the lack of matching automation stand out more sharply.
- The biggest maturity gap here is not missing features in the waveform canvas; it is that the whole tool still lives in one large source file with no test harness.

### 13. VS Code Extension — Score: 6 / 10
**Paths owned:** `vscode-ext/`
**Criteria scored against:**
- Install/build/run path from a clean developer machine
- LSP client integration quality and resilience to missing binaries
- Syntax highlighting and editor experience completeness
- Packaging, versioning, and marketplace-readiness signals
- Clear failure behavior and user guidance
**Last reviewed:** 2026-04-29
**Rationale:**
The VS Code extension is small but functional: it wires up syntax highlighting, launches the language server, exposes a project-selection command, and surfaces startup failures clearly in the editor. The score is held down by release polish and breadth. The package still declares version `0.1.0`, has no visible automated tests, depends on the same minimal LSP capability set scored earlier, and does not show a complete marketplace or VSIX release story. That is enough for internal use, but not yet enough for a polished 1.0.0 editor integration.

**Key measurements:**
- The owned surface is 6 primary project files plus generated and dependency artifacts excluded from review.
- The implementation core is small: `vscode-ext/src/extension.ts` is 238 lines and `vscode-ext/package.json` is 84 lines.
- The extension declares package version `0.1.0`, which is out of step with the rest of the repo's current beta and 1.0.0-readiness framing.
- I found no extension tests, no CI job that builds or exercises the extension, and no VSIX packaging automation in the repo.
- The extension relies on the compiler's `--lsp` surface for almost all higher-level behavior, so its real capability ceiling is currently bounded by the LSP limitations already noted in group 10.

**Needed before 1.0.0:**
- Align package versioning and release metadata with the rest of the project's 1.0.0 story.
- Add basic extension coverage or CI smoke checks for activation, binary-path handling, and the custom project-selection notification flow.
- Decide on the public distribution path: VSIX artifact generation, marketplace publishing, or explicit "local extension only" positioning.
- Document the actual supported feature set more tightly so users are not promised more than diagnostics, hover, completion, and definition.

**Surprising findings:**
- The extension code itself is not the main risk; its maturity is mostly capped by the underlying LSP and the absence of packaging and test automation.
- The project-selection UX is already specialized enough that the extension and server are more tightly coupled than the generic language-support framing suggests.

### 14. Examples, Build & Release Infrastructure — Score: 5 / 10
**Paths owned:** `examples/`, `.github/`, `scripts/`, `compiler/CMakeLists.txt`, `compiler/cmake/`, `.gitignore`, `.jzhdl-lsp.rc`
**Criteria scored against:**
- Clean-build reproducibility for examples and main toolchain artifacts
- CI coverage of the surfaces that matter for a 1.0.0 release
- Release automation, version stamping, and packaging readiness
- Example quality, representativeness, and instructional value
- Friction points in the contributor and release-engineering path
**Last reviewed:** 2026-04-29
**Rationale:**
This is the weakest remaining surface because it combines three things that are all important at release time: examples, CI, and release engineering. The example set is ambitious and representative, but the examples have no local READMEs and depend heavily on per-directory Makefiles and external FPGA toolchains. CI only builds and tests the compiler tree on Ubuntu, and the release script is still a local shell workflow with destructive `git checkout -- .` cleanup, macOS-specific `sed -i ''`, and no binary or extension publishing path. That means the project can be built and exercised by a maintainer, but the repo still lacks a robust, repeatable 1.0.0 release pipeline.

**Key measurements:**
- `examples/` currently contains 13 example directories, 13 Makefiles, and no per-example README files.
- The examples are broad in scope, ranging from simple counters and latches to CPU, SoC, DVI, audio, domains, and crypto designs, but nearly all operational guidance is encoded in Makefiles rather than example-local documentation.
- `.github/workflows/ci.yml` contains a single Ubuntu job that configures `compiler/`, runs CTest, and runs `compiler/tests/run_validation.sh`; it does not build `viewer/`, `vscode-ext/`, or any example targets.
- `scripts/release` is a local shell release flow that edits versions in place, commits and tags, and packages tarballs, but it uses macOS-specific `sed -i ''` invocations and rolls back failures with `git checkout -- .`.
- `scripts/run_examples.sh` produces a synthesis status table, but it depends on external FPGA toolchains and writes a timestamped markdown artifact rather than acting as a CI-enforced gate.

**Needed before 1.0.0:**
- Add a real release pipeline that is cross-platform enough for maintainers, avoids destructive repository resets, and covers the actual shipped artifacts.
- Expand CI to cover at least the viewer build, VS Code extension build, and a representative example or synthesis matrix instead of only the compiler tree.
- Add minimal example-local documentation so examples are not discoverable only through Makefiles and external docs pages.
- Decide which examples are officially supported at 1.0.0 and gate those builds more directly in automation.
- Remove or redesign local-release steps that assume a clean interactive maintainer environment and macOS-specific tooling.

**Surprising findings:**
- The example corpus is one of the repo's strongest demonstrations of ambition, but it is also one of the least self-describing surfaces because there are no example-local READMEs.
- The release script looks more like a maintainer convenience script than a hardened release mechanism, which is a major gap this late in a 1.0.0 readiness pass.
