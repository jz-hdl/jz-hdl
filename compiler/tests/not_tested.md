# Rules Not Tested

## test_misc-repeat_serializer_io.md

| Rule ID | Severity | Reason |
|---------|----------|--------|
| INFO_SERIALIZER_CASCADE | info | Backend-only rule (emitted in `emit_wrapper.c` during Verilog-2005/RTLIL generation). Not reachable via `--info --lint`. |
| SERIALIZER_WIDTH_EXCEEDS_RATIO | error | Backend-only rule (emitted in `emit_wrapper.c` during Verilog-2005/RTLIL generation). Not reachable via `--info --lint`. |
| IO_BACKEND | error | Runtime I/O error (file write failure). Not reachable via `--info --lint`. |
| IO_IR | error | Runtime I/O error (file write failure). Not reachable via `--info --lint`. |

## test_sim-simulation_rules.md

| Rule ID | Severity | Reason |
|---------|----------|--------|
| SIM_RUN_COND_TIMEOUT | error | Runtime-only rule (fires during `--simulate` when `@run_until`/`@run_while` condition not met within timeout). Not reachable via `--info --lint`. |
