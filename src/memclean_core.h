#ifndef MEMCLEAN_CORE_H
#define MEMCLEAN_CORE_H

#include <stddef.h>
#include <stdint.h>

enum mc_clean_flag : unsigned
{
    MC_CLEAN_WORKING = 0x01u,
    MC_CLEAN_FILECACHE = 0x02u,
    MC_CLEAN_MODIFIED = 0x04u,
    MC_CLEAN_STANDBY = 0x08u,
    MC_CLEAN_STANDBY_LOW = 0x10u,
    MC_CLEAN_REGISTRY = 0x20u,
    MC_CLEAN_COMBINE = 0x40u,
    MC_CLEAN_SAFE = MC_CLEAN_WORKING | MC_CLEAN_FILECACHE | MC_CLEAN_MODIFIED,
    MC_CLEAN_ALL = MC_CLEAN_SAFE | MC_CLEAN_STANDBY | MC_CLEAN_STANDBY_LOW | MC_CLEAN_REGISTRY | MC_CLEAN_COMBINE
};

typedef struct MC_SETTINGS
{
    uint32_t mask;
    uint32_t auto_enabled;
    uint32_t auto_percent;
    uint32_t auto_cooldown;
    bool startup;
} MC_SETTINGS;

typedef struct MC_FLAG
{
    const char *name;
    unsigned value;
    bool force;
    const char *help;
} MC_FLAG;

typedef struct MC_CLEAN_PLAN
{
    unsigned mask;
    bool force;
    bool ok;
} MC_CLEAN_PLAN;

enum mc_size_unit
{
    MC_UNIT_KB,
    MC_UNIT_MB,
    MC_UNIT_GB
};

typedef struct MC_SIZE_PARTS
{
    unsigned whole;
    unsigned fraction;
    bool fractional;
    enum mc_size_unit unit;
} MC_SIZE_PARTS;

extern const MC_FLAG MC_FLAGS[];

[[nodiscard]] size_t mc_flag_count(void);
[[nodiscard]] bool mc_flag_match(const wchar_t *arg, const char *name);
[[nodiscard]] MC_CLEAN_PLAN mc_clean_plan(int argc, wchar_t **argv);

[[nodiscard]] MC_SETTINGS mc_settings_default(void);
[[nodiscard]] MC_SETTINGS mc_settings_normalize(MC_SETTINGS settings);
[[nodiscard]] uint32_t mc_percent_adjust(uint32_t current, int delta);

[[nodiscard]] unsigned mc_mask_normalize(unsigned mask);
[[nodiscard]] unsigned mc_mask_toggle(unsigned mask, unsigned bit);
[[nodiscard]] unsigned mc_mask_count(unsigned mask);
[[nodiscard]] bool mc_mask_has(unsigned mask, unsigned bit);
[[nodiscard]] unsigned mc_parse_mask(const wchar_t *text);

[[nodiscard]] bool mc_auto_ready(uint32_t enabled, bool cleaning, bool memory_ok, bool elevated, uint32_t load,
                                 uint32_t threshold, uint64_t now, uint64_t last, uint32_t cooldown_seconds);
[[nodiscard]] bool mc_report_failed(const wchar_t *text);

[[nodiscard]] const char *mc_nt_name(long status);
[[nodiscard]] MC_SIZE_PARTS mc_size_parts(uint64_t bytes);
[[nodiscard]] size_t mc_format_u64(uint64_t value, char *buffer, size_t size);
[[nodiscard]] size_t mc_format_hex(unsigned long value, char *buffer, size_t size);

#endif
