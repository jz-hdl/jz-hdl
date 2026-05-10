#!/usr/bin/env python3
import json
import pathlib
import re
import sys


def main() -> int:
    if len(sys.argv) != 2 or not re.fullmatch(r"\d+\.\d+\.\d+", sys.argv[1]):
        print(f"Usage: {pathlib.Path(sys.argv[0]).name} <major.minor.patch>", file=sys.stderr)
        return 1

    version = sys.argv[1]
    major, minor, patch = version.split(".")
    repo_root = pathlib.Path(__file__).resolve().parent.parent

    (repo_root / "VERSION").write_text(f"{version}\n", encoding="utf-8")

    version_h = repo_root / "compiler" / "include" / "version.h"
    version_h.write_text(
        "/**\n"
        " * @file version.h\n"
        " * @brief Compiler version string.\n"
        " *\n"
        " * NOTE: During a CMake build, a generated version.h in the build directory\n"
        " * takes precedence over this file (it includes the git commit hash).\n"
        " * This fallback exists for non-CMake builds or IDE indexing.\n"
        " */\n"
        "\n"
        "#ifndef JZ_HDL_VERSION_H\n"
        "#define JZ_HDL_VERSION_H\n"
        "\n"
        f"#define JZ_HDL_VERSION_MAJOR {major}\n"
        f"#define JZ_HDL_VERSION_MINOR {minor}\n"
        f"#define JZ_HDL_VERSION_PATCH {patch}\n"
        "\n"
        f"#define JZ_HDL_VERSION_STRING \"Version {version} (unknown)\"\n"
        "\n"
        "#endif /* JZ_HDL_VERSION_H */\n",
        encoding="utf-8",
    )

    for spec_path in (repo_root / "specification").glob("*.md"):
        text = spec_path.read_text(encoding="utf-8")
        updated = re.sub(r"Version: [0-9][0-9.]*", f"Version: {version}", text)
        spec_path.write_text(updated, encoding="utf-8")

    package_json_path = repo_root / "vscode-ext" / "package.json"
    package_json = json.loads(package_json_path.read_text(encoding="utf-8"))
    package_json["version"] = version
    package_json_path.write_text(json.dumps(package_json, indent=4) + "\n", encoding="utf-8")

    package_lock_path = repo_root / "vscode-ext" / "package-lock.json"
    package_lock = json.loads(package_lock_path.read_text(encoding="utf-8"))
    package_lock["version"] = version
    if "" in package_lock.get("packages", {}):
        package_lock["packages"][""]["version"] = version
    package_lock_path.write_text(json.dumps(package_lock, indent=4) + "\n", encoding="utf-8")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
