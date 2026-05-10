#!/usr/bin/env python3
"""Unified config-driven pipeline runner for named repo workflows."""

from __future__ import annotations

import argparse
import glob
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from typing import Any, Sequence

PIPELINE_DIR = os.path.normpath(os.path.dirname(__file__))
REPO_DIR = os.path.normpath(os.path.join(PIPELINE_DIR, ".."))

PIPELINE_PREAMBLE = (
    "This is a non-interactive pipeline run. "
    "Do not greet. Do not present options. Do not ask questions. "
    "Do not follow CLAUDE.md greeting or interaction rules. "
    "Execute the task in this prompt immediately.\n\n"
)

HEADING_RE = re.compile(r"^(#{2,6})\s+(.+?)\s*$")
NUMBERED_RE = re.compile(r"^(\d+(?:\.\d+)*)\b[.)]?\s*(.*)$")
PROMPT_STEP_RE = re.compile(r"^(\d+)-.+\.md$")

COMMON_OPTION_NAMES = {
    "filter",
    "start",
    "end",
    "start_at",
    "dry_run",
    "list",
}
OPTION_DEFAULTS: dict[str, Any] = {
    "filter": None,
    "start": None,
    "end": None,
    "start_at": None,
    "dry_run": False,
    "list": False,
    "spec": None,
    "include_parents": False,
    "roots": [],
    "shard_size": 12,
    "include_third_party": False,
}
OPTION_FLAG_NAMES = {
    "roots": "root",
}
OPTION_PIPELINE_NAMES = {
    "spec": "audit",
    "include_parents": "audit",
    "roots": "security or doxygen",
    "shard_size": "security or doxygen",
    "include_third_party": "security or doxygen",
}


@dataclass(frozen=True)
class StepConfig:
    path: str
    effort: str
    kind: str
    append_template: str | None = None


@dataclass(frozen=True)
class PostStepConfig:
    glob_pattern: str
    min_numeric_prefix: int
    effort: str
    run_when: str


@dataclass(frozen=True)
class PipelineConfig:
    name: str
    description: str
    default_cli: str
    output_dir: str
    placeholders: dict[str, str]
    post_prompt_vars: dict[str, str]
    options: frozenset[str]
    targeting: dict[str, Any]
    steps: tuple[StepConfig, ...]
    post_steps: tuple[PostStepConfig, ...]


@dataclass(frozen=True)
class ResolvedTarget:
    label: str
    output_path: str
    prompt_vars: dict[str, str]
    match_texts: tuple[str, ...]


@dataclass(frozen=True)
class SpecHeading:
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


@dataclass(frozen=True)
class SecurityShard:
    index: int
    paths: tuple[str, ...]

    def label(self) -> str:
        start = os.path.relpath(self.paths[0], REPO_DIR)
        end = os.path.relpath(self.paths[-1], REPO_DIR)
        return (
            f"shard {self.index:03d} ({len(self.paths)} files): "
            f"{start} ... {end}"
        )

    def prompt_files(self) -> str:
        return "\n".join(
            f"- `{os.path.relpath(path, REPO_DIR)}`" for path in self.paths
        )


def repo_path(path: str) -> str:
    return path if os.path.isabs(path) else os.path.join(REPO_DIR, path)


def pipeline_path(path: str) -> str:
    return path if os.path.isabs(path) else os.path.join(PIPELINE_DIR, path)


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


def load_pipeline_config(pipeline_name: str) -> PipelineConfig:
    config_path = os.path.join(PIPELINE_DIR, pipeline_name, "config.json")
    with open(config_path, "r") as f:
        data = json.load(f)

    steps = tuple(
        StepConfig(
            path=repo_path(step["path"]),
            effort=step["effort"],
            kind=step["kind"],
            append_template=step.get("append_template"),
        )
        for step in data["steps"]
    )
    post_steps = tuple(
        PostStepConfig(
            glob_pattern=repo_path(step["glob"]),
            min_numeric_prefix=step["min_numeric_prefix"],
            effort=step["effort"],
            run_when=step["run_when"],
        )
        for step in data.get("post_steps", [])
    )
    return PipelineConfig(
        name=data["name"],
        description=data["description"],
        default_cli=data["default_cli"],
        output_dir=repo_path(data["output_dir"]),
        placeholders=dict(data["placeholders"]),
        post_prompt_vars=dict(data.get("post_prompt_vars", {})),
        options=frozenset(data["options"]),
        targeting=dict(data["targeting"]),
        steps=steps,
        post_steps=post_steps,
    )


def parse_spec_file(
    path: str,
    heading_levels: set[int],
    numbered_only: bool,
) -> list[SpecHeading]:
    headings: list[SpecHeading] = []
    with open(path, "r") as f:
        for lineno, line in enumerate(f, start=1):
            match = HEADING_RE.match(line)
            if not match:
                continue
            marker, raw_heading = match.groups()
            level = len(marker)
            if level not in heading_levels:
                continue
            section = ""
            title = raw_heading
            if numbered_only:
                number_match = NUMBERED_RE.match(raw_heading)
                if not number_match:
                    continue
                section, title = number_match.groups()
            headings.append(
                SpecHeading(
                    path=path,
                    lineno=lineno,
                    level=level,
                    section=section,
                    title=title.strip(),
                    raw_heading=raw_heading,
                )
            )

    targets: list[SpecHeading] = []
    for i, heading in enumerate(headings):
        has_numbered_child = False
        for later in headings[i + 1 :]:
            if later.level <= heading.level:
                break
            if later.level > heading.level:
                has_numbered_child = True
                break
        targets.append(
            SpecHeading(
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


def safe_name(text: str, fallback: str = "") -> str:
    safe = re.sub(r"[^\w]+", "_", text).strip("_")
    return safe or fallback


def resolve_output_dir(
    config: PipelineConfig, args: argparse.Namespace
) -> str:
    output_dir = args.output_dir if args.output_dir is not None else config.output_dir
    return repo_path(output_dir)


def build_output_path(
    config: PipelineConfig, args: argparse.Namespace, fields: dict[str, Any]
) -> str:
    output_format = config.targeting["output_filename"]
    filename = output_format["template"].format(**fields)
    return os.path.join(resolve_output_dir(config, args), filename)


def resolve_post_prompt_vars(
    config: PipelineConfig, args: argparse.Namespace
) -> dict[str, str]:
    output_dir = resolve_output_dir(config, args)
    return {
        key: repo_path(value.format(output_dir=output_dir))
        for key, value in config.post_prompt_vars.items()
    }


def apply_common_filters(
    targets: list[ResolvedTarget],
    *,
    filter_pattern: str | None,
    start_at: str | None,
    start: int | None,
    end: int | None,
    unit_name: str,
) -> tuple[list[ResolvedTarget], int, int]:
    if filter_pattern:
        lowered = filter_pattern.lower()
        targets = [
            target
            for target in targets
            if any(lowered in text.lower() for text in target.match_texts)
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
                f"{unit_name}(s); no {unit_name}(s) will run.",
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
                if any(lowered in text.lower() for text in target.match_texts)
            ),
            None,
        )
        if idx is None:
            print(
                f"Warning: --start-at '{start_at}' not found, running all "
                f"matched {unit_name}(s).",
                file=sys.stderr,
            )
        else:
            start_offset += idx
            targets = targets[idx:]

    if end is not None:
        if end < 1:
            print(
                f"Warning: --end={end} is < 1, treating as 1.",
                file=sys.stderr,
            )
            end = 1
        if end > total_matched:
            print(
                f"Warning: --end={end} exceeds {total_matched} matched "
                f"{unit_name}(s); treating as {total_matched}.",
                file=sys.stderr,
            )
            end = total_matched
        if end < start_offset + 1:
            print(
                f"Warning: --end={end} is before selected start position "
                f"{start_offset + 1}; no {unit_name}(s) will run.",
                file=sys.stderr,
            )
            return [], start_offset, total_matched
        targets = targets[: end - start_offset]

    return targets, start_offset, total_matched


def resolve_spec_targets(
    config: PipelineConfig, args: argparse.Namespace
) -> tuple[list[ResolvedTarget], int, int]:
    discover = config.targeting["discover"]
    group = config.targeting["group"]
    heading_levels = set(discover["heading_levels"])

    paths = sorted(glob.glob(repo_path(discover["glob"])))
    if args.spec:
        paths = [
            path
            for path in paths
            if args.spec.lower() in os.path.basename(path).lower()
        ]

    headings: list[SpecHeading] = []
    for path in paths:
        headings.extend(
            parse_spec_file(
                path,
                heading_levels=heading_levels,
                numbered_only=discover["numbered_only"],
            )
        )

    include_parents_option = group.get("include_parents_option")
    include_parents = bool(
        include_parents_option and getattr(args, include_parents_option)
    )
    if group["type"] == "leaf_sections" and not include_parents:
        headings = [heading for heading in headings if not heading.has_numbered_child]

    targets: list[ResolvedTarget] = []
    for heading in headings:
        output_path = build_output_path(
            config,
            args,
            {
                "section": heading.section,
                "safe_title": safe_name(heading.title),
            },
        )
        label = heading.label()
        prompt_target = heading.prompt_target()
        targets.append(
            ResolvedTarget(
                label=label,
                output_path=output_path,
                prompt_vars={
                    "target": prompt_target,
                    "output_file": output_path,
                },
                match_texts=(label, prompt_target),
            )
        )

    return apply_common_filters(
        targets,
        filter_pattern=args.filter,
        start_at=args.start_at,
        start=args.start,
        end=args.end,
        unit_name=config.targeting["unit_name"],
    )


def is_excluded_dir(
    path: str,
    excluded_dir_names: set[str],
    always_excluded: set[str],
    include_third_party: bool,
) -> bool:
    parts = set(os.path.normpath(path).split(os.sep))
    if excluded_dir_names & parts:
        return True
    if always_excluded & parts:
        return True
    if not include_third_party and "third_party" in parts:
        return True
    return False


def discover_source_files(
    roots: list[str],
    *,
    extensions: set[str],
    excluded_dir_names: set[str],
    always_excluded: set[str],
    include_third_party: bool,
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
                    os.path.join(dirpath, dirname),
                    excluded_dir_names,
                    always_excluded,
                    include_third_party,
                )
            ]
            if is_excluded_dir(
                dirpath,
                excluded_dir_names,
                always_excluded,
                include_third_party,
            ):
                dirnames[:] = []
                continue
            for filename in sorted(filenames):
                ext = os.path.splitext(filename)[1].lower()
                if ext not in extensions:
                    continue
                files.append(os.path.join(dirpath, filename))
    return sorted(set(files))


def resolve_source_shard_targets(
    config: PipelineConfig, args: argparse.Namespace
) -> tuple[list[ResolvedTarget], int, int]:
    discover = config.targeting["discover"]
    group = config.targeting["group"]
    roots = [
        repo_path(root)
        for root in (args.roots or discover["default_roots"])
    ]
    include_third_party = bool(args.include_third_party)
    files = discover_source_files(
        roots,
        extensions=set(discover["source_extensions"]),
        excluded_dir_names=set(discover["excluded_dir_names"]),
        always_excluded=set(discover["exclude_dir_names_always"]),
        include_third_party=include_third_party,
    )
    if not files:
        return [], 0, 0

    shard_size = args.shard_size
    shards = [
        SecurityShard(index=idx, paths=tuple(files[i : i + shard_size]))
        for idx, i in enumerate(range(0, len(files), shard_size), start=1)
    ]

    targets: list[ResolvedTarget] = []
    for shard in shards:
        first = os.path.splitext(os.path.basename(shard.paths[0]))[0]
        output_path = build_output_path(
            config,
            args,
            {
                "index": shard.index,
                "safe_first_basename": safe_name(first, fallback="shard"),
            },
        )
        label = shard.label()
        relpaths = tuple(os.path.relpath(path, REPO_DIR) for path in shard.paths)
        targets.append(
            ResolvedTarget(
                label=label,
                output_path=output_path,
                prompt_vars={
                    "target": label,
                    "files": shard.prompt_files(),
                    "output_file": output_path,
                },
                match_texts=(label, *relpaths),
            )
        )

    return apply_common_filters(
        targets,
        filter_pattern=args.filter,
        start_at=args.start_at,
        start=args.start,
        end=args.end,
        unit_name=config.targeting["unit_name"],
    )


def resolve_targets(
    config: PipelineConfig, args: argparse.Namespace
) -> tuple[list[ResolvedTarget], int, int]:
    discover_type = config.targeting["discover"]["type"]
    group_type = config.targeting["group"]["type"]

    if discover_type == "singleton" and group_type == "singleton":
        output_path = os.path.join(
            resolve_output_dir(config, args),
            os.path.basename(config.targeting["output_path"]),
        )
        label = config.targeting["label"]
        target = ResolvedTarget(
            label=label,
            output_path=output_path,
            prompt_vars={
                "target": label,
                "output_file": output_path,
            },
            match_texts=(label, output_path),
        )
        return [target], 0, 1
    if discover_type == "markdown_headings" and group_type == "leaf_sections":
        return resolve_spec_targets(config, args)
    if discover_type == "source_files" and group_type == "fixed_size_shards":
        return resolve_source_shard_targets(config, args)
    raise ValueError(
        f"unsupported targeting resolver: {discover_type} + {group_type}"
    )


def render_prompt(
    *,
    prompt_path: str,
    step: StepConfig,
    config: PipelineConfig,
    target: ResolvedTarget | None,
    prompt_vars: dict[str, str] | None = None,
) -> str:
    with open(prompt_path, "r") as f:
        prompt = f.read()

    prompt_values: dict[str, str] = {}
    if target is not None:
        prompt_values.update(target.prompt_vars)
    if prompt_vars:
        prompt_values.update(prompt_vars)

    for key, value in prompt_values.items():
        placeholder = config.placeholders.get(key)
        if not placeholder:
            continue
        if placeholder not in prompt and step.kind == "target_prompt":
            print(
                f"Warning: prompt template at {prompt_path} has no "
                f"{placeholder} placeholder; running it unchanged.",
                file=sys.stderr,
            )
            continue
        prompt = prompt.replace(placeholder, value)

    if step.append_template and target is not None:
        prompt += step.append_template.format(output_path=target.output_path)

    return prompt


def expand_post_steps(config: PipelineConfig) -> list[tuple[str, str]]:
    prompt_steps: list[tuple[str, str]] = []
    for step in config.post_steps:
        for path in sorted(glob.glob(step.glob_pattern)):
            name = os.path.basename(path)
            match = PROMPT_STEP_RE.match(name)
            if not match:
                continue
            if int(match.group(1)) < step.min_numeric_prefix:
                continue
            prompt_steps.append((path, step.effort))
    return prompt_steps


def run_post_steps(
    prompt_steps: list[tuple[str, str]],
    config: PipelineConfig,
    args: argparse.Namespace,
    cli: str,
    dry_run: bool = False,
) -> bool:
    prompt_vars = resolve_post_prompt_vars(config, args)
    for step_idx, (prompt_file, effort) in enumerate(prompt_steps, start=1):
        step_name = os.path.basename(prompt_file)
        print(f"post step {step_idx}/{len(prompt_steps)}: {step_name}")
        step = StepConfig(path=prompt_file, effort=effort, kind="post_prompt")
        prompt = render_prompt(
            prompt_path=prompt_file,
            step=step,
            config=config,
            target=None,
            prompt_vars=prompt_vars,
        )

        rc = run_agent(prompt, cli=cli, dry_run=dry_run, effort=effort)
        if rc != 0:
            print(f"  post step {step_idx} FAILED (exit code {rc})")
            return False
        print(f"  post step {step_idx} OK")
    return True


def validate_step_paths(config: PipelineConfig) -> int:
    for step in config.steps:
        if not os.path.exists(step.path):
            print(f"error: prompt not found: {step.path}", file=sys.stderr)
            return 2
    for prompt_file, _ in expand_post_steps(config):
        if not os.path.exists(prompt_file):
            print(f"error: prompt not found: {prompt_file}", file=sys.stderr)
            return 2
    return 0


def run_target_steps(
    *,
    config: PipelineConfig,
    target: ResolvedTarget,
    cli: str,
    dry_run: bool,
) -> bool:
    for step_idx, step in enumerate(config.steps, start=1):
        step_name = os.path.basename(step.path)
        print(f"  step {step_idx}/{len(config.steps)}: {step_name}")
        prompt = render_prompt(
            prompt_path=step.path,
            step=step,
            config=config,
            target=target,
        )
        rc = run_agent(prompt, cli=cli, dry_run=dry_run, effort=step.effort)
        if rc != 0:
            print(f"  step {step_idx} FAILED (exit code {rc})")
            return False
        print(f"  step {step_idx} OK")
    return True


def is_full_run(config: PipelineConfig, args: argparse.Namespace) -> bool:
    option_names = config.targeting.get("full_run_default_options", [])
    return all(getattr(args, name) == OPTION_DEFAULTS[name] for name in option_names)


def run_pipeline(config: PipelineConfig, args: argparse.Namespace) -> int:
    rc = validate_step_paths(config)
    if rc != 0:
        return rc

    targets, start_offset, total_matched = resolve_targets(config, args)
    if not targets:
        no_match_message = config.targeting.get("no_match_message")
        if no_match_message:
            print(no_match_message, file=sys.stderr)
        else:
            print("No targets matched.", file=sys.stderr)
        return 1

    unit_name = config.targeting["unit_name"]
    summary_label = config.targeting["summary_label"]

    if args.list:
        for i, target in enumerate(targets, start=start_offset + 1):
            print(f"{i:3d}  {target.label}")
        print(f"\n{len(targets)} {unit_name}(s) (of {total_matched} matched)")
        return 0

    step_names = [os.path.basename(step.path) for step in config.steps]
    print(f"Pipeline: {config.name}")
    print(f"CLI: {args.cli}")
    print(f"Output dir: {resolve_output_dir(config, args)}")
    print(f"Steps: {' -> '.join(step_names)}")

    post_prompt_steps = expand_post_steps(config)
    if post_prompt_steps:
        print(
            "Post steps: "
            + " -> ".join(os.path.basename(path) for path, _ in post_prompt_steps)
        )

    selected_first = start_offset + 1
    selected_last = start_offset + len(targets)

    if selected_first != 1 or selected_last != total_matched:
        print(
            f"Found {len(targets)} {summary_label} "
            f"({unit_name}s {selected_first}-{selected_last} of "
            f"{total_matched}).\n"
        )
    else:
        print(f"Found {len(targets)} {summary_label}.\n")

    results: dict[str, list[str]] = {"pass": [], "fail": []}

    for i, target in enumerate(targets, start=start_offset + 1):
        print(f"[{i}/{total_matched}] {target.label}")
        if not args.dry_run:
            os.makedirs(resolve_output_dir(config, args), exist_ok=True)

        passed = run_target_steps(
            config=config,
            target=target,
            cli=args.cli,
            dry_run=args.dry_run,
        )
        if not passed:
            results["fail"].append(target.label)
            print("  -> FAILED\n")
            continue

        if args.dry_run:
            print(f"  [dry-run] would write output to {target.output_path}")

        results["pass"].append(target.label)
        print("  -> ALL STEPS OK\n")

    post_steps_ran = False
    post_steps_ok = True
    if post_prompt_steps:
        if (
            not args.dry_run
            and not results["fail"]
            and is_full_run(config, args)
        ):
            print("[post-pass] Running completion steps")
            post_steps_ran = True
            post_steps_ok = run_post_steps(
                post_prompt_steps,
                config=config,
                args=args,
                cli=args.cli,
                dry_run=args.dry_run,
            )
        else:
            print("[post-pass] Skipped post steps")
            if results["fail"]:
                print("  reason: one or more target runs failed")
            else:
                print(
                    "  reason: pipeline run was partial; post steps only run after a full successful run"
                )

    print("=" * 60)
    print(
        f"Done. {len(results['pass'])} passed, "
        f"{len(results['fail'])} failed."
    )
    if post_steps_ran:
        print(f"Post steps: {'OK' if post_steps_ok else 'FAILED'}.")
    if results["fail"]:
        print("\nFailed:")
        for label in results["fail"]:
            print(f"  - {label}")

    if results["fail"]:
        return 1
    if post_steps_ran and not post_steps_ok:
        return 1
    return 0


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Run a named pipeline with the shared pipeline runner. "
            "Pass the pipeline name last: audit, security, doxygen, or project_ready."
        ),
    )
    parser.add_argument(
        "-o",
        "--output-dir",
        type=str,
        default=None,
        help="Override the pipeline output directory for this run.",
    )
    parser.add_argument(
        "--filter",
        type=str,
        default=None,
        help="Limit the matched targets by substring.",
    )
    parser.add_argument(
        "--start",
        type=int,
        default=None,
        metavar="N",
        help="Start at the N-th matched target (1-based). "
        "Mutually exclusive with --start-at.",
    )
    parser.add_argument(
        "--end",
        type=int,
        default=None,
        metavar="N",
        help="Stop after the N-th matched target (1-based, inclusive).",
    )
    parser.add_argument(
        "--start-at",
        type=str,
        default=None,
        help="Skip targets until the first label containing this substring. "
        "Mutually exclusive with --start.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print planned invocations without running the selected CLI.",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="List matched targets and exit.",
    )
    parser.add_argument(
        "--spec",
        type=str,
        default=None,
        help="Only audit spec files whose basename contains this substring.",
    )
    parser.add_argument(
        "--include-parents",
        action="store_true",
        help="Include numbered parent headings when child headings exist.",
    )
    parser.add_argument(
        "--root",
        dest="roots",
        action="append",
        default=[],
        help="Add a source root to review.",
    )
    parser.add_argument(
        "--shard-size",
        type=int,
        default=12,
        metavar="N",
        help="Number of source files per shard for source-sharded pipelines.",
    )
    parser.add_argument(
        "--include-third-party",
        action="store_true",
        help="Include vendored third-party source directories.",
    )

    cli_group = parser.add_mutually_exclusive_group()
    cli_group.add_argument(
        "--codex",
        dest="cli",
        action="store_const",
        const="codex",
        help="Run prompts with Codex CLI.",
    )
    cli_group.add_argument(
        "--claude",
        "--cluade",
        dest="cli",
        action="store_const",
        const="claude",
        help="Run prompts with Claude CLI.",
    )
    parser.set_defaults(cli=None)

    parser.add_argument(
        "pipeline_name",
        choices=("audit", "security", "doxygen", "project_ready"),
        help="Pipeline to run.",
    )

    return parser.parse_args(argv)


def validate_args(
    args: argparse.Namespace, config: PipelineConfig
) -> int:
    if args.start is not None and args.start_at is not None:
        print(
            "error: --start and --start-at are mutually exclusive.",
            file=sys.stderr,
        )
        return 2

    if "shard_size" in config.options and args.shard_size < 1:
        print("error: --shard-size must be >= 1.", file=sys.stderr)
        return 2

    allowed_options = COMMON_OPTION_NAMES | set(config.options)
    for option_name, default_value in OPTION_DEFAULTS.items():
        if option_name in allowed_options:
            continue
        if getattr(args, option_name) != default_value:
            cli_name = OPTION_FLAG_NAMES.get(
                option_name, option_name.replace("_", "-")
            )
            owning_pipeline = OPTION_PIPELINE_NAMES.get(option_name, config.name)
            print(
                f"error: --{cli_name} is only valid for the "
                f"{owning_pipeline} pipeline.",
                file=sys.stderr,
            )
            return 2

    return 0


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    config = load_pipeline_config(args.pipeline_name)
    if args.cli is None:
        args.cli = config.default_cli

    rc = validate_args(args, config)
    if rc != 0:
        return rc

    return run_pipeline(config, args)


if __name__ == "__main__":
    sys.exit(main())
