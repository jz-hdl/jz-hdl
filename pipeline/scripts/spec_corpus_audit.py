#!/usr/bin/env python3
"""Spec-corpus audit runner: drive 7-spec-corpus-audit.md per spec section.

The audit prompt at `pipeline/prompts/tests/7-spec-corpus-audit.md` compares
the validation corpus directly against the specification. It intentionally does
not read test plans. This runner shards the work by numbered spec section so
rate-limit interruptions can be resumed with `--start N` or `--start-at TEXT`.

By default, targets are numbered leaf sections from `specification/*.md`: a
numbered heading is skipped when it has numbered child headings underneath it.
Unnumbered child headings do not suppress the numbered parent.

Usage:
    pipeline/scripts/spec_corpus_audit.py
    pipeline/scripts/spec_corpus_audit.py --spec jz-hdl
    pipeline/scripts/spec_corpus_audit.py --filter "7.5"
    pipeline/scripts/spec_corpus_audit.py --start 32
    pipeline/scripts/spec_corpus_audit.py --start-at "MEM Block"
    pipeline/scripts/spec_corpus_audit.py --list
    pipeline/scripts/spec_corpus_audit.py --dry-run
"""

from __future__ import annotations

import argparse
import glob
import os
import re
import subprocess
import sys
from dataclasses import dataclass

PIPELINE_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
REPO_DIR = os.path.normpath(os.path.join(PIPELINE_DIR, ".."))
SPEC_DIR = os.path.join(REPO_DIR, "specification")
PROMPT_FILE = os.path.join(
    PIPELINE_DIR, "prompts", "tests", "7-spec-corpus-audit.md"
)
PLACEHOLDER = "<SPEC_SECTION_TARGET>"

HEADING_RE = re.compile(r"^(#{2,4})\s+(.+?)\s*$")
NUMBERED_RE = re.compile(r"^(\d+(?:\.\d+)*)\b[.)]?\s*(.*)$")


@dataclass(frozen=True)
class SpecTarget:
    path: str
    lineno: int
    level: int
    section: str
    title: str
    raw_heading: str
    has_numbered_child: bool = False

    @property
    def relpath(self) -> str:
        return os.path.relpath(self.path, REPO_DIR)

    def label(self) -> str:
        title = f" {self.title}" if self.title else ""
        return f"{self.relpath}:{self.lineno}: §{self.section}{title}"

    def prompt_target(self) -> str:
        return (
            f"{self.relpath}:{self.lineno}: "
            f"{'#' * self.level} {self.raw_heading}"
        )


def parse_spec_file(path: str) -> list[SpecTarget]:
    """Extract numbered H2-H4 headings from a spec file."""
    headings: list[SpecTarget] = []
    with open(path, "r") as f:
        for lineno, line in enumerate(f, start=1):
            match = HEADING_RE.match(line)
            if not match:
                continue
            marker, raw_heading = match.groups()
            number_match = NUMBERED_RE.match(raw_heading)
            if not number_match:
                continue
            section, title = number_match.groups()
            headings.append(
                SpecTarget(
                    path=path,
                    lineno=lineno,
                    level=len(marker),
                    section=section,
                    title=title.strip(),
                    raw_heading=raw_heading,
                )
            )

    targets: list[SpecTarget] = []
    for i, heading in enumerate(headings):
        has_numbered_child = False
        for later in headings[i + 1 :]:
            if later.level <= heading.level:
                break
            if later.level > heading.level:
                has_numbered_child = True
                break
        targets.append(
            SpecTarget(
                path=heading.path,
                lineno=heading.lineno,
                level=heading.level,
                section=heading.section,
                title=heading.title,
                raw_heading=heading.raw_heading,
                has_numbered_child=has_numbered_child,
            )
        )
    return targets


def get_spec_targets(
    spec_filter: str | None = None,
    filter_pattern: str | None = None,
    start_at: str | None = None,
    start: int | None = None,
    include_parents: bool = False,
) -> tuple[list[SpecTarget], int, int]:
    """Find spec audit targets, optionally filtered and sliced.

    Returns `(targets, start_offset, total_matched)` where `total_matched` is
    the number of targets after filters but before `--start` or `--start-at`.
    """
    paths = sorted(glob.glob(os.path.join(SPEC_DIR, "*.md")))
    if spec_filter:
        paths = [
            path
            for path in paths
            if spec_filter.lower() in os.path.basename(path).lower()
        ]

    targets: list[SpecTarget] = []
    for path in paths:
        targets.extend(parse_spec_file(path))

    if not include_parents:
        targets = [target for target in targets if not target.has_numbered_child]

    if filter_pattern:
        lowered = filter_pattern.lower()
        targets = [
            target
            for target in targets
            if lowered in target.label().lower()
            or lowered in target.prompt_target().lower()
        ]

    total_matched = len(targets)
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
                f"Warning: --start={start} exceeds {total_matched} matched "
                "section(s); no sections will run.",
                file=sys.stderr,
            )
            return [], start - 1, total_matched
        start_offset = start - 1
        targets = targets[start_offset:]

    if start_at:
        lowered = start_at.lower()
        idx = next(
            (
                i
                for i, target in enumerate(targets)
                if lowered in target.label().lower()
                or lowered in target.prompt_target().lower()
            ),
            None,
        )
        if idx is None:
            print(
                f"Warning: --start-at '{start_at}' not found, running all "
                "matched sections.",
                file=sys.stderr,
            )
        else:
            start_offset += idx
            targets = targets[idx:]

    return targets, start_offset, total_matched


def load_prompt(target: SpecTarget) -> str:
    """Load the audit prompt and substitute the spec section target."""
    with open(PROMPT_FILE, "r") as f:
        prompt = f.read()

    if PLACEHOLDER not in prompt:
        print(
            f"Warning: prompt template at {PROMPT_FILE} has no "
            f"{PLACEHOLDER} placeholder; running it unchanged.",
            file=sys.stderr,
        )
        return prompt
    return prompt.replace(PLACEHOLDER, target.prompt_target())


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
            "Run pipeline/prompts/tests/7-spec-corpus-audit.md against "
            "numbered spec sections, one claude invocation per section."
        ),
    )
    parser.add_argument(
        "--spec",
        type=str,
        default=None,
        help="Only audit spec files whose basename contains this substring "
        "(e.g. 'jz-hdl', 'simulation', 'testbench').",
    )
    parser.add_argument(
        "--filter",
        type=str,
        default=None,
        help="Only audit sections whose label contains this substring "
        "(e.g. '7.5' or 'Memory Initialization').",
    )
    parser.add_argument(
        "--start",
        type=int,
        default=None,
        metavar="N",
        help="Start at the N-th section (1-based) in the matched/sorted list. "
        "Mutually exclusive with --start-at.",
    )
    parser.add_argument(
        "--start-at",
        type=str,
        default=None,
        help="Skip sections until the first label containing this substring. "
        "Mutually exclusive with --start.",
    )
    parser.add_argument(
        "--include-parents",
        action="store_true",
        help="Include numbered parent headings even when numbered child "
        "headings exist beneath them.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the planned invocations without running claude.",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="List the matched spec sections and exit.",
    )
    args = parser.parse_args()

    if args.start is not None and args.start_at is not None:
        print(
            "error: --start and --start-at are mutually exclusive.",
            file=sys.stderr,
        )
        return 2

    if not os.path.exists(PROMPT_FILE):
        print(f"error: audit prompt not found: {PROMPT_FILE}", file=sys.stderr)
        return 2

    targets, start_offset, total_matched = get_spec_targets(
        args.spec,
        args.filter,
        args.start_at,
        args.start,
        args.include_parents,
    )
    if not targets:
        print("No spec sections matched.", file=sys.stderr)
        return 1

    if args.list:
        for i, target in enumerate(targets, start=start_offset + 1):
            print(f"{i:3d}  {target.label()}")
        print(f"\n{len(targets)} section(s) (of {total_matched} matched)")
        return 0

    if start_offset > 0:
        print(
            f"Found {len(targets)} spec section(s) to audit "
            f"(sections {start_offset + 1}-{total_matched} of "
            f"{total_matched}).\n"
        )
    else:
        print(f"Found {len(targets)} spec section(s) to audit.\n")

    results: dict[str, list[str]] = {"pass": [], "fail": []}

    for i, target in enumerate(targets, start=start_offset + 1):
        label = target.label()
        print(f"[{i}/{total_matched}] {label}")

        prompt = load_prompt(target)
        rc = run_claude(prompt, dry_run=args.dry_run)

        if rc == 0:
            results["pass"].append(label)
            print("  -> OK\n")
        else:
            results["fail"].append(label)
            print(f"  -> FAILED (exit code {rc})\n")

    print("=" * 60)
    print(
        f"Done. {len(results['pass'])} audited, "
        f"{len(results['fail'])} failed."
    )
    if results["fail"]:
        print("\nFailed:")
        for label in results["fail"]:
            print(f"  - {label}")

    return 1 if results["fail"] else 0


if __name__ == "__main__":
    sys.exit(main())
