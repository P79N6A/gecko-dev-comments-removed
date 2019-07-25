







































#if !defined jsjaeger_baseassembler_h__ && defined JS_METHODJIT
#define jsjaeger_baseassembler_h__

#include "jscntxt.h"
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

    struct DoublePatch {
        double d;
        DataLabelPtr label;
    };

    
    Label startLabel;
    Vector<CallPatch, 64, SystemAllocPolicy> callPatches;
    Vector<DoublePatch, 16, SystemAllocPolicy> doublePatches;

    
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
        availInCall(0),
        extraStackSpace(0),
        stackAdjust(0)
#ifdef DEBUG
        , callIsAligned(false)
#endif
    {
        startLabel = label();
    }

    
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
static const JSC::MacroAssembler::RegisterID JSReturnReg_Type  = JSC::X86Registers::edi;
static const JSC::MacroAssembler::RegisterID JSReturnReg_Data  = JSC::X86Registers::esi;
static const JSC::MacroAssembler::RegisterID JSParamReg_Argc   = JSC::X86Registers::ecx;
#elif defined(JS_CPU_ARM)
static const JSC::MacroAssembler::RegisterID JSReturnReg_Type  = JSC::ARMRegisters::r5;
static const JSC::MacroAssembler::RegisterID JSReturnReg_Data  = JSC::ARMRegisters::r4;
static const JSC::MacroAssembler::RegisterID JSParamReg_Argc   = JSC::ARMRegisters::r1;
#elif defined(JS_CPU_SPARC)
static const JSC::MacroAssembler::RegisterID JSReturnReg_Type = JSC::SparcRegisters::l2;
static const JSC::MacroAssembler::RegisterID JSReturnReg_Data = JSC::SparcRegisters::l3;
static const JSC::MacroAssembler::RegisterID JSParamReg_Argc  = JSC::SparcRegisters::l4;
#endif

    size_t distanceOf(Label l) {
        return differenceBetween(startLabel, l);
    }

    void loadPtrFromImm(void *ptr, RegisterID reg) {
        loadPtr(ptr, reg);
    }

    void loadShape(RegisterID obj, RegisterID shape) {
        loadPtr(Address(obj, JSObject::offsetOfShape()), shape);
    }

    Jump guardShape(RegisterID objReg, JSObject *obj) {
        return branchPtr(NotEqual, Address(objReg, JSObject::offsetOfShape()),
                         ImmPtr(obj->lastProperty()));
    }

    


    Address objSlotRef(JSObject *obj, RegisterID reg, uint32 slot) {
        move(ImmPtr(obj), reg);
        if (obj->isFixedSlot(slot)) {
            return Address(reg, JSObject::getFixedSlotOffset(slot));
        } else {
            loadPtr(Address(reg, JSObject::offsetOfSlots()), reg);
            return Address(reg, obj->dynamicSlotIndex(slot) * sizeof(Value));
        }
    }

#ifdef JS_CPU_X86
    void idiv(RegisterID reg) {
        m_assembler.cdq();
        m_assembler.idivl_r(reg);
    }

    void fastLoadDouble(RegisterID lo, RegisterID hi, FPRegisterID fpReg) {
        JS_ASSERT(fpReg != Registers::FPConversionTemp);
        if (MacroAssemblerX86Common::getSSEState() >= HasSSE4_1) {
            m_assembler.movd_rr(lo, fpReg);
            m_assembler.pinsrd_rr(hi, fpReg);
        } else {
            m_assembler.movd_rr(lo, fpReg);
            m_assembler.movd_rr(hi, Registers::FPConversionTemp);
            m_assembler.unpcklps_rr(Registers::FPConversionTemp, fpReg);
        }
    }
#endif

    



    void moveInt32OrDouble(RegisterID data, RegisterID type, Address address, FPRegisterID fpreg)
    {
#ifdef JS_CPU_X86
        fastLoadDouble(data, type, fpreg);
        Jump notInteger = testInt32(Assembler::NotEqual, type);
        convertInt32ToDouble(data, fpreg);
        notInteger.linkTo(label(), this);
#else
        Jump notInteger = testInt32(Assembler::NotEqual, type);
        convertInt32ToDouble(data, fpreg);
        Jump fallthrough = jump();
        notInteger.linkTo(label(), this);

        
        storeValueFromComponents(type, data, address);
        loadDouble(address, fpreg);

        fallthrough.linkTo(label(), this);
#endif
    }

    



    template <typename T>
    void moveInt32OrDouble(T address, FPRegisterID fpreg)
    {
        Jump notInteger = testInt32(Assembler::NotEqual, address);
        convertInt32ToDouble(payloadOf(address), fpreg);
        Jump fallthrough = jump();
        notInteger.linkTo(label(), this);
        loadDouble(address, fpreg);
        fallthrough.linkTo(label(), this);
    }

    
    void ensureInMemoryDouble(Address address)
    {
        Jump notInteger = testInt32(Assembler::NotEqual, address);
        convertInt32ToDouble(payloadOf(address), Registers::FPConversionTemp);
        storeDouble(Registers::FPConversionTemp, address);
        notInteger.linkTo(label(), this);
    }

    void negateDouble(FPRegisterID fpreg)
    {
#if defined JS_CPU_X86 || defined JS_CPU_X64
        static const uint64 DoubleNegMask = 0x8000000000000000ULL;
        loadDouble(&DoubleNegMask, Registers::FPConversionTemp);
        xorDouble(Registers::FPConversionTemp, fpreg);
#elif defined JS_CPU_ARM || defined JS_CPU_SPARC
        negDouble(fpreg, fpreg);
#endif
    }

    
    void *getFallibleCallTarget(void *fun) {
#ifdef JS_CPU_ARM
        












        moveWithPatch(Imm32(intptr_t(fun)), JSC::ARMRegisters::ip);

        return JS_FUNC_TO_DATA_PTR(void *, JaegerStubVeneer);
#elif defined(JS_CPU_SPARC)
        







        moveWithPatch(Imm32(intptr_t(fun)), JSC::SparcRegisters::i0);
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
            
            RegisterID reg = availInCall.takeAnyReg().reg();
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
            
            RegisterID reg = availInCall.takeAnyReg().reg();
            lea(address, reg);
            storeArg(i, reg);
            availInCall.putReg(reg);
        } else {
            
            
            
            JS_NOT_REACHED("too much reg pressure");
        }
    }

    void storeArg(uint32 i, ImmPtr imm) {
        JS_ASSERT(callIsAligned);
        RegisterID to;
        if (Registers::regForArg(callConvention, i, &to)) {
            move(imm, to);
            availInCall.takeRegUnchecked(to);
        } else {
            storePtr(imm, addressOfArg(i));
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


#define STUB_CALL_TYPE(type)                                                             \
    Call callWithVMFrame(bool inlining, type stub, jsbytecode *pc,                       \
                         DataLabelPtr *pinlined, uint32 fd) {                            \
        return fallibleVMCall(inlining, JS_FUNC_TO_DATA_PTR(void *, stub),               \
                              pc, pinlined, fd);                                         \
    }

    STUB_CALL_TYPE(JSObjStub);
    STUB_CALL_TYPE(VoidPtrStubUInt32);
    STUB_CALL_TYPE(VoidStubUInt32);
    STUB_CALL_TYPE(VoidStub);

#undef STUB_CALL_TYPE

    void setupFrameDepth(int32 frameDepth) {
        
        
        if (frameDepth >= 0) {
            
            
            addPtr(Imm32(sizeof(StackFrame) + frameDepth * sizeof(jsval)),
                   JSFrameReg,
                   Registers::ClobberInCall);
            storePtr(Registers::ClobberInCall, FrameAddress(VMFrame::offsetOfRegsSp()));
        }
    }

    void setupInfallibleVMFrame(int32 frameDepth) {
        setupFrameDepth(frameDepth);

        
        
        
        move(MacroAssembler::stackPointerRegister, Registers::ArgReg0);
    }

    void setupFallibleVMFrame(bool inlining, jsbytecode *pc,
                              DataLabelPtr *pinlined, int32 frameDepth) {
        setupInfallibleVMFrame(frameDepth);

        
        storePtr(JSFrameReg, FrameAddress(VMFrame::offsetOfFp));

        
        storePtr(ImmPtr(pc), FrameAddress(VMFrame::offsetOfRegsPc()));

        if (inlining) {
            
            DataLabelPtr ptr = storePtrWithPatch(ImmPtr(NULL),
                                                 FrameAddress(VMFrame::offsetOfInlined));
            if (pinlined)
                *pinlined = ptr;
        }

        restoreStackBase();
    }

    void setupFallibleABICall(bool inlining, jsbytecode *pc, int32 frameDepth) {
        setupFrameDepth(frameDepth);

        
        storePtr(JSFrameReg, FrameAddress(VMFrame::offsetOfFp));
        storePtr(ImmPtr(pc), FrameAddress(VMFrame::offsetOfRegsPc()));

        if (inlining) {
            
            storePtr(ImmPtr(NULL), FrameAddress(VMFrame::offsetOfInlined));
        }
    }

    void restoreStackBase() {
#if defined(JS_CPU_X86)
        




        JS_STATIC_ASSERT(JSFrameReg == JSC::X86Registers::ebp);
        move(JSC::X86Registers::esp, JSFrameReg);
        addPtr(Imm32(VMFrame::STACK_BASE_DIFFERENCE), JSFrameReg);
#endif
    }

    
    
    
    Call infallibleVMCall(void *ptr, int32 frameDepth) {
        setupInfallibleVMFrame(frameDepth);
        return wrapVMCall(ptr);
    }

    
    
    
    Call fallibleVMCall(bool inlining, void *ptr, jsbytecode *pc,
                        DataLabelPtr *pinlined, int32 frameDepth) {
        setupFallibleVMFrame(inlining, pc, pinlined, frameDepth);
        Call call = wrapVMCall(ptr);

        
        loadPtr(FrameAddress(VMFrame::offsetOfFp), JSFrameReg);

        return call;
    }

    Call wrapVMCall(void *ptr) {
        JS_ASSERT(!callIsAligned);

        
        setupABICall(Registers::FastCall, 2);

        
        
        
        
        storeArg(0, Registers::ArgReg0);
        storeArg(1, Registers::ArgReg1);

        
        
        
        
        
        return callWithABI(ptr, true);
    }

    
    
    void slowLoadConstantDouble(double d, FPRegisterID fpreg) {
        DoublePatch patch;
        patch.d = d;
        patch.label = loadDouble(NULL, fpreg);
        doublePatches.append(patch);
    }

    size_t numDoubles() { return doublePatches.length(); }

    void finalize(JSC::LinkBuffer &linker, double *doubleVec = NULL) {
        for (size_t i = 0; i < callPatches.length(); i++) {
            CallPatch &patch = callPatches[i];
            linker.link(patch.call, JSC::FunctionPtr(patch.fun));
        }
        for (size_t i = 0; i < doublePatches.length(); i++) {
            DoublePatch &patch = doublePatches[i];
            doubleVec[i] = patch.d;
            linker.patch(patch.label, &doubleVec[i]);
        }
    }

    struct FastArrayLoadFails {
        Jump rangeCheck;
        Jump holeCheck;
    };

    
    Jump guardArrayExtent(int offset, RegisterID reg,
                          const Int32Key &key, Condition cond) {
        Address extent(reg, offset);
        if (key.isConstant()) {
            JS_ASSERT(key.index() >= 0);
            return branch32(cond, extent, Imm32(key.index()));
        }
        return branch32(cond, extent, key.reg());
    }

    
    FastArrayLoadFails fastArrayLoad(RegisterID objReg, const Int32Key &key,
                                     RegisterID typeReg, RegisterID dataReg) {
        JS_ASSERT(objReg != typeReg);

        RegisterID elementsReg = objReg;
        loadPtr(Address(objReg, JSObject::offsetOfElements()), elementsReg);

        FastArrayLoadFails fails;
        fails.rangeCheck = guardArrayExtent(ObjectElements::offsetOfInitializedLength(),
                                            objReg, key, BelowOrEqual);

        
        if (key.isConstant()) {
            Address slot(elementsReg, key.index() * sizeof(Value));
            fails.holeCheck = fastArrayLoadSlot(slot, true, typeReg, dataReg);
        } else {
            BaseIndex slot(elementsReg, key.reg(), JSVAL_SCALE);
            fails.holeCheck = fastArrayLoadSlot(slot, true, typeReg, dataReg);
        }

        return fails;
    }

    void storeKey(const Int32Key &key, Address address) {
        if (key.isConstant())
            store32(Imm32(key.index()), address);
        else
            store32(key.reg(), address);
    }

    void bumpKey(Int32Key &key, int32 delta) {
        if (key.isConstant())
            key.index_ += delta;
        else
            add32(Imm32(delta), key.reg());
    }

    void loadFrameActuals(JSFunction *fun, RegisterID reg) {
        
        load32(Address(JSFrameReg, StackFrame::offsetOfArgs()), reg);
        add32(Imm32(fun->nargs + 2), reg);
        Jump overflowArgs = branchTest32(Assembler::NonZero,
                                         Address(JSFrameReg, StackFrame::offsetOfFlags()),
                                         Imm32(StackFrame::OVERFLOW_ARGS));
        move(Imm32(fun->nargs), reg);
        overflowArgs.linkTo(label(), this);
        lshiftPtr(Imm32(3), reg);
        negPtr(reg);
        addPtr(JSFrameReg, reg);
    }

    void loadBaseShape(RegisterID obj, RegisterID dest) {
        loadPtr(Address(obj, JSObject::offsetOfShape()), dest);
        loadPtr(Address(dest, Shape::offsetOfBase()), dest);
    }

    void loadObjClass(RegisterID obj, RegisterID dest) {
        loadBaseShape(obj, dest);
        loadPtr(Address(dest, BaseShape::offsetOfClass()), dest);
    }

    Jump testClass(Condition cond, RegisterID claspReg, js::Class *clasp) {
        return branchPtr(cond, claspReg, ImmPtr(clasp));
    }

    Jump testObjClass(Condition cond, RegisterID obj, RegisterID temp, js::Class *clasp) {
        loadBaseShape(obj, temp);
        return branchPtr(cond, Address(temp, BaseShape::offsetOfClass()), ImmPtr(clasp));
    }

    Jump testFunction(Condition cond, RegisterID fun, RegisterID temp) {
        return testObjClass(cond, fun, temp, &js::FunctionClass);
    }

    void branchValue(Condition cond, RegisterID reg, int32 value, RegisterID result)
    {
        if (Registers::maskReg(result) & Registers::SingleByteRegs) {
            set32(cond, reg, Imm32(value), result);
        } else {
            Jump j = branch32(cond, reg, Imm32(value));
            move(Imm32(0), result);
            Jump skip = jump();
            j.linkTo(label(), this);
            move(Imm32(1), result);
            skip.linkTo(label(), this);
        }
    }

    void branchValue(Condition cond, RegisterID lreg, RegisterID rreg, RegisterID result)
    {
        if (Registers::maskReg(result) & Registers::SingleByteRegs) {
            set32(cond, lreg, rreg, result);
        } else {
            Jump j = branch32(cond, lreg, rreg);
            move(Imm32(0), result);
            Jump skip = jump();
            j.linkTo(label(), this);
            move(Imm32(1), result);
            skip.linkTo(label(), this);
        }
    }

    void rematPayload(const StateRemat &remat, RegisterID reg) {
        if (remat.inMemory())
            loadPayload(remat.address(), reg);
        else
            move(remat.reg(), reg);
    }

    void loadDynamicSlot(RegisterID objReg, uint32 index,
                         RegisterID typeReg, RegisterID dataReg) {
        loadPtr(Address(objReg, JSObject::offsetOfSlots()), dataReg);
        loadValueAsComponents(Address(dataReg, index * sizeof(Value)), typeReg, dataReg);
    }

    void loadObjProp(JSObject *obj, RegisterID objReg,
                     const js::Shape *shape,
                     RegisterID typeReg, RegisterID dataReg)
    {
        if (obj->isFixedSlot(shape->slot()))
            loadInlineSlot(objReg, shape->slot(), typeReg, dataReg);
        else
            loadDynamicSlot(objReg, obj->dynamicSlotIndex(shape->slot()), typeReg, dataReg);
    }

#ifdef JS_METHODJIT_TYPED_ARRAY
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template <typename T>
    void loadFromTypedArray(int atype, T address, MaybeRegisterID typeReg,
                            AnyRegisterID dataReg, MaybeRegisterID tempReg)
    {
        
        JS_ASSERT_IF(dataReg.isFPReg(), !typeReg.isSet());

        
        JS_ASSERT_IF(atype != js::TypedArray::TYPE_UINT32 || dataReg.isReg(), !tempReg.isSet());

        switch (atype) {
          case js::TypedArray::TYPE_INT8:
            load8SignExtend(address, dataReg.reg());
            if (typeReg.isSet())
                move(ImmType(JSVAL_TYPE_INT32), typeReg.reg());
            break;
          case js::TypedArray::TYPE_UINT8:
          case js::TypedArray::TYPE_UINT8_CLAMPED:
            load8ZeroExtend(address, dataReg.reg());
            if (typeReg.isSet())
                move(ImmType(JSVAL_TYPE_INT32), typeReg.reg());
            break;
          case js::TypedArray::TYPE_INT16:
            load16SignExtend(address, dataReg.reg());
            if (typeReg.isSet())
                move(ImmType(JSVAL_TYPE_INT32), typeReg.reg());
            break;
          case js::TypedArray::TYPE_UINT16:
            load16(address, dataReg.reg());
            if (typeReg.isSet())
                move(ImmType(JSVAL_TYPE_INT32), typeReg.reg());
            break;
          case js::TypedArray::TYPE_INT32:
            load32(address, dataReg.reg());
            if (typeReg.isSet())
                move(ImmType(JSVAL_TYPE_INT32), typeReg.reg());
            break;
          case js::TypedArray::TYPE_UINT32:
          {
            
            
            
            if (dataReg.isReg()) {
                load32(address, dataReg.reg());
                move(ImmType(JSVAL_TYPE_INT32), typeReg.reg());
                Jump safeInt = branch32(Assembler::Below, dataReg.reg(), Imm32(0x80000000));
                convertUInt32ToDouble(dataReg.reg(), Registers::FPConversionTemp);
                breakDouble(Registers::FPConversionTemp, typeReg.reg(), dataReg.reg());
                safeInt.linkTo(label(), this);
            } else {
                load32(address, tempReg.reg());
                convertUInt32ToDouble(tempReg.reg(), dataReg.fpreg());
            }
            break;
          }
          case js::TypedArray::TYPE_FLOAT32:
          case js::TypedArray::TYPE_FLOAT64:
          {
            FPRegisterID fpreg = dataReg.isReg()
                               ? Registers::FPConversionTemp
                               : dataReg.fpreg();
            if (atype == js::TypedArray::TYPE_FLOAT32)
                loadFloat(address, fpreg);
            else
                loadDouble(address, fpreg);
            
            
            
            Jump notNaN = branchDouble(Assembler::DoubleEqual, fpreg, fpreg);
            if (dataReg.isReg())
                loadStaticDouble(&js_NaN, Registers::FPConversionTemp, dataReg.reg());
            else
                slowLoadConstantDouble(js_NaN, fpreg);
            notNaN.linkTo(label(), this);
            if (dataReg.isReg())
                breakDouble(Registers::FPConversionTemp, typeReg.reg(), dataReg.reg());
            break;
          }
        }
    }

    void loadFromTypedArray(int atype, RegisterID objReg, Int32Key key,
                            MaybeRegisterID typeReg, AnyRegisterID dataReg,
                            MaybeRegisterID tempReg)
    {
        int shift = TypedArray::slotWidth(atype);

        if (key.isConstant()) {
            Address addr(objReg, key.index() * shift);
            loadFromTypedArray(atype, addr, typeReg, dataReg, tempReg);
        } else {
            Assembler::Scale scale = Assembler::TimesOne;
            switch (shift) {
              case 2:
                scale = Assembler::TimesTwo;
                break;
              case 4:
                scale = Assembler::TimesFour;
                break;
              case 8:
                scale = Assembler::TimesEight;
                break;
            }
            BaseIndex addr(objReg, key.reg(), scale);
            loadFromTypedArray(atype, addr, typeReg, dataReg, tempReg);
        }
    }

    template <typename S, typename T>
    void storeToTypedIntArray(int atype, S src, T address)
    {
        switch (atype) {
          case js::TypedArray::TYPE_INT8:
          case js::TypedArray::TYPE_UINT8:
          case js::TypedArray::TYPE_UINT8_CLAMPED:
            store8(src, address);
            break;
          case js::TypedArray::TYPE_INT16:
          case js::TypedArray::TYPE_UINT16:
            store16(src, address);
            break;
          case js::TypedArray::TYPE_INT32:
          case js::TypedArray::TYPE_UINT32:
            store32(src, address);
            break;
          default:
            JS_NOT_REACHED("unknown int array type");
        }
    }

    template <typename S, typename T>
    void storeToTypedFloatArray(int atype, S src, T address)
    {
        if (atype == js::TypedArray::TYPE_FLOAT32)
            storeFloat(src, address);
        else
            storeDouble(src, address);
    }

    template <typename T>
    void storeToTypedArray(int atype, ValueRemat vr, T address)
    {
        if (atype == js::TypedArray::TYPE_FLOAT32 || atype == js::TypedArray::TYPE_FLOAT64) {
            if (vr.isConstant())
                storeToTypedFloatArray(atype, ImmDouble(vr.value().toDouble()), address);
            else
                storeToTypedFloatArray(atype, vr.fpReg(), address);
        } else {
            if (vr.isConstant())
                storeToTypedIntArray(atype, Imm32(vr.value().toInt32()), address);
            else
                storeToTypedIntArray(atype, vr.dataReg(), address);
        }
    }

    void storeToTypedArray(int atype, RegisterID objReg, Int32Key key, ValueRemat vr)
    {
        int shift = TypedArray::slotWidth(atype);
        if (key.isConstant()) {
            Address addr(objReg, key.index() * shift);
            storeToTypedArray(atype, vr, addr);
        } else {
            Assembler::Scale scale = Assembler::TimesOne;
            switch (shift) {
            case 2:
                scale = Assembler::TimesTwo;
                break;
            case 4:
                scale = Assembler::TimesFour;
                break;
            case 8:
                scale = Assembler::TimesEight;
                break;
            }
            BaseIndex addr(objReg, key.reg(), scale);
            storeToTypedArray(atype, vr, addr);
        }
    }

    void clampInt32ToUint8(RegisterID reg)
    {
        Jump j = branch32(Assembler::GreaterThanOrEqual, reg, Imm32(0));
        move(Imm32(0), reg);
        Jump done = jump();
        j.linkTo(label(), this);
        j = branch32(Assembler::LessThanOrEqual, reg, Imm32(255));
        move(Imm32(255), reg);
        j.linkTo(label(), this);
        done.linkTo(label(), this);
    }

    
    void clampDoubleToUint8(FPRegisterID fpReg, FPRegisterID fpTemp, RegisterID reg)
    {
        JS_ASSERT(fpTemp != Registers::FPConversionTemp);

        
        zeroDouble(fpTemp);
        Jump positive = branchDouble(Assembler::DoubleGreaterThan, fpReg, fpTemp);
        move(Imm32(0), reg);
        Jump done1 = jump();

        
        positive.linkTo(label(), this);
        slowLoadConstantDouble(0.5, fpTemp);
        addDouble(fpReg, fpTemp);
        Jump notInt = branchTruncateDoubleToInt32(fpTemp, reg);

        
        Jump inRange = branch32(Assembler::BelowOrEqual, reg, Imm32(255));
        notInt.linkTo(label(), this);
        move(Imm32(255), reg);
        Jump done2 = jump();

        
        inRange.linkTo(label(), this);
        convertInt32ToDouble(reg, Registers::FPConversionTemp);
        Jump done3 = branchDouble(Assembler::DoubleNotEqual, fpTemp, Registers::FPConversionTemp);

        
        
        and32(Imm32(~1), reg);

        done1.linkTo(label(), this);
        done2.linkTo(label(), this);
        done3.linkTo(label(), this);
    }
#endif 

    Address objPropAddress(JSObject *obj, RegisterID objReg, uint32 slot)
    {
        if (obj->isFixedSlot(slot))
            return Address(objReg, JSObject::getFixedSlotOffset(slot));
        loadPtr(Address(objReg, JSObject::offsetOfSlots()), objReg);
        return Address(objReg, obj->dynamicSlotIndex(slot) * sizeof(Value));
    }

    static uint32 maskAddress(Address address) {
        return Registers::maskReg(address.base);
    }

    static uint32 maskAddress(BaseIndex address) {
        return Registers::maskReg(address.base) |
               Registers::maskReg(address.index);
    }

    




    bool generateTypeCheck(JSContext *cx, Address address,
                           types::TypeSet *types, Vector<Jump> *mismatches)
    {
        if (types->unknown())
            return true;

        Vector<Jump> matches(cx);

        if (types->hasType(types::Type::DoubleType())) {
            
            if (!matches.append(testNumber(Assembler::Equal, address)))
                return false;
        } else if (types->hasType(types::Type::Int32Type())) {
            if (!matches.append(testInt32(Assembler::Equal, address)))
                return false;
        }

        if (types->hasType(types::Type::UndefinedType())) {
            if (!matches.append(testUndefined(Assembler::Equal, address)))
                return false;
        }

        if (types->hasType(types::Type::BooleanType())) {
            if (!matches.append(testBoolean(Assembler::Equal, address)))
                return false;
        }

        if (types->hasType(types::Type::StringType())) {
            if (!matches.append(testString(Assembler::Equal, address)))
                return false;
        }

        if (types->hasType(types::Type::NullType())) {
            if (!matches.append(testNull(Assembler::Equal, address)))
                return false;
        }

        unsigned count = 0;
        if (types->hasType(types::Type::AnyObjectType())) {
            if (!matches.append(testObject(Assembler::Equal, address)))
                return false;
        } else {
            count = types->getObjectCount();
        }

        if (count != 0) {
            if (!mismatches->append(testObject(Assembler::NotEqual, address)))
                return false;
            RegisterID reg = Registers::ArgReg1;

            loadPayload(address, reg);

            for (unsigned i = 0; i < count; i++) {
                if (JSObject *object = types->getSingleObject(i)) {
                    if (!matches.append(branchPtr(Assembler::Equal, reg, ImmPtr(object))))
                        return false;
                }
            }

            loadPtr(Address(reg, JSObject::offsetOfType()), reg);

            for (unsigned i = 0; i < count; i++) {
                if (types::TypeObject *object = types->getTypeObject(i)) {
                    if (!matches.append(branchPtr(Assembler::Equal, reg, ImmPtr(object))))
                        return false;
                }
            }
        }

        if (!mismatches->append(jump()))
            return false;

        for (unsigned i = 0; i < matches.length(); i++)
            matches[i].linkTo(label(), this);

        return true;
    }

    




    Jump getNewObject(JSContext *cx, RegisterID result, JSObject *templateObject)
    {
        gc::AllocKind allocKind = templateObject->getAllocKind();

        JS_ASSERT(allocKind >= gc::FINALIZE_OBJECT0 && allocKind <= gc::FINALIZE_OBJECT_LAST);
        int thingSize = (int)gc::Arena::thingSize(allocKind);

        JS_ASSERT(cx->typeInferenceEnabled());
        JS_ASSERT(!templateObject->hasDynamicSlots());
        JS_ASSERT(!templateObject->hasDynamicElements());

#ifdef JS_GC_ZEAL
        if (cx->runtime->needZealousGC())
            return jump();
#endif

        



        gc::FreeSpan *list = const_cast<gc::FreeSpan *>
                             (cx->compartment->arenas.getFreeList(allocKind));
        loadPtr(&list->first, result);

        Jump jump = branchPtr(Assembler::BelowOrEqual, AbsoluteAddress(&list->last), result);

        addPtr(Imm32(thingSize), result);
        storePtr(result, &list->first);

        









        int elementsOffset = JSObject::offsetOfFixedElements();

        




        if (templateObject->isDenseArray()) {
            JS_ASSERT(!templateObject->getDenseArrayInitializedLength());
            addPtr(Imm32(-thingSize + elementsOffset), result);
            storePtr(result, Address(result, -elementsOffset + JSObject::offsetOfElements()));
            addPtr(Imm32(-elementsOffset), result);
        } else {
            addPtr(Imm32(-thingSize), result);
            storePtr(ImmPtr(emptyObjectElements), Address(result, JSObject::offsetOfElements()));
        }

        storePtr(ImmPtr(templateObject->lastProperty()), Address(result, JSObject::offsetOfShape()));
        storePtr(ImmPtr(templateObject->type()), Address(result, JSObject::offsetOfType()));
        storePtr(ImmPtr(NULL), Address(result, JSObject::offsetOfSlots()));

        if (templateObject->isDenseArray()) {
            
            store32(Imm32(templateObject->getDenseArrayCapacity()),
                    Address(result, elementsOffset + ObjectElements::offsetOfCapacity()));
            store32(Imm32(templateObject->getDenseArrayInitializedLength()),
                    Address(result, elementsOffset + ObjectElements::offsetOfInitializedLength()));
            store32(Imm32(templateObject->getArrayLength()),
                    Address(result, elementsOffset + ObjectElements::offsetOfLength()));
        } else {
            



            for (unsigned i = 0; i < templateObject->slotSpan(); i++) {
                storeValue(templateObject->getFixedSlot(i),
                           Address(result, JSObject::getFixedSlotOffset(i)));
            }
        }

        if (templateObject->hasPrivate()) {
            uint32 nfixed = templateObject->numFixedSlots();
            storePtr(ImmPtr(templateObject->getPrivate()),
                     Address(result, JSObject::getPrivateDataOffset(nfixed)));
        }

        return jump;
    }

    
    void addCounter(const double *value, double *counter, RegisterID scratch)
    {
        loadDouble(value, Registers::FPConversionTemp);
        move(ImmPtr(counter), scratch);
        addDouble(Address(scratch), Registers::FPConversionTemp);
        storeDouble(Registers::FPConversionTemp, Address(scratch));
    }

    
    void bumpCounter(double *counter, RegisterID scratch)
    {
        addCounter(&oneDouble, counter, scratch);
    }

    
    void bumpStubCounter(JSScript *script, jsbytecode *pc, RegisterID scratch)
    {
        if (script->pcCounters) {
            OpcodeCounts counts = script->getCounts(pc);
            double *counter = &counts.get(OpcodeCounts::BASE_METHODJIT_STUBS);
            bumpCounter(counter, scratch);
        }
    }

    static const double oneDouble;
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
            RegisterID reg = mask.takeAnyReg().reg();
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

