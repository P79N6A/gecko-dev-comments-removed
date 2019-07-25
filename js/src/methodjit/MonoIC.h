







































#if !defined jsjaeger_mono_ic_h__ && defined JS_METHODJIT && defined JS_MONOIC
#define jsjaeger_mono_ic_h__

#include "assembler/assembler/CodeLocation.h"
#include "methodjit/MethodJIT.h"

namespace js {
namespace mjit {
namespace ic {

struct MICInfo {
#ifdef JS_CPU_X86
    static const uint32 GET_DATA_OFFSET = 6;
    static const uint32 GET_TYPE_OFFSET = 12;

    static const uint32 SET_TYPE_OFFSET = 9;
    static const uint32 SET_DATA_CONST_TYPE_OFFSET = 19;
    static const uint32 SET_DATA_TYPE_OFFSET = 15;
#elif JS_CPU_X64
    
#endif

    enum Kind
#ifdef _MSC_VER
    : uint8_t
#endif
    {
        GET,
        SET,
        TRACER
    };

#if defined JS_PUNBOX64
    uint32 patchValueOffset;
#endif
    JSC::CodeLocationLabel entry;
    JSC::CodeLocationLabel stubEntry;
    JSC::CodeLocationLabel load;
    JSC::CodeLocationDataLabelPtr shape;
    JSC::CodeLocationCall stubCall;
    JSC::CodeLocationJump traceHint;
    JSC::CodeLocationJump slowTraceHint;
    Kind kind : 2;
    union {
        struct {
            bool touched : 1;
            bool typeConst : 1;
            bool dataConst : 1;
            bool dataWrite : 1;
        } name;
        bool hasSlowTraceHint;
    } u;
};

void JS_FASTCALL GetGlobalName(VMFrame &f, uint32 index);
void JS_FASTCALL SetGlobalName(VMFrame &f, uint32 index);

} 
} 
} 

#endif 

