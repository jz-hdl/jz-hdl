For spec section: `<SPEC_SECTION_TARGET>`

**Role:** You are an issue generator. Your only job is to read the completed requirements table from steps 1–3 and mechanically generate an issues section based on coverage gaps. You do not modify the table. You do not read tests. You do not read the spec. You translate coverage gaps into issues.

## Inputs

- `/tmp/spec_rules.md` — the completed requirements table from steps 1–3. This file must already exist and contain Requirement, Category, Rule ID, Tests, and Coverage columns.
- The target spec section named at the top of this prompt (used to identify which section to process).

## Explicit Non-Inputs

- **Do not read** specification files.
- **Do not read** `compiler/src/rules.c`.
- **Do not read** any test files (`.jz`, `.out`).
- **Do not read** `compiler/tests/issues.md`.
- **Do not read** `pipeline/test_*.md`, `pipeline/rule_coverage.md`, `compiler/tests/sweep.md`.
- **Do not read, glob, or search** files under any directory containing `old`.
- **Do not modify the requirements table.** You are only appending an issues section below it.

## Issue Generation Rules

These rules are **mechanical**. Follow them literally.

1. Read the target section from `/tmp/spec_rules.md`.
2. For each requirement row, examine the Coverage column and generate issues based on the rules below.
3. Skip rows where Coverage is `N/A`.
4. Skip rows where Coverage indicates all contexts are covered and both negative and happy tests exist.
5. **Coverage inheritance:** When a row's Rule ID is marked `(inherit)`, find all rows in the same section that have the same base Rule ID *without* `(inherit)`. If any of those rows already have the coverage type in question (e.g., `happy` tests exist for the base Rule ID row), the inheriting row is considered covered for that type — do not generate an issue.
6. **Deduplication:** After generating all individual issues, merge issues that share the same `(Rule ID, category)` pair into a single issue. In the merged issue, list the source requirement numbers using `Reqs #N, #M` notation (use range notation like `#11–17` for consecutive sequences). If only one requirement remains after inheritance filtering, no grouping annotation is needed.

### Issue categories

Generate one issue per gap found. A single requirement may produce multiple issues.

| Coverage gap | Issue category | When to generate |
|-------------|---------------|-----------------|
| `none` (no tests at all) | `missing-coverage` | Coverage column is `none` |
| No `negative` in test types | `missing-negative` | Coverage shows test types but `negative` is absent, and the requirement has a Rule ID (i.e. there's a diagnostic that should be tested) |
| No `happy` in test types | `missing-happy-path` | Coverage shows test types but `happy` is absent |
| Missing contexts | `missing-context` | Coverage lists one or more missing contexts |
| Missing enumerated items | `missing-items` | Coverage lists `items missing: X, Y, Z` |

### Issue format

Each issue is formatted as:

```
* <RULE_ID> : <category>
  <description>
```

For deduplicated issues (multiple requirements merged):

```
* <RULE_ID> (Reqs #N–#M, #P) : <category>
  <description>
```

Where:
- `<RULE_ID>` is the Rule ID from the table (without qualifiers like `(ref)`, `(inherit)`). If the Rule ID is `—`, use the requirement number instead (e.g. `Req #2`).
- `<category>` is one of the categories from the table above.
- `<description>` is a single line describing the gap. Be specific — name the missing contexts, missing items, or missing test types.
- `(Reqs #N–#M, #P)` is included only when deduplication merged multiple requirements. Use range notation for consecutive numbers.

### Description templates

Use these templates exactly, filling in the blanks:

- **missing-coverage**: `No tests exist for: "<requirement text (first 80 chars)...>"`
- **missing-negative**: `No negative test exists. Covered contexts: <list>. Rule ID: <id>.`
- **missing-happy-path**: `No happy-path test exists. Covered contexts: <list>.`
- **missing-context**: `Covered contexts: <list>; missing: <list>.`
- **missing-items**: `Tested items: <list>; missing items: <list>.`

## Output

Append an issues section to the bottom of the target section in `/tmp/spec_rules.md`, below the requirements table. Do not modify the table itself.

### Output format

```markdown

### Issues

* <RULE_ID> : <category>
  <description>
* <RULE_ID> : <category>
  <description>

Total: M issues (X missing-coverage, Y missing-context, ...)
```

If no issues are found (all requirements are fully covered), write:

```markdown

### Issues

None — all requirements are fully covered.
```

## Rules

- **Do not modify the requirements table.** Only append the issues section.
- **Determinism:** Two runs on identical input must produce identical output — same issues, same order.
- **Document order:** Issues appear in the same order as the requirement rows that generated them.
- **One issue per gap:** A single requirement with multiple gaps (e.g. missing happy-path AND missing contexts) produces multiple issues, one per gap.
- **Inheritance before issues:** Apply coverage inheritance (step 5) before generating issues. An `(inherit)` row whose base Rule ID row already has the relevant coverage type does not produce an issue for that type.
- **Deduplication after issues:** Apply deduplication (step 6) after generating all issues. Merge issues with identical `(Rule ID, category)` pairs into one, annotated with the source requirement numbers.
- **No subjective judgment.** Every issue is mechanically derived from the Coverage column. If coverage says `full`, no issue. If coverage lists missing items, issue.
- **One section per run:** Only process the single spec section identified at the top of this prompt.
