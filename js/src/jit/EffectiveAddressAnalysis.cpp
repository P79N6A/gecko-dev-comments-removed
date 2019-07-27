





#include "jit/EffectiveAddressAnalysis.h"
#include "jit/MIR.h"
#include "jit/MIRGraph.h"

using namespace js;
using namespace jit;

static void
AnalyzeLsh(TempAllocator& alloc, MLsh* lsh)
{
    if (lsh->specialization() != MIRType_Int32)
        return;

    if (lsh->isRecoveredOnBailout())
        return;

    MDefinition* index = lsh->lhs();
    MOZ_ASSERT(index->type() == MIRType_Int32);

    MDefinition* shift = lsh->rhs();
    if (!shift->isConstantValue())
        return;

    Value shiftValue = shift->constantValue();
    if (!shiftValue.isInt32() || !IsShiftInScaleRange(shiftValue.toInt32()))
        return;

    Scale scale = ShiftToScale(shiftValue.toInt32());

    int32_t displacement = 0;
    MInstruction* last = lsh;
    MDefinition* base = nullptr;
    while (true) {
        if (!last->hasOneUse())
            break;

        MUseIterator use = last->usesBegin();
        if (!use->consumer()->isDefinition() || !use->consumer()->toDefinition()->isAdd())
            break;

        MAdd* add = use->consumer()->toDefinition()->toAdd();
        if (add->specialization() != MIRType_Int32 || !add->isTruncated())
            break;

        MDefinition* other = add->getOperand(1 - add->indexOf(*use));

        if (other->isConstantValue()) {
            displacement += other->constantValue().toInt32();
        } else {
            if (base)
                break;
            base = other;
        }

        last = add;
        if (last->isRecoveredOnBailout())
            return;
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

        MBitAnd* bitAnd = use->consumer()->toDefinition()->toBitAnd();
        if (bitAnd->isRecoveredOnBailout())
            return;

        MDefinition* other = bitAnd->getOperand(1 - bitAnd->indexOf(*use));
        if (!other->isConstantValue() || !other->constantValue().isInt32())
            return;

        uint32_t bitsClearedByShift = elemSize - 1;
        uint32_t bitsClearedByMask = ~uint32_t(other->constantValue().toInt32());
        if ((bitsClearedByShift & bitsClearedByMask) != bitsClearedByMask)
            return;

        bitAnd->replaceAllUsesWith(last);
        return;
    }

    if (base->isRecoveredOnBailout())
        return;

    MEffectiveAddress* eaddr = MEffectiveAddress::New(alloc, base, index, scale, displacement);
    last->replaceAllUsesWith(eaddr);
    last->block()->insertAfter(last, eaddr);
}

template<typename MAsmJSHeapAccessType>
bool
EffectiveAddressAnalysis::tryAddDisplacement(MAsmJSHeapAccessType* ins, int32_t o)
{
    
    
    
    MOZ_ASSERT(ins->offset() >= 0);
    int32_t newOffset = uint32_t(ins->offset()) + o;
    if (newOffset < 0)
        return false;

    
    
    int32_t newEnd = uint32_t(newOffset) + ins->byteSize();
    if (newEnd < 0)
        return false;
    MOZ_ASSERT(uint32_t(newEnd) >= uint32_t(newOffset));

    
    
    size_t range = mir_->foldableOffsetRange(ins);
    if (size_t(newEnd) > range)
        return false;

    
    ins->setOffset(newOffset);
    return true;
}

template<typename MAsmJSHeapAccessType>
void
EffectiveAddressAnalysis::analyzeAsmHeapAccess(MAsmJSHeapAccessType* ins)
{
    MDefinition* ptr = ins->ptr();

    if (ptr->isConstantValue()) {
        
        
        
        
        
        int32_t imm = ptr->constantValue().toInt32();
        if (imm != 0 && tryAddDisplacement(ins, imm)) {
            MInstruction* zero = MConstant::New(graph_.alloc(), Int32Value(0));
            ins->block()->insertBefore(ins, zero);
            ins->replacePtr(zero);
        }
    } else if (ptr->isAdd()) {
        
        
        
        MDefinition* op0 = ptr->toAdd()->getOperand(0);
        MDefinition* op1 = ptr->toAdd()->getOperand(1);
        if (op0->isConstantValue())
            mozilla::Swap(op0, op1);
        if (op1->isConstantValue()) {
            int32_t imm = op1->constantValue().toInt32();
            if (tryAddDisplacement(ins, imm))
                ins->replacePtr(op0);
        }
    }
}















bool
EffectiveAddressAnalysis::analyze()
{
    for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
        for (MInstructionIterator i = block->begin(); i != block->end(); i++) {
            
            
            
            if (i->isLsh())
                AnalyzeLsh(graph_.alloc(), i->toLsh());
            else if (i->isAsmJSLoadHeap())
                analyzeAsmHeapAccess(i->toAsmJSLoadHeap());
            else if (i->isAsmJSStoreHeap())
                analyzeAsmHeapAccess(i->toAsmJSStoreHeap());
        }
    }
    return true;
}
