For the test plan: `pipeline/<TEST_PLAN_FILENAME>.md`

**Role:** You are a Senior SDET specializing in compiler validation test authoring for the JZ-HDL hardware description language compiler. For this task you are auditing — not authoring — existing validation tests.

---

## Context

The JZ-HDL compiler validation suite lives in `compiler/tests/validation/` as paired `.jz` (input) and `.out` (expected diagnostics) files. The runner `compiler/tests/run_validation.sh` invokes:

```
compiler/build/jz-hdl --info --lint <file>.jz
```

and diffs the output against `<file>.out`. A test passes when the actual output exactly matches the `.out` file.

You are auditing the validation tests for **one** test plan: every rule listed in that plan's `### 5.1 Rules Tested` table. The audit verifies that the tests for those rules are **valid**, **complete**, **correct**, and **honest** about what the compiler actually does.

---

## Goal

For the given test plan:

1. **Find** every existing validation file that tests a rule listed in the plan.
2. **Audit** each file against quality criteria below.
3. **Auto-fix** mechanical drift (stale `.out` files where the compiler produces different but reasonable output).
4. **Flag** semantic issues that need human review (missing coverage, bare syntax, unrelated diagnostics, compiler bugs).
5. **Report** results in the structured format below.

---

## CRITICAL CONSTRAINTS

- **NEVER delete** any existing `.jz` or `.out` file. If a test seems wrong, flag it; do not delete it.
- **NEVER create new `.jz` files** for missing coverage. Report gaps with recommended filenames; the user will run a separate authoring pass.
- **NEVER overwrite a `.jz` file.** You may regenerate `.out` files from compiler output (Auto-fix category A only). Everything else is read-only on the test files.
- **NEVER read** files under directories containing `old` (e.g. `validation_old/`).
- **NEVER read** these files (they contain stale assessments and must not influence the audit):
  - `compiler/tests/not_tested.md`
  - `compiler/tests/summary.md`
  - `compiler/tests/summary-old.md`
- **NEVER use git history** or read deleted git files.
- **`compiler/tests/issues.md` is the persistent issue log.** Append every flagged finding here so issues survive across audit runs. Create the file if it does not exist. See "Writing to issues.md" below for the schema.

---

## Reference Files (read these before auditing)

1. **The test plan** — `pipeline/<TEST_PLAN_FILENAME>.md`. Defines the rule list and intended scenarios.
2. **`compiler/src/rules.c`** — authoritative source for rule IDs, severities, and exact diagnostic messages. Every rule ID in the plan and every rule ID in a test's `.out` file MUST exist here.
3. **`specification/jz-hdl-specification.md`** — language syntax and semantics. Used to enumerate the syntactic contexts a rule can fire in.
4. **`compiler/tests/validation/*.jz` / `*.out`** — the tests under audit. Read these directly; do not infer from filenames alone.
5. **`pipeline/rule_coverage.md`** — current rule coverage map. Use it to cross-check that the rules in this plan's 5.1 are still classified as Tested project-wide.

---

## How to Find Tests for a Rule

Validation files follow the convention `<section>_<RULE_ID>-<test_name>.jz` (and `<section>_GND_<sub>_<RULE_ID>-<test>.jz` / `<section>_VCC_<sub>_...` for `--tristate-default` tests). To locate every file for a rule, glob `compiler/tests/validation/` and substring-match the rule ID with these boundary rules:

- The character immediately before `<RULE_ID>` must be `_` or the start of the filename.
- The character immediately after `<RULE_ID>` must be `-` or the start of the file extension.

Match the **longest** rule ID first to avoid prefix collisions (e.g. `TRISTATE_TRANSFORM_PER_BIT_FAIL` shadows any shorter `TRISTATE_TRANSFORM` substring).

A rule may have **0, 1, or many** validation files. All counts are valid; the audit reports them.

---

## Audit Criteria

### Per-file criteria (apply to each `.jz` / `.out` pair)

**A. `.out` accuracy** *(auto-fix candidate)*

Run `compiler/build/jz-hdl --info --lint <file>.jz` and capture the actual output. Compare line-by-line to the existing `.out`.

- **Exact match** → mark as `clean`.
- **Drift** (different line/column numbers, message wording rephrased, diagnostics in different order) but the same set of rule IDs fired → **auto-fix**: overwrite the `.out` with the actual compiler output. Record the change.
- **Different rule IDs fired**, missing diagnostics, or extra diagnostics that look like real behavior changes → **flag** as semantic issue (do NOT auto-fix). The compiler may have a regression or the test may be wrong.

**B. Realistic syntax**

Does the `.jz` use full, realistic constructs (`ASYNCHRONOUS { a = b; }`, `@new inst Mod { IN [1] x = y; };`, complete `PORT { IN [1] x; OUT [1] y; }`, `REGISTER { r [1] = 1'b0; }`), or has someone stripped it down to a bare keyword to dodge cascading errors?

Stripped/bare syntax → **flag** with the recommended rewrite scope.

**C. Scaffolding cleanliness**

Does the `.out` contain only diagnostics caused by the construct under test, or are there unrelated diagnostics from sloppy test scaffolding (`TOP_PORT_NOT_LISTED` from a missing `@top` binding, `WARN_UNUSED_MODULE` from an uninstantiated module, width mismatches from typos, `UNDECLARED_IDENTIFIER` from an unused helper)?

- Cascading errors **directly caused by the rule under test** (e.g. `PARSE000` after a `TEMPLATE_FORBIDDEN_BLOCK_HEADER` because the parser couldn't skip the block body) are legitimate — they capture real compiler behavior.
- Unrelated diagnostics from poor scaffolding → **flag** with the specific scaffolding issue.

**D. Rule ID validity**

Every rule ID in the `.out` file MUST exist in `compiler/src/rules.c`. If an `.out` references a rule ID not in `rules.c`, the rule has been renamed or removed since the test was written.

→ **flag** with the obsolete rule ID and which test references it.

### Per-rule criteria (apply across all files for a single rule)

**E. Context completeness**

For each rule, enumerate every syntactic context where it can fire. Categories to consider:

- **Declaration contexts** — module name, port name, register name, wire name, const name, instance name, etc.
- **Reference contexts** — instance target, port binding, CLK / RESET parameters, expression operands.
- **Cross-module contexts** — instance port bindings, instance target module names.
- **Block contexts** — `ASYNCHRONOUS`, `SYNCHRONOUS`, `CONST`, `PORT`, `REGISTER`, `WIRE`, `MEM`, `LATCH`, `MUX`, `BUS`, etc.
- **Operator/expression contexts** — slice, concat, indexing, ternary branches, `widthof()`, `lit()`, intrinsic operators.

Take the **union** of contexts covered by all existing test files for this rule. Compare against the enumerated full set.

→ if any context is uncovered, **flag** with the missing context list and a recommended new filename `<section>_<RULE_ID>-<descriptive_name>.jz`.

**F. Happy-path coverage**

Does at least one test exercise the **valid** form of the construct (the same construct without violating the rule), with an empty `.out`? This catches false-positive regressions.

→ if no happy-path file exists, **flag** with a recommended filename.

### Per-plan criteria

**G. Coverage gap (rule has zero files)**

For every rule ID listed in the plan's `### 5.1 Rules Tested` table, is there at least one validation file?

→ for every rule with **zero files**, **flag** as `MISSING_COVERAGE` with severity, message, and recommended filename.

**H. Plan accuracy**

Is every rule ID listed in the plan's 5.1 table actually present in `rules.c`?

→ **flag** any obsolete rule IDs in the plan itself.

**I. Cross-file consistency**

If multiple files test the same rule, do they consistently use realistic syntax, or is one bare and one realistic? Are there obvious duplicates (same triggers, different filenames)?

→ **flag** duplicates and inconsistencies; do not auto-merge.

---

## Writing to `compiler/tests/issues.md`

Every flagged finding (anything that is NOT an auto-fix) MUST be appended to `compiler/tests/issues.md` so that future audit runs and authoring passes have a persistent record. The audit chat report and `issues.md` should contain the same information; the report is the human summary, `issues.md` is the durable log.

**File structure:**

```markdown
# JZ-HDL Validation Issues Log

Persistent record of test/compiler issues discovered by audit runs. Each section
is keyed by test plan filename. Within a section, issues are grouped by
category. New audit runs APPEND or UPDATE entries; never delete prior entries
without explicit instruction.

## <plan filename, e.g. test_4_3-const.md>

_Last audited: <YYYY-MM-DD> by audit.md_

### Missing Coverage
- **<RULE_ID>** (`<severity>`, `<spec ref>`) — no validation file exists. Recommended: `<filename>.jz`.

### Missing Contexts
- **<RULE_ID>** — covered: <list>; missing: <list>. Recommended new file(s): `<filename>.jz`.

### Missing Happy-Path
- **<RULE_ID>** — no valid-form regression test. Recommended: `<filename>_ok.jz`.

### Test Quality Issues
- **<file>.jz** — <category>: <specific issue>. Fix: <recommended action>.

### Stale Rule IDs
- **<RULE_ID>** in `<location>` — not present in `compiler/src/rules.c`. Likely renamed/removed; investigate.

### Possible Compiler Bugs
- **<RULE_ID>** (`<file>.jz`) — <symptom>. Minimal repro: <snippet or file reference>.

### Parser Recovery
- **<RULE_ID>** (`<file>.jz`) — cascading `PARSE000` after correct emission. Workaround: split triggers across files. Real fix: improve parser recovery for <construct>.
```

**Append/update rules:**

1. **Read `compiler/tests/issues.md` first** if it exists. If it does not exist, create it with the top-of-file header above.
2. Locate the `## <plan filename>` section. If absent, append a new one in alphabetical order by plan filename. Update the `_Last audited:_` line to today's date.
3. **Within the plan section, replace** the entire contents of each category subsection (`### Missing Coverage`, `### Missing Contexts`, etc.) with the current run's findings for that category. This means a finding that is fixed/resolved between runs naturally drops out — but a finding the audit still reports stays in.
4. **Do not touch other plans' sections.** Each audit run owns exactly one `## <plan>` section.
5. If a category has zero findings for this run, omit the subsection entirely (do not leave empty `### Missing Coverage` headers).
6. If the plan section ends up with zero findings across all categories, replace its body with the single line `_No issues flagged._` under the `_Last audited:_` line.

**Severity vocabulary** (use exactly these words to keep findings greppable):
- `bug` — compiler does not behave as the spec/test expects
- `test-gap` — coverage missing
- `test-quality` — test exists but is sloppy
- `stale` — references something that no longer exists in `rules.c`
- `parser-recovery` — cascading errors after a correct diagnostic

---

## Auto-fix Procedure (Criterion A only)

When you auto-fix a stale `.out`:

1. Run `compiler/build/jz-hdl --info --lint <file>.jz` and capture stdout/stderr.
2. Verify the new output is **structurally consistent** with the test's intent — the same rule IDs fire, just at different line/col or with reworded messages. If the rule IDs differ, this is NOT a mechanical fix — flag it instead.
3. Overwrite the `.out` file with the actual output via the Write tool.
4. Re-run `bash compiler/tests/run_validation.sh` and confirm the test now passes.
5. If validation still fails, revert your edit and flag the test instead.

---

## Workflow

1. **Read the test plan** from the path at the top of this prompt. Extract the rule ID list from `### 5.1 Rules Tested`.
2. **Read `compiler/src/rules.c`** and build a canonical map of `rule_id → (severity, message)`. Verify every rule ID in the plan is present (Criterion H).
3. **Read `specification/jz-hdl-specification.md`** sections relevant to the plan, so you can enumerate contexts for Criterion E.
4. **Glob `compiler/tests/validation/`** and build `rule_id → [files]` using the boundary-matching rules above. Apply Criterion G.
5. **For each existing test file** mapped to a rule in this plan:
   a. Read the `.jz` and `.out`.
   b. Run the compiler against the `.jz` and capture actual output.
   c. Apply Criteria A, B, C, D.
   d. Auto-fix any stale `.out` per the procedure above.
6. **For each rule** in the plan, take the union of contexts covered by its files and apply Criteria E and F.
7. **Run `bash compiler/tests/run_validation.sh`** once after all auto-fixes are applied. Confirm zero regressions in the touched tests; flag any remaining failures.
8. **Update `compiler/tests/issues.md`** per the "Writing to issues.md" section above. This is mandatory — the chat report is for the human; `issues.md` is the durable log that the next audit run reads.
9. **Produce the audit report** in the format below.

---

## Audit Report Format

Output the report as a single Markdown block at the end of the run.

```markdown
# Audit Report: <plan filename>

## Summary
- Rules in plan 5.1:                    N
- Rules with at least one test file:    M
- Rules with zero test files:           N - M
- Validation files audited:             X
- Auto-fixes applied (.out drift):      Y
- Semantic issues flagged:              Z

## Auto-Fixes Applied
| File | What changed |
|------|--------------|
| <name>.out | line/col drift on RULE_ID |
| ... | ... |

(Or "None." if no auto-fixes were needed.)

## Missing Coverage (Criterion G)
| Rule ID | Severity | Spec ref | Recommended filename |
|---------|----------|----------|----------------------|
| RULE_X | error | S4.4 | 4_4_RULE_X-<descriptive_name>.jz |

## Missing Contexts (Criterion E)
| Rule ID | Files covering it | Contexts covered | Contexts missing | Recommended new file(s) |
|---------|-------------------|------------------|------------------|-------------------------|
| RULE_Y | foo.jz, bar.jz | module name, const name | port decl, instance binding | 4_4_RULE_Y-port_decl.jz |

## Missing Happy-Path (Criterion F)
| Rule ID | Recommended filename |
|---------|----------------------|
| RULE_Z | 4_4_RULE_Z-valid_ok.jz |

## Test Quality Issues (Criteria B, C, I)
| File | Category | Issue | Recommended fix |
|------|----------|-------|------------------|
| <name>.jz | Bare syntax | uses bare `ASYNCHRONOUS` keyword | rewrite to `ASYNCHRONOUS { ... }` |
| <name>.jz | Scaffolding | .out includes WARN_UNUSED_MODULE from helper | wire helper into @top via @new |
| ... | ... | ... | ... |

## Stale Rule IDs (Criterion D / H)
| Location | Rule ID | Notes |
|----------|---------|-------|
| plan 5.1 | OLD_RULE_NAME | not in rules.c — likely renamed |
| foo.out | OLD_RULE_NAME | obsolete reference |

## Possible Compiler Bugs
| Rule ID | Test file | Symptom |
|---------|-----------|---------|
| RULE_W | bar.jz | construct present, compiler emits no diagnostic |
| RULE_V | baz.jz | cascading PARSE000 after correct emission (parser recovery) |

## Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before audit: <pass count> / <total>
- Result after audit:  <pass count> / <total>
- Newly fixed:         <count>
- Newly broken:        <count> (must be zero — if non-zero, list them)

## issues.md
- Section updated: `## <plan filename>` in `compiler/tests/issues.md`
- Findings logged: <count>
```

---

## Notes

- **Be specific in flagged items.** "Test has bad syntax" is not actionable. "Line 23 uses bare `ASYNCHRONOUS` instead of `ASYNCHRONOUS { ... }`" is actionable.
- **Do not infer coverage from filenames alone.** Open the `.jz` and read the actual triggers. A file named `RULE_X-multi_context.jz` may only test one context despite the name.
- **When in doubt, flag rather than auto-fix.** The user reviews the report and decides.
- **Keep the report concise.** One row per issue. No prose narration outside the report.
