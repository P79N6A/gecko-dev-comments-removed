








































#include "GreedyAllocator.h"
#include "IonAnalysis.h"
#include "IonSpewer.h"

using namespace js;
using namespace js::ion;

GreedyAllocator::GreedyAllocator(MIRGenerator *gen, LIRGraph &graph)
  : gen(gen),
    graph(graph)
{
}

void
GreedyAllocator::findDefinitionsInLIR(LInstruction *ins)
{
    for (size_t i = 0; i < ins->numDefs(); i++) {
        LDefinition *def = ins->getDef(i);
        JS_ASSERT(def->virtualRegister() < graph.numVirtualRegisters());

        if (def->policy() == LDefinition::PASSTHROUGH)
            continue;

        vars[def->virtualRegister()].define(def, ins);
    }

    for (size_t i = 0; i < ins->numTemps(); i++) {
        LDefinition *temp = ins->getTemp(i);
        JS_ASSERT(temp->virtualRegister() < graph.numVirtualRegisters());
        JS_ASSERT(temp->policy() != LDefinition::PASSTHROUGH);

        vars[temp->virtualRegister()].define(temp, ins);
    }
}

void
GreedyAllocator::findDefinitionsInBlock(LBlock *block)
{
    for (size_t i = 0; i < block->numPhis(); i++)
        findDefinitionsInLIR(block->getPhi(i));
    for (LInstructionIterator i = block->begin(); i != block->end(); i++)
        findDefinitionsInLIR(*i);
}

void
GreedyAllocator::findDefinitions()
{
    for (size_t i = 0; i < graph.numBlocks(); i++)
        findDefinitionsInBlock(graph.getBlock(i));
}

bool
GreedyAllocator::maybeEvict(AnyRegister reg)
{
    if (!state.free.has(reg))
        return evict(reg);
    return true;
}

static inline AnyRegister
GetFixedRegister(LDefinition *def, LUse *use)
{
    return def->type() == LDefinition::DOUBLE
           ? AnyRegister(FloatRegister::FromCode(use->registerCode()))
           : AnyRegister(Register::FromCode(use->registerCode()));
}

static inline AnyRegister
GetAllocatedRegister(const LAllocation *a)
{
    JS_ASSERT(a->isRegister());
    return a->isFloatReg()
           ? AnyRegister(a->toFloatReg()->reg())
           : AnyRegister(a->toGeneralReg()->reg());
}

static inline AnyRegister
GetPresetRegister(const LDefinition *def)
{
    JS_ASSERT(def->policy() == LDefinition::PRESET);
    return GetAllocatedRegister(def->output());
}

bool
GreedyAllocator::prescanDefinition(LDefinition *def)
{
    
    
    
    if (def->policy() == LDefinition::PASSTHROUGH)
        return true;

    VirtualRegister *vr = getVirtualRegister(def);

    
    killReg(vr);

    
    if (vr->hasRegister())
        disallowed.add(vr->reg());

    if (def->policy() == LDefinition::PRESET) {
        const LAllocation *a = def->output();
        if (a->isRegister()) {
            
            
            
            AnyRegister reg = GetPresetRegister(def);
            disallowed.addUnchecked(reg);
            if (!maybeEvict(reg))
                return false;
        }
    }
    return true;
}

bool
GreedyAllocator::prescanDefinitions(LInstruction *ins)
{
    for (size_t i = 0; i < ins->numDefs(); i++) {
        if (!prescanDefinition(ins->getDef(i)))
            return false;
    }
    for (size_t i = 0; i < ins->numTemps(); i++) {
        LDefinition *temp = ins->getTemp(i);
        if (temp->isBogusTemp())
            continue;
        if (!prescanDefinition(temp))
            return false;
    }
    return true;
}

bool
GreedyAllocator::prescanUses(LInstruction *ins)
{
    for (size_t i = 0; i < ins->numOperands(); i++) {
        LAllocation *a = ins->getOperand(i);
        if (!a->isUse()) {
            JS_ASSERT(a->isConstant());
            continue;
        }

        VirtualRegister *vr = getVirtualRegister(a->toUse());
        if (a->toUse()->policy() == LUse::FIXED)
            disallowed.add(GetFixedRegister(vr->def, a->toUse()));
        else if (vr->hasRegister())
            discouraged.addUnchecked(vr->reg());
    }
    return true;
}

static inline bool
IsNunbox(LDefinition::Type type)
{
#ifdef JS_NUNBOX32
    return type == LDefinition::TYPE || type == LDefinition::PAYLOAD;
#else
    return false;
#endif
}

uint32
GreedyAllocator::allocateSlotFor(VirtualRegister *vr)
{
    
    liveSlots_.pushBack(vr);
    vr->setOwnsStackSlot();

    if (IsNunbox(vr->type()))
        return stackSlotAllocator.allocateValueSlot();
    if (vr->isDouble())
        return stackSlotAllocator.allocateDoubleSlot();
    return stackSlotAllocator.allocateSlot();
}

void
GreedyAllocator::allocateStack(VirtualRegister *vr)
{
    if (vr->hasBackingStack())
        return;

    uint32 index;
#ifdef JS_NUNBOX32
    if (IsNunbox(vr->type())) {
        VirtualRegister *other = otherHalfOfNunbox(vr);
        unsigned stackSlot;
        if (!other->hasStackSlot())
            stackSlot = allocateSlotFor(vr);
        else
            stackSlot = BaseOfNunboxSlot(other->type(), other->stackSlot());
        index = stackSlot - OffsetOfNunboxSlot(vr->type());
    } else
#endif
    {
        index = allocateSlotFor(vr);
    }

    IonSpew(IonSpew_RegAlloc, "    assign vr%d := stack%d", vr->def->virtualRegister(), index);

    vr->setStackSlot(index);
}

#ifdef JS_NUNBOX32
GreedyAllocator::VirtualRegister *
GreedyAllocator::otherHalfOfNunbox(VirtualRegister *vreg)
{
    signed offset = OffsetToOtherHalfOfNunbox(vreg->type());
    VirtualRegister *other = &vars[vreg->def->virtualRegister() + offset];
    AssertTypesFormANunbox(vreg->type(), other->type());
    return other;
}
#endif

void
GreedyAllocator::freeStack(VirtualRegister *vr)
{
    if (vr->isDouble())
        stackSlotAllocator.freeDoubleSlot(vr->stackSlot());
    else
        stackSlotAllocator.freeSlot(vr->stackSlot());
}

bool
GreedyAllocator::allocate(LDefinition::Type type, Policy policy, AnyRegister *out)
{
    RegisterSet allowed = RegisterSet::Not(disallowed);
    RegisterSet free = allocatableRegs();
    RegisterSet tryme = RegisterSet::Intersect(free, RegisterSet::Not(discouraged));

    if (tryme.empty(type == LDefinition::DOUBLE)) {
        if (free.empty(type == LDefinition::DOUBLE)) {
            *out = allowed.takeAny(type == LDefinition::DOUBLE);
            if (!evict(*out))
                return false;
        } else {
            *out = free.takeAny(type == LDefinition::DOUBLE);
        }
    } else {
        *out = tryme.takeAny(type == LDefinition::DOUBLE);
    }

    if (policy != TEMPORARY)
        disallowed.add(*out);

    return true;
}

void
GreedyAllocator::freeReg(AnyRegister reg)
{
    state[reg] = NULL;
    state.free.add(reg);
}

void
GreedyAllocator::killReg(VirtualRegister *vr)
{
    if (vr->hasRegister()) {
        AnyRegister reg = vr->reg();
        JS_ASSERT(state[reg] == vr);

        IonSpew(IonSpew_RegAlloc, "    kill vr%d (%s)",
                vr->def->virtualRegister(), reg.name());
        freeReg(reg);
    }
}

void
GreedyAllocator::killStack(VirtualRegister *vr)
{
#if JS_NUNBOX32
    if (IsNunbox(vr->type()) && vr->ownsStackSlot()) {
        uint32 stackSlot = BaseOfNunboxSlot(vr->type(), vr->stackSlot());
        stackSlotAllocator.freeValueSlot(stackSlot);
    } else
#endif
    if (vr->hasStackSlot()) {
        IonSpew(IonSpew_RegAlloc, "    kill vr%d (stack:%d)",
                vr->def->virtualRegister(), vr->stackSlot_);
        freeStack(vr);
    }

    
    if (vr->ownsStackSlot())
        liveSlots_.remove(vr);
}

bool
GreedyAllocator::evict(AnyRegister reg)
{
    VirtualRegister *vr = state[reg];
    JS_ASSERT(vr->reg() == reg);

    
    allocateStack(vr);

    
    
    if (!restore(vr->backingStack(), reg))
        return false;

    freeReg(reg);
    vr->unsetRegister();
    return true;
}

void
GreedyAllocator::assign(VirtualRegister *vr, AnyRegister reg)
{
    JS_ASSERT(!state[reg]);
    IonSpew(IonSpew_RegAlloc, "    assign vr%d := %s", vr->def->virtualRegister(), reg.name());
    state[reg] = vr;
    vr->setRegister(reg);
    state.free.take(reg);
}

bool
GreedyAllocator::allocateReg(VirtualRegister *vreg)
{
    JS_ASSERT(!vreg->hasRegister());
    AnyRegister reg;
    if (!allocate(vreg->type(), DISALLOW, &reg))
        return false;
    assign(vreg, reg);
    return true;
}

bool
GreedyAllocator::allocateRegisterOperand(LAllocation *a, VirtualRegister *vr)
{
    AnyRegister reg;

    
    
    
    
    if (!vr->hasRegister() && !allocateReg(vr))
        return false;
    disallowed.addUnchecked(vr->reg());

    *a = LAllocation(vr->reg());
    return true;
}

static inline bool
DeservesRegister(LDefinition::Type type)
{
#ifdef JS_NUNBOX32
    return type != LDefinition::TYPE;
#else
    return true;
#endif
}

bool
GreedyAllocator::allocateAnyOperand(LAllocation *a, VirtualRegister *vr, bool preferReg)
{
    if (vr->hasRegister()) {
        *a = LAllocation(vr->reg());
        return true;
    }

    
    if ((preferReg || DeservesRegister(vr->type())) && !allocatableRegs().empty(vr->isDouble()))
        return allocateRegisterOperand(a, vr);

    
    allocateStack(vr);
    *a = vr->backingStack();

    return true;
}

bool
GreedyAllocator::allocateFixedOperand(LAllocation *a, VirtualRegister *vr)
{
    
    AnyRegister needed = GetFixedRegister(vr->def, a->toUse());

    *a = LAllocation(needed);

    if (!vr->hasRegister()) {
        if (!maybeEvict(needed))
            return false;
        assign(vr, needed);
        return true;
    }

    if (vr->reg() == needed)
        return true;

    
    return align(vr->reg(), needed);
}

bool
GreedyAllocator::allocateDefinition(LInstruction *ins, LDefinition *def)
{
    VirtualRegister *vr = getVirtualRegister(def);

    LAllocation output;
    switch (def->policy()) {
      case LDefinition::PASSTHROUGH:
        
        return true;

      case LDefinition::DEFAULT:
      case LDefinition::MUST_REUSE_INPUT:
      {
        AnyRegister reg;
        
        if (def->policy() == LDefinition::MUST_REUSE_INPUT &&
            ins->getOperand(def->getReusedInput())->toUse()->isFixedRegister())
        {
            LAllocation *a = ins->getOperand(def->getReusedInput());
            VirtualRegister *vuse = getVirtualRegister(a->toUse());
            reg = GetFixedRegister(vuse->def, a->toUse());
        } else if (vr->hasRegister()) {
            reg = vr->reg();
        } else {
            if (!allocate(vr->type(), DISALLOW, &reg))
                return false;
        }

        if (def->policy() == LDefinition::MUST_REUSE_INPUT) {
            LUse *use = ins->getOperand(def->getReusedInput())->toUse();
            VirtualRegister *vuse = getVirtualRegister(use);
            
            if (vuse->hasRegister() && vuse->reg() == reg) {
                if (!evict(reg))
                    return false;
            }

            
            if (reg.isFloat())
                *use = LUse(reg.fpu(), use->virtualRegister());
            else
                *use = LUse(reg.gpr(), use->virtualRegister());
        }
        output = LAllocation(reg);
        break;
      }

      case LDefinition::PRESET:
      {
        
        
        output = *def->output();
        break;
      }
    }

    if (output.isRegister()) {
        JS_ASSERT_IF(output.isFloatReg(), disallowed.has(output.toFloatReg()->reg()));
        JS_ASSERT_IF(output.isGeneralReg(), disallowed.has(output.toGeneralReg()->reg()));
    }

    
    def->setOutput(output);
    return true;
}

bool
GreedyAllocator::spillDefinition(LDefinition *def)
{
    if (def->policy() == LDefinition::PASSTHROUGH)
        return true;

    VirtualRegister *vr = getVirtualRegister(def);
    const LAllocation *output = def->output();

    if (output->isRegister()) {
        if (vr->hasRegister()) {
            
            
            AnyRegister out = GetAllocatedRegister(output);
            if (out != vr->reg()) {
                if (!spill(*output, vr->reg()))
                    return false;
            }
        }

        
        if (vr->hasStackSlot() && vr->backingStackUsed()) {
            if (!spill(*output, vr->backingStack()))
                return false;
        }
    } else if (vr->hasRegister()) {
        
        
        JS_ASSERT(!vr->hasStackSlot());
        JS_ASSERT(vr->hasBackingStack());
        if (!spill(*output, vr->reg()))
            return false;
    }

    return true;
}

bool
GreedyAllocator::allocateDefinitions(LInstruction *ins)
{
    for (size_t i = 0; i < ins->numDefs(); i++) {
        LDefinition *def = ins->getDef(i);
        if (!allocateDefinition(ins, def))
            return false;

        
        
        
        if (!spillDefinition(def))
            return false;
    }

    return true;
}

bool
GreedyAllocator::allocateTemporaries(LInstruction *ins)
{
    for (size_t i = 0; i < ins->numTemps(); i++) {
        LDefinition *def = ins->getTemp(i);
        if (!allocateDefinition(ins, def))
            return false;
    }

    return true;
}

bool
GreedyAllocator::allocateInputs(LInstruction *ins)
{
    
    
    for (size_t i = 0; i < ins->numOperands(); i++) {
        LAllocation *a = ins->getOperand(i);
        if (!a->isUse())
            continue;
        LUse *use = a->toUse();
        VirtualRegister *vr = getVirtualRegister(use);
        if (use->policy() == LUse::FIXED) {
            if (!allocateFixedOperand(a, vr))
                return false;
        } else if (use->policy() == LUse::REGISTER) {
            if (!allocateRegisterOperand(a, vr))
                return false;
        }
    }

    
    for (size_t i = 0; i < ins->numOperands(); i++) {
        LAllocation *a = ins->getOperand(i);
        if (!a->isUse())
            continue;

        VirtualRegister *vr = getVirtualRegister(a->toUse());
        if (!allocateAnyOperand(a, vr))
            return false;
    }

    return true;
}

bool
GreedyAllocator::spillForCall(LInstruction *ins)
{
    for (AnyRegisterIterator iter(RegisterSet::All()); iter.more(); iter++) {
        if (!maybeEvict(*iter))
            return false;
    }
    return true;
}

void
GreedyAllocator::informSnapshot(LInstruction *ins)
{
    LSnapshot *snapshot = ins->snapshot();
    for (size_t i = 0; i < snapshot->numEntries(); i++) {
        LAllocation *a = snapshot->getEntry(i);
        if (!a->isUse())
            continue;

        
        
        
        
        VirtualRegister *vr = getVirtualRegister(a->toUse());
        allocateStack(vr);
        *a = vr->backingStack();
    }
}

bool
GreedyAllocator::informSafepoint(LSafepoint *safepoint)
{
    for (InlineListIterator<VirtualRegister> iter = liveSlots_.begin();
         iter != liveSlots_.end();
         iter++)
    {
        VirtualRegister *vr = *iter;
        if (vr->type() == LDefinition::OBJECT || vr->type() == LDefinition::BOX) {
            if (!safepoint->addGcSlot(vr->stackSlot()))
                return false;
            continue;
        }

#ifdef JS_NUNBOX32
        if (!IsNunbox(vr->type()))
            continue;

        VirtualRegister *other = otherHalfOfNunbox(vr);

        
        if (vr->hasStackSlot() && other->hasStackSlot()) {
            uint32 slot = BaseOfNunboxSlot(vr->type(), vr->stackSlot());
            if (!safepoint->addValueSlot(slot))
                return false;
        }
#endif
    }
    return true;
}

void
GreedyAllocator::assertValidRegisterState()
{
#ifdef DEBUG
    
    
    for (AnyRegisterIterator iter; iter.more(); iter++) {
        AnyRegister reg = *iter;
        VirtualRegister *vr = state[reg];
        if (!reg.allocatable()) {
            JS_ASSERT(!vr);
            continue;
        }
        JS_ASSERT(!vr == state.free.has(reg));
        JS_ASSERT_IF(vr, vr->reg() == reg);
    }
#endif
}

bool
GreedyAllocator::allocateInstruction(LBlock *block, LInstruction *ins)
{
    if (!gen->ensureBallast())
        return false;

    
    reset();
    assertValidRegisterState();

    
    if (ins->isCall() && !spillForCall(ins))
        return false;

    
    
    if (!prescanDefinitions(ins))
        return false;

    
    
    if (!prescanUses(ins))
        return false;

    
    if (!allocateDefinitions(ins))
        return false;

    
    
    if (!allocateTemporaries(ins))
        return false;

    
    if (!allocateInputs(ins))
        return false;

    
    if (ins->snapshot())
        informSnapshot(ins);

    
    for (size_t i = 0; i < ins->numDefs(); i++) {
        LDefinition *def = ins->getDef(i);
        if (def->policy() == LDefinition::PASSTHROUGH)
            continue;
        killStack(getVirtualRegister(def));
    }

    
    
    if (ins->safepoint()) {
        if (!informSafepoint(ins->safepoint()))
            return false;
    }

    return true;
}

void
GreedyAllocator::findLoopCarriedUses(LInstruction *ins, uint32 lowerBound, uint32 upperBound)
{
    for (size_t i = 0; i < ins->numOperands(); i++) {
        LAllocation *a = ins->getOperand(i);
        if (!a->isUse())
            continue;
        LUse *use = a->toUse();
        VirtualRegister *vr = getVirtualRegister(use);
        if (vr->def->virtualRegister() < lowerBound || vr->def->virtualRegister() > upperBound)
            allocateStack(vr);
    }
}





bool
GreedyAllocator::findLoopCarriedUses(LBlock *backedge)
{
    Vector<LBlock *, 4, SystemAllocPolicy> worklist;
    MBasicBlock *mheader = backedge->mir()->loopHeaderOfBackedge();
    uint32 upperBound = backedge->lastId();
    uint32 lowerBound = mheader->lir()->firstId();

    IonSpew(IonSpew_RegAlloc, "  Finding loop-carried uses.");

    for (size_t i = 0; i < mheader->numContainedInLoop(); i++) {
        LBlock *block = mheader->getContainedInLoop(i)->lir();

        for (LInstructionIterator i = block->begin(); i != block->end(); i++)
            findLoopCarriedUses(*i, lowerBound, upperBound);
        for (size_t i = 0; i < block->numPhis(); i++)
            findLoopCarriedUses(block->getPhi(i), lowerBound, upperBound);
    }

    IonSpew(IonSpew_RegAlloc, "  Done finding loop-carried uses.");

    return true;
}

bool
GreedyAllocator::allocateRegistersInBlock(LBlock *block)
{
    IonSpew(IonSpew_RegAlloc, " Allocating instructions.");

    LInstructionReverseIterator ri = block->instructions().rbegin();

    
    
    
    if (!allocateInstruction(block, *ri))
        return false;
    if (aligns) {
        block->insertBefore(*ri, aligns);
        ri++;
    }

    JS_ASSERT(!spills);
    JS_ASSERT(!restores);

    
    
    
    
    if (!buildPhiMoves(block))
        return false;
    if (phiMoves.moves)
        block->insertBefore(*ri, phiMoves.moves);
    ri++;

    if (block->mir()->isLoopBackedge()) {
        if (!findLoopCarriedUses(block))
            return false;
    }

    for (; ri != block->instructions().rend(); ri++) {
        LInstruction *ins = *ri;

        if (!allocateInstruction(block, ins))
            return false;

        
        if (aligns)
            block->insertBefore(ins, aligns);
        if (restores)
            block->insertAfter(ins, restores);
        if (spills) {
            JS_ASSERT(ri != block->rbegin());
            block->insertAfter(ins, spills);
        }

        assertValidRegisterState();
    }

    IonSpew(IonSpew_RegAlloc, " Done allocating instructions.");
    return true;
}

bool
GreedyAllocator::buildPhiMoves(LBlock *block)
{
    IonSpew(IonSpew_RegAlloc, " Merging phi state."); 

    phiMoves = Mover();

    MBasicBlock *mblock = block->mir();
    if (!mblock->successorWithPhis())
        return true;

    
    uint32 pos = mblock->positionInPhiSuccessor();
    LBlock *successor = mblock->successorWithPhis()->lir();
    for (size_t i = 0; i < successor->numPhis(); i++) {
        LPhi *phi = successor->getPhi(i);
        JS_ASSERT(phi->numDefs() == 1);

        VirtualRegister *phiReg = getVirtualRegister(phi->getDef(0));
        allocateStack(phiReg);

        LAllocation *in = phi->getOperand(pos);
        VirtualRegister *inReg = getVirtualRegister(in->toUse());
        allocateStack(inReg);

        
        if (!inReg->hasRegister() && !allocatableRegs().empty(inReg->isDouble())) {
            if (!allocateReg(inReg))
                return false;
        }

        
        if (inReg->hasRegister()) {
            if (!phiMoves.move(inReg->reg(), phiReg->backingStack()))
                return false;
        } else {
            if (!phiMoves.move(inReg->backingStack(), phiReg->backingStack()))
                return false;
        }
    }

    return true;
}

bool
GreedyAllocator::allocateRegisters()
{
    
    
    for (size_t i = graph.numBlocks() - 1; i < graph.numBlocks(); i--) {
        LBlock *block = graph.getBlock(i);

        IonSpew(IonSpew_RegAlloc, "Allocating block %d", (uint32)i);

        
        JS_ASSERT(state.free == RegisterSet::All());

        
        for (size_t j = 0; j < block->numPhis(); j++) {
            LPhi *phi = block->getPhi(j);
            VirtualRegister *vreg = getVirtualRegister(phi->getDef(0));
            allocateStack(vreg);
        }

        
        if (!allocateRegistersInBlock(block))
            return false;

        LMoveGroup *entrySpills = block->getEntryMoveGroup();

        
        
        for (AnyRegisterIterator iter(RegisterSet::All()); iter.more(); iter++) {
            VirtualRegister *vreg = state[*iter];
            if (!vreg) {
                JS_ASSERT(state.free.has(*iter));
                continue;
            }

            JS_ASSERT(vreg->reg() == *iter);
            JS_ASSERT(!state.free.has(vreg->reg()));
            allocateStack(vreg);

            LAllocation *from = LAllocation::New(vreg->backingStack());
            LAllocation *to = LAllocation::New(vreg->reg());
            if (!entrySpills->add(from, to))
                return false;

            killReg(vreg);
            vreg->unsetRegister();
        }

        
        
        
        
        for (size_t i = 0; i < block->numPhis(); i++) {
            LPhi *phi = block->getPhi(i);
            for (size_t j = 0; j < phi->numOperands(); j++) {
                VirtualRegister *in = getVirtualRegister(phi->getOperand(j)->toUse());
                allocateStack(in);
            }
        }

        
        for (size_t i = 0; i < block->numPhis(); i++) {
            LPhi *phi = block->getPhi(i);
            VirtualRegister *vr = getVirtualRegister(phi->getDef(0));
            JS_ASSERT(!vr->hasRegister());
            killStack(vr);
        }
    }
    return true;
}

bool
GreedyAllocator::allocate()
{
    if (!FindNaturalLoops(gen->graph()))
        return false;

    vars = gen->allocate<VirtualRegister>(graph.numVirtualRegisters());
    if (!vars)
        return false;
    memset(vars, 0, sizeof(VirtualRegister) * graph.numVirtualRegisters());

    findDefinitions();
    if (!allocateRegisters())
        return false;
    graph.setLocalSlotCount(stackSlotAllocator.stackHeight());

    return true;
}

