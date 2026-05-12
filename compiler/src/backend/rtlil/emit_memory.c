/**
 * @file emit_memory.c
 * @brief Emits RTLIL memory initialization and write-port support cells.
 */
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "rtlil_internal.h"
#include "ir.h"
#include "util.h"

/* Reuse alias helpers from Verilog backend. */
#include "backend/verilog-2005/verilog_internal.h"

/**
 * @brief Build an all-zero constant sigspec string.
 * @param buf Destination buffer.
 * @param buf_size Size of `buf` in bytes.
 * @param width Bit width to emit.
 */
static void const_val_sigspec_zero(char *buf, int buf_size, int width);
static int s_mem_emit_errors = 0;

/**
 * @brief Append text to a sigspec buffer.
 * @param buf Destination buffer.
 * @param buf_size Size of `buf` in bytes.
 * @param pos In-out write position within `buf`.
 * @param text Text to append.
 * @return `0` on success, or `-1` if the append would overflow.
 */
static int append_sigspec_text(char *buf, size_t buf_size, size_t *pos,
                               const char *text);

/**
 * @brief Check whether a memory-init blob matches the declared memory size.
 * @param mem Memory declaration to validate.
 * @return Nonzero when the blob size matches the declared depth and width.
 */
static int mem_init_blob_has_expected_size(const IR_Memory *mem);

/**
 * @brief Read one bit from a packed memory-init word.
 * @param word_bytes Packed bytes for a single memory word.
 * @param bytes_per_word Size of `word_bytes` in bytes.
 * @param word_width Declared word width in bits.
 * @param bit_from_msb Zero-based bit index counted from the most-significant
 * output bit.
 * @return Selected bit value as `0` or `1`.
 */
static unsigned mem_init_word_bit(const uint8_t *word_bytes,
                                  int bytes_per_word, int word_width,
                                  int bit_from_msb);

/**
 * @brief Emit one initialized memory word as an RTLIL constant.
 * @param out Destination RTLIL stream.
 * @param word_bytes Packed bytes for a single memory word.
 * @param bytes_per_word Size of `word_bytes` in bytes.
 * @param word_width Declared word width in bits.
 */
static void rtlil_emit_blob_word_const(FILE *out, const uint8_t *word_bytes,
                                       int bytes_per_word, int word_width);

/**
 * @brief Emit one `$meminit_v2` cell for a memory address.
 * @param out Destination RTLIL stream.
 * @param mem Memory being initialized.
 * @param addr Address index to initialize.
 */
static void emit_meminit_cell(FILE *out, const IR_Memory *mem, int addr);
static int rtlil_mem_init_emission_within_limit(const IR_Memory *mem);

static int append_sigspec_text(char *buf,
                               size_t buf_size,
                               size_t *pos,
                               const char *text)
{
    if (!buf || buf_size == 0 || !pos || !text) {
        return -1;
    }
    if (*pos >= buf_size) {
        buf[buf_size - 1] = '\0';
        return -1;
    }

    int written = snprintf(buf + *pos, buf_size - *pos, "%s", text);
    if (written < 0) {
        buf[*pos] = '\0';
        return -1;
    }
    if ((size_t)written >= buf_size - *pos) {
        *pos = buf_size - 1;
        buf[*pos] = '\0';
        return -1;
    }

    *pos += (size_t)written;
    return 0;
}

static int mem_init_blob_has_expected_size(const IR_Memory *mem)
{
    if (!mem || !mem->init.blob) return 0;

    int width = mem->word_width > 0 ? mem->word_width : 1;
    size_t bytes_per_word = (size_t)((width + 7) / 8);
    size_t depth = mem->depth > 0 ? (size_t)mem->depth : 0u;
    size_t expected_num_bytes = depth * bytes_per_word;

    return mem->init.blob->num_bytes >= 0 &&
           (size_t)mem->init.blob->num_bytes == expected_num_bytes;
}

static unsigned mem_init_word_bit(const uint8_t *word_bytes,
                                  int bytes_per_word,
                                  int word_width,
                                  int bit_from_msb)
{
    int target_bit = word_width - 1 - bit_from_msb;
    int byte_in_word = bytes_per_word - 1 - (target_bit / 8);
    int bit_in_byte = target_bit % 8;
    return (unsigned)((word_bytes[byte_in_word] >> bit_in_byte) & 1u);
}

static void rtlil_emit_blob_word_const(FILE *out,
                                       const uint8_t *word_bytes,
                                       int bytes_per_word,
                                       int word_width)
{
    fprintf(out, "%d'", word_width);
    for (int bit = 0; bit < word_width; ++bit) {
        fputc(mem_init_word_bit(word_bytes, bytes_per_word,
                                word_width, bit) ? '1' : '0',
              out);
    }
}

static void emit_meminit_cell(FILE *out,
                              const IR_Memory *mem,
                              int addr)
{
    if (!out || !mem || !mem->name || !mem->init.blob) return;

    int width = mem->word_width > 0 ? mem->word_width : 1;
    int addr_width = mem->address_width > 0 ? mem->address_width : 1;
    int bytes_per_word = (width + 7) / 8;
    const uint8_t *word = mem->init.blob->bytes +
                          ((size_t)addr * (size_t)bytes_per_word);

    int id = rtlil_next_id();
    rtlil_indent(out, 1);
    fprintf(out, "cell $meminit_v2 $auto$%d\n", id);
    rtlil_indent(out, 2);
    fprintf(out, "parameter \\MEMID \"\\\\%s\"\n", mem->name);
    rtlil_indent(out, 2);
    fprintf(out, "parameter \\ABITS %d\n", addr_width);
    rtlil_indent(out, 2);
    fprintf(out, "parameter \\WIDTH %d\n", width);
    rtlil_indent(out, 2);
    fprintf(out, "parameter \\WORDS 1\n");
    rtlil_indent(out, 2);
    fprintf(out, "parameter \\PRIORITY 0\n");
    rtlil_indent(out, 2);
    fprintf(out, "connect \\ADDR ");
    rtlil_emit_const_val(out, addr_width, (uint64_t)addr);
    fputc('\n', out);
    rtlil_indent(out, 2);
    fprintf(out, "connect \\DATA ");
    rtlil_emit_blob_word_const(out, word, bytes_per_word, width);
    fputc('\n', out);
    rtlil_indent(out, 2);
    fprintf(out, "connect \\EN %d'", width);
    for (int bit = 0; bit < width; ++bit) {
        fputc('1', out);
    }
    fputc('\n', out);
    rtlil_indent(out, 1);
    fprintf(out, "end\n");
}

static int rtlil_mem_init_emission_within_limit(const IR_Memory *mem)
{
    size_t limit = jz_input_limit_value(JZ_LIMIT_RTLIL_MEM_INIT_EMIT_BYTES);
    size_t width = 0;
    size_t depth = 0;
    size_t addr_width = 0;
    size_t per_cell = 0;
    size_t total = 0;

    if (!mem) return 0;

    width = (size_t)(mem->word_width > 0 ? mem->word_width : 1);
    depth = (size_t)(mem->depth > 0 ? mem->depth : 0);
    addr_width = (size_t)(mem->address_width > 0 ? mem->address_width : 1);

    if (jz_size_mul_checked(width, 2u, &per_cell) != 0) {
        return 0;
    }
    if (jz_size_add_checked(per_cell, addr_width + 128u, &per_cell) != 0) {
        return 0;
    }
    if (jz_size_mul_checked(per_cell, depth, &total) != 0) {
        return 0;
    }
    return total <= limit;
}

/**
 * @struct MemWriteInfo
 * @brief Captures one memory write discovered in a statement tree.
 */
typedef struct {
    const IR_MemWriteStmt *write;     /**< Memory write statement to emit. */
    const IR_Expr *guard_condition;   /**< Guard condition, or `NULL` if unconditional. */
    int clock_domain_id;              /**< Source clock-domain ID, or `-1` for async logic. */
} MemWriteInfo;

#define MAX_MEM_WRITES 64

static int s_mem_writes_count = 0;
static MemWriteInfo s_mem_writes[MAX_MEM_WRITES];

/**
 * @brief Collect memory writes reachable from a statement subtree.
 * @param stmt Statement subtree to walk.
 * @param guard Guard condition inherited from enclosing control flow.
 * @param cd_id Clock-domain identifier for the enclosing process, or `-1` for
 * async logic.
 */
static void collect_mem_writes_from_stmt(const IR_Stmt *stmt,
                                          const IR_Expr *guard,
                                          int cd_id)
{
    if (!stmt) return;

    switch (stmt->kind) {
    case STMT_MEM_WRITE:
        if (s_mem_writes_count < MAX_MEM_WRITES) {
            s_mem_writes[s_mem_writes_count].write = &stmt->u.mem_write;
            s_mem_writes[s_mem_writes_count].guard_condition = guard;
            s_mem_writes[s_mem_writes_count].clock_domain_id = cd_id;
            s_mem_writes_count++;
        }
        break;

    case STMT_BLOCK: {
        const IR_BlockStmt *blk = &stmt->u.block;
        for (int i = 0; i < blk->count; ++i) {
            collect_mem_writes_from_stmt(&blk->stmts[i], guard, cd_id);
        }
        break;
    }

    case STMT_IF: {
        const IR_IfStmt *ifs = &stmt->u.if_stmt;
        /* Simplification: pass the IF condition as the guard.
         * A more complete implementation would AND the condition chain. */
        collect_mem_writes_from_stmt(ifs->then_block, ifs->condition, cd_id);
        if (ifs->elif_chain)
            collect_mem_writes_from_stmt(ifs->elif_chain, guard, cd_id);
        if (ifs->else_block)
            collect_mem_writes_from_stmt(ifs->else_block, guard, cd_id);
        break;
    }

    case STMT_SELECT: {
        const IR_SelectStmt *sel = &stmt->u.select_stmt;
        for (int i = 0; i < sel->num_cases; ++i) {
            collect_mem_writes_from_stmt(sel->cases[i].body, guard, cd_id);
        }
        break;
    }

    default:
        break;
    }
}

/**
 * @brief Emit one `$memwr_v2` cell for a collected memory write.
 * @param out Destination RTLIL stream.
 * @param mod Module that owns the target memory.
 * @param info Collected write information to emit.
 */
static void emit_memwr_cell(FILE *out, const IR_Module *mod,
                              const MemWriteInfo *info)
{
    const IR_MemWriteStmt *mw = info->write;
    if (!mw || !mw->memory_name) return;

    /* Find the memory to get dimensions. */
    const IR_Memory *mem = NULL;
    for (int i = 0; i < mod->num_memories; ++i) {
        const IR_Memory *m = &mod->memories[i];
        if (m->name && strcmp(m->name, mw->memory_name) == 0) {
            mem = m;
            break;
        }
    }
    if (!mem) return;

    int width = mem->word_width > 0 ? mem->word_width : 1;
    int addr_width = mem->address_width > 0 ? mem->address_width : 1;

    /* Emit address and data sigspecs. */
    char addr_ss[RTLIL_SIGSPEC_MAX];
    if (mw->address) {
        rtlil_emit_expr(out, mod, mw->address, addr_ss, sizeof(addr_ss));
    } else {
        const_val_sigspec_zero(addr_ss, sizeof(addr_ss), addr_width);
    }

    char data_ss[RTLIL_SIGSPEC_MAX];
    if (mw->data) {
        rtlil_emit_expr(out, mod, mw->data, data_ss, sizeof(data_ss));
    } else {
        const_val_sigspec_zero(data_ss, sizeof(data_ss), width);
    }

    /* Write enable: gate with guard condition if present. */
    char en_ss[RTLIL_SIGSPEC_MAX];
    if (info->guard_condition) {
        /* Emit the guard expression as cells, get a 1-bit sigspec. */
        char guard_ss[RTLIL_SIGSPEC_MAX];
        rtlil_emit_expr(out, mod, info->guard_condition,
                         guard_ss, sizeof(guard_ss));

        /* Replicate the 1-bit enable to match the data width.
         * Build a concat: { guard guard ... guard } */
        size_t pos = 0;
        int overflow = append_sigspec_text(en_ss, sizeof(en_ss), &pos, "{ ");
        for (int b = 0; b < width && overflow == 0; ++b) {
            if (b > 0) {
                overflow = append_sigspec_text(en_ss, sizeof(en_ss), &pos, " ");
            }
            if (overflow == 0) {
                overflow = append_sigspec_text(en_ss, sizeof(en_ss), &pos,
                                               guard_ss);
            }
        }
        if (overflow == 0) {
            overflow = append_sigspec_text(en_ss, sizeof(en_ss), &pos, " }");
        }
        if (overflow != 0) {
            fprintf(stderr,
                    "error: memory write enable for memory %s in module %s exceeds RTLIL sigspec buffer\n",
                    mw->memory_name,
                    mod->name ? mod->name : "?");
            s_mem_emit_errors++;
            const_val_sigspec_zero(en_ss, sizeof(en_ss), width);
        }
    } else {
        /* Unconditional write: all bits enabled. */
        int pos = snprintf(en_ss, sizeof(en_ss), "%d'", width);
        for (int b = 0; b < width && pos < (int)sizeof(en_ss) - 1; ++b) {
            en_ss[pos++] = '1';
        }
        en_ss[pos] = '\0';
    }

    /* Find clock signal for this write. */
    const char *clk_name = "1'0";
    int clk_enable = 0;
    int clk_polarity = 1;

    if (info->clock_domain_id >= 0) {
        const IR_ClockDomain *cd = rtlil_find_clock_domain_by_id(
            mod, info->clock_domain_id);
        if (cd) {
            const IR_Signal *clk_sig = rtlil_find_signal_by_id(
                mod, cd->clock_signal_id);
            if (clk_sig && clk_sig->name) {
                static char clk_buf[128];
                snprintf(clk_buf, sizeof(clk_buf), "\\%s", clk_sig->name);
                clk_name = clk_buf;
                clk_enable = 1;
                clk_polarity = (cd->edge == EDGE_FALLING) ? 0 : 1;
            }
        }
    }

    int id = rtlil_next_id();
    rtlil_indent(out, 1);
    fprintf(out, "cell $memwr_v2 $auto$%d\n", id);
    rtlil_indent(out, 2);
    fprintf(out, "parameter \\MEMID \"\\\\%s\"\n", mw->memory_name);
    rtlil_indent(out, 2);
    fprintf(out, "parameter \\ABITS %d\n", addr_width);
    rtlil_indent(out, 2);
    fprintf(out, "parameter \\WIDTH %d\n", width);
    rtlil_indent(out, 2);
    fprintf(out, "parameter \\CLK_ENABLE %d\n", clk_enable);
    rtlil_indent(out, 2);
    fprintf(out, "parameter \\CLK_POLARITY %d\n", clk_polarity);
    rtlil_indent(out, 2);
    fprintf(out, "parameter \\PORTID 0\n");
    rtlil_indent(out, 2);
    fprintf(out, "parameter \\PRIORITY_MASK 0\n");
    rtlil_indent(out, 2);
    fprintf(out, "connect \\ADDR %s\n", addr_ss);
    rtlil_indent(out, 2);
    fprintf(out, "connect \\DATA %s\n", data_ss);
    rtlil_indent(out, 2);
    fprintf(out, "connect \\EN %s\n", en_ss);
    rtlil_indent(out, 2);
    fprintf(out, "connect \\CLK %s\n", clk_name);
    rtlil_indent(out, 1);
    fprintf(out, "end\n");
}

/* Helper to build a zero sigspec string. */
static void const_val_sigspec_zero(char *buf, int buf_size, int width)
{
    if (width <= 0) width = 1;
    int pos = snprintf(buf, (size_t)buf_size, "%d'", width);
    for (int i = 0; i < width && pos < buf_size - 1; ++i) {
        buf[pos++] = '0';
    }
    buf[pos] = '\0';
}

void rtlil_reset_memory_emit_errors(void)
{
    s_mem_emit_errors = 0;
}

int rtlil_memory_emit_errors(void)
{
    return s_mem_emit_errors;
}

/* -------------------------------------------------------------------------
 * Main entry point
 * -------------------------------------------------------------------------
 */

void rtlil_emit_memory_cells(FILE *out, const IR_Module *mod)
{
    if (!out || !mod) return;
    if (mod->num_memories <= 0) return;

    for (int i = 0; i < mod->num_memories; ++i) {
        const IR_Memory *mem = &mod->memories[i];
        if (mem->init_kind == MEM_INIT_FILE) {
            fprintf(stderr,
                    "error: unresolved file-based memory init for memory %s in module %s\n",
                    mem->name ? mem->name : "jz_mem",
                    mod->name ? mod->name : "?");
            s_mem_emit_errors++;
            continue;
        }
        if (mem->init_kind != MEM_INIT_BLOB || !mem->init.blob) {
            continue;
        }
        if (!mem_init_blob_has_expected_size(mem)) {
            int width = mem->word_width > 0 ? mem->word_width : 1;
            int bytes_per_word = (width + 7) / 8;
            size_t expected_num_bytes =
                (size_t)(mem->depth > 0 ? mem->depth : 0) *
                (size_t)bytes_per_word;
            fprintf(stderr,
                    "error: memory init blob size mismatch for memory %s in module %s (expected %zu bytes, got %d)\n",
                    mem->name ? mem->name : "jz_mem",
                    mod->name ? mod->name : "?",
                    expected_num_bytes,
                    mem->init.blob->num_bytes);
            s_mem_emit_errors++;
            continue;
        }
        if (!rtlil_mem_init_emission_within_limit(mem)) {
            fprintf(stderr,
                    "error: RTLIL memory init for memory %s in module %s exceeds the compiler safety emit-size limit\n",
                    mem->name ? mem->name : "jz_mem",
                    mod->name ? mod->name : "?");
            s_mem_emit_errors++;
            continue;
        }
        for (int addr = 0; addr < mem->depth; ++addr) {
            emit_meminit_cell(out, mem, addr);
        }
    }

    /* Collect all memory write statements from clock domains. */
    s_mem_writes_count = 0;

    for (int cd = 0; cd < mod->num_clock_domains; ++cd) {
        const IR_ClockDomain *domain = &mod->clock_domains[cd];
        if (domain->statements) {
            collect_mem_writes_from_stmt(domain->statements, NULL, domain->id);
        }
    }

    /* Also check async block for async writes. */
    if (mod->async_block) {
        collect_mem_writes_from_stmt(mod->async_block, NULL, -1);
    }

    /* Emit $memwr_v2 cells for collected writes. */
    for (int i = 0; i < s_mem_writes_count; ++i) {
        emit_memwr_cell(out, mod, &s_mem_writes[i]);
    }
}
