#!/usr/bin/env python3
"""Source-corpus security review runner.

Runs three security prompts back-to-back per deterministic source shard:
  1. pipeline/prompts/security/1-map-attack-surface.md
  2. pipeline/prompts/security/2-review-findings.md
  3. pipeline/prompts/security/3-generate-issues.md

The runner shards C/C++ source files so interrupted runs can be resumed with
`--start N` or `--start-at TEXT`.

By default, source files come from:
  - compiler/include
  - compiler/src
  - viewer/src

`compiler/src/third_party` is excluded unless `--include-third-party` is used.
The intent is to review project-owned code for memory-safety and related
security bugs, not general CLI-argument validation behavior.

Usage:
    pipeline/scripts/source_corpus_security.py
    pipeline/scripts/source_corpus_security.py --list
    pipeline/scripts/source_corpus_security.py --filter parser
    pipeline/scripts/source_corpus_security.py --start 3
    pipeline/scripts/source_corpus_security.py --start-at "lsp"
    pipeline/scripts/source_corpus_security.py --shard-size 8
    pipeline/scripts/source_corpus_security.py --dry-run
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass

PIPELINE_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
REPO_DIR = os.path.normpath(os.path.join(PIPELINE_DIR, ".."))

PROMPT_STEPS = [
    (
        os.path.join(
            PIPELINE_DIR, "prompts", "security", "1-map-attack-surface.md"
        ),
        "medium",
    ),
    (
        os.path.join(
            PIPELINE_DIR, "prompts", "security", "2-review-findings.md"
        ),
        "high",
    ),
    (
        os.path.join(
            PIPELINE_DIR, "prompts", "security", "3-generate-issues.md"
        ),
        "low",
    ),
]

SECURITY_DIR = os.path.join(REPO_DIR, "security-audit")
TMP_FILE = "/tmp/security_findings.md"
TARGET_PLACEHOLDER = "<SECURITY_REVIEW_TARGET>"
FILES_PLACEHOLDER = "<SECURITY_FILE_LIST>"
DEFAULT_ROOTS = [
    os.path.join(REPO_DIR, "compiler", "include"),
    os.path.join(REPO_DIR, "compiler", "src"),
    os.path.join(REPO_DIR, "viewer", "src"),
]
SOURCE_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
EXCLUDED_DIR_NAMES = {"build", "dist", "out", "target", "node_modules", ".git"}


@dataclass(frozen=True)
class SecurityTarget:
    index: int
    paths: tuple[str, ...]

    def label(self) -> str:
        start = os.path.relpath(self.paths[0], REPO_DIR)
        end = os.path.relpath(self.paths[-1], REPO_DIR)
        return (
            f"shard {self.index:03d} ({len(self.paths)} files): "
            f"{start} … {end}"
        )

    def prompt_target(self) -> str:
        return self.label()

    def prompt_files(self) -> str:
        return "\n".join(
            f"- `{os.path.relpath(path, REPO_DIR)}`" for path in self.paths
        )

    def audit_filename(self) -> str:
        first = os.path.splitext(os.path.basename(self.paths[0]))[0]
        safe = re.sub(r"[^\w]+", "_", first).strip("_") or "shard"
        return f"{self.index:03d}_{safe}.md"


def is_excluded_dir(path: str, include_third_party: bool) -> bool:
    parts = set(os.path.normpath(path).split(os.sep))
    if EXCLUDED_DIR_NAMES & parts:
        return True
    if "old" in parts:
        return True
    if not include_third_party and "third_party" in parts:
        return True
    return False


def discover_source_files(
    roots: list[str], include_third_party: bool = False
) -> list[str]:
    files: list[str] = []
    for root in roots:
        if not os.path.exists(root):
            continue
        for dirpath, dirnames, filenames in os.walk(root):
            dirnames[:] = [
                dirname
                for dirname in sorted(dirnames)
                if not is_excluded_dir(
                    os.path.join(dirpath, dirname), include_third_party
                )
            ]
            if is_excluded_dir(dirpath, include_third_party):
                dirnames[:] = []
                continue
            for filename in sorted(filenames):
                ext = os.path.splitext(filename)[1].lower()
                if ext not in SOURCE_EXTENSIONS:
                    continue
                files.append(os.path.join(dirpath, filename))
    return sorted(set(files))


def build_targets(
    files: list[str],
    shard_size: int,
    filter_pattern: str | None = None,
    start_at: str | None = None,
    start: int | None = None,
) -> tuple[list[SecurityTarget], int, int]:
    targets = [
        SecurityTarget(index=idx, paths=tuple(files[i : i + shard_size]))
        for idx, i in enumerate(range(0, len(files), shard_size), start=1)
    ]

    if filter_pattern:
        lowered = filter_pattern.lower()
        targets = [
            target
            for target in targets
            if lowered in target.label().lower()
            or any(lowered in os.path.relpath(path, REPO_DIR).lower() for path in target.paths)
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
                "shard(s); no shards will run.",
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
                or any(
                    lowered in os.path.relpath(path, REPO_DIR).lower()
                    for path in target.paths
                )
            ),
            None,
        )
        if idx is None:
            print(
                f"Warning: --start-at '{start_at}' not found, running all "
                "matched shards.",
                file=sys.stderr,
            )
        else:
            start_offset += idx
            targets = targets[idx:]

    return targets, start_offset, total_matched


def load_prompt(target: SecurityTarget, prompt_file: str) -> str:
    with open(prompt_file, "r") as f:
        prompt = f.read()

    if TARGET_PLACEHOLDER not in prompt:
        print(
            f"Warning: prompt template at {prompt_file} has no "
            f"{TARGET_PLACEHOLDER} placeholder; running it unchanged.",
            file=sys.stderr,
        )
    else:
        prompt = prompt.replace(TARGET_PLACEHOLDER, target.prompt_target())

    if FILES_PLACEHOLDER not in prompt:
        print(
            f"Warning: prompt template at {prompt_file} has no "
            f"{FILES_PLACEHOLDER} placeholder; running it unchanged.",
            file=sys.stderr,
        )
        return prompt
    return prompt.replace(FILES_PLACEHOLDER, target.prompt_files())


PIPELINE_PREAMBLE = (
    "This is a non-interactive pipeline run. "
    "Do not greet. Do not present options. Do not ask questions. "
    "Do not follow CLAUDE.md greeting or interaction rules. "
    "Execute the task in this prompt immediately.\n\n"
)


def run_claude(
    prompt: str, dry_run: bool = False, effort: str = "high"
) -> int:
    cmd = [
        "claude",
        "-p",
        PIPELINE_PREAMBLE + prompt,
        "--effort",
        effort,
        "--allowedTools",
        "Read,Edit,Write,Glob,Grep,Bash",
    ]

    if dry_run:
        print("  [dry-run] would run: claude -p <prompt> --allowedTools ...")
        return 0

    result = subprocess.run(cmd, capture_output=False)
    return result.returncode


def run_codex(prompt: str, dry_run: bool = False) -> int:
    cmd = [
        "codex",
        "exec",
        PIPELINE_PREAMBLE + prompt,
    ]

    if dry_run:
        print("  [dry-run] would run: codex exec <prompt>")
        return 0

    result = subprocess.run(cmd, capture_output=False)
    return result.returncode


def run_agent(
    prompt: str,
    cli: str,
    dry_run: bool = False,
    effort: str = "high",
) -> int:
    if cli == "claude":
        return run_claude(prompt, dry_run=dry_run, effort=effort)
    if cli == "codex":
        return run_codex(prompt, dry_run=dry_run)
    raise ValueError(f"unsupported cli: {cli}")


def save_review(target: SecurityTarget, dry_run: bool = False) -> str | None:
    os.makedirs(SECURITY_DIR, exist_ok=True)
    dest = os.path.join(SECURITY_DIR, target.audit_filename())
    if dry_run:
        print(f"  [dry-run] would move {TMP_FILE} -> {dest}")
        return dest
    if not os.path.exists(TMP_FILE):
        print(f"  warning: {TMP_FILE} not found, skipping save")
        return None
    shutil.copy2(TMP_FILE, dest)
    os.remove(TMP_FILE)
    print(f"  saved -> {os.path.relpath(dest, REPO_DIR)}")
    return dest


def run_steps(
    target: SecurityTarget,
    prompt_steps: list[tuple[str, str]],
    cli: str,
    dry_run: bool = False,
) -> bool:
    for step_idx, (prompt_file, effort) in enumerate(prompt_steps, start=1):
        step_name = os.path.basename(prompt_file)
        print(f"  step {step_idx}/{len(prompt_steps)}: {step_name}")

        prompt = load_prompt(target, prompt_file)
        rc = run_agent(prompt, cli=cli, dry_run=dry_run, effort=effort)
        if rc != 0:
            print(f"  step {step_idx} FAILED (exit code {rc})")
            return False
        print(f"  step {step_idx} OK")
    return True


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Run the source security review prompts against deterministic "
            "source-file shards."
        ),
    )
    parser.add_argument(
        "--root",
        dest="roots",
        action="append",
        default=[],
        help="Add a source root to review. Defaults to compiler/include, "
        "compiler/src, and viewer/src.",
    )
    parser.add_argument(
        "--filter",
        type=str,
        default=None,
        help="Only review shards whose label or file list contains this "
        "substring.",
    )
    parser.add_argument(
        "--start",
        type=int,
        default=None,
        metavar="N",
        help="Start at the N-th shard (1-based) in the matched/sorted list. "
        "Mutually exclusive with --start-at.",
    )
    parser.add_argument(
        "--start-at",
        type=str,
        default=None,
        help="Skip shards until the first label or file path containing this "
        "substring. Mutually exclusive with --start.",
    )
    parser.add_argument(
        "--shard-size",
        type=int,
        default=12,
        metavar="N",
        help="Number of source files per shard (default: 12).",
    )
    parser.add_argument(
        "--include-third-party",
        action="store_true",
        help="Include project vendored third-party source directories.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the planned invocations without running the selected CLI.",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="List the matched source shards and exit.",
    )
    cli_group = parser.add_mutually_exclusive_group()
    cli_group.add_argument(
        "--codex",
        dest="cli",
        action="store_const",
        const="codex",
        help="Run prompts with Codex CLI (default).",
    )
    cli_group.add_argument(
        "--claude",
        "--cluade",
        dest="cli",
        action="store_const",
        const="claude",
        help="Run prompts with Claude CLI.",
    )
    parser.set_defaults(cli="codex")
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if args.start is not None and args.start_at is not None:
        print(
            "error: --start and --start-at are mutually exclusive.",
            file=sys.stderr,
        )
        return 2
    if args.shard_size < 1:
        print("error: --shard-size must be >= 1.", file=sys.stderr)
        return 2

    for prompt_file, _ in PROMPT_STEPS:
        if not os.path.exists(prompt_file):
            print(
                f"error: prompt not found: {prompt_file}", file=sys.stderr
            )
            return 2

    roots = [
        root if os.path.isabs(root) else os.path.join(REPO_DIR, root)
        for root in (args.roots or DEFAULT_ROOTS)
    ]
    files = discover_source_files(
        roots, include_third_party=args.include_third_party
    )
    if not files:
        print("No source files matched.", file=sys.stderr)
        return 1

    targets, start_offset, total_matched = build_targets(
        files,
        shard_size=args.shard_size,
        filter_pattern=args.filter,
        start_at=args.start_at,
        start=args.start,
    )
    if not targets:
        print("No source shards matched.", file=sys.stderr)
        return 1

    if args.list:
        for i, target in enumerate(targets, start=start_offset + 1):
            print(f"{i:3d}  {target.label()}")
        print(f"\n{len(targets)} shard(s) (of {total_matched} matched)")
        return 0

    step_names = [os.path.basename(p) for p, _ in PROMPT_STEPS]
    print(f"CLI: {args.cli}")
    print(f"Security steps: {' -> '.join(step_names)}")

    if start_offset > 0:
        print(
            f"Found {len(targets)} source shard(s) to review "
            f"(shards {start_offset + 1}-{total_matched} of "
            f"{total_matched}).\n"
        )
    else:
        print(f"Found {len(targets)} source shard(s) to review.\n")

    results: dict[str, list[str]] = {"pass": [], "fail": []}

    for i, target in enumerate(targets, start=start_offset + 1):
        label = target.label()
        print(f"[{i}/{total_matched}] {label}")

        if not args.dry_run and os.path.exists(TMP_FILE):
            os.remove(TMP_FILE)

        passed = run_steps(
            target,
            PROMPT_STEPS,
            cli=args.cli,
            dry_run=args.dry_run,
        )

        if passed:
            review_path = save_review(target, dry_run=args.dry_run)
            if review_path:
                results["pass"].append(label)
                print(f"  -> ALL STEPS OK\n")
            else:
                results["fail"].append(label)
                print(f"  -> FAILED (save)\n")
        else:
            results["fail"].append(label)
            print(f"  -> FAILED\n")

    print("=" * 60)
    print(
        f"Done. {len(results['pass'])} passed, "
        f"{len(results['fail'])} failed."
    )
    if results["fail"]:
        print("\nFailed:")
        for label in results["fail"]:
            print(f"  - {label}")

    return 1 if results["fail"] else 0


if __name__ == "__main__":
    sys.exit(main())
