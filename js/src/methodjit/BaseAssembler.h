







































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

struct StackMarker {
    uint32 base;
    uint32 bytes;

    StackMarker(uint32 base, uint32 bytes)
      : base(base), bytes(bytes)
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
#elif defined(JS_CPU_SPARC)
    static const RegisterID ClobberInCall = JSC::SparcRegisters::l1;
#endif

    
    Label startLabel;
    Vector<CallPatch, 64, SystemAllocPolicy> callPatches;

    
    Registers   availInCall;

    
    
    uint32      extraStackSpace;

    
    Registers::CallConvention callConvention;

    
    
    uint32      stackAdjust;

    
#ifdef DEBUG
    bool        callIsAligned;
#endif

  public:
    Assembler()
      : callPatches(SystemAllocPolicy()),
        extraStackSpace(0),
        stackAdjust(0)
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
#elif defined(JS_CPU_SPARC)
static const JSC::MacroAssembler::RegisterID JSReturnReg_Type = JSC::SparcRegisters::i0;
static const JSC::MacroAssembler::RegisterID JSReturnReg_Data = JSC::SparcRegisters::i1;
static const JSC::MacroAssembler::RegisterID JSParamReg_Argc  = JSC::SparcRegisters::i2;
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

    
    void *getFallibleCallTarget(void *fun) {
#ifdef JS_CPU_ARM
        












        moveWithPatch(Imm32(intptr_t(fun)), JSC::ARMRegisters::ip);

        return JS_FUNC_TO_DATA_PTR(void *, JaegerStubVeneer);
#else
        




        return fun;
#endif
    }

    static inline uint32 align(uint32 bytes, uint32 alignment) {
        return (alignment - (bytes % alignment)) % alignment;
    }

    
    
    
    
    
    
    StackMarker allocStack(uint32 bytes, uint32 alignment = 4) {
        bytes += align(bytes + extraStackSpace, alignment);
        subPtr(Imm32(bytes), stackPointerRegister);
        extraStackSpace += bytes;
        return StackMarker(extraStackSpace, bytes);
    }

    
    void saveReg(RegisterID reg) {
        push(reg);
        extraStackSpace += sizeof(void *);
    }

    
    void restoreReg(RegisterID reg) {
        JS_ASSERT(extraStackSpace >= sizeof(void *));
        extraStackSpace -= sizeof(void *);
        pop(reg);
    }

    static const uint32 StackAlignment = 16;

    static inline uint32 alignForCall(uint32 stackBytes) {
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
        
        
        return align(stackBytes, StackAlignment);
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
#elif defined(JS_CPU_SPARC)
    static const uint32 ShadowStackSpace = 92;
#else
    static const uint32 ShadowStackSpace = 0;
#endif

#if defined(JS_CPU_SPARC)
    static const uint32 BaseStackSpace = 104;
#else
    static const uint32 BaseStackSpace = 0;
#endif

    
    
    
    
    
    
    
    
    
    void setupABICall(Registers::CallConvention convention, uint32 generalArgs) {
        JS_ASSERT(!callIsAligned);

        uint32 numArgRegs = Registers::numArgRegs(convention);
        uint32 pushCount = (generalArgs > numArgRegs)
                           ? generalArgs - numArgRegs
                           : 0;

        
        availInCall = Registers::TempRegs;

        
        
        uint32 total = (pushCount * sizeof(void *)) +
                       extraStackSpace;

        stackAdjust = (pushCount * sizeof(void *)) +
                      alignForCall(total);

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

    
    Address vmFrameOffset(uint32 offs) {
        return Address(stackPointerRegister, stackAdjust + extraStackSpace + offs);
    }

    
    Address addressOfExtra(const StackMarker &marker) {
        
        
        
        
        
        
        
        JS_ASSERT(marker.base <= extraStackSpace);
        return Address(stackPointerRegister, BaseStackSpace + stackAdjust + extraStackSpace - marker.base);
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
            availInCall.takeRegUnchecked(to);
        } else {
            storePtr(reg, addressOfArg(i));
        }
    }

    
    
    void storeArg(uint32 i, Address address) {
        JS_ASSERT(callIsAligned);
        RegisterID to;
        if (Registers::regForArg(callConvention, i, &to)) {
            loadPtr(address, to);
            availInCall.takeRegUnchecked(to);
        } else if (!availInCall.empty()) {
            
            RegisterID reg = availInCall.takeAnyReg();
            loadPtr(address, reg);
            storeArg(i, reg);
            availInCall.putReg(reg);
        } else {
            
            
            
            JS_NOT_REACHED("too much reg pressure");
        }
    }

    
    
    void storeArgAddr(uint32 i, Address address) {
        JS_ASSERT(callIsAligned);
        RegisterID to;
        if (Registers::regForArg(callConvention, i, &to)) {
            lea(address, to);
            availInCall.takeRegUnchecked(to);
        } else if (!availInCall.empty()) {
            
            RegisterID reg = availInCall.takeAnyReg();
            lea(address, reg);
            storeArg(i, reg);
            availInCall.putReg(reg);
        } else {
            
            
            
            JS_NOT_REACHED("too much reg pressure");
        }
    }

    void storeArg(uint32 i, Imm32 imm) {
        JS_ASSERT(callIsAligned);
        RegisterID to;
        if (Registers::regForArg(callConvention, i, &to)) {
            move(imm, to);
            availInCall.takeRegUnchecked(to);
        } else {
            store32(imm, addressOfArg(i));
        }
    }

    
    
    
    
    
    Call callWithABI(void *fun, bool canThrow) {
        
        
        
        
            
            
            
            fun = getFallibleCallTarget(fun);
        

        JS_ASSERT(callIsAligned);

        Call cl = call();
        callPatches.append(CallPatch(cl, fun));

        if (stackAdjust)
            addPtr(Imm32(stackAdjust), stackPointerRegister);

        stackAdjust = 0;

#ifdef DEBUG
        callIsAligned = false;
#endif
        return cl;
    }

    
    void freeStack(const StackMarker &mark) {
        JS_ASSERT(!callIsAligned);
        JS_ASSERT(mark.bytes <= extraStackSpace);

        extraStackSpace -= mark.bytes;
        addPtr(Imm32(mark.bytes), stackPointerRegister);
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
            
            
            addPtr(Imm32(sizeof(StackFrame) + frameDepth * sizeof(jsval)),
                   JSFrameReg,
                   ClobberInCall);
            storePtr(ClobberInCall, FrameAddress(offsetof(VMFrame, regs.sp)));
        }

        
        
        
        move(MacroAssembler::stackPointerRegister, Registers::ArgReg0);
    }

    void setupFallibleVMFrame(jsbytecode *pc, int32 frameDepth) {
        setupInfallibleVMFrame(frameDepth);

        
        storePtr(JSFrameReg, FrameAddress(VMFrame::offsetOfFp));

        
        storePtr(ImmPtr(pc),
                 FrameAddress(offsetof(VMFrame, regs) + offsetof(FrameRegs, pc)));
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
        JS_ASSERT(!callIsAligned);

        
        setupABICall(Registers::FastCall, 2);

        
        
        
        
        storeArg(0, Registers::ArgReg0);
        storeArg(1, Registers::ArgReg1);

        
        
        
        
        
        return callWithABI(ptr, true);
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
            return branch32(BelowOrEqual, capacity, Imm32(key.index()));
        }
        return branch32(BelowOrEqual, capacity, key.reg());
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

    static uint32 maskAddress(Address address) {
        return Registers::maskReg(address.base);
    }

    static uint32 maskAddress(BaseIndex address) {
        return Registers::maskReg(address.base) |
               Registers::maskReg(address.index);
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
      : Address(JSFrameReg, StackFrame::offsetOfFlags())
    {}
};

class PreserveRegisters {
    typedef JSC::MacroAssembler::RegisterID RegisterID;

    Assembler   &masm;
    uint32      count;
    RegisterID  regs[JSC::MacroAssembler::TotalRegisters];

  public:
    PreserveRegisters(Assembler &masm) : masm(masm), count(0) { }
    ~PreserveRegisters() { JS_ASSERT(!count); }

    void preserve(Registers mask) {
        JS_ASSERT(!count);

        while (!mask.empty()) {
            RegisterID reg = mask.takeAnyReg();
            regs[count++] = reg;
            masm.saveReg(reg);
        }
    }

    void restore() {
        while (count)
            masm.restoreReg(regs[--count]);
    }
};

} 
} 

#endif

