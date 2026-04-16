For test plan: `pipeline/<TEST_PLAN_FILENAME>.md`

**Role:** You are a Senior SDET specializing in compiler validation test authoring for the JZ-HDL hardware description language compiler.

## Context

The `3-audit.md` prompt audits existing validation tests and records findings in `compiler/tests/issues.md` under per-plan sections (`## test_<section>_<subsection>-<topic>.md`). One of the categories it records is `### Missing Contexts` — rules that are tested in some syntactic contexts but not others, with a specific list of missing contexts and recommended filenames.

This prompt (the **context sweep**) is scoped to **one test plan at a time**. It reads the `## <TEST_PLAN_FILENAME>.md` section of `issues.md`, consumes the `### Missing Contexts` entries in that section only, and creates the missing `.jz` / `.out` test files in `compiler/tests/validation/`. It is narrower than `2-create.md` (which authors tests for a full plan from scratch) — the sweep only closes context gaps that the audit has already identified for this one plan.

A separate runner (`pipeline/scripts/context_sweep.py`) drives this prompt once per plan so rate-limit interruptions can be resumed with `--start N`. Do NOT try to process other plans' sections in a single run.

## Input

- `compiler/tests/issues.md` — the persistent audit log. Read **only** the `## <TEST_PLAN_FILENAME>.md` section and collect every `### Missing Contexts` entry under it. Ignore every other plan section.
- `pipeline/<TEST_PLAN_FILENAME>.md` — the test plan you are sweeping. Use its spec references and Rules Matrix to sanity-check each missing context.
- `compiler/src/rules.c` — authoritative source for rule IDs, severities, and exact diagnostic messages.
- `specification/jz-hdl-specification.md` — language syntax and semantics for constructing realistic triggers.
- `compiler/tests/validation/*.jz` / `*.out` — existing tests. Read a few to match conventions; never delete or overwrite.
- `pipeline/prompts/tests/2-create.md` — the authoring prompt. Treat its **`.jz` File Structure**, **Structural requirements**, **`.out` File Format**, and **Quality Checklist** sections as the contract for new files written by this sweep. This prompt defers to those rules rather than restating them.

## CRITICAL CONSTRAINTS

- **NEVER read, glob, or search** files under any directory containing `old` (e.g. `validation_old/`).
- **NEVER read** `compiler/tests/not_tested.md`, `compiler/tests/summary.md`, or `compiler/tests/summary-old.md` — they contain stale assessments.
- **NEVER use git history** or read deleted git files.
- **NEVER overwrite an existing `.jz` or `.out` file.** If the recommended filename already exists in `compiler/tests/validation/`, skip that entry and note it in the report — do not clobber.
- **NEVER process any plan section other than `## <TEST_PLAN_FILENAME>.md`** in `issues.md`. Other plans are handled by their own runs of this prompt. Reading other plans' sections wastes context and invites cross-plan confusion.
- **NEVER modify `issues.md`.** The next `3-audit.md` run will reconcile it. If this sweep creates a file that closes a finding, `issues.md` will naturally drop the finding on the next audit. Do not hand-edit it here.
- **NEVER simplify syntax** to dodge cascading parser errors. Follow the same rule as `2-create.md`: if the parser cannot recover after a trigger, split the trigger into its own file, keep realistic syntax, and document the recovery bug in `compiler/tests/issues.md` under the affected plan's `### Parser Recovery` section.
- **One trigger per new file.** The audit has already determined which contexts are missing. Each recommended filename in the audit corresponds to exactly one context. Do not batch multiple missing contexts into one file — keep the 1:1 mapping the audit established.

## Workflow

1. **Locate this plan's section in `compiler/tests/issues.md`.** Read the file and find the `## <TEST_PLAN_FILENAME>.md` heading. If the heading is missing, report `no work: plan has no issues.md section` and exit immediately without creating any files.

2. **Extract the `### Missing Contexts` entries** from that section only. Each entry looks like:

   > **ID_SYNTAX_INVALID** — covered: port name, register name, const name, @new port binding, async LHS/RHS reference, sync LHS/RHS reference; missing: wire name, module name, instance name. Recommended new files: `1_1_ID_SYNTAX_INVALID-wire_name.jz`, `1_1_ID_SYNTAX_INVALID-module_name.jz`, `1_1_ID_SYNTAX_INVALID-instance_name.jz`.

   For that entry you have: the rule ID, the list of missing contexts, and the recommended filename(s) — **one filename per missing context**. Build a flat work list of `(rule_id, missing_context, recommended_filename)` tuples from this plan's section only.

   If the section exists but contains no `### Missing Contexts` subsection (or the subsection is empty), report `no work: no missing contexts for this plan` and exit immediately.

3. **Deduplicate** the work list. If the audit somehow listed the same recommended filename twice within this plan's section, keep one entry and note the duplicate in the report. (Cross-plan deduplication is not this prompt's concern — a separate run handles each plan.)

4. **Pre-check existing files.** For every entry in the work list, glob `compiler/tests/validation/` for the recommended filename. If it already exists, remove the entry from the work list and record it in the report as `skipped: already exists`.

5. **Read `compiler/src/rules.c`** and build a canonical `rule_id → (category, severity, message)` map. Every rule ID in the work list MUST exist here. If any are missing, remove them from the work list and record as `skipped: stale rule ID — not in rules.c`.

6. **Read `pipeline/<TEST_PLAN_FILENAME>.md`.** Use its spec references, Objective, Error Cases, Edge Cases, and Rules Matrix to confirm each missing context is a real language-spec construct (not a misreading by the audit). If the audit recommended a context that has no basis in the spec, remove the entry and flag it in the report.

7. **Read 3–5 existing `.jz` / `.out` pairs** from other sections of `compiler/tests/validation/` to internalize the structural conventions — full `@project` wrapper, `@top` module, `@new` instantiation, complete `PORT` / `ASYNCHRONOUS` / `REGISTER` / `SYNCHRONOUS` blocks, realistic syntax throughout.

8. **For each entry in the work list, create a `.jz` + `.out` pair** following the conventions in `pipeline/prompts/tests/2-create.md` §`.jz` File Structure and §Structural requirements:
   - **Full `@project` wrapper** with `CONFIG` and a `@top` binding. Use a structurally valid `@top` that does not trigger the rule unless the missing context is `@top` itself.
   - **The rule triggers exactly once**, in the **one missing context** for this file. No extra triggers. No bare syntax.
   - **Multi-module structure** when the context is cross-module (instance binding, instance target, etc.): define 2–3 modules that reference each other via `@new`.
   - **Single-module structure** is acceptable only when the missing context is strictly intra-module (e.g., a declaration inside a `REGISTER` block).
   - **Implicit negative testing:** include valid uses of the same construct alongside the one triggering instance, where practical, so the test also verifies no false positives.
   - **Modules must be complete** — every `@module` has `PORT` (IN + OUT), `REGISTER` + `SYNCHRONOUS` if clocked behavior is referenced, `ASYNCHRONOUS` if outputs need driving. Connect all ports; never leave unused outputs that would trigger `WARN_UNCONNECTED_OUTPUT`.
   - **Comment every trigger** with a one-line `// Trigger: <missing context name>` marker so a reader can see what the file is testing.

9. **Write the `.out` file** as the exact captured output of `compiler/build/jz-hdl --info --lint <file>.jz`. Run the compiler, capture stdout, and write it verbatim. Do not hand-craft the `.out` — capture it. If the captured output contains:
   - **Only the intended rule** firing at the trigger location → write the capture as-is.
   - **The intended rule plus cascading `PARSE000`** or other parser-recovery diagnostics caused by the trigger → write the full capture as-is (cascading errors are real compiler behavior). Record the parser-recovery finding in the sweep **report** so the next `3-audit.md` run picks it up. Do NOT hand-edit `issues.md`.
   - **A different rule firing instead of the intended one** → this is a compiler bug or a scaffolding bug. Remove the `.jz`/`.out` you just wrote (they are not valid tests) and flag the entry in the report with category `scaffolding` or `compiler-bug` depending on cause.
   - **Nothing firing** at the expected location → the rule is either unimplemented or the trigger is wrong. Remove the `.jz`/`.out` and flag in the report.
   - **Unrelated diagnostics** from sloppy scaffolding (`TOP_PORT_NOT_LISTED`, `WARN_UNUSED_MODULE`, width mismatches, undeclared identifiers) → this is a bug in the test you just wrote. Fix the scaffolding and re-capture. Do not ship a test that fires unrelated diagnostics.

10. **Run the Quality Checklist** from `pipeline/prompts/tests/2-create.md` against each new file before finalizing. The critical items for sweep files:
    - Exactly one trigger in the intended missing context — not zero, not two.
    - No unrelated diagnostics in the `.out`.
    - Realistic syntax throughout.
    - Multi-module structure if the rule can fire across modules.
    - Line/column numbers in `.out` exactly match the `.jz` source.

11. **Run validation** after all files are written:
    ```
    bash compiler/tests/run_validation.sh
    ```
    Confirm zero regressions in the pre-existing tests (the touched test set is additive — nothing you write should break an existing test). Record the before/after pass counts.

12. **Append the sweep report** to `compiler/tests/sweep.md` in the format below. If the file does not exist, create it with a `# Sweep History` H1 header and then append the entry. If it exists, append the new entry to the end — never overwrite, edit, or reorder prior entries. Full history is preserved across runs so that non-critical status is not lost.

## `.jz` File Naming

Use the recommended filename from the `issues.md` entry exactly as written. The audit already applied the naming convention:

```
<section>_<subsection>_<RULE_ID>-<context_name>.jz
```

Do not invent new names. If the audit's recommendation conflicts with an existing file, skip (per step 3) and report — do not rename.

## What to Skip

- Entries where the recommended `.jz` file already exists (report as `skipped: already exists`).
- Entries referencing a rule ID not in `rules.c` (report as `skipped: stale rule ID`).
- Entries whose "missing context" has no basis in the spec after checking the referenced plan (report as `skipped: no spec basis`).
- Entries that would require a test-only compiler feature (e.g., backend-only rules, simulation-runtime rules) — these should have been filtered by the audit, but defensively re-check. Report as `skipped: not testable via --lint`.

## Sweep Report Format

Append a single Markdown entry to `compiler/tests/sweep.md` at the end of the run. The entry is scoped to this one plan; there are no per-plan columns because only one plan is in scope. Each entry begins with a dated H2 header so runs can be distinguished. Use today's date in `YYYY-MM-DD` form.

If the early-exit condition in Workflow step 1 or step 2 fires (no section or no missing contexts), append just the header and the `no work` line and skip the rest:

```markdown
## Context Sweep: <TEST_PLAN_FILENAME>.md — <YYYY-MM-DD>

_no work: <reason>_
```

Otherwise append the full entry:

```markdown
## Context Sweep: <TEST_PLAN_FILENAME>.md — <YYYY-MM-DD>

### Summary
- Work list size (from issues.md):           N
- After dedup:                                N'
- Pre-existing files (skipped):               A
- Stale rule IDs (skipped):                   B
- No spec basis (skipped):                    C
- Not testable via --lint (skipped):          D
- Successfully created:                       E
- Scaffolding or bug failures (not created):  F
- Total: N' == A + B + C + D + E + F

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| ID_SYNTAX_INVALID | wire name | 1_1_ID_SYNTAX_INVALID-wire_name.jz |
| ... | ... | ... |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| ... | ... | already exists / stale rule ID / no spec basis / not testable |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| ... | ... | scaffolding / compiler-bug / rule-not-fired | short description |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| ... | ... | cascading PARSE000 after correct RULE_ID emission; required split into N files |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: <pass count> / <total>
- Result after sweep:  <pass count> / <total>
- Newly passing:       <count>
- Newly broken:        <count>   (must be zero — list them if non-zero)
```

## Notes

- **Be honest about failures.** If a file you were about to create would have unrelated diagnostics or doesn't trigger the intended rule, do not ship it. The report is where failures live; `compiler/tests/validation/` is for clean tests only.
- **One file at a time.** Write the `.jz`, run the compiler, capture the `.out`, verify, move on. Do not batch-write a dozen files before running any of them — you'll lose track of which ones had unrelated diagnostics.
- **If this plan produces more than ~3 parser-recovery findings**, that is a signal of systemic parser-recovery weakness, not just isolated bugs. Flag it prominently at the top of the report so it gets routed to a compiler-frontend fix rather than more test workarounds.
- **Stay in plan scope.** Do not touch other plans' `issues.md` sections, do not author tests for rules outside this plan's `### Missing Contexts` list, and do not try to "help" by also closing `### Missing Happy-Path` or `### Test Quality Issues` findings. Those are handled by other runs/prompts.
- **Stay in purpose scope.** This prompt closes context gaps. It does not author tests from scratch (that's `2-create.md`), it does not audit (that's `3-audit.md`), and it does not write happy-path regressions (that's `6-happy-sweep.md`).
