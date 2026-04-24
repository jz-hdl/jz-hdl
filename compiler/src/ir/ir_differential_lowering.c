#include <stdio.h>
#include <string.h>

#include "chip_data.h"
#include "diagnostic.h"
#include "ir.h"

static int pin_has_diff_mapping(const IR_Project *proj, const IR_Pin *pin)
{
    if (!proj || !pin || pin->mode != PIN_MODE_DIFFERENTIAL) return 0;
    for (int i = 0; i < proj->num_mappings; ++i) {
        const IR_PinMapping *m = &proj->mappings[i];
        if (m->logical_pin_name && pin->name &&
            strcmp(m->logical_pin_name, pin->name) == 0 &&
            m->board_pin_n_id && m->board_pin_n_id[0] != '\0') {
            return 1;
        }
    }
    return 0;
}

static int find_signal_width_for_pin(const IR_Design *design,
                                      const IR_Project *proj,
                                      int pin_idx,
                                      int pin_bit)
{
    if (!design || !proj) return 0;

    const IR_Module *top_mod = NULL;
    if (proj->top_module_id >= 0 && proj->top_module_id < design->num_modules) {
        top_mod = &design->modules[proj->top_module_id];
    }
    if (!top_mod) return 0;

    for (int t = 0; t < proj->num_top_bindings; ++t) {
        const IR_TopBinding *tb = &proj->top_bindings[t];
        if (tb->pin_id == pin_idx && tb->pin_bit_index == pin_bit) {
            for (int s = 0; s < top_mod->num_signals; ++s) {
                if (top_mod->signals[s].id == tb->top_port_signal_id) {
                    return top_mod->signals[s].width;
                }
            }
        }
    }

    for (int t = 0; t < proj->num_top_bindings; ++t) {
        const IR_TopBinding *tb = &proj->top_bindings[t];
        if (tb->pin_id == pin_idx && tb->pin_bit_index == -1) {
            for (int s = 0; s < top_mod->num_signals; ++s) {
                if (top_mod->signals[s].id == tb->top_port_signal_id) {
                    return top_mod->signals[s].width;
                }
            }
        }
    }

    return 0;
}

static int pin_needs_output_serializer(const IR_Pin *pin)
{
    if (!pin || pin->kind != PIN_OUT) return 0;
    return (pin->fclk_name && pin->fclk_name[0] != '\0') ||
           (pin->pclk_name && pin->pclk_name[0] != '\0');
}

int jz_ir_differential_lowering(IR_Design *design,
                                 JZDiagnosticList *diagnostics)
{
    if (!design || !design->project) return 0;

    IR_Project *proj = design->project;
    const char *proj_filename = (design->num_source_files > 0 &&
                                 design->source_files[0].path)
                              ? design->source_files[0].path : NULL;

    JZChipData proj_chip_data;
    int have_proj_chip = 0;
    if (proj->chip_id && proj->chip_id[0]) {
        JZChipLoadStatus st = jz_chip_data_load(proj->chip_id, proj_filename,
                                               &proj_chip_data);
        if (st == JZ_CHIP_LOAD_OK) {
            have_proj_chip = 1;
        }
    }

    int rc = 0;
    int has_any_ser = have_proj_chip &&
        jz_chip_diff_serializer_ratio(&proj_chip_data) > 0;

    for (int i = 0; i < proj->num_pins; ++i) {
        IR_Pin *pin = &proj->pins[i];
        pin->diff_out_uses_serializer = 0;
        pin->diff_out_data_width = 0;
        pin->diff_out_serializer_ratio = 0;
        pin->diff_out_surplus_lanes = 0;

        if (!pin_has_diff_mapping(proj, pin)) continue;
        if (!pin_needs_output_serializer(pin) || !has_any_ser) continue;

        int data_width = pin->ser_width > 0 ? pin->ser_width
                       : find_signal_width_for_pin(design, proj, i, 0);
        if (data_width <= 0) {
            data_width = jz_chip_diff_serializer_ratio(&proj_chip_data);
        }
        if (data_width <= 0) continue;

        int sel_ratio = jz_chip_diff_best_serializer_ratio(&proj_chip_data,
                                                           data_width);
        if (sel_ratio <= 0) {
            const char *name = pin->name ? pin->name : "jz_pin";
            int max_ratio = jz_chip_diff_max_serializer_ratio(&proj_chip_data);
            for (int bit = 0; bit < pin->width; ++bit) {
                char suffix[32];
                if (pin->width > 1) {
                    snprintf(suffix, sizeof(suffix), "%d", bit);
                } else {
                    suffix[0] = '\0';
                }
                if (diagnostics) {
                    JZLocation loc = {0};
                    char msg[256];
                    snprintf(msg, sizeof(msg),
                             "Port width %d exceeds maximum serializer ratio %d for pin %s%s",
                             data_width, max_ratio, name, suffix);
                    jz_diagnostic_report(diagnostics, loc, JZ_SEVERITY_ERROR,
                                         "SERIALIZER_WIDTH_EXCEEDS_RATIO", msg);
                }
            }
            rc = 1;
            continue;
        }

        pin->diff_out_uses_serializer = 1;
        pin->diff_out_data_width = data_width;
        pin->diff_out_serializer_ratio = sel_ratio;
        pin->diff_out_surplus_lanes = sel_ratio - data_width;

        if (diagnostics && sel_ratio != data_width) {
            const char *name = pin->name ? pin->name : "jz_pin";
            for (int bit = 0; bit < pin->width; ++bit) {
                char suffix[32];
                JZLocation loc = {0};
                char msg[256];
                if (pin->width > 1) {
                    snprintf(suffix, sizeof(suffix), "%d", bit);
                } else {
                    suffix[0] = '\0';
                }
                snprintf(msg, sizeof(msg),
                         "Pin %s%s: using %d:1 serializer for %d-bit data",
                         name, suffix, sel_ratio, data_width);
                jz_diagnostic_report(diagnostics, loc, JZ_SEVERITY_NOTE,
                                     "INFO_SERIALIZER_CASCADE", msg);
            }
        }
    }

    if (have_proj_chip) {
        jz_chip_data_free(&proj_chip_data);
    }

    return rc;
}
