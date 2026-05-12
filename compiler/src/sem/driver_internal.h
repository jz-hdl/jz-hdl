/**
 * @file driver_internal.h
 * @brief Shared semantic-driver types and helper declarations.
 */

#ifndef JZ_HDL_DRIVER_INTERNAL_H
#define JZ_HDL_DRIVER_INTERNAL_H

#include "ast.h"
#include "util.h"
#include "diagnostic.h"
#include "rules.h"
#include "chip_data.h"
#include "tristate_types.h"

/** Forward declaration of the bit-vector type used by expression inference. */
struct JZBitvecType;

/** @brief Symbol kinds recorded in module and project semantic tables. */
typedef enum JZSymbolKind {
    JZ_SYM_MODULE,    /**< Module declaration. */
    JZ_SYM_BLACKBOX,  /**< Blackbox declaration. */
    JZ_SYM_CONST,     /**< Module-level constant declaration. */
    JZ_SYM_PORT,      /**< Port declaration. */
    JZ_SYM_WIRE,      /**< Wire declaration. */
    JZ_SYM_REGISTER,  /**< Register declaration. */
    JZ_SYM_LATCH,     /**< Latch declaration. */
    JZ_SYM_MEM,       /**< Memory declaration. */
    JZ_SYM_MUX,       /**< MUX declaration. */
    JZ_SYM_INSTANCE,  /**< Module instance declaration. */
    JZ_SYM_CONFIG,    /**< Project-level CONFIG entry. */
    JZ_SYM_CLOCK,     /**< Project-level CLOCKS entry. */
    JZ_SYM_PIN,       /**< Project-level pin entry. */
    JZ_SYM_MAP_ENTRY, /**< Project-level MAP entry. */
    JZ_SYM_GLOBAL,    /**< GLOBAL block declaration. */
    JZ_SYM_BUS        /**< Project-level BUS declaration. */
} JZSymbolKind;

/** @brief Symbol-table entry for semantic name resolution. */
typedef struct JZSymbol {
    const char   *name;           /**< Pointer into the declaration node name. */
    JZSymbolKind  kind;           /**< Semantic kind of the declaration. */
    JZASTNode    *node;           /**< AST declaration node. */
    int           id;             /**< Stable signal ID, or `-1` when not a signal. */
    int           can_be_z;       /**< Non-zero when a driver may assign `z`. */
    JZASTNode    *feature_guard;  /**< Owning `@feature` guard, or `NULL`. */
    JZASTNode    *feature_branch; /**< Active branch node within the guard. */
} JZSymbol;

/** @brief Per-module symbol table and derived semantic state. */
typedef struct JZModuleScope {
    const char *name;          /**< Module or blackbox name. */
    JZASTNode  *node;          /**< `JZ_AST_MODULE` or `JZ_AST_BLACKBOX` node. */
    JZBuffer    symbols;       /**< Array of `JZSymbol` entries. */
    JZBuffer    bus_signal_decls; /**< Array of synthesized BUS member declarations. */
    int         next_signal_id;/**< Next stable ID for signal-like declarations. */
} JZModuleScope;

/** @brief Alias relationship between two net declarations. */
typedef struct JZAliasEdge {
    JZASTNode *lhs_decl; /**< Base declaration on the alias left-hand side. */
    JZASTNode *rhs_decl; /**< Base declaration on the alias right-hand side. */
    JZASTNode *stmt;     /**< Originating `JZ_AST_STMT_ASSIGN` node. */
} JZAliasEdge;

/** @brief Net-graph node used by flow, alias, and tri-state analysis. */
typedef struct JZNet {
    JZBuffer atoms;        /**< Array of declaration nodes in the net. */
    JZBuffer driver_stmts; /**< Array of assignment statements that drive the net. */
    JZBuffer sink_stmts;   /**< Array of assignment statements that read the net. */
    JZBuffer alias_edges;  /**< Array of alias relationships within the net. */
} JZNet;

/** @brief Maps a declaration node to its containing net index. */
typedef struct JZNetBinding {
    JZASTNode *decl; /**< Declaration node belonging to a net. */
    size_t     net_ix;/**< Index into the owning `JZNet` array. */
} JZNetBinding;

/** @brief Identifies a specific MEM port field use. */
typedef struct JZMemPortRef {
    JZASTNode *mem_decl; /**< `JZ_AST_MEM_DECL` node. */
    JZASTNode *port;     /**< `JZ_AST_MEM_PORT` node. */
    int        field;    /**< `JZMemPortField` selector. */
} JZMemPortRef;

/** @brief Key used to deduplicate MEM write tracking. */
typedef struct JZMemWriteKey {
    JZASTNode *mem_decl; /**< Owning memory declaration. */
    JZASTNode *port;     /**< OUT port node. */
} JZMemWriteKey;

/** @brief Key for synchronous MEM read-assignment tracking. */
typedef struct JZMemSyncReadAssignKey {
    JZASTNode *mem_decl;  /**< Owning memory declaration. */
    JZASTNode *port;      /**< OUT SYNC port node. */
    JZASTNode *dest_decl; /**< Destination declaration node. */
    JZASTNode *addr_expr; /**< Address expression used for the read. */
} JZMemSyncReadAssignKey;

/** @brief MEM port fields that can appear in semantic checks. */
typedef enum JZMemPortField {
    MEM_PORT_FIELD_NONE = 0, /**< No specific field resolved. */
    MEM_PORT_FIELD_ADDR,     /**< Address field. */
    MEM_PORT_FIELD_DATA,     /**< Read-data field. */
    MEM_PORT_FIELD_WDATA     /**< Write-data field for INOUT ports. */
} JZMemPortField;

/** @brief Bit-range covered by an assignment target. */
typedef struct JZAssignRange {
    int      has_range; /**< Non-zero when `lsb` and `msb` are valid. */
    unsigned lsb;       /**< Least-significant assigned bit. */
    unsigned msb;       /**< Most-significant assigned bit. */
} JZAssignRange;

/** @brief Normalized assignment target used by exclusivity checks. */
typedef struct JZAssignTargetEntry {
    JZASTNode    *decl;        /**< Target declaration node. */
    int           is_register; /**< Non-zero when the target is a register. */
    JZAssignRange range;       /**< Assigned bit-range within the target. */
    int           is_nested;   /**< Non-zero when found in nested control flow. */
    int           can_drive_z; /**< Non-zero when the assignment may drive `z`. */
} JZAssignTargetEntry;

/** @brief Decoded BUS access information for expression and flow checks. */
typedef struct JZBusAccessInfo {
    const JZASTNode *port_decl;   /**< Referenced BUS port declaration. */
    const JZASTNode *bus_def;     /**< Referenced project BUS definition. */
    const JZASTNode *signal_decl; /**< Referenced BUS signal declaration. */
    int has_index;                /**< Non-zero when an array index is present. */
    int is_wildcard;              /**< Non-zero when the access uses a wildcard. */
    int index_known;              /**< Non-zero when `index_value` is known. */
    unsigned index_value;         /**< Resolved array index value. */
    unsigned count;               /**< Resolved BUS array element count. */
    int is_array;                 /**< Non-zero when the BUS port is arrayed. */
    int readable;                 /**< Non-zero when the access permits reads. */
    int writable;                 /**< Non-zero when the access permits writes. */
    char bus_id[128];             /**< BUS identifier text. */
    char role[128];               /**< SOURCE or TARGET role text. */
    char signal_name[128];        /**< BUS member signal name. */
} JZBusAccessInfo;

/** @brief Decoded child-instance port access information. */
typedef struct JZInstancePortAccessInfo {
    const JZASTNode *instance_decl;   /**< Referenced module instance declaration. */
    const JZASTNode *binding_decl;    /**< Port binding declaration within the instance. */
    const JZASTNode *child_port_decl; /**< Child-module port declaration. */
    int has_index;                    /**< Non-zero when an array index is present. */
    int index_known;                  /**< Non-zero when `index_value` is known. */
    unsigned index_value;             /**< Resolved array index value. */
    unsigned count;                   /**< Resolved instance array count. */
    int is_array;                     /**< Non-zero when the instance is arrayed. */
    char instance_name[128];          /**< Instance identifier text. */
    char port_name[128];              /**< Child port identifier text. */
} JZInstancePortAccessInfo;

/**
 * @brief Report a semantic diagnostic by rule identifier.
 * @param diagnostics Diagnostic sink to receive the message.
 * @param loc Source location for the diagnostic.
 * @param rule_id Rule identifier to report.
 * @param explanation Optional explanatory text stored with the diagnostic.
 */
void sem_report_rule(JZDiagnosticList *diagnostics,
                     JZLocation loc,
                     const char *rule_id,
                     const char *explanation);

/**
 * @brief Parse a strictly positive decimal integer.
 * @param s Input string to parse.
 * @param out Receives the parsed value on success.
 * @return Non-zero on success, or zero when parsing fails.
 */
int parse_simple_positive_int(const char *s, unsigned *out);
/**
 * @brief Parse a non-negative decimal integer.
 * @param s Input string to parse.
 * @param out Receives the parsed value on success.
 * @return Non-zero on success, or zero when parsing fails.
 */
int parse_simple_nonnegative_int(const char *s, unsigned *out);
/**
 * @brief Parse an unsigned literal lexeme.
 * @param s Literal text to evaluate.
 * @param out Receives the parsed value on success.
 * @return Non-zero on success, or zero when the literal is not supported.
 */
int parse_literal_unsigned_value(const char *s, unsigned *out);
/**
 * @brief Evaluate a simple width or depth expression as a positive integer.
 * @param s Expression text to evaluate.
 * @param out Receives the computed value on success.
 * @return Non-zero on success, or zero when evaluation fails.
 */
int eval_simple_positive_decl_int(const char *s, unsigned *out);
/**
 * @brief Parse a signed decimal integer.
 * @param s Input string to parse.
 * @param out Receives the parsed value on success.
 * @return Non-zero on success, or zero when parsing fails.
 */
int parse_simple_signed_int(const char *s, long long *out);
/**
 * @brief Resolve a BUS access expression into semantic metadata.
 * @param expr BUS access expression to resolve.
 * @param mod_scope Module scope containing the expression.
 * @param project_symbols Project-level symbols used to resolve BUS definitions.
 * @param out Receives the resolved BUS access information.
 * @param diagnostics Diagnostic sink for access errors.
 * @return Non-zero on success, or zero when the access cannot be resolved.
 */
int sem_resolve_bus_access(const JZASTNode *expr,
                           const JZModuleScope *mod_scope,
                           const JZBuffer *project_symbols,
                           JZBusAccessInfo *out,
                           JZDiagnosticList *diagnostics);
/**
 * @brief Resolve an instance-port access expression into semantic metadata.
 * @param expr Instance-port expression to resolve.
 * @param mod_scope Module scope containing the expression.
 * @param project_symbols Project-level symbols used during resolution.
 * @param out Receives the resolved access information.
 * @param diagnostics Diagnostic sink for access errors.
 * @return Non-zero on success, or zero when the access cannot be resolved.
 */
int sem_resolve_instance_port_access(const JZASTNode *expr,
                                     const JZModuleScope *mod_scope,
                                     const JZBuffer *project_symbols,
                                     JZInstancePortAccessInfo *out,
                                     JZDiagnosticList *diagnostics);
/**
 * @brief Get or synthesize a BUS member signal declaration for a module port.
 * @param scope Module scope that owns the BUS port.
 * @param bus_port_name BUS port identifier.
 * @param has_index Non-zero when the BUS port is indexed.
 * @param index Array index value when `has_index` is non-zero.
 * @param signal_name BUS member signal name.
 * @param signal_decl BUS definition node for the member signal.
 * @return The existing or synthesized declaration node, or `NULL` on failure.
 */
JZASTNode *sem_bus_get_or_create_signal_decl(JZModuleScope *scope,
                                             const char *bus_port_name,
                                             int has_index,
                                             unsigned index,
                                             const char *signal_name,
                                             const JZASTNode *signal_decl);

/**
 * @brief Report use of undeclared CONFIG values in a width expression.
 * @param expr Width expression text to inspect.
 * @param loc Source location of the expression.
 * @param project_symbols Project-level CONFIG symbols.
 * @param diagnostics Diagnostic sink for reported issues.
 * @return Non-zero when a diagnostic was emitted, or zero otherwise.
 */
int sem_check_undeclared_config_in_width(const char *expr,
                                         JZLocation loc,
                                         const JZBuffer *project_symbols,
                                         JZDiagnosticList *diagnostics);

/**
 * @brief Check whether an expression text references a GLOBAL symbol.
 * @param expr Expression text to scan.
 * @param project_symbols Project-level symbol table.
 * @return Non-zero when a GLOBAL reference is found, or zero otherwise.
 */
int sem_expr_has_global_ref(const char *expr,
                            const JZBuffer *project_symbols);

/**
 * @brief Infer the bit-vector type of a literal expression.
 * @param node Literal AST node to inspect.
 * @param diagnostics Diagnostic sink for literal-width errors.
 * @param out Receives the inferred type.
 */
void infer_literal_type(JZASTNode *node,
                        JZDiagnosticList *diagnostics,
                        struct JZBitvecType *out);
/**
 * @brief Check whether a literal lexeme is a known constant zero.
 * @param lex Literal text to inspect.
 * @param out_known Receives whether the zero/non-zero result is definite.
 * @return Non-zero when the literal is definitively zero, or zero otherwise.
 */
int sem_literal_is_const_zero(const char *lex, int *out_known);

/*
 * CONST/CONFIG-based integer and width evaluation helpers.
 *
 * These helpers are used by front-end consumers (including IR construction)
 * to obtain fully-resolved integer values for width/depth expressions given
 * the current module scope and project-level CONFIG table.
 */
/**
 * @brief Evaluate a CONST or CONFIG expression in module scope.
 * @param expr Expression text to evaluate.
 * @param scope Module scope used to resolve local constants.
 * @param project_symbols Project-level symbols used for CONFIG lookup.
 * @param out_value Receives the evaluated integer value.
 * @return `0` on success, or non-zero when evaluation fails.
 */
int sem_eval_const_expr_in_module(const char *expr,
                                  const JZModuleScope *scope,
                                  const JZBuffer *project_symbols,
                                  long long *out_value);

/**
 * @brief Resolve a string-valued CONST visible from a module.
 * @param name Constant name to resolve.
 * @param scope Module scope used for lookup.
 * @param project_symbols Project-level symbols used for lookup.
 * @param out_str Receives the resolved string literal.
 * @param diagnostics Diagnostic sink for lookup failures.
 * @param loc Source location used for diagnostics.
 * @return `0` on success, or non-zero when resolution fails.
 */
int sem_resolve_string_const(const char *name,
                             const JZModuleScope *scope,
                             const JZBuffer *project_symbols,
                             const char **out_str,
                             JZDiagnosticList *diagnostics,
                             JZLocation loc);

/**
 * @brief Evaluate a project-scope CONST or CONFIG expression.
 * @param expr Expression text to evaluate.
 * @param project_symbols Project-level symbols used for lookup.
 * @param out_value Receives the evaluated integer value.
 * @return `0` on success, or non-zero when evaluation fails.
 */
int sem_eval_const_expr_in_project(const char *expr,
                                   const JZBuffer *project_symbols,
                                   long long *out_value);

/**
 * @brief Evaluate a width expression in module scope.
 * @param expr Width expression text to evaluate.
 * @param scope Module scope used to resolve local names.
 * @param project_symbols Project-level symbols used for CONFIG lookup.
 * @param out_width Receives the evaluated width.
 * @return `0` on success, or non-zero when evaluation fails.
 */
int sem_eval_width_expr(const char *expr,
                        const JZModuleScope *scope,
                        const JZBuffer *project_symbols,
                        unsigned *out_width);

/**
 * @brief Evaluate a width expression and report failures at a specific location.
 * @param expr Width expression text to evaluate.
 * @param scope Module scope used to resolve local names.
 * @param project_symbols Project-level symbols used for CONFIG lookup.
 * @param out_width Receives the evaluated width.
 * @param loc Source location used for diagnostics.
 * @return `0` on success, or non-zero when evaluation fails.
 */
int sem_eval_width_expr_at_loc(const char *expr,
                               const JZModuleScope *scope,
                               const JZBuffer *project_symbols,
                               unsigned *out_width,
                               JZLocation loc);

/**
 * @brief Expand `widthof(...)` calls within a width expression.
 * @param expr Width expression text to expand.
 * @param scope Module scope used to resolve referenced declarations.
 * @param project_symbols Project-level symbols used during expansion.
 * @param out_expanded Receives the expanded expression string.
 * @param depth Current recursion depth for nested expansion.
 * @param loc Source location associated with the expression.
 * @return `0` on success, or non-zero when expansion fails.
 */
int sem_expand_widthof_in_width_expr(const char *expr,
                                     const JZModuleScope *scope,
                                     const JZBuffer *project_symbols,
                                     char **out_expanded,
                                     int depth,
                                     JZLocation loc);

/**
 * @brief Expand `widthof(...)` calls and report failures through diagnostics.
 * @param expr Width expression text to expand.
 * @param scope Module scope used to resolve referenced declarations.
 * @param project_symbols Project-level symbols used during expansion.
 * @param out_expanded Receives the expanded expression string.
 * @param depth Current recursion depth for nested expansion.
 * @param diagnostics Diagnostic sink for expansion failures.
 * @param loc Source location associated with the expression.
 * @return `0` on success, or non-zero when expansion fails.
 */
int sem_expand_widthof_in_width_expr_diag(const char *expr,
                                          const JZModuleScope *scope,
                                          const JZBuffer *project_symbols,
                                          char **out_expanded,
                                          int depth,
                                          JZDiagnosticList *diagnostics,
                                          JZLocation loc);

/**
 * @brief Check whether an expression text contains a `lit(...)` call.
 * @param expr_text Expression text to scan.
 * @return Non-zero when a `lit(...)` call is present, or zero otherwise.
 */
int sem_expr_has_lit_call(const char *expr_text);

/* Core expression type/width inference helper implemented in driver_operators.c
 * and reused by MEM and flow passes.
 */
void infer_expr_type(JZASTNode *expr,
                     const JZModuleScope *mod_scope,
                     const JZBuffer *project_symbols,
                     JZDiagnosticList *diagnostics,
                     struct JZBitvecType *out);

/**
 * @brief Validate explicit literal widths within a module.
 * @param scope Module scope to scan.
 * @param project_symbols Project-level symbols used for CONFIG lookup.
 * @param diagnostics Diagnostic sink for reported issues.
 */
void sem_check_module_literal_widths(const JZModuleScope *scope,
                                     const JZBuffer *project_symbols,
                                     JZDiagnosticList *diagnostics);

/**
 * @brief Add a symbol to a module scope.
 * @param scope Module scope receiving the symbol.
 * @param kind Symbol kind to add.
 * @param name Identifier text for the symbol.
 * @param decl Declaration node for the symbol.
 * @param diagnostics Diagnostic sink for duplicate-name errors.
 * @return `0` on success, negative on allocation failure, or zero on rejection.
 */
int module_scope_add_symbol(JZModuleScope *scope,
                            JZSymbolKind kind,
                            const char *name,
                            JZASTNode *decl,
                            JZDiagnosticList *diagnostics);

/**
 * @brief Add a symbol to a module scope with feature-guard ownership metadata.
 * @param scope Module scope receiving the symbol.
 * @param kind Symbol kind to add.
 * @param name Identifier text for the symbol.
 * @param decl Declaration node for the symbol.
 * @param feature_guard Owning `@feature` guard, or `NULL`.
 * @param feature_branch Branch node within the feature guard, or `NULL`.
 * @param diagnostics Diagnostic sink for duplicate-name errors.
 * @return `0` on success, negative on allocation failure, or zero on rejection.
 */
int module_scope_add_symbol_featured(JZModuleScope *scope,
                                     JZSymbolKind kind,
                                     const char *name,
                                     JZASTNode *decl,
                                     JZASTNode *feature_guard,
                                     JZASTNode *feature_branch,
                                     JZDiagnosticList *diagnostics);

/**
 * @brief Look up a symbol by name within a module scope.
 * @param scope Module scope to search.
 * @param name Identifier text to resolve.
 * @return The matching symbol, or `NULL` when none exists.
 */
const JZSymbol *module_scope_lookup(const JZModuleScope *scope,
                                    const char *name);

/**
 * @brief Look up the first symbol visible at a specific source location.
 * @param scope Module scope to search.
 * @param name Identifier text to resolve.
 * @param use_loc Source location of the use site.
 * @return The matching visible symbol, or `NULL` when none exists.
 */
const JZSymbol *module_scope_lookup_visible(const JZModuleScope *scope,
                                            const char *name,
                                            JZLocation use_loc);

/**
 * @brief Look up the first symbol of a specific kind visible at a use site.
 * @param scope Module scope to search.
 * @param name Identifier text to resolve.
 * @param kind Required symbol kind.
 * @param use_loc Source location of the use site.
 * @return The matching visible symbol, or `NULL` when none exists.
 */
const JZSymbol *module_scope_lookup_kind_visible(const JZModuleScope *scope,
                                                 const char *name,
                                                 JZSymbolKind kind,
                                                 JZLocation use_loc);

/**
 * @brief Look up a symbol by name and kind within a module scope.
 * @param scope Module scope to search.
 * @param name Identifier text to resolve.
 * @param kind Required symbol kind.
 * @return The matching symbol, or `NULL` when none exists.
 */
const JZSymbol *module_scope_lookup_kind(const JZModuleScope *scope,
                                         const char *name,
                                         JZSymbolKind kind);

/* MEM helper used across modules. */
int sem_match_mem_port_slice(JZASTNode *slice,
                             const JZModuleScope *mod_scope,
                             JZDiagnosticList *diagnostics,
                             JZMemPortRef *out);
int sem_match_mem_port_qualified_ident(JZASTNode *expr,
                                       const JZModuleScope *mod_scope,
                                       JZDiagnosticList *diagnostics,
                                       JZMemPortRef *out);

/** @brief Expression read-rule context flags. */
typedef struct JZExprReadRulesContext {
    int is_sync_context;      /**< Non-zero when checking synchronous context rules. */
    int is_instance_binding;  /**< Non-zero when checking an instance binding. */
    int check_bus_rules;      /**< Non-zero when BUS-specific rules should run. */
} JZExprReadRulesContext;

/* MEM declaration/access/usage helpers implemented in driver_mem.c. */
void sem_check_mem_access_expr(JZASTNode *expr,
                               const JZModuleScope *mod_scope,
                               const JZBuffer *project_symbols,
                               JZDiagnosticList *diagnostics);
void sem_check_expr_read_rules(JZASTNode *expr,
                               const JZModuleScope *mod_scope,
                               const JZBuffer *project_symbols,
                               const JZExprReadRulesContext *ctx,
                               JZDiagnosticList *diagnostics);
void sem_check_mem_addr_assign(const JZMemPortRef *ref,
                               JZASTNode *addr_expr,
                               const JZModuleScope *mod_scope,
                               const JZBuffer *project_symbols,
                               JZDiagnosticList *diagnostics);

void sem_track_mem_out_write(JZBuffer *writes,
                             const JZMemPortRef *ref,
                             JZDiagnosticList *diagnostics,
                             JZLocation loc);

void sem_check_module_mem_and_mux_decls(const JZModuleScope *scope,
                                        const JZBuffer *project_symbols,
                                        JZDiagnosticList *diagnostics);

void sem_check_module_mem_chip_configs(const JZModuleScope *scope,
                                       const JZBuffer *project_symbols,
                                       const JZChipData *chip,
                                       JZDiagnosticList *diagnostics);

void sem_check_module_latch_chip_support(const JZModuleScope *scope,
                                          const JZChipData *chip,
                                          JZASTNode *project,
                                          const JZBuffer *project_symbols,
                                          JZDiagnosticList *diagnostics);

void sem_check_module_mem_port_usage(const JZModuleScope *scope,
                                     JZDiagnosticList *diagnostics);

void sem_check_project_mem_resources(JZASTNode *project,
                                     const JZBuffer *module_scopes,
                                     const JZBuffer *project_symbols,
                                     const JZChipData *chip,
                                     JZDiagnosticList *diagnostics);

/* Project-level symbol table helpers shared between driver_project.c and driver_project_hw.c. */
JZModuleScope *find_module_scope_for_node(JZBuffer *scopes,
                                          JZASTNode *node);

const JZSymbol *project_lookup(const JZBuffer *symbols,
                               const char *name,
                               JZSymbolKind kind);

const JZSymbol *project_lookup_module_or_blackbox(const JZBuffer *symbols,
                                                  const char *name);

JZASTNode *sem_find_project_top_new(JZASTNode *project);

/* Project-level symbol table and semantic helpers implemented in driver_project.c. */
int build_symbol_tables(JZASTNode *project,
                        JZBuffer *module_scopes,
                        JZBuffer *project_symbols,
                        JZDiagnosticList *diagnostics);

void sem_check_project_name_unique(JZASTNode *project,
                                   const JZBuffer *project_symbols,
                                   JZDiagnosticList *diagnostics);

void sem_check_project_config(JZASTNode *project,
                              JZDiagnosticList *diagnostics);

void sem_check_project_clocks(JZASTNode *project,
                              JZBuffer *module_scopes,
                              const JZBuffer *project_symbols,
                              JZDiagnosticList *diagnostics);

void sem_check_project_clock_gen(JZASTNode *project,
                                 const JZBuffer *project_symbols,
                                 const JZChipData *chip,
                                 JZDiagnosticList *diagnostics);

void sem_check_project_pins(JZASTNode *project,
                            const JZBuffer *project_symbols,
                            const JZChipData *chip,
                            JZDiagnosticList *diagnostics);

void sem_check_project_map(JZASTNode *project,
                           const JZBuffer *project_symbols,
                           JZDiagnosticList *diagnostics);

void sem_check_project_buses(JZASTNode *project,
                             const JZBuffer *project_symbols,
                             JZDiagnosticList *diagnostics);

void sem_check_project_blackboxes(JZASTNode *project,
                                  JZDiagnosticList *diagnostics);

void sem_check_globals(JZASTNode *project,
                       const JZBuffer *project_symbols,
                       JZDiagnosticList *diagnostics);

void sem_check_project_top_new(JZASTNode *project,
                               JZBuffer *module_scopes,
                               const JZBuffer *project_symbols,
                               JZDiagnosticList *diagnostics);

void sem_check_unused_modules(JZASTNode *project,
                              JZDiagnosticList *diagnostics);

void resolve_names_recursive(JZASTNode *node,
                             JZBuffer *module_scopes,
                             const JZBuffer *project_symbols,
                             const JZModuleScope *current_scope,
                             JZDiagnosticList *diagnostics);

/* Flow-pass entry points implemented in driver_flow.c.
 *
 * The net-graph construction now accepts project_symbols so that reporting
 * helpers (such as the alias-resolution report) can incorporate
 * project-level context like CLOCKS/IN_PINS/MAP when describing nets.
 */
void sem_build_net_graphs(JZASTNode *root,
                          JZBuffer *module_scopes,
                          const JZBuffer *project_symbols,
                          JZDiagnosticList *diagnostics,
                          int emit_reports);

/* Alias-report emission (implemented in src/report/alias/alias_report.c). */
void sem_emit_alias_report_for_module(const JZModuleScope *scope,
                                      JZBuffer *nets,
                                      const JZBuffer *module_scopes,
                                      const JZBuffer *project_symbols,
                                      JZASTNode *project_root);
void sem_emit_alias_report_finalize(void);

/* Tri-state report emission (implemented in src/report/tristate/tristate_report.c). */
void sem_emit_tristate_report_for_module(const JZModuleScope *scope,
                                         JZBuffer *nets,
                                         const JZBuffer *module_scopes,
                                         const JZBuffer *project_symbols,
                                         JZASTNode *project_root);
void sem_emit_tristate_report_finalize(void);

/* Memory-report emission (implemented in src/report/memory_report.c). */
void sem_emit_memory_report(JZASTNode *root,
                            const JZBuffer *module_scopes,
                            const JZBuffer *project_symbols,
                            const JZChipData *chip,
                            const char *input_filename);

void sem_check_exclusive_assignments(JZASTNode *root,
                                     JZBuffer *module_scopes,
                                     const JZBuffer *project_symbols,
                                     JZDiagnosticList *diagnostics);

/* Exclusive assignment helpers shared with clock-domain checks (driver_clocks.c). */
void sem_excl_collect_targets_from_lhs(JZASTNode *lhs,
                                       const JZModuleScope *scope,
                                       const JZBuffer *project_symbols,
                                       int is_sync,
                                       int is_nested,
                                       JZBuffer *out);

void sem_check_dead_code(JZASTNode *root,
                         JZBuffer *module_scopes,
                         const JZBuffer *project_symbols,
                         JZDiagnosticList *diagnostics);

void sem_check_sync_clock_domains(JZBuffer *module_scopes,
                                  JZDiagnosticList *diagnostics);


/* --- Declarations for functions split across driver_*.c files --- */

/* driver.c helpers shared with new files */
void sem_check_mux_selectors_recursive(JZASTNode *node,
                                       const JZModuleScope *mod_scope,
                                       const JZBuffer *project_symbols,
                                       JZDiagnosticList *diagnostics);

int sem_expr_contains_x_literal_anywhere(const JZASTNode *expr);

void sem_lhs_observable_classify(JZASTNode *lhs,
                                 const JZModuleScope *mod_scope,
                                 int *out_has_register,
                                 int *out_has_out_inout,
                                 int *out_has_mem);

int sem_expr_has_latch_identifier(const JZASTNode *expr,
                                  const JZModuleScope *mod_scope);

int sem_bus_port_has_writable_signal(const JZASTNode *port_decl,
                                    const JZBuffer *project_symbols);

int sem_slice_literal_width(JZASTNode *slice, unsigned *out_width);

/**
 * @brief Evaluate a pure-literal AST expression tree to an integer.
 *
 * Handles EXPR_LITERAL leaves and EXPR_BINARY nodes with arithmetic
 * operators (ADD, SUB, MUL, DIV, MOD).  Used for template-expanded
 * slice bounds where IDX substitution produces expression trees like
 * 0*11+10 rather than simple literal nodes.
 *
 * @param expr  AST expression node.
 * @param out   Output value.
 * @return 1 on success, 0 on failure (unknown node type, div-by-zero, etc.).
 */
int sem_try_const_eval_ast_expr(const JZASTNode *expr, long *out);

int sem_extract_identifier_like(const char *s,
                                char *ident,
                                size_t ident_cap);

int sem_eval_simple_index_literal(JZASTNode *idx, unsigned *out);

int sem_literal_has_x_bits(const char *lex);
int sem_literal_has_z_bits(const char *lex);

/* driver_control.c */
void sem_check_block_control_flow(JZASTNode *block,
                                  const JZModuleScope *mod_scope,
                                  const JZBuffer *project_symbols,
                                  int is_async,
                                  int is_sync,
                                  JZDiagnosticList *diagnostics);

/* driver_assign.c */
void sem_check_block_assignments(JZASTNode *block,
                                 const JZModuleScope *mod_scope,
                                 const JZBuffer *project_symbols,
                                 JZDiagnosticList *diagnostics,
                                 int is_sync,
                                 JZBuffer *mem_out_writes,
                                 JZBuffer *mem_sync_reads,
                                 JZBuffer *mem_inout_addrs,
                                 JZBuffer *mem_inout_wdatas);

/* driver_width.c */
int sem_instance_width_expr_is_invalid(const char *expr,
                                       const JZModuleScope *parent_scope,
                                       const JZBuffer *project_symbols);

int sem_expr_has_undefined_width_ident(const char *expr_text,
                                       const JZModuleScope *scope,
                                       const JZBuffer *project_symbols);

int sem_expr_has_nonpositive_simple_width_literal(const char *expr_text);

void sem_check_module_decl_widths(const JZModuleScope *scope,
                                  const JZBuffer *project_symbols,
                                  JZDiagnosticList *diagnostics);

int sem_check_clog2_expr_simple(const char *expr_text,
                                JZLocation loc,
                                JZDiagnosticList *diagnostics);

/* driver_instance.c */
void sem_check_module_instantiations(const JZModuleScope *scope,
                                     JZBuffer *module_scopes,
                                     const JZBuffer *project_symbols,
                                     JZDiagnosticList *diagnostics);

/* driver_expr.c */
void sem_check_project_checks(JZASTNode *root,
                              const JZBuffer *project_symbols,
                              JZDiagnosticList *diagnostics);

void sem_check_module_checks(const JZModuleScope *scope,
                             const JZBuffer *project_symbols,
                             JZDiagnosticList *diagnostics);

void sem_check_module_expressions(const JZModuleScope *scope,
                                  const JZBuffer *project_symbols,
                                  JZDiagnosticList *diagnostics);

void sem_check_expressions(JZASTNode *root,
                           JZBuffer *module_scopes,
                           const JZBuffer *project_symbols,
                           const JZChipData *chip,
                           JZDiagnosticList *diagnostics);

void sem_check_module_slices(const JZModuleScope *scope,
                             const JZBuffer *project_symbols,
                             JZDiagnosticList *diagnostics);

/* driver.c (was static, now called from driver_expr.c) */
void sem_check_module_const_blocks(const JZModuleScope *scope,
                                   const JZBuffer *project_symbols,
                                   JZDiagnosticList *diagnostics);

int sem_block_reads_name(const JZASTNode *node, const char *name);

/* driver_net.c */
JZNetBinding *sem_net_find_binding(JZBuffer *bindings,
                                   JZASTNode *decl);

/* Build a complete net graph for a single module: declarations, aliases, usage,
 * CDC clock sinks, and instance bindings. On success, *nets and *bindings are
 * populated and owned by the caller (must free via sem_net_free_module_graph).
 * Returns 0 on success, non-zero on failure (buffers are freed on failure).
 */
int sem_net_build_module_graph(const JZModuleScope *scope,
                               const JZBuffer *project_symbols,
                               JZBuffer *nets_out,
                               JZBuffer *bindings_out);

/* Free a net graph populated by sem_net_build_module_graph. */
void sem_net_free_module_graph(JZBuffer *nets, JZBuffer *bindings);

/* driver_comb.c */
void sem_check_combinational_loops_for_module(const JZModuleScope *scope,
                                              JZBuffer *nets,
                                              JZBuffer *bindings,
                                              JZBuffer *module_scopes,
                                              const JZBuffer *project_symbols,
                                              JZBuffer *module_comb_cache,
                                              JZDiagnosticList *diagnostics);

void sem_comb_free_module_comb_cache(JZBuffer *cache);

void sem_comb_collect_targets_from_lhs(JZASTNode *lhs,
                                       const JZModuleScope *scope,
                                       JZBuffer *out_decls);

void sem_comb_collect_sources_from_expr(JZASTNode *expr,
                                        const JZModuleScope *scope,
                                        JZBuffer *out_decls);

/* driver_tristate.c — tri-state proof engine */

/* Set project symbols context for resolving qualified identifiers (e.g., DEV.ROM)
 * in tristate analysis. Must be called before tristate analysis.
 */
void jz_tristate_set_project_symbols(const JZBuffer *project_symbols);

/* Set parent module scope for resolving module CONST values (e.g., DEV_A)
 * in tristate analysis. Must be called before analyzing each module.
 */
void jz_tristate_set_parent_scope(const JZModuleScope *scope);

void jz_tristate_analyze_net(JZTristateNetInfo *info,
                             const JZNet *net,
                             const char *net_name,
                             const char *bus_field,
                             const JZModuleScope *scope,
                             const JZBuffer *module_scopes);

int sem_tristate_check_net(const JZNet *net,
                           const char *net_name,
                           const JZModuleScope *scope,
                           const JZBuffer *module_scopes);

int jz_tristate_net_is_bus_port(const JZNet *net);
const char *jz_tristate_extract_bus_field(const JZASTNode *stmt);
unsigned jz_tristate_decl_width_simple(JZASTNode *decl);

#endif /* JZ_HDL_DRIVER_INTERNAL_H */
