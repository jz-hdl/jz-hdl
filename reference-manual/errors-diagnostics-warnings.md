---
url: /jz-hdl/reference-manual/errors-diagnostics-warnings.md
---

# Errors and Diagnostics

## Severity Levels

Every diagnostic emitted by the JZ-HDL compiler has one of three severity levels:

| Severity | Meaning | Effect |
| --- | --- | --- |
| **ERROR** | The design violates a language rule and cannot be compiled. | Compilation stops. No output is produced. |
| **WARNING** | The design is legal but likely contains an unintended issue (unused declarations, missing DEFAULT clauses, etc.). | Compilation continues. The design may work, but you should investigate. |
| **INFO** | Advisory information about the design (e.g., runtime division-by-zero risk, blackbox overrides that cannot be validated). | Compilation continues. Informational only — no action required unless the note is relevant to your intent. |

### Controlling Diagnostic Output

| CLI Flag | Effect |
| --- | --- |
| `--info` | Show INFO-level diagnostics in addition to errors and warnings. Without this flag, only errors and warnings are displayed. |
| `--warn-as-error` | Promote all warnings to errors. Useful for CI pipelines where you want zero tolerance for potential issues. |
| `--Wno-group=NAME` | Suppress all diagnostics in the named group. For example, `--Wno-group=GENERAL_WARNINGS` silences unused register warnings, dead code warnings, etc. |
| `--color` | Force colored output. Errors appear in red, warnings in yellow, and info in blue. |

### Listing All Rules

Use `--lint-rules` to print every diagnostic rule ID, its severity, and a short description:

```bash
jz-hdl design.jz --lint-rules
```

This is useful for discovering the exact rule ID to reference when suppressing diagnostics with `--Wno-group`, or when searching this page for details about a specific diagnostic.

## Diagnostic Reference

The sections below list all diagnostic rules organized by category. Each entry shows the rule ID, severity, typical cause, and how to fix it.

## Parse & Lexical

### PARSE.COMMENT\_IN\_TOKEN — Comment appears inside a token

* Severity: ERROR
* Cause: `/*` or `//` started inside an identifier/number/operator token.
* Fix: Move comment boundaries so they appear between tokens or on separate lines.

### PARSE.COMMENT\_NESTED\_BLOCK — Nested block comment detected

* Severity: ERROR
* Cause: `/* ... /* ... */ ... */` — nested block comments are not supported.
* Fix: Remove inner `/*` or close the outer block before starting another; use line comments instead.

### PARSE.DIRECTIVE\_INVALID\_CONTEXT — Directive used in invalid location

* Severity: ERROR
* Cause: Structural directives (@project, @module, @import, @new, @blackbox, @endproj, @endmod, etc.) placed where not allowed.
* Fix: Place directives only in permitted locations per the spec (e.g., @import only immediately inside @project).

### LEXICAL.ID\_SYNTAX\_INVALID — Identifier invalid

* Severity: ERROR
* Cause: Identifier violates regex (bad characters, starts with digit) or length > 255.
* Fix: Rename to valid identifier per rules: \[A‑Za‑z\_]\[A‑Za‑z0‑9\_]{0,254} and not a single underscore.

### LEXICAL.ID\_SINGLE\_UNDERSCORE — Illegal use of single underscore

* Severity: ERROR
* Cause: `_` used as regular identifier outside no‑connect context (allowed only in @top/@new port bindings).
* Fix: Use a named identifier or `_` only in permitted instantiation/top-level no‑connect positions.

### PARSE.KEYWORD\_AS\_IDENTIFIER — Reserved keyword used as identifier

* Severity: ERROR
* Cause: Using a reserved uppercase keyword as an identifier.
* Fix: Rename; reserved keywords (IF, ELSE, CONST, IN, OUT, etc.) cannot be used as identifiers.

### PARSE.IF\_COND\_MISSING\_PARENS — IF/ELIF condition missing parentheses

* Severity: ERROR
* Cause: `IF cond {` instead of `IF (cond) {`.
* Fix: Parenthesize the condition.

### PARSE.LIT\_DECIMAL\_HAS\_XZ — Decimal literal contains x or z

* Severity: ERROR
* Cause: Decimal literals do not support `x`/`z` digits (e.g., `8'd10x`).
* Fix: Use hex or binary base for x/z values.

### PARSE.LIT\_INVALID\_DIGIT\_FOR\_BASE — Invalid digit for literal base

* Severity: ERROR
* Cause: Literal contains a digit not valid for its base (e.g., `8'b102`).
* Fix: Use valid digits for the base.

### PARSE.INSTANCE\_UNDEFINED\_MODULE — Instantiation references non-existent module

* Severity: ERROR
* Fix: Ensure the module is defined or imported before use.

***

## Literals, Types & Widths

### LITERALS\_AND\_TYPES.LIT\_UNSIZED — Unsized literal used

* Severity: ERROR
* Cause: Literal missing width (e.g., `'hFF`).
* Fix: Provide explicit width: `8'hFF`.

### LITERALS\_AND\_TYPES.LIT\_OVERFLOW — Literal intrinsic width > declared width

* Severity: ERROR
* Cause: Value requires more bits than declared.
* Fix: Increase literal width or shorten the value so intrinsic width ≤ declared width.

### LITERALS\_AND\_TYPES.LIT\_UNDERSCORE\_AT\_EDGES — Underscore at start/end of literal

* Severity: ERROR
* Cause: Literal has leading or trailing `_`.
* Fix: Remove underscores at edges.

### LITERALS\_AND\_TYPES.LIT\_UNDEFINED\_CONST\_WIDTH — Undefined CONST used as width

* Severity: ERROR
* Cause: Using a module CONST name in a literal width that does not exist.
* Fix: Declare the CONST or use a numeric width; CONST scope is module-local.

### WIDTHS\_AND\_SLICING.WIDTH\_NONPOSITIVE\_OR\_NONINT — Invalid declared width

* Severity: ERROR
* Cause: Declared width <= 0 or not integer.
* Fix: Use positive integer or valid CONST resolving to positive integer.

### WIDTHS\_AND\_SLICING.SLICE\_MSB\_LESS\_THAN\_LSB — Slice MSB < LSB

* Severity: ERROR
* Cause: `signal[H:L]` where H < L.
* Fix: Reverse indices or correct indices so MSB ≥ LSB.

### WIDTHS\_AND\_SLICING.SLICE\_INDEX\_OUT\_OF\_RANGE — Slice index out of bounds

* Severity: ERROR
* Cause: Index < 0 or >= signal width.
* Fix: Use valid indices within \[0, width−1].

### OPERATORS\_AND\_EXPRESSIONS.TYPE\_BINOP\_WIDTH\_MISMATCH — Binary op operands differ widths

* Severity: ERROR
* Cause: Operators requiring equal widths (e.g., `+`, `&`, `==`) receive different widths.
* Fix: Make operand widths equal via explicit extension (`=z`/`=s` context) or resize using concatenation or intrinsics.

***

## Operators & Expressions

### OPERATORS\_AND\_EXPRESSIONS.UNARY\_ARITH\_MISSING\_PARENS — Unary arithmetic missing parentheses

* Severity: ERROR
* Cause: Used `-flag` instead of `(-flag)`.
* Fix: Parenthesize unary arithmetic: `(-flag)`.

### OPERATORS\_AND\_EXPRESSIONS.LOGICAL\_WIDTH\_NOT\_1 — `&&`/`||`/`!` used on multi-bit

* Severity: ERROR
* Cause: Logical operators require width‑1 operands.
* Fix: Reduce to a single-bit condition (e.g., `(a != 0)`), or use bitwise operators.

### OPERATORS\_AND\_EXPRESSIONS.TERNARY\_COND\_WIDTH\_NOT\_1 — Ternary condition not width‑1

* Severity: ERROR
* Fix: Ensure the condition expression is width‑1.

### OPERATORS\_AND\_EXPRESSIONS.TERNARY\_BRANCH\_WIDTH\_MISMATCH — Branches widths differ

* Severity: ERROR
* Cause: true/false expressions have different widths.
* Fix: Make branches equal width by extending or slicing.

### OPERATORS\_AND\_EXPRESSIONS.CONCAT\_EMPTY — Empty concatenation `{}` used

* Severity: ERROR
* Fix: Remove empty concat or supply elements.

### OPERATORS\_AND\_EXPRESSIONS.DIV\_CONST\_ZERO — Division by constant zero

* Severity: ERROR
* Cause: A compile-time divisor literal = 0.
* Fix: Remove or guard division; use conditional checks.

### OPERATORS\_AND\_EXPRESSIONS.DIV\_UNGUARDED\_RUNTIME\_ZERO — Divisor may be zero at runtime

* Severity: WARNING
* Cause: A division or modulus with a non-constant divisor is not enclosed in an `IF (divisor != 0)` guard.
* Fix: Add runtime guards: `IF (divisor != 0) ... ELSE ...`.

### OPERATORS\_AND\_EXPRESSIONS.SPECIAL\_DRIVER\_IN\_EXPRESSION — GND/VCC in expression

* Severity: ERROR
* Cause: GND/VCC may not appear in arithmetic/logical expressions.
* Fix: Use sized literals (`1'b0`, `1'b1`) instead.

### OPERATORS\_AND\_EXPRESSIONS.SPECIAL\_DRIVER\_IN\_CONCAT — GND/VCC in concatenation

* Severity: ERROR
* Fix: Use sized literals instead of GND/VCC in concatenations.

### OPERATORS\_AND\_EXPRESSIONS.SPECIAL\_DRIVER\_SLICED — GND/VCC sliced or indexed

* Severity: ERROR
* Fix: GND/VCC are whole-signal drivers; use sized literals if you need specific bits.

### OPERATORS\_AND\_EXPRESSIONS.SPECIAL\_DRIVER\_IN\_INDEX — GND/VCC in index expression

* Severity: ERROR
* Fix: Use integer literals for indices.

### OPERATORS\_AND\_EXPRESSIONS.SBIT\_SET\_WIDTH\_NOT\_1 — sbit() set argument not width-1

* Severity: ERROR
* Fix: The third argument to sbit() must be a 1-bit expression.

### OPERATORS\_AND\_EXPRESSIONS.GBIT\_INDEX\_OUT\_OF\_RANGE — gbit() index out of range

* Severity: ERROR
* Fix: Ensure index is within \[0, width(source)-1].

### OPERATORS\_AND\_EXPRESSIONS.GSLICE\_INDEX\_OUT\_OF\_RANGE — gslice() index out of range

* Severity: ERROR
* Fix: Ensure index + width <= width(source).

### OPERATORS\_AND\_EXPRESSIONS.GSLICE\_WIDTH\_INVALID — gslice() width invalid

* Severity: ERROR
* Fix: Width parameter must be a positive integer constant.

### OPERATORS\_AND\_EXPRESSIONS.SSLICE\_INDEX\_OUT\_OF\_RANGE — sslice() index out of range

* Severity: ERROR
* Fix: Ensure index + width <= width(source).

### OPERATORS\_AND\_EXPRESSIONS.SSLICE\_WIDTH\_INVALID — sslice() width invalid

* Severity: ERROR
* Fix: Width parameter must be a positive integer constant.

### OPERATORS\_AND\_EXPRESSIONS.SSLICE\_VALUE\_WIDTH\_MISMATCH — sslice() value width mismatch

* Severity: ERROR
* Fix: Value argument width must match the width parameter.

### OPERATORS\_AND\_EXPRESSIONS.OH2B\_INPUT\_TOO\_NARROW — oh2b() source too narrow

* Severity: ERROR
* Fix: oh2b() source must be at least 2 bits wide.

### OPERATORS\_AND\_EXPRESSIONS.B2OH\_WIDTH\_INVALID — b2oh() width invalid

* Severity: ERROR
* Fix: Width must be a positive constant >= 2.

### OPERATORS\_AND\_EXPRESSIONS.PRIENC\_INPUT\_TOO\_NARROW — prienc() source too narrow

* Severity: ERROR
* Fix: prienc() source must be at least 2 bits wide.

### OPERATORS\_AND\_EXPRESSIONS.BSWAP\_WIDTH\_NOT\_BYTE\_ALIGNED — bswap() width not byte-aligned

* Severity: ERROR
* Fix: Source width must be a multiple of 8.

***

## Identifiers & Scope

### IDENTIFIERS\_AND\_SCOPE.ID\_DUP\_IN\_MODULE — Duplicate identifier in module

* Severity: ERROR
* Cause: Two declarations with same name (ports, wires, registers, consts, instances).
* Fix: Rename one declaration; ensure uniqueness in module scope.

### IDENTIFIERS\_AND\_SCOPE.MODULE\_NAME\_DUP\_IN\_PROJECT — Module name not unique project-wide

* Severity: ERROR
* Fix: Rename module or resolve import duplication.

### IDENTIFIERS\_AND\_SCOPE.INSTANCE\_NAME\_DUP\_IN\_MODULE — Duplicate instance name

* Severity: ERROR
* Fix: Use unique instance names.

### IDENTIFIERS\_AND\_SCOPE.INSTANCE\_NAME\_CONFLICT — Instance name conflicts other identifier

* Severity: ERROR
* Fix: Rename instance so it doesn't collide with port/wire/register/CONST.

### IDENTIFIERS\_AND\_SCOPE.UNDECLARED\_IDENTIFIER — Undeclared name used

* Severity: ERROR
* Fix: Declare the identifier or correct the name.

### IDENTIFIERS\_AND\_SCOPE.AMBIGUOUS\_REFERENCE — Ambiguous reference

* Severity: ERROR
* Cause: Name could refer to multiple things without instance/qualification.
* Fix: Qualify with `instance.port` or rename conflicting identifiers.

***

## CONST / CONFIG / GLOBAL

### CONST\_RULES.CONST\_NEGATIVE\_OR\_NONINT — CONST invalid value

* Severity: ERROR
* Fix: Initialize CONST with nonnegative integer.

### CONFIG\_BLOCK.CONFIG\_MULTIPLE\_BLOCKS — Multiple CONFIG blocks

* Severity: ERROR
* Fix: Merge into single CONFIG block.

### CONFIG\_BLOCK.CONFIG\_FORWARD\_REF — CONFIG forward reference

* Severity: ERROR
* Cause: CONFIG entry references a later CONFIG entry.
* Fix: Reorder entries or use computed constants appropriately.

### CONFIG\_BLOCK.CONFIG\_USED\_WHERE\_FORBIDDEN / CONST\_USED\_WHERE\_FORBIDDEN

* Severity: ERROR
* Cause: CONFIG/CONST used at runtime (in ASYNCHRONOUS/SYNCHRONOUS expressions).
* Fix: Use @global sized literals for runtime values; CONFIG/CONST only for compile-time contexts.

### CONFIG\_BLOCK.CONST\_STRING\_IN\_NUMERIC\_CONTEXT — String CONST/CONFIG in numeric context

* Severity: ERROR
* Cause: A string-valued CONST or CONFIG entry is used where a numeric expression is expected (e.g., in a width bracket `[STR_CONST]` or MEM depth).
* Fix: Use a numeric CONST/CONFIG for integer contexts. String values are only valid in `@file()` path arguments.

### CONFIG\_BLOCK.CONST\_NUMERIC\_IN\_STRING\_CONTEXT — Numeric CONST/CONFIG in string context

* Severity: ERROR
* Cause: A numeric CONST or CONFIG entry is used where a string is expected (e.g., `@file(NUM_CONST)`).
* Fix: Use a string CONST/CONFIG for `@file()` paths (e.g., `INIT_FILE = "data.bin";`).

### GLOBAL\_BLOCK.GLOBAL\_INVALID\_EXPR\_TYPE — Global constant must be sized literal

* Severity: ERROR
* Fix: Define global constants as sized literals (e.g., `8'hFF`).

***

## Ports, Wires & Registers

### PORT\_WIRE\_REGISTER\_DECLS.PORT\_MISSING\_WIDTH — Port declared without width

* Severity: ERROR
* Fix: Add mandatory `[N]` width.

### PORT\_WIRE\_REGISTER\_DECLS.PORT\_DIRECTION\_MISMATCH\_IN — Assigning to IN port

* Severity: ERROR
* Cause: Writing to an `IN` port outside allowed INOUT tri‑state patterns.
* Fix: Only read `IN` ports; if bidirectional behavior is needed use `INOUT` and tri‑state logic.

### PORT\_WIRE\_REGISTER\_DECLS.PORT\_DIRECTION\_MISMATCH\_OUT — Reading from OUT port

* Severity: ERROR
* Cause: Reading an `OUT` port as if it were a driver.
* Fix: Do not read module `OUT` ports inside the same module; use internal regs/wires for observation.

### PORT\_WIRE\_REGISTER\_DECLS.PORT\_TRISTATE\_MISMATCH — `z` assigned to non-INOUT

* Severity: ERROR
* Fix: Only assign `z` to INOUT ports.

### PORT\_WIRE\_REGISTER\_DECLS.WIRE\_MULTI\_DIMENSIONAL / REG\_MULTI\_DIMENSIONAL — Multi-dimensional illegal

* Severity: ERROR
* Fix: Use MEM for arrays; declare WIRE/REGISTER as single-dimensional scalars.

### PORT\_WIRE\_REGISTER\_DECLS.REG\_MISSING\_INIT\_LITERAL — Register missing reset literal

* Severity: ERROR
* Fix: Provide a sized literal reset value (no `x` bits).

### PORT\_WIRE\_REGISTER\_DECLS.REG\_INIT\_CONTAINS\_X — Register reset contains `x`

* Severity: ERROR
* Fix: Use known 0/1 bits for reset; `x` not permitted.

### PORT\_WIRE\_REGISTER\_DECLS.REG\_INIT\_CONTAINS\_Z — Register reset contains `z`

* Severity: ERROR
* Fix: Use known 0/1 bits for reset; `z` not permitted.

### PORT\_WIRE\_REGISTER\_DECLS.REG\_INIT\_WIDTH\_MISMATCH — Register init width mismatch

* Severity: ERROR
* Fix: Init literal width must match declared register width.

### PORT\_WIRE\_REGISTER\_DECLS.WRITE\_WIRE\_IN\_SYNC — Writing wire in SYNCHRONOUS

* Severity: ERROR
* Fix: Writes to wires must occur in ASYNCHRONOUS; registers are updated in SYNCHRONOUS blocks.

### PORT\_WIRE\_REGISTER\_DECLS.ASSIGN\_TO\_NON\_REGISTER\_IN\_SYNC — LHS not a register in sync assignment

* Severity: ERROR
* Fix: Use a REGISTER on LHS for synchronous assignments.

### PORT\_WIRE\_REGISTER\_DECLS.MODULE\_MISSING\_PORT / MODULE\_PORT\_IN\_ONLY

* Severity: ERROR / WARN
* Fix: Ensure module declares PORT block and includes outputs if expected; a module with only IN ports may be dead code.

***

## Modules & Instantiation

### MODULE\_AND\_INSTANTIATION.INSTANCE\_MISSING\_PORT — Not all child ports listed in @new

* Severity: ERROR
* Fix: Include all child ports in the `@new` instantiation block.

### MODULE\_AND\_INSTANTIATION.INSTANCE\_PORT\_WIDTH\_MISMATCH — Port width mismatch

* Severity: ERROR
* Fix: Ensure evaluated widths (after OVERRIDE) match between parent binding and child port.

### MODULE\_AND\_INSTANTIATION.INSTANCE\_PORT\_DIRECTION\_MISMATCH — Direction mismatch

* Severity: ERROR
* Fix: Connect IN ports to drivers, OUT ports to sinks or parent signals with correct roles; respect pin category.

### MODULE\_AND\_INSTANTIATION.INSTANCE\_OVERRIDE\_CONST\_UNDEFINED — Override targets unknown

* Severity: ERROR
* Fix: Only override CONST names that exist in the child module.

### MODULE\_AND\_INSTANTIATION.INSTANCE\_PARENT\_SIGNAL\_WIDTH\_MISMATCH — Parent signal width wrong

* Severity: ERROR
* Fix: Ensure parent signal has the width declared in instantiation clause (check parent's CONST resolution).

***

## MUX Rules

### MUX\_RULES.MUX\_ASSIGN\_LHS — Assigning to MUX is forbidden

* Severity: ERROR
* Fix: Treat MUX only as read-only source; create a wire/register to hold selected data if you must drive it.

### MUX\_RULES.MUX\_AGG\_SOURCE\_WIDTH\_MISMATCH — Aggregation widths differ

* Severity: ERROR
* Fix: Make all aggregated sources identical width before declaring the MUX.

### MUX\_RULES.MUX\_SLICE\_WIDTH\_NOT\_DIVISOR — Auto-slicing invalid partition

* Severity: ERROR
* Fix: Choose element\_width that exactly divides wide\_source width.

### MUX\_RULES.MUX\_SELECTOR\_OUT\_OF\_RANGE\_CONST — Selector statically out of range

* Severity: ERROR
* Fix: Correct selector width or bounds; ensure compile-time indices fall within valid range.

***

## Assignments & Exclusive Assignment Rule

### ASSIGNMENTS\_AND\_EXCLUSIVE.ASSIGN\_WIDTH\_NO\_MODIFIER — Width mismatch without z/s

* Severity: ERROR
* Cause: `=`, `=>`, `<=` used when widths differ (modifier required).
* Fix: Use `=z`/`=s`, `=>z`/`=>s`, or resize operands so widths match.

### ASSIGNMENTS\_AND\_EXCLUSIVE.ASSIGN\_TRUNCATES — Assignment would truncate RHS

* Severity: ERROR
* Fix: Prevent truncation by slicing explicitly or making LHS wide enough; avoid implicit truncation.

### ASSIGNMENTS\_AND\_EXCLUSIVE.ASSIGN\_SLICE\_WIDTH\_MISMATCH — Slice width mismatch

* Severity: ERROR
* Fix: Ensure source expression width equals the slice width.

### ASSIGNMENTS\_AND\_EXCLUSIVE.ASSIGN\_CONCAT\_WIDTH\_MISMATCH — Concatenation width mismatch

* Severity: ERROR
* Fix: Make expression width equal sum of LHS part widths (or use extension modifiers if allowed).

### ASSIGNMENTS\_AND\_EXCLUSIVE.ASSIGN\_MULTIPLE\_SAME\_BITS — Exclusive assignment violation

* Severity: ERROR
* Cause: More than one assignment to same bits along some execution path.
* Fix: Rework control flow so assignments are in mutually exclusive branches or merge into a single assignment using ternary/select.

### ASSIGNMENTS\_AND\_EXCLUSIVE.ASSIGN\_INDEPENDENT\_IF\_SELECT — Independent chains conflict

* Severity: ERROR
* Cause: Separate IF chains at same nesting level each assign same identifier (compiler treats as concurrent).
* Fix: Combine into single SELECT/IF/ELIF chain or restructure to make assignments exclusive.

### ASSIGNMENTS\_AND\_EXCLUSIVE.ASSIGN\_SHADOWING — Shadowing assignment (root + nested)

* Severity: ERROR
* Fix: Avoid assigning at root and again in nested blocks to same bits; use conditional assignments consistently.

### ASSIGNMENTS\_AND\_EXCLUSIVE.ASSIGN\_SLICE\_OVERLAP — Overlapping part-selects assigned in same path

* Severity: ERROR
* Fix: Ensure part-selects are non-overlapping on any execution path or combine into single concatenation assignment.

### ASSIGNMENTS\_AND\_EXCLUSIVE.ASYNC\_UNDEFINED\_PATH\_NO\_DRIVER — ASYNC leaves net without driver

* Severity: ERROR
* Cause: Conditional ASYNCHRONOUS assignment path leaves a wire/port undriven (no ELSE) and no other driver exists.
* Fix: Provide DEFAULT branch or alternate driver; ensure every execution path that reads the net has a driver.

***

## ASYNCHRONOUS Block Rules

### ASYNC\_BLOCK\_RULES.ASYNC\_INVALID\_STATEMENT\_TARGET — LHS not assignable

* Severity: ERROR
* Fix: LHS must be assignable (wire, port, register current-value read in ASYNC can't be written there).

### ASYNC\_BLOCK\_RULES.ASYNC\_ASSIGN\_REGISTER — Register written in ASYNCHRONOUS

* Severity: ERROR
* Fix: Move register writes into SYNCHRONOUS blocks.

### ASYNC\_BLOCK\_RULES.ASYNC\_ALIAS\_LITERAL\_RHS — Alias with literal RHS in ASYNC

* Severity: ERROR
* Cause: Using `a = 1'b1;` in ASYNCHRONOUS is forbidden (alias RHS must not be bare literal).
* Fix: Use directional drive `a <= 1'b1;` or `a => 1'b1;` instead.

### ASYNC\_BLOCK\_RULES.ASYNC\_FLOATING\_Z\_READ — All drivers `z` on read

* Severity: ERROR
* Fix: Ensure at least one driver supplies 0/1 for read conditions or avoid reading when bus released.

***

## SYNCHRONOUS Block Rules

### SYNC\_BLOCK\_RULES.SYNC\_MULTI\_ASSIGN\_SAME\_REG\_BITS — Multiple reg assignments in path

* Severity: ERROR
* Fix: Ensure each register bit assigned at most once per path; move assignments into mutually exclusive branches or combine.

### SYNC\_BLOCK\_RULES.SYNC\_ROOT\_AND\_CONDITIONAL\_ASSIGN — Root + conditional conflict

* Severity: ERROR
* Fix: Do not assign same register at root and inside nested condition; place all domain assignments within consistent conditional structure.

### SYNC\_BLOCK\_RULES.SYNC\_SLICE\_WIDTH\_MISMATCH — Register slice width mismatch

* Severity: ERROR
* Fix: Ensure RHS width equals slice width.

### SYNC\_BLOCK\_RULES.SYNC\_CONCAT\_DUP\_REG — Concatenation contains same register multiple times

* Severity: ERROR
* Fix: Use unique registers in concatenation LHS.

### SYNC\_BLOCK\_RULES.SYNC\_NO\_ALIAS — Aliasing not allowed in synchronous blocks

* Severity: ERROR
* Fix: Use directional operators (`<=`, `=>`) only; alias operators are forbidden in SYNCHRONOUS.

### SYNC\_BLOCK\_RULES.DOMAIN\_CONFLICT / MULTI\_CLK\_ASSIGN / DUPLICATE\_BLOCK — Clock/CDC domain errors

* Severity: ERROR
* Cause: Register read/write used in block whose `CLK` doesn't match home domain; same register assigned in different clock blocks; duplicate synchronous block for a clock.
* Fix: Respect Register locality/home-domain rules; use CDC entries to cross domains; consolidate per-clock logic into single SYNCHRONOUS block.

### SYNC\_BLOCK\_RULES.CDC\_SOURCE\_NOT\_REGISTER / CDC\_BIT\_WIDTH\_NOT\_1

* Severity: ERROR
* Fix: CDC sources must be `REGISTER`; BIT type registers must have width 1.

### SYNC\_BLOCK\_RULES.SYNC\_CLK\_WIDTH\_NOT\_1 — Clock signal not width-1

* Severity: ERROR
* Fix: SYNCHRONOUS CLK signal must have width \[1].

### SYNC\_BLOCK\_RULES.SYNC\_MISSING\_CLK — Missing CLK parameter

* Severity: ERROR
* Fix: SYNCHRONOUS block must declare a CLK parameter.

### SYNC\_BLOCK\_RULES.SYNC\_EDGE\_INVALID — Invalid edge value

* Severity: ERROR
* Fix: EDGE must be Rising, Falling, or Both.

### SYNC\_BLOCK\_RULES.SYNC\_RESET\_ACTIVE\_INVALID — Invalid RESET\_ACTIVE

* Severity: ERROR
* Fix: RESET\_ACTIVE must be High or Low.

### SYNC\_BLOCK\_RULES.SYNC\_RESET\_TYPE\_INVALID — Invalid RESET\_TYPE

* Severity: ERROR
* Fix: RESET\_TYPE must be Clocked or Immediate.

### SYNC\_BLOCK\_RULES.SYNC\_RESET\_WIDTH\_NOT\_1 — Reset signal not width-1

* Severity: ERROR
* Fix: RESET signal must have width \[1].

### SYNC\_BLOCK\_RULES.SYNC\_UNKNOWN\_PARAM — Unknown SYNCHRONOUS parameter

* Severity: ERROR
* Fix: Valid parameters are CLK, RESET, EDGE, RESET\_ACTIVE, RESET\_TYPE.

### SYNC\_BLOCK\_RULES.SYNC\_EDGE\_BOTH\_WARNING — Dual-edge clocking warning

* Severity: WARN
* Cause: EDGE=Both may not be supported by all FPGA architectures.
* Fix: Use Rising or Falling unless dual-edge is intentional.

### SYNC\_BLOCK\_RULES.CDC\_SOURCE\_NOT\_PLAIN\_REG / CDC\_DEST\_ALIAS\_ASSIGNED / CDC\_STAGES\_INVALID / CDC\_TYPE\_INVALID / CDC\_RAW\_STAGES\_FORBIDDEN / CDC\_PULSE\_WIDTH\_NOT\_1 / CDC\_DEST\_ALIAS\_DUP

* Severity: ERROR
* Fix: CDC source must be a plain register identifier (not slice/expression). Destination alias is read-only. Stages must be positive. Type must be BIT/BUS/FIFO/HANDSHAKE/PULSE/MCP/RAW. RAW has no stages parameter. PULSE source must be width 1. Dest alias name must be unique.

***

## Control Flow (IF / SELECT)

### CONTROL\_FLOW\_IF\_SELECT.IF\_COND\_WIDTH\_NOT\_1 — IF/ELIF condition width ≠ 1

* Severity: ERROR
* Fix: Ensure condition expression reduces to 1 bit (e.g., compare equality).

### CONTROL\_FLOW\_IF\_SELECT.CONTROL\_FLOW\_OUTSIDE\_BLOCK — IF/SELECT used outside block

* Severity: ERROR
* Fix: Place control flow inside ASYNCHRONOUS or SYNCHRONOUS blocks.

### CONTROL\_FLOW\_IF\_SELECT.SELECT\_DUP\_CASE\_VALUE — Duplicate CASE label

* Severity: ERROR
* Fix: Remove or collapse duplicate CASE labels.

### CONTROL\_FLOW\_IF\_SELECT.ASYNC\_ALIAS\_IN\_CONDITIONAL — Alias operator inside conditional

* Severity: ERROR
* Fix: Use conditional expressions or directional assignments; aliasing is for unconditional net merges only.

### CONTROL\_FLOW\_IF\_SELECT.SELECT\_DEFAULT\_RECOMMENDED\_ASYNC — Missing DEFAULT in ASYNCHRONOUS SELECT

* Severity: WARN
* Cause: May leave nets floating for some selector values.
* Fix: Add DEFAULT clause and specify safe values.

***

## Functions & Compile-time Utilities

### FUNCTIONS\_AND\_CLOG2.CLOG2\_NONPOSITIVE\_ARG — clog2 arg ≤ 0

* Severity: ERROR
* Fix: Provide positive integer argument.

### FUNCTIONS\_AND\_CLOG2.CLOG2\_INVALID\_CONTEXT — clog2 used at runtime

* Severity: ERROR
* Fix: Use clog2 only in compile-time contexts (widths, CONST initializers, OVERRIDE). For runtime indexing use other constructs.

### FUNCTIONS\_AND\_CLOG2.WIDTHOF\_INVALID\_CONTEXT — widthof used at runtime

* Severity: ERROR
* Fix: Use widthof only in compile-time contexts.

### FUNCTIONS\_AND\_CLOG2.FUNC\_RESULT\_TRUNCATED\_SILENTLY — Function result truncated by assignment

* Severity: ERROR
* Fix: Check sizes of intrinsic operator results and assign to matching width or slice explicitly.

***

## Net Drivers, Tri‑State & Observability

### NET\_DRIVERS\_AND\_TRI\_STATE.NET\_FLOATING\_WITH\_SINK — Net has sinks but zero active drivers

* Severity: ERROR
* Fix: Provide a driver on all execution paths that read the net (or avoid reading when released).

### NET\_DRIVERS\_AND\_TRI\_STATE.NET\_TRI\_STATE\_ALL\_Z\_READ — All drivers `z` while read

* Severity: ERROR
* Fix: Ensure at least one driver provides known 0/1 at the moment of any read.

### NET\_DRIVERS\_AND\_TRI\_STATE.NET\_DANGLING\_UNUSED — Net declared with no drivers and no sinks

* Severity: WARN
* Fix: Remove unused declaration or add intended drivers/sinks.

### OBSERVABILITY\_X.OBS\_X\_TO\_OBSERVABLE\_SINK — x bits reach observable sink

* Severity: ERROR
* Cause: Expression containing `x` used where its `x` bits could reach REGISTER init, MEM init, OUT/INOUT port, or top-level pin.
* Fix: Mask structurally (not algebraically) before observable sinks (e.g., use conditional logic to drive defined values), or avoid `x` in those values.

***

## Combinational Loops

### COMBINATIONAL\_LOOPS.COMB\_LOOP\_UNCONDITIONAL — Flow-sensitive comb loop (error)

* Severity: ERROR
* Cause: Unconditional cycle in ASYNCHRONOUS assignments (e.g., a = b; b = a;).
* Fix: Break cycle by introducing registers, restructuring logic, or making assignments mutually exclusive.

### COMBINATIONAL\_LOOPS.COMB\_LOOP\_CONDITIONAL\_SAFE — Cycle within mutually exclusive branches

* Severity: WARN (informational, considered safe)
* Note: Flow-sensitive analysis treats mutually exclusive branches as safe; verify intended behavior.

***

## Project, Imports & Top-level

### PROJECT\_AND\_IMPORTS.PROJECT\_MULTIPLE\_PER\_FILE — Multiple @project

* Severity: ERROR
* Fix: Use a single @project per file.

### PROJECT\_AND\_IMPORTS.IMPORT\_FILE\_MULTIPLE\_TIMES — Same import repeated

* Severity: ERROR
* Fix: Remove duplicate imports or normalize import graph to avoid re-importing same file.

### PROJECT\_AND\_IMPORTS.IMPORT\_NOT\_AT\_PROJECT\_TOP — @import in wrong place

* Severity: ERROR
* Fix: Move `@import` to immediately after `@project` header, before CONFIG/CLOCKS/PIN/blackbox/top.

### TOP\_LEVEL\_INSTANTIATION.TOP\_PORT\_NOT\_LISTED — Top-file misses a port

* Severity: ERROR
* Fix: List all module ports in `@top` mapping; specify `_` if intentionally unconnected.

### CLOCKS\_PINS\_MAP.\* — Various clock/pin/map errors

* Severity: ERROR/WARN depending on item (CLOCK\_PORT\_WIDTH\_NOT\_1, MAP\_PIN\_DECLARED\_NOT\_MAPPED, MAP\_DUP\_PHYSICAL\_LOCATION, etc.)
* Fix: Ensure CLOCKS entries match IN\_PINS (width 1), map every declared pin in MAP, avoid mapping two logical pins to same physical pin unless tri-state intended.

### CLOCKS\_PINS\_MAP.PIN\_MODE\_INVALID — Invalid pin mode value

* Severity: ERROR
* Cause: `mode` attribute is not `SINGLE` or `DIFFERENTIAL`.
* Fix: Use `mode=SINGLE` or `mode=DIFFERENTIAL`.

### CLOCKS\_PINS\_MAP.PIN\_MODE\_STANDARD\_MISMATCH — Pin mode conflicts with I/O standard

* Severity: ERROR
* Cause: `mode=DIFFERENTIAL` used with a single-ended standard (e.g., LVCMOS33), or `mode=SINGLE` used with a differential standard (e.g., LVDS25).
* Fix: Use a standard that matches the pin mode. Single-ended standards (LVCMOS, LVTTL, PCI33, SSTL, HSTL) require `mode=SINGLE`; differential standards (LVDS, TMDS, BLVDS, DIFF\_SSTL, etc.) require `mode=DIFFERENTIAL`.

### CLOCKS\_PINS\_MAP.PIN\_PULL\_INVALID — Invalid pull value

* Severity: ERROR
* Cause: `pull` attribute is not `UP`, `DOWN`, or `NONE`.
* Fix: Use `pull=UP`, `pull=DOWN`, or `pull=NONE`.

### CLOCKS\_PINS\_MAP.PIN\_PULL\_ON\_OUTPUT — Pull resistor on output-only pin

* Severity: ERROR
* Cause: `pull=UP` or `pull=DOWN` specified on an `OUT_PINS` entry.
* Fix: Remove pull attribute from output pins. Pull resistors are only meaningful on input or bidirectional pins.

### CLOCKS\_PINS\_MAP.PIN\_TERM\_INVALID — Invalid termination value

* Severity: ERROR
* Cause: `term` attribute is not `ON` or `OFF`.
* Fix: Use `term=ON` or `term=OFF`.

### CLOCKS\_PINS\_MAP.PIN\_TERM\_INVALID\_FOR\_STANDARD — Termination not supported for standard

* Severity: ERROR
* Cause: `term=ON` used with a standard that does not support on-die termination (e.g., LVCMOS33, LVTTL, PCI33).
* Fix: Termination is only valid for `mode=DIFFERENTIAL` pins or for single-ended SSTL/HSTL standards. Remove `term=ON` or change to a termination-capable standard.

### CLOCKS\_PINS\_MAP.MAP\_DIFF\_EXPECTED\_PAIR — Differential pin needs P, N MAP syntax

* Severity: ERROR
* Cause: A pin declared with `mode=DIFFERENTIAL` is mapped with a scalar value instead of the `{ P=pin, N=pin }` pair syntax.
* Fix: Use differential MAP syntax: `pin = { P=33, N=34 };`

### CLOCKS\_PINS\_MAP.MAP\_SINGLE\_UNEXPECTED\_PAIR — Single-ended pin must not use P, N pair

* Severity: ERROR
* Cause: A single-ended pin is mapped with `{ P=pin, N=pin }` pair syntax instead of a scalar value.
* Fix: Use scalar MAP syntax: `pin = 33;`

### CLOCKS\_PINS\_MAP.MAP\_DIFF\_MISSING\_PN — Differential MAP missing P or N

* Severity: ERROR
* Cause: Differential MAP entry is missing the `P` or `N` identifier.
* Fix: Provide both P and N: `pin = { P=33, N=34 };`

### CLOCKS\_PINS\_MAP.MAP\_DIFF\_SAME\_PIN — P and N map to same physical pin

* Severity: ERROR
* Cause: Both P and N in a differential MAP entry refer to the same physical pin.
* Fix: Use different physical pins for P (positive) and N (negative).

***

## Blackboxes & Top-level

### BLACKBOX\_RULES.BLACKBOX\_BODY\_DISALLOWED — Blackbox contains body

* Severity: ERROR
* Fix: Blackboxes must declare only PORT (and optional CONST) — no wires/registers/blocks.

### BLACKBOX\_RULES.BLACKBOX\_UNDEFINED\_IN\_NEW — Unknown blackbox instantiation

* Severity: ERROR
* Fix: Declare the blackbox in the project before instantiation.

### BLACKBOX\_RULES.BLACKBOX\_OVERRIDE\_UNCHECKED — OVERRIDE unvalidated

* Severity: INFO
* Note: OVERRIDE values are passed through to vendor IP; tool cannot validate them.

***

## MEM (Memory) — Declaration & Access

### MEM\_DECLARATION.MEM\_UNDEFINED\_NAME — Memory not declared

* Severity: ERROR
* Fix: Declare MEM block before using `mem.port[addr]` access.

### MEM\_DECLARATION.MEM\_INVALID\_WORD\_WIDTH / MEM\_INVALID\_DEPTH — Word width or depth invalid

* Severity: ERROR
* Fix: Use positive integer or valid CONST resolving to positive integer.

### MEM\_DECLARATION.MEM\_INIT\_CONTAINS\_X / MEM\_INIT\_LITERAL\_OVERFLOW / MEM\_INIT\_FILE\_TOO\_LARGE — Init problems

* Severity: ERROR
* Fix: Ensure MEM init literal contains only 0/1 (no `x`), fits word width; ensure init file length ≤ depth × word\_width.

### MEM\_DECLARATION.MEM\_EMPTY\_PORT\_LIST — MEM without ports

* Severity: ERROR
* Fix: Declare at least one OUT or IN port.

### MEM\_ACCESS.MEM\_PORT\_UNDEFINED — Port name not declared in MEM

* Severity: ERROR
* Fix: Use declared MEM port names.

### MEM\_ACCESS.MEM\_READ\_SYNC\_WITH\_EQUALS — Sync read used with `=` instead of `<=`

* Severity: ERROR
* Fix: Use `<=` in SYNCHRONOUS block for synchronous reads.

### MEM\_ACCESS.MEM\_WRITE\_IN\_ASYNC\_BLOCK — Write used in ASYNCHRONOUS

* Severity: ERROR
* Fix: Perform writes in SYNCHRONOUS blocks only.

### MEM\_ACCESS.MEM\_ADDR\_WIDTH\_TOO\_WIDE / MEM\_CONST\_ADDR\_OUT\_OF\_RANGE — Address width / const address out-of-range

* Severity: ERROR
* Fix: Use address width ≤ ceil(log2(depth)); ensure constant addresses < depth.

### MEM\_ACCESS.MEM\_MULTIPLE\_WRITES\_SAME\_IN — Multiple writes to same IN port in one block

* Severity: ERROR
* Fix: Ensure only one write to each IN port per SYNCHRONOUS block per execution path (use conditional exclusivity).

### MEM\_ACCESS.MEM\_INVALID\_WRITE\_MODE — Unknown write mode

* Severity: ERROR
* Fix: Use WRITE\_FIRST, READ\_FIRST, or NO\_CHANGE.

### MEM\_ACCESS.MEM\_INOUT\_INDEXED — INOUT port indexed

* Severity: ERROR
* Fix: INOUT ports may not use `mem.port[addr]` syntax; use `.addr`/`.data`/`.wdata` pseudo-fields.

### MEM\_ACCESS.MEM\_INOUT\_WDATA\_IN\_ASYNC — INOUT .wdata in ASYNCHRONOUS

* Severity: ERROR
* Fix: INOUT port `.wdata` may only be assigned in SYNCHRONOUS blocks.

### MEM\_ACCESS.MEM\_INOUT\_ADDR\_IN\_ASYNC — INOUT .addr in ASYNCHRONOUS

* Severity: ERROR
* Fix: INOUT port `.addr` may only be assigned in SYNCHRONOUS blocks.

### MEM\_ACCESS.MEM\_INOUT\_WDATA\_WRONG\_OP — INOUT .wdata wrong operator

* Severity: ERROR
* Fix: INOUT port `.wdata` must be assigned with `<=` operator.

### MEM\_ACCESS.MEM\_MULTIPLE\_ADDR\_ASSIGNS — Multiple INOUT .addr assignments

* Severity: ERROR
* Fix: Assign `.addr` at most once per execution path.

### MEM\_ACCESS.MEM\_MULTIPLE\_WDATA\_ASSIGNS — Multiple INOUT .wdata assignments

* Severity: ERROR
* Fix: Assign `.wdata` at most once per execution path.

### MEM\_DECLARATION.MEM\_INOUT\_MIXED\_WITH\_IN\_OUT — INOUT mixed with IN/OUT

* Severity: ERROR
* Fix: INOUT ports cannot be mixed with IN or OUT ports in the same MEM declaration.

### MEM\_DECLARATION.MEM\_INOUT\_ASYNC — INOUT with ASYNC keyword

* Severity: ERROR
* Fix: INOUT ports are always synchronous; do not specify ASYNC/SYNC keyword.

### MEM\_WARNINGS.MEM\_WARN\_PORT\_NEVER\_ACCESSED — MEM port unused

* Severity: WARN
* Fix: Remove unused port or add intended accesses.

### MEM\_WARNINGS.MEM\_WARN\_PARTIAL\_INIT — Init file smaller than depth

* Severity: WARN
* Fix: Provide full init file or accept zero-padding.

***

## General Warnings & Info

### GENERAL\_WARNINGS.WARN\_UNUSED\_REGISTER — Unused register

* Severity: WARN
* Fix: Remove register if not needed or use it in logic.

### GENERAL\_WARNINGS.WARN\_UNCONNECTED\_OUTPUT — Unconnected module output

* Severity: WARN
* Fix: Drive the output, connect to top-level pin, or intentionally bind to `_` and document intent.

### GENERAL\_WARNINGS.WARN\_INCOMPLETE\_SELECT\_ASYNC — ASYNC SELECT missing DEFAULT

* Severity: WARN
* Fix: Add DEFAULT to prevent floating nets.

### GENERAL\_WARNINGS.WARN\_DEAD\_CODE\_UNREACHABLE — Unreachable statements

* Severity: WARN
* Fix: Remove or correct unreachable logic conditions.

### GENERAL\_WARNINGS.WARN\_UNUSED\_MODULE — Module never instantiated

* Severity: WARN
* Fix: Instantiate module or remove it.

### GENERAL\_WARNINGS.WARN\_UNSINKED\_REGISTER — Register written but never read

* Severity: WARN
* Fix: Read the register value or remove it if not needed.

### GENERAL\_WARNINGS.WARN\_UNDRIVEN\_REGISTER — Register read but never written

* Severity: WARN
* Fix: Assign the register in a SYNCHRONOUS block.

### GENERAL\_WARNINGS.WARN\_UNUSED\_WIRE — Wire never driven or read

* Severity: WARN
* Fix: Remove unused wire declaration.

### GENERAL\_WARNINGS.WARN\_UNUSED\_PORT — Port never used

* Severity: WARN
* Fix: Remove unused port or connect it.

### GENERAL\_WARNINGS.WARN\_INTERNAL\_TRISTATE — Internal tri-state not FPGA-compatible

* Severity: WARN
* Cause: Internal tri-state logic is not supported by most FPGAs.
* Fix: Use `--tristate-default` to enable automatic tri-state elimination.

***

## Latch Rules

### LATCH\_RULES.LATCH\_ASSIGN\_NON\_GUARDED — Latch assignment not guarded

* Severity: ERROR
* Cause: LATCH must be written via guarded assignment `name <= enable : data;` in ASYNCHRONOUS blocks.
* Fix: Use guarded syntax for D latches or `name <= set : reset;` for SR latches.

### LATCH\_RULES.LATCH\_ASSIGN\_IN\_SYNC — Latch written in SYNCHRONOUS

* Severity: ERROR
* Fix: Move latch assignments to ASYNCHRONOUS blocks; use REGISTER for edge-triggered storage.

### LATCH\_RULES.LATCH\_ENABLE\_WIDTH\_NOT\_1 — D-latch enable not width-1

* Severity: ERROR
* Fix: Enable expression must have width \[1].

### LATCH\_RULES.LATCH\_ALIAS\_FORBIDDEN — Latch aliased

* Severity: ERROR
* Fix: Latches may not be aliased using `=`; they must not be merged into other nets.

### LATCH\_RULES.LATCH\_INVALID\_TYPE — Invalid latch type

* Severity: ERROR
* Fix: LATCH type must be `D` or `SR`.

### LATCH\_RULES.LATCH\_WIDTH\_INVALID — Invalid latch width

* Severity: ERROR
* Fix: Width must be a positive integer.

### LATCH\_RULES.LATCH\_SR\_WIDTH\_MISMATCH — SR latch set/reset width mismatch

* Severity: ERROR
* Fix: Set and reset expression widths must match the latch width.

### LATCH\_RULES.LATCH\_AS\_CLOCK\_OR\_CDC — Latch used as clock or CDC source

* Severity: ERROR
* Fix: Latches may not be used as clock signals or in CDC declarations.

### LATCH\_RULES.LATCH\_IN\_CONST\_CONTEXT — Latch in compile-time context

* Severity: ERROR
* Fix: Latch identifiers may not appear in `@check`/`@feature` conditions.

### LATCH\_RULES.LATCH\_CHIP\_UNSUPPORTED — Latch not supported by chip

* Severity: ERROR
* Fix: Selected chip does not support the latch type.

***

## Template Rules

### TEMPLATE.TEMPLATE\_UNDEFINED — Undefined template

* Severity: ERROR
* Fix: Ensure the template is defined before `@apply`.

### TEMPLATE.TEMPLATE\_ARG\_COUNT\_MISMATCH — Argument count mismatch

* Severity: ERROR
* Fix: `@apply` argument count must match template parameter count.

### TEMPLATE.TEMPLATE\_COUNT\_NOT\_NONNEG\_INT — Invalid @apply count

* Severity: ERROR
* Fix: Count must resolve to a non-negative integer.

### TEMPLATE.TEMPLATE\_NESTED\_DEF — Nested template definition

* Severity: ERROR
* Fix: `@template` definitions may not be nested.

### TEMPLATE.TEMPLATE\_FORBIDDEN\_DECL — Declaration inside template

* Severity: ERROR
* Fix: Use `@scratch` for temporary signals; WIRE/REGISTER/PORT/CONST/MEM/MUX are forbidden.

### TEMPLATE.TEMPLATE\_FORBIDDEN\_BLOCK\_HEADER — Block header inside template

* Severity: ERROR
* Fix: SYNCHRONOUS/ASYNCHRONOUS block headers are not allowed in template body.

### TEMPLATE.TEMPLATE\_FORBIDDEN\_DIRECTIVE — Directive inside template

* Severity: ERROR
* Fix: @new/@module/@feature and other structural directives are forbidden.

### TEMPLATE.TEMPLATE\_SCRATCH\_OUTSIDE — @scratch outside template

* Severity: ERROR
* Fix: `@scratch` may only appear inside a `@template` body.

### TEMPLATE.TEMPLATE\_APPLY\_OUTSIDE\_BLOCK — @apply outside block

* Severity: ERROR
* Fix: `@apply` may only appear inside ASYNCHRONOUS or SYNCHRONOUS blocks.

### TEMPLATE.TEMPLATE\_DUP\_NAME — Duplicate template name

* Severity: ERROR
* Fix: Template names must be unique within the same scope.

### TEMPLATE.TEMPLATE\_DUP\_PARAM — Duplicate parameter name

* Severity: ERROR
* Fix: Parameter names must be unique within the template definition.

### TEMPLATE.TEMPLATE\_SCRATCH\_WIDTH\_INVALID — Invalid @scratch width

* Severity: ERROR
* Fix: Width must be a positive integer constant expression.

### TEMPLATE.TEMPLATE\_EXTERNAL\_REF — External reference in template

* Severity: ERROR
* Fix: All identifiers must be parameters, @scratch wires, or compile-time constants. Pass external signals as arguments.

***

## Feature Guard Rules

### FEATURE\_GUARDS.FEATURE\_COND\_WIDTH\_NOT\_1 — Feature condition not width-1

* Severity: ERROR
* Fix: Feature guard condition must evaluate to a 1-bit value.

### FEATURE\_GUARDS.FEATURE\_EXPR\_INVALID\_CONTEXT — Invalid feature condition

* Severity: ERROR
* Fix: Feature guard condition may only reference CONFIG, module CONST, literals, and logical/comparison operators.

### FEATURE\_GUARDS.FEATURE\_NESTED — Nested @feature

* Severity: ERROR
* Fix: @feature guards may not be nested inside other @feature guards.

### FEATURE\_GUARDS.FEATURE\_VALIDATION\_BOTH\_PATHS — Both paths must validate

* Severity: ERROR
* Fix: Both the enabled and disabled branches must pass full semantic validation.

***

## @check Rules

### CHECK\_RULES.CHECK\_FAILED — Compile-time assertion failed

* Severity: ERROR
* Cause: `@check` expression evaluated to zero.
* Fix: Correct the assertion condition or the values it depends on.

### CHECK\_RULES.CHECK\_INVALID\_EXPR\_TYPE — Invalid @check expression

* Severity: ERROR
* Fix: Expression must be a nonnegative integer constant over literals, CONST, and CONFIG.

### CHECK\_RULES.CHECK\_INVALID\_PLACEMENT — @check in invalid location

* Severity: ERROR
* Fix: @check may not appear inside conditional or @feature bodies.

***

## BUS Rules

### BUS\_RULES.BUS\_DEF\_DUP\_NAME — Duplicate BUS definition

* Severity: ERROR
* Fix: BUS definition names must be unique within the project.

### BUS\_RULES.BUS\_DEF\_SIGNAL\_DUP\_NAME — Duplicate BUS signal name

* Severity: ERROR
* Fix: Signal names within a BUS definition must be unique.

### BUS\_RULES.BUS\_DEF\_INVALID\_DIR — Invalid BUS signal direction

* Severity: ERROR
* Fix: Direction must be IN, OUT, or INOUT.

### BUS\_RULES.BUS\_PORT\_UNKNOWN\_BUS — Unknown BUS reference

* Severity: ERROR
* Fix: BUS port references a BUS name not declared in the project.

### BUS\_RULES.BUS\_PORT\_INVALID\_ROLE — Invalid BUS role

* Severity: ERROR
* Fix: Role must be SOURCE or TARGET.

### BUS\_RULES.BUS\_PORT\_ARRAY\_COUNT\_INVALID — Invalid BUS array count

* Severity: ERROR
* Fix: Array count must be a positive integer constant expression.

### BUS\_RULES.BUS\_PORT\_INDEX\_REQUIRED — BUS index required

* Severity: ERROR
* Fix: Arrayed BUS access requires an explicit index or wildcard `[*]`.

### BUS\_RULES.BUS\_PORT\_INDEX\_NOT\_ARRAY — Indexed access on non-array BUS

* Severity: ERROR
* Fix: Only arrayed BUS ports support indexed access.

### BUS\_RULES.BUS\_PORT\_INDEX\_OUT\_OF\_RANGE — BUS index out of range

* Severity: ERROR
* Fix: Index must be within \[0, count-1].

### BUS\_RULES.BUS\_PORT\_NOT\_BUS — Member access on non-BUS port

* Severity: ERROR
* Fix: Dot notation is only valid on BUS ports.

### BUS\_RULES.BUS\_SIGNAL\_UNDEFINED — Undefined BUS signal

* Severity: ERROR
* Fix: Signal does not exist in the BUS definition.

### BUS\_RULES.BUS\_SIGNAL\_READ\_FROM\_WRITABLE — Read from writable BUS signal

* Severity: ERROR
* Fix: This signal is write-only from the current role's perspective.

### BUS\_RULES.BUS\_SIGNAL\_WRITE\_TO\_READABLE — Write to readable BUS signal

* Severity: ERROR
* Fix: This signal is read-only from the current role's perspective.

### BUS\_RULES.BUS\_WILDCARD\_WIDTH\_MISMATCH — Wildcard width mismatch

* Severity: ERROR
* Fix: RHS width must be 1 (broadcast) or equal to the array count (element-wise).

### BUS\_RULES.BUS\_TRISTATE\_MISMATCH — Tri-state on non-writable BUS signal

* Severity: ERROR
* Fix: Only writable BUS signals (INOUT or OUT from this role) may be assigned `z`.

### BUS\_RULES.BUS\_BULK\_BUS\_MISMATCH — Bulk BUS assignment bus mismatch

* Severity: ERROR
* Fix: Both sides of a bulk BUS assignment must reference the same BUS definition.

### BUS\_RULES.BUS\_BULK\_ROLE\_CONFLICT — Bulk BUS assignment role conflict

* Severity: ERROR
* Fix: Cannot assign between instances with the same role (SOURCE-SOURCE or TARGET-TARGET).

***

## CLOCK\_GEN Rules

### CLOCK\_GEN\_RULES.CLOCK\_GEN\_INPUT\_NOT\_DECLARED — Input clock not in CLOCKS

* Severity: ERROR
* Fix: CLOCK\_GEN input must reference a clock declared in the CLOCKS block.

### CLOCK\_GEN\_RULES.CLOCK\_GEN\_INPUT\_NO\_PERIOD — Input clock missing period

* Severity: ERROR
* Fix: CLOCK\_GEN input clock must have a period declared in CLOCKS.

### CLOCK\_GEN\_RULES.CLOCK\_GEN\_INPUT\_FREQ\_OUT\_OF\_RANGE — Input frequency out of range

* Severity: ERROR
* Fix: Input clock frequency is outside the chip's supported range for this generator type.

### CLOCK\_GEN\_RULES.CLOCK\_GEN\_OUTPUT\_NOT\_DECLARED — Output clock not in CLOCKS

* Severity: ERROR
* Fix: CLOCK\_GEN output must reference a clock declared in the CLOCKS block.

### CLOCK\_GEN\_RULES.CLOCK\_GEN\_OUTPUT\_HAS\_PERIOD — Output clock has period

* Severity: ERROR
* Fix: CLOCK\_GEN output clocks must not have a period in CLOCKS (derived automatically).

### CLOCK\_GEN\_RULES.CLOCK\_GEN\_OUTPUT\_IS\_INPUT\_PIN — Output clock is input pin

* Severity: ERROR
* Fix: CLOCK\_GEN outputs must not be declared as IN\_PINS.

### CLOCK\_GEN\_RULES.CLOCK\_GEN\_MULTIPLE\_DRIVERS — Clock multiply driven

* Severity: ERROR
* Fix: A clock may only be driven by one CLOCK\_GEN output.

### CLOCK\_GEN\_RULES.CLOCK\_GEN\_INPUT\_IS\_SELF\_OUTPUT — Self-referencing clock

* Severity: ERROR
* Fix: CLOCK\_GEN input must not be an output of the same block.

### CLOCK\_GEN\_RULES.CLOCK\_GEN\_INVALID\_TYPE — Invalid generator type

* Severity: ERROR
* Fix: Generator type must be PLL, DLL, CLKDIV, or OSC.

### CLOCK\_GEN\_RULES.CLOCK\_GEN\_MISSING\_INPUT — Missing IN clock

* Severity: ERROR
* Fix: Generator must declare an IN clock (except OSC which has no external input).

### CLOCK\_GEN\_RULES.CLOCK\_GEN\_MISSING\_OUTPUT — Missing OUT clock

* Severity: ERROR
* Fix: Generator must declare at least one OUT clock.

### CLOCK\_GEN\_RULES.CLOCK\_GEN\_PARAM\_OUT\_OF\_RANGE — CONFIG parameter out of range

* Severity: ERROR
* Fix: Parameter value is outside the chip's valid range.

### CLOCK\_GEN\_RULES.CLOCK\_GEN\_DERIVED\_OUT\_OF\_RANGE — Derived frequency out of range

* Severity: ERROR
* Fix: Computed VCO or output frequency is outside the chip's valid range. Adjust CONFIG parameters.

***

## Path Security

### PATH\_SECURITY.PATH\_ABSOLUTE\_FORBIDDEN — Absolute path used

* Severity: ERROR
* Cause: An `@import` or `@file()` path begins with `/` (or drive letter on Windows).
* Fix: Use relative paths or pass `--allow-absolute-paths`.

### PATH\_SECURITY.PATH\_TRAVERSAL\_FORBIDDEN — Directory traversal

* Severity: ERROR
* Cause: Path contains `..` component.
* Fix: Remove `..` from path or pass `--allow-traversal`.

### PATH\_SECURITY.PATH\_OUTSIDE\_SANDBOX — Path outside sandbox

* Severity: ERROR
* Cause: Resolved canonical path does not start with any permitted sandbox root.
* Fix: Use `--sandbox-root=<dir>` to add additional permitted directories.

### PATH\_SECURITY.PATH\_SYMLINK\_ESCAPE — Symlink escapes sandbox

* Severity: ERROR
* Cause: A symbolic link inside the sandbox resolves to a target outside the sandbox.
* Fix: Remove the symlink or add the target directory to `--sandbox-root`.

***

## lit() Intrinsic Rules

### FUNCTIONS\_AND\_LIT.LIT\_WIDTH\_INVALID — lit() width invalid

* Severity: ERROR
* Fix: Width must be a positive integer constant expression.

### FUNCTIONS\_AND\_LIT.LIT\_VALUE\_INVALID — lit() value invalid

* Severity: ERROR
* Fix: Value must be a nonnegative integer constant expression.

### FUNCTIONS\_AND\_LIT.LIT\_VALUE\_OVERFLOW — lit() value overflows width

* Severity: ERROR
* Fix: Value exceeds the declared width.

### FUNCTIONS\_AND\_LIT.LIT\_INVALID\_CONTEXT — lit() in compile-time context

* Severity: ERROR
* Fix: lit() produces a runtime literal; it is not valid where a compile-time integer constant is required.

***

## Testbench Rules

### TESTBENCH.TB\_MODULE\_NOT\_FOUND — Module not found

* Severity: ERROR
* Fix: @testbench module name must refer to a module in scope.

### TESTBENCH.TB\_PORT\_NOT\_CONNECTED — Port not connected

* Severity: ERROR
* Fix: All module ports must be connected in @new.

### TESTBENCH.TB\_PORT\_WIDTH\_MISMATCH — Port width mismatch

* Severity: ERROR
* Fix: Port width must match module declared width.

### TESTBENCH.TB\_NEW\_RHS\_INVALID — Invalid @new RHS

* Severity: ERROR
* Fix: @new RHS must be a testbench CLOCK or WIRE.

### TESTBENCH.TB\_SETUP\_POSITION — @setup position invalid

* Severity: ERROR
* Fix: @setup must appear exactly once per TEST, after @new, before other directives.

### TESTBENCH.TB\_CLOCK\_NOT\_DECLARED — Clock not declared

* Severity: ERROR
* Fix: @clock clock identifier must refer to a declared CLOCK.

### TESTBENCH.TB\_CLOCK\_CYCLE\_NOT\_POSITIVE — Non-positive cycle count

* Severity: ERROR
* Fix: @clock cycle count must be a positive integer.

### TESTBENCH.TB\_UPDATE\_NOT\_WIRE — @update targets non-wire

* Severity: ERROR
* Fix: @update may only assign testbench WIRE identifiers.

### TESTBENCH.TB\_UPDATE\_CLOCK\_ASSIGN — @update assigns clock

* Severity: ERROR
* Fix: @update may not assign clock signals.

### TESTBENCH.TB\_EXPECT\_WIDTH\_MISMATCH — @expect width mismatch

* Severity: ERROR
* Fix: @expect value width must match signal width.

### TESTBENCH.TB\_NO\_TEST\_BLOCKS — No TEST blocks

* Severity: ERROR
* Fix: @testbench must contain at least one TEST block.

### TESTBENCH.TB\_MULTIPLE\_NEW — Multiple @new

* Severity: ERROR
* Fix: Each TEST must contain exactly one @new instantiation.

### TESTBENCH.TB\_PROJECT\_MIXED — Mixed @project and @testbench

* Severity: ERROR
* Fix: A file may not contain both @project and @testbench.

***

## Simulation Rules

### SIMULATION.SIM\_PROJECT\_MIXED — Mixed @project and @simulation

* Severity: ERROR
* Fix: A file may not contain both @project and @simulation.
