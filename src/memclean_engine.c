#include "memclean_engine.h"

typedef NTSTATUS(__stdcall *MC_NT_SET)(ULONG, PVOID, ULONG);
typedef NTSTATUS(__stdcall *MC_NT_QUERY)(ULONG, PVOID, ULONG, PULONG);

static MC_NT_SET g_nt_set;
static MC_NT_QUERY g_nt_query;

static bool enable_privilege(const wchar_t *name)
{
    HANDLE token;
    TOKEN_PRIVILEGES privileges;
    LUID luid;
    bool result = false;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
        return false;
    if (LookupPrivilegeValueW(nullptr, name, &luid))
    {
        privileges.PrivilegeCount = 1;
        privileges.Privileges[0].Luid = luid;
        privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        SetLastError(ERROR_SUCCESS);
        if (AdjustTokenPrivileges(token, false, &privileges, 0, nullptr, nullptr) && GetLastError() == ERROR_SUCCESS)
            result = true;
    }
    CloseHandle(token);
    return result;
}

bool mc_init(void)
{
    HMODULE module;

    if (g_nt_set && g_nt_query)
        return true;

    module = GetModuleHandleW(L"ntdll.dll");
    if (!module)
        return false;

    g_nt_set = (MC_NT_SET)(void *)GetProcAddress(module, "NtSetSystemInformation");
    g_nt_query = (MC_NT_QUERY)(void *)GetProcAddress(module, "NtQuerySystemInformation");

    return g_nt_set != nullptr && g_nt_query != nullptr;
}

bool mc_is_elevated(void)
{
    HANDLE token;
    TOKEN_ELEVATION info;
    DWORD length;
    bool result = false;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return false;
    if (GetTokenInformation(token, TokenElevation, &info, sizeof(info), &length))
        result = info.TokenIsElevated != 0;
    CloseHandle(token);
    return result;
}

void mc_get_status(MC_STATUS *status)
{
    MEMORYSTATUSEX memory;
    MC_MEMORY_LIST_INFO list;
    MC_FILECACHE_INFO cache;
    SYSTEM_INFO system;
    uint64_t page_size;

    *status = (MC_STATUS){0};

    memory.dwLength = sizeof(memory);
    if (GlobalMemoryStatusEx(&memory))
    {
        status->memory_ok = true;
        status->total = memory.ullTotalPhys;
        status->available = memory.ullAvailPhys;
        status->used = memory.ullTotalPhys - memory.ullAvailPhys;
        status->load = memory.dwMemoryLoad;
    }

    list = (MC_MEMORY_LIST_INFO){0};
    status->list_status = g_nt_query ? g_nt_query(MC_CLASS_MEMORY_LIST, &list, (ULONG)sizeof(list), nullptr) : -1;

    cache = (MC_FILECACHE_INFO){0};
    status->cache_status = g_nt_query ? g_nt_query(MC_CLASS_FILECACHE_EX, &cache, (ULONG)sizeof(cache), nullptr) : -1;

    GetSystemInfo(&system);
    page_size = system.dwPageSize;

    if (status->list_status >= 0)
    {
        for (int i = 0; i < 8; i++)
            status->standby += list.PageCountByPriority[i];
        status->standby *= page_size;
        status->standby_low = list.PageCountByPriority[0] * page_size;
        status->modified = list.ModifiedPageCount * page_size;
    }

    if (status->cache_status >= 0)
        status->cache = cache.CurrentSize;

    status->elevated = mc_is_elevated();
}

static NTSTATUS set_list(ULONG command)
{
    return g_nt_set(MC_CLASS_MEMORY_LIST, &command, (ULONG)sizeof(command));
}

int mc_clean(unsigned mask, bool force, MC_CLEAN_RESULT *result)
{
    MEMORYSTATUSEX before;
    MEMORYSTATUSEX after;
    MC_FILECACHE_INFO cache = {0};
    MC_COMBINE_INFO combine = {0};
    int failures = 0;

    *result = (MC_CLEAN_RESULT){0};
    result->mask = mask;
    result->elevated = mc_is_elevated();

    if (!result->elevated && !force)
        return -1;

    result->profile_enabled = enable_privilege(L"SeProfileSingleProcessPrivilege");
    result->quota_enabled = enable_privilege(L"SeIncreaseQuotaPrivilege");

    before.dwLength = sizeof(before);
    result->have_before = GlobalMemoryStatusEx(&before) != 0;
    if (result->have_before)
        result->available_before = before.ullAvailPhys;

    if (mc_mask_has(mask, MC_CLEAN_WORKING))
    {
        result->working = set_list(MC_CMD_EMPTY_WORKINGSETS);
        if (result->working < 0)
            failures++;
    }

    if (mc_mask_has(mask, MC_CLEAN_FILECACHE))
    {
        cache.MinimumWorkingSet = MAXSIZE_T;
        cache.MaximumWorkingSet = MAXSIZE_T;
        result->filecache = g_nt_set(MC_CLASS_FILECACHE_EX, &cache, (ULONG)sizeof(cache));
        if (result->filecache < 0)
            failures++;
    }

    if (mc_mask_has(mask, MC_CLEAN_MODIFIED))
    {
        result->modified = set_list(MC_CMD_FLUSH_MODIFIED);
        if (result->modified < 0)
            failures++;
    }

    if (mc_mask_has(mask, MC_CLEAN_STANDBY))
    {
        result->standby = set_list(MC_CMD_PURGE_STANDBY);
        if (result->standby < 0)
            failures++;
    }

    if (mc_mask_has(mask, MC_CLEAN_STANDBY_LOW))
    {
        result->standby_low = set_list(MC_CMD_PURGE_LOW_STANDBY);
        if (result->standby_low < 0)
            failures++;
    }

    if (mc_mask_has(mask, MC_CLEAN_REGISTRY))
    {
        result->registry = g_nt_set(MC_CLASS_REGISTRY, nullptr, 0);
        if (result->registry < 0)
            failures++;
    }

    if (mc_mask_has(mask, MC_CLEAN_COMBINE))
    {
        result->combine = g_nt_set(MC_CLASS_COMBINE, &combine, (ULONG)sizeof(combine));
        result->combined_pages = combine.PagesCombined;
        if (result->combine < 0)
            failures++;
    }

    after.dwLength = sizeof(after);
    result->have_after = GlobalMemoryStatusEx(&after) != 0;
    if (result->have_after)
        result->available_after = after.ullAvailPhys;

    return failures;
}
