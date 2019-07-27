





#include "jit/MacroAssembler.h"

#include "jsinfer.h"
#include "jsprf.h"

#include "builtin/TypedObject.h"
#include "gc/GCTrace.h"
#include "jit/AtomicOp.h"
#include "jit/Bailouts.h"
#include "jit/BaselineFrame.h"
#include "jit/BaselineIC.h"
#include "jit/BaselineJIT.h"
#include "jit/Lowering.h"
#include "jit/MIR.h"
#include "jit/ParallelFunctions.h"
#include "vm/ForkJoin.h"
#include "vm/TraceLogging.h"

#include "jsgcinlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "vm/Interpreter-inl.h"

using namespace js;
using namespace js::jit;

using JS::GenericNaN;

namespace {



class TypeWrapper {
    types::Type t_;

  public:
    explicit TypeWrapper(types::Type t) : t_(t) {}

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
    inline JSObject *getSingleObjectNoBarrier(unsigned) const {
        if (t_.isSingleObject())
            return t_.singleObjectNoBarrier();
        return nullptr;
    }
    inline types::TypeObject *getTypeObjectNoBarrier(unsigned) const {
        if (t_.isTypeObject())
            return t_.typeObjectNoBarrier();
        return nullptr;
    }
};

} 

template <typename Source, typename TypeSet> void
MacroAssembler::guardTypeSet(const Source &address, const TypeSet *types, BarrierKind kind,
                             Register scratch, Label *miss)
{
    MOZ_ASSERT(kind == BarrierKind::TypeTagOnly || kind == BarrierKind::TypeSet);
    MOZ_ASSERT(!types->unknown());

    Label matched;
    types::Type tests[8] = {
        types::Type::Int32Type(),
        types::Type::UndefinedType(),
        types::Type::BooleanType(),
        types::Type::StringType(),
        types::Type::SymbolType(),
        types::Type::NullType(),
        types::Type::MagicArgType(),
        types::Type::AnyObjectType()
    };

    
    
    if (types->hasType(types::Type::DoubleType())) {
        MOZ_ASSERT(types->hasType(types::Type::Int32Type()));
        tests[0] = types::Type::DoubleType();
    }

    Register tag = extractTag(address, scratch);

    
    BranchType lastBranch;
    for (size_t i = 0; i < mozilla::ArrayLength(tests); i++) {
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

    
    MOZ_ASSERT(scratch != InvalidReg);
    branchTestObject(NotEqual, tag, miss);
    if (kind != BarrierKind::TypeTagOnly) {
        Register obj = extractObject(address, scratch);
        guardObjectType(obj, types, scratch, miss);
    } else {
#ifdef DEBUG
        Label fail;
        Register obj = extractObject(address, scratch);
        guardObjectType(obj, types, scratch, &fail);
        jump(&matched);
        bind(&fail);
        assumeUnreachable("Unexpected object type");
#endif
    }

    bind(&matched);
}

template <typename TypeSet> void
MacroAssembler::guardObjectType(Register obj, const TypeSet *types,
                                Register scratch, Label *miss)
{
    MOZ_ASSERT(!types->unknown());
    MOZ_ASSERT(!types->hasType(types::Type::AnyObjectType()));
    MOZ_ASSERT(types->getObjectCount());
    MOZ_ASSERT(scratch != InvalidReg);

    
    
    
    
    
    
    Label matched;

    BranchGCPtr lastBranch;
    MOZ_ASSERT(!lastBranch.isInitialized());
    bool hasTypeObjects = false;
    unsigned count = types->getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        if (!types->getSingleObjectNoBarrier(i)) {
            hasTypeObjects = hasTypeObjects || types->getTypeObjectNoBarrier(i);
            continue;
        }

        if (lastBranch.isInitialized())
            lastBranch.emit(*this);

        JSObject *object = types->getSingleObjectNoBarrier(i);
        lastBranch = BranchGCPtr(Equal, obj, ImmGCPtr(object), &matched);
    }

    if (hasTypeObjects) {
        
        
        
        
        if (lastBranch.isInitialized())
            lastBranch.emit(*this);
        lastBranch = BranchGCPtr();

        
        
        loadPtr(Address(obj, JSObject::offsetOfType()), scratch);

        for (unsigned i = 0; i < count; i++) {
            if (!types->getTypeObjectNoBarrier(i))
                continue;

            if (lastBranch.isInitialized())
                lastBranch.emit(*this);

            types::TypeObject *object = types->getTypeObjectNoBarrier(i);
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
    guardTypeSet(address, &wrapper, BarrierKind::TypeSet, scratch, miss);
}

template void MacroAssembler::guardTypeSet(const Address &address, const types::TemporaryTypeSet *types,
                                           BarrierKind kind, Register scratch, Label *miss);
template void MacroAssembler::guardTypeSet(const ValueOperand &value, const types::TemporaryTypeSet *types,
                                           BarrierKind kind, Register scratch, Label *miss);

template void MacroAssembler::guardTypeSet(const Address &address, const types::HeapTypeSet *types,
                                           BarrierKind kind, Register scratch, Label *miss);
template void MacroAssembler::guardTypeSet(const ValueOperand &value, const types::HeapTypeSet *types,
                                           BarrierKind kind, Register scratch, Label *miss);
template void MacroAssembler::guardTypeSet(const TypedOrValueRegister &reg, const types::HeapTypeSet *types,
                                           BarrierKind kind, Register scratch, Label *miss);

template void MacroAssembler::guardTypeSet(const Address &address, const types::TypeSet *types,
                                           BarrierKind kind, Register scratch, Label *miss);
template void MacroAssembler::guardTypeSet(const ValueOperand &value, const types::TypeSet *types,
                                           BarrierKind kind, Register scratch, Label *miss);

template void MacroAssembler::guardTypeSet(const Address &address, const TypeWrapper *types,
                                           BarrierKind kind, Register scratch, Label *miss);
template void MacroAssembler::guardTypeSet(const ValueOperand &value, const TypeWrapper *types,
                                           BarrierKind kind, Register scratch, Label *miss);

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

template<typename S, typename T>
static void
StoreToTypedFloatArray(MacroAssembler &masm, int arrayType, const S &value, const T &dest)
{
    switch (arrayType) {
      case Scalar::Float32:
        masm.storeFloat32(value, dest);
        break;
      case Scalar::Float64:
#ifdef JS_MORE_DETERMINISTIC
        
        masm.canonicalizeDouble(value);
#endif
        masm.storeDouble(value, dest);
        break;
      default:
        MOZ_CRASH("Invalid typed array type");
    }
}

void
MacroAssembler::storeToTypedFloatArray(Scalar::Type arrayType, FloatRegister value,
                                       const BaseIndex &dest)
{
    StoreToTypedFloatArray(*this, arrayType, value, dest);
}
void
MacroAssembler::storeToTypedFloatArray(Scalar::Type arrayType, FloatRegister value,
                                       const Address &dest)
{
    StoreToTypedFloatArray(*this, arrayType, value, dest);
}

template<typename T>
void
MacroAssembler::loadFromTypedArray(Scalar::Type arrayType, const T &src, AnyRegister dest, Register temp,
                                   Label *fail)
{
    switch (arrayType) {
      case Scalar::Int8:
        load8SignExtend(src, dest.gpr());
        break;
      case Scalar::Uint8:
      case Scalar::Uint8Clamped:
        load8ZeroExtend(src, dest.gpr());
        break;
      case Scalar::Int16:
        load16SignExtend(src, dest.gpr());
        break;
      case Scalar::Uint16:
        load16ZeroExtend(src, dest.gpr());
        break;
      case Scalar::Int32:
        load32(src, dest.gpr());
        break;
      case Scalar::Uint32:
        if (dest.isFloat()) {
            load32(src, temp);
            convertUInt32ToDouble(temp, dest.fpu());
        } else {
            load32(src, dest.gpr());

            
            
            
            branchTest32(Assembler::Signed, dest.gpr(), dest.gpr(), fail);
        }
        break;
      case Scalar::Float32:
        loadFloat32(src, dest.fpu());
        canonicalizeFloat(dest.fpu());
        break;
      case Scalar::Float64:
        loadDouble(src, dest.fpu());
        canonicalizeDouble(dest.fpu());
        break;
      default:
        MOZ_CRASH("Invalid typed array type");
    }
}

template void MacroAssembler::loadFromTypedArray(Scalar::Type arrayType, const Address &src, AnyRegister dest,
                                                 Register temp, Label *fail);
template void MacroAssembler::loadFromTypedArray(Scalar::Type arrayType, const BaseIndex &src, AnyRegister dest,
                                                 Register temp, Label *fail);

template<typename T>
void
MacroAssembler::loadFromTypedArray(Scalar::Type arrayType, const T &src, const ValueOperand &dest,
                                   bool allowDouble, Register temp, Label *fail)
{
    switch (arrayType) {
      case Scalar::Int8:
      case Scalar::Uint8:
      case Scalar::Uint8Clamped:
      case Scalar::Int16:
      case Scalar::Uint16:
      case Scalar::Int32:
        loadFromTypedArray(arrayType, src, AnyRegister(dest.scratchReg()), InvalidReg, nullptr);
        tagValue(JSVAL_TYPE_INT32, dest.scratchReg(), dest);
        break;
      case Scalar::Uint32:
        
        load32(src, temp);
        if (allowDouble) {
            
            
            Label done, isDouble;
            branchTest32(Assembler::Signed, temp, temp, &isDouble);
            {
                tagValue(JSVAL_TYPE_INT32, temp, dest);
                jump(&done);
            }
            bind(&isDouble);
            {
                convertUInt32ToDouble(temp, ScratchDoubleReg);
                boxDouble(ScratchDoubleReg, dest);
            }
            bind(&done);
        } else {
            
            branchTest32(Assembler::Signed, temp, temp, fail);
            tagValue(JSVAL_TYPE_INT32, temp, dest);
        }
        break;
      case Scalar::Float32:
        loadFromTypedArray(arrayType, src, AnyRegister(ScratchFloat32Reg), dest.scratchReg(),
                           nullptr);
        convertFloat32ToDouble(ScratchFloat32Reg, ScratchDoubleReg);
        boxDouble(ScratchDoubleReg, dest);
        break;
      case Scalar::Float64:
        loadFromTypedArray(arrayType, src, AnyRegister(ScratchDoubleReg), dest.scratchReg(),
                           nullptr);
        boxDouble(ScratchDoubleReg, dest);
        break;
      default:
        MOZ_CRASH("Invalid typed array type");
    }
}

template void MacroAssembler::loadFromTypedArray(Scalar::Type arrayType, const Address &src, const ValueOperand &dest,
                                                 bool allowDouble, Register temp, Label *fail);
template void MacroAssembler::loadFromTypedArray(Scalar::Type arrayType, const BaseIndex &src, const ValueOperand &dest,
                                                 bool allowDouble, Register temp, Label *fail);

template<typename T>
void
MacroAssembler::compareExchangeToTypedIntArray(Scalar::Type arrayType, const T &mem,
                                               Register oldval, Register newval,
                                               Register temp, AnyRegister output)
{
    switch (arrayType) {
      case Scalar::Int8:
        compareExchange8SignExtend(mem, oldval, newval, output.gpr());
        break;
      case Scalar::Uint8:
        compareExchange8ZeroExtend(mem, oldval, newval, output.gpr());
        break;
      case Scalar::Uint8Clamped:
        compareExchange8ZeroExtend(mem, oldval, newval, output.gpr());
        break;
      case Scalar::Int16:
        compareExchange16SignExtend(mem, oldval, newval, output.gpr());
        break;
      case Scalar::Uint16:
        compareExchange16ZeroExtend(mem, oldval, newval, output.gpr());
        break;
      case Scalar::Int32:
        compareExchange32(mem, oldval, newval, output.gpr());
        break;
      case Scalar::Uint32:
        
        
        MOZ_ASSERT(output.isFloat());
        compareExchange32(mem, oldval, newval, temp);
        convertUInt32ToDouble(temp, output.fpu());
        break;
      default:
        MOZ_CRASH("Invalid typed array type");
    }
}

template void
MacroAssembler::compareExchangeToTypedIntArray(Scalar::Type arrayType, const Address &mem,
                                               Register oldval, Register newval, Register temp,
                                               AnyRegister output);
template void
MacroAssembler::compareExchangeToTypedIntArray(Scalar::Type arrayType, const BaseIndex &mem,
                                               Register oldval, Register newval, Register temp,
                                               AnyRegister output);

template<typename S, typename T>
void
MacroAssembler::atomicBinopToTypedIntArray(AtomicOp op, Scalar::Type arrayType, const S &value,
                                           const T &mem, Register temp1, Register temp2, AnyRegister output)
{
    
    switch (arrayType) {
      case Scalar::Int8:
        switch (op) {
          case AtomicFetchAddOp:
            atomicFetchAdd8SignExtend(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchSubOp:
            atomicFetchSub8SignExtend(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchAndOp:
            atomicFetchAnd8SignExtend(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchOrOp:
            atomicFetchOr8SignExtend(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchXorOp:
            atomicFetchXor8SignExtend(value, mem, temp1, output.gpr());
            break;
          default:
            MOZ_CRASH("Invalid typed array atomic operation");
        }
        break;
      case Scalar::Uint8:
        switch (op) {
          case AtomicFetchAddOp:
            atomicFetchAdd8ZeroExtend(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchSubOp:
            atomicFetchSub8ZeroExtend(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchAndOp:
            atomicFetchAnd8ZeroExtend(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchOrOp:
            atomicFetchOr8ZeroExtend(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchXorOp:
            atomicFetchXor8ZeroExtend(value, mem, temp1, output.gpr());
            break;
          default:
            MOZ_CRASH("Invalid typed array atomic operation");
        }
        break;
      case Scalar::Int16:
        switch (op) {
          case AtomicFetchAddOp:
            atomicFetchAdd16SignExtend(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchSubOp:
            atomicFetchSub16SignExtend(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchAndOp:
            atomicFetchAnd16SignExtend(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchOrOp:
            atomicFetchOr16SignExtend(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchXorOp:
            atomicFetchXor16SignExtend(value, mem, temp1, output.gpr());
            break;
          default:
            MOZ_CRASH("Invalid typed array atomic operation");
        }
        break;
      case Scalar::Uint16:
        switch (op) {
          case AtomicFetchAddOp:
            atomicFetchAdd16ZeroExtend(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchSubOp:
            atomicFetchSub16ZeroExtend(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchAndOp:
            atomicFetchAnd16ZeroExtend(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchOrOp:
            atomicFetchOr16ZeroExtend(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchXorOp:
            atomicFetchXor16ZeroExtend(value, mem, temp1, output.gpr());
            break;
          default:
            MOZ_CRASH("Invalid typed array atomic operation");
        }
        break;
      case Scalar::Int32:
        switch (op) {
          case AtomicFetchAddOp:
            atomicFetchAdd32(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchSubOp:
            atomicFetchSub32(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchAndOp:
            atomicFetchAnd32(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchOrOp:
            atomicFetchOr32(value, mem, temp1, output.gpr());
            break;
          case AtomicFetchXorOp:
            atomicFetchXor32(value, mem, temp1, output.gpr());
            break;
          default:
            MOZ_CRASH("Invalid typed array atomic operation");
        }
        break;
      case Scalar::Uint32:
        
        
        MOZ_ASSERT(output.isFloat());
        switch (op) {
          case AtomicFetchAddOp:
            atomicFetchAdd32(value, mem, InvalidReg, temp1);
            break;
          case AtomicFetchSubOp:
            atomicFetchSub32(value, mem, InvalidReg, temp1);
            break;
          case AtomicFetchAndOp:
            atomicFetchAnd32(value, mem, temp2, temp1);
            break;
          case AtomicFetchOrOp:
            atomicFetchOr32(value, mem, temp2, temp1);
            break;
          case AtomicFetchXorOp:
            atomicFetchXor32(value, mem, temp2, temp1);
            break;
          default:
            MOZ_CRASH("Invalid typed array atomic operation");
        }
        convertUInt32ToDouble(temp1, output.fpu());
        break;
      default:
        MOZ_CRASH("Invalid typed array type");
    }
}

template void
MacroAssembler::atomicBinopToTypedIntArray(AtomicOp op, Scalar::Type arrayType,
                                           const Imm32 &value, const Address &mem,
                                           Register temp1, Register temp2, AnyRegister output);
template void
MacroAssembler::atomicBinopToTypedIntArray(AtomicOp op, Scalar::Type arrayType,
                                           const Imm32 &value, const BaseIndex &mem,
                                           Register temp1, Register temp2, AnyRegister output);
template void
MacroAssembler::atomicBinopToTypedIntArray(AtomicOp op, Scalar::Type arrayType,
                                           const Register &value, const Address &mem,
                                           Register temp1, Register temp2, AnyRegister output);
template void
MacroAssembler::atomicBinopToTypedIntArray(AtomicOp op, Scalar::Type arrayType,
                                           const Register &value, const BaseIndex &mem,
                                           Register temp1, Register temp2, AnyRegister output);



void
MacroAssembler::checkAllocatorState(Label *fail)
{
    
    if (js::gc::TraceEnabled())
        jump(fail);

# ifdef JS_GC_ZEAL
    
    branch32(Assembler::NotEqual,
             AbsoluteAddress(GetJitContext()->runtime->addressOfGCZeal()), Imm32(0),
             fail);
# endif

    
    
    if (GetJitContext()->compartment->hasObjectMetadataCallback())
        jump(fail);
}


bool
MacroAssembler::shouldNurseryAllocate(gc::AllocKind allocKind, gc::InitialHeap initialHeap)
{
    
    
    
    
    
    return IsNurseryAllocable(allocKind) && initialHeap != gc::TenuredHeap;
}


void
MacroAssembler::nurseryAllocate(Register result, Register slots, gc::AllocKind allocKind,
                                size_t nDynamicSlots, gc::InitialHeap initialHeap, Label *fail)
{
    MOZ_ASSERT(IsNurseryAllocable(allocKind));
    MOZ_ASSERT(initialHeap != gc::TenuredHeap);

    
    
    
    if (nDynamicSlots >= Nursery::MaxNurserySlots) {
        jump(fail);
        return;
    }

    
    
    const Nursery &nursery = GetJitContext()->runtime->gcNursery();
    Register temp = slots;
    int thingSize = int(gc::Arena::thingSize(allocKind));
    int totalSize = thingSize + nDynamicSlots * sizeof(HeapSlot);
    loadPtr(AbsoluteAddress(nursery.addressOfPosition()), result);
    computeEffectiveAddress(Address(result, totalSize), temp);
    branchPtr(Assembler::Below, AbsoluteAddress(nursery.addressOfCurrentEnd()), temp, fail);
    storePtr(temp, AbsoluteAddress(nursery.addressOfPosition()));

    if (nDynamicSlots)
        computeEffectiveAddress(Address(result, thingSize), slots);
}


void
MacroAssembler::freeListAllocate(Register result, Register temp, gc::AllocKind allocKind, Label *fail)
{
    CompileZone *zone = GetJitContext()->compartment->zone();
    int thingSize = int(gc::Arena::thingSize(allocKind));

    Label fallback;
    Label success;

    
    
    loadPtr(AbsoluteAddress(zone->addressOfFreeListFirst(allocKind)), result);
    branchPtr(Assembler::BelowOrEqual, AbsoluteAddress(zone->addressOfFreeListLast(allocKind)), result, &fallback);
    computeEffectiveAddress(Address(result, thingSize), temp);
    storePtr(temp, AbsoluteAddress(zone->addressOfFreeListFirst(allocKind)));
    jump(&success);

    bind(&fallback);
    
    
    
    branchPtr(Assembler::Equal, result, ImmPtr(0), fail);
    
    loadPtr(Address(result, js::gc::FreeSpan::offsetOfFirst()), temp);
    storePtr(temp, AbsoluteAddress(zone->addressOfFreeListFirst(allocKind)));
    loadPtr(Address(result, js::gc::FreeSpan::offsetOfLast()), temp);
    storePtr(temp, AbsoluteAddress(zone->addressOfFreeListLast(allocKind)));

    bind(&success);
}

void
MacroAssembler::callMallocStub(size_t nbytes, Register result, Label *fail)
{
    
    const Register regNBytes = CallTempReg0;

    MOZ_ASSERT(nbytes > 0);
    MOZ_ASSERT(nbytes <= INT32_MAX);

    if (regNBytes != result)
        push(regNBytes);
    move32(Imm32(nbytes), regNBytes);
    call(GetJitContext()->runtime->jitRuntime()->mallocStub());
    if (regNBytes != result) {
        movePtr(regNBytes, result);
        pop(regNBytes);
    }
    branchTest32(Assembler::Zero, result, result, fail);
}

void
MacroAssembler::callFreeStub(Register slots)
{
    
    const Register regSlots = CallTempReg0;

    push(regSlots);
    movePtr(slots, regSlots);
    call(GetJitContext()->runtime->jitRuntime()->freeStub());
    pop(regSlots);
}


void
MacroAssembler::allocateObject(Register result, Register slots, gc::AllocKind allocKind,
                               uint32_t nDynamicSlots, gc::InitialHeap initialHeap, Label *fail)
{
    MOZ_ASSERT(allocKind >= gc::FINALIZE_OBJECT0 && allocKind <= gc::FINALIZE_OBJECT_LAST);

    checkAllocatorState(fail);

    if (shouldNurseryAllocate(allocKind, initialHeap))
        return nurseryAllocate(result, slots, allocKind, nDynamicSlots, initialHeap, fail);

    if (!nDynamicSlots)
        return freeListAllocate(result, slots, allocKind, fail);

    callMallocStub(nDynamicSlots * sizeof(HeapValue), slots, fail);

    Label failAlloc;
    Label success;

    push(slots);
    freeListAllocate(result, slots, allocKind, &failAlloc);
    pop(slots);
    jump(&success);

    bind(&failAlloc);
    pop(slots);
    callFreeStub(slots);
    jump(fail);

    bind(&success);
}

void
MacroAssembler::newGCThing(Register result, Register temp, NativeObject *templateObj,
                            gc::InitialHeap initialHeap, Label *fail)
{
    
    
    
    MOZ_ASSERT(!templateObj->numDynamicSlots());

    gc::AllocKind allocKind = templateObj->asTenured().getAllocKind();
    MOZ_ASSERT(allocKind >= gc::FINALIZE_OBJECT0 && allocKind <= gc::FINALIZE_OBJECT_LAST);

    allocateObject(result, temp, allocKind, templateObj->numDynamicSlots(), initialHeap, fail);
}

void
MacroAssembler::createGCObject(Register obj, Register temp, JSObject *templateObj,
                               gc::InitialHeap initialHeap, Label *fail, bool initFixedSlots)
{
    gc::AllocKind allocKind = templateObj->asTenured().getAllocKind();
    MOZ_ASSERT(allocKind >= gc::FINALIZE_OBJECT0 && allocKind <= gc::FINALIZE_OBJECT_LAST);

    uint32_t nDynamicSlots = 0;
    if (templateObj->isNative()) {
        nDynamicSlots = templateObj->as<NativeObject>().numDynamicSlots();

        
        
        
        if (templateObj->as<NativeObject>().denseElementsAreCopyOnWrite())
            allocKind = gc::FINALIZE_OBJECT0_BACKGROUND;
    }

    allocateObject(obj, temp, allocKind, nDynamicSlots, initialHeap, fail);
    initGCThing(obj, temp, templateObj, initFixedSlots);
}





void
MacroAssembler::allocateNonObject(Register result, Register temp, gc::AllocKind allocKind, Label *fail)
{
    checkAllocatorState(fail);
    freeListAllocate(result, temp, allocKind, fail);
}

void
MacroAssembler::newGCString(Register result, Register temp, Label *fail)
{
    allocateNonObject(result, temp, js::gc::FINALIZE_STRING, fail);
}

void
MacroAssembler::newGCFatInlineString(Register result, Register temp, Label *fail)
{
    allocateNonObject(result, temp, js::gc::FINALIZE_FAT_INLINE_STRING, fail);
}

void
MacroAssembler::newGCThingPar(Register result, Register cx, Register tempReg1, Register tempReg2,
                              gc::AllocKind allocKind, Label *fail)
{
#ifdef JSGC_FJGENERATIONAL
    if (IsNurseryAllocable(allocKind))
        return newGCNurseryThingPar(result, cx, tempReg1, tempReg2, allocKind, fail);
#endif
    return newGCTenuredThingPar(result, cx, tempReg1, tempReg2, allocKind, fail);
}

#ifdef JSGC_FJGENERATIONAL
void
MacroAssembler::newGCNurseryThingPar(Register result, Register cx,
                                     Register tempReg1, Register tempReg2,
                                     gc::AllocKind allocKind, Label *fail)
{
    MOZ_ASSERT(IsNurseryAllocable(allocKind));

    uint32_t thingSize = uint32_t(gc::Arena::thingSize(allocKind));

    
    
    
    

    
    size_t offsetOfPosition =
        ForkJoinContext::offsetOfFJNursery() + gc::ForkJoinNursery::offsetOfPosition();
    size_t offsetOfEnd =
        ForkJoinContext::offsetOfFJNursery() + gc::ForkJoinNursery::offsetOfCurrentEnd();
    loadPtr(Address(cx, offsetOfPosition), result);
    loadPtr(Address(cx, offsetOfEnd), tempReg2);
    computeEffectiveAddress(Address(result, thingSize), tempReg1);
    branchPtr(Assembler::Below, tempReg2, tempReg1, fail);
    storePtr(tempReg1, Address(cx, offsetOfPosition));
}
#endif

void
MacroAssembler::newGCTenuredThingPar(Register result, Register cx,
                                     Register tempReg1, Register tempReg2,
                                     gc::AllocKind allocKind, Label *fail)
{
    
    
    
    
    
    
    
    
    
    

    uint32_t thingSize = (uint32_t)gc::Arena::thingSize(allocKind);

    
    
    loadPtr(Address(cx, ThreadSafeContext::offsetOfAllocator()),
            tempReg1);

    
    
    uint32_t offset = (offsetof(Allocator, arenas) +
                       js::gc::ArenaLists::getFreeListOffset(allocKind));
    addPtr(Imm32(offset), tempReg1);

    
    
    loadPtr(Address(tempReg1, gc::FreeList::offsetOfFirst()), tempReg2);

    
    
    branchPtr(Assembler::BelowOrEqual,
              Address(tempReg1, gc::FreeList::offsetOfLast()),
              tempReg2,
              fail);

    
    
    
    movePtr(tempReg2, result);
    addPtr(Imm32(thingSize), tempReg2);

    
    
    storePtr(tempReg2, Address(tempReg1, gc::FreeList::offsetOfFirst()));
}

void
MacroAssembler::newGCThingPar(Register result, Register cx, Register tempReg1, Register tempReg2,
                              NativeObject *templateObject, Label *fail)
{
    gc::AllocKind allocKind = templateObject->asTenured().getAllocKind();
    MOZ_ASSERT(allocKind >= gc::FINALIZE_OBJECT0 && allocKind <= gc::FINALIZE_OBJECT_LAST);
    MOZ_ASSERT(!templateObject->numDynamicSlots());

    newGCThingPar(result, cx, tempReg1, tempReg2, allocKind, fail);
}

void
MacroAssembler::newGCStringPar(Register result, Register cx, Register tempReg1, Register tempReg2,
                               Label *fail)
{
    newGCTenuredThingPar(result, cx, tempReg1, tempReg2, js::gc::FINALIZE_STRING, fail);
}

void
MacroAssembler::newGCFatInlineStringPar(Register result, Register cx, Register tempReg1,
                                        Register tempReg2, Label *fail)
{
    newGCTenuredThingPar(result, cx, tempReg1, tempReg2, js::gc::FINALIZE_FAT_INLINE_STRING, fail);
}

void
MacroAssembler::copySlotsFromTemplate(Register obj, const NativeObject *templateObj,
                                      uint32_t start, uint32_t end)
{
    uint32_t nfixed = Min(templateObj->numFixedSlots(), end);
    for (unsigned i = start; i < nfixed; i++)
        storeValue(templateObj->getFixedSlot(i), Address(obj, NativeObject::getFixedSlotOffset(i)));
}

void
MacroAssembler::fillSlotsWithConstantValue(Address base, Register temp,
                                           uint32_t start, uint32_t end, const Value &v)
{
    MOZ_ASSERT(v.isUndefined() || IsUninitializedLexical(v));

    if (start >= end)
        return;

#ifdef JS_NUNBOX32
    
    
    jsval_layout jv = JSVAL_TO_IMPL(v);

    Address addr = base;
    move32(Imm32(jv.s.payload.i32), temp);
    for (unsigned i = start; i < end; ++i, addr.offset += sizeof(HeapValue))
        store32(temp, ToPayload(addr));

    addr = base;
    move32(Imm32(jv.s.tag), temp);
    for (unsigned i = start; i < end; ++i, addr.offset += sizeof(HeapValue))
        store32(temp, ToType(addr));
#else
    moveValue(v, temp);
    for (uint32_t i = start; i < end; ++i, base.offset += sizeof(HeapValue))
        storePtr(temp, base);
#endif
}

void
MacroAssembler::fillSlotsWithUndefined(Address base, Register temp, uint32_t start, uint32_t end)
{
    fillSlotsWithConstantValue(base, temp, start, end, UndefinedValue());
}

void
MacroAssembler::fillSlotsWithUninitialized(Address base, Register temp, uint32_t start, uint32_t end)
{
    fillSlotsWithConstantValue(base, temp, start, end, MagicValue(JS_UNINITIALIZED_LEXICAL));
}

static void
FindStartOfUndefinedAndUninitializedSlots(NativeObject *templateObj, uint32_t nslots,
                                          uint32_t *startOfUndefined, uint32_t *startOfUninitialized)
{
    MOZ_ASSERT(nslots == templateObj->lastProperty()->slotSpan(templateObj->getClass()));
    MOZ_ASSERT(nslots > 0);
    uint32_t first = nslots;
    for (; first != 0; --first) {
        if (!IsUninitializedLexical(templateObj->getSlot(first - 1)))
            break;
    }
    *startOfUninitialized = first;
    for (; first != 0; --first) {
        if (templateObj->getSlot(first - 1) != UndefinedValue()) {
            *startOfUndefined = first;
            return;
        }
    }
    *startOfUndefined = 0;
}

void
MacroAssembler::initGCSlots(Register obj, Register slots, NativeObject *templateObj,
                            bool initFixedSlots)
{
    
    
    uint32_t nslots = templateObj->lastProperty()->slotSpan(templateObj->getClass());
    if (nslots == 0)
        return;

    uint32_t nfixed = templateObj->numUsedFixedSlots();
    uint32_t ndynamic = templateObj->numDynamicSlots();

    
    
    
    
    
    
    
    
    
    
    
    
    uint32_t startOfUndefined = nslots;
    uint32_t startOfUninitialized = nslots;
    FindStartOfUndefinedAndUninitializedSlots(templateObj, nslots,
                                              &startOfUndefined, &startOfUninitialized);
    MOZ_ASSERT(startOfUndefined <= nfixed); 
    MOZ_ASSERT_IF(startOfUndefined != nfixed, startOfUndefined <= startOfUninitialized);
    MOZ_ASSERT_IF(!templateObj->is<CallObject>(), startOfUninitialized == nslots);

    
    copySlotsFromTemplate(obj, templateObj, 0, startOfUndefined);

    
    if (initFixedSlots) {
        fillSlotsWithUndefined(Address(obj, NativeObject::getFixedSlotOffset(startOfUndefined)), slots,
                               startOfUndefined, Min(startOfUninitialized, nfixed));
        size_t offset = NativeObject::getFixedSlotOffset(startOfUninitialized);
        fillSlotsWithUninitialized(Address(obj, offset), slots, startOfUninitialized, nfixed);
    }

    if (ndynamic) {
        
        
        push(obj);
        loadPtr(Address(obj, NativeObject::offsetOfSlots()), obj);

        
        fillSlotsWithUndefined(Address(obj, 0), slots, 0, ndynamic);

        
        fillSlotsWithUninitialized(Address(obj, 0), slots, startOfUninitialized - nfixed,
                                   nslots - startOfUninitialized);

        pop(obj);
    }
}

void
MacroAssembler::initGCThing(Register obj, Register slots, JSObject *templateObj,
                            bool initFixedSlots)
{
    

    storePtr(ImmGCPtr(templateObj->lastProperty()), Address(obj, JSObject::offsetOfShape()));
    storePtr(ImmGCPtr(templateObj->type()), Address(obj, JSObject::offsetOfType()));

    if (templateObj->isNative()) {
        NativeObject *ntemplate = &templateObj->as<NativeObject>();
        MOZ_ASSERT_IF(!ntemplate->denseElementsAreCopyOnWrite(), !ntemplate->hasDynamicElements());

        if (ntemplate->hasDynamicSlots())
            storePtr(slots, Address(obj, NativeObject::offsetOfSlots()));
        else
            storePtr(ImmPtr(nullptr), Address(obj, NativeObject::offsetOfSlots()));

        if (ntemplate->denseElementsAreCopyOnWrite()) {
            storePtr(ImmPtr((const Value *) ntemplate->getDenseElements()),
                     Address(obj, NativeObject::offsetOfElements()));
        } else if (ntemplate->is<ArrayObject>()) {
            Register temp = slots;
            MOZ_ASSERT(!ntemplate->getDenseInitializedLength());

            int elementsOffset = NativeObject::offsetOfFixedElements();

            computeEffectiveAddress(Address(obj, elementsOffset), temp);
            storePtr(temp, Address(obj, NativeObject::offsetOfElements()));

            
            store32(Imm32(ntemplate->getDenseCapacity()),
                    Address(obj, elementsOffset + ObjectElements::offsetOfCapacity()));
            store32(Imm32(ntemplate->getDenseInitializedLength()),
                    Address(obj, elementsOffset + ObjectElements::offsetOfInitializedLength()));
            store32(Imm32(ntemplate->as<ArrayObject>().length()),
                    Address(obj, elementsOffset + ObjectElements::offsetOfLength()));
            store32(Imm32(ntemplate->shouldConvertDoubleElements()
                          ? ObjectElements::CONVERT_DOUBLE_ELEMENTS
                          : 0),
                    Address(obj, elementsOffset + ObjectElements::offsetOfFlags()));
            MOZ_ASSERT(!ntemplate->hasPrivate());
        } else {
            storePtr(ImmPtr(emptyObjectElements), Address(obj, NativeObject::offsetOfElements()));

            initGCSlots(obj, slots, ntemplate, initFixedSlots);

            if (ntemplate->hasPrivate()) {
                uint32_t nfixed = ntemplate->numFixedSlots();
                storePtr(ImmPtr(ntemplate->getPrivate()),
                         Address(obj, NativeObject::getPrivateDataOffset(nfixed)));
            }
        }
    } else if (templateObj->is<InlineTypedObject>()) {
        InlineTypedObject *ntemplate = &templateObj->as<InlineTypedObject>();

        
        size_t nbytes = ntemplate->size();
        size_t offset = 0;
        while (nbytes) {
            uintptr_t value = *(uintptr_t *)(ntemplate->inlineTypedMem() + offset);
            storePtr(ImmWord(value),
                     Address(obj, InlineTypedObject::offsetOfDataStart() + offset));
            nbytes = (nbytes < sizeof(uintptr_t)) ? 0 : nbytes - sizeof(uintptr_t);
            offset += sizeof(uintptr_t);
        }
    } else {
        MOZ_CRASH("Unknown object");
    }

#ifdef JS_GC_TRACE
    RegisterSet regs = RegisterSet::Volatile();
    PushRegsInMask(regs);
    regs.takeUnchecked(obj);
    Register temp = regs.takeGeneral();

    setupUnalignedABICall(2, temp);
    passABIArg(obj);
    movePtr(ImmGCPtr(templateObj->type()), temp);
    passABIArg(temp);
    callWithABI(JS_FUNC_TO_DATA_PTR(void *, js::gc::TraceCreateObject));

    PopRegsInMask(RegisterSet::Volatile());
#endif
}

void
MacroAssembler::compareStrings(JSOp op, Register left, Register right, Register result,
                               Label *fail)
{
    MOZ_ASSERT(IsEqualityOp(op));

    Label done;
    Label notPointerEqual;
    
    branchPtr(Assembler::NotEqual, left, right, &notPointerEqual);
    move32(Imm32(op == JSOP_EQ || op == JSOP_STRICTEQ), result);
    jump(&done);

    bind(&notPointerEqual);

    Label notAtom;
    
    Imm32 atomBit(JSString::ATOM_BIT);
    branchTest32(Assembler::Zero, Address(left, JSString::offsetOfFlags()), atomBit, &notAtom);
    branchTest32(Assembler::Zero, Address(right, JSString::offsetOfFlags()), atomBit, &notAtom);

    cmpPtrSet(JSOpToCondition(MCompare::Compare_String, op), left, right, result);
    jump(&done);

    bind(&notAtom);
    
    loadStringLength(left, result);
    branch32(Assembler::Equal, Address(right, JSString::offsetOfLength()), result, fail);
    move32(Imm32(op == JSOP_NE || op == JSOP_STRICTNE), result);

    bind(&done);
}

void
MacroAssembler::loadStringChars(Register str, Register dest)
{
    Label isInline, done;
    branchTest32(Assembler::NonZero, Address(str, JSString::offsetOfFlags()),
                 Imm32(JSString::INLINE_CHARS_BIT), &isInline);

    loadPtr(Address(str, JSString::offsetOfNonInlineChars()), dest);
    jump(&done);

    bind(&isInline);
    computeEffectiveAddress(Address(str, JSInlineString::offsetOfInlineStorage()), dest);

    bind(&done);
}

void
MacroAssembler::loadStringChar(Register str, Register index, Register output)
{
    MOZ_ASSERT(str != output);
    MOZ_ASSERT(index != output);

    loadStringChars(str, output);

    Label isLatin1, done;
    branchTest32(Assembler::NonZero, Address(str, JSString::offsetOfFlags()),
                 Imm32(JSString::LATIN1_CHARS_BIT), &isLatin1);
    load16ZeroExtend(BaseIndex(output, index, TimesTwo), output);
    jump(&done);

    bind(&isLatin1);
    load8ZeroExtend(BaseIndex(output, index, TimesOne), output);

    bind(&done);
}

void
MacroAssembler::checkInterruptFlagPar(Register tempReg, Label *fail)
{
    movePtr(ImmPtr(GetJitContext()->runtime->addressOfInterruptParUint32()), tempReg);
    branch32(Assembler::NonZero, Address(tempReg, 0), Imm32(0), fail);
}



void
MacroAssembler::linkExitFrame()
{
    AbsoluteAddress jitTop(GetJitContext()->runtime->addressOfJitTop());
    storePtr(StackPointer, jitTop);
}



void
MacroAssembler::linkParallelExitFrame(Register pt)
{
    Address jitTop(pt, offsetof(PerThreadData, jitTop));
    storePtr(StackPointer, jitTop);
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
        MOZ_ASSERT(!regs.has(BaselineStackReg));
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
        
        enterFakeExitFrame(ExitFrameLayout::BareToken());

        
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

            
            addPtr(Imm32(ExitFrameLayout::SizeWithFooter()), StackPointer);

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

            
            addPtr(Imm32(ExitFrameLayout::SizeWithFooter()), StackPointer);

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
        MOZ_CRASH("No such execution mode");
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
        MOZ_CRASH("No such execution mode");
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
        MOZ_CRASH("No such execution mode");
    }
}

void
MacroAssembler::handleFailure(ExecutionMode executionMode)
{
    
    
    if (sps_)
        sps_->skipNextReenter();
    leaveSPSFrame();

    JitCode *excTail;
    switch (executionMode) {
      case SequentialExecution:
        excTail = GetJitContext()->runtime->jitRuntime()->getExceptionTail();
        break;
      case ParallelExecution:
        excTail = GetJitContext()->runtime->jitRuntime()->getExceptionTailParallel();
        break;
      default:
        MOZ_CRASH("No such execution mode");
    }
    jump(excTail);

    
    if (sps_)
        sps_->reenter(*this, InvalidReg);
}

#ifdef DEBUG
static void
AssumeUnreachable_(const char *output) {
    MOZ_ReportAssertionFailure(output, __FILE__, __LINE__);
}
#endif

void
MacroAssembler::assumeUnreachable(const char *output)
{
#ifdef DEBUG
    if (!IsCompilingAsmJS()) {
        RegisterSet regs = RegisterSet::Volatile();
        PushRegsInMask(regs);
        Register temp = regs.takeGeneral();

        setupUnalignedABICall(1, temp);
        movePtr(ImmPtr(output), temp);
        passABIArg(temp);
        callWithABINoProfiling(JS_FUNC_TO_DATA_PTR(void *, AssumeUnreachable_));

        PopRegsInMask(RegisterSet::Volatile());
    }
#endif

    breakpoint();
}

static void
Printf0_(const char *output) {
    
    
    
    fprintf(stderr, "%s", output);
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
    fprintf(stderr, "%s", line);
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

#ifdef JS_TRACE_LOGGING
void
MacroAssembler::tracelogStart(Register logger, uint32_t textId)
{
    if (!TraceLogTextIdEnabled(textId))
        return;

    void (&TraceLogFunc)(TraceLogger*, uint32_t) = TraceLogStartEvent;

    PushRegsInMask(RegisterSet::Volatile());

    RegisterSet regs = RegisterSet::Volatile();
    regs.takeUnchecked(logger);

    Register temp = regs.takeGeneral();

    setupUnalignedABICall(2, temp);
    passABIArg(logger);
    move32(Imm32(textId), temp);
    passABIArg(temp);
    callWithABINoProfiling(JS_FUNC_TO_DATA_PTR(void *, TraceLogFunc));

    PopRegsInMask(RegisterSet::Volatile());
}

void
MacroAssembler::tracelogStart(Register logger, Register textId)
{
    void (&TraceLogFunc)(TraceLogger*, uint32_t) = TraceLogStartEvent;

    PushRegsInMask(RegisterSet::Volatile());

    RegisterSet regs = RegisterSet::Volatile();
    regs.takeUnchecked(logger);
    regs.takeUnchecked(textId);

    Register temp = regs.takeGeneral();

    setupUnalignedABICall(2, temp);
    passABIArg(logger);
    passABIArg(textId);
    callWithABINoProfiling(JS_FUNC_TO_DATA_PTR(void *, TraceLogFunc));

    PopRegsInMask(RegisterSet::Volatile());
}

void
MacroAssembler::tracelogStop(Register logger, uint32_t textId)
{
    if (!TraceLogTextIdEnabled(textId))
        return;

    PushRegsInMask(RegisterSet::Volatile());

    RegisterSet regs = RegisterSet::Volatile();
    regs.takeUnchecked(logger);

    Register temp = regs.takeGeneral();

    setupUnalignedABICall(2, temp);
    passABIArg(logger);
    move32(Imm32(textId), temp);
    passABIArg(temp);

    callWithABINoProfiling(JS_FUNC_TO_DATA_PTR(void *, TraceLogEvent));

    PopRegsInMask(RegisterSet::Volatile());
}

void
MacroAssembler::tracelogStop(Register logger, Register textId)
{
    PushRegsInMask(RegisterSet::Volatile());
    RegisterSet regs = RegisterSet::Volatile();
    regs.takeUnchecked(logger);

    regs.takeUnchecked(textId);

    Register temp = regs.takeGeneral();

    setupUnalignedABICall(2, temp);
    passABIArg(logger);
    passABIArg(textId);
    callWithABINoProfiling(JS_FUNC_TO_DATA_PTR(void *, TraceLogEvent));

    PopRegsInMask(RegisterSet::Volatile());
}
#endif

void
MacroAssembler::convertInt32ValueToDouble(const Address &address, Register scratch, Label *done)
{
    branchTestInt32(Assembler::NotEqual, address, done);
    unboxInt32(address, scratch);
    convertInt32ToDouble(scratch, ScratchDoubleReg);
    storeDouble(ScratchDoubleReg, address);
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
    FloatRegister tmp = output;
    if (outputType == MIRType_Float32 && hasMultiAlias())
        tmp = ScratchDoubleReg;

    unboxDouble(value, tmp);
    if (outputType == MIRType_Float32)
        convertDoubleToFloat32(tmp, output);

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

    MOZ_ASSERT(v.isObject());
    jump(fail);
    return true;
}

void
MacroAssembler::PushEmptyRooted(VMFunction::RootType rootType)
{
    switch (rootType) {
      case VMFunction::RootNone:
        MOZ_CRASH("Handle must have root type");
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
        MOZ_CRASH("Handle must have root type");
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
    MOZ_ASSERT(IsFloatingPointType(outputType));

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
      case MIRType_Symbol:
        jump(fail);
        break;
      case MIRType_Undefined:
        loadConstantFloatingPoint(GenericNaN(), float(GenericNaN()), output, outputType);
        break;
      default:
        MOZ_CRASH("Bad MIRType");
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

    MOZ_ASSERT_IF(handleStrings, conversion == IntConversion_Any);

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

    MOZ_ASSERT(v.isObject());

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
      case MIRType_Symbol:
      case MIRType_Object:
        jump(fail);
        break;
      default:
        MOZ_CRASH("Bad MIRType");
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
    
    
    MOZ_ASSERT(JSFunction::offsetOfNargs() % sizeof(uint32_t) == 0);
    MOZ_ASSERT(JSFunction::offsetOfFlags() == JSFunction::offsetOfNargs() + 2);

    
    
    
    
    
    

    
    load32(Address(fun, JSFunction::offsetOfNargs()), scratch);
    int32_t bits = IMM32_16ADJ(JSFunction::INTERPRETED);
    branchTest32(Assembler::Zero, scratch, Imm32(bits), label);

    
    
    Label done;
    bits = IMM32_16ADJ( (JSFunction::IS_FUN_PROTO | JSFunction::ARROW | JSFunction::SELF_HOSTED) );
    branchTest32(Assembler::Zero, scratch, Imm32(bits), &done);
    {
        
        
        
        bits = IMM32_16ADJ(JSFunction::SELF_HOSTED_CTOR);
        branchTest32(Assembler::Zero, scratch, Imm32(bits), label);

#ifdef DEBUG
        bits = IMM32_16ADJ(JSFunction::IS_FUN_PROTO);
        branchTest32(Assembler::Zero, scratch, Imm32(bits), &done);
        assumeUnreachable("Function.prototype should not have the SELF_HOSTED_CTOR flag");
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
          case MIRType_Symbol:
            branchTestSymbol(Equal, tag, label);
            break;
          case MIRType_Object:
            branchTestObject(Equal, tag, label);
            break;
          default:
            MOZ_CRASH("Unsupported type");
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

    
    storePtr(ImmPtr(enterJitLabel), Address(temp, ProfileEntry::offsetOfLabel()));
    storePtr(framePtr,              Address(temp, ProfileEntry::offsetOfSpOrScript()));
    store32(Imm32(0),               Address(temp, ProfileEntry::offsetOfLineOrPc()));
    store32(Imm32(ProfileEntry::IS_CPP_ENTRY), Address(temp, ProfileEntry::offsetOfFlags()));

    
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
