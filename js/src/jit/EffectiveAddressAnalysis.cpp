





#include "jit/EffectiveAddressAnalysis.h"
#include "jit/MIR.h"
#include "jit/MIRGraph.h"

using namespace js;
using namespace jit;

static void
AnalyzeLsh(TempAllocator &alloc, MLsh *lsh)
{
    if (lsh->specialization() != MIRType_Int32)
        return;

    MDefinition *index = lsh->lhs();
    JS_ASSERT(index->type() == MIRType_Int32);

    MDefinition *shift = lsh->rhs();
    if (!shift->isConstant())
        return;

    Value shiftValue = shift->toConstant()->value();
    if (!shiftValue.isInt32() || !IsShiftInScaleRange(shiftValue.toInt32()))
        return;

    Scale scale = ShiftToScale(shiftValue.toInt32());

    int32_t displacement = 0;
    MInstruction *last = lsh;
    MDefinition *base = nullptr;
    while (true) {
        if (!last->hasOneUse())
            break;

        MUseIterator use = last->usesBegin();
        if (!use->consumer()->isDefinition() || !use->consumer()->toDefinition()->isAdd())
            break;

        MAdd *add = use->consumer()->toDefinition()->toAdd();
        if (add->specialization() != MIRType_Int32 || !add->isTruncated())
            break;

        MDefinition *other = add->getOperand(1 - add->indexOf(*use));

        if (other->isConstant()) {
            displacement += other->toConstant()->value().toInt32();
        } else {
            if (base)
                break;
            base = other;
        }

        last = add;
    }

    if (!base) {
        uint32_t elemSize = 1 << ScaleToShift(scale);
        if (displacement % elemSize != 0)
            return;

        if (!last->hasOneUse())
            return;

        MUseIterator use = last->usesBegin();
        if (!use->consumer()->isDefinition() || !use->consumer()->toDefinition()->isBitAnd())
            return;

        MBitAnd *bitAnd = use->consumer()->toDefinition()->toBitAnd();
        MDefinition *other = bitAnd->getOperand(1 - bitAnd->indexOf(*use));
        if (!other->isConstant() || !other->toConstant()->value().isInt32())
            return;

        uint32_t bitsClearedByShift = elemSize - 1;
        uint32_t bitsClearedByMask = ~uint32_t(other->toConstant()->value().toInt32());
        if ((bitsClearedByShift & bitsClearedByMask) != bitsClearedByMask)
            return;

        bitAnd->replaceAllUsesWith(last);
        return;
    }

    MEffectiveAddress *eaddr = MEffectiveAddress::New(alloc, base, index, scale, displacement);
    last->replaceAllUsesWith(eaddr);
    last->block()->insertAfter(last, eaddr);
}















bool
EffectiveAddressAnalysis::analyze()
{
    for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
        for (MInstructionIterator i = block->begin(); i != block->end(); i++) {
            if (i->isLsh())
                AnalyzeLsh(graph_.alloc(), i->toLsh());
        }
    }
    return true;
}
