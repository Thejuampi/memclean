#ifndef MEMCLEAN_REPORT_H
#define MEMCLEAN_REPORT_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "memclean_engine.h"

void mc_out_text(HANDLE out, const char *text);
void mc_out_u64(HANDLE out, uint64_t value);
void mc_out_size(HANDLE out, uint64_t bytes);
void mc_out_nt(HANDLE out, NTSTATUS status);
void mc_report_status(HANDLE out, const MC_STATUS *status);
void mc_report_clean(HANDLE out, const MC_CLEAN_RESULT *result);

#endif
