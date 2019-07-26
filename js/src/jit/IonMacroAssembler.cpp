





#include "jit/IonMacroAssembler.h"

#include "jsinfer.h"
#include "jsprf.h"

#include "builtin/TypeRepresentation.h"
#include "jit/Bailouts.h"
#include "jit/BaselineFrame.h"
#include "jit/BaselineIC.h"
#include "jit/BaselineJIT.h"
#include "jit/BaselineRegisters.h"
#include "jit/Lowering.h"
#include "jit/MIR.h"
#include "vm/ForkJoin.h"

#include "jsgcinlines.h"
#include "jsinferinlines.h"

using namespace js;
using namespace js::jit;

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
        return NULL;
    }
    inline types::TypeObject *getTypeObject(unsigned) const {
        if (t_.isTypeObject())
            return t_.typeObject();
        return NULL;
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
MacroAssembler::PushRegsInMask(RegisterSet set)
{
    int32_t diffF = set.fpus().size() * sizeof(double);
    int32_t diffG = set.gprs().size() * STACK_SLOT_SIZE;

#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    
    
    for (GeneralRegisterBackwardIterator iter(set.gprs()); iter.more(); iter++) {
        diffG -= STACK_SLOT_SIZE;
        Push(*iter);
    }
#elif defined(JS_CPU_ARM)
    if (set.gprs().size() > 1) {
        adjustFrame(diffG);
        startDataTransferM(IsStore, StackPointer, DB, WriteBack);
        for (GeneralRegisterBackwardIterator iter(set.gprs()); iter.more(); iter++) {
            diffG -= STACK_SLOT_SIZE;
            transferReg(*iter);
        }
        finishDataTransfer();
    } else {
        reserveStack(diffG);
        for (GeneralRegisterBackwardIterator iter(set.gprs()); iter.more(); iter++) {
            diffG -= STACK_SLOT_SIZE;
            storePtr(*iter, Address(StackPointer, diffG));
        }
    }
#else
    reserveStack(diffG);
    for (GeneralRegisterBackwardIterator iter(set.gprs()); iter.more(); iter++) {
        diffG -= STACK_SLOT_SIZE;
        storePtr(*iter, Address(StackPointer, diffG));
    }
#endif
    JS_ASSERT(diffG == 0);

#ifdef JS_CPU_ARM
    adjustFrame(diffF);
    diffF += transferMultipleByRuns(set.fpus(), IsStore, StackPointer, DB);
#else
    reserveStack(diffF);
    for (FloatRegisterBackwardIterator iter(set.fpus()); iter.more(); iter++) {
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
        adjustFrame(-reservedF);
    } else
#endif
    {
        for (FloatRegisterBackwardIterator iter(set.fpus()); iter.more(); iter++) {
            diffF -= sizeof(double);
            if (!ignore.has(*iter))
                loadDouble(Address(StackPointer, diffF), *iter);
        }
        freeStack(reservedF);
    }
    JS_ASSERT(diffF == 0);

#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    
    
    
    if (ignore.empty(false)) {
        for (GeneralRegisterForwardIterator iter(set.gprs()); iter.more(); iter++) {
            diffG -= STACK_SLOT_SIZE;
            Pop(*iter);
        }
    } else
#endif
#ifdef JS_CPU_ARM
    if (set.gprs().size() > 1 && ignore.empty(false)) {
        startDataTransferM(IsLoad, StackPointer, IA, WriteBack);
        for (GeneralRegisterBackwardIterator iter(set.gprs()); iter.more(); iter++) {
            diffG -= STACK_SLOT_SIZE;
            transferReg(*iter);
        }
        finishDataTransfer();
        adjustFrame(-reservedG);
    } else
#endif
    {
        for (GeneralRegisterBackwardIterator iter(set.gprs()); iter.more(); iter++) {
            diffG -= STACK_SLOT_SIZE;
            if (!ignore.has(*iter))
                loadPtr(Address(StackPointer, diffG), *iter);
        }
        freeStack(reservedG);
    }
    JS_ASSERT(diffG == 0);
}

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
MacroAssembler::moveNurseryPtr(const ImmMaybeNurseryPtr &ptr, const Register &reg)
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
      case ScalarTypeRepresentation::TYPE_FLOAT32:
        if (LIRGenerator::allowFloat32Optimizations()) {
            masm.storeFloat(value, dest);
        } else {
#ifdef JS_MORE_DETERMINISTIC
            
            masm.canonicalizeDouble(value);
#endif
            masm.convertDoubleToFloat(value, ScratchFloatReg);
            masm.storeFloat(ScratchFloatReg, dest);
        }
        break;
      case ScalarTypeRepresentation::TYPE_FLOAT64:
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
      case ScalarTypeRepresentation::TYPE_INT8:
        load8SignExtend(src, dest.gpr());
        break;
      case ScalarTypeRepresentation::TYPE_UINT8:
      case ScalarTypeRepresentation::TYPE_UINT8_CLAMPED:
        load8ZeroExtend(src, dest.gpr());
        break;
      case ScalarTypeRepresentation::TYPE_INT16:
        load16SignExtend(src, dest.gpr());
        break;
      case ScalarTypeRepresentation::TYPE_UINT16:
        load16ZeroExtend(src, dest.gpr());
        break;
      case ScalarTypeRepresentation::TYPE_INT32:
        load32(src, dest.gpr());
        break;
      case ScalarTypeRepresentation::TYPE_UINT32:
        if (dest.isFloat()) {
            load32(src, temp);
            convertUInt32ToDouble(temp, dest.fpu());
        } else {
            load32(src, dest.gpr());
            test32(dest.gpr(), dest.gpr());
            j(Assembler::Signed, fail);
        }
        break;
      case ScalarTypeRepresentation::TYPE_FLOAT32:
        if (LIRGenerator::allowFloat32Optimizations()) {
            loadFloat(src, dest.fpu());
            canonicalizeFloat(dest.fpu());
        } else {
            loadFloatAsDouble(src, dest.fpu());
            canonicalizeDouble(dest.fpu());
        }
        break;
      case ScalarTypeRepresentation::TYPE_FLOAT64:
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
      case ScalarTypeRepresentation::TYPE_INT8:
      case ScalarTypeRepresentation::TYPE_UINT8:
      case ScalarTypeRepresentation::TYPE_UINT8_CLAMPED:
      case ScalarTypeRepresentation::TYPE_INT16:
      case ScalarTypeRepresentation::TYPE_UINT16:
      case ScalarTypeRepresentation::TYPE_INT32:
        loadFromTypedArray(arrayType, src, AnyRegister(dest.scratchReg()), InvalidReg, NULL);
        tagValue(JSVAL_TYPE_INT32, dest.scratchReg(), dest);
        break;
      case ScalarTypeRepresentation::TYPE_UINT32:
        
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
      case ScalarTypeRepresentation::TYPE_FLOAT32:
        loadFromTypedArray(arrayType, src, AnyRegister(ScratchFloatReg), dest.scratchReg(), NULL);
        if (LIRGenerator::allowFloat32Optimizations())
            convertFloatToDouble(ScratchFloatReg, ScratchFloatReg);
        boxDouble(ScratchFloatReg, dest);
        break;
      case ScalarTypeRepresentation::TYPE_FLOAT64:
        loadFromTypedArray(arrayType, src, AnyRegister(ScratchFloatReg), dest.scratchReg(), NULL);
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
MacroAssembler::clampDoubleToUint8(FloatRegister input, Register output)
{
    JS_ASSERT(input != ScratchFloatReg);
#ifdef JS_CPU_ARM
    ma_vimm(0.5, ScratchFloatReg);
    if (hasVFPv3()) {
        Label notSplit;
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

    } else {
        Label outOfRange;
        ma_vcmpz(input);
        
        ma_vadd(input, ScratchFloatReg, input);
        
        as_vcvt(VFPRegister(ScratchFloatReg).uintOverlay(), VFPRegister(input));
        
        as_vxfer(output, InvalidReg, ScratchFloatReg, FloatToCore);
        as_vmrs(pc);
        ma_b(&outOfRange, Overflow);
        ma_cmp(output, Imm32(0xff));
        ma_mov(Imm32(0xff), output, NoSetCond, Above);
        ma_b(&outOfRange, Above);
        
        as_vcvt(ScratchFloatReg, VFPRegister(ScratchFloatReg).uintOverlay());
        
        as_vcmp(ScratchFloatReg, input);
        as_vmrs(pc);
        ma_bic(Imm32(1), output, NoSetCond, Zero);
        bind(&outOfRange);
    }
#else

    Label positive, done;

    
    zeroDouble(ScratchFloatReg);
    branchDouble(DoubleGreaterThan, input, ScratchFloatReg, &positive);
    {
        move32(Imm32(0), output);
        jump(&done);
    }

    bind(&positive);

    
    loadConstantDouble(0.5, ScratchFloatReg);
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
MacroAssembler::newGCThing(const Register &result, gc::AllocKind allocKind, Label *fail)
{
    

    int thingSize = int(gc::Arena::thingSize(allocKind));

    Zone *zone = GetIonContext()->compartment->zone();

#ifdef JS_GC_ZEAL
    
    branch32(Assembler::NotEqual,
             AbsoluteAddress(&GetIonContext()->runtime->gcZeal_), Imm32(0),
             fail);
#endif

    
    
    if (GetIonContext()->compartment->objectMetadataCallback)
        jump(fail);

#ifdef JSGC_GENERATIONAL
    Nursery &nursery = GetIonContext()->runtime->gcNursery;
    if (nursery.isEnabled() && allocKind <= gc::FINALIZE_OBJECT_LAST) {
        
        
        
        loadPtr(AbsoluteAddress(nursery.addressOfPosition()), result);
        addPtr(Imm32(thingSize), result);
        branchPtr(Assembler::BelowOrEqual, AbsoluteAddress(nursery.addressOfCurrentEnd()), result, fail);
        storePtr(result, AbsoluteAddress(nursery.addressOfPosition()));
        subPtr(Imm32(thingSize), result);
        return;
    }
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
MacroAssembler::newGCThing(const Register &result, JSObject *templateObject, Label *fail)
{
    gc::AllocKind allocKind = templateObject->tenuredGetAllocKind();
    JS_ASSERT(allocKind >= gc::FINALIZE_OBJECT0 && allocKind <= gc::FINALIZE_OBJECT_LAST);
    JS_ASSERT(!templateObject->hasDynamicElements());

    newGCThing(result, allocKind, fail);
}

void
MacroAssembler::newGCString(const Register &result, Label *fail)
{
    newGCThing(result, js::gc::FINALIZE_STRING, fail);
}

void
MacroAssembler::newGCShortString(const Register &result, Label *fail)
{
    newGCThing(result, js::gc::FINALIZE_SHORT_STRING, fail);
}

void
MacroAssembler::newGCThingPar(const Register &result, const Register &slice,
                              const Register &tempReg1, const Register &tempReg2,
                              gc::AllocKind allocKind, Label *fail)
{
    
    
    
    
    
    
    

    uint32_t thingSize = (uint32_t)gc::Arena::thingSize(allocKind);

    
    
    loadPtr(Address(slice, ThreadSafeContext::offsetOfAllocator()),
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
MacroAssembler::newGCThingPar(const Register &result, const Register &slice,
                              const Register &tempReg1, const Register &tempReg2,
                              JSObject *templateObject, Label *fail)
{
    gc::AllocKind allocKind = templateObject->tenuredGetAllocKind();
    JS_ASSERT(allocKind >= gc::FINALIZE_OBJECT0 && allocKind <= gc::FINALIZE_OBJECT_LAST);
    JS_ASSERT(!templateObject->hasDynamicElements());

    newGCThingPar(result, slice, tempReg1, tempReg2, allocKind, fail);
}

void
MacroAssembler::newGCStringPar(const Register &result, const Register &slice,
                               const Register &tempReg1, const Register &tempReg2,
                               Label *fail)
{
    newGCThingPar(result, slice, tempReg1, tempReg2, js::gc::FINALIZE_STRING, fail);
}

void
MacroAssembler::newGCShortStringPar(const Register &result, const Register &slice,
                                    const Register &tempReg1, const Register &tempReg2,
                                    Label *fail)
{
    newGCThingPar(result, slice, tempReg1, tempReg2, js::gc::FINALIZE_SHORT_STRING, fail);
}

void
MacroAssembler::initGCThing(const Register &obj, JSObject *templateObject)
{
    

    storePtr(ImmGCPtr(templateObject->lastProperty()), Address(obj, JSObject::offsetOfShape()));
    storePtr(ImmGCPtr(templateObject->type()), Address(obj, JSObject::offsetOfType()));
    storePtr(ImmPtr(NULL), Address(obj, JSObject::offsetOfSlots()));

    if (templateObject->is<ArrayObject>()) {
        JS_ASSERT(!templateObject->getDenseInitializedLength());

        int elementsOffset = JSObject::offsetOfFixedElements();

        addPtr(Imm32(elementsOffset), obj);
        storePtr(obj, Address(obj, -elementsOffset + JSObject::offsetOfElements()));
        addPtr(Imm32(-elementsOffset), obj);

        
        store32(Imm32(templateObject->getDenseCapacity()),
                Address(obj, elementsOffset + ObjectElements::offsetOfCapacity()));
        store32(Imm32(templateObject->getDenseInitializedLength()),
                Address(obj, elementsOffset + ObjectElements::offsetOfInitializedLength()));
        store32(Imm32(templateObject->as<ArrayObject>().length()),
                Address(obj, elementsOffset + ObjectElements::offsetOfLength()));
        store32(Imm32(templateObject->shouldConvertDoubleElements()
                      ? ObjectElements::CONVERT_DOUBLE_ELEMENTS
                      : 0),
                Address(obj, elementsOffset + ObjectElements::offsetOfFlags()));
    } else {
        storePtr(ImmPtr(emptyObjectElements), Address(obj, JSObject::offsetOfElements()));

        
        
        size_t nslots = Min(templateObject->numFixedSlots(), templateObject->slotSpan());
        for (unsigned i = 0; i < nslots; i++) {
            storeValue(templateObject->getFixedSlot(i),
                       Address(obj, JSObject::getFixedSlotOffset(i)));
        }
    }

    if (templateObject->hasPrivate()) {
        uint32_t nfixed = templateObject->numFixedSlots();
        storePtr(ImmPtr(templateObject->getPrivate()),
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
MacroAssembler::checkInterruptFlagsPar(const Register &tempReg,
                                            Label *fail)
{
    movePtr(ImmPtr(&GetIonContext()->runtime->interrupt), tempReg);
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
        

        
        JS_ASSERT(CalleeToken_Function == 0);

        loadPtr(Address(script, JSFunction::offsetOfNativeOrScript()), script);
    }

    bind(&performOsr);

    const Register ionScript = regs.takeAny();
    const Register osrEntry = regs.takeAny();

    loadPtr(Address(script, JSScript::offsetOfIonScript()), ionScript);
    load32(Address(ionScript, IonScript::offsetOfOsrEntryOffset()), osrEntry);

    
    const Register code = ionScript;
    loadPtr(Address(ionScript, IonScript::offsetOfMethod()), code);
    loadPtr(Address(code, IonCode::offsetOfCode()), code);
    addPtr(osrEntry, code);

    
    
    enterOsr(calleeToken, code);
    ret();
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
        makeFrameDescriptor(temp, IonFrame_BaselineJS);
        push(temp);
        loadPtr(Address(bailoutInfo, offsetof(BaselineBailoutInfo, resumeAddr)), temp);
        push(temp);
        enterFakeExitFrame();

        
        Label noMonitor;
        Label done;
        branchPtr(Assembler::Equal,
                  Address(bailoutInfo, offsetof(BaselineBailoutInfo, monitorStub)),
                  ImmPtr(NULL),
                  &noMonitor);

        
        
        
        {
            
            pushValue(Address(bailoutInfo, offsetof(BaselineBailoutInfo, valueR0)));
            loadPtr(Address(bailoutInfo, offsetof(BaselineBailoutInfo, resumeFramePtr)), temp);
            push(temp);
            loadPtr(Address(bailoutInfo, offsetof(BaselineBailoutInfo, resumeAddr)), temp);
            push(temp);
            loadPtr(Address(bailoutInfo, offsetof(BaselineBailoutInfo, monitorStub)), temp);
            push(temp);

            
            setupUnalignedABICall(1, temp);
            passABIArg(bailoutInfo);
            callWithABI(JS_FUNC_TO_DATA_PTR(void *, FinishBailoutToBaseline));
            branchTest32(Zero, ReturnReg, ReturnReg, exceptionLabel());

            
            GeneralRegisterSet enterMonRegs(GeneralRegisterSet::All());
            enterMonRegs.take(R0);
            enterMonRegs.take(BaselineStubReg);
            enterMonRegs.take(BaselineFrameReg);
            enterMonRegs.takeUnchecked(BaselineTailCallReg);
            Register jitcodeReg = enterMonRegs.takeAny();

            pop(BaselineStubReg);
            pop(BaselineTailCallReg);
            pop(BaselineFrameReg);
            popValue(R0);

            
            addPtr(Imm32(IonExitFrameLayout::SizeWithFooter()), StackPointer);

            loadPtr(Address(BaselineStubReg, ICStub::offsetOfStubCode()), jitcodeReg);
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
            push(BaselineTailCallReg);
#endif
            jump(jitcodeReg);
        }

        
        
        
        bind(&noMonitor);
        {
            
            pushValue(Address(bailoutInfo, offsetof(BaselineBailoutInfo, valueR0)));
            pushValue(Address(bailoutInfo, offsetof(BaselineBailoutInfo, valueR1)));
            loadPtr(Address(bailoutInfo, offsetof(BaselineBailoutInfo, resumeFramePtr)), temp);
            push(temp);
            loadPtr(Address(bailoutInfo, offsetof(BaselineBailoutInfo, resumeAddr)), temp);
            push(temp);

            
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
        loadPtr(Address(dest, IonCode::offsetOfCode()), dest);
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
        loadPtr(Address(dest, IonCode::offsetOfCode()), dest);
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
MacroAssembler::loadForkJoinSlice(Register slice, Register scratch)
{
    
    
    
    setupUnalignedABICall(0, scratch);
    callWithABI(JS_FUNC_TO_DATA_PTR(void *, ForkJoinSlicePar));
    if (ReturnReg != slice)
        movePtr(ReturnReg, slice);
}

void
MacroAssembler::loadContext(Register cxReg, Register scratch, ExecutionMode executionMode)
{
    switch (executionMode) {
      case SequentialExecution:
        
        loadJSContext(cxReg);
        break;
      case ParallelExecution:
        loadForkJoinSlice(cxReg, scratch);
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("No such execution mode");
    }
}

void
MacroAssembler::enterParallelExitFrameAndLoadSlice(const VMFunction *f, Register slice,
                                                   Register scratch)
{
    loadForkJoinSlice(slice, scratch);
    
    loadPtr(Address(slice, offsetof(ForkJoinSlice, perThreadData)), scratch);
    linkParallelExitFrame(scratch);
    
    exitCodePatch_ = PushWithPatch(ImmWord(-1));
    
    Push(ImmPtr(f));
}

void
MacroAssembler::enterFakeParallelExitFrame(Register slice, Register scratch,
                                           IonCode *codeVal)
{
    
    loadPtr(Address(slice, offsetof(ForkJoinSlice, perThreadData)), scratch);
    linkParallelExitFrame(scratch);
    Push(ImmPtr(codeVal));
    Push(ImmPtr(NULL));
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
        enterParallelExitFrameAndLoadSlice(f, cxReg, scratch);
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("No such execution mode");
    }
}

void
MacroAssembler::enterFakeExitFrame(Register cxReg, Register scratch,
                                   ExecutionMode executionMode,
                                   IonCode *codeVal)
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

void
MacroAssembler::pushCalleeToken(Register callee, ExecutionMode mode)
{
    
    
    
    tagCallee(callee, mode);
    push(callee);
    clearCalleeTag(callee, mode);
}

void
MacroAssembler::PushCalleeToken(Register callee, ExecutionMode mode)
{
    tagCallee(callee, mode);
    Push(callee);
    clearCalleeTag(callee, mode);
}

void
MacroAssembler::tagCallee(Register callee, ExecutionMode mode)
{
    switch (mode) {
      case SequentialExecution:
        
        return;
      case ParallelExecution:
        orPtr(Imm32(CalleeToken_ParallelFunction), callee);
        return;
      default:
        MOZ_ASSUME_UNREACHABLE("No such execution mode");
    }
}

void
MacroAssembler::clearCalleeTag(Register callee, ExecutionMode mode)
{
    switch (mode) {
      case SequentialExecution:
        
        return;
      case ParallelExecution:
        andPtr(Imm32(~0x3), callee);
        return;
      default:
        MOZ_ASSUME_UNREACHABLE("No such execution mode");
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
    movePtr(ImmPtr(output), temp);
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
    movePtr(ImmPtr(output), temp);
    passABIArg(temp);
    passABIArg(value);
    callWithABI(JS_FUNC_TO_DATA_PTR(void *, printf1_));

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
    callWithABI(JS_FUNC_TO_DATA_PTR(void *, TraceLogStart));

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
    callWithABI(JS_FUNC_TO_DATA_PTR(void *, TraceLogStop));

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
    callWithABI(JS_FUNC_TO_DATA_PTR(void *, TraceLogStop));

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

static const double DoubleZero = 0.0;
static const double DoubleOne  = 1.0;
static const float FloatZero = 0.0;
static const float FloatOne  = 1.0;
static const float FloatNaN = js_NaN;

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

    
    loadConstantFloatingPoint(js_NaN, FloatNaN, output, outputType);
    jump(&done);

    bind(&isNull);
    loadConstantFloatingPoint(DoubleZero, FloatZero, output, outputType);
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
        convertDoubleToFloat(output, output);
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
            loadConstantFloatingPoint(DoubleOne, FloatOne, output, outputType);
        else
            loadConstantFloatingPoint(DoubleZero, FloatZero, output, outputType);
        return true;
    }

    if (v.isNull()) {
        loadConstantFloatingPoint(DoubleZero, FloatZero, output, outputType);
        return true;
    }

    if (v.isUndefined()) {
        loadConstantFloatingPoint(js_NaN, FloatNaN, output, outputType);
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
        Push(ImmPtr(NULL));
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
        loadConstantFloatingPoint(DoubleZero, FloatZero, output, outputType);
        break;
      case MIRType_Boolean:
      case MIRType_Int32:
        convertInt32ToFloatingPoint(src.typedReg().gpr(), output, outputType);
        break;
      case MIRType_Float32:
        if (outputIsDouble) {
            convertFloatToDouble(src.typedReg().fpu(), output);
        } else {
            if (src.typedReg().fpu() != output)
                moveFloat(src.typedReg().fpu(), output);
        }
        break;
      case MIRType_Double:
        if (outputIsDouble) {
            if (src.typedReg().fpu() != output)
                moveDouble(src.typedReg().fpu(), output);
        } else {
            convertDoubleToFloat(src.typedReg().fpu(), output);
        }
        break;
      case MIRType_Object:
      case MIRType_String:
        jump(fail);
        break;
      case MIRType_Undefined:
        loadConstantFloatingPoint(js_NaN, FloatNaN, output, outputType);
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
                                  Label *fail, IntConversionBehavior behavior)
{
    Register tag = splitTagForTest(value);
    bool handleStrings = (behavior == IntConversion_Truncate ||
                          behavior == IntConversion_ClampToUint8) &&
                         handleStringEntry &&
                         handleStringRejoin;
    bool zeroObjects = behavior == IntConversion_ClampToUint8;

    Label done, isInt32, isBool, isDouble, isNull, isString;

    branchEqualTypeIfNeeded(MIRType_Int32, maybeInput, tag, &isInt32);
    branchEqualTypeIfNeeded(MIRType_Boolean, maybeInput, tag, &isBool);
    branchEqualTypeIfNeeded(MIRType_Double, maybeInput, tag, &isDouble);

    
    
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
        if (zeroObjects)
            branchEqualTypeIfNeeded(MIRType_Object, maybeInput, tag, &isNull);
        branchTestUndefined(Assembler::NotEqual, tag, fail);
        break;
    }

    
    if (isNull.used())
        bind(&isNull);
    mov(Imm32(0), output);
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
    bool zeroObjects = behavior == IntConversion_ClampToUint8;

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
            if (mozilla::DoubleIsInt32(d, &i))
                move32(Imm32(i), output);
            else
                jump(fail);
            break;
          }
          case IntConversion_Truncate:
            move32(Imm32(ToInt32(d)), output);
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

    if (zeroObjects)
        move32(Imm32(0), output);
    else
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
        convertDoubleToInt(src.typedReg().fpu(), output, temp, NULL, fail, behavior);
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
    
    
    JS_STATIC_ASSERT(offsetof(JSFunction, nargs) % sizeof(uint32_t) == 0);
    JS_STATIC_ASSERT(offsetof(JSFunction, flags) == offsetof(JSFunction, nargs) + 2);
    JS_STATIC_ASSERT(IS_LITTLE_ENDIAN);

    
    
    
    
    
    

    
    load32(Address(fun, offsetof(JSFunction, nargs)), scratch);
    branchTest32(Assembler::Zero, scratch, Imm32(JSFunction::INTERPRETED << 16), label);

    
    
    Label done;
    uint32_t bits = (JSFunction::IS_FUN_PROTO | JSFunction::SELF_HOSTED) << 16;
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
MacroAssembler::branchEqualTypeIfNeeded(MIRType type, MDefinition *maybeDef, const Register &tag,
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
