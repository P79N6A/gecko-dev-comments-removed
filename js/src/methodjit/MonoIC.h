







































#if !defined jsjaeger_mono_ic_h__ && defined JS_METHODJIT && defined JS_MONOIC
#define jsjaeger_mono_ic_h__

#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/CodeLocation.h"
#include "methodjit/MethodJIT.h"
#include "CodeGenIncludes.h"

namespace js {
namespace mjit {
namespace ic {

struct MICInfo {
#ifdef JS_CPU_X86
    static const uint32 GET_DATA_OFFSET = 6;
    static const uint32 GET_TYPE_OFFSET = 12;

    static const uint32 SET_TYPE_OFFSET = 6;
    static const uint32 SET_DATA_CONST_TYPE_OFFSET = 16;
    static const uint32 SET_DATA_TYPE_OFFSET = 12;
#elif JS_CPU_X64 || JS_CPU_ARM
    
    
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

    
    JSC::CodeLocationLabel entry;
    JSC::CodeLocationLabel stubEntry;

    

    
    JSC::CodeLocationLabel load;
    JSC::CodeLocationDataLabel32 shape;
    JSC::CodeLocationCall stubCall;
#if defined JS_PUNBOX64
    uint32 patchValueOffset;
#endif

    
    JSC::CodeLocationJump traceHint;
    JSC::CodeLocationJump slowTraceHint;

    
    Kind kind : 3;
    union {
        
        struct {
            bool touched : 1;
            bool typeConst : 1;
            bool dataConst : 1;
        } name;
        
        bool hasSlowTraceHint;
    } u;
};

void JS_FASTCALL GetGlobalName(VMFrame &f, uint32 index);
void JS_FASTCALL SetGlobalName(VMFrame &f, uint32 index);


struct CallICInfo {
    typedef JSC::MacroAssembler::RegisterID RegisterID;

    enum PoolIndex {
        Pool_ScriptStub,
        Pool_ClosureStub,
        Pool_NativeStub,
        Total_Pools
    };

    JSC::ExecutablePool *pools[Total_Pools];

    
    JSObject *fastGuardedObject;
    JSObject *fastGuardedNative;
    Value constantThis;

    uint32 argc : 16;
    uint32 frameDepth : 16;

    
    JSC::CodeLocationDataLabelPtr funGuard;

    
    JSC::CodeLocationLabel slowPathStart;

    
    JSC::CodeLocationJump funJump;

    
    uint32 hotCallOffset   : 16;
    uint32 joinPointOffset : 16;

    
    uint32 oolCallOffset   : 16;

    
    uint32 oolJumpOffset   : 16;

    
    uint32 hotPathOffset   : 16;

    
    uint32 slowJoinOffset  : 16;

    RegisterID funObjReg : 5;
    RegisterID funPtrReg : 5;
    bool isConstantThis : 1;
    bool hit : 1;
    bool hasJsFunCheck : 1;

    inline void reset() {
        fastGuardedObject = NULL;
        fastGuardedNative = NULL;
        hit = false;
        hasJsFunCheck = false;
        pools[0] = pools[1] = pools[2] = NULL;
    }

    inline void releasePools() {
        releasePool(Pool_ScriptStub);
        releasePool(Pool_ClosureStub);
        releasePool(Pool_NativeStub);
    }

    inline void releasePool(PoolIndex index) {
        if (pools[index]) {
            pools[index]->release();
            pools[index] = NULL;
        }
    }
};

void * JS_FASTCALL New(VMFrame &f, uint32 index);
void * JS_FASTCALL Call(VMFrame &f, uint32 index);
void JS_FASTCALL NativeNew(VMFrame &f, uint32 index);
void JS_FASTCALL NativeCall(VMFrame &f, uint32 index);

void PurgeMICs(JSContext *cx, JSScript *script);
void SweepCallICs(JSContext *cx, JSScript *script);

} 
} 
} 

#endif 

