





































#ifndef jsbit_h___
#define jsbit_h___

#include "jstypes.h"
#include "jsutil.h"

JS_BEGIN_EXTERN_C




typedef JSUword     jsbitmap_t;     
typedef jsbitmap_t  jsbitmap;       

#define JS_TEST_BIT(_map,_bit) \
    ((_map)[(_bit)>>JS_BITS_PER_WORD_LOG2] & (1L << ((_bit) & (JS_BITS_PER_WORD-1))))
#define JS_SET_BIT(_map,_bit) \
    ((_map)[(_bit)>>JS_BITS_PER_WORD_LOG2] |= (1L << ((_bit) & (JS_BITS_PER_WORD-1))))
#define JS_CLEAR_BIT(_map,_bit) \
    ((_map)[(_bit)>>JS_BITS_PER_WORD_LOG2] &= ~(1L << ((_bit) & (JS_BITS_PER_WORD-1))))




extern JS_PUBLIC_API(JSIntn) JS_CeilingLog2(JSUint32 i);




extern JS_PUBLIC_API(JSIntn) JS_FloorLog2(JSUint32 i);









#if defined(_WIN32) && (_MSC_VER >= 1300) && defined(_M_IX86)

unsigned char _BitScanForward(unsigned long * Index, unsigned long Mask);
unsigned char _BitScanReverse(unsigned long * Index, unsigned long Mask);
# pragma intrinsic(_BitScanForward,_BitScanReverse)

__forceinline static int
__BitScanForward32(unsigned int val)
{
   unsigned long idx;

   _BitScanForward(&idx, (unsigned long)val);
   return (int)idx;
}
__forceinline static int
__BitScanReverse32(unsigned int val)
{
   unsigned long idx;

   _BitScanReverse(&idx, (unsigned long)val);
   return (int)(31-idx);
}
# define js_bitscan_ctz32(val)  __BitScanForward32(val)
# define js_bitscan_clz32(val)  __BitScanReverse32(val)
# define JS_HAS_BUILTIN_BITSCAN32

#elif (__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)

# define js_bitscan_ctz32(val)  __builtin_ctz(val)
# define js_bitscan_clz32(val)  __builtin_clz(val)
# define JS_HAS_BUILTIN_BITSCAN32
# if (JS_BYTES_PER_WORD == 8)
#  define js_bitscan_ctz64(val)  __builtin_ctzll(val)
#  define js_bitscan_clz64(val)  __builtin_clzll(val)
#  define JS_HAS_BUILTIN_BITSCAN64
# endif

#endif





#ifdef JS_HAS_BUILTIN_BITSCAN32





# define JS_CEILING_LOG2(_log2,_n)                                            \
    JS_BEGIN_MACRO                                                            \
        JS_STATIC_ASSERT(sizeof(unsigned int) == sizeof(JSUint32));           \
        unsigned int j_ = (unsigned int)(_n);                                 \
        (_log2) = (j_ <= 1 ? 0 : 32 - js_bitscan_clz32(j_ - 1));              \
    JS_END_MACRO
#else
# define JS_CEILING_LOG2(_log2,_n)                                            \
    JS_BEGIN_MACRO                                                            \
        JSUint32 j_ = (JSUint32)(_n);                                         \
        (_log2) = 0;                                                          \
        if ((j_) & ((j_)-1))                                                  \
            (_log2) += 1;                                                     \
        if ((j_) >> 16)                                                       \
            (_log2) += 16, (j_) >>= 16;                                       \
        if ((j_) >> 8)                                                        \
            (_log2) += 8, (j_) >>= 8;                                         \
        if ((j_) >> 4)                                                        \
            (_log2) += 4, (j_) >>= 4;                                         \
        if ((j_) >> 2)                                                        \
            (_log2) += 2, (j_) >>= 2;                                         \
        if ((j_) >> 1)                                                        \
            (_log2) += 1;                                                     \
    JS_END_MACRO
#endif







#ifdef JS_HAS_BUILTIN_BITSCAN32





# define JS_FLOOR_LOG2(_log2,_n)                                              \
    JS_BEGIN_MACRO                                                            \
        JS_STATIC_ASSERT(sizeof(unsigned int) == sizeof(JSUint32));           \
        (_log2) = 31 - js_bitscan_clz32(((unsigned int)(_n)) | 1);            \
    JS_END_MACRO
#else
# define JS_FLOOR_LOG2(_log2,_n)                                              \
    JS_BEGIN_MACRO                                                            \
        JSUint32 j_ = (JSUint32)(_n);                                         \
        (_log2) = 0;                                                          \
        if ((j_) >> 16)                                                       \
            (_log2) += 16, (j_) >>= 16;                                       \
        if ((j_) >> 8)                                                        \
            (_log2) += 8, (j_) >>= 8;                                         \
        if ((j_) >> 4)                                                        \
            (_log2) += 4, (j_) >>= 4;                                         \
        if ((j_) >> 2)                                                        \
            (_log2) += 2, (j_) >>= 2;                                         \
        if ((j_) >> 1)                                                        \
            (_log2) += 1;                                                     \
    JS_END_MACRO
#endif







#define JS_CEILING_LOG2W(n) ((n) <= 1 ? 0 : 1 + JS_FLOOR_LOG2W((n) - 1))







#define JS_FLOOR_LOG2W(n) (JS_ASSERT((n) != 0), js_FloorLog2wImpl(n))

#if JS_BYTES_PER_WORD == 4

# ifdef JS_HAS_BUILTIN_BITSCAN32
JS_STATIC_ASSERT(sizeof(unsigned) == sizeof(JSUword));
#  define js_FloorLog2wImpl(n)                                                \
   ((JSUword)(JS_BITS_PER_WORD - 1 - js_bitscan_clz32(n)))
# else
#  define js_FloorLog2wImpl(n) ((JSUword)JS_FloorLog2(n))
#endif

#elif JS_BYTES_PER_WORD == 8

# ifdef JS_HAS_BUILTIN_BITSCAN64
JS_STATIC_ASSERT(sizeof(unsigned long long) == sizeof(JSUword));
#  define js_FloorLog2wImpl(n)                                                \
   ((JSUword)(JS_BITS_PER_WORD - 1 - js_bitscan_clz64(n)))
# else
extern JSUword js_FloorLog2wImpl(JSUword n);
# endif

#else

# error "NOT SUPPORTED"

#endif


JS_END_EXTERN_C
#endif 
