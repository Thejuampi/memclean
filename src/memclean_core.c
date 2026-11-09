#include "memclean_core.h"
#include "memclean_common.h"

static constexpr uint32_t MC_PERCENT_MIN = 10;
static constexpr uint32_t MC_PERCENT_MAX = 99;
static constexpr uint32_t MC_PERCENT_DEFAULT = 85;
static constexpr uint32_t MC_COOLDOWN_MIN = 30;
static constexpr uint32_t MC_COOLDOWN_MAX = 86400;
static constexpr uint32_t MC_COOLDOWN_DEFAULT = 300;
static constexpr char HEX[] = "0123456789ABCDEF";
static constexpr wchar_t NEEDLE[] = L"failed (";

const MC_FLAG MC_FLAGS[] = {
    {.name = "--working", .value = MC_CLEAN_WORKING, .help = "empty all working sets (default)"},
    {.name = "--filecache", .value = MC_CLEAN_FILECACHE, .help = "flush the system file cache (default)"},
    {.name = "--modified", .value = MC_CLEAN_MODIFIED, .help = "flush the modified page list (default)"},
    {.name = "--standby", .value = MC_CLEAN_STANDBY, .help = "purge the standby list (disk churn)"},
    {.name = "--standby-low", .value = MC_CLEAN_STANDBY_LOW, .help = "purge the low-priority standby list"},
    {.name = "--registry", .value = MC_CLEAN_REGISTRY, .help = "flush registry hives (Windows 8.1+)"},
    {.name = "--combine", .value = MC_CLEAN_COMBINE, .help = "combine memory lists (Windows 10+)"},
    {.name = "--all", .value = MC_CLEAN_ALL, .help = "every region above"},
    {.name = "--safe", .value = MC_CLEAN_SAFE, .help = "only the default set"},
    {.name = "--force", .force = true, .help = "attempt without administrator rights"},
};

size_t mc_flag_count(void)
{
    return MC_ARRAY_COUNT(MC_FLAGS);
}

bool mc_flag_match(const wchar_t *arg, const char *name)
{
    while (*arg && *name)
    {
        wchar_t left = *arg++;
        char right = *name++;

        if (left >= L'A' && left <= L'Z')
            left += 32;
        if (right >= 'A' && right <= 'Z')
            right += 32;
        if (left != (wchar_t)(unsigned char)right)
            return false;
    }

    return *arg == 0 && *name == 0;
}

MC_CLEAN_PLAN mc_clean_plan(int argc, wchar_t **argv)
{
    MC_CLEAN_PLAN plan = {.mask = 0, .force = false, .ok = true};
    bool chosen = false;
    size_t count = mc_flag_count();

    for (int i = 2; i < argc; i++)
    {
        bool found = false;

        for (size_t j = 0; j < count; j++)
        {
            if (!mc_flag_match(argv[i], MC_FLAGS[j].name))
                continue;
            if (MC_FLAGS[j].force)
            {
                plan.force = true;
            }
            else
            {
                plan.mask |= MC_FLAGS[j].value;
                chosen = true;
            }
            found = true;
            break;
        }

        if (!found)
        {
            plan.ok = false;
            return plan;
        }
    }

    if (!chosen)
        plan.mask = MC_CLEAN_SAFE;
    return plan;
}

MC_SETTINGS mc_settings_default(void)
{
    return (MC_SETTINGS){.mask = MC_CLEAN_SAFE,
                         .auto_enabled = 0,
                         .auto_percent = MC_PERCENT_DEFAULT,
                         .auto_cooldown = MC_COOLDOWN_DEFAULT,
                         .startup = false};
}

MC_SETTINGS mc_settings_normalize(MC_SETTINGS settings)
{
    settings.mask = mc_mask_normalize(settings.mask);
    settings.auto_enabled = settings.auto_enabled ? 1 : 0;
    if (settings.auto_percent < MC_PERCENT_MIN)
        settings.auto_percent = MC_PERCENT_MIN;
    if (settings.auto_percent > MC_PERCENT_MAX)
        settings.auto_percent = MC_PERCENT_MAX;
    if (settings.auto_cooldown < MC_COOLDOWN_MIN)
        settings.auto_cooldown = MC_COOLDOWN_MIN;
    if (settings.auto_cooldown > MC_COOLDOWN_MAX)
        settings.auto_cooldown = MC_COOLDOWN_MAX;
    return settings;
}

uint32_t mc_percent_adjust(uint32_t current, int delta)
{
    int value = (int)current + delta;

    if (value < (int)MC_PERCENT_MIN)
        value = (int)MC_PERCENT_MIN;
    if (value > (int)MC_PERCENT_MAX)
        value = (int)MC_PERCENT_MAX;
    return (uint32_t)value;
}

unsigned mc_mask_normalize(unsigned mask)
{
    mask &= MC_CLEAN_ALL;
    return mask ? mask : MC_CLEAN_SAFE;
}

unsigned mc_mask_toggle(unsigned mask, unsigned bit)
{
    return (mask ^ bit) & MC_CLEAN_ALL;
}

unsigned mc_mask_count(unsigned mask)
{
    unsigned count = 0;

    for (unsigned bit = MC_CLEAN_WORKING; bit <= MC_CLEAN_COMBINE; bit <<= 1)
        if (mask & bit)
            count++;
    return count;
}

bool mc_mask_has(unsigned mask, unsigned bit)
{
    return (mask & bit) != 0;
}

unsigned mc_parse_mask(const wchar_t *text)
{
    unsigned mask = 0;

    for (; *text >= L'0' && *text <= L'9'; text++)
        mask = mask * 10 + (unsigned)(*text - L'0');
    return mask & MC_CLEAN_ALL;
}

bool mc_auto_ready(uint32_t enabled, bool cleaning, bool memory_ok, bool elevated, uint32_t load, uint32_t threshold,
                   uint64_t now, uint64_t last, uint32_t cooldown_seconds)
{
    if (!enabled || cleaning || !memory_ok || !elevated || load < threshold)
        return false;
    if (now < last)
        return false;
    return now - last >= (uint64_t)cooldown_seconds * 1000;
}

bool mc_report_failed(const wchar_t *text)
{
    if (!text)
        return true;

    for (const wchar_t *p = text; *p; p++)
    {
        const wchar_t *a = p;
        const wchar_t *b = NEEDLE;
        while (*a && *b && *a == *b)
        {
            a++;
            b++;
        }
        if (!*b)
            return true;
    }

    return false;
}

const char *mc_nt_name(long status)
{
    switch ((unsigned long)status)
    {
    case 0xC0000061UL:
        return "STATUS_PRIVILEGE_NOT_HELD";
    case 0xC0000022UL:
        return "STATUS_ACCESS_DENIED";
    case 0xC000000DUL:
        return "STATUS_INVALID_PARAMETER";
    case 0xC0000003UL:
        return "STATUS_INVALID_INFO_CLASS";
    case 0xC0000002UL:
        return "STATUS_NOT_IMPLEMENTED";
    case 0xC00000BBUL:
        return "STATUS_NOT_SUPPORTED";
    }
    return nullptr;
}

MC_SIZE_PARTS mc_size_parts(uint64_t bytes)
{
    MC_SIZE_PARTS parts = {.whole = 0, .fraction = 0, .fractional = false, .unit = MC_UNIT_KB};

    if (bytes >= MC_GIB)
    {
        parts.whole = (unsigned)(bytes / MC_GIB);
        parts.fraction = (unsigned)(((bytes % MC_GIB) * 10) / MC_GIB);
        parts.fractional = true;
        parts.unit = MC_UNIT_GB;
    }
    else if (bytes >= MC_MIB)
    {
        parts.whole = (unsigned)(bytes / MC_MIB);
        parts.fraction = (unsigned)(((bytes % MC_MIB) * 10) / MC_MIB);
        parts.fractional = true;
        parts.unit = MC_UNIT_MB;
    }
    else
    {
        parts.whole = (unsigned)((bytes + MC_KIB / 2) / MC_KIB);
    }

    return parts;
}

size_t mc_format_u64(uint64_t value, char *buffer, size_t size)
{
    char digits[24];
    size_t count = 0;
    size_t index = 0;

    if (size == 0)
        return 0;

    if (value == 0)
    {
        if (size > 1)
        {
            buffer[0] = '0';
            buffer[1] = 0;
            return 1;
        }
        buffer[0] = 0;
        return 0;
    }

    while (value != 0)
    {
        digits[count++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (count > 0 && index + 1 < size)
        buffer[index++] = digits[--count];
    buffer[index] = 0;
    return index;
}

size_t mc_format_hex(unsigned long value, char *buffer, size_t size)
{
    char digits[sizeof(unsigned long) * 2];
    size_t count = 0;
    size_t index = 0;

    if (size == 0)
        return 0;

    if (value == 0)
    {
        if (size > 1)
        {
            buffer[0] = '0';
            buffer[1] = 0;
            return 1;
        }
        buffer[0] = 0;
        return 0;
    }

    while (value != 0)
    {
        digits[count++] = HEX[value & 0xF];
        value >>= 4;
    }

    while (count > 0 && index + 1 < size)
        buffer[index++] = digits[--count];
    buffer[index] = 0;
    return index;
}
