#include <stddef.h>
#include <stdint.h>
#include <emmintrin.h>

static constexpr size_t MC_VECTOR_BYTES = 16;
static constexpr size_t MC_UNROLLED_BYTES = 64;
static constexpr size_t MC_REP_MIN = 1024;

typedef uint64_t unaligned_u64 __attribute__((aligned(1), may_alias));
typedef uint32_t unaligned_u32 __attribute__((aligned(1), may_alias));
typedef uint16_t unaligned_u16 __attribute__((aligned(1), may_alias));

static inline void copy_small(unsigned char *dst, const unsigned char *src, size_t count)
{
    if (count >= 8)
    {
        uint64_t first = *(const unaligned_u64 *)src;
        uint64_t last = count > 8 ? *(const unaligned_u64 *)(src + count - 8) : first;
        *(unaligned_u64 *)dst = first;
        if (count > 8)
            *(unaligned_u64 *)(dst + count - 8) = last;
    }
    else if (count >= 4)
    {
        uint32_t first = *(const unaligned_u32 *)src;
        uint32_t last = count > 4 ? *(const unaligned_u32 *)(src + count - 4) : first;
        *(unaligned_u32 *)dst = first;
        if (count > 4)
            *(unaligned_u32 *)(dst + count - 4) = last;
    }
    else if (count >= 2)
    {
        uint16_t first = *(const unaligned_u16 *)src;
        uint16_t last = count > 2 ? *(const unaligned_u16 *)(src + count - 2) : first;
        *(unaligned_u16 *)dst = first;
        if (count > 2)
            *(unaligned_u16 *)(dst + count - 2) = last;
    }
    else if (count)
    {
        *dst = *src;
    }
}

static inline void fill_small(unsigned char *dst, unsigned char value, size_t count)
{
    uint64_t word = (uint64_t)value * 0x0101010101010101ull;

    if (count >= 8)
    {
        *(unaligned_u64 *)dst = word;
        if (count > 8)
            *(unaligned_u64 *)(dst + count - 8) = word;
    }
    else if (count >= 4)
    {
        *(unaligned_u32 *)dst = (uint32_t)word;
        if (count > 4)
            *(unaligned_u32 *)(dst + count - 4) = (uint32_t)word;
    }
    else if (count >= 2)
    {
        *(unaligned_u16 *)dst = (uint16_t)word;
        if (count > 2)
            *(unaligned_u16 *)(dst + count - 2) = (uint16_t)word;
    }
    else if (count)
    {
        *dst = value;
    }
}

static inline void copy_vector(unsigned char *dst, const unsigned char *src)
{
    _mm_storeu_si128((__m128i *)dst, _mm_loadu_si128((const __m128i *)src));
}

static inline void fill_vector(unsigned char *dst, __m128i pattern)
{
    _mm_storeu_si128((__m128i *)dst, pattern);
}

static inline void copy_rep(unsigned char *dst, const unsigned char *src, size_t count)
{
    __asm__ volatile("rep movsb" : "+D"(dst), "+S"(src), "+c"(count) : : "memory");
}

static inline void fill_rep(unsigned char *dst, int value, size_t count)
{
    __asm__ volatile("rep stosb" : "+D"(dst), "+c"(count) : "a"(value) : "memory");
}

void *memset(void *dst, int value, size_t count)
{
    unsigned char *d = (unsigned char *)dst;
    unsigned char v = (unsigned char)value;

    if (count == 0)
        return dst;
    if (count == 1)
    {
        *d = v;
        return dst;
    }

    if (count < MC_VECTOR_BYTES)
    {
        fill_small(d, v, count);
        return dst;
    }

    if (count >= MC_REP_MIN)
    {
        fill_rep(d, value, count);
        return dst;
    }

    __m128i pattern = _mm_set1_epi8((char)v);
    while (count >= MC_UNROLLED_BYTES)
    {
        fill_vector(d + 0, pattern);
        fill_vector(d + 16, pattern);
        fill_vector(d + 32, pattern);
        fill_vector(d + 48, pattern);
        d += MC_UNROLLED_BYTES;
        count -= MC_UNROLLED_BYTES;
    }
    while (count >= MC_VECTOR_BYTES)
    {
        fill_vector(d, pattern);
        d += MC_VECTOR_BYTES;
        count -= MC_VECTOR_BYTES;
    }
    fill_small(d, v, count);
    return dst;
}

static void *copy_forward(unsigned char *d, const unsigned char *s, size_t count)
{
    unsigned char *start = d;

    if (count == 0)
        return start;
    if (count == 1)
    {
        *d = *s;
        return start;
    }

    if (count < MC_VECTOR_BYTES)
    {
        copy_small(d, s, count);
        return start;
    }

    if (count >= MC_REP_MIN)
    {
        copy_rep(d, s, count);
        return start;
    }

    while (count >= MC_UNROLLED_BYTES)
    {
        __m128i a = _mm_loadu_si128((const __m128i *)(s + 0));
        __m128i b = _mm_loadu_si128((const __m128i *)(s + 16));
        __m128i c = _mm_loadu_si128((const __m128i *)(s + 32));
        __m128i e = _mm_loadu_si128((const __m128i *)(s + 48));
        _mm_storeu_si128((__m128i *)(d + 0), a);
        _mm_storeu_si128((__m128i *)(d + 16), b);
        _mm_storeu_si128((__m128i *)(d + 32), c);
        _mm_storeu_si128((__m128i *)(d + 48), e);
        d += MC_UNROLLED_BYTES;
        s += MC_UNROLLED_BYTES;
        count -= MC_UNROLLED_BYTES;
    }
    while (count >= MC_VECTOR_BYTES)
    {
        copy_vector(d, s);
        d += MC_VECTOR_BYTES;
        s += MC_VECTOR_BYTES;
        count -= MC_VECTOR_BYTES;
    }
    copy_small(d, s, count);
    return start;
}

void *memcpy(void *restrict dst, const void *restrict src, size_t count)
{
    return copy_forward((unsigned char *)dst, (const unsigned char *)src, count);
}

void *memmove(void *dst, const void *src, size_t count)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    uintptr_t di = (uintptr_t)d;
    uintptr_t si = (uintptr_t)s;

    if (di == si || count == 0)
        return dst;
    if (di < si || di - si >= count)
        return copy_forward(d, s, count);

    d += count;
    s += count;
    while (count >= MC_VECTOR_BYTES)
    {
        d -= MC_VECTOR_BYTES;
        s -= MC_VECTOR_BYTES;
        copy_vector(d, s);
        count -= MC_VECTOR_BYTES;
    }
    while (count)
    {
        *--d = *--s;
        count--;
    }
    return dst;
}
