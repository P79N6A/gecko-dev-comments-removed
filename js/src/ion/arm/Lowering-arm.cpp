






#include "ion/MIR.h"
#include "ion/Lowering.h"
#include "Assembler-arm.h"
#include "ion/shared/Lowering-shared-inl.h"

using namespace js;
using namespace js::ion;

bool
LIRGeneratorARM::useBox(LInstruction *lir, size_t n, MDefinition *mir,
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
LIRGeneratorARM::useBoxFixed(LInstruction *lir, size_t n, MDefinition *mir, Register reg1,
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

bool
LIRGeneratorARM::lowerConstantDouble(double d, MInstruction *mir)
{
    uint32_t index;
    if (!lirGraph_.addConstantToPool(DoubleValue(d), &index))
        return false;

    LDouble *lir = new LDouble(LConstantIndex::FromIndex(index));
    return define(lir, mir);
}

bool
LIRGeneratorARM::visitConstant(MConstant *ins)
{
    if (ins->type() == MIRType_Double) {
        uint32_t index;
        if (!lirGraph_.addConstantToPool(ins->value(), &index))
            return false;
        LDouble *lir = new LDouble(LConstantIndex::FromIndex(index));
        return define(lir, ins);
    }

    
    if (ins->canEmitAtUses())
        return emitAtUses(ins);

    return LIRGeneratorShared::visitConstant(ins);
}

bool
LIRGeneratorARM::visitBox(MBox *box)
{
    MDefinition *inner = box->getOperand(0);

    
    if (inner->type() == MIRType_Double)
        return defineBox(new LBoxDouble(useRegisterAtStart(inner), tempCopy(inner, 0)), box);

    if (box->canEmitAtUses())
        return emitAtUses(box);

    if (inner->isConstant())
        return defineBox(new LValue(inner->toConstant()->value()), box);

    LBox *lir = new LBox(use(inner), inner->type());

    
    
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
LIRGeneratorARM::visitUnbox(MUnbox *unbox)
{
    
    
    
    MDefinition *inner = unbox->getOperand(0);

    if (!ensureDefined(inner))
        return false;

    if (unbox->type() == MIRType_Double) {
        LUnboxDouble *lir = new LUnboxDouble();
        if (unbox->fallible() && !assignSnapshot(lir, unbox->bailoutKind()))
            return false;
        if (!useBox(lir, LUnboxDouble::Input, inner))
            return false;
        return define(lir, unbox);
    }

    
    LUnbox *lir = new LUnbox;
    lir->setOperand(0, usePayloadInRegisterAtStart(inner));
    lir->setOperand(1, useType(inner, LUse::REGISTER));

    if (unbox->fallible() && !assignSnapshot(lir, unbox->bailoutKind()))
        return false;

    
    
    
    
    
    
    return defineReuseInput(lir, unbox, 0);
}

bool
LIRGeneratorARM::visitReturn(MReturn *ret)
{
    MDefinition *opd = ret->getOperand(0);
    JS_ASSERT(opd->type() == MIRType_Value);

    LReturn *ins = new LReturn;
    ins->setOperand(0, LUse(JSReturnReg_Type));
    ins->setOperand(1, LUse(JSReturnReg_Data));
    return fillBoxUses(ins, 0, opd) && add(ins);
}


bool
LIRGeneratorARM::lowerForALU(LInstructionHelper<1, 1, 0> *ins, MDefinition *mir, MDefinition *input)
{
    ins->setOperand(0, useRegister(input));
    return define(ins, mir,
                  LDefinition(LDefinition::TypeFrom(mir->type()), LDefinition::DEFAULT));
}


bool
LIRGeneratorARM::lowerForALU(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir, MDefinition *lhs, MDefinition *rhs)
{
    ins->setOperand(0, useRegister(lhs));
    ins->setOperand(1, useRegisterOrConstant(rhs));
    return define(ins, mir,
                  LDefinition(LDefinition::TypeFrom(mir->type()), LDefinition::DEFAULT));
}

bool
LIRGeneratorARM::lowerForFPU(LInstructionHelper<1, 1, 0> *ins, MDefinition *mir, MDefinition *input)
{
    ins->setOperand(0, useRegister(input));
    return define(ins, mir,
                  LDefinition(LDefinition::TypeFrom(mir->type()), LDefinition::DEFAULT));

}

bool
LIRGeneratorARM::lowerForFPU(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir, MDefinition *lhs, MDefinition *rhs)
{
    ins->setOperand(0, useRegister(lhs));
    ins->setOperand(1, useRegister(rhs));
    return define(ins, mir,
                  LDefinition(LDefinition::TypeFrom(mir->type()), LDefinition::DEFAULT));
}

bool
LIRGeneratorARM::defineUntypedPhi(MPhi *phi, size_t lirIndex)
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
LIRGeneratorARM::lowerUntypedPhiInput(MPhi *phi, uint32_t inputPosition, LBlock *block, size_t lirIndex)
{
    
    MDefinition *operand = phi->getOperand(inputPosition);
    LPhi *type = block->getPhi(lirIndex + VREG_TYPE_OFFSET);
    LPhi *payload = block->getPhi(lirIndex + VREG_DATA_OFFSET);
    type->setOperand(inputPosition, LUse(operand->virtualRegister() + VREG_TYPE_OFFSET, LUse::ANY));
    payload->setOperand(inputPosition, LUse(VirtualRegisterOfPayload(operand), LUse::ANY));
}

bool
LIRGeneratorARM::lowerForShift(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir, MDefinition *lhs, MDefinition *rhs)
{

    ins->setOperand(0, useRegister(lhs));
    ins->setOperand(1, useRegisterOrConstant(rhs));
    return define(ins, mir);
}

bool
LIRGeneratorARM::lowerDivI(MDiv *div)
{
    LDivI *lir = new LDivI(useFixed(div->lhs(), r0), use(div->rhs(), r1),
                           tempFixed(r2), tempFixed(r3));
    if (div->fallible() && !assignSnapshot(lir))
        return false;
    return defineFixed(lir, div, LAllocation(AnyRegister(r0)));
}

bool
LIRGeneratorARM::lowerMulI(MMul *mul, MDefinition *lhs, MDefinition *rhs)
{
    LMulI *lir = new LMulI;
    if (mul->fallible() && !assignSnapshot(lir))
        return false;
    return lowerForALU(lir, mul, lhs, rhs);
}

bool
LIRGeneratorARM::lowerModI(MMod *mod)
{
    if (mod->rhs()->isConstant()) {
        int32_t rhs = mod->rhs()->toConstant()->value().toInt32();
        int32_t shift;
        JS_FLOOR_LOG2(shift, rhs);
        if (rhs > 0 && 1 << shift == rhs) {
            LModPowTwoI *lir = new LModPowTwoI(useRegister(mod->lhs()), shift);
            if (mod->fallible() && !assignSnapshot(lir))
                return false;
            return define(lir, mod);
        } else if (shift < 31 && (1 << (shift+1)) - 1 == rhs) {
            LModMaskI *lir = new LModMaskI(useRegister(mod->lhs()), temp(LDefinition::GENERAL), shift+1);
            if (mod->fallible() && !assignSnapshot(lir))
                return false;
            return define(lir, mod);
        }
    }
    LModI *lir = new LModI(useFixed(mod->lhs(), r0), use(mod->rhs(), r1),
                           tempFixed(r2), tempFixed(r3), temp(LDefinition::GENERAL));

    if (mod->fallible() && !assignSnapshot(lir))
        return false;
    return defineFixed(lir, mod, LAllocation(AnyRegister(r1)));
}

bool
LIRGeneratorARM::visitPowHalf(MPowHalf *ins)
{
    MDefinition *input = ins->input();
    JS_ASSERT(input->type() == MIRType_Double);
    LPowHalfD *lir = new LPowHalfD(useRegisterAtStart(input));
    return defineReuseInput(lir, ins, 0);
}

LTableSwitch *
LIRGeneratorARM::newLTableSwitch(const LAllocation &in, const LDefinition &inputCopy,
                                       MTableSwitch *tableswitch)
{
    return new LTableSwitch(in, inputCopy, tableswitch);
}

LTableSwitchV *
LIRGeneratorARM::newLTableSwitchV(MTableSwitch *tableswitch)
{
    return new LTableSwitchV(temp(), tempFloat(), tableswitch);
}

bool
LIRGeneratorARM::visitGuardShape(MGuardShape *ins)
{
    JS_ASSERT(ins->obj()->type() == MIRType_Object);

    LDefinition tempObj = temp(LDefinition::OBJECT);
    LGuardShape *guard = new LGuardShape(useRegister(ins->obj()), tempObj);
    if (!assignSnapshot(guard, ins->bailoutKind()))
        return false;
    if (!add(guard, ins))
        return false;
    return redefine(ins, ins->obj());
}

bool
LIRGeneratorARM::visitStoreTypedArrayElement(MStoreTypedArrayElement *ins)
{
    JS_ASSERT(ins->elements()->type() == MIRType_Elements);
    JS_ASSERT(ins->index()->type() == MIRType_Int32);

    if (ins->isFloatArray())
        JS_ASSERT(ins->value()->type() == MIRType_Double);
    else
        JS_ASSERT(ins->value()->type() == MIRType_Int32);

    LUse elements = useRegister(ins->elements());
    LAllocation index = useRegisterOrConstant(ins->index());
    LAllocation value = useRegisterOrNonDoubleConstant(ins->value());
    return add(new LStoreTypedArrayElement(elements, index, value), ins);
}

bool
LIRGeneratorARM::visitInterruptCheck(MInterruptCheck *ins)
{
    LInterruptCheck *lir = new LInterruptCheck();
    if (!add(lir))
        return false;
    if (!assignSafepoint(lir, ins))
        return false;
    return true;
}

bool
LIRGeneratorARM::lowerUrshD(MUrsh *mir)
{
    MDefinition *lhs = mir->lhs();
    MDefinition *rhs = mir->rhs();

    JS_ASSERT(lhs->type() == MIRType_Int32);
    JS_ASSERT(rhs->type() == MIRType_Int32);

    LUrshD *lir = new LUrshD(useRegister(lhs), useRegisterOrConstant(rhs), temp());
    return define(lir, mir);
}
