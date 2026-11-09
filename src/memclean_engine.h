#ifndef MEMCLEAN_ENGINE_H
#define MEMCLEAN_ENGINE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include "memclean_core.h"

typedef LONG NTSTATUS;

typedef struct MC_FILECACHE_INFO
{
    SIZE_T CurrentSize;
    SIZE_T PeakSize;
    ULONG PageFaultCount;
    SIZE_T MinimumWorkingSet;
    SIZE_T MaximumWorkingSet;
    SIZE_T CurrentSizeIncludingTransitionInPages;
    SIZE_T PeakSizeIncludingTransitionInPages;
    ULONG TransitionRePurposeCount;
    ULONG Flags;
} MC_FILECACHE_INFO;

typedef struct MC_MEMORY_LIST_INFO
{
    SIZE_T ZeroPageCount;
    SIZE_T FreePageCount;
    SIZE_T ModifiedPageCount;
    SIZE_T ModifiedNoWritePageCount;
    SIZE_T BadPageCount;
    SIZE_T PageCountByPriority[8];
    SIZE_T RepurposedPagesByPriority[8];
    SIZE_T ModifiedPageCountPageFile;
} MC_MEMORY_LIST_INFO;

typedef struct MC_COMBINE_INFO
{
    HANDLE Handle;
    SIZE_T PagesCombined;
    ULONG Flags;
} MC_COMBINE_INFO;

enum mc_class : ULONG
{
    MC_CLASS_MEMORY_LIST = 80,
    MC_CLASS_FILECACHE_EX = 81,
    MC_CLASS_COMBINE = 130,
    MC_CLASS_REGISTRY = 155
};

enum mc_command : ULONG
{
    MC_CMD_EMPTY_WORKINGSETS = 2,
    MC_CMD_FLUSH_MODIFIED = 3,
    MC_CMD_PURGE_STANDBY = 4,
    MC_CMD_PURGE_LOW_STANDBY = 5
};

typedef struct MC_STATUS
{
    bool memory_ok;
    uint64_t total;
    uint64_t available;
    uint64_t used;
    DWORD load;
    uint64_t standby;
    uint64_t standby_low;
    uint64_t modified;
    uint64_t cache;
    NTSTATUS list_status;
    NTSTATUS cache_status;
    bool elevated;
} MC_STATUS;

typedef struct MC_CLEAN_RESULT
{
    unsigned mask;
    bool elevated;
    bool profile_enabled;
    bool quota_enabled;
    NTSTATUS working;
    NTSTATUS filecache;
    NTSTATUS modified;
    NTSTATUS standby;
    NTSTATUS standby_low;
    NTSTATUS registry;
    NTSTATUS combine;
    SIZE_T combined_pages;
    uint64_t available_before;
    uint64_t available_after;
    bool have_before;
    bool have_after;
} MC_CLEAN_RESULT;

[[nodiscard]] bool mc_init(void);
[[nodiscard]] bool mc_is_elevated(void);
void mc_get_status(MC_STATUS *status);
[[nodiscard]] int mc_clean(unsigned mask, bool force, MC_CLEAN_RESULT *result);

#endif
