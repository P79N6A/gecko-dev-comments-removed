







































#if !defined jsjaeger_mono_ic_h__ && defined JS_METHODJIT
#define jsjaeger_mono_ic_h__

#include "assembler/assembler/CodeLocation.h"
#include "methodjit/MethodJIT.h"

#define ENABLE_MIC 1

namespace js {
namespace mjit {
namespace ic {

struct MICInfo {
#ifdef JS_CPU_X86
    static const uint32 TYPE_OFFSET = 9;
    static const uint32 DATA_OFFSET = 15;
#endif

    JSC::CodeLocationLabel entry;
    JSC::CodeLocationLabel stubEntry;
    JSC::CodeLocationLabel load;
    JSC::CodeLocationDataLabelPtr shape;
    JSC::CodeLocationCall stubCall;
    bool touched : 1;
    uint32 lastSlot : 31;
};

void JS_FASTCALL GetGlobalName(VMFrame &f, uint32 index);

}
} 
} 

#endif 

