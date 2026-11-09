#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include "memclean_core.h"
#include "memclean_engine.h"
#include "memclean_report.h"

static constexpr int FLAG_COLUMN = 16;

static HANDLE g_out;

static bool same(const wchar_t *left, const wchar_t *right)
{
    return lstrcmpiW(left, right) == 0;
}

static void usage(void)
{
    size_t count = mc_flag_count();

    mc_out_text(g_out, "MemClean CLI - native Windows memory cleaner\r\n"
                       "\r\n"
                       "Usage:\r\n"
                       "  memclean-cli status\r\n"
                       "  memclean-cli clean [flags]\r\n"
                       "\r\n"
                       "Clean flags:\r\n");

    for (size_t i = 0; i < count; i++)
    {
        int length = lstrlenA(MC_FLAGS[i].name);

        mc_out_text(g_out, "  ");
        mc_out_text(g_out, MC_FLAGS[i].name);
        do
        {
            mc_out_text(g_out, " ");
            length++;
        } while (length < FLAG_COLUMN);
        mc_out_text(g_out, MC_FLAGS[i].help);
        mc_out_text(g_out, "\r\n");
    }

    mc_out_text(g_out, "\r\nNotes:\r\n"
                       "  Cleaning requires an elevated process. Status does not.\r\n"
                       "  MinGW build: no Visual C++ runtime and no C runtime DLLs.\r\n");
}

static int cmd_status(void)
{
    MC_STATUS status;

    mc_get_status(&status);
    mc_report_status(g_out, &status);
    return status.memory_ok ? 0 : 3;
}

static int cmd_clean(unsigned mask, bool force)
{
    MC_CLEAN_RESULT result;
    int failures;

    failures = mc_clean(mask, force, &result);

    if (failures < 0)
    {
        mc_out_text(g_out, "error: administrator rights required for cleaning.\r\n");
        mc_out_text(g_out, "hint: run this command from an elevated terminal.\r\n");
        return 2;
    }

    mc_report_clean(g_out, &result);
    return failures ? 3 : 0;
}

static int cmd_clean_args(int argc, wchar_t **argv)
{
    MC_CLEAN_PLAN plan = mc_clean_plan(argc, argv);

    if (!plan.ok)
    {
        mc_out_text(g_out, "error: unknown flag\r\n\r\n");
        usage();
        return 1;
    }

    return cmd_clean(plan.mask, plan.force);
}

[[noreturn]] void entry(void)
{
    wchar_t **argv;
    int argc = 0;
    int code = 1;

    g_out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (g_out == nullptr || g_out == INVALID_HANDLE_VALUE)
        g_out = GetStdHandle(STD_ERROR_HANDLE);

    if (!mc_init())
    {
        mc_out_text(g_out, "error: ntdll exports not found\r\n");
        ExitProcess(1);
    }

    argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argv == nullptr || argc < 2)
    {
        usage();
    }
    else if (same(argv[1], L"status"))
    {
        code = cmd_status();
    }
    else if (same(argv[1], L"clean"))
    {
        code = cmd_clean_args(argc, argv);
    }
    else
    {
        mc_out_text(g_out, "error: unknown command\r\n\r\n");
        usage();
    }

    if (argv)
        LocalFree(argv);
    ExitProcess((UINT)code);
}
