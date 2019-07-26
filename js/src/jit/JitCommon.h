





#ifndef jit_JitCommon_h
#define jit_JitCommon_h



#if defined(JS_ARM_SIMULATOR)
#include "jit/arm/Simulator-arm.h"
#elif defined(JS_MIPS_SIMULATOR)
#include "jit/mips/Simulator-mips.h"
#endif

#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)

#define CALL_GENERATED_CODE(entry, p0, p1, p2, p3, p4, p5, p6, p7)     \
    (js::jit::Simulator::Current()->call(                              \
        JS_FUNC_TO_DATA_PTR(uint8_t *, entry), 8, p0, p1, p2, p3, p4, p5, p6, p7) & 0xffffffff)

#ifdef JS_YARR

#define CALL_GENERATED_YARR_CODE3(entry, p0, p1, p2)     \
    js::jit::Simulator::Current()->call(JS_FUNC_TO_DATA_PTR(uint8_t *, entry), 3, p0, p1, p2)

#define CALL_GENERATED_YARR_CODE4(entry, p0, p1, p2, p3) \
    js::jit::Simulator::Current()->call(JS_FUNC_TO_DATA_PTR(uint8_t *, entry), 4, p0, p1, p2, p3)

#else 

#define CALL_GENERATED_REGEXP(entry, p0)                 \
    js::jit::Simulator::Current()->call(JS_FUNC_TO_DATA_PTR(uint8_t *, entry), 1, p0)

#endif 

#define CALL_GENERATED_ASMJS(entry, p0, p1)              \
    (Simulator::Current()->call(JS_FUNC_TO_DATA_PTR(uint8_t *, entry), 2, p0, p1) & 0xffffffff)

#else


#define CALL_GENERATED_CODE(entry, p0, p1, p2, p3, p4, p5, p6, p7)   \
  entry(p0, p1, p2, p3, p4, p5, p6, p7)

#ifdef JS_YARR

#define CALL_GENERATED_YARR_CODE3(entry, p0, p1, p2)                 \
  entry(p0, p1, p2)

#define CALL_GENERATED_YARR_CODE4(entry, p0, p1, p2, p3)             \
  entry(p0, p1, p2, p3)

#else 

#define CALL_GENERATED_REGEXP(entry, p0)                             \
  entry(p0)

#endif 

#define CALL_GENERATED_ASMJS(entry, p0, p1)                          \
  entry(p0, p1)

#endif

#endif 
