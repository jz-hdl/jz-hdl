**Role:** You are an audit summarizer. Your only job is to read the completed `audit/runner.log` and produce a prioritized `audit/todo.md` file with an overall summary at the top. You do not modify any audit section files and you do not append to the runner log.

## Inputs

- `audit/runner.log` — the completed audit log

## Explicit Non-Inputs

- **Do not modify** `audit/runner.log`
- **Do not read** specification files unless a log entry explicitly names one
- **Do not read** `pipeline/test_*.md`, `pipeline/rule_coverage.md`, `compiler/tests/issues.md`, `compiler/tests/sweep.md`
- **Do not read, glob, or search** files under any directory containing `old`
- **Do not edit** any `audit/*.md` section file other than `audit/todo.md`

## Task

Create or overwrite `audit/todo.md` with:

1. An `# Audit To-Do` heading
2. An `## Overall Summary` section
3. A prioritized action list grouped into:
   - `## P0: Real Compiler And Spec Blockers`
   - `## P1: Large Audit-Noise Buckets To Clean Next`
   - `## P2: Audit-Taxonomy Cleanup Rules To Apply`
   - `## P3: Suggested Execution Order`

## Summary Rules

Compute the summary directly from `audit/runner.log`:

- Count total entries
- Count `TEST-ISSUE`, `COMPILER-BUG`, and `SPECIFICATION-BUG` entries
- Identify the dominant audit-noise patterns from the log text
- Identify the highest-volume files by number of log entries

Use concise bullets. Do not copy large verbatim blocks from the log.

## Prioritization Rules

### P0

List every real compiler/spec blocker from the log:

- Every `[COMPILER-BUG]` entry
- Every `[SPECIFICATION-BUG]` entry

Compress duplicates into one bullet when they are the same underlying problem.

### P1

List the largest noisy audit clusters to clean next:

- prioritize files with the most `TEST-ISSUE` entries
- prefer clusters dominated by stale coverage, wrong ownership, or descriptive/example-only rows

### P2

Summarize recurring audit-taxonomy cleanup patterns suggested by the log:

- stale/already-covered rows
- descriptive or umbrella rows
- wrong rule ownership
- impossible context asks
- non-validation-observable asks
- mixed rows that bundle real bugs with audit noise

### P3

Give a short recommended order of work:

1. real compiler/spec blockers
2. rerun audit after those fixes
3. largest noisy clusters
4. remaining taxonomy cleanup

## Output

Write `audit/todo.md` from scratch. Overwrite any existing file content.

Keep it concise and practical. The file should be readable as a working checklist for the next audit pass.
