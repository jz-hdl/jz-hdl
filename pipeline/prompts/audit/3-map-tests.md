For spec section: `<SPEC_SECTION_TARGET>`

**Role:** You are a test-mapping analyst. Your only job is to read the requirements list produced by steps 1–2 and add `Tests` and `Coverage` columns by finding which validation tests exercise each requirement. You do not modify requirements, categories, coverage domains, applicable contexts, split IDs, or primary rule IDs.

## Inputs

- `/tmp/spec_rules.md` — the requirements list from steps 1–2. This file must already exist and contain `Category`, `Coverage Domain`, `Applicable Contexts`, `Split ID`, and `Primary Rule ID` columns.
- `compiler/tests/validation/*.jz` and paired `.out` files — the validation test corpus.
- The target spec section named at the top of this prompt (used to identify which section in `spec_rules.md` to update and to scope test discovery).

## Explicit Non-Inputs

- **Do not read** specification files.
- **Do not read** `compiler/src/rules.c`.
- **Do not read** `pipeline/test_*.md`, `pipeline/rule_coverage.md`, `compiler/tests/issues.md`, `compiler/tests/sweep.md`.
- **Do not read, glob, or search** files under any directory containing `old`.
- **Do not editorialize on coverage.** Compute coverage mechanically using the rules below.
- **Do not modify the Requirement, Category, Coverage Domain, Applicable Contexts, Split ID, or Primary Rule ID columns.**

## Test Discovery

These rules are **mechanical**. Follow them literally.

### Step 1: Find candidate test files

Use all three discovery methods. A file is a candidate if it matches **any** of them.

a. **Section-prefix naming:** Glob for `compiler/tests/validation/<section_prefix>_*.jz` where `<section_prefix>` is the section number with dots replaced by underscores (e.g. §1.1 -> `1_1_*`, §4.13.1 -> `4_13_1_*`).

b. **Rule-ID naming:** For each `Primary Rule ID` in the requirements table, strip qualifiers like `(ref)` and `(inherit)`, then glob for `compiler/tests/validation/*_<RULE_ID>-*.jz` (e.g. `*_KEYWORD_AS_IDENTIFIER-*.jz`, `*_ID_SYNTAX_INVALID-*.jz`). Skip rows whose Primary Rule ID is `N/A` or `—`.

c. **Diagnostic output search:** For each `Primary Rule ID` in the requirements table, strip qualifiers like `(ref)` and `(inherit)`, then search all `.out` files in `compiler/tests/validation/` for the bare `RULE_ID` string (e.g. `KEYWORD_AS_IDENTIFIER`, `ID_SYNTAX_INVALID`). Any `.out` file containing the string makes its paired `.jz` a candidate. Skip rows whose Primary Rule ID is `N/A` or `—`.

### Non-validation rows

Requirements whose `Coverage Domain` is `cross-reference`, `golden`, `simulation`, or `non-actionable` are outside validation-fixture mapping. Set both `Tests` and `Coverage` to `N/A` and skip test discovery for them.

### Step 2: Read and classify each candidate

For every candidate `.jz` file found in Step 1:

1. Read the `.jz` file.
2. Read its paired `.out` file.
3. Determine which requirements this test exercises by examining:
   - **The `.out` file:** Which rule IDs appear in the diagnostic output? Each diagnostic line maps to requirement rows sharing that primary rule.
   - **The `.jz` file contents:** What constructs, keywords, identifiers, or scenarios does the test use? Match these against the requirement text.
   - **Comments in the `.jz` file:** Comments like `// Trigger 1: keyword IF as module name` indicate which specific items or contexts are being tested.
4. A single test file may exercise multiple requirements. List it under every requirement it exercises.

### Step 3: Classify each test

For each test-to-requirement mapping, record:

**Test type:**

- **`negative`** — the paired `.out` file contains one or more diagnostics. Mixed fixtures with any diagnostics are always `negative`.
- **`happy`** — the paired `.out` file contains only the `File:` header line and no diagnostics at all.

Rules:

- A file with any diagnostic lines may contribute negative coverage, context evidence, and item evidence.
- A file with any diagnostic lines may **not** contribute happy-path coverage for any requirement.

**Contexts tested:**

- Determine which of the row's `Applicable Contexts` are actually exercised by the test.
- Use only the contexts listed in the row's `Applicable Contexts` column.
- If the row's `Applicable Contexts` is `N/A`, record no contexts for that row.

### Step 4: Compute coverage

For each requirement row, compute coverage mechanically.

#### 4a. Test types present

Check which test types exist for this requirement:

- `has-negative` — at least one negative test exists
- `has-happy` — at least one happy test exists

#### 4b. Context coverage

Use the row's `Applicable Contexts` column as the authoritative applicable context set:

- If `Applicable Contexts` is `N/A`, context coverage is `N/A`.
- Otherwise, compare the contexts actually exercised by tests against the exact listed contexts.
- Do **not** infer extra applicable contexts from the rule family, section title, or requirement category.

#### 4c. Coverage value

Combine 4a and 4b into the `Coverage` column:

- If `Coverage Domain` is not `validation`: `N/A`
- If no tests exist at all: `none`
- If `Applicable Contexts` is `N/A`: `negative, happy | contexts: N/A`, `negative | contexts: N/A`, or `happy | contexts: N/A`
- If `Applicable Contexts` is a context list:
  1. Test types: `negative, happy` or `negative` or `happy`
  2. Contexts tested: list the context labels found
  3. Contexts missing: list the applicable context labels not found, or `none` if all covered

Format:

- `negative, happy | contexts: async-expr, sync-expr | missing: new-binding`
- `negative | contexts: N/A`
- `happy | contexts: all`

If all applicable contexts are covered, use `contexts: all`.

**`enumerated-list` rows** additionally track item coverage:

- After the context info, add `| items missing: X, Y, Z` or `| items: all`
- Only count an item as covered if the test actually uses that specific enumerated item

## Important

- **Do not rely on filenames alone.** Read the actual `.jz` and `.out` content.
- **One file may cover many requirements.** List it under every requirement it truly exercises.
- **Mixed fixtures are not happy-path evidence.** If a file has any diagnostics at all, it is never `happy`.
- **Applicable Contexts is authoritative.** Do not synthesize broader context sets.

## Output

**Replace** the target section in `/tmp/spec_rules.md` in-place with the updated table that now includes the `Tests` and `Coverage` columns. Do not change the `# Spec Section Requirements` H1 header. Do not change other sections. Do not change the Requirement, Category, Coverage Domain, Applicable Contexts, Split ID, or Primary Rule ID column values.

## Report Format

The updated section must use this format:

```markdown
## §<section> <title>

Source: `<spec-file-relpath>` lines <start>–<end>

| # | Requirement | Category | Coverage Domain | Applicable Contexts | Split ID | Primary Rule ID | Tests | Coverage |
|---|-------------|----------|-----------------|---------------------|----------|-----------------|-------|----------|
| 1 | <unchanged> | <unchanged> | <unchanged> | <unchanged> | <unchanged> | <unchanged> | `filename.jz` (negative) {contexts}, `filename2.jz` (happy) {contexts} or — or N/A | structured coverage value or none or N/A |
| 2 | ... | ... | ... | ... | ... | ... | ... | ... |

Total: N requirements
```

### Tests column format

- List each test file as its basename (no path), e.g. `1_1_ID_SYNTAX_INVALID-length_exceeded.jz`
- After each filename, add `(negative)` or `(happy)` in parentheses
- After the test type, add the actually exercised contexts in braces, e.g. `(negative) {module-name, wire-name}`
- For enumerated-list items, also add the specific list members tested in brackets, e.g. `1_1_KEYWORD_AS_IDENTIFIER-more_keywords.jz (negative) {module-name, wire-name} [IF, ELSE, CASE]`
- Separate multiple test files with `, `
- Use `—` if no validation test was found
- Use `N/A` for rows whose `Coverage Domain` is not `validation`

## Rules

- **Do not modify existing columns.** The Requirement, Category, Coverage Domain, Applicable Contexts, Split ID, and Primary Rule ID columns must be identical to the step 2 output, character for character.
- **Do not add or remove rows.** Preserve the step 2 row set exactly.
- **Do not reorder rows.** Preserve document order.
- **Determinism:** Two runs on identical inputs and test corpus must produce identical output.
- **Read before mapping.** Do not count a file as evidence until you have read both the `.jz` and `.out` files.
- **Mechanical coverage only.** Coverage is computed by the deterministic rules above — no subjective judgment.
- **One section per run:** Only process the single spec section identified at the top of this prompt.
