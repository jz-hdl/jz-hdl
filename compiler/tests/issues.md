
## test_8_4-global_value_semantics.md

* ASSIGN_WIDTH_NO_MODIFIER : compiler-bug
  Concatenation width mismatch fires ASSIGN_CONCAT_WIDTH_MISMATCH (higher priority per rules.c line 13) instead of ASSIGN_WIDTH_NO_MODIFIER. The intended rule cannot fire in concatenation context. File attempted: `8_4_ASSIGN_WIDTH_NO_MODIFIER-global_in_concat.jz`.
* ASSIGN_WIDTH_NO_MODIFIER : missing-context
  Covered: ASYNC `=` (narrow→wide), SYNC `<=` (wide→narrow, narrow→wide), ASYNC `<=` alias (wide→narrow), both helper and top modules, global in expression (resolved by sweep); missing: global in concatenation. Recommended new file: `8_4_ASSIGN_WIDTH_NO_MODIFIER-global_in_concat.jz`. Note: sweep found rule-not-fired — ASSIGN_CONCAT_WIDTH_MISMATCH preempts.
* ASSIGN_WIDTH_NO_MODIFIER : missing-happy-path
  Happy-path file exists (`8_4_HAPPY_PATH-global_value_semantics_ok.jz`) but only covers direct assignment contexts; missing: global in expression, concatenation, conditional (ternary) with matching widths. Recommended: `8_4_HAPPY_PATH-global_in_expressions_ok.jz`.


## test_9_3-check_placement_rules.md

* DIRECTIVE_INVALID_CONTEXT : compiler-bug
  @check inside non-ASYNC/SYNC blocks (PORT, REGISTER, WIRE, CONST, MEM, LATCH, MUX, BUS, CDC) fires PARSE000 instead of DIRECTIVE_INVALID_CONTEXT. These blocks use specialized parsers that don't recognize directives; only the statement parser (used by ASYNC/SYNC) can detect and report DIRECTIVE_INVALID_CONTEXT. The audit bundled 9 missing contexts into one filename. All 9 exhibit the same behavior: PARSE000 fires, not the intended rule. File attempted: `9_3_DIRECTIVE_INVALID_CONTEXT-check_in_other_blocks.jz`.
* DIRECTIVE_INVALID_CONTEXT : missing-context
  Covered: ASYNCHRONOUS block, SYNCHRONOUS block; missing: other block types (PORT, REGISTER, WIRE, CONST, MEM, LATCH, MUX, BUS, CDC). Recommended new file: `9_3_DIRECTIVE_INVALID_CONTEXT-check_in_other_blocks.jz`. Note: sweep confirmed compiler-bug — PARSE000 fires instead.
* CLOCK_GEN_VARIANT_AMBIGUOUS : missing-coverage
  (`error`, `S9.3`) — no validation file exists. Recommended: `9_3_CLOCK_GEN_VARIANT_AMBIGUOUS-ambiguous_variant.jz`. Note: requires chip JSON fixture with overlapping variants.
* CLOCK_GEN_VARIANT_NO_MATCH : missing-coverage
  (`error`, `S9.3`) — no validation file exists. Recommended: `9_3_CLOCK_GEN_VARIANT_NO_MATCH-no_matching_variant.jz`. Note: requires chip JSON fixture with clock_gen variants.
* PROJECT_CHIP_DATA_VARIANT_INVALID : missing-coverage
  (`error`, `S9.3`) — no validation file exists. Recommended: `9_3_PROJECT_CHIP_DATA_VARIANT_INVALID-invalid_chip_variants.jz`. Note: requires chip JSON fixture; may need non-standard test harness.
* CLOCK_GEN_VARIANT_AMBIGUOUS : missing-happy-path
  No valid-form regression test. Recommended: `9_3_HAPPY_PATH-clock_gen_unambiguous_ok.jz`.
* CLOCK_GEN_VARIANT_NO_MATCH : missing-happy-path
  No valid-form regression test. Recommended: `9_3_HAPPY_PATH-clock_gen_match_ok.jz`.
* PROJECT_CHIP_DATA_VARIANT_INVALID : missing-happy-path
  No valid-form regression test. Recommended: `9_3_HAPPY_PATH-chip_variant_ok.jz`.
* Plan 5.1 : test-quality
  Rules PROJECT_CHIP_DATA_VARIANT_INVALID, CLOCK_GEN_VARIANT_NO_MATCH, and CLOCK_GEN_VARIANT_AMBIGUOUS are about clock generation chip data variants, not @check placement. They reference "S9.3" in rules.c but appear thematically misplaced in this plan. Consider moving to a dedicated clock_gen test plan.

## test_9_4-check_expression_rules.md

* CHECK_INVALID_EXPR_TYPE : missing-context
  Covered: port ref (helper+top), register ref (helper+top), wire ref (top), port slice (helper+top), register slice (helper+top), wire slice (top), memory port ref (resolved by sweep), nested forbidden operand (resolved by sweep). Note: this entry is fully resolved.

## test_9_5-check_evaluation_order.md

* CHECK_INVALID_EXPR_TYPE : compiler-bug
  When a module's CONST is overridden via OVERRIDE in @new, @check inside that module incorrectly fires CHECK_INVALID_EXPR_TYPE. Per S9.5, @check should see CONST values after OVERRIDE evaluation. Confirmed: same module's @check compiles clean without OVERRIDE, fails with OVERRIDE present. File attempted: `9_5_HAPPY_PATH-check_config_override_ok.jz`.
* CHECK_INVALID_EXPR_TYPE : missing-context
  Covered: bare undefined ID at project scope, undefined in arithmetic at project scope, undefined in comparison at project scope, undefined inside clog2 at project scope, transitive CONST resolution (resolved by sweep); missing: CONFIG override at instantiation in happy-path. Recommended new file: `9_5_HAPPY_PATH-check_config_override_ok.jz`. Note: sweep confirmed compiler-bug — @check incorrectly fires with OVERRIDE present.

## test_9_7-check_error_conditions.md

* DIRECTIVE_INVALID_CONTEXT : compiler-bug
  @check inside CDC block emits PARSE000 ("expected CDC type BIT/BUS/FIFO") instead of DIRECTIVE_INVALID_CONTEXT. The CDC block parser does not recognize @check as a structural directive — it falls through to the CDC-type keyword expectation. The ASYNC and SYNC block parsers correctly emit DIRECTIVE_INVALID_CONTEXT for this case (see 9_3 tests). File attempted: `9_7_DIRECTIVE_INVALID_CONTEXT-check_in_cdc.jz`.
* DIRECTIVE_INVALID_CONTEXT : parser-recovery
  (`9_3_DIRECTIVE_INVALID_CONTEXT-check_in_async.jz`) — cascading `PARSE000` after correct DIRECTIVE_INVALID_CONTEXT emission at line 57. Parser cannot recover from `(` token following `@check` inside block. Workaround: accept cascading error in .out. Real fix: improve parser recovery for directives inside blocks.
* DIRECTIVE_INVALID_CONTEXT : parser-recovery
  (`9_3_DIRECTIVE_INVALID_CONTEXT-check_in_sync.jz`) — cascading `PARSE000` after correct DIRECTIVE_INVALID_CONTEXT emission at line 61. Same parser recovery issue as above.
* DIRECTIVE_INVALID_CONTEXT : missing-context
  Covered: ASYNCHRONOUS block, SYNCHRONOUS block; missing: CDC block. Recommended new file: `9_7_DIRECTIVE_INVALID_CONTEXT-check_in_cdc.jz`. Note: sweep confirmed compiler-bug — PARSE000 fires instead.
* PARSE000 : compiler-bug
  Rule ID in `9_3_DIRECTIVE_INVALID_CONTEXT-check_in_async.out` and `9_3_DIRECTIVE_INVALID_CONTEXT-check_in_sync.out` — not registered in `compiler/src/rules.c`. PARSE000 is dynamically generated by the parser (hardcoded string in `parser_core.c:83`), not a registered rule.
* DIRECTIVE_INVALID_CONTEXT : compiler-bug
  (`9_3_DIRECTIVE_INVALID_CONTEXT-check_in_async.jz`, `9_3_DIRECTIVE_INVALID_CONTEXT-check_in_sync.jz`) — @check inside ASYNC/SYNC blocks fires DIRECTIVE_INVALID_CONTEXT with message "Structural directives (@project/@module/@endproj/@endmod/@blackbox/@new/@import) used in invalid location" — message is misleading because @check is not a structural directive. Ideally should fire CHECK_INVALID_PLACEMENT (S9.3) or the message should be updated to include @check.

## test_sim-simulation_rules.md

* SIM_WRONG_TOOL : missing-happy-path
  No valid-form regression test. A happy-path would require `--simulate` mode (not `--lint`), which the validation runner does not support. Recommended: `sim_SIM_WRONG_TOOL-no_simulation_ok.jz` (file with module only, no @simulation block, clean lint).
* SIM_PROJECT_MIXED : missing-happy-path
  No valid-form regression test for a file with @project only (no @simulation). Recommended: `sim_SIM_PROJECT_MIXED-project_only_ok.jz`.
* SIM_RUN_COND_TIMEOUT : not-testable
  Runtime-only rule (fires during `--simulate` when `@run_until`/`@run_while` condition not met within timeout). Not reachable via `--info --lint`.

## test_tb-testbench_rules.md

* TB_WRONG_TOOL : missing-coverage
  (`error`, `S-tb`) — no validation file exists. This rule fires under `--info --lint` and IS testable by the standard validation harness. Recommended: `tb_TB_WRONG_TOOL-testbench_with_lint.jz`.
* TB_CLOCK_CYCLE_NOT_POSITIVE : missing-coverage
  (`error`, `S-tb`) — no validation file exists. Requires `--test` mode. Recommended: `tb_TB_CLOCK_CYCLE_NOT_POSITIVE-zero_cycle.jz`, `tb_TB_CLOCK_CYCLE_NOT_POSITIVE-negative_cycle.jz`.
* TB_CLOCK_NOT_DECLARED : missing-coverage
  (`error`, `S-tb`) — no validation file exists. Requires `--test` mode. Recommended: `tb_TB_CLOCK_NOT_DECLARED-undeclared_clock.jz`.
* TB_EXPECT_WIDTH_MISMATCH : missing-coverage
  (`error`, `S-tb`) — no validation file exists. Requires `--test` mode. Recommended: `tb_TB_EXPECT_WIDTH_MISMATCH-wrong_width.jz`.
* TB_MODULE_NOT_FOUND : missing-coverage
  (`error`, `S-tb`) — no validation file exists. Requires `--test` mode. Recommended: `tb_TB_MODULE_NOT_FOUND-unknown_module.jz`.
* TB_MULTIPLE_NEW : missing-coverage
  (`error`, `S-tb`) — no validation file exists. Requires `--test` mode. Recommended: `tb_TB_MULTIPLE_NEW-two_new.jz`.
* TB_NEW_RHS_INVALID : missing-coverage
  (`error`, `S-tb`) — no validation file exists. Requires `--test` mode. Recommended: `tb_TB_NEW_RHS_INVALID-undeclared_rhs.jz`.
* TB_NO_TEST_BLOCKS : missing-coverage
  (`error`, `S-tb`) — no validation file exists. Requires `--test` mode. Recommended: `tb_TB_NO_TEST_BLOCKS-empty_testbench.jz`.
* TB_PORT_NOT_CONNECTED : missing-coverage
  (`error`, `S-tb`) — no validation file exists. Requires `--test` mode. Recommended: `tb_TB_PORT_NOT_CONNECTED-missing_port.jz`.
* TB_PORT_WIDTH_MISMATCH : missing-coverage
  (`error`, `S-tb`) — no validation file exists. Requires `--test` mode. Recommended: `tb_TB_PORT_WIDTH_MISMATCH-wrong_width.jz`.
* TB_PROJECT_MIXED : missing-coverage
  (`error`, `S-tb`) — no validation file exists. This rule fires under `--info --lint` (per plan section 6.1) and IS testable by the standard validation harness. Recommended: `tb_TB_PROJECT_MIXED-project_and_testbench.jz`.
* TB_SETUP_POSITION : missing-coverage
  (`error`, `S-tb`) — no validation file exists. Requires `--test` mode. Recommended: `tb_TB_SETUP_POSITION-before_new.jz`, `tb_TB_SETUP_POSITION-duplicate.jz`.
* TB_UPDATE_CLOCK_ASSIGN : missing-coverage
  (`error`, `S-tb`) — no validation file exists. Requires `--test` mode. Recommended: `tb_TB_UPDATE_CLOCK_ASSIGN-assign_clock.jz`.
* TB_UPDATE_NOT_WIRE : missing-coverage
  (`error`, `S-tb`) — no validation file exists. Requires `--test` mode. Recommended: `tb_TB_UPDATE_NOT_WIRE-assign_nonwire.jz`.
* TB_CLOCK_CYCLE_NOT_POSITIVE : missing-happy-path
  No valid-form regression test. Would require `--test` mode.
* TB_CLOCK_NOT_DECLARED : missing-happy-path
  No valid-form regression test. Would require `--test` mode.
* TB_EXPECT_WIDTH_MISMATCH : missing-happy-path
  No valid-form regression test. Would require `--test` mode.
* TB_MODULE_NOT_FOUND : missing-happy-path
  No valid-form regression test. Would require `--test` mode.
* TB_MULTIPLE_NEW : missing-happy-path
  No valid-form regression test. Would require `--test` mode.
* TB_NEW_RHS_INVALID : missing-happy-path
  No valid-form regression test. Would require `--test` mode.
* TB_NO_TEST_BLOCKS : missing-happy-path
  No valid-form regression test. Would require `--test` mode.
* TB_PORT_NOT_CONNECTED : missing-happy-path
  No valid-form regression test. Would require `--test` mode.
* TB_PORT_WIDTH_MISMATCH : missing-happy-path
  No valid-form regression test. Would require `--test` mode.
* TB_PROJECT_MIXED : missing-happy-path
  No valid-form regression test. Recommended: `tb_TB_PROJECT_MIXED-testbench_only_ok.jz` (file with @testbench only, no @project, tested via --lint).
* TB_SETUP_POSITION : missing-happy-path
  No valid-form regression test. Would require `--test` mode.
* TB_UPDATE_CLOCK_ASSIGN : missing-happy-path
  No valid-form regression test. Would require `--test` mode.
* TB_UPDATE_NOT_WIRE : missing-happy-path
  No valid-form regression test. Would require `--test` mode.
* TB_WRONG_TOOL : missing-happy-path
  No valid-form regression test. Recommended: `tb_TB_WRONG_TOOL-no_testbench_ok.jz` (file with module only, no @testbench block, clean lint).
* Plan inconsistency : test-quality
  Plan intro note states "The one exception is `TB_WRONG_TOOL`" (only rule testable via `--lint`), but section 6.1 lists both TB_WRONG_TOOL and TB_PROJECT_MIXED as `--lint` mode. The plan intro should be updated to mention both exceptions, or section 6.1 should be corrected.
