#include <string.h>

#include "ir_internal.h"

static char *clone_string(JZArena *arena, const char *src)
{
    if (!src) return NULL;
    return ir_strdup_arena(arena, src);
}

static int clone_int_array(JZArena *arena,
                           const int *src,
                           int count,
                           int **out)
{
    int *dst;

    if (!out) return -1;
    *out = NULL;
    if (!src || count <= 0) return 0;

    dst = (int *)jz_arena_alloc(arena, (size_t)count * sizeof(int));
    if (!dst) return -1;
    memcpy(dst, src, (size_t)count * sizeof(int));
    *out = dst;
    return 0;
}

static int clone_expr(const IR_Expr *src,
                      JZArena *arena,
                      IR_Expr **out);

static int clone_stmt(const IR_Stmt *src,
                      JZArena *arena,
                      IR_Stmt **out);

static int clone_expr_concat_operands(const IR_Expr *src,
                                      IR_Expr *dst,
                                      JZArena *arena)
{
    int i;

    dst->u.concat.operands = NULL;
    if (src->u.concat.num_operands <= 0) return 0;

    dst->u.concat.operands = (IR_Expr **)jz_arena_alloc(
        arena, (size_t)src->u.concat.num_operands * sizeof(IR_Expr *));
    if (!dst->u.concat.operands) return -1;

    for (i = 0; i < src->u.concat.num_operands; i++) {
        if (clone_expr(src->u.concat.operands[i],
                       arena,
                       &dst->u.concat.operands[i]) != 0) {
            return -1;
        }
    }
    return 0;
}

static int clone_expr(const IR_Expr *src,
                      JZArena *arena,
                      IR_Expr **out)
{
    IR_Expr *dst;

    if (!out) return -1;
    *out = NULL;
    if (!src) return 0;

    dst = (IR_Expr *)jz_arena_alloc(arena, sizeof(IR_Expr));
    if (!dst) return -1;
    memcpy(dst, src, sizeof(IR_Expr));
    dst->const_name = clone_string(arena, src->const_name);
    if (src->const_name && !dst->const_name) return -1;

    switch (src->kind) {
    case EXPR_UNARY_NOT:
    case EXPR_UNARY_NEG:
    case EXPR_LOGICAL_NOT:
        if (clone_expr(src->u.unary.operand, arena, &dst->u.unary.operand) != 0)
            return -1;
        break;

    case EXPR_BINARY_ADD:
    case EXPR_BINARY_SUB:
    case EXPR_BINARY_MUL:
    case EXPR_BINARY_DIV:
    case EXPR_BINARY_MOD:
    case EXPR_BINARY_AND:
    case EXPR_BINARY_OR:
    case EXPR_BINARY_XOR:
    case EXPR_BINARY_SHL:
    case EXPR_BINARY_SHR:
    case EXPR_BINARY_ASHR:
    case EXPR_BINARY_EQ:
    case EXPR_BINARY_NEQ:
    case EXPR_BINARY_LT:
    case EXPR_BINARY_GT:
    case EXPR_BINARY_LTE:
    case EXPR_BINARY_GTE:
    case EXPR_LOGICAL_AND:
    case EXPR_LOGICAL_OR:
    case EXPR_INTRINSIC_USUB:
    case EXPR_INTRINSIC_SSUB:
    case EXPR_INTRINSIC_UMIN:
    case EXPR_INTRINSIC_UMAX:
    case EXPR_INTRINSIC_SMIN:
    case EXPR_INTRINSIC_SMAX:
        if (clone_expr(src->u.binary.left, arena, &dst->u.binary.left) != 0)
            return -1;
        if (clone_expr(src->u.binary.right, arena, &dst->u.binary.right) != 0)
            return -1;
        break;

    case EXPR_TERNARY:
        if (clone_expr(src->u.ternary.condition, arena, &dst->u.ternary.condition) != 0)
            return -1;
        if (clone_expr(src->u.ternary.true_val, arena, &dst->u.ternary.true_val) != 0)
            return -1;
        if (clone_expr(src->u.ternary.false_val, arena, &dst->u.ternary.false_val) != 0)
            return -1;
        break;

    case EXPR_CONCAT:
        if (clone_expr_concat_operands(src, dst, arena) != 0)
            return -1;
        break;

    case EXPR_SLICE:
        if (clone_expr(src->u.slice.base_expr, arena, &dst->u.slice.base_expr) != 0)
            return -1;
        break;

    case EXPR_INTRINSIC_UADD:
    case EXPR_INTRINSIC_SADD:
    case EXPR_INTRINSIC_UMUL:
    case EXPR_INTRINSIC_SMUL:
    case EXPR_INTRINSIC_GBIT:
    case EXPR_INTRINSIC_SBIT:
    case EXPR_INTRINSIC_GSLICE:
    case EXPR_INTRINSIC_SSLICE:
    case EXPR_INTRINSIC_OH2B:
    case EXPR_INTRINSIC_B2OH:
    case EXPR_INTRINSIC_PRIENC:
    case EXPR_INTRINSIC_LZC:
    case EXPR_INTRINSIC_ABS:
    case EXPR_INTRINSIC_POPCOUNT:
    case EXPR_INTRINSIC_REVERSE:
    case EXPR_INTRINSIC_BSWAP:
    case EXPR_INTRINSIC_REDUCE_AND:
    case EXPR_INTRINSIC_REDUCE_OR:
    case EXPR_INTRINSIC_REDUCE_XOR:
        if (clone_expr(src->u.intrinsic.source, arena, &dst->u.intrinsic.source) != 0)
            return -1;
        if (clone_expr(src->u.intrinsic.index, arena, &dst->u.intrinsic.index) != 0)
            return -1;
        if (clone_expr(src->u.intrinsic.value, arena, &dst->u.intrinsic.value) != 0)
            return -1;
        break;

    case EXPR_MEM_READ:
        dst->u.mem_read.memory_name = clone_string(arena, src->u.mem_read.memory_name);
        if (src->u.mem_read.memory_name && !dst->u.mem_read.memory_name)
            return -1;
        dst->u.mem_read.port_name = clone_string(arena, src->u.mem_read.port_name);
        if (src->u.mem_read.port_name && !dst->u.mem_read.port_name)
            return -1;
        if (clone_expr(src->u.mem_read.address, arena, &dst->u.mem_read.address) != 0)
            return -1;
        break;

    case EXPR_LITERAL:
    case EXPR_SIGNAL_REF:
        break;
    }

    *out = dst;
    return 0;
}

static int clone_stmt_block(const IR_Stmt *src,
                            IR_Stmt *dst,
                            JZArena *arena)
{
    int i;

    dst->u.block.stmts = NULL;
    if (src->u.block.count <= 0) return 0;

    dst->u.block.stmts = (IR_Stmt *)jz_arena_alloc(
        arena, (size_t)src->u.block.count * sizeof(IR_Stmt));
    if (!dst->u.block.stmts) return -1;

    for (i = 0; i < src->u.block.count; i++) {
        IR_Stmt *child = NULL;
        if (clone_stmt(&src->u.block.stmts[i], arena, &child) != 0)
            return -1;
        memcpy(&dst->u.block.stmts[i], child, sizeof(IR_Stmt));
    }
    return 0;
}

static int clone_stmt(const IR_Stmt *src,
                      JZArena *arena,
                      IR_Stmt **out)
{
    IR_Stmt *dst;
    int i;

    if (!out) return -1;
    *out = NULL;
    if (!src) return 0;

    dst = (IR_Stmt *)jz_arena_alloc(arena, sizeof(IR_Stmt));
    if (!dst) return -1;
    memcpy(dst, src, sizeof(IR_Stmt));

    switch (src->kind) {
    case STMT_ASSIGNMENT:
        if (clone_expr(src->u.assign.rhs, arena, &dst->u.assign.rhs) != 0)
            return -1;
        break;

    case STMT_IF:
        if (clone_expr(src->u.if_stmt.condition, arena, &dst->u.if_stmt.condition) != 0)
            return -1;
        if (clone_stmt(src->u.if_stmt.then_block, arena, &dst->u.if_stmt.then_block) != 0)
            return -1;
        if (clone_stmt(src->u.if_stmt.elif_chain, arena, &dst->u.if_stmt.elif_chain) != 0)
            return -1;
        if (clone_stmt(src->u.if_stmt.else_block, arena, &dst->u.if_stmt.else_block) != 0)
            return -1;
        break;

    case STMT_SELECT:
        dst->u.select_stmt.selector = NULL;
        dst->u.select_stmt.cases = NULL;
        if (clone_expr(src->u.select_stmt.selector, arena, &dst->u.select_stmt.selector) != 0)
            return -1;
        if (src->u.select_stmt.num_cases > 0) {
            dst->u.select_stmt.cases = (IR_SelectCase *)jz_arena_alloc(
                arena, (size_t)src->u.select_stmt.num_cases * sizeof(IR_SelectCase));
            if (!dst->u.select_stmt.cases) return -1;
            for (i = 0; i < src->u.select_stmt.num_cases; i++) {
                memset(&dst->u.select_stmt.cases[i], 0, sizeof(IR_SelectCase));
                if (clone_expr(src->u.select_stmt.cases[i].case_value,
                               arena,
                               &dst->u.select_stmt.cases[i].case_value) != 0) {
                    return -1;
                }
                if (clone_stmt(src->u.select_stmt.cases[i].body,
                               arena,
                               &dst->u.select_stmt.cases[i].body) != 0) {
                    return -1;
                }
            }
        }
        break;

    case STMT_BLOCK:
        if (clone_stmt_block(src, dst, arena) != 0)
            return -1;
        break;

    case STMT_MEM_WRITE:
        dst->u.mem_write.memory_name = clone_string(arena, src->u.mem_write.memory_name);
        if (src->u.mem_write.memory_name && !dst->u.mem_write.memory_name)
            return -1;
        dst->u.mem_write.port_name = clone_string(arena, src->u.mem_write.port_name);
        if (src->u.mem_write.port_name && !dst->u.mem_write.port_name)
            return -1;
        if (clone_expr(src->u.mem_write.address, arena, &dst->u.mem_write.address) != 0)
            return -1;
        if (clone_expr(src->u.mem_write.data, arena, &dst->u.mem_write.data) != 0)
            return -1;
        break;
    }

    *out = dst;
    return 0;
}

static int clone_signals(const IR_Signal *src,
                         int count,
                         JZArena *arena,
                         IR_Signal **out)
{
    IR_Signal *dst;
    int i;

    if (!out) return -1;
    *out = NULL;
    if (!src || count <= 0) return 0;

    dst = (IR_Signal *)jz_arena_alloc(arena, (size_t)count * sizeof(IR_Signal));
    if (!dst) return -1;
    memcpy(dst, src, (size_t)count * sizeof(IR_Signal));

    for (i = 0; i < count; i++) {
        dst[i].name = clone_string(arena, src[i].name);
        if (src[i].name && !dst[i].name) return -1;
        if (src[i].kind == SIG_REGISTER && src[i].u.reg.reset_value_gnd_vcc) {
            dst[i].u.reg.reset_value_gnd_vcc = clone_string(
                arena, src[i].u.reg.reset_value_gnd_vcc);
            if (!dst[i].u.reg.reset_value_gnd_vcc) return -1;
        }
    }

    *out = dst;
    return 0;
}

static int clone_clock_domains(const IR_ClockDomain *src,
                               int count,
                               JZArena *arena,
                               IR_ClockDomain **out)
{
    IR_ClockDomain *dst;
    int i;

    if (!out) return -1;
    *out = NULL;
    if (!src || count <= 0) return 0;

    dst = (IR_ClockDomain *)jz_arena_alloc(
        arena, (size_t)count * sizeof(IR_ClockDomain));
    if (!dst) return -1;
    memcpy(dst, src, (size_t)count * sizeof(IR_ClockDomain));

    for (i = 0; i < count; i++) {
        if (clone_int_array(arena,
                            src[i].register_ids,
                            src[i].num_registers,
                            &dst[i].register_ids) != 0) {
            return -1;
        }
        if (src[i].num_sensitivity > 0) {
            dst[i].sensitivity_list = (IR_SensitivityEntry *)jz_arena_alloc(
                arena, (size_t)src[i].num_sensitivity * sizeof(IR_SensitivityEntry));
            if (!dst[i].sensitivity_list) return -1;
            memcpy(dst[i].sensitivity_list,
                   src[i].sensitivity_list,
                   (size_t)src[i].num_sensitivity * sizeof(IR_SensitivityEntry));
        } else {
            dst[i].sensitivity_list = NULL;
        }
        if (clone_stmt(src[i].statements, arena, &dst[i].statements) != 0)
            return -1;
    }

    *out = dst;
    return 0;
}

static int clone_instances(const IR_Instance *src,
                           int count,
                           JZArena *arena,
                           IR_Instance **out)
{
    IR_Instance *dst;
    int i;
    int j;

    if (!out) return -1;
    *out = NULL;
    if (!src || count <= 0) return 0;

    dst = (IR_Instance *)jz_arena_alloc(arena, (size_t)count * sizeof(IR_Instance));
    if (!dst) return -1;
    memcpy(dst, src, (size_t)count * sizeof(IR_Instance));

    for (i = 0; i < count; i++) {
        dst[i].name = clone_string(arena, src[i].name);
        if (src[i].name && !dst[i].name) return -1;

        if (src[i].num_connections > 0) {
            dst[i].connections = (IR_InstanceConnection *)jz_arena_alloc(
                arena, (size_t)src[i].num_connections * sizeof(IR_InstanceConnection));
            if (!dst[i].connections) return -1;
            memcpy(dst[i].connections,
                   src[i].connections,
                   (size_t)src[i].num_connections * sizeof(IR_InstanceConnection));
            for (j = 0; j < src[i].num_connections; j++) {
                dst[i].connections[j].const_expr = clone_string(
                    arena, src[i].connections[j].const_expr);
                if (src[i].connections[j].const_expr &&
                    !dst[i].connections[j].const_expr) {
                    return -1;
                }
            }
        } else {
            dst[i].connections = NULL;
        }

        if (src[i].num_params > 0) {
            dst[i].params = (IR_InstanceParam *)jz_arena_alloc(
                arena, (size_t)src[i].num_params * sizeof(IR_InstanceParam));
            if (!dst[i].params) return -1;
            memcpy(dst[i].params,
                   src[i].params,
                   (size_t)src[i].num_params * sizeof(IR_InstanceParam));
            for (j = 0; j < src[i].num_params; j++) {
                dst[i].params[j].name = clone_string(arena, src[i].params[j].name);
                if (src[i].params[j].name && !dst[i].params[j].name) return -1;
                dst[i].params[j].string_value = clone_string(
                    arena, src[i].params[j].string_value);
                if (src[i].params[j].string_value && !dst[i].params[j].string_value)
                    return -1;
            }
        } else {
            dst[i].params = NULL;
        }
    }

    *out = dst;
    return 0;
}

static int clone_memories(const IR_Memory *src,
                          int count,
                          JZArena *arena,
                          IR_Memory **out)
{
    IR_Memory *dst;
    int i;
    int j;

    if (!out) return -1;
    *out = NULL;
    if (!src || count <= 0) return 0;

    dst = (IR_Memory *)jz_arena_alloc(arena, (size_t)count * sizeof(IR_Memory));
    if (!dst) return -1;
    memcpy(dst, src, (size_t)count * sizeof(IR_Memory));

    for (i = 0; i < count; i++) {
        dst[i].name = clone_string(arena, src[i].name);
        if (src[i].name && !dst[i].name) return -1;

        if (src[i].init_is_file) {
            dst[i].init.file_path = clone_string(arena, src[i].init.file_path);
            if (src[i].init.file_path && !dst[i].init.file_path) return -1;
        }

        if (src[i].num_ports > 0) {
            dst[i].ports = (IR_MemoryPort *)jz_arena_alloc(
                arena, (size_t)src[i].num_ports * sizeof(IR_MemoryPort));
            if (!dst[i].ports) return -1;
            memcpy(dst[i].ports,
                   src[i].ports,
                   (size_t)src[i].num_ports * sizeof(IR_MemoryPort));
            for (j = 0; j < src[i].num_ports; j++) {
                dst[i].ports[j].name = clone_string(arena, src[i].ports[j].name);
                if (src[i].ports[j].name && !dst[i].ports[j].name) return -1;
            }
        } else {
            dst[i].ports = NULL;
        }
    }

    *out = dst;
    return 0;
}

static int clone_cdcs(const IR_CDC *src,
                      int count,
                      JZArena *arena,
                      IR_CDC **out)
{
    IR_CDC *dst;
    int i;

    if (!out) return -1;
    *out = NULL;
    if (!src || count <= 0) return 0;

    dst = (IR_CDC *)jz_arena_alloc(arena, (size_t)count * sizeof(IR_CDC));
    if (!dst) return -1;
    memcpy(dst, src, (size_t)count * sizeof(IR_CDC));

    for (i = 0; i < count; i++) {
        dst[i].dest_alias_name = clone_string(arena, src[i].dest_alias_name);
        if (src[i].dest_alias_name && !dst[i].dest_alias_name) return -1;
    }

    *out = dst;
    return 0;
}

static int clone_port_alias_groups(const IR_PortAliasGroup *src,
                                   int count,
                                   JZArena *arena,
                                   IR_PortAliasGroup **out)
{
    IR_PortAliasGroup *dst;
    int i;

    if (!out) return -1;
    *out = NULL;
    if (!src || count <= 0) return 0;

    dst = (IR_PortAliasGroup *)jz_arena_alloc(
        arena, (size_t)count * sizeof(IR_PortAliasGroup));
    if (!dst) return -1;
    memcpy(dst, src, (size_t)count * sizeof(IR_PortAliasGroup));

    for (i = 0; i < count; i++) {
        if (clone_int_array(arena,
                            src[i].port_ids,
                            src[i].count,
                            &dst[i].port_ids) != 0) {
            return -1;
        }
    }

    *out = dst;
    return 0;
}

static int clone_modules(const IR_Module *src,
                         int count,
                         JZArena *arena,
                         IR_Module **out)
{
    IR_Module *dst;
    int i;

    if (!out) return -1;
    *out = NULL;
    if (!src || count <= 0) return 0;

    dst = (IR_Module *)jz_arena_alloc(arena, (size_t)count * sizeof(IR_Module));
    if (!dst) return -1;
    memcpy(dst, src, (size_t)count * sizeof(IR_Module));

    for (i = 0; i < count; i++) {
        dst[i].name = clone_string(arena, src[i].name);
        if (src[i].name && !dst[i].name) return -1;
        if (clone_signals(src[i].signals,
                          src[i].num_signals,
                          arena,
                          &dst[i].signals) != 0) {
            return -1;
        }
        if (clone_clock_domains(src[i].clock_domains,
                                src[i].num_clock_domains,
                                arena,
                                &dst[i].clock_domains) != 0) {
            return -1;
        }
        if (clone_instances(src[i].instances,
                            src[i].num_instances,
                            arena,
                            &dst[i].instances) != 0) {
            return -1;
        }
        if (clone_memories(src[i].memories,
                           src[i].num_memories,
                           arena,
                           &dst[i].memories) != 0) {
            return -1;
        }
        if (clone_cdcs(src[i].cdc_crossings,
                       src[i].num_cdc_crossings,
                       arena,
                       &dst[i].cdc_crossings) != 0) {
            return -1;
        }
        if (clone_stmt(src[i].async_block, arena, &dst[i].async_block) != 0)
            return -1;
        if (clone_port_alias_groups(src[i].port_alias_groups,
                                    src[i].num_port_alias_groups,
                                    arena,
                                    &dst[i].port_alias_groups) != 0) {
            return -1;
        }
    }

    *out = dst;
    return 0;
}

static int clone_clock_gen_units(const IR_ClockGenUnit *src,
                                 int count,
                                 JZArena *arena,
                                 IR_ClockGenUnit **out)
{
    IR_ClockGenUnit *dst;
    int i;
    int j;

    if (!out) return -1;
    *out = NULL;
    if (!src || count <= 0) return 0;

    dst = (IR_ClockGenUnit *)jz_arena_alloc(
        arena, (size_t)count * sizeof(IR_ClockGenUnit));
    if (!dst) return -1;
    memcpy(dst, src, (size_t)count * sizeof(IR_ClockGenUnit));

    for (i = 0; i < count; i++) {
        dst[i].type = clone_string(arena, src[i].type);
        if (src[i].type && !dst[i].type) return -1;

        if (src[i].num_inputs > 0) {
            dst[i].inputs = (IR_ClockGenInput *)jz_arena_alloc(
                arena, (size_t)src[i].num_inputs * sizeof(IR_ClockGenInput));
            if (!dst[i].inputs) return -1;
            memcpy(dst[i].inputs,
                   src[i].inputs,
                   (size_t)src[i].num_inputs * sizeof(IR_ClockGenInput));
            for (j = 0; j < src[i].num_inputs; j++) {
                dst[i].inputs[j].selector = clone_string(arena, src[i].inputs[j].selector);
                if (src[i].inputs[j].selector && !dst[i].inputs[j].selector) return -1;
                dst[i].inputs[j].signal_name = clone_string(arena, src[i].inputs[j].signal_name);
                if (src[i].inputs[j].signal_name && !dst[i].inputs[j].signal_name)
                    return -1;
            }
        } else {
            dst[i].inputs = NULL;
        }

        if (src[i].num_outputs > 0) {
            dst[i].outputs = (IR_ClockGenOutput *)jz_arena_alloc(
                arena, (size_t)src[i].num_outputs * sizeof(IR_ClockGenOutput));
            if (!dst[i].outputs) return -1;
            memcpy(dst[i].outputs,
                   src[i].outputs,
                   (size_t)src[i].num_outputs * sizeof(IR_ClockGenOutput));
            for (j = 0; j < src[i].num_outputs; j++) {
                dst[i].outputs[j].selector = clone_string(arena, src[i].outputs[j].selector);
                if (src[i].outputs[j].selector && !dst[i].outputs[j].selector) return -1;
                dst[i].outputs[j].clock_name = clone_string(arena, src[i].outputs[j].clock_name);
                if (src[i].outputs[j].clock_name && !dst[i].outputs[j].clock_name)
                    return -1;
            }
        } else {
            dst[i].outputs = NULL;
        }

        if (src[i].num_configs > 0) {
            dst[i].configs = (IR_ClockGenConfig *)jz_arena_alloc(
                arena, (size_t)src[i].num_configs * sizeof(IR_ClockGenConfig));
            if (!dst[i].configs) return -1;
            memcpy(dst[i].configs,
                   src[i].configs,
                   (size_t)src[i].num_configs * sizeof(IR_ClockGenConfig));
            for (j = 0; j < src[i].num_configs; j++) {
                dst[i].configs[j].param_name = clone_string(arena, src[i].configs[j].param_name);
                if (src[i].configs[j].param_name && !dst[i].configs[j].param_name)
                    return -1;
                dst[i].configs[j].param_value = clone_string(arena, src[i].configs[j].param_value);
                if (src[i].configs[j].param_value && !dst[i].configs[j].param_value)
                    return -1;
            }
        } else {
            dst[i].configs = NULL;
        }
    }

    *out = dst;
    return 0;
}

static int clone_project(const IR_Project *src,
                         JZArena *arena,
                         IR_Project **out)
{
    IR_Project *dst;
    int i;

    if (!out) return -1;
    *out = NULL;
    if (!src) return 0;

    dst = (IR_Project *)jz_arena_alloc(arena, sizeof(IR_Project));
    if (!dst) return -1;
    memcpy(dst, src, sizeof(IR_Project));

    dst->name = clone_string(arena, src->name);
    if (src->name && !dst->name) return -1;
    dst->chip_id = clone_string(arena, src->chip_id);
    if (src->chip_id && !dst->chip_id) return -1;

    if (src->num_clocks > 0) {
        dst->clocks = (IR_Clock *)jz_arena_alloc(arena, (size_t)src->num_clocks * sizeof(IR_Clock));
        if (!dst->clocks) return -1;
        memcpy(dst->clocks, src->clocks, (size_t)src->num_clocks * sizeof(IR_Clock));
        for (i = 0; i < src->num_clocks; i++) {
            dst->clocks[i].name = clone_string(arena, src->clocks[i].name);
            if (src->clocks[i].name && !dst->clocks[i].name) return -1;
        }
    } else {
        dst->clocks = NULL;
    }

    if (src->num_clock_gens > 0) {
        dst->clock_gens = (IR_ClockGen *)jz_arena_alloc(
            arena, (size_t)src->num_clock_gens * sizeof(IR_ClockGen));
        if (!dst->clock_gens) return -1;
        memcpy(dst->clock_gens,
               src->clock_gens,
               (size_t)src->num_clock_gens * sizeof(IR_ClockGen));
        for (i = 0; i < src->num_clock_gens; i++) {
            if (clone_clock_gen_units(src->clock_gens[i].units,
                                      src->clock_gens[i].num_units,
                                      arena,
                                      &dst->clock_gens[i].units) != 0) {
                return -1;
            }
        }
    } else {
        dst->clock_gens = NULL;
    }

    if (src->num_pins > 0) {
        dst->pins = (IR_Pin *)jz_arena_alloc(arena, (size_t)src->num_pins * sizeof(IR_Pin));
        if (!dst->pins) return -1;
        memcpy(dst->pins, src->pins, (size_t)src->num_pins * sizeof(IR_Pin));
        for (i = 0; i < src->num_pins; i++) {
            dst->pins[i].name = clone_string(arena, src->pins[i].name);
            if (src->pins[i].name && !dst->pins[i].name) return -1;
            dst->pins[i].standard = clone_string(arena, src->pins[i].standard);
            if (src->pins[i].standard && !dst->pins[i].standard) return -1;
            dst->pins[i].fclk_name = clone_string(arena, src->pins[i].fclk_name);
            if (src->pins[i].fclk_name && !dst->pins[i].fclk_name) return -1;
            dst->pins[i].pclk_name = clone_string(arena, src->pins[i].pclk_name);
            if (src->pins[i].pclk_name && !dst->pins[i].pclk_name) return -1;
            dst->pins[i].reset_name = clone_string(arena, src->pins[i].reset_name);
            if (src->pins[i].reset_name && !dst->pins[i].reset_name) return -1;
        }
    } else {
        dst->pins = NULL;
    }

    if (src->num_mappings > 0) {
        dst->mappings = (IR_PinMapping *)jz_arena_alloc(
            arena, (size_t)src->num_mappings * sizeof(IR_PinMapping));
        if (!dst->mappings) return -1;
        memcpy(dst->mappings,
               src->mappings,
               (size_t)src->num_mappings * sizeof(IR_PinMapping));
        for (i = 0; i < src->num_mappings; i++) {
            dst->mappings[i].logical_pin_name = clone_string(
                arena, src->mappings[i].logical_pin_name);
            if (src->mappings[i].logical_pin_name && !dst->mappings[i].logical_pin_name)
                return -1;
            dst->mappings[i].board_pin_id = clone_string(arena, src->mappings[i].board_pin_id);
            if (src->mappings[i].board_pin_id && !dst->mappings[i].board_pin_id)
                return -1;
            dst->mappings[i].board_pin_n_id = clone_string(
                arena, src->mappings[i].board_pin_n_id);
            if (src->mappings[i].board_pin_n_id && !dst->mappings[i].board_pin_n_id)
                return -1;
        }
    } else {
        dst->mappings = NULL;
    }

    if (src->num_top_bindings > 0) {
        dst->top_bindings = (IR_TopBinding *)jz_arena_alloc(
            arena, (size_t)src->num_top_bindings * sizeof(IR_TopBinding));
        if (!dst->top_bindings) return -1;
        memcpy(dst->top_bindings,
               src->top_bindings,
               (size_t)src->num_top_bindings * sizeof(IR_TopBinding));
        for (i = 0; i < src->num_top_bindings; i++) {
            dst->top_bindings[i].clock_name = clone_string(arena, src->top_bindings[i].clock_name);
            if (src->top_bindings[i].clock_name && !dst->top_bindings[i].clock_name)
                return -1;
        }
    } else {
        dst->top_bindings = NULL;
    }

    *out = dst;
    return 0;
}

static int clone_source_files(const IR_SourceFile *src,
                              int count,
                              JZArena *arena,
                              IR_SourceFile **out)
{
    IR_SourceFile *dst;
    int i;

    if (!out) return -1;
    *out = NULL;
    if (!src || count <= 0) return 0;

    dst = (IR_SourceFile *)jz_arena_alloc(
        arena, (size_t)count * sizeof(IR_SourceFile));
    if (!dst) return -1;
    memcpy(dst, src, (size_t)count * sizeof(IR_SourceFile));

    for (i = 0; i < count; i++) {
        dst[i].path = clone_string(arena, src[i].path);
        if (src[i].path && !dst[i].path) return -1;
    }

    *out = dst;
    return 0;
}

int ir_clone_design(const IR_Design *src,
                    JZArena *arena,
                    IR_Design **out_clone)
{
    IR_Design *dst;

    if (!src || !arena || !out_clone) return -1;
    *out_clone = NULL;

    dst = (IR_Design *)jz_arena_alloc(arena, sizeof(IR_Design));
    if (!dst) return -1;
    memcpy(dst, src, sizeof(IR_Design));

    dst->name = clone_string(arena, src->name);
    if (src->name && !dst->name) return -1;

    if (clone_modules(src->modules, src->num_modules, arena, &dst->modules) != 0)
        return -1;
    if (clone_project(src->project, arena, &dst->project) != 0)
        return -1;
    if (clone_source_files(src->source_files,
                           src->num_source_files,
                           arena,
                           &dst->source_files) != 0) {
        return -1;
    }

    *out_clone = dst;
    return 0;
}
