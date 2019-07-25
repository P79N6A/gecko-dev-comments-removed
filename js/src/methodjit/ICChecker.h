









































#if !defined jsjaeger_icchecker_h__ && defined JS_METHODJIT
#define jsjaeger_icchecker_h__

#include "assembler/assembler/MacroAssembler.h"

namespace js {
namespace mjit {

#if defined DEBUG && defined JS_CPU_ARM
static inline void
CheckInstMask(void *addr, uint32 mask, uint32 expected)
{
    uint32 inst = *static_cast<uint32 *>(addr);
    JS_ASSERT((inst & mask) == expected);
}

static inline void
CheckIsLDR(JSC::CodeLocationLabel label, uint8 rd)
{
    JS_ASSERT((rd & 0xf) == rd);
    CheckInstMask(label.executableAddress(), 0xfc50f000, 0xe4100000 | (rd << 12));
}

static inline void
CheckIsBLX(JSC::CodeLocationLabel label, uint8 rsrc)
{
    JS_ASSERT((rsrc & 0xf) == rsrc);
    CheckInstMask(label.executableAddress(), 0xfff000ff, 0xe1200030 | rsrc);
}

static inline void
CheckIsStubCall(JSC::CodeLocationLabel label)
{
    CheckIsLDR(label.labelAtOffset(-4), JSC::ARMRegisters::ip);
    CheckIsLDR(label.labelAtOffset(0), JSC::ARMRegisters::r8);
    CheckIsBLX(label.labelAtOffset(4), JSC::ARMRegisters::r8);
}
#else
static inline void CheckIsStubCall(JSC::CodeLocationLabel label) {}
#endif

} 
} 

#endif
