






#include "jsinfer.h"
#include "jsinferinlines.h"
#include "IonMacroAssembler.h"
#include "gc/Root.h"
#include "Bailouts.h"
#include "vm/ForkJoin.h"

using namespace js;
using namespace js::ion;



class TypeWrapper {
    types::Type t_;

  public:
    TypeWrapper(types::Type t) : t_(t) {}

    inline bool unknown() const {
        return t_.isUnknown();
    }
    inline bool hasType(types::Type t) const {
        if (t == types::Type::Int32Type())
            return t == t_ || t_ == types::Type::DoubleType();
        return t == t_;
    }
    inline unsigned getObjectCount() const {
        if (t_.isAnyObject() || t_.isUnknown() || !t_.isObject())
            return 0;
        return 1;
    }
    inline JSObject *getSingleObject(unsigned) const {
        if (t_.isSingleObject())
            return t_.singleObject();
        return NULL;
    }
    inline types::TypeObject *getTypeObject(unsigned) const {
        if (t_.isTypeObject())
            return t_.typeObject();
        return NULL;
    }
};

template <typename Source, typename TypeSet> void
MacroAssembler::guardTypeSet(const Source &address, const TypeSet *types,
                             Register scratch, Label *matched, Label *miss)
{
    JS_ASSERT(!types->unknown());

    Register tag = extractTag(address, scratch);

    if (types->hasType(types::Type::DoubleType())) {
        
        JS_ASSERT(types->hasType(types::Type::Int32Type()));
        branchTestNumber(Equal, tag, matched);
    } else if (types->hasType(types::Type::Int32Type())) {
        branchTestInt32(Equal, tag, matched);
    }

    if (types->hasType(types::Type::UndefinedType()))
        branchTestUndefined(Equal, tag, matched);
    if (types->hasType(types::Type::BooleanType()))
        branchTestBoolean(Equal, tag, matched);
    if (types->hasType(types::Type::StringType()))
        branchTestString(Equal, tag, matched);
    if (types->hasType(types::Type::NullType()))
        branchTestNull(Equal, tag, matched);

    if (types->hasType(types::Type::AnyObjectType())) {
        branchTestObject(Equal, tag, matched);
    } else if (types->getObjectCount()) {
        branchTestObject(NotEqual, tag, miss);
        Register obj = extractObject(address, scratch);

        unsigned count = types->getObjectCount();
        for (unsigned i = 0; i < count; i++) {
            if (JSObject *object = types->getSingleObject(i))
                branchPtr(Equal, obj, ImmGCPtr(object), matched);
        }

        loadPtr(Address(obj, JSObject::offsetOfType()), scratch);

        for (unsigned i = 0; i < count; i++) {
            if (types::TypeObject *object = types->getTypeObject(i))
                branchPtr(Equal, scratch, ImmGCPtr(object), matched);
        }
    }
}

template <typename Source> void
MacroAssembler::guardType(const Source &address, types::Type type,
                          Register scratch, Label *matched, Label *miss)
{
    TypeWrapper wrapper(type);
    guardTypeSet(address, &wrapper, scratch, matched, miss);
}

template void MacroAssembler::guardTypeSet(const Address &address, const types::StackTypeSet *types,
                                           Register scratch, Label *matched, Label *miss);
template void MacroAssembler::guardTypeSet(const ValueOperand &value, const types::StackTypeSet *types,
                                           Register scratch, Label *matched, Label *miss);

template void MacroAssembler::guardTypeSet(const Address &address, const types::TypeSet *types,
                                           Register scratch, Label *matched, Label *miss);
template void MacroAssembler::guardTypeSet(const ValueOperand &value, const types::TypeSet *types,
                                           Register scratch, Label *matched, Label *miss);

template void MacroAssembler::guardTypeSet(const Address &address, const TypeWrapper *types,
                                           Register scratch, Label *matched, Label *miss);
template void MacroAssembler::guardTypeSet(const ValueOperand &value, const TypeWrapper *types,
                                           Register scratch, Label *matched, Label *miss);

template void MacroAssembler::guardType(const Address &address, types::Type type,
                                        Register scratch, Label *matched, Label *miss);
template void MacroAssembler::guardType(const ValueOperand &value, types::Type type,
                                        Register scratch, Label *matched, Label *miss);

void
MacroAssembler::PushRegsInMask(RegisterSet set)
{
    int32_t diffF = set.fpus().size() * sizeof(double);
    int32_t diffG = set.gprs().size() * STACK_SLOT_SIZE;

    reserveStack(diffG);
#ifdef JS_CPU_ARM
    if (set.gprs().size() > 1) {
        startDataTransferM(IsStore, StackPointer, IA, NoWriteBack);
        for (GeneralRegisterIterator iter(set.gprs()); iter.more(); iter++) {
            diffG -= STACK_SLOT_SIZE;
            transferReg(*iter);
        }
        finishDataTransfer();
    } else
#endif
    {
        for (GeneralRegisterIterator iter(set.gprs()); iter.more(); iter++) {
            diffG -= STACK_SLOT_SIZE;
            storePtr(*iter, Address(StackPointer, diffG));
        }
    }
    JS_ASSERT(diffG == 0);

    reserveStack(diffF);
#ifdef JS_CPU_ARM
    diffF -= transferMultipleByRuns(set.fpus(), IsStore, StackPointer, IA);
#else
    for (FloatRegisterIterator iter(set.fpus()); iter.more(); iter++) {
        diffF -= sizeof(double);
        storeDouble(*iter, Address(StackPointer, diffF));
    }
#endif
    JS_ASSERT(diffF == 0);
}

void
MacroAssembler::PopRegsInMaskIgnore(RegisterSet set, RegisterSet ignore)
{
    int32_t diffG = set.gprs().size() * STACK_SLOT_SIZE;
    int32_t diffF = set.fpus().size() * sizeof(double);
    const int32_t reservedG = diffG;
    const int32_t reservedF = diffF;

#ifdef JS_CPU_ARM
    
    
    if (ignore.empty(true)) {
        diffF -= transferMultipleByRuns(set.fpus(), IsLoad, StackPointer, IA);
    } else
#endif
    {
        for (FloatRegisterIterator iter(set.fpus()); iter.more(); iter++) {
            diffF -= sizeof(double);
            if (!ignore.has(*iter))
                loadDouble(Address(StackPointer, diffF), *iter);
        }
    }
    freeStack(reservedF);
    JS_ASSERT(diffF == 0);

#ifdef JS_CPU_ARM
    if (set.gprs().size() > 1 && ignore.empty(false)) {
        startDataTransferM(IsLoad, StackPointer, IA, NoWriteBack);
        for (GeneralRegisterIterator iter(set.gprs()); iter.more(); iter++) {
            diffG -= STACK_SLOT_SIZE;
            transferReg(*iter);
        }
        finishDataTransfer();
    } else
#endif
    {
        for (GeneralRegisterIterator iter(set.gprs()); iter.more(); iter++) {
            diffG -= STACK_SLOT_SIZE;
            if (!ignore.has(*iter))
                loadPtr(Address(StackPointer, diffG), *iter);
        }
    }
    freeStack(reservedG);
    JS_ASSERT(diffG == 0);
}

template<typename T>
void
MacroAssembler::loadFromTypedArray(int arrayType, const T &src, AnyRegister dest, Register temp,
                                   Label *fail)
{
    switch (arrayType) {
      case TypedArray::TYPE_INT8:
        load8SignExtend(src, dest.gpr());
        break;
      case TypedArray::TYPE_UINT8:
      case TypedArray::TYPE_UINT8_CLAMPED:
        load8ZeroExtend(src, dest.gpr());
        break;
      case TypedArray::TYPE_INT16:
        load16SignExtend(src, dest.gpr());
        break;
      case TypedArray::TYPE_UINT16:
        load16ZeroExtend(src, dest.gpr());
        break;
      case TypedArray::TYPE_INT32:
        load32(src, dest.gpr());
        break;
      case TypedArray::TYPE_UINT32:
        if (dest.isFloat()) {
            load32(src, temp);
            convertUInt32ToDouble(temp, dest.fpu());
        } else {
            load32(src, dest.gpr());
            test32(dest.gpr(), dest.gpr());
            j(Assembler::Signed, fail);
        }
        break;
      case TypedArray::TYPE_FLOAT32:
      case TypedArray::TYPE_FLOAT64:
      {
        if (arrayType == js::TypedArray::TYPE_FLOAT32)
            loadFloatAsDouble(src, dest.fpu());
        else
            loadDouble(src, dest.fpu());

        
        Label notNaN;
        branchDouble(DoubleOrdered, dest.fpu(), dest.fpu(), &notNaN);
        {
            loadStaticDouble(&js_NaN, dest.fpu());
        }
        bind(&notNaN);
        break;
      }
      default:
        JS_NOT_REACHED("Invalid typed array type");
        break;
    }
}

template void MacroAssembler::loadFromTypedArray(int arrayType, const Address &src, AnyRegister dest,
                                                 Register temp, Label *fail);
template void MacroAssembler::loadFromTypedArray(int arrayType, const BaseIndex &src, AnyRegister dest,
                                                 Register temp, Label *fail);

template<typename T>
void
MacroAssembler::loadFromTypedArray(int arrayType, const T &src, const ValueOperand &dest,
                                   bool allowDouble, Register temp, Label *fail)
{
    switch (arrayType) {
      case TypedArray::TYPE_INT8:
      case TypedArray::TYPE_UINT8:
      case TypedArray::TYPE_UINT8_CLAMPED:
      case TypedArray::TYPE_INT16:
      case TypedArray::TYPE_UINT16:
      case TypedArray::TYPE_INT32:
        loadFromTypedArray(arrayType, src, AnyRegister(dest.scratchReg()), InvalidReg, NULL);
        tagValue(JSVAL_TYPE_INT32, dest.scratchReg(), dest);
        break;
      case TypedArray::TYPE_UINT32:
        
        load32(src, temp);
        test32(temp, temp);
        if (allowDouble) {
            
            
            Label done, isDouble;
            j(Assembler::Signed, &isDouble);
            {
                tagValue(JSVAL_TYPE_INT32, temp, dest);
                jump(&done);
            }
            bind(&isDouble);
            {
                convertUInt32ToDouble(temp, ScratchFloatReg);
                boxDouble(ScratchFloatReg, dest);
            }
            bind(&done);
        } else {
            
            j(Assembler::Signed, fail);
            tagValue(JSVAL_TYPE_INT32, temp, dest);
        }
        break;
      case TypedArray::TYPE_FLOAT32:
      case TypedArray::TYPE_FLOAT64:
        loadFromTypedArray(arrayType, src, AnyRegister(ScratchFloatReg), dest.scratchReg(), NULL);
        boxDouble(ScratchFloatReg, dest);
        break;
      default:
        JS_NOT_REACHED("Invalid typed array type");
        break;
    }
}

template void MacroAssembler::loadFromTypedArray(int arrayType, const Address &src, const ValueOperand &dest,
                                                 bool allowDouble, Register temp, Label *fail);
template void MacroAssembler::loadFromTypedArray(int arrayType, const BaseIndex &src, const ValueOperand &dest,
                                                 bool allowDouble, Register temp, Label *fail);


void
MacroAssembler::clampDoubleToUint8(FloatRegister input, Register output)
{
    JS_ASSERT(input != ScratchFloatReg);
#ifdef JS_CPU_ARM
    Label notSplit;
    ma_vimm(0.5, ScratchFloatReg);
    ma_vadd(input, ScratchFloatReg, ScratchFloatReg);
    
    
    as_vcvtFixed(ScratchFloatReg, false, 24, true);
    
    as_vxfer(output, InvalidReg, ScratchFloatReg, FloatToCore);
    
    
    
    ma_tst(output, Imm32(0x00ffffff));
    
    ma_lsr(Imm32(24), output, output);
    
    
    ma_b(&notSplit, NonZero);
    as_vxfer(ScratchRegister, InvalidReg, input, FloatToCore);
    ma_cmp(ScratchRegister, Imm32(0));
    
    
    ma_bic(Imm32(1), output, NoSetCond, Zero);
    bind(&notSplit);
#else

    Label positive, done;

    
    zeroDouble(ScratchFloatReg);
    branchDouble(DoubleGreaterThan, input, ScratchFloatReg, &positive);
    {
        move32(Imm32(0), output);
        jump(&done);
    }

    bind(&positive);

    
    static const double DoubleHalf = 0.5;
    loadStaticDouble(&DoubleHalf, ScratchFloatReg);
    addDouble(ScratchFloatReg, input);

    Label outOfRange;
    branchTruncateDouble(input, output, &outOfRange);
    branch32(Assembler::Above, output, Imm32(255), &outOfRange);
    {
        
        convertInt32ToDouble(output, ScratchFloatReg);
        branchDouble(DoubleNotEqual, input, ScratchFloatReg, &done);

        
        
        and32(Imm32(~1), output);
        jump(&done);
    }

    
    bind(&outOfRange);
    {
        move32(Imm32(255), output);
    }

    bind(&done);
#endif
}

void
MacroAssembler::newGCThing(const Register &result,
                           JSObject *templateObject, Label *fail)
{
    

    gc::AllocKind allocKind = templateObject->getAllocKind();
    JS_ASSERT(allocKind >= gc::FINALIZE_OBJECT0 && allocKind <= gc::FINALIZE_OBJECT_LAST);
    int thingSize = (int)gc::Arena::thingSize(allocKind);

    JS_ASSERT(!templateObject->hasDynamicElements());

    Zone *zone = GetIonContext()->compartment->zone();

#ifdef JS_GC_ZEAL
    
    movePtr(ImmWord(zone->rt), result);
    loadPtr(Address(result, offsetof(JSRuntime, gcZeal_)), result);
    branch32(Assembler::NotEqual, result, Imm32(0), fail);
#endif

    
    
    
    
    gc::FreeSpan *list = const_cast<gc::FreeSpan *>
                         (zone->allocator.arenas.getFreeList(allocKind));
    loadPtr(AbsoluteAddress(&list->first), result);
    branchPtr(Assembler::BelowOrEqual, AbsoluteAddress(&list->last), result, fail);

    addPtr(Imm32(thingSize), result);
    storePtr(result, AbsoluteAddress(&list->first));
    subPtr(Imm32(thingSize), result);
}

void
MacroAssembler::parNewGCThing(const Register &result,
                              const Register &threadContextReg,
                              const Register &tempReg1,
                              const Register &tempReg2,
                              JSObject *templateObject,
                              Label *fail)
{
    
    
    
    
    
    
    
    
    

    gc::AllocKind allocKind = templateObject->getAllocKind();
    uint32_t thingSize = (uint32_t)gc::Arena::thingSize(allocKind);

    
    
    loadPtr(Address(threadContextReg, offsetof(js::ForkJoinSlice, allocator)),
            tempReg1);

    
    
    uint32_t offset = (offsetof(Allocator, arenas) +
                       js::gc::ArenaLists::getFreeListOffset(allocKind));
    addPtr(Imm32(offset), tempReg1);

    
    
    loadPtr(Address(tempReg1, offsetof(gc::FreeSpan, first)), tempReg2);

    
    
    branchPtr(Assembler::BelowOrEqual,
              Address(tempReg1, offsetof(gc::FreeSpan, last)),
              tempReg2,
              fail);

    
    
    
    movePtr(tempReg2, result);
    addPtr(Imm32(thingSize), tempReg2);

    
    
    storePtr(tempReg2, Address(tempReg1, offsetof(gc::FreeSpan, first)));
}

void
MacroAssembler::initGCThing(const Register &obj, JSObject *templateObject)
{
    

    storePtr(ImmGCPtr(templateObject->lastProperty()), Address(obj, JSObject::offsetOfShape()));
    storePtr(ImmGCPtr(templateObject->type()), Address(obj, JSObject::offsetOfType()));
    storePtr(ImmWord((void *)NULL), Address(obj, JSObject::offsetOfSlots()));

    if (templateObject->isArray()) {
        JS_ASSERT(!templateObject->getDenseInitializedLength());

        int elementsOffset = JSObject::offsetOfFixedElements();

        addPtr(Imm32(elementsOffset), obj);
        storePtr(obj, Address(obj, -elementsOffset + JSObject::offsetOfElements()));
        addPtr(Imm32(-elementsOffset), obj);

        
        store32(Imm32(templateObject->getDenseCapacity()),
                Address(obj, elementsOffset + ObjectElements::offsetOfCapacity()));
        store32(Imm32(templateObject->getDenseInitializedLength()),
                Address(obj, elementsOffset + ObjectElements::offsetOfInitializedLength()));
        store32(Imm32(templateObject->getArrayLength()),
                Address(obj, elementsOffset + ObjectElements::offsetOfLength()));
        store32(Imm32(templateObject->shouldConvertDoubleElements() ? 1 : 0),
                Address(obj, elementsOffset + ObjectElements::offsetOfConvertDoubleElements()));
    } else {
        storePtr(ImmWord(emptyObjectElements), Address(obj, JSObject::offsetOfElements()));

        
        
        size_t nslots = Min(templateObject->numFixedSlots(), templateObject->slotSpan());
        for (unsigned i = 0; i < nslots; i++) {
            storeValue(templateObject->getFixedSlot(i),
                       Address(obj, JSObject::getFixedSlotOffset(i)));
        }
    }

    if (templateObject->hasPrivate()) {
        uint32_t nfixed = templateObject->numFixedSlots();
        storePtr(ImmWord(templateObject->getPrivate()),
                 Address(obj, JSObject::getPrivateDataOffset(nfixed)));
    }
}

void
MacroAssembler::compareStrings(JSOp op, Register left, Register right, Register result,
                               Register temp, Label *fail)
{
    JS_ASSERT(IsEqualityOp(op));

    Label done;
    Label notPointerEqual;
    
    branchPtr(Assembler::NotEqual, left, right, &notPointerEqual);
    move32(Imm32(op == JSOP_EQ || op == JSOP_STRICTEQ), result);
    jump(&done);

    bind(&notPointerEqual);
    loadPtr(Address(left, JSString::offsetOfLengthAndFlags()), result);
    loadPtr(Address(right, JSString::offsetOfLengthAndFlags()), temp);

    Label notAtom;
    
    Imm32 atomBit(JSString::ATOM_BIT);
    branchTest32(Assembler::Zero, result, atomBit, &notAtom);
    branchTest32(Assembler::Zero, temp, atomBit, &notAtom);

    cmpPtr(left, right);
    emitSet(JSOpToCondition(op), result);
    jump(&done);

    bind(&notAtom);
    
    rshiftPtr(Imm32(JSString::LENGTH_SHIFT), result);
    rshiftPtr(Imm32(JSString::LENGTH_SHIFT), temp);
    branchPtr(Assembler::Equal, result, temp, fail);
    move32(Imm32(op == JSOP_NE || op == JSOP_STRICTNE), result);

    bind(&done);
}

void
MacroAssembler::parCheckInterruptFlags(const Register &tempReg,
                                       Label *fail)
{
    JSCompartment *compartment = GetIonContext()->compartment;

    void *interrupt = (void*)&compartment->rt->interrupt;
    movePtr(ImmWord(interrupt), tempReg);
    load32(Address(tempReg, 0), tempReg);
    branchTest32(Assembler::NonZero, tempReg, tempReg, fail);
}

void
MacroAssembler::maybeRemoveOsrFrame(Register scratch)
{
    
    
    
    Label osrRemoved;
    loadPtr(Address(StackPointer, IonCommonFrameLayout::offsetOfDescriptor()), scratch);
    and32(Imm32(FRAMETYPE_MASK), scratch);
    branch32(Assembler::NotEqual, scratch, Imm32(IonFrame_Osr), &osrRemoved);
    addPtr(Imm32(sizeof(IonOsrFrameLayout)), StackPointer);
    bind(&osrRemoved);
}

void
MacroAssembler::performOsr()
{
    GeneralRegisterSet regs = GeneralRegisterSet::All();
    if (FramePointer != InvalidReg && sps_ && sps_->enabled())
        regs.take(FramePointer);

    
    regs.take(OsrFrameReg);

    
    maybeRemoveOsrFrame(regs.getAny());

    const Register script = regs.takeAny();
    const Register calleeToken = regs.takeAny();

    
    loadPtr(Address(OsrFrameReg, StackFrame::offsetOfExec()), script);
    mov(script, calleeToken);

    Label isFunction, performOsr;
    branchTest32(Assembler::NonZero,
                 Address(OsrFrameReg, StackFrame::offsetOfFlags()),
                 Imm32(StackFrame::FUNCTION),
                 &isFunction);

    {
        
        orPtr(Imm32(CalleeToken_Script), calleeToken);
        jump(&performOsr);
    }

    bind(&isFunction);
    {
        
        orPtr(Imm32(CalleeToken_Function), calleeToken);
        loadPtr(Address(script, JSFunction::offsetOfNativeOrScript()), script);
    }

    bind(&performOsr);

    const Register ionScript = regs.takeAny();
    const Register osrEntry = regs.takeAny();

    loadPtr(Address(script, offsetof(JSScript, ion)), ionScript);
    load32(Address(ionScript, IonScript::offsetOfOsrEntryOffset()), osrEntry);

    
    const Register code = ionScript;
    loadPtr(Address(ionScript, IonScript::offsetOfMethod()), code);
    loadPtr(Address(code, IonCode::offsetOfCode()), code);
    addPtr(osrEntry, code);

    
    
    enterOsr(calleeToken, code);
    ret();
}

void
MacroAssembler::generateBailoutTail(Register scratch)
{
    enterExitFrame();

    Label reflow;
    Label interpret;
    Label exception;
    Label osr;
    Label recompile;
    Label boundscheck;
    Label overrecursed;
    Label invalidate;

    
    
    
    
    
    
    
    
    
    
    

    branch32(LessThan, ReturnReg, Imm32(BAILOUT_RETURN_FATAL_ERROR), &interpret);
    branch32(Equal, ReturnReg, Imm32(BAILOUT_RETURN_FATAL_ERROR), &exception);

    branch32(LessThan, ReturnReg, Imm32(BAILOUT_RETURN_RECOMPILE_CHECK), &reflow);
    branch32(Equal, ReturnReg, Imm32(BAILOUT_RETURN_RECOMPILE_CHECK), &recompile);

    branch32(Equal, ReturnReg, Imm32(BAILOUT_RETURN_BOUNDS_CHECK), &boundscheck);
    branch32(Equal, ReturnReg, Imm32(BAILOUT_RETURN_OVERRECURSED), &overrecursed);
    branch32(Equal, ReturnReg, Imm32(BAILOUT_RETURN_SHAPE_GUARD), &invalidate);

    
    {
        setupUnalignedABICall(0, scratch);
        callWithABI(JS_FUNC_TO_DATA_PTR(void *, CachedShapeGuardFailure));

        branchTest32(Zero, ReturnReg, ReturnReg, &exception);
        jump(&interpret);
    }

    
    bind(&invalidate);
    {
        setupUnalignedABICall(0, scratch);
        callWithABI(JS_FUNC_TO_DATA_PTR(void *, ShapeGuardFailure));

        branchTest32(Zero, ReturnReg, ReturnReg, &exception);
        jump(&interpret);
    }

    
    bind(&boundscheck);
    {
        setupUnalignedABICall(0, scratch);
        callWithABI(JS_FUNC_TO_DATA_PTR(void *, BoundsCheckFailure));

        branchTest32(Zero, ReturnReg, ReturnReg, &exception);
        jump(&interpret);
    }

    
    bind(&recompile);
    {
        setupUnalignedABICall(0, scratch);
        callWithABI(JS_FUNC_TO_DATA_PTR(void *, RecompileForInlining));

        branchTest32(Zero, ReturnReg, ReturnReg, &exception);
        jump(&interpret);
    }

    
    bind(&reflow);
    {
        setupUnalignedABICall(1, scratch);
        passABIArg(ReturnReg);
        callWithABI(JS_FUNC_TO_DATA_PTR(void *, ReflowTypeInfo));

        branchTest32(Zero, ReturnReg, ReturnReg, &exception);
        jump(&interpret);
    }

    
    bind(&overrecursed);
    {
        loadJSContext(ReturnReg);
        setupUnalignedABICall(1, scratch);
        passABIArg(ReturnReg);
        callWithABI(JS_FUNC_TO_DATA_PTR(void *, js_ReportOverRecursed));
        jump(&exception);
    }

    bind(&interpret);
    {
        
        subPtr(Imm32(sizeof(Value)), StackPointer);
        mov(StackPointer, ReturnReg);

        
        setupUnalignedABICall(1, scratch);
        passABIArg(ReturnReg);
        callWithABI(JS_FUNC_TO_DATA_PTR(void *, ThunkToInterpreter));

        
        popValue(JSReturnOperand);

        
        JS_STATIC_ASSERT(!Interpret_Error);
        branchTest32(Zero, ReturnReg, ReturnReg, &exception);

        
        leaveExitFrame();

        branch32(Equal, ReturnReg, Imm32(Interpret_OSR), &osr);

        
        ret();
    }

    bind(&osr);
    {
        unboxPrivate(JSReturnOperand, OsrFrameReg);
        performOsr();
    }

    bind(&exception);
    {
        handleException();
    }
}

void printf0_(const char *output) {
    printf("%s", output);
}

void
MacroAssembler::printf(const char *output)
{
    RegisterSet regs = RegisterSet::Volatile();
    PushRegsInMask(regs);

    Register temp = regs.takeGeneral();

    setupUnalignedABICall(1, temp);
    movePtr(ImmWord(output), temp);
    passABIArg(temp);
    callWithABI(JS_FUNC_TO_DATA_PTR(void *, printf0_));

    PopRegsInMask(RegisterSet::Volatile());
}

void printf1_(const char *output, uintptr_t value) {
    char *line = JS_sprintf_append(NULL, output, value);
    printf("%s", line);
    js_free(line);
}

void
MacroAssembler::printf(const char *output, Register value)
{
    RegisterSet regs = RegisterSet::Volatile();
    PushRegsInMask(regs);

    regs.maybeTake(value);

    Register temp = regs.takeGeneral();

    setupUnalignedABICall(2, temp);
    movePtr(ImmWord(output), temp);
    passABIArg(temp);
    passABIArg(value);
    callWithABI(JS_FUNC_TO_DATA_PTR(void *, printf1_));

    PopRegsInMask(RegisterSet::Volatile());
}
