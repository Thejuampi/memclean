#include "memclean_report.h"
#include "memclean_core.h"

#define NL "\r\n"

void mc_out_text(HANDLE out, const char *text)
{
    DWORD written;
    WriteFile(out, text, (DWORD)lstrlenA(text), &written, nullptr);
}

void mc_out_u64(HANDLE out, uint64_t value)
{
    char buffer[24];
    size_t length = mc_format_u64(value, buffer, sizeof(buffer));
    DWORD written;

    WriteFile(out, buffer, (DWORD)length, &written, nullptr);
}

static void out_hex(HANDLE out, unsigned long value)
{
    char buffer[24];
    size_t length = mc_format_hex(value, buffer, sizeof(buffer));
    DWORD written;

    WriteFile(out, buffer, (DWORD)length, &written, nullptr);
}

static void out_unit(HANDLE out, enum mc_size_unit unit)
{
    switch (unit)
    {
    case MC_UNIT_GB:
        mc_out_text(out, " GB");
        break;
    case MC_UNIT_MB:
        mc_out_text(out, " MB");
        break;
    case MC_UNIT_KB:
        mc_out_text(out, " KB");
        break;
    }
}

void mc_out_size(HANDLE out, uint64_t bytes)
{
    MC_SIZE_PARTS parts = mc_size_parts(bytes);

    mc_out_u64(out, parts.whole);
    if (parts.fractional)
    {
        mc_out_text(out, ".");
        mc_out_u64(out, parts.fraction);
    }
    out_unit(out, parts.unit);
}

void mc_out_nt(HANDLE out, NTSTATUS status)
{
    const char *name = mc_nt_name(status);
    mc_out_text(out, "0x");
    out_hex(out, (unsigned long)status);
    if (name)
    {
        mc_out_text(out, " ");
        mc_out_text(out, name);
    }
}

static void op_line(HANDLE out, const char *name, NTSTATUS status)
{
    mc_out_text(out, "  ");
    mc_out_text(out, name);
    mc_out_text(out, ": ");
    if (status >= 0)
    {
        mc_out_text(out, "ok" NL);
    }
    else
    {
        mc_out_text(out, "failed (");
        mc_out_nt(out, status);
        mc_out_text(out, ")" NL);
    }
}

void mc_report_status(HANDLE out, const MC_STATUS *status)
{
    if (!status->memory_ok)
    {
        mc_out_text(out, "error: GlobalMemoryStatusEx failed" NL);
        return;
    }

    mc_out_text(out, "Memory" NL);
    mc_out_text(out, "  physical total:     ");
    mc_out_size(out, status->total);
    mc_out_text(out, NL);
    mc_out_text(out, "  physical available: ");
    mc_out_size(out, status->available);
    mc_out_text(out, NL);
    mc_out_text(out, "  in use:             ");
    mc_out_size(out, status->used);
    mc_out_text(out, " (");
    mc_out_u64(out, status->load);
    mc_out_text(out, "%)" NL);

    if (status->list_status >= 0)
    {
        mc_out_text(out, "  standby:            ");
        mc_out_size(out, status->standby);
        mc_out_text(out, " (low priority ");
        mc_out_size(out, status->standby_low);
        mc_out_text(out, ")" NL);
        mc_out_text(out, "  modified:           ");
        mc_out_size(out, status->modified);
        mc_out_text(out, NL);
    }
    else
    {
        mc_out_text(out, "  memory lists:       unavailable (");
        mc_out_nt(out, status->list_status);
        mc_out_text(out, ")" NL);
    }

    if (status->cache_status >= 0)
    {
        mc_out_text(out, "  system file cache:  ");
        mc_out_size(out, status->cache);
        mc_out_text(out, NL);
    }
    else
    {
        mc_out_text(out, "  system file cache:  unavailable (");
        mc_out_nt(out, status->cache_status);
        mc_out_text(out, ")" NL);
    }

    mc_out_text(out, "  elevation:          ");
    mc_out_text(out, status->elevated ? "yes" NL : "no (cleaning needs admin)" NL);
}

void mc_report_clean(HANDLE out, const MC_CLEAN_RESULT *result)
{
    mc_out_text(out, "Clean" NL);
    mc_out_text(out, "  elevation: ");
    mc_out_text(out, result->elevated ? "yes" NL : "no (--force)" NL);
    mc_out_text(out, "  profile privilege: ");
    mc_out_text(out, result->profile_enabled ? "enabled" NL : "unavailable" NL);
    mc_out_text(out, "  quota privilege:   ");
    mc_out_text(out, result->quota_enabled ? "enabled" NL : "unavailable" NL);

    if (mc_mask_has(result->mask, MC_CLEAN_WORKING))
        op_line(out, "working set", result->working);
    if (mc_mask_has(result->mask, MC_CLEAN_FILECACHE))
        op_line(out, "file cache", result->filecache);
    if (mc_mask_has(result->mask, MC_CLEAN_MODIFIED))
        op_line(out, "modified list", result->modified);
    if (mc_mask_has(result->mask, MC_CLEAN_STANDBY))
        op_line(out, "standby list", result->standby);
    if (mc_mask_has(result->mask, MC_CLEAN_STANDBY_LOW))
        op_line(out, "low standby list", result->standby_low);
    if (mc_mask_has(result->mask, MC_CLEAN_REGISTRY))
        op_line(out, "registry cache", result->registry);

    if (mc_mask_has(result->mask, MC_CLEAN_COMBINE))
    {
        mc_out_text(out, "  combine lists: ");
        if (result->combine >= 0)
        {
            mc_out_text(out, "ok (");
            mc_out_u64(out, result->combined_pages);
            mc_out_text(out, " pages)" NL);
        }
        else
        {
            mc_out_text(out, "failed (");
            mc_out_nt(out, result->combine);
            mc_out_text(out, ")" NL);
        }
    }

    if (result->have_before && result->have_after)
    {
        mc_out_text(out, "  available: ");
        mc_out_size(out, result->available_before);
        mc_out_text(out, " -> ");
        mc_out_size(out, result->available_after);
        mc_out_text(out, " (");
        if (result->available_after >= result->available_before)
        {
            mc_out_text(out, "+");
            mc_out_size(out, result->available_after - result->available_before);
        }
        else
        {
            mc_out_text(out, "-");
            mc_out_size(out, result->available_before - result->available_after);
        }
        mc_out_text(out, ")" NL);
    }
}
