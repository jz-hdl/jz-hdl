import{_ as a,o as n,c as e,ai as i}from"./chunks/framework.C4ntERNc.js";const m=JSON.parse('{"title":"Memory","description":"","frontmatter":{"title":"Memory","lang":"en-US","layout":"doc","outline":"deep"},"headers":[],"relativePath":"reference-manual/memory.md","filePath":"reference-manual/memory.md"}'),l={name:"reference-manual/memory.md"};function p(t,s,d,r,o,c){return n(),e("div",null,[...s[0]||(s[0]=[i(`<div style="display:none;" hidden="true" aria-hidden="true">Are you an LLM? You can read better optimized documentation at /jz-hdl/reference-manual/memory.md for this page in Markdown format</div><h1 id="memory" tabindex="-1">Memory <a class="header-anchor" href="#memory" aria-label="Permalink to “Memory”">​</a></h1><div class="tip custom-block"><p class="custom-block-title">Formal Reference</p><p>For a concise normative summary of all MEM rules, port types, and access syntax, see the <a href="/jz-hdl/reference-manual/formal-reference/memory.html">Memory Formal Reference</a>. This page provides detailed explanations, examples, and practical guidance.</p></div><h2 id="overview" tabindex="-1">Overview <a class="header-anchor" href="#overview" aria-label="Permalink to “Overview”">​</a></h2><ul><li>MEM declares internal arrays (word_width × depth) synthesized as Block RAM, Distributed RAM, or vendor-specific memories.</li><li>MEM supports multiple named read (<code>OUT</code>) ports (ASYNC or SYNC), synchronous write (<code>IN</code>) ports, and combined read/write (<code>INOUT</code>) ports.</li><li>Address widths are derived as <code>ceil(log2(depth))</code>. Minimum address width is 1 bit; a single-word memory (depth=1) uses a 1-bit address.</li><li>Initialization can be a sized literal (same value for every word) or a file (<code>@file(...)</code>).</li><li>MEM access obeys synchronous vs. asynchronous rules depending on port kind and block context.</li></ul><hr><h2 id="memory-port-modes" tabindex="-1">Memory Port Modes <a class="header-anchor" href="#memory-port-modes" aria-label="Permalink to “Memory Port Modes”">​</a></h2><p>JZ-HDL MEM declarations express all common BSRAM operating modes through port type combinations. The compiler analyzes port declarations to determine the required BSRAM mode.</p><table tabindex="0"><thead><tr><th style="text-align:left;">Port Configuration</th><th style="text-align:left;">BSRAM Mode</th><th style="text-align:left;">Description</th></tr></thead><tbody><tr><td style="text-align:left;"><code>OUT</code> only</td><td style="text-align:left;">Read Only Memory</td><td style="text-align:left;">Read-only, initialized at power-on</td></tr><tr><td style="text-align:left;"><code>INOUT</code> ×1</td><td style="text-align:left;">Single Port</td><td style="text-align:left;">One shared-address port for read and write</td></tr><tr><td style="text-align:left;"><code>IN</code> + <code>OUT</code></td><td style="text-align:left;">Semi-Dual Port</td><td style="text-align:left;">Separate write port (Port A) and read port (Port B)</td></tr><tr><td style="text-align:left;"><code>INOUT</code> ×2</td><td style="text-align:left;">Dual Port</td><td style="text-align:left;">Two independent read/write ports</td></tr></tbody></table><p><strong>Notes:</strong></p><ul><li><code>INOUT</code> ports cannot be mixed with <code>IN</code> or <code>OUT</code> ports in the same MEM declaration.</li><li>A MEM uses either <code>IN</code>/<code>OUT</code> ports (Semi-Dual Port or Read Only) or <code>INOUT</code> ports (Single Port or Dual Port), never both.</li></ul><hr><h2 id="canonical-syntax" tabindex="-1">Canonical Syntax <a class="header-anchor" href="#canonical-syntax" aria-label="Permalink to “Canonical Syntax”">​</a></h2><p>Use inside a <code>@module</code> body:</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>MEM(type=[BLOCK|DISTRIBUTED]) {</span></span>
<span class="line"><span>  &lt;name&gt; [&lt;word_width&gt;] [&lt;depth&gt;] = &lt;init&gt; {</span></span>
<span class="line"><span>    OUT   &lt;port_id&gt; [ASYNC | SYNC];</span></span>
<span class="line"><span>    IN    &lt;port_id&gt; { WRITE_MODE = &lt;mode&gt;; };   // or: IN &lt;port_id&gt;;</span></span>
<span class="line"><span>    INOUT &lt;port_id&gt;;                            // combined read/write port</span></span>
<span class="line"><span>  };</span></span>
<span class="line"><span>  ...</span></span>
<span class="line"><span>}</span></span></code></pre></div><ul><li><code>type</code> optional; compiler may infer when omitted.</li><li>Word width and depth are positive integers or module-local <code>CONST</code> names (compile-time).</li><li>The init value is a sized literal, <code>@file(&quot;path&quot;)</code>, <code>@file(CONST_NAME)</code>, or <code>@file(CONFIG.NAME)</code>.</li><li>Each MEM must include at least one port.</li><li><code>INOUT</code> ports cannot be mixed with <code>IN</code> or <code>OUT</code> ports in the same MEM declaration.</li></ul><hr><h2 id="declaration-rules" tabindex="-1">Declaration Rules <a class="header-anchor" href="#declaration-rules" aria-label="Permalink to “Declaration Rules”">​</a></h2><ul><li>Memory name must be unique in the module.</li><li>Port names must be unique within the MEM and must not clash with module-level identifiers.</li><li>If <code>type=BLOCK</code> is specified, the compiler verifies constraints (e.g., all OUT ports must be <code>SYNC</code> if required by target).</li><li>Address width = <code>clog2(depth)</code> (compile-time). Minimum address width is 1 bit; a single-word memory (depth=1) uses a 1-bit address (the sole word lives at address <code>1&#39;b0</code>).</li><li>Word init literals must not contain <code>x</code>.</li><li>File-based init files must not contain undefined bits.</li><li><code>INOUT</code> ports cannot be mixed with <code>IN</code> or <code>OUT</code> ports in the same MEM declaration.</li><li>All port names (<code>IN</code>, <code>OUT</code>, <code>INOUT</code>) must be distinct within the MEM block.</li></ul><hr><h2 id="port-types" tabindex="-1">Port Types <a class="header-anchor" href="#port-types" aria-label="Permalink to “Port Types”">​</a></h2><ul><li><p><code>OUT port_id ASYNC</code></p><ul><li>Asynchronous read: combinational address → data path.</li><li>Used in <code>ASYNCHRONOUS</code> logic (<code>=</code> / <code>=&gt;</code> / <code>&lt;=</code> as allowed).</li><li>Access syntax: <code>mem.port[address_expr]</code> (RHS).</li></ul></li><li><p><code>OUT port_id SYNC</code></p><ul><li>Synchronous read: address sampled on clock; data available on a latched read output next cycle.</li><li>Exposes two pseudo-fields: <code>.addr</code> and <code>.data</code>.</li><li><code>.addr</code> is assigned in <code>SYNCHRONOUS</code> blocks with <code>&lt;=</code>.</li><li><code>.data</code> is the read result, valid the cycle after <code>.addr</code> is sampled.</li><li><code>.data</code> is readable in any block (ASYNCHRONOUS or SYNCHRONOUS).</li><li><code>mem.port[addr]</code> indexing is illegal for SYNC ports (use <code>.addr</code>/<code>.data</code> instead).</li><li><code>.addr</code> may be assigned at most once per execution path.</li></ul></li><li><p><code>IN port_id</code></p><ul><li>Write port(s) are always synchronous: address and data sampled at clock.</li><li>Access in <code>SYNCHRONOUS</code> blocks: <code>mem.port[address_expr] &lt;= data_expr;</code></li><li>Each <code>IN</code> port may be written at most once per <code>SYNCHRONOUS</code> block (per Exclusive Assignment Rule).</li></ul></li><li><p><code>INOUT port_id</code></p><ul><li>Combined read/write port with shared address.</li><li>Always synchronous (no <code>ASYNC</code>/<code>SYNC</code> keyword; <code>SYNC</code> is implicit).</li><li>Exposes three pseudo-fields: <ul><li><code>.addr</code> — address input, set via <code>&lt;=</code> in <code>SYNCHRONOUS</code> blocks</li><li><code>.data</code> — read data output (1 cycle latency), readable in any block</li><li><code>.wdata</code> — write data input, assigned via <code>&lt;=</code> in <code>SYNCHRONOUS</code> blocks</li></ul></li><li>If <code>.wdata</code> is not assigned in a given execution path, no write occurs (read-only cycle).</li><li><code>.addr</code> and <code>.wdata</code> may each be assigned at most once per execution path.</li><li><code>mem.port[addr]</code> indexing syntax is illegal on <code>INOUT</code> ports (must use <code>.addr</code>).</li><li>Write modes (<code>WRITE_FIRST</code>, <code>READ_FIRST</code>, <code>NO_CHANGE</code>) apply to <code>INOUT</code> ports.</li></ul></li></ul><hr><h2 id="access-syntax-and-context" tabindex="-1">Access Syntax and Context <a class="header-anchor" href="#access-syntax-and-context" aria-label="Permalink to “Access Syntax and Context”">​</a></h2><ul><li><p>Asynchronous read (ASYNCHRONOUS blocks):</p><ul><li><code>dst = mem.port[index];</code></li><li><code>index</code> width must be ≤ address width; narrower indices are zero‑extended.</li><li><code>dst</code> width must equal <code>word_width</code>.</li></ul></li><li><p>Synchronous read via SYNC OUT (SYNCHRONOUS blocks):</p><ul><li><code>mem.port.addr &lt;= index;</code> schedules the address to be sampled.</li><li><code>reg_or_net &lt;= mem.port.data;</code> reads the registered output.</li><li>The <code>.data</code> output is valid one cycle after <code>.addr</code> is sampled.</li></ul></li><li><p>Synchronous write via IN (SYNCHRONOUS blocks):</p><ul><li><code>mem.port[index] &lt;= data;</code></li><li><code>index</code> and <code>data</code> may be narrower; zero‑extend to address/word width as required.</li><li>Conditional writes via <code>IF</code>/<code>SELECT</code> are allowed (must obey Exclusive Assignment Rule).</li></ul></li><li><p>INOUT access (SYNCHRONOUS blocks):</p><ul><li><code>mem.port.addr &lt;= index;</code> sets the shared address.</li><li><code>reg_or_net &lt;= mem.port.data;</code> reads the data (1 cycle latency).</li><li><code>mem.port.wdata &lt;= data;</code> writes data at the current address.</li><li>If <code>.wdata</code> is not assigned, no write occurs (read-only cycle).</li></ul></li><li><p>General width behavior:</p><ul><li>Address index widths must be ≤ derived address width. If narrower, zero-extend. If statically ≥ depth → compile error.</li><li>Data width must be ≤ <code>word_width</code>. If narrower, zero-extend or use modifiers (where supported) per assignment rules.</li></ul></li></ul><hr><h2 id="read-write-semantics" tabindex="-1">Read / Write Semantics <a class="header-anchor" href="#read-write-semantics" aria-label="Permalink to “Read / Write Semantics”">​</a></h2><ul><li>Reads and writes are independent ports; a memory may have multiple OUT and IN ports.</li><li>When a read and a write target the same address in the same cycle, the observed read value depends on the corresponding write port’s <code>WRITE_MODE</code>: <ul><li><code>WRITE_FIRST</code> (default): read returns the newly written data.</li><li><code>READ_FIRST</code>: read returns the old data (pre-write).</li><li><code>NO_CHANGE</code>: read retains its previous output value for that cycle.</li></ul></li><li>On subsequent cycles the stored word equals the write data regardless of mode.</li></ul><hr><h2 id="write-modes" tabindex="-1">Write Modes <a class="header-anchor" href="#write-modes" aria-label="Permalink to “Write Modes”">​</a></h2><p>You may declare write mode per <code>IN</code> or <code>INOUT</code> port:</p><p>Shorthand:</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>IN wr;                // default WRITE_FIRST</span></span>
<span class="line"><span>IN wr WRITE_FIRST;</span></span>
<span class="line"><span>IN wr READ_FIRST;</span></span>
<span class="line"><span>IN wr NO_CHANGE;</span></span>
<span class="line"><span></span></span>
<span class="line"><span>INOUT rw;             // default WRITE_FIRST</span></span>
<span class="line"><span>INOUT rw WRITE_FIRST;</span></span>
<span class="line"><span>INOUT rw READ_FIRST;</span></span>
<span class="line"><span>INOUT rw NO_CHANGE;</span></span></code></pre></div><p>Attribute form:</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>IN wr {</span></span>
<span class="line"><span>  WRITE_MODE = READ_FIRST;</span></span>
<span class="line"><span>};</span></span>
<span class="line"><span></span></span>
<span class="line"><span>INOUT rw {</span></span>
<span class="line"><span>  WRITE_MODE = READ_FIRST;</span></span>
<span class="line"><span>};</span></span></code></pre></div><p>Meaning:</p><ul><li>WRITE_FIRST — newly written data visible on same-cycle reads.</li><li>READ_FIRST — old data visible on same-cycle reads.</li><li>NO_CHANGE — read output unchanged for that cycle.</li></ul><p>For <code>INOUT</code> ports, write mode controls what <code>.data</code> shows when <code>.wdata</code> is assigned in the same cycle.</p><hr><h2 id="initialization" tabindex="-1">Initialization <a class="header-anchor" href="#initialization" aria-label="Permalink to “Initialization”">​</a></h2><ul><li>Literal initialization: the init value is a sized literal (must not contain <code>x</code>) and applies to all words. <ul><li>Example: <code>mem [8] [256] = 8&#39;h00 { ... }</code></li></ul></li><li>File-based initialization: <code>= @file(&quot;path&quot;)</code>, <code>= @file(CONST_NAME)</code>, or <code>= @file(CONFIG.NAME)</code><ul><li>The path argument may be a literal string, a module-local string CONST, or a project-level string CONFIG reference.</li><li>Using a numeric CONST/CONFIG where a string path is expected is a compile error (<code>CONST_NUMERIC_IN_STRING_CONTEXT</code>).</li><li>Supported formats: <code>.bin</code>, <code>.hex</code> (Intel HEX), <code>.mif</code>, <code>.coe</code>, <code>.mem</code> (Verilog memory format with <code>0</code>/<code>1</code> and <code>//</code> comments), and tool-specific formats.</li><li>File size must be ≤ <code>depth × word_width</code> bits. Smaller files are zero-padded. Larger files → compile error.</li><li>Files must not encode unknown bits; any undefined bits cause a compile error.</li></ul></li><li>Initialization evaluated at compile time.</li></ul><hr><h2 id="derived-address-width-and-bounds" tabindex="-1">Derived Address Width and Bounds <a class="header-anchor" href="#derived-address-width-and-bounds" aria-label="Permalink to “Derived Address Width and Bounds”">​</a></h2><ul><li>Address width W = <code>clog2(depth)</code> (compile-time).</li><li>Minimum address width is 1 bit; a single-word memory (depth=1) uses a 1-bit address (the sole word lives at address <code>1&#39;b0</code>).</li><li>If an address expression is statically provable ≥ <code>depth</code> → compile-time error.</li><li>If an address may be out of range at runtime, the behavior: <ul><li>For asynchronous reads: implementation-defined / simulation must abort (per MUX/INDEX rules).</li><li>For gslice/gsbit-like constructs, out-of-range read bits return 0; but for plain mem access, tools must follow the spec (static OOB is error; dynamic OOB is tool-defined — prefer guarding or ensuring index width correctness).</li></ul></li></ul><p>(Compiler implementations should statically check constants; for dynamic indices, zero-extend narrower indices and do bounds handling per the design policy.)</p><hr><h2 id="constraints-rules-summary" tabindex="-1">Constraints &amp; Rules Summary <a class="header-anchor" href="#constraints-rules-summary" aria-label="Permalink to “Constraints &amp; Rules Summary”">​</a></h2><ul><li>MEM must have at least one declared port.</li><li>OUT ASYNC reads may not be used in SYNCHRONOUS blocks as RHS without appropriate <code>&lt;=</code> semantics.</li><li>IN ports may only be written in SYNCHRONOUS blocks. Writing in ASYNCHRONOUS → compile error.</li><li>Each <code>IN</code> port: at most one write per <code>SYNCHRONOUS</code> block (per-path exclusivity applies).</li><li>Each MEM port name unique per MEM and distinct from module identifiers.</li><li>Literal init must not contain <code>x</code>.</li><li>CONST names in word_width/depth resolved in module <code>CONST</code> scope; must be resolvable at compile time.</li></ul><hr><h2 id="examples" tabindex="-1">Examples <a class="header-anchor" href="#examples" aria-label="Permalink to “Examples”">​</a></h2><p>Simple ROM (async read)</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>MEM {</span></span>
<span class="line"><span>  sine_lut [8] [256] = @file(&quot;sine_table.hex&quot;) {</span></span>
<span class="line"><span>    OUT addr ASYNC;</span></span>
<span class="line"><span>  };</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>ASYNCHRONOUS {</span></span>
<span class="line"><span>  data = sine_lut.addr[index];</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>Synchronous register-file style</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>MEM {</span></span>
<span class="line"><span>  regfile [32] [32] = 32&#39;h0000_0000 {</span></span>
<span class="line"><span>    OUT rd_a ASYNC;</span></span>
<span class="line"><span>    OUT rd_b ASYNC;</span></span>
<span class="line"><span>    IN  wr;</span></span>
<span class="line"><span>  };</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>ASYNCHRONOUS {</span></span>
<span class="line"><span>  rd_a_out = regfile.rd_a[rd_addr_a];</span></span>
<span class="line"><span>  rd_b_out = regfile.rd_b[rd_addr_b];</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>SYNCHRONOUS(CLK=clk) {</span></span>
<span class="line"><span>  IF (wr_en) {</span></span>
<span class="line"><span>    regfile.wr[wr_addr] &lt;= wr_data;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>Synchronous read (registered output)</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>MEM {</span></span>
<span class="line"><span>  cache [32] [1024] = 32&#39;h0000_0000 {</span></span>
<span class="line"><span>    OUT rd SYNC;</span></span>
<span class="line"><span>  };</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>SYNCHRONOUS(CLK=clk) {</span></span>
<span class="line"><span>  cache.rd.addr &lt;= addr;</span></span>
<span class="line"><span>  read_data &lt;= cache.rd.data;</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>Dual-write prohibition (illegal)</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>SYNCHRONOUS(CLK=clk) {</span></span>
<span class="line"><span>  mem.wr[a] &lt;= x;</span></span>
<span class="line"><span>  mem.wr[b] &lt;= y;  // ERROR: same IN port written twice in the same block</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>Single Port Memory (INOUT)</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>MEM(TYPE=BLOCK) {</span></span>
<span class="line"><span>  mem [16] [256] = 16&#39;h0000 {</span></span>
<span class="line"><span>    INOUT rw;</span></span>
<span class="line"><span>  };</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>SYNCHRONOUS(CLK=clk) {</span></span>
<span class="line"><span>  mem.rw.addr &lt;= addr;</span></span>
<span class="line"><span>  rd_data &lt;= mem.rw.data;</span></span>
<span class="line"><span>  IF (wr_en) {</span></span>
<span class="line"><span>    mem.rw.wdata &lt;= wr_data;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>True Dual Port Memory (2× INOUT)</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>MEM(TYPE=BLOCK) {</span></span>
<span class="line"><span>  mem [16] [256] = 16&#39;h0000 {</span></span>
<span class="line"><span>    INOUT port_a;</span></span>
<span class="line"><span>    INOUT port_b;</span></span>
<span class="line"><span>  };</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>SYNCHRONOUS(CLK=clk) {</span></span>
<span class="line"><span>  mem.port_a.addr &lt;= addr_a;</span></span>
<span class="line"><span>  rd_data_a &lt;= mem.port_a.data;</span></span>
<span class="line"><span>  IF (wr_en_a) {</span></span>
<span class="line"><span>    mem.port_a.wdata &lt;= wr_data_a;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  mem.port_b.addr &lt;= addr_b;</span></span>
<span class="line"><span>  rd_data_b &lt;= mem.port_b.data;</span></span>
<span class="line"><span>  IF (wr_en_b) {</span></span>
<span class="line"><span>    mem.port_b.wdata &lt;= wr_data_b;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>}</span></span></code></pre></div><p><strong>Note:</strong> For True Dual Port memories, write behavior when both ports write to the same address in the same cycle is undefined (hardware-dependent).</p><p>Synchronous FIFO</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>@module sync_fifo_8x32</span></span>
<span class="line"><span>  CONST {</span></span>
<span class="line"><span>    WIDTH = 8;</span></span>
<span class="line"><span>    DEPTH = 32;</span></span>
<span class="line"><span>    ADDR_WIDTH = 5;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  PORT {</span></span>
<span class="line"><span>    IN  [8] din;</span></span>
<span class="line"><span>    OUT [8] dout;</span></span>
<span class="line"><span>    IN  [1] wr_en;</span></span>
<span class="line"><span>    IN  [1] rd_en;</span></span>
<span class="line"><span>    IN  [1] clk;</span></span>
<span class="line"><span>    OUT [1] full;</span></span>
<span class="line"><span>    OUT [1] empty;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  REGISTER {</span></span>
<span class="line"><span>    wr_ptr [ADDR_WIDTH + 1] = {(ADDR_WIDTH + 1){1&#39;b0}};</span></span>
<span class="line"><span>    rd_ptr [ADDR_WIDTH + 1] = {(ADDR_WIDTH + 1){1&#39;b0}};</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  MEM {</span></span>
<span class="line"><span>    fifo_mem [8] [32] = 8&#39;h00 {</span></span>
<span class="line"><span>      OUT rd ASYNC;</span></span>
<span class="line"><span>      IN  wr;</span></span>
<span class="line"><span>    };</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  ASYNCHRONOUS {</span></span>
<span class="line"><span>    full = (wr_ptr[ADDR_WIDTH] != rd_ptr[ADDR_WIDTH]) &amp;</span></span>
<span class="line"><span>           (wr_ptr[ADDR_WIDTH - 1 : 0] == rd_ptr[ADDR_WIDTH - 1 : 0]);</span></span>
<span class="line"><span>    empty = (wr_ptr == rd_ptr) ? 1&#39;b1 : 1&#39;b0;</span></span>
<span class="line"><span>    dout = fifo_mem.rd[rd_ptr[ADDR_WIDTH - 1 : 0]];</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  SYNCHRONOUS(CLK=clk) {</span></span>
<span class="line"><span>    IF (wr_en &amp; ~full) {</span></span>
<span class="line"><span>      fifo_mem.wr[wr_ptr[ADDR_WIDTH - 1 : 0]] &lt;= din;</span></span>
<span class="line"><span>      wr_ptr &lt;= wr_ptr + 1;</span></span>
<span class="line"><span>    }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>    IF (rd_en &amp; ~empty) {</span></span>
<span class="line"><span>      rd_ptr &lt;= rd_ptr + 1;</span></span>
<span class="line"><span>    }</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>@endmod</span></span></code></pre></div><p>Registered Read Cache (Semi-Dual Port with SYNC read)</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>@module l1_cache_64x256</span></span>
<span class="line"><span>  CONST {</span></span>
<span class="line"><span>    LINE_WIDTH = 64;</span></span>
<span class="line"><span>    NUM_LINES = 256;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  PORT {</span></span>
<span class="line"><span>    IN  [8] read_addr;</span></span>
<span class="line"><span>    OUT [64] read_data;</span></span>
<span class="line"><span>    IN  [8] write_addr;</span></span>
<span class="line"><span>    IN  [64] write_data;</span></span>
<span class="line"><span>    IN  [1] write_en;</span></span>
<span class="line"><span>    IN  [1] clk;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  MEM {</span></span>
<span class="line"><span>    cache_mem [64] [256] = 64&#39;h0000_0000_0000_0000 {</span></span>
<span class="line"><span>      OUT rd SYNC;</span></span>
<span class="line"><span>      IN  wr;</span></span>
<span class="line"><span>    };</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  SYNCHRONOUS(CLK=clk) {</span></span>
<span class="line"><span>    cache_mem.rd.addr &lt;= read_addr;</span></span>
<span class="line"><span>    read_data &lt;= cache_mem.rd.data;</span></span>
<span class="line"><span></span></span>
<span class="line"><span>    IF (write_en) {</span></span>
<span class="line"><span>      cache_mem.wr[write_addr] &lt;= write_data;</span></span>
<span class="line"><span>    }</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>@endmod</span></span></code></pre></div><p>Triple-Port Memory (2 Read, 1 Write)</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>@module triple_port_mem_32x256</span></span>
<span class="line"><span>  PORT {</span></span>
<span class="line"><span>    IN  [8] rd_addr_0;</span></span>
<span class="line"><span>    IN  [8] rd_addr_1;</span></span>
<span class="line"><span>    OUT [32] rd_data_0;</span></span>
<span class="line"><span>    OUT [32] rd_data_1;</span></span>
<span class="line"><span>    IN  [8] wr_addr;</span></span>
<span class="line"><span>    IN  [32] wr_data;</span></span>
<span class="line"><span>    IN  [1] wr_en;</span></span>
<span class="line"><span>    IN  [1] clk;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  MEM {</span></span>
<span class="line"><span>    mem [32] [256] = 32&#39;h0000_0000 {</span></span>
<span class="line"><span>      OUT rd_0 ASYNC;</span></span>
<span class="line"><span>      OUT rd_1 ASYNC;</span></span>
<span class="line"><span>      IN  wr;</span></span>
<span class="line"><span>    };</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  ASYNCHRONOUS {</span></span>
<span class="line"><span>    rd_data_0 = mem.rd_0[rd_addr_0];</span></span>
<span class="line"><span>    rd_data_1 = mem.rd_1[rd_addr_1];</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  SYNCHRONOUS(CLK=clk) {</span></span>
<span class="line"><span>    IF (wr_en) {</span></span>
<span class="line"><span>      mem.wr[wr_addr] &lt;= wr_data;</span></span>
<span class="line"><span>    }</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>@endmod</span></span></code></pre></div><p>Quad-Port Memory (2 Read, 2 Write)</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>@module quad_port_mem_16x128</span></span>
<span class="line"><span>  PORT {</span></span>
<span class="line"><span>    IN  [7] rd_addr_0;</span></span>
<span class="line"><span>    IN  [7] rd_addr_1;</span></span>
<span class="line"><span>    OUT [16] rd_data_0;</span></span>
<span class="line"><span>    OUT [16] rd_data_1;</span></span>
<span class="line"><span>    IN  [7] wr_addr_0;</span></span>
<span class="line"><span>    IN  [7] wr_addr_1;</span></span>
<span class="line"><span>    IN  [16] wr_data_0;</span></span>
<span class="line"><span>    IN  [16] wr_data_1;</span></span>
<span class="line"><span>    IN  [1] wr_en_0;</span></span>
<span class="line"><span>    IN  [1] wr_en_1;</span></span>
<span class="line"><span>    IN  [1] clk;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  MEM {</span></span>
<span class="line"><span>    mem [16] [128] = 16&#39;h0000 {</span></span>
<span class="line"><span>      OUT rd_0 ASYNC;</span></span>
<span class="line"><span>      OUT rd_1 ASYNC;</span></span>
<span class="line"><span>      IN  wr_0;</span></span>
<span class="line"><span>      IN  wr_1;</span></span>
<span class="line"><span>    };</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  ASYNCHRONOUS {</span></span>
<span class="line"><span>    rd_data_0 = mem.rd_0[rd_addr_0];</span></span>
<span class="line"><span>    rd_data_1 = mem.rd_1[rd_addr_1];</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  SYNCHRONOUS(CLK=clk) {</span></span>
<span class="line"><span>    IF (wr_en_0) {</span></span>
<span class="line"><span>      mem.wr_0[wr_addr_0] &lt;= wr_data_0;</span></span>
<span class="line"><span>    }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>    IF (wr_en_1) {</span></span>
<span class="line"><span>      mem.wr_1[wr_addr_1] &lt;= wr_data_1;</span></span>
<span class="line"><span>    }</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>@endmod</span></span></code></pre></div><p>Configurable Memory with Parameters</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>@module param_mem</span></span>
<span class="line"><span>  CONST {</span></span>
<span class="line"><span>    WORD_WIDTH = 32;</span></span>
<span class="line"><span>    DEPTH = 256;</span></span>
<span class="line"><span>    ADDR_WIDTH = 8;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  PORT {</span></span>
<span class="line"><span>    IN  [ADDR_WIDTH] rd_addr;</span></span>
<span class="line"><span>    IN  [ADDR_WIDTH] wr_addr;</span></span>
<span class="line"><span>    IN  [WORD_WIDTH] wr_data;</span></span>
<span class="line"><span>    IN  [1] wr_en;</span></span>
<span class="line"><span>    OUT [WORD_WIDTH] rd_data;</span></span>
<span class="line"><span>    IN  [1] clk;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  MEM {</span></span>
<span class="line"><span>    storage [WORD_WIDTH] [DEPTH] = {WORD_WIDTH{1&#39;b0}} {</span></span>
<span class="line"><span>      OUT rd SYNC;</span></span>
<span class="line"><span>      IN  wr;</span></span>
<span class="line"><span>    };</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  SYNCHRONOUS(CLK=clk) {</span></span>
<span class="line"><span>    storage.rd.addr &lt;= rd_addr;</span></span>
<span class="line"><span>    rd_data &lt;= storage.rd.data;</span></span>
<span class="line"><span></span></span>
<span class="line"><span>    IF (wr_en) {</span></span>
<span class="line"><span>      storage.wr[wr_addr] &lt;= wr_data;</span></span>
<span class="line"><span>    }</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>@endmod</span></span></code></pre></div><h2 id="mem-in-module-instantiation" tabindex="-1">MEM in Module Instantiation <a class="header-anchor" href="#mem-in-module-instantiation" aria-label="Permalink to “MEM in Module Instantiation”">​</a></h2><p>Memories are internal to modules and cannot be directly accessed from parent modules. To expose memory operations, wrap them in a module interface:</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>@module memory_wrapper</span></span>
<span class="line"><span>  PORT {</span></span>
<span class="line"><span>    IN  [1]  clk;</span></span>
<span class="line"><span>    IN  [8]  addr;</span></span>
<span class="line"><span>    IN  [16] wr_data;</span></span>
<span class="line"><span>    IN  [1]  wr_en;</span></span>
<span class="line"><span>    OUT [16] rd_data;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  MEM(TYPE=BLOCK) {</span></span>
<span class="line"><span>    mem [16] [256] = 16&#39;h0000 {</span></span>
<span class="line"><span>      INOUT rw;</span></span>
<span class="line"><span>    };</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span></span></span>
<span class="line"><span>  SYNCHRONOUS(CLK=clk) {</span></span>
<span class="line"><span>    mem.rw.addr &lt;= addr;</span></span>
<span class="line"><span>    rd_data &lt;= mem.rw.data;</span></span>
<span class="line"><span>    IF (wr_en) {</span></span>
<span class="line"><span>      mem.rw.wdata &lt;= wr_data;</span></span>
<span class="line"><span>    }</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>@endmod</span></span></code></pre></div><p>The parent module instantiates the wrapper and accesses memory through its ports.</p><h2 id="const-evaluation-in-mem" tabindex="-1">CONST Evaluation in MEM <a class="header-anchor" href="#const-evaluation-in-mem" aria-label="Permalink to “CONST Evaluation in MEM”">​</a></h2><p>Numeric CONST names may be used in word_width and depth expressions. These are resolved in the module&#39;s CONST scope at compile time:</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>CONST {</span></span>
<span class="line"><span>  WIDTH = 32;</span></span>
<span class="line"><span>  DEPTH = 1024;</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>MEM {</span></span>
<span class="line"><span>  mem [WIDTH] [DEPTH] = {WIDTH{1&#39;b0}} {</span></span>
<span class="line"><span>    OUT rd SYNC;</span></span>
<span class="line"><span>    IN  wr;</span></span>
<span class="line"><span>  };</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>String CONST names may be used in <code>@file()</code> path arguments:</p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>CONST {</span></span>
<span class="line"><span>  WIDTH = 32;</span></span>
<span class="line"><span>  DEPTH = 1024;</span></span>
<span class="line"><span>  INIT_FILE = &quot;firmware.bin&quot;;</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>MEM {</span></span>
<span class="line"><span>  rom [WIDTH] [DEPTH] = @file(INIT_FILE) {</span></span>
<span class="line"><span>    OUT rd ASYNC;</span></span>
<span class="line"><span>  };</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>Project-level string CONFIG references may also be used: <code>@file(CONFIG.FIRMWARE)</code>.</p><p>When used with OVERRIDE, the overriding module&#39;s CONST values apply, allowing the same module to be instantiated with different memory sizes.</p><hr><h2 id="common-errors-and-diagnostics" tabindex="-1">Common Errors and Diagnostics <a class="header-anchor" href="#common-errors-and-diagnostics" aria-label="Permalink to “Common Errors and Diagnostics”">​</a></h2><ul><li><p>Declaration Errors</p><ul><li>Invalid or duplicate MEM name.</li><li>Invalid <code>word_width</code> or <code>depth</code> (≤ 0 or unresolved CONST).</li><li>Missing initialization clause.</li><li>Port name duplicates or conflicts with module identifiers.</li><li><code>INOUT</code> ports mixed with <code>IN</code> or <code>OUT</code> ports in the same MEM declaration.</li><li><code>INOUT</code> port declared with <code>ASYNC</code> keyword (not supported).</li></ul></li><li><p>Access Errors</p><ul><li>Asynchronous read used in SYNCHRONOUS incorrectly (forgetting <code>&lt;=</code>).</li><li>Write in ASYNCHRONOUS block.</li><li>Multiple writes to the same <code>IN</code> port in one <code>SYNCHRONOUS</code> block (Exclusive Assignment violation).</li><li>Address or data width mismatch (too wide) — truncation is not implicit.</li><li>Constant out-of-range address (address literal ≥ depth) — compile error.</li><li>Using <code>mem.port[addr]</code> indexing syntax on <code>INOUT</code> ports (must use <code>.addr</code>).</li><li>Assigning <code>.wdata</code> in ASYNCHRONOUS block.</li><li>Assigning <code>.addr</code> in ASYNCHRONOUS block.</li><li>Multiple <code>.addr</code> assignments to the same <code>INOUT</code> port per execution path.</li><li>Multiple <code>.wdata</code> assignments to the same <code>INOUT</code> port per execution path.</li></ul></li><li><p>Initialization Errors</p><ul><li>Init literal overflow (literal intrinsic width &gt; declared word width).</li><li>Init file not found or too large for memory depth.</li><li>Init literal or file contains <code>x</code>/undefined bits.</li><li>Numeric CONST/CONFIG used in <code>@file()</code> path (<code>CONST_NUMERIC_IN_STRING_CONTEXT</code>).</li><li>String CONST/CONFIG used in width/depth expression (<code>CONST_STRING_IN_NUMERIC_CONTEXT</code>).</li></ul></li><li><p>Behavioral Warnings</p><ul><li>Port declared but never accessed.</li><li>Partial initialization (file smaller than memory) — zero-padding warning.</li><li>Using ASYNC reads in combinational paths that create loops — flow-sensitive loop detection may flag cycles.</li></ul></li></ul><hr><h2 id="synthesis-and-implementation-notes" tabindex="-1">Synthesis and Implementation Notes <a class="header-anchor" href="#synthesis-and-implementation-notes" aria-label="Permalink to “Synthesis and Implementation Notes”">​</a></h2><ul><li>Compiler infers FPGA/ASIC memory primitives from MEM declarations. <ul><li>ASYNC vs SYNC read ports influence whether the tool implements combinational read paths or registered outputs.</li><li><code>type=BLOCK</code> guides inference toward block RAMs; compiler may validate constraints (e.g., read port timing).</li></ul></li><li>Write modes map to vendor BRAM settings: <ul><li><code>WRITE_FIRST</code>, <code>READ_FIRST</code>, <code>NO_CHANGE</code> → vendor BRAM write-mode attributes.</li></ul></li><li>File-based initialization may be passed to the backend (vendor tools) as memory init files.</li><li>For small depths use DISTRIBUTED RAM; for larger depths prefer BLOCK BRAM. If omitted, compiler makes a choice based on <code>depth</code> and port timing.</li></ul><p><strong>Port Type Inference:</strong></p><ul><li><code>ASYNC</code> <code>OUT</code> ports → combinational read (data available same cycle)</li><li><code>SYNC</code> <code>OUT</code> ports → registered read (data available next cycle)</li><li><code>IN</code> ports → synchronous write (captured at clock edge)</li><li><code>INOUT</code> ports → synchronous read/write with shared address (read data available next cycle)</li></ul><hr><h2 id="mem-as-a-register-array-replacement" tabindex="-1">MEM as a Register Array Replacement <a class="header-anchor" href="#mem-as-a-register-array-replacement" aria-label="Permalink to “MEM as a Register Array Replacement”">​</a></h2><div class="info custom-block"><p class="custom-block-title">Looking for register arrays?</p><p>JZ-HDL does not support multi-dimensional REGISTER syntax (e.g., <code>name [depth] [width]</code>). Use MEM instead — it provides the same functionality with explicit port semantics.</p></div><p>A <code>MEM(type=DISTRIBUTED)</code> with an <code>OUT ASYNC</code> read port is the direct equivalent of a register array in other HDLs. Both synthesize to flip-flops plus a read mux, with identical timing:</p><ul><li><strong>Read latency</strong>: zero additional cycles (combinational, same as a register read)</li><li><strong>Write timing</strong>: synchronous, captured at the clock edge (same as a register write)</li><li><strong>Synthesis result</strong>: LUT-based storage with mux/decoder (same fabric resources)</li></ul><p>The only difference is that MEM requires explicit port declarations, which makes the number of read ports, write ports, and their timing (ASYNC vs SYNC) unambiguous in the source code.</p><p><strong>Example: 8-entry, 32-bit register file with 2 read ports and 1 write port</strong></p><div class="language-text"><button title="Copy Code" class="copy"></button><span class="lang">text</span><pre class="shiki shiki-themes github-light github-dark" style="--shiki-light:#24292e;--shiki-dark:#e1e4e8;--shiki-light-bg:#fff;--shiki-dark-bg:#24292e;" tabindex="0" dir="ltr"><code><span class="line"><span>MEM(type=DISTRIBUTED) {</span></span>
<span class="line"><span>  regfile [32] [8] = 32&#39;h0000_0000 {</span></span>
<span class="line"><span>    OUT rd_a ASYNC;</span></span>
<span class="line"><span>    OUT rd_b ASYNC;</span></span>
<span class="line"><span>    IN  wr;</span></span>
<span class="line"><span>  };</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>ASYNCHRONOUS {</span></span>
<span class="line"><span>  read_data_a = regfile.rd_a[rd_addr_a];</span></span>
<span class="line"><span>  read_data_b = regfile.rd_b[rd_addr_b];</span></span>
<span class="line"><span>}</span></span>
<span class="line"><span></span></span>
<span class="line"><span>SYNCHRONOUS(CLK=clk) {</span></span>
<span class="line"><span>  IF (wr_en) {</span></span>
<span class="line"><span>    regfile.wr[wr_addr] &lt;= wr_data;</span></span>
<span class="line"><span>  }</span></span>
<span class="line"><span>}</span></span></code></pre></div><p>This is equivalent to what <code>reg [31:0] regfile [0:7]</code> would provide in Verilog, but with the read/write port structure made explicit.</p><hr><h2 id="best-practices" tabindex="-1">Best Practices <a class="header-anchor" href="#best-practices" aria-label="Permalink to “Best Practices”">​</a></h2><ul><li>Always declare <code>WORD_WIDTH</code> and <code>DEPTH</code> using numeric <code>CONST</code> when parameterizing modules.</li><li>Use string <code>CONST</code> or <code>CONFIG</code> for <code>@file()</code> paths to make initialization files configurable across builds.</li><li>Prefer explicit <code>clog2(DEPTH)</code> or <code>ADDR_WIDTH</code> constants for address signals so widths are consistent and self-documenting.</li><li>Use synchronous reads (<code>SYNC</code>) when you need registered, timing-stable outputs or to pipeline memory reads.</li><li>Guard dynamic indices when they might be out-of-range or ensure index width covers <code>clog2(depth)</code>.</li><li>For register files, prefer ASYNC read ports for zero-latency reads and a single synchronous write port; ensure write-first/read-first behavior matches desired architectural semantics.</li><li>Avoid reading and writing the same address from different ports in the same cycle unless the write mode semantics are explicitly what you require.</li></ul><hr><h2 id="troubleshooting-checklist" tabindex="-1">Troubleshooting Checklist <a class="header-anchor" href="#troubleshooting-checklist" aria-label="Permalink to “Troubleshooting Checklist”">​</a></h2><ul><li>If you see a floating/undefined read value: <ul><li>Verify read port and write ports address widths and that at least one driver provides known data.</li><li>Check for mistaken ASYNCHRONOUS write attempts (illegal).</li></ul></li><li>If synthesis maps memory into many small LUTs: <ul><li>Consider changing <code>type</code> or increasing depth to encourage BRAM inference or supply vendor-specific attributes.</li></ul></li><li>If reads return unexpected values on same-cycle read/write: <ul><li>Confirm the <code>IN</code> port’s <code>WRITE_MODE</code> and whether you intended WRITE_FIRST (default) or READ_FIRST.</li></ul></li></ul>`,107)])])}const u=a(l,[["render",p]]);export{m as __pageData,u as default};
