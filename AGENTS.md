# Planning
- For tasks that touch more than 3 files, create a plan and get approval before writing code
- Break large tasks into steps and track progress
- Always use the Plan tool when making plans

# Decision Making
- Always fix compiler bugs, never work around them
- Don't guess, ask when uncertain
- Don't expand scope without asking
- Present options instead of choosing
- Explain what you're about to do before doing it
- When told something is wrong, stop and review all changes made
    - explain what is correct and what is not
    - ask how to proceed, do not immediately revert

# Communication
- Be concise — no filler, no restating what I said
- When I ask a question, answer it and stop
- Don't apologize, just fix it
- Don't explain obvious things
- Don't use platitudes

# Tool Constraints
- Use the available read/search/edit tools that exist in the current environment.
- Do not create or edit files using shell heredocs, `cat >`, `echo >`, or similar shell write tricks.
- Use patch/edit tools for file changes.
- Use fast search tools where available.
- Do not use destructive git commands unless explicitly requested.

# Safety
- Never delete files without asking
- Never force-push
- Don't overwrite uncommitted work
- Always run compiler/tests/run_validation.sh after working on the compiler

# Validation Fixture Rules
- A validation test for rule X must emit only rule X unless there is no valid way to do so.
- Extra diagnostics are allowed only when they are semantically inseparable from the rule under test. If so, stop and explain why before changing the fixture.
- Do not remove setup code just to remove diagnostics, first determine whether the setup is required to make the program semantically valid.
- If removing setup creates warnings/errors, that edit is wrong, restore or redesign the fixture.
- Prefer making the fixture valid and isolated over changing `.out` to accept scaffolding warnings.
- Before editing a `.jz` validation test, identify:
  - the target diagnostic
  - every current non-target diagnostic
  - whether each non-target diagnostic is valid or a compiler bug
  - the planned fixture structure that avoids non-target diagnostics
- After editing a validation fixture, run that individual test and confirm the output contains only intended diagnostics before updating `.out`.

# Compiler Work
- Always rebuild the compiler before regenerating expected `.out` files.
- Always run `compiler/tests/run_validation.sh` after compiler changes.
- If setting `JZ_HDL_BIN`, use an absolute path because the validation script changes directories during golden tests.
- Treat a failing individual validation test as a blocker. Do not proceed to final or ask unrelated follow-up questions while a known fixture mismatch remains.

# Information
- specification/chip-info-specification.md : Chip data file format (compiler/data/*.json)
- specification/jz-hdl-specification.md : The JZ-HDL specification
- specification/jzw.md : The jzw file format specification
- specification/simulation-specification.md : The JZ-HDL simulation specification
- specification/testbench-specification.md : The JZ-HDL testbench specification
- datasheets/ : Manufacturer fpga datasheets

# Git
- Don’t use a generic message
- When committing always create a commit message the describes what is in the commit
- Use a summary line plus bullets for multi-change commits
