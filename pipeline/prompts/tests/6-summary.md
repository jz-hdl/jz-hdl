**Role:** You are a Senior SDET reconciling the JZ-HDL compiler test-coverage worklog into a single flat remaining-issues list.

## Context

The validation workflow maintains three living documents:

- `compiler/tests/issues.md` — audit log keyed by test plan. Written by `3-audit.md` in verbose form with subsections: `### Missing Coverage`, `### Missing Contexts`, `### Missing Happy-Path`, `### Test Quality Issues`, `### Parser Recovery`, `### Possible Compiler Bugs`.
- `compiler/tests/sweep.md` — append-only per-plan sweep run reports. Each report may *resolve* `issues.md` entries (via `### Files Created`) or *add* new entries (via `### Scaffolding or Compiler Bugs Found`, `### Parser Recovery Findings`).
- `compiler/tests/not_tested.md` — rules that cannot be tested via `--lint` (backend-only, runtime-only). Keyed by plan with a `Rule | Severity | Reason` table.

This prompt reconciles those three files and **overwrites `compiler/tests/issues.md`** with a flat per-plan bullet list of every issue that still remains. The verbose subsection structure is intentionally discarded — after this runs, `issues.md` is an operator-readable worklist.

## Input

- `compiler/tests/issues.md` — current audit log (verbose format).
- `compiler/tests/sweep.md` — full sweep history.
- `compiler/tests/not_tested.md` — untestable-rules list.

Read all three in full before writing anything.

## CRITICAL CONSTRAINTS

- **NEVER read, glob, or search** files under any directory containing `old`.
- **NEVER use git history** or read deleted git files.
- **DO NOT** modify `sweep.md` or `not_tested.md`.
- **DO NOT** invent issues that are not present in one of the three input files.
- **DO NOT** merge independent issues for the same rule into one bullet — emit one bullet per issue.
- **DO NOT** modify `compiler/tests/summary.md` (no longer written by this prompt).
- The rewritten `issues.md` replaces the previous content wholesale. The next `3-audit.md` run will regenerate verbose sections; that is expected.

## Issue Types

Use exactly one of these labels after the colon. The `Key` column is what precedes the colon in each bullet.

| Issue type           | Source                                                                                 | Key      |
|----------------------|----------------------------------------------------------------------------------------|----------|
| `missing-coverage`   | `issues.md` § Missing Coverage                                                         | rule ID  |
| `missing-context`    | `issues.md` § Missing Contexts                                                         | rule ID  |
| `missing-happy-path` | `issues.md` § Missing Happy-Path                                                       | rule ID  |
| `compiler-bug`       | `issues.md` § Possible Compiler Bugs, or `sweep.md` § Scaffolding or Compiler Bugs Found | rule ID  |
| `parser-recovery`    | `issues.md` § Parser Recovery, or `sweep.md` § Parser Recovery Findings                | rule ID  |
| `test-quality`       | `issues.md` § Test Quality Issues                                                      | filename |
| `not-testable`       | `not_tested.md`                                                                        | rule ID  |

Sweep categories `compiler-bug`, `rule-not-fired`, and `compiler-bug / rule-not-fired` all map to `compiler-bug`.

## Reconciliation Rules

1. **Build the working set from `issues.md`.** Walk every `## <plan>.md` H2 and every `###` subsection under it. Each bullet becomes one `(plan, key, issue_type, summary)` entry.

2. **Drop entries resolved by `sweep.md`.** For every `### Files Created` row in any sweep report, find the matching working-set entry by recommended filename (or by `rule_id + missing context` when filename match is ambiguous) and remove it.
   - Context-sweep `Files Created` resolves `missing-context` entries.
   - Happy-sweep `Files Created` resolves `missing-happy-path` entries.
   - Either sweep's `Files Created` resolves `missing-coverage` entries when the recommended filename matches.
   - `### Skipped Files` rows with reason `already exists` also resolve the matching entry.
   - `### Skipped Files` rows with reason `stale rule ID`, `no error test`, `not testable via --lint`, `no valid form` do **not** resolve the entry — the issue is unfixed.

3. **Add entries discovered by `sweep.md`.** For every `### Scaffolding or Compiler Bugs Found` row, ensure a `compiler-bug` entry exists for `(plan, rule_id)`. For every `### Parser Recovery Findings` row, ensure a `parser-recovery` entry exists. If `issues.md` already has an equivalent entry for the same `(plan, rule_id, issue_type)`, keep one — do not duplicate.

4. **Add entries from `not_tested.md`.** For each row in each plan's table, add a `not-testable` entry with `Reason` as the summary. Use the same plan heading `not_tested.md` uses.

5. The final working set = (issues.md entries − resolved) ∪ (new sweep entries) ∪ (not_tested.md entries).

## Output Format

Overwrite `compiler/tests/issues.md` with exactly this structure:

```markdown
# Compiler Test Issues

_Last reconciled: <YYYY-MM-DD> by summary.md_

## test_<N>_<M>-<name>.md

* <KEY> : <issue-type>
  Full description of the issue. Can span multiple indented lines if needed
  to capture all relevant detail — covered contexts, missing contexts,
  recommended filenames, root-cause hypotheses, etc.
* <KEY> : <issue-type>
  Another issue description.

## test_<next plan>.md

* ...
```

### Bullet rules

- The `* <KEY> : <issue-type>` header line, then one or more indented description lines (two-space indent).
- `<KEY>` is the rule ID for every type except `test-quality`, which uses the filename.

### Description

The indented lines after the header are the **full description** of the issue — not a summary, not a one-liner. Preserve the substance and detail from the source entry. The reader should understand what the bug is, what was expected, what actually happens, and what the recommended next step is without having to look anything up.

- `missing-coverage`: describe what rule is untested, what severity/spec-ref applies, and include the recommended filename.
- `missing-context`: list all covered contexts, all missing contexts, and include every recommended filename.
- `missing-happy-path`: describe what valid-form test is missing and include the recommended filename.
- `compiler-bug`: describe the symptom fully — what was expected to happen, what actually happens, what file/line demonstrated it, and any root-cause hypothesis from the source entry.
- `parser-recovery`: describe the cascading-error pattern, which file triggered it, and the workaround or real fix noted in the source.
- `test-quality`: describe the specific defect — wrong name, stale comment, duplicate, dedup masking, misclassification, etc. — and the recommended fix.
- `not-testable`: the reason verbatim from `not_tested.md`.

### Ordering

1. **Plan sections** in the same order they appear in the current `issues.md`. Any plan that appears only in `not_tested.md` goes at the end.
2. **Within a plan**, sort bullets by issue type using this priority: `compiler-bug`, `parser-recovery`, `missing-coverage`, `missing-context`, `missing-happy-path`, `test-quality`, `not-testable`.
3. **Within an issue type**, sort alphabetically by `<KEY>`.

### Empty plans

If a plan has zero remaining entries after reconciliation, **omit its H2 heading entirely**.

## Workflow

1. Read `compiler/tests/issues.md` in full.
2. Read `compiler/tests/sweep.md` in full.
3. Read `compiler/tests/not_tested.md` in full.
4. Build the working set (Reconciliation step 1).
5. Apply sweep resolutions (step 2).
6. Apply sweep additions (step 3).
7. Apply `not_tested.md` additions (step 4).
8. Sort per Ordering.
9. Overwrite `compiler/tests/issues.md` with the new content.
10. Print a one-line count to stdout:
    `Reconciled: <remaining> remaining across <N> plans (<removed> resolved, <added> added).`

## Notes

- The next `3-audit.md` run will replace the flat form with fresh verbose sections. That is the intended handoff — `summary.md` produces a readable snapshot; `audit.md` produces a detailed re-examination.
- If a rule has three independent issues (e.g. `missing-context`, `missing-happy-path`, `compiler-bug`), emit three bullets. Never collapse.
- If a sweep report is ambiguous about whether a finding resolves an `issues.md` entry, prefer **not** dropping the entry — false positives (stale items) are recoverable by the next audit, but dropping a real issue silently is not.
