#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include "memclean_core.h"
#include "memclean_host.h"
#include "memclean_view.h"
#include "memclean_ids.h"
#include "memclean_strings.h"

static constexpr COLORREF MC_BACKGROUND = RGB(6, 16, 15);
static constexpr int MC_WINDOW_WIDTH = 520;
static constexpr int MC_WINDOW_HEIGHT = 602;
static constexpr int MC_TOPBAR_HEIGHT = 52;
static constexpr UINT_PTR MC_TIMER_ID = 1;
static constexpr UINT MC_TIMER_MS = 1000;
static constexpr wchar_t MC_WORKER_FLAG[] = L"--worker";
static constexpr wchar_t MC_TRAY_FLAG[] = L"--tray";

static HINSTANCE g_instance;
static HWND g_window;
static HICON g_icon;
static HICON g_icon_small;
static HBRUSH g_brush;
static MC_STATUS g_status;
static bool g_hidden;
static bool g_cleaning;
static bool g_balloon;
static DWORD g_tip = ~0u;
static ULONGLONG g_last;
static UINT g_dpi;

static void update_status(void)
{
    mc_get_status(&g_status);
    view_memory(&g_status);
    view_elevated(g_status.elevated);
    if (g_status.load != g_tip)
    {
        mc_tray_tip(g_status.load);
        g_tip = g_status.load;
    }
}

static void on_region(int index)
{
    mc_cfg.mask = mc_mask_toggle(mc_cfg.mask, view_region_bit(index));
    mc_cfg_save();
    view_refresh(g_window, IDC_WORKING + index);
}

static void on_auto(void)
{
    enum mc_auto_result result = mc_toggle_auto(g_window);

    if (result == MC_AUTO_RELAUNCH)
        DestroyWindow(g_window);
    else if (result == MC_AUTO_CANCELED)
        view_status(MC_TEXT_AUTO_CANCELED, VIEW_STATUS_WARNING);
    else if (result == MC_AUTO_NEEDS_ADMIN)
        view_status(MC_TEXT_AUTO_NEEDS_ADMIN, VIEW_STATUS_WARNING);
    else
        view_status(mc_cfg.auto_enabled ? MC_TEXT_AUTO_ON : MC_TEXT_AUTO_OFF, VIEW_STATUS_NORMAL);
    if (result != MC_AUTO_RELAUNCH)
        view_refresh(g_window, IDC_AUTO);
}

static void on_startup(void)
{
    if (mc_toggle_startup())
        view_status(mc_cfg.startup ? MC_TEXT_START_ON : MC_TEXT_START_OFF, VIEW_STATUS_NORMAL);
    else
        view_status(MC_TEXT_START_FAIL, VIEW_STATUS_WARNING);
    view_refresh(g_window, IDC_STARTUP);
}

static void start_clean(bool manual)
{
    if (g_cleaning)
        return;
    if (mc_cfg.mask == 0)
    {
        view_status(MC_TEXT_SELECT, VIEW_STATUS_WARNING);
        return;
    }
    g_cleaning = true;
    view_cleaning(true);
    view_status(manual ? MC_TEXT_CLEANING : MC_TEXT_AUTO_RUNNING, VIEW_STATUS_ACTIVE);

    enum mc_clean_launch result = mc_clean_start(g_window, mc_cfg.mask, manual);
    if (result == MC_CLEAN_OK)
    {
        g_last = GetTickCount64();
        return;
    }

    g_cleaning = false;
    view_cleaning(false);
    view_status(result == MC_CLEAN_CANCELED ? MC_TEXT_ELEVATION_CANCELED
                                            : (result == MC_CLEAN_TEMP ? MC_TEXT_TEMP_ERROR : MC_TEXT_START_ERROR),
                VIEW_STATUS_WARNING);
}

static void maybe_auto(void)
{
    if (!mc_auto_ready(mc_cfg.auto_enabled, g_cleaning, g_status.memory_ok, g_status.elevated, g_status.load,
                       mc_cfg.auto_percent, GetTickCount64(), g_last, mc_cfg.auto_cooldown))
        return;
    start_clean(false);
}

static void clean_done(bool manual, wchar_t *text)
{
    bool failed = mc_report_failed(text);

    g_cleaning = false;
    view_cleaning(false);
    view_result(manual, text, failed);
    view_status(failed ? MC_TEXT_CLEAN_FAILED : MC_TEXT_CLEAN_DONE, failed ? VIEW_STATUS_WARNING : VIEW_STATUS_NORMAL);
    update_status();
}

static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        g_window = window;
        view_init(window, g_instance, g_dpi);
        SetTimer(window, MC_TIMER_ID, MC_TIMER_MS, nullptr);
        return 0;
    case WM_TIMER:
        if (wParam == MC_TIMER_ID)
        {
            update_status();
            maybe_auto();
        }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        view_paint(window);
        return 0;
    case WM_DRAWITEM:
        view_draw((DRAWITEMSTRUCT *)lParam);
        return true;
    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        int notification = HIWORD(wParam);
        int index = view_region_index(id);

        if (index >= 0)
        {
            if (notification == BN_CLICKED)
                on_region(index);
            return 0;
        }

        switch (id)
        {
        case IDC_CLEAN:
            if (notification == BN_CLICKED)
                start_clean(true);
            break;
        case IDC_AUTO:
            if (notification == BN_CLICKED)
                on_auto();
            break;
        case IDC_STARTUP:
            if (notification == BN_CLICKED)
                on_startup();
            break;
        case IDC_MINUS:
            if (notification == BN_CLICKED)
            {
                mc_adjust_percent(-5);
                view_invalidate(window);
            }
            break;
        case IDC_PLUS:
            if (notification == BN_CLICKED)
            {
                mc_adjust_percent(5);
                view_invalidate(window);
            }
            break;
        case IDC_WINDOW_MIN:
            if (notification == BN_CLICKED)
                ShowWindow(window, SW_HIDE);
            break;
        case IDC_WINDOW_CLOSE:
            if (notification == BN_CLICKED)
                PostMessageW(window, WM_CLOSE, 0, 0);
            break;
        case IDC_COFFEE:
            if (notification == BN_CLICKED)
                ShellExecuteW(window, L"open", MC_TEXT_COFFEE_URL, nullptr, nullptr, SW_SHOWNORMAL);
            break;
        case IDM_OPEN:
            view_show(window);
            break;
        case IDM_CLEAN:
            start_clean(true);
            break;
        case IDM_AUTO:
            on_auto();
            break;
        case IDM_STARTUP:
            on_startup();
            break;
        case IDM_ELEVATE:
            if (mc_elevate(window))
                DestroyWindow(window);
            else
                view_status(MC_TEXT_ELEVATION_CANCELED, VIEW_STATUS_WARNING);
            break;
        case IDM_EXIT:
            DestroyWindow(window);
            break;
        case IDM_ABOUT:
            view_about();
            break;
        }
        return 0;
    }
    case MC_WM_TRAY:
        if (LOWORD(lParam) == WM_LBUTTONUP)
            view_show(window);
        else if (LOWORD(lParam) == WM_RBUTTONUP)
            view_tray_menu(window);
        return 0;
    case MC_WM_CLEAN_DONE:
        clean_done((bool)wParam, (wchar_t *)lParam);
        return 0;
    case WM_NCHITTEST:
    {
        LRESULT hit = DefWindowProcW(window, message, wParam, lParam);

        if (hit == HTCLIENT)
        {
            POINT point;
            point.x = (int)(short)LOWORD(lParam);
            point.y = (int)(short)HIWORD(lParam);
            ScreenToClient(window, &point);
            if (point.y >= 0 && point.y < MulDiv(MC_TOPBAR_HEIGHT, (int)g_dpi, 96))
                return HTCAPTION;
        }
        return hit;
    }
    case WM_NCLBUTTONDBLCLK:
        if (wParam == HTCAPTION)
        {
            ShowWindow(window, SW_HIDE);
            return 0;
        }
        break;
    case WM_CLOSE:
        ShowWindow(window, SW_HIDE);
        if (!g_balloon)
        {
            mc_tray_balloon(MC_TEXT_APP, MC_TEXT_CLOSE, NIIF_INFO);
            g_balloon = true;
        }
        return 0;
    case WM_DESTROY:
        KillTimer(window, MC_TIMER_ID);
        mc_tray_remove();
        view_done();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

static int gui_run(void)
{
    MSG message;
    HWND existing;
    WNDCLASSEXW window_class = {0};
    RECT work_area = {0};
    HDC screen;
    int width;
    int height;
    int x;
    int y;

    g_instance = GetModuleHandleW(nullptr);
    g_icon = (HICON)LoadImageW(g_instance, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXICON),
                               GetSystemMetrics(SM_CYICON), 0);
    g_icon_small = (HICON)LoadImageW(g_instance, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                     GetSystemMetrics(SM_CYSMICON), 0);
    if (!g_icon)
        g_icon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));
    if (!g_icon_small)
        g_icon_small = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));

    if (!mc_instance_ok())
    {
        existing = FindWindowW(L"MemCleanMain", nullptr);
        if (existing)
        {
            ShowWindow(existing, SW_SHOW);
            SetForegroundWindow(existing);
        }
        return 0;
    }

    mc_cfg_load();

    screen = GetDC(nullptr);
    g_dpi = screen ? (UINT)GetDeviceCaps(screen, LOGPIXELSX) : 96;
    if (screen)
        ReleaseDC(nullptr, screen);

    g_brush = CreateSolidBrush(MC_BACKGROUND);
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_DROPSHADOW;
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = g_instance;
    window_class.hIcon = g_icon;
    window_class.hIconSm = g_icon_small;
    window_class.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    window_class.hbrBackground = g_brush;
    window_class.lpszClassName = L"MemCleanMain";

    if (!RegisterClassExW(&window_class))
        return 1;

    width = MulDiv(MC_WINDOW_WIDTH, (int)g_dpi, 96);
    height = MulDiv(MC_WINDOW_HEIGHT, (int)g_dpi, 96);
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0);
    x = work_area.left + (work_area.right - work_area.left - width) / 2;
    y = work_area.top + (work_area.bottom - work_area.top - height) / 2;
    if (x < work_area.left)
        x = work_area.left;
    if (y < work_area.top)
        y = work_area.top;

    g_window = CreateWindowExW(WS_EX_APPWINDOW | WS_EX_CONTROLPARENT, L"MemCleanMain", MC_TEXT_APP,
                               WS_POPUP | WS_CLIPCHILDREN, x, y, width, height, nullptr, nullptr, g_instance, nullptr);
    if (!g_window)
        return 1;

    mc_tray_add(g_window, g_icon);
    g_last = GetTickCount64();
    update_status();

    if (mc_cfg.auto_enabled && !mc_is_elevated())
    {
        view_status(MC_TEXT_PAUSED, VIEW_STATUS_WARNING);
        mc_tray_balloon(MC_TEXT_APP, MC_TEXT_AUTO_PAUSED, NIIF_WARNING);
    }
    else
    {
        view_status(MC_TEXT_READY, VIEW_STATUS_NORMAL);
    }

    if (!g_hidden)
    {
        ShowWindow(g_window, SW_SHOW);
        UpdateWindow(g_window);
        SetForegroundWindow(g_window);
    }

    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        if (!IsDialogMessageW(g_window, &message))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    return (int)message.wParam;
}

[[noreturn]] void entry(void)
{
    wchar_t **argv;
    int argc = 0;
    int code;

    if (!mc_init())
        ExitProcess(1);
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv && argc >= 2 && lstrcmpiW(argv[1], MC_WORKER_FLAG) == 0)
        mc_worker_main(argc, argv);
    if (argv && argc >= 2 && lstrcmpiW(argv[1], MC_TRAY_FLAG) == 0)
        g_hidden = true;
    if (argv)
        LocalFree(argv);
    code = gui_run();
    ExitProcess((UINT)code);
}
