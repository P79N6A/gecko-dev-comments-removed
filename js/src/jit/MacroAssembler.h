





#ifndef jit_MacroAssembler_h
#define jit_MacroAssembler_h

#include "mozilla/MathAlgorithms.h"

#include "jscompartment.h"

#if defined(JS_CODEGEN_X86)
# include "jit/x86/MacroAssembler-x86.h"
#elif defined(JS_CODEGEN_X64)
# include "jit/x64/MacroAssembler-x64.h"
#elif defined(JS_CODEGEN_ARM)
# include "jit/arm/MacroAssembler-arm.h"
#elif defined(JS_CODEGEN_MIPS)
# include "jit/mips/MacroAssembler-mips.h"
#elif defined(JS_CODEGEN_NONE)
# include "jit/none/MacroAssembler-none.h"
#else
# error "Unknown architecture!"
#endif
#include "jit/AtomicOp.h"
#include "jit/IonInstrumentation.h"
#include "jit/JitCompartment.h"
#include "jit/VMFunctions.h"
#include "vm/ProxyObject.h"
#include "vm/Shape.h"
#include "vm/UnboxedObject.h"





# define PER_ARCH

#if defined(JS_CODEGEN_X86)
# define ONLY_X86_X64
#elif defined(JS_CODEGEN_X64)
# define ONLY_X86_X64
#elif defined(JS_CODEGEN_ARM)
# define ONLY_X86_X64 = delete
#elif defined(JS_CODEGEN_MIPS)
# define ONLY_X86_X64 = delete
#elif defined(JS_CODEGEN_NONE)
# define ONLY_X86_X64 = delete
#else
# error "Unknown architecture!"
#endif

#ifdef IS_LITTLE_ENDIAN
#define IMM32_16ADJ(X) X << 16
#else
#define IMM32_16ADJ(X) X
#endif

namespace js {
namespace jit {




class MacroAssembler : public MacroAssemblerSpecific
{
    MacroAssembler* thisFromCtor() {
        return this;
    }

  public:
    class AutoRooter : public JS::AutoGCRooter
    {
        MacroAssembler* masm_;

      public:
        AutoRooter(JSContext* cx, MacroAssembler* masm)
          : JS::AutoGCRooter(cx, IONMASM),
            masm_(masm)
        { }

        MacroAssembler* masm() const {
            return masm_;
        }
    };

    


    class Branch
    {
        bool init_;
        Condition cond_;
        Label* jump_;
        Register reg_;

      public:
        Branch()
          : init_(false),
            cond_(Equal),
            jump_(nullptr),
            reg_(Register::FromCode(0))      
        { }

        Branch(Condition cond, Register reg, Label* jump)
          : init_(true),
            cond_(cond),
            jump_(jump),
            reg_(reg)
        { }

        bool isInitialized() const {
            return init_;
        }

        Condition cond() const {
            return cond_;
        }

        Label* jump() const {
            return jump_;
        }

        Register reg() const {
            return reg_;
        }

        void invertCondition() {
            cond_ = InvertCondition(cond_);
        }

        void relink(Label* jump) {
            jump_ = jump;
        }

        virtual void emit(MacroAssembler& masm) = 0;
    };

    



    class BranchType : public Branch
    {
        TypeSet::Type type_;

      public:
        BranchType()
          : Branch(),
            type_(TypeSet::UnknownType())
        { }

        BranchType(Condition cond, Register reg, TypeSet::Type type, Label* jump)
          : Branch(cond, reg, jump),
            type_(type)
        { }

        void emit(MacroAssembler& masm) {
            MOZ_ASSERT(isInitialized());
            MIRType mirType = MIRType_None;

            if (type_.isPrimitive()) {
                if (type_.isMagicArguments())
                    mirType = MIRType_MagicOptimizedArguments;
                else
                    mirType = MIRTypeFromValueType(type_.primitive());
            } else if (type_.isAnyObject()) {
                mirType = MIRType_Object;
            } else {
                MOZ_CRASH("Unknown conversion to mirtype");
            }

            if (mirType == MIRType_Double)
                masm.branchTestNumber(cond(), reg(), jump());
            else
                masm.branchTestMIRType(cond(), reg(), mirType, jump());
        }

    };

    


    class BranchGCPtr : public Branch
    {
        ImmGCPtr ptr_;

      public:
        BranchGCPtr()
          : Branch(),
            ptr_(ImmGCPtr(nullptr))
        { }

        BranchGCPtr(Condition cond, Register reg, ImmGCPtr ptr, Label* jump)
          : Branch(cond, reg, jump),
            ptr_(ptr)
        { }

        void emit(MacroAssembler& masm) {
            MOZ_ASSERT(isInitialized());
            masm.branchPtr(cond(), reg(), ptr_, jump());
        }
    };

    mozilla::Maybe<AutoRooter> autoRooter_;
    mozilla::Maybe<JitContext> jitContext_;
    mozilla::Maybe<AutoJitContextAlloc> alloc_;

  private:
    
    
    
    bool emitProfilingInstrumentation_;

    
    NonAssertingLabel failureLabel_;

  public:
    MacroAssembler()
      : emitProfilingInstrumentation_(false)
    {
        JitContext* jcx = GetJitContext();
        JSContext* cx = jcx->cx;
        if (cx)
            constructRoot(cx);

        if (!jcx->temp) {
            MOZ_ASSERT(cx);
            alloc_.emplace(cx);
        }

        moveResolver_.setAllocator(*jcx->temp);
#ifdef JS_CODEGEN_ARM
        initWithAllocator();
        m_buffer.id = jcx->getNextAssemblerId();
#endif
    }

    
    
    explicit MacroAssembler(JSContext* cx, IonScript* ion = nullptr,
                            JSScript* script = nullptr, jsbytecode* pc = nullptr)
      : emitProfilingInstrumentation_(false)
    {
        constructRoot(cx);
        jitContext_.emplace(cx, (js::jit::TempAllocator*)nullptr);
        alloc_.emplace(cx);
        moveResolver_.setAllocator(*jitContext_->temp);
#ifdef JS_CODEGEN_ARM
        initWithAllocator();
        m_buffer.id = GetJitContext()->getNextAssemblerId();
#endif
        if (ion) {
            setFramePushed(ion->frameSize());
            if (pc && cx->runtime()->spsProfiler.enabled())
                emitProfilingInstrumentation_ = true;
        }
    }

    
    struct AsmJSToken {};
    explicit MacroAssembler(AsmJSToken)
      : emitProfilingInstrumentation_(false)
    {
#ifdef JS_CODEGEN_ARM
        initWithAllocator();
        m_buffer.id = 0;
#endif
    }

    void enableProfilingInstrumentation() {
        emitProfilingInstrumentation_ = true;
    }

    void resetForNewCodeGenerator(TempAllocator& alloc) {
        setFramePushed(0);
        moveResolver_.clearTempObjectPool();
        moveResolver_.setAllocator(alloc);
    }

    void constructRoot(JSContext* cx) {
        autoRooter_.emplace(cx, this);
    }

    MoveResolver& moveResolver() {
        return moveResolver_;
    }

    size_t instructionsSize() const {
        return size();
    }

  public:
    
    

    void PushRegsInMask(LiveRegisterSet set) PER_ARCH;
    void PushRegsInMask(LiveGeneralRegisterSet set);

    void PopRegsInMask(LiveRegisterSet set);
    void PopRegsInMask(LiveGeneralRegisterSet set);
    void PopRegsInMaskIgnore(LiveRegisterSet set, LiveRegisterSet ignore) PER_ARCH;

    void Push(const Operand op) PER_ARCH ONLY_X86_X64;
    void Push(Register reg) PER_ARCH;
    void Push(const Imm32 imm) PER_ARCH;
    void Push(const ImmWord imm) PER_ARCH;
    void Push(const ImmPtr imm) PER_ARCH;
    void Push(const ImmGCPtr ptr) PER_ARCH;
    void Push(FloatRegister reg) PER_ARCH;
    void Push(jsid id, Register scratchReg);
    void Push(TypedOrValueRegister v);
    void Push(ConstantOrRegister v);
    void Push(const ValueOperand& val);
    void Push(const Value& val);
    void Push(JSValueType type, Register reg);
    void PushValue(const Address& addr);
    void PushEmptyRooted(VMFunction::RootType rootType);

    void Pop(const Operand op) PER_ARCH ONLY_X86_X64;
    void Pop(Register reg) PER_ARCH;
    void Pop(FloatRegister t) PER_ARCH ONLY_X86_X64;
    void Pop(const ValueOperand& val) PER_ARCH;
    void popRooted(VMFunction::RootType rootType, Register cellReg, const ValueOperand& valueReg);

    void adjustStack(int amount);

  public:

    
    
    template <typename Source, typename TypeSet>
    void guardTypeSet(const Source& address, const TypeSet* types, BarrierKind kind, Register scratch, Label* miss);
    template <typename TypeSet>
    void guardObjectType(Register obj, const TypeSet* types, Register scratch, Label* miss);
    template <typename Source>
    void guardType(const Source& address, TypeSet::Type type, Register scratch, Label* miss);

    void guardTypeSetMightBeIncomplete(Register obj, Register scratch, Label* label);

    void loadObjShape(Register objReg, Register dest) {
        loadPtr(Address(objReg, JSObject::offsetOfShape()), dest);
    }
    void loadObjGroup(Register objReg, Register dest) {
        loadPtr(Address(objReg, JSObject::offsetOfGroup()), dest);
    }
    void loadBaseShape(Register objReg, Register dest) {
        loadObjShape(objReg, dest);
        loadPtr(Address(dest, Shape::offsetOfBase()), dest);
    }
    void loadObjClass(Register objReg, Register dest) {
        loadObjGroup(objReg, dest);
        loadPtr(Address(dest, ObjectGroup::offsetOfClasp()), dest);
    }
    void branchTestObjClass(Condition cond, Register obj, Register scratch, const js::Class* clasp,
                            Label* label) {
        loadObjGroup(obj, scratch);
        branchPtr(cond, Address(scratch, ObjectGroup::offsetOfClasp()), ImmPtr(clasp), label);
    }
    void branchTestObjShape(Condition cond, Register obj, const Shape* shape, Label* label) {
        branchPtr(cond, Address(obj, JSObject::offsetOfShape()), ImmGCPtr(shape), label);
    }
    void branchTestObjShape(Condition cond, Register obj, Register shape, Label* label) {
        branchPtr(cond, Address(obj, JSObject::offsetOfShape()), shape, label);
    }
    void branchTestObjGroup(Condition cond, Register obj, ObjectGroup* group, Label* label) {
        branchPtr(cond, Address(obj, JSObject::offsetOfGroup()), ImmGCPtr(group), label);
    }
    void branchTestObjGroup(Condition cond, Register obj, Register group, Label* label) {
        branchPtr(cond, Address(obj, JSObject::offsetOfGroup()), group, label);
    }
    void branchTestProxyHandlerFamily(Condition cond, Register proxy, Register scratch,
                                      const void* handlerp, Label* label) {
        Address handlerAddr(proxy, ProxyObject::offsetOfHandler());
        loadPtr(handlerAddr, scratch);
        Address familyAddr(scratch, BaseProxyHandler::offsetOfFamily());
        branchPtr(cond, familyAddr, ImmPtr(handlerp), label);
    }

    template <typename Value>
    void branchTestMIRType(Condition cond, const Value& val, MIRType type, Label* label) {
        switch (type) {
          case MIRType_Null:      return branchTestNull(cond, val, label);
          case MIRType_Undefined: return branchTestUndefined(cond, val, label);
          case MIRType_Boolean:   return branchTestBoolean(cond, val, label);
          case MIRType_Int32:     return branchTestInt32(cond, val, label);
          case MIRType_String:    return branchTestString(cond, val, label);
          case MIRType_Symbol:    return branchTestSymbol(cond, val, label);
          case MIRType_Object:    return branchTestObject(cond, val, label);
          case MIRType_Double:    return branchTestDouble(cond, val, label);
          case MIRType_MagicOptimizedArguments: 
          case MIRType_MagicIsConstructing:
          case MIRType_MagicHole: return branchTestMagic(cond, val, label);
          default:
            MOZ_CRASH("Bad MIRType");
        }
    }

    
    void branchIfFalseBool(Register reg, Label* label) {
        
        branchTest32(Assembler::Zero, reg, Imm32(0xFF), label);
    }

    
    void branchIfTrueBool(Register reg, Label* label) {
        
        branchTest32(Assembler::NonZero, reg, Imm32(0xFF), label);
    }

    void loadObjPrivate(Register obj, uint32_t nfixed, Register dest) {
        loadPtr(Address(obj, NativeObject::getPrivateDataOffset(nfixed)), dest);
    }

    void loadObjProto(Register obj, Register dest) {
        loadPtr(Address(obj, JSObject::offsetOfGroup()), dest);
        loadPtr(Address(dest, ObjectGroup::offsetOfProto()), dest);
    }

    void loadStringLength(Register str, Register dest) {
        load32(Address(str, JSString::offsetOfLength()), dest);
    }

    void loadFunctionFromCalleeToken(Address token, Register dest) {
        loadPtr(token, dest);
        andPtr(Imm32(uint32_t(CalleeTokenMask)), dest);
    }
    void PushCalleeToken(Register callee, bool constructing) {
        if (constructing) {
            orPtr(Imm32(CalleeToken_FunctionConstructing), callee);
            Push(callee);
            andPtr(Imm32(uint32_t(CalleeTokenMask)), callee);
        } else {
            static_assert(CalleeToken_Function == 0, "Non-constructing call requires no tagging");
            Push(callee);
        }
    }

    void loadStringChars(Register str, Register dest);
    void loadStringChar(Register str, Register index, Register output);

    void branchIfRope(Register str, Label* label) {
        Address flags(str, JSString::offsetOfFlags());
        static_assert(JSString::ROPE_FLAGS == 0, "Rope type flags must be 0");
        branchTest32(Assembler::Zero, flags, Imm32(JSString::TYPE_FLAGS_MASK), label);
    }

    void loadJSContext(Register dest) {
        loadPtr(AbsoluteAddress(GetJitContext()->runtime->addressOfJSContext()), dest);
    }
    void loadJitActivation(Register dest) {
        loadPtr(AbsoluteAddress(GetJitContext()->runtime->addressOfActivation()), dest);
    }

    template<typename T>
    void loadTypedOrValue(const T& src, TypedOrValueRegister dest) {
        if (dest.hasValue())
            loadValue(src, dest.valueReg());
        else
            loadUnboxedValue(src, dest.type(), dest.typedReg());
    }

    template<typename T>
    void loadElementTypedOrValue(const T& src, TypedOrValueRegister dest, bool holeCheck,
                                 Label* hole) {
        if (dest.hasValue()) {
            loadValue(src, dest.valueReg());
            if (holeCheck)
                branchTestMagic(Assembler::Equal, dest.valueReg(), hole);
        } else {
            if (holeCheck)
                branchTestMagic(Assembler::Equal, src, hole);
            loadUnboxedValue(src, dest.type(), dest.typedReg());
        }
    }

    template <typename T>
    void storeTypedOrValue(TypedOrValueRegister src, const T& dest) {
        if (src.hasValue()) {
            storeValue(src.valueReg(), dest);
        } else if (IsFloatingPointType(src.type())) {
            FloatRegister reg = src.typedReg().fpu();
            if (src.type() == MIRType_Float32) {
                convertFloat32ToDouble(reg, ScratchDoubleReg);
                reg = ScratchDoubleReg;
            }
            storeDouble(reg, dest);
        } else {
            storeValue(ValueTypeFromMIRType(src.type()), src.typedReg().gpr(), dest);
        }
    }

    template <typename T>
    void storeObjectOrNull(Register src, const T& dest) {
        Label notNull, done;
        branchTestPtr(Assembler::NonZero, src, src, &notNull);
        storeValue(NullValue(), dest);
        jump(&done);
        bind(&notNull);
        storeValue(JSVAL_TYPE_OBJECT, src, dest);
        bind(&done);
    }

    template <typename T>
    void storeConstantOrRegister(ConstantOrRegister src, const T& dest) {
        if (src.constant())
            storeValue(src.value(), dest);
        else
            storeTypedOrValue(src.reg(), dest);
    }

    void storeCallResult(Register reg) {
        if (reg != ReturnReg)
            mov(ReturnReg, reg);
    }

    void storeCallFloatResult(FloatRegister reg) {
        if (reg != ReturnDoubleReg)
            moveDouble(ReturnDoubleReg, reg);
    }

    void storeCallResultValue(AnyRegister dest) {
#if defined(JS_NUNBOX32)
        unboxValue(ValueOperand(JSReturnReg_Type, JSReturnReg_Data), dest);
#elif defined(JS_PUNBOX64)
        unboxValue(ValueOperand(JSReturnReg), dest);
#else
#error "Bad architecture"
#endif
    }

    void storeCallResultValue(ValueOperand dest) {
#if defined(JS_NUNBOX32)
        
        
        
        
        
        if (dest.typeReg() == JSReturnReg_Data) {
            if (dest.payloadReg() == JSReturnReg_Type) {
                
                mov(JSReturnReg_Type, ReturnReg);
                mov(JSReturnReg_Data, JSReturnReg_Type);
                mov(ReturnReg, JSReturnReg_Data);
            } else {
                mov(JSReturnReg_Data, dest.payloadReg());
                mov(JSReturnReg_Type, dest.typeReg());
            }
        } else {
            mov(JSReturnReg_Type, dest.typeReg());
            mov(JSReturnReg_Data, dest.payloadReg());
        }
#elif defined(JS_PUNBOX64)
        if (dest.valueReg() != JSReturnReg)
            movq(JSReturnReg, dest.valueReg());
#else
#error "Bad architecture"
#endif
    }

    void storeCallResultValue(TypedOrValueRegister dest) {
        if (dest.hasValue())
            storeCallResultValue(dest.valueReg());
        else
            storeCallResultValue(dest.typedReg());
    }

    template <typename T>
    Register extractString(const T& source, Register scratch) {
        return extractObject(source, scratch);
    }

    void branchIfFunctionHasNoScript(Register fun, Label* label) {
        
        
        MOZ_ASSERT(JSFunction::offsetOfNargs() % sizeof(uint32_t) == 0);
        MOZ_ASSERT(JSFunction::offsetOfFlags() == JSFunction::offsetOfNargs() + 2);
        Address address(fun, JSFunction::offsetOfNargs());
        int32_t bit = IMM32_16ADJ(JSFunction::INTERPRETED);
        branchTest32(Assembler::Zero, address, Imm32(bit), label);
    }
    void branchIfInterpreted(Register fun, Label* label) {
        
        
        MOZ_ASSERT(JSFunction::offsetOfNargs() % sizeof(uint32_t) == 0);
        MOZ_ASSERT(JSFunction::offsetOfFlags() == JSFunction::offsetOfNargs() + 2);
        Address address(fun, JSFunction::offsetOfNargs());
        int32_t bit = IMM32_16ADJ(JSFunction::INTERPRETED);
        branchTest32(Assembler::NonZero, address, Imm32(bit), label);
    }

    void branchIfNotInterpretedConstructor(Register fun, Register scratch, Label* label);

    void bumpKey(Int32Key* key, int diff) {
        if (key->isRegister())
            add32(Imm32(diff), key->reg());
        else
            key->bumpConstant(diff);
    }

    void storeKey(const Int32Key& key, const Address& dest) {
        if (key.isRegister())
            store32(key.reg(), dest);
        else
            store32(Imm32(key.constant()), dest);
    }

    template<typename T>
    void branchKey(Condition cond, const T& length, const Int32Key& key, Label* label) {
        if (key.isRegister())
            branch32(cond, length, key.reg(), label);
        else
            branch32(cond, length, Imm32(key.constant()), label);
    }

    void branchTestNeedsIncrementalBarrier(Condition cond, Label* label) {
        MOZ_ASSERT(cond == Zero || cond == NonZero);
        CompileZone* zone = GetJitContext()->compartment->zone();
        AbsoluteAddress needsBarrierAddr(zone->addressOfNeedsIncrementalBarrier());
        branchTest32(cond, needsBarrierAddr, Imm32(0x1), label);
    }

    template <typename T>
    void callPreBarrier(const T& address, MIRType type) {
        Label done;

        if (type == MIRType_Value)
            branchTestGCThing(Assembler::NotEqual, address, &done);

        Push(PreBarrierReg);
        computeEffectiveAddress(address, PreBarrierReg);

        const JitRuntime* rt = GetJitContext()->runtime->jitRuntime();
        JitCode* preBarrier = rt->preBarrier(type);

        call(preBarrier);
        Pop(PreBarrierReg);

        bind(&done);
    }

    template <typename T>
    void patchableCallPreBarrier(const T& address, MIRType type) {
        Label done;

        
        
        CodeOffsetLabel nopJump = toggledJump(&done);
        writePrebarrierOffset(nopJump);

        callPreBarrier(address, type);
        jump(&done);

        haltingAlign(8);
        bind(&done);
    }

    void canonicalizeDouble(FloatRegister reg) {
        Label notNaN;
        branchDouble(DoubleOrdered, reg, reg, &notNaN);
        loadConstantDouble(JS::GenericNaN(), reg);
        bind(&notNaN);
    }

    void canonicalizeFloat(FloatRegister reg) {
        Label notNaN;
        branchFloat(DoubleOrdered, reg, reg, &notNaN);
        loadConstantFloat32(float(JS::GenericNaN()), reg);
        bind(&notNaN);
    }

    template<typename T>
    void loadFromTypedArray(Scalar::Type arrayType, const T& src, AnyRegister dest, Register temp, Label* fail,
                            bool canonicalizeDoubles = true, unsigned numElems = 0);

    template<typename T>
    void loadFromTypedArray(Scalar::Type arrayType, const T& src, const ValueOperand& dest, bool allowDouble,
                            Register temp, Label* fail);

    template<typename S, typename T>
    void storeToTypedIntArray(Scalar::Type arrayType, const S& value, const T& dest) {
        switch (arrayType) {
          case Scalar::Int8:
          case Scalar::Uint8:
          case Scalar::Uint8Clamped:
            store8(value, dest);
            break;
          case Scalar::Int16:
          case Scalar::Uint16:
            store16(value, dest);
            break;
          case Scalar::Int32:
          case Scalar::Uint32:
            store32(value, dest);
            break;
          default:
            MOZ_CRASH("Invalid typed array type");
        }
    }

    template<typename T>
    void compareExchangeToTypedIntArray(Scalar::Type arrayType, const T& mem, Register oldval, Register newval,
                                        Register temp, AnyRegister output);

    
    template<typename S, typename T>
    void atomicBinopToTypedIntArray(AtomicOp op, Scalar::Type arrayType, const S& value,
                                    const T& mem, Register temp1, Register temp2, AnyRegister output);

    
    template<typename S, typename T>
    void atomicBinopToTypedIntArray(AtomicOp op, Scalar::Type arrayType, const S& value, const T& mem);

    void storeToTypedFloatArray(Scalar::Type arrayType, FloatRegister value, const BaseIndex& dest,
                                unsigned numElems = 0);
    void storeToTypedFloatArray(Scalar::Type arrayType, FloatRegister value, const Address& dest,
                                unsigned numElems = 0);

    
    template <typename T>
    void loadUnboxedProperty(T address, JSValueType type, TypedOrValueRegister output);

    
    
    
    template <typename T>
    void storeUnboxedProperty(T address, JSValueType type,
                              ConstantOrRegister value, Label* failure);

    Register extractString(const Address& address, Register scratch) {
        return extractObject(address, scratch);
    }
    Register extractString(const ValueOperand& value, Register scratch) {
        return extractObject(value, scratch);
    }

    using MacroAssemblerSpecific::extractTag;
    Register extractTag(const TypedOrValueRegister& reg, Register scratch) {
        if (reg.hasValue())
            return extractTag(reg.valueReg(), scratch);
        mov(ImmWord(MIRTypeToTag(reg.type())), scratch);
        return scratch;
    }

    using MacroAssemblerSpecific::extractObject;
    Register extractObject(const TypedOrValueRegister& reg, Register scratch) {
        if (reg.hasValue())
            return extractObject(reg.valueReg(), scratch);
        MOZ_ASSERT(reg.type() == MIRType_Object);
        return reg.typedReg().gpr();
    }

    
    
    void clampDoubleToUint8(FloatRegister input, Register output);

    using MacroAssemblerSpecific::ensureDouble;

    template <typename S>
    void ensureDouble(const S& source, FloatRegister dest, Label* failure) {
        Label isDouble, done;
        branchTestDouble(Assembler::Equal, source, &isDouble);
        branchTestInt32(Assembler::NotEqual, source, failure);

        convertInt32ToDouble(source, dest);
        jump(&done);

        bind(&isDouble);
        unboxDouble(source, dest);

        bind(&done);
    }

    
    
    void branchEqualTypeIfNeeded(MIRType type, MDefinition* maybeDef, Register tag, Label* label);

    
  private:
    void checkAllocatorState(Label* fail);
    bool shouldNurseryAllocate(gc::AllocKind allocKind, gc::InitialHeap initialHeap);
    void nurseryAllocate(Register result, Register temp, gc::AllocKind allocKind,
                         size_t nDynamicSlots, gc::InitialHeap initialHeap, Label* fail);
    void freeListAllocate(Register result, Register temp, gc::AllocKind allocKind, Label* fail);
    void allocateObject(Register result, Register temp, gc::AllocKind allocKind,
                        uint32_t nDynamicSlots, gc::InitialHeap initialHeap, Label* fail);
    void allocateNonObject(Register result, Register temp, gc::AllocKind allocKind, Label* fail);
    void copySlotsFromTemplate(Register obj, const NativeObject* templateObj,
                               uint32_t start, uint32_t end);
    void fillSlotsWithConstantValue(Address addr, Register temp, uint32_t start, uint32_t end,
                                    const Value& v);
    void fillSlotsWithUndefined(Address addr, Register temp, uint32_t start, uint32_t end);
    void fillSlotsWithUninitialized(Address addr, Register temp, uint32_t start, uint32_t end);
    void initGCSlots(Register obj, Register temp, NativeObject* templateObj, bool initContents);

  public:
    void callMallocStub(size_t nbytes, Register result, Label* fail);
    void callFreeStub(Register slots);
    void createGCObject(Register result, Register temp, JSObject* templateObj,
                        gc::InitialHeap initialHeap, Label* fail, bool initContents = true,
                        bool convertDoubleElements = false);

    void newGCThing(Register result, Register temp, JSObject* templateObj,
                     gc::InitialHeap initialHeap, Label* fail);
    void initGCThing(Register obj, Register temp, JSObject* templateObj,
                     bool initContents = true, bool convertDoubleElements = false);

    void initUnboxedObjectContents(Register object, UnboxedPlainObject* templateObject);

    void newGCString(Register result, Register temp, Label* fail);
    void newGCFatInlineString(Register result, Register temp, Label* fail);

    
    
    void compareStrings(JSOp op, Register left, Register right, Register result,
                        Label* fail);

    
    
    
  private:
    CodeOffsetLabel exitCodePatch_;

  private:
    void linkExitFrame();

  public:
    void PushStubCode() {
        exitCodePatch_ = PushWithPatch(ImmWord(-1));
    }

    void enterExitFrame(const VMFunction* f = nullptr) {
        linkExitFrame();
        
        PushStubCode();
        
        Push(ImmPtr(f));
    }

    
    
    void enterFakeExitFrame(JitCode* codeVal) {
        linkExitFrame();
        Push(ImmPtr(codeVal));
        Push(ImmPtr(nullptr));
    }

    void leaveExitFrame(size_t extraFrame = 0) {
        freeStack(ExitFooterFrame::Size() + extraFrame);
    }

    bool hasEnteredExitFrame() const {
        return exitCodePatch_.offset() != 0;
    }

    
    void generateBailoutTail(Register scratch, Register bailoutInfo);

    
    
    
    
    
    

    template <typename T>
    void callWithABI(const T& fun, MoveOp::Type result = MoveOp::GENERAL) {
        profilerPreCall();
        MacroAssemblerSpecific::callWithABI(fun, result);
        profilerPostReturn();
    }

    
    uint32_t callJit(Register callee) {
        profilerPreCall();
        MacroAssemblerSpecific::callJit(callee);
        uint32_t ret = currentOffset();
        profilerPostReturn();
        return ret;
    }

    
    uint32_t callWithExitFrame(Label* target) {
        profilerPreCall();
        MacroAssemblerSpecific::callWithExitFrame(target);
        uint32_t ret = currentOffset();
        profilerPostReturn();
        return ret;
    }

    
    uint32_t callWithExitFrame(JitCode* target) {
        profilerPreCall();
        MacroAssemblerSpecific::callWithExitFrame(target);
        uint32_t ret = currentOffset();
        profilerPostReturn();
        return ret;
    }

    
    uint32_t callWithExitFrame(JitCode* target, Register dynStack) {
        profilerPreCall();
        MacroAssemblerSpecific::callWithExitFrame(target, dynStack);
        uint32_t ret = currentOffset();
        profilerPostReturn();
        return ret;
    }

    void branchTestObjectTruthy(bool truthy, Register objReg, Register scratch,
                                Label* slowCheck, Label* checked)
    {
        
        
        
        loadObjClass(objReg, scratch);
        Address flags(scratch, Class::offsetOfFlags());

        branchTestClassIsProxy(true, scratch, slowCheck);

        Condition cond = truthy ? Assembler::Zero : Assembler::NonZero;
        branchTest32(cond, flags, Imm32(JSCLASS_EMULATES_UNDEFINED), checked);
    }

    void branchTestClassIsProxy(bool proxy, Register clasp, Label* label)
    {
        branchTest32(proxy ? Assembler::NonZero : Assembler::Zero,
                     Address(clasp, Class::offsetOfFlags()),
                     Imm32(JSCLASS_IS_PROXY), label);
    }

    void branchTestObjectIsProxy(bool proxy, Register object, Register scratch, Label* label)
    {
        loadObjClass(object, scratch);
        branchTestClassIsProxy(proxy, scratch, label);
    }

  public:
#ifndef JS_CODEGEN_ARM64
    
    
    
    template <typename T>
    void addToStackPtr(T t) { addPtr(t, getStackPointer()); }
    template <typename T>
    void addStackPtrTo(T t) { addPtr(getStackPointer(), t); }

    template <typename T>
    void subFromStackPtr(T t) { subPtr(t, getStackPointer()); }
    template <typename T>
    void subStackPtrFrom(T t) { subPtr(getStackPointer(), t); }

    template <typename T>
    void andToStackPtr(T t) { andPtr(t, getStackPointer()); }
    template <typename T>
    void andStackPtrTo(T t) { andPtr(getStackPointer(), t); }

    template <typename T>
    void moveToStackPtr(T t) { movePtr(t, getStackPointer()); }
    template <typename T>
    void moveStackPtrTo(T t) { movePtr(getStackPointer(), t); }

    template <typename T>
    void loadStackPtr(T t) { loadPtr(t, getStackPointer()); }
    template <typename T>
    void storeStackPtr(T t) { storePtr(getStackPointer(), t); }

    
    
    
    template <typename T>
    void branchTestStackPtr(Condition cond, T t, Label* label) {
        branchTestPtr(cond, getStackPointer(), t, label);
    }
    template <typename T>
    void branchStackPtr(Condition cond, T rhs, Label* label) {
        branchPtr(cond, getStackPointer(), rhs, label);
    }
    template <typename T>
    void branchStackPtrRhs(Condition cond, T lhs, Label* label) {
        branchPtr(cond, lhs, getStackPointer(), label);
    }
#endif 

  private:
    
    
    
    void profilerPreCall() {
        if (!emitProfilingInstrumentation_)
            return;
        profilerPreCallImpl();
    }

    void profilerPostReturn() {
        if (!emitProfilingInstrumentation_)
            return;
        profilerPostReturnImpl();
    }

  public:
    void loadBaselineOrIonRaw(Register script, Register dest, Label* failure);
    void loadBaselineOrIonNoArgCheck(Register callee, Register dest, Label* failure);

    void loadBaselineFramePtr(Register framePtr, Register dest);

    void pushBaselineFramePtr(Register framePtr, Register scratch) {
        loadBaselineFramePtr(framePtr, scratch);
        push(scratch);
    }

  private:
    void handleFailure();

  public:
    Label* exceptionLabel() {
        
        return &failureLabel_;
    }

    Label* failureLabel() {
        return &failureLabel_;
    }

    void finish();
    void link(JitCode* code);

    void assumeUnreachable(const char* output);

    template<typename T>
    void assertTestInt32(Condition cond, const T& value, const char* output);

    void printf(const char* output);
    void printf(const char* output, Register value);

#ifdef JS_TRACE_LOGGING
    void tracelogStartId(Register logger, uint32_t textId, bool force = false);
    void tracelogStartId(Register logger, Register textId);
    void tracelogStartEvent(Register logger, Register event);
    void tracelogStopId(Register logger, uint32_t textId, bool force = false);
    void tracelogStopId(Register logger, Register textId);
#endif

#define DISPATCH_FLOATING_POINT_OP(method, type, arg1d, arg1f, arg2)    \
    MOZ_ASSERT(IsFloatingPointType(type));                              \
    if (type == MIRType_Double)                                         \
        method##Double(arg1d, arg2);                                    \
    else                                                                \
        method##Float32(arg1f, arg2);                                   \

    void loadConstantFloatingPoint(double d, float f, FloatRegister dest, MIRType destType) {
        DISPATCH_FLOATING_POINT_OP(loadConstant, destType, d, f, dest);
    }
    void boolValueToFloatingPoint(ValueOperand value, FloatRegister dest, MIRType destType) {
        DISPATCH_FLOATING_POINT_OP(boolValueTo, destType, value, value, dest);
    }
    void int32ValueToFloatingPoint(ValueOperand value, FloatRegister dest, MIRType destType) {
        DISPATCH_FLOATING_POINT_OP(int32ValueTo, destType, value, value, dest);
    }
    void convertInt32ToFloatingPoint(Register src, FloatRegister dest, MIRType destType) {
        DISPATCH_FLOATING_POINT_OP(convertInt32To, destType, src, src, dest);
    }

#undef DISPATCH_FLOATING_POINT_OP

    void convertValueToFloatingPoint(ValueOperand value, FloatRegister output, Label* fail,
                                     MIRType outputType);
    bool convertValueToFloatingPoint(JSContext* cx, const Value& v, FloatRegister output,
                                     Label* fail, MIRType outputType);
    bool convertConstantOrRegisterToFloatingPoint(JSContext* cx, ConstantOrRegister src,
                                                  FloatRegister output, Label* fail,
                                                  MIRType outputType);
    void convertTypedOrValueToFloatingPoint(TypedOrValueRegister src, FloatRegister output,
                                            Label* fail, MIRType outputType);

    void convertInt32ValueToDouble(const Address& address, Register scratch, Label* done);
    void convertValueToDouble(ValueOperand value, FloatRegister output, Label* fail) {
        convertValueToFloatingPoint(value, output, fail, MIRType_Double);
    }
    bool convertValueToDouble(JSContext* cx, const Value& v, FloatRegister output, Label* fail) {
        return convertValueToFloatingPoint(cx, v, output, fail, MIRType_Double);
    }
    bool convertConstantOrRegisterToDouble(JSContext* cx, ConstantOrRegister src,
                                           FloatRegister output, Label* fail)
    {
        return convertConstantOrRegisterToFloatingPoint(cx, src, output, fail, MIRType_Double);
    }
    void convertTypedOrValueToDouble(TypedOrValueRegister src, FloatRegister output, Label* fail) {
        convertTypedOrValueToFloatingPoint(src, output, fail, MIRType_Double);
    }

    void convertValueToFloat(ValueOperand value, FloatRegister output, Label* fail) {
        convertValueToFloatingPoint(value, output, fail, MIRType_Float32);
    }
    bool convertValueToFloat(JSContext* cx, const Value& v, FloatRegister output, Label* fail) {
        return convertValueToFloatingPoint(cx, v, output, fail, MIRType_Float32);
    }
    bool convertConstantOrRegisterToFloat(JSContext* cx, ConstantOrRegister src,
                                          FloatRegister output, Label* fail)
    {
        return convertConstantOrRegisterToFloatingPoint(cx, src, output, fail, MIRType_Float32);
    }
    void convertTypedOrValueToFloat(TypedOrValueRegister src, FloatRegister output, Label* fail) {
        convertTypedOrValueToFloatingPoint(src, output, fail, MIRType_Float32);
    }

    enum IntConversionBehavior {
        IntConversion_Normal,
        IntConversion_NegativeZeroCheck,
        IntConversion_Truncate,
        IntConversion_ClampToUint8,
    };

    enum IntConversionInputKind {
        IntConversion_NumbersOnly,
        IntConversion_NumbersOrBoolsOnly,
        IntConversion_Any
    };

    
    
    
    void convertDoubleToInt(FloatRegister src, Register output, FloatRegister temp,
                            Label* truncateFail, Label* fail, IntConversionBehavior behavior);

    
    
    
    
    void convertValueToInt(ValueOperand value, MDefinition* input,
                           Label* handleStringEntry, Label* handleStringRejoin,
                           Label* truncateDoubleSlow,
                           Register stringReg, FloatRegister temp, Register output,
                           Label* fail, IntConversionBehavior behavior,
                           IntConversionInputKind conversion = IntConversion_Any);
    void convertValueToInt(ValueOperand value, FloatRegister temp, Register output, Label* fail,
                           IntConversionBehavior behavior)
    {
        convertValueToInt(value, nullptr, nullptr, nullptr, nullptr, InvalidReg, temp, output,
                          fail, behavior);
    }
    bool convertValueToInt(JSContext* cx, const Value& v, Register output, Label* fail,
                           IntConversionBehavior behavior);
    bool convertConstantOrRegisterToInt(JSContext* cx, ConstantOrRegister src, FloatRegister temp,
                                        Register output, Label* fail, IntConversionBehavior behavior);
    void convertTypedOrValueToInt(TypedOrValueRegister src, FloatRegister temp, Register output,
                                  Label* fail, IntConversionBehavior behavior);

    
    
    
    void convertValueToInt32(ValueOperand value, FloatRegister temp, Register output, Label* fail,
                             bool negativeZeroCheck)
    {
        convertValueToInt(value, temp, output, fail, negativeZeroCheck
                          ? IntConversion_NegativeZeroCheck
                          : IntConversion_Normal);
    }
    void convertValueToInt32(ValueOperand value, MDefinition* input,
                             FloatRegister temp, Register output, Label* fail,
                             bool negativeZeroCheck, IntConversionInputKind conversion = IntConversion_Any)
    {
        convertValueToInt(value, input, nullptr, nullptr, nullptr, InvalidReg, temp, output, fail,
                          negativeZeroCheck
                          ? IntConversion_NegativeZeroCheck
                          : IntConversion_Normal,
                          conversion);
    }
    bool convertValueToInt32(JSContext* cx, const Value& v, Register output, Label* fail,
                             bool negativeZeroCheck)
    {
        return convertValueToInt(cx, v, output, fail, negativeZeroCheck
                                 ? IntConversion_NegativeZeroCheck
                                 : IntConversion_Normal);
    }
    bool convertConstantOrRegisterToInt32(JSContext* cx, ConstantOrRegister src, FloatRegister temp,
                                          Register output, Label* fail, bool negativeZeroCheck)
    {
        return convertConstantOrRegisterToInt(cx, src, temp, output, fail, negativeZeroCheck
                                              ? IntConversion_NegativeZeroCheck
                                              : IntConversion_Normal);
    }
    void convertTypedOrValueToInt32(TypedOrValueRegister src, FloatRegister temp, Register output,
                                    Label* fail, bool negativeZeroCheck)
    {
        convertTypedOrValueToInt(src, temp, output, fail, negativeZeroCheck
                                 ? IntConversion_NegativeZeroCheck
                                 : IntConversion_Normal);
    }

    
    
    
    void truncateValueToInt32(ValueOperand value, FloatRegister temp, Register output, Label* fail) {
        convertValueToInt(value, temp, output, fail, IntConversion_Truncate);
    }
    void truncateValueToInt32(ValueOperand value, MDefinition* input,
                              Label* handleStringEntry, Label* handleStringRejoin,
                              Label* truncateDoubleSlow,
                              Register stringReg, FloatRegister temp, Register output, Label* fail)
    {
        convertValueToInt(value, input, handleStringEntry, handleStringRejoin, truncateDoubleSlow,
                          stringReg, temp, output, fail, IntConversion_Truncate);
    }
    void truncateValueToInt32(ValueOperand value, MDefinition* input,
                              FloatRegister temp, Register output, Label* fail)
    {
        convertValueToInt(value, input, nullptr, nullptr, nullptr, InvalidReg, temp, output, fail,
                          IntConversion_Truncate);
    }
    bool truncateValueToInt32(JSContext* cx, const Value& v, Register output, Label* fail) {
        return convertValueToInt(cx, v, output, fail, IntConversion_Truncate);
    }
    bool truncateConstantOrRegisterToInt32(JSContext* cx, ConstantOrRegister src, FloatRegister temp,
                                           Register output, Label* fail)
    {
        return convertConstantOrRegisterToInt(cx, src, temp, output, fail, IntConversion_Truncate);
    }
    void truncateTypedOrValueToInt32(TypedOrValueRegister src, FloatRegister temp, Register output,
                                     Label* fail)
    {
        convertTypedOrValueToInt(src, temp, output, fail, IntConversion_Truncate);
    }

    
    void clampValueToUint8(ValueOperand value, FloatRegister temp, Register output, Label* fail) {
        convertValueToInt(value, temp, output, fail, IntConversion_ClampToUint8);
    }
    void clampValueToUint8(ValueOperand value, MDefinition* input,
                           Label* handleStringEntry, Label* handleStringRejoin,
                           Register stringReg, FloatRegister temp, Register output, Label* fail)
    {
        convertValueToInt(value, input, handleStringEntry, handleStringRejoin, nullptr,
                          stringReg, temp, output, fail, IntConversion_ClampToUint8);
    }
    void clampValueToUint8(ValueOperand value, MDefinition* input,
                           FloatRegister temp, Register output, Label* fail)
    {
        convertValueToInt(value, input, nullptr, nullptr, nullptr, InvalidReg, temp, output, fail,
                          IntConversion_ClampToUint8);
    }
    bool clampValueToUint8(JSContext* cx, const Value& v, Register output, Label* fail) {
        return convertValueToInt(cx, v, output, fail, IntConversion_ClampToUint8);
    }
    bool clampConstantOrRegisterToUint8(JSContext* cx, ConstantOrRegister src, FloatRegister temp,
                                        Register output, Label* fail)
    {
        return convertConstantOrRegisterToInt(cx, src, temp, output, fail,
                                              IntConversion_ClampToUint8);
    }
    void clampTypedOrValueToUint8(TypedOrValueRegister src, FloatRegister temp, Register output,
                                  Label* fail)
    {
        convertTypedOrValueToInt(src, temp, output, fail, IntConversion_ClampToUint8);
    }

  public:
    class AfterICSaveLive {
        friend class MacroAssembler;
        explicit AfterICSaveLive(uint32_t initialStack)
#ifdef JS_DEBUG
          : initialStack(initialStack)
#endif
        {}

      public:
#ifdef JS_DEBUG
        uint32_t initialStack;
#endif
        uint32_t alignmentPadding;
    };

    void alignFrameForICArguments(AfterICSaveLive& aic);
    void restoreFrameAlignmentForICArguments(AfterICSaveLive& aic);

    AfterICSaveLive icSaveLive(LiveRegisterSet& liveRegs) {
        PushRegsInMask(liveRegs);
        AfterICSaveLive aic(framePushed());
        alignFrameForICArguments(aic);
        return aic;
    }

    bool icBuildOOLFakeExitFrame(void* fakeReturnAddr, AfterICSaveLive& aic) {
        return buildOOLFakeExitFrame(fakeReturnAddr);
    }

    void icRestoreLive(LiveRegisterSet& liveRegs, AfterICSaveLive& aic) {
        restoreFrameAlignmentForICArguments(aic);
        MOZ_ASSERT(framePushed() == aic.initialStack);
        PopRegsInMask(liveRegs);
    }

    
    
    
    void alignJitStackBasedOnNArgs(Register nargs);
    void alignJitStackBasedOnNArgs(uint32_t nargs);

    void assertStackAlignment(uint32_t alignment, int32_t offset = 0) {
#ifdef DEBUG
        Label ok, bad;
        MOZ_ASSERT(IsPowerOfTwo(alignment));

        
        offset %= alignment;
        if (offset < 0)
            offset += alignment;

        
        uint32_t off = offset;
        while (off) {
            uint32_t lowestBit = 1 << mozilla::CountTrailingZeroes32(off);
            branchTestStackPtr(Assembler::Zero, Imm32(lowestBit), &bad);
            off ^= lowestBit;
        }

        
        branchTestStackPtr(Assembler::Zero, Imm32((alignment - 1) ^ offset), &ok);

        bind(&bad);
        breakpoint();
        bind(&ok);
#endif
    }

    void profilerPreCallImpl();
    void profilerPreCallImpl(Register reg, Register reg2);
    void profilerPostReturnImpl() {}
};

static inline Assembler::DoubleCondition
JSOpToDoubleCondition(JSOp op)
{
    switch (op) {
      case JSOP_EQ:
      case JSOP_STRICTEQ:
        return Assembler::DoubleEqual;
      case JSOP_NE:
      case JSOP_STRICTNE:
        return Assembler::DoubleNotEqualOrUnordered;
      case JSOP_LT:
        return Assembler::DoubleLessThan;
      case JSOP_LE:
        return Assembler::DoubleLessThanOrEqual;
      case JSOP_GT:
        return Assembler::DoubleGreaterThan;
      case JSOP_GE:
        return Assembler::DoubleGreaterThanOrEqual;
      default:
        MOZ_CRASH("Unexpected comparison operation");
    }
}




static inline Assembler::Condition
JSOpToCondition(JSOp op, bool isSigned)
{
    if (isSigned) {
        switch (op) {
          case JSOP_EQ:
          case JSOP_STRICTEQ:
            return Assembler::Equal;
          case JSOP_NE:
          case JSOP_STRICTNE:
            return Assembler::NotEqual;
          case JSOP_LT:
            return Assembler::LessThan;
          case JSOP_LE:
            return Assembler::LessThanOrEqual;
          case JSOP_GT:
            return Assembler::GreaterThan;
          case JSOP_GE:
            return Assembler::GreaterThanOrEqual;
          default:
            MOZ_CRASH("Unrecognized comparison operation");
        }
    } else {
        switch (op) {
          case JSOP_EQ:
          case JSOP_STRICTEQ:
            return Assembler::Equal;
          case JSOP_NE:
          case JSOP_STRICTNE:
            return Assembler::NotEqual;
          case JSOP_LT:
            return Assembler::Below;
          case JSOP_LE:
            return Assembler::BelowOrEqual;
          case JSOP_GT:
            return Assembler::Above;
          case JSOP_GE:
            return Assembler::AboveOrEqual;
          default:
            MOZ_CRASH("Unrecognized comparison operation");
        }
    }
}

static inline size_t
StackDecrementForCall(uint32_t alignment, size_t bytesAlreadyPushed, size_t bytesToPush)
{
    return bytesToPush +
           ComputeByteAlignment(bytesAlreadyPushed + bytesToPush, alignment);
}

} 
} 

#endif
