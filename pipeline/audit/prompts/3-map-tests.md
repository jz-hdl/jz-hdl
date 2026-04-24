For spec section: `<SPEC_SECTION_TARGET>`

**Role:** You are a test-mapping analyst. Your only job is to read the requirements list produced by steps 1–2 and add `Tests` and `Coverage` columns by finding which validation tests exercise each requirement. You do not modify requirements, categories, coverage domains, applicable contexts, coverage keys, split IDs, or primary rule IDs.

## Inputs

- `<OUTPUT_FILE>` — the requirements list from steps 1–2. This file must already exist and contain `Category`, `Coverage Domain`, `Applicable Contexts`, `Coverage Keys`, `Split ID`, and `Primary Rule ID` columns.
- `compiler/tests/validation/*.jz` and paired `.out` files — the validation test corpus.
- The target spec section named at the top of this prompt (used to identify which section in `<OUTPUT_FILE>` to update and to scope test discovery).

## Explicit Non-Inputs

- **Do not read** specification files.
- **Do not read** `compiler/src/rules.c`.
- **Do not read** `pipeline/test_*.md`, `pipeline/rule_coverage.md`, `compiler/tests/issues.md`, `compiler/tests/sweep.md`.
- **Do not read, glob, or search** files under any directory containing `old`.
- **Do not editorialize on coverage.** Compute coverage mechanically using the rules below.
- **Do not modify the Requirement, Category, Coverage Domain, Applicable Contexts, Coverage Keys, Split ID, or Primary Rule ID columns.**

## Corpus Index And Test Discovery

These rules are **mechanical**. Follow them literally.

### Step 1: Build a global validation corpus index

Read **every** `compiler/tests/validation/*.jz` file and its paired `.out` file once for this run. Build an internal corpus index entry for each test containing:

- basename
- section prefix from the filename
- test type: `negative` or `happy`
- diagnostic rule IDs from the `.out` file
- comment hints from the `.jz` file
- obvious constructs and syntax forms used in the `.jz` file
- tracked contexts actually exercised in the `.jz` file

The construct and syntax evidence should be recorded in the same semantic space as `Coverage Keys` where possible:

- construct family
- concrete syntax forms
- observable behaviors
- diagnostic IDs
- tracked contexts

Do **not** rely on filename patterns alone for the corpus index.

### Step 2: Select candidate tests per requirement row

For each validation row, select candidate tests from the global corpus index using the evidence below.

#### Strong evidence

A test is a candidate if **any** strong evidence matches:

a. **Section-prefix naming:** The test basename has the same section prefix as the target section number with dots replaced by underscores.

b. **Rule-ID naming or diagnostics:** The row has a `Primary Rule ID` other than `N/A` or `—`, and the test basename or `.out` diagnostics contain that bare `RULE_ID`.

c. **Coverage-key exact match:** A `diagnostic:` or `form:` key from the row's `Coverage Keys` appears directly in the test's diagnostics, comments, basename, or obvious syntax usage.

#### Medium evidence

If no strong evidence matches, a test is still a candidate when **two or more** of these medium signals align:

- one or more `construct:` keys overlap
- one or more `behavior:` keys overlap
- one or more `context:` keys overlap
- the test comments describe the same allowed/forbidden scenario as the requirement text
- the test uses the same concrete syntax form even if it was authored for another section

Cross-section matches are allowed and expected. A test from another section still counts if the evidence above shows it exercises the requirement.

### Non-validation rows

Requirements whose `Coverage Domain` is `cross-reference`, `golden`, `simulation`, or `non-actionable` are outside validation-fixture mapping. Set both `Tests` and `Coverage` to `N/A` and skip test discovery for them.

### Step 3: Read and classify each candidate

For every candidate test selected in Step 2:

1. Use the corpus index entry plus the underlying `.jz`/`.out` content.
2. Determine which requirements this test exercises by examining:
   - **The `.out` file:** Which rule IDs appear in the diagnostic output? Each diagnostic line maps directly to rows sharing that primary rule.
   - **The `Coverage Keys`:** Match the row's `construct:`, `form:`, `behavior:`, `diagnostic:`, and `context:` keys against the test's indexed evidence.
   - **The `.jz` file contents:** Verify that the actual construct, syntax form, and context are present in code, not just implied by the filename.
   - **Comments in the `.jz` file:** Comments like `// Trigger 1: keyword IF as module name` or `// Covers:` style comments are evidence for specific items or contexts.
3. A single test file may exercise multiple requirements. List it under every requirement it truly exercises.
4. For `happy` coverage, construct/form/context evidence is sufficient even when there is no matching diagnostic rule ID. Do **not** require a rule-ID match for positive semantic coverage.

### Step 4: Classify each test

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

### Step 5: Compute coverage

For each requirement row, compute coverage mechanically.

#### 5a. Test types present

Check which test types exist for this requirement:

- `has-negative` — at least one negative test exists
- `has-happy` — at least one happy test exists

#### 5b. Context coverage

Use the row's `Applicable Contexts` column as the authoritative applicable context set:

- If `Applicable Contexts` is `N/A`, context coverage is `N/A`.
- Otherwise, compare the contexts actually exercised by tests against the exact listed contexts.
- Do **not** infer extra applicable contexts from the rule family, section title, or requirement category.

#### 5c. Coverage value

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
- **Use the whole corpus.** Candidate discovery starts from the full validation index, not just same-section files.
- **One file may cover many requirements.** List it under every requirement it truly exercises.
- **Mixed fixtures are not happy-path evidence.** If a file has any diagnostics at all, it is never `happy`.
- **Applicable Contexts is authoritative.** Do not synthesize broader context sets.
- **Coverage Keys drive semantic matching.** Happy-path and cross-section coverage may be proven by construct/form/context evidence even without a matching rule ID.

## Output

**Replace** the target section in `<OUTPUT_FILE>` in-place with the updated table that now includes the `Tests` and `Coverage` columns. Do not change the `# Spec Section Requirements` H1 header. Do not change other sections. Do not change the Requirement, Category, Coverage Domain, Applicable Contexts, Coverage Keys, Split ID, or Primary Rule ID column values.

## Report Format

The updated section must use this format:

```markdown
## §<section> <title>

Source: `<spec-file-relpath>` lines <start>–<end>

| # | Requirement | Category | Coverage Domain | Applicable Contexts | Coverage Keys | Split ID | Primary Rule ID | Tests | Coverage |
|---|-------------|----------|-----------------|---------------------|---------------|----------|-----------------|-------|----------|
| 1 | <unchanged> | <unchanged> | <unchanged> | <unchanged> | <unchanged> | <unchanged> | <unchanged> | `filename.jz` (negative) {contexts}, `filename2.jz` (happy) {contexts} or — or N/A | structured coverage value or none or N/A |
| 2 | ... | ... | ... | ... | ... | ... | ... | ... | ... |

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

- **Do not modify existing columns.** The Requirement, Category, Coverage Domain, Applicable Contexts, Coverage Keys, Split ID, and Primary Rule ID columns must be identical to the step 2 output, character for character.
- **Do not add or remove rows.** Preserve the step 2 row set exactly.
- **Do not reorder rows.** Preserve document order.
- **Determinism:** Two runs on identical inputs and test corpus must produce identical output.
- **Read before mapping.** Do not count a file as evidence until you have read both the `.jz` and `.out` files.
- **Mechanical coverage only.** Coverage is computed by the deterministic rules above — no subjective judgment.
- **One section per run:** Only process the single spec section identified at the top of this prompt.
