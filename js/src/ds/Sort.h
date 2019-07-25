






































#ifndef Sort_h__
#define Sort_h__

#include "jstypes.h"

namespace js {

namespace detail {

template<typename T>
JS_ALWAYS_INLINE void
CopyNonEmptyArray(T *dst, const T *src, size_t nelems)
{
    JS_ASSERT(nelems != 0);
    const T *end = src + nelems;
    do {
        *dst++ = *src++;
    } while (src != end);
}


template<typename T, typename Comparator>
JS_ALWAYS_INLINE bool
MergeArrayRuns(T *dst, const T *src, size_t run1, size_t run2, Comparator c)
{
    JS_ASSERT(run1 >= 1);
    JS_ASSERT(run2 >= 1);

    
    const T *b = src + run1;
    bool lessOrEqual;
    if (!c(b[-1],  b[0], &lessOrEqual))
        return false;

    if (!lessOrEqual) {
        
        for (const T *a = src;;) {
            if (!c(*a, *b, &lessOrEqual))
                return false;
            if (lessOrEqual) {
                *dst++ = *a++;
                if (!--run1) {
                    src = b;
                    break;
                }
            } else {
                *dst++ = *b++;
                if (!--run2) {
                    src = a;
                    break;
                }
            }
        }
    }
    CopyNonEmptyArray(dst, src, run1 + run2);
    return true;
}

} 














template<typename T, typename Comparator>
bool
MergeSort(T *array, size_t nelems, T *scratch, Comparator c)
{
    const size_t INS_SORT_LIMIT = 3;

    if (nelems <= 1)
        return true;

    



    for (size_t lo = 0; lo < nelems; lo += INS_SORT_LIMIT) {
        size_t hi = lo + INS_SORT_LIMIT;
        if (hi >= nelems)
            hi = nelems;
        for (size_t i = lo + 1; i != hi; i++) {
            for (size_t j = i; ;) {
                bool lessOrEqual;
                if (!c(array[j - 1], array[j], &lessOrEqual))
                    return false;
                if (lessOrEqual)
                    break;
                T tmp = array[j - 1];
                array[j - 1] = array[j];
                array[j] = tmp;
                if (--j == lo)
                    break;
            }
        }
    }

    T *vec1 = array;
    T *vec2 = scratch;
    for (size_t run = INS_SORT_LIMIT; run < nelems; run *= 2) {
        for (size_t lo = 0; lo < nelems; lo += 2 * run) {
            size_t hi = lo + run;
            if (hi >= nelems) {
                detail::CopyNonEmptyArray(vec2 + lo, vec1 + lo, nelems - lo);
                break;
            }
            size_t run2 = (run <= nelems - hi) ? run : nelems - hi;
            if (!detail::MergeArrayRuns(vec2 + lo, vec1 + lo, run, run2, c))
                return false;
        }
        T *swap = vec1;
        vec1 = vec2;
        vec2 = swap;
    }
    if (vec1 == scratch)
        detail::CopyNonEmptyArray(array, scratch, nelems);
    return true;
}

} 

#endif
