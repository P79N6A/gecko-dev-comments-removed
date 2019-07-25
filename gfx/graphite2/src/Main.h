

























#pragma once

#include <cstdlib>
#include "graphite2/Types.h"

#ifdef GR2_CUSTOM_HEADER
#include GR2_CUSTOM_HEADER
#endif

namespace graphite2 {

typedef gr_uint8        uint8;
typedef gr_uint8        byte;
typedef gr_uint16       uint16;
typedef gr_uint32       uint32;
typedef gr_int8         int8;
typedef gr_int16        int16;
typedef gr_int32        int32;
typedef size_t          uintptr;



template <typename T> T * gralloc(size_t n)
{
    return reinterpret_cast<T*>(malloc(sizeof(T) * n));
}

template <typename T> T * grzeroalloc(size_t n)
{
    return reinterpret_cast<T*>(calloc(n, sizeof(T)));
}

} 

#define CLASS_NEW_DELETE \
    void * operator new   (size_t size){ return malloc(size);} \
    void * operator new   (size_t, void * p) throw() { return p; } \
    void * operator new[] (size_t size) {return malloc(size);} \
    void * operator new[] (size_t, void * p) throw() { return p; } \
    void operator delete   (void * p) throw() { free(p);} \
    void operator delete   (void *, void *) throw() {} \
    void operator delete[] (void * p)throw() { free(p); } \
    void operator delete[] (void *, void *) throw() {}

#ifdef __GNUC__
#define GR_MAYBE_UNUSED __attribute__((unused))
#else
#define GR_MAYBE_UNUSED
#endif
