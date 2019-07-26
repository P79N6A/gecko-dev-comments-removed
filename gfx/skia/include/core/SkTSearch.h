








#ifndef SkTSearch_DEFINED
#define SkTSearch_DEFINED

#include "SkTypes.h"



















template <typename T>
int SkTSearch(const T* base, int count, const T& target, size_t elemSize)
{
    SkASSERT(count >= 0);
    if (count <= 0)
        return ~0;

    SkASSERT(base != NULL); 

    int lo = 0;
    int hi = count - 1;

    while (lo < hi)
    {
        int mid = (hi + lo) >> 1;
        const T* elem = (const T*)((const char*)base + mid * elemSize);

        if (*elem < target)
            lo = mid + 1;
        else
            hi = mid;
    }

    const T* elem = (const T*)((const char*)base + hi * elemSize);
    if (*elem != target)
    {
        if (*elem < target)
            hi += 1;
        hi = ~hi;
    }
    return hi;
}

template <typename T, int (COMPARE)(const T*, const T*)>
int SkTSearch(const T* base, int count, const T& target, size_t elemSize)
{
    SkASSERT(count >= 0);
    if (count <= 0) {
        return ~0;
    }

    SkASSERT(base != NULL); 

    int lo = 0;
    int hi = count - 1;

    while (lo < hi) {
        int mid = (hi + lo) >> 1;
        const T* elem = (const T*)((const char*)base + mid * elemSize);

        if (COMPARE(elem, &target) < 0)
            lo = mid + 1;
        else
            hi = mid;
    }

    const T* elem = (const T*)((const char*)base + hi * elemSize);
    int pred = COMPARE(elem, &target);
    if (pred != 0) {
        if (pred < 0)
            hi += 1;
        hi = ~hi;
    }
    return hi;
}

template <typename T>
int SkTSearch(const T* base, int count, const T& target, size_t elemSize,
              int (*compare)(const T*, const T*))
{
    SkASSERT(count >= 0);
    if (count <= 0) {
        return ~0;
    }

    SkASSERT(base != NULL); 

    int lo = 0;
    int hi = count - 1;

    while (lo < hi) {
        int mid = (hi + lo) >> 1;
        const T* elem = (const T*)((const char*)base + mid * elemSize);

        if ((*compare)(elem, &target) < 0)
            lo = mid + 1;
        else
            hi = mid;
    }

    const T* elem = (const T*)((const char*)base + hi * elemSize);
    int pred = (*compare)(elem, &target);
    if (pred != 0) {
        if (pred < 0)
            hi += 1;
        hi = ~hi;
    }
    return hi;
}

template <typename T>
int SkTSearch(const T** base, int count, const T* target, size_t elemSize,
    int (*compare)(const T*, const T*))
{
    SkASSERT(count >= 0);
    if (count <= 0)
        return ~0;

    SkASSERT(base != NULL); 

    int lo = 0;
    int hi = count - 1;

    while (lo < hi)
    {
        int mid = (hi + lo) >> 1;
        const T* elem = *(const T**)((const char*)base + mid * elemSize);

        if ((*compare)(elem, target) < 0)
            lo = mid + 1;
        else
            hi = mid;
    }

    const T* elem = *(const T**)((const char*)base + hi * elemSize);
    int pred = (*compare)(elem, target);
    if (pred != 0)
    {
        if (pred < 0)
            hi += 1;
        hi = ~hi;
    }
    return hi;
}

template <typename T,  int (COMPARE)(const T*, const T*)>
int SkTSearch(const T** base, int count, const T* target, size_t elemSize)
{
    SkASSERT(count >= 0);
    if (count <= 0)
        return ~0;

    SkASSERT(base != NULL); 

    int lo = 0;
    int hi = count - 1;

    while (lo < hi)
    {
        int mid = (hi + lo) >> 1;
        const T* elem = *(const T**)((const char*)base + mid * elemSize);

        if (COMPARE(elem, target) < 0)
            lo = mid + 1;
        else
            hi = mid;
    }

    const T* elem = *(const T**)((const char*)base + hi * elemSize);
    int pred = COMPARE(elem, target);
    if (pred != 0)
    {
        if (pred < 0)
            hi += 1;
        hi = ~hi;
    }
    return hi;
}
int SkStrSearch(const char*const* base, int count, const char target[],
                size_t target_len, size_t elemSize);
int SkStrSearch(const char*const* base, int count, const char target[],
                size_t elemSize);




int SkStrLCSearch(const char*const* base, int count, const char target[],
                  size_t target_len, size_t elemSize);
int SkStrLCSearch(const char*const* base, int count, const char target[],
                  size_t elemSize);






class SkAutoAsciiToLC {
public:
    SkAutoAsciiToLC(const char str[], size_t len = (size_t)-1);
    ~SkAutoAsciiToLC();

    const char* lc() const { return fLC; }
    size_t      length() const { return fLength; }

private:
    char*   fLC;    
    size_t  fLength;
    enum {
        STORAGE = 64
    };
    char    fStorage[STORAGE+1];
};


#define SkCastForQSort(compare) reinterpret_cast<int (*)(const void*, const void*)>(compare)

#endif

