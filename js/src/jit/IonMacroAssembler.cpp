





#include "jit/IonMacroAssembler.h"

#include "jsinfer.h"
#include "jsprf.h"

#include "builtin/TypedObject.h"
#include "jit/Bailouts.h"
#include "jit/BaselineFrame.h"
#include "jit/BaselineIC.h"
#include "jit/BaselineJIT.h"
#include "jit/Lowering.h"
#include "jit/MIR.h"
#include "jit/ParallelFunctions.h"
#include "vm/ForkJoin.h"

#ifdef JSGC_GENERATIONAL
# include "jsgcinlines.h"
#endif
#include "jsinferinlines.h"
#include "jsobjinlines.h"

using namespace js;
using namespace js::jit;

using JS::GenericNaN;

namespace {



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
        return nullptr;
    }
    inline types::TypeObject *getTypeObject(unsigned) const {
        if (t_.isTypeObject())
            return t_.typeObject();
        return nullptr;
    }
};

} 

template <typename Source, typename TypeSet> void
MacroAssembler::guardTypeSet(const Source &address, const TypeSet *types,
                             Register scratch, Label *miss)
{
    JS_ASSERT(!types->unknown());

    Label matched;
    types::Type tests[7] = {
        types::Type::Int32Type(),
        types::Type::UndefinedType(),
        types::Type::BooleanType(),
        types::Type::StringType(),
        types::Type::NullType(),
        types::Type::MagicArgType(),
        types::Type::AnyObjectType()
    };

    
    
    if (types->hasType(types::Type::DoubleType())) {
        JS_ASSERT(types->hasType(types::Type::Int32Type()));
        tests[0] = types::Type::DoubleType();
    }

    Register tag = extractTag(address, scratch);

    
    BranchType lastBranch;
    for (size_t i = 0; i < 7; i++) {
        if (!types->hasType(tests[i]))
            continue;

        if (lastBranch.isInitialized())
            lastBranch.emit(*this);
        lastBranch = BranchType(Equal, tag, tests[i], &matched);
    }

    
    if (types->hasType(types::Type::AnyObjectType()) || !types->getObjectCount()) {
        if (!lastBranch.isInitialized()) {
            jump(miss);
            return;
        }

        lastBranch.invertCondition();
        lastBranch.relink(miss);
        lastBranch.emit(*this);

        bind(&matched);
        return;
    }

    if (lastBranch.isInitialized())
        lastBranch.emit(*this);

    
    JS_ASSERT(scratch != InvalidReg);
    branchTestObject(NotEqual, tag, miss);
    Register obj = extractObject(address, scratch);
    guardObjectType(obj, types, scratch, miss);

    bind(&matched);
}

template <typename TypeSet> void
MacroAssembler::guardObjectType(Register obj, const TypeSet *types,
                                Register scratch, Label *miss)
{
    JS_ASSERT(!types->unknown());
    JS_ASSERT(!types->hasType(types::Type::AnyObjectType()));
    JS_ASSERT(types->getObjectCount());
    JS_ASSERT(scratch != InvalidReg);

    Label matched;

    BranchGCPtr lastBranch;
    JS_ASSERT(!lastBranch.isInitialized());
    bool hasTypeObjects = false;
    unsigned count = types->getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        if (!types->getSingleObject(i)) {
            hasTypeObjects = hasTypeObjects || types->getTypeObject(i);
            continue;
        }

        if (lastBranch.isInitialized())
            lastBranch.emit(*this);

        JSObject *object = types->getSingleObject(i);
        lastBranch = BranchGCPtr(Equal, obj, ImmGCPtr(object), &matched);
    }

    if (hasTypeObjects) {
        
        
        
        
        if (lastBranch.isInitialized())
            lastBranch.emit(*this);
        lastBranch = BranchGCPtr();

        
        
        loadPtr(Address(obj, JSObject::offsetOfType()), scratch);

        for (unsigned i = 0; i < count; i++) {
            if (!types->getTypeObject(i))
                continue;

            if (lastBranch.isInitialized())
                lastBranch.emit(*this);

            types::TypeObject *object = types->getTypeObject(i);
            lastBranch = BranchGCPtr(Equal, scratch, ImmGCPtr(object), &matched);
        }
    }

    if (!lastBranch.isInitialized()) {
        jump(miss);
        return;
    }

    lastBranch.invertCondition();
    lastBranch.relink(miss);
    lastBranch.emit(*this);

    bind(&matched);
    return;
}

template <typename Source> void
MacroAssembler::guardType(const Source &address, types::Type type,
                          Register scratch, Label *miss)
{
    TypeWrapper wrapper(type);
    guardTypeSet(address, &wrapper, scratch, miss);
}

template void MacroAssembler::guardTypeSet(const Address &address, const types::TemporaryTypeSet *types,
                                           Register scratch, Label *miss);
template void MacroAssembler::guardTypeSet(const ValueOperand &value, const types::TemporaryTypeSet *types,
                                           Register scratch, Label *miss);

template void MacroAssembler::guardTypeSet(const Address &address, const types::HeapTypeSet *types,
                                           Register scratch, Label *miss);
template void MacroAssembler::guardTypeSet(const ValueOperand &value, const types::HeapTypeSet *types,
                                           Register scratch, Label *miss);
template void MacroAssembler::guardTypeSet(const TypedOrValueRegister &reg, const types::HeapTypeSet *types,
                                           Register scratch, Label *miss);

template void MacroAssembler::guardTypeSet(const Address &address, const types::TypeSet *types,
                                           Register scratch, Label *miss);
template void MacroAssembler::guardTypeSet(const ValueOperand &value, const types::TypeSet *types,
                                           Register scratch, Label *miss);

template void MacroAssembler::guardTypeSet(const Address &address, const TypeWrapper *types,
                                           Register scratch, Label *miss);
template void MacroAssembler::guardTypeSet(const ValueOperand &value, const TypeWrapper *types,
                                           Register scratch, Label *miss);

template void MacroAssembler::guardObjectType(Register obj, const types::TemporaryTypeSet *types,
                                              Register scratch, Label *miss);
template void MacroAssembler::guardObjectType(Register obj, const types::TypeSet *types,
                                              Register scratch, Label *miss);
template void MacroAssembler::guardObjectType(Register obj, const TypeWrapper *types,
                                              Register scratch, Label *miss);

template void MacroAssembler::guardType(const Address &address, types::Type type,
                                        Register scratch, Label *miss);
template void MacroAssembler::guardType(const ValueOperand &value, types::Type type,
                                        Register scratch, Label *miss);

void
MacroAssembler::branchNurseryPtr(Condition cond, const Address &ptr1, const ImmMaybeNurseryPtr &ptr2,
                                 Label *label)
{
#ifdef JSGC_GENERATIONAL
    if (ptr2.value && gc::IsInsideNursery(GetIonContext()->cx->runtime(), (void *)ptr2.value))
        embedsNurseryPointers_ = true;
#endif
    branchPtr(cond, ptr1, ptr2, label);
}

void
MacroAssembler::moveNurseryPtr(const ImmMaybeNurseryPtr &ptr, Register reg)
{
#ifdef JSGC_GENERATIONAL
    if (ptr.value && gc::IsInsideNursery(GetIonContext()->cx->runtime(), (void *)ptr.value))
        embedsNurseryPointers_ = true;
#endif
    movePtr(ptr, reg);
}

template<typename S, typename T>
static void
StoreToTypedFloatArray(MacroAssembler &masm, int arrayType, const S &value, const T &dest)
{
    switch (arrayType) {
      case ScalarTypeDescr::TYPE_FLOAT32:
        if (LIRGenerator::allowFloat32Optimizations()) {
            masm.storeFloat32(value, dest);
        } else {
#ifdef JS_MORE_DETERMINISTIC
            
            masm.canonicalizeDouble(value);
#endif
            masm.convertDoubleToFloat32(value, ScratchFloatReg);
            masm.storeFloat32(ScratchFloatReg, dest);
        }
        break;
      case ScalarTypeDescr::TYPE_FLOAT64:
#ifdef JS_MORE_DETERMINISTIC
        
        masm.canonicalizeDouble(value);
#endif
        masm.storeDouble(value, dest);
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("Invalid typed array type");
    }
}

void
MacroAssembler::storeToTypedFloatArray(int arrayType, const FloatRegister &value,
                                       const BaseIndex &dest)
{
    StoreToTypedFloatArray(*this, arrayType, value, dest);
}
void
MacroAssembler::storeToTypedFloatArray(int arrayType, const FloatRegister &value,
                                       const Address &dest)
{
    StoreToTypedFloatArray(*this, arrayType, value, dest);
}

template<typename T>
void
MacroAssembler::loadFromTypedArray(int arrayType, const T &src, AnyRegister dest, Register temp,
                                   Label *fail)
{
    switch (arrayType) {
      case ScalarTypeDescr::TYPE_INT8:
        load8SignExtend(src, dest.gpr());
        break;
      case ScalarTypeDescr::TYPE_UINT8:
      case ScalarTypeDescr::TYPE_UINT8_CLAMPED:
        load8ZeroExtend(src, dest.gpr());
        break;
      case ScalarTypeDescr::TYPE_INT16:
        load16SignExtend(src, dest.gpr());
        break;
      case ScalarTypeDescr::TYPE_UINT16:
        load16ZeroExtend(src, dest.gpr());
        break;
      case ScalarTypeDescr::TYPE_INT32:
        load32(src, dest.gpr());
        break;
      case ScalarTypeDescr::TYPE_UINT32:
        if (dest.isFloat()) {
            load32(src, temp);
            convertUInt32ToDouble(temp, dest.fpu());
        } else {
            load32(src, dest.gpr());

            
            
            
            test32(dest.gpr(), dest.gpr());
            j(Assembler::Signed, fail);
        }
        break;
      case ScalarTypeDescr::TYPE_FLOAT32:
        if (LIRGenerator::allowFloat32Optimizations()) {
            loadFloat32(src, dest.fpu());
            canonicalizeFloat(dest.fpu());
        } else {
            loadFloatAsDouble(src, dest.fpu());
            canonicalizeDouble(dest.fpu());
        }
        break;
      case ScalarTypeDescr::TYPE_FLOAT64:
        loadDouble(src, dest.fpu());
        canonicalizeDouble(dest.fpu());
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("Invalid typed array type");
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
      case ScalarTypeDescr::TYPE_INT8:
      case ScalarTypeDescr::TYPE_UINT8:
      case ScalarTypeDescr::TYPE_UINT8_CLAMPED:
      case ScalarTypeDescr::TYPE_INT16:
      case ScalarTypeDescr::TYPE_UINT16:
      case ScalarTypeDescr::TYPE_INT32:
        loadFromTypedArray(arrayType, src, AnyRegister(dest.scratchReg()), InvalidReg, nullptr);
        tagValue(JSVAL_TYPE_INT32, dest.scratchReg(), dest);
        break;
      case ScalarTypeDescr::TYPE_UINT32:
        
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
      case ScalarTypeDescr::TYPE_FLOAT32:
        loadFromTypedArray(arrayType, src, AnyRegister(ScratchFloatReg), dest.scratchReg(),
                           nullptr);
        if (LIRGenerator::allowFloat32Optimizations())
            convertFloat32ToDouble(ScratchFloatReg, ScratchFloatReg);
        boxDouble(ScratchFloatReg, dest);
        break;
      case ScalarTypeDescr::TYPE_FLOAT64:
        loadFromTypedArray(arrayType, src, AnyRegister(ScratchFloatReg), dest.scratchReg(),
                           nullptr);
        boxDouble(ScratchFloatReg, dest);
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("Invalid typed array type");
    }
}

template void MacroAssembler::loadFromTypedArray(int arrayType, const Address &src, const ValueOperand &dest,
                                                 bool allowDouble, Register temp, Label *fail);
template void MacroAssembler::loadFromTypedArray(int arrayType, const BaseIndex &src, const ValueOperand &dest,
                                                 bool allowDouble, Register temp, Label *fail);

void
MacroAssembler::newGCThing(Register result, Register temp, gc::AllocKind allocKind, Label *fail,
                           gc::InitialHeap initialHeap )
{
    

    int thingSize = int(gc::Arena::thingSize(allocKind));

#ifdef JS_GC_ZEAL
    
    branch32(Assembler::NotEqual,
             AbsoluteAddress(GetIonContext()->runtime->addressOfGCZeal()), Imm32(0),
             fail);
#endif

    
    
    if (GetIonContext()->compartment->hasObjectMetadataCallback())
        jump(fail);

#ifdef JSGC_GENERATIONAL
    
    
    if (allocKind <= gc::FINALIZE_OBJECT_LAST && initialHeap != gc::TenuredHeap) {
        
        
        
        const Nursery &nursery = GetIonContext()->runtime->gcNursery();
        loadPtr(AbsoluteAddress(nursery.addressOfPosition()), result);
        computeEffectiveAddress(Address(result, thingSize), temp);
        branchPtr(Assembler::BelowOrEqual, AbsoluteAddress(nursery.addressOfCurrentEnd()), temp, fail);
        storePtr(temp, AbsoluteAddress(nursery.addressOfPosition()));
        return;
    }
#endif 

    CompileZone *zone = GetIonContext()->compartment->zone();

    
    
    
    
    loadPtr(AbsoluteAddress(zone->addressOfFreeListFirst(allocKind)), result);
    branchPtr(Assembler::BelowOrEqual, AbsoluteAddress(zone->addressOfFreeListLast(allocKind)), result, fail);
    computeEffectiveAddress(Address(result, thingSize), temp);
    storePtr(temp, AbsoluteAddress(zone->addressOfFreeListFirst(allocKind)));
}

void
MacroAssembler::newGCThing(Register result, Register temp, JSObject *templateObject, Label *fail,
                           gc::InitialHeap initialHeap)
{
    gc::AllocKind allocKind = templateObject->tenuredGetAllocKind();
    JS_ASSERT(allocKind >= gc::FINALIZE_OBJECT0 && allocKind <= gc::FINALIZE_OBJECT_LAST);

    newGCThing(result, temp, allocKind, fail, initialHeap);
}

void
MacroAssembler::newGCString(Register result, Register temp, Label *fail)
{
    newGCThing(result, temp, js::gc::FINALIZE_STRING, fail);
}

void
MacroAssembler::newGCFatInlineString(Register result, Register temp, Label *fail)
{
    newGCThing(result, temp, js::gc::FINALIZE_FAT_INLINE_STRING, fail);
}

void
MacroAssembler::newGCThingPar(Register result, Register cx, Register tempReg1, Register tempReg2,
                              gc::AllocKind allocKind, Label *fail)
{
    
    
    
    
    
    
    

    uint32_t thingSize = (uint32_t)gc::Arena::thingSize(allocKind);

    
    
    loadPtr(Address(cx, ThreadSafeContext::offsetOfAllocator()),
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
MacroAssembler::newGCThingPar(Register result, Register cx, Register tempReg1, Register tempReg2,
                              JSObject *templateObject, Label *fail)
{
    gc::AllocKind allocKind = templateObject->tenuredGetAllocKind();
    JS_ASSERT(allocKind >= gc::FINALIZE_OBJECT0 && allocKind <= gc::FINALIZE_OBJECT_LAST);

    newGCThingPar(result, cx, tempReg1, tempReg2, allocKind, fail);
}

void
MacroAssembler::newGCStringPar(Register result, Register cx, Register tempReg1, Register tempReg2,
                               Label *fail)
{
    newGCThingPar(result, cx, tempReg1, tempReg2, js::gc::FINALIZE_STRING, fail);
}

void
MacroAssembler::newGCFatInlineStringPar(Register result, Register cx, Register tempReg1,
                                        Register tempReg2, Label *fail)
{
    newGCThingPar(result, cx, tempReg1, tempReg2, js::gc::FINALIZE_FAT_INLINE_STRING, fail);
}

void
MacroAssembler::copySlotsFromTemplate(Register obj, Register temp, const JSObject *templateObj,
                                      uint32_t start, uint32_t end)
{
    uint32_t nfixed = Min(templateObj->numFixedSlots(), end);
    for (unsigned i = start; i < nfixed; i++)
        storeValue(templateObj->getFixedSlot(i), Address(obj, JSObject::getFixedSlotOffset(i)));
}

void
MacroAssembler::fillSlotsWithUndefined(Register obj, Register temp, const JSObject *templateObj,
                                       uint32_t start, uint32_t end)
{
#ifdef JS_NUNBOX32
    
    
    jsval_layout jv = JSVAL_TO_IMPL(UndefinedValue());
    uint32_t nfixed = Min(templateObj->numFixedSlots(), end);

    mov(ImmWord(jv.s.tag), temp);
    for (unsigned i = start; i < nfixed; i++)
        store32(temp, ToType(Address(obj, JSObject::getFixedSlotOffset(i))));

    mov(ImmWord(jv.s.payload.i32), temp);
    for (unsigned i = start; i < nfixed; i++)
        store32(temp, ToPayload(Address(obj, JSObject::getFixedSlotOffset(i))));
#else
    moveValue(UndefinedValue(), temp);
    uint32_t nfixed = Min(templateObj->numFixedSlots(), end);
    for (unsigned i = start; i < nfixed; i++)
        storePtr(temp, Address(obj, JSObject::getFixedSlotOffset(i)));
#endif
}

static uint32_t
FindStartOfUndefinedSlots(JSObject *templateObj, uint32_t nslots)
{
    JS_ASSERT(nslots == templateObj->lastProperty()->slotSpan(templateObj->getClass()));
    JS_ASSERT(nslots > 0);
    for (uint32_t first = nslots; first != 0; --first) {
        if (templateObj->getSlot(first - 1) != UndefinedValue())
            return first;
    }
    return 0;
}

void
MacroAssembler::initGCSlots(Register obj, Register temp, JSObject *templateObj)
{
    
    
    uint32_t nslots = templateObj->lastProperty()->slotSpan(templateObj->getClass());
    if (nslots == 0)
        return;

    
    
    
    
    
    
    
    uint32_t startOfUndefined = FindStartOfUndefinedSlots(templateObj, nslots);
    copySlotsFromTemplate(obj, temp, templateObj, 0, startOfUndefined);
    fillSlotsWithUndefined(obj, temp, templateObj, startOfUndefined, nslots);
}

void
MacroAssembler::initGCThing(Register obj, Register temp, JSObject *templateObj)
{
    

    JS_ASSERT(!templateObj->hasDynamicElements());

    storePtr(ImmGCPtr(templateObj->lastProperty()), Address(obj, JSObject::offsetOfShape()));
    storePtr(ImmGCPtr(templateObj->type()), Address(obj, JSObject::offsetOfType()));
    storePtr(ImmPtr(nullptr), Address(obj, JSObject::offsetOfSlots()));

    if (templateObj->is<ArrayObject>()) {
        JS_ASSERT(!templateObj->getDenseInitializedLength());

        int elementsOffset = JSObject::offsetOfFixedElements();

        computeEffectiveAddress(Address(obj, elementsOffset), temp);
        storePtr(temp, Address(obj, JSObject::offsetOfElements()));

        
        store32(Imm32(templateObj->getDenseCapacity()),
                Address(obj, elementsOffset + ObjectElements::offsetOfCapacity()));
        store32(Imm32(templateObj->getDenseInitializedLength()),
                Address(obj, elementsOffset + ObjectElements::offsetOfInitializedLength()));
        store32(Imm32(templateObj->as<ArrayObject>().length()),
                Address(obj, elementsOffset + ObjectElements::offsetOfLength()));
        store32(Imm32(templateObj->shouldConvertDoubleElements()
                      ? ObjectElements::CONVERT_DOUBLE_ELEMENTS
                      : 0),
                Address(obj, elementsOffset + ObjectElements::offsetOfFlags()));
        JS_ASSERT(!templateObj->hasPrivate());
    } else {
        storePtr(ImmPtr(emptyObjectElements), Address(obj, JSObject::offsetOfElements()));

        initGCSlots(obj, temp, templateObj);

        if (templateObj->hasPrivate()) {
            uint32_t nfixed = templateObj->numFixedSlots();
            storePtr(ImmPtr(templateObj->getPrivate()),
                     Address(obj, JSObject::getPrivateDataOffset(nfixed)));
        }
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
    emitSet(JSOpToCondition(MCompare::Compare_String, op), result);
    jump(&done);

    bind(&notAtom);
    
    rshiftPtr(Imm32(JSString::LENGTH_SHIFT), result);
    rshiftPtr(Imm32(JSString::LENGTH_SHIFT), temp);
    branchPtr(Assembler::Equal, result, temp, fail);
    move32(Imm32(op == JSOP_NE || op == JSOP_STRICTNE), result);

    bind(&done);
}

void
MacroAssembler::checkInterruptFlagPar(Register tempReg, Label *fail)
{
#ifdef JS_THREADSAFE
    movePtr(ImmPtr(GetIonContext()->runtime->addressOfInterruptPar()), tempReg);
    branch32(Assembler::NonZero, Address(tempReg, 0), Imm32(0), fail);
#else
    MOZ_ASSUME_UNREACHABLE("JSRuntime::interruptPar doesn't exist on non-threadsafe builds.");
#endif
}

static void
ReportOverRecursed(JSContext *cx)
{
    js_ReportOverRecursed(cx);
}

void
MacroAssembler::generateBailoutTail(Register scratch, Register bailoutInfo)
{
    enterExitFrame();

    Label baseline;

    
    
    
    
    JS_STATIC_ASSERT(BAILOUT_RETURN_OK == 0);
    JS_STATIC_ASSERT(BAILOUT_RETURN_FATAL_ERROR == 1);
    JS_STATIC_ASSERT(BAILOUT_RETURN_OVERRECURSED == 2);

    branch32(Equal, ReturnReg, Imm32(BAILOUT_RETURN_OK), &baseline);
    branch32(Equal, ReturnReg, Imm32(BAILOUT_RETURN_FATAL_ERROR), exceptionLabel());

    
    {
        loadJSContext(ReturnReg);
        setupUnalignedABICall(1, scratch);
        passABIArg(ReturnReg);
        callWithABI(JS_FUNC_TO_DATA_PTR(void *, ReportOverRecursed));
        jump(exceptionLabel());
    }

    bind(&baseline);
    {
        
        GeneralRegisterSet regs(GeneralRegisterSet::All());
        JS_ASSERT(!regs.has(BaselineStackReg));
        regs.take(bailoutInfo);

        
        loadPtr(Address(bailoutInfo, offsetof(BaselineBailoutInfo, incomingStack)),
                BaselineStackReg);

        Register copyCur = regs.takeAny();
        Register copyEnd = regs.takeAny();
        Register temp = regs.takeAny();

        
        loadPtr(Address(bailoutInfo, offsetof(BaselineBailoutInfo, copyStackTop)), copyCur);
        loadPtr(Address(bailoutInfo, offsetof(BaselineBailoutInfo, copyStackBottom)), copyEnd);
        {
            Label copyLoop;
            Label endOfCopy;
            bind(&copyLoop);
            branchPtr(Assembler::BelowOrEqual, copyCur, copyEnd, &endOfCopy);
            subPtr(Imm32(4), copyCur);
            subPtr(Imm32(4), BaselineStackReg);
            load32(Address(copyCur, 0), temp);
            store32(temp, Address(BaselineStackReg, 0));
            jump(&copyLoop);
            bind(&endOfCopy);
        }

        
        loadPtr(Address(bailoutInfo, offsetof(BaselineBailoutInfo, resumeFramePtr)), temp);
        load32(Address(temp, BaselineFrame::reverseOffsetOfFrameSize()), temp);
        makeFrameDescriptor(temp, JitFrame_BaselineJS);
        push(temp);
        push(Address(bailoutInfo, offsetof(BaselineBailoutInfo, resumeAddr)));
        enterFakeExitFrame();

        
        Label noMonitor;
        Label done;
        branchPtr(Assembler::Equal,
                  Address(bailoutInfo, offsetof(BaselineBailoutInfo, monitorStub)),
                  ImmPtr(nullptr),
                  &noMonitor);

        
        
        
        {
            
            pushValue(Address(bailoutInfo, offsetof(BaselineBailoutInfo, valueR0)));
            push(Address(bailoutInfo, offsetof(BaselineBailoutInfo, resumeFramePtr)));
            push(Address(bailoutInfo, offsetof(BaselineBailoutInfo, resumeAddr)));
            push(Address(bailoutInfo, offsetof(BaselineBailoutInfo, monitorStub)));

            
            setupUnalignedABICall(1, temp);
            passABIArg(bailoutInfo);
            callWithABI(JS_FUNC_TO_DATA_PTR(void *, FinishBailoutToBaseline));
            branchTest32(Zero, ReturnReg, ReturnReg, exceptionLabel());

            
            GeneralRegisterSet enterMonRegs(GeneralRegisterSet::All());
            enterMonRegs.take(R0);
            enterMonRegs.take(BaselineStubReg);
            enterMonRegs.take(BaselineFrameReg);
            enterMonRegs.takeUnchecked(BaselineTailCallReg);

            pop(BaselineStubReg);
            pop(BaselineTailCallReg);
            pop(BaselineFrameReg);
            popValue(R0);

            
            addPtr(Imm32(IonExitFrameLayout::SizeWithFooter()), StackPointer);

#if defined(JS_CODEGEN_X86) || defined(JS_CODEGEN_X64)
            push(BaselineTailCallReg);
#endif
            jump(Address(BaselineStubReg, ICStub::offsetOfStubCode()));
        }

        
        
        
        bind(&noMonitor);
        {
            
            pushValue(Address(bailoutInfo, offsetof(BaselineBailoutInfo, valueR0)));
            pushValue(Address(bailoutInfo, offsetof(BaselineBailoutInfo, valueR1)));
            push(Address(bailoutInfo, offsetof(BaselineBailoutInfo, resumeFramePtr)));
            push(Address(bailoutInfo, offsetof(BaselineBailoutInfo, resumeAddr)));

            
            setupUnalignedABICall(1, temp);
            passABIArg(bailoutInfo);
            callWithABI(JS_FUNC_TO_DATA_PTR(void *, FinishBailoutToBaseline));
            branchTest32(Zero, ReturnReg, ReturnReg, exceptionLabel());

            
            GeneralRegisterSet enterRegs(GeneralRegisterSet::All());
            enterRegs.take(R0);
            enterRegs.take(R1);
            enterRegs.take(BaselineFrameReg);
            Register jitcodeReg = enterRegs.takeAny();

            pop(jitcodeReg);
            pop(BaselineFrameReg);
            popValue(R1);
            popValue(R0);

            
            addPtr(Imm32(IonExitFrameLayout::SizeWithFooter()), StackPointer);

            jump(jitcodeReg);
        }
    }
}

void
MacroAssembler::loadBaselineOrIonRaw(Register script, Register dest, ExecutionMode mode,
                                     Label *failure)
{
    if (mode == SequentialExecution) {
        loadPtr(Address(script, JSScript::offsetOfBaselineOrIonRaw()), dest);
        if (failure)
            branchTestPtr(Assembler::Zero, dest, dest, failure);
    } else {
        loadPtr(Address(script, JSScript::offsetOfParallelIonScript()), dest);
        if (failure)
            branchPtr(Assembler::BelowOrEqual, dest, ImmPtr(ION_COMPILING_SCRIPT), failure);
        loadPtr(Address(dest, IonScript::offsetOfMethod()), dest);
        loadPtr(Address(dest, JitCode::offsetOfCode()), dest);
    }
}

void
MacroAssembler::loadBaselineOrIonNoArgCheck(Register script, Register dest, ExecutionMode mode,
                                            Label *failure)
{
    if (mode == SequentialExecution) {
        loadPtr(Address(script, JSScript::offsetOfBaselineOrIonSkipArgCheck()), dest);
        if (failure)
            branchTestPtr(Assembler::Zero, dest, dest, failure);
    } else {
        
        Register offset = script;
        if (script == dest) {
            GeneralRegisterSet regs(GeneralRegisterSet::All());
            regs.take(dest);
            offset = regs.takeAny();
        }

        loadPtr(Address(script, JSScript::offsetOfParallelIonScript()), dest);
        if (failure)
            branchPtr(Assembler::BelowOrEqual, dest, ImmPtr(ION_COMPILING_SCRIPT), failure);

        Push(offset);
        load32(Address(script, IonScript::offsetOfSkipArgCheckEntryOffset()), offset);

        loadPtr(Address(dest, IonScript::offsetOfMethod()), dest);
        loadPtr(Address(dest, JitCode::offsetOfCode()), dest);
        addPtr(offset, dest);

        Pop(offset);
    }
}

void
MacroAssembler::loadBaselineFramePtr(Register framePtr, Register dest)
{
    if (framePtr != dest)
        movePtr(framePtr, dest);
    subPtr(Imm32(BaselineFrame::Size()), dest);
}

void
MacroAssembler::loadForkJoinContext(Register cx, Register scratch)
{
    
    
    
    setupUnalignedABICall(0, scratch);
    callWithABI(JS_FUNC_TO_DATA_PTR(void *, ForkJoinContextPar));
    if (ReturnReg != cx)
        movePtr(ReturnReg, cx);
}

void
MacroAssembler::loadContext(Register cxReg, Register scratch, ExecutionMode executionMode)
{
    switch (executionMode) {
      case SequentialExecution:
        
        loadJSContext(cxReg);
        break;
      case ParallelExecution:
        loadForkJoinContext(cxReg, scratch);
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("No such execution mode");
    }
}

void
MacroAssembler::enterParallelExitFrameAndLoadContext(const VMFunction *f, Register cx,
                                                     Register scratch)
{
    loadForkJoinContext(cx, scratch);
    
    loadPtr(Address(cx, offsetof(ForkJoinContext, perThreadData)), scratch);
    linkParallelExitFrame(scratch);
    
    exitCodePatch_ = PushWithPatch(ImmWord(-1));
    
    Push(ImmPtr(f));
}

void
MacroAssembler::enterFakeParallelExitFrame(Register cx, Register scratch,
                                           JitCode *codeVal)
{
    
    loadPtr(Address(cx, offsetof(ForkJoinContext, perThreadData)), scratch);
    linkParallelExitFrame(scratch);
    Push(ImmPtr(codeVal));
    Push(ImmPtr(nullptr));
}

void
MacroAssembler::enterExitFrameAndLoadContext(const VMFunction *f, Register cxReg, Register scratch,
                                             ExecutionMode executionMode)
{
    switch (executionMode) {
      case SequentialExecution:
        
        enterExitFrame(f);
        loadJSContext(cxReg);
        break;
      case ParallelExecution:
        enterParallelExitFrameAndLoadContext(f, cxReg, scratch);
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("No such execution mode");
    }
}

void
MacroAssembler::enterFakeExitFrame(Register cxReg, Register scratch,
                                   ExecutionMode executionMode,
                                   JitCode *codeVal)
{
    switch (executionMode) {
      case SequentialExecution:
        
        enterFakeExitFrame(codeVal);
        break;
      case ParallelExecution:
        enterFakeParallelExitFrame(cxReg, scratch, codeVal);
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("No such execution mode");
    }
}

void
MacroAssembler::handleFailure(ExecutionMode executionMode)
{
    
    
    if (sps_)
        sps_->skipNextReenter();
    leaveSPSFrame();

    void *handler;
    switch (executionMode) {
      case SequentialExecution:
        handler = JS_FUNC_TO_DATA_PTR(void *, jit::HandleException);
        break;
      case ParallelExecution:
        handler = JS_FUNC_TO_DATA_PTR(void *, jit::HandleParallelFailure);
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("No such execution mode");
    }
    MacroAssemblerSpecific::handleFailureWithHandler(handler);

    
    if (sps_)
        sps_->reenter(*this, InvalidReg);
}

#ifdef DEBUG
static inline bool
IsCompilingAsmJS()
{
    
    IonContext *ictx = MaybeGetIonContext();
    return ictx && ictx->compartment == nullptr;
}

static void
AssumeUnreachable_(const char *output) {
    MOZ_ReportAssertionFailure(output, __FILE__, __LINE__);
}
#endif

void
MacroAssembler::assumeUnreachable(const char *output)
{
#ifdef DEBUG
    RegisterSet regs = RegisterSet::Volatile();
    PushRegsInMask(regs);
    Register temp = regs.takeGeneral();

    
    
    
    
    if (IsCompilingAsmJS()) {
        setupUnalignedABICall(0, temp);
        callWithABINoProfiling(AsmJSImm_AssumeUnreachable);
    } else {
        setupUnalignedABICall(1, temp);
        movePtr(ImmPtr(output), temp);
        passABIArg(temp);
        callWithABINoProfiling(JS_FUNC_TO_DATA_PTR(void *, AssumeUnreachable_));
    }
    PopRegsInMask(RegisterSet::Volatile());
#endif

    breakpoint();
}

static void
Printf0_(const char *output) {
    printf("%s", output);
}

void
MacroAssembler::printf(const char *output)
{
    RegisterSet regs = RegisterSet::Volatile();
    PushRegsInMask(regs);

    Register temp = regs.takeGeneral();

    setupUnalignedABICall(1, temp);
    movePtr(ImmPtr(output), temp);
    passABIArg(temp);
    callWithABINoProfiling(JS_FUNC_TO_DATA_PTR(void *, Printf0_));

    PopRegsInMask(RegisterSet::Volatile());
}

static void
Printf1_(const char *output, uintptr_t value) {
    char *line = JS_sprintf_append(nullptr, output, value);
    printf("%s", line);
    js_free(line);
}

void
MacroAssembler::printf(const char *output, Register value)
{
    RegisterSet regs = RegisterSet::Volatile();
    PushRegsInMask(regs);

    regs.takeUnchecked(value);

    Register temp = regs.takeGeneral();

    setupUnalignedABICall(2, temp);
    movePtr(ImmPtr(output), temp);
    passABIArg(temp);
    passABIArg(value);
    callWithABINoProfiling(JS_FUNC_TO_DATA_PTR(void *, Printf1_));

    PopRegsInMask(RegisterSet::Volatile());
}

#if JS_TRACE_LOGGING
void
MacroAssembler::tracelogStart(JSScript *script)
{
    void (&TraceLogStart)(TraceLogging*, TraceLogging::Type, JSScript*) = TraceLog;
    RegisterSet regs = RegisterSet::Volatile();
    PushRegsInMask(regs);

    Register temp = regs.takeGeneral();
    Register type = regs.takeGeneral();
    Register rscript = regs.takeGeneral();

    setupUnalignedABICall(3, temp);
    movePtr(ImmPtr(TraceLogging::defaultLogger()), temp);
    passABIArg(temp);
    move32(Imm32(TraceLogging::SCRIPT_START), type);
    passABIArg(type);
    movePtr(ImmGCPtr(script), rscript);
    passABIArg(rscript);
    callWithABINoProfiling(JS_FUNC_TO_DATA_PTR(void *, TraceLogStart));

    PopRegsInMask(RegisterSet::Volatile());
}

void
MacroAssembler::tracelogStop()
{
    void (&TraceLogStop)(TraceLogging*, TraceLogging::Type) = TraceLog;
    RegisterSet regs = RegisterSet::Volatile();
    PushRegsInMask(regs);

    Register temp = regs.takeGeneral();
    Register logger = regs.takeGeneral();
    Register type = regs.takeGeneral();

    setupUnalignedABICall(2, temp);
    movePtr(ImmPtr(TraceLogging::defaultLogger()), logger);
    passABIArg(logger);
    move32(Imm32(TraceLogging::SCRIPT_STOP), type);
    passABIArg(type);
    callWithABINoProfiling(JS_FUNC_TO_DATA_PTR(void *, TraceLogStop));

    PopRegsInMask(RegisterSet::Volatile());
}

void
MacroAssembler::tracelogLog(TraceLogging::Type type)
{
    void (&TraceLogStop)(TraceLogging*, TraceLogging::Type) = TraceLog;
    RegisterSet regs = RegisterSet::Volatile();
    PushRegsInMask(regs);

    Register temp = regs.takeGeneral();
    Register logger = regs.takeGeneral();
    Register rtype = regs.takeGeneral();

    setupUnalignedABICall(2, temp);
    movePtr(ImmPtr(TraceLogging::defaultLogger()), logger);
    passABIArg(logger);
    move32(Imm32(type), rtype);
    passABIArg(rtype);
    callWithABINoProfiling(JS_FUNC_TO_DATA_PTR(void *, TraceLogStop));

    PopRegsInMask(RegisterSet::Volatile());
}
#endif

void
MacroAssembler::convertInt32ValueToDouble(const Address &address, Register scratch, Label *done)
{
    branchTestInt32(Assembler::NotEqual, address, done);
    unboxInt32(address, scratch);
    convertInt32ToDouble(scratch, ScratchFloatReg);
    storeDouble(ScratchFloatReg, address);
}

void
MacroAssembler::convertValueToFloatingPoint(ValueOperand value, FloatRegister output,
                                            Label *fail, MIRType outputType)
{
    Register tag = splitTagForTest(value);

    Label isDouble, isInt32, isBool, isNull, done;

    branchTestDouble(Assembler::Equal, tag, &isDouble);
    branchTestInt32(Assembler::Equal, tag, &isInt32);
    branchTestBoolean(Assembler::Equal, tag, &isBool);
    branchTestNull(Assembler::Equal, tag, &isNull);
    branchTestUndefined(Assembler::NotEqual, tag, fail);

    
    loadConstantFloatingPoint(GenericNaN(), float(GenericNaN()), output, outputType);
    jump(&done);

    bind(&isNull);
    loadConstantFloatingPoint(0.0, 0.0f, output, outputType);
    jump(&done);

    bind(&isBool);
    boolValueToFloatingPoint(value, output, outputType);
    jump(&done);

    bind(&isInt32);
    int32ValueToFloatingPoint(value, output, outputType);
    jump(&done);

    bind(&isDouble);
    unboxDouble(value, output);
    if (outputType == MIRType_Float32)
        convertDoubleToFloat32(output, output);
    bind(&done);
}

bool
MacroAssembler::convertValueToFloatingPoint(JSContext *cx, const Value &v, FloatRegister output,
                                            Label *fail, MIRType outputType)
{
    if (v.isNumber() || v.isString()) {
        double d;
        if (v.isNumber())
            d = v.toNumber();
        else if (!StringToNumber(cx, v.toString(), &d))
            return false;

        loadConstantFloatingPoint(d, (float)d, output, outputType);
        return true;
    }

    if (v.isBoolean()) {
        if (v.toBoolean())
            loadConstantFloatingPoint(1.0, 1.0f, output, outputType);
        else
            loadConstantFloatingPoint(0.0, 0.0f, output, outputType);
        return true;
    }

    if (v.isNull()) {
        loadConstantFloatingPoint(0.0, 0.0f, output, outputType);
        return true;
    }

    if (v.isUndefined()) {
        loadConstantFloatingPoint(GenericNaN(), float(GenericNaN()), output, outputType);
        return true;
    }

    JS_ASSERT(v.isObject());
    jump(fail);
    return true;
}

void
MacroAssembler::PushEmptyRooted(VMFunction::RootType rootType)
{
    switch (rootType) {
      case VMFunction::RootNone:
        MOZ_ASSUME_UNREACHABLE("Handle must have root type");
      case VMFunction::RootObject:
      case VMFunction::RootString:
      case VMFunction::RootPropertyName:
      case VMFunction::RootFunction:
      case VMFunction::RootCell:
        Push(ImmPtr(nullptr));
        break;
      case VMFunction::RootValue:
        Push(UndefinedValue());
        break;
    }
}

void
MacroAssembler::popRooted(VMFunction::RootType rootType, Register cellReg,
                          const ValueOperand &valueReg)
{
    switch (rootType) {
      case VMFunction::RootNone:
        MOZ_ASSUME_UNREACHABLE("Handle must have root type");
      case VMFunction::RootObject:
      case VMFunction::RootString:
      case VMFunction::RootPropertyName:
      case VMFunction::RootFunction:
      case VMFunction::RootCell:
        Pop(cellReg);
        break;
      case VMFunction::RootValue:
        Pop(valueReg);
        break;
    }
}

bool
MacroAssembler::convertConstantOrRegisterToFloatingPoint(JSContext *cx, ConstantOrRegister src,
                                                         FloatRegister output, Label *fail,
                                                         MIRType outputType)
{
    if (src.constant())
        return convertValueToFloatingPoint(cx, src.value(), output, fail, outputType);

    convertTypedOrValueToFloatingPoint(src.reg(), output, fail, outputType);
    return true;
}

void
MacroAssembler::convertTypedOrValueToFloatingPoint(TypedOrValueRegister src, FloatRegister output,
                                                   Label *fail, MIRType outputType)
{
    JS_ASSERT(IsFloatingPointType(outputType));

    if (src.hasValue()) {
        convertValueToFloatingPoint(src.valueReg(), output, fail, outputType);
        return;
    }

    bool outputIsDouble = outputType == MIRType_Double;
    switch (src.type()) {
      case MIRType_Null:
        loadConstantFloatingPoint(0.0, 0.0f, output, outputType);
        break;
      case MIRType_Boolean:
      case MIRType_Int32:
        convertInt32ToFloatingPoint(src.typedReg().gpr(), output, outputType);
        break;
      case MIRType_Float32:
        if (outputIsDouble) {
            convertFloat32ToDouble(src.typedReg().fpu(), output);
        } else {
            if (src.typedReg().fpu() != output)
                moveFloat32(src.typedReg().fpu(), output);
        }
        break;
      case MIRType_Double:
        if (outputIsDouble) {
            if (src.typedReg().fpu() != output)
                moveDouble(src.typedReg().fpu(), output);
        } else {
            convertDoubleToFloat32(src.typedReg().fpu(), output);
        }
        break;
      case MIRType_Object:
      case MIRType_String:
        jump(fail);
        break;
      case MIRType_Undefined:
        loadConstantFloatingPoint(GenericNaN(), float(GenericNaN()), output, outputType);
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("Bad MIRType");
    }
}

void
MacroAssembler::convertDoubleToInt(FloatRegister src, Register output, FloatRegister temp,
                                   Label *truncateFail, Label *fail,
                                   IntConversionBehavior behavior)
{
    switch (behavior) {
      case IntConversion_Normal:
      case IntConversion_NegativeZeroCheck:
        convertDoubleToInt32(src, output, fail, behavior == IntConversion_NegativeZeroCheck);
        break;
      case IntConversion_Truncate:
        branchTruncateDouble(src, output, truncateFail ? truncateFail : fail);
        break;
      case IntConversion_ClampToUint8:
        
        moveDouble(src, temp);
        clampDoubleToUint8(temp, output);
        break;
    }
}

void
MacroAssembler::convertValueToInt(ValueOperand value, MDefinition *maybeInput,
                                  Label *handleStringEntry, Label *handleStringRejoin,
                                  Label *truncateDoubleSlow,
                                  Register stringReg, FloatRegister temp, Register output,
                                  Label *fail, IntConversionBehavior behavior,
                                  IntConversionInputKind conversion)
{
    Register tag = splitTagForTest(value);
    bool handleStrings = (behavior == IntConversion_Truncate ||
                          behavior == IntConversion_ClampToUint8) &&
                         handleStringEntry &&
                         handleStringRejoin;

    JS_ASSERT_IF(handleStrings, conversion == IntConversion_Any);

    Label done, isInt32, isBool, isDouble, isNull, isString;

    branchEqualTypeIfNeeded(MIRType_Int32, maybeInput, tag, &isInt32);
    if (conversion == IntConversion_Any || conversion == IntConversion_NumbersOrBoolsOnly)
        branchEqualTypeIfNeeded(MIRType_Boolean, maybeInput, tag, &isBool);
    branchEqualTypeIfNeeded(MIRType_Double, maybeInput, tag, &isDouble);

    if (conversion == IntConversion_Any) {
        
        
        switch (behavior) {
          case IntConversion_Normal:
          case IntConversion_NegativeZeroCheck:
            branchTestNull(Assembler::NotEqual, tag, fail);
            break;

          case IntConversion_Truncate:
          case IntConversion_ClampToUint8:
            branchEqualTypeIfNeeded(MIRType_Null, maybeInput, tag, &isNull);
            if (handleStrings)
                branchEqualTypeIfNeeded(MIRType_String, maybeInput, tag, &isString);
            branchEqualTypeIfNeeded(MIRType_Object, maybeInput, tag, fail);
            branchTestUndefined(Assembler::NotEqual, tag, fail);
            break;
        }
    } else {
        jump(fail);
    }

    
    if (isNull.used())
        bind(&isNull);
    mov(ImmWord(0), output);
    jump(&done);

    
    if (handleStrings) {
        bind(&isString);
        unboxString(value, stringReg);
        jump(handleStringEntry);
    }

    
    if (isDouble.used() || handleStrings) {
        if (isDouble.used()) {
            bind(&isDouble);
            unboxDouble(value, temp);
        }

        if (handleStrings)
            bind(handleStringRejoin);

        convertDoubleToInt(temp, output, temp, truncateDoubleSlow, fail, behavior);
        jump(&done);
    }

    
    if (isBool.used()) {
        bind(&isBool);
        unboxBoolean(value, output);
        jump(&done);
    }

    
    if (isInt32.used()) {
        bind(&isInt32);
        unboxInt32(value, output);
        if (behavior == IntConversion_ClampToUint8)
            clampIntToUint8(output);
    }

    bind(&done);
}

bool
MacroAssembler::convertValueToInt(JSContext *cx, const Value &v, Register output, Label *fail,
                                  IntConversionBehavior behavior)
{
    bool handleStrings = (behavior == IntConversion_Truncate ||
                          behavior == IntConversion_ClampToUint8);

    if (v.isNumber() || (handleStrings && v.isString())) {
        double d;
        if (v.isNumber())
            d = v.toNumber();
        else if (!StringToNumber(cx, v.toString(), &d))
            return false;

        switch (behavior) {
          case IntConversion_Normal:
          case IntConversion_NegativeZeroCheck: {
            
            int i;
            if (mozilla::NumberIsInt32(d, &i))
                move32(Imm32(i), output);
            else
                jump(fail);
            break;
          }
          case IntConversion_Truncate:
            move32(Imm32(js::ToInt32(d)), output);
            break;
          case IntConversion_ClampToUint8:
            move32(Imm32(ClampDoubleToUint8(d)), output);
            break;
        }

        return true;
    }

    if (v.isBoolean()) {
        move32(Imm32(v.toBoolean() ? 1 : 0), output);
        return true;
    }

    if (v.isNull() || v.isUndefined()) {
        move32(Imm32(0), output);
        return true;
    }

    JS_ASSERT(v.isObject());

    jump(fail);
    return true;
}

bool
MacroAssembler::convertConstantOrRegisterToInt(JSContext *cx, ConstantOrRegister src,
                                               FloatRegister temp, Register output,
                                               Label *fail, IntConversionBehavior behavior)
{
    if (src.constant())
        return convertValueToInt(cx, src.value(), output, fail, behavior);

    convertTypedOrValueToInt(src.reg(), temp, output, fail, behavior);
    return true;
}

void
MacroAssembler::convertTypedOrValueToInt(TypedOrValueRegister src, FloatRegister temp,
                                         Register output, Label *fail,
                                         IntConversionBehavior behavior)
{
    if (src.hasValue()) {
        convertValueToInt(src.valueReg(), temp, output, fail, behavior);
        return;
    }

    switch (src.type()) {
      case MIRType_Undefined:
      case MIRType_Null:
        move32(Imm32(0), output);
        break;
      case MIRType_Boolean:
      case MIRType_Int32:
        if (src.typedReg().gpr() != output)
            move32(src.typedReg().gpr(), output);
        if (src.type() == MIRType_Int32 && behavior == IntConversion_ClampToUint8)
            clampIntToUint8(output);
        break;
      case MIRType_Double:
        convertDoubleToInt(src.typedReg().fpu(), output, temp, nullptr, fail, behavior);
        break;
      case MIRType_Float32:
        
        convertFloat32ToDouble(src.typedReg().fpu(), temp);
        convertDoubleToInt(temp, output, temp, nullptr, fail, behavior);
        break;
      case MIRType_String:
      case MIRType_Object:
        jump(fail);
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("Bad MIRType");
    }
}

void
MacroAssembler::finish()
{
    if (sequentialFailureLabel_.used()) {
        bind(&sequentialFailureLabel_);
        handleFailure(SequentialExecution);
    }
    if (parallelFailureLabel_.used()) {
        bind(&parallelFailureLabel_);
        handleFailure(ParallelExecution);
    }

    MacroAssemblerSpecific::finish();
}

void
MacroAssembler::branchIfNotInterpretedConstructor(Register fun, Register scratch, Label *label)
{
    
    
    JS_ASSERT(JSFunction::offsetOfNargs() % sizeof(uint32_t) == 0);
    JS_ASSERT(JSFunction::offsetOfFlags() == JSFunction::offsetOfNargs() + 2);
    JS_STATIC_ASSERT(IS_LITTLE_ENDIAN);

    
    
    
    
    
    

    
    load32(Address(fun, JSFunction::offsetOfNargs()), scratch);
    branchTest32(Assembler::Zero, scratch, Imm32(JSFunction::INTERPRETED << 16), label);

    
    
    Label done;
    uint32_t bits = (JSFunction::IS_FUN_PROTO | JSFunction::ARROW | JSFunction::SELF_HOSTED) << 16;
    branchTest32(Assembler::Zero, scratch, Imm32(bits), &done);
    {
        
        
        
        branchTest32(Assembler::Zero, scratch, Imm32(JSFunction::SELF_HOSTED_CTOR << 16), label);

#ifdef DEBUG
        
        branchTest32(Assembler::Zero, scratch, Imm32(JSFunction::IS_FUN_PROTO << 16), &done);
        breakpoint();
#endif
    }
    bind(&done);
}

void
MacroAssembler::branchEqualTypeIfNeeded(MIRType type, MDefinition *maybeDef, Register tag,
                                        Label *label)
{
    if (!maybeDef || maybeDef->mightBeType(type)) {
        switch (type) {
          case MIRType_Null:
            branchTestNull(Equal, tag, label);
            break;
          case MIRType_Boolean:
            branchTestBoolean(Equal, tag, label);
            break;
          case MIRType_Int32:
            branchTestInt32(Equal, tag, label);
            break;
          case MIRType_Double:
            branchTestDouble(Equal, tag, label);
            break;
          case MIRType_String:
            branchTestString(Equal, tag, label);
            break;
          case MIRType_Object:
            branchTestObject(Equal, tag, label);
            break;
          default:
            MOZ_ASSUME_UNREACHABLE("Unsupported type");
        }
    }
}






const char MacroAssembler::enterJitLabel[] = "EnterJIT";




void
MacroAssembler::spsMarkJit(SPSProfiler *p, Register framePtr, Register temp)
{
    Label spsNotEnabled;
    uint32_t *enabledAddr = p->addressOfEnabled();
    load32(AbsoluteAddress(enabledAddr), temp);
    push(temp); 
    branchTest32(Assembler::Equal, temp, temp, &spsNotEnabled);

    Label stackFull;
    
    
    spsProfileEntryAddressSafe(p, 0, temp, &stackFull);

    storePtr(ImmPtr(enterJitLabel), Address(temp, ProfileEntry::offsetOfString()));
    storePtr(framePtr,              Address(temp, ProfileEntry::offsetOfStackAddress()));
    storePtr(ImmWord(uintptr_t(0)), Address(temp, ProfileEntry::offsetOfScript()));
    store32(Imm32(ProfileEntry::NullPCIndex), Address(temp, ProfileEntry::offsetOfPCIdx()));

    
    bind(&stackFull);
    loadPtr(AbsoluteAddress(p->addressOfSizePointer()), temp);
    add32(Imm32(1), Address(temp, 0));

    bind(&spsNotEnabled);
}



void
MacroAssembler::spsUnmarkJit(SPSProfiler *p, Register temp)
{
    Label spsNotEnabled;
    pop(temp); 
    branchTest32(Assembler::Equal, temp, temp, &spsNotEnabled);

    spsPopFrameSafe(p, temp);

    bind(&spsNotEnabled);
}
