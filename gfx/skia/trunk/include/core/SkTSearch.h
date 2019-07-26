








#ifndef SkTSearch_DEFINED
#define SkTSearch_DEFINED

#include "SkTypes.h"
























template <typename T, typename K, typename LESS>
int SkTSearch(const T base[], int count, const K& key, size_t elemSize, LESS& less)
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

        if (less(*elem, key))
            lo = mid + 1;
        else
            hi = mid;
    }

    const T* elem = (const T*)((const char*)base + hi * elemSize);
    if (less(*elem, key)) {
        hi += 1;
        hi = ~hi;
    } else if (less(key, *elem)) {
        hi = ~hi;
    }
    return hi;
}


template <typename T, bool (LESS)(const T&, const T&)> struct SkTLessFunctionToFunctorAdaptor {
    bool operator()(const T& a, const T& b) { return LESS(a, b); }
};


template <typename T, bool (LESS)(const T&, const T&)>
int SkTSearch(const T base[], int count, const T& target, size_t elemSize) {
    static SkTLessFunctionToFunctorAdaptor<T, LESS> functor;
    return SkTSearch(base, count, target, elemSize, functor);
}


template <typename T> struct SkTLessFunctor {
    bool operator()(const T& a, const T& b) { return a < b; }
};


template <typename T>
int SkTSearch(const T base[], int count, const T& target, size_t elemSize) {
    static SkTLessFunctor<T> functor;
    return SkTSearch(base, count, target, elemSize, functor);
}


template <typename T, bool (LESS)(const T&, const T&)> struct SkTLessFunctionToPtrFunctorAdaptor {
    bool operator() (const T* t, const T* k) { return LESS(*t, *k); }
};



template <typename T, bool (LESS)(const T&, const T&)>
int SkTSearch(T* base[], int count, T* target, size_t elemSize) {
    static SkTLessFunctionToPtrFunctorAdaptor<T, LESS> functor;
    return SkTSearch(base, count, target, elemSize, functor);
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
