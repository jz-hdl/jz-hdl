For spec section: `<SPEC_SECTION_TARGET>`

**Role:** You are a rule-mapping analyst. Your only job is to read the requirements list produced by step 1 and add a primary rule ID column by matching requirements to `compiler/src/rules.c`. You do not assess coverage. You do not read tests.

## Inputs

- `<OUTPUT_FILE>` — the requirements list from step 1. This file must already exist.
- `compiler/src/rules.c` — the rule table. Each entry has a category, rule ID, and a message containing a spec reference (e.g. `S1.1`, `S4.2/S8.1`).
- The target spec section named at the top of this prompt (used only to identify which section in `<OUTPUT_FILE>` to update).

## Explicit Non-Inputs

- **Do not read** specification files. The requirements are already extracted.
- **Do not read** any test files (`.jz`, `.out`).
- **Do not read** `pipeline/test_*.md`, `pipeline/rule_coverage.md`, `compiler/tests/issues.md`, `compiler/tests/sweep.md`.
- **Do not read, glob, or search** files under any directory containing `old`.
- **Do not assess coverage.**
- **Do not modify the Requirement, Category, Coverage Domain, Applicable Contexts, or Coverage Keys columns.** You are only adding `Split ID` and `Primary Rule ID` columns, unless you must split a row as described below.

## Matching Rules

These rules are **mechanical**. Follow them literally.

1. Read `compiler/src/rules.c` and extract every rule entry in file order: `{ "CATEGORY", "RULE_ID", N, MODE, "message" }`.
2. Read the target section from `<OUTPUT_FILE>`.
3. For each requirement row, find candidate matches using these criteria in priority order:
   a. **Cross-reference and non-validation rows:** If `Coverage Domain` is `cross-reference`, `golden`, `simulation`, or `non-actionable`, the Primary Rule ID is `N/A`. Skip all further matching for that row.
   b. **Explicit diagnostic code in requirement text:** If the requirement text contains a backtick-quoted rule ID (e.g. `` `ID_SYNTAX_INVALID` ``), match the rule entry whose `RULE_ID` matches exactly. This is the strongest match.
   c. **Constraint-to-diagnostic inheritance:** If a `constraint` or `syntax-definition` row describes the same condition that a later `diagnostic-mapping` row maps to a Rule ID, the row inherits that Rule ID. Mark inherited IDs with `(inherit)`.
   d. **Enumerated-list child-to-parent inheritance:** If an `enumerated-list` sub-item row appears under a parent list-header row, and a `diagnostic-mapping` row assigns a Rule ID to that parent list, every matching sub-item inherits that Rule ID. Mark inherited IDs with `(inherit)`.
   e. **Strong same-condition spec-reference match:** A rule whose message references the target section may match only if the message and requirement row describe the same condition, not merely the same section. The construct, actor, and polarity must align.
      - **Construct alignment:** the same kind of thing is being constrained (e.g. MEM INOUT `.addr`, `@check`, duplicate instance name, BUS wildcard assignment).
      - **Actor alignment:** the same subject is being constrained (e.g. instance name vs module name, GLOBAL vs CONST, widthof target vs widthof context).
      - **Polarity alignment:** both the requirement and the rule are about the same allowed/forbidden behavior, not just nearby concepts in the same section.
      - If any of these do not align clearly, it is **not** a match.
   f. **No match:** If none of (b)–(e) produces a match, the Primary Rule ID is `—`.
4. Format each matched rule as `CATEGORY.RULE_ID` (e.g. `PARSE.KEYWORD_AS_IDENTIFIER`, `LEXICAL.ID_SYNTAX_INVALID`).
5. If multiple candidate rules remain after applying the rules above, choose the **primary** rule by the first matching entry in `compiler/src/rules.c` file order. This is the rule priority.
6. Add qualifiers indicating how the primary rule was matched:
   - **explicit diagnostic code** -> no qualifier
   - **constraint-to-diagnostic inheritance** -> append `(inherit)`
   - **enumerated-list child-to-parent inheritance** -> append `(inherit)`
   - **strong same-condition spec-reference match** -> append `(ref)`

## Optional Row Splitting

Prefer to keep one extracted row and assign one primary rule.

You may split a row **only** if all of the following are true:

1. The single extracted requirement row contains multiple independent diagnostic mappings or multiple independent conditions.
2. Those conditions cannot be represented honestly by one primary rule.
3. Each split row can preserve the original requirement text exactly.

If you split:

- Duplicate the row once per independent condition.
- Preserve `Requirement`, `Category`, `Coverage Domain`, `Applicable Contexts`, and `Coverage Keys` exactly.
- Assign a stable `Split ID` to every output row:
  - Use `base` for rows that were not split.
  - For split rows, use `split-1`, `split-2`, ... in the duplicated row order.
- Assign one Primary Rule ID to each split row.
- Preserve document order.
- Renumber the `#` column for the whole section after splitting.

If you do not split:

- Add `Split ID` with the value `base`.

## Output

**Replace** the target section in `<OUTPUT_FILE>` in-place with the updated table that now includes the `Split ID` and `Primary Rule ID` columns while preserving `Coverage Keys`. Do not change the `# Spec Section Requirements` H1 header. Do not change other sections.

## Report Format

The updated section must use this format:

```markdown
## §<section> <title>

Source: `<spec-file-relpath>` lines <start>–<end>

| # | Requirement | Category | Coverage Domain | Applicable Contexts | Coverage Keys | Split ID | Primary Rule ID |
|---|-------------|----------|-----------------|---------------------|---------------|----------|-----------------|
| 1 | <unchanged requirement text> | <unchanged category> | <unchanged coverage domain> | <unchanged contexts> | <unchanged keys> | base or split-N | CATEGORY.RULE_ID or — or N/A |
| 2 | ... | ... | ... | ... | ... | ... | ... |

Total: N requirements
```

## Rules

- **Do not modify extracted content.** The Requirement, Category, Coverage Domain, Applicable Contexts, and Coverage Keys columns must be identical to the step 1 output, character for character, unless you split a row under the Optional Row Splitting rule.
- **Stable row identity:** Every output row must have a `Split ID`. Use `base` when unsplit, or `split-N` when split.
- **Prefer no row splitting.** Keep the row count the same unless splitting is necessary to avoid a dishonest primary-rule assignment.
- **Do not reorder rows.** Preserve document order.
- **Determinism:** Two runs on identical inputs must produce identical output.
- **Soft completeness:** Every rule in `rules.c` whose message references the target spec section should appear in at least one row's Primary Rule ID column **when a row clearly matches it**. Do not force a weak match just to satisfy completeness.
- **No false matches:** Do not match a rule to a requirement just because they share a spec section reference. The same-condition evidence must be clear.
- **One section per run:** Only process the single spec section identified at the top of this prompt.
