#ifndef MEMCLEAN_HOST_H
#define MEMCLEAN_HOST_H

#include <windows.h>
#include "memclean_core.h"
#include "memclean_engine.h"

enum
{
    MC_WM_TRAY = WM_APP + 1,
    MC_WM_CLEAN_DONE = WM_APP + 2
};

enum mc_clean_launch
{
    MC_CLEAN_OK = 0,
    MC_CLEAN_CANCELED = 1,
    MC_CLEAN_ERROR = 2,
    MC_CLEAN_TEMP = 3
};

enum mc_auto_result
{
    MC_AUTO_TOGGLED = 0,
    MC_AUTO_RELAUNCH = 1,
    MC_AUTO_CANCELED = 2,
    MC_AUTO_NEEDS_ADMIN = 3
};

extern MC_SETTINGS mc_cfg;

void mc_cfg_load(void);
void mc_cfg_save(void);
[[nodiscard]] bool mc_startup_get(void);
[[nodiscard]] bool mc_startup_set(bool enabled);

[[nodiscard]] bool mc_instance_ok(void);
void mc_mutex_release(void);
void mc_mutex_restore(void);

void mc_tray_add(HWND owner, HICON icon);
void mc_tray_tip(DWORD load);
void mc_tray_balloon(const wchar_t *title, const wchar_t *text, DWORD flags);
void mc_tray_remove(void);

[[nodiscard]] enum mc_clean_launch mc_clean_start(HWND owner, unsigned mask, bool manual);
[[nodiscard]] bool mc_elevate(HWND owner);
void mc_worker_main(int argc, wchar_t **argv);
[[nodiscard]] enum mc_auto_result mc_toggle_auto(HWND owner);
[[nodiscard]] bool mc_toggle_startup(void);
void mc_adjust_percent(int delta);

#endif
