





#include "jit/EagerSimdUnbox.h"

#include "jit/MIR.h"
#include "jit/MIRGenerator.h"
#include "jit/MIRGraph.h"

namespace js {
namespace jit {

static SimdTypeDescr::Type
MIRTypeToSimdTypeDescr(MIRType type)
{
    MOZ_ASSERT(IsSimdType(type));
    switch (type) {
      case MIRType_Float32x4:   return SimdTypeDescr::TYPE_FLOAT32;
      case MIRType_Int32x4:     return SimdTypeDescr::TYPE_INT32;
      default:                  break;
    }
    MOZ_CRASH("unexpected MIRType");
}



static bool
CanUnboxSimdPhi(const JitCompartment *jitCompartment, MPhi *phi, MIRType unboxType)
{
    MOZ_ASSERT(phi->type() == MIRType_Object);

    
    
    
    if (!jitCompartment->maybeGetSimdTemplateObjectFor(MIRTypeToSimdTypeDescr(unboxType)))
        return false;

    MResumePoint *entry = phi->block()->entryResumePoint();
    for (MUseIterator i(phi->usesBegin()), e(phi->usesEnd()); i != e; i++) {
        
        
        if ((*i)->consumer() == entry && !entry->isRecoverableOperand(*i))
            return false;

        if (!(*i)->consumer()->isDefinition())
            continue;

        MDefinition *def = (*i)->consumer()->toDefinition();
        if (def->isSimdUnbox() && def->toSimdUnbox()->type() != unboxType)
            return false;
    }

    return true;
}

static void
UnboxSimdPhi(const JitCompartment *jitCompartment, MIRGraph &graph, MPhi *phi, MIRType unboxType)
{
    TempAllocator &alloc = graph.alloc();

    
    for (size_t i = 0, e = phi->numOperands(); i < e; i++) {
        MDefinition *op = phi->getOperand(i);
        MSimdUnbox *unbox = MSimdUnbox::New(alloc, op, unboxType);
        op->block()->insertAtEnd(unbox);
        phi->replaceOperand(i, unbox);
    }

    
    phi->setResultType(unboxType);

    MBasicBlock *phiBlock = phi->block();
    MInstruction *atRecover = phiBlock->safeInsertTop(nullptr, MBasicBlock::IgnoreRecover);
    MInstruction *at = phiBlock->safeInsertTop(atRecover);

    
    MUseIterator i(phi->usesBegin()), e(phi->usesEnd());

    
    JSObject *templateObject =
        jitCompartment->maybeGetSimdTemplateObjectFor(MIRTypeToSimdTypeDescr(unboxType));
    InlineTypedObject *inlineTypedObject = &templateObject->as<InlineTypedObject>();
    MSimdBox *recoverBox = MSimdBox::New(alloc, nullptr, phi, inlineTypedObject, gc::DefaultHeap);
    recoverBox->setRecoveredOnBailout();
    phiBlock->insertBefore(atRecover, recoverBox);

    MSimdBox *box = nullptr;
    while (i != e) {
        MUse *use = *i++;
        MNode *ins = use->consumer();

        if ((ins->isDefinition() && ins->toDefinition()->isRecoveredOnBailout()) ||
            (ins->isResumePoint() && ins->toResumePoint()->isRecoverableOperand(use)))
        {
            use->replaceProducer(recoverBox);
            continue;
        }

        if (!box) {
            box = MSimdBox::New(alloc, nullptr, phi, inlineTypedObject, gc::DefaultHeap);
            phiBlock->insertBefore(at, box);
        }

        use->replaceProducer(box);
    }
}

bool
EagerSimdUnbox(MIRGenerator *mir, MIRGraph &graph)
{
    const JitCompartment *jitCompartment = GetJitContext()->compartment->jitCompartment();
    for (PostorderIterator block = graph.poBegin(); block != graph.poEnd(); block++) {
        if (mir->shouldCancel("Eager Simd Unbox"))
            return false;

        for (MInstructionReverseIterator ins = block->rbegin(); ins != block->rend(); ins++) {
            if (!ins->isSimdUnbox())
                continue;

            MSimdUnbox *unbox = ins->toSimdUnbox();
            if (!unbox->input()->isPhi())
                continue;

            MPhi *phi = unbox->input()->toPhi();
            if (!CanUnboxSimdPhi(jitCompartment, phi, unbox->type()))
                continue;

            UnboxSimdPhi(jitCompartment, graph, phi, unbox->type());
        }
    }

    return true;
}

} 
} 
