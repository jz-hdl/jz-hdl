For spec section: `<SPEC_SECTION_TARGET>`

**Role:** You are a Senior SDET auditing the JZ-HDL validation corpus directly against the specification.

## Context

The existing plan-based workflow checks whether validation tests match the test plans. This prompt is different: it deliberately ignores the test plans and audits the validation corpus against the specification itself.

The question for this audit is:

> Looking only at the specification and the existing validation corpus, do the tests correctly and sufficiently validate the specified behavior for this spec section?

This prompt is scoped to **one spec section at a time**. A separate runner (`pipeline/scripts/spec_corpus_audit.py`) invokes it once per spec section so long runs can be resumed with `--start N` or `--start-at <text>`.

## Inputs

- The target spec section named at the top of this prompt.
- `specification/*.md` — the specification is the source of truth. Read the target section first; read related sections only when the target section depends on them.
- `compiler/tests/validation/*.jz` and paired `.out` files — the validation corpus being audited.
- `compiler/src/rules.c` — use only to decode rule IDs, severities, messages, and spec references. It may help classify findings, but it does not override the specification.

## Explicit Non-Inputs

- **Do not read** `pipeline/test_*.md`.
- **Do not read** `pipeline/rule_coverage.md`.
- **Do not read** `compiler/tests/issues.md`.
- **Do not read** `compiler/tests/sweep.md`.
- **Do not use git history** or read deleted git files.
- **Do not read, glob, or search** files under any directory containing `old`.

The point of this audit is to avoid inheriting assumptions from the plan-based pipeline.

## Outputs

Append one dated section to `compiler/tests/spec_corpus_audit.md`. If the file does not exist, create it with a `# Spec Corpus Audit` H1 header.

Do not modify validation files. Do not modify the specification. Do not modify `issues.md`. This is an audit-only prompt.

## Audit Model

For the target spec section:

1. Extract the normative requirements from the specification.
   - Include required behavior, forbidden behavior, allowed syntax, invalid syntax, edge cases, and explicitly stated diagnostics or runtime behavior.
   - Treat words like `must`, `must not`, `shall`, `allowed`, `forbidden`, `valid`, `invalid`, `error`, `warning`, and `runtime error` as strong signals.
   - If the section is descriptive only and has no testable requirement, say so explicitly.

2. Find relevant validation files without using test plans.
   - Prefer files whose names begin with the target section prefix when applicable, e.g. section `7.5` maps to `7_5_*.jz`.
   - Search `.out` files for rule IDs whose `rules.c` entry points at this spec section.
   - Search `.jz` files for constructs, keywords, operators, directives, or file formats described by the spec section.
   - Read candidate `.jz` files directly before counting them as evidence.

3. Compare the corpus to the spec.
   - A requirement is **covered-negative** when at least one test intentionally violates it and expects the correct diagnostic or behavior.
   - A requirement is **covered-happy** when at least one valid test exercises the same construct without diagnostics.
   - A requirement is **covered-runtime** when it is validated through simulation/testbench/runtime output rather than `--lint`.
   - A requirement is **partial** when tests cover only some required contexts, edge cases, widths, modes, or variants.
   - A requirement is **missing** when no relevant test exercises it.
   - A requirement is **mismatch** when a test expects behavior that conflicts with the spec.
   - A requirement is **ambiguous** when the spec is not precise enough to decide what the test should expect.

4. Audit relevant tests for spec correctness.
   - The `.jz` must actually exercise the claimed spec requirement.
   - The `.out` must expect diagnostics only for behavior the spec forbids.
   - Happy-path tests must not silently avoid the construct under audit.
   - Scaffolding diagnostics do not count as spec coverage unless the scaffolding itself is the specified behavior.
   - If compiler output differs from the spec, classify the finding as `compiler-spec-mismatch`, `test-spec-mismatch`, or `spec-ambiguous` based on the evidence.

## Finding Categories

Use these labels exactly:

- `missing-negative` — spec forbids behavior but no negative validation test covers it.
- `missing-happy` — spec allows behavior but no clean valid-form test covers it.
- `partial-coverage` — tests cover the requirement only in some required contexts or edge cases.
- `test-spec-mismatch` — a validation test expects behavior that contradicts the spec.
- `compiler-spec-mismatch` — the compiler appears to accept forbidden behavior or reject allowed behavior, based on a relevant test or direct compiler run.
- `spec-ambiguous` — the spec is too unclear to decide the correct test expectation.
- `not-testable-here` — the requirement cannot be validated by the current validation corpus style; explain what kind of test would be needed.
- `no-testable-requirement` — the target section is descriptive and has no concrete behavior to test.

## Workflow

1. Parse `<SPEC_SECTION_TARGET>` to identify the spec file, section number, section title, and heading line.
2. Read the target spec section from its heading through the next heading of the same or higher level.
3. Read related spec sections only when the target section references or depends on them.
4. Read `compiler/src/rules.c` and collect rule IDs that cite or clearly implement the target spec section.
5. Discover candidate validation files using section-prefix, rule-ID, and construct-keyword searches. Exclude any path containing `old`.
6. Read every candidate `.jz` / `.out` pair that looks relevant. Do not count a file as evidence until you have read it.
7. Build the requirement coverage table.
8. Build mismatch, ambiguity, and not-testable findings.
9. If a potential compiler/spec mismatch can be confirmed by running one existing validation file, run:
   ```
   compiler/build/jz-hdl --info --lint <file>.jz
   ```
   Do not create new tests for this audit. Do not update `.out` files.
10. Append the audit report to `compiler/tests/spec_corpus_audit.md`.

## Report Format

Append exactly one Markdown section:

```markdown
## Spec Corpus Audit: <spec file> §<section> <title> — <YYYY-MM-DD>

### Summary
- Requirements identified:       N
- Relevant validation files read: M
- Covered requirements:          A
- Partial requirements:          B
- Missing requirements:          C
- Mismatches/ambiguities:        D

### Requirement Coverage
| Requirement | Status | Evidence | Finding |
|-------------|--------|----------|---------|
| <short requirement> | covered-negative / covered-happy / covered-runtime / partial / missing / mismatch / ambiguous | <files or -> | <finding label or OK> |

### Test/Spec Mismatches
| File | Test expects | Spec says | Finding |
|------|--------------|-----------|---------|
| <file.jz> | <diagnostic or accepted behavior> | <spec requirement> | test-spec-mismatch / compiler-spec-mismatch |

### Missing or Partial Coverage
| Requirement | Category | Recommended next step |
|-------------|----------|-----------------------|
| <requirement> | missing-negative / missing-happy / partial-coverage / not-testable-here | <specific action> |

### Ambiguous Spec
| Requirement | Ambiguity | Suggested clarification |
|-------------|-----------|-------------------------|
| <requirement> | <why unclear> | <wording or decision needed> |

### Notes
- <short note, or "None.">
```

If the section has no testable requirement, still append a report with one `no-testable-requirement` row and `None.` for mismatch sections.

## Rules

- The specification is the source of truth.
- Do not use test plans as evidence.
- Do not treat `rules.c` as proof the spec is covered; it only explains diagnostics.
- Do not treat filenames as proof of coverage; read the tests.
- Do not fix files in this prompt.
- Prefer concrete file evidence over broad claims.
- Keep the report concise: one row per requirement or issue.
