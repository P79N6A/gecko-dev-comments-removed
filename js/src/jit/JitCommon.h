





#ifndef jit_JitCommon_h
#define jit_JitCommon_h



#if defined(JS_SIMULATOR_ARM)
#include "jit/arm/Simulator-arm.h"
#elif defined(JS_SIMULATOR_ARM64)
# include "jit/arm64/vixl/Simulator-vixl.h"
#elif defined(JS_SIMULATOR_MIPS)
#include "jit/mips/Simulator-mips.h"
#endif

#ifdef JS_SIMULATOR

#define CALL_GENERATED_CODE(entry, p0, p1, p2, p3, p4, p5, p6, p7)     \
    (js::jit::Simulator::Current()->call(                              \
        JS_FUNC_TO_DATA_PTR(uint8_t*, entry), 8, p0, p1, p2, p3, p4, p5, p6, p7) & 0xffffffff)

#define CALL_GENERATED_1(entry, p0)                     \
    (js::jit::Simulator::Current()->call(               \
        JS_FUNC_TO_DATA_PTR(uint8_t*, entry), 1, p0) & 0xffffffff)

#define CALL_GENERATED_2(entry, p0, p1)                                 \
    (js::jit::Simulator::Current()->call(                               \
        JS_FUNC_TO_DATA_PTR(uint8_t*, entry), 2, p0, p1) & 0xffffffff)

#else


#define CALL_GENERATED_CODE(entry, p0, p1, p2, p3, p4, p5, p6, p7)   \
  entry(p0, p1, p2, p3, p4, p5, p6, p7)

#define CALL_GENERATED_1(entry, p0)      entry(p0)
#define CALL_GENERATED_2(entry, p0, p1)  entry(p0, p1)

#endif

#endif 
