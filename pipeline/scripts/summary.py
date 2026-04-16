#!/usr/bin/env python3
"""Summary runner: drive 6-summary.md in a single claude invocation.

The prompt at `pipeline/prompts/tests/6-summary.md` reads
`compiler/tests/issues.md`, `compiler/tests/sweep.md`, and
`compiler/tests/not_tested.md`, reconciles them, and overwrites
`compiler/tests/issues.md` with a flat per-plan bullet list of the
remaining issues. It is a one-shot job — there is no per-plan loop or
resume support, unlike `context_sweep.py` / `happy_sweep.py`.

Usage:
    pipeline/scripts/summary.py            # run the summary prompt
    pipeline/scripts/summary.py --dry-run  # print the invocation only
"""

import argparse
import os
import subprocess
import sys

PIPELINE_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
PROMPT_FILE = os.path.join(PIPELINE_DIR, "prompts", "tests", "6-summary.md")


def load_prompt() -> str:
    with open(PROMPT_FILE, "r") as f:
        return f.read()


def run_claude(prompt: str, dry_run: bool = False) -> int:
    """Invoke `claude -p` with the given prompt and return its exit code."""
    cmd = [
        "claude",
        "-p",
        prompt,
        "--allowedTools",
        "Read,Edit,Write,Glob,Grep,Bash",
    ]

    if dry_run:
        print("[dry-run] would run: claude -p <prompt> --allowedTools ...")
        return 0

    result = subprocess.run(cmd, capture_output=False)
    return result.returncode


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Run pipeline/prompts/tests/6-summary.md in a single claude "
            "invocation to reconcile compiler/tests/{issues,sweep,not_tested}.md "
            "and overwrite compiler/tests/issues.md with a flat remaining-issues "
            "list."
        ),
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the planned invocation without running claude.",
    )
    args = parser.parse_args()

    if not os.path.exists(PROMPT_FILE):
        print(f"error: summary prompt not found: {PROMPT_FILE}", file=sys.stderr)
        return 2

    prompt = load_prompt()
    rc = run_claude(prompt, dry_run=args.dry_run)

    if rc == 0:
        print("Done.")
        return 0
    print(f"FAILED (exit code {rc})", file=sys.stderr)
    return rc


if __name__ == "__main__":
    sys.exit(main())
