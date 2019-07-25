







































#if !defined jsjaeger_mono_ic_h__ && defined JS_METHODJIT && defined JS_MONOIC
#define jsjaeger_mono_ic_h__

#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/CodeLocation.h"
#include "methodjit/MethodJIT.h"
#include "CodeGenIncludes.h"

namespace js {
namespace mjit {

class FrameSize
{
    uint32 frameDepth_ : 16;
    uint32 argc_;
  public:
    void initStatic(uint32 frameDepth, uint32 argc) {
        JS_ASSERT(frameDepth > 0);
        frameDepth_ = frameDepth;
        argc_ = argc;
    }

    void initDynamic() {
        frameDepth_ = 0;
        argc_ = -1;  
    }

    bool isStatic() const {
        return frameDepth_ > 0;
    }

    bool isDynamic() const {
        return frameDepth_ == 0;
    }

    uint32 staticFrameDepth() const {
        JS_ASSERT(isStatic());
        return frameDepth_;
    }

    uint32 staticArgc() const {
        JS_ASSERT(isStatic());
        return argc_;
    }

    uint32 getArgc(VMFrame &f) const {
        return isStatic() ? staticArgc() : f.u.call.dynamicArgc;
    }
};

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
        SET
    };

    
    JSC::CodeLocationLabel entry;
    JSC::CodeLocationLabel stubEntry;

    

    
    JSC::CodeLocationLabel load;
    JSC::CodeLocationDataLabel32 shape;
    JSC::CodeLocationCall stubCall;
#if defined JS_PUNBOX64
    uint32 patchValueOffset;
#endif

    
    Kind kind : 3;
    union {
        
        struct {
            bool touched : 1;
            bool typeConst : 1;
            bool dataConst : 1;
        } name;
    } u;
};

struct TraceICInfo {
    TraceICInfo() {}

    JSC::CodeLocationLabel stubEntry;
    JSC::CodeLocationLabel jumpTarget;
    JSC::CodeLocationJump traceHint;
    JSC::CodeLocationJump slowTraceHint;
#ifdef DEBUG
    jsbytecode *jumpTargetPC;
#endif
    
    
    void *traceData;
    uintN traceEpoch;

    bool hasSlowTraceHint : 1;
};

static const uint16 BAD_TRACEIC_INDEX = (uint16)0xffff;

void JS_FASTCALL GetGlobalName(VMFrame &f, ic::MICInfo *ic);
void JS_FASTCALL SetGlobalName(VMFrame &f, ic::MICInfo *ic);

struct EqualityICInfo {
    typedef JSC::MacroAssembler::RegisterID RegisterID;

    JSC::CodeLocationLabel stubEntry;
    JSC::CodeLocationCall stubCall;
    BoolStub stub;
    JSC::CodeLocationLabel target;
    JSC::CodeLocationLabel fallThrough;
    JSC::CodeLocationJump jumpToStub;

    ValueRemat lvr, rvr;

    bool generated : 1;
    JSC::MacroAssembler::RegisterID tempReg : 5;
    Assembler::Condition cond;
};

JSBool JS_FASTCALL Equality(VMFrame &f, ic::EqualityICInfo *ic);


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

    
    jsbytecode *pc;

    FrameSize frameSize;

    
    JSC::CodeLocationDataLabelPtr funGuard;

    
    JSC::CodeLocationLabel slowPathStart;

    
    JSC::CodeLocationJump funJump;

    
    uint32 hotJumpOffset   : 16;
    uint32 joinPointOffset : 16;

    
    uint32 oolCallOffset   : 16;

    
    uint32 oolJumpOffset   : 16;

    
    uint32 hotPathOffset   : 16;

    
    uint32 slowJoinOffset  : 16;

    RegisterID funObjReg : 5;
    RegisterID funPtrReg : 5;
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

void * JS_FASTCALL New(VMFrame &f, ic::CallICInfo *ic);
void * JS_FASTCALL Call(VMFrame &f, ic::CallICInfo *ic);
void JS_FASTCALL NativeNew(VMFrame &f, ic::CallICInfo *ic);
void JS_FASTCALL NativeCall(VMFrame &f, ic::CallICInfo *ic);
JSBool JS_FASTCALL SplatApplyArgs(VMFrame &f);

void PurgeMICs(JSContext *cx, JSScript *script);
void SweepCallICs(JSScript *script);

} 
} 
} 

#endif 

