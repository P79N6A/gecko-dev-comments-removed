







































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

struct CallDescription {
    CallDescription(uint32 argc) : argc(argc), code(NULL)
    { }
    JSObject    *callee;
    JSFunction  *fun;
    uint32      argc;
    void        *code;
    bool        unjittable;     
};

struct CallIC {
    typedef JSC::MacroAssembler::RegisterID RegisterID;
    typedef JSC::MacroAssembler::Call Call;

    JSC::CodeLocationLabel fastPathStart;
    JSC::CodeLocationLabel slowPathStart;

    jsbytecode  *pc;
    FrameSize   frameSize;

    uint32      slowCallOffset : 12;        
    uint32      inlineRejoinOffset : 10;    
    uint32      completedRejoinOffset : 10; 
    uint32      inlineJumpOffset : 10;      
    uint32      inlineCalleeGuard : 10;     
                                            
                                            
    uint32      inlineCallOffset : 10;      

    RegisterID  calleeData : 5;
    bool hasExtendedInlinePath : 1;

    
    uint32 rvalOffset;

    
    mutable JSC::ExecutablePool *pool;

    
    
    mutable JSObject *inlineCallee;

    
    
    mutable JSObject *guardedNative;

    bool shouldDisable(JSContext *cx, JITScript *jit, CallDescription &call);
    void purge(JITScript *jit, Repatcher &repatcher);
    void disable(Repatcher &repatcher);
    bool update(JSContext *cx, JITScript *jit, CallDescription &call);
    JSOp op() const;
    void reenable(JITScript *jit, Repatcher &repatcher);
    void purgeInlineGuard(Repatcher &repatcher);
    void purgeStub(Repatcher &repatcher);

    JSC::CodeLocationLabel inlineRejoinLabel() const {
       return fastPathStart.labelAtOffset(inlineRejoinOffset);
    }
    void *returnAddress() const {
        return inlineRejoinLabel().executableAddress();
    }
    JSC::CodeLocationLabel completedRejoinLabel() const {
        return fastPathStart.labelAtOffset(completedRejoinOffset);
    }
    JSC::CodeLocationJump inlineJump() const {
        return fastPathStart.jumpAtOffset(inlineJumpOffset);
    }
    JSC::CodeLocationCall slowCall() const {
        return slowPathStart.callAtOffset(slowCallOffset);
    }
    JSC::CodeLocationDataLabelPtr calleePtr() const {
        JS_ASSERT(hasExtendedInlinePath);
        return fastPathStart.dataLabelPtrAtOffset(inlineCalleeGuard);
    }
    JSC::CodeLocationJump inlineCall() const {
        JS_ASSERT(hasExtendedInlinePath);
        return fastPathStart.jumpAtOffset(inlineCallOffset);
    }
};

template <bool UpdateIC> void * JS_FASTCALL New(VMFrame &f, ic::CallIC *ic);
template <bool DynamicArgc, bool UpdateIC> void * JS_FASTCALL Call(VMFrame &f, ic::CallIC *ic);
template <bool LazyArgsObj> JSBool JS_FASTCALL SplatApplyArgs(VMFrame &f);
void * JS_FASTCALL FailedFunCall(VMFrame &f, ic::CallIC *ic);
void * JS_FASTCALL FailedFunApply(VMFrame &f, ic::CallIC *ic);
void * JS_FASTCALL FailedFunApplyLazyArgs(VMFrame &f, ic::CallIC *ic);

void PurgeMICs(JSContext *cx, JSScript *script);
void SweepICs(JSContext *cx, JSScript *script);
void PurgeICs(JSContext *cx, JSScript *script);

} 
} 
} 

#endif 

