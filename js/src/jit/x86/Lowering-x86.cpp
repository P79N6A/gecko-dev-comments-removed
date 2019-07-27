





#include "jit/x86/Lowering-x86.h"

#include "jit/MIR.h"
#include "jit/x86/Assembler-x86.h"

#include "jit/shared/Lowering-shared-inl.h"

using namespace js;
using namespace js::jit;

void
LIRGeneratorX86::useBox(LInstruction *lir, size_t n, MDefinition *mir,
                        LUse::Policy policy, bool useAtStart)
{
    MOZ_ASSERT(mir->type() == MIRType_Value);

    ensureDefined(mir);
    lir->setOperand(n, LUse(mir->virtualRegister(), policy, useAtStart));
    lir->setOperand(n + 1, LUse(VirtualRegisterOfPayload(mir), policy, useAtStart));
}

void
LIRGeneratorX86::useBoxFixed(LInstruction *lir, size_t n, MDefinition *mir, Register reg1,
                             Register reg2)
{
    MOZ_ASSERT(mir->type() == MIRType_Value);
    MOZ_ASSERT(reg1 != reg2);

    ensureDefined(mir);
    lir->setOperand(n, LUse(reg1, mir->virtualRegister()));
    lir->setOperand(n + 1, LUse(reg2, VirtualRegisterOfPayload(mir)));
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

LDefinition
LIRGeneratorX86::tempByteOpRegister()
{
    return tempFixed(eax);
}

void
LIRGeneratorX86::visitBox(MBox *box)
{
    MDefinition *inner = box->getOperand(0);

    
    if (IsFloatingPointType(inner->type())) {
        defineBox(new(alloc()) LBoxFloatingPoint(useRegisterAtStart(inner), tempCopy(inner, 0),
                                                 inner->type()), box);
        return;
    }

    if (box->canEmitAtUses()) {
        emitAtUses(box);
        return;
    }

    if (inner->isConstant()) {
        defineBox(new(alloc()) LValue(inner->toConstant()->value()), box);
        return;
    }

    LBox *lir = new(alloc()) LBox(use(inner), inner->type());

    
    
    uint32_t vreg = getVirtualRegister();

    
    
    
    
    
    
    lir->setDef(0, LDefinition(vreg, LDefinition::GENERAL));
    lir->setDef(1, LDefinition::BogusTemp());
    box->setVirtualRegister(vreg);
    add(lir);
}

void
LIRGeneratorX86::visitUnbox(MUnbox *unbox)
{
    MDefinition *inner = unbox->getOperand(0);

    if (inner->type() == MIRType_ObjectOrNull) {
        LUnboxObjectOrNull *lir = new(alloc()) LUnboxObjectOrNull(useRegisterAtStart(inner));
        if (unbox->fallible())
            assignSnapshot(lir, unbox->bailoutKind());
        defineReuseInput(lir, unbox, 0);
        return;
    }

    
    
    
    MOZ_ASSERT(inner->type() == MIRType_Value);

    ensureDefined(inner);

    if (IsFloatingPointType(unbox->type())) {
        LUnboxFloatingPoint *lir = new(alloc()) LUnboxFloatingPoint(unbox->type());
        if (unbox->fallible())
            assignSnapshot(lir, unbox->bailoutKind());
        useBox(lir, LUnboxFloatingPoint::Input, inner);
        define(lir, unbox);
        return;
    }

    
    LUnbox *lir = new(alloc()) LUnbox;
    lir->setOperand(0, usePayloadInRegisterAtStart(inner));
    lir->setOperand(1, useType(inner, LUse::ANY));

    if (unbox->fallible())
        assignSnapshot(lir, unbox->bailoutKind());

    
    
    
    
    
    defineReuseInput(lir, unbox, 0);
}

void
LIRGeneratorX86::visitReturn(MReturn *ret)
{
    MDefinition *opd = ret->getOperand(0);
    MOZ_ASSERT(opd->type() == MIRType_Value);

    LReturn *ins = new(alloc()) LReturn;
    ins->setOperand(0, LUse(JSReturnReg_Type));
    ins->setOperand(1, LUse(JSReturnReg_Data));
    fillBoxUses(ins, 0, opd);
    add(ins);
}

void
LIRGeneratorX86::defineUntypedPhi(MPhi *phi, size_t lirIndex)
{
    LPhi *type = current->getPhi(lirIndex + VREG_TYPE_OFFSET);
    LPhi *payload = current->getPhi(lirIndex + VREG_DATA_OFFSET);

    uint32_t typeVreg = getVirtualRegister();

    phi->setVirtualRegister(typeVreg);

    uint32_t payloadVreg = getVirtualRegister();
    MOZ_ASSERT(typeVreg + 1 == payloadVreg);

    type->setDef(0, LDefinition(typeVreg, LDefinition::TYPE));
    payload->setDef(0, LDefinition(payloadVreg, LDefinition::PAYLOAD));
    annotate(type);
    annotate(payload);
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

void
LIRGeneratorX86::visitAsmJSUnsignedToDouble(MAsmJSUnsignedToDouble *ins)
{
    MOZ_ASSERT(ins->input()->type() == MIRType_Int32);
    LAsmJSUInt32ToDouble *lir = new(alloc()) LAsmJSUInt32ToDouble(useRegisterAtStart(ins->input()), temp());
    define(lir, ins);
}

void
LIRGeneratorX86::visitAsmJSUnsignedToFloat32(MAsmJSUnsignedToFloat32 *ins)
{
    MOZ_ASSERT(ins->input()->type() == MIRType_Int32);
    LAsmJSUInt32ToFloat32 *lir = new(alloc()) LAsmJSUInt32ToFloat32(useRegisterAtStart(ins->input()), temp());
    define(lir, ins);
}

void
LIRGeneratorX86::visitAsmJSLoadHeap(MAsmJSLoadHeap *ins)
{
    MDefinition *ptr = ins->ptr();
    LAllocation ptrAlloc;
    MOZ_ASSERT(ptr->type() == MIRType_Int32);

    
    if (ptr->isConstant() && !ins->needsBoundsCheck()) {
        
        MOZ_ASSERT(ptr->toConstant()->value().toInt32() >= 0);
        ptrAlloc = LAllocation(ptr->toConstant()->vp());
    } else {
        ptrAlloc = useRegisterAtStart(ptr);
    }
    LAsmJSLoadHeap *lir = new(alloc()) LAsmJSLoadHeap(ptrAlloc);
    define(lir, ins);
}

void
LIRGeneratorX86::visitAsmJSStoreHeap(MAsmJSStoreHeap *ins)
{
    MDefinition *ptr = ins->ptr();
    LAsmJSStoreHeap *lir;
    MOZ_ASSERT(ptr->type() == MIRType_Int32);

    if (ptr->isConstant() && !ins->needsBoundsCheck()) {
        MOZ_ASSERT(ptr->toConstant()->value().toInt32() >= 0);
        LAllocation ptrAlloc = LAllocation(ptr->toConstant()->vp());
        switch (ins->accessType()) {
          case Scalar::Int8: case Scalar::Uint8:
            
            lir = new(alloc()) LAsmJSStoreHeap(ptrAlloc, useFixed(ins->value(), eax));
            break;
          case Scalar::Int16: case Scalar::Uint16:
          case Scalar::Int32: case Scalar::Uint32:
          case Scalar::Float32: case Scalar::Float64:
          case Scalar::Float32x4: case Scalar::Int32x4:
            
            lir = new(alloc()) LAsmJSStoreHeap(ptrAlloc, useRegisterAtStart(ins->value()));
            break;
          case Scalar::Uint8Clamped:
          case Scalar::MaxTypedArrayViewType:
            MOZ_CRASH("unexpected array type");
        }
        add(lir, ins);
        return;
    }

    switch (ins->accessType()) {
      case Scalar::Int8: case Scalar::Uint8:
        
        lir = new(alloc()) LAsmJSStoreHeap(useRegister(ins->ptr()), useFixed(ins->value(), eax));
        break;
      case Scalar::Int16: case Scalar::Uint16:
      case Scalar::Int32: case Scalar::Uint32:
      case Scalar::Float32: case Scalar::Float64:
      case Scalar::Float32x4: case Scalar::Int32x4:
        
        
        lir = new(alloc()) LAsmJSStoreHeap(useRegisterAtStart(ptr), useRegisterAtStart(ins->value()));
        break;
      case Scalar::Uint8Clamped:
      case Scalar::MaxTypedArrayViewType:
        MOZ_CRASH("unexpected array type");
    }

    add(lir, ins);
}

void
LIRGeneratorX86::visitStoreTypedArrayElementStatic(MStoreTypedArrayElementStatic *ins)
{
    
    
    LStoreTypedArrayElementStatic *lir;
    switch (ins->accessType()) {
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
      default: MOZ_CRASH("unexpected array type");
    }

    add(lir, ins);
}

void
LIRGeneratorX86::visitAsmJSLoadFuncPtr(MAsmJSLoadFuncPtr *ins)
{
    define(new(alloc()) LAsmJSLoadFuncPtr(useRegisterAtStart(ins->index())), ins);
}

void
LIRGeneratorX86::visitSubstr(MSubstr *ins)
{
    
    
    
    LSubstr *lir = new (alloc()) LSubstr(useRegister(ins->string()),
                                         useRegister(ins->begin()),
                                         useRegister(ins->length()),
                                         temp(),
                                         LDefinition::BogusTemp(),
                                         tempByteOpRegister());
    define(lir, ins);
    assignSafepoint(lir, ins);
}
