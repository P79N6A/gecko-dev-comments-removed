








































#include "ion/MIR.h"
#include "ion/Lowering.h"
#include "Assembler-arm.h"
#include "ion/shared/Lowering-shared-inl.h"
#include "Lowering-arm-inl.h"

using namespace js;
using namespace js::ion;

bool
LIRGeneratorARM::useBox(LInstruction *lir, size_t n, MDefinition *mir,
                        LUse::Policy policy)
{
    JS_ASSERT(mir->type() == MIRType_Value);
    if (!ensureDefined(mir))
        return false;
    lir->setOperand(n, LUse(mir->id(), policy));
    lir->setOperand(n + 1, LUse(VirtualRegisterOfPayload(mir), policy));
    return true;
}

bool
LIRGeneratorARM::lowerConstantDouble(double d, MInstruction *mir)
{
    uint32 index;
    if (!lirGraph_.addConstantToPool(d, &index))
        return false;

    LDouble *lir = new LDouble(LConstantIndex::FromIndex(index));
    return define(lir, mir);
}

bool
LIRGeneratorARM::visitConstant(MConstant *ins)
{
    if (ins->canEmitAtUses() && ins->type() != MIRType_Double)
        return emitAtUses(ins);

    if (ins->type() == MIRType_Double) {
        uint32 index;
        if (!lirGraph_.addConstantToPool(ins, &index))
            return false;
        LDouble *lir = new LDouble(LConstantIndex::FromIndex(index));
        return define(lir, ins);
    }

    return LIRGeneratorShared::visitConstant(ins);
}

bool
LIRGeneratorARM::visitBox(MBox *box)
{
    MDefinition *inner = box->getOperand(0);

    
    if (inner->type() == MIRType_Double)
        return defineBox(new LBoxDouble(use(inner, LUse::COPY)), box);

    if (box->canEmitAtUses())
        return emitAtUses(box);

    if (inner->isConstant())
        return defineBox(new LValue(inner->toConstant()->value()), box);

    LBox *lir = new LBox(use(inner), inner->type());

    
    
    uint32 vreg = getVirtualRegister();
    if (vreg >= MAX_VIRTUAL_REGISTERS)
        return false;

    lir->setDef(0, LDefinition(vreg, LDefinition::TYPE));
    lir->setDef(1, LDefinition(inner->id(), LDefinition::PAYLOAD, LDefinition::REDEFINED));
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
        if (unbox->fallible() && !assignSnapshot(lir))
            return false;
        if (!useBox(lir, LUnboxDouble::Input, inner))
            return false;
        return define(lir, unbox);
    }

    LUnbox *lir = new LUnbox(unbox->type());
    lir->setOperand(0, useType(inner, LUse::REGISTER));
    lir->setOperand(1, usePayloadInRegister(inner));

    
    LDefinition::Type type = LDefinition::TypeFrom(unbox->type());
    lir->setDef(0, LDefinition(VirtualRegisterOfPayload(inner), type, LDefinition::REDEFINED));
    unbox->setVirtualRegister(VirtualRegisterOfPayload(inner));

    return assignSnapshot(lir) && add(lir);
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

LSnapshot *
LIRGeneratorARM::buildSnapshot(LInstruction *ins, MResumePoint *rp, BailoutKind kind)
{
    LSnapshot *snapshot = LSnapshot::New(gen, rp, kind);
    if (!snapshot)
        return NULL;

    FlattenedMResumePointIter iter(rp);
    if (!iter.init())
        return NULL;

    size_t i = 0;
    for (MResumePoint **it = iter.begin(), **end = iter.end(); it != end; ++it) {
        MResumePoint *mir = *it;
        for (size_t j = 0; j < mir->numOperands(); ++i, ++j) {
            MDefinition *ins = mir->getOperand(j);
            LAllocation *type = snapshot->typeOfSlot(i);
            LAllocation *payload = snapshot->payloadOfSlot(i);

            
            
            
            
            
            if (ins->isConstant() || ins->isUnused()) {
                *type = LConstantIndex::Bogus();
                *payload = LConstantIndex::Bogus();
            } else if (ins->type() != MIRType_Value) {
                *type = LConstantIndex::Bogus();
                *payload = use(ins, LUse::KEEPALIVE);
            } else {
                *type = useType(ins, LUse::KEEPALIVE);
                *payload = usePayload(ins, LUse::KEEPALIVE);
            }
        }
    }

    return snapshot;
}

bool
LIRGeneratorARM::assignSnapshot(LInstruction *ins, BailoutKind kind)
{
    LSnapshot *snapshot = buildSnapshot(ins, lastResumePoint_, kind);
    if (!snapshot)
        return false;

    ins->assignSnapshot(snapshot);
    return true;
}

bool
LIRGeneratorARM::assignPostSnapshot(MInstruction *mir, LInstruction *ins)
{
    JS_ASSERT(mir->resumePoint());
    
    JS_ASSERT(!postSnapshot_);

    LSnapshot *snapshot = buildSnapshot(ins, mir->resumePoint(), Bailout_Normal);
    if (!snapshot)
        return false;

    ins->assignPostSnapshot(snapshot);
    postSnapshot_ = snapshot;
    return true;
}

bool
LIRGeneratorARM::assignSafepoint(MInstruction *mir, LInstruction *ins)
{
    ins->setMir(mir);
    if (mir->isIdempotent()) {
        if (!ins->snapshot())
            return assignSnapshot(ins);
        return true;
    } else {
        if (!ins->postSnapshot())
            return assignPostSnapshot(mir, ins);
        return true;
    }
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

    uint32 typeVreg = getVirtualRegister();
    if (typeVreg >= MAX_VIRTUAL_REGISTERS)
        return false;

    phi->setVirtualRegister(typeVreg);

    uint32 payloadVreg = getVirtualRegister();
    if (payloadVreg >= MAX_VIRTUAL_REGISTERS)
        return false;
    JS_ASSERT(typeVreg + 1 == payloadVreg);

    type->setDef(0, LDefinition(typeVreg, LDefinition::TYPE));
    payload->setDef(0, LDefinition(payloadVreg, LDefinition::PAYLOAD));
    return annotate(type) && annotate(payload);
}

void
LIRGeneratorARM::lowerUntypedPhiInput(MPhi *phi, uint32 inputPosition, LBlock *block, size_t lirIndex)
{
    
    MDefinition *operand = phi->getOperand(inputPosition);
    LPhi *type = block->getPhi(lirIndex + VREG_TYPE_OFFSET);
    LPhi *payload = block->getPhi(lirIndex + VREG_DATA_OFFSET);
    type->setOperand(inputPosition, LUse(operand->id() + VREG_TYPE_OFFSET, LUse::ANY));
    payload->setOperand(inputPosition, LUse(VirtualRegisterOfPayload(operand), LUse::ANY));
}

bool
LIRGeneratorARM::lowerForShift(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir, MDefinition *lhs, MDefinition *rhs)
{

    ins->setOperand(0, useRegister(lhs));
    
    if (rhs->isConstant()) {
        ins->setOperand(1, useOrConstant(rhs));
    } else {
        
        
        
        LBitOp *LAnd = new LBitOp(JSOP_BITAND);
        LAnd->setOperand(0, useRegister(rhs));
        LAnd->setOperand(1, LConstantIndex::FromIndex(0x1f));
        LAnd->setDef(0, LDefinition(LDefinition::TypeFrom(mir->type()), LDefinition::DEFAULT));
        uint32 vreg = getVirtualRegister();
        if (vreg >= MAX_VIRTUAL_REGISTERS)
            return false;
        LAnd->getDef(0)->setVirtualRegister(vreg);
        if (!add(LAnd))
            return false;
        LUse policy = LUse(LUse::REGISTER);
        policy.setVirtualRegister(vreg);
        ins->setOperand(1, policy);
    }
    return define(ins, mir,
                  LDefinition(LDefinition::TypeFrom(mir->type()), LDefinition::DEFAULT));
}

bool
LIRGeneratorARM::lowerDivI(MDiv *div)
{
    LDivI *lir = new LDivI(useFixed(div->lhs(), r0), useFixed(div->rhs(), r1),
                           tempFixed(r2), tempFixed(r3));
    
    return define(lir, div,
                  LDefinition(LDefinition::TypeFrom(div->type()), LDefinition::DEFAULT))
    && assignSnapshot(lir);
}

bool
LIRGeneratorARM::visitTableSwitch(MTableSwitch *tableswitch)
{
    MDefinition *opd = tableswitch->getOperand(0);

    
    JS_ASSERT(tableswitch->numSuccessors() > 0);

    
    if (tableswitch->numSuccessors() == 1)
        return add(new LGoto(tableswitch->getDefault()));

    
    if (opd->type() != MIRType_Int32 && opd->type() != MIRType_Double)
        return add(new LGoto(tableswitch->getDefault()));

    
    
    LAllocation index;
    LDefinition tempInt;
    if (opd->type() == MIRType_Int32) {
        index = useCopy(opd);
        tempInt = LDefinition::BogusTemp();
    } else {
        index = useRegister(opd);
        tempInt = temp(LDefinition::INTEGER);
    }
    return add(new LTableSwitch(index, tempInt, tableswitch));
}

bool
LIRGeneratorARM::visitGuardShape(MGuardShape *ins)
{
    LDefinition tempInt = temp(LDefinition::INTEGER);
    LGuardShape *guard = new LGuardShape(useRegister(ins->obj()), tempInt);
    return assignSnapshot(guard) && add(guard, ins);
}
