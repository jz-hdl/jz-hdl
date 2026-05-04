For doxygen review target: `<DOXYGEN_REVIEW_TARGET>`

Files in scope:
<DOXYGEN_FILE_LIST>

**Role:** You are a source documentation maintainer. Your job is to review and fix API-facing doxygen in the listed files only. You must verify existing comments for accuracy and readability, add missing doxygen where required, and normalize declaration layout for file-local functions without changing implementation behavior.

## Scope

- Edit only the files listed above.
- You may read directly related repo files when needed to verify whether an existing comment is accurate or to find the declaration corresponding to a definition.
- Do not edit unlisted files, even if you discover a related declaration elsewhere. Record that as a follow-up in the shard report instead.

## Required Outcomes

For every listed header/source file:

- Add a file-level doxygen block with `@file` and an accurate `@brief` if missing.
- If an existing file-level comment is weak, stale, or non-doxygen API prose, rewrite it into clear doxygen.

For every listed function declaration:

- Ensure it has doxygen.
- Use full API docs: `@brief`, `@param` for every parameter with a short useful description, and `@return` when the function returns a value other than `void`.
- Verify existing doxygen is accurate, readable, and aligned with the implementation. Rewrite it when it is stale, vague, or misleading.

For file-local private functions in listed `.c` / `.cc` / `.cpp` / `.cxx` files:

- Every file-local function must have a declaration near the top of its file, before the first function definition and as early as semantic dependencies permit.
- Put the full doxygen on that declaration, not on the out-of-line definition.
- If a file-local function currently has doxygen on the definition, move or rewrite that documentation onto the declaration and leave the definition without duplicate API doxygen.

For header-only or `static inline` functions whose full definition is the declaration site:

- This is the only exception to the declaration-only rule.
- Put the full doxygen directly on that definition.

For every listed `struct` and `enum`:

- Ensure the type itself has doxygen.
- Ensure every struct field has a readable doxygen trailing comment or equivalent field-level doxygen.
- Ensure every enum member has a readable doxygen trailing comment or equivalent member-level doxygen.

## Preservation Rules

- Do not change implementation behavior.
- Do not remove or rewrite inline code comments inside function bodies or adjacent to logic unless the comment is an API doc block attached to the wrong declaration/definition site.
- Do not add docs for non-function API outside this scope. Globals, macros, typedefs, and unions are out of scope unless they are required only as part of an in-scope struct or enum declaration.
- Keep edits focused. Do not perform style cleanups unrelated to doxygen or file-local declaration placement.

## Declaration Placement Rules

- For file-local declarations, create a dedicated declaration block near the top of the file after includes and required type declarations.
- Preserve valid existing ordering when possible.
- Do not create invalid forward declarations that reference types not yet declared. If a local type definition must come first, place the file-local declarations immediately after that prerequisite block and note the reason in the report if the placement is not literally at the top.

## Cross-File Rules

- If a listed source file defines a public function whose declaration lives in an unlisted header, do not document the definition just to satisfy the rule. Verify accuracy by reading the header if needed, and record any required header fix as a deferred follow-up in the report.
- If both declaration and definition are listed, put or keep the API docs on the declaration only, except for the header-only/static-inline exception above.

## Validation

- If you edit any file under `compiler/`, run:
  - `cmake --build compiler/build`
  - `bash compiler/tests/run_validation.sh`
- If you edit only `viewer/` files, report that no compiler files changed and do not run the compiler validation commands.
- If validation fails, fix the issue before finishing. Do not leave the shard in a failing state.

## Output

Write `<OUTPUT_FILE>` from scratch with a `# Doxygen Review` H1 header and one section for this target.

## Report Format

```markdown
# Doxygen Review

## <target label>

Files reviewed: N
Files changed: N

### Changes

- `path/to/file.h`: one concise line describing the doxygen and declaration changes made.
- `path/to/file.c`: one concise line describing the doxygen and declaration changes made.

### Deferred Follow-Ups

- `path/to/unlisted/header.h`: short reason this related file needs a later shard fix.

### Validation

- `cmake --build compiler/build`
- `bash compiler/tests/run_validation.sh`

Result: pass
```

If there are no deferred follow-ups, write `None.` under that section.

If no files needed changes, still produce the report and explain that the listed files already met the rules.

## Rules

- Be accurate. Do not invent behavior details that the code does not implement.
- Prefer short, readable prose over template noise.
- Keep report entries deterministic and ordered by file path.
- Only process the listed target.
