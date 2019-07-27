





#include "jit/LoopUnroller.h"

#include "jit/MIRGraph.h"

using namespace js;
using namespace js::jit;

using mozilla::ArrayLength;

namespace {

struct LoopUnroller
{
    typedef HashMap<MDefinition*, MDefinition*,
                    PointerHasher<MDefinition*, 2>, SystemAllocPolicy> DefinitionMap;

    explicit LoopUnroller(MIRGraph& graph)
      : graph(graph), alloc(graph.alloc())
    {}

    MIRGraph& graph;
    TempAllocator& alloc;

    
    MBasicBlock* header;
    MBasicBlock* backedge;

    
    MBasicBlock* unrolledHeader;
    MBasicBlock* unrolledBackedge;

    
    
    
    MBasicBlock* oldPreheader;
    MBasicBlock* newPreheader;

    
    DefinitionMap unrolledDefinitions;

    MDefinition* getReplacementDefinition(MDefinition* def);
    MResumePoint* makeReplacementResumePoint(MBasicBlock* block, MResumePoint* rp);
    void makeReplacementInstruction(MInstruction* ins);

    void go(LoopIterationBound* bound);
};

} 

MDefinition*
LoopUnroller::getReplacementDefinition(MDefinition* def)
{
    if (def->block()->id() < header->id()) {
        
        return def;
    }

    DefinitionMap::Ptr p = unrolledDefinitions.lookup(def);
    if (!p) {
        
        
        
        MOZ_ASSERT(def->isConstant());

        MConstant* constant = MConstant::New(alloc, def->toConstant()->value());
        oldPreheader->insertBefore(*oldPreheader->begin(), constant);
        return constant;
    }

    return p->value();
}

void
LoopUnroller::makeReplacementInstruction(MInstruction* ins)
{
    MDefinitionVector inputs(alloc);
    for (size_t i = 0; i < ins->numOperands(); i++) {
        MDefinition* old = ins->getOperand(i);
        MDefinition* replacement = getReplacementDefinition(old);
        if (!inputs.append(replacement))
            CrashAtUnhandlableOOM("LoopUnroller::makeReplacementDefinition");
    }

    MInstruction* clone = ins->clone(alloc, inputs);

    unrolledBackedge->add(clone);

    if (!unrolledDefinitions.putNew(ins, clone))
        CrashAtUnhandlableOOM("LoopUnroller::makeReplacementDefinition");

    if (MResumePoint* old = ins->resumePoint()) {
        MResumePoint* rp = makeReplacementResumePoint(unrolledBackedge, old);
        clone->setResumePoint(rp);
    }
}

MResumePoint*
LoopUnroller::makeReplacementResumePoint(MBasicBlock* block, MResumePoint* rp)
{
    MDefinitionVector inputs(alloc);
    for (size_t i = 0; i < rp->numOperands(); i++) {
        MDefinition* old = rp->getOperand(i);
        MDefinition* replacement = old->isUnused() ? old : getReplacementDefinition(old);
        if (!inputs.append(replacement))
            CrashAtUnhandlableOOM("LoopUnroller::makeReplacementResumePoint");
    }

    MResumePoint* clone = MResumePoint::New(alloc, block, rp, inputs);
    if (!clone)
        CrashAtUnhandlableOOM("LoopUnroller::makeReplacementResumePoint");

    return clone;
}

void
LoopUnroller::go(LoopIterationBound* bound)
{
    
    static const size_t UnrollCount = 10;

    JitSpew(JitSpew_Unrolling, "Attempting to unroll loop");

    header = bound->header;

    
    if (!header->isLoopHeader())
        return;

    backedge = header->backedge();
    oldPreheader = header->loopPredecessor();

    MOZ_ASSERT(oldPreheader->numSuccessors() == 1);

    
    
    MTest* test = bound->test;
    if (header->lastIns() != test)
        return;
    if (test->ifTrue() == backedge) {
        if (test->ifFalse()->id() <= backedge->id())
            return;
    } else if (test->ifFalse() == backedge) {
        if (test->ifTrue()->id() <= backedge->id())
            return;
    } else {
        return;
    }
    if (backedge->numPredecessors() != 1 || backedge->numSuccessors() != 1)
        return;
    MOZ_ASSERT(backedge->phisEmpty());

    MBasicBlock* bodyBlocks[] = { header, backedge };

    
    for (size_t i = 0; i < ArrayLength(bodyBlocks); i++) {
        MBasicBlock* block = bodyBlocks[i];
        for (MInstructionIterator iter(block->begin()); iter != block->end(); iter++) {
            MInstruction* ins = *iter;
            if (ins->canClone())
                continue;
            if (ins->isTest() || ins->isGoto() || ins->isInterruptCheck())
                continue;
#ifdef DEBUG
            JitSpew(JitSpew_Unrolling, "Aborting: can't clone instruction %s", ins->opName());
#endif
            return;
        }
    }

    
    
    
    
    LinearSum remainingIterationsInequality(bound->boundSum);
    if (!remainingIterationsInequality.add(bound->currentSum, -1))
        return;
    if (!remainingIterationsInequality.add(-int32_t(UnrollCount)))
        return;

    
    
    for (size_t i = 0; i < remainingIterationsInequality.numTerms(); i++) {
        MDefinition* def = remainingIterationsInequality.term(i).term;
        if (def->isDiscarded())
            return;
        if (def->block()->id() < header->id())
            continue;
        if (def->block() == header && def->isPhi())
            continue;
        return;
    }

    

    JitSpew(JitSpew_Unrolling, "Unrolling loop");

    
    
    CompileInfo& info = oldPreheader->info();
    if (header->trackedPc()) {
        unrolledHeader =
            MBasicBlock::New(graph, nullptr, info,
                             oldPreheader, header->trackedSite(), MBasicBlock::LOOP_HEADER);
        unrolledBackedge =
            MBasicBlock::New(graph, nullptr, info,
                             unrolledHeader, backedge->trackedSite(), MBasicBlock::NORMAL);
        newPreheader =
            MBasicBlock::New(graph, nullptr, info,
                             unrolledHeader, oldPreheader->trackedSite(), MBasicBlock::NORMAL);
    } else {
        unrolledHeader = MBasicBlock::NewAsmJS(graph, info, oldPreheader, MBasicBlock::LOOP_HEADER);
        unrolledBackedge = MBasicBlock::NewAsmJS(graph, info, unrolledHeader, MBasicBlock::NORMAL);
        newPreheader = MBasicBlock::NewAsmJS(graph, info, unrolledHeader, MBasicBlock::NORMAL);
    }

    unrolledHeader->discardAllResumePoints();
    unrolledBackedge->discardAllResumePoints();
    newPreheader->discardAllResumePoints();

    
    graph.insertBlockAfter(oldPreheader, unrolledHeader);
    graph.insertBlockAfter(unrolledHeader, unrolledBackedge);
    graph.insertBlockAfter(unrolledBackedge, newPreheader);
    graph.renumberBlocksAfter(oldPreheader);

    if (!unrolledDefinitions.init())
        CrashAtUnhandlableOOM("LoopUnroller::go");

    
    
    MOZ_ASSERT(header->getPredecessor(0) == oldPreheader);
    for (MPhiIterator iter(header->phisBegin()); iter != header->phisEnd(); iter++) {
        MPhi* old = *iter;
        MOZ_ASSERT(old->numOperands() == 2);
        MPhi* phi = MPhi::New(alloc);
        phi->setResultType(old->type());
        phi->setResultTypeSet(old->resultTypeSet());
        phi->setRange(old->range());

        unrolledHeader->addPhi(phi);

        if (!phi->reserveLength(2))
            CrashAtUnhandlableOOM("LoopUnroller::go");

        
        
        phi->addInput(old->getOperand(0));

        
        old->replaceOperand(0, phi);

        if (!unrolledDefinitions.putNew(old, phi))
            CrashAtUnhandlableOOM("LoopUnroller::go");
    }

    
    
    MResumePoint* headerResumePoint = header->entryResumePoint();
    if (headerResumePoint) {
        MResumePoint* rp = makeReplacementResumePoint(unrolledHeader, headerResumePoint);
        unrolledHeader->setEntryResumePoint(rp);

        
        unrolledHeader->add(MInterruptCheck::New(alloc));
    }

    
    for (size_t i = 0; i < remainingIterationsInequality.numTerms(); i++) {
        MDefinition* def = remainingIterationsInequality.term(i).term;
        MDefinition* replacement = getReplacementDefinition(def);
        remainingIterationsInequality.replaceTerm(i, replacement);
    }
    MCompare* compare = ConvertLinearInequality(alloc, unrolledHeader, remainingIterationsInequality);
    MTest* unrolledTest = MTest::New(alloc, compare, unrolledBackedge, newPreheader);
    unrolledHeader->end(unrolledTest);

    
    
    
    if (headerResumePoint) {
        MResumePoint* rp = makeReplacementResumePoint(unrolledBackedge, headerResumePoint);
        unrolledBackedge->setEntryResumePoint(rp);
    }

    
    
    if (headerResumePoint) {
        MResumePoint* rp = makeReplacementResumePoint(newPreheader, headerResumePoint);
        newPreheader->setEntryResumePoint(rp);
    }

    
    MOZ_ASSERT(UnrollCount > 1);
    size_t unrollIndex = 0;
    while (true) {
        
        for (size_t i = 0; i < ArrayLength(bodyBlocks); i++) {
            MBasicBlock* block = bodyBlocks[i];
            for (MInstructionIterator iter(block->begin()); iter != block->end(); iter++) {
                MInstruction* ins = *iter;
                if (ins->canClone()) {
                    makeReplacementInstruction(*iter);
                } else {
                    
                    MOZ_ASSERT(ins->isTest() || ins->isGoto() || ins->isInterruptCheck());
                }
            }
        }

        
        
        MDefinitionVector phiValues(alloc);
        MOZ_ASSERT(header->getPredecessor(1) == backedge);
        for (MPhiIterator iter(header->phisBegin()); iter != header->phisEnd(); iter++) {
            MPhi* old = *iter;
            MDefinition* oldInput = old->getOperand(1);
            if (!phiValues.append(getReplacementDefinition(oldInput)))
                CrashAtUnhandlableOOM("LoopUnroller::go");
        }

        unrolledDefinitions.clear();

        if (unrollIndex == UnrollCount - 1) {
            
            
            size_t phiIndex = 0;
            for (MPhiIterator iter(unrolledHeader->phisBegin()); iter != unrolledHeader->phisEnd(); iter++) {
                MPhi* phi = *iter;
                phi->addInput(phiValues[phiIndex++]);
            }
            MOZ_ASSERT(phiIndex == phiValues.length());
            break;
        }

        
        size_t phiIndex = 0;
        for (MPhiIterator iter(header->phisBegin()); iter != header->phisEnd(); iter++) {
            MPhi* old = *iter;
            if (!unrolledDefinitions.putNew(old, phiValues[phiIndex++]))
                CrashAtUnhandlableOOM("LoopUnroller::go");
        }
        MOZ_ASSERT(phiIndex == phiValues.length());

        unrollIndex++;
    }

    MGoto* backedgeJump = MGoto::New(alloc, unrolledHeader);
    unrolledBackedge->end(backedgeJump);

    
    MOZ_ASSERT(oldPreheader->lastIns()->isGoto());
    oldPreheader->discardLastIns();
    oldPreheader->end(MGoto::New(alloc, unrolledHeader));

    
    newPreheader->end(MGoto::New(alloc, header));

    
    if (!unrolledHeader->addPredecessorWithoutPhis(unrolledBackedge))
        CrashAtUnhandlableOOM("LoopUnroller::go");
    header->replacePredecessor(oldPreheader, newPreheader);
    oldPreheader->setSuccessorWithPhis(unrolledHeader, 0);
    newPreheader->setSuccessorWithPhis(header, 0);
    unrolledBackedge->setSuccessorWithPhis(unrolledHeader, 1);
}

bool
jit::UnrollLoops(MIRGraph& graph, const LoopIterationBoundVector& bounds)
{
    if (bounds.empty())
        return true;

    for (size_t i = 0; i < bounds.length(); i++) {
        LoopUnroller unroller(graph);
        unroller.go(bounds[i]);
    }

    
    
    
    ClearDominatorTree(graph);
    if (!BuildDominatorTree(graph))
        return false;

    return true;
}
