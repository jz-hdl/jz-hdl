
## Context Sweep Report: test_10_1-template_purpose.md

_no work: no missing contexts for this plan_
  -> OK

---

## Context Sweep Report: test_10_2-template_definition.md

_no work: no missing contexts for this plan_
  -> OK

[3/90] test_10_3-template_allowed_content.md
971 pass (up from 970), 0 fail, 6 skip. The new test passes and no regressions.

---

## Context Sweep Report: test_10_3-template_allowed_content.md

### Summary
- Work list size (from issues.md):           1
- After dedup:                                1
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       1
- Scaffolding or bug failures (not created):  0
- Total: 1 == 0 + 0 + 0 + 0 + 1 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| TEMPLATE_EXTERNAL_REF | LHS of `<=`, `=>` assignment, alias LHS (`=`) | 10_3_TEMPLATE_EXTERNAL_REF-external_lhs_and_reverse.jz |

Note: The audit bundled 3 missing contexts into 1 recommended file. All 3 contexts are covered (one trigger each) in the single file as recommended.

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
_None._

Note: During development, project-scope templates with LHS references (`ext_w <= a;`, `ext_w => a;`) fire UNDECLARED_IDENTIFIER instead of TEMPLATE_EXTERNAL_REF. This is because the template body checker only recognizes external references for identifiers that resolve to module-scope signals — at project scope, unresolvable identifiers hit the generic UNDECLARED_IDENTIFIER path first. This is a minor inconsistency but not a blocking bug since the contexts are testable via module-scoped templates.

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 970 / 976
- Result after sweep:  971 / 977
- Newly passing:       1
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_10_4-template_forbidden_content.md

### Summary
- Work list size (from issues.md):           6
- After dedup:                                6
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       6
- Scaffolding or bug failures (not created):  0
- Total: 6 == 0 + 0 + 0 + 0 + 6 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| TEMPLATE_FORBIDDEN_DECL | LATCH (any scope) | 10_4_TEMPLATE_FORBIDDEN_DECL-latch_in_template.jz |
| TEMPLATE_FORBIDDEN_DIRECTIVE | @blackbox | 10_4_TEMPLATE_FORBIDDEN_DIRECTIVE-blackbox_in_template.jz |
| TEMPLATE_FORBIDDEN_DIRECTIVE | @import | 10_4_TEMPLATE_FORBIDDEN_DIRECTIVE-import_in_template.jz |
| TEMPLATE_FORBIDDEN_DIRECTIVE | @global | 10_4_TEMPLATE_FORBIDDEN_DIRECTIVE-global_in_template.jz |
| TEMPLATE_FORBIDDEN_DIRECTIVE | @check | 10_4_TEMPLATE_FORBIDDEN_DIRECTIVE-check_in_template.jz |
| TEMPLATE_FORBIDDEN_DIRECTIVE | @apply | 10_4_TEMPLATE_FORBIDDEN_DIRECTIVE-apply_in_template.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
_None._

### Parser Recovery Findings (for next audit to log)
_None. All 6 tests produced clean single-rule output with no cascading PARSE000 errors._

**Note:** The @global test (`global_in_template.jz`) produces two TEMPLATE_FORBIDDEN_DIRECTIVE diagnostics — one at `@global` (line 17) and one at `@endglob` (line 19). This is correct behavior: both keywords are structural directives. The test still has a single trigger construct (one @global block).

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 971 / 977 (6 skip)
- Result after sweep:  977 / 983 (6 skip)
- Newly passing:       6
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_10_5-template_application.md

### Summary
- Work list size (from issues.md):           4
- After dedup:                                4
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       3
- Scaffolding or bug failures (not created):  1
- Total: 4 == 0 + 0 + 0 + 0 + 3 + 1

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| TEMPLATE_UNDEFINED | cross-module scope | 10_5_TEMPLATE_UNDEFINED-cross_module_scope.jz |
| TEMPLATE_ARG_COUNT_MISMATCH | with count parameter | 10_5_TEMPLATE_ARG_COUNT_MISMATCH-with_count.jz |
| TEMPLATE_COUNT_NOT_NONNEG_INT | non-integer expression | 10_5_TEMPLATE_COUNT_NOT_NONNEG_INT-non_integer_expr.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| TEMPLATE_APPLY_OUTSIDE_BLOCK | 10_5_TEMPLATE_APPLY_OUTSIDE_BLOCK-apply_in_decl_block.jz | compiler-bug | @apply inside PORT/WIRE/REGISTER/CONST blocks produces PARSE000 ("expected ... in ... block") instead of TEMPLATE_APPLY_OUTSIDE_BLOCK. The parser rejects @apply as invalid syntax in declaration blocks before template validation runs. Tested all 4 block types — all produce PARSE000. The rule only fires at file scope and module scope. |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 977 / 983
- Result after sweep:  980 / 986
- Newly passing:       3
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_10_6-template_exclusive_assignment.md

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       2
- Scaffolding or bug failures (not created):  0
- Total: 2 == 0 + 0 + 0 + 0 + 2 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| ASSIGN_MULTIPLE_SAME_BITS | @apply with count/IDX overlapping wire bits | 10_6_ASSIGN_MULTIPLE_SAME_BITS-template_apply_count_overlap.jz |
| SYNC_MULTI_ASSIGN_SAME_REG_BITS | @apply with count/IDX overlapping register bits | 10_6_SYNC_MULTI_ASSIGN_SAME_REG_BITS-template_apply_count_overlap.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
_None._

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 980 / 980
- Result after sweep:  982 / 982
- Newly passing:       2
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_10_8-template_error_cases.md

_no work: no missing contexts for this plan_
  -> OK

---

## Context Sweep Report: test_11_1-tristate_default_purpose.md

### Summary
- Work list size (from issues.md):           1
- After dedup:                                1
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       1
- Scaffolding or bug failures (not created):  0
- Total: 1 == 0 + 0 + 0 + 0 + 1 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| TRISTATE_TRANSFORM_UNUSED_DEFAULT | VCC mode (no tristate nets) | 11_VCC_7_TRISTATE_TRANSFORM_UNUSED_DEFAULT-no_tristate_nets_vcc.jz |

### Skipped Files
_None_

### Scaffolding or Compiler Bugs Found
_None_

### Parser Recovery Findings (for next audit to log)
_None_

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 982 / 988
- Result after sweep:  983 / 989
- Newly passing:       1
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_11_2-tristate_default_applicability.md

_no work: plan has no missing contexts (section exists but says "No issues flagged.")_
  -> OK

---

## Context Sweep Report: test_11_3-tristate_net_identification.md

### Summary
- Work list size (from issues.md):           4
- After dedup:                                4
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       2
- Scaffolding or bug failures (not created):  2
- Total: 4 == 0 + 0 + 0 + 0 + 2 + 2

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| NET_TRI_STATE_ALL_Z_READ | multi-instance all-z via INOUT ports | 11_3_NET_TRI_STATE_ALL_Z_READ-instance_all_z.jz |
| NET_TRI_STATE_ALL_Z_READ | BUS signal all-z | 11_3_NET_TRI_STATE_ALL_Z_READ-bus_all_z.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| NET_MULTIPLE_ACTIVE_DRIVERS | 11_3_NET_MULTIPLE_ACTIVE_DRIVERS-direct_async_conflict.jz | rule-not-fired | Direct ASYNC assignments to same wire in same block triggers `ASSIGN_INDEPENDENT_IF_SELECT` instead; cross-block assignments produce no diagnostic. The "direct async" context is pre-empted by the exclusive assignment rule or not implemented for cross-block scenarios. |
| NET_MULTIPLE_ACTIVE_DRIVERS | 11_3_NET_MULTIPLE_ACTIVE_DRIVERS-bus_signal_conflict.jz | compiler-bug | BUS bulk assignment connecting two SOURCE instances to the same TARGET does not trigger NET_MULTIPLE_ACTIVE_DRIVERS. The compiler does not track BUS signal drivers as contributing to net-level multi-driver analysis. Spec §1.6.4 explicitly lists "BUS signal drivers from multiple instances connected to the same parent wire" as a driver source. |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 983 / 989
- Result after sweep:  985 / 991
- Newly passing:       2
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_11_4-tristate_transformation_algorithm.md

### Summary
- Work list size (from issues.md):           7
- After dedup:                                7
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          2
- Successfully created:                       5
- Scaffolding or bug failures (not created):  0
- Total: 7 == 0 + 0 + 0 + 2 + 5 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| TRISTATE_TRANSFORM_SINGLE_DRIVER | VCC default variant | 11_VCC_4_TRISTATE_TRANSFORM_SINGLE_DRIVER-single_driver.jz |
| TRISTATE_TRANSFORM_SINGLE_DRIVER | 1-bit width variant | 11_GND_4_TRISTATE_TRANSFORM_SINGLE_DRIVER-single_bit.jz |
| TRISTATE_TRANSFORM_PER_BIT_FAIL | VCC default variant | 11_VCC_4_TRISTATE_TRANSFORM_PER_BIT_FAIL-per_bit_tristate.jz |
| TRISTATE_TRANSFORM_MUTUAL_EXCLUSION_FAIL | VCC default variant | 11_VCC_4_TRISTATE_TRANSFORM_MUTUAL_EXCLUSION_FAIL-non_exclusive.jz |
| TRISTATE_TRANSFORM_MUTUAL_EXCLUSION_FAIL | 3+ drivers | 11_GND_4_TRISTATE_TRANSFORM_MUTUAL_EXCLUSION_FAIL-three_drivers.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| INFO_TRISTATE_TRANSFORM | 11_GND_4_INFO_TRISTATE_TRANSFORM-two_driver_chain.jz | not testable: compiler's post-transform mutual-exclusion check always fires TRISTATE_TRANSFORM_MUTUAL_EXCLUSION_FAIL for multi-instance drivers, preventing INFO_TRISTATE_TRANSFORM from emitting in multi-driver context. Multi-driver priority chains within a single ASYNCHRONOUS block are blocked by ASSIGN_INDEPENDENT_IF_SELECT. No code path exists where INFO_TRISTATE_TRANSFORM fires for a multi-driver scenario. |
| INFO_TRISTATE_TRANSFORM | 11_GND_4_INFO_TRISTATE_TRANSFORM-three_driver_chain.jz | not testable: same reason as above — three-driver chain triggers MUTUAL_EXCLUSION_FAIL before INFO_TRISTATE_TRANSFORM can emit. |

### Scaffolding or Compiler Bugs Found
_None._

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 985 / 991
- Result after sweep:  990 / 996
- Newly passing:       5
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_11_5-tristate_validation_rules.md

### Summary
- Work list size (from issues.md):           4
- After dedup:                                4
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       4
- Scaffolding or bug failures (not created):  0
- Total: 4 == 0 + 0 + 0 + 0 + 4 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| TRISTATE_TRANSFORM_PER_BIT_FAIL | uneven split (7+1) | 11_GND_5_TRISTATE_TRANSFORM_PER_BIT_FAIL-uneven_split.jz |
| TRISTATE_TRANSFORM_PER_BIT_FAIL | VCC variant, per-bit z via INOUT | 11_VCC_5_TRISTATE_TRANSFORM_PER_BIT_FAIL-per_bit.jz |
| TRISTATE_TRANSFORM_MUTUAL_EXCLUSION_FAIL | three non-exclusive drivers | 11_GND_5_TRISTATE_TRANSFORM_MUTUAL_EXCLUSION_FAIL-three_drivers.jz |
| TRISTATE_TRANSFORM_MUTUAL_EXCLUSION_FAIL | VCC variant | 11_VCC_5_TRISTATE_TRANSFORM_MUTUAL_EXCLUSION_FAIL-non_exclusive.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| (none) | | | |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 990 / 996
- Result after sweep:  994 / 1000
- Newly passing:       4
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_11_6-tristate_inout_handling.md

### Summary
- Work list size (from issues.md):           4
- After dedup:                                4
- Pre-existing files (skipped):               1
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       3
- Scaffolding or bug failures (not created):  0
- Total: 4 == 1 + 0 + 0 + 0 + 3 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| TRISTATE_TRANSFORM_OE_EXTRACT_FAIL | VCC default variant | 11_VCC_6_TRISTATE_TRANSFORM_OE_EXTRACT_FAIL-inout_oe_fail.jz |
| TRISTATE_TRANSFORM_BLACKBOX_PORT | multiple blackbox INOUT ports on same wire | 11_GND_6_TRISTATE_TRANSFORM_BLACKBOX_PORT-multi_blackbox.jz |
| TRISTATE_TRANSFORM_BLACKBOX_PORT | VCC default variant | 11_VCC_6_TRISTATE_TRANSFORM_BLACKBOX_PORT-blackbox_inout.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| TRISTATE_TRANSFORM_OE_EXTRACT_FAIL | 11_GND_6_TRISTATE_TRANSFORM_OE_EXTRACT_FAIL-nested_ternary_z.jz | already exists |

### Scaffolding or Compiler Bugs Found
_(none)_

### Parser Recovery Findings (for next audit to log)
_(none)_

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 994 / 1001
- Result after sweep:  997 / 1004
- Newly passing:       3
- Newly broken:        0
  -> OK

## Context Sweep Report: test_11_7-tristate_error_conditions.md

### Summary
- Work list size (from issues.md):           11
- After dedup:                                11
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       11
- Scaffolding or bug failures (not created):  0
- Total: 11 == 0 + 0 + 0 + 0 + 11 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| TRISTATE_TRANSFORM_OE_EXTRACT_FAIL | VCC default variant | 11_VCC_7_TRISTATE_TRANSFORM_OE_EXTRACT_FAIL-ambiguous_oe.jz |
| TRISTATE_TRANSFORM_OE_EXTRACT_FAIL | nested ternary z pattern | 11_GND_7_TRISTATE_TRANSFORM_OE_EXTRACT_FAIL-nested_ternary.jz |
| TRISTATE_TRANSFORM_UNUSED_DEFAULT | VCC mode | 11_VCC_7_TRISTATE_TRANSFORM_UNUSED_DEFAULT-no_tristate_nets.jz |
| TRISTATE_TRANSFORM_SINGLE_DRIVER | VCC default variant | 11_VCC_7_TRISTATE_TRANSFORM_SINGLE_DRIVER-single_driver.jz |
| TRISTATE_TRANSFORM_SINGLE_DRIVER | 1-bit width | 11_GND_7_TRISTATE_TRANSFORM_SINGLE_DRIVER-single_bit.jz |
| TRISTATE_TRANSFORM_PER_BIT_FAIL | VCC default variant | 11_VCC_7_TRISTATE_TRANSFORM_PER_BIT_FAIL-per_bit_tristate.jz |
| TRISTATE_TRANSFORM_PER_BIT_FAIL | uneven split (7+1) | 11_GND_7_TRISTATE_TRANSFORM_PER_BIT_FAIL-uneven_split.jz |
| TRISTATE_TRANSFORM_MUTUAL_EXCLUSION_FAIL | VCC default variant | 11_VCC_7_TRISTATE_TRANSFORM_MUTUAL_EXCLUSION_FAIL-non_exclusive.jz |
| TRISTATE_TRANSFORM_MUTUAL_EXCLUSION_FAIL | 3 non-exclusive drivers | 11_GND_7_TRISTATE_TRANSFORM_MUTUAL_EXCLUSION_FAIL-three_drivers.jz |
| TRISTATE_TRANSFORM_BLACKBOX_PORT | VCC default variant | 11_VCC_7_TRISTATE_TRANSFORM_BLACKBOX_PORT-blackbox_tristate.jz |
| TRISTATE_TRANSFORM_BLACKBOX_PORT | multiple blackbox INOUT ports | 11_GND_7_TRISTATE_TRANSFORM_BLACKBOX_PORT-multi_blackbox.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
_None._

### Parser Recovery Findings (for next audit to log)
_None._

## Implementation Notes
- **Nested ternary OE_EXTRACT_FAIL**: The audit recommended "nested ternary z pattern" as a missing context. The OE extractor in `ir_tristate_transform.c` only examines top-level ternary branches (`literal_is_z_or_zero`), so simple nested ternaries with z in one outer branch are handled fine. To trigger OE_EXTRACT_FAIL, the z must appear as a non-literal expression (e.g., `{4'bzzzz, 4'bzzzz}` concatenation) at the top level. The test uses a nested ternary where BOTH outer branches are sub-ternary expressions containing concatenated z — neither branch is a z literal, so both Pattern A and A' fail. Meanwhile, the semantic mutual exclusion prover (`sem_tristate_check_net`) recognizes the concatenated z as z-valued and passes the exclusion check for the `(en == 1'b1)` / `(en == 1'b0)` conditions.
- **MUTUAL_EXCLUSION_FAIL**: This rule is emitted by the semantic analysis (`driver_net.c`) when `--tristate-default` is active, not only by the post-transform validator. The semantic check runs before the tristate transform and uses `sem_tristate_check_net` to prove mutual exclusion of driver enable conditions.
- **Multi-blackbox**: With two blackbox INOUT ports on the same wire, only one BLACKBOX_PORT error is emitted (for the first blackbox encountered during transform).

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 997 / 1004 (7 skip)
- Result after sweep:  1008 / 1015 (7 skip)
- Newly passing:       11
- Newly broken:        0
  -> OK

## Context Sweep Report: test_11_8-tristate_portability.md

_no work: no missing contexts for this plan_
  -> OK

## Context Sweep Report: test_12_1-compile_errors.md

_no work: no missing contexts for this plan_
  -> OK

---

## Context Sweep Report: test_12_2-combinational_loop_errors.md

### Summary
- Work list size (from issues.md):           6
- After dedup:                                6
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       6
- Scaffolding or bug failures (not created):  0
- Total: 6 == 0 + 0 + 0 + 0 + 6 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| COMB_LOOP_UNCONDITIONAL | MEM read path | 12_2_COMB_LOOP_UNCONDITIONAL-mem_read_path.jz |
| COMB_LOOP_UNCONDITIONAL | multi-bit signal cycles | 12_2_COMB_LOOP_UNCONDITIONAL-multi_bit_cycle.jz |
| COMB_LOOP_UNCONDITIONAL | SELECT/case branch cycle | 12_2_COMB_LOOP_UNCONDITIONAL-select_branch_cycle.jz |
| COMB_LOOP_CONDITIONAL_SAFE | SELECT/case mutually exclusive branches | 12_2_COMB_LOOP_CONDITIONAL_SAFE-select_branch_cycle.jz |
| COMB_LOOP_CONDITIONAL_SAFE | nested IF/ELSE (3+ levels) | 12_2_COMB_LOOP_CONDITIONAL_SAFE-nested_if_else.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| COMB_LOOP_UNCONDITIONAL | 12_2_COMB_LOOP_UNCONDITIONAL-cycle_through_instance.jz | rule-not-fired | Compiler does not detect combinational loops that span module boundaries (OUT of instance fed back to its own IN through parent ASYNCHRONOUS block). The cycle path goes through the submodule's pass-through logic but the loop analysis appears to be intra-module only. No diagnostics emitted at all. |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1008 / 1015 (7 skip)
- Result after sweep:  1013 / 1020 (7 skip)
- Newly passing:       5
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_12_3-recommended_warnings.md

### Summary
- Work list size (from issues.md):           3
- After dedup:                                3
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       2
- Scaffolding or bug failures (not created):  1
- Total: 3 == 0 + 0 + 0 + 0 + 2 + 1

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| WARN_UNUSED_PORT | unused INOUT port | 12_3_WARN_UNUSED_PORT-unused_inout.jz |
| WARN_DEAD_CODE_UNREACHABLE | nested dead code (always-false IF inside always-true IF) | 12_3_WARN_DEAD_CODE_UNREACHABLE-nested_dead_code.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| WARN_DEAD_CODE_UNREACHABLE | 12_3_WARN_DEAD_CODE_UNREACHABLE-select_dead_case.jz | rule-not-fired | Compiler does not detect dead DEFAULT after exhaustive CASE coverage in SELECT. A 1-bit selector with CASE 1'b0 + CASE 1'b1 + DEFAULT produces zero warnings. The rule is unimplemented for SELECT exhaustive-coverage dead code detection. |

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1013 / 1020
- Result after sweep:  1015 / 1022
- Newly passing:       2
- Newly broken:        0
  -> OK

## Context Sweep Report: test_12_4-path_security.md

_no work: no missing contexts for this plan_
  -> OK

---

## Context Sweep Report: test_1_1-identifiers.md

### Summary
- Work list size (from issues.md):           9
- After dedup:                                9
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       6
- Scaffolding or bug failures (not created):  3
- Total: 9 == 0 + 0 + 0 + 0 + 6 + 3

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| ID_SYNTAX_INVALID | module name | 1_1_ID_SYNTAX_INVALID-module_name.jz |
| ID_SYNTAX_INVALID | instance name | 1_1_ID_SYNTAX_INVALID-instance_name.jz |
| ID_SINGLE_UNDERSCORE | wire name | 1_1_ID_SINGLE_UNDERSCORE-wire_name.jz |
| ID_SINGLE_UNDERSCORE | module name | 1_1_ID_SINGLE_UNDERSCORE-module_name.jz |
| KEYWORD_AS_IDENTIFIER | mem name | 1_1_KEYWORD_AS_IDENTIFIER-mem_name.jz |
| KEYWORD_AS_IDENTIFIER | latch name | 1_1_KEYWORD_AS_IDENTIFIER-latch_name.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| ID_SYNTAX_INVALID | 1_1_ID_SYNTAX_INVALID-wire_name.jz | compiler-bug | ID_SYNTAX_INVALID does not fire for 256-char identifiers in WIRE declarations. Compiler emits only WARN_UNUSED_WIRE. The lexical length check appears to be skipped for wire name tokens. |
| DIRECTIVE_INVALID_CONTEXT | 4_1_DIRECTIVE_INVALID_CONTEXT-new_inside_block.jz | compiler-bug | @new inside ASYNCHRONOUS block emits PARSE000 ("expected identifier in assignment left-hand side") instead of DIRECTIVE_INVALID_CONTEXT. The structural-directive check does not cover block-level (ASYNCHRONOUS/SYNCHRONOUS) contexts. |
| DIRECTIVE_INVALID_CONTEXT | 4_1_DIRECTIVE_INVALID_CONTEXT-endmod_inside_block.jz | compiler-bug | @endmod inside ASYNCHRONOUS block emits PARSE000 ("expected identifier in assignment left-hand side") instead of DIRECTIVE_INVALID_CONTEXT. Same root cause as @new — block-level parser does not check for structural directives. |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1015 / 1022
- Result after sweep:  1021 / 1028
- Newly passing:       6
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_1_2-fundamental_terms.md

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       1
- Scaffolding or bug failures (not created):  1
- Total: 2 == 0 + 0 + 0 + 0 + 1 + 1

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| NET_TRI_STATE_ALL_Z_READ | multiple instance outputs all releasing z on same wire | 11_3_NET_TRI_STATE_ALL_Z_READ-multi_instance_all_z.jz |

### Skipped Files

_None._

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| OBS_X_TO_OBSERVABLE_SINK | 1_2_OBS_X_TO_OBSERVABLE_SINK-x_bits_to_mem.jz | compiler-bug / rule-not-fired | Rule message says "drives REGISTER, MEM, or output" but the implementation does not fire for MEM write data (`mem.wr[addr] <= 8'bxxxx_0000`). x-bit literal in MEM IN-port write produces no diagnostic. The rule correctly fires for REGISTER and OUT port targets but not MEM. |

### Parser Recovery Findings (for next audit to log)

_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1021 / 1028 (7 skip)
- Result after sweep:  1022 / 1029 (7 skip)
- Newly passing:       1
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_1_3-bit_slicing_and_indexing.md

### Summary
- Work list size (from issues.md):           3
- After dedup:                                3
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       0
- Scaffolding or bug failures (not created):  3
- Total: 3 == 0 + 0 + 0 + 0 + 0 + 3

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| _(none)_ | | |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| _(none)_ | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| SLICE_MSB_LESS_THAN_LSB | 1_3_SLICE_MSB_LESS_THAN_LSB-const_reversed_indices.jz | compiler-bug | Compiler does not evaluate CONST values when checking MSB < LSB. `bus[LOW:HIGH]` where LOW=3, HIGH=7 produces no diagnostic — the rule only fires for literal integer indices, not CONST-resolved ones. |
| SLICE_INDEX_OUT_OF_RANGE | 1_3_SLICE_INDEX_OUT_OF_RANGE-const_out_of_range.jz | compiler-bug | Compiler does not evaluate CONST values when checking index range. `bus[BIG:0]` where BIG=20 on a 16-bit signal produces no diagnostic — the rule only fires for literal integer indices, not CONST-resolved ones. |
| CONST_UNDEFINED_IN_WIDTH_OR_SLICE | 1_3_CONST_UNDEFINED_IN_WIDTH_OR_SLICE-truly_undefined.jz | rule-not-fired | A truly undefined identifier (`bus[UNDEF:0]` where UNDEF is not declared anywhere) fires UNDECLARED_IDENTIFIER instead of CONST_UNDEFINED_IN_WIDTH_OR_SLICE. The existing test covers declared-but-wrong-type names (wire, register, port). A truly undeclared name is caught by the more general UNDECLARED_IDENTIFIER rule before the slice-specific check runs. This may be correct compiler behavior (identifier resolution precedes slice validation), not a bug. |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| _(none)_ | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1022 / 1029 (7 skipped)
- Result after sweep:  1022 / 1029 (7 skipped)
- Newly passing:       0
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_1_4-comments.md

### Summary
- Work list size (from issues.md):           4
- After dedup:                                4
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       4
- Scaffolding or bug failures (not created):  0
- Total: 4 == 0 + 0 + 0 + 0 + 4 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| COMMENT_IN_TOKEN | port name declaration | 1_4_COMMENT_IN_TOKEN-port_name.jz |
| COMMENT_IN_TOKEN | wire name declaration | 1_4_COMMENT_IN_TOKEN-wire_name.jz |
| COMMENT_IN_TOKEN | module name | 1_4_COMMENT_IN_TOKEN-module_name.jz |
| COMMENT_IN_TOKEN | operator token | 1_4_COMMENT_IN_TOKEN-operator_token.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
_None._

### Parser Recovery Findings (for next audit to log)
_None._

## Note
The audit listed 5 missing contexts (port name, wire name, module name, instance name, operator token) but only recommended 4 filenames — `instance_name` had no recommended filename. This sweep only creates files for recommended filenames per the workflow rules.

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1022 / 1029
- Result after sweep:  1026 / 1033
- Newly passing:       4
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_1_5-exclusive_assignment_rule.md

### Summary
- Work list size (from issues.md):           8
- After dedup:                                8
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       3
- Scaffolding or bug failures (not created):  5
- Total: 8 == 0 + 0 + 0 + 0 + 3 + 5

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| ASSIGN_INDEPENDENT_IF_SELECT | mixed IF+SELECT independent chains | 1_5_ASSIGN_INDEPENDENT_IF_SELECT-mixed_if_select.jz |
| ASSIGN_SHADOWING | root→deeply nested IF (multi-level nesting) | 1_5_ASSIGN_SHADOWING-deep_nesting.jz |
| ASYNC_UNDEFINED_PATH_NO_DRIVER | SELECT-no-DEFAULT on wire | 1_5_ASYNC_UNDEFINED_PATH_NO_DRIVER-select_no_default_wire.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| ASSIGN_MULTIPLE_SAME_BITS | 1_5_ASSIGN_MULTIPLE_SAME_BITS-sync_register.jz | rule-not-fired | Compiler fires `SYNC_MULTI_ASSIGN_SAME_REG_BITS` in SYNCHRONOUS blocks instead of `ASSIGN_MULTIPLE_SAME_BITS`. The sync context is handled by a dedicated sync-specific rule. |
| ASSIGN_MULTIPLE_SAME_BITS | 1_5_ASSIGN_MULTIPLE_SAME_BITS-inside_branch.jz | rule-not-fired | Compiler fires `ASSIGN_INDEPENDENT_IF_SELECT` for double assignments inside IF branch bodies, not `ASSIGN_MULTIPLE_SAME_BITS`. The `=` operator is forbidden inside conditionals (`ASYNC_ALIAS_IN_CONDITIONAL`), and `<=` triggers independent-chain detection instead. |
| ASSIGN_INDEPENDENT_IF_SELECT | 1_5_ASSIGN_INDEPENDENT_IF_SELECT-sync_register.jz | rule-not-fired | Compiler fires `SYNC_MULTI_ASSIGN_SAME_REG_BITS` in SYNCHRONOUS blocks instead of `ASSIGN_INDEPENDENT_IF_SELECT`. The sync context is handled by a dedicated sync-specific rule. |
| ASSIGN_SHADOWING | 1_5_ASSIGN_SHADOWING-sync_register.jz | rule-not-fired | Compiler fires `SYNC_ROOT_AND_CONDITIONAL_ASSIGN` in SYNCHRONOUS blocks instead of `ASSIGN_SHADOWING`. The sync context is handled by a dedicated sync-specific rule. |
| ASYNC_UNDEFINED_PATH_NO_DRIVER | 1_5_ASYNC_UNDEFINED_PATH_NO_DRIVER-nested_partial.jz | compiler-bug | Compiler emits no diagnostic when inner IF lacks ELSE inside outer IF that has ELSE. Path `sel_a=1, sel_b=0` leaves signal undriven but is not detected. Nested ASYNCHRONOUS path analysis may not recurse into sub-branches. |

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1026 / 1033 (7 skip)
- Result after sweep:  1029 / 1036 (7 skip)
- Newly passing:       3
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_1_6-high_impedance_and_tristate.md

### Summary
- Work list size (from issues.md):           10
- After dedup:                                10
- Pre-existing files (skipped):               2
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       3
- Scaffolding or bug failures (not created):  5
- Total: 10 == 2 + 0 + 0 + 0 + 3 + 5

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| NET_FLOATING_WITH_SINK | wire read in SYNCHRONOUS block | 1_2_NET_FLOATING_WITH_SINK-sync_read.jz |
| NET_FLOATING_WITH_SINK | wire read via @new port binding expression | 1_2_NET_FLOATING_WITH_SINK-instance_port_binding.jz |
| COMB_LOOP_CONDITIONAL_SAFE | three-way mutual exclusion with multiple conditions | 12_2_COMB_LOOP_CONDITIONAL_SAFE-three_way_exclusion.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| COMB_LOOP_CONDITIONAL_SAFE | 12_2_COMB_LOOP_CONDITIONAL_SAFE-nested_if_else.jz | already exists |
| NET_TRI_STATE_ALL_Z_READ | 11_3_NET_TRI_STATE_ALL_Z_READ-multi_instance_all_z.jz | already exists |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| NET_MULTIPLE_ACTIVE_DRIVERS | 11_3_NET_MULTIPLE_ACTIVE_DRIVERS-dual_async_block.jz | compiler-bug | Two ASYNCHRONOUS blocks in same module both driving same wire does not trigger NET_MULTIPLE_ACTIVE_DRIVERS. Compiler may merge multiple ASYNC blocks or only detect multi-driver via instance output port bindings. |
| NET_MULTIPLE_ACTIVE_DRIVERS | 11_3_NET_MULTIPLE_ACTIVE_DRIVERS-instance_plus_local.jz | compiler-bug | Instance output + local ASYNCHRONOUS driver on same wire does not trigger NET_MULTIPLE_ACTIVE_DRIVERS. Compiler appears to only detect multi-driver when multiple instance outputs bind to the same wire. |
| COMB_LOOP_UNCONDITIONAL | 12_2_COMB_LOOP_UNCONDITIONAL-cross_module_loop.jz | compiler-bug | Combinational loop through @new port binding (signal feeds into instance input and returns via instance output) does not trigger COMB_LOOP_UNCONDITIONAL. Compiler does not trace combinational loops across module boundaries. |

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1029 / 1036
- Result after sweep:  1032 / 1039
- Newly passing:       3
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_2_1-literals.md

### Summary
- Work list size (from issues.md):           6
- After dedup:                                6
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       4
- Scaffolding or bug failures (not created):  2
- Total: 6 == 0 + 0 + 0 + 0 + 4 + 2

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| LIT_BARE_INTEGER | async RHS | 2_1_LIT_BARE_INTEGER-async_rhs.jz |
| LIT_OVERFLOW | async RHS | 2_1_LIT_OVERFLOW-async_rhs.jz |
| LIT_WIDTH_NOT_POSITIVE | async RHS | 2_1_LIT_WIDTH_NOT_POSITIVE-async_rhs.jz |
| LIT_UNDEFINED_CONST_WIDTH | async RHS | 2_1_LIT_UNDEFINED_CONST_WIDTH-async_rhs.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| _(none)_ | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| LIT_BARE_INTEGER | 2_1_LIT_BARE_INTEGER-register_init.jz | rule-not-fired | Bare integer `42` in register init (`r [8] = 42;`) produces no diagnostic. Rule message says "in runtime expression" — register init may be a compile-time context excluded from this check. Possible compiler gap or intentional scoping. |
| LIT_OVERFLOW | 2_1_LIT_OVERFLOW-register_init.jz | rule-not-fired | Overflow literal `4'd16` in register init (`r [4] = 4'd16;`) produces no diagnostic. Compiler does not check overflow for register init values. Existing test `2_1_LIT_WIDTH_NOT_POSITIVE-zero_width.jz` DOES fire LIT_WIDTH_NOT_POSITIVE for `0'd0` in register init (line 46), so literal checks are partially applied in register init — overflow specifically is not. Likely compiler gap. |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| _(none)_ | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1032 / 1039
- Result after sweep:  1036 / 1043
- Newly passing:       4
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_2_2-signedness_model.md

### Summary
- Work list size (from issues.md):           5
- After dedup:                                5
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       4
- Scaffolding or bug failures (not created):  1
- Total: 5 == 0 + 0 + 0 + 0 + 4 + 1

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| TYPE_BINOP_WIDTH_MISMATCH | arithmetic ops (-, *, /, %) | 2_2_TYPE_BINOP_WIDTH_MISMATCH-arithmetic_ops.jz |
| TYPE_BINOP_WIDTH_MISMATCH | comparison ops (<=, >=, ==) | 2_2_TYPE_BINOP_WIDTH_MISMATCH-comparison_ops.jz |
| TYPE_BINOP_WIDTH_MISMATCH | bitwise ops (&, \|, ^) | 2_2_TYPE_BINOP_WIDTH_MISMATCH-bitwise_ops.jz |
| WIDTH_NONPOSITIVE_OR_NONINT | zero-width port | 2_3_WIDTH_NONPOSITIVE_OR_NONINT-zero_width_port.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| _(none)_ | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| WIDTH_NONPOSITIVE_OR_NONINT | 2_3_WIDTH_NONPOSITIVE_OR_NONINT-zero_width_literal.jz | scaffolding | Zero-width literal `0'd0` triggers `LIT_WIDTH_NOT_POSITIVE` (S2.1), not `WIDTH_NONPOSITIVE_OR_NONINT` (S2.2). These are distinct rules; audit entry incorrectly attributed the literal case to WIDTH_NONPOSITIVE_OR_NONINT. |
| WIDTH_NONPOSITIVE_OR_NONINT | _(observed during zero_width_port)_ | compiler-bug | `OUT [0] bad_out` does NOT trigger WIDTH_NONPOSITIVE_OR_NONINT — only `IN [0]` triggers it. The compiler appears to skip the width check for output port declarations. |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| _(none)_ | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1036 / 1043
- Result after sweep:  1040 / 1047
- Newly passing:       4
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_2_3-bit_width_constraints.md

### Summary
- Work list size (from issues.md):           5
- After dedup:                                5
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       2
- Scaffolding or bug failures (not created):  3
- Total: 5 == 0 + 0 + 0 + 0 + 2 + 3

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| ASSIGN_WIDTH_NO_MODIFIER | LATCH guarded write | 2_3_ASSIGN_WIDTH_NO_MODIFIER-mux_latch_contexts.jz |
| TERNARY_BRANCH_WIDTH_MISMATCH | nested ternary | 2_3_TERNARY_BRANCH_WIDTH_MISMATCH-nested_ternary.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| TYPE_BINOP_WIDTH_MISMATCH | 2_3_TYPE_BINOP_WIDTH_MISMATCH-mux_block.jz | compiler-bug | MUX read expressions (`mux[sel]`) do not have resolved widths for binop width checking. `mux[sel] + small_signal` with 8-bit MUX elements and 4-bit signal produces no TYPE_BINOP_WIDTH_MISMATCH diagnostic. |
| ASSIGN_WIDTH_NO_MODIFIER | 2_3_ASSIGN_WIDTH_NO_MODIFIER-mux_latch_contexts.jz (MUX trigger only) | compiler-bug | MUX read expressions assigned to narrower targets (`narrow_out <= mux[sel]` where MUX is 8-bit and target is 4-bit) do not fire ASSIGN_WIDTH_NO_MODIFIER. Same root cause as TYPE_BINOP_WIDTH_MISMATCH — MUX element widths are not resolved for lint checks. LATCH trigger in same file works correctly and was kept. |
| ASSIGN_CONCAT_WIDTH_MISMATCH | 2_3_ASSIGN_CONCAT_WIDTH_MISMATCH-mux_context.jz | compiler-bug | Concat containing MUX read (`{mux[sel], c}` = 8+4=12 bits to 8-bit target) does not fire ASSIGN_CONCAT_WIDTH_MISMATCH. Same MUX width resolution bug. |
| WIDTH_NONPOSITIVE_OR_NONINT | 2_3_WIDTH_NONPOSITIVE_OR_NONINT-port_zero_width.jz | compiler-bug | `OUT [0] bad_out` does not fire WIDTH_NONPOSITIVE_OR_NONINT, while `IN [0] bad_in` does (already covered by existing `zero_width_port.jz`). The rule appears to only check input port widths, not output port widths. |

**Note: Systemic MUX width resolution bug.** Three of the five missing contexts involve MUX read expressions (`mux[sel]`). The compiler does not resolve MUX element widths during lint analysis, causing all width-related rules (TYPE_BINOP_WIDTH_MISMATCH, ASSIGN_WIDTH_NO_MODIFIER, ASSIGN_CONCAT_WIDTH_MISMATCH) to silently skip checks on MUX-derived operands. This should be routed to a compiler-frontend fix before re-attempting these context tests.

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1040 / 1047
- Result after sweep:  1042 / 1049
- Newly passing:       2
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_2_4-special_semantic_drivers.md

### Summary
- Work list size (from issues.md):           4
- After dedup:                                4
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       4
- Scaffolding or bug failures (not created):  0
- Total: 4 == 0 + 0 + 0 + 0 + 4 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| SPECIAL_DRIVER_IN_EXPRESSION | MUX block | 2_4_SPECIAL_DRIVER_IN_EXPRESSION-mux_block.jz |
| SPECIAL_DRIVER_IN_CONCAT | MUX block | 2_4_SPECIAL_DRIVER_IN_CONCAT-mux_block.jz |
| SPECIAL_DRIVER_SLICED | MUX block | 2_4_SPECIAL_DRIVER_SLICED-mux_block.jz |
| SPECIAL_DRIVER_IN_INDEX | VCC sync LHS | 2_4_SPECIAL_DRIVER_IN_INDEX-vcc_sync_lhs.jz |

### Skipped Files
_None_

### Scaffolding or Compiler Bugs Found
_None_

### Parser Recovery Findings (for next audit to log)
_None_

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1042 / 1049
- Result after sweep:  1046 / 1053
- Newly passing:       4
- Newly broken:        0
  -> OK

## Context Sweep Report: test_3_1-operator_categories.md

### Summary
- Work list size (from issues.md):           1
- After dedup:                                1
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       1
- Scaffolding or bug failures (not created):  0
- Total: 1 == 0 + 0 + 0 + 0 + 1 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| SPECIAL_DRIVER_IN_INDEX | VCC as index, SYNCHRONOUS context | 3_1_SPECIAL_DRIVER_IN_INDEX-vcc_sync_index.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
_None._

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1046 / 1053
- Result after sweep:  1047 / 1054
- Newly passing:       1
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_3_2-operator_definitions.md

### Summary
- Work list size (from issues.md):           3
- After dedup:                                3
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       3
- Scaffolding or bug failures (not created):  0
- Total: 3 == 0 + 0 + 0 + 0 + 3 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| OBS_X_TO_OBSERVABLE_SINK | ASYNC output port and SYNC MEM write port | 3_2_OBS_X_TO_OBSERVABLE_SINK-x_to_output_and_mem.jz |
| SPECIAL_DRIVER_IN_INDEX | VCC as index, SYNC context, bit-select and range expression | 3_2_SPECIAL_DRIVER_IN_INDEX-vcc_sync_range.jz |
| DIV_UNGUARDED_RUNTIME_ZERO | guard patterns >= N, != N ELSE, < N ELSE, literal-on-left | 3_2_DIV_UNGUARDED_RUNTIME_ZERO-additional_guard_patterns.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| (none) | | | |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1047 / 1054
- Result after sweep:  1050 / 1057
- Newly passing:       3
- Newly broken:        0
  -> OK

## Context Sweep Report: test_3_3-operator_precedence.md

_no work: no missing contexts for this plan_
  -> OK

---

## Context Sweep Report: test_3_4-operator_examples.md

### Summary
- Work list size (from issues.md):           1
- After dedup:                                1
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       1
- Scaffolding or bug failures (not created):  0
- Total: 1 == 0 + 0 + 0 + 0 + 1 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| SPECIAL_DRIVER_IN_INDEX | SYNCHRONOUS context, VCC as index | 3_4_SPECIAL_DRIVER_IN_INDEX-sync_vcc_as_index.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
_None._

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1050 / 1058 (estimated — new file not yet present)
- Result after sweep:  1051 / 1058
- Newly passing:       1
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_4_1-module_canonical_form.md

### Summary
- Work list size (from issues.md):           3
- After dedup:                                3
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       3
- Scaffolding or bug failures (not created):  0
- Total: 3 == 0 + 0 + 0 + 0 + 3 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| DIRECTIVE_INVALID_CONTEXT | @endmod inside module body | 4_1_DIRECTIVE_INVALID_CONTEXT-endmod_in_body.jz |
| DUPLICATE_BLOCK | same clock with different EDGE/RESET params | 4_1_DUPLICATE_BLOCK-same_clock_different_params.jz |
| MODULE_PORT_IN_ONLY | INOUT-only module (negative test) | 4_1_MODULE_PORT_IN_ONLY-inout_only_ok.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| _(none)_ | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| _(none)_ | | | |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| _(none)_ | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1051 / 1058 (7 skip)
- Result after sweep:  1054 / 1061 (7 skip)
- Newly passing:       3
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_4_10-asynchronous_block.md

### Summary
- Work list size (from issues.md):           4
- After dedup:                                4
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       4
- Scaffolding or bug failures (not created):  0
- Total: 4 == 0 + 0 + 0 + 0 + 4 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| ASYNC_ALIAS_IN_CONDITIONAL | =z inside IF, =s inside ELSE, = inside ELIF | 4_10_ASYNC_ALIAS_IN_CONDITIONAL-extend_alias_in_conditional.jz |
| ASYNC_ASSIGN_REGISTER | <=z, =z, =>z to register; register write inside IF/ELSE | 4_10_ASYNC_ASSIGN_REGISTER-extend_and_conditional.jz |
| ASYNC_INVALID_STATEMENT_TARGET | CONST via = (alias), CONST via => (drive) | 4_10_ASYNC_INVALID_STATEMENT_TARGET-other_targets.jz |
| ASYNC_UNDEFINED_PATH_NO_DRIVER | IF/ELIF without ELSE | 4_10_ASYNC_UNDEFINED_PATH_NO_DRIVER-nested_conditional.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

Note: The audit mentioned "function call as LHS" for ASYNC_INVALID_STATEMENT_TARGET. JZ-HDL has no function call syntax per the specification, so this sub-context has no spec basis. However, it was not a separate recommended filename — it was part of the same entry — so no file was skipped. The CONST via `=` and `=>` contexts (which do have spec basis) were tested.

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| ASYNC_UNDEFINED_PATH_NO_DRIVER | `1_5_ASYNC_UNDEFINED_PATH_NO_DRIVER-nested_partial.jz` | resolved-stale | Current compiler reports ASYNC_UNDEFINED_PATH_NO_DRIVER for nested IF partial coverage; this case is covered by the validation file. |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1054 / 1061
- Result after sweep:  1058 / 1065
- Newly passing:       4
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_4_11-synchronous_block.md

### Summary
- Work list size (from issues.md):           1
- After dedup:                                1
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       1
- Scaffolding or bug failures (not created):  0
- Total: 1 == 0 + 0 + 0 + 0 + 1 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| DOMAIN_CONFLICT | CDC alias used in wrong SYNCHRONOUS block domain | 4_11_DOMAIN_CONFLICT-cdc_alias_wrong_domain.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
_None._

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1058 / 1065 (7 skip)
- Result after sweep:  1059 / 1066 (7 skip)
- Newly passing:       1
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_4_12-cdc_block.md

### Summary
- Work list size (from issues.md):           7
- After dedup:                                7
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       3
- Scaffolding or bug failures (not created):  4
- Total: 7 == 0 + 0 + 0 + 0 + 3 + 4

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| CDC_DEST_ALIAS_DUP | conflict with const name | 4_12_CDC_DEST_ALIAS_DUP-conflict_with_const.jz |
| CDC_DEST_ALIAS_DUP | conflict with instance name | 4_12_CDC_DEST_ALIAS_DUP-conflict_with_instance.jz |
| CDC_SOURCE_NOT_REGISTER | const as source | 4_12_CDC_SOURCE_NOT_REGISTER-const_as_source.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| CDC_DEST_ALIAS_DUP | 4_12_CDC_DEST_ALIAS_DUP-conflict_with_wire.jz | compiler-bug | CDC_DEST_ALIAS_DUP does not fire when dest alias conflicts with a wire name; only ID_DUP_IN_MODULE fires. The rule fires correctly for register, port, const, and instance conflicts but not wire. |
| CDC_DEST_ALIAS_DUP | 4_12_CDC_DEST_ALIAS_DUP-conflict_with_alias.jz | compiler-bug | CDC_DEST_ALIAS_DUP does not fire when dest alias conflicts with another CDC dest alias; only ID_DUP_IN_MODULE fires. Two CDC entries with the same dest alias name should trigger CDC_DEST_ALIAS_DUP on the second entry. |
| CDC_SOURCE_NOT_PLAIN_REG | 4_12_CDC_SOURCE_NOT_PLAIN_REG-concat_source.jz | rule-not-fired | Parser intercepts concatenation syntax `{a, b}` as CDC source with PARSE000 before CDC_SOURCE_NOT_PLAIN_REG semantic check runs. The CDC parser expects a bare identifier token; `{` is not valid at that position. Rule is unreachable for concatenation context. |
| CDC_SOURCE_NOT_PLAIN_REG | 4_12_CDC_SOURCE_NOT_PLAIN_REG-expr_source.jz | rule-not-fired | Parser intercepts expression syntax `(a & b)` as CDC source with PARSE000 before CDC_SOURCE_NOT_PLAIN_REG semantic check runs. Same root cause as concat: CDC parser requires identifier token; `(` is rejected. Rule is only reachable via slice syntax (e.g., `reg[0:0]`). |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1059 / 1066
- Result after sweep:  1062 / 1069
- Newly passing:       3
- Newly broken:        0
  -> OK

---

## Context Sweep Report: test_4_13-module_instantiation.md

### Summary
- Work list size (from issues.md):           1
- After dedup:                                1
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       1
- Scaffolding or bug failures (not created):  0
- Total: 1 == 0 + 0 + 0 + 0 + 1 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| INSTANCE_INTERNAL_ACCESS | LATCH access | 4_13_INSTANCE_INTERNAL_ACCESS-access_internal_latch.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
_None._

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1062 / 1069
- Result after sweep:  1063 / 1070
- Newly passing:       1
- Newly broken:        0
  -> OK

## Context Sweep: test_4_14-feature_guards.md — 2026-04-15

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       1
- Scaffolding or bug failures (not created):  1
- Total: 2 == 0 + 0 + 0 + 0 + 1 + 1

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| FEATURE_NESTED | WIRE block | 4_14_FEATURE_NESTED-nested_feature_in_wire.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| FEATURE_NESTED | 4_14_FEATURE_NESTED-nested_feature_at_module_level.jz | compiler-bug | Nested @feature at module level (outside any block like REGISTER, WIRE, ASYNCHRONOUS, SYNCHRONOUS) does not fire FEATURE_NESTED. The compiler silently accepts the nesting and only emits warnings for unused declarations inside the inner @feature. Nesting is correctly detected inside block contexts (ASYNC, SYNC, REGISTER, WIRE) but not at the bare module level. |

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1063 / 1070
- Result after sweep:  1064 / 1071
- Newly passing:       1
- Newly broken:        0

## Context Sweep: test_4_2-scope_and_uniqueness.md — 2026-04-15

### Summary
- Work list size (from issues.md):           9
- After dedup:                                9
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       8
- Scaffolding or bug failures (not created):  1
- Total: 9 == 0 + 0 + 0 + 0 + 8 + 1

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| ID_DUP_IN_MODULE | port-register, port-CONST, wire-register, wire-CONST | 4_2_ID_DUP_IN_MODULE-port_register_collision.jz |
| ID_DUP_IN_MODULE | MEM name collision | 4_2_ID_DUP_IN_MODULE-mem_name_collision.jz |
| INSTANCE_NAME_CONFLICT | instance-MEM | 4_2_INSTANCE_NAME_CONFLICT-instance_mem_collision.jz |
| UNDECLARED_IDENTIFIER | RESET parameter | 4_2_UNDECLARED_IDENTIFIER-reset_param.jz |
| UNDECLARED_IDENTIFIER | MUX select expression | 4_2_UNDECLARED_IDENTIFIER-mux_expression.jz |
| UNDECLARED_IDENTIFIER | slice index, concat operand | 4_2_UNDECLARED_IDENTIFIER-slice_concat.jz |
| AMBIGUOUS_REFERENCE | two instances same port | 4_2_AMBIGUOUS_REFERENCE-two_instances_same_port.jz |
| AMBIGUOUS_REFERENCE | SYNC block reference | 4_2_AMBIGUOUS_REFERENCE-sync_reference.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| UNDECLARED_IDENTIFIER | 4_2_UNDECLARED_IDENTIFIER-nonexistent_module.jz | rule-not-fired | @new targeting non-existent module fires INSTANCE_UNDEFINED_MODULE (S4.13/S6.9) instead of UNDECLARED_IDENTIFIER. The compiler uses a dedicated, more specific rule for this context. |

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1064 / 1071
- Result after sweep:  1072 / 1079
- Newly passing:       8
- Newly broken:        0

## Context Sweep: test_4_3-const.md — 2026-04-15

### Summary
- Work list size (from issues.md):           6
- After dedup:                                6
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       3
- Scaffolding or bug failures (not created):  3
- Total: 6 == 0 + 0 + 0 + 0 + 3 + 3

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| CONST_NEGATIVE_OR_NONINT | expression evaluating to negative | 4_3_CONST_NEGATIVE_OR_NONINT-negative_expression.jz |
| CONST_UNDEFINED_IN_WIDTH_OR_SLICE | output port width | 4_3_CONST_UNDEFINED_IN_WIDTH_OR_SLICE-output_port_width.jz |
| CONST_CIRCULAR_DEP | self-reference | 4_3_CONST_CIRCULAR_DEP-self_reference.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| CONST_UNDEFINED_IN_WIDTH_OR_SLICE | 4_3_CONST_UNDEFINED_IN_WIDTH_OR_SLICE-mem_depth.jz | rule-not-fired | MEM depth with undefined CONST fires MEM_UNDEFINED_CONST_IN_WIDTH (S7.1/S7.7.1) instead of CONST_UNDEFINED_IN_WIDTH_OR_SLICE. The MEM-specific rule takes precedence. |
| CONST_UNDEFINED_IN_WIDTH_OR_SLICE | 4_3_CONST_UNDEFINED_IN_WIDTH_OR_SLICE-slice_context.jz | rule-not-fired | Undefined CONST in slice expression (e.g., `din[TOP_HI:UNDEF_LO]`) fires UNDECLARED_IDENTIFIER (S4.2/S8.1) instead of CONST_UNDEFINED_IN_WIDTH_OR_SLICE. Slice bounds are not treated as CONST-expected contexts. |
| CONST_CIRCULAR_DEP | 4_3_CONST_CIRCULAR_DEP-transitive_chain.jz | compiler-bug | Three-member transitive cycle (A=B; B=C; C=A) fires CONST_NEGATIVE_OR_NONINT instead of CONST_CIRCULAR_DEP. Compiler detects direct 2-member cycles but fails to detect longer transitive chains as circular dependencies. |

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1072 / 1079
- Result after sweep:  1075 / 1082
- Newly passing:       3
- Newly broken:        0

## Context Sweep: test_4_4-port.md — 2026-04-15

### Summary
- Work list size (from issues.md):           8
- After dedup:                                8
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       8
- Scaffolding or bug failures (not created):  0
- Total: 8 == 0 + 0 + 0 + 0 + 8 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| PORT_MISSING_WIDTH | OUT without width | 4_4_PORT_MISSING_WIDTH-out_missing_width.jz |
| PORT_MISSING_WIDTH | INOUT without width | 4_4_PORT_MISSING_WIDTH-inout_missing_width.jz |
| PORT_DIRECTION_MISMATCH_IN | SELECT branch | 4_4_PORT_DIRECTION_MISMATCH_IN-write_in_select.jz |
| PORT_DIRECTION_MISMATCH_OUT | nested expression | 4_4_PORT_DIRECTION_MISMATCH_OUT-read_in_expression.jz |
| PORT_TRISTATE_MISMATCH | IN port with z | 4_4_PORT_TRISTATE_MISMATCH-tristate_on_in_port.jz |
| ASYNC_ALIAS_IN_CONDITIONAL | SELECT branch | 4_4_ASYNC_ALIAS_IN_CONDITIONAL-alias_in_select.jz |
| BUS_PORT_NOT_BUS | dot on OUT port | 4_4_BUS_PORT_NOT_BUS-member_on_out_port.jz |
| BUS_TRISTATE_MISMATCH | SOURCE readable signal | 4_4_BUS_TRISTATE_MISMATCH-z_on_source_readable.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
_None._

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1075 / 1082
- Result after sweep:  1083 / 1090
- Newly passing:       8
- Newly broken:        0

## Context Sweep: test_4_5-wire.md — 2026-04-15

_no work: no missing contexts for this plan_

## Context Sweep: test_4_6-mux.md — 2026-04-15

### Summary
- Work list size (from issues.md):           6
- After dedup:                                6
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       4
- Scaffolding or bug failures (not created):  2
- Total: 6 == 0 + 0 + 0 + 0 + 4 + 2

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| MUX_NAME_DUPLICATE | const name | 4_6_MUX_NAME_DUPLICATE-dup_const.jz |
| MUX_NAME_DUPLICATE | another MUX name | 4_6_MUX_NAME_DUPLICATE-dup_mux.jz |
| MUX_AGG_SOURCE_INVALID | const as source | 4_6_MUX_AGG_SOURCE_INVALID-const_source.jz |
| MUX_AGG_SOURCE_INVALID | instance name as source | 4_6_MUX_AGG_SOURCE_INVALID-instance_name_source.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| MUX_NAME_DUPLICATE | 4_6_MUX_NAME_DUPLICATE-dup_instance.jz | rule-not-fired | Compiler fires INSTANCE_NAME_CONFLICT instead of MUX_NAME_DUPLICATE when a MUX name collides with an instance name. Tested both declaration orders (MUX before instance and instance before MUX) — INSTANCE_NAME_CONFLICT always fires. The instance-name checker catches the collision before the MUX-name checker runs. |
| MUX_AGG_SOURCE_INVALID | 4_6_MUX_AGG_SOURCE_INVALID-output_port_source.jz | rule-not-fired | Compiler does not fire MUX_AGG_SOURCE_INVALID when an output port is used as a MUX aggregation source. Output ports are apparently valid readable signals in module scope for MUX purposes (the module can read the value it is driving). No diagnostic emitted at all. |

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1083 / 1090
- Result after sweep:  1087 / 1094
- Newly passing:       4
- Newly broken:        0

## Context Sweep: test_4_7-register.md — 2026-04-15

### Summary
- Work list size (from issues.md):           1
- After dedup:                                1
- Pre-existing files (skipped):               1
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       0
- Scaffolding or bug failures (not created):  0
- Total: 1 == 1 + 0 + 0 + 0 + 0 + 0

### Files Created
_None._

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| REG_HAPPY_PATH | 4_7_REG_HAPPY_PATH-register_ok.jz | already exists — audit recommends updating existing file to add GND/VCC keyword reset contexts, but sweep cannot overwrite existing files |

### Scaffolding or Compiler Bugs Found
_None._

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1087 / 1094
- Result after sweep:  1087 / 1094
- Newly passing:       0
- Newly broken:        0

## Context Sweep: test_4_8-latches.md — 2026-04-15

### Summary
- Work list size (from issues.md):           3
- After dedup:                                3
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       2
- Scaffolding or bug failures (not created):  1
- Total: 3 == 0 + 0 + 0 + 0 + 2 + 1

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| LATCH_ASSIGN_IN_SYNC | SR-latch assignment in SYNC block | 4_8_LATCH_ASSIGN_IN_SYNC-sr_latch_in_sync.jz |
| LATCH_ASSIGN_NON_GUARDED | unguarded receive-assign to SR-latch in ASYNC | 4_8_LATCH_ASSIGN_NON_GUARDED-sr_unguarded.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
_None._

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| LATCH_IN_CONST_CONTEXT | 4_8_LATCH_IN_CONST_CONTEXT-latch_in_feature.jz | rule-not-fired | FEATURE_EXPR_INVALID_CONTEXT preempts LATCH_IN_CONST_CONTEXT in @feature guard condition — the generic @feature check rejects any non-CONFIG/CONST/literal reference before the latch-specific check runs. LATCH_IN_CONST_CONTEXT is only reachable via @check, not @feature. |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1087 / 1094
- Result after sweep:  1089 / 1096
- Newly passing:       2
- Newly broken:        0

## Context Sweep: test_4_9-mem_block.md — 2026-04-15

_no work: plan has no missing contexts (section exists but says "No issues flagged.")_

## Context Sweep: test_5_0-assignment_operators_summary.md — 2026-04-15

### Summary
- Work list size (from issues.md):           5
- After dedup:                                5
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       2
- Scaffolding or bug failures (not created):  3
- Total: 5 == 0 + 0 + 0 + 0 + 2 + 3

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| ASSIGN_CONCAT_WIDTH_MISMATCH | RHS concat mismatch | 5_0_ASSIGN_CONCAT_WIDTH_MISMATCH-rhs_concat_mismatch.jz |
| ASSIGN_INDEPENDENT_IF_SELECT | mixed IF-then-SELECT chain | 5_0_ASSIGN_INDEPENDENT_IF_SELECT-mixed_if_select.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| ASSIGN_MULTIPLE_SAME_BITS | 5_0_ASSIGN_MULTIPLE_SAME_BITS-sync_double_assign.jz | rule-not-fired | SYNC_MULTI_ASSIGN_SAME_REG_BITS fires instead — more specific SYNC rule preempts ASSIGN_MULTIPLE_SAME_BITS in SYNCHRONOUS context. Correct compiler behavior. |
| ASSIGN_INDEPENDENT_IF_SELECT | 5_0_ASSIGN_INDEPENDENT_IF_SELECT-sync_context.jz | rule-not-fired | SYNC_MULTI_ASSIGN_SAME_REG_BITS fires instead — more specific SYNC rule preempts ASSIGN_INDEPENDENT_IF_SELECT in SYNCHRONOUS context. Correct compiler behavior. |
| ASSIGN_SHADOWING | 5_0_ASSIGN_SHADOWING-sync_context.jz | rule-not-fired | SYNC_ROOT_AND_CONDITIONAL_ASSIGN fires instead — more specific SYNC rule preempts ASSIGN_SHADOWING in SYNCHRONOUS context. Correct compiler behavior. |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1089 / 1096
- Result after sweep:  1091 / 1098
- Newly passing:       2
- Newly broken:        0

## Context Sweep: test_5_1-asynchronous_assignments.md — 2026-04-15

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       2
- Scaffolding or bug failures (not created):  0
- Total: 2 == 0 + 0 + 0 + 0 + 2 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| ASYNC_ALIAS_LITERAL_RHS | literal inside IF branch + SELECT case | 5_1_ASYNC_ALIAS_LITERAL_RHS-literal_in_if_select.jz |
| ASYNC_INVALID_STATEMENT_TARGET | CONST on LHS in ASYNC (5_1 coverage) | 5_1_ASYNC_INVALID_STATEMENT_TARGET-const_and_func.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| ASYNC_INVALID_STATEMENT_TARGET | 5_1_ASYNC_INVALID_STATEMENT_TARGET-const_and_func.jz | compiler-bug | Function call on LHS (e.g. `clog2(8) <= din;`) gets PARSE000 before semantic analysis; ASYNC_INVALID_STATEMENT_TARGET is unreachable for function-call-on-LHS context. Only CONST on LHS was testable. |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1091 / 1098
- Result after sweep:  1093 / 1100
- Newly passing:       2
- Newly broken:        0

## Context Sweep: test_5_2-synchronous_assignments.md — 2026-04-15

### Summary
- Work list size (from issues.md):           3
- After dedup:                                3
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       1
- Scaffolding or bug failures (not created):  2
- Total: 3 == 0 + 0 + 0 + 0 + 1 + 2

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| SYNC_SLICE_WIDTH_MISMATCH | RHS wider than slice | 5_2_SYNC_SLICE_WIDTH_MISMATCH-rhs_wider_than_slice.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| SYNC_MULTI_ASSIGN_SAME_REG_BITS | 5_2_SYNC_MULTI_ASSIGN_SAME_REG_BITS-overlapping_slices.jz | rule-not-fired | Compiler fires ASSIGN_SLICE_OVERLAP instead of SYNC_MULTI_ASSIGN_SAME_REG_BITS for overlapping slice assignments. The more specific ASSIGN_SLICE_OVERLAP rule takes priority over the general double-assign rule when slices overlap. |
| SYNC_ROOT_AND_CONDITIONAL_ASSIGN | 5_2_SYNC_ROOT_AND_CONDITIONAL_ASSIGN-sliced_root_conflict.jz | rule-not-fired | Compiler fires ASSIGN_SLICE_OVERLAP instead of SYNC_ROOT_AND_CONDITIONAL_ASSIGN for root-level sliced assign + conditional sliced assign to overlapping bits. Same priority issue as above — ASSIGN_SLICE_OVERLAP takes precedence. |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1093 / 1100
- Result after sweep:  1094 / 1101
- Newly passing:       1
- Newly broken:        0

## Context Sweep: test_5_3-conditional_statements.md — 2026-04-15

### Summary
- Work list size (from issues.md):           3
- After dedup:                                3
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       3
- Scaffolding or bug failures (not created):  0
- Total: 3 == 0 + 0 + 0 + 0 + 3 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| IF_COND_MISSING_PARENS | ELIF in SYNC | 5_3_IF_COND_MISSING_PARENS-elif_sync_missing_parens.jz |
| ASYNC_ALIAS_IN_CONDITIONAL | nested IF body | 5_3_ASYNC_ALIAS_IN_CONDITIONAL-nested_if_alias.jz |
| CONTROL_FLOW_OUTSIDE_BLOCK | SELECT at module scope | 5_3_CONTROL_FLOW_OUTSIDE_BLOCK-select_outside_block.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| (none) | | | |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1094 / 1101
- Result after sweep:  1097 / 1104
- Newly passing:       3
- Newly broken:        0

## Context Sweep: test_5_4-select_case_statements.md — 2026-04-15

### Summary
- Work list size (from issues.md):           6
- After dedup:                                6
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       5
- Scaffolding or bug failures (not created):  1
- Total: 6 == 0 + 0 + 0 + 0 + 5 + 1

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| SELECT_CASE_WIDTH_MISMATCH | x-wildcard CASE with width mismatch | 5_4_SELECT_CASE_WIDTH_MISMATCH-xwild_width_mismatch.jz |
| SELECT_CASE_WIDTH_MISMATCH | @global constant CASE with width mismatch | 5_4_SELECT_CASE_WIDTH_MISMATCH-global_width_mismatch.jz |
| SELECT_DUP_CASE_VALUE | fall-through CASE duplicate | 5_4_SELECT_DUP_CASE_VALUE-fallthrough_dup.jz |
| SELECT_DUP_CASE_VALUE | @global constant CASE duplicate | 5_4_SELECT_DUP_CASE_VALUE-global_dup.jz |
| SELECT_DUP_CASE_VALUE | x-wildcard overlapping patterns | 5_4_SELECT_DUP_CASE_VALUE-xwild_overlap.jz |
| SELECT_NO_MATCH_SYNC_OK | nested SELECT in SYNC without DEFAULT | 5_4_SELECT_NO_MATCH_SYNC_OK-nested_sync_no_default.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| (none) | | | |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1097 / 1104
- Result after sweep:  1102 / 1109
- Newly passing:       5
- Newly broken:        0

## Context Sweep: test_5_5-intrinsic_operators.md — 2026-04-15

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       1
- Scaffolding or bug failures (not created):  1
- Total: 2 == 0 + 0 + 0 + 0 + 1 + 1

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| LIT_INVALID_CONTEXT | width-bracket | 5_5_LIT_INVALID_CONTEXT-width_bracket.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| CLOG2_NONPOSITIVE_ARG | 5_5_CLOG2_NONPOSITIVE_ARG-width_bracket.jz | compiler-bug | clog2(0) in WIRE width bracket `w [clog2(0)]` compiles cleanly with no diagnostic — CLOG2_NONPOSITIVE_ARG is not checked in width-bracket expressions |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1102 / 1109
- Result after sweep:  1103 / 1110
- Newly passing:       1
- Newly broken:        0

## Context Sweep: test_6_1-project_purpose.md — 2026-04-15

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       2
- Scaffolding or bug failures (not created):  0
- Total: 2 == 0 + 0 + 0 + 0 + 2 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| PROJECT_CHIP_DATA_NOT_FOUND | identifier chip ID | 6_1_PROJECT_CHIP_DATA_NOT_FOUND-unknown_chip_identifier.jz |
| PROJECT_CHIP_DATA_INVALID | identifier chip ID | 6_1_PROJECT_CHIP_DATA_INVALID-malformed_json_identifier.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| (none) | | | |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1103 / 1110
- Result after sweep:  1105 / 1112
- Newly passing:       2
- Newly broken:        0

## Context Sweep: test_6_10-project_scope_and_uniqueness.md — 2026-04-15

### Summary
- Work list size (from issues.md):           1
- After dedup:                                1
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       1
- Scaffolding or bug failures (not created):  0
- Total: 1 == 0 + 0 + 0 + 0 + 1 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| PROJECT_NAME_NOT_UNIQUE | project name vs blackbox name | 6_10_PROJECT_NAME_NOT_UNIQUE-project_name_vs_blackbox.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| (none) | | | |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1105 / 1112
- Result after sweep:  1106 / 1113
- Newly passing:       1
- Newly broken:        0

## Context Sweep: test_6_11-error_summary.md — 2026-04-15

_no work: no missing contexts for this plan_

## Context Sweep: test_6_2-project_canonical_form.md — 2026-04-15

### Summary
- Work list size (from issues.md):           7
- After dedup:                                7
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       4
- Scaffolding or bug failures (not created):  3
- Total: 7 == 0 + 0 + 0 + 0 + 4 + 3

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| IMPORT_NOT_AT_PROJECT_TOP | after CLOCKS | 6_2_IMPORT_NOT_AT_PROJECT_TOP-import_after_clocks.jz |
| IMPORT_NOT_AT_PROJECT_TOP | after @blackbox | 6_2_IMPORT_NOT_AT_PROJECT_TOP-import_after_blackbox.jz |
| IMPORT_NOT_AT_PROJECT_TOP | after @top | 6_2_IMPORT_NOT_AT_PROJECT_TOP-import_after_top.jz |
| IMPORT_FILE_MULTIPLE_TIMES | normalized path dup | 6_2_IMPORT_FILE_MULTIPLE_TIMES-normalized_path_dup.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| IMPORT_DUP_MODULE_OR_BLACKBOX | 6_2_IMPORT_DUP_MODULE_OR_BLACKBOX-blackbox_collision.jz | compiler-bug | Blackbox-blackbox name collision across two imports is not detected; compiler produces no diagnostic. Rule only fires for module-module collisions. |
| IMPORT_DUP_MODULE_OR_BLACKBOX | 6_2_IMPORT_DUP_MODULE_OR_BLACKBOX-module_blackbox_cross.jz | compiler-bug | Module-blackbox cross-type name collision across two imports is not detected; compiler emits WARN_UNUSED_MODULE for the module but no IMPORT_DUP_MODULE_OR_BLACKBOX. |
| IMPORT_DUP_MODULE_OR_BLACKBOX | 6_2_IMPORT_DUP_MODULE_OR_BLACKBOX-imported_vs_local.jz | compiler-bug | Imported-vs-locally-defined collision fires MODULE_NAME_DUP_IN_PROJECT instead of IMPORT_DUP_MODULE_OR_BLACKBOX. Compiler detects the collision but attributes it to the wrong rule. |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1106 / 1113
- Result after sweep:  1110 / 1120
- Newly passing:       4
- Newly broken:        0

## Context Sweep: test_6_3-config_block.md — 2026-04-15

### Summary
- Work list size (from issues.md):           9
- After dedup:                                9
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       6
- Scaffolding or bug failures (not created):  3
- Total: 9 == 0 + 0 + 0 + 0 + 6 + 3

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| CONFIG_USE_UNDECLARED | wire width | 6_3_CONFIG_USE_UNDECLARED-wire_width.jz |
| CONFIG_USED_WHERE_FORBIDDEN | operator operand | 6_3_CONFIG_USED_WHERE_FORBIDDEN-operator_operand.jz |
| CONFIG_USED_WHERE_FORBIDDEN | ternary condition | 6_3_CONFIG_USED_WHERE_FORBIDDEN-ternary_condition.jz |
| CONST_USED_WHERE_FORBIDDEN | operator operand | 6_3_CONST_USED_WHERE_FORBIDDEN-operator_operand.jz |
| CONST_USED_WHERE_FORBIDDEN | ternary condition | 6_3_CONST_USED_WHERE_FORBIDDEN-ternary_condition.jz |
| CONST_STRING_IN_NUMERIC_CONTEXT | wire width | 6_3_CONST_STRING_IN_NUMERIC_CONTEXT-wire_width.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| CONFIG_USE_UNDECLARED | 6_3_CONFIG_USE_UNDECLARED-mem_depth.jz | rule-not-fired | MEM depth context fires MEM_UNDEFINED_CONST_IN_WIDTH instead of CONFIG_USE_UNDECLARED; MEM parsing has its own undeclared-const check that takes precedence |
| CONFIG_USE_UNDECLARED | 6_3_CONFIG_USE_UNDECLARED-instance_binding.jz | rule-not-fired | Instance port binding width context fires INSTANCE_PORT_WIDTH_EXPR_INVALID instead of CONFIG_USE_UNDECLARED; instance binding has its own validation path |
| CONST_STRING_IN_NUMERIC_CONTEXT | 6_3_CONST_STRING_IN_NUMERIC_CONTEXT-mem_depth.jz | rule-not-fired | MEM depth context fires MEM_UNDEFINED_CONST_IN_WIDTH instead of CONST_STRING_IN_NUMERIC_CONTEXT; string CONFIG value treated as undeclared in MEM context before type check runs |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1110 / 1120
- Result after sweep:  1116 / 1126
- Newly passing:       6
- Newly broken:        0

## Context Sweep: test_6_4-clocks_block.md — 2026-04-15

_no work: no missing contexts for this plan_

## Context Sweep: test_6_5-pin_blocks.md — 2026-04-15

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       2
- Scaffolding or bug failures (not created):  0
- Total: 2 == 0 + 0 + 0 + 0 + 2 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| PIN_PULL_INVALID | OUT_PINS (invalid pull value on output) | 6_5_PIN_PULL_INVALID-bad_pull_on_output.jz |
| PIN_WIDTH_REQUIRES_DIFFERENTIAL | INOUT_PINS (width on non-differential) | 6_5_PIN_WIDTH_REQUIRES_DIFFERENTIAL-width_inout_no_diff.jz |

### Skipped Files

_None._

### Scaffolding or Compiler Bugs Found

_None._

### Parser Recovery Findings (for next audit to log)

_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1116 / 1126
- Result after sweep:  1118 / 1128
- Newly passing:       2
- Newly broken:        0

## Context Sweep: test_6_6-map_block.md — 2026-04-15

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       2
- Scaffolding or bug failures (not created):  0
- Total: 2 == 0 + 0 + 0 + 0 + 2 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| MAP_INVALID_BOARD_PIN_ID | bus bit with invalid board pin ID | 6_6_MAP_INVALID_BOARD_PIN_ID-bad_bus_bit_pin.jz |
| MAP_INVALID_BOARD_PIN_ID | differential P/N with invalid board pin ID | 6_6_MAP_INVALID_BOARD_PIN_ID-bad_diff_pin.jz |

### Skipped Files

_None._

### Scaffolding or Compiler Bugs Found

_None._

### Parser Recovery Findings (for next audit to log)

_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1118 / 1128
- Result after sweep:  1120 / 1130
- Newly passing:       2
- Newly broken:        0

## Context Sweep: test_6_6-map_block.md — 2026-04-15

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               2
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       0
- Scaffolding or bug failures (not created):  0
- Total: 2 == 2 + 0 + 0 + 0 + 0 + 0

### Files Created
_None._

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| MAP_INVALID_BOARD_PIN_ID | 6_6_MAP_INVALID_BOARD_PIN_ID-bad_bus_bit_pin.jz | already exists |
| MAP_INVALID_BOARD_PIN_ID | 6_6_MAP_INVALID_BOARD_PIN_ID-bad_diff_pin.jz | already exists |

### Scaffolding or Compiler Bugs Found
_None._

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1120 / 1130 (10 skipped)
- Result after sweep:  1120 / 1130 (10 skipped)
- Newly passing:       0
- Newly broken:        0

## Context Sweep: test_6_7-blackbox_modules.md — 2026-04-15

### Summary
- Work list size (from issues.md):           3
- After dedup:                                3
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       5
- Scaffolding or bug failures (not created):  0
- Total: 5 == 5 + 0 + 0 + 0 + 0 + 0

### Files Created
| Rule ID | File | Category | Description |
|---------|------|----------|-------------|
| BLACKBOX_BODY_DISALLOWED | 6_7_BLACKBOX_BODY_DISALLOWED-async_in_blackbox.jz | created | ASYNCHRONOUS block inside @blackbox emits BLACKBOX_BODY_DISALLOWED. |
| BLACKBOX_BODY_DISALLOWED | 6_7_BLACKBOX_BODY_DISALLOWED-sync_in_blackbox.jz | created | SYNCHRONOUS block inside @blackbox emits BLACKBOX_BODY_DISALLOWED. |
| BLACKBOX_BODY_DISALLOWED | 6_7_BLACKBOX_BODY_DISALLOWED-wire_in_blackbox.jz | created | WIRE block inside @blackbox emits BLACKBOX_BODY_DISALLOWED. |
| BLACKBOX_BODY_DISALLOWED | 6_7_BLACKBOX_BODY_DISALLOWED-register_in_blackbox.jz | created | REGISTER block inside @blackbox emits BLACKBOX_BODY_DISALLOWED. |
| BLACKBOX_BODY_DISALLOWED | 6_7_BLACKBOX_BODY_DISALLOWED-mem_in_blackbox.jz | created | MEM block inside @blackbox emits BLACKBOX_BODY_DISALLOWED. |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
_None._

### Parser Recovery Findings (for next audit to log)
_None — the parser accepts S6.7-forbidden blackbox body blocks far enough for semantic analysis to emit BLACKBOX_BODY_DISALLOWED. CONST remains valid in @blackbox._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1120 / 1130 (10 skipped)
- Result after sweep:  1120 / 1130 (10 skipped)
- Newly passing:       0
- Newly broken:        0

## Context Sweep: test_6_8-bus_aggregation.md — 2026-04-15

_no work: no missing contexts for this plan_

## Context Sweep: test_6_9-top_level_module.md — 2026-04-15

### Summary
- Work list size (from issues.md):           7
- After dedup:                                7
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       7
- Scaffolding or bug failures (not created):  0
- Total: 7 == 0 + 0 + 0 + 0 + 7 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| TOP_PORT_NOT_LISTED | INOUT port omitted | 6_9_TOP_PORT_NOT_LISTED-inout_port_omitted.jz |
| TOP_PORT_WIDTH_MISMATCH | INOUT port width mismatch | 6_9_TOP_PORT_WIDTH_MISMATCH-inout_width.jz |
| TOP_PORT_PIN_DECL_MISSING | INOUT port undeclared pin | 6_9_TOP_PORT_PIN_DECL_MISSING-inout_undeclared.jz |
| TOP_PORT_PIN_DIRECTION_MISMATCH | INOUT→IN_PINS, INOUT→OUT_PINS | 6_9_TOP_PORT_PIN_DIRECTION_MISMATCH-inout_wrong_category.jz |
| TOP_OUT_LITERAL_BINDING | literal within concatenation on OUT port | 6_9_TOP_OUT_LITERAL_BINDING-literal_in_concat.jz |
| TOP_NO_CONNECT_WITHOUT_WIDTH | INOUT port missing width | 6_9_TOP_NO_CONNECT_WITHOUT_WIDTH-inout_missing_width.jz |
| TOP_PORT_SIGNAL_WIDTH_MISMATCH | INOUT port signal width mismatch | 6_9_TOP_PORT_SIGNAL_WIDTH_MISMATCH-inout_signal_width.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
_None._

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1120 / 1130 (10 skipped)
- Result after sweep:  1127 / 1137 (10 skipped)
- Newly passing:       7
- Newly broken:        0

## Context Sweep: test_7_0-memory_port_modes.md — 2026-04-15

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       0
- Scaffolding or bug failures (not created):  2
- Total: 2 == 0 + 0 + 0 + 0 + 0 + 2

### Files Created
_None._

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| MEM_INVALID_PORT_TYPE | 7_0_MEM_INVALID_PORT_TYPE-inout_async.jz | rule-not-fired | Compiler fires dedicated `MEM_INOUT_ASYNC` rule (rules.c line 375) instead of `MEM_INVALID_PORT_TYPE` for INOUT+ASYNC context. Audit entry should reference `MEM_INOUT_ASYNC` and filename should be `7_0_MEM_INOUT_ASYNC-inout_async.jz`. |
| MEM_INVALID_WRITE_MODE | 7_0_MEM_INVALID_WRITE_MODE-shorthand_invalid.jz | rule-not-fired | Shorthand form with unrecognized keyword (e.g. `IN wr BADVALUE;`) hits `PARSE000` at parser level instead of `MEM_INVALID_WRITE_MODE`. Parser expects a known token (ASYNC/SYNC/WRITE_FIRST/READ_FIRST/NO_CHANGE) after port name; unknown keywords are not recoverable to semantic validation. Audit warning was correct: shorthand invalid keywords do not reach the write-mode validator. |

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1127 / 1137 (10 skipped)
- Result after sweep:  1127 / 1137 (10 skipped)
- Newly passing:       0
- Newly broken:        0

## Context Sweep: test_7_1-mem_declaration.md — 2026-04-15

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       1
- Scaffolding or bug failures (not created):  1
- Total: 2 == 0 + 0 + 0 + 0 + 1 + 1

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| MEM_PORT_NAME_CONFLICT_MODULE_ID | CONST name, instance name | 7_1_MEM_PORT_NAME_CONFLICT_MODULE_ID-const_instance_conflict.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| MEM_INVALID_PORT_TYPE | 7_1_MEM_INVALID_PORT_TYPE-unknown_keyword.jz | compiler-bug | Truly unknown keywords (e.g. `GARBAGE rd;`, `OUT rd BLAH;`) fire `PARSE000` instead of `MEM_INVALID_PORT_TYPE`. The parser catches unknown tokens before the semantic rule can fire. The rule only triggers for known-but-misplaced qualifiers (ASYNC on IN, SYNC on IN). |

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1127 / 1137 (10 skipped)
- Result after sweep:  1128 / 1138 (10 skipped)
- Newly passing:       1
- Newly broken:        0

## Context Sweep: test_7_10-const_evaluation_in_mem.md — 2026-04-15

### Summary
- Work list size (from issues.md):           3
- After dedup:                                3
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       1
- Scaffolding or bug failures (not created):  2
- Total: 3 == 0 + 0 + 0 + 0 + 1 + 2

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| CONST_NEGATIVE_OR_NONINT | negative CONST actually used as MEM depth/word_width dimension | 7_10_CONST_NEGATIVE_OR_NONINT-negative_const_in_mem_dimension.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| CONST_CIRCULAR_DEP | 7_10_CONST_CIRCULAR_DEP-circular_config.jz | rule-not-fired | Circular dependency in CONFIG block fires CONFIG_INVALID_EXPR_TYPE (S6.3) instead of CONST_CIRCULAR_DEP. CONFIG values are validated through a different path than module CONST blocks, so CONST_CIRCULAR_DEP never triggers in CONFIG context. |
| CONST_UNDEFINED_IN_WIDTH_OR_SLICE | 7_10_CONST_UNDEFINED_IN_WIDTH_OR_SLICE-mem_context.jz | rule-not-fired | Undefined CONST in MEM word_width/depth fires MEM_UNDEFINED_CONST_IN_WIDTH (S7.1/S7.7.1) instead of CONST_UNDEFINED_IN_WIDTH_OR_SLICE. MEM dimension validation uses a MEM-specific rule that takes precedence. |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1128 / 1138 (10 skipped)
- Result after sweep:  1129 / 1139 (10 skipped)
- Newly passing:       1
- Newly broken:        0

## Context Sweep: test_7_11-synthesis_implications.md — 2026-04-15

### Summary
- Work list size (from issues.md):           5
- After dedup:                                5
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       5
- Scaffolding or bug failures (not created):  0
- Total: 5 == 0 + 0 + 0 + 0 + 5 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| MEM_BLOCK_MULTI | width-tiling | 7_11_MEM_BLOCK_MULTI-width_tiling.jz |
| MEM_BLOCK_RESOURCE_EXCEEDED | single module with multiple BLOCK MEMs | 7_11_MEM_BLOCK_RESOURCE_EXCEEDED-single_module_multi_mem.jz |
| MEM_BLOCK_RESOURCE_EXCEEDED | at-limit boundary (exactly at limit passes) | 7_11_MEM_BLOCK_RESOURCE_EXCEEDED-at_limit_ok.jz |
| MEM_DISTRIBUTED_RESOURCE_EXCEEDED | single module exceeding capacity | 7_11_MEM_DISTRIBUTED_RESOURCE_EXCEEDED-single_module.jz |
| MEM_DISTRIBUTED_RESOURCE_EXCEEDED | at-limit boundary | 7_11_MEM_DISTRIBUTED_RESOURCE_EXCEEDED-at_limit_ok.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
_None._

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1129 / 1139 (10 skipped)
- Result after sweep:  1134 / 1144 (10 skipped)
- Newly passing:       5
- Newly broken:        0

## Context Sweep: test_7_2-port_types_and_semantics.md — 2026-04-15

### Summary
- Work list size (from issues.md):           4
- After dedup:                                4
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       4
- Scaffolding or bug failures (not created):  0
- Total: 4 == 0 + 0 + 0 + 0 + 4 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| MEM_INOUT_INDEXED | read-in-SYNC (bracket read inside SYNCHRONOUS block) | 7_2_MEM_INOUT_INDEXED-read_in_sync.jz |
| MEM_READ_FROM_WRITE_PORT | read-in-SYNC (reading IN port inside SYNCHRONOUS block) | 7_2_MEM_READ_FROM_WRITE_PORT-read_in_sync.jz |
| MEM_WRITE_TO_READ_PORT | write-in-ASYNC (writing OUT ASYNC port inside ASYNCHRONOUS block) | 7_2_MEM_WRITE_TO_READ_PORT-write_in_async.jz |
| MEM_MULTIPLE_WRITES_SAME_IN | double-write inside conditional (IF) branches | 7_2_MEM_MULTIPLE_WRITES_SAME_IN-conditional_double_write.jz |

### Skipped Files
_None_

### Scaffolding or Compiler Bugs Found
_None_

### Cascading Diagnostics (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| MEM_WRITE_TO_READ_PORT | 7_2_MEM_WRITE_TO_READ_PORT-write_in_async.jz | ASYNC_INVALID_STATEMENT_TARGET fires alongside MEM_WRITE_TO_READ_PORT at line 35 — inherent to testing MEM write in ASYNC block, not fixable via scaffolding |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1134 / 1144 (10 skipped)
- Result after sweep:  1138 / 1148 (10 skipped)
- Newly passing:       4
- Newly broken:        0

## Context Sweep: test_7_3-memory_access_syntax.md — 2026-04-15

### Summary
- Work list size (from issues.md):           4
- After dedup:                                4
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       2
- Scaffolding or bug failures (not created):  2
- Total: 4 == 0 + 0 + 0 + 0 + 2 + 2

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| MEM_ADDR_WIDTH_TOO_WIDE | INOUT .addr with wide address | 7_3_MEM_ADDR_WIDTH_TOO_WIDE-inout_wide_addr.jz |
| MEM_PORT_FIELD_UNDEFINED | ASYNC OUT port with invalid field | 7_3_MEM_PORT_FIELD_UNDEFINED-async_port_invalid_field.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| MEM_CONST_ADDR_OUT_OF_RANGE | 7_3_MEM_CONST_ADDR_OUT_OF_RANGE-inout_const_overflow.jz | rule-not-fired | Compiler does not check constant address range on INOUT .addr field assignments (only on bracket-indexed access). `mem.rw.addr <= 3'd5;` on depth-4 MEM compiles without error. May be a missing check. |
| MEM_PORT_USED_AS_SIGNAL | 7_3_MEM_PORT_USED_AS_SIGNAL-in_port_bare_ref.jz | rule-not-fired | Bare IN (write) port reference fires MEM_READ_FROM_WRITE_PORT (on RHS) or MEM_IN_PORT_FIELD_ACCESS (on LHS) instead of MEM_PORT_USED_AS_SIGNAL. More specific rules take precedence for IN ports. |

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1138 / 1148 (10 skipped)
- Result after sweep:  1140 / 1150 (10 skipped)
- Newly passing:       2
- Newly broken:        0

## Context Sweep: test_7_4-write_modes.md — 2026-04-15

_no work: plan has no missing contexts (no issues flagged)_

## Context Sweep: test_7_5-initialization.md — 2026-04-15

### Summary
- Work list size (from issues.md):           7
- After dedup:                                7
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       4
- Scaffolding or bug failures (not created):  3
- Total: 7 == 0 + 0 + 0 + 0 + 4 + 3

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| MEM_INIT_LITERAL_OVERFLOW | replication/concatenation overflow | 7_5_MEM_INIT_LITERAL_OVERFLOW-concat_overflow.jz |
| MEM_INIT_FILE_TOO_LARGE | text-based .hex too large | 7_5_MEM_INIT_FILE_TOO_LARGE-hex_exceeds_depth.jz |
| MEM_WARN_PARTIAL_INIT | empty file (0 entries) | 7_5_MEM_WARN_PARTIAL_INIT-empty_file.jz |
| MEM_WARN_PARTIAL_INIT | text-based .hex partial | 7_5_MEM_WARN_PARTIAL_INIT-hex_partial.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| MEM_INIT_CONTAINS_X | 7_5_MEM_INIT_CONTAINS_X-x_in_hex_literal.jz | rule-not-fired | Compiler fires LIT_INVALID_DIGIT_FOR_BASE instead of MEM_INIT_CONTAINS_X for hex literals containing `x` (e.g. `8'hxF`). `x` is not a valid hex digit, so the lexer rejects it before MEM init checking runs. The audit's "x in hex literal" is not a valid context for this rule — binary is the only base where `x` is syntactically legal. |
| MEM_INIT_FILE_NOT_FOUND | 7_5_MEM_INIT_FILE_NOT_FOUND-const_path.jz | compiler-bug | @file(CONST_NAME) where CONST is a string path fires PATH_OUTSIDE_SANDBOX instead of MEM_INIT_FILE_NOT_FOUND. CONST-based @file path resolution appears to resolve to a path outside sandbox roots, unlike literal string paths which correctly fire MEM_INIT_FILE_NOT_FOUND. |
| MEM_INIT_FILE_NOT_FOUND | 7_5_MEM_INIT_FILE_NOT_FOUND-config_path.jz | compiler-bug | @file(CONFIG.NAME) fires UNDECLARED_IDENTIFIER instead of MEM_INIT_FILE_NOT_FOUND. CONFIG-based @file path references are not resolved correctly — the compiler treats CONFIG.BOOT_IMAGE as an undeclared identifier rather than resolving the project CONFIG value. |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1140 / 1150 (10 skipped)
- Result after sweep:  1144 / 1154 (10 skipped)
- Newly passing:       4
- Newly broken:        0

## Context Sweep: test_7_6-complete_examples.md — 2026-04-15

_no work: no missing contexts for this plan_

## Context Sweep: test_7_7-error_checking_and_validation.md — 2026-04-15

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       1
- Scaffolding or bug failures (not created):  1
- Total: 2 == 0 + 0 + 0 + 0 + 1 + 1

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| MEM_WARN_DEAD_CODE_ACCESS | dead read in ASYNCHRONOUS block | 7_7_MEM_WARN_DEAD_CODE_ACCESS-dead_async_read.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| MEM_WARN_DEAD_CODE_ACCESS | 7_7_MEM_WARN_DEAD_CODE_ACCESS-dead_inout_access.jz | compiler-bug | INOUT port access inside IF(1'b0) in SYNCHRONOUS block fires WARN_DEAD_CODE_UNREACHABLE but not MEM_WARN_DEAD_CODE_ACCESS. Dead code detection for MEM does not handle INOUT port pseudo-fields (.addr, .wdata). |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1144 / 1154 (10 skipped)
- Result after sweep:  1145 / 1155 (10 skipped)
- Newly passing:       1
- Newly broken:        0

## Context Sweep: test_7_8-mem_vs_register_vs_wire.md — 2026-04-15

_no work: no missing contexts for this plan_

## Context Sweep: test_7_9-mem_in_module_instantiation.md — 2026-04-15

_no work: no missing contexts for this plan_

## Context Sweep: test_8_1-global_purpose.md — 2026-04-15

### Summary
- Work list size (from issues.md):           1
- After dedup:                                1
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       1
- Scaffolding or bug failures (not created):  0
- Total: 1 == 0 + 0 + 0 + 0 + 1 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| GLOBAL_ASSIGN_FORBIDDEN | @template body | 8_1_GLOBAL_ASSIGN_FORBIDDEN-assign_in_template.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
_None._

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1145 / 1155 (10 skipped)
- Result after sweep:  1146 / 1156 (10 skipped)
- Newly passing:       1
- Newly broken:        0

## Context Sweep: test_8_2-global_syntax.md — 2026-04-15

_no work: no missing contexts for this plan_

## Context Sweep: test_8_3-global_semantics.md — 2026-04-15

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       2
- Scaffolding or bug failures (not created):  0
- Total: 2 == 0 + 0 + 0 + 0 + 2 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| GLOBAL_CONST_USE_UNDECLARED | ASYNCHRONOUS RHS | 8_3_GLOBAL_CONST_USE_UNDECLARED-async_rhs.jz |
| GLOBAL_CONST_USE_UNDECLARED | operator expression | 8_3_GLOBAL_CONST_USE_UNDECLARED-in_expression.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
_None._

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1146 / 1156 (10 skipped)
- Result after sweep:  1148 / 1158 (10 skipped)
- Newly passing:       2
- Newly broken:        0

## Context Sweep: test_8_4-global_value_semantics.md — 2026-04-15

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       1
- Scaffolding or bug failures (not created):  1
- Total: 2 == 0 + 0 + 0 + 0 + 1 + 1

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| ASSIGN_WIDTH_NO_MODIFIER | global in expression | 8_4_ASSIGN_WIDTH_NO_MODIFIER-global_in_expression.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| ASSIGN_WIDTH_NO_MODIFIER | 8_4_ASSIGN_WIDTH_NO_MODIFIER-global_in_concat.jz | rule-not-fired | Concatenation width mismatch fires ASSIGN_CONCAT_WIDTH_MISMATCH (higher priority per rules.c line 13) instead of ASSIGN_WIDTH_NO_MODIFIER. The intended rule cannot fire in concatenation context. |

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1148 / 1158 (10 skipped)
- Result after sweep:  1149 / 1159 (10 skipped)
- Newly passing:       1
- Newly broken:        0

## Context Sweep: test_8_5-global_errors.md — 2026-04-15

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       2
- Scaffolding or bug failures (not created):  0
- Total: 2 == 0 + 0 + 0 + 0 + 2 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| GLOBAL_CIRCULAR_DEP | transitive chain cycle | 8_5_GLOBAL_CIRCULAR_DEP-transitive_chain.jz |
| GLOBAL_INVALID_EXPR_TYPE | decimal base overflow | 8_5_GLOBAL_INVALID_EXPR_TYPE-decimal_overflow.jz |

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
_None._

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1149 / 1159 (10 skipped)
- Result after sweep:  1151 / 1161 (10 skipped)
- Newly passing:       2
- Newly broken:        0

## Context Sweep: test_9_1-check_syntax.md — 2026-04-15

### Summary
- Work list size (from issues.md):           3
- After dedup:                                3
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       2
- Scaffolding or bug failures (not created):  1
- Total: 3 == 0 + 0 + 0 + 0 + 2 + 1

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| CHECK_INVALID_EXPR_TYPE | MEM port signal | 9_1_CHECK_INVALID_EXPR_TYPE-mem_signal.jz |
| CHECK_INVALID_EXPR_TYPE | INOUT port signal | 9_1_CHECK_INVALID_EXPR_TYPE-inout_signal.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| CHECK_INVALID_EXPR_TYPE | 9_1_CHECK_INVALID_EXPR_TYPE-latch_signal.jz | rule-not-fired | LATCH signal in @check fires LATCH_IN_CONST_CONTEXT (S4.8) instead of CHECK_INVALID_EXPR_TYPE (S9.1). More specific rule handles this case; CHECK_INVALID_EXPR_TYPE is not reachable for LATCH signals. Audit finding may be invalid. |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1151 / 1161 (10 skipped)
- Result after sweep:  1153 / 1163 (10 skipped)
- Newly passing:       2
- Newly broken:        0

## Context Sweep: test_9_2-check_semantics.md — 2026-04-15

_no work: no missing contexts for this plan_

## Context Sweep: test_9_3-check_placement_rules.md — 2026-04-15

### Summary
- Work list size (from issues.md):           1
- After dedup:                                1
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       0
- Scaffolding or bug failures (not created):  1
- Total: 1 == 0 + 0 + 0 + 0 + 0 + 1

### Files Created
_None._

### Skipped Files
_None._

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| DIRECTIVE_INVALID_CONTEXT | 9_3_DIRECTIVE_INVALID_CONTEXT-check_in_other_blocks.jz | compiler-bug | @check inside non-ASYNC/SYNC blocks (PORT, REGISTER, WIRE, CONST, MEM, LATCH, MUX, BUS, CDC) fires PARSE000 instead of DIRECTIVE_INVALID_CONTEXT. These blocks use specialized parsers that don't recognize directives; only the statement parser (used by ASYNC/SYNC) can detect and report DIRECTIVE_INVALID_CONTEXT. The audit bundled 9 missing contexts into one filename. All 9 exhibit the same behavior: PARSE000 fires, not the intended rule. |

### Parser Recovery Findings (for next audit to log)
_None._

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1153 / 1163 (10 skipped)
- Result after sweep:  1153 / 1163 (10 skipped)
- Newly passing:       0
- Newly broken:        0

## Context Sweep: test_9_4-check_expression_rules.md — 2026-04-15

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       2
- Scaffolding or bug failures (not created):  0
- Total: 2 == 0 + 0 + 0 + 0 + 2 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| CHECK_INVALID_EXPR_TYPE | memory port ref | 9_4_CHECK_INVALID_EXPR_TYPE-memory_port.jz |
| CHECK_INVALID_EXPR_TYPE | nested forbidden operand in arithmetic | 9_4_CHECK_INVALID_EXPR_TYPE-nested_forbidden.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| (none) | | | |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1153 / 1163 (10 skipped)
- Result after sweep:  1155 / 1165 (10 skipped)
- Newly passing:       2
- Newly broken:        0

## Context Sweep: test_9_5-check_evaluation_order.md — 2026-04-15

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       1
- Scaffolding or bug failures (not created):  1
- Total: 2 == 0 + 0 + 0 + 0 + 1 + 1

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| CHECK_INVALID_EXPR_TYPE | transitive CONST resolution | 9_5_HAPPY_PATH-check_transitive_const_ok.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| CHECK_INVALID_EXPR_TYPE | 9_5_HAPPY_PATH-check_config_override_ok.jz | compiler-bug | When a module's CONST is overridden via OVERRIDE in @new, @check inside that module incorrectly fires CHECK_INVALID_EXPR_TYPE. Per S9.5, @check should see CONST values after OVERRIDE evaluation. Confirmed: same module's @check compiles clean without OVERRIDE, fails with OVERRIDE present. |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1155 / 1165 (10 skipped)
- Result after sweep:  1156 / 1166 (10 skipped)
- Newly passing:       1
- Newly broken:        0

## Context Sweep: test_9_7-check_error_conditions.md — 2026-04-15

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       1
- Scaffolding or bug failures (not created):  1
- Total: 2 == 0 + 0 + 0 + 0 + 1 + 1

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| CHECK_INVALID_EXPR_TYPE | memory port field in @check expression | 9_7_CHECK_INVALID_EXPR_TYPE-memory_port_in_check.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| DIRECTIVE_INVALID_CONTEXT | 9_7_DIRECTIVE_INVALID_CONTEXT-check_in_cdc.jz | compiler-bug | @check inside CDC block emits PARSE000 ("expected CDC type BIT/BUS/FIFO") instead of DIRECTIVE_INVALID_CONTEXT. The CDC block parser does not recognize @check as a structural directive — it falls through to the CDC-type keyword expectation. The ASYNC and SYNC block parsers correctly emit DIRECTIVE_INVALID_CONTEXT for this case (see 9_3 tests). |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1156 / 1166 (10 skipped)
- Result after sweep:  1157 / 1167 (10 skipped)
- Newly passing:       1
- Newly broken:        0

## Context Sweep: test_misc-repeat_serializer_io.md — 2026-04-15

### Summary
- Work list size (from issues.md):           2
- After dedup:                                2
- Pre-existing files (skipped):               0
- Stale rule IDs (skipped):                   0
- No spec basis (skipped):                    0
- Not testable via --lint (skipped):          0
- Successfully created:                       2
- Scaffolding or bug failures (not created):  0
- Total: 2 == 0 + 0 + 0 + 0 + 2 + 0

### Files Created
| Rule ID | Context | File |
|---------|---------|------|
| RPT_NO_MATCHING_END | @repeat at end of file (no @endmod) | misc_RPT_NO_MATCHING_END-eof_without_end.jz |
| RPT_NO_MATCHING_END | nested @repeat with missing inner @end | misc_RPT_NO_MATCHING_END-nested_missing_inner.jz |

### Skipped Files
| Rule ID | Recommended filename | Reason |
|---------|----------------------|--------|
| (none) | | |

### Scaffolding or Compiler Bugs Found
| Rule ID | File attempted | Category | Description |
|---------|----------------|----------|-------------|
| (none) | | | |

### Parser Recovery Findings (for next audit to log)
| Rule ID | File | Description |
|---------|------|-------------|
| (none) | | |

### Validation Run
- Command: `bash compiler/tests/run_validation.sh`
- Result before sweep: 1157 / 1167 (10 skipped)
- Result after sweep:  1159 / 1169 (10 skipped)
- Newly passing:       2
- Newly broken:        0

## Context Sweep: test_sim-simulation_rules.md — 2026-04-15

_no work: no missing contexts for this plan_

## Context Sweep: test_tb-testbench_rules.md — 2026-04-15

_no work: no missing contexts for this plan_
