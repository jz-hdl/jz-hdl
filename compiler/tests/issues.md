
## test_3_1-operator_categories.md

* 3_1_SPECIAL_DRIVER_IN_INDEX-gnd_vcc_in_index.jz : test-quality
  Thin test: single module, 1 trigger (GND only), ASYNC only, no false-positive guards. All other 3_1 tests use multi-module structure with ASYNC+SYNC and include false-positive coverage. Fix: rewrite to match quality of sibling tests (add VCC trigger, SYNC context, valid `r[signal]` guard).
* 3_1_SPECIAL_DRIVER_IN_INDEX-gnd_vcc_in_index.jz : test-quality
  Stale comment: line 19 says "parser rejects as PARSE000" but compiler emits `SPECIAL_DRIVER_IN_INDEX` correctly. Fix: update comment.
* 3_1_SPECIAL_DRIVER_IN_INDEX-gnd_vcc_in_index.jz : test-quality
  Duplicate: identical content to `2_4_SPECIAL_DRIVER_IN_INDEX-gnd_vcc_in_index.jz`.
* 3_1_SPECIAL_DRIVER_IN_EXPRESSION-gnd_vcc_in_expr.jz : test-quality
  Duplicate: identical content to `2_4_SPECIAL_DRIVER_IN_EXPRESSION-gnd_vcc_in_expr.jz`.
* 3_1_SPECIAL_DRIVER_IN_CONCAT-gnd_vcc_in_concat.jz : test-quality
  Duplicate: identical content to `2_4_SPECIAL_DRIVER_IN_CONCAT-gnd_vcc_in_concat.jz`.
* 3_1_SPECIAL_DRIVER_SLICED-vcc_gnd_sliced.jz : test-quality
  Duplicate: identical content to `1_3_SPECIAL_DRIVER_SLICED-vcc_gnd_sliced.jz`.
* Plan section 4 : test-quality
  Lists only 3 of 13 existing `3_1_*` validation files. Missing 10 files: `3_1_UNARY_ARITH_MISSING_PARENS-*.jz`, `3_1_TERNARY_COND_WIDTH_NOT_1-*.jz`, `3_1_TERNARY_BRANCH_WIDTH_MISMATCH-*.jz`, `3_1_CONCAT_EMPTY-*.jz`, `3_1_DIV_CONST_ZERO-*.jz`, `3_1_DIV_UNGUARDED_RUNTIME_ZERO-*.jz`, `3_1_SPECIAL_DRIVER_IN_EXPRESSION-*.jz`, `3_1_SPECIAL_DRIVER_IN_CONCAT-*.jz`, `3_1_SPECIAL_DRIVER_SLICED-*.jz`, `3_1_SPECIAL_DRIVER_IN_INDEX-*.jz`. Fix: update plan to list all existing files.

## test_2_4-special_semantic_drivers.md

* SPECIAL_DRIVER_IN_INDEX : compiler-bug
  SPECIAL_DRIVER_IN_INDEX only emits once per file regardless of number of violations. Multiple triggers across modules are suppressed after the first diagnostic. The audit listed 4 missing contexts (VCC as index, SYNCHRONOUS context, MUX block context, LHS index context) but only one can be tested per file. MUX block context and RHS-VCC-in-sync remain uncoverable without additional files. Observed during sweep for `2_4_SPECIAL_DRIVER_IN_INDEX-vcc_sync_lhs.jz`.
* SPECIAL_DRIVER_IN_INDEX : missing-context
  Covered: GND as bit-select index in ASYNC, VCC sync LHS; missing: MUX block context (cannot test due to single-emission bug). Recommended: `2_4_SPECIAL_DRIVER_IN_INDEX-vcc_sync_lhs.jz` (created, covers VCC + SYNC + LHS). MUX block context still missing.
* 2_4_SPECIAL_DRIVER_IN_INDEX-gnd_vcc_in_index.jz : test-quality
  Stale comment: line 19 says "parser rejects as PARSE000" but compiler actually emits `SPECIAL_DRIVER_IN_INDEX` correctly. Fix: update comment to match actual behavior.
* 3_1_SPECIAL_DRIVER_IN_EXPRESSION-gnd_vcc_in_expr.jz : test-quality
  Duplicate: identical `.jz` content to `2_4_SPECIAL_DRIVER_IN_EXPRESSION-gnd_vcc_in_expr.jz`. Fix: remove duplicate or differentiate test scenarios.
* 3_1_SPECIAL_DRIVER_IN_CONCAT-gnd_vcc_in_concat.jz : test-quality
  Duplicate: identical `.jz` content to `2_4_SPECIAL_DRIVER_IN_CONCAT-gnd_vcc_in_concat.jz`. Fix: remove duplicate or differentiate test scenarios.
* 3_1_SPECIAL_DRIVER_IN_INDEX-gnd_vcc_in_index.jz : test-quality
  Duplicate: identical `.jz` content to `2_4_SPECIAL_DRIVER_IN_INDEX-gnd_vcc_in_index.jz`. Fix: remove duplicate or differentiate test scenarios.
* 1_3_SPECIAL_DRIVER_SLICED-vcc_gnd_sliced.jz and 3_1_SPECIAL_DRIVER_SLICED-vcc_gnd_sliced.jz : test-quality
  Duplicate: identical `.jz` content to each other (different from `2_4_SPECIAL_DRIVER_SLICED-gnd_vcc_sliced.jz`). Fix: remove duplicates or differentiate test scenarios.
* Plan section 4 : test-quality
  Plan lists `2_4_SPECIAL_DRIVER_HAPPY_PATH-valid_gnd_vcc_ok.jz` but actual file is `2_4_HAPPY_PATH-special_drivers_ok.jz`. Fix: update plan to match actual filename.

## test_3_2-operator_definitions.md

* OBS_X_TO_OBSERVABLE_SINK : compiler-bug
  SYNC to MEM context does not fire: `sem_lhs_observable_classify` in `driver_assign.c` only classifies REGISTER/LATCH and OUT/INOUT as observable sinks, not MEM write ports. Rule message says "REGISTER, MEM, or output" but MEM is not checked. Test covers ASYNC to output only. File attempted: `3_2_OBS_X_TO_OBSERVABLE_SINK-x_to_output_and_mem.jz`.
* SPECIAL_DRIVER_IN_INDEX : compiler-bug
  Only one SPECIAL_DRIVER_IN_INDEX fires per compilation. When multiple VCC/GND-in-index violations exist across modules or statements, only the first is reported; subsequent violations are silently suppressed. Root cause unknown — diagnostic reporting has no dedup, and statement iteration has no early exit. Test uses a single trigger that covers all three missing contexts (VCC, SYNC, range) simultaneously. File attempted: `3_2_SPECIAL_DRIVER_IN_INDEX-vcc_sync_range.jz`.
* DIV_UNGUARDED_RUNTIME_ZERO : missing-context
  Covered: SYNC unguarded `/` and `%`, plus guard patterns `!=`, `>`, `==`, `>= N`, `!= N ELSE`, `< N ELSE`, literal-on-left; originally missing guard patterns were resolved by sweep file `3_2_DIV_UNGUARDED_RUNTIME_ZERO-additional_guard_patterns.jz`. Note: this entry is fully resolved.
* 2_4_SPECIAL_DRIVER_IN_INDEX-gnd_vcc_in_index.jz : test-quality
  Stale comment: line 19 says "parser rejects as PARSE000" but compiler actually emits `SPECIAL_DRIVER_IN_INDEX` correctly. Fix: update comment. (Also flagged by test_2_4 audit.)
* 3_1_SPECIAL_DRIVER_IN_INDEX-gnd_vcc_in_index.jz : test-quality
  Same stale comment as above, and identical duplicate of `2_4_SPECIAL_DRIVER_IN_INDEX-gnd_vcc_in_index.jz`.
* Cross-section duplicates : test-quality
  The following 3_1_ files are byte-for-byte identical to their 2_4_/1_3_ counterparts: `3_1_SPECIAL_DRIVER_IN_EXPRESSION-gnd_vcc_in_expr.jz` = `2_4_...`, `3_1_SPECIAL_DRIVER_IN_CONCAT-gnd_vcc_in_concat.jz` = `2_4_...`, `3_1_SPECIAL_DRIVER_IN_INDEX-gnd_vcc_in_index.jz` = `2_4_...`, `3_1_SPECIAL_DRIVER_SLICED-vcc_gnd_sliced.jz` = `1_3_...`. Fix: remove duplicates or differentiate scenarios. (Also flagged by test_2_4 audit.)
* Plan section 4 : test-quality
  Plan lists `3_2_OPERATOR_SEMANTICS-operator_semantics_ok.jz` but actual file is `3_2_HAPPY_PATH-operator_semantics_ok.jz`. Fix: update plan to match actual filename.

## test_3_4-operator_examples.md

* Plan section 4 : test-quality
  Lists `3_4_OPERATOR_EXAMPLES-spec_examples_ok.jz` but actual file is `3_4_HAPPY_PATH-operator_examples_ok.jz`. Fix: update plan to match actual filename.
* 3_1_TERNARY_BRANCH_WIDTH_MISMATCH-branch_mismatch.jz : test-quality
  Cross-file duplicate: byte-for-byte identical to `3_2_TERNARY_BRANCH_WIDTH_MISMATCH-branch_width_mismatch.jz`.
* 3_1_CONCAT_EMPTY-empty_concatenation.jz : test-quality
  Cross-file duplicate: byte-for-byte identical to `3_2_CONCAT_EMPTY-empty_concatenation.jz`.
* 3_1_DIV_CONST_ZERO-constant_zero_divisor.jz : test-quality
  Cross-file duplicate: byte-for-byte identical to `3_2_DIV_CONST_ZERO-constant_zero_divisor.jz`.
* 3_1_DIV_UNGUARDED_RUNTIME_ZERO-unguarded_division.jz : test-quality
  Cross-file duplicate: byte-for-byte identical to `3_2_DIV_UNGUARDED_RUNTIME_ZERO-unguarded_division.jz`.
* 3_1_TERNARY_COND_WIDTH_NOT_1-multibit_condition.jz : test-quality
  Cross-file duplicate: byte-for-byte identical to `3_2_TERNARY_COND_WIDTH_NOT_1-multibit_condition.jz`.
* 2_4_SPECIAL_DRIVER_IN_EXPRESSION-gnd_vcc_in_expr.jz : test-quality
  Cross-file duplicate: byte-for-byte identical to `3_1_SPECIAL_DRIVER_IN_EXPRESSION-gnd_vcc_in_expr.jz`.
* 2_4_SPECIAL_DRIVER_IN_CONCAT-gnd_vcc_in_concat.jz : test-quality
  Cross-file duplicate: byte-for-byte identical to `3_1_SPECIAL_DRIVER_IN_CONCAT-gnd_vcc_in_concat.jz`.
* 1_3_SPECIAL_DRIVER_SLICED-vcc_gnd_sliced.jz : test-quality
  Cross-file duplicate: byte-for-byte identical to `3_1_SPECIAL_DRIVER_SLICED-vcc_gnd_sliced.jz`.
* 2_4_SPECIAL_DRIVER_IN_INDEX-gnd_vcc_in_index.jz : test-quality
  Cross-file duplicate: byte-for-byte identical to `3_1_SPECIAL_DRIVER_IN_INDEX-gnd_vcc_in_index.jz`.

## test_4_10-asynchronous_block.md

* ASYNC_FLOATING_Z_READ : compiler-bug
  Rule ID in plan 5.1 — not present in `compiler/src/rules.c`. Listed as error rule for "Reading a net whose only driver is z (floating)" but no such rule exists. Likely never implemented or renamed.
* ASYNC_UNDEFINED_PATH_NO_DRIVER : compiler-bug
  Nested IF with partial inner coverage (`IF (a) { IF (b) { out <= in; } } ELSE { out <= in; }`) does not trigger ASYNC_UNDEFINED_PATH_NO_DRIVER when `a=1, b=0` leaves signal undriven. The compiler does not analyze sub-paths within an outer IF branch that has at least one assignment. Only the IF/ELIF-without-ELSE variant was kept in the final test. Found during sweep for test_4_10.
* 5_0_WIDTH_ASSIGN_MISMATCH_NO_EXT-alias_width_mismatch.jz : test-quality
  Wrong rule triggered: test is named for WIDTH_ASSIGN_MISMATCH_NO_EXT but compiler emits ASSIGN_WIDTH_NO_MODIFIER (higher-priority rule suppresses it). The .out file contains zero WIDTH_ASSIGN_MISMATCH_NO_EXT diagnostics. Fix: create a test that actually triggers WIDTH_ASSIGN_MISMATCH_NO_EXT, or determine if the rule is dead code.
* 4_10_ASYNC_INVALID_STATEMENT_TARGET-invalid_lhs_in_async.jz : test-quality
  Limited target variety: only tests CONST as invalid LHS via `<=`. Missing: `=` and `=>` operator variants with CONST, and other non-assignable targets mentioned in the rule message (e.g. function call). Fix: add triggers for `=`/`=>` to CONST and other invalid target types.

## test_4_12-cdc_block.md

* CDC_DEST_ALIAS_DUP : compiler-bug
  CDC_DEST_ALIAS_DUP does not fire when dest alias conflicts with a wire name; only ID_DUP_IN_MODULE fires. The rule fires correctly for register, port, const, and instance conflicts but not wire. File attempted: `4_12_CDC_DEST_ALIAS_DUP-conflict_with_wire.jz`.
* CDC_DEST_ALIAS_DUP : compiler-bug
  CDC_DEST_ALIAS_DUP does not fire when dest alias conflicts with another CDC dest alias; only ID_DUP_IN_MODULE fires. Two CDC entries with the same dest alias name should trigger CDC_DEST_ALIAS_DUP on the second entry. File attempted: `4_12_CDC_DEST_ALIAS_DUP-conflict_with_alias.jz`.
* CDC_SOURCE_NOT_PLAIN_REG : compiler-bug
  Parser intercepts concatenation syntax `{a, b}` as CDC source with PARSE000 before CDC_SOURCE_NOT_PLAIN_REG semantic check runs. The CDC parser expects a bare identifier token; `{` is not valid at that position. Rule is unreachable for concatenation context. File attempted: `4_12_CDC_SOURCE_NOT_PLAIN_REG-concat_source.jz`.
* CDC_SOURCE_NOT_PLAIN_REG : compiler-bug
  Parser intercepts expression syntax `(a & b)` as CDC source with PARSE000 before CDC_SOURCE_NOT_PLAIN_REG semantic check runs. Same root cause as concat: CDC parser requires identifier token; `(` is rejected. Rule is only reachable via slice syntax (e.g., `reg[0:0]`). File attempted: `4_12_CDC_SOURCE_NOT_PLAIN_REG-expr_source.jz`.
* CDC_DEST_ALIAS_DUP : missing-context
  Covered: conflict with register name, conflict with port name, conflict with const name (resolved), conflict with instance name (resolved); missing: conflict with wire name, conflict with another CDC dest alias. Recommended new files: `4_12_CDC_DEST_ALIAS_DUP-conflict_with_wire.jz`, `4_12_CDC_DEST_ALIAS_DUP-conflict_with_alias.jz`. Note: sweep confirmed both are compiler-bugs.
* CDC_SOURCE_NOT_PLAIN_REG : missing-context
  Covered: sliced register; missing: concatenation as source, expression as source. Recommended new files: `4_12_CDC_SOURCE_NOT_PLAIN_REG-concat_source.jz`, `4_12_CDC_SOURCE_NOT_PLAIN_REG-expr_source.jz`. Note: sweep confirmed both are compiler-bugs (parser intercepts before semantic check).
* 4_12_CDC_SOURCE_NOT_PLAIN_REG-sliced_source.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNSINKED_REGISTER` at lines 22 and 57 because the rejected CDC slice source leaves wide_reg/top_reg with no read path. Fix: add an explicit combinational read of wide_reg/top_reg (e.g., wire them to an output port) to eliminate unrelated warnings.
* 4_12_CDC_DEST_ALIAS_DUP-alias_name_conflict.jz : test-quality
  Scaffolding: `.out` includes cascading `CDC_DEST_ALIAS_ASSIGNED` at line 40 because `existing_reg` is both a register and a (duplicate) dest alias, so writing to the register triggers the alias-assign rule. Also includes `ID_DUP_IN_MODULE` at lines 29 and 62 as cascading from the same duplicate name. These are legitimate cascading diagnostics but add noise. Fix: use a register name that isn't written in any block to avoid triggering `CDC_DEST_ALIAS_ASSIGNED`.

## test_4_14-feature_guards.md

* FEATURE_NESTED : compiler-bug
  Nested @feature at module level (outside any block like REGISTER, WIRE, ASYNCHRONOUS, SYNCHRONOUS) does not fire FEATURE_NESTED. The compiler silently accepts the nesting and only emits warnings for unused declarations inside the inner @feature. Nesting is correctly detected inside block contexts (ASYNC, SYNC, REGISTER, WIRE) but not at the bare module level. File attempted: `4_14_FEATURE_NESTED-nested_feature_at_module_level.jz`.
* FEATURE_VALIDATION_BOTH_PATHS : compiler-bug
  Rule ID in plan section 3 (I/O matrix rows 4 and 6) — not present in `compiler/src/rules.c`. The plan references this as an expected rule for both-path validation failures, but no such rule exists in the compiler. Both-path validation may produce other existing diagnostics (e.g. `UNDECLARED_IDENTIFIER`, port-undriven errors) rather than a dedicated rule.
* FEATURE_NESTED : missing-context
  Covered: ASYNCHRONOUS, SYNCHRONOUS, REGISTER, @else body, WIRE block (resolved by sweep); missing: module level. Recommended new file: `4_14_FEATURE_NESTED-nested_feature_at_module_level.jz`. Note: sweep confirmed compiler-bug — nesting not detected at module level.
* 4_14_FEATURE_COND_WIDTH_NOT_1-wide_cond_in_decl.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_REGISTER` (line 40) and `WARN_UNUSED_WIRE` (line 47) from declarations inside error-triggering feature guards that exist solely as trigger content. Fix: add reads/writes for `r2` and `w` in ASYNCHRONOUS/SYNCHRONOUS blocks to eliminate unrelated warnings.
* 4_14_FEATURE_EXPR_INVALID_CONTEXT-signal_in_decl_block.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_REGISTER` (line 40) and `WARN_UNUSED_WIRE` (line 47) from declarations inside error-triggering feature guards. Fix: same as above — use `r2` and `w` in logic blocks.
* 4_14_FEATURE_NESTED-nested_feature_in_register.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_REGISTER` (line 32) for `r2` declared inside nested feature guard. Fix: add a read of `r2` in ASYNCHRONOUS block.

## test_4_2-scope_and_uniqueness.md

* UNDECLARED_IDENTIFIER : compiler-bug
  @new targeting non-existent module fires INSTANCE_UNDEFINED_MODULE (S4.13/S6.9) instead of UNDECLARED_IDENTIFIER. The compiler uses a dedicated, more specific rule for this context. File attempted: `4_2_UNDECLARED_IDENTIFIER-nonexistent_module.jz`.
* UNDECLARED_IDENTIFIER : missing-context
  Covered: ASYNC RHS, SYNC RHS, CLK parameter, @new port binding value, instance port reference (inst.bad_port), RESET parameter (resolved), MUX select expression (resolved), slice index and concat operand (resolved); missing: @new target module name (non-existent module). Recommended new file: `4_2_UNDECLARED_IDENTIFIER-nonexistent_module.jz`. Note: sweep found rule-not-fired — INSTANCE_UNDEFINED_MODULE preempts.
* BLACKBOX_NAME_DUP_IN_PROJECT : missing-happy-path
  The shared happy-path file `4_2_HAPPY_PATH-scope_uniqueness_ok.jz` does not include any `@blackbox` declarations, so there is no happy-path regression test for unique blackbox names. Recommended: `4_2_HAPPY_PATH-blackbox_unique_ok.jz`.
* 4_2_BLACKBOX_NAME_DUP_IN_PROJECT-blackbox_name_conflicts.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_MODULE` (line 45:1) for `SharedName` module that exists solely to trigger the blackbox-module name collision. Fix: restructure so SharedName is instantiated elsewhere or accept this as an inherent consequence of the test scenario.

## test_4_3-const.md

* CONST_CIRCULAR_DEP : compiler-bug
  Three-member transitive cycle (A=B; B=C; C=A) fires CONST_NEGATIVE_OR_NONINT instead of CONST_CIRCULAR_DEP. Compiler detects direct 2-member cycles but fails to detect longer transitive chains as circular dependencies. File attempted: `4_3_CONST_CIRCULAR_DEP-transitive_chain.jz`.
* CONST_UNDEFINED_IN_WIDTH_OR_SLICE : compiler-bug
  MEM depth with undefined CONST fires MEM_UNDEFINED_CONST_IN_WIDTH (S7.1/S7.7.1) instead of CONST_UNDEFINED_IN_WIDTH_OR_SLICE. The MEM-specific rule takes precedence. File attempted: `4_3_CONST_UNDEFINED_IN_WIDTH_OR_SLICE-mem_depth.jz`.
* CONST_UNDEFINED_IN_WIDTH_OR_SLICE : compiler-bug
  Undefined CONST in slice expression (e.g., `din[TOP_HI:UNDEF_LO]`) fires UNDECLARED_IDENTIFIER (S4.2/S8.1) instead of CONST_UNDEFINED_IN_WIDTH_OR_SLICE. Slice bounds are not treated as CONST-expected contexts. File attempted: `4_3_CONST_UNDEFINED_IN_WIDTH_OR_SLICE-slice_context.jz`.
* CONST_CIRCULAR_DEP : missing-context
  Covered: direct 2-member cycle (A=B; B=A) in helper and top modules, self-reference (resolved); missing: 3-member transitive cycle (A=B; B=C; C=A). Recommended new file: `4_3_CONST_CIRCULAR_DEP-transitive_chain.jz`. Note: sweep confirmed compiler-bug — transitive chains not detected.
* CONST_UNDEFINED_IN_WIDTH_OR_SLICE : missing-context
  Covered: input port width, wire width, register width, output port width (resolved); missing: MEM depth, slice context. Recommended new files: `4_3_CONST_UNDEFINED_IN_WIDTH_OR_SLICE-mem_depth.jz`, `4_3_CONST_UNDEFINED_IN_WIDTH_OR_SLICE-slice_context.jz`. Note: sweep confirmed both are preempted by other rules.
* Plan section 4 / 5.1 inconsistency : test-quality
  Plan Section 4 lists test files for CONST_STRING_IN_NUMERIC_CONTEXT, CONST_NUMERIC_IN_STRING_CONTEXT, and CONST_USED_WHERE_FORBIDDEN, but these rules are absent from Section 5.1. In `rule_coverage.md` these rules are tracked under `test_6_3-config_block.md`. Fix: either add these 3 rules to Section 5.1 or remove the corresponding files from Section 4 to avoid confusion.

## test_4_4-port.md

* ASYNC_ALIAS_IN_CONDITIONAL : missing-happy-path
  No dedicated happy-path file; valid `<=` inside IF is tested inline in the error test but the general happy-path file doesn't exercise alias `=` at ASYNC root (valid context). Recommended: `4_4_ASYNC_ALIAS_IN_CONDITIONAL-valid_alias_ok.jz`.
* 4_4_BUS_PORT_NOT_BUS-member_on_non_bus.jz : test-quality
  Stale comment: line 3-4 says "NOTE: BUS_PORT_NOT_BUS is not emitted by the compiler" but the compiler DOES emit BUS_PORT_NOT_BUS correctly. Fix: remove the stale NOTE comment (lines 3-4).
* 4_4_BUS_PORT_UNKNOWN_BUS-unknown_bus_name.jz : test-quality
  Stale comment: line 3 says "NOTE: BUS_PORT_UNKNOWN_BUS is not emitted by the compiler" but the compiler DOES emit BUS_PORT_UNKNOWN_BUS correctly. Fix: remove the stale NOTE comment (line 3).
* 4_4_BUS_PORT_ARRAY_COUNT_INVALID-bad_array_count.jz : test-quality
  Scaffolding: WARN_UNUSED_PORT co-fires at line 25 for the zero-count BUS port. This is a legitimate cascading effect (zero-count port has no usable signals), not a scaffolding defect.

## test_4_5-wire.md

* 4_5_WIRE_MULTI_DIMENSIONAL-multi_dim_helper.jz : test-quality
  Stale comment: lines 3-4 say "NOTE: Compiler emits PARSE000 instead of WIRE_MULTI_DIMENSIONAL because the parser cannot handle the second dimension bracket" but the compiler correctly emits WIRE_MULTI_DIMENSIONAL. Fix: remove the stale NOTE comment (lines 3-4).
* 4_5_WIRE_MULTI_DIMENSIONAL-multi_dim_top.jz : test-quality
  Stale comment: lines 3-4 say "NOTE: Compiler emits PARSE000 instead of WIRE_MULTI_DIMENSIONAL because the parser cannot handle the second dimension bracket" but the compiler correctly emits WIRE_MULTI_DIMENSIONAL. Fix: remove the stale NOTE comment (lines 3-4).

## test_4_6-mux.md

* MUX_AGG_SOURCE_INVALID : compiler-bug
  Compiler does not fire MUX_AGG_SOURCE_INVALID when an output port is used as a MUX aggregation source. Output ports are apparently valid readable signals in module scope for MUX purposes (the module can read the value it is driving). No diagnostic emitted at all. File attempted: `4_6_MUX_AGG_SOURCE_INVALID-output_port_source.jz`.
* MUX_NAME_DUPLICATE : compiler-bug
  Compiler fires INSTANCE_NAME_CONFLICT instead of MUX_NAME_DUPLICATE when a MUX name collides with an instance name. Tested both declaration orders (MUX before instance and instance before MUX) — INSTANCE_NAME_CONFLICT always fires. The instance-name checker catches the collision before the MUX-name checker runs. File attempted: `4_6_MUX_NAME_DUPLICATE-dup_instance.jz`.
* MUX_AGG_SOURCE_INVALID : missing-context
  Covered: undefined identifier, const as source (resolved), instance name as source (resolved); missing: output port as source. Recommended new file: `4_6_MUX_AGG_SOURCE_INVALID-output_port_source.jz`. Note: sweep found rule-not-fired — output ports are valid MUX sources.
* MUX_NAME_DUPLICATE : missing-context
  Covered: port name, wire name, register name, const name (resolved), another MUX name (resolved); missing: instance name. Recommended new file: `4_6_MUX_NAME_DUPLICATE-dup_instance.jz`. Note: sweep found rule-not-fired — INSTANCE_NAME_CONFLICT preempts.

## test_4_7-register.md

* REG_MISSING_INIT_LITERAL : compiler-bug
  (`4_7_REG_MISSING_INIT_LITERAL-missing_init.jz`) — .jz has 3 triggers (lines 22, 50, 52) but compiler only emits diagnostic for line 22 (HelperMod). TopMod triggers `r_noinit_top [8];` and `r_noinit_1bit [1];` are silently ignored. Minimal repro: two modules with missing register inits; only the first module's error fires.
* REG_MULTI_DIMENSIONAL : compiler-bug
  (`4_7_REG_MULTI_DIMENSIONAL-multi_dim_register.jz`) — .jz has 2 triggers (lines 22, 50) but compiler only emits diagnostic for line 22 (HelperMod). TopMod trigger `r_bad_top [16] [2];` is silently ignored. Minimal repro: two modules with multi-dimensional registers; only the first module's error fires.
* REG_HAPPY_PATH : missing-context
  Covered: standard 8-bit register, 1-bit register, multiple registers in one block, read in ASYNC, write in SYNC, read-current/write-next; missing: GND keyword reset, VCC keyword reset. Recommended: update `4_7_REG_HAPPY_PATH-register_ok.jz` to include `data [8] = GND;` and `flags [8] = VCC;`. Note: sweep skipped as "already exists" — existing file needs manual update.
* 4_7_WARN_UNDRIVEN_REGISTER-read_never_written.jz : test-quality
  Non-idiomatic syntax: lines 27 and 55 use `<=` (register write operator) in ASYNCHRONOUS blocks instead of `=` (combinational assignment). Fix: change `data_out <= r_undriven;` to `data_out = r_undriven;` and `dout <= r_undriven_top;` to `dout = r_undriven_top;`.

## test_4_8-latches.md

* LATCH_IN_CONST_CONTEXT : compiler-bug
  FEATURE_EXPR_INVALID_CONTEXT preempts LATCH_IN_CONST_CONTEXT in @feature guard condition — the generic @feature check rejects any non-CONFIG/CONST/literal reference before the latch-specific check runs. LATCH_IN_CONST_CONTEXT is only reachable via @check, not @feature. File attempted: `4_8_LATCH_IN_CONST_CONTEXT-latch_in_feature.jz`.
* LATCH_WIDTH_INVALID : compiler-bug
  (`4_8_LATCH_WIDTH_INVALID-invalid_latch_width.jz`) — rule is unreachable. Zero-width latch `lat_zero [0] D;` triggers generic WIDTH_NONPOSITIVE_OR_NONINT instead of latch-specific LATCH_WIDTH_INVALID. The generic width check fires before the latch-specific check runs. Either LATCH_WIDTH_INVALID should preempt the generic check, or the rule is dead code.
* LATCH_IN_CONST_CONTEXT : missing-context
  Covered: @check condition; missing: @feature guard condition. Recommended new file: `4_8_LATCH_IN_CONST_CONTEXT-latch_in_feature.jz`. Note: sweep found rule-not-fired — FEATURE_EXPR_INVALID_CONTEXT preempts.
* 4_8_LATCH_IN_CONST_CONTEXT-latch_in_const.jz : test-quality
  Stale comment: line 5 says "rule not implemented — compiler does not emit this diagnostic" but compiler does emit LATCH_IN_CONST_CONTEXT correctly. Fix: remove the stale comment.
* 4_8_LATCH_SR_WIDTH_MISMATCH-sr_width_mismatch.jz : test-quality
  Stale comment: line 5 says "rule not implemented — compiler does not emit this diagnostic" but compiler does emit LATCH_SR_WIDTH_MISMATCH correctly. Fix: remove the stale comment.
* 4_8_LATCH_AS_CLOCK_OR_CDC-latch_as_cdc_clock.jz : test-quality
  Scaffolding noise: .out includes MULTI_CLK_ASSIGN (line 29) and DOMAIN_CONFLICT (line 49) from CDC register setup, not from the LATCH_AS_CLOCK_OR_CDC rule under test. Fix: restructure test to isolate latch-as-CDC-clock triggers without cascading domain errors.

## test_5_0-assignment_operators_summary.md

* ASSIGN_INDEPENDENT_IF_SELECT : compiler-bug
  SYNC_MULTI_ASSIGN_SAME_REG_BITS fires instead — more specific SYNC rule preempts ASSIGN_INDEPENDENT_IF_SELECT in SYNCHRONOUS context. Correct compiler behavior. File attempted: `5_0_ASSIGN_INDEPENDENT_IF_SELECT-sync_context.jz`.
* ASSIGN_MULTIPLE_SAME_BITS : compiler-bug
  SYNC_MULTI_ASSIGN_SAME_REG_BITS fires instead — more specific SYNC rule preempts ASSIGN_MULTIPLE_SAME_BITS in SYNCHRONOUS context. Correct compiler behavior. File attempted: `5_0_ASSIGN_MULTIPLE_SAME_BITS-sync_double_assign.jz`.
* ASSIGN_SHADOWING : compiler-bug
  SYNC_ROOT_AND_CONDITIONAL_ASSIGN fires instead — more specific SYNC rule preempts ASSIGN_SHADOWING in SYNCHRONOUS context. Correct compiler behavior. File attempted: `5_0_ASSIGN_SHADOWING-sync_context.jz`.
* ASSIGN_MULTIPLE_SAME_BITS : missing-context
  Covered: unconditional double assign to port/wire in ASYNC, template @apply double assign in ASYNC; missing: SYNCHRONOUS context (double register assignment on same path). Recommended new file: `5_0_ASSIGN_MULTIPLE_SAME_BITS-sync_double_assign.jz`. Note: sweep found rule-not-fired — SYNC_MULTI_ASSIGN_SAME_REG_BITS preempts.
* ASSIGN_INDEPENDENT_IF_SELECT : missing-context
  Covered: independent IFs on port/wire (ASYNC), independent SELECTs on port (ASYNC); missing: SYNCHRONOUS context. Recommended new file: `5_0_ASSIGN_INDEPENDENT_IF_SELECT-sync_context.jz`. Note: sweep found rule-not-fired — SYNC_MULTI_ASSIGN_SAME_REG_BITS preempts. Mixed IF-then-SELECT context was resolved.
* ASSIGN_SHADOWING : missing-context
  Covered: root-then-IF on port/wire (ASYNC), root-then-SELECT on port (ASYNC); missing: SYNCHRONOUS context (root register assignment shadowed by nested IF). Recommended new file: `5_0_ASSIGN_SHADOWING-sync_context.jz`. Note: sweep found rule-not-fired — SYNC_ROOT_AND_CONDITIONAL_ASSIGN preempts.
* 5_0_WIDTH_ASSIGN_MISMATCH_NO_EXT-alias_width_mismatch.jz : test-quality
  Rule suppression: test is named for WIDTH_ASSIGN_MISMATCH_NO_EXT but ASSIGN_WIDTH_NO_MODIFIER (higher priority) always fires instead. The .out contains only ASSIGN_WIDTH_NO_MODIFIER diagnostics. The test documents the suppression behavior but does NOT validate WIDTH_ASSIGN_MISMATCH_NO_EXT. Fix: either find a scenario where WIDTH_ASSIGN_MISMATCH_NO_EXT fires without being suppressed, or reclassify this test as documenting suppression behavior only.
* 5_0_ASSIGN_SLICE_WIDTH_MISMATCH-slice_width_mismatch.jz : test-quality
  Cross-rule firing: SYNC triggers at lines 39 and 72 fire SYNC_SLICE_WIDTH_MISMATCH instead of ASSIGN_SLICE_WIDTH_MISMATCH. This is correct compiler behavior (SYNC has its own rule) but means the test does not validate ASSIGN_SLICE_WIDTH_MISMATCH in SYNC context. Consider: is this the intended design, or should ASSIGN_SLICE_WIDTH_MISMATCH also fire in SYNC?

## test_5_1-asynchronous_assignments.md

* ASYNC_FLOATING_Z_READ : compiler-bug
  Rule ID in plan 5.1 table — not present in `compiler/src/rules.c`. Rule was never implemented or has been renamed/removed. Plan lists test file `5_1_ASYNC_FLOATING_Z_READ-floating_z_read.jz` which does not exist.
* ASYNC_INVALID_STATEMENT_TARGET : compiler-bug
  Function call on LHS (e.g. `clog2(8) <= din;`) gets PARSE000 before semantic analysis; ASYNC_INVALID_STATEMENT_TARGET is unreachable for function-call-on-LHS context. Only CONST on LHS was testable. Observed during sweep for `5_1_ASYNC_INVALID_STATEMENT_TARGET-const_and_func.jz`.
* ASYNC_UNDEFINED_PATH_NO_DRIVER : missing-coverage
  (`error`, `S1.5/S4.10/S5.1`) — no `5_1_` prefixed validation file exists. Rule is tested by `1_5_ASYNC_UNDEFINED_PATH_NO_DRIVER-partial_coverage.jz` and `4_10_ASYNC_UNDEFINED_PATH_NO_DRIVER-partial_coverage.jz` (cross-section). Recommended: `5_1_ASYNC_UNDEFINED_PATH_NO_DRIVER-partial_coverage.jz`.
* 5_1_ASYNC_INVALID_STATEMENT_TARGET-mem_sync_in_async.jz : test-quality
  Wrong rule tested: file is named for ASYNC_INVALID_STATEMENT_TARGET but compiler fires MEM_SYNC_ADDR_IN_ASYNC_BLOCK (a MEM_ACCESS rule). The .jz uses MEM SYNC OUT port `.addr` assignment in ASYNC, which triggers the more specific MEM rule, not the general ASYNC_INVALID_STATEMENT_TARGET. Fix: rewrite test to use a non-assignable target that actually triggers ASYNC_INVALID_STATEMENT_TARGET (e.g. CONST on LHS as in `4_10_ASYNC_INVALID_STATEMENT_TARGET-invalid_lhs_in_async.jz`).
* 5_1_ASYNC_ASSIGN_REGISTER-register_in_async.jz : test-quality
  Exact duplicate: file is byte-identical to `4_7_ASYNC_ASSIGN_REGISTER-register_in_async.jz`. Both produce identical .out. One copy is sufficient; the duplicate adds maintenance burden without additional coverage.

## test_5_2-synchronous_assignments.md

* SYNC_MULTI_ASSIGN_SAME_REG_BITS : compiler-bug
  Compiler fires ASSIGN_SLICE_OVERLAP instead of SYNC_MULTI_ASSIGN_SAME_REG_BITS for overlapping slice assignments. The more specific ASSIGN_SLICE_OVERLAP rule takes priority over the general double-assign rule when slices overlap. File attempted: `5_2_SYNC_MULTI_ASSIGN_SAME_REG_BITS-overlapping_slices.jz`.
* SYNC_ROOT_AND_CONDITIONAL_ASSIGN : compiler-bug
  Compiler fires ASSIGN_SLICE_OVERLAP instead of SYNC_ROOT_AND_CONDITIONAL_ASSIGN for root-level sliced assign + conditional sliced assign to overlapping bits. Same priority issue as above — ASSIGN_SLICE_OVERLAP takes precedence. File attempted: `5_2_SYNC_ROOT_AND_CONDITIONAL_ASSIGN-sliced_root_conflict.jz`.
* SYNC_MULTI_ASSIGN_SAME_REG_BITS : missing-context
  Covered: full-register double assign at root, inside IF branch; missing: overlapping slice assignments (e.g., `r[7:4] <= x; r[5:2] <= y;` where bits [5:4] are assigned twice via overlapping slices). Recommended new file: `5_2_SYNC_MULTI_ASSIGN_SAME_REG_BITS-overlapping_slices.jz`. Note: sweep found ASSIGN_SLICE_OVERLAP preempts.
* SYNC_ROOT_AND_CONDITIONAL_ASSIGN : missing-context
  Covered: root + IF, root + SELECT with CASE/DEFAULT; missing: root-level sliced assign + conditional sliced assign to overlapping bits. Recommended new file: `5_2_SYNC_ROOT_AND_CONDITIONAL_ASSIGN-sliced_root_conflict.jz`. Note: sweep found ASSIGN_SLICE_OVERLAP preempts.

## test_5_4-select_case_statements.md

* SELECT_DUP_CASE_VALUE : compiler-bug
  Compiler does not detect SELECT_DUP_CASE_VALUE when both CASE values are @global constants with the same numeric value (e.g., OPCODES.NOP=4'h0 and OPCODES.HALT=4'h0). Also not detected when @global constant duplicates a bare integer (e.g., OPCODES.SUB=4'h2 and CASE 2). The compiler likely does not resolve @global constants to their numeric values during duplicate CASE detection. File attempted: `5_4_SELECT_DUP_CASE_VALUE-global_dup.jz`.
* SELECT_DEFAULT_RECOMMENDED_ASYNC : missing-coverage
  (`warning`, `S5.4/S8.3`) — no validation file triggers this rule. The test `5_4_SELECT_DEFAULT_RECOMMENDED_ASYNC-async_select_no_default.jz` is misnamed: it triggers `WARN_INCOMPLETE_SELECT_ASYNC` (incomplete coverage) instead of `SELECT_DEFAULT_RECOMMENDED_ASYNC` (complete coverage, readability recommendation). To trigger `SELECT_DEFAULT_RECOMMENDED_ASYNC`, a SELECT must have complete CASE coverage for all possible selector values but omit DEFAULT. Recommended: `5_4_SELECT_DEFAULT_RECOMMENDED_ASYNC-complete_coverage_no_default.jz`.
* SELECT_DUP_CASE_VALUE : missing-context
  Covered: bare integer dup, sized hex dup, sized binary dup, ASYNC, SYNC, nested SELECT, helper module, fall-through CASE duplicate (resolved), x-wildcard overlapping patterns (resolved); missing: `@global` constant duplicate. Recommended new file: `5_4_SELECT_DUP_CASE_VALUE-global_dup.jz`. Note: sweep confirmed compiler-bug — @global constants not resolved during dup detection.
* SELECT with @global CASE : missing-happy-path
  No test exercises `@global` constant as CASE value. Recommended: `5_4_HAPPY_PATH-select_global_case_ok.jz`.
* 5_4_SELECT_DEFAULT_RECOMMENDED_ASYNC-async_select_no_default.jz : test-quality
  Misnamed: file is named for `SELECT_DEFAULT_RECOMMENDED_ASYNC` but triggers `WARN_INCOMPLETE_SELECT_ASYNC` instead. The `.out` contains `ASYNC_UNDEFINED_PATH_NO_DRIVER` (cascading from missing DEFAULT in incomplete-coverage SELECT) and `WARN_INCOMPLETE_SELECT_ASYNC`, neither of which is the plan's target rule. Fix: rename file to match the rule it actually tests (`WARN_INCOMPLETE_SELECT_ASYNC`) or rewrite to trigger `SELECT_DEFAULT_RECOMMENDED_ASYNC` by providing complete CASE coverage without DEFAULT.

## test_5_5-intrinsic_operators.md

* CLOG2_NONPOSITIVE_ARG : compiler-bug
  clog2(0) in WIRE width bracket `w [clog2(0)]` compiles cleanly with no diagnostic — CLOG2_NONPOSITIVE_ARG is not checked in width-bracket expressions. File attempted: `5_5_CLOG2_NONPOSITIVE_ARG-width_bracket.jz`.
* CLOG2_NONPOSITIVE_ARG : missing-context
  Covered: CONST block initializer (helper + top); missing: width-bracket context (e.g., `WIRE { w [clog2(0)]; }`). Recommended new file: `5_5_CLOG2_NONPOSITIVE_ARG-width_bracket.jz`. Note: sweep confirmed compiler-bug — not checked in width-bracket context.
* 5_5_FUNC_RESULT_TRUNCATED_SILENTLY-intrinsic_truncation.jz : test-quality
  Stale comment: lines 5-6 say "Compiler bug — FUNC_RESULT_TRUNCATED_SILENTLY is not emitted; compiler fires ASSIGN_WIDTH_NO_MODIFIER instead" but the compiler correctly emits FUNC_RESULT_TRUNCATED_SILENTLY. Fix: remove the stale NOTE comment.
* 5_5_WIDTHOF_INVALID_SYNTAX-slice_argument.jz : test-quality
  Stale comment: line 5 says "NOTE: Compiler bug — this rule is not implemented" but the compiler correctly emits WIDTHOF_INVALID_SYNTAX. Fix: remove the stale NOTE comment.
* 5_5_WIDTHOF_INVALID_TARGET-non_signal_target.jz : test-quality
  Stale comment: line 5 says "NOTE: Compiler bug — this rule is not implemented" but the compiler correctly emits WIDTHOF_INVALID_TARGET. Fix: remove the stale NOTE comment.
* 5_5_WIDTHOF_WIDTH_NOT_RESOLVABLE-unresolvable_width.jz : test-quality
  Stale comment: line 5 says "NOTE: Compiler bug — this rule is not implemented" but the compiler correctly emits WIDTHOF_WIDTH_NOT_RESOLVABLE. Fix: remove the stale NOTE comment.
* 5_5_WIDTHOF_WIDTH_NOT_RESOLVABLE-unresolvable_width.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_WIRE` from `w_bad [UNDEFINED_CONST]` which is an artifact of the test setup, not the rule under test. Fix: drive or read `w_bad` minimally to suppress the warning, or accept as inherent to the scenario.

## test_6_1-project_purpose.md

* PROJECT_CHIP_DATA_NOT_FOUND / PROJECT_CHIP_DATA_INVALID : missing-happy-path
  Existing happy-path (`6_1_HAPPY_PATH-project_chip_ok.jz`) uses default GENERIC (no CHIP property). No happy-path tests the valid form with an explicit `CHIP=<known_chip>` property. Recommended: `6_1_HAPPY_PATH-project_specific_chip_ok.jz`.

## test_6_2-project_canonical_form.md

* IMPORT_DUP_MODULE_OR_BLACKBOX : compiler-bug
  Blackbox-blackbox name collision across two imports is not detected; compiler produces no diagnostic. Rule only fires for module-module collisions. File attempted: `6_2_IMPORT_DUP_MODULE_OR_BLACKBOX-blackbox_collision.jz`.
* IMPORT_DUP_MODULE_OR_BLACKBOX : compiler-bug
  Module-blackbox cross-type name collision across two imports is not detected; compiler emits WARN_UNUSED_MODULE for the module but no IMPORT_DUP_MODULE_OR_BLACKBOX. File attempted: `6_2_IMPORT_DUP_MODULE_OR_BLACKBOX-module_blackbox_cross.jz`.
* IMPORT_DUP_MODULE_OR_BLACKBOX : compiler-bug
  Imported-vs-locally-defined collision fires MODULE_NAME_DUP_IN_PROJECT instead of IMPORT_DUP_MODULE_OR_BLACKBOX. Compiler detects the collision but attributes it to the wrong rule. File attempted: `6_2_IMPORT_DUP_MODULE_OR_BLACKBOX-imported_vs_local.jz`.
* IMPORT_DUP_MODULE_OR_BLACKBOX : missing-context
  Covered: module-module collision between two imported files; missing: blackbox-blackbox collision, module-blackbox cross-type collision, imported-vs-locally-defined collision. Recommended new files: `6_2_IMPORT_DUP_MODULE_OR_BLACKBOX-blackbox_collision.jz`, `6_2_IMPORT_DUP_MODULE_OR_BLACKBOX-module_blackbox_cross.jz`, `6_2_IMPORT_DUP_MODULE_OR_BLACKBOX-imported_vs_local.jz`. Note: sweep confirmed all three are compiler-bugs.

## test_6_3-config_block.md

* CONFIG_USE_UNDECLARED : compiler-bug
  MEM depth context fires MEM_UNDEFINED_CONST_IN_WIDTH instead of CONFIG_USE_UNDECLARED; MEM parsing has its own undeclared-const check that takes precedence. File attempted: `6_3_CONFIG_USE_UNDECLARED-mem_depth.jz`.
* CONFIG_USE_UNDECLARED : compiler-bug
  Instance port binding width context fires INSTANCE_PORT_WIDTH_EXPR_INVALID instead of CONFIG_USE_UNDECLARED; instance binding has its own validation path. File attempted: `6_3_CONFIG_USE_UNDECLARED-instance_binding.jz`.
* CONST_NUMERIC_IN_STRING_CONTEXT : compiler-bug
  (`6_3_CONST_NUMERIC_IN_STRING_CONTEXT-numeric_as_string.jz`) — Compiler emits `UNDECLARED_IDENTIFIER` for `CONFIG.WIDTH` and `CONFIG.DEPTH` inside `@file()` even though they are declared in the project CONFIG block. The numeric-in-string-context error is correct, but the undeclared-identifier error is spurious.
* CONST_STRING_IN_NUMERIC_CONTEXT : compiler-bug
  MEM depth context fires MEM_UNDEFINED_CONST_IN_WIDTH instead of CONST_STRING_IN_NUMERIC_CONTEXT; string CONFIG value treated as undeclared in MEM context before type check runs. File attempted: `6_3_CONST_STRING_IN_NUMERIC_CONTEXT-mem_depth.jz`.
* CONFIG_USE_UNDECLARED : missing-context
  Covered: port width, CONST initializer, register width, wire width (resolved); missing: MEM depth, instance port binding width. Recommended new files: `6_3_CONFIG_USE_UNDECLARED-mem_depth.jz`, `6_3_CONFIG_USE_UNDECLARED-instance_binding.jz`. Note: sweep confirmed both are preempted by other rules.
* CONST_STRING_IN_NUMERIC_CONTEXT : missing-context
  Covered: port width, CONST initializer, register width, wire width (resolved); missing: MEM depth. Recommended new file: `6_3_CONST_STRING_IN_NUMERIC_CONTEXT-mem_depth.jz`. Note: sweep confirmed preempted by MEM_UNDEFINED_CONST_IN_WIDTH.
* 6_3_CONFIG_MULTIPLE_BLOCKS-multiple_config.jz : test-quality
  Scaffolding: `.out` includes `ASSIGN_WIDTH_NO_MODIFIER` at line 47:9 because register `r [1]` is assigned to `data` which is `[CONFIG.WIDTH]` (8-bit). Fix: change `r [1]` to `r [CONFIG.WIDTH]` and update `r <= ~r` to match width, or use a 1-bit `data` port.
* 6_3_CONST_NUMERIC_IN_STRING_CONTEXT-numeric_as_string.jz : test-quality
  Scaffolding/Possible bug: `.out` includes `UNDECLARED_IDENTIFIER` at lines 27:36 and 55:36 alongside `CONST_NUMERIC_IN_STRING_CONTEXT`. `CONFIG.WIDTH` and `CONFIG.DEPTH` are declared numeric CONFIG entries — the `UNDECLARED_IDENTIFIER` diagnostic is spurious. Likely a compiler issue where the @file() path resolution emits UNDECLARED_IDENTIFIER before/alongside the type mismatch check.

## test_6_4-clocks_block.md

* 6_4_CLOCK_GEN_OUTPUT_HAS_PERIOD-gen_out_with_period.jz : test-quality
  Scaffolding: `.out` includes 2x `CLOCK_NAME_NOT_IN_PINS` and 2x `CLOCK_SOURCE_AMBIGUOUS` alongside target `CLOCK_GEN_OUTPUT_HAS_PERIOD`. These fire because CLOCK_GEN output clocks with periods trigger unrelated clock-source checks. Fix: restructure test so CLOCK_GEN output clocks don't trigger IN_PINS/ambiguity rules, or split into separate files.
* 6_4_CLOCK_SOURCE_AMBIGUOUS-dual_source.jz : test-quality
  Scaffolding: `.out` includes 2x `CLOCK_NAME_NOT_IN_PINS` and 2x `CLOCK_GEN_OUTPUT_HAS_PERIOD` alongside target `CLOCK_SOURCE_AMBIGUOUS`. These are inherent cascades from the ambiguity scenario (period on a CLOCK_GEN output). Fix: accept as inherent or restructure to isolate the ambiguity trigger.
* 6_4_CLOCK_GEN_PAD_EXCLUSIVE_CONFLICT-ref_clk_as_logic.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_PORT` at line 46:17 for unused `slow` port. Fix: use `slow` in the module body or remove it from PORT.

## test_6_6-map_block.md

* MAP_INVALID_BOARD_PIN_ID : missing-happy-path
  No happy-path file with `CHIP=` that verifies valid board pin IDs pass chip-specific validation. The existing `6_6_HAPPY_PATH-map_ok.jz` has no CHIP directive so chip-specific pin ID validation is not exercised. Recommended: `6_6_HAPPY_PATH-map_with_chip_ok.jz`.
* Plan section 4 : test-quality
  `6_6_MAP_DUP_PHYSICAL_LOCATION-diff_duplicate.jz` exists but is not listed in the plan's Section 4 (Existing Validation Tests) table. Fix: update plan to include this file.
* Plan section 4 : test-quality
  `6_6_MAP_INVALID_BOARD_PIN_ID-bad_pin_format.jz` exists but is not listed in the plan's Section 4 (Existing Validation Tests) table. Fix: update plan to include this file.

## test_6_7-blackbox_modules.md

* BLACKBOX_BODY_DISALLOWED : compiler-bug
  (`6_7_BLACKBOX_BODY_DISALLOWED-const_in_blackbox.jz`) — Rule fires for CONST blocks in @blackbox, but spec S6.7 (line 4192) lists only "no ASYNCHRONOUS, SYNCHRONOUS, WIRE, REGISTER, or MEM blocks" — CONST is not forbidden. Furthermore, spec S7.11 (line 5689) says "Blackbox CONST values are lowered to Verilog localparam declarations," implying CONST is intended to be valid. The rule message also omits CONST from its list: "(ASYNCHRONOUS/SYNCHRONOUS/WIRE/REGISTER/MEM)". Either the compiler should allow CONST in blackboxes (spec intent) or the spec and rule message need updating to include CONST.
* BLACKBOX_BODY_DISALLOWED : compiler-bug
  Parser emits PARSE000 ("unexpected token in @blackbox body; expected CONST or PORT") before semantic rule BLACKBOX_BODY_DISALLOWED can fire for ASYNCHRONOUS blocks. Rule is unreachable for ASYNC context. File attempted: `6_7_BLACKBOX_BODY_DISALLOWED-async_in_blackbox.jz`.
* BLACKBOX_BODY_DISALLOWED : compiler-bug
  Parser emits PARSE000 before BLACKBOX_BODY_DISALLOWED can fire for REGISTER blocks. Rule is unreachable for REGISTER context. File attempted: `6_7_BLACKBOX_BODY_DISALLOWED-register_in_blackbox.jz`.
* BLACKBOX_BODY_DISALLOWED : compiler-bug
  Parser emits PARSE000 before BLACKBOX_BODY_DISALLOWED can fire for WIRE blocks. Rule is unreachable for WIRE context. File attempted: `6_7_BLACKBOX_BODY_DISALLOWED-wire_in_blackbox.jz`.
* BLACKBOX_BODY_DISALLOWED : missing-context
  Covered: CONST block (single entry, multiple entries, across multiple blackboxes); missing: ASYNCHRONOUS, SYNCHRONOUS, WIRE, REGISTER, MEM blocks inside @blackbox. Note: parser rejects these with PARSE000 before the semantic rule fires, so BLACKBOX_BODY_DISALLOWED is unreachable for those block types. CONST is the only block that bypasses the parser and reaches semantic analysis. Sweep confirmed all three attempted contexts hit PARSE000.
* 6_7_BLACKBOX_NAME_DUP_IN_PROJECT-blackbox_name_conflicts.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_MODULE` at line 46:1 for `ConflictMod` which exists solely to trigger the bb-module name conflict but is never instantiated. Fix: instantiate `ConflictMod` in `TopMod` (or accept the warning as inherent since the conflicting name prevents clean instantiation).

## test_6_8-bus_aggregation.md

* BUS_DEF_INVALID_DIR : missing-coverage
  (`error`, `S6.8`) — no validation file exists. Plan says "(planned)". `rule_coverage.md` incorrectly marks this as "Tested". Recommended: `6_8_BUS_DEF_INVALID_DIR-invalid_direction.jz`.
* BUS_DEF_INVALID_DIR : missing-happy-path
  No test exists at all (see Missing Coverage above). Recommended: `6_8_BUS_DEF_INVALID_DIR-valid_directions_ok.jz`.
* 6_8_HAPPY_PATH-bus_ok.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_PORT` at 80:31 for EnableMod's `ebus` BUS port, which is bound to `_` at all instantiation sites and never externally connected. Fix: add a bulk BUS assignment using `inst_e.ebus` in TopMod's ASYNCHRONOUS block (e.g., connect to a TARGET instance of ENABLE_BUS), or suppress by removing the EnableMod instantiation from this test.

## test_6_9-top_level_module.md

* HAPPY_PATH : missing-happy-path
  Existing happy path (`6_9_HAPPY_PATH-top_module_ok.jz`) covers IN/OUT ports but not INOUT ports bound to INOUT_PINS. Recommended: `6_9_HAPPY_PATH-top_module_inout_ok.jz`.
* TOP_PORT_SIGNAL_WIDTH_MISMATCH.jz : test-quality
  Naming: file uses non-standard name (no `6_9_` prefix). Fix: rename to `6_9_TOP_PORT_SIGNAL_WIDTH_MISMATCH-signal_width_mismatch.jz` (and `.out`).
* TOP_PORT_SIGNAL_WIDTH_MISMATCH.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_PORT` (line 39, clk port unused in module body) and `ASSIGN_WIDTH_NO_MODIFIER` (line 45, `leds <= sw` width mismatch unrelated to rule under test). Fix: use clk in a SYNCHRONOUS block and match widths in the ASYNCHRONOUS assignment.
* 6_9_INSTANCE_UNDEFINED_MODULE-top_undefined_module.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_MODULE` from `ActualModule` which is never instantiated. Fix: remove the `ActualModule` definition or give the @top a valid module name and trigger the error differently.

## test_6_10-project_scope_and_uniqueness.md

* 6_10_BLACKBOX_NAME_DUP_IN_PROJECT-blackbox_name_conflicts.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_MODULE` at line 46:1 for the `SharedName` module which exists solely to trigger the bb-module name conflict but is not instantiated. Fix: instantiate `SharedName` in `TopMod` (or accept the warning as inherent to the scenario since the conflicting name prevents clean instantiation).

## test_7_0-memory_port_modes.md

* MEM_INVALID_PORT_TYPE : compiler-bug
  Compiler fires dedicated `MEM_INOUT_ASYNC` rule (rules.c line 375) instead of `MEM_INVALID_PORT_TYPE` for INOUT+ASYNC context. Audit entry should reference `MEM_INOUT_ASYNC` and filename should be `7_0_MEM_INOUT_ASYNC-inout_async.jz`. File attempted: `7_0_MEM_INVALID_PORT_TYPE-inout_async.jz`.
* MEM_INVALID_WRITE_MODE : compiler-bug
  Shorthand form with unrecognized keyword (e.g. `IN wr BADVALUE;`) hits `PARSE000` at parser level instead of `MEM_INVALID_WRITE_MODE`. Parser expects a known token (ASYNC/SYNC/WRITE_FIRST/READ_FIRST/NO_CHANGE) after port name; unknown keywords are not recoverable to semantic validation. File attempted: `7_0_MEM_INVALID_WRITE_MODE-shorthand_invalid.jz`.
* MEM_INVALID_PORT_TYPE : missing-context
  Covered: `IN` port with `ASYNC`, `IN` port with `SYNC`; missing: `INOUT` port with `ASYNC`. Recommended new file: `7_0_MEM_INVALID_PORT_TYPE-inout_async.jz`. Note: sweep found rule-not-fired — MEM_INOUT_ASYNC fires instead.
* MEM_INVALID_WRITE_MODE : missing-context
  Covered: `IN` port attribute form (`WRITE_MODE = INVALID`), `INOUT` port attribute form (`WRITE_MODE = BADMODE`); missing: shorthand form with unrecognized keyword. Recommended new file: `7_0_MEM_INVALID_WRITE_MODE-shorthand_invalid.jz`. Note: sweep found rule-not-fired — PARSE000 intercepts.

## test_7_1-mem_declaration.md

* MEM_INVALID_PORT_TYPE : compiler-bug
  Truly unknown keywords (e.g. `GARBAGE rd;`, `OUT rd BLAH;`) fire `PARSE000` instead of `MEM_INVALID_PORT_TYPE`. The parser catches unknown tokens before the semantic rule can fire. The rule only triggers for known-but-misplaced qualifiers (ASYNC on IN, SYNC on IN). File attempted: `7_1_MEM_INVALID_PORT_TYPE-unknown_keyword.jz`.
* MEM_INVALID_PORT_TYPE : missing-context
  Covered: `ASYNC` on `IN` port, `SYNC` on `IN` port; missing: truly unknown keyword (e.g. `OUT rd BLAH;` or `GARBAGE rd;`). Recommended new file: `7_1_MEM_INVALID_PORT_TYPE-unknown_keyword.jz`. Note: sweep confirmed compiler-bug — parser intercepts before semantic rule.
* 7_1_MEM_CHIP_CONFIG_UNSUPPORTED-unsupported_chip.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_PORT` for HelperMod `addr` port (unused because MEM is invalid). Fix: use `addr` in a dummy expression or remove from HelperMod PORT.
* 7_1_MEM_EMPTY_PORT_LIST-no_ports.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_PORT` for HelperMod `addr` port. Fix: same as above.
* 7_1_MEM_INOUT_ASYNC-inout_async.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_PORT` for HelperMod `addr` port. Fix: same as above.
* 7_1_MEM_INOUT_MIXED_WITH_IN_OUT-mixed_inout.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_PORT` for HelperMod `addr` port. Fix: same as above.
* 7_1_MEM_INVALID_DEPTH-zero_depth.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_PORT` for HelperMod `addr` port. Fix: same as above.
* 7_1_MEM_INVALID_WORD_WIDTH-zero_width.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_PORT` for HelperMod `addr` port. Fix: same as above.
* 7_1_MEM_MISSING_INIT-missing_init.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_PORT` for HelperMod `addr` port. Fix: same as above.
* 7_1_MEM_PORT_NAME_CONFLICT_MODULE_ID-port_name_conflict.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_PORT` for HelperMod `addr` port. Fix: same as above.
* 7_1_MEM_TYPE_BLOCK_WITH_ASYNC_OUT-block_async_out.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_PORT` for HelperMod `addr` port. Fix: same as above.
* 7_1_MEM_UNDEFINED_CONST_IN_WIDTH-undefined_const.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_PORT` for HelperMod `addr` port. Fix: same as above.
* 7_1_MEM_UNDEFINED_NAME-undefined_mem_name.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNSINKED_REGISTER` for HelperMod register `r` (written but never read because MEM access is undefined). Fix: use `r` in `data` output.

## test_7_2-port_types_and_semantics.md

* 7_2_MEM_READ_FROM_WRITE_PORT-read_from_in_port.jz : test-quality
  Scaffolding: OUT rd port (lines 28, 58) is declared but never read, causing `MEM_WARN_PORT_NEVER_ACCESSED` warnings unrelated to the rule under test. Fix: add a valid `data = mem_a.rd[addr] ^ ...` read to consume the OUT port, eliminating the scaffolding warning.

## test_7_3-memory_access_syntax.md

* MEM_CONST_ADDR_OUT_OF_RANGE : compiler-bug
  Compiler does not check constant address range on INOUT .addr field assignments (only on bracket-indexed access). `mem.rw.addr <= 3'd5;` on depth-4 MEM compiles without error. May be a missing check. File attempted: `7_3_MEM_CONST_ADDR_OUT_OF_RANGE-inout_const_overflow.jz`.
* MEM_PORT_USED_AS_SIGNAL : compiler-bug
  Bare IN (write) port reference fires MEM_READ_FROM_WRITE_PORT (on RHS) or MEM_IN_PORT_FIELD_ACCESS (on LHS) instead of MEM_PORT_USED_AS_SIGNAL. More specific rules take precedence for IN ports. File attempted: `7_3_MEM_PORT_USED_AS_SIGNAL-in_port_bare_ref.jz`.
* MEM_CONST_ADDR_OUT_OF_RANGE : missing-context
  Covered: ASYNC read bracket, SYNC write bracket; missing: INOUT .addr with out-of-range constant. Recommended new file: `7_3_MEM_CONST_ADDR_OUT_OF_RANGE-inout_const_overflow.jz`. Note: sweep confirmed compiler-bug — constant address range not checked on INOUT .addr field.
* MEM_PORT_USED_AS_SIGNAL : missing-context
  Covered: ASYNC OUT bare ref, SYNC OUT bare ref (bare_port_ref.jz), INOUT bare ref (inout_bare_ref.jz); missing: IN (write) port bare ref. Recommended new file: `7_3_MEM_PORT_USED_AS_SIGNAL-in_port_bare_ref.jz`. Note: sweep found rule-not-fired — more specific rules preempt.

## test_7_5-initialization.md

* MEM_INIT_FILE_NOT_FOUND : compiler-bug
  @file(CONST_NAME) where CONST is a string path fires PATH_OUTSIDE_SANDBOX instead of MEM_INIT_FILE_NOT_FOUND. CONST-based @file path resolution appears to resolve to a path outside sandbox roots, unlike literal string paths which correctly fire MEM_INIT_FILE_NOT_FOUND. File attempted: `7_5_MEM_INIT_FILE_NOT_FOUND-const_path.jz`.
* MEM_INIT_FILE_NOT_FOUND : compiler-bug
  @file(CONFIG.NAME) fires UNDECLARED_IDENTIFIER instead of MEM_INIT_FILE_NOT_FOUND. CONFIG-based @file path references are not resolved correctly — the compiler treats CONFIG.BOOT_IMAGE as an undeclared identifier rather than resolving the project CONFIG value. File attempted: `7_5_MEM_INIT_FILE_NOT_FOUND-config_path.jz`.
* MEM_INIT_CONTAINS_X : compiler-bug
  Compiler fires LIT_INVALID_DIGIT_FOR_BASE instead of MEM_INIT_CONTAINS_X for hex literals containing `x` (e.g. `8'hxF`). `x` is not a valid hex digit, so the lexer rejects it before MEM init checking runs. The audit's "x in hex literal" is not a valid context for this rule — binary is the only base where `x` is syntactically legal. File attempted: `7_5_MEM_INIT_CONTAINS_X-x_in_hex_literal.jz`.
* MEM_INIT_CONTAINS_X : missing-context
  Covered: partial x in binary, all-x in binary; missing: x in hex literal (e.g. `8'hxF`). Recommended new file: `7_5_MEM_INIT_CONTAINS_X-x_in_hex_literal.jz`. Note: sweep found rule-not-fired — `x` is not a valid hex digit, lexer intercepts.
* MEM_INIT_FILE_NOT_FOUND : missing-context
  Covered: literal string path to nonexistent file; missing: @file with CONST path to nonexistent file, @file with CONFIG path to nonexistent file. Recommended new files: `7_5_MEM_INIT_FILE_NOT_FOUND-const_path.jz`, `7_5_MEM_INIT_FILE_NOT_FOUND-config_path.jz`. Note: sweep confirmed both are compiler-bugs.
* MEM_INIT_LITERAL_OVERFLOW : missing-happy-path
  Happy path exists (7_5_HAPPY_PATH) but does not cover replication or concatenation init. Recommended: `7_5_HAPPY_PATH-replication_concat_ok.jz`.
* MEM_INIT_FILE_NOT_FOUND : missing-happy-path
  Happy path exists but does not cover @file with CONST or CONFIG paths. Recommended: `7_5_HAPPY_PATH-const_config_file_ok.jz`.
* 7_5_MEM_INIT_FILE_CONTAINS_X-xz_in_file.jz : test-quality
  Scaffolding: .out includes `MEM_WARN_PARTIAL_INIT` warnings alongside `MEM_INIT_FILE_CONTAINS_X` errors because test data files (mem_xz_data.hex, mem_xz_data.mem) have fewer entries than the 4-entry depth. Fix: use test data files with exactly 4 entries, or reduce MEM depth to match file sizes.
* Plan sections 2.2/3 : test-quality
  `CONST_NUMERIC_IN_STRING_CONTEXT` appears in error cases (2.2 #7) and I/O matrix (3 #8) but is absent from 5.1 Rules Tested. Tests exist in other plans (4_3, 6_3). Either add to 5.1 or remove from sections 2/3 for consistency.

## test_7_7-error_checking_and_validation.md

* MEM_WARN_DEAD_CODE_ACCESS : compiler-bug
  INOUT port access inside IF(1'b0) in SYNCHRONOUS block fires WARN_DEAD_CODE_UNREACHABLE but not MEM_WARN_DEAD_CODE_ACCESS. Dead code detection for MEM does not handle INOUT port pseudo-fields (.addr, .wdata). File attempted: `7_7_MEM_WARN_DEAD_CODE_ACCESS-dead_inout_access.jz`.
* MEM_WARN_DEAD_CODE_ACCESS : missing-context
  Covered: dead write in SYNCHRONOUS block (IF(1'b0) guard), dead read in ASYNCHRONOUS block (resolved by sweep); missing: dead INOUT access in SYNCHRONOUS block. Recommended new file: `7_7_MEM_WARN_DEAD_CODE_ACCESS-dead_inout_access.jz`. Note: sweep confirmed compiler-bug — INOUT port pseudo-fields not handled.

## test_7_9-mem_in_module_instantiation.md

* UNDECLARED_IDENTIFIER : compiler-bug
  Rule ID in `plan 5.1` — plan lists `UNDECLARED_IDENTIFIER` but the compiler actually emits `MEM_UNDEFINED_NAME` for child MEM access from parent. The test file `7_9_UNDECLARED_IDENTIFIER-child_mem_access_from_parent.jz` is also misnamed; should reference `MEM_UNDEFINED_NAME`.
* 7_9_UNDECLARED_IDENTIFIER-child_mem_access_from_parent.jz : test-quality
  Filename mismatch: file is named with `UNDECLARED_IDENTIFIER` but the compiler emits `MEM_UNDEFINED_NAME`. Fix: rename file to `7_9_MEM_UNDEFINED_NAME-child_mem_access_from_parent.jz` (and corresponding `.out`).

## test_7_10-const_evaluation_in_mem.md

* CONST_CIRCULAR_DEP : compiler-bug
  Circular dependency in CONFIG block fires CONFIG_INVALID_EXPR_TYPE (S6.3) instead of CONST_CIRCULAR_DEP. CONFIG values are validated through a different path than module CONST blocks, so CONST_CIRCULAR_DEP never triggers in CONFIG context. File attempted: `7_10_CONST_CIRCULAR_DEP-circular_config.jz`.
* CONST_UNDEFINED_IN_WIDTH_OR_SLICE : compiler-bug
  Undefined CONST in MEM word_width/depth fires MEM_UNDEFINED_CONST_IN_WIDTH (S7.1/S7.7.1) instead of CONST_UNDEFINED_IN_WIDTH_OR_SLICE. MEM dimension validation uses a MEM-specific rule that takes precedence. File attempted: `7_10_CONST_UNDEFINED_IN_WIDTH_OR_SLICE-mem_context.jz`.
* CONST_CIRCULAR_DEP : missing-context
  Covered: circular CONSTs in module CONST blocks (helper + top); missing: circular dependency in project CONFIG block. Recommended new file: `7_10_CONST_CIRCULAR_DEP-circular_config.jz`. Note: sweep found rule-not-fired — CONFIG block uses different validation path.
* CONST_UNDEFINED_IN_WIDTH_OR_SLICE : missing-context
  Covered: wire name in slice, register name in slice, port name in slice, undefined const in port width, wire width, register width; missing: undefined CONST in MEM word_width, undefined CONST in MEM depth. Recommended new file: `7_10_CONST_UNDEFINED_IN_WIDTH_OR_SLICE-mem_context.jz`. Note: sweep found rule-not-fired — MEM_UNDEFINED_CONST_IN_WIDTH preempts.
* CONST_CIRCULAR_DEP : missing-happy-path
  No dedicated happy-path file for non-circular CONST chains (the 7_10_HAPPY_PATH file uses independent CONSTs, not chains). Recommended: `4_3_CONST_CIRCULAR_DEP-valid_chain_ok.jz`.
* 7_10_CONST_NEGATIVE_OR_NONINT-negative_const_mem_depth.jz : test-quality
  Misleading test: filename says "negative_const_mem_depth" but neither HelperMod (line 15-37) nor HelperMod2 (line 39-62) contains a MEM block. The negative CONSTs (NEG_DEPTH=-4, NEG_WIDTH=-2) are declared but never used in MEM dimensions. Rule fires at CONST declaration time so the test passes, but it does not exercise the plan's intended scenario (negative CONST used as MEM dimension). Fix: add MEM blocks that reference the negative CONSTs, or rename to reflect actual trigger.
* 7_1_MEM_UNDEFINED_CONST_IN_WIDTH-undefined_const.jz : test-quality
  Scaffolding: `.out` includes `WARN_UNUSED_PORT` at line 17 from HelperMod port `addr` which is declared but never read. Fix: use `addr` in an expression or remove it from HelperMod's PORT block.

## test_8_3-global_semantics.md

* GLOBAL_CONST_USE_UNDECLARED : missing-context
  Covered: SYNCHRONOUS RHS (two modules), ASYNCHRONOUS RHS (resolved by sweep), operator expression (resolved by sweep). Note: this entry is fully resolved.

## test_8_4-global_value_semantics.md

* ASSIGN_WIDTH_NO_MODIFIER : compiler-bug
  Concatenation width mismatch fires ASSIGN_CONCAT_WIDTH_MISMATCH (higher priority per rules.c line 13) instead of ASSIGN_WIDTH_NO_MODIFIER. The intended rule cannot fire in concatenation context. File attempted: `8_4_ASSIGN_WIDTH_NO_MODIFIER-global_in_concat.jz`.
* ASSIGN_WIDTH_NO_MODIFIER : missing-context
  Covered: ASYNC `=` (narrow→wide), SYNC `<=` (wide→narrow, narrow→wide), ASYNC `<=` alias (wide→narrow), both helper and top modules, global in expression (resolved by sweep); missing: global in concatenation. Recommended new file: `8_4_ASSIGN_WIDTH_NO_MODIFIER-global_in_concat.jz`. Note: sweep found rule-not-fired — ASSIGN_CONCAT_WIDTH_MISMATCH preempts.
* ASSIGN_WIDTH_NO_MODIFIER : missing-happy-path
  Happy-path file exists (`8_4_HAPPY_PATH-global_value_semantics_ok.jz`) but only covers direct assignment contexts; missing: global in expression, concatenation, conditional (ternary) with matching widths. Recommended: `8_4_HAPPY_PATH-global_in_expressions_ok.jz`.

## test_8_5-global_errors.md

* 8_5_LIT_OVERFLOW-global_literal_overflow.jz : test-quality
  Naming: filename uses `LIT_OVERFLOW` instead of the rule ID `GLOBAL_INVALID_EXPR_TYPE`, making rule-based pattern matching miss this file. Fix: rename to `8_5_GLOBAL_INVALID_EXPR_TYPE-literal_overflow.jz`.

## test_9_1-check_syntax.md

* CHECK_INVALID_EXPR_TYPE : compiler-bug
  LATCH signal in @check fires LATCH_IN_CONST_CONTEXT (S4.8) instead of CHECK_INVALID_EXPR_TYPE (S9.1). More specific rule handles this case; CHECK_INVALID_EXPR_TYPE is not reachable for LATCH signals. Audit finding may be invalid. File attempted: `9_1_CHECK_INVALID_EXPR_TYPE-latch_signal.jz`.
* CHECK_INVALID_EXPR_TYPE : missing-context
  Covered: undefined identifier (project scope), IN port signal (helper module), IN port signal (top module), register signal, wire signal, MEM port signal (resolved by sweep), INOUT port signal (resolved by sweep); missing: LATCH signal. Recommended new file: `9_1_CHECK_INVALID_EXPR_TYPE-latch_signal.jz`. Note: sweep found rule-not-fired — LATCH_IN_CONST_CONTEXT preempts.

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
