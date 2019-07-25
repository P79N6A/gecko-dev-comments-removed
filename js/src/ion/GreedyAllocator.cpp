








































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

        VirtualRegister *vr = &vars[def->virtualRegister()];
        vr->init(def, ins);
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

        LUse *use = a->toUse();
        VirtualRegister *vr = getVirtualRegister(use);

        if (use->policy() == LUse::FIXED)
            disallowed.add(GetFixedRegister(vr->def, use));
        else if (vr->hasRegister())
            discouraged.addUnchecked(vr->reg());
    }
    return true;
}

void
GreedyAllocator::allocateStack(VirtualRegister *vr)
{
    if (vr->hasBackingStack())
        return;

    uint32 index;
    if (vr->isDouble())
        index = stackSlotAllocator.allocateDoubleSlot();
    else
        index = stackSlotAllocator.allocateSlot();

    IonSpew(IonSpew_RegAlloc, "    assign vr%d := stack%d", vr->def->virtualRegister(), index);

    vr->setStackSlot(index);
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
GreedyAllocator::freeStack(VirtualRegister *vr)
{
    if (vr->isDouble())
        stackSlotAllocator.freeDoubleSlot(vr->stackSlot());
    else
        stackSlotAllocator.freeSlot(vr->stackSlot());
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
    if (vr->hasStackSlot()) {
        IonSpew(IonSpew_RegAlloc, "    kill vr%d (stack:%d)",
                vr->def->virtualRegister(), vr->stackSlot_);
        freeStack(vr);
    }
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
GreedyAllocator::allocateRegisterOperand(LAllocation *a, VirtualRegister *vr)
{
    AnyRegister reg;

    
    
    
    
    if (vr->hasRegister()) {
        reg = vr->reg();
        disallowed.addUnchecked(reg);
    } else {
        
        if (!allocate(vr->type(), DISALLOW, &reg))
            return false;
        assign(vr, reg);
    }

    *a = LAllocation(reg);
    return true;
}

bool
GreedyAllocator::allocateWritableOperand(LAllocation *a, VirtualRegister *vr)
{
    AnyRegister reg;
    if (!vr->hasRegister()) {
        
        
        if (!allocate(vr->type(), DISALLOW, &reg))
            return false;
        assign(vr, reg);
    } else {
        if (allocatableRegs().empty(vr->isDouble())) {
            
            if (!allocate(vr->type(), DISALLOW, &reg))
                return false;
            align(vr->reg(), reg);
        } else {
            
            reg = vr->reg();

            allocateStack(vr);
            if (!restore(vr->backingStack(), reg))
                return false;
        }
    }

    *a = LAllocation(reg);
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
GreedyAllocator::allocateSameAsInput(LDefinition *def, LAllocation *a, AnyRegister *out)
{
    LUse *use = a->toUse();
    VirtualRegister *vdef = getVirtualRegister(def);
    VirtualRegister *vuse = getVirtualRegister(use);

    JS_ASSERT(vdef->isDouble() == vuse->isDouble());

    AnyRegister reg;

    
    
    
    if (use->isFixedRegister()) {
        reg = GetFixedRegister(def, use);
    } else if (vdef->hasRegister()) {
        reg = vdef->reg();
    } else {
        if (!allocate(vdef->type(), DISALLOW, &reg))
            return false;
    }
    JS_ASSERT(disallowed.has(reg));

    if (vuse->hasRegister()) {
        JS_ASSERT(vuse->reg() != reg);
        if (!align(vuse->reg(), reg))
            return false;
    } else {
        
        
        
        assign(vuse, reg);
    }

    
    *a = LAllocation(reg);

    *out = reg;
    return true;
}

bool
GreedyAllocator::allocateDefinitions(LInstruction *ins)
{
    for (size_t i = 0; i < ins->numDefs(); i++) {
        LDefinition *def = ins->getDef(i);
        VirtualRegister *vr = getVirtualRegister(def);

        LAllocation output;
        switch (def->policy()) {
          case LDefinition::PASSTHROUGH:
            
            continue;

          case LDefinition::DEFAULT:
          {
            
            if (vr->hasRegister()) {
                output = LAllocation(vr->reg());
            } else {
                AnyRegister reg;
                if (!allocate(vr->type(), DISALLOW, &reg))
                    return false;
                output = LAllocation(reg);
            }
            break;
          }

          case LDefinition::PRESET:
          {
            
            
            output = *def->output();
            break;
          }

          case LDefinition::MUST_REUSE_INPUT:
          {
            AnyRegister out_reg;
            if (!allocateSameAsInput(def, ins->getOperand(0), &out_reg))
                return false;
            output = LAllocation(out_reg);
            break;
          }
        }

        if (output.isRegister()) {
            JS_ASSERT_IF(output.isFloatReg(), disallowed.has(output.toFloatReg()->reg()));
            JS_ASSERT_IF(output.isGeneralReg(), disallowed.has(output.toGeneralReg()->reg()));
        }

        
        
        
        if (output.isRegister()) {
            if (vr->hasRegister()) {
                
                
                AnyRegister out = GetAllocatedRegister(&output);
                if (out != vr->reg()) {
                    if (!spill(output, vr->reg()))
                        return false;
                }
            }

            
            if (vr->hasStackSlot() && vr->backingStackUsed()) {
                if (!spill(output, vr->backingStack()))
                    return false;
            }
        } else if (vr->hasRegister()) {
            
            
            JS_ASSERT(!vr->hasStackSlot());
            JS_ASSERT(vr->hasBackingStack());
            if (!spill(output, vr->reg()))
                return false;
        }

        
        *def = LDefinition(def->type(), output);
    }

    return true;
}

bool
GreedyAllocator::allocateTemporaries(LInstruction *ins)
{
    for (size_t i = 0; i < ins->numTemps(); i++) {
        LDefinition *def = ins->getTemp(i);
        if (def->policy() == LDefinition::PRESET)
            continue;

        JS_ASSERT(def->policy() == LDefinition::DEFAULT);
        AnyRegister reg;
        if (!allocate(def->type(), DISALLOW, &reg))
            return false;
        *def = LDefinition(def->type(), LAllocation(reg));
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
        } else if (use->policy() == LUse::COPY) {
            if (!allocateWritableOperand(a, vr))
                return false;
        }
    }

    
    
    if (!allocateTemporaries(ins))
        return false;

    
    for (size_t i = 0; i < ins->numOperands(); i++) {
        LAllocation *a = ins->getOperand(i);
        if (!a->isUse())
            continue;

        LUse *use = a->toUse();
        JS_ASSERT(use->policy() == LUse::ANY);

        VirtualRegister *vr = getVirtualRegister(use);
        if (!allocateAnyOperand(a, vr))
            return false;
    }

    return true;
}

bool
GreedyAllocator::spillForCall(LInstruction *ins)
{
    for (AnyRegisterIterator iter(ins->spillRegs()); iter.more(); iter++) {
        if (!maybeEvict(*iter))
            return false;
    }
    return true;
}

void
GreedyAllocator::informSnapshot(LSnapshot *snapshot)
{
    for (size_t i = 0; i < snapshot->numEntries(); i++) {
        LAllocation *a = snapshot->getEntry(i);
        if (!a->isUse())
            continue;

        LUse *use = a->toUse();
        VirtualRegister *vr = getVirtualRegister(use);
        if (vr->hasRegister()) {
            *a = LAllocation(vr->reg());
        } else {
            allocateStack(vr);
            *a = vr->backingStack();
        }
    }
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

    
    if (ins->snapshot() && ins->op() != LInstruction::LOp_CaptureAllocations)
        informSnapshot(ins->snapshot());

    
    if (!allocateDefinitions(ins))
        return false;

    
    if (!allocateInputs(ins))
        return false;

    
    for (size_t i = 0; i < ins->numDefs(); i++) {
        LDefinition *def = ins->getDef(i);
        if (def->policy() == LDefinition::PASSTHROUGH)
            continue;
        killStack(getVirtualRegister(def));
    }

    
    if (ins->postSnapshot())
        informSnapshot(ins->postSnapshot());

    if (aligns)
        block->insertBefore(ins, aligns);

    return true;
}

bool
GreedyAllocator::allocateRegistersInBlock(LBlock *block)
{
    IonSpew(IonSpew_RegAlloc, " Allocating instructions.");

    LInstructionReverseIterator ri = block->instructions().rbegin();

    
    
    
    if (!allocateInstruction(block, *ri))
        return false;
    ri++;

    JS_ASSERT(!spills);

    if (restores) {
        
        
        
        
        for (size_t i = 0; i < block->mir()->numSuccessors(); i++) {
            MBasicBlock *msuccessor = block->mir()->getSuccessor(i);
            if (msuccessor->id() <= block->mir()->id())
                continue;

            Mover moves;
            LBlock *successor = msuccessor->lir();
            for (size_t i = 0; i < restores->numMoves(); i++) {
                const LMove &move = restores->getMove(i);
                if (!moves.move(*move.from(), *move.to()))
                    return false;
            }

            successor->insertBefore(*successor->begin(), moves.moves);
        }
    }

    if (block->mir()->isLoopBackedge()) {
        
        
        
        if (!prepareBackedge(block))
            return false;
    }
    blockInfo(block)->out = state;

    for (; ri != block->instructions().rend(); ri++) {
        LInstruction *ins = *ri;

        if (!allocateInstruction(block, ins))
            return false;

        
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
GreedyAllocator::mergeRegisterState(const AnyRegister &reg, LBlock *left, LBlock *right)
{
    VirtualRegister *vleft = state[reg];
    VirtualRegister *vright = blockInfo(right)->in[reg];

    
    if (vleft == vright)
        return true;

    
    
    if (!vright)
        return true;

    BlockInfo *rinfo = blockInfo(right);

    if (!vleft && !vright->hasRegister()) {
        
        
        
        assign(vright, reg);
        return true;
    }

    
    
    
    
    
    
    
    
    if (!vright->hasRegister() && !allocatableRegs().empty(vright->isDouble())) {
        AnyRegister reg;
        if (!allocate(vright->type(), DISALLOW, &reg))
            return false;
        assign(vright, reg);
    }

    if (vright->hasRegister()) {
        JS_ASSERT(vright->reg() != reg);

        IonSpew(IonSpew_RegAlloc, "    merging vr%d %s to %s",
                vright->def->virtualRegister(), vright->reg().name(), reg.name());
        if (!rinfo->restores.move(vright->reg(), reg))
            return false;
    } else {
        allocateStack(vright);

        IonSpew(IonSpew_RegAlloc, "    merging vr%d stack to %s",
                vright->def->virtualRegister(), reg.name());
        if (!rinfo->restores.move(vright->backingStack(), reg))
            return false;
    }

    return true;
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

        for (LInstructionIterator i = block->begin(); i != block->end(); i++) {
            LInstruction *ins = *i;
            for (size_t i = 0; i < ins->numOperands(); i++) {
                LAllocation *a = ins->getOperand(i);
                if (!a->isUse())
                    continue;
                LUse *use = a->toUse();
                VirtualRegister *vr = getVirtualRegister(use);
                if (vr->def->virtualRegister() < lowerBound ||
                    vr->def->virtualRegister() > upperBound) {
                    allocateStack(vr);
                }
            }
        }
    }

    IonSpew(IonSpew_RegAlloc, "  Done finding loop-carried uses.");

    return true;
}

bool
GreedyAllocator::prepareBackedge(LBlock *block)
{
    MBasicBlock *msuccessor = block->mir()->successorWithPhis();
    if (!msuccessor)
        return true;

    IonSpew(IonSpew_RegAlloc, "  Preparing backedge.");

    LBlock *successor = msuccessor->lir();

    uint32 pos = block->mir()->positionInPhiSuccessor();
    for (size_t i = 0; i < successor->numPhis(); i++) {
        LPhi *phi = successor->getPhi(i);
        LAllocation *a = phi->getOperand(pos);
        if (!a->isUse())
            continue;
        VirtualRegister *vr = getVirtualRegister(a->toUse());

        
        
        LAllocation result;
        if (!allocateAnyOperand(&result, vr, true))
            return false;

        
        
        
        phi->getDef(0)->setOutput(result);
    }

    IonSpew(IonSpew_RegAlloc, "  Done preparing backedge.");

    if (!findLoopCarriedUses(block))
        return false;

    return true;
}

bool
GreedyAllocator::mergeBackedgeState(LBlock *header, LBlock *backedge)
{
    BlockInfo *info = blockInfo(backedge);

    IonSpew(IonSpew_RegAlloc, "  Merging backedge state.");

    
    
    Mover carried;
    for (AnyRegisterIterator iter; iter.more(); iter++) {
        AnyRegister reg = *iter;
        VirtualRegister *inVr = state[reg];
        if (!inVr)
            continue;

        VirtualRegister *outVr = info->out[reg];
        if (inVr == outVr)
            continue;

        
        
        
        
        
        
        
        allocateStack(inVr);

        IonSpew(IonSpew_RegAlloc, "    loop carried vr%d (stack:%d) -> %s",
                inVr->def->virtualRegister(), inVr->stackSlot_, reg.name());

        if (!carried.move(inVr->backingStack(), reg))
            return false;
    }
    if (carried.moves) {
        LInstruction *ins = *header->instructions().begin();
        header->insertBefore(ins, carried.moves);
    }

    IonSpew(IonSpew_RegAlloc, "  Done merging backedge state.");
    IonSpew(IonSpew_RegAlloc, "  Handling loop phis.");

    Mover phis;

    
    for (size_t i = 0; i < header->numPhis(); i++) {
        LPhi *phi = header->getPhi(i);
        LDefinition *def = phi->getDef(0);
        VirtualRegister *vr = getVirtualRegister(def);

        JS_ASSERT(def->policy() == LDefinition::PRESET);
        const LAllocation *a = def->output();

        if (vr->hasStackSlot() && !phis.move(*a, vr->backingStack()))
            return false;

        if (vr->hasRegister() && (!a->isRegister() || vr->reg() != a->toRegister())) {
            if (!phis.move(*a, vr->reg()))
                return false;
        }
    }

    if (phis.moves) {
        LInstruction *ins = *backedge->instructions().rbegin();
        backedge->insertBefore(ins, phis.moves);
    }

    IonSpew(IonSpew_RegAlloc, "  Done handling loop phis.");

    return true;
}

bool
GreedyAllocator::mergePhiState(LBlock *block)
{
    MBasicBlock *mblock = block->mir();
    if (!mblock->successorWithPhis())
        return true;

    IonSpew(IonSpew_RegAlloc, "  Merging phi state.");

    
    reset();

    Mover phis;

    uint32 pos = mblock->positionInPhiSuccessor();
    LBlock *successor = mblock->successorWithPhis()->lir();
    for (size_t i = 0; i < successor->numPhis(); i++) {
        LPhi *phi = successor->getPhi(i);
        VirtualRegister *def = getVirtualRegister(phi->getDef(0));

        
        if (!def->hasRegister() && !def->hasStackSlot()) {
            IonSpew(IonSpew_RegAlloc, "    ignoring unused phi vr%d", def->def->virtualRegister());
            continue;
        }

        LAllocation *a = phi->getOperand(pos);
        VirtualRegister *use = getVirtualRegister(a->toUse());

        
        if (!use->hasRegister()) {
            if (def->hasRegister() && !state[def->reg()]) {
                assign(use, def->reg());
            } else {
                LAllocation unused;
                if (!allocateAnyOperand(&unused, use, true))
                    return false;
            }
        }

        
        if (def->hasRegister()) {
            if (use->hasRegister()) {
                if (use->reg() != def->reg()) {
                    IonSpew(IonSpew_RegAlloc, "    vr%d (%s) -> phi vr%d (%s)",
                            use->def->virtualRegister(), use->reg().name(),
                            def->def->virtualRegister(), def->reg().name());

                    if (!phis.move(use->reg(), def->reg()))
                        return false;
                }
            } else {
                IonSpew(IonSpew_RegAlloc, "    vr%d (stack:%d) -> phi vr%d (%s)",
                        use->def->virtualRegister(), use->stackSlot_,
                        def->def->virtualRegister(), def->reg().name());

                if (!phis.move(use->backingStack(), def->reg()))
                    return false;
            }
        }

        
        if (def->hasStackSlot()) {
            if (use->hasRegister()) {
                IonSpew(IonSpew_RegAlloc, "    vr%d (%s) -> phi vr%d (stack:%d)",
                        use->def->virtualRegister(), use->reg().name(),
                        def->def->virtualRegister(), use->stackSlot_);

                if (!phis.move(use->reg(), def->backingStack()))
                    return false;
            } else if (use->backingStack() != def->backingStack()) {
                IonSpew(IonSpew_RegAlloc, "    vr%d (stack:%d) -> phi vr%d (stack:%d)",
                        use->def->virtualRegister(), use->stackSlot_,
                        def->def->virtualRegister(), def->stackSlot_);

                if (!phis.move(use->backingStack(), def->backingStack()))
                    return false;
            }
        }
    }

    
    JS_ASSERT(!aligns);
    JS_ASSERT(!spills);
    LInstruction *before = *block->instructions().rbegin();
    if (restores)
        block->insertBefore(before, restores);
    if (phis.moves)
        block->insertBefore(before, phis.moves);

    IonSpew(IonSpew_RegAlloc, "  Done merging phi state.");

    return true;
}

bool
GreedyAllocator::mergeAllocationState(LBlock *block)
{
    MBasicBlock *mblock = block->mir();

    if (!mblock->numSuccessors()) {
        state = AllocationState();
        return true;
    }

    IonSpew(IonSpew_RegAlloc, " Merging allocation state.");

    
    LBlock *leftblock = mblock->getSuccessor(0)->lir();
    state = blockInfo(leftblock)->in;

    
    
    for (AnyRegisterIterator iter; iter.more(); iter++) {
        AnyRegister reg = *iter;
        if (VirtualRegister *vr = state[reg]) {
            vr->setRegister(reg);
            IonSpew(IonSpew_RegAlloc, "    vr%d inherits %s",
                    vr->def->virtualRegister(), reg.name());
        }
    }

    
    for (size_t i = 1; i < mblock->numSuccessors(); i++) {
        LBlock *rightblock = mblock->getSuccessor(i)->lir();

        for (AnyRegisterIterator iter; iter.more(); iter++) {
            AnyRegister reg = *iter;
            if (!mergeRegisterState(reg, leftblock, rightblock))
                return false;
        }

        
        BlockInfo *info = blockInfo(rightblock);
        if (info->restores.moves)
            rightblock->insertBefore(*rightblock->begin(), info->restores.moves);
    }

    if (!mergePhiState(block))
        return false;

    IonSpew(IonSpew_RegAlloc, " Done merging allocation state.");

    return true;
}

bool
GreedyAllocator::allocateRegisters()
{
    
    
    for (size_t i = graph.numBlocks() - 1; i < graph.numBlocks(); i--) {
        LBlock *block = graph.getBlock(i);

        IonSpew(IonSpew_RegAlloc, "Allocating block %d", (uint32)i);

        
        if (!mergeAllocationState(block))
            return false;

        
        if (!allocateRegistersInBlock(block))
            return false;

        
        
        if (block->mir()->isLoopHeader()) {
            if (!mergeBackedgeState(block, block->mir()->backedge()->lir()))
                return false;
        }

        
        for (size_t i = 0; i < block->numPhis(); i++) {
            LPhi *phi = block->getPhi(i);
            JS_ASSERT(phi->numDefs() == 1);

            VirtualRegister *vr = getVirtualRegister(phi->getDef(0));
            killReg(vr);
            killStack(vr);
        }

        
        
        
        
        
        blockInfo(block)->in = state;
        for (AnyRegisterIterator iter; iter.more(); iter++) {
            AnyRegister reg = *iter;
            VirtualRegister *vr = state[reg];
            if (vr) {
                JS_ASSERT(vr->reg() == reg);
                vr->unsetRegister();
            }
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

    blocks = gen->allocate<BlockInfo>(graph.numBlockIds());
    for (size_t i = 0; i < graph.numBlockIds(); i++)
        new (&blocks[i]) BlockInfo();

    findDefinitions();
    if (!allocateRegisters())
        return false;
    graph.setLocalSlotCount(stackSlotAllocator.stackHeight());

    return true;
}

