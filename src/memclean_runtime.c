#include <stddef.h>
#include <stdint.h>

void *memset(void *dst, int value, size_t count)
{
    unsigned char *p = (unsigned char *)dst;
    while (count--)
        *p++ = (unsigned char)value;
    return dst;
}

void *memcpy(void *restrict dst, const void *restrict src, size_t count)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (count--)
        *d++ = *s++;
    return dst;
}

void *memmove(void *dst, const void *src, size_t count)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;

    if ((uintptr_t)d < (uintptr_t)s)
    {
        while (count--)
            *d++ = *s++;
    }
    else
    {
        d += count;
        s += count;
        while (count--)
            *--d = *--s;
    }

    return dst;
}
