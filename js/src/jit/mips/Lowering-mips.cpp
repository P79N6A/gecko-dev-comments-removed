





#include "mozilla/MathAlgorithms.h"


#include "jit/Lowering.h"
#include "jit/mips/Assembler-mips.h"
#include "jit/MIR.h"

#include "jit/shared/Lowering-shared-inl.h"

using namespace js;
using namespace js::jit;

using mozilla::FloorLog2;

void
LIRGeneratorMIPS::useBox(LInstruction *lir, size_t n, MDefinition *mir,
                         LUse::Policy policy, bool useAtStart)
{
    MOZ_ASSERT(mir->type() == MIRType_Value);

    ensureDefined(mir);
    lir->setOperand(n, LUse(mir->virtualRegister(), policy, useAtStart));
    lir->setOperand(n + 1, LUse(VirtualRegisterOfPayload(mir), policy, useAtStart));
}

void
LIRGeneratorMIPS::useBoxFixed(LInstruction *lir, size_t n, MDefinition *mir, Register reg1,
                              Register reg2)
{
    MOZ_ASSERT(mir->type() == MIRType_Value);
    MOZ_ASSERT(reg1 != reg2);

    ensureDefined(mir);
    lir->setOperand(n, LUse(reg1, mir->virtualRegister()));
    lir->setOperand(n + 1, LUse(reg2, VirtualRegisterOfPayload(mir)));
}

LAllocation
LIRGeneratorMIPS::useByteOpRegister(MDefinition *mir)
{
    return useRegister(mir);
}

LAllocation
LIRGeneratorMIPS::useByteOpRegisterOrNonDoubleConstant(MDefinition *mir)
{
    return useRegisterOrNonDoubleConstant(mir);
}

LDefinition
LIRGeneratorMIPS::tempByteOpRegister()
{
    return temp();
}

void
LIRGeneratorMIPS::lowerConstantDouble(double d, MInstruction *mir)
{
    return define(new(alloc()) LDouble(d), mir);
}

void
LIRGeneratorMIPS::lowerConstantFloat32(float d, MInstruction *mir)
{
    define(new(alloc()) LFloat32(d), mir);
}

void
LIRGeneratorMIPS::visitConstant(MConstant *ins)
{
    if (ins->type() == MIRType_Double)
        lowerConstantDouble(ins->value().toDouble(), ins);
    else if (ins->type() == MIRType_Float32)
        lowerConstantFloat32(ins->value().toDouble(), ins);
    else if (ins->canEmitAtUses())
        emitAtUses(ins);
    else
        LIRGeneratorShared::visitConstant(ins);
}

void
LIRGeneratorMIPS::visitBox(MBox *box)
{
    MDefinition *inner = box->getOperand(0);

    
    if (IsFloatingPointType(inner->type())) {
        defineBox(new(alloc()) LBoxFloatingPoint(useRegisterAtStart(inner),
                                                 tempCopy(inner, 0), inner->type()), box);
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
LIRGeneratorMIPS::visitUnbox(MUnbox *unbox)
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
    lir->setOperand(1, useType(inner, LUse::REGISTER));

    if (unbox->fallible())
        assignSnapshot(lir, unbox->bailoutKind());

    
    
    
    
    
    defineReuseInput(lir, unbox, 0);
}

void
LIRGeneratorMIPS::visitReturn(MReturn *ret)
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
LIRGeneratorMIPS::lowerForALU(LInstructionHelper<1, 1, 0> *ins,
                              MDefinition *mir, MDefinition *input)
{
    ins->setOperand(0, useRegister(input));
    define(ins, mir, LDefinition(LDefinition::TypeFrom(mir->type()), LDefinition::REGISTER));
}


void
LIRGeneratorMIPS::lowerForALU(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir,
                              MDefinition *lhs, MDefinition *rhs)
{
    ins->setOperand(0, useRegister(lhs));
    ins->setOperand(1, useRegisterOrConstant(rhs));
    define(ins, mir, LDefinition(LDefinition::TypeFrom(mir->type()), LDefinition::REGISTER));
}

void
LIRGeneratorMIPS::lowerForFPU(LInstructionHelper<1, 1, 0> *ins, MDefinition *mir,
                              MDefinition *input)
{
    ins->setOperand(0, useRegister(input));
    define(ins, mir, LDefinition(LDefinition::TypeFrom(mir->type()), LDefinition::REGISTER));
}

template<size_t Temps>
void
LIRGeneratorMIPS::lowerForFPU(LInstructionHelper<1, 2, Temps> *ins, MDefinition *mir,
                              MDefinition *lhs, MDefinition *rhs)
{
    ins->setOperand(0, useRegister(lhs));
    ins->setOperand(1, useRegister(rhs));
    define(ins, mir, LDefinition(LDefinition::TypeFrom(mir->type()), LDefinition::REGISTER));
}

template void LIRGeneratorMIPS::lowerForFPU(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir,
                                            MDefinition *lhs, MDefinition *rhs);
template void LIRGeneratorMIPS::lowerForFPU(LInstructionHelper<1, 2, 1> *ins, MDefinition *mir,
                                            MDefinition *lhs, MDefinition *rhs);

void
LIRGeneratorMIPS::lowerForBitAndAndBranch(LBitAndAndBranch *baab, MInstruction *mir,
                                          MDefinition *lhs, MDefinition *rhs)
{
    baab->setOperand(0, useRegisterAtStart(lhs));
    baab->setOperand(1, useRegisterOrConstantAtStart(rhs));
    add(baab, mir);
}

void
LIRGeneratorMIPS::defineUntypedPhi(MPhi *phi, size_t lirIndex)
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
LIRGeneratorMIPS::lowerUntypedPhiInput(MPhi *phi, uint32_t inputPosition,
                                       LBlock *block, size_t lirIndex)
{
    MDefinition *operand = phi->getOperand(inputPosition);
    LPhi *type = block->getPhi(lirIndex + VREG_TYPE_OFFSET);
    LPhi *payload = block->getPhi(lirIndex + VREG_DATA_OFFSET);
    type->setOperand(inputPosition, LUse(operand->virtualRegister() + VREG_TYPE_OFFSET,
                                         LUse::ANY));
    payload->setOperand(inputPosition, LUse(VirtualRegisterOfPayload(operand), LUse::ANY));
}

void
LIRGeneratorMIPS::lowerForShift(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir,
                                MDefinition *lhs, MDefinition *rhs)
{
    ins->setOperand(0, useRegister(lhs));
    ins->setOperand(1, useRegisterOrConstant(rhs));
    define(ins, mir);
}

void
LIRGeneratorMIPS::lowerDivI(MDiv *div)
{
    if (div->isUnsigned()) {
        lowerUDiv(div);
        return;
    }

    
    
    if (div->rhs()->isConstant()) {
        int32_t rhs = div->rhs()->toConstant()->value().toInt32();
        
        
        
        
        
        int32_t shift = FloorLog2(rhs);
        if (rhs > 0 && 1 << shift == rhs) {
            LDivPowTwoI *lir = new(alloc()) LDivPowTwoI(useRegister(div->lhs()), shift, temp());
            if (div->fallible())
                assignSnapshot(lir, Bailout_DoubleOutput);
            define(lir, div);
            return;
        }
    }

    LDivI *lir = new(alloc()) LDivI(useRegister(div->lhs()), useRegister(div->rhs()), temp());
    if (div->fallible())
        assignSnapshot(lir, Bailout_DoubleOutput);
    define(lir, div);
}

void
LIRGeneratorMIPS::lowerMulI(MMul *mul, MDefinition *lhs, MDefinition *rhs)
{
    LMulI *lir = new(alloc()) LMulI;
    if (mul->fallible())
        assignSnapshot(lir, Bailout_DoubleOutput);

    lowerForALU(lir, mul, lhs, rhs);
}

void
LIRGeneratorMIPS::lowerModI(MMod *mod)
{
    if (mod->isUnsigned()) {
        lowerUMod(mod);
        return;
    }

    if (mod->rhs()->isConstant()) {
        int32_t rhs = mod->rhs()->toConstant()->value().toInt32();
        int32_t shift = FloorLog2(rhs);
        if (rhs > 0 && 1 << shift == rhs) {
            LModPowTwoI *lir = new(alloc()) LModPowTwoI(useRegister(mod->lhs()), shift);
            if (mod->fallible())
                assignSnapshot(lir, Bailout_DoubleOutput);
            define(lir, mod);
            return;
        } else if (shift < 31 && (1 << (shift + 1)) - 1 == rhs) {
            LModMaskI *lir = new(alloc()) LModMaskI(useRegister(mod->lhs()),
                                                    temp(LDefinition::GENERAL),
                                                    temp(LDefinition::GENERAL),
                                                    shift + 1);
            if (mod->fallible())
                assignSnapshot(lir, Bailout_DoubleOutput);
            define(lir, mod);
            return;
        }
    }
    LModI *lir = new(alloc()) LModI(useRegister(mod->lhs()), useRegister(mod->rhs()),
                           temp(LDefinition::GENERAL));

    if (mod->fallible())
        assignSnapshot(lir, Bailout_DoubleOutput);
    define(lir, mod);
}

void
LIRGeneratorMIPS::visitPowHalf(MPowHalf *ins)
{
    MDefinition *input = ins->input();
    MOZ_ASSERT(input->type() == MIRType_Double);
    LPowHalfD *lir = new(alloc()) LPowHalfD(useRegisterAtStart(input));
    defineReuseInput(lir, ins, 0);
}

LTableSwitch *
LIRGeneratorMIPS::newLTableSwitch(const LAllocation &in, const LDefinition &inputCopy,
                                  MTableSwitch *tableswitch)
{
    return new(alloc()) LTableSwitch(in, inputCopy, temp(), tableswitch);
}

LTableSwitchV *
LIRGeneratorMIPS::newLTableSwitchV(MTableSwitch *tableswitch)
{
    return new(alloc()) LTableSwitchV(temp(), tempDouble(), temp(), tableswitch);
}

void
LIRGeneratorMIPS::visitGuardShape(MGuardShape *ins)
{
    MOZ_ASSERT(ins->obj()->type() == MIRType_Object);

    LDefinition tempObj = temp(LDefinition::OBJECT);
    LGuardShape *guard = new(alloc()) LGuardShape(useRegister(ins->obj()), tempObj);
    assignSnapshot(guard, ins->bailoutKind());
    add(guard, ins);
    redefine(ins, ins->obj());
}

void
LIRGeneratorMIPS::visitGuardObjectGroup(MGuardObjectGroup *ins)
{
    MOZ_ASSERT(ins->obj()->type() == MIRType_Object);

    LDefinition tempObj = temp(LDefinition::OBJECT);
    LGuardObjectGroup *guard = new(alloc()) LGuardObjectGroup(useRegister(ins->obj()), tempObj);
    assignSnapshot(guard, ins->bailoutKind());
    add(guard, ins);
    redefine(ins, ins->obj());
}

void
LIRGeneratorMIPS::lowerUrshD(MUrsh *mir)
{
    MDefinition *lhs = mir->lhs();
    MDefinition *rhs = mir->rhs();

    MOZ_ASSERT(lhs->type() == MIRType_Int32);
    MOZ_ASSERT(rhs->type() == MIRType_Int32);

    LUrshD *lir = new(alloc()) LUrshD(useRegister(lhs), useRegisterOrConstant(rhs), temp());
    define(lir, mir);
}

void
LIRGeneratorMIPS::visitAsmJSNeg(MAsmJSNeg *ins)
{
    if (ins->type() == MIRType_Int32) {
        define(new(alloc()) LNegI(useRegisterAtStart(ins->input())), ins);
    } else if (ins->type() == MIRType_Float32) {
        define(new(alloc()) LNegF(useRegisterAtStart(ins->input())), ins);
    } else {
        MOZ_ASSERT(ins->type() == MIRType_Double);
        define(new(alloc()) LNegD(useRegisterAtStart(ins->input())), ins);
    }
}

void
LIRGeneratorMIPS::lowerUDiv(MDiv *div)
{
    MDefinition *lhs = div->getOperand(0);
    MDefinition *rhs = div->getOperand(1);

    LUDiv *lir = new(alloc()) LUDiv;
    lir->setOperand(0, useRegister(lhs));
    lir->setOperand(1, useRegister(rhs));
    if (div->fallible())
        assignSnapshot(lir, Bailout_DoubleOutput);

    define(lir, div);
}

void
LIRGeneratorMIPS::lowerUMod(MMod *mod)
{
    MDefinition *lhs = mod->getOperand(0);
    MDefinition *rhs = mod->getOperand(1);

    LUMod *lir = new(alloc()) LUMod;
    lir->setOperand(0, useRegister(lhs));
    lir->setOperand(1, useRegister(rhs));
    if (mod->fallible())
        assignSnapshot(lir, Bailout_DoubleOutput);

    define(lir, mod);
}

void
LIRGeneratorMIPS::visitAsmJSUnsignedToDouble(MAsmJSUnsignedToDouble *ins)
{
    MOZ_ASSERT(ins->input()->type() == MIRType_Int32);
    LAsmJSUInt32ToDouble *lir = new(alloc()) LAsmJSUInt32ToDouble(useRegisterAtStart(ins->input()));
    define(lir, ins);
}

void
LIRGeneratorMIPS::visitAsmJSUnsignedToFloat32(MAsmJSUnsignedToFloat32 *ins)
{
    MOZ_ASSERT(ins->input()->type() == MIRType_Int32);
    LAsmJSUInt32ToFloat32 *lir = new(alloc()) LAsmJSUInt32ToFloat32(useRegisterAtStart(ins->input()));
    define(lir, ins);
}

void
LIRGeneratorMIPS::visitAsmJSLoadHeap(MAsmJSLoadHeap *ins)
{
    MDefinition *ptr = ins->ptr();
    MOZ_ASSERT(ptr->type() == MIRType_Int32);
    LAllocation ptrAlloc;

    
    
    if (ptr->isConstantValue() && !ins->needsBoundsCheck()) {
        
        MOZ_ASSERT(ptr->constantValue().toInt32() >= 0);
        ptrAlloc = LAllocation(ptr->constantVp());
    } else
        ptrAlloc = useRegisterAtStart(ptr);

    define(new(alloc()) LAsmJSLoadHeap(ptrAlloc), ins);
}

void
LIRGeneratorMIPS::visitAsmJSStoreHeap(MAsmJSStoreHeap *ins)
{
    MDefinition *ptr = ins->ptr();
    MOZ_ASSERT(ptr->type() == MIRType_Int32);
    LAllocation ptrAlloc;

    if (ptr->isConstantValue() && !ins->needsBoundsCheck()) {
        MOZ_ASSERT(ptr->constantValue().toInt32() >= 0);
        ptrAlloc = LAllocation(ptr->constantVp());
    } else
        ptrAlloc = useRegisterAtStart(ptr);

    add(new(alloc()) LAsmJSStoreHeap(ptrAlloc, useRegisterAtStart(ins->value())), ins);
}

void
LIRGeneratorMIPS::visitAsmJSLoadFuncPtr(MAsmJSLoadFuncPtr *ins)
{
    define(new(alloc()) LAsmJSLoadFuncPtr(useRegister(ins->index())), ins);
}

void
LIRGeneratorMIPS::lowerTruncateDToInt32(MTruncateToInt32 *ins)
{
    MDefinition *opd = ins->input();
    MOZ_ASSERT(opd->type() == MIRType_Double);

    define(new(alloc()) LTruncateDToInt32(useRegister(opd), LDefinition::BogusTemp()), ins);
}

void
LIRGeneratorMIPS::lowerTruncateFToInt32(MTruncateToInt32 *ins)
{
    MDefinition *opd = ins->input();
    MOZ_ASSERT(opd->type() == MIRType_Float32);

    define(new(alloc()) LTruncateFToInt32(useRegister(opd), LDefinition::BogusTemp()), ins);
}

void
LIRGeneratorMIPS::visitSubstr(MSubstr *ins)
{
    LSubstr *lir = new (alloc()) LSubstr(useRegister(ins->string()),
                                         useRegister(ins->begin()),
                                         useRegister(ins->length()),
                                         temp(),
                                         temp(),
                                         tempByteOpRegister());
    define(lir, ins);
    assignSafepoint(lir, ins);
}

void
LIRGeneratorMIPS::visitStoreTypedArrayElementStatic(MStoreTypedArrayElementStatic *ins)
{
    MOZ_CRASH("NYI");
}

void
LIRGeneratorMIPS::visitSimdBinaryArith(MSimdBinaryArith *ins)
{
    MOZ_CRASH("NYI");
}

void
LIRGeneratorMIPS::visitSimdSelect(MSimdSelect *ins)
{
    MOZ_CRASH("NYI");
}

void
LIRGeneratorMIPS::visitSimdSplatX4(MSimdSplatX4 *ins)
{
    MOZ_CRASH("NYI");
}

void
LIRGeneratorMIPS::visitSimdValueX4(MSimdValueX4 *ins)
{
    MOZ_CRASH("NYI");
}

void
LIRGeneratorMIPS::visitCompareExchangeTypedArrayElement(MCompareExchangeTypedArrayElement *ins)
{
    MOZ_CRASH("NYI");
}

void
LIRGeneratorMIPS::visitAsmJSCompareExchangeHeap(MAsmJSCompareExchangeHeap *ins)
{
    MOZ_CRASH("NYI");
}

void
LIRGeneratorMIPS::visitAsmJSAtomicBinopHeap(MAsmJSAtomicBinopHeap *ins)
{
    MOZ_CRASH("NYI");
}

void
LIRGeneratorMIPS::visitAtomicTypedArrayElementBinop(MAtomicTypedArrayElementBinop *ins)
{
    MOZ_CRASH("NYI");
}
