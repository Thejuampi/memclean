#ifndef MEMCLEAN_VIEW_H
#define MEMCLEAN_VIEW_H

#include <windows.h>
#include "memclean_engine.h"

enum mc_view_status
{
    VIEW_STATUS_NORMAL = 0,
    VIEW_STATUS_ACTIVE = 1,
    VIEW_STATUS_WARNING = 2
};

void view_init(HWND window, HINSTANCE instance, UINT dpi);
void view_done(void);
void view_paint(HWND window);
void view_draw(DRAWITEMSTRUCT *item);
void view_memory(const MC_STATUS *status);
void view_status(const wchar_t *text, enum mc_view_status kind);
void view_elevated(bool elevated);
void view_cleaning(bool cleaning);
void view_invalidate(HWND window);
void view_show(HWND window);
void view_refresh(HWND window, int id);
void view_about(void);
void view_result(bool manual, wchar_t *text, bool failed);
void view_tray_menu(HWND owner);
[[nodiscard]] int view_region_index(int id);
[[nodiscard]] unsigned view_region_bit(int index);

#endif
