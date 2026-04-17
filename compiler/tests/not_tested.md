# Rules Not Tested

## test_misc-repeat_serializer_io.md

| Rule ID | Severity | Reason |
|---------|----------|--------|
| IO_BACKEND | error | Runtime I/O error (file write failure). Not reachable via `--info --lint`. |
| IO_IR | error | Runtime I/O error (file write failure). Not reachable via `--info --lint`. |

## test_sim-simulation_rules.md

| Rule ID | Severity | Reason |
|---------|----------|--------|
| SIM_RUN_COND_TIMEOUT | error | Runtime-only rule (fires during `--simulate` when `@run_until`/`@run_while` condition not met within timeout). Not reachable via `--info --lint`. |
