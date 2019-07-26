





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
    JS_ASSERT(mir->type() == MIRType_Value);

    if (!ensureDefined(mir))
        return false;
    lir->setOperand(n, LUse(mir->virtualRegister(), policy, useAtStart));
    return true;
}

bool
LIRGeneratorX64::useBoxFixed(LInstruction *lir, size_t n, MDefinition *mir, Register reg1, Register)
{
    JS_ASSERT(mir->type() == MIRType_Value);

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
        return define(new LValue(opd->toConstant()->value()), box, LDefinition(LDefinition::BOX));

    LBox *ins = new LBox(opd->type(), useRegister(opd));
    return define(ins, box, LDefinition(LDefinition::BOX));
}

bool
LIRGeneratorX64::visitUnbox(MUnbox *unbox)
{
    MDefinition *box = unbox->getOperand(0);
    LUnboxBase *lir;
    if (unbox->type() == MIRType_Double)
        lir = new LUnboxDouble(useRegister(box));
    else
        lir = new LUnbox(useRegister(box));

    if (unbox->fallible() && !assignSnapshot(lir, unbox->bailoutKind()))
        return false;

    return define(lir, unbox);
}

bool
LIRGeneratorX64::visitReturn(MReturn *ret)
{
    MDefinition *opd = ret->getOperand(0);
    JS_ASSERT(opd->type() == MIRType_Value);

    LReturn *ins = new LReturn;
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
    JS_ASSERT(ins->input()->type() == MIRType_Int32);
    LUInt32ToDouble *lir = new LUInt32ToDouble(useRegisterAtStart(ins->input()));
    return define(lir, ins);
}

bool
LIRGeneratorX64::visitAsmJSLoadHeap(MAsmJSLoadHeap *ins)
{
    MDefinition *ptr = ins->ptr();
    JS_ASSERT(ptr->type() == MIRType_Int32);

    
    
    
    
    if (ptr->isConstant() && ptr->toConstant()->value().toInt32() >= 0) {
        LAsmJSLoadHeap *lir = new LAsmJSLoadHeap(LAllocation(ptr->toConstant()->vp()));
        return define(lir, ins);
    }
    return define(new LAsmJSLoadHeap(useRegisterAtStart(ptr)), ins);
}

bool
LIRGeneratorX64::visitAsmJSStoreHeap(MAsmJSStoreHeap *ins)
{
    MDefinition *ptr = ins->ptr();
    JS_ASSERT(ptr->type() == MIRType_Int32);
    LAsmJSStoreHeap *lir;

    
    
    
    LAllocation ptrAlloc = useRegisterOrNonNegativeConstantAtStart(ptr);
    switch (ins->viewType()) {
      case ArrayBufferView::TYPE_INT8: case ArrayBufferView::TYPE_UINT8:
      case ArrayBufferView::TYPE_INT16: case ArrayBufferView::TYPE_UINT16:
      case ArrayBufferView::TYPE_INT32: case ArrayBufferView::TYPE_UINT32:
        lir = new LAsmJSStoreHeap(ptrAlloc, useRegisterOrConstantAtStart(ins->value()));
        break;
      case ArrayBufferView::TYPE_FLOAT32: case ArrayBufferView::TYPE_FLOAT64:
        lir = new LAsmJSStoreHeap(ptrAlloc, useRegisterAtStart(ins->value()));
        break;
      default: MOZ_ASSUME_UNREACHABLE("unexpected array type");
    }

    return add(lir, ins);
}

bool
LIRGeneratorX64::visitAsmJSLoadFuncPtr(MAsmJSLoadFuncPtr *ins)
{
    return define(new LAsmJSLoadFuncPtr(useRegister(ins->index()), temp()), ins);
}

LGetPropertyCacheT *
LIRGeneratorX64::newLGetPropertyCacheT(MGetPropertyCache *ins)
{
    return new LGetPropertyCacheT(useRegister(ins->object()), LDefinition::BogusTemp());
}

LGetElementCacheT *
LIRGeneratorX64::newLGetElementCacheT(MGetElementCache *ins)
{
    return new LGetElementCacheT(useRegister(ins->object()),
                                 useRegister(ins->index()),
                                 LDefinition::BogusTemp());
}

bool
LIRGeneratorX64::visitStoreTypedArrayElementStatic(MStoreTypedArrayElementStatic *ins)
{
    MOZ_ASSUME_UNREACHABLE("NYI");
}
