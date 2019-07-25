







































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

    enum Kind {
        GET,
        SET
    };

    JSC::CodeLocationLabel entry;
    JSC::CodeLocationLabel stubEntry;
    JSC::CodeLocationLabel load;
    JSC::CodeLocationDataLabelPtr shape;
    JSC::CodeLocationCall stubCall;
    bool touched : 1;
    bool typeConst : 1;
    bool dataConst : 1;
    bool dataWrite : 1;
    Kind kind : 2;
};

void JS_FASTCALL GetGlobalName(VMFrame &f, uint32 index);
void JS_FASTCALL SetGlobalName(VMFrame &f, uint32 index);

}
} 
} 

#endif 

