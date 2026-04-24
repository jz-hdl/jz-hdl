import{_ as a,o as n,c as i,ai as p}from"./chunks/framework.C4ntERNc.js";const E=JSON.parse('{"title":"Clock Domains","description":"","frontmatter":{"title":"Clock Domains","lang":"en-US","layout":"doc","outline":"deep"},"headers":[],"relativePath":"reference-manual/clock-domains.md","filePath":"reference-manual/clock-domains.md"}'),l={name:"reference-manual/clock-domains.md"};function e(t,s,h,k,c,r){return n(),i("div",null,[...s[0]||(s[0]=[p(`<div style="display:none;" hidden="true" aria-hidden="true">Are you an LLM? You can read better optimized documentation at /jz-hdl/reference-manual/clock-domains.md for this page in Markdown format</div><h1 id="clock-domains" tabindex="-1">Clock Domains <a class="header-anchor" href="#clock-domains" aria-label="Permalink to “Clock Domains”">​</a></h1><h2 id="overview" tabindex="-1">Overview <a class="header-anchor" href="#overview" aria-label="Permalink to “Overview”">​</a></h2><p>Clock domains are explicit in JZ-HDL. Registers have a single home domain (the clock they are synchronized to). Direct use of a register across different clock domains is forbidden. The <code>CDC</code> block provides explicit, designer‑declared crossings that create safe, compiler‑understood synchronized views of register values between domains.</p><p>Key goals:</p><ul><li>Make cross‑domain behavior explicit and auditable.</li><li>Prevent accidental multi‑domain register usage.</li><li>Let the compiler enforce domain locality and generate appropriate synchronizer structures.</li></ul><h2 id="syntax" tabindex="-1">Syntax <a class="header-anchor" href="#syntax" aria-label="Permalink to “Syntax”">​</a></h2><p>CDC entries appear inside a module body in a <code>CDC { ... }</code> block.</p><p>Basic form:</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>CDC {</span></span>
<span class="line"><span>  &lt;cdc_type&gt;[n_stages] &lt;source_reg&gt; (&lt;src_clk&gt;) =&gt; &lt;dest_alias&gt; (&lt;dest_clk&gt;);</span></span>
<span class="line"><span>  ...</span></span>
<span class="line"><span>}</span></span></code></pre></div><ul><li><code>cdc_type</code> is one of: <code>BIT</code>, <code>BUS</code>, <code>FIFO</code>, <code>HANDSHAKE</code>, <code>PULSE</code>, <code>MCP</code>, <code>RAW</code></li><li><code>[n_stages]</code> optional positive integer; default = 2 (must NOT be provided for <code>RAW</code>)</li><li><code>source_reg</code>: a <code>REGISTER</code> identifier defined in the same module (plain name, no slices/concat)</li><li><code>(src_clk)</code>: clock identifier for the source/home domain (must match a <code>SYNCHRONOUS</code> block <code>CLK</code> or a top-level clock input)</li><li><code>=&gt; dest_alias (dest_clk)</code>: creates a read‑only alias visible in the destination clock domain</li></ul><p>Examples:</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>CDC {</span></span>
<span class="line"><span>  BIT    status_sync (clk_io) =&gt; cpu_status (clk_cpu);</span></span>
<span class="line"><span>  BUS[3] flags_bus    (clk_a)  =&gt; flags_view (clk_b);</span></span>
<span class="line"><span>  FIFO[4] payload     (clk_prod) =&gt; consumer_view (clk_cons);</span></span>
<span class="line"><span>}</span></span></code></pre></div><h2 id="semantics" tabindex="-1">Semantics <a class="header-anchor" href="#semantics" aria-label="Permalink to “Semantics”">​</a></h2><ul><li>The CDC entry sets the <strong>home clock domain</strong> of the source register to <code>src_clk</code>.</li><li>The dest alias is a read‑only signal that represents the synchronized view of the source register after approximately <code>n_stages</code> cycles in the destination clock.</li><li>The source register may only be read/written in <code>SYNCHRONOUS</code> blocks whose <code>CLK</code> equals <code>src_clk</code> (its home domain).</li><li>The dest alias may only be read in <code>SYNCHRONOUS</code> blocks whose <code>CLK</code> equals <code>dest_clk</code>. It can also be read combinationally in <code>ASYNCHRONOUS</code> blocks, but that combinational use must respect domain semantics (see &quot;Usage Notes&quot;).</li><li>The compiler elaborates the <code>CDC</code> entry into appropriate synchronization hardware: <ul><li><code>BIT[n]</code>: N-stage single-bit flip-flop synchronizer. The source register <strong>must have width [1]</strong> — using BIT with a multi-bit register is a compile error (<code>CDC_BIT_WIDTH_NOT_1</code>).</li><li><code>BUS[n]</code>: Multi‑bit synchronized path; intended only when the source follows Gray‑code or single‑bit change discipline.</li><li><code>FIFO[n]</code>: Asynchronous FIFO for arbitrary multi‑bit transfers (handles wide or multi‑bit simultaneous changes safely).</li><li><code>HANDSHAKE[n]</code>: Req/ack handshake protocol for infrequent multi‑bit transfers.</li><li><code>PULSE[n]</code>: Toggle-based pulse synchronizer. The source register <strong>must have width [1]</strong> — using PULSE with a multi-bit register is a compile error (<code>CDC_PULSE_WIDTH_NOT_1</code>).</li><li><code>MCP[n]</code>: Multi‑cycle path formulation for stable multi‑bit data.</li><li><code>RAW</code>: Direct unsynchronized wire connection (no crossing logic).</li></ul></li></ul><h2 id="cdc-types-and-intended-use" tabindex="-1">CDC Types and Intended Use <a class="header-anchor" href="#cdc-types-and-intended-use" aria-label="Permalink to “CDC Types and Intended Use”">​</a></h2><ul><li><p>BIT[n_stages]</p><ul><li>Use for single‑bit control/status signals.</li><li>Result <code>dest_alias</code> width: 1.</li><li><code>n_stages</code> typically 2 or 3 in practice.</li><li>Latency: exactly <code>n_stages</code> cycles of <code>dest_clk</code>.</li></ul></li><li><p>BUS[n_stages]</p><ul><li>Use for multi‑bit values that change in a safe, bounded way (e.g., Gray‑encoded counters, handshaked state encodings where only one bit flips at a time).</li><li>Not safe for arbitrary parallel changes (multi‑bit updates).</li><li>If source can change arbitrarily, prefer <code>FIFO</code>.</li><li>Latency: exactly <code>n_stages</code> cycles of <code>dest_clk</code>.</li></ul></li><li><p>FIFO[n_stages]</p><ul><li>Use for arbitrary multi‑bit transfers (data buses, registers updated with new values every cycle).</li><li>Produces a staged, safe transfer; semantics approximates a buffer or asynchronous FIFO depending on implementation.</li><li>Latency: between 1 cycle (existing data) and <code>n_stages + 2</code> cycles (fresh write).</li></ul></li><li><p>HANDSHAKE[n_stages]</p><ul><li>Use for infrequent multi‑bit transfers where throughput is not critical.</li><li>Source latches data and asserts request; destination syncs request, latches data, and asserts ack; source syncs ack and deasserts request.</li><li>Safe for arbitrary data widths.</li><li>Latency: variable depending on clock ratio; transfer completes after full req/ack handshake.</li></ul></li><li><p>PULSE[n_stages]</p><ul><li>Use for single‑bit pulse events (width == 1 only).</li><li>Source toggles a register on each pulse; destination syncs the toggle and XOR‑detects edges to produce output pulses.</li><li>Latency: <code>n_stages</code> cycles for pulse detection; one output pulse per input pulse.</li></ul></li><li><p>MCP[n_stages]</p><ul><li>Multi‑cycle path formulation for stable multi‑bit data transfers.</li><li>Source holds data stable and asserts an enable; destination syncs the enable and samples data when the enable is seen.</li><li>Uses the same req/ack protocol as HANDSHAKE for safe handoff.</li><li>Latency: variable, similar to HANDSHAKE; data held stable during transfer.</li></ul></li><li><p>RAW</p><ul><li>Direct unsynchronized view — no crossing logic is inserted.</li><li>The destination alias is a direct wire connection (<code>assign</code>) to the source register.</li><li>The <code>[n_stages]</code> parameter must NOT be provided (compile error if present).</li><li>Any register width is allowed.</li><li>Latency: 0 cycles (direct connection).</li><li>Use only when the designer knows the signals are safe (e.g., quasi‑static configuration registers, signals with external synchronization, or signals that are stable at the time of sampling).</li><li><strong>Warning:</strong> RAW explicitly opts out of CDC safety guarantees. Metastability protection is the designer&#39;s responsibility.</li></ul></li></ul><h2 id="validation-rules-compiler-enforced" tabindex="-1">Validation Rules (Compiler Enforced) <a class="header-anchor" href="#validation-rules-compiler-enforced" aria-label="Permalink to “Validation Rules (Compiler Enforced)”">​</a></h2><ul><li>Source register must be a <code>REGISTER</code> declared in the containing module.</li><li>Source register identifier must be a plain name (no slices, concatenations, instance-qualified names).</li><li>The <code>CDC</code> entry establishes the home domain for the source register; that register: <ul><li>May only be assigned (in <code>SYNCHRONOUS</code>) inside a <code>SYNCHRONOUS</code> block whose <code>CLK</code> equals <code>src_clk</code>.</li><li>May only be read inside <code>SYNCHRONOUS</code> blocks whose <code>CLK</code> equals <code>src_clk</code>.</li><li>Any attempt to use the source register in a <code>SYNCHRONOUS</code> block with a different <code>CLK</code> is a DOMAIN_CONFLICT error.</li></ul></li><li>The dest alias: <ul><li>Is created by the compiler as a read‑only signal name (not a register the designer assigns).</li><li>May only be read in <code>SYNCHRONOUS</code> blocks whose <code>CLK</code> equals <code>dest_clk</code>.</li><li>Attempting to assign to the dest alias is a compile error.</li></ul></li><li>A module may have at most one <code>SYNCHRONOUS</code> block per unique clock signal. (See SYNCHRONOUS block rules.)</li><li>Multiple CDC entries may declare crossings between the same pair of clocks or different clocks; each source register&#39;s home domain is set by its CORESCDC entry.</li><li>If a register is referenced by a CDC entry, that CDC entry must appear before any <code>SYNCHRONOUS</code> block that uses the alias or the source register (tool-specific ordering rule for resolvability).</li><li>For <code>BUS[n_stages]</code>, compiler may generate warnings if it detects potential multi‑bit simultaneous changes that violate Gray‑code assumptions (static/flow analysis best‑effort).</li></ul><h2 id="usage-notes" tabindex="-1">Usage Notes <a class="header-anchor" href="#usage-notes" aria-label="Permalink to “Usage Notes”">​</a></h2><ul><li>The <code>CDC</code> block does not replace proper CDC design discipline; it documents intent and enables the toolchain to synthesize correct synchronizers and raise violations.</li><li>The destination alias is an explicit name you should use in the destination domain&#39;s synchronous logic, not the source register name. <ul><li>Correct: <code>IF (cpu_status) { ... }</code> where <code>cpu_status</code> is <code>dest_alias</code>.</li><li>Incorrect: reading <code>status_reg</code> inside <code>CLK=cpu_clk</code> when <code>status_reg</code> is home to <code>clk_io</code>.</li></ul></li><li><code>dest_alias</code> is visible combinationally in <code>ASYNCHRONOUS</code> blocks, but be careful: combinational logic that mixes <code>dest_alias</code> with signals from the destination clock domain should not be used to create cross‑domain control paths (avoid asynchronous handshakes without explicit synchronizers).</li><li>If you need to sample a multi‑bit value that changes arbitrarily, use <code>FIFO</code> to avoid metastability and data corruption.</li><li>A single register may have multiple destination aliases to different clocks (fan‑out synchronization to multiple domains).</li></ul><h2 id="examples" tabindex="-1">Examples <a class="header-anchor" href="#examples" aria-label="Permalink to “Examples”">​</a></h2><h3 id="single‐bit-synchronizer-bit-2-stages-default" tabindex="-1">Single‑bit synchronizer (BIT, 2 stages default) <a class="header-anchor" href="#single‐bit-synchronizer-bit-2-stages-default" aria-label="Permalink to “Single‑bit synchronizer (BIT, 2 stages default)”">​</a></h3><p>A 1‑bit <code>event_flag</code> is written in the <code>clk_a</code> domain and read via the synchronized alias <code>event_flag_sync</code> in the <code>clk_b</code> domain. The compiler inserts a 2‑stage flip‑flop chain.</p><div class="vp-code-group"><div class="tabs"><input type="radio" name="group-467" id="tab-468" checked><label data-title="cdc_bit.jz" for="tab-468">cdc_bit.jz</label><input type="radio" name="group-467" id="tab-469"><label data-title="project.jz" for="tab-469">project.jz</label><input type="radio" name="group-467" id="tab-470"><label data-title="Generated Verilog" for="tab-470">Generated Verilog</label><input type="radio" name="group-467" id="tab-471"><label data-title="Generated RTLIL" for="tab-471">Generated RTLIL</label></div><div class="blocks"><div class="language-jz active"><button title="Copy Code" class="copy"></button><span class="lang">jz</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// CDC BIT example: single-bit synchronizer (2 stages default)</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">//</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// Demonstrates crossing a 1-bit flag from clk_a to clk_b.</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// The BIT synchronizer inserts a 2-stage flip-flop chain.</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">@module</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> cdc_bit</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    PORT</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] clk_a;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] clk_b;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] rst_n;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] trigger;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        OUT</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> [1] led;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    REGISTER</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        event_flag [1] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;"> 1&#39;b0</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        cpu_seen   [1] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;"> 1&#39;b0</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    CDC {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        BIT event_flag (clk_a) </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=&gt;</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> event_flag_sync (clk_b);</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    ASYNCHRONOUS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        led </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">&lt;= </span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">cpu_seen;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    SYNCHRONOUS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">(CLK</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">clk_a RESET</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">rst_n RESET_ACTIVE</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">Low) {</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">        IF</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> (trigger) {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">            event_flag </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">&lt;= </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">1&#39;b1</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        } </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">ELSE</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">            event_flag </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">&lt;= </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">1&#39;b0</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        }</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    SYNCHRONOUS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">(CLK</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">clk_b RESET</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">rst_n RESET_ACTIVE</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">Low) {</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">        IF</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> (event_flag_sync) {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">            cpu_seen </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">&lt;= </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">1&#39;b1</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        } </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">ELSE</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">            cpu_seen </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">&lt;= </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">1&#39;b0</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        }</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">@endmod</span></span></code></pre></div><div class="language-jz"><button title="Copy Code" class="copy"></button><span class="lang">jz</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">@project</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">(CHIP</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">&quot;GW1NR</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">-</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">9</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">-</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">QN88</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">-</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">C6</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">-</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">I5&quot;) CDC_BIT_EXAMPLE</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    @import</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> &quot;cdc_bit.jz&quot;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">    CLOCKS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        SCLK     </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">period</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">37.04 }; </span><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// 27MHz</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        CLK_FAST;                     </span><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// 108MHz (PLL)</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">    IN_PINS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        SCLK   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">standard</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LVCMOS33 };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        DONE   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">standard</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LVCMOS33 };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        KEY[2] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">standard</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LVCMOS33 };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">    OUT_PINS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[1] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">standard</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LVCMOS33, </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">drive</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">8 };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">    MAP</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        SCLK   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 52;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        KEY[0] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 3;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        KEY[1] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 4;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[0] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 10;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        DONE   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> IOL14A;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    CLOCK_GEN {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        PLL {</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">            IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  REF_CLK SCLK;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">            OUT</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> BASE    CLK_FAST;</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">            CONFIG</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">                IDIV  </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 2;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">                FBDIV </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 7;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">                ODIV  </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 8;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">            };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    @top</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> cdc_bit {</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] clk_a   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> SCLK;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] clk_b   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> CLK_FAST;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] rst_n   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> KEY[0];</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] trigger </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> KEY[1];</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        OUT</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> [1] led     </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;"> ~</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LED;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">@endproj</span></span></code></pre></div><div class="language-v"><button title="Copy Code" class="copy"></button><span class="lang">v</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// This Verilog was transpiled from JZ-HDL.</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// jz-hdl version: Version 0.1.7 (c6d52fc)</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// Intended for use with yosys.</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">\`default_nettype none</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">module JZHDL_LIB_CDC_BIT (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    clk_dest,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    data_in,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    data_out</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">);</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Ports</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input clk_dest;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input data_in;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    output data_out;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Signals</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg sync_ff1;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg sync_ff2;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign data_out = sync_ff2;</span></span>
<span class="line"></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @(posedge clk_dest) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        sync_ff1 &lt;= data_in;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        sync_ff2 &lt;= sync_ff1;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">endmodule</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">module cdc_bit (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    clk_a,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    clk_b,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    rst_n,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    trigger,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    led</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">);</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Ports</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input clk_a;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input clk_b;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input rst_n;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input trigger;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    output reg led;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Signals</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg event_flag;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg cpu_seen;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire event_flag_sync;</span></span>
<span class="line"></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    JZHDL_LIB_CDC_BIT u_cdc_bit_event_flag_sync (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .clk_dest(clk_b),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .data_in(event_flag),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .data_out(event_flag_sync)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    );</span></span>
<span class="line"></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @* begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        led = cpu_seen;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @(posedge clk_a) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        if (!rst_n) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            event_flag &lt;= 1&#39;b0;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        else begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            if (trigger) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">                event_flag &lt;= 1&#39;b1;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            else begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">                event_flag &lt;= 1&#39;b0;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @(posedge clk_b) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        if (!rst_n) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            cpu_seen &lt;= 1&#39;b0;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        else begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            if (event_flag_sync) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">                cpu_seen &lt;= 1&#39;b1;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            else begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">                cpu_seen &lt;= 1&#39;b0;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">endmodule</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">module top (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    SCLK,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    DONE,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    KEY,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    LED</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">);</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input SCLK;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input DONE;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input [1:0] KEY;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    output LED;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Top-level logical→physical pin mapping</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_bit.clk_a -&gt; SCLK (board 52)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_bit.clk_b -&gt; CLK_FAST (clock gen)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_bit.rst_n -&gt; KEY[0] (board 3)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_bit.trigger -&gt; KEY[1] (board 4)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_bit.led -&gt; LED[0] (board 10)</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire jz_inv_led;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[0] = ~jz_inv_led;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire CLK_FAST;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire jz_unused_pll_LOCK_cg0_u0;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire jz_unused_pll_PHASE_cg0_u0;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire jz_unused_pll_DIV_cg0_u0;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire jz_unused_pll_DIV3_cg0_u0;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // CLOCK_GEN PLL instantiation (from chip data)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    rPLL #(</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DEVICE(&quot;GW1N-9C&quot;),          // Specify your device</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .FCLKIN(&quot;26.998&quot;),       // Input frequency in MHz</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .IDIV_SEL(2),           // IDIV: Input divider</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .FBDIV_SEL(7),         // FBDIV: Feedback divider</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .ODIV_SEL(8),           // ODIV: Output divider</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .PSDA_SEL(&quot;0000&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DUTYDA_SEL(&quot;1000&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_IDIV_SEL(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_FBDIV_SEL(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_ODIV_SEL(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_DA_EN(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_SDIV_SEL(2),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUT_FT_DIR(1&#39;b1),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTP_FT_DIR(1&#39;b1),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUT_DLY_STEP(0),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTP_DLY_STEP(0),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKFB_SEL(&quot;internal&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUT_BYPASS(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTP_BYPASS(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD_BYPASS(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD_SRC(&quot;CLKOUT&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD3_SRC(&quot;CLKOUT&quot;)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">) u_pll_0_0 (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUT(CLK_FAST),   // Primary output</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .LOCK(jz_unused_pll_LOCK_cg0_u0),     // High when stable</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTP(jz_unused_pll_PHASE_cg0_u0), // Phase shifted output</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD(jz_unused_pll_DIV_cg0_u0),   // Divided output</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD3(jz_unused_pll_DIV3_cg0_u0), // Divided by 3 output</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .RESET(1&#39;b0),        // Reset signal</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .RESET_P(1&#39;b0),      // PLL power down</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKIN(SCLK),  // Reference clock input</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKFB(1&#39;b0)         // External feedback</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">);</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    cdc_bit u_top (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .clk_a(SCLK),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .clk_b(CLK_FAST),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .rst_n(KEY[0]),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .trigger(KEY[1]),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .led(jz_inv_led)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    );</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">endmodule</span></span></code></pre></div><div class="language-il"><button title="Copy Code" class="copy"></button><span class="lang">il</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span># Generated by jz-hdl RTLIL backend</span></span>
<span class="line"><span># jz-hdl version: Version 0.1.7 (c6d52fc)</span></span>
<span class="line"><span></span></span>
<span class="line"><span>attribute \\src &quot;unknown:1&quot;</span></span>
<span class="line"><span>module \\JZHDL_LIB_CDC_BIT</span></span>
<span class="line"><span>  wire width 1 input 1 \\clk_dest</span></span>
<span class="line"><span>  wire width 1 input 2 \\data_in</span></span>
<span class="line"><span>  wire width 1 output 3 \\data_out</span></span>
<span class="line"><span>  wire width 1 \\sync_ff1</span></span>
<span class="line"><span>  wire width 1 \\sync_ff2</span></span>
<span class="line"><span>  connect \\data_out \\sync_ff2</span></span>
<span class="line"><span>  wire $0\\sync_ff1[0:0]</span></span>
<span class="line"><span>  wire $0\\sync_ff2[0:0]</span></span>
<span class="line"><span>  process $proc$clk0$1</span></span>
<span class="line"><span>    assign $0\\sync_ff1[0:0] \\sync_ff1</span></span>
<span class="line"><span>    assign $0\\sync_ff2[0:0] \\sync_ff2</span></span>
<span class="line"><span>    assign $0\\sync_ff1[0:0] \\data_in</span></span>
<span class="line"><span>    assign $0\\sync_ff2[0:0] \\sync_ff1</span></span>
<span class="line"><span>    sync posedge \\clk_dest</span></span>
<span class="line"><span>      update \\sync_ff1 $0\\sync_ff1[0:0]</span></span>
<span class="line"><span>      update \\sync_ff2 $0\\sync_ff2[0:0]</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>end</span></span>
<span class="line"><span></span></span>
<span class="line"><span>attribute \\src &quot;jz-hdl:6&quot;</span></span>
<span class="line"><span>module \\cdc_bit</span></span>
<span class="line"><span>  wire width 1 input 1 \\clk_a</span></span>
<span class="line"><span>  wire width 1 input 2 \\clk_b</span></span>
<span class="line"><span>  wire width 1 input 3 \\rst_n</span></span>
<span class="line"><span>  wire width 1 input 4 \\trigger</span></span>
<span class="line"><span>  wire width 1 output 5 \\led</span></span>
<span class="line"><span>  wire width 1 \\event_flag</span></span>
<span class="line"><span>  wire width 1 \\cpu_seen</span></span>
<span class="line"><span>  wire width 1 \\event_flag_sync</span></span>
<span class="line"><span>  cell \\JZHDL_LIB_CDC_BIT \\u_cdc_bit_event_flag_sync</span></span>
<span class="line"><span>    connect \\clk_dest \\clk_b</span></span>
<span class="line"><span>    connect \\data_in \\event_flag</span></span>
<span class="line"><span>    connect \\data_out \\event_flag_sync</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire $0\\led[0:0]</span></span>
<span class="line"><span>  process $proc$async$2</span></span>
<span class="line"><span>    assign $0\\led[0:0] \\cpu_seen</span></span>
<span class="line"><span>    sync always</span></span>
<span class="line"><span>      update \\led $0\\led[0:0]</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire $0\\event_flag[0:0]</span></span>
<span class="line"><span>  wire width 1 $auto$3</span></span>
<span class="line"><span>  cell $logic_not $auto$4</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\rst_n</span></span>
<span class="line"><span>    connect \\Y $auto$3</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  process $proc$clk0$5</span></span>
<span class="line"><span>    assign $0\\event_flag[0:0] \\event_flag</span></span>
<span class="line"><span>    switch $auto$3</span></span>
<span class="line"><span>      case 1&#39;1</span></span>
<span class="line"><span>        assign $0\\event_flag[0:0] 1&#39;0</span></span>
<span class="line"><span>      case</span></span>
<span class="line"><span>        switch \\trigger</span></span>
<span class="line"><span>          case 1&#39;1</span></span>
<span class="line"><span>            assign $0\\event_flag[0:0] 1&#39;1</span></span>
<span class="line"><span>          case</span></span>
<span class="line"><span>            assign $0\\event_flag[0:0] 1&#39;0</span></span>
<span class="line"><span>        end</span></span>
<span class="line"><span>    end</span></span>
<span class="line"><span>    sync posedge \\clk_a</span></span>
<span class="line"><span>      update \\event_flag $0\\event_flag[0:0]</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire $0\\cpu_seen[0:0]</span></span>
<span class="line"><span>  wire width 1 $auto$6</span></span>
<span class="line"><span>  cell $logic_not $auto$7</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\rst_n</span></span>
<span class="line"><span>    connect \\Y $auto$6</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  process $proc$clk1$8</span></span>
<span class="line"><span>    assign $0\\cpu_seen[0:0] \\cpu_seen</span></span>
<span class="line"><span>    switch $auto$6</span></span>
<span class="line"><span>      case 1&#39;1</span></span>
<span class="line"><span>        assign $0\\cpu_seen[0:0] 1&#39;0</span></span>
<span class="line"><span>      case</span></span>
<span class="line"><span>        switch \\event_flag_sync</span></span>
<span class="line"><span>          case 1&#39;1</span></span>
<span class="line"><span>            assign $0\\cpu_seen[0:0] 1&#39;1</span></span>
<span class="line"><span>          case</span></span>
<span class="line"><span>            assign $0\\cpu_seen[0:0] 1&#39;0</span></span>
<span class="line"><span>        end</span></span>
<span class="line"><span>    end</span></span>
<span class="line"><span>    sync posedge \\clk_b</span></span>
<span class="line"><span>      update \\cpu_seen $0\\cpu_seen[0:0]</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>end</span></span>
<span class="line"><span></span></span>
<span class="line"><span>attribute \\top 1</span></span>
<span class="line"><span>module \\top</span></span>
<span class="line"><span>  wire width 1 input 1 \\SCLK</span></span>
<span class="line"><span>  wire width 1 input 2 \\DONE</span></span>
<span class="line"><span>  wire width 2 input 3 \\KEY</span></span>
<span class="line"><span>  wire width 1 output 4 \\LED</span></span>
<span class="line"><span>  wire width 1 \\CLK_FAST</span></span>
<span class="line"><span>  wire width 1 \\jz_unused_pll_LOCK_cg0_u0</span></span>
<span class="line"><span>  wire width 1 \\jz_unused_pll_PHASE_cg0_u0</span></span>
<span class="line"><span>  wire width 1 \\jz_unused_pll_DIV_cg0_u0</span></span>
<span class="line"><span>  wire width 1 \\jz_unused_pll_DIV3_cg0_u0</span></span>
<span class="line"><span>  cell \\rPLL $auto$0_0</span></span>
<span class="line"><span>  parameter \\DEVICE &quot;GW1N-9C&quot;</span></span>
<span class="line"><span>  parameter \\FCLKIN &quot;26.998&quot;</span></span>
<span class="line"><span>  parameter \\IDIV_SEL 2</span></span>
<span class="line"><span>  parameter \\FBDIV_SEL 7</span></span>
<span class="line"><span>  parameter \\ODIV_SEL 8</span></span>
<span class="line"><span>  parameter \\PSDA_SEL &quot;0000&quot;</span></span>
<span class="line"><span>  parameter \\DUTYDA_SEL &quot;1000&quot;</span></span>
<span class="line"><span>  parameter \\DYN_IDIV_SEL &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\DYN_FBDIV_SEL &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\DYN_ODIV_SEL &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\DYN_DA_EN &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\DYN_SDIV_SEL 2</span></span>
<span class="line"><span>  parameter \\CLKOUT_FT_DIR 1</span></span>
<span class="line"><span>  parameter \\CLKOUTP_FT_DIR 1</span></span>
<span class="line"><span>  parameter \\CLKOUT_DLY_STEP 0</span></span>
<span class="line"><span>  parameter \\CLKOUTP_DLY_STEP 0</span></span>
<span class="line"><span>  parameter \\CLKFB_SEL &quot;internal&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUT_BYPASS &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUTP_BYPASS &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUTD_BYPASS &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUTD_SRC &quot;CLKOUT&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUTD3_SRC &quot;CLKOUT&quot;</span></span>
<span class="line"><span>  connect \\CLKIN \\SCLK</span></span>
<span class="line"><span>  connect \\CLKOUT \\CLK_FAST</span></span>
<span class="line"><span>  connect \\LOCK \\jz_unused_pll_LOCK_cg0_u0</span></span>
<span class="line"><span>  connect \\CLKOUTP \\jz_unused_pll_PHASE_cg0_u0</span></span>
<span class="line"><span>  connect \\CLKOUTD \\jz_unused_pll_DIV_cg0_u0</span></span>
<span class="line"><span>  connect \\CLKOUTD3 \\jz_unused_pll_DIV3_cg0_u0</span></span>
<span class="line"><span>  connect \\RESET 1&#39;0</span></span>
<span class="line"><span>  connect \\RESET_P 1&#39;0</span></span>
<span class="line"><span>  connect \\CLKFB 1&#39;0</span></span>
<span class="line"><span>end</span></span>
<span class="line"><span>  wire \\jz_inv_led</span></span>
<span class="line"><span>  cell $not $auto$9</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\jz_inv_led</span></span>
<span class="line"><span>    connect \\Y \\LED [0]</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  cell \\cdc_bit \\u_top</span></span>
<span class="line"><span>    connect \\clk_a \\SCLK</span></span>
<span class="line"><span>    connect \\clk_b \\CLK_FAST</span></span>
<span class="line"><span>    connect \\rst_n \\KEY [0]</span></span>
<span class="line"><span>    connect \\trigger \\KEY [1]</span></span>
<span class="line"><span>    connect \\led \\jz_inv_led</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>end</span></span></code></pre></div></div></div><h3 id="multi‐bit-gray‐code-bus-bus-3-stages" tabindex="-1">Multi‑bit Gray‑code bus (BUS, 3 stages) <a class="header-anchor" href="#multi‐bit-gray‐code-bus-bus-3-stages" aria-label="Permalink to “Multi‑bit Gray‑code bus (BUS, 3 stages)”">​</a></h3><p>An 8‑bit <code>gray_ptr</code> register crosses from <code>clk_a</code> to <code>clk_b</code> using a 3‑stage BUS synchronizer. This is safe only when the source follows Gray‑code or single‑bit‑change discipline.</p><div class="vp-code-group"><div class="tabs"><input type="radio" name="group-479" id="tab-480" checked><label data-title="cdc_bus.jz" for="tab-480">cdc_bus.jz</label><input type="radio" name="group-479" id="tab-481"><label data-title="project.jz" for="tab-481">project.jz</label><input type="radio" name="group-479" id="tab-482"><label data-title="Generated Verilog" for="tab-482">Generated Verilog</label><input type="radio" name="group-479" id="tab-483"><label data-title="Generated RTLIL" for="tab-483">Generated RTLIL</label></div><div class="blocks"><div class="language-jz active"><button title="Copy Code" class="copy"></button><span class="lang">jz</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// CDC BUS example: multi-bit Gray-code synchronizer (3 stages)</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">//</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// Demonstrates crossing an 8-bit Gray-coded pointer from clk_a to clk_b.</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// The BUS synchronizer safely transfers multi-bit values that follow</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// Gray-code discipline (only one bit changes at a time).</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">@module</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> cdc_bus</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    PORT</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] clk_a;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] clk_b;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] rst_n;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        OUT</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> [6] leds;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    REGISTER</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        gray_ptr [8] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;"> 8&#39;h00</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        read_ptr [8] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;"> 8&#39;h00</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    CDC {</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">        BUS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">[3] gray_ptr (clk_a) </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=&gt;</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> gray_ptr_sync (clk_b);</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    ASYNCHRONOUS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        leds </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">&lt;= </span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">read_ptr[5:0];</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    SYNCHRONOUS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">(CLK</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">clk_a RESET</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">rst_n RESET_ACTIVE</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">Low) {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        gray_ptr </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">&lt;= </span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">gray_ptr </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">+</span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;"> 8&#39;h01</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    SYNCHRONOUS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">(CLK</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">clk_b RESET</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">rst_n RESET_ACTIVE</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">Low) {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        read_ptr </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">&lt;= </span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">gray_ptr_sync;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">@endmod</span></span></code></pre></div><div class="language-jz"><button title="Copy Code" class="copy"></button><span class="lang">jz</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">@project</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">(CHIP</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">&quot;GW1NR</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">-</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">9</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">-</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">QN88</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">-</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">C6</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">-</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">I5&quot;) CDC_BUS_EXAMPLE</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    @import</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> &quot;cdc_bus.jz&quot;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">    CLOCKS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        SCLK     </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">period</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">37.04 }; </span><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// 27MHz</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        CLK_FAST;                     </span><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// 108MHz (PLL)</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">    IN_PINS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        SCLK   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">standard</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LVCMOS33 };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        DONE   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">standard</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LVCMOS33 };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        KEY[1] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">standard</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LVCMOS33 };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">    OUT_PINS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[6] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">standard</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LVCMOS33, </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">drive</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">8 };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">    MAP</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        SCLK   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 52;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        KEY[0] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 3;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[0] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 10;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[1] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 11;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[2] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 13;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[3] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 14;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[4] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 15;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[5] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 16;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        DONE   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> IOL14A;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    CLOCK_GEN {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        PLL {</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">            IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  REF_CLK SCLK;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">            OUT</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> BASE    CLK_FAST;</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">            CONFIG</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">                IDIV  </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 2;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">                FBDIV </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 7;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">                ODIV  </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 8;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">            };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    @top</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> cdc_bus {</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] clk_a </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> SCLK;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] clk_b </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> CLK_FAST;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] rst_n </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> KEY[0];</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        OUT</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> [6] leds  </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;"> ~</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LED;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">@endproj</span></span></code></pre></div><div class="language-v"><button title="Copy Code" class="copy"></button><span class="lang">v</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// This Verilog was transpiled from JZ-HDL.</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// jz-hdl version: Version 0.1.7 (c6d52fc)</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// Intended for use with yosys.</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">\`default_nettype none</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">module JZHDL_LIB_CDC_BUS__W8 (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    clk_src,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    clk_dest,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    data_in,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    data_out</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">);</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Ports</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input clk_src;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input clk_dest;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input [7:0] data_in;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    output reg [7:0] data_out;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Signals</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg [7:0] gray_src;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg [7:0] gray_sync1;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg [7:0] gray_sync2;</span></span>
<span class="line"></span>
<span class="line"></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @* begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        data_out[7] = gray_sync2[7];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        data_out[6] = data_out[7] ^ gray_sync2[6];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        data_out[5] = data_out[6] ^ gray_sync2[5];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        data_out[4] = data_out[5] ^ gray_sync2[4];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        data_out[3] = data_out[4] ^ gray_sync2[3];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        data_out[2] = data_out[3] ^ gray_sync2[2];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        data_out[1] = data_out[2] ^ gray_sync2[1];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        data_out[0] = data_out[1] ^ gray_sync2[0];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @(posedge clk_src) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        gray_src &lt;= data_in &gt;&gt; 8&#39;b00000001 ^ data_in;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @(posedge clk_dest) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        gray_sync1 &lt;= gray_src;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        gray_sync2 &lt;= gray_sync1;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">endmodule</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">module cdc_bus (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    clk_a,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    clk_b,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    rst_n,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    leds</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">);</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Ports</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input clk_a;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input clk_b;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input rst_n;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    output reg [5:0] leds;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Signals</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg [7:0] gray_ptr;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg [7:0] read_ptr;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire [7:0] gray_ptr_sync;</span></span>
<span class="line"></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    JZHDL_LIB_CDC_BUS__W8 u_cdc_bus_gray_ptr_sync (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .clk_src(clk_a),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .clk_dest(clk_b),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .data_in(gray_ptr),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .data_out(gray_ptr_sync)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    );</span></span>
<span class="line"></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @* begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        leds = read_ptr[5:0];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @(posedge clk_a) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        if (!rst_n) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            gray_ptr &lt;= 8&#39;b00000000;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        else begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            gray_ptr &lt;= gray_ptr + 8&#39;b00000001;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @(posedge clk_b) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        if (!rst_n) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            read_ptr &lt;= 8&#39;b00000000;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        else begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            read_ptr &lt;= gray_ptr_sync;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">endmodule</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">module top (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    SCLK,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    DONE,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    KEY,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    LED</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">);</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input SCLK;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input DONE;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input KEY;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    output [5:0] LED;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Top-level logical→physical pin mapping</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_bus.clk_a -&gt; SCLK (board 52)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_bus.clk_b -&gt; CLK_FAST (clock gen)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_bus.rst_n -&gt; KEY[0] (board 3)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_bus.leds[5] -&gt; LED[5] (board 16)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_bus.leds[4] -&gt; LED[4] (board 15)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_bus.leds[3] -&gt; LED[3] (board 14)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_bus.leds[2] -&gt; LED[2] (board 13)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_bus.leds[1] -&gt; LED[1] (board 11)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_bus.leds[0] -&gt; LED[0] (board 10)</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire [5:0] jz_inv_leds;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[5] = ~jz_inv_leds[5];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[4] = ~jz_inv_leds[4];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[3] = ~jz_inv_leds[3];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[2] = ~jz_inv_leds[2];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[1] = ~jz_inv_leds[1];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[0] = ~jz_inv_leds[0];</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire CLK_FAST;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire jz_unused_pll_LOCK_cg0_u0;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire jz_unused_pll_PHASE_cg0_u0;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire jz_unused_pll_DIV_cg0_u0;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire jz_unused_pll_DIV3_cg0_u0;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // CLOCK_GEN PLL instantiation (from chip data)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    rPLL #(</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DEVICE(&quot;GW1N-9C&quot;),          // Specify your device</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .FCLKIN(&quot;26.998&quot;),       // Input frequency in MHz</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .IDIV_SEL(2),           // IDIV: Input divider</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .FBDIV_SEL(7),         // FBDIV: Feedback divider</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .ODIV_SEL(8),           // ODIV: Output divider</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .PSDA_SEL(&quot;0000&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DUTYDA_SEL(&quot;1000&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_IDIV_SEL(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_FBDIV_SEL(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_ODIV_SEL(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_DA_EN(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_SDIV_SEL(2),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUT_FT_DIR(1&#39;b1),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTP_FT_DIR(1&#39;b1),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUT_DLY_STEP(0),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTP_DLY_STEP(0),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKFB_SEL(&quot;internal&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUT_BYPASS(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTP_BYPASS(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD_BYPASS(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD_SRC(&quot;CLKOUT&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD3_SRC(&quot;CLKOUT&quot;)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">) u_pll_0_0 (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUT(CLK_FAST),   // Primary output</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .LOCK(jz_unused_pll_LOCK_cg0_u0),     // High when stable</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTP(jz_unused_pll_PHASE_cg0_u0), // Phase shifted output</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD(jz_unused_pll_DIV_cg0_u0),   // Divided output</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD3(jz_unused_pll_DIV3_cg0_u0), // Divided by 3 output</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .RESET(1&#39;b0),        // Reset signal</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .RESET_P(1&#39;b0),      // PLL power down</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKIN(SCLK),  // Reference clock input</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKFB(1&#39;b0)         // External feedback</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">);</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    cdc_bus u_top (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .clk_a(SCLK),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .clk_b(CLK_FAST),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .rst_n(KEY[0]),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .leds({jz_inv_leds[5], jz_inv_leds[4], jz_inv_leds[3], jz_inv_leds[2], jz_inv_leds[1], jz_inv_leds[0]})</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    );</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">endmodule</span></span></code></pre></div><div class="language-il"><button title="Copy Code" class="copy"></button><span class="lang">il</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span># Generated by jz-hdl RTLIL backend</span></span>
<span class="line"><span># jz-hdl version: Version 0.1.7 (c6d52fc)</span></span>
<span class="line"><span></span></span>
<span class="line"><span>attribute \\src &quot;unknown:1&quot;</span></span>
<span class="line"><span>module \\JZHDL_LIB_CDC_BUS__W8</span></span>
<span class="line"><span>  wire width 1 input 1 \\clk_src</span></span>
<span class="line"><span>  wire width 1 input 2 \\clk_dest</span></span>
<span class="line"><span>  wire width 8 input 3 \\data_in</span></span>
<span class="line"><span>  wire width 8 output 4 \\data_out</span></span>
<span class="line"><span>  wire width 8 \\gray_src</span></span>
<span class="line"><span>  wire width 8 \\gray_sync1</span></span>
<span class="line"><span>  wire width 8 \\gray_sync2</span></span>
<span class="line"><span>  wire width 8 $0\\data_out[7:0]</span></span>
<span class="line"><span>  wire width 1 $auto$1</span></span>
<span class="line"><span>  cell $xor $auto$2</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\data_out [7]</span></span>
<span class="line"><span>    connect \\B \\gray_sync2 [6]</span></span>
<span class="line"><span>    connect \\Y $auto$1</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 1 $auto$3</span></span>
<span class="line"><span>  cell $xor $auto$4</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\data_out [6]</span></span>
<span class="line"><span>    connect \\B \\gray_sync2 [5]</span></span>
<span class="line"><span>    connect \\Y $auto$3</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 1 $auto$5</span></span>
<span class="line"><span>  cell $xor $auto$6</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\data_out [5]</span></span>
<span class="line"><span>    connect \\B \\gray_sync2 [4]</span></span>
<span class="line"><span>    connect \\Y $auto$5</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 1 $auto$7</span></span>
<span class="line"><span>  cell $xor $auto$8</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\data_out [4]</span></span>
<span class="line"><span>    connect \\B \\gray_sync2 [3]</span></span>
<span class="line"><span>    connect \\Y $auto$7</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 1 $auto$9</span></span>
<span class="line"><span>  cell $xor $auto$10</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\data_out [3]</span></span>
<span class="line"><span>    connect \\B \\gray_sync2 [2]</span></span>
<span class="line"><span>    connect \\Y $auto$9</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 1 $auto$11</span></span>
<span class="line"><span>  cell $xor $auto$12</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\data_out [2]</span></span>
<span class="line"><span>    connect \\B \\gray_sync2 [1]</span></span>
<span class="line"><span>    connect \\Y $auto$11</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 1 $auto$13</span></span>
<span class="line"><span>  cell $xor $auto$14</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\data_out [1]</span></span>
<span class="line"><span>    connect \\B \\gray_sync2 [0]</span></span>
<span class="line"><span>    connect \\Y $auto$13</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  process $proc$async$15</span></span>
<span class="line"><span>    assign $0\\data_out[7:0] [7] \\gray_sync2 [7]</span></span>
<span class="line"><span>    assign $0\\data_out[7:0] [6] $auto$1</span></span>
<span class="line"><span>    assign $0\\data_out[7:0] [5] $auto$3</span></span>
<span class="line"><span>    assign $0\\data_out[7:0] [4] $auto$5</span></span>
<span class="line"><span>    assign $0\\data_out[7:0] [3] $auto$7</span></span>
<span class="line"><span>    assign $0\\data_out[7:0] [2] $auto$9</span></span>
<span class="line"><span>    assign $0\\data_out[7:0] [1] $auto$11</span></span>
<span class="line"><span>    assign $0\\data_out[7:0] [0] $auto$13</span></span>
<span class="line"><span>    sync always</span></span>
<span class="line"><span>      update \\data_out $0\\data_out[7:0]</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 8 $0\\gray_src[7:0]</span></span>
<span class="line"><span>  wire width 8 $auto$16</span></span>
<span class="line"><span>  cell $shr $auto$17</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 8</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 8</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 8</span></span>
<span class="line"><span>    connect \\A \\data_in</span></span>
<span class="line"><span>    connect \\B 8&#39;00000001</span></span>
<span class="line"><span>    connect \\Y $auto$16</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 8 $auto$18</span></span>
<span class="line"><span>  cell $xor $auto$19</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 8</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 8</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 8</span></span>
<span class="line"><span>    connect \\A $auto$16</span></span>
<span class="line"><span>    connect \\B \\data_in</span></span>
<span class="line"><span>    connect \\Y $auto$18</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  process $proc$clk0$20</span></span>
<span class="line"><span>    assign $0\\gray_src[7:0] \\gray_src</span></span>
<span class="line"><span>    assign $0\\gray_src[7:0] $auto$18</span></span>
<span class="line"><span>    sync posedge \\clk_src</span></span>
<span class="line"><span>      update \\gray_src $0\\gray_src[7:0]</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 8 $0\\gray_sync1[7:0]</span></span>
<span class="line"><span>  wire width 8 $0\\gray_sync2[7:0]</span></span>
<span class="line"><span>  process $proc$clk1$21</span></span>
<span class="line"><span>    assign $0\\gray_sync1[7:0] \\gray_sync1</span></span>
<span class="line"><span>    assign $0\\gray_sync2[7:0] \\gray_sync2</span></span>
<span class="line"><span>    assign $0\\gray_sync1[7:0] \\gray_src</span></span>
<span class="line"><span>    assign $0\\gray_sync2[7:0] \\gray_sync1</span></span>
<span class="line"><span>    sync posedge \\clk_dest</span></span>
<span class="line"><span>      update \\gray_sync1 $0\\gray_sync1[7:0]</span></span>
<span class="line"><span>      update \\gray_sync2 $0\\gray_sync2[7:0]</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>end</span></span>
<span class="line"><span></span></span>
<span class="line"><span>attribute \\src &quot;jz-hdl:7&quot;</span></span>
<span class="line"><span>module \\cdc_bus</span></span>
<span class="line"><span>  wire width 1 input 1 \\clk_a</span></span>
<span class="line"><span>  wire width 1 input 2 \\clk_b</span></span>
<span class="line"><span>  wire width 1 input 3 \\rst_n</span></span>
<span class="line"><span>  wire width 6 output 4 \\leds</span></span>
<span class="line"><span>  wire width 8 \\gray_ptr</span></span>
<span class="line"><span>  wire width 8 \\read_ptr</span></span>
<span class="line"><span>  wire width 8 \\gray_ptr_sync</span></span>
<span class="line"><span>  cell \\JZHDL_LIB_CDC_BUS__W8 \\u_cdc_bus_gray_ptr_sync</span></span>
<span class="line"><span>    connect \\clk_src \\clk_a</span></span>
<span class="line"><span>    connect \\clk_dest \\clk_b</span></span>
<span class="line"><span>    connect \\data_in \\gray_ptr</span></span>
<span class="line"><span>    connect \\data_out \\gray_ptr_sync</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 6 $0\\leds[5:0]</span></span>
<span class="line"><span>  process $proc$async$22</span></span>
<span class="line"><span>    assign $0\\leds[5:0] \\read_ptr [5:0]</span></span>
<span class="line"><span>    sync always</span></span>
<span class="line"><span>      update \\leds $0\\leds[5:0]</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 8 $0\\gray_ptr[7:0]</span></span>
<span class="line"><span>  wire width 1 $auto$23</span></span>
<span class="line"><span>  cell $logic_not $auto$24</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\rst_n</span></span>
<span class="line"><span>    connect \\Y $auto$23</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 8 $auto$25</span></span>
<span class="line"><span>  cell $add $auto$26</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 8</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 8</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 8</span></span>
<span class="line"><span>    connect \\A \\gray_ptr</span></span>
<span class="line"><span>    connect \\B 8&#39;00000001</span></span>
<span class="line"><span>    connect \\Y $auto$25</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  process $proc$clk0$27</span></span>
<span class="line"><span>    assign $0\\gray_ptr[7:0] \\gray_ptr</span></span>
<span class="line"><span>    switch $auto$23</span></span>
<span class="line"><span>      case 1&#39;1</span></span>
<span class="line"><span>        assign $0\\gray_ptr[7:0] 8&#39;00000000</span></span>
<span class="line"><span>      case</span></span>
<span class="line"><span>        assign $0\\gray_ptr[7:0] $auto$25</span></span>
<span class="line"><span>    end</span></span>
<span class="line"><span>    sync posedge \\clk_a</span></span>
<span class="line"><span>      update \\gray_ptr $0\\gray_ptr[7:0]</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 8 $0\\read_ptr[7:0]</span></span>
<span class="line"><span>  wire width 1 $auto$28</span></span>
<span class="line"><span>  cell $logic_not $auto$29</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\rst_n</span></span>
<span class="line"><span>    connect \\Y $auto$28</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  process $proc$clk1$30</span></span>
<span class="line"><span>    assign $0\\read_ptr[7:0] \\read_ptr</span></span>
<span class="line"><span>    switch $auto$28</span></span>
<span class="line"><span>      case 1&#39;1</span></span>
<span class="line"><span>        assign $0\\read_ptr[7:0] 8&#39;00000000</span></span>
<span class="line"><span>      case</span></span>
<span class="line"><span>        assign $0\\read_ptr[7:0] \\gray_ptr_sync</span></span>
<span class="line"><span>    end</span></span>
<span class="line"><span>    sync posedge \\clk_b</span></span>
<span class="line"><span>      update \\read_ptr $0\\read_ptr[7:0]</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>end</span></span>
<span class="line"><span></span></span>
<span class="line"><span>attribute \\top 1</span></span>
<span class="line"><span>module \\top</span></span>
<span class="line"><span>  wire width 1 input 1 \\SCLK</span></span>
<span class="line"><span>  wire width 1 input 2 \\DONE</span></span>
<span class="line"><span>  wire width 1 input 3 \\KEY</span></span>
<span class="line"><span>  wire width 6 output 4 \\LED</span></span>
<span class="line"><span>  wire width 1 \\CLK_FAST</span></span>
<span class="line"><span>  wire width 1 \\jz_unused_pll_LOCK_cg0_u0</span></span>
<span class="line"><span>  wire width 1 \\jz_unused_pll_PHASE_cg0_u0</span></span>
<span class="line"><span>  wire width 1 \\jz_unused_pll_DIV_cg0_u0</span></span>
<span class="line"><span>  wire width 1 \\jz_unused_pll_DIV3_cg0_u0</span></span>
<span class="line"><span>  cell \\rPLL $auto$0_0</span></span>
<span class="line"><span>  parameter \\DEVICE &quot;GW1N-9C&quot;</span></span>
<span class="line"><span>  parameter \\FCLKIN &quot;26.998&quot;</span></span>
<span class="line"><span>  parameter \\IDIV_SEL 2</span></span>
<span class="line"><span>  parameter \\FBDIV_SEL 7</span></span>
<span class="line"><span>  parameter \\ODIV_SEL 8</span></span>
<span class="line"><span>  parameter \\PSDA_SEL &quot;0000&quot;</span></span>
<span class="line"><span>  parameter \\DUTYDA_SEL &quot;1000&quot;</span></span>
<span class="line"><span>  parameter \\DYN_IDIV_SEL &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\DYN_FBDIV_SEL &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\DYN_ODIV_SEL &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\DYN_DA_EN &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\DYN_SDIV_SEL 2</span></span>
<span class="line"><span>  parameter \\CLKOUT_FT_DIR 1</span></span>
<span class="line"><span>  parameter \\CLKOUTP_FT_DIR 1</span></span>
<span class="line"><span>  parameter \\CLKOUT_DLY_STEP 0</span></span>
<span class="line"><span>  parameter \\CLKOUTP_DLY_STEP 0</span></span>
<span class="line"><span>  parameter \\CLKFB_SEL &quot;internal&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUT_BYPASS &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUTP_BYPASS &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUTD_BYPASS &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUTD_SRC &quot;CLKOUT&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUTD3_SRC &quot;CLKOUT&quot;</span></span>
<span class="line"><span>  connect \\CLKIN \\SCLK</span></span>
<span class="line"><span>  connect \\CLKOUT \\CLK_FAST</span></span>
<span class="line"><span>  connect \\LOCK \\jz_unused_pll_LOCK_cg0_u0</span></span>
<span class="line"><span>  connect \\CLKOUTP \\jz_unused_pll_PHASE_cg0_u0</span></span>
<span class="line"><span>  connect \\CLKOUTD \\jz_unused_pll_DIV_cg0_u0</span></span>
<span class="line"><span>  connect \\CLKOUTD3 \\jz_unused_pll_DIV3_cg0_u0</span></span>
<span class="line"><span>  connect \\RESET 1&#39;0</span></span>
<span class="line"><span>  connect \\RESET_P 1&#39;0</span></span>
<span class="line"><span>  connect \\CLKFB 1&#39;0</span></span>
<span class="line"><span>end</span></span>
<span class="line"><span>  cell \\cdc_bus \\u_top</span></span>
<span class="line"><span>    connect \\clk_a \\SCLK</span></span>
<span class="line"><span>    connect \\clk_b \\CLK_FAST</span></span>
<span class="line"><span>    connect \\rst_n \\KEY [0]</span></span>
<span class="line"><span>    connect \\leds { \\LED [5] \\LED [4] \\LED [3] \\LED [2] \\LED [1] \\LED [0] }</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>end</span></span></code></pre></div></div></div><h3 id="wide-arbitrary-data-via-fifo-fifo-4-stages" tabindex="-1">Wide arbitrary data via FIFO (FIFO, 4 stages) <a class="header-anchor" href="#wide-arbitrary-data-via-fifo-fifo-4-stages" aria-label="Permalink to “Wide arbitrary data via FIFO (FIFO, 4 stages)”">​</a></h3><p>A 64‑bit <code>packet_word</code> register crosses from <code>clk_a</code> to <code>clk_b</code> using a 4‑stage FIFO synchronizer. Unlike BUS, FIFO handles arbitrary multi‑bit changes safely.</p><div class="vp-code-group"><div class="tabs"><input type="radio" name="group-491" id="tab-492" checked><label data-title="cdc_fifo.jz" for="tab-492">cdc_fifo.jz</label><input type="radio" name="group-491" id="tab-493"><label data-title="project.jz" for="tab-493">project.jz</label><input type="radio" name="group-491" id="tab-494"><label data-title="Generated Verilog" for="tab-494">Generated Verilog</label><input type="radio" name="group-491" id="tab-495"><label data-title="Generated RTLIL" for="tab-495">Generated RTLIL</label></div><div class="blocks"><div class="language-jz active"><button title="Copy Code" class="copy"></button><span class="lang">jz</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// CDC FIFO example: wide arbitrary data transfer (4 stages)</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">//</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// Demonstrates crossing a 64-bit data word from clk_a to clk_b.</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// The FIFO synchronizer handles arbitrary multi-bit changes safely,</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// unlike BUS which requires Gray-code discipline.</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">@module</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> cdc_fifo</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    PORT</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] clk_a;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] clk_b;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] rst_n;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        OUT</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> [6] leds;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    REGISTER</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        packet_word [64] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;"> 64&#39;h0000_0000_0000_0000</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        data_out    [64] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;"> 64&#39;h0000_0000_0000_0000</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    CDC {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        FIFO[4] packet_word (clk_a) </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=&gt;</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> packet_view (clk_b);</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    ASYNCHRONOUS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        leds </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">&lt;= </span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">data_out[5:0];</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    SYNCHRONOUS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">(CLK</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">clk_a RESET</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">rst_n RESET_ACTIVE</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">Low) {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        packet_word </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">&lt;= </span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">packet_word </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">+</span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;"> 64&#39;h0000_0000_0000_0001</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    SYNCHRONOUS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">(CLK</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">clk_b RESET</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">rst_n RESET_ACTIVE</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">Low) {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        data_out </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">&lt;= </span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">packet_view;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">@endmod</span></span></code></pre></div><div class="language-jz"><button title="Copy Code" class="copy"></button><span class="lang">jz</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">@project</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">(CHIP</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">&quot;GW1NR</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">-</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">9</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">-</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">QN88</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">-</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">C6</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">-</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">I5&quot;) CDC_FIFO_EXAMPLE</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    @import</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> &quot;cdc_fifo.jz&quot;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">    CLOCKS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        SCLK     </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">period</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">37.04 }; </span><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// 27MHz</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        CLK_FAST;                     </span><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// 108MHz (PLL)</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">    IN_PINS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        SCLK   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">standard</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LVCMOS33 };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        DONE   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">standard</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LVCMOS33 };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        KEY[1] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">standard</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LVCMOS33 };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">    OUT_PINS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[6] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">standard</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LVCMOS33, </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">drive</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">8 };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">    MAP</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        SCLK   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 52;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        KEY[0] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 3;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[0] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 10;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[1] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 11;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[2] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 13;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[3] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 14;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[4] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 15;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[5] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 16;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        DONE   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> IOL14A;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    CLOCK_GEN {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        PLL {</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">            IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  REF_CLK SCLK;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">            OUT</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> BASE    CLK_FAST;</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">            CONFIG</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">                IDIV  </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 2;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">                FBDIV </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 7;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">                ODIV  </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 8;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">            };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    @top</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> cdc_fifo {</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] clk_a </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> SCLK;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] clk_b </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> CLK_FAST;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] rst_n </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> KEY[0];</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        OUT</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> [6] leds  </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;"> ~</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LED;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">@endproj</span></span></code></pre></div><div class="language-v"><button title="Copy Code" class="copy"></button><span class="lang">v</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// This Verilog was transpiled from JZ-HDL.</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// jz-hdl version: Version 0.1.7 (c6d52fc)</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// Intended for use with yosys.</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">\`default_nettype none</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">module JZHDL_LIB_CDC_FIFO__W64 (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    clk_wr,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    clk_rd,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    rst_wr,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    rst_rd,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    data_in,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    write_en,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    read_en,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    data_out,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    full,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    empty</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">);</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Ports</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input clk_wr;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input clk_rd;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input rst_wr;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input rst_rd;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input [63:0] data_in;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input write_en;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input read_en;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    output [63:0] data_out;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    output full;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    output empty;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Signals</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg [4:0] wr_ptr_bin;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg [4:0] wr_ptr_gray;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg [4:0] rd_ptr_bin;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg [4:0] rd_ptr_gray;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg [4:0] wr_ptr_gray_sync1;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg [4:0] wr_ptr_gray_sync2;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg [4:0] rd_ptr_gray_sync1;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg [4:0] rd_ptr_gray_sync2;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Memories</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    (* ram_style = &quot;distributed&quot; *) reg [63:0] mem[0:15];</span></span>
<span class="line"></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign data_out = mem[rd_ptr_bin[3:0]];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign full = wr_ptr_gray == {~rd_ptr_gray_sync2[4:3], rd_ptr_gray_sync2[2:0]};</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign empty = rd_ptr_gray == wr_ptr_gray_sync2;</span></span>
<span class="line"></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @(posedge clk_wr or posedge rst_wr) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        if (rst_wr) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            wr_ptr_bin &lt;= 5&#39;b00000;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            wr_ptr_gray &lt;= 5&#39;b00000;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        else begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            if (write_en &amp;&amp; ~full) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">                mem[wr_ptr_bin[3:0]] &lt;= data_in;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">                wr_ptr_bin &lt;= wr_ptr_bin + 5&#39;b00001;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">                wr_ptr_gray &lt;= wr_ptr_bin + 5&#39;b00001 &gt;&gt; 5&#39;b00001 ^ wr_ptr_bin + 5&#39;b00001;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @(posedge clk_rd or posedge rst_rd) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        if (rst_rd) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            rd_ptr_bin &lt;= 5&#39;b00000;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            rd_ptr_gray &lt;= 5&#39;b00000;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        else begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            if (read_en &amp;&amp; ~empty) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">                rd_ptr_bin &lt;= rd_ptr_bin + 5&#39;b00001;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">                rd_ptr_gray &lt;= rd_ptr_bin + 5&#39;b00001 &gt;&gt; 5&#39;b00001 ^ rd_ptr_bin + 5&#39;b00001;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @(posedge clk_wr or posedge rst_wr) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        if (rst_wr) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            rd_ptr_gray_sync1 &lt;= 5&#39;b00000;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            rd_ptr_gray_sync2 &lt;= 5&#39;b00000;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        else begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            rd_ptr_gray_sync1 &lt;= rd_ptr_gray;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            rd_ptr_gray_sync2 &lt;= rd_ptr_gray_sync1;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @(posedge clk_rd or posedge rst_rd) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        if (rst_rd) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            wr_ptr_gray_sync1 &lt;= 5&#39;b00000;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            wr_ptr_gray_sync2 &lt;= 5&#39;b00000;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        else begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            wr_ptr_gray_sync1 &lt;= wr_ptr_gray;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            wr_ptr_gray_sync2 &lt;= wr_ptr_gray_sync1;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">endmodule</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">module cdc_fifo (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    clk_a,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    clk_b,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    rst_n,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    leds</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">);</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Ports</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input clk_a;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input clk_b;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input rst_n;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    output reg [5:0] leds;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Signals</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg [63:0] packet_word;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg [63:0] data_out;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire [63:0] packet_view;</span></span>
<span class="line"></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    JZHDL_LIB_CDC_FIFO__W64 u_cdc_fifo_packet_view (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .clk_wr(clk_a),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .clk_rd(clk_b),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .rst_wr(rst_n),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .rst_rd(rst_n),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .data_in(packet_word),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .write_en(1&#39;b1),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .read_en(1&#39;b1),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .data_out(packet_view)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    );</span></span>
<span class="line"></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @* begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        leds = data_out[5:0];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @(posedge clk_a) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        if (!rst_n) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            packet_word &lt;= 64&#39;b0000000000000000000000000000000000000000000000000000000000000000;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        else begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            packet_word &lt;= packet_word + 64&#39;b0000000000000000000000000000000000000000000000000000000000000001;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @(posedge clk_b) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        if (!rst_n) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            data_out &lt;= 64&#39;b0000000000000000000000000000000000000000000000000000000000000000;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        else begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            data_out &lt;= packet_view;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">endmodule</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">module top (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    SCLK,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    DONE,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    KEY,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    LED</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">);</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input SCLK;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input DONE;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input KEY;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    output [5:0] LED;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Top-level logical→physical pin mapping</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_fifo.clk_a -&gt; SCLK (board 52)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_fifo.clk_b -&gt; CLK_FAST (clock gen)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_fifo.rst_n -&gt; KEY[0] (board 3)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_fifo.leds[5] -&gt; LED[5] (board 16)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_fifo.leds[4] -&gt; LED[4] (board 15)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_fifo.leds[3] -&gt; LED[3] (board 14)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_fifo.leds[2] -&gt; LED[2] (board 13)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_fifo.leds[1] -&gt; LED[1] (board 11)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_fifo.leds[0] -&gt; LED[0] (board 10)</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire [5:0] jz_inv_leds;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[5] = ~jz_inv_leds[5];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[4] = ~jz_inv_leds[4];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[3] = ~jz_inv_leds[3];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[2] = ~jz_inv_leds[2];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[1] = ~jz_inv_leds[1];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[0] = ~jz_inv_leds[0];</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire CLK_FAST;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire jz_unused_pll_LOCK_cg0_u0;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire jz_unused_pll_PHASE_cg0_u0;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire jz_unused_pll_DIV_cg0_u0;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire jz_unused_pll_DIV3_cg0_u0;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // CLOCK_GEN PLL instantiation (from chip data)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    rPLL #(</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DEVICE(&quot;GW1N-9C&quot;),          // Specify your device</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .FCLKIN(&quot;26.998&quot;),       // Input frequency in MHz</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .IDIV_SEL(2),           // IDIV: Input divider</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .FBDIV_SEL(7),         // FBDIV: Feedback divider</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .ODIV_SEL(8),           // ODIV: Output divider</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .PSDA_SEL(&quot;0000&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DUTYDA_SEL(&quot;1000&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_IDIV_SEL(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_FBDIV_SEL(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_ODIV_SEL(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_DA_EN(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_SDIV_SEL(2),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUT_FT_DIR(1&#39;b1),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTP_FT_DIR(1&#39;b1),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUT_DLY_STEP(0),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTP_DLY_STEP(0),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKFB_SEL(&quot;internal&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUT_BYPASS(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTP_BYPASS(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD_BYPASS(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD_SRC(&quot;CLKOUT&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD3_SRC(&quot;CLKOUT&quot;)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">) u_pll_0_0 (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUT(CLK_FAST),   // Primary output</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .LOCK(jz_unused_pll_LOCK_cg0_u0),     // High when stable</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTP(jz_unused_pll_PHASE_cg0_u0), // Phase shifted output</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD(jz_unused_pll_DIV_cg0_u0),   // Divided output</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD3(jz_unused_pll_DIV3_cg0_u0), // Divided by 3 output</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .RESET(1&#39;b0),        // Reset signal</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .RESET_P(1&#39;b0),      // PLL power down</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKIN(SCLK),  // Reference clock input</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKFB(1&#39;b0)         // External feedback</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">);</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    cdc_fifo u_top (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .clk_a(SCLK),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .clk_b(CLK_FAST),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .rst_n(KEY[0]),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .leds({jz_inv_leds[5], jz_inv_leds[4], jz_inv_leds[3], jz_inv_leds[2], jz_inv_leds[1], jz_inv_leds[0]})</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    );</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">endmodule</span></span></code></pre></div><div class="language-il"><button title="Copy Code" class="copy"></button><span class="lang">il</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span># Generated by jz-hdl RTLIL backend</span></span>
<span class="line"><span># jz-hdl version: Version 0.1.7 (c6d52fc)</span></span>
<span class="line"><span></span></span>
<span class="line"><span>attribute \\src &quot;unknown:1&quot;</span></span>
<span class="line"><span>module \\JZHDL_LIB_CDC_FIFO__W64</span></span>
<span class="line"><span>  wire width 1 input 1 \\clk_wr</span></span>
<span class="line"><span>  wire width 1 input 2 \\clk_rd</span></span>
<span class="line"><span>  wire width 1 input 3 \\rst_wr</span></span>
<span class="line"><span>  wire width 1 input 4 \\rst_rd</span></span>
<span class="line"><span>  wire width 64 input 5 \\data_in</span></span>
<span class="line"><span>  wire width 1 input 6 \\write_en</span></span>
<span class="line"><span>  wire width 1 input 7 \\read_en</span></span>
<span class="line"><span>  wire width 64 output 8 \\data_out</span></span>
<span class="line"><span>  wire width 1 output 9 \\full</span></span>
<span class="line"><span>  wire width 1 output 10 \\empty</span></span>
<span class="line"><span>  wire width 5 \\wr_ptr_bin</span></span>
<span class="line"><span>  wire width 5 \\wr_ptr_gray</span></span>
<span class="line"><span>  wire width 5 \\rd_ptr_bin</span></span>
<span class="line"><span>  wire width 5 \\rd_ptr_gray</span></span>
<span class="line"><span>  wire width 5 \\wr_ptr_gray_sync1</span></span>
<span class="line"><span>  wire width 5 \\wr_ptr_gray_sync2</span></span>
<span class="line"><span>  wire width 5 \\rd_ptr_gray_sync1</span></span>
<span class="line"><span>  wire width 5 \\rd_ptr_gray_sync2</span></span>
<span class="line"><span>  memory width 64 size 16 \\mem</span></span>
<span class="line"><span>  wire width 64 $auto$1</span></span>
<span class="line"><span>  cell $memrd_v2 $auto$2</span></span>
<span class="line"><span>    parameter \\MEMID &quot;\\\\mem&quot;</span></span>
<span class="line"><span>    parameter \\ABITS 4</span></span>
<span class="line"><span>    parameter \\WIDTH 64</span></span>
<span class="line"><span>    parameter \\CLK_ENABLE 0</span></span>
<span class="line"><span>    parameter \\CLK_POLARITY 1</span></span>
<span class="line"><span>    parameter \\TRANSPARENCY_MASK 0</span></span>
<span class="line"><span>    parameter \\COLLISION_X_MASK 0</span></span>
<span class="line"><span>    parameter \\CE_OVER_SRST 0</span></span>
<span class="line"><span>    parameter \\ARST_VALUE 64&#39;0000000000000000000000000000000000000000000000000000000000000000</span></span>
<span class="line"><span>    parameter \\SRST_VALUE 64&#39;0000000000000000000000000000000000000000000000000000000000000000</span></span>
<span class="line"><span>    parameter \\INIT_VALUE 64&#39;xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx</span></span>
<span class="line"><span>    connect \\ADDR \\rd_ptr_bin [3:0]</span></span>
<span class="line"><span>    connect \\DATA $auto$1</span></span>
<span class="line"><span>    connect \\EN 1&#39;1</span></span>
<span class="line"><span>    connect \\ARST 1&#39;0</span></span>
<span class="line"><span>    connect \\SRST 1&#39;0</span></span>
<span class="line"><span>    connect \\CLK 1&#39;0</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  connect \\data_out $auto$1</span></span>
<span class="line"><span>  wire width 2 $auto$3</span></span>
<span class="line"><span>  cell $not $auto$4</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 2</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 2</span></span>
<span class="line"><span>    connect \\A \\rd_ptr_gray_sync2 [4:3]</span></span>
<span class="line"><span>    connect \\Y $auto$3</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 1 $auto$5</span></span>
<span class="line"><span>  cell $eq $auto$6</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 5</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 5</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\wr_ptr_gray</span></span>
<span class="line"><span>    connect \\B { $auto$3 \\rd_ptr_gray_sync2 [2:0] }</span></span>
<span class="line"><span>    connect \\Y $auto$5</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  connect \\full $auto$5</span></span>
<span class="line"><span>  wire width 1 $auto$7</span></span>
<span class="line"><span>  cell $eq $auto$8</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 5</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 5</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\rd_ptr_gray</span></span>
<span class="line"><span>    connect \\B \\wr_ptr_gray_sync2</span></span>
<span class="line"><span>    connect \\Y $auto$7</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  connect \\empty $auto$7</span></span>
<span class="line"><span>  wire width 1 $auto$9</span></span>
<span class="line"><span>  cell $not $auto$10</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\full</span></span>
<span class="line"><span>    connect \\Y $auto$9</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 1 $auto$11</span></span>
<span class="line"><span>  cell $logic_and $auto$12</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\write_en</span></span>
<span class="line"><span>    connect \\B $auto$9</span></span>
<span class="line"><span>    connect \\Y $auto$11</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  cell $memwr_v2 $auto$13</span></span>
<span class="line"><span>    parameter \\MEMID &quot;\\\\mem&quot;</span></span>
<span class="line"><span>    parameter \\ABITS 4</span></span>
<span class="line"><span>    parameter \\WIDTH 64</span></span>
<span class="line"><span>    parameter \\CLK_ENABLE 1</span></span>
<span class="line"><span>    parameter \\CLK_POLARITY 1</span></span>
<span class="line"><span>    parameter \\PORTID 0</span></span>
<span class="line"><span>    parameter \\PRIORITY_MASK 0</span></span>
<span class="line"><span>    connect \\ADDR \\wr_ptr_bin [3:0]</span></span>
<span class="line"><span>    connect \\DATA \\data_in</span></span>
<span class="line"><span>    connect \\EN { $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 $auto$11 }</span></span>
<span class="line"><span>    connect \\CLK \\clk_wr</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 5 $0\\wr_ptr_bin[4:0]</span></span>
<span class="line"><span>  wire width 5 $0\\wr_ptr_gray[4:0]</span></span>
<span class="line"><span>  wire width 1 $auto$14</span></span>
<span class="line"><span>  cell $not $auto$15</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\full</span></span>
<span class="line"><span>    connect \\Y $auto$14</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 1 $auto$16</span></span>
<span class="line"><span>  cell $logic_and $auto$17</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\write_en</span></span>
<span class="line"><span>    connect \\B $auto$14</span></span>
<span class="line"><span>    connect \\Y $auto$16</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 5 $auto$18</span></span>
<span class="line"><span>  cell $add $auto$19</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 5</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 5</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 5</span></span>
<span class="line"><span>    connect \\A \\wr_ptr_bin</span></span>
<span class="line"><span>    connect \\B 5&#39;00001</span></span>
<span class="line"><span>    connect \\Y $auto$18</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 5 $auto$20</span></span>
<span class="line"><span>  cell $add $auto$21</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 5</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 5</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 5</span></span>
<span class="line"><span>    connect \\A \\wr_ptr_bin</span></span>
<span class="line"><span>    connect \\B 5&#39;00001</span></span>
<span class="line"><span>    connect \\Y $auto$20</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 5 $auto$22</span></span>
<span class="line"><span>  cell $shr $auto$23</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 5</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 5</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 5</span></span>
<span class="line"><span>    connect \\A $auto$20</span></span>
<span class="line"><span>    connect \\B 5&#39;00001</span></span>
<span class="line"><span>    connect \\Y $auto$22</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 5 $auto$24</span></span>
<span class="line"><span>  cell $add $auto$25</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 5</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 5</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 5</span></span>
<span class="line"><span>    connect \\A \\wr_ptr_bin</span></span>
<span class="line"><span>    connect \\B 5&#39;00001</span></span>
<span class="line"><span>    connect \\Y $auto$24</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 5 $auto$26</span></span>
<span class="line"><span>  cell $xor $auto$27</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 5</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 5</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 5</span></span>
<span class="line"><span>    connect \\A $auto$22</span></span>
<span class="line"><span>    connect \\B $auto$24</span></span>
<span class="line"><span>    connect \\Y $auto$26</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  process $proc$clk0$28</span></span>
<span class="line"><span>    assign $0\\wr_ptr_bin[4:0] \\wr_ptr_bin</span></span>
<span class="line"><span>    assign $0\\wr_ptr_gray[4:0] \\wr_ptr_gray</span></span>
<span class="line"><span>    switch \\rst_wr</span></span>
<span class="line"><span>      case 1&#39;1</span></span>
<span class="line"><span>        assign $0\\wr_ptr_bin[4:0] 5&#39;00000</span></span>
<span class="line"><span>        assign $0\\wr_ptr_gray[4:0] 5&#39;00000</span></span>
<span class="line"><span>      case</span></span>
<span class="line"><span>        switch $auto$16</span></span>
<span class="line"><span>          case 1&#39;1</span></span>
<span class="line"><span>            # memory write: mem (handled by $memwr_v2 cell)</span></span>
<span class="line"><span>            assign $0\\wr_ptr_bin[4:0] $auto$18</span></span>
<span class="line"><span>            assign $0\\wr_ptr_gray[4:0] $auto$26</span></span>
<span class="line"><span>          case</span></span>
<span class="line"><span>        end</span></span>
<span class="line"><span>    end</span></span>
<span class="line"><span>    sync posedge \\clk_wr</span></span>
<span class="line"><span>      update \\wr_ptr_bin $0\\wr_ptr_bin[4:0]</span></span>
<span class="line"><span>      update \\wr_ptr_gray $0\\wr_ptr_gray[4:0]</span></span>
<span class="line"><span>    sync high \\rst_wr</span></span>
<span class="line"><span>      update \\wr_ptr_bin 5&#39;00000</span></span>
<span class="line"><span>      update \\wr_ptr_gray 5&#39;00000</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 5 $0\\rd_ptr_bin[4:0]</span></span>
<span class="line"><span>  wire width 5 $0\\rd_ptr_gray[4:0]</span></span>
<span class="line"><span>  wire width 1 $auto$29</span></span>
<span class="line"><span>  cell $not $auto$30</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\empty</span></span>
<span class="line"><span>    connect \\Y $auto$29</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 1 $auto$31</span></span>
<span class="line"><span>  cell $logic_and $auto$32</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\read_en</span></span>
<span class="line"><span>    connect \\B $auto$29</span></span>
<span class="line"><span>    connect \\Y $auto$31</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 5 $auto$33</span></span>
<span class="line"><span>  cell $add $auto$34</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 5</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 5</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 5</span></span>
<span class="line"><span>    connect \\A \\rd_ptr_bin</span></span>
<span class="line"><span>    connect \\B 5&#39;00001</span></span>
<span class="line"><span>    connect \\Y $auto$33</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 5 $auto$35</span></span>
<span class="line"><span>  cell $add $auto$36</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 5</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 5</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 5</span></span>
<span class="line"><span>    connect \\A \\rd_ptr_bin</span></span>
<span class="line"><span>    connect \\B 5&#39;00001</span></span>
<span class="line"><span>    connect \\Y $auto$35</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 5 $auto$37</span></span>
<span class="line"><span>  cell $shr $auto$38</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 5</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 5</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 5</span></span>
<span class="line"><span>    connect \\A $auto$35</span></span>
<span class="line"><span>    connect \\B 5&#39;00001</span></span>
<span class="line"><span>    connect \\Y $auto$37</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 5 $auto$39</span></span>
<span class="line"><span>  cell $add $auto$40</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 5</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 5</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 5</span></span>
<span class="line"><span>    connect \\A \\rd_ptr_bin</span></span>
<span class="line"><span>    connect \\B 5&#39;00001</span></span>
<span class="line"><span>    connect \\Y $auto$39</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 5 $auto$41</span></span>
<span class="line"><span>  cell $xor $auto$42</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 5</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 5</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 5</span></span>
<span class="line"><span>    connect \\A $auto$37</span></span>
<span class="line"><span>    connect \\B $auto$39</span></span>
<span class="line"><span>    connect \\Y $auto$41</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  process $proc$clk1$43</span></span>
<span class="line"><span>    assign $0\\rd_ptr_bin[4:0] \\rd_ptr_bin</span></span>
<span class="line"><span>    assign $0\\rd_ptr_gray[4:0] \\rd_ptr_gray</span></span>
<span class="line"><span>    switch \\rst_rd</span></span>
<span class="line"><span>      case 1&#39;1</span></span>
<span class="line"><span>        assign $0\\rd_ptr_bin[4:0] 5&#39;00000</span></span>
<span class="line"><span>        assign $0\\rd_ptr_gray[4:0] 5&#39;00000</span></span>
<span class="line"><span>      case</span></span>
<span class="line"><span>        switch $auto$31</span></span>
<span class="line"><span>          case 1&#39;1</span></span>
<span class="line"><span>            assign $0\\rd_ptr_bin[4:0] $auto$33</span></span>
<span class="line"><span>            assign $0\\rd_ptr_gray[4:0] $auto$41</span></span>
<span class="line"><span>          case</span></span>
<span class="line"><span>        end</span></span>
<span class="line"><span>    end</span></span>
<span class="line"><span>    sync posedge \\clk_rd</span></span>
<span class="line"><span>      update \\rd_ptr_bin $0\\rd_ptr_bin[4:0]</span></span>
<span class="line"><span>      update \\rd_ptr_gray $0\\rd_ptr_gray[4:0]</span></span>
<span class="line"><span>    sync high \\rst_rd</span></span>
<span class="line"><span>      update \\rd_ptr_bin 5&#39;00000</span></span>
<span class="line"><span>      update \\rd_ptr_gray 5&#39;00000</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 5 $0\\rd_ptr_gray_sync1[4:0]</span></span>
<span class="line"><span>  wire width 5 $0\\rd_ptr_gray_sync2[4:0]</span></span>
<span class="line"><span>  process $proc$clk2$44</span></span>
<span class="line"><span>    assign $0\\rd_ptr_gray_sync1[4:0] \\rd_ptr_gray_sync1</span></span>
<span class="line"><span>    assign $0\\rd_ptr_gray_sync2[4:0] \\rd_ptr_gray_sync2</span></span>
<span class="line"><span>    switch \\rst_wr</span></span>
<span class="line"><span>      case 1&#39;1</span></span>
<span class="line"><span>        assign $0\\rd_ptr_gray_sync1[4:0] 5&#39;00000</span></span>
<span class="line"><span>        assign $0\\rd_ptr_gray_sync2[4:0] 5&#39;00000</span></span>
<span class="line"><span>      case</span></span>
<span class="line"><span>        assign $0\\rd_ptr_gray_sync1[4:0] \\rd_ptr_gray</span></span>
<span class="line"><span>        assign $0\\rd_ptr_gray_sync2[4:0] \\rd_ptr_gray_sync1</span></span>
<span class="line"><span>    end</span></span>
<span class="line"><span>    sync posedge \\clk_wr</span></span>
<span class="line"><span>      update \\rd_ptr_gray_sync1 $0\\rd_ptr_gray_sync1[4:0]</span></span>
<span class="line"><span>      update \\rd_ptr_gray_sync2 $0\\rd_ptr_gray_sync2[4:0]</span></span>
<span class="line"><span>    sync high \\rst_wr</span></span>
<span class="line"><span>      update \\rd_ptr_gray_sync1 5&#39;00000</span></span>
<span class="line"><span>      update \\rd_ptr_gray_sync2 5&#39;00000</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 5 $0\\wr_ptr_gray_sync1[4:0]</span></span>
<span class="line"><span>  wire width 5 $0\\wr_ptr_gray_sync2[4:0]</span></span>
<span class="line"><span>  process $proc$clk3$45</span></span>
<span class="line"><span>    assign $0\\wr_ptr_gray_sync1[4:0] \\wr_ptr_gray_sync1</span></span>
<span class="line"><span>    assign $0\\wr_ptr_gray_sync2[4:0] \\wr_ptr_gray_sync2</span></span>
<span class="line"><span>    switch \\rst_rd</span></span>
<span class="line"><span>      case 1&#39;1</span></span>
<span class="line"><span>        assign $0\\wr_ptr_gray_sync1[4:0] 5&#39;00000</span></span>
<span class="line"><span>        assign $0\\wr_ptr_gray_sync2[4:0] 5&#39;00000</span></span>
<span class="line"><span>      case</span></span>
<span class="line"><span>        assign $0\\wr_ptr_gray_sync1[4:0] \\wr_ptr_gray</span></span>
<span class="line"><span>        assign $0\\wr_ptr_gray_sync2[4:0] \\wr_ptr_gray_sync1</span></span>
<span class="line"><span>    end</span></span>
<span class="line"><span>    sync posedge \\clk_rd</span></span>
<span class="line"><span>      update \\wr_ptr_gray_sync1 $0\\wr_ptr_gray_sync1[4:0]</span></span>
<span class="line"><span>      update \\wr_ptr_gray_sync2 $0\\wr_ptr_gray_sync2[4:0]</span></span>
<span class="line"><span>    sync high \\rst_rd</span></span>
<span class="line"><span>      update \\wr_ptr_gray_sync1 5&#39;00000</span></span>
<span class="line"><span>      update \\wr_ptr_gray_sync2 5&#39;00000</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>end</span></span>
<span class="line"><span></span></span>
<span class="line"><span>attribute \\src &quot;jz-hdl:7&quot;</span></span>
<span class="line"><span>module \\cdc_fifo</span></span>
<span class="line"><span>  wire width 1 input 1 \\clk_a</span></span>
<span class="line"><span>  wire width 1 input 2 \\clk_b</span></span>
<span class="line"><span>  wire width 1 input 3 \\rst_n</span></span>
<span class="line"><span>  wire width 6 output 4 \\leds</span></span>
<span class="line"><span>  wire width 64 \\packet_word</span></span>
<span class="line"><span>  wire width 64 \\data_out</span></span>
<span class="line"><span>  wire width 64 \\packet_view</span></span>
<span class="line"><span>  cell \\JZHDL_LIB_CDC_FIFO__W64 \\u_cdc_fifo_packet_view</span></span>
<span class="line"><span>    connect \\clk_wr \\clk_a</span></span>
<span class="line"><span>    connect \\clk_rd \\clk_b</span></span>
<span class="line"><span>    connect \\rst_wr \\rst_n</span></span>
<span class="line"><span>    connect \\rst_rd \\rst_n</span></span>
<span class="line"><span>    connect \\data_in \\packet_word</span></span>
<span class="line"><span>    connect \\write_en 1&#39;1</span></span>
<span class="line"><span>    connect \\read_en 1&#39;1</span></span>
<span class="line"><span>    connect \\data_out \\packet_view</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 6 $0\\leds[5:0]</span></span>
<span class="line"><span>  process $proc$async$46</span></span>
<span class="line"><span>    assign $0\\leds[5:0] \\data_out [5:0]</span></span>
<span class="line"><span>    sync always</span></span>
<span class="line"><span>      update \\leds $0\\leds[5:0]</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 64 $0\\packet_word[63:0]</span></span>
<span class="line"><span>  wire width 1 $auto$47</span></span>
<span class="line"><span>  cell $logic_not $auto$48</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\rst_n</span></span>
<span class="line"><span>    connect \\Y $auto$47</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 64 $auto$49</span></span>
<span class="line"><span>  cell $add $auto$50</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 64</span></span>
<span class="line"><span>    parameter \\B_SIGNED 0</span></span>
<span class="line"><span>    parameter \\B_WIDTH 64</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 64</span></span>
<span class="line"><span>    connect \\A \\packet_word</span></span>
<span class="line"><span>    connect \\B 64&#39;0000000000000000000000000000000000000000000000000000000000000001</span></span>
<span class="line"><span>    connect \\Y $auto$49</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  process $proc$clk0$51</span></span>
<span class="line"><span>    assign $0\\packet_word[63:0] \\packet_word</span></span>
<span class="line"><span>    switch $auto$47</span></span>
<span class="line"><span>      case 1&#39;1</span></span>
<span class="line"><span>        assign $0\\packet_word[63:0] 64&#39;0000000000000000000000000000000000000000000000000000000000000000</span></span>
<span class="line"><span>      case</span></span>
<span class="line"><span>        assign $0\\packet_word[63:0] $auto$49</span></span>
<span class="line"><span>    end</span></span>
<span class="line"><span>    sync posedge \\clk_a</span></span>
<span class="line"><span>      update \\packet_word $0\\packet_word[63:0]</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 64 $0\\data_out[63:0]</span></span>
<span class="line"><span>  wire width 1 $auto$52</span></span>
<span class="line"><span>  cell $logic_not $auto$53</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\rst_n</span></span>
<span class="line"><span>    connect \\Y $auto$52</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  process $proc$clk1$54</span></span>
<span class="line"><span>    assign $0\\data_out[63:0] \\data_out</span></span>
<span class="line"><span>    switch $auto$52</span></span>
<span class="line"><span>      case 1&#39;1</span></span>
<span class="line"><span>        assign $0\\data_out[63:0] 64&#39;0000000000000000000000000000000000000000000000000000000000000000</span></span>
<span class="line"><span>      case</span></span>
<span class="line"><span>        assign $0\\data_out[63:0] \\packet_view</span></span>
<span class="line"><span>    end</span></span>
<span class="line"><span>    sync posedge \\clk_b</span></span>
<span class="line"><span>      update \\data_out $0\\data_out[63:0]</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>end</span></span>
<span class="line"><span></span></span>
<span class="line"><span>attribute \\top 1</span></span>
<span class="line"><span>module \\top</span></span>
<span class="line"><span>  wire width 1 input 1 \\SCLK</span></span>
<span class="line"><span>  wire width 1 input 2 \\DONE</span></span>
<span class="line"><span>  wire width 1 input 3 \\KEY</span></span>
<span class="line"><span>  wire width 6 output 4 \\LED</span></span>
<span class="line"><span>  wire width 1 \\CLK_FAST</span></span>
<span class="line"><span>  wire width 1 \\jz_unused_pll_LOCK_cg0_u0</span></span>
<span class="line"><span>  wire width 1 \\jz_unused_pll_PHASE_cg0_u0</span></span>
<span class="line"><span>  wire width 1 \\jz_unused_pll_DIV_cg0_u0</span></span>
<span class="line"><span>  wire width 1 \\jz_unused_pll_DIV3_cg0_u0</span></span>
<span class="line"><span>  cell \\rPLL $auto$0_0</span></span>
<span class="line"><span>  parameter \\DEVICE &quot;GW1N-9C&quot;</span></span>
<span class="line"><span>  parameter \\FCLKIN &quot;26.998&quot;</span></span>
<span class="line"><span>  parameter \\IDIV_SEL 2</span></span>
<span class="line"><span>  parameter \\FBDIV_SEL 7</span></span>
<span class="line"><span>  parameter \\ODIV_SEL 8</span></span>
<span class="line"><span>  parameter \\PSDA_SEL &quot;0000&quot;</span></span>
<span class="line"><span>  parameter \\DUTYDA_SEL &quot;1000&quot;</span></span>
<span class="line"><span>  parameter \\DYN_IDIV_SEL &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\DYN_FBDIV_SEL &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\DYN_ODIV_SEL &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\DYN_DA_EN &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\DYN_SDIV_SEL 2</span></span>
<span class="line"><span>  parameter \\CLKOUT_FT_DIR 1</span></span>
<span class="line"><span>  parameter \\CLKOUTP_FT_DIR 1</span></span>
<span class="line"><span>  parameter \\CLKOUT_DLY_STEP 0</span></span>
<span class="line"><span>  parameter \\CLKOUTP_DLY_STEP 0</span></span>
<span class="line"><span>  parameter \\CLKFB_SEL &quot;internal&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUT_BYPASS &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUTP_BYPASS &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUTD_BYPASS &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUTD_SRC &quot;CLKOUT&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUTD3_SRC &quot;CLKOUT&quot;</span></span>
<span class="line"><span>  connect \\CLKIN \\SCLK</span></span>
<span class="line"><span>  connect \\CLKOUT \\CLK_FAST</span></span>
<span class="line"><span>  connect \\LOCK \\jz_unused_pll_LOCK_cg0_u0</span></span>
<span class="line"><span>  connect \\CLKOUTP \\jz_unused_pll_PHASE_cg0_u0</span></span>
<span class="line"><span>  connect \\CLKOUTD \\jz_unused_pll_DIV_cg0_u0</span></span>
<span class="line"><span>  connect \\CLKOUTD3 \\jz_unused_pll_DIV3_cg0_u0</span></span>
<span class="line"><span>  connect \\RESET 1&#39;0</span></span>
<span class="line"><span>  connect \\RESET_P 1&#39;0</span></span>
<span class="line"><span>  connect \\CLKFB 1&#39;0</span></span>
<span class="line"><span>end</span></span>
<span class="line"><span>  cell \\cdc_fifo \\u_top</span></span>
<span class="line"><span>    connect \\clk_a \\SCLK</span></span>
<span class="line"><span>    connect \\clk_b \\CLK_FAST</span></span>
<span class="line"><span>    connect \\rst_n \\KEY [0]</span></span>
<span class="line"><span>    connect \\leds { \\LED [5] \\LED [4] \\LED [3] \\LED [2] \\LED [1] \\LED [0] }</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>end</span></span></code></pre></div></div></div><h3 id="raw-unsynchronized-view-raw-quasi‐static-config" tabindex="-1">Raw unsynchronized view (RAW, quasi‑static config) <a class="header-anchor" href="#raw-unsynchronized-view-raw-quasi‐static-config" aria-label="Permalink to “Raw unsynchronized view (RAW, quasi‑static config)”">​</a></h3><p>A 16‑bit <code>config_word</code> register crosses unsynchronized via RAW. No CDC logic is inserted — the designer guarantees the value is stable when read.</p><div class="vp-code-group"><div class="tabs"><input type="radio" name="group-503" id="tab-504" checked><label data-title="cdc_raw.jz" for="tab-504">cdc_raw.jz</label><input type="radio" name="group-503" id="tab-505"><label data-title="project.jz" for="tab-505">project.jz</label><input type="radio" name="group-503" id="tab-506"><label data-title="Generated Verilog" for="tab-506">Generated Verilog</label><input type="radio" name="group-503" id="tab-507"><label data-title="Generated RTLIL" for="tab-507">Generated RTLIL</label></div><div class="blocks"><div class="language-jz active"><button title="Copy Code" class="copy"></button><span class="lang">jz</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// CDC RAW example: unsynchronized quasi-static register</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">//</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// Demonstrates a RAW crossing for a configuration register written</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// once at startup and stable thereafter. No synchronization logic</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// is inserted — the designer guarantees the value is stable when read.</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">@module</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> cdc_raw</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    PORT</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] clk_a;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] clk_b;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] rst_n;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        OUT</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> [6] leds;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    REGISTER</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        config_word  [16] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;"> 16&#39;hA5A5</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        local_config [16] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;"> 16&#39;h0000</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    CDC {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        RAW config_word (clk_a) </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=&gt;</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> config_view (clk_b);</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    ASYNCHRONOUS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        leds </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">&lt;= </span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">local_config[5:0];</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    SYNCHRONOUS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">(CLK</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">clk_a RESET</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">rst_n RESET_ACTIVE</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">Low) {</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">        // config_word is quasi-static: written once, stable thereafter</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        config_word </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">&lt;= </span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">config_word;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    SYNCHRONOUS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">(CLK</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">clk_b RESET</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">rst_n RESET_ACTIVE</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">Low) {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        local_config </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">&lt;= </span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">config_view;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">@endmod</span></span></code></pre></div><div class="language-jz"><button title="Copy Code" class="copy"></button><span class="lang">jz</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">@project</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">(CHIP</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">&quot;GW1NR</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">-</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">9</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">-</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">QN88</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">-</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">C6</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">-</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">I5&quot;) CDC_RAW_EXAMPLE</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    @import</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> &quot;cdc_raw.jz&quot;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">    CLOCKS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        SCLK     </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">period</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">37.04 }; </span><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// 27MHz</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        CLK_FAST;                     </span><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// 108MHz (PLL)</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">    IN_PINS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        SCLK   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">standard</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LVCMOS33 };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        DONE   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">standard</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LVCMOS33 };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        KEY[1] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">standard</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LVCMOS33 };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">    OUT_PINS</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[6] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> { </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">standard</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LVCMOS33, </span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">drive</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">8 };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">    MAP</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        SCLK   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 52;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        KEY[0] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 3;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[0] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 10;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[1] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 11;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[2] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 13;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[3] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 14;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[4] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 15;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        LED[5] </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 16;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        DONE   </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> IOL14A;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    CLOCK_GEN {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        PLL {</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">            IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  REF_CLK SCLK;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">            OUT</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> BASE    CLK_FAST;</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">            CONFIG</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">                IDIV  </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 2;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">                FBDIV </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 7;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">                ODIV  </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> 8;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">            };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        };</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    @top</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> cdc_raw {</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] clk_a </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> SCLK;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] clk_b </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> CLK_FAST;</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        IN</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">  [1] rst_n </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> KEY[0];</span></span>
<span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">        OUT</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;"> [6] leds  </span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">=</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;"> ~</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">LED;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">@endproj</span></span></code></pre></div><div class="language-v"><button title="Copy Code" class="copy"></button><span class="lang">v</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// This Verilog was transpiled from JZ-HDL.</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// jz-hdl version: Version 0.1.7 (c6d52fc)</span></span>
<span class="line"><span style="--shiki-light:#6A737D;--shiki-dark:#6A737D;">// Intended for use with yosys.</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">\`default_nettype none</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">module cdc_raw (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    clk_a,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    clk_b,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    rst_n,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    leds</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">);</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Ports</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input clk_a;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input clk_b;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input rst_n;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    output reg [5:0] leds;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Signals</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg [15:0] config_word;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    reg [15:0] local_config;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire [15:0] config_view;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign config_view = config_word;</span></span>
<span class="line"></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @* begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        leds = local_config[5:0];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @(posedge clk_a) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        if (!rst_n) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            config_word &lt;= 16&#39;b1010010110100101;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        else begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            config_word &lt;= config_word;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    always @(posedge clk_b) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        if (!rst_n) begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            local_config &lt;= 16&#39;b0000000000000000;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        else begin</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">            local_config &lt;= config_view;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    end</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">endmodule</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">module top (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    SCLK,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    DONE,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    KEY,</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    LED</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">);</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input SCLK;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input DONE;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    input KEY;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    output [5:0] LED;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // Top-level logical→physical pin mapping</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_raw.clk_a -&gt; SCLK (board 52)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_raw.clk_b -&gt; CLK_FAST (clock gen)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_raw.rst_n -&gt; KEY[0] (board 3)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_raw.leds[5] -&gt; LED[5] (board 16)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_raw.leds[4] -&gt; LED[4] (board 15)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_raw.leds[3] -&gt; LED[3] (board 14)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_raw.leds[2] -&gt; LED[2] (board 13)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_raw.leds[1] -&gt; LED[1] (board 11)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    //   cdc_raw.leds[0] -&gt; LED[0] (board 10)</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire [5:0] jz_inv_leds;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[5] = ~jz_inv_leds[5];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[4] = ~jz_inv_leds[4];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[3] = ~jz_inv_leds[3];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[2] = ~jz_inv_leds[2];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[1] = ~jz_inv_leds[1];</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    assign LED[0] = ~jz_inv_leds[0];</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire CLK_FAST;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire jz_unused_pll_LOCK_cg0_u0;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire jz_unused_pll_PHASE_cg0_u0;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire jz_unused_pll_DIV_cg0_u0;</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    wire jz_unused_pll_DIV3_cg0_u0;</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    // CLOCK_GEN PLL instantiation (from chip data)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    rPLL #(</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DEVICE(&quot;GW1N-9C&quot;),          // Specify your device</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .FCLKIN(&quot;26.998&quot;),       // Input frequency in MHz</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .IDIV_SEL(2),           // IDIV: Input divider</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .FBDIV_SEL(7),         // FBDIV: Feedback divider</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .ODIV_SEL(8),           // ODIV: Output divider</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .PSDA_SEL(&quot;0000&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DUTYDA_SEL(&quot;1000&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_IDIV_SEL(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_FBDIV_SEL(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_ODIV_SEL(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_DA_EN(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .DYN_SDIV_SEL(2),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUT_FT_DIR(1&#39;b1),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTP_FT_DIR(1&#39;b1),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUT_DLY_STEP(0),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTP_DLY_STEP(0),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKFB_SEL(&quot;internal&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUT_BYPASS(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTP_BYPASS(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD_BYPASS(&quot;FALSE&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD_SRC(&quot;CLKOUT&quot;),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD3_SRC(&quot;CLKOUT&quot;)</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">) u_pll_0_0 (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUT(CLK_FAST),   // Primary output</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .LOCK(jz_unused_pll_LOCK_cg0_u0),     // High when stable</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTP(jz_unused_pll_PHASE_cg0_u0), // Phase shifted output</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD(jz_unused_pll_DIV_cg0_u0),   // Divided output</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKOUTD3(jz_unused_pll_DIV3_cg0_u0), // Divided by 3 output</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .RESET(1&#39;b0),        // Reset signal</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .RESET_P(1&#39;b0),      // PLL power down</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKIN(SCLK),  // Reference clock input</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    .CLKFB(1&#39;b0)         // External feedback</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">);</span></span>
<span class="line"></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    cdc_raw u_top (</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .clk_a(SCLK),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .clk_b(CLK_FAST),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .rst_n(KEY[0]),</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">        .leds({jz_inv_leds[5], jz_inv_leds[4], jz_inv_leds[3], jz_inv_leds[2], jz_inv_leds[1], jz_inv_leds[0]})</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">    );</span></span>
<span class="line"><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">endmodule</span></span></code></pre></div><div class="language-il"><button title="Copy Code" class="copy"></button><span class="lang">il</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span># Generated by jz-hdl RTLIL backend</span></span>
<span class="line"><span># jz-hdl version: Version 0.1.7 (c6d52fc)</span></span>
<span class="line"><span></span></span>
<span class="line"><span>attribute \\src &quot;jz-hdl:7&quot;</span></span>
<span class="line"><span>module \\cdc_raw</span></span>
<span class="line"><span>  wire width 1 input 1 \\clk_a</span></span>
<span class="line"><span>  wire width 1 input 2 \\clk_b</span></span>
<span class="line"><span>  wire width 1 input 3 \\rst_n</span></span>
<span class="line"><span>  wire width 6 output 4 \\leds</span></span>
<span class="line"><span>  wire width 16 \\config_word</span></span>
<span class="line"><span>  wire width 16 \\local_config</span></span>
<span class="line"><span>  wire width 16 \\config_view</span></span>
<span class="line"><span>  connect \\config_view \\config_word</span></span>
<span class="line"><span>  wire width 6 $0\\leds[5:0]</span></span>
<span class="line"><span>  process $proc$async$1</span></span>
<span class="line"><span>    assign $0\\leds[5:0] \\local_config [5:0]</span></span>
<span class="line"><span>    sync always</span></span>
<span class="line"><span>      update \\leds $0\\leds[5:0]</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 16 $0\\config_word[15:0]</span></span>
<span class="line"><span>  wire width 1 $auto$2</span></span>
<span class="line"><span>  cell $logic_not $auto$3</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\rst_n</span></span>
<span class="line"><span>    connect \\Y $auto$2</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  process $proc$clk0$4</span></span>
<span class="line"><span>    assign $0\\config_word[15:0] \\config_word</span></span>
<span class="line"><span>    switch $auto$2</span></span>
<span class="line"><span>      case 1&#39;1</span></span>
<span class="line"><span>        assign $0\\config_word[15:0] 16&#39;1010010110100101</span></span>
<span class="line"><span>      case</span></span>
<span class="line"><span>        assign $0\\config_word[15:0] \\config_word</span></span>
<span class="line"><span>    end</span></span>
<span class="line"><span>    sync posedge \\clk_a</span></span>
<span class="line"><span>      update \\config_word $0\\config_word[15:0]</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  wire width 16 $0\\local_config[15:0]</span></span>
<span class="line"><span>  wire width 1 $auto$5</span></span>
<span class="line"><span>  cell $logic_not $auto$6</span></span>
<span class="line"><span>    parameter \\A_SIGNED 0</span></span>
<span class="line"><span>    parameter \\A_WIDTH 1</span></span>
<span class="line"><span>    parameter \\Y_WIDTH 1</span></span>
<span class="line"><span>    connect \\A \\rst_n</span></span>
<span class="line"><span>    connect \\Y $auto$5</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>  process $proc$clk1$7</span></span>
<span class="line"><span>    assign $0\\local_config[15:0] \\local_config</span></span>
<span class="line"><span>    switch $auto$5</span></span>
<span class="line"><span>      case 1&#39;1</span></span>
<span class="line"><span>        assign $0\\local_config[15:0] 16&#39;0000000000000000</span></span>
<span class="line"><span>      case</span></span>
<span class="line"><span>        assign $0\\local_config[15:0] \\config_view</span></span>
<span class="line"><span>    end</span></span>
<span class="line"><span>    sync posedge \\clk_b</span></span>
<span class="line"><span>      update \\local_config $0\\local_config[15:0]</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>end</span></span>
<span class="line"><span></span></span>
<span class="line"><span>attribute \\top 1</span></span>
<span class="line"><span>module \\top</span></span>
<span class="line"><span>  wire width 1 input 1 \\SCLK</span></span>
<span class="line"><span>  wire width 1 input 2 \\DONE</span></span>
<span class="line"><span>  wire width 1 input 3 \\KEY</span></span>
<span class="line"><span>  wire width 6 output 4 \\LED</span></span>
<span class="line"><span>  wire width 1 \\CLK_FAST</span></span>
<span class="line"><span>  wire width 1 \\jz_unused_pll_LOCK_cg0_u0</span></span>
<span class="line"><span>  wire width 1 \\jz_unused_pll_PHASE_cg0_u0</span></span>
<span class="line"><span>  wire width 1 \\jz_unused_pll_DIV_cg0_u0</span></span>
<span class="line"><span>  wire width 1 \\jz_unused_pll_DIV3_cg0_u0</span></span>
<span class="line"><span>  cell \\rPLL $auto$0_0</span></span>
<span class="line"><span>  parameter \\DEVICE &quot;GW1N-9C&quot;</span></span>
<span class="line"><span>  parameter \\FCLKIN &quot;26.998&quot;</span></span>
<span class="line"><span>  parameter \\IDIV_SEL 2</span></span>
<span class="line"><span>  parameter \\FBDIV_SEL 7</span></span>
<span class="line"><span>  parameter \\ODIV_SEL 8</span></span>
<span class="line"><span>  parameter \\PSDA_SEL &quot;0000&quot;</span></span>
<span class="line"><span>  parameter \\DUTYDA_SEL &quot;1000&quot;</span></span>
<span class="line"><span>  parameter \\DYN_IDIV_SEL &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\DYN_FBDIV_SEL &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\DYN_ODIV_SEL &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\DYN_DA_EN &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\DYN_SDIV_SEL 2</span></span>
<span class="line"><span>  parameter \\CLKOUT_FT_DIR 1</span></span>
<span class="line"><span>  parameter \\CLKOUTP_FT_DIR 1</span></span>
<span class="line"><span>  parameter \\CLKOUT_DLY_STEP 0</span></span>
<span class="line"><span>  parameter \\CLKOUTP_DLY_STEP 0</span></span>
<span class="line"><span>  parameter \\CLKFB_SEL &quot;internal&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUT_BYPASS &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUTP_BYPASS &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUTD_BYPASS &quot;FALSE&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUTD_SRC &quot;CLKOUT&quot;</span></span>
<span class="line"><span>  parameter \\CLKOUTD3_SRC &quot;CLKOUT&quot;</span></span>
<span class="line"><span>  connect \\CLKIN \\SCLK</span></span>
<span class="line"><span>  connect \\CLKOUT \\CLK_FAST</span></span>
<span class="line"><span>  connect \\LOCK \\jz_unused_pll_LOCK_cg0_u0</span></span>
<span class="line"><span>  connect \\CLKOUTP \\jz_unused_pll_PHASE_cg0_u0</span></span>
<span class="line"><span>  connect \\CLKOUTD \\jz_unused_pll_DIV_cg0_u0</span></span>
<span class="line"><span>  connect \\CLKOUTD3 \\jz_unused_pll_DIV3_cg0_u0</span></span>
<span class="line"><span>  connect \\RESET 1&#39;0</span></span>
<span class="line"><span>  connect \\RESET_P 1&#39;0</span></span>
<span class="line"><span>  connect \\CLKFB 1&#39;0</span></span>
<span class="line"><span>end</span></span>
<span class="line"><span>  cell \\cdc_raw \\u_top</span></span>
<span class="line"><span>    connect \\clk_a \\SCLK</span></span>
<span class="line"><span>    connect \\clk_b \\CLK_FAST</span></span>
<span class="line"><span>    connect \\rst_n \\KEY [0]</span></span>
<span class="line"><span>    connect \\leds { \\LED [5] \\LED [4] \\LED [3] \\LED [2] \\LED [1] \\LED [0] }</span></span>
<span class="line"><span>  end</span></span>
<span class="line"><span>end</span></span></code></pre></div></div></div><h2 id="common-errors-and-diagnostics" tabindex="-1">Common Errors and Diagnostics <a class="header-anchor" href="#common-errors-and-diagnostics" aria-label="Permalink to “Common Errors and Diagnostics”">​</a></h2><ul><li><p>DOMAIN_CONFLICT</p><ul><li>Cause: Using a <code>REGISTER</code> in a synchronous block whose <code>CLK</code> differs from the register&#39;s home domain (as set by CDC or by where the register is first assigned).</li><li>Fix: Move the register usage to the correct <code>SYNCHRONOUS</code> block or add a CDC entry that creates an alias for cross‑domain use.</li></ul></li><li><p>DUPLICATE_CDC_ENTRY</p><ul><li>Cause: Two CDC entries attempt to set home domain for the same source register inconsistently.</li><li>Fix: Consolidate CDC entries; each register should have a single definitive home domain.</li></ul></li><li><p>INVALID_CDC_TARGET</p><ul><li>Cause: The source register is not a plain register identifier (slice, concat, or undefined).</li><li>Fix: Use the plain register name; if you need to cross slices or fields, create separate registers or use FIFO.</li></ul></li><li><p>INVALID_CDC_TYPE</p><ul><li>Cause: Using <code>BIT</code> for multi‑bit sources or <code>BUS</code> for sources that change arbitrarily.</li><li>Fix: Use the correct CDC type. Use <code>BIT</code> for width==1, <code>BUS</code> only when Gray‑code discipline is followed, otherwise <code>FIFO</code>.</li></ul></li><li><p>UNSAFE_BUS_WARNING (warning)</p><ul><li>Cause: Static/heuristic analysis detects that a <code>BUS</code> source may change multiple bits simultaneously.</li><li>Fix: Use <code>FIFO</code> or redesign the producer to follow Gray‑code or single‑bit change discipline.</li></ul></li><li><p>MISSING_CDC_FOR_CROSS_DOMAIN_USE</p><ul><li>Cause: A register is referenced in a different domain without a CDC entry.</li><li>Fix: Add a CDC entry or redesign to avoid cross‑domain reads.</li></ul></li><li><p>MULTI_CLK_ASSIGN / REGISTER_LOCALITY_VIOLATION</p><ul><li>Cause: Assigning the same register in more than one <code>SYNCHRONOUS</code> block for different clocks.</li><li>Fix: Ensure a register is written only in its home domain; use CDC to observe it elsewhere.</li></ul></li></ul><h2 id="best-practices" tabindex="-1">Best Practices <a class="header-anchor" href="#best-practices" aria-label="Permalink to “Best Practices”">​</a></h2><ul><li><p>Be explicit and conservative:</p><ul><li>Prefer <code>FIFO</code> for multi‑bit transfers unless you can guarantee single‑bit changes (Gray code) and understand the implications.</li><li>Use <code>BIT[2]</code> or <code>BIT[3]</code> for status/control signals; 2 stages is common, 3 for safety in noisy environments.</li></ul></li><li><p>Name aliases clearly:</p><ul><li>Use systematic naming like <code>reg_sync_destclk</code> or <code>src_to_dst_signal</code> so intent is obvious.</li></ul></li><li><p>One synchronous block per clock:</p><ul><li>Place all logic for a given clock in the same <code>SYNCHRONOUS(CLK=...)</code> block to satisfy Synchronous Block Uniqueness.</li></ul></li><li><p>Keep CDC block near register declarations:</p><ul><li>Place <code>CDC { ... }</code> entries close to the <code>REGISTER</code> declarations for readability and to help tools resolve names.</li></ul></li><li><p>Document constraints:</p><ul><li>If a <code>BUS</code> CDC requires Gray‑coding, document the producer&#39;s requirement in comments and add static checks or assertions when possible.</li></ul></li><li><p>Verify with static checks and timing:</p><ul><li>Run CDC-specific static checks and, where possible, formal checks to ensure no combinational paths create control hazards across domains.</li><li>Ensure timing constraints for synchronizers are included in downstream SDC (synthesis/timing) flows.</li></ul></li></ul><h2 id="anti‐patterns-what-to-avoid" tabindex="-1">Anti‑patterns (What to avoid) <a class="header-anchor" href="#anti‐patterns-what-to-avoid" aria-label="Permalink to “Anti‑patterns (What to avoid)”">​</a></h2><ul><li>Reading a source register directly in another clock domain without CDC — causes DOMAIN_CONFLICT and metastability.</li><li>Using <code>BUS</code> for wide registers that can change every cycle with arbitrary bit patterns — leads to data corruption.</li><li>Mixing alias names and source register names across domains (ambiguous intent).</li><li>Trying to synchronize slices of a multi‑bit register via separate <code>BIT</code> synchronizers without ensuring atomic update semantics on the producer side.</li></ul><h2 id="checklist-for-adding-a-cdc-crossing" tabindex="-1">Checklist for Adding a CDC Crossing <a class="header-anchor" href="#checklist-for-adding-a-cdc-crossing" aria-label="Permalink to “Checklist for Adding a CDC Crossing”">​</a></h2><ul><li>Confirm the source is a <code>REGISTER</code> and has a single, clear producer domain.</li><li>Decide the appropriate CDC type: <ul><li>BIT → single‑bit flag.</li><li>BUS → multi‑bit Gray‑code or single‑bit change guaranteed.</li><li>FIFO → arbitrary multi‑bit transfers.</li><li>RAW → quasi‑static or externally synchronized signals (no CDC logic inserted).</li></ul></li><li>Choose <code>n_stages</code> (default 2). For safety or noisy inputs, increase to 3.</li><li>Add the <code>CDC</code> entry near the register declaration.</li><li>Replace any cross‑domain uses with reads of the <code>dest_alias</code>.</li><li>Run static checks; address compiler warnings/errors.</li><li>Add documentation/comments explaining invariants (e.g., Gray‑code property).</li></ul><h2 id="synthesis-and-implementation-notes" tabindex="-1">Synthesis and Implementation Notes <a class="header-anchor" href="#synthesis-and-implementation-notes" aria-label="Permalink to “Synthesis and Implementation Notes”">​</a></h2><ul><li>The compiler will lower <code>CDC</code> entries into synthesizable primitives: <ul><li>BIT → chain of flip‑flops with optional meta‑stable handling.</li><li>BUS → bank of flip‑flops and optional handshaking or gating depending on implementation.</li><li>FIFO → dual‑clock FIFO or asynchronous FIFO implementation using pointers and synchronizers.</li></ul></li><li>Downstream tools should map these to: <ul><li>Vendor synchronizer primitives or hand‑optimized flops.</li><li>FIFO IP blocks for <code>FIFO</code> CDC entries when available.</li></ul></li><li>Ensure timing constraints (SDC) include created synchronizer registers so STA correctly analyzes setup/hold for destination domain.</li></ul>`,44)])])}const g=a(l,[["render",e]]);export{E as __pageData,g as default};
