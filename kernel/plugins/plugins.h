/* Basalt - plugin registry (M1): modules loaded by Limine become plugins. */
#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BASALT_PLUGIN_API 1

typedef int (*bplugin_init_fn)(void);
typedef void (*bplugin_name_fn)(char *out, size_t cap);

struct bplugin {
    const char *name;      /* short name, e.g. "hello" */
    const char *version;   /* "1.0" */
    bplugin_init_fn init;  /* optional, returns 0 on success */
};

/* Scan Limine modules; register each as a plugin. Returns count found. */
uint64_t plugins_init(void);

/* Number of registered plugins. */
uint64_t plugins_count(void);

/* Name of the i-th plugin, or "none". */
const char *plugin_name(uint64_t i);

/* Call init() of every plugin that has one; returns number initialized ok. */
uint64_t plugins_boot(void);

#ifdef __cplusplus
}
#endif
