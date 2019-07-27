





#include "jit/StupidAllocator.h"

#include "jstypes.h"

using namespace js;
using namespace js::jit;

static inline uint32_t
DefaultStackSlot(uint32_t vreg)
{
    return vreg * sizeof(Value);
}

LAllocation *
StupidAllocator::stackLocation(uint32_t vreg)
{
    LDefinition *def = virtualRegisters[vreg];
    if (def->policy() == LDefinition::FIXED && def->output()->isArgument())
        return def->output();

    return new(alloc()) LStackSlot(DefaultStackSlot(vreg));
}

StupidAllocator::RegisterIndex
StupidAllocator::registerIndex(AnyRegister reg)
{
    for (size_t i = 0; i < registerCount; i++) {
        if (reg == registers[i].reg)
            return i;
    }
    MOZ_ASSUME_UNREACHABLE("Bad register");
}

bool
StupidAllocator::init()
{
    if (!RegisterAllocator::init())
        return false;

    if (!virtualRegisters.appendN((LDefinition *)nullptr, graph.numVirtualRegisters()))
        return false;

    for (size_t i = 0; i < graph.numBlocks(); i++) {
        LBlock *block = graph.getBlock(i);
        for (LInstructionIterator ins = block->begin(); ins != block->end(); ins++) {
            for (size_t j = 0; j < ins->numDefs(); j++) {
                LDefinition *def = ins->getDef(j);
                virtualRegisters[def->virtualRegister()] = def;
            }

            for (size_t j = 0; j < ins->numTemps(); j++) {
                LDefinition *def = ins->getTemp(j);
                if (def->isBogusTemp())
                    continue;
                virtualRegisters[def->virtualRegister()] = def;
            }
        }
        for (size_t j = 0; j < block->numPhis(); j++) {
            LPhi *phi = block->getPhi(j);
            LDefinition *def = phi->getDef(0);
            uint32_t vreg = def->virtualRegister();

            virtualRegisters[vreg] = def;
        }
    }

    
    {
        registerCount = 0;
        RegisterSet remainingRegisters(allRegisters_);
        while (!remainingRegisters.empty( false))
            registers[registerCount++].reg = AnyRegister(remainingRegisters.takeGeneral());

        while (!remainingRegisters.empty( true))
            registers[registerCount++].reg = AnyRegister(remainingRegisters.takeFloat());

        JS_ASSERT(registerCount <= MAX_REGISTERS);
    }

    return true;
}

bool
StupidAllocator::allocationRequiresRegister(const LAllocation *alloc, AnyRegister reg)
{
    if (alloc->isRegister() && alloc->toRegister() == reg)
        return true;
    if (alloc->isUse()) {
        const LUse *use = alloc->toUse();
        if (use->policy() == LUse::FIXED) {
            AnyRegister usedReg = GetFixedRegister(virtualRegisters[use->virtualRegister()], use);
            if (usedReg.aliases(reg))
                return true;
        }
    }
    return false;
}

bool
StupidAllocator::registerIsReserved(LInstruction *ins, AnyRegister reg)
{
    
    for (LInstruction::InputIterator alloc(*ins); alloc.more(); alloc.next()) {
        if (allocationRequiresRegister(*alloc, reg))
            return true;
    }
    for (size_t i = 0; i < ins->numTemps(); i++) {
        if (allocationRequiresRegister(ins->getTemp(i)->output(), reg))
            return true;
    }
    for (size_t i = 0; i < ins->numDefs(); i++) {
        if (allocationRequiresRegister(ins->getDef(i)->output(), reg))
            return true;
    }
    return false;
}

AnyRegister
StupidAllocator::ensureHasRegister(LInstruction *ins, uint32_t vreg)
{
    

    
    RegisterIndex existing = findExistingRegister(vreg);
    if (existing != UINT32_MAX) {
        if (registerIsReserved(ins, registers[existing].reg)) {
            evictAliasedRegister(ins, existing);
        } else {
            registers[existing].age = ins->id();
            return registers[existing].reg;
        }
    }

    RegisterIndex best = allocateRegister(ins, vreg);
    loadRegister(ins, vreg, best, virtualRegisters[vreg]->type());

    return registers[best].reg;
}

StupidAllocator::RegisterIndex
StupidAllocator::allocateRegister(LInstruction *ins, uint32_t vreg)
{
    
    
    
    JS_ASSERT(ins);

    LDefinition *def = virtualRegisters[vreg];
    JS_ASSERT(def);

    RegisterIndex best = UINT32_MAX;

    for (size_t i = 0; i < registerCount; i++) {
        AnyRegister reg = registers[i].reg;

        if (!def->isCompatibleReg(reg))
            continue;

        
        if (registerIsReserved(ins, reg))
            continue;

        if (registers[i].vreg == MISSING_ALLOCATION ||
            best == UINT32_MAX ||
            registers[best].age > registers[i].age)
        {
            best = i;
        }
    }

    evictAliasedRegister(ins, best);
    return best;
}

void
StupidAllocator::syncRegister(LInstruction *ins, RegisterIndex index)
{
    if (registers[index].dirty) {
        LMoveGroup *input = getInputMoveGroup(ins->id());
        LAllocation *source = new(alloc()) LAllocation(registers[index].reg);

        uint32_t existing = registers[index].vreg;
        LAllocation *dest = stackLocation(existing);
        input->addAfter(source, dest, registers[index].type);

        registers[index].dirty = false;
    }
}

void
StupidAllocator::evictRegister(LInstruction *ins, RegisterIndex index)
{
    syncRegister(ins, index);
    registers[index].set(MISSING_ALLOCATION);
}

void
StupidAllocator::evictAliasedRegister(LInstruction *ins, RegisterIndex index)
{
    for (size_t i = 0; i < registers[index].reg.numAliased(); i++) {
        uint32_t aindex = registerIndex(registers[index].reg.aliased(i));
        syncRegister(ins, aindex);
        registers[aindex].set(MISSING_ALLOCATION);
    }
}

void
StupidAllocator::loadRegister(LInstruction *ins, uint32_t vreg, RegisterIndex index, LDefinition::Type type)
{
    
    LMoveGroup *input = getInputMoveGroup(ins->id());
    LAllocation *source = stackLocation(vreg);
    LAllocation *dest = new(alloc()) LAllocation(registers[index].reg);
    input->addAfter(source, dest, type);
    registers[index].set(vreg, ins);
    registers[index].type = type;
}

StupidAllocator::RegisterIndex
StupidAllocator::findExistingRegister(uint32_t vreg)
{
    for (size_t i = 0; i < registerCount; i++) {
        if (registers[i].vreg == vreg)
            return i;
    }
    return UINT32_MAX;
}

bool
StupidAllocator::go()
{
    
    
    
    
    
    
    
    
    

    
    
    
    
    
    graph.setLocalSlotCount(DefaultStackSlot(graph.numVirtualRegisters()));

    if (!init())
        return false;

    for (size_t blockIndex = 0; blockIndex < graph.numBlocks(); blockIndex++) {
        LBlock *block = graph.getBlock(blockIndex);
        JS_ASSERT(block->mir()->id() == blockIndex);

        for (size_t i = 0; i < registerCount; i++)
            registers[i].set(MISSING_ALLOCATION);

        for (LInstructionIterator iter = block->begin(); iter != block->end(); iter++) {
            LInstruction *ins = *iter;

            if (ins == *block->rbegin())
                syncForBlockEnd(block, ins);

            allocateForInstruction(ins);
        }
    }

    return true;
}

void
StupidAllocator::syncForBlockEnd(LBlock *block, LInstruction *ins)
{
    
    
    
    
    
    

    for (size_t i = 0; i < registerCount; i++)
        syncRegister(ins, i);

    LMoveGroup *group = nullptr;

    MBasicBlock *successor = block->mir()->successorWithPhis();
    if (successor) {
        uint32_t position = block->mir()->positionInPhiSuccessor();
        LBlock *lirsuccessor = successor->lir();
        for (size_t i = 0; i < lirsuccessor->numPhis(); i++) {
            LPhi *phi = lirsuccessor->getPhi(i);

            uint32_t sourcevreg = phi->getOperand(position)->toUse()->virtualRegister();
            uint32_t destvreg = phi->getDef(0)->virtualRegister();

            if (sourcevreg == destvreg)
                continue;

            LAllocation *source = stackLocation(sourcevreg);
            LAllocation *dest = stackLocation(destvreg);

            if (!group) {
                
                
                LMoveGroup *input = getInputMoveGroup(ins->id());
                if (input->numMoves() == 0) {
                    group = input;
                } else {
                    group = LMoveGroup::New(alloc());
                    block->insertAfter(input, group);
                }
            }

            group->add(source, dest, phi->getDef(0)->type());
        }
    }
}

void
StupidAllocator::allocateForInstruction(LInstruction *ins)
{
    
    if (ins->isCall()) {
        for (size_t i = 0; i < registerCount; i++)
            syncRegister(ins, i);
    }

    
    for (LInstruction::InputIterator alloc(*ins); alloc.more(); alloc.next()) {
        if (!alloc->isUse())
            continue;
        LUse *use = alloc->toUse();
        uint32_t vreg = use->virtualRegister();
        if (use->policy() == LUse::REGISTER) {
            AnyRegister reg = ensureHasRegister(ins, vreg);
            alloc.replace(LAllocation(reg));
        } else if (use->policy() == LUse::FIXED) {
            AnyRegister reg = GetFixedRegister(virtualRegisters[vreg], use);
            RegisterIndex index = registerIndex(reg);
            if (registers[index].vreg != vreg) {
                
                evictAliasedRegister(ins, registerIndex(reg));
                
                RegisterIndex existing = findExistingRegister(vreg);
                if (existing != UINT32_MAX)
                    evictRegister(ins, existing);
                loadRegister(ins, vreg, index, virtualRegisters[vreg]->type());
            }
            alloc.replace(LAllocation(reg));
        } else {
            
            
            
        }
    }

    
    for (size_t i = 0; i < ins->numTemps(); i++) {
        LDefinition *def = ins->getTemp(i);
        if (!def->isBogusTemp())
            allocateForDefinition(ins, def);
    }
    for (size_t i = 0; i < ins->numDefs(); i++) {
        LDefinition *def = ins->getDef(i);
        allocateForDefinition(ins, def);
    }

    
    for (LInstruction::InputIterator alloc(*ins); alloc.more(); alloc.next()) {
        if (!alloc->isUse())
            continue;
        LUse *use = alloc->toUse();
        uint32_t vreg = use->virtualRegister();
        JS_ASSERT(use->policy() != LUse::REGISTER && use->policy() != LUse::FIXED);

        RegisterIndex index = findExistingRegister(vreg);
        if (index == UINT32_MAX) {
            LAllocation *stack = stackLocation(use->virtualRegister());
            alloc.replace(*stack);
        } else {
            registers[index].age = ins->id();
            alloc.replace(LAllocation(registers[index].reg));
        }
    }

    
    if (ins->isCall()) {
        for (size_t i = 0; i < registerCount; i++) {
            if (!registers[i].dirty)
                registers[i].set(MISSING_ALLOCATION);
        }
    }
}

void
StupidAllocator::allocateForDefinition(LInstruction *ins, LDefinition *def)
{
    uint32_t vreg = def->virtualRegister();

    CodePosition from;
    if ((def->output()->isRegister() && def->policy() == LDefinition::FIXED) ||
        def->policy() == LDefinition::MUST_REUSE_INPUT)
    {
        
        
        RegisterIndex index =
            registerIndex(def->policy() == LDefinition::FIXED
                          ? def->output()->toRegister()
                          : ins->getOperand(def->getReusedInput())->toRegister());
        evictRegister(ins, index);
        registers[index].set(vreg, ins, true);
        registers[index].type = virtualRegisters[vreg]->type();
        def->setOutput(LAllocation(registers[index].reg));
    } else if (def->policy() == LDefinition::FIXED) {
        
        def->setOutput(*stackLocation(vreg));
    } else {
        
        RegisterIndex best = allocateRegister(ins, vreg);
        registers[best].set(vreg, ins, true);
        registers[best].type = virtualRegisters[vreg]->type();
        def->setOutput(LAllocation(registers[best].reg));
    }
}
