/**
 * @file sim_vcd.c
 * @brief Emits Value Change Dump waveform traces.
 *
 * Implements the IEEE 1364 VCD format for waveform output.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sim_vcd.h"

#define VCD_MAX_SIGNALS 4096

/**
 * @struct VCDSignal
 * @brief Metadata for one VCD-tracked signal.
 */
typedef struct VCDSignal {
    char *scope;    /**< Scope name emitted in the VCD hierarchy. */
    char *name;     /**< Leaf signal name. */
    int   width;    /**< Declared signal width in bits. */
    char  ident[8]; /**< Encoded VCD identifier string. */
} VCDSignal;

/**
 * @struct VCDWriter
 * @brief Mutable VCD output state.
 */
struct VCDWriter {
    FILE      *fp;           /**< Destination stream. */
    uint64_t   timescale_ps; /**< Timescale in picoseconds per VCD unit. */
    uint64_t   current_time; /**< Last emitted timestamp in VCD units. */
    int        time_written; /**< Non-zero after the first timestamp is emitted. */
    VCDSignal  signals[VCD_MAX_SIGNALS]; /**< Registered signal metadata. */
    int        num_signals;  /**< Number of active entries in @ref signals. */
    int        defs_ended;   /**< Non-zero after `$enddefinitions`. */
};

/**
 * @brief Generate a printable VCD identifier string for a signal index.
 * @param idx Zero-based signal index.
 * @param out Destination character buffer.
 * @param out_sz Size of @p out in bytes.
 */
static void make_vcd_ident(int idx, char *out, size_t out_sz);

static void make_vcd_ident(int idx, char *out, size_t out_sz) {
    if (idx < 94 && out_sz >= 2) {
        out[0] = (char)('!' + idx);
        out[1] = '\0';
    } else if (out_sz >= 3) {
        out[0] = (char)('!' + (idx / 94));
        out[1] = (char)('!' + (idx % 94));
        out[2] = '\0';
    } else {
        out[0] = '!';
        out[1] = '\0';
    }
}

VCDWriter *vcd_open(const char *filename, uint64_t timescale_ps)
{
    FILE *fp = fopen(filename, "w");
    if (!fp) return NULL;

    VCDWriter *w = (VCDWriter *)calloc(1, sizeof(VCDWriter));
    if (!w) {
        fclose(fp);
        return NULL;
    }

    w->fp = fp;
    w->timescale_ps = timescale_ps;
    w->current_time = 0;
    w->time_written = 0;
    w->num_signals = 0;
    w->defs_ended = 0;

    /* Write VCD header */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char date_buf[64];
    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(fp, "$date\n  %s\n$end\n", date_buf);
    fprintf(fp, "$version\n  JZ-HDL Simulator 1.0\n$end\n");

    /* Timescale */
    if (timescale_ps >= 1000000000ULL) {
        fprintf(fp, "$timescale %llums $end\n", (unsigned long long)(timescale_ps / 1000000000ULL));
    } else if (timescale_ps >= 1000000ULL) {
        fprintf(fp, "$timescale %lluus $end\n", (unsigned long long)(timescale_ps / 1000000ULL));
    } else if (timescale_ps >= 1000ULL) {
        fprintf(fp, "$timescale %lluns $end\n", (unsigned long long)(timescale_ps / 1000ULL));
    } else {
        fprintf(fp, "$timescale %llups $end\n", (unsigned long long)timescale_ps);
    }

    return w;
}

int vcd_add_signal(VCDWriter *w, const char *scope, const char *name, int width)
{
    if (!w || w->defs_ended || w->num_signals >= VCD_MAX_SIGNALS) return -1;

    int idx = w->num_signals;
    VCDSignal *sig = &w->signals[idx];
    sig->scope = scope ? strdup(scope) : strdup("top");
    sig->name = strdup(name);
    sig->width = width;
    make_vcd_ident(idx, sig->ident, sizeof(sig->ident));

    w->num_signals++;
    return idx;
}

void vcd_end_definitions(VCDWriter *w)
{
    if (!w || w->defs_ended) return;

    /* Group signals by scope */
    const char *current_scope = NULL;
    for (int i = 0; i < w->num_signals; i++) {
        VCDSignal *sig = &w->signals[i];
        if (!current_scope || strcmp(current_scope, sig->scope) != 0) {
            if (current_scope) {
                fprintf(w->fp, "$upscope $end\n");
            }
            fprintf(w->fp, "$scope module %s $end\n", sig->scope);
            current_scope = sig->scope;
        }
        fprintf(w->fp, "$var wire %d %s %s $end\n",
                sig->width, sig->ident, sig->name);
    }
    if (current_scope) {
        fprintf(w->fp, "$upscope $end\n");
    }

    fprintf(w->fp, "$enddefinitions $end\n");
    w->defs_ended = 1;
}

void vcd_set_time(VCDWriter *w, uint64_t time_ps)
{
    if (!w || !w->defs_ended) return;

    /* Convert from ps to timescale units */
    uint64_t time_units = time_ps / w->timescale_ps;

    if (!w->time_written || time_units != w->current_time) {
        fprintf(w->fp, "#%llu\n", (unsigned long long)time_units);
        w->current_time = time_units;
        w->time_written = 1;
    }
}

void vcd_dump_value(VCDWriter *w, int sig_id, SimValue value)
{
    if (!w || !w->defs_ended || sig_id < 0 || sig_id >= w->num_signals) return;

    VCDSignal *sig = &w->signals[sig_id];
    char bin[SIM_VAL_WORDS * 64 + 1];

    if (sig->width == 1) {
        char ch = '0';
        if (value.xmask[0] & 1U) ch = 'x';
        else if (value.zmask[0] & 1U) ch = 'z';
        else if (value.val[0] & 1U) ch = '1';
        fprintf(w->fp, "%c%s\n", ch, sig->ident);
    } else {
        value.width = sig->width;
        sim_val_to_bin(value, bin, sizeof(bin));
        fprintf(w->fp, "b%s %s\n", bin, sig->ident);
    }
}

void vcd_close(VCDWriter *w)
{
    if (!w) return;

    if (w->fp) {
        fclose(w->fp);
    }

    for (int i = 0; i < w->num_signals; i++) {
        free(w->signals[i].scope);
        free(w->signals[i].name);
    }

    free(w);
}
