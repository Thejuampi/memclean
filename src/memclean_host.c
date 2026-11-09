#include "memclean_host.h"
#include "memclean_report.h"
#include "memclean_common.h"
#include "memclean_strings.h"
#include <shellapi.h>

static constexpr wchar_t MC_RUN_KEY[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static constexpr wchar_t MC_CFG_KEY[] = L"Software\\MemClean";
static constexpr wchar_t MC_RUN_VALUE[] = L"MemClean";
static constexpr wchar_t MC_MUTEX_NAME[] = L"Local\\MemClean";
static constexpr wchar_t MC_TEMP_PREFIX[] = L"mcr";

static constexpr int MC_PATH_CHARS = 1024;
static constexpr int MC_COMMAND_CHARS = 1100;
static constexpr DWORD MC_REPORT_MAX = 65536;

MC_SETTINGS mc_cfg;

static NOTIFYICONDATAW g_tray;
static HANDLE g_mutex;

static const wchar_t *const g_keys[] = {L"Mask", L"AutoEnabled", L"AutoPercent", L"AutoCooldown"};
static uint32_t *const g_values[] = {&mc_cfg.mask, &mc_cfg.auto_enabled, &mc_cfg.auto_percent, &mc_cfg.auto_cooldown};

static DWORD read_dword(HKEY key, const wchar_t *name, DWORD fallback)
{
    DWORD value = fallback;
    DWORD size = sizeof(value);
    DWORD type = 0;

    if (RegQueryValueExW(key, name, 0, &type, (BYTE *)&value, &size) == ERROR_SUCCESS && type == REG_DWORD &&
        size == sizeof(value))
        return value;
    return fallback;
}

static void write_dword(HKEY key, const wchar_t *name, uint32_t value)
{
    DWORD data = value;

    RegSetValueExW(key, name, 0, REG_DWORD, (const BYTE *)&data, (DWORD)sizeof(data));
}

bool mc_startup_get(void)
{
    HKEY key;
    wchar_t buffer[MC_PATH_CHARS];
    DWORD size = sizeof(buffer);
    DWORD type = 0;
    bool result = false;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, MC_RUN_KEY, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return false;
    if (RegQueryValueExW(key, MC_RUN_VALUE, 0, &type, (BYTE *)buffer, &size) == ERROR_SUCCESS && type == REG_SZ)
        result = true;
    RegCloseKey(key);
    return result;
}

bool mc_startup_set(bool enabled)
{
    HKEY key;
    wchar_t exe[MC_PATH_CHARS];
    wchar_t command[MC_COMMAND_CHARS];
    bool result = true;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, MC_RUN_KEY, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS)
        return false;

    if (enabled)
    {
        GetModuleFileNameW(nullptr, exe, MC_PATH_CHARS);
        wsprintfW(command, L"\"%s\" --tray", exe);
        result = RegSetValueExW(key, MC_RUN_VALUE, 0, REG_SZ, (const BYTE *)command,
                                (DWORD)((lstrlenW(command) + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
    }
    else
    {
        result = RegDeleteValueW(key, MC_RUN_VALUE) == ERROR_SUCCESS;
    }

    RegCloseKey(key);
    return result;
}

void mc_cfg_load(void)
{
    HKEY key;

    mc_cfg = mc_settings_default();

    if (RegOpenKeyExW(HKEY_CURRENT_USER, MC_CFG_KEY, 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
    {
        for (size_t i = 0; i < MC_ARRAY_COUNT(g_keys); i++)
            *g_values[i] = read_dword(key, g_keys[i], *g_values[i]);
        RegCloseKey(key);
    }

    mc_cfg.startup = mc_startup_get();
    mc_cfg = mc_settings_normalize(mc_cfg);
}

void mc_cfg_save(void)
{
    HKEY key;

    if (RegCreateKeyExW(HKEY_CURRENT_USER, MC_CFG_KEY, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) !=
        ERROR_SUCCESS)
        return;
    for (size_t i = 0; i < MC_ARRAY_COUNT(g_keys); i++)
        write_dword(key, g_keys[i], *g_values[i]);
    RegCloseKey(key);
}

bool mc_instance_ok(void)
{
    g_mutex = CreateMutexW(nullptr, false, MC_MUTEX_NAME);
    return !(g_mutex && GetLastError() == ERROR_ALREADY_EXISTS);
}

void mc_mutex_release(void)
{
    if (g_mutex)
    {
        CloseHandle(g_mutex);
        g_mutex = nullptr;
    }
}

void mc_mutex_restore(void)
{
    if (!g_mutex)
        g_mutex = CreateMutexW(nullptr, false, MC_MUTEX_NAME);
}

void mc_tray_add(HWND owner, HICON icon)
{
    g_tray = (NOTIFYICONDATAW){0};
    g_tray.cbSize = sizeof(g_tray);
    g_tray.hWnd = owner;
    g_tray.uID = 1;
    g_tray.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_tray.uCallbackMessage = MC_WM_TRAY;
    g_tray.hIcon = icon;
    lstrcpyW(g_tray.szTip, MC_TEXT_APP);
    Shell_NotifyIconW(NIM_ADD, &g_tray);
}

void mc_tray_tip(DWORD load)
{
    wsprintfW(g_tray.szTip, L"%s - %u%% used", MC_TEXT_APP, (unsigned)load);
    g_tray.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &g_tray);
}

void mc_tray_balloon(const wchar_t *title, const wchar_t *text, DWORD flags)
{
    NOTIFYICONDATAW data = g_tray;

    data.uFlags = NIF_INFO;
    data.uTimeout = 5000;
    data.dwInfoFlags = flags;
    lstrcpynW(data.szInfoTitle, title, 64);
    lstrcpynW(data.szInfo, text, 256);
    Shell_NotifyIconW(NIM_MODIFY, &data);
}

void mc_tray_remove(void)
{
    Shell_NotifyIconW(NIM_DELETE, &g_tray);
}

typedef struct MC_CLEAN_CONTEXT
{
    HANDLE process;
    HWND owner;
    wchar_t report[MC_PATH_CHARS];
    bool manual;
} MC_CLEAN_CONTEXT;

static DWORD WINAPI clean_thread(LPVOID parameter)
{
    MC_CLEAN_CONTEXT *context = (MC_CLEAN_CONTEXT *)parameter;
    wchar_t *text = nullptr;
    char *data = nullptr;
    HANDLE file;
    DWORD size;
    DWORD read;

    WaitForSingleObject(context->process, INFINITE);
    CloseHandle(context->process);

    file = CreateFileW(context->report, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE)
    {
        size = GetFileSize(file, nullptr);
        if (size != INVALID_FILE_SIZE && size > 0 && size < MC_REPORT_MAX)
        {
            data = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + 1);
            if (data && ReadFile(file, data, size, &read, nullptr))
                data[read] = 0;
        }
        CloseHandle(file);
    }

    DeleteFileW(context->report);

    if (data)
    {
        int length = MultiByteToWideChar(CP_UTF8, 0, data, -1, nullptr, 0);
        if (length > 0)
        {
            text = (wchar_t *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)length * sizeof(wchar_t));
            if (text)
                MultiByteToWideChar(CP_UTF8, 0, data, -1, text, length);
        }
        HeapFree(GetProcessHeap(), 0, data);
    }

    PostMessageW(context->owner, MC_WM_CLEAN_DONE, (WPARAM)context->manual, (LPARAM)text);
    HeapFree(GetProcessHeap(), 0, context);
    return 0;
}

enum mc_clean_launch mc_clean_start(HWND owner, unsigned mask, bool manual)
{
    wchar_t exe[MC_PATH_CHARS];
    wchar_t temp[MC_PATH_CHARS];
    wchar_t report[MC_PATH_CHARS];
    wchar_t parameters[MC_COMMAND_CHARS];
    SHELLEXECUTEINFOW info = {0};
    MC_CLEAN_CONTEXT *context;
    HANDLE thread;

    if (!GetModuleFileNameW(nullptr, exe, MC_PATH_CHARS) || !GetTempPathW(MC_PATH_CHARS, temp) ||
        !GetTempFileNameW(temp, MC_TEMP_PREFIX, 0, report))
        return MC_CLEAN_TEMP;

    wsprintfW(parameters, L"--worker %u \"%s\"", mask, report);

    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.hwnd = owner;
    info.lpVerb = mc_is_elevated() ? L"open" : L"runas";
    info.lpFile = exe;
    info.lpParameters = parameters;
    info.nShow = SW_HIDE;

    if (!ShellExecuteExW(&info))
    {
        DWORD error = GetLastError();
        DeleteFileW(report);
        return error == ERROR_CANCELLED ? MC_CLEAN_CANCELED : MC_CLEAN_ERROR;
    }

    context = (MC_CLEAN_CONTEXT *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*context));
    if (!context)
    {
        CloseHandle(info.hProcess);
        DeleteFileW(report);
        return MC_CLEAN_ERROR;
    }

    context->process = info.hProcess;
    context->owner = owner;
    context->manual = manual;
    lstrcpynW(context->report, report, MC_PATH_CHARS);

    thread = CreateThread(nullptr, 0, clean_thread, context, 0, nullptr);
    if (!thread)
    {
        CloseHandle(info.hProcess);
        DeleteFileW(report);
        HeapFree(GetProcessHeap(), 0, context);
        return MC_CLEAN_ERROR;
    }

    CloseHandle(thread);
    return MC_CLEAN_OK;
}

bool mc_elevate(HWND owner)
{
    wchar_t exe[MC_PATH_CHARS];
    SHELLEXECUTEINFOW info = {0};

    GetModuleFileNameW(nullptr, exe, MC_PATH_CHARS);
    mc_mutex_release();

    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.hwnd = owner;
    info.lpVerb = L"runas";
    info.lpFile = exe;
    info.nShow = SW_SHOWNORMAL;

    if (ShellExecuteExW(&info))
    {
        if (info.hProcess)
            CloseHandle(info.hProcess);
        return true;
    }

    mc_mutex_restore();
    return false;
}

enum mc_auto_result mc_toggle_auto(HWND owner)
{
    bool target = !mc_cfg.auto_enabled;

    if (target && !mc_is_elevated())
    {
        ShowWindow(owner, SW_SHOW);
        SetForegroundWindow(owner);

        if (MessageBoxW(owner, MC_TEXT_AUTO_PROMPT, MC_TEXT_APP, MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            mc_cfg.auto_enabled = 1;
            mc_cfg_save();
            if (mc_elevate(owner))
                return MC_AUTO_RELAUNCH;
            mc_cfg.auto_enabled = 0;
            mc_cfg_save();
            return MC_AUTO_CANCELED;
        }

        mc_cfg.auto_enabled = 0;
        mc_cfg_save();
        return MC_AUTO_NEEDS_ADMIN;
    }

    mc_cfg.auto_enabled = target ? 1 : 0;
    mc_cfg_save();
    return MC_AUTO_TOGGLED;
}

bool mc_toggle_startup(void)
{
    bool target = !mc_cfg.startup;

    if (!mc_startup_set(target))
        return false;
    mc_cfg.startup = target;
    return true;
}

void mc_adjust_percent(int delta)
{
    uint32_t next = mc_percent_adjust(mc_cfg.auto_percent, delta);

    if (next == mc_cfg.auto_percent)
        return;

    mc_cfg.auto_percent = next;
    mc_cfg_save();
}

void mc_worker_main(int argc, wchar_t **argv)
{
    unsigned mask;
    HANDLE out;
    MC_CLEAN_RESULT result;
    int failures;

    if (argc < 4 || lstrcmpiW(argv[1], L"--worker") != 0)
        return;

    mask = mc_parse_mask(argv[2]);
    if (!mc_init() || mask == 0)
        ExitProcess(4);

    out = CreateFileW(argv[3], GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (out == INVALID_HANDLE_VALUE)
        ExitProcess(4);

    failures = mc_clean(mask, false, &result);

    if (failures < 0)
        mc_out_text(out, "error: administrator rights required\r\n");
    else
        mc_report_clean(out, &result);

    CloseHandle(out);
    ExitProcess(failures < 0 ? 4 : (failures ? 3 : 0));
}
