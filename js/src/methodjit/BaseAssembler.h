







































#if !defined jsjaeger_baseassembler_h__ && defined JS_METHODJIT
#define jsjaeger_baseassembler_h__

#include "jscntxt.h"
#include "jstl.h"
#include "assembler/assembler/MacroAssemblerCodeRef.h"
#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/LinkBuffer.h"
#include "assembler/moco/MocoStubs.h"
#include "methodjit/MethodJIT.h"
#include "methodjit/MachineRegs.h"
#include "CodeGenIncludes.h"
#include "jsobjinlines.h"
#include "jsscopeinlines.h"

namespace js {
namespace mjit {

class MaybeRegisterID {
    typedef JSC::MacroAssembler::RegisterID RegisterID;

  public:
    MaybeRegisterID()
      : reg_(Registers::ReturnReg), set(false)
    { }

    MaybeRegisterID(RegisterID reg)
      : reg_(reg), set(true)
    { }

    inline RegisterID reg() const { JS_ASSERT(set); return reg_; }
    inline void setReg(const RegisterID r) { reg_ = r; set = true; }
    inline bool isSet() const { return set; }

    MaybeRegisterID & operator =(const MaybeRegisterID &other) {
        set = other.set;
        reg_ = other.reg_;
        return *this;
    }

    MaybeRegisterID & operator =(RegisterID r) {
        setReg(r);
        return *this;
    }

  private:
    RegisterID reg_;
    bool set;
};



struct Int32Key {
    typedef JSC::MacroAssembler::RegisterID RegisterID;

    MaybeRegisterID reg_;
    int32 index_;

    Int32Key() : index_(0) { }

    static Int32Key FromRegister(RegisterID reg) {
        Int32Key key;
        key.reg_ = reg;
        return key;
    }
    static Int32Key FromConstant(int32 index) {
        Int32Key key;
        key.index_ = index;
        return key;
    }

    int32 index() const {
        JS_ASSERT(!reg_.isSet());
        return index_;
    }

    RegisterID reg() const { return reg_.reg(); }
    bool isConstant() const { return !reg_.isSet(); }
};

class MaybeJump {
    typedef JSC::MacroAssembler::Jump Jump;
  public:
    MaybeJump()
      : set(false)
    { }

    inline Jump getJump() const { JS_ASSERT(set); return jump; }
    inline Jump get() const { JS_ASSERT(set); return jump; }
    inline void setJump(const Jump &j) { jump = j; set = true; }
    inline bool isSet() const { return set; }

    inline MaybeJump &operator=(Jump j) { setJump(j); return *this; }

  private:
    Jump jump;
    bool set;
};

struct FrameAddress : JSC::MacroAssembler::Address
{
    FrameAddress(int32 offset)
      : Address(JSC::MacroAssembler::stackPointerRegister, offset)
    { }
};

struct ImmIntPtr : public JSC::MacroAssembler::ImmPtr
{
    ImmIntPtr(intptr_t val)
      : ImmPtr(reinterpret_cast<void*>(val))
    { }
};

class Assembler : public ValueAssembler
{
    struct CallPatch {
        CallPatch(Call cl, void *fun)
          : call(cl), fun(fun)
        { }

        Call call;
        JSC::FunctionPtr fun;
    };

    
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    static const RegisterID ClobberInCall = JSC::X86Registers::ecx;
#elif defined(JS_CPU_ARM)
    static const RegisterID ClobberInCall = JSC::ARMRegisters::r2;
#endif

    
    Label startLabel;
    Vector<CallPatch, 64, SystemAllocPolicy> callPatches;

    
    uint32      saveCount;
    RegisterID  savedRegs[TotalRegisters];

    
    Registers::CallConvention callConvention;

    
    
    uint32      stackAdjust;

    
#ifdef DEBUG
    bool        callIsAligned;
#endif

  public:
    Assembler()
      : callPatches(SystemAllocPolicy()),
        saveCount(0)
#ifdef DEBUG
        , callIsAligned(false)
#endif
    {
        startLabel = label();
    }

    
    static const uint32 TotalFPRegisters = FPRegisters::TotalFPRegisters;

    
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
static const JSC::MacroAssembler::RegisterID JSReturnReg_Type  = JSC::X86Registers::ecx;
static const JSC::MacroAssembler::RegisterID JSReturnReg_Data  = JSC::X86Registers::edx;
static const JSC::MacroAssembler::RegisterID JSParamReg_Argc   = JSC::X86Registers::ecx;
#elif defined(JS_CPU_ARM)
static const JSC::MacroAssembler::RegisterID JSReturnReg_Type  = JSC::ARMRegisters::r2;
static const JSC::MacroAssembler::RegisterID JSReturnReg_Data  = JSC::ARMRegisters::r1;
static const JSC::MacroAssembler::RegisterID JSParamReg_Argc   = JSC::ARMRegisters::r1;
#endif

    size_t distanceOf(Label l) {
        return differenceBetween(startLabel, l);
    }

    void load32FromImm(void *ptr, RegisterID reg) {
        load32(ptr, reg);
    }

    void loadShape(RegisterID obj, RegisterID shape) {
        load32(Address(obj, offsetof(JSObject, objShape)), shape);
    }

    Jump guardShape(RegisterID objReg, JSObject *obj) {
        return branch32(NotEqual, Address(objReg, offsetof(JSObject, objShape)),
                        Imm32(obj->shape()));
    }

    Jump testFunction(Condition cond, RegisterID fun) {
        return branchPtr(cond, Address(fun, offsetof(JSObject, clasp)),
                         ImmPtr(&js_FunctionClass));
    }

    


    Address objSlotRef(JSObject *obj, RegisterID reg, uint32 slot) {
        move(ImmPtr(&obj->slots), reg);
        loadPtr(reg, reg);
        return Address(reg, slot * sizeof(Value));
    }

#ifdef JS_CPU_X86
    void idiv(RegisterID reg) {
        m_assembler.cdq();
        m_assembler.idivl_r(reg);
    }

    void fastLoadDouble(RegisterID lo, RegisterID hi, FPRegisterID fpReg) {
        if (MacroAssemblerX86Common::getSSEState() >= HasSSE4_1) {
            m_assembler.movd_rr(lo, fpReg);
            m_assembler.pinsrd_rr(hi, fpReg);
        } else {
            m_assembler.movd_rr(lo, fpReg);
            m_assembler.movd_rr(hi, FPRegisters::Temp0);
            m_assembler.unpcklps_rr(FPRegisters::Temp0, fpReg);
        }
    }
#endif

    
    void *getCallTarget(void *fun) {
#ifdef JS_CPU_ARM
        










        void *pfun = JS_FUNC_TO_DATA_PTR(void *, JaegerStubVeneer);

        




        move(Imm32(intptr_t(fun)), JSC::ARMRegisters::ip);
#else
        




        void *pfun = fun;
#endif
        return pfun;
    }

    
    void saveRegs(uint32 volatileMask) {
        
        JS_ASSERT(saveCount == 0);
        
        JS_ASSERT(!callIsAligned);

        Registers set(volatileMask);
        while (!set.empty()) {
            JS_ASSERT(saveCount < TotalRegisters);

            RegisterID reg = set.takeAnyReg();
            savedRegs[saveCount++] = reg;
            push(reg);
        }
    }

    static const uint32 StackAlignment = 16;

    static inline uint32 alignForCall(uint32 stackBytes) {
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
        
        
        return (StackAlignment - (stackBytes % StackAlignment)) % StackAlignment;
#else
        return 0;
#endif
    }

    
    
    
    
    
    
    
    
    
    
#ifdef _WIN64
    static const uint32 ReturnStackAdjustment = 32;
#elif defined(JS_CPU_X86) && defined(JS_NO_FASTCALL)
    static const uint32 ReturnStackAdjustment = 16;
#else
    static const uint32 ReturnStackAdjustment = 0;
#endif

    void throwInJIT() {
        if (ReturnStackAdjustment)
            subPtr(Imm32(ReturnStackAdjustment), stackPointerRegister);
        move(ImmPtr(JS_FUNC_TO_DATA_PTR(void *, JaegerThrowpoline)), Registers::ReturnReg);
        jump(Registers::ReturnReg);
    }

    
#ifdef _WIN64
    static const uint32 ShadowStackSpace = 32;
#else
    static const uint32 ShadowStackSpace = 0;
#endif

    
    
    
    
    void setupABICall(Registers::CallConvention convention, uint32 generalArgs) {
        JS_ASSERT(!callIsAligned);

        uint32 numArgRegs = Registers::numArgRegs(convention);
        uint32 pushCount = (generalArgs > numArgRegs)
                           ? generalArgs - numArgRegs
                           : 0;

        
        stackAdjust = (pushCount + saveCount) * sizeof(void *);
        stackAdjust += alignForCall(stackAdjust);

#ifdef _WIN64
        
        
        stackAdjust += ShadowStackSpace;
#endif

        if (stackAdjust)
            subPtr(Imm32(stackAdjust), stackPointerRegister);

        callConvention = convention;
#ifdef DEBUG
        callIsAligned = true;
#endif
    }

    
    
    
    Address addressOfArg(uint32 i) {
        uint32 numArgRegs = Registers::numArgRegs(callConvention);
        JS_ASSERT(i >= numArgRegs);

        
        
        int32 spOffset = ((i - numArgRegs) * sizeof(void *)) + ShadowStackSpace;
        return Address(stackPointerRegister, spOffset);
    }

    
    void storeArg(uint32 i, RegisterID reg) {
        JS_ASSERT(callIsAligned);
        RegisterID to;
        if (Registers::regForArg(callConvention, i, &to)) {
            if (reg != to)
                move(reg, to);
        } else {
            storePtr(reg, addressOfArg(i));
        }
    }

    void storeArg(uint32 i, Imm32 imm) {
        JS_ASSERT(callIsAligned);
        RegisterID to;
        if (Registers::regForArg(callConvention, i, &to))
            move(imm, to);
        else
            store32(imm, addressOfArg(i));
    }

    
    
    
    
    
    Call callWithABI(void *fun) {
        JS_ASSERT(callIsAligned);

        Call cl = call();
        callPatches.append(CallPatch(cl, fun));

        if (stackAdjust)
            addPtr(Imm32(stackAdjust), stackPointerRegister);

#ifdef DEBUG
        callIsAligned = false;
#endif
        return cl;
    }

    
    void restoreRegs() {
        
        while (saveCount)
            pop(savedRegs[--saveCount]);
    }

    
    unsigned callReturnOffset(Call call) {
        return getLinkerCallReturnOffset(call);
    }


#define STUB_CALL_TYPE(type)                                                \
    Call callWithVMFrame(type stub, jsbytecode *pc, uint32 fd) {            \
        return fallibleVMCall(JS_FUNC_TO_DATA_PTR(void *, stub), pc, fd);   \
    }

    STUB_CALL_TYPE(JSObjStub);
    STUB_CALL_TYPE(VoidPtrStubUInt32);
    STUB_CALL_TYPE(VoidStubUInt32);
    STUB_CALL_TYPE(VoidStub);

#undef STUB_CALL_TYPE

    void setupInfallibleVMFrame(int32 frameDepth) {
        
        
        if (frameDepth >= 0) {
            
            
            addPtr(Imm32(sizeof(JSStackFrame) + frameDepth * sizeof(jsval)),
                   JSFrameReg,
                   ClobberInCall);
            storePtr(ClobberInCall, FrameAddress(offsetof(VMFrame, regs.sp)));
        }

        
        
        
        move(MacroAssembler::stackPointerRegister, Registers::ArgReg0);
    }

    void setupFallibleVMFrame(jsbytecode *pc, int32 frameDepth) {
        setupInfallibleVMFrame(frameDepth);

        
        storePtr(JSFrameReg, FrameAddress(offsetof(VMFrame, regs.fp)));

        
        storePtr(ImmPtr(pc),
                 FrameAddress(offsetof(VMFrame, regs) + offsetof(JSFrameRegs, pc)));
    }

    
    
    
    Call infallibleVMCall(void *ptr, int32 frameDepth) {
        setupInfallibleVMFrame(frameDepth);
        return wrapVMCall(ptr);
    }

    
    
    
    Call fallibleVMCall(void *ptr, jsbytecode *pc, int32 frameDepth) {
        setupFallibleVMFrame(pc, frameDepth);
        return wrapVMCall(ptr);
    }

    Call wrapVMCall(void *ptr) {
        JS_ASSERT(!saveCount);
        JS_ASSERT(!callIsAligned);

        
        setupABICall(Registers::FastCall, 2);

        
        
        
        
        storeArg(0, Registers::ArgReg0);
        storeArg(1, Registers::ArgReg1);

        return callWithABI(getCallTarget(ptr));
    }

    void finalize(JSC::LinkBuffer &linker) {
        for (size_t i = 0; i < callPatches.length(); i++) {
            CallPatch &patch = callPatches[i];
            linker.link(patch.call, JSC::FunctionPtr(patch.fun));
        }
    }

    struct FastArrayLoadFails {
        Jump rangeCheck;
        Jump holeCheck;
    };

    Jump guardArrayCapacity(RegisterID objReg, const Int32Key &key) {
        Address capacity(objReg, offsetof(JSObject, capacity));
        if (key.isConstant()) {
            JS_ASSERT(key.index() >= 0);
            return branch32(BelowOrEqual, payloadOf(capacity), Imm32(key.index()));
        }
        return branch32(BelowOrEqual, payloadOf(capacity), key.reg());
    }

    
    FastArrayLoadFails fastArrayLoad(RegisterID objReg, const Int32Key &key,
                                     RegisterID typeReg, RegisterID dataReg) {
        JS_ASSERT(objReg != typeReg);

        FastArrayLoadFails fails;
        fails.rangeCheck = guardArrayCapacity(objReg, key);

        RegisterID dslotsReg = objReg;
        loadPtr(Address(objReg, offsetof(JSObject, slots)), dslotsReg);

        
        if (key.isConstant()) {
            Address slot(objReg, key.index() * sizeof(Value));
            fails.holeCheck = fastArrayLoadSlot(slot, typeReg, dataReg);
        } else {
            BaseIndex slot(objReg, key.reg(), JSVAL_SCALE);
            fails.holeCheck = fastArrayLoadSlot(slot, typeReg, dataReg);
        }

        return fails;
    }

    void loadObjClass(RegisterID objReg, RegisterID destReg) {
        loadPtr(Address(objReg, offsetof(JSObject, clasp)), destReg);
    }

    Jump testClass(Condition cond, RegisterID claspReg, js::Class *clasp) {
        return branchPtr(cond, claspReg, ImmPtr(clasp));
    }

    Jump testObjClass(Condition cond, RegisterID objReg, js::Class *clasp) {
        return branchPtr(cond, Address(objReg, offsetof(JSObject, clasp)), ImmPtr(clasp));
    }

    void rematPayload(const StateRemat &remat, RegisterID reg) {
        if (remat.inMemory())
            loadPayload(remat.address(), reg);
        else
            move(remat.reg(), reg);
    }

    void loadDynamicSlot(RegisterID objReg, uint32 slot,
                         RegisterID typeReg, RegisterID dataReg) {
        loadPtr(Address(objReg, offsetof(JSObject, slots)), dataReg);
        loadValueAsComponents(Address(dataReg, slot * sizeof(Value)), typeReg, dataReg);
    }

    void loadObjProp(JSObject *obj, RegisterID objReg,
                     const js::Shape *shape,
                     RegisterID typeReg, RegisterID dataReg)
    {
        if (shape->isMethod())
            loadValueAsComponents(ObjectValue(shape->methodObject()), typeReg, dataReg);
        else if (obj->hasSlotsArray())
            loadDynamicSlot(objReg, shape->slot, typeReg, dataReg);
        else
            loadInlineSlot(objReg, shape->slot, typeReg, dataReg);
    }
};


#define STRICT_VARIANT(f)                                                     \
    (FunctionTemplateConditional(script->strictModeCode,                      \
                                 f<true>, f<false>))


static const JSC::MacroAssembler::RegisterID JSReturnReg_Type = Assembler::JSReturnReg_Type;
static const JSC::MacroAssembler::RegisterID JSReturnReg_Data = Assembler::JSReturnReg_Data;
static const JSC::MacroAssembler::RegisterID JSParamReg_Argc  = Assembler::JSParamReg_Argc;

struct FrameFlagsAddress : JSC::MacroAssembler::Address
{
    FrameFlagsAddress()
      : Address(JSFrameReg, JSStackFrame::offsetOfFlags())
    {}
};

} 
} 

#endif

