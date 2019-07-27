





#include "jit/x86/Lowering-x86.h"

#include "jit/MIR.h"
#include "jit/x86/Assembler-x86.h"

#include "jit/shared/Lowering-shared-inl.h"

using namespace js;
using namespace js::jit;

LDefinition
LIRGeneratorX86::tempForDispatchCache(MIRType outputType)
{
    
    
    
    
    
    if (gen->info().executionMode() != ParallelExecution)
        return LDefinition::BogusTemp();

    
    if (outputType == MIRType_None)
        return temp();

    
    if (outputType == MIRType_Double)
        return temp();

    
    return LDefinition::BogusTemp();
}

bool
LIRGeneratorX86::useBox(LInstruction *lir, size_t n, MDefinition *mir,
                        LUse::Policy policy, bool useAtStart)
{
    JS_ASSERT(mir->type() == MIRType_Value);

    if (!ensureDefined(mir))
        return false;
    lir->setOperand(n, LUse(mir->virtualRegister(), policy, useAtStart));
    lir->setOperand(n + 1, LUse(VirtualRegisterOfPayload(mir), policy, useAtStart));
    return true;
}

bool
LIRGeneratorX86::useBoxFixed(LInstruction *lir, size_t n, MDefinition *mir, Register reg1,
                             Register reg2)
{
    JS_ASSERT(mir->type() == MIRType_Value);
    JS_ASSERT(reg1 != reg2);

    if (!ensureDefined(mir))
        return false;
    lir->setOperand(n, LUse(reg1, mir->virtualRegister()));
    lir->setOperand(n + 1, LUse(reg2, VirtualRegisterOfPayload(mir)));
    return true;
}

LAllocation
LIRGeneratorX86::useByteOpRegister(MDefinition *mir)
{
    return useFixed(mir, eax);
}

LAllocation
LIRGeneratorX86::useByteOpRegisterOrNonDoubleConstant(MDefinition *mir)
{
    return useFixed(mir, eax);
}

bool
LIRGeneratorX86::visitBox(MBox *box)
{
    MDefinition *inner = box->getOperand(0);

    
    if (IsFloatingPointType(inner->type()))
        return defineBox(new(alloc()) LBoxFloatingPoint(useRegisterAtStart(inner), tempCopy(inner, 0),
                                                        inner->type()), box);

    if (box->canEmitAtUses())
        return emitAtUses(box);

    if (inner->isConstant())
        return defineBox(new(alloc()) LValue(inner->toConstant()->value()), box);

    LBox *lir = new(alloc()) LBox(use(inner), inner->type());

    
    
    uint32_t vreg = getVirtualRegister();
    if (vreg >= MAX_VIRTUAL_REGISTERS)
        return false;

    
    
    
    
    
    
    lir->setDef(0, LDefinition(vreg, LDefinition::GENERAL));
    lir->setDef(1, LDefinition(inner->virtualRegister(), LDefinition::TypeFrom(inner->type()),
                               LDefinition::PASSTHROUGH));
    box->setVirtualRegister(vreg);
    return add(lir);
}

bool
LIRGeneratorX86::visitUnbox(MUnbox *unbox)
{
    
    
    
    MDefinition *inner = unbox->getOperand(0);

    if (!ensureDefined(inner))
        return false;

    if (IsFloatingPointType(unbox->type())) {
        LUnboxFloatingPoint *lir = new(alloc()) LUnboxFloatingPoint(unbox->type());
        if (unbox->fallible() && !assignSnapshot(lir, unbox->bailoutKind()))
            return false;
        if (!useBox(lir, LUnboxFloatingPoint::Input, inner))
            return false;
        return define(lir, unbox);
    }

    
    LUnbox *lir = new(alloc()) LUnbox;
    lir->setOperand(0, usePayloadInRegisterAtStart(inner));
    lir->setOperand(1, useType(inner, LUse::ANY));

    if (unbox->fallible() && !assignSnapshot(lir, unbox->bailoutKind()))
        return false;

    
    
    
    
    
    
    return defineReuseInput(lir, unbox, 0);
}

bool
LIRGeneratorX86::visitReturn(MReturn *ret)
{
    MDefinition *opd = ret->getOperand(0);
    JS_ASSERT(opd->type() == MIRType_Value);

    LReturn *ins = new(alloc()) LReturn;
    ins->setOperand(0, LUse(JSReturnReg_Type));
    ins->setOperand(1, LUse(JSReturnReg_Data));
    return fillBoxUses(ins, 0, opd) && add(ins);
}

bool
LIRGeneratorX86::defineUntypedPhi(MPhi *phi, size_t lirIndex)
{
    LPhi *type = current->getPhi(lirIndex + VREG_TYPE_OFFSET);
    LPhi *payload = current->getPhi(lirIndex + VREG_DATA_OFFSET);

    uint32_t typeVreg = getVirtualRegister();
    if (typeVreg >= MAX_VIRTUAL_REGISTERS)
        return false;

    phi->setVirtualRegister(typeVreg);

    uint32_t payloadVreg = getVirtualRegister();
    if (payloadVreg >= MAX_VIRTUAL_REGISTERS)
        return false;
    JS_ASSERT(typeVreg + 1 == payloadVreg);

    type->setDef(0, LDefinition(typeVreg, LDefinition::TYPE));
    payload->setDef(0, LDefinition(payloadVreg, LDefinition::PAYLOAD));
    annotate(type);
    annotate(payload);
    return true;
}

void
LIRGeneratorX86::lowerUntypedPhiInput(MPhi *phi, uint32_t inputPosition, LBlock *block, size_t lirIndex)
{
    MDefinition *operand = phi->getOperand(inputPosition);
    LPhi *type = block->getPhi(lirIndex + VREG_TYPE_OFFSET);
    LPhi *payload = block->getPhi(lirIndex + VREG_DATA_OFFSET);
    type->setOperand(inputPosition, LUse(operand->virtualRegister() + VREG_TYPE_OFFSET, LUse::ANY));
    payload->setOperand(inputPosition, LUse(VirtualRegisterOfPayload(operand), LUse::ANY));
}

bool
LIRGeneratorX86::visitAsmJSUnsignedToDouble(MAsmJSUnsignedToDouble *ins)
{
    JS_ASSERT(ins->input()->type() == MIRType_Int32);
    LAsmJSUInt32ToDouble *lir = new(alloc()) LAsmJSUInt32ToDouble(useRegisterAtStart(ins->input()), temp());
    return define(lir, ins);
}

bool
LIRGeneratorX86::visitAsmJSUnsignedToFloat32(MAsmJSUnsignedToFloat32 *ins)
{
    JS_ASSERT(ins->input()->type() == MIRType_Int32);
    LAsmJSUInt32ToFloat32 *lir = new(alloc()) LAsmJSUInt32ToFloat32(useRegisterAtStart(ins->input()), temp());
    return define(lir, ins);
}

bool
LIRGeneratorX86::visitAsmJSLoadHeap(MAsmJSLoadHeap *ins)
{
    MDefinition *ptr = ins->ptr();
    LAllocation ptrAlloc;
    JS_ASSERT(ptr->type() == MIRType_Int32);

    
    if (ptr->isConstant() && ins->skipBoundsCheck()) {
        int32_t ptrValue = ptr->toConstant()->value().toInt32();
        
        JS_ASSERT(ptrValue >= 0);
        ptrAlloc = LAllocation(ptr->toConstant()->vp());
    } else {
        ptrAlloc = useRegisterAtStart(ptr);
    }
    LAsmJSLoadHeap *lir = new(alloc()) LAsmJSLoadHeap(ptrAlloc);
    return define(lir, ins);
}

bool
LIRGeneratorX86::visitAsmJSStoreHeap(MAsmJSStoreHeap *ins)
{
    MDefinition *ptr = ins->ptr();
    LAsmJSStoreHeap *lir;
    JS_ASSERT(ptr->type() == MIRType_Int32);

    if (ptr->isConstant() && ins->skipBoundsCheck()) {
        int32_t ptrValue = ptr->toConstant()->value().toInt32();
        JS_ASSERT(ptrValue >= 0);
        LAllocation ptrAlloc = LAllocation(ptr->toConstant()->vp());
        switch (ins->viewType()) {
          case Scalar::Int8: case Scalar::Uint8:
            
            lir = new(alloc()) LAsmJSStoreHeap(ptrAlloc, useFixed(ins->value(), eax));
            break;
          case Scalar::Int16: case Scalar::Uint16:
          case Scalar::Int32: case Scalar::Uint32:
          case Scalar::Float32: case Scalar::Float64:
            
            lir = new(alloc()) LAsmJSStoreHeap(ptrAlloc, useRegisterAtStart(ins->value()));
            break;
          default: MOZ_ASSUME_UNREACHABLE("unexpected array type");
        }
        return add(lir, ins);
    }

    switch (ins->viewType()) {
      case Scalar::Int8: case Scalar::Uint8:
        
        lir = new(alloc()) LAsmJSStoreHeap(useRegister(ins->ptr()), useFixed(ins->value(), eax));
        break;
      case Scalar::Int16: case Scalar::Uint16:
      case Scalar::Int32: case Scalar::Uint32:
      case Scalar::Float32: case Scalar::Float64:
        
        
        lir = new(alloc()) LAsmJSStoreHeap(useRegisterAtStart(ptr), useRegisterAtStart(ins->value()));
        break;
      default: MOZ_ASSUME_UNREACHABLE("unexpected array type");
    }

    return add(lir, ins);
}

bool
LIRGeneratorX86::visitStoreTypedArrayElementStatic(MStoreTypedArrayElementStatic *ins)
{
    
    
    LStoreTypedArrayElementStatic *lir;
    switch (ins->viewType()) {
      case Scalar::Int8: case Scalar::Uint8:
      case Scalar::Uint8Clamped:
        lir = new(alloc()) LStoreTypedArrayElementStatic(useRegister(ins->ptr()),
                                                         useFixed(ins->value(), eax));
        break;
      case Scalar::Int16: case Scalar::Uint16:
      case Scalar::Int32: case Scalar::Uint32:
      case Scalar::Float32: case Scalar::Float64:
        lir = new(alloc()) LStoreTypedArrayElementStatic(useRegisterAtStart(ins->ptr()),
                                                         useRegisterAtStart(ins->value()));
        break;
      default: MOZ_ASSUME_UNREACHABLE("unexpected array type");
    }

    return add(lir, ins);
}

bool
LIRGeneratorX86::visitAsmJSLoadFuncPtr(MAsmJSLoadFuncPtr *ins)
{
    return define(new(alloc()) LAsmJSLoadFuncPtr(useRegisterAtStart(ins->index())), ins);
}
