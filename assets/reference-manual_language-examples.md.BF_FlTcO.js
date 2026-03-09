import{_ as n,o as a,c as p,ai as e}from"./chunks/framework.C4ntERNc.js";const u=JSON.parse('{"title":"Language Examples","description":"","frontmatter":{"title":"Language Examples","lang":"en-US","layout":"doc","outline":"deep"},"headers":[],"relativePath":"reference-manual/language-examples.md","filePath":"reference-manual/language-examples.md"}'),l={name:"reference-manual/language-examples.md"};function i(t,s,c,r,d,o){return a(),p("div",null,[...s[0]||(s[0]=[e(`<div style="display:none;" hidden="true" aria-hidden="true">Are you an LLM? You can read better optimized documentation at /jz-hdl/reference-manual/language-examples.md for this page in Markdown format</div><h1 id="language-examples" tabindex="-1">Language Examples <a class="header-anchor" href="#language-examples" aria-label="Permalink to “Language Examples”">​</a></h1><p>Complete, self-contained JZ-HDL examples demonstrating core language features.</p><h2 id="simple-1-bit-register" tabindex="-1">Simple 1-Bit Register <a class="header-anchor" href="#simple-1-bit-register" aria-label="Permalink to “Simple 1-Bit Register”">​</a></h2><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>@module flipflop</span></span>
<span class="line"><span>  PORT {</span></span>
<span class="line"><span>    IN  [1] d;</span></span>
<span class="line"><span>    OUT [1] q;</span></span>
<span class="line"><span>    IN  [1] clk;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  REGISTER {</span></span>
<span class="line"><span>    state [1] = 1&#39;b0;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  ASYNCHRONOUS {</span></span>
<span class="line"><span>    q = state;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  SYNCHRONOUS(CLK=clk) {</span></span>
<span class="line"><span>    state &lt;= d;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>@endmod</span></span></code></pre></div><h2 id="bus-slice-and-synchronous-update" tabindex="-1">Bus Slice and Synchronous Update <a class="header-anchor" href="#bus-slice-and-synchronous-update" aria-label="Permalink to “Bus Slice and Synchronous Update”">​</a></h2><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>@module slice_example</span></span>
<span class="line"><span>  CONST { W = 8; }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  PORT {</span></span>
<span class="line"><span>    IN  [16] inbus;</span></span>
<span class="line"><span>    OUT [8] out;</span></span>
<span class="line"><span>    IN  [1] clk;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  REGISTER {</span></span>
<span class="line"><span>    r [8] = 8&#39;h00;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  ASYNCHRONOUS {</span></span>
<span class="line"><span>    out = r;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  SYNCHRONOUS(CLK=clk) {</span></span>
<span class="line"><span>    r &lt;= inbus[15:8];</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>@endmod</span></span></code></pre></div><h2 id="module-instantiation" tabindex="-1">Module Instantiation <a class="header-anchor" href="#module-instantiation" aria-label="Permalink to “Module Instantiation”">​</a></h2><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>@module top</span></span>
<span class="line"><span>  PORT {</span></span>
<span class="line"><span>    IN  [8] a;</span></span>
<span class="line"><span>    IN  [8] b;</span></span>
<span class="line"><span>    OUT [9] sum;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  @new adder_inst adder_module {</span></span>
<span class="line"><span>    IN  [8] a = a;</span></span>
<span class="line"><span>    IN  [8] b = b;</span></span>
<span class="line"><span>    OUT [9] sum = sum;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>@endmod</span></span></code></pre></div><h2 id="tri-state-bidirectional-port" tabindex="-1">Tri-State / Bidirectional Port <a class="header-anchor" href="#tri-state-bidirectional-port" aria-label="Permalink to “Tri-State / Bidirectional Port”">​</a></h2><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>@module tristate_buffer</span></span>
<span class="line"><span>  PORT {</span></span>
<span class="line"><span>    IN    [8] data_in;</span></span>
<span class="line"><span>    IN    [1] enable;</span></span>
<span class="line"><span>    INOUT [8] data_bus;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  ASYNCHRONOUS {</span></span>
<span class="line"><span>    data_bus = enable ? data_in : 8&#39;bzzzz_zzzz;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>@endmod</span></span></code></pre></div><h2 id="counter-with-load-and-reset" tabindex="-1">Counter with Load and Reset <a class="header-anchor" href="#counter-with-load-and-reset" aria-label="Permalink to “Counter with Load and Reset”">​</a></h2><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>@module counter</span></span>
<span class="line"><span>  PORT {</span></span>
<span class="line"><span>    IN  [1] clk;</span></span>
<span class="line"><span>    IN  [1] reset;</span></span>
<span class="line"><span>    IN  [1] load;</span></span>
<span class="line"><span>    IN  [8] load_value;</span></span>
<span class="line"><span>    OUT [16] count_wide;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  REGISTER {</span></span>
<span class="line"><span>    counter_reg [16] = 16&#39;h0000;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  ASYNCHRONOUS {</span></span>
<span class="line"><span>    // Receive with explicit zero-extend (8 → 16)</span></span>
<span class="line"><span>    count_wide &lt;=z load_value;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  SYNCHRONOUS(</span></span>
<span class="line"><span>    CLK=clk</span></span>
<span class="line"><span>    RESET=reset</span></span>
<span class="line"><span>    RESET_ACTIVE=High</span></span>
<span class="line"><span>  ) {</span></span>
<span class="line"><span>    IF (load) {</span></span>
<span class="line"><span>      // Explicit zero-extend 8 → 16 into counter</span></span>
<span class="line"><span>      counter_reg &lt;=z load_value;</span></span>
<span class="line"><span>    } ELSE {</span></span>
<span class="line"><span>      counter_reg &lt;= counter_reg + 1;</span></span>
<span class="line"><span>    }</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>@endmod</span></span></code></pre></div><h2 id="alu-with-select-case" tabindex="-1">ALU with SELECT / CASE <a class="header-anchor" href="#alu-with-select-case" aria-label="Permalink to “ALU with SELECT / CASE”">​</a></h2><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>@module cpu_alu</span></span>
<span class="line"><span>  CONST {</span></span>
<span class="line"><span>    XLEN = 32;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  PORT {</span></span>
<span class="line"><span>    IN  [XLEN] operand_a;</span></span>
<span class="line"><span>    IN  [XLEN] operand_b;</span></span>
<span class="line"><span>    IN  [4] control;</span></span>
<span class="line"><span>    OUT [XLEN] result;</span></span>
<span class="line"><span>    OUT [1] zero;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  WIRE {</span></span>
<span class="line"><span>    alu_result [XLEN];</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  ASYNCHRONOUS {</span></span>
<span class="line"><span>    SELECT (control) {</span></span>
<span class="line"><span>      CASE 4&#39;h0 {</span></span>
<span class="line"><span>        alu_result = operand_a + operand_b;</span></span>
<span class="line"><span>      }</span></span>
<span class="line"><span>      CASE 4&#39;h1 {</span></span>
<span class="line"><span>        alu_result = operand_a - operand_b;</span></span>
<span class="line"><span>      }</span></span>
<span class="line"><span>      CASE 4&#39;h2 {</span></span>
<span class="line"><span>        alu_result = operand_a &amp; operand_b;</span></span>
<span class="line"><span>      }</span></span>
<span class="line"><span>      CASE 4&#39;h3 {</span></span>
<span class="line"><span>        alu_result = operand_a | operand_b;</span></span>
<span class="line"><span>      }</span></span>
<span class="line"><span>      CASE 4&#39;h4 {</span></span>
<span class="line"><span>        alu_result = operand_a ^ operand_b;</span></span>
<span class="line"><span>      }</span></span>
<span class="line"><span>      DEFAULT {</span></span>
<span class="line"><span>        alu_result = 32&#39;h0000_0000;</span></span>
<span class="line"><span>      }</span></span>
<span class="line"><span>    }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>    result = alu_result;</span></span>
<span class="line"><span>    zero = (alu_result == 32&#39;h0000_0000) ? 1&#39;b1 : 1&#39;b0;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>@endmod</span></span></code></pre></div><h2 id="arithmetic-with-carry-capture" tabindex="-1">Arithmetic with Carry Capture <a class="header-anchor" href="#arithmetic-with-carry-capture" aria-label="Permalink to “Arithmetic with Carry Capture”">​</a></h2><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>@module adder_with_carry</span></span>
<span class="line"><span>  CONST { WIDTH = 8; }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  PORT {</span></span>
<span class="line"><span>    IN  [WIDTH] a;</span></span>
<span class="line"><span>    IN  [WIDTH] b;</span></span>
<span class="line"><span>    OUT [WIDTH] sum;</span></span>
<span class="line"><span>    OUT [1] carry;</span></span>
<span class="line"><span>    IN  [1] clk;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  REGISTER {</span></span>
<span class="line"><span>    result [WIDTH + 1] = {1&#39;b0, WIDTH&#39;d0};</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  ASYNCHRONOUS {</span></span>
<span class="line"><span>    sum = result[WIDTH - 1:0];</span></span>
<span class="line"><span>    carry = result[WIDTH];</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  SYNCHRONOUS(CLK=clk) {</span></span>
<span class="line"><span>    result &lt;= uadd(a, b);</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>@endmod</span></span></code></pre></div><h2 id="sign-extend-in-synchronous-assignment" tabindex="-1">Sign-Extend in SYNCHRONOUS Assignment <a class="header-anchor" href="#sign-extend-in-synchronous-assignment" aria-label="Permalink to “Sign-Extend in SYNCHRONOUS Assignment”">​</a></h2><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>@module sign_extend_example</span></span>
<span class="line"><span>  PORT {</span></span>
<span class="line"><span>    IN  [8] input_byte;</span></span>
<span class="line"><span>    OUT [16] extended_output;</span></span>
<span class="line"><span>    IN  [1] clk;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  REGISTER {</span></span>
<span class="line"><span>    extended_reg [16] = 16&#39;h0000;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  ASYNCHRONOUS {</span></span>
<span class="line"><span>    extended_output = extended_reg;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  SYNCHRONOUS(CLK=clk) {</span></span>
<span class="line"><span>    // Explicit sign-extend 8 → 16 bits</span></span>
<span class="line"><span>    extended_reg &lt;=s input_byte;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>@endmod</span></span></code></pre></div><h2 id="sliced-register-updates" tabindex="-1">Sliced Register Updates <a class="header-anchor" href="#sliced-register-updates" aria-label="Permalink to “Sliced Register Updates”">​</a></h2><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>@module sliced_register_example</span></span>
<span class="line"><span>  PORT {</span></span>
<span class="line"><span>    IN  [1] clk;</span></span>
<span class="line"><span>    IN  [4] nibble_a;</span></span>
<span class="line"><span>    IN  [4] nibble_b;</span></span>
<span class="line"><span>    OUT [8] result;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  REGISTER {</span></span>
<span class="line"><span>    data [8] = 8&#39;h00;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  ASYNCHRONOUS {</span></span>
<span class="line"><span>    result = data;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  SYNCHRONOUS(CLK=clk) {</span></span>
<span class="line"><span>    // Update different nibbles without affecting each other</span></span>
<span class="line"><span>    data[7:4] &lt;= nibble_a;</span></span>
<span class="line"><span>    data[3:0] &lt;= nibble_b;</span></span>
<span class="line"><span>    // Each nibble assigned once; non-overlapping ranges</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>@endmod</span></span></code></pre></div><h2 id="tri-state-transceiver-with-read-write-control" tabindex="-1">Tri-State Transceiver with Read/Write Control <a class="header-anchor" href="#tri-state-transceiver-with-read-write-control" aria-label="Permalink to “Tri-State Transceiver with Read/Write Control”">​</a></h2><p><strong>Behavior:</strong></p><ul><li><code>rw = 1</code>: Internal driver sends <code>buffer</code> onto <code>data</code>; <code>buffer</code> captures (echoes own write)</li><li><code>rw = 0</code>: Release bus (<code>z</code>); external drivers control <code>data</code>; <code>buffer</code> captures external data</li></ul><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>@module tristate_transceiver</span></span>
<span class="line"><span>  PORT {</span></span>
<span class="line"><span>    IN    [1] clk;</span></span>
<span class="line"><span>    IN    [1] rw;           // 1 = drive, 0 = release</span></span>
<span class="line"><span>    INOUT [8] data;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  REGISTER {</span></span>
<span class="line"><span>    buffer [8] = 8&#39;h00;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  ASYNCHRONOUS {</span></span>
<span class="line"><span>    data = rw ? buffer : 8&#39;bzzzz_zzzz;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  SYNCHRONOUS(CLK=clk) {</span></span>
<span class="line"><span>    buffer &lt;= data;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>@endmod</span></span></code></pre></div>`,25)])])}const g=n(l,[["render",i]]);export{u as __pageData,g as default};
