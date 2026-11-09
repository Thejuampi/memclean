#ifndef MEMCLEAN_COMMON_H
#define MEMCLEAN_COMMON_H

#include <stddef.h>

enum
{
    MC_KIB = 1024,
    MC_MIB = 1024 * 1024,
    MC_GIB = 1024 * 1024 * 1024
};

#define MC_ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

#endif
