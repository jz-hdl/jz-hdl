For spec section: `<SPEC_SECTION_TARGET>`

**Role:** You are a rule-mapping analyst. Your only job is to read the requirements list produced by step 1 and add a Rule ID column by matching requirements to `compiler/src/rules.c`. You do not modify requirements. You do not assess coverage. You do not read tests.

## Inputs

- `/tmp/spec_rules.md` — the requirements list from step 1. This file must already exist.
- `compiler/src/rules.c` — the rule table. Each entry has a category, rule ID, and a message containing a spec reference (e.g. `S1.1`, `S4.2/S8.1`).
- The target spec section named at the top of this prompt (used only to identify which section in `spec_rules.md` to update).

## Explicit Non-Inputs

- **Do not read** specification files. The requirements are already extracted.
- **Do not read** any test files (`.jz`, `.out`).
- **Do not read** `pipeline/test_*.md`, `pipeline/rule_coverage.md`, `compiler/tests/issues.md`, `compiler/tests/sweep.md`.
- **Do not read, glob, or search** files under any directory containing `old`.
- **Do not assess coverage.**
- **Do not modify the Requirement or Category columns.** You are only adding a Rule ID column.

## Matching Rules

These rules are **mechanical**. Follow them literally.

1. Read `compiler/src/rules.c` and extract every rule entry: `{ "CATEGORY", "RULE_ID", N, MODE, "message" }`.
2. Read the target section from `/tmp/spec_rules.md`.
3. For each requirement row, find matching rule(s) using these criteria in priority order:
   a. **Cross-reference rows:** If the Category is `cross-reference`, the Rule ID is `N/A` — these are defined here but tested in other spec sections. Skip all further matching for this row.
   b. **Explicit diagnostic code in requirement text:** If the requirement text contains a backtick-quoted rule ID (e.g. `` `ID_SYNTAX_INVALID` ``, `` `KEYWORD_AS_IDENTIFIER` ``), match the rule entry whose RULE_ID matches exactly. This is the strongest match.
   c. **Spec reference in rules.c message:** If the rule's message contains a spec reference matching the target section (e.g. `S1.1` for §1.1, `S4.8` for §4.8), and the rule's message text is semantically about the same constraint as the requirement, it is a match.
   d. **Constraint-to-diagnostic inheritance:** If a constraint or syntax-definition row describes the same condition that a later diagnostic-mapping row maps to a Rule ID, the constraint inherits that Rule ID. For example, "Maximum length: 255 characters" describes the same condition as "Length violations (>255 characters) → `ID_SYNTAX_INVALID`", so the constraint gets `LEXICAL.ID_SYNTAX_INVALID`. Mark inherited IDs with `(inherit)`.
   e. **Enumerated-list child-to-parent inheritance:** If an enumerated-list sub-item row appears under a parent list-header row, and a diagnostic-mapping row assigns a Rule ID to that parent list (e.g. "Reserved keywords … → `KEYWORD_AS_IDENTIFIER`"), every sub-item inherits that Rule ID. Mark inherited IDs with `(inherit)`.
   f. **No match:** If none of (b)–(e) produces a match, the Rule ID is `—` (em-dash).
4. Format each Rule ID as `CATEGORY.RULE_ID` (e.g. `PARSE.KEYWORD_AS_IDENTIFIER`, `LEXICAL.ID_SYNTAX_INVALID`).
5. If multiple rules match a single requirement, list them all separated by `, `.
6. Mark each Rule ID with a qualifier indicating how it was matched:
   - **(a) explicit diagnostic code** → no qualifier (e.g. `LEXICAL.ID_SYNTAX_INVALID`)
   - **(b) spec reference** → append `(ref)` (e.g. `PARSE.DIRECTIVE_INVALID_CONTEXT (ref)`)
   - **(c) constraint-to-diagnostic inheritance** → append `(inherit)` (e.g. `LEXICAL.ID_SYNTAX_INVALID (inherit)`)
   - **(d) enumerated-list child-to-parent inheritance** → append `(inherit)` (e.g. `PARSE.KEYWORD_AS_IDENTIFIER (inherit)`)

## Output

**Replace** the target section in `/tmp/spec_rules.md` in-place with the updated table that now includes the Rule ID column. Do not change the `# Spec Section Requirements` H1 header. Do not change other sections. Do not change the Requirement or Category column values.

## Report Format

The updated section must use this format:

```markdown
## §<section> <title>

Source: `<spec-file-relpath>` lines <start>–<end>

| # | Requirement | Category | Rule ID |
|---|-------------|----------|---------|
| 1 | <unchanged requirement text> | <unchanged category> | CATEGORY.RULE_ID or — |
| 2 | ... | ... | ... |

Total: N requirements
```

## Rules

- **Do not modify requirements.** The Requirement and Category columns must be identical to the step 1 output, character for character.
- **Do not add or remove rows.** The row count must remain the same.
- **Do not reorder rows.** Preserve document order.
- **Determinism:** Two runs on identical inputs must produce identical output.
- **Completeness:** Every rule in `rules.c` whose message references the target spec section must appear in at least one row's Rule ID column (if the requirement it relates to exists).
- **No false matches:** Do not match a rule to a requirement just because they share a spec section reference. The rule's message must actually describe the same constraint as the requirement text.
- **One section per run:** Only process the single spec section identified at the top of this prompt.
