#!/usr/bin/env python3
"""Context-sweep runner: drive 4-context-sweep.md one test plan at a time.

The sweep prompt at `pipeline/prompts/tests/4-context-sweep.md` is written to
process a single plan's `### Missing Contexts` entries in
`compiler/tests/issues.md`. This runner drives it per plan so a rate-limit
interruption can be resumed with `--start N` the same way `audit_tests.py`
works. The `<TEST_PLAN_FILENAME>` placeholder in the prompt is replaced with
the current plan's basename (without the `.md` suffix) before invocation.

Files are created via the sweep prompt itself; this script only orchestrates
runs and reports pass/fail.

Usage:
    pipeline/scripts/context_sweep.py                       # sweep every plan
    pipeline/scripts/context_sweep.py --filter test_4_     # subset
    pipeline/scripts/context_sweep.py --start 32           # resume at plan 32
    pipeline/scripts/context_sweep.py --start-at test_6_4  # resume by substring
    pipeline/scripts/context_sweep.py --list               # show targets, exit
    pipeline/scripts/context_sweep.py --dry-run            # show invocations
"""

import argparse
import glob
import os
import subprocess
import sys

PIPELINE_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
PROMPT_FILE = os.path.join(PIPELINE_DIR, "prompts", "tests", "4-context-sweep.md")
PLACEHOLDER = "<TEST_PLAN_FILENAME>"


def load_prompt(test_filename: str) -> str:
    """Load the sweep prompt and substitute the test plan filename.

    `test_filename` is the basename with the `.md` suffix (e.g.
    `test_4_3-const.md`). The prompt template contains
    `pipeline/<TEST_PLAN_FILENAME>.md`, so we strip `.md` from the basename
    before substitution to avoid producing `.md.md`.
    """
    with open(PROMPT_FILE, "r") as f:
        prompt = f.read()

    stem = test_filename[:-3] if test_filename.endswith(".md") else test_filename
    if PLACEHOLDER not in prompt:
        print(
            f"Warning: prompt template at {PROMPT_FILE} has no "
            f"{PLACEHOLDER} placeholder; running it unchanged.",
            file=sys.stderr,
        )
        return prompt
    return prompt.replace(PLACEHOLDER, stem)


def get_test_files(
    filter_pattern: str | None = None,
    start_at: str | None = None,
    start: int | None = None,
) -> tuple[list[str], int, int]:
    """Find all `test_*.md` files in `pipeline/`, optionally filtered.

    Returns `(files, start_offset, total_matched)` where:
      - `files` is the plan list to actually run, after filtering and slicing.
      - `start_offset` is the 0-based index of `files[0]` in the full filtered
        list (0 when no `--start`/`--start-at` is applied).
      - `total_matched` is the length of the full filtered list before
        `--start`/`--start-at` slicing. Progress indicators should use
        `[start_offset + i / total_matched]` so the displayed number is the
        value the caller should pass to `--start` to resume at that plan.

    `--filter` is applied first, then `--start` (1-based index into the
    filtered list), then `--start-at` (substring match into the filtered
    list). `--start` and `--start-at` are mutually exclusive; the caller is
    expected to enforce that in argument parsing.
    """
    pattern = os.path.join(PIPELINE_DIR, "test_*.md")
    files = sorted(glob.glob(pattern))
    if filter_pattern:
        files = [f for f in files if filter_pattern in os.path.basename(f)]

    total_matched = len(files)
    start_offset = 0

    if start is not None:
        if start < 1:
            print(
                f"Warning: --start={start} is < 1, treating as 1.",
                file=sys.stderr,
            )
            start = 1
        if start > total_matched:
            print(
                f"Warning: --start={start} exceeds {total_matched} matched file(s); "
                "no plans will run.",
                file=sys.stderr,
            )
            return [], start - 1, total_matched
        start_offset = start - 1
        files = files[start_offset:]

    if start_at:
        idx = next(
            (i for i, f in enumerate(files) if start_at in os.path.basename(f)),
            None,
        )
        if idx is None:
            print(
                f"Warning: --start-at '{start_at}' not found, running all files.",
                file=sys.stderr,
            )
        else:
            start_offset += idx
            files = files[idx:]

    return files, start_offset, total_matched


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
        print("  [dry-run] would run: claude -p <prompt> --allowedTools ...")
        return 0

    result = subprocess.run(cmd, capture_output=False)
    return result.returncode


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Run pipeline/prompts/tests/4-context-sweep.md against each test "
            "plan, one claude invocation per plan, so rate-limit interruptions "
            "can be resumed with --start N."
        ),
    )
    parser.add_argument(
        "--filter",
        type=str,
        default=None,
        help="Only sweep test files whose basename contains this substring "
        "(e.g. 'test_4_1').",
    )
    parser.add_argument(
        "--start",
        type=int,
        default=None,
        metavar="N",
        help="Start at the N-th plan (1-based) in the matched/sorted list. "
        "Useful for resuming after hitting a rate limit; pair with --filter "
        "if needed. Mutually exclusive with --start-at.",
    )
    parser.add_argument(
        "--start-at",
        type=str,
        default=None,
        help="Skip plans until the first basename containing this substring "
        "(e.g. 'test_4_10'). Mutually exclusive with --start.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the planned invocations without running claude.",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="List the matched test plans and exit.",
    )
    args = parser.parse_args()

    if args.start is not None and args.start_at is not None:
        print(
            "error: --start and --start-at are mutually exclusive.",
            file=sys.stderr,
        )
        return 2

    if not os.path.exists(PROMPT_FILE):
        print(f"error: sweep prompt not found: {PROMPT_FILE}", file=sys.stderr)
        return 2

    test_files, start_offset, total_matched = get_test_files(
        args.filter, args.start_at, args.start
    )
    if not test_files:
        print("No test_*.md files matched.", file=sys.stderr)
        return 1

    if args.list:
        for i, f in enumerate(test_files, start=start_offset + 1):
            print(f"{i:3d}  {os.path.basename(f)}")
        print(f"\n{len(test_files)} file(s) (of {total_matched} matched)")
        return 0

    if start_offset > 0:
        print(
            f"Found {len(test_files)} test plan(s) to sweep "
            f"(plans {start_offset + 1}-{total_matched} of {total_matched}).\n"
        )
    else:
        print(f"Found {len(test_files)} test plan(s) to sweep.\n")

    results: dict[str, list[str]] = {"pass": [], "fail": []}

    for i, test_path in enumerate(test_files, start=start_offset + 1):
        test_filename = os.path.basename(test_path)
        print(f"[{i}/{total_matched}] {test_filename}")

        prompt = load_prompt(test_filename)
        rc = run_claude(prompt, dry_run=args.dry_run)

        if rc == 0:
            results["pass"].append(test_filename)
            print("  -> OK\n")
        else:
            results["fail"].append(test_filename)
            print(f"  -> FAILED (exit code {rc})\n")

    print("=" * 60)
    print(
        f"Done. {len(results['pass'])} swept, {len(results['fail'])} failed."
    )
    if results["fail"]:
        print("\nFailed:")
        for f in results["fail"]:
            print(f"  - {f}")

    return 1 if results["fail"] else 0


if __name__ == "__main__":
    sys.exit(main())
