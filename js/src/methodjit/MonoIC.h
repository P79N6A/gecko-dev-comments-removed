







































#if !defined jsjaeger_mono_ic_h__ && defined JS_METHODJIT && defined JS_MONOIC
#define jsjaeger_mono_ic_h__

#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/CodeLocation.h"
#include "assembler/moco/MocoStubs.h"
#include "methodjit/MethodJIT.h"
#include "CodeGenIncludes.h"
#include "methodjit/ICRepatcher.h"

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

    uint32 staticLocalSlots() const {
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

struct GlobalNameIC
{
    typedef JSC::MacroAssembler::RegisterID RegisterID;

    JSC::CodeLocationLabel  fastPathStart;
    JSC::CodeLocationCall   slowPathCall;

    








    int32 loadStoreOffset   : 15;
    int32 shapeOffset       : 15;
    bool usePropertyCache   : 1;
};

struct GetGlobalNameIC : public GlobalNameIC
{
};

struct SetGlobalNameIC : public GlobalNameIC
{
    JSC::CodeLocationLabel  slowPathStart;

    
    JSC::JITCode            extraStub;

    
    int32 inlineShapeJump : 10;   
    int32 extraShapeGuard : 6;    
    bool objConst : 1;          
    RegisterID objReg   : 5;    
    RegisterID shapeReg : 5;    
    bool hasExtraStub : 1;      

    int32 fastRejoinOffset : 16;  
    int32 extraStoreOffset : 16;  

    
    ValueRemat vr;              

    void patchInlineShapeGuard(Repatcher &repatcher, int32 shape);
    void patchExtraShapeGuard(Repatcher &repatcher, int32 shape);
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
    uint32 loopCounter;
    uint32 loopCounterStart;

    bool initialized : 1;
    bool hasSlowTraceHint : 1;
};

static const uint16 BAD_TRACEIC_INDEX = (uint16)0xffff;

void JS_FASTCALL GetGlobalName(VMFrame &f, ic::GetGlobalNameIC *ic);
void JS_FASTCALL SetGlobalName(VMFrame &f, ic::SetGlobalNameIC *ic);

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

    
    uint32 icCallOffset    : 16;

    
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
void SweepCallICs(JSContext *cx, JSScript *script, bool purgeAll);

} 
} 
} 

#endif 

