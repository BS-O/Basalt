/*
 * Basalt - plugin registry (M1).
 *
 * A Basalt plugin is a flat binary loaded by Limine as a module. Its first
 * byte is 0xBF followed by a little-endian struct bplugin_header:
 *
 *   struct bplugin_header {
 *       uint32_t magic;     // 0xB45A1T57 ('B','s','p','l' hash) -> 'BSPL'
 *       uint32_t api;       // BASALT_PLUGIN_API
 *       bplugin_init_fn init;
 *       const char *name;   // virtual address, module is mapped by Limine
 *       const char *version;
 *   };
 *
 * The struct addresses refer to the module's own mapping, so the plugin
 * must be linked at the address Limine loads it at (or use PIC and store
 * the slide). For M1, tools/bplugin.py emits a minimal header+name blob
 * with absolute addresses that are fixed up at build time.
 */
#include "plugins.h"
#include <limine.h>
#include "string.h"

extern volatile struct limine_module_request module_request;

#define BSPL_MAGIC 0x4253504cu /* 'BSPL' */

struct bplugin_header {
    uint32_t magic;
    uint32_t api;
    uint64_t init;
    uint64_t name;
    uint64_t version;
};

#define MAX_PLUGINS 16

static struct {
    const char *name;
    const char *version;
    uint64_t init;
} reg[MAX_PLUGINS];
static uint64_t reg_count;

uint64_t plugins_init(void)
{
    reg_count = 0;
    if (module_request.response == 0) return 0;
    struct limine_module_response *mods = module_request.response;
    for (uint64_t i = 0; i < mods->module_count && reg_count < MAX_PLUGINS; i++) {
        struct limine_file *f = mods->modules[i];
        if (f->size < sizeof(struct bplugin_header)) continue;
        uint8_t *base = (uint8_t *)(uintptr_t)f->address;
        struct bplugin_header *h = (struct bplugin_header *)base;
        if (h->magic != BSPL_MAGIC) continue;
        if (h->api != BASALT_PLUGIN_API) continue;
        /* name/version are module-relative offsets (tools/bplugin.py) */
        reg[reg_count].name = (const char *)(base + h->name);
        reg[reg_count].version = (const char *)(base + h->version);
        reg[reg_count].init = h->init;
        reg_count++;
    }
    return reg_count;
}

uint64_t plugins_count(void) { return reg_count; }

const char *plugin_name(uint64_t i)
{
    if (i >= reg_count) return "none";
    const char *n = reg[i].name;
    /* paranoia: mapped name should be printable */
    if (n == 0 || n[0] == 0) return "none";
    return n;
}

uint64_t plugins_boot(void)
{
    uint64_t ok = 0;
    for (uint64_t i = 0; i < reg_count; i++) {
        if (reg[i].init == 0) { ok++; continue; }
        int r = ((bplugin_init_fn)(uintptr_t)reg[i].init)();
        if (r == 0) ok++;
    }
    return ok;
}
