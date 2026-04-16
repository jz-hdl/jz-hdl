# Plan: Project-Wide 1.0.0 Readiness Review

## Context

The user wants a full 1.0.0 release-readiness review of the `jz-hdl-dev` project. This is a meta-task: the work itself is reading the codebase and producing `release-todo.md` at the project root. No source code is modified.

Deliverable: `/Users/justinzaun/Development/jz-hdl-dev/release-todo.md` — a markdown file that scores each logical unit of the project 1–10 against 1.0.0 release quality, explains the score, lists high-level items outstanding before 1.0.0, and gives an overall project score with rationale.

**This plan is idempotent.** It can be run on a clean repo (no `release-todo.md`) or on a repo that already has one. On a fresh run it creates the file from scratch. On a re-run it **updates the existing file in place**: keeps the group structure, refreshes scores and rationales where the underlying code has changed, leaves unchanged sections alone (or refreshes their measurements with current numbers), and recomputes the calibration + overall score against the new state. The plan never wholesale-deletes or rewrites the file when one already exists — it edits.

User-specified execution process (3 steps, file is updated after each):
1. Full scan → identify logical groups → write initial file with group list (or, on re-run, reconcile the group list against the existing file).
2. Detailed per-group review → score, rationale, TODO list per group. Step 2a repeats #2 for every group found in #1. On re-run, each agent sees the existing section and is asked to update it rather than write from scratch.
3. Overall review → aggregate judgment, explain how the overall rank was reached. On re-run, this section is recomputed and replaced.

Agreed clarifications from the user:
- **Criteria:** I choose criteria per-group based on what makes sense for that unit (compiler vs. spec vs. example score on different things). Each group's criteria must be listed in the file.
- **Depth:** Accurate, group-by-group. Deep enough to be trustworthy, not just a TODO/FIXME scan.
- **Subagents:** `Explore` subagents may be dispatched in parallel as a **starting point** per group, but their summaries are not the final product — I must read critical files directly and form my own judgment before scoring.
- **File lifecycle:** Additive. `release-todo.md` grows and becomes more complete with each step. Do not replace-in-place or lose earlier content.
- **Overall rank:** Independent judgment that takes everything into account, with an explanation of how the score was reached.
- **Priority tags on TODO items:** Not needed at this point.
- **Format:** Markdown.
- **Exclusions:** `datasheets/`, `.git/`, and any build/output folders (`build/`, `target/`, `dist/`, `out/`, `node_modules/`, compiler-emitted `*.jzw`/reports inside examples). `examples/` source is **in**; example build outputs are **out**.
- **Approval gating:** None. The user wants this all in one pass.

## Top-Level Project Map (from initial scan)

Languages/tooling:
- **C** (~165 files) — main compiler at `compiler/`, built with CMake, bundles SQLite3
- **C++** — waveform viewer at `viewer/`, built with CMake
- **TypeScript/Node** — VS Code extension at `vscode-ext/`
- **Markdown + VitePress** — documentation site at `docs/`
- **Markdown** — language/sim/testbench/chip-info/jzw specifications at `specification/`
- **Makefile** — per-example builds at `examples/*/Makefile`

Top-level directories in scope:
- `compiler/` — lexer, parser, AST, semantic, IR, backends (Verilog, RTLIL), simulator, diagnostics, reports, LSP, CLI, chip data loader, SQLite
- `compiler/data/` — `*.json` chip definition files (vendor FPGA support data)
- `compiler/tests/` — validation/CTest suite
- `specification/` — 5 authoritative spec documents
- `pipeline/` — ~88 `test_*.md` rule-coverage specs (~316 test cases)
- `examples/` — 14 example projects (counter, latch, uart_echo, ascon, lcd, dvi, pll, uart_audio, cpu, soc, domains, terminal, etc.) — sources only
- `docs/` — VitePress documentation site
- `viewer/` — C++ waveform viewer
- `vscode-ext/` — VS Code extension + LSP client glue
- `scripts/` — utility/build scripts
- `.github/` — CI workflows
- Root files: `README.md`, `CLAUDE.md`, `AGENTS.md`, root `CMakeLists.txt`

Out of scope: `datasheets/`, `.git/`, `compiler/build/`, `viewer/build/`, `docs/.vitepress/dist/`, `vscode-ext/node_modules/`, `vscode-ext/out/`, any example `build/` directories, generated `*.jzw` inside example dirs.

## Tentative Logical Groups (to be validated in Step 1)

These are my starting-point groups. Step 1 will confirm, split, or merge them based on what the deep scan reveals. **Do not treat this list as final** — the actual group list is decided during execution.

1. **Language & Specification** — `specification/`, `pipeline/` rule docs
2. **Compiler — Frontend** — lexer, parser, AST, semantic analysis in `compiler/src/`
3. **Compiler — IR & Middle-end** — IR representation, transforms
4. **Compiler — Backends** — Verilog, RTLIL, any other code generation
5. **Simulator** — `compiler/src/sim/` and related
6. **Testbench & Verification** — testbench runner, assertions, validation tests
7. **Diagnostics & Reports** — diagnostics, rules engine, alias/memory/tristate reports
8. **Chip Data & Vendor Support** — `compiler/data/*.json`, `chip_data.c`, vendor FPGA data
9. **CLI & LSP** — `main.c`, `cli_*.c`, `path_security.c`, LSP server
10. **Waveform Viewer** — `viewer/`
11. **VS Code Extension** — `vscode-ext/`
12. **Examples** — `examples/` project sources
13. **Documentation Site** — `docs/` VitePress site + root README
14. **Build & CI Infrastructure** — root CMake, compiler CMake, `.github/`, `scripts/`, validation runner

Expected final count: 10–14 groups. Step 1 finalizes this.

## Per-Group Criteria (by group type)

Different unit types score on different things. The specific criteria used for each group will be written into that group's section of `release-todo.md`. General rubric by type:

**Specification/documentation groups** (Language Spec, Docs Site, Pipeline rules):
- Completeness vs. what the implementation actually supports
- Consistency across documents (no contradictions between spec and sim-spec, etc.)
- Clarity for a new reader
- Versioning/changelog presence
- Broken links, dead references, stale content

**Compiler component groups** (Frontend, IR, Backends, Diagnostics, CLI/LSP):
- Feature completeness vs. language spec
- Error handling & diagnostic quality (messages, source locations, recovery)
- Test coverage (CTest + pipeline rule coverage)
- Known compiler bugs / TODO / FIXME density
- Memory safety, leak-freedom, UB (for C code)
- Build cleanliness: warnings, sanitizer output if runnable
- API/CLI stability — would changes break users at 1.0.0?

**Simulator / testbench runtime groups**:
- Spec conformance (sim-spec and testbench-spec)
- Correctness on the examples (do they simulate and produce expected output?)
- Performance sanity (no obvious pathologies)
- Waveform output correctness (jzw spec conformance)

**Chip data / vendor support**:
- Chip coverage (which vendor parts are actually usable end-to-end)
- `.json` correctness vs. `chip-info-specification.md`
- Pin/resource definition completeness
- Any "fixed_pins" / special resources correctly modeled

**Tooling groups** (VS Code ext, Viewer):
- Does it build / install / run on a clean machine?
- Feature completeness vs. what the CLI/simulator can do
- Packaging/distribution story for 1.0.0 (marketplace entry? release binaries?)
- Error handling on missing tools, bad input

**Examples**:
- Do they all build from clean?
- Do they all simulate (where applicable)?
- Are they representative of the language's features?
- Code quality (don't teach bad patterns)
- README/comments explaining what each example demonstrates

**Build & CI infrastructure**:
- Reproducible build from clean checkout
- CI coverage (does CI actually exercise everything it should?)
- Release story (tags, binaries, version stamping)
- Contributor on-ramp (README build instructions work as written)

I will list the specific criteria I scored against at the top of each group's section in `release-todo.md`, so the score is auditable.

## Execution Steps

### Step 0 — Detect prior review state

Before doing any work, check whether `/Users/justinzaun/Development/jz-hdl-dev/release-todo.md` already exists.

- **If the file does NOT exist:** this is a **fresh run**. Proceed to Step 1 and follow it as written. The status header begins as "Step 1 complete" and progresses through "Review complete".
- **If the file DOES exist:** this is a **re-run**. Read the file in full into the main context. Capture: the existing group list (names + owned paths), each group's existing score and rationale, the existing Top Blockers list, the existing overall score, the existing date and "Last reviewed" timestamps if present. Update the header to reflect a re-run in progress (e.g. "Re-run in progress (previous review: <prior date>)"), but do **not** delete or rewrite any section yet. Then proceed to Step 1 in re-run mode.

The mode (fresh vs. re-run) determines the behavior of Steps 1–3 below. The mode is decided in Step 0 and does not change mid-run.

### Step 1 — Logical Group Identification & Initial File

**Fresh run:**
1. Glob the top-level project tree (in-scope only) to ground the groups in real directories and files.
2. Skim `README.md`, `AGENTS.md`, root `CMakeLists.txt`, `compiler/CMakeLists.txt`, `compiler/src/main.c`, and one-line every file in `compiler/src/` via Glob, to finalize the group boundaries.
3. Decide the final group list (expected 10–14 groups). Each group must be non-overlapping and must own specific paths.
4. **Write** `/Users/justinzaun/Development/jz-hdl-dev/release-todo.md` with:
   - Header (title, today's date, status: "Step 1 complete")
   - Exclusions list
   - Groups section: each group listed with its owned paths and the criteria I'll score it against (no score or rationale yet)
   - Placeholder overall section
5. Mark Step 1 done in the file's status header.

**Re-run:**
1. Glob the top-level project tree as in a fresh run, to ground the **current** state of the directory tree.
2. Compare the discovered structure against the **existing** group list captured in Step 0:
   - **Unchanged groups** (same name, same owned paths still exist): leave the section untouched in this step. Step 2 will refresh it.
   - **Group whose owned paths have shifted** (some files moved/renamed/added/deleted within the same logical unit): update the "Paths owned" block in place to match reality, but keep the score/rationale/TODO list pending until Step 2 re-evaluates.
   - **New group needed** (a new top-level directory or subsystem appeared that doesn't fit any existing group): add a new group section with paths and criteria, marked `Score: _pending_ / 10` and `Last reviewed: never`. Step 2 will score it.
   - **Group no longer applies** (the underlying code has been deleted or merged into another group): do **not** silently delete the section. Mark it `Score: N/A — group removed on <today's date>` with a one-line note explaining what happened, then exclude it from Step 2 and Step 3 calculations. The historical context stays in the file.
   - **Group needs to be split or merged** (e.g., a single group has grown enough to warrant two): present this as a structural change in a brief note in the file's "Re-run notes" section (created if needed), perform the split/merge, and re-score the affected groups in Step 2.
3. Update the file's header date to today, set status to "Re-run Step 1 complete (group list reconciled)", and record a one-line note in a "Re-run notes" subsection (under the header) summarizing what changed in the group list (or "no structural changes" if nothing did).
4. Do not touch any group's score/rationale/TODO list in Step 1 — that's Step 2's job.

In both modes, Step 1 produces a file whose group list matches the current code state. Scoring happens in Step 2.

### Step 1 file format conventions for re-runs
- **Header date** = today's date (always refreshed on each run)
- **Last reviewed** (per group) = today's date for groups that get re-evaluated in Step 2 of this run; previous timestamp for groups whose section is left unchanged
- **Re-run notes** subsection (only present after at least one re-run) records structural changes to the group list across runs in reverse chronological order

### Step 2 — Detailed Per-Group Review (repeated for every group)

**Execution model:** Each group's review runs in its own independent `Agent` (general-purpose subagent) context. The main context is kept lean — it only orchestrates. This protects against context pressure across a 10–14-group review. Groups run **serially**, one at a time, so the main context can sanity-check each returned section before dispatching the next.

For each group in the finalized list, in order, the main context does this:

1. **Read the current `release-todo.md`** to capture its full text (it has been updated with every prior group's completed section).
2. **Dispatch a general-purpose `Agent`** with a fully self-contained prompt containing:
   - The group name and its owned paths (from Step 1).
   - The criteria the group is to be scored against (from Step 1).
   - **The current full text of `release-todo.md`** — so the agent can see which groups have already been reviewed, what scores they received, and stay calibrated with the established tone and stringency. The agent is told: "use the prior groups as calibration anchors — do not invent your own scale."
   - **On a re-run: the previous section for this specific group**, extracted from the existing file (this is part of the "current full text"). The agent is told: "this group was previously scored. Read the previous rationale, key measurements, and TODO list. Then re-evaluate against the current state of the code. If the previous review is still accurate, return a refreshed section that keeps the same score and structure but updates measurements (line counts, TODO counts, etc.) with current numbers and refreshes any TODO items that have been completed. If something has materially changed (new code, new bugs, fixed bugs, new TODOs, score should change), return the updated section and **note the score change inline as `Score: X / 10 (was Y, <one-line reason>)`**. Do not silently change a score without flagging it."
   - **Explicit scoring anchors** so the agent doesn't re-invent the scale: `1 = broken/missing`, `3 = fragile, many gaps`, `5 = usable but rough`, `7 = solid, minor gaps`, `8 = production-ready with minor rough edges`, `9 = spec-complete and polished`, `10 = shipped-quality`.
   - The required work: explore the group's paths, read the critical files directly (not just grep), scan for `TODO`/`FIXME`/`XXX`/`HACK`/`assert(0)`/`abort(`/`not implemented`/`stub`/`unimplemented`, cross-reference against any relevant spec document, identify maturity signals (warnings, test coverage, empty error paths, stub functions), form an independent judgment.
   - The exclusions list (datasheets/, .git/, build folders, generated .jzw/reports).
   - The exact markdown section format to return (paths, criteria, score, rationale, key measurements, "Needed before 1.0.0" bullet list, and a short "Surprising findings" note if anything unexpected showed up). On a re-run the section must also include a `**Last reviewed:** <today's date>` line.
   - A hard rule: **return only the completed markdown section**, no commentary, no preamble, no summary — it will be pasted directly into `release-todo.md`.
3. **Receive the agent's section text.** Sanity-check it in the main context: does the score match the evidence in the rationale? Are the TODO items specific and actionable? Are the cited files real (spot-check one with Read)? On a re-run, also verify: did the score change? If yes, is the change flagged inline and explained? Did any "Needed before 1.0.0" item that was on the previous list get marked done/removed/refreshed? If anything looks off, dispatch a follow-up agent with a corrective prompt rather than silently accepting.
4. **Edit `release-todo.md`** to replace the group's section with the validated text (using `Edit` with the previous section as `old_string` and the new section as `new_string` — this preserves surrounding structure). Update the status header to show which groups are complete.
5. Move to the next group.

**Re-run optimization (optional, prefer correctness over speed):** If a re-run agent reports zero file-system changes within a group's owned paths since the last review (verified by spot-checking modification times or by the agent's own scan finding no new/removed/changed files), the section may be left as-is with only the `Last reviewed:` date refreshed. This shortcut is allowed only when **all** of the following hold: (a) no files added, removed, or substantially modified in the group's paths; (b) the group's score is not flagged for review by Step 3 of a previous run; (c) no cross-cutting blockers (Step 3) reference this group. When in doubt, do the full re-evaluation.

**Why each agent sees the current `release-todo.md`:** (user-specified) it gives each new agent the established calibration signal from prior groups, keeps naming/formatting consistent, and lets the agent cross-reference findings (e.g. "the Compiler Frontend already noted diagnostic gaps, and the same gaps show up here"). Without this, each agent scores in a vacuum. On a re-run this is doubly important — the agent uses the prior text as both the calibration anchor *and* the previous-state baseline for its own group.

**Calibration pass happens in Step 3**, not inside Step 2 — no re-scoring mid-review.

### Step 3 — Calibration & Overall Review
1. Re-read `release-todo.md` from top to bottom to reload the full picture in the main context.
2. **Calibration pass.** Compare scores across groups against the agreed anchors (1 = broken, 5 = usable but rough, 8 = production-ready, etc.). Look for inconsistencies — e.g. a 7/10 with rationale that reads more like a 5/10, or two groups with nearly identical maturity but different scores. Adjust scores where warranted and add a one-line note to that group's rationale explaining the adjustment (e.g. "Score revised from 7 to 6 during calibration — gap depth comparable to Simulator group"). On a re-run, calibration adjustments **chain on top of** previous run's adjustments — e.g. "Score revised from 7 to 6 during this run's calibration (was 8 → 7 in the prior run)".
3. Consider weighting: which groups are critical-path for 1.0.0? A weak compiler frontend matters more than a weak VS Code extension. State this weighting explicitly in the overall rationale.
4. Form an independent overall score 1–10 and write 3–6 sentences explaining exactly how the score was reached (which groups pulled it up, which pulled it down, what the top 3 blockers are, and what the weighting was). On a re-run, also state how the overall score has changed since the previous run and which groups drove the change.
5. **Replace** (don't append to) the **"Top 1.0.0 Blockers" consolidated list** at the top of the file — the 5–10 items that most matter for the overall score *as of this run*. Items that have been resolved since the last run are removed; items that newly become relevant are added; items that are still outstanding may stay (with a brief note if their characterization changed). Do not preserve a stale blocker list under any circumstances — the top section is the current snapshot.
6. **Edit** `release-todo.md` to add/replace the overall section, the calibration adjustments, and the consolidated blocker list. Update status header to "Review complete" with today's date and note "(re-run; previous review: <prior date>)" if applicable.

**Re-run sanity checks for Step 3:**
- Did any group's score change without an explicit `(was Y, ...)` note? If yes, that's a bug — go back and add the note.
- Did the overall score change in a direction inconsistent with the group changes? E.g., several groups went up but the overall went down, with no weighting change to explain it. If yes, re-derive the overall and explain the discrepancy.
- Did any blocker on the previous run's list get silently dropped without resolution evidence? If yes, restore it or explain why it was dropped.

## Critical Files

Only one file is written/modified by this task:
- `/Users/justinzaun/Development/jz-hdl-dev/release-todo.md` — created in Step 1, edited in every sub-part of Step 2, edited again in Step 3.

All other files are read-only references. No source changes. No commits.

## File Format for `release-todo.md`

```markdown
# jz-hdl-dev 1.0.0 Release Readiness

**Status:** <step status>
**Date:** <today's date>
**Previous review:** <prior date, only present after a re-run>
**Reviewer:** Claude (Opus 4.6)

## Re-run notes (only present after at least one re-run)
- <today's date>: <one-line summary of structural changes — group adds/removes/splits/merges, or "no structural changes">
- <prior date>: <prior summary>

## Exclusions
- <list>

## Top 1.0.0 Blockers (cross-cutting)
_(filled in Step 3 — replaced wholesale on each re-run)_

## Overall Score: X / 10
_(filled in Step 3, with rationale; on a re-run, includes a "Change since last run" line)_

---

## Groups

### 1. <Group name> — Score: X / 10
**Paths owned:** `<paths>`
**Criteria scored against:**
- <criterion 1>
- <criterion 2>
- ...
**Last reviewed:** <today's date or prior date if section unchanged this run>
**Rationale:**
<2–4 sentences. On a re-run with a score change, lead with the inline `Score: X / 10 (was Y, <reason>)` marker on the heading.>

**Key measurements:**
- <numbers — refreshed on every re-run>

**Needed before 1.0.0:**
- <item>
- <item>

**Surprising findings (optional):**
- <item>

---
<repeat for every group>
```

The header `Date:` and per-group `Last reviewed:` fields are the only freshness indicators in the file. They are always today's date for the current run; the previous run's date is only preserved in the header `Previous review:` field and in the per-group `Last reviewed:` field for sections the current run intentionally skipped.

## Verification

This task has no automated test. Verification = the user reads `release-todo.md` and sanity-checks the scores and TODO lists against their own mental model of the project. Specifically the user can check:

- Does the group list cover the whole project (minus exclusions)?
- Does each group's score match their intuition? Where it doesn't, the rationale should explain why.
- Are the TODO items actionable and specific (not "improve quality")?
- Does the overall score's rationale honestly reflect the group scores?

If anything is wrong, the user tells me what, and per CLAUDE.md I stop and review before reverting.

## Notes / Risks

- **Context pressure.** A 13-group deep review is a lot of reading. I'll rely on subagents to absorb file bodies and return summaries, then spot-verify critical files myself. If context gets tight I'll note it and the user can decide whether to shorten remaining groups.
- **Subagent accuracy.** Per the user's instruction, subagent summaries are a **starting point**, not the final word. I'll always read at least a few files directly before scoring.
- **Scoring calibration drift.** First groups may score differently than later ones. In Step 3 I'll re-read all scores and adjust if any are clearly miscalibrated relative to siblings.
- **Scope creep.** I will not fix anything I find, won't refactor, won't create side files (unless the user asks). Review only.
- **Idempotency / re-run risk.** The plan is idempotent by design, but the failure modes are different from a fresh run: (a) **silent score drift** — an agent updates a section with new measurements but doesn't notice the score should change. Step 2's hard rule is to always flag score changes inline; Step 3's sanity check verifies it. (b) **stale TODO items** — a previous run's "Needed before 1.0.0" item may have been silently completed without the agent noticing. Each re-run agent must walk every TODO and decide if it's still applicable. (c) **blocker list rot** — Step 3 must replace the top blocker list wholesale, not append to it. (d) **group reshuffling lossage** — if a group is split, merged, or removed in Step 1, the historical context (previous score, previous TODO list) must be preserved either by carrying it into the new group's section or by leaving the old section in place with a `Score: N/A — group removed` marker. Never silently delete prior content on a re-run.
- **Re-run vs. fresh run ambiguity.** Step 0 decides the mode based solely on whether `release-todo.md` exists. There is no other signal. If the user wants a forced fresh run, they should delete the file before running the plan; if they want a re-run, they should leave it in place. Do not try to second-guess the user by partially preserving state.
