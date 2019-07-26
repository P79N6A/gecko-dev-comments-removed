





#ifndef jit_JitCommon_h
#define jit_JitCommon_h



#ifdef JS_ARM_SIMULATOR
#include "jit/arm/Simulator-arm.h"


#define CALL_GENERATED_CODE(entry, p0, p1, p2, p3, p4, p5, p6, p7)     \
    (js::jit::Simulator::Current()->call(                              \
        JS_FUNC_TO_DATA_PTR(uint8_t *, entry), 8, p0, p1, p2, p3, p4, p5, p6, p7) & 0xffffffff)

#define CALL_GENERATED_YARR_CODE3(entry, p0, p1, p2)     \
    js::jit::Simulator::Current()->call(JS_FUNC_TO_DATA_PTR(uint8_t *, entry), 3, p0, p1, p2)

#define CALL_GENERATED_YARR_CODE4(entry, p0, p1, p2, p3) \
    js::jit::Simulator::Current()->call(JS_FUNC_TO_DATA_PTR(uint8_t *, entry), 4, p0, p1, p2, p3)

#define CALL_GENERATED_ASMJS(entry, p0, p1)              \
    (Simulator::Current()->call(JS_FUNC_TO_DATA_PTR(uint8_t *, entry), 2, p0, p1) & 0xffffffff)

#else


#define CALL_GENERATED_CODE(entry, p0, p1, p2, p3, p4, p5, p6, p7)   \
  entry(p0, p1, p2, p3, p4, p5, p6, p7)

#define CALL_GENERATED_YARR_CODE3(entry, p0, p1, p2)                 \
  entry(p0, p1, p2)

#define CALL_GENERATED_YARR_CODE4(entry, p0, p1, p2, p3)             \
  entry(p0, p1, p2, p3)

#define CALL_GENERATED_ASMJS(entry, p0, p1)                          \
  entry(p0, p1)

#endif

#endif 
