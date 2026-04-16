For test plan: `pipeline/<TEST_PLAN_FILENAME>.md`

**Role:** You are a Senior SDET specializing in compiler validation test authoring for the JZ-HDL hardware description language compiler.

## Context

The `3-audit.md` prompt audits existing validation tests and records findings in `compiler/tests/issues.md` under per-plan sections (`## test_<section>_<subsection>-<topic>.md`). One of the categories it records is `### Missing Happy-Path` — rules that have error-case tests but no dedicated "valid form" regression test, along with recommended filenames.

This prompt (the **happy-path sweep**) is scoped to **one test plan at a time**. It reads the `## <TEST_PLAN_FILENAME>.md` section of `issues.md`, consumes the `### Missing Happy-Path` entries in that section only, and creates the missing `.jz` / `.out` pairs in `compiler/tests/validation/`. Happy-path tests use an **empty** `.out` file (just the `File:` header, no diagnostics) — they verify that valid code compiles cleanly without false positives.

This prompt is narrower than `2-create.md` and parallel in structure to `4-context-sweep.md`. It does only one thing: close happy-path gaps flagged by the audit for this plan.

A separate runner (`pipeline/scripts/happy_sweep.py`) drives this prompt once per plan so rate-limit interruptions can be resumed with `--start N`. Do NOT try to process other plans' sections in a single run.

## Input

- `compiler/tests/issues.md` — the persistent audit log. Read **only** the `## <TEST_PLAN_FILENAME>.md` section and collect every `### Missing Happy-Path` entry under it. Ignore every other plan section.
- `compiler/src/rules.c` — authoritative source for rule IDs, severities, and exact diagnostic messages. Used to verify the rule exists and to understand what scenarios should NOT fire it.
- `specification/jz-hdl-specification.md` — language syntax and semantics for constructing valid, realistic code that exercises the rule's subject construct without tripping it.
- `pipeline/<TEST_PLAN_FILENAME>.md` — the test plan you are sweeping. Cross-check the rule's scope and any Happy-Path scenarios the plan lists.
- `compiler/tests/validation/*.jz` / `*.out` — existing tests. Read the corresponding error-case test for each rule (it exists — that's why the rule is in 5.1 Rules Tested) to understand what the rule forbids, then construct a file that uses the same constructs without violating the rule.
- `pipeline/prompts/tests/2-create.md` — the authoring prompt. Its **`.jz` File Structure**, **Structural requirements**, and **Happy-Path Tests** sections are the contract for new files written by this sweep. This prompt defers to those rules rather than restating them.

## CRITICAL CONSTRAINTS

- **NEVER read, glob, or search** files under any directory containing `old` (e.g. `validation_old/`).
- **NEVER read** `compiler/tests/not_tested.md`, `compiler/tests/summary.md`, or `compiler/tests/summary-old.md` — they contain stale assessments.
- **NEVER use git history** or read deleted git files.
- **NEVER overwrite an existing `.jz` or `.out` file.** If the recommended filename already exists in `compiler/tests/validation/`, skip that entry and note it in the report.
- **NEVER process any plan section other than `## <TEST_PLAN_FILENAME>.md`** in `issues.md`. Other plans are handled by their own runs of this prompt. Reading other plans' sections wastes context and invites cross-plan confusion.
- **NEVER modify `issues.md`.** The next `3-audit.md` run will reconcile it. The sweep creates files; the next audit removes the resolved finding.
- **NEVER emit any diagnostic** in a happy-path test. A happy-path `.jz` that produces ANY output other than the `File:` header line is a failed test — fix it or do not ship it.
- **Use full, realistic syntax.** Happy-path tests are regression protection. If you simplify the construct to avoid tripping the rule, you are not testing the real-world case. A happy-path test must look like production JZ-HDL code that a user would write.

## What a Happy-Path Test Is

A happy-path test exercises the **exact construct** the rule governs, but uses it in its **valid form** so the rule does not fire. Examples:

- For a rule like `ID_SINGLE_UNDERSCORE` (which forbids `_` as an identifier), the happy-path test uses `_` only in valid no-connect contexts (e.g., `IN [1] clk = _` in a `@new` binding) and never as a declared identifier.
- For `KEYWORD_AS_IDENTIFIER` (which forbids reserved words as identifiers), the happy-path test uses reserved words only in their keyword role and uses normal identifiers for names.
- For `DIRECTIVE_INVALID_CONTEXT` (which forbids certain `@`-directives inside blocks), the happy-path test places every `@`-directive in a legal context.

The test must actually exercise the construct — a happy-path file that does not contain the subject construct at all is not a happy-path test, it is a pointless compile. Include enough of the construct's valid uses that a future compiler regression (making the rule fire on valid code) would be caught.

## Input/Output for Happy-Path Tests

- **`.jz` file:** valid JZ-HDL. Full `@project` wrapper, valid `@top`, one or more complete `@module` definitions exercising the construct in valid ways. Multi-module via `@new` is preferred but not strictly required (unlike error-case tests) — happy-path for a purely intra-module construct can be a single complete module.
- **`.out` file:** exactly one line, followed by a trailing newline:
  ```
  File: <FILENAME>.jz
  ```
  No diagnostics, no `info`/`warning`/`error` lines, no spaces other than between `File:` and the filename. The file must end with a newline.

## Workflow

1. **Locate this plan's section in `compiler/tests/issues.md`.** Read the file and find the `## <TEST_PLAN_FILENAME>.md` heading. If the heading is missing, report `no work: plan has no issues.md section` and exit immediately without creating any files. Extract every entry under `### Missing Happy-Path` in that section only. An entry looks like:

   > **ID_SINGLE_UNDERSCORE** — no dedicated happy-path file (valid `_` uses exist inline in the error test, but no separate `_ok.jz`). Recommended: `1_1_ID_SINGLE_UNDERSCORE-valid_no_connect_ok.jz`.

   Build a flat work list of `(rule_id, recommended_filename)` tuples from this plan's section only. If the section exists but contains no `### Missing Happy-Path` subsection (or it is empty), report `no work: no missing happy-path for this plan` and exit immediately.

2. **Deduplicate** the work list. If the audit listed the same recommended filename twice within this plan's section, keep one entry and note the duplicate. (Cross-plan deduplication is not this prompt's concern — a separate run handles each plan.)

3. **Pre-check existing files.** For every entry, glob `compiler/tests/validation/` for the recommended filename. If it already exists, remove the entry from the work list and record it as `skipped: already exists`.

4. **Read `compiler/src/rules.c`** once and verify every rule ID in the work list exists. Remove stale entries and record them as `skipped: stale rule ID`.

5. **Read the corresponding error-case test** for each rule in the work list. The error test shows what the rule forbids; the happy-path test must exercise the same construct without forbidden uses. If no error-case test exists for a rule (which would mean the rule has no coverage at all), remove the entry and record it as `skipped: no error test — needs 5-context-sweep or full author first`.

6. **Read the referenced test plan** and the relevant spec section. Note any valid-form scenarios the plan explicitly lists — those are the canonical happy-path shapes to use.

7. **Read 3–5 existing `_ok.jz` files** from other sections of `compiler/tests/validation/` to match naming and structural conventions. If none exist (this is the first happy-path sweep), read 3–5 error-case tests and construct the happy-path analog yourself.

8. **For each entry, write the `.jz` file** following `pipeline/prompts/tests/2-create.md` §Happy-Path Tests:
   - **Full `@project` wrapper** with `CONFIG` and `@top`.
   - **Valid uses of the construct under test.** For each forbidden context in the error-case test, write a valid-context analog in the happy-path test. Include comments naming each valid use so a reader can see what is being protected.
   - **Complete modules.** Every `@module` has `PORT` (IN + OUT) and either `ASYNCHRONOUS` or `REGISTER` + `SYNCHRONOUS`. All ports connected. Nothing that would trigger `WARN_UNCONNECTED_OUTPUT`, `WARN_UNUSED_MODULE`, `TOP_PORT_NOT_LISTED`, or any other scaffolding diagnostic.
   - **Multi-module structure is optional** for happy-path tests — a single complete module is acceptable when the construct under test does not need cross-module interaction. Prefer multi-module when the rule can fire across instances (to also protect the valid cross-module form from regression).
   - **No error triggers.** Zero. A happy-path test with even one diagnostic is a failed happy-path test.

9. **Write the `.out` file** as the literal text:
   ```
   File: <FILENAME>.jz
   ```
   (one line, followed by a trailing newline) BEFORE running the compiler. This is the expected output.

10. **Run the compiler** on the new `.jz`:
    ```
    compiler/build/jz-hdl --info --lint <file>.jz
    ```
    Capture stdout. Compare to the `.out` you wrote in step 9.
    - **Exact match** (just the `File:` header line) → test is valid. Move on.
    - **Any diagnostic** in the captured output → the `.jz` has a scaffolding bug or the construct you used is not actually valid. **Do not adjust the `.out` to match.** Fix the `.jz` until the capture is clean, then re-run. If you cannot make it clean after two attempts, remove the `.jz`/`.out` and flag in the report as `scaffolding-failure` — do not ship a happy-path test that produces diagnostics.
    - **The intended rule firing** in the captured output → you accidentally wrote an error-case, not a happy-path. This is a bug in your understanding of the rule. Remove the files and flag with category `rule-fires-in-happy-path`.

11. **Run the Quality Checklist** (abbreviated from `2-create.md` for happy-path scope):
    - **Clean capture:** `.out` contains only the `File:` header line.
    - **Construct present:** the `.jz` actually uses the construct the rule governs. A happy-path test that doesn't touch the construct is pointless.
    - **Realistic syntax:** complete blocks, realistic identifiers, not simplified.
    - **No unrelated scaffolding diagnostics:** no `WARN_UNUSED_MODULE`, no `TOP_PORT_NOT_LISTED`, no width mismatches, no undeclared identifiers.
    - **Correct name:** matches the recommended filename from `issues.md` with an `_ok` suffix before `.jz`. If the recommendation is missing `_ok`, add it — happy-path tests MUST end in `_ok.jz` per the convention in `2-create.md`.

12. **Run validation** after all files are written:
    ```
    bash compiler/tests/run_validation.sh
    ```
    Confirm zero regressions. Every newly created happy-path test must pass (empty `.out` = clean compile). Record before/after pass counts.

13. **Append the sweep report** to `compiler/tests/sweep.md` in the format below. If the file does not exist, create it with a `# Sweep History` H1 header and then append the entry. If it exists, append the new entry to the end — never overwrite, edit, or reorder prior entries. Full history is preserved across runs so that non-critical status is not lost.

## `.jz` File Naming

Use the recommended filename from the `issues.md` entry, ensuring it ends in `_ok.jz`:

```
<section>_<subsection>_<RULE_ID>-<descriptive_name>_ok.jz
```

If the audit's recommendation does not include `_ok`, add it before `.jz`. If the audit's recommendation conflicts with an existing file, skip (per step 3) and report — do not rename.

## What to Skip

- Entries where the recommended `.jz` already exists (`skipped: already exists`).
- Entries referencing a rule ID not in `rules.c` (`skipped: stale rule ID`).
- Entries for rules that have no error-case test at all (`skipped: needs error test first`) — these should be routed to `2-create.md` or `5-context-sweep.md` before a happy-path can be meaningfully added.
- Entries for backend-only or simulation-runtime rules that cannot be exercised via `--lint` (`skipped: not testable via --lint`).
- Rules whose "valid form" is semantically identical to their "invalid form" (rare, but possible for some dead-code or suppressed rules). Flag these for human review; do not try to manufacture a valid form.

## Sweep Report Format

Append a single Markdown entry to `compiler/tests/sweep.md` at the end of the run. Each entry begins with a dated H2 header so runs can be distinguished. Use today's date in `YYYY-MM-DD` form.

```markdown
## Happy-Path Sweep — <YYYY-MM-DD>

### Summary
- Work list size (from issues.md):           N
- After dedup:                                N'
- Pre-existing files (skipped):               A
- Stale rule IDs (skipped):                   B
- No error test exists (skipped):             C
- Not testable via --lint (skipped):          D
- No meaningful valid form (skipped):         E
- Successfully created:                       F
- Scaffolding failures (not created):         G
- Rule-fires-in-happy-path (not created):     H
- Total: N' == A + B + C + D + E + F + G + H

### Files Created
| Plan | Rule ID | File |
|------|---------|------|
| test_1_1-identifiers.md | ID_SINGLE_UNDERSCORE | 1_1_ID_SINGLE_UNDERSCORE-valid_no_connect_ok.jz |
| ... | ... | ... |

### Skipped Files
| Plan | Rule ID | Recommended filename | Reason |
|------|---------|----------------------|--------|
| ... | ... | ... | already exists / stale rule ID / no error test / not testable / no valid form |

### Scaffolding Failures (not created)
| Plan | Rule ID | File attempted | Diagnostics that fired | Recommended next step |
|------|---------|----------------|------------------------|-----------------------|
| ... | ... | ... | e.g. WARN_UNUSED_MODULE, TOP_PORT_NOT_LISTED | rewrite scaffolding / check construct validity |

### Rule-Fires-In-Happy-Path (not created)
| Plan | Rule ID | File attempted | Symptom |
|------|---------|----------------|---------|
| ... | ... | ... | rule fired on construct believed to be valid; likely compiler bug or misunderstood rule |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: <pass count> / <total>
- Result after sweep:  <pass count> / <total>
- Newly passing:       <count>
- Newly broken:        <count>   (must be zero — list them if non-zero)
```

## Notes

- **Happy-path tests are regression protection, not feature tests.** They should look like the simplest realistic code a user would write that exercises the construct. Avoid contrived examples — if the simplest valid form of the construct is a three-line module, that's your happy-path.
- **When in doubt, prefer more valid uses over fewer.** A happy-path test with 5 valid uses of a construct catches more regressions than one with a single use. But every use must be genuinely valid — "more coverage" is never an excuse for fudging.
- **If a happy-path test refuses to compile cleanly**, that is either a scaffolding bug or a compiler bug. Scaffolding bugs: fix the `.jz`. Compiler bugs: flag in the report with category `rule-fires-in-happy-path` and let the next audit route it to a compiler-side fix. Never "adjust" the `.out` to accept the diagnostic — that defeats the whole purpose of the happy-path.
- **Stay in scope.** This prompt closes happy-path gaps. It does not author error-case tests (`2-create.md`), audit (`3-audit.md`), or close context gaps (`5-context-sweep.md`).
