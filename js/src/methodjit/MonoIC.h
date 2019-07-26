






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
    uint32_t frameDepth_ : 16;
    uint32_t argc_;
  public:
    void initStatic(uint32_t frameDepth, uint32_t argc) {
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

    uint32_t staticLocalSlots() const {
        JS_ASSERT(isStatic());
        return frameDepth_;
    }

    uint32_t staticArgc() const {
        JS_ASSERT(isStatic());
        return argc_;
    }

    uint32_t getArgc(VMFrame &f) const {
        return isStatic() ? staticArgc() : f.u.call.dynamicArgc;
    }

    bool lowered(jsbytecode *pc) const {
        return isDynamic() || staticArgc() != GET_ARGC(pc);
    }

    RejoinState rejoinState(jsbytecode *pc, bool native) {
        if (isStatic()) {
            if (staticArgc() == GET_ARGC(pc))
                return native ? REJOIN_NATIVE : REJOIN_CALL_PROLOGUE;
            JS_ASSERT(staticArgc() == GET_ARGC(pc) - 1);
            return native ? REJOIN_NATIVE_LOWERED : REJOIN_CALL_PROLOGUE_LOWERED_CALL;
        }
        return native ? REJOIN_NATIVE_LOWERED : REJOIN_CALL_PROLOGUE_LOWERED_APPLY;
    }

    bool lowered(jsbytecode *pc) {
        return !isStatic() || staticArgc() != GET_ARGC(pc);
    }
};

namespace ic {

struct GlobalNameIC
{
    typedef JSC::MacroAssembler::RegisterID RegisterID;

    JSC::CodeLocationLabel  fastPathStart;
    JSC::CodeLocationCall   slowPathCall;

    








    int32_t loadStoreOffset   : 15;
    int32_t shapeOffset       : 15;
};

struct GetGlobalNameIC : public GlobalNameIC
{
};

struct SetGlobalNameIC : public GlobalNameIC
{
    JSC::CodeLocationLabel  slowPathStart;

    
    int32_t inlineShapeJump : 10;   
    bool objConst : 1;          
    RegisterID objReg   : 5;    
    RegisterID shapeReg : 5;    

    int32_t fastRejoinOffset : 16;  

    
    ValueRemat vr;              

    void patchInlineShapeGuard(Repatcher &repatcher, RawShape shape);
};

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

    
    JSCList links;

    enum PoolIndex {
        Pool_ScriptStub,
        Pool_ClosureStub,
        Total_Pools
    };

    JSC::ExecutablePool *pools[Total_Pools];

    
    JSObject *fastGuardedObject;
    JSObject *fastGuardedNative;

    
    CallSite *call;

    FrameSize frameSize;

    
    JSC::CodeLocationLabel funGuardLabel;

    
    JSC::CodeLocationDataLabelPtr funGuard;

    
    JSC::CodeLocationLabel slowPathStart;

    
    JSC::CodeLocationJump funJump;

    



    JSC::CodeLocationLabel funJumpTarget;

    



    bool hasIonStub_;
    JSC::JITCode lastOolCode_;
    JSC::CodeLocationJump lastOolJump_;

    
    uint32_t hotJumpOffset   : 16;
    uint32_t joinPointOffset : 16;

    
    uint32_t oolCallOffset   : 16;

    
    uint32_t oolJumpOffset   : 16;

    
    uint32_t icCallOffset    : 16;

    
    uint32_t hotPathOffset   : 16;

    
    uint32_t slowJoinOffset  : 16;

    
    uint32_t ionJoinOffset : 16;

    RegisterID funObjReg : 5;
    bool hit : 1;
    bool hasJsFunCheck : 1;
    bool typeMonitored : 1;

    inline void releasePool(PoolIndex index) {
        if (pools[index]) {
            pools[index]->release();
            pools[index] = NULL;
        }
    }

    inline void purgeGuardedObject() {
        JS_ASSERT(fastGuardedObject);
        releasePool(CallICInfo::Pool_ClosureStub);
        hasJsFunCheck = false;
        fastGuardedObject = NULL;
        JS_REMOVE_LINK(&links);
    }

    bool hasJMStub() const {
        return !!pools[Pool_ScriptStub];
    }
    bool hasIonStub() const {
        return hasIonStub_;
    }
    bool hasStubOolJump() const {
        return hasIonStub();
    }
    JSC::CodeLocationLabel icCall() {
        return slowPathStart.labelAtOffset(icCallOffset);
    }
    JSC::CodeLocationJump oolJump() {
        return slowPathStart.jumpAtOffset(oolJumpOffset);
    }
    JSC::CodeLocationJump lastOolJump() {
        if (hasStubOolJump())
            return lastOolJump_;
        return oolJump();
    }
    JSC::JITCode lastOolCode() {
        JS_ASSERT(hasStubOolJump());
        return lastOolCode_;
    }
    void updateLastOolJump(JSC::CodeLocationJump jump, JSC::JITCode code) {
        lastOolJump_ = jump;
        lastOolCode_ = code;
    }
    JSC::CodeLocationLabel nativeRejoin() {
        return slowPathStart.labelAtOffset(slowJoinOffset);
    }
    JSC::CodeLocationLabel ionJoinPoint() {
        return funGuard.labelAtOffset(ionJoinOffset);
    }

    inline void reset(Repatcher &repatcher) {
        if (fastGuardedObject) {
            repatcher.repatch(funGuard, NULL);
            repatcher.relink(funJump, slowPathStart);
            purgeGuardedObject();
        }
        if (fastGuardedNative) {
            repatcher.relink(funJump, slowPathStart);
            fastGuardedNative = NULL;
        }
        if (pools[Pool_ScriptStub] || hasIonStub()) {
            repatcher.relink(oolJump(), icCall());
            releasePool(Pool_ScriptStub);
        }
        hit = false;
        hasIonStub_ = false;
    }
};

void * JS_FASTCALL New(VMFrame &f, ic::CallICInfo *ic);
void * JS_FASTCALL Call(VMFrame &f, ic::CallICInfo *ic);
void * JS_FASTCALL NativeNew(VMFrame &f, ic::CallICInfo *ic);
void * JS_FASTCALL NativeCall(VMFrame &f, ic::CallICInfo *ic);
JSBool JS_FASTCALL SplatApplyArgs(VMFrame &f);

void GenerateArgumentCheckStub(VMFrame &f);

} 
} 
} 

#endif 

