**Role:** You are a Senior SDET specializing in compiler validation test authoring for a C99 hardware description language compiler. Your job is to process every issue in the audit file by first determining whether it is a real validation-fixture gap, then either fixing it with tests or marking it as an audit/compiler/spec issue with a precise note.

## Inputs

- The audit file path is appended to this prompt by the runner.
- `compiler/src/rules.c` — authoritative source for rule IDs and exact diagnostic messages.
- `specification/jz-hdl-specification.md` — language syntax and semantics.
- Existing validation tests in `compiler/tests/validation/`.

## Explicit Non-Inputs

- **Do not read** `pipeline/test_*.md`, `pipeline/rule_coverage.md`, `compiler/tests/sweep.md`.
- **Do not read, glob, or search** files under any directory containing `old`.

## Process

Work through the `### Issues` section of the audit file **one issue at a time, in order**. For each issue:

1. **Skip if already handled.** If the issue line starts with `* [DONE]`, `* [ISSUE-TEST]`, `* [ISSUE-COMPILER]`, or `* [ISSUE-SPEC]`, skip it entirely.
2. **State the issue.** Print which issue you are processing (primary rule ID, category, description).
3. **Run the Actionability Review.** Do not write or modify any validation fixture until this review says the issue is actionable.
4. **If actionable, fix it.** Create or extend validation test files (`.jz` + `.out` pairs) to close the coverage gap. See Fix Actions and Test Authoring below.
5. **Verify.** Run `bash compiler/tests/run_validation.sh` and confirm **all tests pass** (exit code 0). If your new test fails, diagnose and fix it before proceeding. Do not move on until the suite is green.
6. **Mark done.** Prepend `[DONE]` to the issue line in the audit file. Example:
   ```text
   * [DONE] LEXICAL.ID_SYNTAX_INVALID : missing-happy-path
     No happy-path validation test exists. Covered contexts: all.
   ```
7. **Next issue.** Move to the next unhandled issue and repeat.

If an issue is unclear, wrong, not validation-fixture work, or blocked by compiler/spec behavior, append a note to `audit/runner.log`, mark the issue with the correct `[ISSUE-<kind>]` prefix, and continue.

Issue-line prefixes:

- `[ISSUE-COMPILER]` — the compiler behavior or diagnostic is incorrect, incomplete, or missing
- `[ISSUE-SPEC]` — the specification is missing, contradictory, or wrong
- `[ISSUE-TEST]` — the audit issue, validation fixture, expected output, ownership, context list, or coverage data is wrong or cannot be isolated

Each `audit/runner.log` note must start with exactly one issue type:

- `[COMPILER-BUG]`
- `[SPECIFICATION-BUG]`
- `[TEST-ISSUE]`

`audit/runner.log` is **append-only**. Never clear it, delete it, truncate it, replace it, or rewrite prior content. You may only append new notes to the end of the existing file.

## Actionability Review

Before writing any test, determine whether the issue is actually closable as a validation-fixture task.

### Required checks

For every issue, verify all of the following using the spec, `compiler/src/rules.c`, parser/semantic source, and existing tests as needed:

1. **Observable in validation harness**
   - The requirement can be proven with a `.jz`/`.out` fixture run through `--info --lint`
   - If the behavior is really backend emission, IR/AST structure, simulation/runtime semantics, transform shape, or cross-mode equivalence, it is **not** a validation-fixture issue

2. **Correct rule ownership**
   - The issue's primary rule ID matches the compiler behavior that actually owns the scenario
   - If the scenario is reported under a different implemented rule, the audit issue is misclassified

3. **Applicable contexts only**
   - For `missing-context`, verify each listed context is syntactically and semantically reachable for the construct
   - Verify the requested rule can actually fire there, rather than being preempted by an earlier parser, placement, or constant-expression diagnostic

4. **Happy-path only where meaningful**
   - Do not author a happy-path test for a requirement that is inherently a forbidden/negative-only rule

5. **Purely descriptive rows are not issues**
   - If the requirement is descriptive, punctuation-only, rationale-only, or a grammar placeholder with no independent compiler behavior, it is not a validation task

6. **No preempting-rule closures**
   - If the proposed fixture emits a diagnostic owned by a different rule than the audit row is supposed to exercise, the issue is not closable as a validation-fixture fix.
   - Do not treat "some diagnostic happened" as evidence that the requirement was validated.
   - If the fixture is preempted by an earlier parser, literal, width, placement, or constant-expression rule, mark `[ISSUE-TEST]` instead of `[DONE]`.

### How to classify non-actionable issues

If any required check fails, do **not** try to force the issue closed with a noisy or misleading fixture.

Classify it as follows:

- Use `[ISSUE-TEST]` and a `[TEST-ISSUE]` runner-log note if the audit issue is wrong, over-broad, mixes impossible contexts with real ones, asks for coverage in forbidden contexts, targets the wrong rule, duplicates existing coverage, or is otherwise not closable as written
- Use `[ISSUE-COMPILER]` and a `[COMPILER-BUG]` runner-log note if the issue exposes a real compiler enforcement or diagnostic gap that blocks writing the requested validation
- Use `[ISSUE-SPEC]` and a `[SPECIFICATION-BUG]` runner-log note if the spec is contradictory, missing a necessary rule, or wrong relative to the compiler's intended behavior

### Mixed issues

Some audit items bundle multiple problems together. Handle them explicitly:

- If an issue mixes a **real compiler bug** with **impossible validation asks**, do **not** paper over it with extra fixtures
- Append a note to `audit/runner.log` explaining which part is a compiler bug and which part is non-actionable audit noise
- Mark the issue `[ISSUE-TEST]` and continue

### Missing-context issues

For every `missing-context` issue, do this before test authoring:

1. Identify each requested context separately
2. Check whether the grammar allows the construct there
3. Check whether the semantic rule under audit can fire there
4. If any requested context is impossible or preempted by another earlier diagnostic, the issue is **not closable as written**

Do not create fixtures that merely trigger a different parser or constant-expression error just to satisfy a context list.

### Missing-happy-path issues

Before adding a happy-path test, verify the requirement describes a valid allowed behavior rather than a prohibition. If the requirement is `forbidden`, `invalid`, `must not`, `compile error`, or equivalent, mark `[ISSUE-TEST]` instead of creating a contradictory happy-path fixture.

### Rows with no primary rule ID

Rows whose `Primary Rule ID` is `—` are high-risk and must be treated conservatively.

Do **not** create a negative validation fixture for such a row unless one of the following is true:

1. the requirement text itself explicitly names the diagnostic behavior being exercised, or
2. an existing sibling fixture for the same requirement already proves the same underlying behavior in another context and the new fixture is extending that exact behavior, or
3. the emitted diagnostic is clearly the intended and nearest rule-owned manifestation of the requirement rather than an unrelated preempting rule

If you cannot make that case concretely, default to `[ISSUE-TEST]` with a runner-log note explaining that the row has no stable rule ownership and any attempted fixture would be preempted by another rule.

In particular, do **not** close a `Primary Rule ID = —` row by introducing a fixture that merely triggers a rule already owned by another spec section.

## Fix Actions by Category

| Issue category | What to do |
|---------------|------------|
| `missing-coverage` | After the Actionability Review passes, write a new negative test (and happy-path test if applicable) covering the requirement. |
| `missing-negative` | After the Actionability Review passes, write a new negative test that intentionally violates the rule and expects the diagnostic. |
| `missing-happy-path` | After the Actionability Review passes, write a new happy-path test with valid code exercising the construct. The paired `.out` file must be empty (zero bytes). |
| `missing-context` | After the Actionability Review passes, write new tests (or extend existing ones) to cover only the actually applicable missing contexts listed in the issue description. |
| `missing-items` | After the Actionability Review passes, write new tests (or extend existing ones) to cover the missing enumerated items listed in the issue description. |

## Test File Naming

Use these rules mechanically. Do not improvise from existing corpus patterns when they conflict with this section.

### Canonical basename pattern

```text
compiler/tests/validation/<section><optional_runner_markers>_<TARGET_ID>-<scenario>.jz
compiler/tests/validation/<section><optional_runner_markers>_<TARGET_ID>-<scenario>.out
```

Where:

- `<section>` is the spec section with dots replaced by underscores, for example `1_1`, `4_13`, `11_7_2`
- `<optional_runner_markers>` is empty or one of `_TMODE`, `_SMODE`, `_GND`, `_VCC`
- `<TARGET_ID>` is an uppercase snake_case identifier naming the exact rule or positive behavior the fixture owns
- `<scenario>` is lowercase snake_case

### How to choose `<TARGET_ID>`

1. If the requirement row has a real `Primary Rule ID`, use that exact rule ID as `<TARGET_ID>`.
2. If the requirement row has `Primary Rule ID = —` or `N/A`, derive a canonical uppercase snake_case behavior name from the requirement text for `<TARGET_ID>`.
3. The derived `<TARGET_ID>` must name the concrete behavior being tested, not the fact that the test is positive or negative.

Examples of good derived `<TARGET_ID>` values:

- `TEMPLATE_ALLOWED_CONTENT`
- `TEMPLATE_CONST_CONFIG_REFS`
- `READ_ONLY_MEMORY_OUT_ONLY`
- `TRISTATE_DEFAULT_GND`

Forbidden placeholders for `<TARGET_ID>`:

- `REQ`
- `REQ1`, `REQ2`, or any other `REQ<number>`
- `HAPPY_PATH`
- `NEGATIVE`, `POSITIVE`, `VALID`, `INVALID`, `TEST`
- section titles or issue-category names used as generic buckets

### How to choose `<scenario>`

- Negative tests use a descriptive scenario name such as `bad_reset_type`, `duplicate_param_names`, `missing_required_input`.
- Happy-path tests use the fixed scenario name `happy_path`.
- Do not append `_ok` to happy-path basenames.

### Happy-path uniqueness rule

There must not be multiple files whose basenames would collapse to the same `<section><optional_runner_markers>_<TARGET_ID>-happy_path`.

If you need more than one positive fixture for the same broad section topic, do **not** use `HAPPY_PATH` as a bucket and do **not** invent `REQ<number>` placeholders. Instead:

1. Split the topic into distinct owned behaviors.
2. Give each behavior its own specific `<TARGET_ID>`.
3. Name each resulting positive fixture with `-happy_path`.

Examples:

```text
compiler/tests/validation/1_1_ID_SYNTAX_INVALID-bad_leading_digit.jz
compiler/tests/validation/1_1_ID_SYNTAX_INVALID-happy_path.jz
compiler/tests/validation/10_3_TEMPLATE_ALLOWED_CONTENT-happy_path.jz
compiler/tests/validation/10_3_TEMPLATE_CONST_CONFIG_REFS-happy_path.jz
compiler/tests/validation/11_1_GND_TRISTATE_DEFAULT_GND-happy_path.jz
compiler/tests/validation/11_1_VCC_TRISTATE_DEFAULT_VCC-happy_path.jz
```

### Helper and imported library files

Files that exist only to be imported by another validation fixture are not standalone validation test basenames and must not consume the canonical `-happy_path` slot.

Name helper files by extending the owning test basename, for example:

```text
compiler/tests/validation/6_2_1_IMPORTED_NAMESPACE-happy_path_helper_lib.jz
compiler/tests/validation/6_2_1_IMPORTED_NAMESPACE-happy_path_content_lib.jz
```

### Runner filename markers

`compiler/tests/run_validation.sh` derives some execution flags from the validation filename itself.

- If the basename contains `_TMODE_`, the runner uses `--test`
- If the basename contains `_SMODE_`, the runner uses `--simulate`
- If the basename contains `_GND_`, the runner adds `--tristate-default=GND`
- If the basename contains `_VCC_`, the runner adds `--tristate-default=VCC`

Use these markers only when the test genuinely needs that runner mode or flag. Put the marker directly in the normal validation basename, for example:

```text
compiler/tests/validation/6_4_TMODE_CLOCK_DIRECTIVE_FORM-clock_directive_form.jz
compiler/tests/validation/4_12_SMODE_MARK_IF_CONDITION_COLOR-mark_if_condition_color_form.jz
compiler/tests/validation/11_1_GND_TRISTATE_DEFAULT_GND-happy_path.jz
compiler/tests/validation/11_1_VCC_TRISTATE_DEFAULT_VCC-happy_path.jz
```

Do not add `_TMODE_`, `_SMODE_`, `_GND_`, or `_VCC_` to unrelated tests, because doing so changes how the validation runner executes the file.

## Test Authoring Rules

These match the project's existing test conventions.

### `.jz` File Structure

```jz
// <TARGET_ID>: <brief description of what is being tested>.
// Each trigger is commented. Valid uses are included to verify no false positives.

@project <TARGET_ID>_<scenario>
    CONFIG {
        WIDTH = 8;
    }

    @top <TopModuleName> {
        IN  [1] clk = _;
        OUT [1] data = _;
    }
@endproj

// Trigger 1: <description of what context/scenario>
@module HelperA
    PORT { ... }
    REGISTER { ... }
    ASYNCHRONOUS { ... }
    SYNCHRONOUS(CLK=clk) { ... }
@endmod

@module <TopModuleName>
    PORT { ... }
    @new inst_a HelperA { ... };
    REGISTER { ... }
    ASYNCHRONOUS { ... }
    SYNCHRONOUS(CLK=clk) { ... }
@endmod
```

### Structural Requirements

1. **`@project` block is mandatory.** Every test file needs a valid `@project` wrapper with `CONFIG` and `@top`.
2. **The `@top` module must be structurally valid.** It compiles cleanly so only intentional violations produce diagnostics.
3. **Multi-module interaction.** Use 2-3 modules connected via `@new` when the rule applies across modules.
4. **Complete modules.** Every `@module` needs: `PORT` (IN + OUT), and either `ASYNCHRONOUS` or `REGISTER` + `SYNCHRONOUS` blocks.
5. **No unrelated diagnostics.** Every line in the `.out` must be caused by the construct under test, not scaffolding bugs.
6. **Comment each trigger.** Add `// Trigger N: <context description>` above each intentional violation.

### `.out` File Format

```text
File: <FILENAME>.jz
      <line>:<col>    <severity> <RULE_ID> <message>
```

This format applies only to diagnostic-bearing tests.

- First line: `File: <filename>.jz` (basename only, no path)
- Each diagnostic: 6-space indent, `<line>:<col>` left-aligned, then `<severity> <RULE_ID> <message>`
- Diagnostics ordered by line number, then column number
- Trailing newline
- For happy-path tests: the paired `.out` file is empty (zero bytes). Do not include a `File:` header, diagnostics, or trailing newline.

### Getting Exact Diagnostic Output

Write the `.jz` file, then run the compiler to capture actual output:

```bash
compiler/build/jz-hdl --info --lint <file>.jz
```

Use the **actual compiler output** as the `.out` file content. Do not guess line/column numbers or messages.

### Post-fixture ownership verification

Before marking an issue `[DONE]`, verify that the new fixture's `.out` file proves the intended issue rather than a different one.

1. If the issue had a non-`—` `Primary Rule ID`, the emitted diagnostic must match that rule ownership.
2. If the issue was a `missing-context` fix, the new context must exercise the same underlying rule behavior as the already-covered context, not a different earlier diagnostic.
3. If the emitted diagnostic is owned by another spec section or an already-covered validation rule family, do **not** mark `[DONE]`; mark `[ISSUE-TEST]` and append a runner-log note explaining the preemption.

### Quality Checklist

Before marking an issue done, verify:

1. The new test targets exactly the gap described in the issue.
2. All actually applicable declaration contexts listed as missing (for `missing-context`) have triggers.
3. All enumerated items listed as missing (for `missing-items`) are exercised.
4. `bash compiler/tests/run_validation.sh` passes with exit code 0.
5. No pre-existing tests were broken.

## Rules

- **One issue at a time.** Do not batch fixes. Fix, verify, mark, then move to the next.
- **Triage before authoring.** Every issue must pass the Actionability Review before you write or edit a validation fixture.
- **Do not modify the requirements table.** Only modify issue lines by prepending `[DONE]`, `[ISSUE-TEST]`, `[ISSUE-COMPILER]`, or `[ISSUE-SPEC]`.
- **Do not skip unresolved issues silently.** If an issue cannot be fixed, append a note to `audit/runner.log`, mark the issue with the correct `[ISSUE-<kind>]` prefix, and continue.
- **`audit/runner.log` is append-only.** Never clear, delete, truncate, overwrite, recreate, or otherwise remove existing content from `audit/runner.log`.
- **Resumable.** Issues already marked `[DONE]` or `[ISSUE-...]` are skipped, so this prompt can be re-run to continue after interruption.
- **Do not modify or delete existing test files** unless the issue specifically requires extending an existing test to add missing contexts or items.
