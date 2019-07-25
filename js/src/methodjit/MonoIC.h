







































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
    static const uint32 GET_DATA_OFFSET = 9;
    static const uint32 GET_TYPE_OFFSET = 15;
    static const uint32 SET_TYPE_OFFSET = 9;
    static const uint32 SET_DATA_CONST_TYPE_OFFSET = 19;
    static const uint32 SET_DATA_TYPE_OFFSET = 15;
#endif

    enum Type {
        GET,
        SET
    };

    JSC::CodeLocationLabel entry;
    JSC::CodeLocationLabel stubEntry;
    JSC::CodeLocationLabel load;
    JSC::CodeLocationDataLabelPtr shape;
    JSC::CodeLocationCall stubCall;
    bool touched;
    bool typeConst;
    bool dataConst;
    bool dataWrite;
    uint32 lastSlot;
    Type type;
};

void JS_FASTCALL GetGlobalName(VMFrame &f, uint32 index);
void JS_FASTCALL SetGlobalName(VMFrame &f, uint32 index);

}
} 
} 

#endif 

