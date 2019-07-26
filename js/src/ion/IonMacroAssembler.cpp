






#include "jsinfer.h"
#include "jsinferinlines.h"
#include "IonMacroAssembler.h"
#include "gc/Root.h"

using namespace js;
using namespace js::ion;

template <typename T> void
MacroAssembler::guardTypeSet(const T &address, types::TypeSet *types,
                             Register scratch, Label *mismatched)
{
    JS_ASSERT(!types->unknown());

    Label matched;
    Register tag = extractTag(address, scratch);

    if (types->hasType(types::Type::DoubleType())) {
        
        JS_ASSERT(types->hasType(types::Type::Int32Type()));
        branchTestNumber(Equal, tag, &matched);
    } else if (types->hasType(types::Type::Int32Type())) {
        branchTestInt32(Equal, tag, &matched);
    }

    if (types->hasType(types::Type::UndefinedType()))
        branchTestUndefined(Equal, tag, &matched);
    if (types->hasType(types::Type::BooleanType()))
        branchTestBoolean(Equal, tag, &matched);
    if (types->hasType(types::Type::StringType()))
        branchTestString(Equal, tag, &matched);
    if (types->hasType(types::Type::NullType()))
        branchTestNull(Equal, tag, &matched);

    if (types->hasType(types::Type::AnyObjectType())) {
        branchTestObject(Equal, tag, &matched);
    } else if (types->getObjectCount()) {
        branchTestObject(NotEqual, tag, mismatched);
        Register obj = extractObject(address, scratch);

        unsigned count = types->getObjectCount();
        for (unsigned i = 0; i < count; i++) {
            if (JSObject *object = types->getSingleObject(i))
                branchPtr(Equal, obj, ImmGCPtr(object), &matched);
        }

        loadPtr(Address(obj, JSObject::offsetOfType()), scratch);

        for (unsigned i = 0; i < count; i++) {
            if (types::TypeObject *object = types->getTypeObject(i))
                branchPtr(Equal, scratch, ImmGCPtr(object), &matched);
        }
    }

    jump(mismatched);
    bind(&matched);
}

template void MacroAssembler::guardTypeSet(const Address &address, types::TypeSet *types,
                                           Register scratch, Label *mismatched);
template void MacroAssembler::guardTypeSet(const ValueOperand &value, types::TypeSet *types,
                                           Register scratch, Label *mismatched);

void
MacroAssembler::PushRegsInMask(RegisterSet set)
{
    size_t diff = set.gprs().size() * STACK_SLOT_SIZE +
                  set.fpus().size() * sizeof(double);

    reserveStack(diff);

    for (GeneralRegisterIterator iter(set.gprs()); iter.more(); iter++) {
        diff -= STACK_SLOT_SIZE;
        storePtr(*iter, Address(StackPointer, diff));
    }
    for (FloatRegisterIterator iter(set.fpus()); iter.more(); iter++) {
        diff -= sizeof(double);
        storeDouble(*iter, Address(StackPointer, diff));
    }
}

void
MacroAssembler::PopRegsInMask(RegisterSet set)
{
    size_t diff = set.gprs().size() * STACK_SLOT_SIZE +
                  set.fpus().size() * sizeof(double);
    size_t reserved = diff;

    for (GeneralRegisterIterator iter(set.gprs()); iter.more(); iter++) {
        diff -= STACK_SLOT_SIZE;
        loadPtr(Address(StackPointer, diff), *iter);
    }
    for (FloatRegisterIterator iter(set.fpus()); iter.more(); iter++) {
        diff -= sizeof(double);
        loadDouble(Address(StackPointer, diff), *iter);
    }

    freeStack(reserved);
}

void
MacroAssembler::branchTestValueTruthy(const ValueOperand &value, Label *ifTrue, FloatRegister fr)
{
    Register tag = splitTagForTest(value);
    Label ifFalse;
    Assembler::Condition cond;

    
    
    
    
    branchTestUndefined(Assembler::Equal, tag, &ifFalse);

    branchTestNull(Assembler::Equal, tag, &ifFalse);
    branchTestObject(Assembler::Equal, tag, ifTrue);

    Label notBoolean;
    branchTestBoolean(Assembler::NotEqual, tag, &notBoolean);
    branchTestBooleanTruthy(false, value, &ifFalse);
    jump(ifTrue);
    bind(&notBoolean);

    Label notInt32;
    branchTestInt32(Assembler::NotEqual, tag, &notInt32);
    cond = testInt32Truthy(false, value);
    j(cond, &ifFalse);
    jump(ifTrue);
    bind(&notInt32);

    
    Label notString;
    branchTestString(Assembler::NotEqual, tag, &notString);
    cond = testStringTruthy(false, value);
    j(cond, &ifFalse);
    jump(ifTrue);
    bind(&notString);

    
    unboxDouble(value, fr);
    cond = testDoubleTruthy(false, fr);
    j(cond, &ifFalse);
    jump(ifTrue);
    bind(&ifFalse);
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
                                   bool allowDouble, Label *fail)
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
        load32(src, dest.scratchReg());
        test32(dest.scratchReg(), dest.scratchReg());
        if (allowDouble) {
            
            
            Label done, isDouble;
            j(Assembler::Signed, &isDouble);
            {
                tagValue(JSVAL_TYPE_INT32, dest.scratchReg(), dest);
                jump(&done);
            }
            bind(&isDouble);
            {
                convertUInt32ToDouble(dest.scratchReg(), ScratchFloatReg);
                boxDouble(ScratchFloatReg, dest);
            }
            bind(&done);
        } else {
            
            j(Assembler::Signed, fail);
            tagValue(JSVAL_TYPE_INT32, dest.scratchReg(), dest);
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
                                                 bool allowDouble, Label *fail);
template void MacroAssembler::loadFromTypedArray(int arrayType, const BaseIndex &src, const ValueOperand &dest,
                                                 bool allowDouble, Label *fail);


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

    JSCompartment *compartment = GetIonContext()->compartment;

#ifdef JS_GC_ZEAL
    
    movePtr(ImmWord(compartment->rt), result);
    loadPtr(Address(result, offsetof(JSRuntime, gcZeal_)), result);
    branch32(Assembler::NotEqual, result, Imm32(0), fail);
#endif

    
    
    
    
    gc::FreeSpan *list = const_cast<gc::FreeSpan *>
                         (compartment->arenas.getFreeList(allocKind));
    loadPtr(AbsoluteAddress(&list->first), result);
    branchPtr(Assembler::BelowOrEqual, AbsoluteAddress(&list->last), result, fail);

    addPtr(Imm32(thingSize), result);
    storePtr(result, AbsoluteAddress(&list->first));
    subPtr(Imm32(thingSize), result);
}

void
MacroAssembler::initGCThing(const Register &obj, JSObject *templateObject)
{
    

    storePtr(ImmGCPtr(templateObject->lastProperty()), Address(obj, JSObject::offsetOfShape()));
    storePtr(ImmGCPtr(templateObject->type()), Address(obj, JSObject::offsetOfType()));
    storePtr(ImmWord((void *)NULL), Address(obj, JSObject::offsetOfSlots()));

    if (templateObject->isDenseArray()) {
        JS_ASSERT(!templateObject->getDenseArrayInitializedLength());

        int elementsOffset = JSObject::offsetOfFixedElements();

        addPtr(Imm32(elementsOffset), obj);
        storePtr(obj, Address(obj, -elementsOffset + JSObject::offsetOfElements()));
        addPtr(Imm32(-elementsOffset), obj);

        
        store32(Imm32(templateObject->getDenseArrayCapacity()),
                Address(obj, elementsOffset + ObjectElements::offsetOfCapacity()));
        store32(Imm32(templateObject->getDenseArrayInitializedLength()),
                Address(obj, elementsOffset + ObjectElements::offsetOfInitializedLength()));
        store32(Imm32(templateObject->getArrayLength()),
                Address(obj, elementsOffset + ObjectElements::offsetOfLength()));
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
MacroAssembler::maybeRemoveOsrFrame(Register scratch)
{
    
    
    
    Label osrRemoved;
    movePtr(Address(StackPointer, IonCommonFrameLayout::offsetOfDescriptor()), scratch);
    and32(Imm32(FRAMETYPE_MASK), scratch);
    branch32(Assembler::NotEqual, scratch, Imm32(IonFrame_Osr), &osrRemoved);
    addPtr(Imm32(sizeof(IonOsrFrameLayout)), StackPointer);
    bind(&osrRemoved);
}

void
MacroAssembler::performOsr()
{
    GeneralRegisterSet regs = GeneralRegisterSet::All();

    
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
    loadPtr(Address(code, IonCode::OffsetOfCode()), code);
    addPtr(osrEntry, code);

    
    
    enterOsr(calleeToken, code);
    ret();
}

