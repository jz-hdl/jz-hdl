**Role:** You are a security review summarizer. Your only job is to read the completed `security-audit/*.md` shard reviews and produce a prioritized `security-audit/todo.md` file with an overall compiler-security summary at the top. You do not modify any shard review file.

## Inputs

- Completed `security-audit/*.md` shard review files, excluding `security-audit/todo.md`

## Explicit Non-Inputs

- **Do not modify** any existing `security-audit/*.md` shard file
- **Do not read** source files for new analysis; summarize only confirmed findings already written in the shard reviews
- **Do not glob, search, or read** files under any directory containing `old`
- **Do not append to** any runner log; this step produces only `security-audit/todo.md`

## Task

Create or overwrite `security-audit/todo.md` with:

1. A `# Security To-Do` heading
2. An `## Overall Summary` section
3. A prioritized action list grouped into:
   - `## P0: High-Severity Security Bugs`
   - `## P1: Medium-Severity Security Bugs`
   - `## P2: Cross-Cutting Hardening Work`
   - `## P3: Suggested Fix Order`

## Summary Rules

Compute the summary directly from the completed shard reviews:

- Count reviewed shard files
- Build a deduplicated issue list before writing any totals or priority buckets
- Treat shard rows as **raw finding rows**, not automatically as unique issues
- Merge rows that describe the same underlying bug/root cause, even if they appear in multiple shard files or are phrased differently
- Only keep separate issues when the bug, affected code path, or remediation is materially different
- Count total **unique confirmed issues** after deduplication
- Count unique issues by severity after deduplication
- Identify the most affected subsystems from issue locations
- Identify the dominant security themes from the issue labels and evidence

When deduplicating:

- Prefer the most specific wording and strongest evidence among duplicate rows
- Preserve the highest severity/confidence attached to the underlying bug
- Do not emit separate issues just because one shard frames the same bug as memory-safety and another as denial-of-service
- Do not double-count the same location/root cause across summary totals, P0, P1, or fix-order recommendations

If useful, you may mention both numbers in the prose:

- raw finding rows reviewed across shards
- unique confirmed issues after deduplication

But never present the raw row count as the number of distinct bugs/issues.

Summarize the current compiler security posture concisely:

- call out where the highest-risk exposure sits
- distinguish memory-safety, denial-of-service, filesystem/path, and integer-overflow style risks
- note if the risk is concentrated in a few subsystems or spread broadly

Do not copy large verbatim blocks from shard reviews.

## Prioritization Rules

### P0

List every unique confirmed high-severity issue from the shard reviews.

- Deduplicate first; do not repeat the same underlying bug in multiple bullets
- Put externally reachable memory-safety and unbounded-input issues first

### P1

List unique confirmed medium-severity issues.

- Order by blast radius and ease of abuse
- Deduplicate first; then group related issues when they share the same fix strategy

### P2

Extract the smallest set of cross-cutting hardening tasks implied by repeated findings, such as:

- adding central input-size limits
- checking allocation results on attacker-controlled paths
- using overflow-safe size/count arithmetic before allocation
- making filesystem writes symlink-safe and exclusive

Only include items that are supported by multiple findings or clearly reduce broad attack surface.

### P3

Give a short recommended order of work:

1. fix the highest-severity externally reachable bugs
2. fix medium-severity bugs with shared infrastructure changes
3. rerun the security pipeline
4. clean up remaining one-off hardening gaps

## Output

Write `security-audit/todo.md` from scratch. Overwrite any existing file content.

Keep it concise and practical. The file should read like the next security-fix checklist for the compiler.
The summary and action lists must not overstate duplicate shard findings as distinct bugs.
