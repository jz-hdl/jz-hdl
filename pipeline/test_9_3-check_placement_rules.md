# Test Plan: 9.3 @check Placement Rules

**Specification Reference:** Section 9.3 of jz-hdl-specification.md

## 1. Objective
Verify that `@check` is accepted at project scope and module scope (between declarations/blocks), and rejected inside ASYNCHRONOUS and SYNCHRONOUS blocks with DIRECTIVE_INVALID_CONTEXT errors. Per the spec, `@check` is a compile-time directive and must not appear inside runtime logic blocks.

## 2. Test Scenarios

### 2.1 Happy Path
1. `@check` at project scope (after CONFIG, before @top) compiles cleanly
2. Multiple `@check` at project scope in sequence
3. `@check` at module scope in helper module (after CONST, before ASYNCHRONOUS)
4. `@check` at module scope in top module (after CONST)
5. `@check` between `@new` and REGISTER declarations (still module scope)
6. `@check` immediately before ASYNCHRONOUS block (still module scope)

### 2.2 Error Cases
1. `@check` inside ASYNCHRONOUS block produces DIRECTIVE_INVALID_CONTEXT (+ PARSE000 follow-on)
2. `@check` inside SYNCHRONOUS block produces DIRECTIVE_INVALID_CONTEXT (+ PARSE000 follow-on)
3. `@check` inside @feature guard body produces CHECK_INVALID_PLACEMENT -- @check may not appear inside conditional or @feature bodies

| 1 | PROJECT_CHIP_DATA_VARIANT_INVALID | Chip JSON clock_gen variants are invalid (parse error, not exhaustive, or not disjoint) | Error: PROJECT_CHIP_DATA_VARIANT_INVALID | PROJECT_CHIP_DATA_VARIANT_INVALID |
| 2 | CLOCK_GEN_VARIANT_NO_MATCH | No chip clock_gen variant matches the elaborated CLOCK_GEN facts (input source / output count) | Error: CLOCK_GEN_VARIANT_NO_MATCH | CLOCK_GEN_VARIANT_NO_MATCH |
| 3 | CLOCK_GEN_VARIANT_AMBIGUOUS | Multiple chip clock_gen variants match the elaborated CLOCK_GEN facts (chip JSON is not disjoint) | Error: CLOCK_GEN_VARIANT_AMBIGUOUS | CLOCK_GEN_VARIANT_AMBIGUOUS |
### 2.3 Edge Cases
1. `@check` immediately before a block definition (valid -- at module scope, not inside block)
2. Valid `@check` at project and module scope coexists with invalid placement -- only invalid triggers

## 3. Input/Output Matrix
| # | Scenario | Triggering Construct | Expected Rule ID | Severity |
|---|----------|---------------------|-----------------|----------|
| 1 | @check inside ASYNCHRONOUS block | `@check (1, "async check");` inside ASYNC | DIRECTIVE_INVALID_CONTEXT | error |
| 2 | @check inside ASYNCHRONOUS block (follow-on) | Parse error after invalid directive | PARSE000 | error |
| 3 | @check inside SYNCHRONOUS block | `@check (1, "sync check");` inside SYNC | DIRECTIVE_INVALID_CONTEXT | error |
| 4 | @check inside SYNCHRONOUS block (follow-on) | Parse error after invalid directive | PARSE000 | error |
| 5 | @check inside @feature guard | `@check (1, "feat check");` inside @feature body | CHECK_INVALID_PLACEMENT | error |
| 6 | PROJECT_CHIP_DATA_VARIANT_INVALID | Chip JSON clock_gen variants are invalid (parse error, not exhaustive, or not disjoint) | PROJECT_CHIP_DATA_VARIANT_INVALID | error |
| 7 | CLOCK_GEN_VARIANT_NO_MATCH | No chip clock_gen variant matches the elaborated CLOCK_GEN facts (input source / output count) | CLOCK_GEN_VARIANT_NO_MATCH | error |
| 8 | CLOCK_GEN_VARIANT_AMBIGUOUS | Multiple chip clock_gen variants match the elaborated CLOCK_GEN facts (chip JSON is not disjoint) | CLOCK_GEN_VARIANT_AMBIGUOUS | error |

## 4. Existing Validation Tests
| Test File | Rule Tested | Triggers |
|-----------|-------------|----------|
| `9_3_HAPPY_PATH-check_placement_ok.jz` | (none -- clean) | 0 diagnostics |
| `9_3_DIRECTIVE_INVALID_CONTEXT-check_in_async.jz` | DIRECTIVE_INVALID_CONTEXT, PARSE000 | 2 diagnostics |
| `9_3_DIRECTIVE_INVALID_CONTEXT-check_in_sync.jz` | DIRECTIVE_INVALID_CONTEXT, PARSE000 | 2 diagnostics |
| *(planned)* `9_3_CHECK_INVALID_PLACEMENT-check_in_feature.jz` | CHECK_INVALID_PLACEMENT | @check inside @feature guard body |

## 5. Rules Matrix

### 5.1 Rules Tested
| Rule ID | Severity | Test Case(s) |
|---------|----------|-------------|
| DIRECTIVE_INVALID_CONTEXT | error | `9_3_DIRECTIVE_INVALID_CONTEXT-check_in_async.jz`, `9_3_DIRECTIVE_INVALID_CONTEXT-check_in_sync.jz` |
| CHECK_INVALID_PLACEMENT | error | S9.3 @check may not appear inside conditional or @feature bodies | 9_3_CHECK_INVALID_PLACEMENT-check_in_feature.jz |
| PROJECT_CHIP_DATA_VARIANT_INVALID | error | S9.3 Chip JSON clock_gen variants are invalid (parse error, not exhaustive, or not disjoint) | (planned) |
| CLOCK_GEN_VARIANT_NO_MATCH | error | S9.3 No chip clock_gen variant matches the elaborated CLOCK_GEN facts (input source / output count) | (planned) |
| CLOCK_GEN_VARIANT_AMBIGUOUS | error | S9.3 Multiple chip clock_gen variants match the elaborated CLOCK_GEN facts (chip JSON is not disjoint) | (planned) |

### 5.2 Rules Not Tested

All rules for this section are tested.

