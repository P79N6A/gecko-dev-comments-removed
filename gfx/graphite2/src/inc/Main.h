

























#pragma once

#include <cstdlib>
#include "graphite2/Types.h"

#ifdef GRAPHITE2_CUSTOM_HEADER
#include GRAPHITE2_CUSTOM_HEADER
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

#if GRAPHITE2_TELEMETRY
struct telemetry
{
    class category;

    static size_t   * _category;
    static void set_category(size_t & t) throw()    { _category = &t; }
    static void stop() throw()                      { _category = 0; }
    static void count_bytes(size_t n) throw()       { if (_category) *_category += n; }

    size_t  misc,
            silf,
            glyph,
            code,
            states,
            starts,
            transitions;

    telemetry() : misc(0), silf(0), glyph(0), code(0), states(0), starts(0), transitions(0) {}
};

class telemetry::category
{
    size_t * _prev;
public:
    category(size_t & t) : _prev(_category) { _category = &t; }
    ~category() { _category = _prev; }
};

#else
struct telemetry  {};
#endif




template <typename T> T * gralloc(size_t n)
{
#if GRAPHITE2_TELEMETRY
    telemetry::count_bytes(sizeof(T) * n);
#endif
    return reinterpret_cast<T*>(malloc(sizeof(T) * n));
}

template <typename T> T * grzeroalloc(size_t n)
{
#if GRAPHITE2_TELEMETRY
    telemetry::count_bytes(sizeof(T) * n);
#endif
    return reinterpret_cast<T*>(calloc(n, sizeof(T)));
}

template <typename T>
inline T min(const T a, const T b)
{
    return a < b ? a : b;
}

template <typename T>
inline T max(const T a, const T b)
{
    return a > b ? a : b;
}

} 

#define CLASS_NEW_DELETE \
    void * operator new   (size_t size){ return gralloc<byte>(size);} \
    void * operator new   (size_t, void * p) throw() { return p; } \
    void * operator new[] (size_t size) {return gralloc<byte>(size);} \
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
