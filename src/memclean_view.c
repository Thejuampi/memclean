#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <objidl.h>
#include <gdiplus.h>
#include <string.h>

#include "memclean_view.h"
#include "memclean_host.h"
#include "memclean_ids.h"
#include "memclean_core.h"
#include "memclean_common.h"
#include "memclean_strings.h"

static constexpr int IDR_MARK = 101;
static constexpr int IDB_COFFEE = 102;
static constexpr int MC_STATUS_CHARS = 256;
static constexpr int DWMWA_CORNER = 33;
static constexpr int DWMWA_BORDER = 34;

static constexpr COLORREF C_BG = RGB(6, 16, 15);
static constexpr COLORREF C_TOP = RGB(7, 19, 17);
static constexpr COLORREF C_PANEL = RGB(10, 23, 21);
static constexpr COLORREF C_PANEL2 = RGB(13, 28, 26);
static constexpr COLORREF C_TEXT = RGB(237, 245, 241);
static constexpr COLORREF C_MUTED = RGB(145, 164, 158);
static constexpr COLORREF C_ACCENT = RGB(217, 255, 67);
static constexpr COLORREF C_INK = RGB(7, 16, 15);
static constexpr COLORREF C_LINE = RGB(36, 50, 47);
static constexpr COLORREF C_LINE2 = RGB(24, 38, 35);
static constexpr COLORREF C_LINEB = RGB(58, 76, 70);
static constexpr COLORREF C_GRID = RGB(12, 24, 22);
static constexpr COLORREF C_ORANGE = RGB(230, 159, 0);
static constexpr COLORREF C_BLUE = RGB(86, 180, 233);
static constexpr COLORREF C_PINK = RGB(204, 121, 167);

static UINT g_dpi;

static int sc(int value)
{
    return MulDiv(value, (int)g_dpi, 96);
}

enum
{
    F_BRAND,
    F_SUB,
    F_KICK,
    F_MONO,
    F_MONOB,
    F_SMALL,
    F_BODY,
    F_H2,
    F_BIG,
    F_BTN,
    F_COUNT
};

typedef struct MC_REGION
{
    int id;
    unsigned bit;
    const wchar_t *label;
} MC_REGION;

typedef struct MC_LABEL
{
    short x;
    short y;
    short w;
    short h;
    BYTE f;
    COLORREF c;
    UINT flags;
    BYTE tracking;
    const wchar_t *text;
} MC_LABEL;

static const MC_REGION g_regions[] = {{IDC_WORKING, MC_CLEAN_WORKING, L"Working set"},
                                      {IDC_FILECACHE, MC_CLEAN_FILECACHE, L"File cache"},
                                      {IDC_MODIFIED, MC_CLEAN_MODIFIED, L"Modified list"},
                                      {IDC_STANDBY, MC_CLEAN_STANDBY, L"Standby list"},
                                      {IDC_STANDBY_LOW, MC_CLEAN_STANDBY_LOW, L"Low standby list"},
                                      {IDC_REGISTRY, MC_CLEAN_REGISTRY, L"Registry cache"},
                                      {IDC_COMBINE, MC_CLEAN_COMBINE, L"Combine lists"}};

static constexpr int REGION_COUNT = (int)MC_ARRAY_COUNT(g_regions);

static const struct
{
    int px;
    int weight;
    int face;
} g_font_def[F_COUNT] = {{13, FW_BOLD, 0}, {9, FW_NORMAL, 0},  {10, FW_BOLD, 0},   {12, FW_NORMAL, 0},
                         {12, FW_BOLD, 0}, {10, FW_NORMAL, 0}, {13, FW_NORMAL, 1}, {16, 600, 1},
                         {40, 600, 1},     {12, FW_BOLD, 0}};

static const MC_LABEL g_labels[] = {
    {56, 11, 220, 16, F_BRAND, C_TEXT, DT_LEFT | DT_VCENTER | DT_SINGLELINE, 2, L"MEMCLEAN"},
    {56, 28, 220, 13, F_SUB, C_MUTED, DT_LEFT | DT_VCENTER | DT_SINGLELINE, 1, L"NATIVE MEMORY TOOL"},
    {24, 62, 300, 14, F_KICK, C_ACCENT, DT_LEFT | DT_VCENTER | DT_SINGLELINE, 1, L"PHYSICAL MEMORY"},
    {24, 184, 112, 14, F_SMALL, C_MUTED, DT_LEFT | DT_VCENTER | DT_SINGLELINE, 1, L"AVAILABLE"},
    {144, 184, 112, 14, F_SMALL, C_MUTED, DT_LEFT | DT_VCENTER | DT_SINGLELINE, 1, L"STANDBY"},
    {264, 184, 112, 14, F_SMALL, C_MUTED, DT_LEFT | DT_VCENTER | DT_SINGLELINE, 1, L"MODIFIED"},
    {384, 184, 112, 14, F_SMALL, C_MUTED, DT_LEFT | DT_VCENTER | DT_SINGLELINE, 1, L"CACHE"},
    {40, 238, 200, 14, F_KICK, C_ACCENT, DT_LEFT | DT_VCENTER | DT_SINGLELINE, 1, L"TARGETS"},
    {40, 250, 300, 22, F_H2, C_TEXT, DT_LEFT | DT_VCENTER | DT_SINGLELINE, 0, L"Clean regions"},
    {40, 406, 220, 14, F_KICK, C_ACCENT, DT_LEFT | DT_VCENTER | DT_SINGLELINE, 1, L"AUTOMATION"},
    {40, 418, 260, 22, F_H2, C_TEXT, DT_LEFT | DT_VCENTER | DT_SINGLELINE, 0, L"Auto-clean"},
    {40, 452, 160, 16, F_KICK, C_MUTED, DT_LEFT | DT_VCENTER | DT_SINGLELINE, 1, L"WHEN MEMORY REACHES"}};

static constexpr int LABEL_COUNT = (int)MC_ARRAY_COUNT(g_labels);

static HWND g_window;
static HINSTANCE g_instance;
static HDC g_dc;
static GpGraphics *g_gp;
static HFONT g_font[F_COUNT];
static GpBitmap *g_mark;
static HBITMAP g_coffee;
static HICON g_shield;
static HWND g_clean;
static MC_STATUS g_mem;
static wchar_t g_status[MC_STATUS_CHARS];
static enum mc_view_status g_kind;
static int g_w;
static int g_h;
static ULONG_PTR g_token;
static bool g_gdiplus;
static bool g_limited = true;
static bool g_cleaning;

static ARGB argb(COLORREF color)
{
    return ((ARGB)0xFF << 24) | ((ARGB)GetRValue(color) << 16) | ((ARGB)GetGValue(color) << 8) | (ARGB)GetBValue(color);
}

static GpGraphics *gp_open(HDC dc)
{
    GpGraphics *g = nullptr;

    if (GdipCreateFromHDC(dc, &g) != Ok)
        return nullptr;
    GdipSetSmoothingMode(g, SmoothingModeAntiAlias);
    GdipSetPixelOffsetMode(g, PixelOffsetModeHalf);
    return g;
}

static GpPath *path_round(RECT r, int radius)
{
    GpPath *path = nullptr;
    REAL d;

    if (radius <= 0)
        radius = 1;
    d = (REAL)(radius * 2);
    if (d > (REAL)(r.bottom - r.top))
        d = (REAL)(r.bottom - r.top);
    if (d > (REAL)(r.right - r.left))
        d = (REAL)(r.right - r.left);
    if (GdipCreatePath(FillModeAlternate, &path) != Ok)
        return nullptr;
    GdipAddPathArc(path, (REAL)r.left, (REAL)r.top, d, d, 180.0f, 90.0f);
    GdipAddPathArc(path, (REAL)r.right - d, (REAL)r.top, d, d, 270.0f, 90.0f);
    GdipAddPathArc(path, (REAL)r.right - d, (REAL)r.bottom - d, d, d, 0.0f, 90.0f);
    GdipAddPathArc(path, (REAL)r.left, (REAL)r.bottom - d, d, d, 90.0f, 90.0f);
    GdipClosePathFigure(path);
    return path;
}

static void fill_rect(GpGraphics *g, RECT r, COLORREF c)
{
    GpSolidFill *b = nullptr;

    if (GdipCreateSolidFill(argb(c), &b) != Ok)
        return;
    GdipFillRectangle(g, (GpBrush *)b, (REAL)r.left, (REAL)r.top, (REAL)(r.right - r.left), (REAL)(r.bottom - r.top));
    GdipDeleteBrush((GpBrush *)b);
}

static void fill_round(GpGraphics *g, RECT r, int radius, COLORREF c)
{
    GpPath *p = path_round(r, radius);
    GpSolidFill *b = nullptr;

    if (!p)
        return;
    if (GdipCreateSolidFill(argb(c), &b) == Ok)
    {
        GdipFillPath(g, (GpBrush *)b, p);
        GdipDeleteBrush((GpBrush *)b);
    }
    GdipDeletePath(p);
}

static void stroke_round(GpGraphics *g, RECT r, int radius, COLORREF c)
{
    GpPath *p = path_round(r, radius);
    GpPen *pen = nullptr;

    if (!p)
        return;
    if (GdipCreatePen1(argb(c), 1.0f, UnitPixel, &pen) == Ok)
    {
        GdipDrawPath(g, pen, p);
        GdipDeletePen(pen);
    }
    GdipDeletePath(p);
}

static void fill_dot(GpGraphics *g, int x, int y, int radius, COLORREF c)
{
    GpSolidFill *b = nullptr;

    if (GdipCreateSolidFill(argb(c), &b) != Ok)
        return;
    GdipFillEllipse(g, (GpBrush *)b, (REAL)(x - radius), (REAL)(y - radius), (REAL)(radius * 2), (REAL)(radius * 2));
    GdipDeleteBrush((GpBrush *)b);
}

static void draw_line(GpGraphics *g, int x1, int y1, int x2, int y2, COLORREF c)
{
    GpPen *pen = nullptr;

    if (GdipCreatePen1(argb(c), 1.0f, UnitPixel, &pen) != Ok)
        return;
    GdipDrawLine(g, pen, (REAL)x1, (REAL)y1, (REAL)x2, (REAL)y2);
    GdipDeletePen(pen);
}

static void text_draw(HDC dc, HFONT font, COLORREF color, const wchar_t *text, RECT r, UINT flags, int tracking)
{
    HGDIOBJ old_font = SelectObject(dc, font);
    int old_track = SetTextCharacterExtra(dc, tracking);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    DrawTextW(dc, text, -1, &r, flags | DT_NOPREFIX);
    SetTextCharacterExtra(dc, old_track);
    SelectObject(dc, old_font);
}

static int text_width(HDC dc, HFONT font, const wchar_t *text)
{
    HGDIOBJ old_font = SelectObject(dc, font);
    SIZE size;

    GetTextExtentPoint32W(dc, text, lstrlenW(text), &size);
    SelectObject(dc, old_font);
    return size.cx;
}

static int tracked_width(HDC dc, HFONT font, const wchar_t *text, int tracking)
{
    int length = lstrlenW(text);
    int width = text_width(dc, font, text);

    if (length > 1 && tracking != 0)
        width += (length - 1) * tracking;
    return width;
}

static RECT rect(int x, int y, int w, int h)
{
    RECT r;
    r.left = sc(x);
    r.top = sc(y);
    r.right = sc(x + w);
    r.bottom = sc(y + h);
    return r;
}

static RECT inset(RECT r, int amount)
{
    r.left += amount;
    r.top += amount;
    r.right -= amount;
    r.bottom -= amount;
    return r;
}

static void fill(int x, int y, int w, int h, int radius, COLORREF c)
{
    fill_round(g_gp, rect(x, y, w, h), sc(radius), c);
}

static void frame(int x, int y, int w, int h, int radius, COLORREF c)
{
    stroke_round(g_gp, rect(x, y, w, h), sc(radius), c);
}

static void panel(int x, int y, int w, int h, int radius, COLORREF fill_color, COLORREF border_color)
{
    fill(x, y, w, h, radius, fill_color);
    if (border_color)
        frame(x, y, w, h, radius, border_color);
}

static void panel_px(RECT r, int radius, COLORREF fill_color, COLORREF border_color)
{
    fill_round(g_gp, r, radius, fill_color);
    if (border_color)
        stroke_round(g_gp, r, radius, border_color);
}

static void pill_px(RECT r, COLORREF fill_color, COLORREF border_color)
{
    panel_px(r, (r.bottom - r.top) / 2, fill_color, border_color);
}

static void check_box(RECT box)
{
    draw_line(g_gp, box.left + sc(3), box.top + sc(7), box.left + sc(6), box.top + sc(10), C_INK);
    draw_line(g_gp, box.left + sc(6), box.top + sc(10), box.left + sc(11), box.top + sc(4), C_INK);
}

static void line(int x1, int y1, int x2, int y2, COLORREF c)
{
    draw_line(g_gp, sc(x1), sc(y1), sc(x2), sc(y2), c);
}

static void text_px(RECT r, int f, COLORREF c, UINT flags, int tracking, const wchar_t *s)
{
    text_draw(g_dc, g_font[f], c, s, r, flags, tracking);
}

static void text(int x, int y, int w, int h, int f, COLORREF c, UINT flags, int tracking, const wchar_t *s)
{
    text_px(rect(x, y, w, h), f, c, flags, tracking, s);
}

static void tk(int x, int y, int w, int h, int f, COLORREF c, int tracking, const wchar_t *s)
{
    text(x, y, w, h, f, c, DT_LEFT | DT_VCENTER | DT_SINGLELINE, tracking, s);
}

static int tw(int f, const wchar_t *s)
{
    return text_width(g_dc, g_font[f], s);
}

static int twt(int f, const wchar_t *s, int tracking)
{
    return tracked_width(g_dc, g_font[f], s, tracking);
}

static const wchar_t *size_unit_name(enum mc_size_unit unit)
{
    switch (unit)
    {
    case MC_UNIT_GB:
        return L"GB";
    case MC_UNIT_MB:
        return L"MB";
    case MC_UNIT_KB:
        return L"KB";
    }

    return L"KB";
}

static void split_size(uint64_t bytes, wchar_t *number, wchar_t *unit)
{
    MC_SIZE_PARTS parts = mc_size_parts(bytes);

    if (parts.fractional)
        wsprintfW(number, L"%u.%u", parts.whole, parts.fraction);
    else
        wsprintfW(number, L"%u", parts.whole);
    lstrcpyW(unit, size_unit_name(parts.unit));
}

static void format_size_w(uint64_t bytes, wchar_t *buffer)
{
    wchar_t number[16];
    wchar_t unit[8];

    split_size(bytes, number, unit);
    wsprintfW(buffer, L"%s %s", number, unit);
}

int view_region_index(int id)
{
    for (int i = 0; i < REGION_COUNT; i++)
        if (g_regions[i].id == id)
            return i;
    return -1;
}

unsigned view_region_bit(int index)
{
    return (index >= 0 && index < REGION_COUNT) ? g_regions[index].bit : 0;
}

static GpBitmap *load_mark(void)
{
    HRSRC resource = FindResourceW(g_instance, MAKEINTRESOURCEW(IDR_MARK), RT_RCDATA);
    DWORD size;
    HGLOBAL loaded;
    HGLOBAL memory;
    void *data;
    void *destination;
    IStream *stream = nullptr;
    GpBitmap *bitmap = nullptr;

    if (!resource)
        return nullptr;
    size = SizeofResource(g_instance, resource);
    loaded = LoadResource(g_instance, resource);
    data = loaded ? LockResource(loaded) : nullptr;
    if (!data || size == 0)
        return nullptr;
    memory = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!memory)
        return nullptr;
    destination = GlobalLock(memory);
    if (!destination)
    {
        GlobalFree(memory);
        return nullptr;
    }
    memcpy(destination, data, size);
    GlobalUnlock(memory);
    if (CreateStreamOnHGlobal(memory, true, &stream) != S_OK)
    {
        GlobalFree(memory);
        return nullptr;
    }
    if (GdipCreateBitmapFromStream(stream, &bitmap) != Ok)
        bitmap = nullptr;
    stream->lpVtbl->Release(stream);
    return bitmap;
}

static void fonts_create(void)
{
    for (int i = 0; i < F_COUNT; i++)
    {
        int face = g_font_def[i].face;
        g_font[i] = CreateFontW(-sc(g_font_def[i].px), 0, 0, 0, g_font_def[i].weight, false, false, false,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                                face ? (DEFAULT_PITCH | FF_SWISS) : (FIXED_PITCH | FF_MODERN),
                                face ? L"Segoe UI" : L"Consolas");
    }
}

static void fonts_delete(void)
{
    for (int i = 0; i < F_COUNT; i++)
        if (g_font[i])
            DeleteObject(g_font[i]);
}

static HWND add_button(int id, int x, int y, int w, int h, const wchar_t *s, bool tab)
{
    HWND button = CreateWindowExW(0, L"BUTTON", s,
                                  WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_OWNERDRAW | (tab ? WS_TABSTOP : 0),
                                  sc(x), sc(y), sc(w), sc(h), g_window, (HMENU)(INT_PTR)id, g_instance, nullptr);
    if (button)
        SendMessageW(button, WM_SETFONT, (WPARAM)g_font[F_BODY], true);
    return button;
}

static void controls_create(void)
{
    static const struct
    {
        int id;
        int x;
        int y;
        int w;
        int h;
        const wchar_t *text;
        bool tab;
    } defs[] = {{IDC_WINDOW_MIN, 436, 8, 36, 36, L"Minimize", false},
                {IDC_WINDOW_CLOSE, 472, 8, 36, 36, L"Close", false},
                {IDC_AUTO, 432, 414, 48, 26, L"Auto-clean", true},
                {IDC_MINUS, 200, 446, 22, 22, L"Decrease threshold", true},
                {IDC_PLUS, 282, 446, 22, 22, L"Increase threshold", true},
                {IDC_STARTUP, 40, 484, 440, 24, L"Start with Windows", true},
                {IDC_CLEAN, 24, 526, 472, 44, L"Clean now", true},
                {IDC_COFFEE, 411, 577, 85, 23, MC_TEXT_COFFEE, false}};

    for (int i = 0; i < REGION_COUNT; i++)
    {
        int column = i < 4 ? 0 : 1;
        int row = i < 4 ? i : i - 4;
        add_button(g_regions[i].id, 40 + column * 224, 284 + row * 24, 208, 22, g_regions[i].label, true);
    }

    for (size_t i = 0; i < MC_ARRAY_COUNT(defs); i++)
    {
        HWND button = add_button(defs[i].id, defs[i].x, defs[i].y, defs[i].w, defs[i].h, defs[i].text, defs[i].tab);
        if (defs[i].id == IDC_CLEAN)
            g_clean = button;
    }
}

static void paint_background(void)
{
    GpLineGradient *gradient = nullptr;
    GpPointF from = {0, 0};
    GpPointF to = {0, (REAL)g_h};

    if (GdipCreateLineBrush(&from, &to, argb(C_TOP), argb(C_BG), WrapModeTile, &gradient) == Ok)
    {
        GdipFillRectangle(g_gp, (GpBrush *)gradient, 0, 0, (REAL)g_w, (REAL)g_h);
        GdipDeleteBrush((GpBrush *)gradient);
    }
    else
    {
        fill_rect(g_gp, (RECT){0, 0, g_w, g_h}, C_BG);
    }

    for (int i = 0; i < g_w; i += sc(32))
        draw_line(g_gp, i, 0, i, g_h, C_GRID);
    for (int i = 0; i < g_h; i += sc(32))
        draw_line(g_gp, 0, i, g_w, i, C_GRID);
}

static void paint_top(void)
{
    const wchar_t *access = g_limited ? L"LIMITED" : L"ADMIN";
    COLORREF access_color = g_limited ? C_ORANGE : C_ACCENT;
    int right = sc(424);
    int live_width = twt(F_KICK, L"LIVE", sc(1));
    int live_x = right - live_width;
    int dot_x = live_x - sc(11);
    int pill_width = twt(F_KICK, access, sc(1)) + sc(20);
    RECT pill = {dot_x - sc(14) - pill_width, sc(16), dot_x - sc(14), sc(36)};

    draw_line(g_gp, 0, sc(52), g_w, sc(52), C_LINE);

    if (g_mark)
    {
        GdipSetInterpolationMode(g_gp, InterpolationModeHighQualityBicubic);
        GdipDrawImageRectI(g_gp, (GpImage *)g_mark, sc(24), sc(15), sc(22), sc(22));
    }
    else
    {
        fill_dot(g_gp, sc(35), sc(26), sc(11), C_ACCENT);
    }

    pill_px(pill, C_PANEL2, C_LINE);
    text_px(pill, F_KICK, access_color, DT_CENTER | DT_VCENTER | DT_SINGLELINE, sc(1), access);

    fill_dot(g_gp, dot_x, sc(26), sc(3), C_ACCENT);
    text_px((RECT){live_x, sc(17), right, sc(35)}, F_KICK, C_MUTED, DT_LEFT | DT_VCENTER | DT_SINGLELINE, sc(1),
            L"LIVE");
}

static void paint_labels(void)
{
    for (int i = 0; i < LABEL_COUNT; i++)
        text(g_labels[i].x, g_labels[i].y, g_labels[i].w, g_labels[i].h, g_labels[i].f, g_labels[i].c,
             g_labels[i].flags, g_labels[i].tracking, g_labels[i].text);
}

static void paint_hero(void)
{
    wchar_t number[16];
    wchar_t unit[8];
    wchar_t total[32];
    wchar_t buffer[96];
    int number_width;

    split_size(g_mem.used, number, unit);
    format_size_w(g_mem.total, total);
    number_width = tw(F_BIG, number);

    text(24, 76, 220, 50, F_BIG, C_TEXT, DT_LEFT | DT_TOP | DT_SINGLELINE, 0, number);
    {
        RECT unit_rect = {sc(24) + number_width + sc(8), sc(101), sc(24) + number_width + sc(8) + sc(40), sc(126)};
        RECT used_rect = {unit_rect.right + sc(12), sc(101), unit_rect.right + sc(82), sc(126)};
        text_px(unit_rect, F_BRAND, C_MUTED, DT_LEFT | DT_BOTTOM | DT_SINGLELINE, 0, unit);
        text_px(used_rect, F_KICK, C_MUTED, DT_LEFT | DT_BOTTOM | DT_SINGLELINE, sc(1), L"USED");
    }

    wsprintfW(buffer, L"OF %s  ·  %u%% IN USE", total, (unsigned)g_mem.load);
    text(24, 134, 440, 16, F_SMALL, C_MUTED, DT_LEFT | DT_VCENTER | DT_SINGLELINE, 0, buffer);

    panel(24, 156, 472, 8, 4, C_PANEL2, C_LINE);
    if (g_mem.load > 0)
    {
        int width = 472 * (int)g_mem.load / 100;
        if (width < 8)
            width = 8;
        if (width > 472)
            width = 472;
        fill(24, 156, width, 8, 4, C_ACCENT);
    }

    {
        COLORREF colors[4] = {C_ACCENT, C_BLUE, C_ORANGE, C_PINK};
        uint64_t values[4] = {g_mem.available, g_mem.standby, g_mem.modified, g_mem.cache};

        for (int i = 0; i < 4; i++)
        {
            int x = 24 + i * 120;
            fill(x, 176, 16, 2, 1, colors[i]);
            if (i == 1 && g_mem.list_status < 0)
                lstrcpyW(buffer, L"-");
            else if (i == 3 && g_mem.cache_status < 0)
                lstrcpyW(buffer, L"-");
            else
                format_size_w(values[i], buffer);
            text(x, 198, 112, 18, F_MONO, C_TEXT, DT_LEFT | DT_VCENTER | DT_SINGLELINE, 0, buffer);
        }
    }
}

static void paint_cards(void)
{
    wchar_t badge[32];
    wchar_t percent[16];
    wchar_t cooldown[32];
    int badge_width;

    panel(24, 224, 472, 156, 16, C_PANEL, C_LINE);
    panel(24, 392, 472, 120, 16, C_PANEL, C_LINE);

    if (mc_cfg.mask == MC_CLEAN_SAFE)
        lstrcpyW(badge, L"SAFE SET");
    else
        wsprintfW(badge, L"%u ACTIVE", mc_mask_count(mc_cfg.mask));
    badge_width = twt(F_KICK, badge, sc(1)) + sc(20);
    {
        RECT badge_rect = {sc(480) - badge_width, sc(240), sc(480), sc(260)};
        pill_px(badge_rect, C_PANEL2, C_LINE);
        text_px(badge_rect, F_KICK, C_MUTED, DT_CENTER | DT_VCENTER | DT_SINGLELINE, sc(1), badge);
    }

    wsprintfW(percent, L"%u%%", (unsigned)mc_cfg.auto_percent);
    text(226, 446, 56, 24, F_MONOB, C_ACCENT, DT_CENTER | DT_VCENTER | DT_SINGLELINE, 0, percent);
    wsprintfW(cooldown, L"COOLDOWN %u MIN", (unsigned)((mc_cfg.auto_cooldown + 59) / 60));
    text(320, 452, 160, 16, F_KICK, C_MUTED, DT_LEFT | DT_VCENTER | DT_SINGLELINE, sc(1), cooldown);
    line(40, 476, 480, 476, C_LINE2);
}

static void paint_status(void)
{
    COLORREF dot_color = g_kind == VIEW_STATUS_WARNING ? C_ORANGE : C_ACCENT;
    COLORREF text_color = g_kind == VIEW_STATUS_WARNING ? C_ORANGE : (g_kind == VIEW_STATUS_ACTIVE ? C_TEXT : C_MUTED);

    fill_dot(g_gp, sc(28), sc(588), sc(3), dot_color);
    tk(40, 580, 360, 16, F_SMALL, text_color, sc(1), g_status);
}

static void paint_ui(void)
{
    paint_background();
    paint_cards();
    paint_top();
    paint_labels();
    paint_hero();
    paint_status();
}

static void draw_region(DRAWITEMSTRUCT *item, int index)
{
    RECT r = item->rcItem;
    RECT box;
    RECT label;
    int size = sc(14);
    bool checked = mc_mask_has(mc_cfg.mask, g_regions[index].bit);

    fill_rect(g_gp, r, C_PANEL);
    if (item->itemState & ODS_SELECTED)
        fill_round(g_gp, r, sc(6), C_PANEL2);

    box.left = r.left;
    box.top = r.top + (r.bottom - r.top - size) / 2;
    box.right = box.left + size;
    box.bottom = box.top + size;

    if (checked)
    {
        fill_round(g_gp, box, sc(4), C_ACCENT);
        check_box(box);
    }
    else
    {
        panel_px(box, sc(4), C_PANEL2, C_LINEB);
    }

    label = r;
    label.left = box.right + sc(8);
    text_px(label, F_BODY, C_TEXT, DT_LEFT | DT_VCENTER | DT_SINGLELINE, 0, g_regions[index].label);

    if (item->itemState & ODS_FOCUS)
        stroke_round(g_gp, inset(r, sc(2)), sc(5), C_ACCENT);
}

static void draw_toggle(DRAWITEMSTRUCT *item, bool checked, const wchar_t *label)
{
    RECT r = item->rcItem;
    RECT pill;
    int width = sc(40);
    int height = sc(22);
    int knob = sc(14);

    fill_rect(g_gp, r, C_PANEL);

    if (label)
    {
        RECT text_rect = r;
        text_rect.right -= width + sc(16);
        text_px(text_rect, F_KICK, C_MUTED, DT_LEFT | DT_VCENTER | DT_SINGLELINE, sc(1), label);
    }

    pill.right = r.right - sc(4);
    pill.left = pill.right - width;
    pill.top = r.top + (r.bottom - r.top - height) / 2;
    pill.bottom = pill.top + height;

    pill_px(pill, checked ? C_ACCENT : C_PANEL2, checked ? 0 : C_LINEB);
    fill_dot(g_gp, checked ? pill.right - height / 2 : pill.left + height / 2, (pill.top + pill.bottom) / 2, knob / 2,
             checked ? C_INK : C_MUTED);

    if (item->itemState & ODS_FOCUS)
        stroke_round(g_gp, inset(r, sc(2)), sc(6), C_ACCENT);
}

static void draw_step(DRAWITEMSTRUCT *item, bool plus)
{
    RECT r = item->rcItem;

    fill_rect(g_gp, r, C_PANEL);
    panel_px(r, sc(6), (item->itemState & ODS_SELECTED) ? C_PANEL2 : C_PANEL, C_LINEB);
    text_px(r, F_MONOB, C_TEXT, DT_CENTER | DT_VCENTER | DT_SINGLELINE, 0, plus ? L"+" : L"\x2212");
    if (item->itemState & ODS_FOCUS)
        stroke_round(g_gp, inset(r, sc(1)), sc(6), C_ACCENT);
}

static void draw_clean(DRAWITEMSTRUCT *item)
{
    RECT r = item->rcItem;
    const wchar_t *label = g_cleaning ? MC_TEXT_CLEANING : L"CLEAN NOW";
    COLORREF fill_color = g_cleaning ? C_PANEL2 : ((item->itemState & ODS_SELECTED) ? RGB(196, 232, 60) : C_ACCENT);
    COLORREF text_color = g_cleaning ? C_ACCENT : C_INK;
    int text_width_value;
    int icon_width;
    int total;
    int x;
    RECT text_rect;
    bool shield = g_shield && g_limited;

    fill_rect(g_gp, r, C_BG);
    panel_px(r, sc(10), fill_color, g_cleaning ? C_LINEB : 0);
    if (item->itemState & ODS_FOCUS)
        stroke_round(g_gp, inset(r, sc(3)), sc(8), C_INK);

    text_width_value = twt(F_BTN, label, sc(2));
    icon_width = shield ? sc(16) : 0;
    total = text_width_value + icon_width + (shield ? sc(8) : 0);
    x = r.left + (r.right - r.left - total) / 2;

    if (shield)
        DrawIconEx(g_dc, x, r.top + (r.bottom - r.top - sc(16)) / 2, g_shield, sc(16), sc(16), 0, nullptr, DI_NORMAL);
    text_rect = r;
    text_rect.left = x + icon_width + (shield ? sc(8) : 0);
    text_px(text_rect, F_BTN, text_color, DT_LEFT | DT_VCENTER | DT_SINGLELINE, sc(2), label);
}

static void draw_coffee(DRAWITEMSTRUCT *item)
{
    RECT r = item->rcItem;
    HDC memory;
    HGDIOBJ old;
    BITMAP bitmap;

    fill_rect(g_gp, r, C_BG);
    if (!g_coffee || !GetObjectW(g_coffee, sizeof(bitmap), &bitmap))
        return;

    memory = CreateCompatibleDC(g_dc);
    if (!memory)
        return;

    old = SelectObject(memory, g_coffee);
    SetStretchBltMode(g_dc, HALFTONE);
    StretchBlt(g_dc, r.left, r.top, r.right - r.left, r.bottom - r.top, memory, 0, 0, bitmap.bmWidth, bitmap.bmHeight,
               SRCCOPY);
    SelectObject(memory, old);
    DeleteDC(memory);

    if (item->itemState & ODS_FOCUS)
        stroke_round(g_gp, inset(r, sc(1)), sc(4), C_ACCENT);
}

static void draw_caption(DRAWITEMSTRUCT *item, bool close)
{
    RECT r = item->rcItem;
    int center_x = (r.left + r.right) / 2;
    int center_y = (r.top + r.bottom) / 2;
    COLORREF color = (item->itemState & ODS_SELECTED) ? C_TEXT : C_MUTED;

    fill_rect(g_gp, r, C_TOP);
    if (item->itemState & ODS_SELECTED)
        fill_round(g_gp, inset(r, sc(4)), sc(6), close ? RGB(60, 28, 32) : C_PANEL2);

    if (close)
    {
        draw_line(g_gp, center_x - sc(4), center_y - sc(4), center_x + sc(4), center_y + sc(4), color);
        draw_line(g_gp, center_x + sc(4), center_y - sc(4), center_x - sc(4), center_y + sc(4), color);
    }
    else
    {
        draw_line(g_gp, center_x - sc(5), center_y, center_x + sc(5), center_y, color);
    }

    if (item->itemState & ODS_FOCUS)
        stroke_round(g_gp, inset(r, sc(5)), sc(5), C_ACCENT);
}

void view_draw(DRAWITEMSTRUCT *item)
{
    int index = view_region_index(item->CtlID);

    g_dc = item->hDC;
    g_gp = gp_open(item->hDC);
    if (!g_gp)
        return;

    if (index >= 0)
        draw_region(item, index);
    else
    {
        switch (item->CtlID)
        {
        case IDC_AUTO:
            draw_toggle(item, mc_cfg.auto_enabled != 0, nullptr);
            break;
        case IDC_STARTUP:
            draw_toggle(item, mc_cfg.startup, L"START WITH WINDOWS");
            break;
        case IDC_MINUS:
            draw_step(item, false);
            break;
        case IDC_PLUS:
            draw_step(item, true);
            break;
        case IDC_CLEAN:
            draw_clean(item);
            break;
        case IDC_COFFEE:
            draw_coffee(item);
            break;
        case IDC_WINDOW_MIN:
            draw_caption(item, false);
            break;
        case IDC_WINDOW_CLOSE:
            draw_caption(item, true);
            break;
        }
    }

    GdipDeleteGraphics(g_gp);
    g_gp = nullptr;
    g_dc = nullptr;
}

static void paint_window(HWND window)
{
    PAINTSTRUCT paint;
    HDC dc = BeginPaint(window, &paint);
    RECT client;
    HDC memory;
    HBITMAP bitmap;
    HGDIOBJ old;

    GetClientRect(window, &client);
    g_w = client.right;
    g_h = client.bottom;
    memory = CreateCompatibleDC(dc);
    bitmap = CreateCompatibleBitmap(dc, g_w, g_h);
    old = SelectObject(memory, bitmap);
    g_dc = memory;
    g_gp = gp_open(memory);

    if (g_gp)
    {
        paint_ui();
        GdipDeleteGraphics(g_gp);
        g_gp = nullptr;
    }
    else
    {
        RECT full = {0, 0, g_w, g_h};
        FillRect(memory, &full, (HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    BitBlt(dc, 0, 0, g_w, g_h, memory, 0, 0, SRCCOPY);
    SelectObject(memory, old);
    DeleteObject(bitmap);
    DeleteDC(memory);
    g_dc = nullptr;
    EndPaint(window, &paint);
}

static void window_effects(HWND window)
{
    HMODULE module = LoadLibraryW(L"dwmapi.dll");
    typedef HRESULT(WINAPI * DWM_SET_ATTRIBUTE)(HWND, DWORD, LPCVOID, DWORD);
    DWM_SET_ATTRIBUTE set_attribute;
    int corner = 2;
    COLORREF border = C_LINE;

    if (!module)
        return;
    set_attribute = (DWM_SET_ATTRIBUTE)(void *)GetProcAddress(module, "DwmSetWindowAttribute");
    if (set_attribute)
    {
        set_attribute(window, DWMWA_CORNER, &corner, sizeof(corner));
        set_attribute(window, DWMWA_BORDER, &border, sizeof(border));
    }
    FreeLibrary(module);
}

void view_init(HWND window, HINSTANCE instance, UINT dpi)
{
    GdiplusStartupInput input = {0};
    SHSTOCKICONINFO shield = {0};

    g_window = window;
    g_instance = instance;
    g_dpi = dpi;

    input.GdiplusVersion = 1;
    if (GdiplusStartup(&g_token, &input, nullptr) == Ok)
        g_gdiplus = true;

    shield.cbSize = sizeof(shield);
    if (SHGetStockIconInfo(77, SHGSI_ICON | SHGSI_SMALLICON, &shield) == S_OK)
        g_shield = shield.hIcon;

    g_mark = load_mark();
    g_coffee = LoadBitmapW(g_instance, MAKEINTRESOURCEW(IDB_COFFEE));
    lstrcpynW(g_status, MC_TEXT_READY, MC_STATUS_CHARS);
    fonts_create();
    controls_create();
    window_effects(window);
}

void view_done(void)
{
    fonts_delete();
    if (g_mark)
        GdipDisposeImage((GpImage *)g_mark);
    if (g_coffee)
        DeleteObject(g_coffee);
    if (g_shield)
        DestroyIcon(g_shield);
    if (g_gdiplus)
        GdiplusShutdown(g_token);
}

void view_paint(HWND window)
{
    paint_window(window);
}

void view_memory(const MC_STATUS *status)
{
    g_mem = *status;
    InvalidateRect(g_window, nullptr, false);
}

void view_status(const wchar_t *text, enum mc_view_status kind)
{
    lstrcpynW(g_status, text, MC_STATUS_CHARS);
    g_kind = kind;
    InvalidateRect(g_window, nullptr, false);
}

void view_elevated(bool elevated)
{
    bool limited = !elevated;

    if (limited == g_limited)
        return;
    g_limited = limited;
    InvalidateRect(g_window, nullptr, false);
    if (g_clean)
        InvalidateRect(g_clean, nullptr, false);
}

void view_cleaning(bool cleaning)
{
    g_cleaning = cleaning;
    if (g_clean)
    {
        EnableWindow(g_clean, !cleaning);
        InvalidateRect(g_clean, nullptr, false);
    }
}

void view_invalidate(HWND window)
{
    InvalidateRect(window, nullptr, false);
}

void view_show(HWND window)
{
    ShowWindow(window, SW_SHOW);
    SetForegroundWindow(window);
}

void view_refresh(HWND window, int id)
{
    HWND control = GetDlgItem(window, id);
    if (control)
        InvalidateRect(control, nullptr, false);
    InvalidateRect(window, nullptr, false);
}

void view_about(void)
{
    MessageBoxW(g_window, MC_TEXT_ABOUT_BODY, MC_TEXT_ABOUT_TITLE, MB_OK | MB_ICONINFORMATION);
}

void view_result(bool manual, wchar_t *text, bool failed)
{
    if (text)
    {
        if (manual)
            MessageBoxW(g_window, text, MC_TEXT_RESULT_TITLE, MB_OK | (failed ? MB_ICONWARNING : MB_ICONINFORMATION));
        else
            mc_tray_balloon(MC_TEXT_APP, failed ? MC_TEXT_AUTO_FAIL : MC_TEXT_AUTO_OK,
                            failed ? NIIF_WARNING : NIIF_INFO);
        HeapFree(GetProcessHeap(), 0, text);
    }
    else if (manual)
    {
        MessageBoxW(g_window, MC_TEXT_NO_REPORT, MC_TEXT_APP, MB_OK | MB_ICONWARNING);
    }
}

void view_tray_menu(HWND owner)
{
    HMENU menu = CreatePopupMenu();
    POINT point;
    int command;

    if (!menu)
        return;

    AppendMenuW(menu, MF_STRING, IDM_OPEN, L"Open MemClean");
    AppendMenuW(menu, MF_STRING, IDM_CLEAN, L"Clean now");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (mc_cfg.auto_enabled ? MF_CHECKED : 0), IDM_AUTO, L"Auto-clean");
    AppendMenuW(menu, MF_STRING | (mc_cfg.startup ? MF_CHECKED : 0), IDM_STARTUP, L"Start with Windows");
    if (!mc_is_elevated())
        AppendMenuW(menu, MF_STRING, IDM_ELEVATE, L"Restart as administrator");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_ABOUT, L"About MemClean");
    AppendMenuW(menu, MF_STRING, IDM_EXIT, L"Exit");

    GetCursorPos(&point);
    SetForegroundWindow(owner);
    command = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RETURNCMD, point.x, point.y,
                             0, owner, nullptr);
    DestroyMenu(menu);
    PostMessageW(owner, WM_NULL, 0, 0);
    if (command)
        PostMessageW(owner, WM_COMMAND, command, 0);
}
