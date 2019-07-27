





#include "jit/shared/Lowering-shared-inl.h"

#include "jit/LIR.h"
#include "jit/MIR.h"

#include "vm/Symbol.h"

using namespace js;
using namespace jit;

bool
LIRGeneratorShared::ShouldReorderCommutative(MDefinition *lhs, MDefinition *rhs, MInstruction *ins)
{
    
    MOZ_ASSERT(lhs->hasDefUses());
    MOZ_ASSERT(rhs->hasDefUses());

    
    if (rhs->isConstant())
        return false;
    if (lhs->isConstant())
        return true;

    
    
    
    
    bool rhsSingleUse = rhs->hasOneDefUse();
    bool lhsSingleUse = lhs->hasOneDefUse();
    if (rhsSingleUse) {
        if (!lhsSingleUse)
            return true;
    } else {
        if (lhsSingleUse)
            return false;
    }

    
    
    
    
    
    
    
    if (rhsSingleUse &&
        rhs->isPhi() &&
        rhs->block()->isLoopHeader() &&
        ins == rhs->toPhi()->getLoopBackedgeOperand())
    {
        return true;
    }

    return false;
}

void
LIRGeneratorShared::ReorderCommutative(MDefinition **lhsp, MDefinition **rhsp, MInstruction *ins)
{
    MDefinition *lhs = *lhsp;
    MDefinition *rhs = *rhsp;

    if (ShouldReorderCommutative(lhs, rhs, ins)) {
        *rhsp = lhs;
        *lhsp = rhs;
    }
}

void
LIRGeneratorShared::visitConstant(MConstant *ins)
{
    const Value &v = ins->value();
    switch (ins->type()) {
      case MIRType_Boolean:
        define(new(alloc()) LInteger(v.toBoolean()), ins);
        break;
      case MIRType_Int32:
        define(new(alloc()) LInteger(v.toInt32()), ins);
        break;
      case MIRType_String:
        define(new(alloc()) LPointer(v.toString()), ins);
        break;
      case MIRType_Symbol:
        define(new(alloc()) LPointer(v.toSymbol()), ins);
        break;
      case MIRType_Object:
        define(new(alloc()) LPointer(&v.toObject()), ins);
        break;
      default:
        
        
        MOZ_CRASH("unexpected constant type");
    }
}

void
LIRGeneratorShared::defineTypedPhi(MPhi *phi, size_t lirIndex)
{
    LPhi *lir = current->getPhi(lirIndex);

    uint32_t vreg = getVirtualRegister();

    phi->setVirtualRegister(vreg);
    lir->setDef(0, LDefinition(vreg, LDefinition::TypeFrom(phi->type())));
    annotate(lir);
}

void
LIRGeneratorShared::lowerTypedPhiInput(MPhi *phi, uint32_t inputPosition, LBlock *block, size_t lirIndex)
{
    MDefinition *operand = phi->getOperand(inputPosition);
    LPhi *lir = block->getPhi(lirIndex);
    lir->setOperand(inputPosition, LUse(operand->virtualRegister(), LUse::ANY));
}

LRecoverInfo *
LIRGeneratorShared::getRecoverInfo(MResumePoint *rp)
{
    if (cachedRecoverInfo_ && cachedRecoverInfo_->mir() == rp)
        return cachedRecoverInfo_;

    LRecoverInfo *recoverInfo = LRecoverInfo::New(gen, rp);
    if (!recoverInfo)
        return nullptr;

    cachedRecoverInfo_ = recoverInfo;
    return recoverInfo;
}

#ifdef DEBUG
bool
LRecoverInfo::OperandIter::canOptimizeOutIfUnused()
{
    MDefinition *ins = **this;

    
    
    
    if ((ins->isUnused() || ins->type() == MIRType_MagicOptimizedOut) &&
        (*it_)->isResumePoint())
    {
        return !(*it_)->toResumePoint()->isObservableOperand(op_);
    }

    return true;
}
#endif

#ifdef JS_NUNBOX32
LSnapshot *
LIRGeneratorShared::buildSnapshot(LInstruction *ins, MResumePoint *rp, BailoutKind kind)
{
    LRecoverInfo *recoverInfo = getRecoverInfo(rp);
    if (!recoverInfo)
        return nullptr;

    LSnapshot *snapshot = LSnapshot::New(gen, recoverInfo, kind);
    if (!snapshot)
        return nullptr;

    size_t index = 0;
    for (LRecoverInfo::OperandIter it(recoverInfo); !it; ++it) {
        
        MOZ_ASSERT(it.canOptimizeOutIfUnused());

        MDefinition *ins = *it;

        if (ins->isRecoveredOnBailout())
            continue;

        LAllocation *type = snapshot->typeOfSlot(index);
        LAllocation *payload = snapshot->payloadOfSlot(index);
        ++index;

        if (ins->isBox())
            ins = ins->toBox()->getOperand(0);

        
        MOZ_ASSERT_IF(ins->isUnused(), !ins->isGuard());

        
        
        
        MOZ_ASSERT_IF(!ins->isConstant(), !ins->isEmittedAtUses());

        
        
        
        
        
        if (ins->isConstant() || ins->isUnused()) {
            *type = LAllocation();
            *payload = LAllocation();
        } else if (ins->type() != MIRType_Value) {
            *type = LAllocation();
            *payload = use(ins, LUse(LUse::KEEPALIVE));
        } else {
            *type = useType(ins, LUse::KEEPALIVE);
            *payload = usePayload(ins, LUse::KEEPALIVE);
        }
    }

    return snapshot;
}

#elif JS_PUNBOX64

LSnapshot *
LIRGeneratorShared::buildSnapshot(LInstruction *ins, MResumePoint *rp, BailoutKind kind)
{
    LRecoverInfo *recoverInfo = getRecoverInfo(rp);
    if (!recoverInfo)
        return nullptr;

    LSnapshot *snapshot = LSnapshot::New(gen, recoverInfo, kind);
    if (!snapshot)
        return nullptr;

    size_t index = 0;
    for (LRecoverInfo::OperandIter it(recoverInfo); !it; ++it) {
        
        MOZ_ASSERT(it.canOptimizeOutIfUnused());

        MDefinition *def = *it;

        if (def->isRecoveredOnBailout())
            continue;

        if (def->isBox())
            def = def->toBox()->getOperand(0);

        
        MOZ_ASSERT_IF(def->isUnused(), !def->isGuard());

        
        
        
        MOZ_ASSERT_IF(!def->isConstant(), !def->isEmittedAtUses());

        LAllocation *a = snapshot->getEntry(index++);

        if (def->isUnused()) {
            *a = LAllocation();
            continue;
        }

        *a = useKeepaliveOrConstant(def);
    }

    return snapshot;
}
#endif

void
LIRGeneratorShared::assignSnapshot(LInstruction *ins, BailoutKind kind)
{
    
    
    MOZ_ASSERT(ins->id() == 0);

    LSnapshot *snapshot = buildSnapshot(ins, lastResumePoint_, kind);
    if (snapshot)
        ins->assignSnapshot(snapshot);
    else
        gen->abort("buildSnapshot failed");
}

void
LIRGeneratorShared::assignSafepoint(LInstruction *ins, MInstruction *mir, BailoutKind kind)
{
    MOZ_ASSERT(!osiPoint_);
    MOZ_ASSERT(!ins->safepoint());

    ins->initSafepoint(alloc());

    MResumePoint *mrp = mir->resumePoint() ? mir->resumePoint() : lastResumePoint_;
    LSnapshot *postSnapshot = buildSnapshot(ins, mrp, kind);
    if (!postSnapshot) {
        gen->abort("buildSnapshot failed");
        return;
    }

    osiPoint_ = new(alloc()) LOsiPoint(ins->safepoint(), postSnapshot);

    if (!lirGraph_.noteNeedsSafepoint(ins))
        gen->abort("noteNeedsSafepoint failed");
}

