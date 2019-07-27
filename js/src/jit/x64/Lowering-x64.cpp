





#include "jit/x64/Lowering-x64.h"

#include "jit/MIR.h"
#include "jit/x64/Assembler-x64.h"

#include "jit/shared/Lowering-shared-inl.h"

using namespace js;
using namespace js::jit;

bool
LIRGeneratorX64::useBox(LInstruction *lir, size_t n, MDefinition *mir,
                        LUse::Policy policy, bool useAtStart)
{
    MOZ_ASSERT(mir->type() == MIRType_Value);

    if (!ensureDefined(mir))
        return false;
    lir->setOperand(n, LUse(mir->virtualRegister(), policy, useAtStart));
    return true;
}

bool
LIRGeneratorX64::useBoxFixed(LInstruction *lir, size_t n, MDefinition *mir, Register reg1, Register)
{
    MOZ_ASSERT(mir->type() == MIRType_Value);

    if (!ensureDefined(mir))
        return false;
    lir->setOperand(n, LUse(reg1, mir->virtualRegister()));
    return true;
}

LAllocation
LIRGeneratorX64::useByteOpRegister(MDefinition *mir)
{
    return useRegister(mir);
}

LAllocation
LIRGeneratorX64::useByteOpRegisterOrNonDoubleConstant(MDefinition *mir)
{
    return useRegisterOrNonDoubleConstant(mir);
}

LDefinition
LIRGeneratorX64::tempByteOpRegister()
{
    return temp();
}

LDefinition
LIRGeneratorX64::tempToUnbox()
{
    return temp();
}

bool
LIRGeneratorX64::visitBox(MBox *box)
{
    MDefinition *opd = box->getOperand(0);

    
    if (opd->isConstant() && box->canEmitAtUses())
        return emitAtUses(box);

    if (opd->isConstant())
        return define(new(alloc()) LValue(opd->toConstant()->value()), box, LDefinition(LDefinition::BOX));

    LBox *ins = new(alloc()) LBox(opd->type(), useRegister(opd));
    return define(ins, box, LDefinition(LDefinition::BOX));
}

bool
LIRGeneratorX64::visitUnbox(MUnbox *unbox)
{
    MDefinition *box = unbox->getOperand(0);
    MOZ_ASSERT(box->type() == MIRType_Value);

    LUnboxBase *lir;
    if (IsFloatingPointType(unbox->type())) {
        lir = new(alloc()) LUnboxFloatingPoint(useRegisterAtStart(box), unbox->type());
    } else if (unbox->fallible()) {
        
        
        lir = new(alloc()) LUnbox(useRegisterAtStart(box));
    } else {
        lir = new(alloc()) LUnbox(useAtStart(box));
    }

    if (unbox->fallible() && !assignSnapshot(lir, unbox->bailoutKind()))
        return false;

    return define(lir, unbox);
}

bool
LIRGeneratorX64::visitReturn(MReturn *ret)
{
    MDefinition *opd = ret->getOperand(0);
    MOZ_ASSERT(opd->type() == MIRType_Value);

    LReturn *ins = new(alloc()) LReturn;
    ins->setOperand(0, useFixed(opd, JSReturnReg));
    return add(ins);
}

bool
LIRGeneratorX64::defineUntypedPhi(MPhi *phi, size_t lirIndex)
{
    return defineTypedPhi(phi, lirIndex);
}

void
LIRGeneratorX64::lowerUntypedPhiInput(MPhi *phi, uint32_t inputPosition, LBlock *block, size_t lirIndex)
{
    lowerTypedPhiInput(phi, inputPosition, block, lirIndex);
}

bool
LIRGeneratorX64::visitAsmJSUnsignedToDouble(MAsmJSUnsignedToDouble *ins)
{
    MOZ_ASSERT(ins->input()->type() == MIRType_Int32);
    LAsmJSUInt32ToDouble *lir = new(alloc()) LAsmJSUInt32ToDouble(useRegisterAtStart(ins->input()));
    return define(lir, ins);
}

bool
LIRGeneratorX64::visitAsmJSUnsignedToFloat32(MAsmJSUnsignedToFloat32 *ins)
{
    MOZ_ASSERT(ins->input()->type() == MIRType_Int32);
    LAsmJSUInt32ToFloat32 *lir = new(alloc()) LAsmJSUInt32ToFloat32(useRegisterAtStart(ins->input()));
    return define(lir, ins);
}

bool
LIRGeneratorX64::visitAsmJSLoadHeap(MAsmJSLoadHeap *ins)
{
    MDefinition *ptr = ins->ptr();
    MOZ_ASSERT(ptr->type() == MIRType_Int32);

    
    
    
    
    
    LAllocation ptrAlloc = ins->needsBoundsCheck()
                           ? useRegisterAtStart(ptr)
                           : useRegisterOrNonNegativeConstantAtStart(ptr);

    return define(new(alloc()) LAsmJSLoadHeap(ptrAlloc), ins);
}

bool
LIRGeneratorX64::visitAsmJSStoreHeap(MAsmJSStoreHeap *ins)
{
    MDefinition *ptr = ins->ptr();
    MOZ_ASSERT(ptr->type() == MIRType_Int32);

    
    
    
    
    
    LAllocation ptrAlloc = ins->needsBoundsCheck()
                           ? useRegisterAtStart(ptr)
                           : useRegisterOrNonNegativeConstantAtStart(ptr);

    LAsmJSStoreHeap *lir;
    switch (ins->viewType()) {
      case Scalar::Int8:
      case Scalar::Uint8:
      case Scalar::Int16:
      case Scalar::Uint16:
      case Scalar::Int32:
      case Scalar::Uint32:
        lir = new(alloc()) LAsmJSStoreHeap(ptrAlloc, useRegisterOrConstantAtStart(ins->value()));
        break;
      case Scalar::Float32:
      case Scalar::Float64:
      case Scalar::Float32x4:
      case Scalar::Int32x4:
        lir = new(alloc()) LAsmJSStoreHeap(ptrAlloc, useRegisterAtStart(ins->value()));
        break;
      case Scalar::Uint8Clamped:
      case Scalar::MaxTypedArrayViewType:
        MOZ_CRASH("unexpected array type");
    }

    return add(lir, ins);
}

bool
LIRGeneratorX64::visitAsmJSLoadFuncPtr(MAsmJSLoadFuncPtr *ins)
{
    return define(new(alloc()) LAsmJSLoadFuncPtr(useRegister(ins->index()), temp()), ins);
}

bool
LIRGeneratorX64::visitSubstr(MSubstr *ins)
{
    LSubstr *lir = new (alloc()) LSubstr(useRegister(ins->string()),
                                         useRegister(ins->begin()),
                                         useRegister(ins->length()),
                                         temp(),
                                         temp(),
                                         tempByteOpRegister());
    return define(lir, ins) && assignSafepoint(lir, ins);
}

bool
LIRGeneratorX64::visitStoreTypedArrayElementStatic(MStoreTypedArrayElementStatic *ins)
{
    MOZ_CRASH("NYI");
}
