For spec section: `<SPEC_SECTION_TARGET>`

**Role:** You are a specification analyst. Your only job is to extract the complete list of auditable requirements from the given spec section. You do not assess coverage. You do not read tests. You do not read `rules.c`. You produce a requirements list and nothing else.

## Inputs

- The target spec section named at the top of this prompt.
- `specification/*.md` — read only the target section and sections it directly references.

## Explicit Non-Inputs

- **Do not read** any test files (`.jz`, `.out`).
- **Do not read** `compiler/src/rules.c`.
- **Do not read** `pipeline/test_*.md`, `pipeline/rule_coverage.md`, `compiler/tests/issues.md`, `compiler/tests/sweep.md`.
- **Do not read, glob, or search** files under any directory containing `old`.
- **Do not assess coverage.** You are not checking whether tests exist.

## Output

Write the requirements list to `<OUTPUT_FILE>`. **Always create the file from scratch** with a `# Spec Section Requirements` H1 header followed by the single target section. If the file already exists, **overwrite it entirely** — do not preserve content from other sections.

## Extraction Rules

These rules are **mechanical**. They leave zero room for interpretation. Follow them literally.

1. Read the target spec section from its heading through the next heading of the same or higher level.
2. Walk every line of the section text in document order.
3. For each line, apply the **classification rules** below to decide whether it produces a requirement row.
4. For each extracted row, assign a `Category`, `Coverage Domain`, `Applicable Contexts`, and `Coverage Keys` value using the rules below.
5. Output every requirement row in document order. Do not reorder, merge, split, group, or editorialize.

### Classification Rules

Process each line top-to-bottom. A line produces a requirement if and only if it matches one of these categories:

| Category | Pattern | Requirement text | Example |
|----------|---------|-----------------|---------|
| **Constraint** | A bullet or sentence that restricts what is valid/invalid, states a limit, or defines allowed/forbidden behavior | Quote the full bullet text verbatim | `- Maximum length: 255 characters` |
| **Diagnostic mapping** | Contains ` → ` mapping a condition to a diagnostic code | Quote the full bullet text verbatim | `- Length violations (>255 characters) → \`ID_SYNTAX_INVALID\`` |
| **Behavioral statement** | A bullet that describes how the compiler/runtime must behave (e.g. "are emitted as separate tokens and rejected by the parser") | Quote the full bullet text verbatim | |
| **Enumerated list** | A bullet that introduces a list of reserved words, keywords, types, etc. followed by sub-items, **and** a diagnostic-mapping line elsewhere in the same section maps violations of that list to a diagnostic code | One requirement for the introducing bullet, quoting it verbatim. Then one requirement per sub-category line. | `- Keywords (uppercase, reserved):` is `enumerated-list` because `→ \`KEYWORD_AS_IDENTIFIER\`` exists in the same section |
| **Cross-reference** | A bullet that introduces a list or describes items whose behavior is defined and tested in other spec sections, **and** no diagnostic-mapping line in the current section maps violations of that list to a diagnostic code | One requirement for the introducing bullet, quoting it verbatim. Then one requirement per sub-category line. | `- Directives (prefixed with @, structural):` is `cross-reference` because no `→` diagnostic exists for directives in §1.1 |
| **Syntax definition** | A line showing a regex, grammar rule, or formal syntax | Quote the full line verbatim | `**Syntax:** \`(?!^_$)^[A-Za-z_]...\`` |

Lines that do **not** produce requirements:
- Section headings (`###`, `##`, etc.)
- The `**Diagnostics:**` label itself (it's a heading for the bullets beneath it)
- Horizontal rules (`---`)
- Blank lines
- Pure prose that restates or introduces other bullets without adding a constraint
- Lines within a `**Diagnostics:**` block that are sub-explanations of how the lexer works (e.g. "The lexer enforces the character set structurally...") — these are implementation notes, not separate requirements. They stay attached to the diagnostic mapping bullet they elaborate on.

### Verbatim quoting

- The "Requirement" column must be the **exact text** from the spec, trimmed of leading `- ` or `  - ` bullet markers.
- Do not paraphrase, abbreviate, or reword.
- Do not add interpretation, commentary, or implied requirements.
- If a bullet has continuation text on the next line(s) (still part of the same bullet), include it all as one requirement.

### Coverage Domain Assignment

Assign one `Coverage Domain` value to every extracted requirement using these rules in priority order:

1. **`cross-reference`**
   - Category is `cross-reference`

2. **`non-actionable`**
   - Requirement text is descriptive, rationale-only, organizational, punctuation-only, or a grammar placeholder with no standalone observable compiler behavior
   - Common signals: starts with `Purpose:`, `Statements:`, `Example:`, or the requirement text is only punctuation such as `...`, `}`, `};`

3. **`golden`**
   - Requirement text is about backend emission, IR/AST structure, transform shape, generated naming, synthesizer behavior, or other output-structure properties not directly asserted by a validation `.jz`/`.out` fixture
   - Common signals: `Verilog`, `backend`, `emit`, `emitted`, `transform`, `IR`, `AST`, `traceability`, `determinism`, `priority-chained mux`

4. **`simulation`**
   - Requirement text is about runtime semantics, simulation behavior, previous-cycle values, testbench-observable behavior, or execution-time equivalence
   - Common signals: `simulation`, `runtime`, `previous cycle`, `testbench`, `behavioral equivalence`, `read-during-write semantics`

5. **`validation`**
   - All remaining rows

Fallback rule:

- If classification is uncertain between `validation` and any other coverage domain, choose `validation`.

### Applicable Context Assignment

Assign `Applicable Contexts` using only the requirement text. Use these fixed labels:

`module-name`, `port-name`, `wire-name`, `register-name`, `const-name`, `config-name`, `latch-name`, `mem-name`, `mux-name`, `instance-name`, `async-expr`, `sync-expr`, `new-binding`, `check-expr`, `feature-cond`, `template-param`

Assignment rules:

1. If `Coverage Domain` is `cross-reference`, `non-actionable`, `golden`, or `simulation`, use `N/A`.
2. If the requirement is about identifier naming, reserved identifiers, duplicate identifiers, or keyword-as-identifier usage across declarations, use:
   `module-name, port-name, wire-name, register-name, const-name, config-name, latch-name, mem-name, mux-name, instance-name`
3. If the requirement text explicitly scopes itself to one or more tracked contexts, list exactly those contexts:
   - `ASYNCHRONOUS` / async RHS / async expression -> `async-expr`
   - `SYNCHRONOUS` / sync RHS / sync expression -> `sync-expr`
   - `@new` binding / instance binding / port binding -> `new-binding`
   - `@check` expression -> `check-expr`
   - `@feature` condition -> `feature-cond`
   - template parameter -> `template-param`
4. If multiple tracked contexts are named, list all of them in the order shown above.
5. If the requirement does not vary across the tracked contexts, or the requirement text does not identify a tracked context set, use `N/A`.

Fallback rules:

- If the row is `validation` and the requirement appears context-sensitive but the exact tracked context cannot be determined from the single line alone, prefer the nearest explicit tracked context named in the surrounding syntax or allowed-context text.
- If the row is `validation` and no such surrounding explicit tracked context exists, use `N/A` rather than inventing contexts.

### Coverage Keys Assignment

Assign `Coverage Keys` to help downstream test mapping find existing semantic coverage across sections. These keys are **not** coverage judgments. They are compact search hints extracted from the requirement text and nearby syntax.

Format rules:

- Use a comma-separated list of `type:value` keys.
- Use lowercase kebab-case for every `value`.
- Use only these key types: `construct`, `form`, `behavior`, `diagnostic`, `context`.
- Keep keys compact and literal. Do not write prose sentences.
- Emit 2-6 keys for `validation` rows when possible.
- If `Coverage Domain` is `cross-reference`, `non-actionable`, `golden`, or `simulation`, use `N/A`.

Assignment rules for `validation` rows:

1. Always emit at least one `construct:` key naming the main language construct or feature under discussion.
   - Examples: `construct:mem-sync-read`, `construct:template-apply`, `construct:feature-guard`, `construct:tristate-net`, `construct:identifier-scope`
2. If the row defines a concrete syntax form, emit one or more `form:` keys for that form.
   - Examples: `form:mem-port-addr-assign`, `form:mem-port-data-read`, `form:new-binding`, `form:check-expr`, `form:feature-cond`
3. If the row states an observable semantic rule or allowed/forbidden behavior, emit a `behavior:` key for that behavior.
   - Examples: `behavior:zero-extend-narrow-addr`, `behavior:single-write-per-path`, `behavior:sync-read-requires-receive`, `behavior:nested-feature-forbidden`
4. If the requirement text itself contains a backtick-quoted diagnostic code, emit `diagnostic:<rule-id>` using lowercase kebab-case.
5. If `Applicable Contexts` is not `N/A`, also emit matching `context:` keys for those exact tracked contexts.
6. If multiple keys of the same type apply, list the most specific ones first.

Fallback rules:

- When uncertain, prefer broader but still truthful keys over inventing narrow ones.
- Reuse the nearest surrounding syntax or allowed-form text when the single requirement line is too short to produce a meaningful `construct:` or `form:` key on its own.
- Do not emit placeholder keys like `construct:general-rule` or `behavior:valid`.

## Report Format

```markdown
## §<section> <title>

Source: `<spec-file-relpath>` lines <start>–<end>

| # | Requirement | Category | Coverage Domain | Applicable Contexts | Coverage Keys |
|---|-------------|----------|-----------------|---------------------|---------------|
| 1 | <verbatim spec text> | constraint / diagnostic-mapping / behavioral / enumerated-list / cross-reference / syntax-definition | validation / golden / simulation / non-actionable / cross-reference | context list or N/A | comma-separated `type:value` keys or N/A |
| 2 | ... | ... | ... | ... | ... |

Total: N requirements
```

## Rules

- **Completeness:** Every qualifying line must appear. Missing a requirement is a defect.
- **Determinism:** Two runs on identical spec text must produce identical output — same requirements, same order, same text.
- **No judgment beyond classification.** Do not assess whether requirements are covered, partial, or missing. Only extract and classify.
- **No external input:** Do not read tests, rules.c, or any file other than the specification.
- **Document order:** Requirements appear in the order they occur in the spec text.
- **One section per run:** Only process the single spec section identified at the top of this prompt.
