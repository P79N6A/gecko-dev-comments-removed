








































#include "ion/LIR.h"
#include "ion/MIR.h"
#include "ion/MIRGraph.h"
#include "Lowering-shared.h"
#include "Lowering-shared-inl.h"

using namespace js;
using namespace ion;

bool
LIRGeneratorShared::visitConstant(MConstant *ins)
{
    const Value &v = ins->value();
    switch (ins->type()) {
      case MIRType_Boolean:
        return define(new LInteger(v.toBoolean()), ins);
      case MIRType_Int32:
        return define(new LInteger(v.toInt32()), ins);
      case MIRType_String:
        return define(new LPointer(v.toString()), ins);
      case MIRType_Object:
        return define(new LPointer(&v.toObject()), ins);
      case MIRType_Magic:
      case MIRType_ArgObj:
        return define(new LInteger(v.whyMagic()), ins);
      default:
        
        
        JS_NOT_REACHED("unexpected constant type");
        return false;
    }
    return true;
}

bool
LIRGeneratorShared::defineTypedPhi(MPhi *phi, size_t lirIndex)
{
    LPhi *lir = current->getPhi(lirIndex);

    uint32 vreg = getVirtualRegister();
    if (vreg >= MAX_VIRTUAL_REGISTERS)
        return false;

    phi->setVirtualRegister(vreg);
    lir->setDef(0, LDefinition(vreg, LDefinition::TypeFrom(phi->type())));
    annotate(lir);
    return true;
}

void
LIRGeneratorShared::lowerTypedPhiInput(MPhi *phi, uint32 inputPosition, LBlock *block, size_t lirIndex)
{
    MDefinition *operand = phi->getOperand(inputPosition);
    LPhi *lir = block->getPhi(lirIndex);
    lir->setOperand(inputPosition, LUse(operand->virtualRegister(), LUse::ANY));
}

#ifdef JS_NUNBOX32
LSnapshot *
LIRGeneratorShared::buildSnapshot(LInstruction *ins, MResumePoint *rp, BailoutKind kind)
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

            if (ins->isPassArg())
                ins = ins->toPassArg()->getArgument();
            JS_ASSERT(!ins->isPassArg());

            
            JS_ASSERT_IF(ins->isUnused(), !ins->isGuard());

            
            
            
            
            
            if (ins->isConstant() || ins->isUnused()) {
                *type = LConstantIndex::Bogus();
                *payload = LConstantIndex::Bogus();
            } else if (ins->type() != MIRType_Value) {
                *type = LConstantIndex::Bogus();
                *payload = use(ins, LUse::KEEPALIVE);
            } else {
                if (!ensureDefined(ins))
                    return NULL;
                *type = useType(ins, LUse::KEEPALIVE);
                *payload = usePayload(ins, LUse::KEEPALIVE);
            }
        }
    }

    return snapshot;
}

#elif JS_PUNBOX64

LSnapshot *
LIRGeneratorShared::buildSnapshot(LInstruction *ins, MResumePoint *rp, BailoutKind kind)
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
            MDefinition *def = mir->getOperand(j);

            if (def->isPassArg())
                def = def->toPassArg()->getArgument();

            LAllocation *a = snapshot->getEntry(i);

            if (def->isUnused()) {
                *a = LConstantIndex::Bogus();
                continue;
            }

            *a = useKeepaliveOrConstant(def);
        }
    }

    return snapshot;
}
#endif

bool
LIRGeneratorShared::assignSnapshot(LInstruction *ins, BailoutKind kind)
{
    
    
    JS_ASSERT(ins->id() == 0);

    LSnapshot *snapshot = buildSnapshot(ins, lastResumePoint_, kind);
    if (!snapshot)
        return false;

    ins->assignSnapshot(snapshot);
    return true;
}

bool
LIRGeneratorShared::assignSafepoint(LInstruction *ins, MInstruction *mir)
{
    JS_ASSERT(!osiPoint_);
    JS_ASSERT(!ins->safepoint());

    ins->initSafepoint();

    MResumePoint *mrp = mir->resumePoint() ? mir->resumePoint() : lastResumePoint_;
    LSnapshot *postSnapshot = buildSnapshot(ins, mrp, Bailout_Normal);
    if (!postSnapshot)
        return false;

    osiPoint_ = new LOsiPoint(ins->safepoint(), postSnapshot);

    return lirGraph_.noteNeedsSafepoint(ins);
}

