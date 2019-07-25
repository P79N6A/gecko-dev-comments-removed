








#ifndef SkTypes_DEFINED
#define SkTypes_DEFINED

#include "SkPreConfig.h"
#include "SkUserConfig.h"
#include "SkPostConfig.h"

#ifndef SK_IGNORE_STDINT_DOT_H
    #include <stdint.h>
#endif

#include <stdio.h>






#define SKIA_VERSION_MAJOR  1
#define SKIA_VERSION_MINOR  0
#define SKIA_VERSION_PATCH  0








SK_API extern void sk_out_of_memory(void);




SK_API extern void sk_throw(void);

enum {
    SK_MALLOC_TEMP  = 0x01, 
    SK_MALLOC_THROW = 0x02  
};





SK_API extern void* sk_malloc_flags(size_t size, unsigned flags);


SK_API extern void* sk_malloc_throw(size_t size);



SK_API extern void* sk_realloc_throw(void* buffer, size_t size);


SK_API extern void sk_free(void*);


static inline void sk_bzero(void* buffer, size_t size) {
    memset(buffer, 0, size);
}



#ifdef SK_OVERRIDE_GLOBAL_NEW
#include <new>

inline void* operator new(size_t size) {
    return sk_malloc_throw(size);
}

inline void operator delete(void* p) {
    sk_free(p);
}
#endif



#define SK_INIT_TO_AVOID_WARNING    = 0

#ifndef SkDebugf
    void SkDebugf(const char format[], ...);
#endif

#ifdef SK_DEBUG
    #define SkASSERT(cond)              SK_DEBUGBREAK(cond)
    #define SkDEBUGCODE(code)           code
    #define SkDECLAREPARAM(type, var)   , type var
    #define SkPARAM(var)                , var

    #define SkDEBUGF(args       )       SkDebugf args
    #define SkAssertResult(cond)        SkASSERT(cond)
#else
    #define SkASSERT(cond)
    #define SkDEBUGCODE(code)
    #define SkDEBUGF(args)
    #define SkDECLAREPARAM(type, var)
    #define SkPARAM(var)


    #define SkAssertResult(cond)        cond
#endif

namespace {

template <bool>
struct SkCompileAssert {
};

}  

#define SK_COMPILE_ASSERT(expr, msg) \
    typedef SkCompileAssert<(bool(expr))> msg[bool(expr) ? 1 : -1]







typedef int S8CPU;





typedef unsigned U8CPU;





typedef int S16CPU;





typedef unsigned U16CPU;





typedef int SkBool;




typedef uint8_t SkBool8;

#ifdef SK_DEBUG
    SK_API int8_t      SkToS8(long);
    SK_API uint8_t     SkToU8(size_t);
    SK_API int16_t     SkToS16(long);
    SK_API uint16_t    SkToU16(size_t);
    SK_API int32_t     SkToS32(long);
    SK_API uint32_t    SkToU32(size_t);
#else
    #define SkToS8(x)   ((int8_t)(x))
    #define SkToU8(x)   ((uint8_t)(x))
    #define SkToS16(x)  ((int16_t)(x))
    #define SkToU16(x)  ((uint16_t)(x))
    #define SkToS32(x)  ((int32_t)(x))
    #define SkToU32(x)  ((uint32_t)(x))
#endif



#define SkToBool(cond)  ((cond) != 0)

#define SK_MaxS16   32767
#define SK_MinS16   -32767
#define SK_MaxU16   0xFFFF
#define SK_MinU16   0
#define SK_MaxS32   0x7FFFFFFF
#define SK_MinS32   0x80000001
#define SK_MaxU32   0xFFFFFFFF
#define SK_MinU32   0
#define SK_NaN32    0x80000000



static inline bool SkIsS16(long x) {
    return (int16_t)x == x;
}



static inline bool SkIsU16(long x) {
    return (uint16_t)x == x;
}


#ifndef SK_OFFSETOF
    #define SK_OFFSETOF(type, field)    ((char*)&(((type*)1)->field) - (char*)1)
#endif



#define SK_ARRAY_COUNT(array)       (sizeof(array) / sizeof(array[0]))



#define SkAlign2(x)     (((x) + 1) >> 1 << 1)


#define SkAlign4(x)     (((x) + 3) >> 2 << 2)

typedef uint32_t SkFourByteTag;
#define SkSetFourByteTag(a, b, c, d)    (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))



typedef int32_t SkUnichar;


typedef uint32_t SkMSec;


#define SK_MSec1 1000


#define SK_MSecMax 0x7FFFFFFF


#define SkMSec_LT(a, b)     ((int32_t)(a) - (int32_t)(b) < 0)


#define SkMSec_LE(a, b)     ((int32_t)(a) - (int32_t)(b) <= 0)




#ifdef __cplusplus



static inline int Sk32ToBool(uint32_t n) {
    return (n | (0-n)) >> 31;
}

template <typename T> inline void SkTSwap(T& a, T& b) {
    T c(a);
    a = b;
    b = c;
}

static inline int32_t SkAbs32(int32_t value) {
#ifdef SK_CPU_HAS_CONDITIONAL_INSTR
    if (value < 0)
        value = -value;
    return value;
#else
    int32_t mask = value >> 31;
    return (value ^ mask) - mask;
#endif
}

static inline int32_t SkMax32(int32_t a, int32_t b) {
    if (a < b)
        a = b;
    return a;
}

static inline int32_t SkMin32(int32_t a, int32_t b) {
    if (a > b)
        a = b;
    return a;
}

static inline int32_t SkSign32(int32_t a) {
    return (a >> 31) | ((unsigned) -a >> 31);
}

static inline int32_t SkFastMin32(int32_t value, int32_t max) {
#ifdef SK_CPU_HAS_CONDITIONAL_INSTR
    if (value > max)
        value = max;
    return value;
#else
    int diff = max - value;
    
    diff &= (diff >> 31);
    return value + diff;
#endif
}



static inline int32_t SkPin32(int32_t value, int32_t min, int32_t max) {
#ifdef SK_CPU_HAS_CONDITIONAL_INSTR
    if (value < min)
        value = min;
    if (value > max)
        value = max;
#else
    if (value < min)
        value = min;
    else if (value > max)
        value = max;
#endif
    return value;
}

static inline uint32_t SkSetClearShift(uint32_t bits, bool cond,
                                       unsigned shift) {
    SkASSERT((int)cond == 0 || (int)cond == 1);
    return (bits & ~(1 << shift)) | ((int)cond << shift);
}

static inline uint32_t SkSetClearMask(uint32_t bits, bool cond,
                                      uint32_t mask) {
    return cond ? bits | mask : bits & ~mask;
}





template <typename T>
T SkTBitOr(T a, T b) {
    return (T)(a | b);
}




template <typename Dst> Dst SkTCast(const void* ptr) {
    union {
        const void* src;
        Dst dst;
    } data;
    data.src = ptr;
    return data.dst;
}








class SK_API SkNoncopyable {
public:
    SkNoncopyable() {}

private:
    SkNoncopyable(const SkNoncopyable&);
    SkNoncopyable& operator=(const SkNoncopyable&);
};

class SkAutoFree : SkNoncopyable {
public:
    SkAutoFree() : fPtr(NULL) {}
    explicit SkAutoFree(void* ptr) : fPtr(ptr) {}
    ~SkAutoFree() { sk_free(fPtr); }

    

    void* get() const { return fPtr; }

    



    void* set(void* ptr) {
        void* prev = fPtr;
        fPtr = ptr;
        return prev;
    }

    



    void* detach() { return this->set(NULL); }

    


    void free() {
        sk_free(fPtr);
        fPtr = NULL;
    }

private:
    void* fPtr;
    
    SkAutoFree(const SkAutoFree&);
    SkAutoFree& operator=(const SkAutoFree&);
};






class SkAutoMalloc : public SkNoncopyable {
public:
    explicit SkAutoMalloc(size_t size = 0) {
        fPtr = size ? sk_malloc_throw(size) : NULL;
        fSize = size;
    }

    ~SkAutoMalloc() {
        sk_free(fPtr);
    }

    



    enum OnShrink {
        




        kAlloc_OnShrink,
        
        




        kReuse_OnShrink,
    };

    


    void* reset(size_t size, OnShrink shrink = kAlloc_OnShrink) {
        if (size == fSize || (kReuse_OnShrink == shrink && size < fSize)) {
            return fPtr;
        }

        sk_free(fPtr);
        fPtr = size ? sk_malloc_throw(size) : NULL;
        fSize = size;

        return fPtr;
    }

    


    void free() {
        this->reset(0);
    }

    


    void* get() { return fPtr; }
    const void* get() const { return fPtr; }

   



    void* detach() {
        void* ptr = fPtr;
        fPtr = NULL;
        fSize = 0;
        return ptr;
    }

private:
    void*   fPtr;
    size_t  fSize;  
};







template <size_t kSize> class SkAutoSMalloc : SkNoncopyable {
public:
    




    SkAutoSMalloc() {
        fPtr = fStorage;
        fSize = 0;
    }

    




    explicit SkAutoSMalloc(size_t size) {
        fPtr = fStorage;
        fSize = 0;
        this->reset(size);
    }

    



    ~SkAutoSMalloc() {
        if (fPtr != (void*)fStorage) {
            sk_free(fPtr);
        }
    }

    





    void* get() const { return fPtr; }

    





    void* reset(size_t size,
                SkAutoMalloc::OnShrink shrink = SkAutoMalloc::kAlloc_OnShrink) {
        if (size == fSize || (SkAutoMalloc::kReuse_OnShrink == shrink &&
                              size < fSize)) {
            return fPtr;
        }

        if (fPtr != (void*)fStorage) {
            sk_free(fPtr);
        }

        if (size <= kSize) {
            fPtr = fStorage;
        } else {
            fPtr = sk_malloc_flags(size, SK_MALLOC_THROW | SK_MALLOC_TEMP);
        }
        return fPtr;
    }

private:
    void*       fPtr;
    size_t      fSize;  
    uint32_t    fStorage[(kSize + 3) >> 2];
};

#endif 

#endif
