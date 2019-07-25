





































#include "jscntxt.h"
#include "FrameState.h"
#include "FrameState-inl.h"
#include "StubCompiler.h"

using namespace js;
using namespace js::mjit;
using namespace js::analyze;


JS_STATIC_ASSERT(sizeof(FrameEntry) % 8 == 0);

FrameState::FrameState(JSContext *cx, JSScript *script, JSFunction *fun,
                       mjit::Compiler &cc, Assembler &masm, StubCompiler &stubcc,
                       LifetimeScript &liveness)
  : cx(cx), script(script), fun(fun),
    nargs(fun ? fun->nargs : 0),
    masm(masm), stubcc(stubcc), freeRegs(Registers::AvailAnyRegs), entries(NULL),
    activeLoop(NULL), loopRegs(0),
    loopJoins(CompilerAllocPolicy(cx, cc)),
    loopPatches(CompilerAllocPolicy(cx, cc)),
    analysis(NULL), liveness(liveness),
#if defined JS_NUNBOX32
    reifier(cx, *thisFromCtor()),
#endif
    closedVars(NULL),
    closedArgs(NULL),
    usesArguments(script->usesArguments),
    inTryBlock(false)
{
}

FrameState::~FrameState()
{
    cx->free(entries);
}

bool
FrameState::init()
{
    
    uint32 nentries = feLimit();
    if (!nentries) {
        sp = spBase = locals = args = NULL;
        return true;
    }

    eval = script->usesEval || cx->compartment->debugMode;

    size_t totalBytes = sizeof(FrameEntry) * nentries +                     
                        sizeof(FrameEntry *) * nentries +                   
                        sizeof(types::TypeSet *) * script->nslots +         
                        (eval
                         ? 0
                         : sizeof(JSPackedBool) * script->nslots) +         
                        (eval || usesArguments
                         ? 0
                         : sizeof(JSPackedBool) * nargs);                   

    uint8 *cursor = (uint8 *)cx->calloc(totalBytes);
    if (!cursor)
        return false;

#if defined JS_NUNBOX32
    if (!reifier.init(nentries))
        return false;
#endif

    entries = (FrameEntry *)cursor;
    cursor += sizeof(FrameEntry) * nentries;

    callee_ = entries;
    this_ = entries + 1;
    args = entries + 2;
    locals = args + nargs;
    spBase = locals + script->nfixed;
    sp = spBase;

    tracker.entries = (FrameEntry **)cursor;
    cursor += sizeof(FrameEntry *) * nentries;

    typeSets = (types::TypeSet **)cursor;
    cursor += sizeof(types::TypeSet *) * script->nslots;

    if (!eval) {
        if (script->nslots) {
            closedVars = (JSPackedBool *)cursor;
            cursor += sizeof(JSPackedBool) * script->nslots;
        }
        if (!usesArguments && nargs) {
            closedArgs = (JSPackedBool *)cursor;
            cursor += sizeof(JSPackedBool) * nargs;
        }
    }

    JS_ASSERT(reinterpret_cast<uint8 *>(entries) + totalBytes == cursor);

    return true;
}

void
FrameState::takeReg(AnyRegisterID reg)
{
    if (freeRegs.hasReg(reg)) {
        freeRegs.takeReg(reg);
        clearLoopReg(reg);
        JS_ASSERT(!regstate(reg).usedBy());
    } else {
        JS_ASSERT(regstate(reg).fe());
        evictReg(reg);
        regstate(reg).forget();
    }
}

#ifdef DEBUG
const char *
FrameState::entryName(FrameEntry *fe) const
{
    if (fe == this_)
        return "'this'";
    if (fe == callee_)
        return "callee";

    static char buf[50];
    if (isArg(fe))
        JS_snprintf(buf, sizeof(buf), "arg %d", fe - args);
    else if (isLocal(fe))
        JS_snprintf(buf, sizeof(buf), "local %d", fe - locals);
    else
        JS_snprintf(buf, sizeof(buf), "slot %d", fe - spBase);
    return buf;
}
#endif

void
FrameState::evictReg(AnyRegisterID reg)
{
    FrameEntry *fe = regstate(reg).fe();

    JaegerSpew(JSpew_Regalloc, "evicting %s from %s\n", entryName(fe), reg.name());

    if (regstate(reg).type() == RematInfo::TYPE) {
        syncType(fe);
        fe->type.setMemory();
    } else if (reg.isReg()) {
        syncData(fe);
        fe->data.setMemory();
    } else {
        syncFe(fe);
        fe->data.setMemory();
    }
}

inline Lifetime *
FrameState::variableLive(FrameEntry *fe, jsbytecode *pc) const
{
    uint32 offset = pc - script->code;
    if (fe == this_)
        return liveness.thisLive(offset);
    if (isArg(fe)) {
        JS_ASSERT(!isClosedArg(fe - args));
        return liveness.argLive(fe - args, offset);
    }
    if (isLocal(fe)) {
        JS_ASSERT(!isClosedVar(fe - locals));
        return liveness.localLive(fe - locals, offset);
    }

    
    JS_NOT_REACHED("Stack/callee entry");
    return NULL;
}

bool
FrameState::isEntryCopied(FrameEntry *fe) const
{
    



    JS_ASSERT(fe->isCopied());

    for (uint32 i = fe->trackerIndex() + 1; i < tracker.nentries; i++) {
        FrameEntry *nfe = tracker[i];
        if (nfe < sp && nfe->isCopy() && nfe->copyOf() == fe)
            return true;
    }

    return false;
}

AnyRegisterID
FrameState::bestEvictReg(uint32 mask, bool includePinned) const
{
    
    JS_ASSERT((mask & Registers::AvailRegs) != (mask & Registers::AvailFPRegs));

    AnyRegisterID fallback;
    uint32 fallbackOffset = uint32(-1);

    JaegerSpew(JSpew_Regalloc, "picking best register to evict:\n");

    for (uint32 i = 0; i < Registers::TotalAnyRegisters; i++) {
        AnyRegisterID reg = AnyRegisterID::fromRaw(i);

        
        if (!(Registers::maskReg(reg) & mask))
            continue;

        
        FrameEntry *fe = includePinned ? regstate(reg).usedBy() : regstate(reg).fe();
        if (!fe)
            continue;

        






        if (fe == callee_) {
            JS_ASSERT(fe->data.synced() && fe->type.synced());
            JaegerSpew(JSpew_Regalloc, "result: %s is callee\n", reg.name());
            return reg;
        }

        if (fe >= spBase) {
            if (!fallback.isSet()) {
                fallback = reg;
                fallbackOffset = 0;
            }
            JaegerSpew(JSpew_Regalloc, "    %s is on stack\n", reg.name());
            continue;
        }

        




        if (fe->isCopied() && isEntryCopied(fe)) {
            if (!fallback.isSet()) {
                fallback = reg;
                fallbackOffset = 0;
            }
            JaegerSpew(JSpew_Regalloc, "    %s has copies\n", reg.name());
            continue;
        }

        






        Lifetime *lifetime = variableLive(fe, PC);
        if (!lifetime) {
            



            if (!fe->data.synced())
                fe->data.sync();
            if (!fe->type.synced())
                fe->type.sync();
            JaegerSpew(JSpew_Regalloc, "result: %s (%s) is dead\n", entryName(fe), reg.name());
            return reg;
        }

        



        JS_ASSERT_IF(lifetime->loopTail, activeLoop);
        if (lifetime->loopTail && !activeLoop->alloc->hasAnyReg(indexOfFe(fe))) {
            JaegerSpew(JSpew_Regalloc, "result: %s (%s) only live in later iterations\n",
                       entryName(fe), reg.name());
            return reg;
        }

        JaegerSpew(JSpew_Regalloc, "    %s (%s): %u\n", entryName(fe), reg.name(), lifetime->end);

        








        if (!fallback.isSet() || lifetime->end > fallbackOffset) {
            fallback = reg;
            fallbackOffset = lifetime->end;
        }
    }

    JS_ASSERT(fallback.isSet());

    JaegerSpew(JSpew_Regalloc, "result %s\n", fallback.name());
    return fallback;
}

AnyRegisterID
FrameState::evictSomeReg(uint32 mask)
{
    AnyRegisterID reg = bestEvictReg(mask, false);
    evictReg(reg);
    return reg;
}

void
FrameState::resetInternalState()
{
    for (uint32 i = 0; i < tracker.nentries; i++)
        tracker[i]->untrack();

    tracker.reset();
    freeRegs = Registers(Registers::AvailAnyRegs);
}

void
FrameState::discardFrame()
{
    resetInternalState();
    PodArrayZero(regstate_);
}

void
FrameState::forgetEverything()
{
    resetInternalState();

#ifdef DEBUG
    for (uint32 i = 0; i < Registers::TotalAnyRegisters; i++) {
        AnyRegisterID reg = AnyRegisterID::fromRaw(i);
        JS_ASSERT(!regstate(reg).usedBy());
    }
#endif
}

void
FrameState::flushLoopJoins()
{
    for (unsigned i = 0; i < loopPatches.length(); i++) {
        const StubJoinPatch &p = loopPatches[i];
        stubcc.patchJoin(p.join.index, p.join.script, p.address, p.reg);
    }
    loopJoins.clear();
    loopPatches.clear();
}

bool
FrameState::pushLoop(jsbytecode *head, Jump entry, jsbytecode *entryTarget)
{
    if (activeLoop) {
        





        activeLoop->alloc->clearLoops();
        flushLoopJoins();
    }

    LoopState *loop = (LoopState *) cx->calloc(sizeof(*activeLoop));
    if (!loop)
        return false;

    loop->outer = activeLoop;
    loop->head = head;
    loop->entry = entry;
    loop->entryTarget = entryTarget;
    activeLoop = loop;

    RegisterAllocation *&alloc = liveness.getCode(head).allocation;
    JS_ASSERT(!alloc);

    alloc = ArenaNew<RegisterAllocation>(liveness.pool, true);
    if (!alloc)
        return false;

    loop->alloc = alloc;
    loopRegs = Registers::AvailAnyRegs;
    return true;
}

void
FrameState::popLoop(jsbytecode *head, Jump *pjump, jsbytecode **ppc)
{
    JS_ASSERT(activeLoop && activeLoop->head == head && activeLoop->alloc);
    activeLoop->alloc->clearLoops();

#ifdef DEBUG
    if (IsJaegerSpewChannelActive(JSpew_Regalloc)) {
        JaegerSpew(JSpew_Regalloc, "loop allocation at %u:", head - script->code);
        dumpAllocation(activeLoop->alloc);
    }
#endif

    flushLoopJoins();

    activeLoop->entry.linkTo(masm.label(), &masm);
    prepareForJump(activeLoop->entryTarget, masm, true);

    *pjump = masm.jump();
    *ppc = activeLoop->entryTarget;

    LoopState *loop = activeLoop->outer;

    cx->free(activeLoop);
    activeLoop = loop;

    loopRegs = 0;
}

void
FrameState::setLoopReg(AnyRegisterID reg, FrameEntry *fe)
{
    JS_ASSERT(activeLoop && activeLoop->alloc->loop(reg));
    loopRegs.takeReg(reg);

    fe->lastLoop = activeLoop->head;

    uint32 slot = indexOfFe(fe);
    regstate(reg).associate(fe, RematInfo::DATA);

    JaegerSpew(JSpew_Regalloc, "allocating loop register %s for %s\n", reg.name(), entryName(fe));

    activeLoop->alloc->set(reg, slot, true);

    



    for (unsigned i = 0; i < loopJoins.length(); i++) {
        StubJoinPatch p;
        p.join = loopJoins[i];
        p.address = addressOf(fe);
        p.reg = reg;
        loopPatches.append(p);
    }

    if (activeLoop->entryTarget &&
        activeLoop->entryTarget != activeLoop->head &&
        PC >= activeLoop->entryTarget) {
        




        RegisterAllocation *entry = liveness.getCode(activeLoop->entryTarget).allocation;
        JS_ASSERT(entry && !entry->assigned(reg));
        entry->set(reg, slot, true);
    }
}

#ifdef DEBUG
void
FrameState::dumpAllocation(RegisterAllocation *alloc)
{
    for (unsigned i = 0; i < Registers::TotalAnyRegisters; i++) {
        AnyRegisterID reg = AnyRegisterID::fromRaw(i);
        if (alloc->assigned(reg)) {
            printf(" (%s: %s%s)", reg.name(), entryName(entries + alloc->slot(reg)),
                   alloc->synced(reg) ? "" : " unsynced");
        }
    }
    printf("\n");
}
#endif

RegisterAllocation *
FrameState::computeAllocation(jsbytecode *target)
{
    RegisterAllocation *alloc = ArenaNew<RegisterAllocation>(liveness.pool, false);
    if (!alloc)
        return NULL;

    if (analysis->getCode(target).exceptionEntry || analysis->getCode(target).switchTarget ||
        JSOp(*target) == JSOP_TRAP) {
        
        return alloc;
    }

    



    Registers regs = Registers::AvailRegs;
    while (!regs.empty()) {
        AnyRegisterID reg = regs.takeAnyReg();
        if (freeRegs.hasReg(reg) || regstate(reg).type() == RematInfo::TYPE)
            continue;
        FrameEntry *fe = regstate(reg).fe();
        if (fe == callee_ || fe >= spBase || !variableLive(fe, target))
            continue;
        alloc->set(reg, indexOfFe(fe), fe->data.synced());
    }

#ifdef DEBUG
    if (IsJaegerSpewChannelActive(JSpew_Regalloc)) {
        JaegerSpew(JSpew_Regalloc, "allocation at %u:", target - script->code);
        dumpAllocation(alloc);
    }
#endif

    return alloc;
}

void
FrameState::relocateReg(AnyRegisterID reg, RegisterAllocation *alloc, Uses uses)
{
    






    JS_ASSERT(alloc->assigned(reg) && !freeRegs.hasReg(reg));

    for (unsigned i = 0; i < uses.nuses; i++) {
        FrameEntry *fe = peek(-1 - i);
        if (fe->isCopy())
            fe = fe->copyOf();
        if (reg.isReg() && fe->data.inRegister() && fe->data.reg() == reg.reg()) {
            pinReg(reg);
            RegisterID nreg = allocReg();
            unpinReg(reg);

            JaegerSpew(JSpew_Regalloc, "relocating %s\n", reg.name());

            masm.move(reg.reg(), nreg);
            regstate(reg).forget();
            regstate(nreg).associate(fe, RematInfo::DATA);
            fe->data.setRegister(nreg);
            freeRegs.putReg(reg);
            return;
        }
    }

    JaegerSpew(JSpew_Regalloc, "could not relocate %s\n", reg.name());

    takeReg(reg);
    freeRegs.putReg(reg);
}

bool
FrameState::syncForBranch(jsbytecode *target, Uses uses)
{
    
#ifdef DEBUG
    Registers checkRegs(Registers::AvailAnyRegs);
    while (!checkRegs.empty()) {
        AnyRegisterID reg = checkRegs.takeAnyReg();
        JS_ASSERT_IF(!freeRegs.hasReg(reg), regstate(reg).fe());
    }
#endif

    RegisterAllocation *&alloc = liveness.getCode(target).allocation;
    if (!alloc) {
        alloc = computeAllocation(target);
        if (!alloc)
            return false;
    }

    




    for (uint32 i = tracker.nentries - 1; i < tracker.nentries; i--) {
        FrameEntry *fe = tracker[i];

        if (fe >= sp - uses.nuses) {
            
            continue;
        }

        unsigned index = indexOfFe(fe);
        if (!fe->isCopy() && alloc->hasAnyReg(index)) {
            
            if (!fe->isType(JSVAL_TYPE_DOUBLE))
                syncType(fe);
        } else {
            syncFe(fe);
            if (fe->isCopy())
                fe->resetSynced();
        }
    }

    





    Registers regs(Registers::AvailAnyRegs);
    while (!regs.empty()) {
        AnyRegisterID reg = regs.takeAnyReg();
        if (!alloc->assigned(reg))
            continue;
        FrameEntry *fe = getOrTrack(alloc->slot(reg));
        JS_ASSERT(!fe->isCopy());

        JS_ASSERT_IF(!fe->isType(JSVAL_TYPE_DOUBLE), fe->type.synced());
        if (!fe->data.synced() && alloc->synced(reg))
            syncData(fe);

        if (fe->dataInRegister(reg))
            continue;

        if (!freeRegs.hasReg(reg))
            relocateReg(reg, alloc, uses);

        







        if (reg.isReg()) {
            if (fe->isType(JSVAL_TYPE_DOUBLE)) {
                syncFe(fe);
                forgetAllRegs(fe);
                fe->resetSynced();
            }

            RegisterID nreg = reg.reg();
            if (fe->data.inMemory()) {
                masm.loadPayload(addressOf(fe), nreg);
            } else if (fe->isConstant()) {
                masm.loadValuePayload(fe->getValue(), nreg);
            } else {
                JS_ASSERT(fe->data.inRegister() && fe->data.reg() != nreg);
                masm.move(fe->data.reg(), nreg);
                freeRegs.putReg(fe->data.reg());
                regstate(fe->data.reg()).forget();
            }

            fe->data.setRegister(nreg);
        } else {
            JS_ASSERT(fe->isType(JSVAL_TYPE_DOUBLE));

            FPRegisterID nreg = reg.fpreg();
            if (fe->data.inMemory()) {
                masm.loadDouble(addressOf(fe), nreg);
            } else if (fe->isConstant()) {
                masm.slowLoadConstantDouble(fe->getValue().toDouble(), nreg);
            } else {
                JS_ASSERT(fe->data.inFPRegister() && fe->data.fpreg() != nreg);
                masm.moveDouble(fe->data.fpreg(), nreg);
                freeRegs.putReg(fe->data.fpreg());
                regstate(fe->data.fpreg()).forget();
            }

            fe->data.setFPRegister(nreg);
        }

        freeRegs.takeReg(reg);
        regstate(reg).associate(fe, RematInfo::DATA);
    }

    return true;
}

bool
FrameState::discardForJoin(jsbytecode *target, uint32 stackDepth)
{
    RegisterAllocation *&alloc = liveness.getCode(target).allocation;

    if (!alloc) {
        



        alloc = ArenaNew<RegisterAllocation>(liveness.pool, false);
        if (!alloc)
            return false;
    }

    resetInternalState();
    PodArrayZero(regstate_);

    Registers regs(Registers::AvailAnyRegs);
    while (!regs.empty()) {
        AnyRegisterID reg = regs.takeAnyReg();
        if (!alloc->assigned(reg))
            continue;
        FrameEntry *fe = getOrTrack(alloc->slot(reg));

        freeRegs.takeReg(reg);

        



        if (reg.isReg()) {
            fe->data.setRegister(reg.reg());
        } else {
            fe->setType(JSVAL_TYPE_DOUBLE);
            fe->data.setFPRegister(reg.fpreg());
        }

        regstate(reg).associate(fe, RematInfo::DATA);
        if (!alloc->synced(reg))
            fe->data.unsync();
    }

    sp = spBase + stackDepth;

    PodZero(typeSets, stackDepth);

    return true;
}

bool
FrameState::consistentRegisters(jsbytecode *target)
{
    





    RegisterAllocation *alloc = liveness.getCode(target).allocation;
    JS_ASSERT(alloc);

    Registers regs(Registers::AvailAnyRegs);
    while (!regs.empty()) {
        AnyRegisterID reg = regs.takeAnyReg();
        if (alloc->assigned(reg)) {
            FrameEntry *needed = getOrTrack(alloc->slot(reg));
            if (!freeRegs.hasReg(reg)) {
                FrameEntry *fe = regstate(reg).fe();
                if (fe != needed)
                    return false;
            } else {
                return false;
            }
        }
    }

    return true;
}

void
FrameState::prepareForJump(jsbytecode *target, Assembler &masm, bool synced)
{
    JS_ASSERT_IF(!synced, !consistentRegisters(target));

    RegisterAllocation *alloc = liveness.getCode(target).allocation;
    JS_ASSERT(alloc);

    Registers regs(Registers::AvailAnyRegs);
    while (!regs.empty()) {
        AnyRegisterID reg = regs.takeAnyReg();
        if (!alloc->assigned(reg))
            continue;

        const FrameEntry *fe = getOrTrack(alloc->slot(reg));
        if (synced || !fe->backing()->dataInRegister(reg)) {
            JS_ASSERT_IF(!synced, fe->data.synced());
            if (reg.isReg())
                masm.loadPayload(addressOf(fe), reg.reg());
            else
                masm.loadDouble(addressOf(fe), reg.fpreg());
        }
    }
}

void
FrameState::storeTo(FrameEntry *fe, Address address, bool popped)
{
    if (fe->isConstant()) {
        masm.storeValue(fe->getValue(), address);
        return;
    }

    if (fe->isCopy())
        fe = fe->copyOf();

    
    JS_ASSERT(!freeRegs.hasReg(address.base));

    
    JS_ASSERT_IF((fe->type.inMemory() || fe->data.inMemory()),
                 addressOf(fe).base != address.base ||
                 addressOf(fe).offset != address.offset);

    if (fe->data.inFPRegister()) {
        masm.storeDouble(fe->data.fpreg(), address);
        return;
    }

    if (fe->isType(JSVAL_TYPE_DOUBLE)) {
        JS_ASSERT(fe->data.inMemory());
        masm.loadDouble(addressOf(fe), Registers::FPConversionTemp);
        masm.storeDouble(Registers::FPConversionTemp, address);
        return;
    }

#if defined JS_PUNBOX64
    if (fe->type.inMemory() && fe->data.inMemory()) {
        
        RegisterID vreg = Registers::ValueReg;
        masm.loadPtr(addressOf(fe), vreg);
        masm.storePtr(vreg, address);
        return;
    }

    JS_ASSERT(!fe->isType(JSVAL_TYPE_DOUBLE));

    





    bool canPinDreg = true;
    bool wasInRegister = fe->data.inRegister();

    
    MaybeRegisterID dreg;
    if (fe->data.inRegister()) {
        dreg = fe->data.reg();
    } else {
        JS_ASSERT(fe->data.inMemory());
        if (popped) {
            dreg = allocReg();
            masm.loadPayload(addressOf(fe), dreg.reg());
            canPinDreg = false;
        } else {
            dreg = allocAndLoadReg(fe, false, RematInfo::DATA).reg();
            fe->data.setRegister(dreg.reg());
        }
    }
    
    
    if (fe->type.inRegister()) {
        masm.storeValueFromComponents(fe->type.reg(), dreg.reg(), address);
    } else if (fe->isTypeKnown()) {
        masm.storeValueFromComponents(ImmType(fe->getKnownType()), dreg.reg(), address);
    } else {
        JS_ASSERT(fe->type.inMemory());
        if (canPinDreg)
            pinReg(dreg.reg());

        RegisterID treg;
        if (popped) {
            treg = allocReg();
            masm.loadTypeTag(addressOf(fe), treg);
        } else {
            treg = allocAndLoadReg(fe, false, RematInfo::TYPE).reg();
        }
        masm.storeValueFromComponents(treg, dreg.reg(), address);

        if (popped)
            freeReg(treg);
        else
            fe->type.setRegister(treg);

        if (canPinDreg)
            unpinReg(dreg.reg());
    }

    
    if (!wasInRegister && popped)
        freeReg(dreg.reg());

#elif defined JS_NUNBOX32

    if (fe->data.inRegister()) {
        masm.storePayload(fe->data.reg(), address);
    } else {
        JS_ASSERT(fe->data.inMemory());
        RegisterID reg;
        if (popped) {
            reg = allocReg();
            masm.loadPayload(addressOf(fe), reg);
        } else {
            reg = allocAndLoadReg(fe, false, RematInfo::DATA).reg();
        }
        masm.storePayload(reg, address);
        if (popped)
            freeReg(reg);
        else
            fe->data.setRegister(reg);
    }

    if (fe->isTypeKnown()) {
        masm.storeTypeTag(ImmType(fe->getKnownType()), address);
    } else if (fe->type.inRegister()) {
        masm.storeTypeTag(fe->type.reg(), address);
    } else {
        JS_ASSERT(fe->type.inMemory());
        RegisterID reg;
        if (popped) {
            reg = allocReg();
            masm.loadTypeTag(addressOf(fe), reg);
        } else {
            reg = allocAndLoadReg(fe, false, RematInfo::TYPE).reg();
        }
        masm.storeTypeTag(reg, address);
        if (popped)
            freeReg(reg);
        else
            fe->type.setRegister(reg);
    }
#endif
}

void
FrameState::loadThisForReturn(RegisterID typeReg, RegisterID dataReg, RegisterID tempReg)
{
    return loadForReturn(getThis(), typeReg, dataReg, tempReg);
}

void FrameState::loadForReturn(FrameEntry *fe, RegisterID typeReg, RegisterID dataReg, RegisterID tempReg)
{
    JS_ASSERT(dataReg != typeReg && dataReg != tempReg && typeReg != tempReg);

    if (fe->isConstant()) {
        masm.loadValueAsComponents(fe->getValue(), typeReg, dataReg);
        return;
    }

    if (fe->isType(JSVAL_TYPE_DOUBLE)) {
        FPRegisterID fpreg = tempFPRegForData(fe);
        masm.breakDouble(fpreg, typeReg, dataReg);
        return;
    }

    if (fe->isCopy())
        fe = fe->copyOf();

    MaybeRegisterID maybeType = maybePinType(fe);
    MaybeRegisterID maybeData = maybePinData(fe);

    if (fe->isTypeKnown()) {
        
        if (!maybeData.isSet())
            masm.loadPayload(addressOf(fe), dataReg);
        else if (maybeData.reg() != dataReg)
            masm.move(maybeData.reg(), dataReg);
        masm.move(ImmType(fe->getKnownType()), typeReg);
        return;
    }

    
    
    if (fe->type.inMemory() && fe->data.inMemory()) {
        masm.loadValueAsComponents(addressOf(fe), typeReg, dataReg);
        return;
    }

    
    JS_ASSERT(maybeType.isSet() || maybeData.isSet());

    
    
    
    if (!maybeType.isSet()) {
        JS_ASSERT(maybeData.isSet());
        if (maybeData.reg() != typeReg)
            maybeType = typeReg;
        else
            maybeType = tempReg;
        masm.loadTypeTag(addressOf(fe), maybeType.reg());
    } else if (!maybeData.isSet()) {
        JS_ASSERT(maybeType.isSet());
        if (maybeType.reg() != dataReg)
            maybeData = dataReg;
        else
            maybeData = tempReg;
        masm.loadPayload(addressOf(fe), maybeData.reg());
    }

    RegisterID type = maybeType.reg();
    RegisterID data = maybeData.reg();

    if (data == typeReg && type == dataReg) {
        masm.move(type, tempReg);
        masm.move(data, dataReg);
        masm.move(tempReg, typeReg);
    } else if (data != dataReg) {
        if (type == typeReg) {
            masm.move(data, dataReg);
        } else if (type != dataReg) {
            masm.move(data, dataReg);
            if (type != typeReg)
                masm.move(type, typeReg);
        } else {
            JS_ASSERT(data != typeReg);
            masm.move(type, typeReg);
            masm.move(data, dataReg);
        }
    } else if (type != typeReg) {
        masm.move(type, typeReg);
    }
}

#ifdef DEBUG
void
FrameState::assertValidRegisterState() const
{
    Registers checkedFreeRegs(Registers::AvailAnyRegs);

    for (uint32 i = 0; i < tracker.nentries; i++) {
        FrameEntry *fe = tracker[i];
        if (fe >= sp)
            continue;

        JS_ASSERT(i == fe->trackerIndex());
        JS_ASSERT_IF(fe->isCopy(),
                     fe->trackerIndex() > fe->copyOf()->trackerIndex());
        JS_ASSERT_IF(fe->isCopy(), fe > fe->copyOf());
        JS_ASSERT_IF(fe->isCopy(),
                     !fe->type.inRegister() && !fe->data.inRegister() && !fe->data.inFPRegister());
        JS_ASSERT_IF(fe->isCopy(), fe->copyOf() < sp);
        JS_ASSERT_IF(fe->isCopy(), fe->copyOf()->isCopied());

        if (fe->isCopy())
            continue;
        if (fe->type.inRegister()) {
            checkedFreeRegs.takeReg(fe->type.reg());
            JS_ASSERT(regstate(fe->type.reg()).fe() == fe);
        }
        if (fe->data.inRegister()) {
            checkedFreeRegs.takeReg(fe->data.reg());
            JS_ASSERT(regstate(fe->data.reg()).fe() == fe);
        }
        if (fe->data.inFPRegister()) {
            JS_ASSERT(fe->isType(JSVAL_TYPE_DOUBLE));
            checkedFreeRegs.takeReg(fe->data.fpreg());
            JS_ASSERT(regstate(fe->data.fpreg()).fe() == fe);
        }
    }

    JS_ASSERT(checkedFreeRegs == freeRegs);

    for (uint32 i = 0; i < Registers::TotalRegisters; i++) {
        AnyRegisterID reg = (RegisterID) i;
        JS_ASSERT(!regstate(reg).isPinned());
        JS_ASSERT_IF(regstate(reg).fe(), !freeRegs.hasReg(reg));
        JS_ASSERT_IF(regstate(reg).fe(), regstate(reg).fe()->isTracked());
    }

    for (uint32 i = 0; i < Registers::TotalFPRegisters; i++) {
        AnyRegisterID reg = (FPRegisterID) i;
        JS_ASSERT(!regstate(reg).isPinned());
        JS_ASSERT_IF(regstate(reg).fe(), !freeRegs.hasReg(reg));
        JS_ASSERT_IF(regstate(reg).fe(), regstate(reg).fe()->isTracked());
        JS_ASSERT_IF(regstate(reg).fe(), regstate(reg).type() == RematInfo::DATA);
    }
}
#endif

#if defined JS_NUNBOX32
void
FrameState::syncFancy(Assembler &masm, Registers avail, FrameEntry *resumeAt,
                      FrameEntry *bottom) const
{
    reifier.reset(&masm, avail, resumeAt, bottom);

    for (FrameEntry *fe = resumeAt; fe >= bottom; fe--) {
        if (!fe->isTracked())
            continue;

        reifier.sync(fe);
    }
}
#endif

void
FrameState::sync(Assembler &masm, Uses uses) const
{
    if (!entries)
        return;

    
    Registers allRegs(Registers::AvailAnyRegs);
    while (!allRegs.empty()) {
        AnyRegisterID reg = allRegs.takeAnyReg();
        FrameEntry *fe = regstate(reg).usedBy();
        if (!fe)
            continue;

        JS_ASSERT(fe->isTracked());

#if defined JS_PUNBOX64
        
        ensureFeSynced(fe, masm);

        
        if (regstate(reg).type() == RematInfo::DATA && fe->type.inRegister())
            allRegs.takeReg(fe->type.reg());
        else if (regstate(reg).type() == RematInfo::TYPE && fe->data.inRegister())
            allRegs.takeReg(fe->data.reg());
#elif defined JS_NUNBOX32
        
        if (fe->isType(JSVAL_TYPE_DOUBLE)) {
            ensureFeSynced(fe, masm);
        } else if (regstate(reg).type() == RematInfo::DATA) {
            JS_ASSERT(fe->data.reg() == reg.reg());
            ensureDataSynced(fe, masm);
        } else {
            JS_ASSERT(fe->type.reg() == reg.reg());
            ensureTypeSynced(fe, masm);
        }
#endif
    }

    



    Registers avail(freeRegs.freeMask & Registers::AvailRegs);
    Registers temp(Registers::TempAnyRegs);

    for (FrameEntry *fe = sp - 1; fe >= entries; fe--) {
        if (!fe->isTracked())
            continue;

        if (fe->isType(JSVAL_TYPE_DOUBLE)) {
            
            ensureFeSynced(fe, masm);
            continue;
        }

        FrameEntry *backing = fe;

        if (!fe->isCopy()) {
            if (fe->data.inRegister())
                avail.putReg(fe->data.reg());
            if (fe->type.inRegister())
                avail.putReg(fe->type.reg());
        } else {
            backing = fe->copyOf();
            JS_ASSERT(!backing->isConstant() && !fe->isConstant());

#if defined JS_PUNBOX64
            if ((!fe->type.synced() && backing->type.inMemory()) ||
                (!fe->data.synced() && backing->data.inMemory())) {
    
                RegisterID syncReg = Registers::ValueReg;

                
                if (backing->type.synced() && backing->data.synced()) {
                    masm.loadValue(addressOf(backing), syncReg);
                } else if (backing->type.inMemory()) {
                    masm.loadTypeTag(addressOf(backing), syncReg);
                    masm.orPtr(backing->data.reg(), syncReg);
                } else {
                    JS_ASSERT(backing->data.inMemory());
                    masm.loadPayload(addressOf(backing), syncReg);
                    if (backing->isTypeKnown())
                        masm.orPtr(ImmType(backing->getKnownType()), syncReg);
                    else
                        masm.orPtr(backing->type.reg(), syncReg);
                }

                masm.storeValue(syncReg, addressOf(fe));
                continue;
            }
#elif defined JS_NUNBOX32
            
            if ((!fe->type.synced() && backing->type.inMemory()) ||
                (!fe->data.synced() && backing->data.inMemory())) {
                syncFancy(masm, avail, fe, entries);
                return;
            }
#endif
        }

        
#if defined JS_PUNBOX64
        
        if (!fe->type.inRegister() && !fe->data.inRegister())
            ensureFeSynced(fe, masm);
#elif defined JS_NUNBOX32
        
        if (!fe->data.inRegister())
            ensureDataSynced(fe, masm);
        if (!fe->type.inRegister())
            ensureTypeSynced(fe, masm);
#endif
    }
}

void
FrameState::syncAndKill(Registers kill, Uses uses, Uses ignore)
{
    if (activeLoop) {
        



        activeLoop->alloc->clearLoops();
        loopRegs = 0;
    }

    FrameEntry *spStop = sp - ignore.nuses;

    
    Registers search(kill.freeMask & ~freeRegs.freeMask);
    while (!search.empty()) {
        AnyRegisterID reg = search.takeAnyReg();
        FrameEntry *fe = regstate(reg).usedBy();
        if (!fe || fe >= spStop)
            continue;

        JS_ASSERT(fe->isTracked());

#if defined JS_PUNBOX64
        
        ensureFeSynced(fe, masm);

        if (!fe->type.synced())
            fe->type.sync();
        if (!fe->data.synced())
            fe->data.sync();

        
        if (regstate(reg).type() == RematInfo::DATA) {
            if (!fe->isType(JSVAL_TYPE_DOUBLE)) {
                JS_ASSERT(fe->data.reg() == reg.reg());
                if (fe->type.inRegister() && search.hasReg(fe->type.reg()))
                    search.takeReg(fe->type.reg());
            }
        } else {
            JS_ASSERT(fe->type.reg() == reg.reg());
            if (fe->data.inRegister() && search.hasReg(fe->data.reg()))
                search.takeReg(fe->data.reg());
        }
#elif defined JS_NUNBOX32
        
        if (fe->isType(JSVAL_TYPE_DOUBLE)) {
            syncFe(fe);
        } else if (regstate(reg).type() == RematInfo::DATA) {
            JS_ASSERT(fe->data.reg() == reg.reg());
            syncData(fe);
        } else {
            JS_ASSERT(fe->type.reg() == reg.reg());
            syncType(fe);
        }
#endif
    }

    uint32 maxvisits = tracker.nentries;

    for (FrameEntry *fe = sp - 1; fe >= entries && maxvisits; fe--) {
        if (!fe->isTracked())
            continue;

        maxvisits--;

        if (fe >= spStop)
            continue;

        syncFe(fe);

        
        if (fe->data.inRegister() && !regstate(fe->data.reg()).isPinned()) {
            forgetReg(fe->data.reg());
            fe->data.setMemory();
        }
        if (fe->data.inFPRegister() && !regstate(fe->data.fpreg()).isPinned()) {
            forgetReg(fe->data.fpreg());
            fe->data.setMemory();
        }
        if (fe->type.inRegister() && !regstate(fe->type.reg()).isPinned()) {
            forgetReg(fe->type.reg());
            fe->type.setMemory();
        }
    }

    



    search = Registers(kill.freeMask & ~freeRegs.freeMask);
    while (!search.empty()) {
        AnyRegisterID reg = search.takeAnyReg();
        FrameEntry *fe = regstate(reg).usedBy();
        if (!fe || fe >= spStop)
            continue;

        JS_ASSERT(fe->isTracked() && !fe->isType(JSVAL_TYPE_DOUBLE));

        if (regstate(reg).type() == RematInfo::DATA) {
            JS_ASSERT(fe->data.reg() == reg.reg());
            JS_ASSERT(fe->data.synced());
            fe->data.setMemory();
        } else {
            JS_ASSERT(fe->type.reg() == reg.reg());
            JS_ASSERT(fe->type.synced());
            fe->type.setMemory();
        }

        forgetReg(reg);
    }
}

void
FrameState::merge(Assembler &masm, Changes changes) const
{
    





    





    for (unsigned i = 0; i < changes.nchanges; i++) {
        FrameEntry *fe = sp - 1 - i;
        if (fe->isType(JSVAL_TYPE_DOUBLE))
            masm.ensureInMemoryDouble(addressOf(fe));
    }

    uint32 mask = Registers::AvailAnyRegs & ~freeRegs.freeMask;
    Registers search(mask);

    while (!search.empty(mask)) {
        AnyRegisterID reg = search.peekReg(mask);
        FrameEntry *fe = regstate(reg).usedBy();

        if (!fe) {
            search.takeReg(reg);
            continue;
        }

        if (fe->isType(JSVAL_TYPE_DOUBLE)) {
            JS_ASSERT(fe->data.fpreg() == reg.fpreg());
            search.takeReg(fe->data.fpreg());
            masm.loadDouble(addressOf(fe), fe->data.fpreg());
        } else if (fe->data.inRegister() && fe->type.inRegister()) {
            search.takeReg(fe->data.reg());
            search.takeReg(fe->type.reg());
            masm.loadValueAsComponents(addressOf(fe), fe->type.reg(), fe->data.reg());
        } else {
            if (fe->data.inRegister()) {
                search.takeReg(fe->data.reg());
                masm.loadPayload(addressOf(fe), fe->data.reg());
            }
            if (fe->type.inRegister()) {
                search.takeReg(fe->type.reg());
                masm.loadTypeTag(addressOf(fe), fe->type.reg());
            }
        }
    }
}

JSC::MacroAssembler::RegisterID
FrameState::copyDataIntoReg(FrameEntry *fe)
{
    return copyDataIntoReg(this->masm, fe);
}

void
FrameState::copyDataIntoReg(FrameEntry *fe, RegisterID hint)
{
    JS_ASSERT(!fe->data.isConstant());
    JS_ASSERT(!fe->isType(JSVAL_TYPE_DOUBLE));

    if (fe->isCopy())
        fe = fe->copyOf();

    if (!fe->data.inRegister())
        tempRegForData(fe);

    RegisterID reg = fe->data.reg();
    if (reg == hint) {
        if (freeRegs.empty(Registers::AvailRegs)) {
            ensureDataSynced(fe, masm);
            fe->data.setMemory();
        } else {
            reg = allocReg();
            masm.move(hint, reg);
            fe->data.setRegister(reg);
            regstate(reg).associate(regstate(hint).fe(), RematInfo::DATA);
        }
        regstate(hint).forget();
    } else {
        pinReg(reg);
        takeReg(hint);
        unpinReg(reg);
        masm.move(reg, hint);
    }
}

JSC::MacroAssembler::RegisterID
FrameState::copyDataIntoReg(Assembler &masm, FrameEntry *fe)
{
    JS_ASSERT(!fe->data.isConstant());

    if (fe->isCopy())
        fe = fe->copyOf();

    if (fe->data.inRegister()) {
        RegisterID reg = fe->data.reg();
        if (freeRegs.empty(Registers::AvailRegs)) {
            ensureDataSynced(fe, masm);
            fe->data.setMemory();
            regstate(reg).forget();
        } else {
            RegisterID newReg = allocReg();
            masm.move(reg, newReg);
            reg = newReg;
        }
        return reg;
    }

    RegisterID reg = allocReg();

    if (!freeRegs.empty(Registers::AvailRegs))
        masm.move(tempRegForData(fe), reg);
    else
        masm.loadPayload(addressOf(fe),reg);

    return reg;
}

JSC::MacroAssembler::RegisterID
FrameState::copyTypeIntoReg(FrameEntry *fe)
{
    JS_ASSERT(!fe->type.isConstant());

    if (fe->isCopy())
        fe = fe->copyOf();

    if (fe->type.inRegister()) {
        RegisterID reg = fe->type.reg();
        if (freeRegs.empty(Registers::AvailRegs)) {
            ensureTypeSynced(fe, masm);
            fe->type.setMemory();
            regstate(reg).forget();
        } else {
            RegisterID newReg = allocReg();
            masm.move(reg, newReg);
            reg = newReg;
        }
        return reg;
    }

    RegisterID reg = allocReg();

    if (!freeRegs.empty(Registers::AvailRegs))
        masm.move(tempRegForType(fe), reg);
    else
        masm.loadTypeTag(addressOf(fe), reg);

    return reg;
}

JSC::MacroAssembler::RegisterID
FrameState::copyInt32ConstantIntoReg(FrameEntry *fe)
{
    return copyInt32ConstantIntoReg(masm, fe);
}

JSC::MacroAssembler::RegisterID
FrameState::copyInt32ConstantIntoReg(Assembler &masm, FrameEntry *fe)
{
    JS_ASSERT(fe->data.isConstant());

    if (fe->isCopy())
        fe = fe->copyOf();

    RegisterID reg = allocReg();
    masm.move(Imm32(fe->getValue().toInt32()), reg);
    return reg;
}

JSC::MacroAssembler::RegisterID
FrameState::ownRegForType(FrameEntry *fe)
{
    JS_ASSERT(!fe->type.isConstant());

    RegisterID reg;
    if (fe->isCopy()) {
        
        FrameEntry *backing = fe->copyOf();
        if (!backing->type.inRegister()) {
            JS_ASSERT(backing->type.inMemory());
            tempRegForType(backing);
        }

        if (freeRegs.empty(Registers::AvailRegs)) {
            
            ensureTypeSynced(backing, masm);
            reg = backing->type.reg();
            backing->type.setMemory();
            regstate(reg).forget();
        } else {
            reg = allocReg();
            masm.move(backing->type.reg(), reg);
        }
        return reg;
    }

    if (fe->type.inRegister()) {
        reg = fe->type.reg();

        
        JS_ASSERT(regstate(reg).fe() == fe);
        JS_ASSERT(regstate(reg).type() == RematInfo::TYPE);
        regstate(reg).forget();
        fe->type.invalidate();
    } else {
        JS_ASSERT(fe->type.inMemory());
        reg = allocReg();
        masm.loadTypeTag(addressOf(fe), reg);
    }
    return reg;
}

JSC::MacroAssembler::RegisterID
FrameState::ownRegForData(FrameEntry *fe)
{
    JS_ASSERT(!fe->data.isConstant());
    JS_ASSERT(!fe->isType(JSVAL_TYPE_DOUBLE));

    RegisterID reg;
    if (fe->isCopy()) {
        
        FrameEntry *backing = fe->copyOf();
        if (!backing->data.inRegister()) {
            JS_ASSERT(backing->data.inMemory());
            tempRegForData(backing);
        }

        if (freeRegs.empty(Registers::AvailRegs)) {
            
            ensureDataSynced(backing, masm);
            reg = backing->data.reg();
            backing->data.setMemory();
            regstate(reg).forget();
        } else {
            reg = allocReg();
            masm.move(backing->data.reg(), reg);
        }
        return reg;
    }

    if (fe->isCopied()) {
        FrameEntry *copy = uncopy(fe);
        if (fe->isCopied()) {
            fe->type.invalidate();
            fe->data.invalidate();
            return copyDataIntoReg(copy);
        }
    }
    
    if (fe->data.inRegister()) {
        reg = fe->data.reg();
        
        JS_ASSERT(regstate(reg).fe() == fe);
        JS_ASSERT(regstate(reg).type() == RematInfo::DATA);
        regstate(reg).forget();
        fe->data.invalidate();
    } else {
        JS_ASSERT(fe->data.inMemory());
        reg = allocReg();
        masm.loadPayload(addressOf(fe), reg);
    }
    return reg;
}

void
FrameState::discardFe(FrameEntry *fe)
{
    forgetEntry(fe);
    fe->type.setMemory();
    fe->data.setMemory();
}

void
FrameState::pushDouble(FPRegisterID fpreg)
{
    FrameEntry *fe = rawPush();
    fe->resetUnsynced();
    fe->setType(JSVAL_TYPE_DOUBLE);
    fe->data.setFPRegister(fpreg);
    regstate(fpreg).associate(fe, RematInfo::DATA);
}

void
FrameState::pushDouble(Address address)
{
    FPRegisterID fpreg = allocFPReg();
    masm.loadDouble(address, fpreg);
    pushDouble(fpreg);
}

void
FrameState::ensureDouble(FrameEntry *fe)
{
    if (fe->isConstant()) {
        JS_ASSERT(fe->getValue().isInt32());
        Value newValue = DoubleValue(double(fe->getValue().toInt32()));
        fe->setConstant(Jsvalify(newValue));
        return;
    }

    if (fe->isType(JSVAL_TYPE_DOUBLE))
        return;

    FrameEntry *backing = fe;
    if (fe->isCopy()) {
        
        backing = fe->copyOf();
        fe->clear();
    } else if (fe->isCopied()) {
        
        for (uint32 i = fe->trackerIndex() + 1; i < tracker.nentries; i++) {
            FrameEntry *nfe = tracker[i];
            if (nfe < sp && nfe->isCopy() && nfe->copyOf() == fe) {
                syncFe(nfe);
                nfe->resetSynced();
            }
        }
    }

    FPRegisterID fpreg = allocFPReg();

    if (backing->isType(JSVAL_TYPE_INT32)) {
        RegisterID data = tempRegForData(backing);
        masm.convertInt32ToDouble(data, fpreg);
    } else {
        syncFe(backing);
        masm.moveInt32OrDouble(addressOf(backing), fpreg);
    }

    forgetAllRegs(fe);
    fe->resetUnsynced();
    fe->setType(JSVAL_TYPE_DOUBLE);
    fe->data.setFPRegister(fpreg);
    regstate(fpreg).associate(fe, RematInfo::DATA);

    fe->data.unsync();
    fe->type.unsync();
}

void
FrameState::pushCopyOf(uint32 index)
{
    FrameEntry *backing = entryFor(index);
    FrameEntry *fe = rawPush();
    fe->resetUnsynced();
    if (backing->isConstant()) {
        fe->setConstant(Jsvalify(backing->getValue()));
    } else {
        if (backing->isTypeKnown())
            fe->setType(backing->getKnownType());
        else
            fe->type.invalidate();
        fe->data.invalidate();
        if (backing->isCopy()) {
            backing = backing->copyOf();
            fe->setCopyOf(backing);
        } else {
            fe->setCopyOf(backing);
            backing->setCopied();
        }

        
        JS_ASSERT(backing->isCopied());
        if (fe->trackerIndex() < backing->trackerIndex())
            swapInTracker(fe, backing);
    }
}

FrameEntry *
FrameState::walkTrackerForUncopy(FrameEntry *original)
{
    uint32 firstCopy = InvalidIndex;
    FrameEntry *bestFe = NULL;
    uint32 ncopies = 0;
    for (uint32 i = original->trackerIndex() + 1; i < tracker.nentries; i++) {
        FrameEntry *fe = tracker[i];
        if (fe >= sp)
            continue;
        if (fe->isCopy() && fe->copyOf() == original) {
            if (firstCopy == InvalidIndex) {
                firstCopy = i;
                bestFe = fe;
            } else if (fe < bestFe) {
                bestFe = fe;
            }
            ncopies++;
        }
    }

    if (!ncopies) {
        JS_ASSERT(firstCopy == InvalidIndex);
        JS_ASSERT(!bestFe);
        return NULL;
    }

    JS_ASSERT(firstCopy != InvalidIndex);
    JS_ASSERT(bestFe);
    JS_ASSERT(bestFe > original);

    
    bestFe->setCopyOf(NULL);
    if (ncopies > 1) {
        bestFe->setCopied();
        for (uint32 i = firstCopy; i < tracker.nentries; i++) {
            FrameEntry *other = tracker[i];
            if (other >= sp || other == bestFe)
                continue;

            
            JS_ASSERT(other != original);

            if (!other->isCopy() || other->copyOf() != original)
                continue;

            other->setCopyOf(bestFe);

            






            if (other->trackerIndex() < bestFe->trackerIndex())
                swapInTracker(bestFe, other);
        }
    } else {
        bestFe->setNotCopied();
    }

    return bestFe;
}

FrameEntry *
FrameState::walkFrameForUncopy(FrameEntry *original)
{
    FrameEntry *bestFe = NULL;
    uint32 ncopies = 0;

    
    uint32 maxvisits = tracker.nentries;

    for (FrameEntry *fe = original + 1; fe < sp && maxvisits; fe++) {
        if (!fe->isTracked())
            continue;

        maxvisits--;

        if (fe->isCopy() && fe->copyOf() == original) {
            if (!bestFe) {
                bestFe = fe;
                bestFe->setCopyOf(NULL);
            } else {
                fe->setCopyOf(bestFe);
                if (fe->trackerIndex() < bestFe->trackerIndex())
                    swapInTracker(bestFe, fe);
            }
            ncopies++;
        }
    }

    if (ncopies)
        bestFe->setCopied();

    return bestFe;
}

FrameEntry *
FrameState::uncopy(FrameEntry *original)
{
    JS_ASSERT(original->isCopied());

    






















    FrameEntry *fe;
    if ((tracker.nentries - original->trackerIndex()) * 2 > uint32(sp - original))
        fe = walkFrameForUncopy(original);
    else
        fe = walkTrackerForUncopy(original);
    if (!fe) {
        original->setNotCopied();
        return NULL;
    }

    




    if (!original->isTypeKnown()) {
        




        if (original->type.inMemory() && !fe->type.synced())
            tempRegForType(original);
        fe->type.inherit(original->type);
        if (fe->type.inRegister())
            regstate(fe->type.reg()).reassociate(fe);
    } else {
        JS_ASSERT(fe->isTypeKnown());
        JS_ASSERT(fe->getKnownType() == original->getKnownType());
    }
    if (original->isType(JSVAL_TYPE_DOUBLE)) {
        if (original->data.inMemory() && !fe->data.synced())
            tempFPRegForData(original);
        fe->data.inherit(original->data);
        if (fe->data.inFPRegister())
            regstate(fe->data.fpreg()).reassociate(fe);
    } else {
        if (original->data.inMemory() && !fe->data.synced())
            tempRegForData(original);
        fe->data.inherit(original->data);
        if (fe->data.inRegister())
            regstate(fe->data.reg()).reassociate(fe);
    }

    return fe;
}

bool
FrameState::hasOnlyCopy(FrameEntry *backing, FrameEntry *fe)
{
    JS_ASSERT(backing->isCopied() && fe->copyOf() == backing);

    for (uint32 i = backing->trackerIndex() + 1; i < tracker.nentries; i++) {
        FrameEntry *nfe = tracker[i];
        if (nfe != fe && nfe < sp && nfe->isCopy() && nfe->copyOf() == backing)
            return false;
    }

    return true;
}

void
FrameState::storeLocal(uint32 n, JSValueType type, bool popGuaranteed, bool fixedType)
{
    FrameEntry *local = getLocal(n);

    if (isClosedVar(n)) {
        JS_ASSERT(local->data.inMemory());
        storeTo(peek(-1), addressOf(local), popGuaranteed);
        return;
    }

    storeTop(local, type, popGuaranteed);

    if (activeLoop)
        local->lastLoop = activeLoop->head;

    if (type != JSVAL_TYPE_UNKNOWN && type != JSVAL_TYPE_DOUBLE &&
        fixedType && !local->type.synced()) {
        
        local->type.sync();
    }

    if (inTryBlock)
        syncFe(local);
}

void
FrameState::storeArg(uint32 n, JSValueType type, bool popGuaranteed)
{
    
    
    FrameEntry *arg = getArg(n);

    if (isClosedArg(n)) {
        JS_ASSERT(arg->data.inMemory());
        storeTo(peek(-1), addressOf(arg), popGuaranteed);
        return;
    }

    storeTop(arg, type, popGuaranteed);

    if (activeLoop)
        arg->lastLoop = activeLoop->head;

    if (type != JSVAL_TYPE_UNKNOWN && type != JSVAL_TYPE_DOUBLE && !arg->type.synced()) {
        
        arg->type.sync();
    }

    syncFe(arg);
}

void
FrameState::forgetEntry(FrameEntry *fe)
{
    if (fe->isCopied()) {
        uncopy(fe);
        if (!fe->isCopied())
            forgetAllRegs(fe);
    } else {
        forgetAllRegs(fe);
    }

    if (fe >= sp)
        typeSets[fe - spBase] = NULL;
}

void
FrameState::storeTop(FrameEntry *target, JSValueType type, bool popGuaranteed)
{
    
    FrameEntry *top = peek(-1);
    if (top->isCopy() && top->copyOf() == target) {
        JS_ASSERT(target->isCopied());
        return;
    }

    
    forgetEntry(target);
    target->resetUnsynced();

    
    if (top->isConstant()) {
        target->setCopyOf(NULL);
        target->setNotCopied();
        target->setConstant(Jsvalify(top->getValue()));
        return;
    }

    












    FrameEntry *backing = top;
    bool copied = false;
    if (top->isCopy()) {
        backing = top->copyOf();
        JS_ASSERT(backing->trackerIndex() < top->trackerIndex());

        if (backing < target) {
            
            if (target->trackerIndex() < backing->trackerIndex())
                swapInTracker(backing, target);
            target->setNotCopied();
            target->setCopyOf(backing);
            if (backing->isTypeKnown())
                target->setType(backing->getKnownType());
            else
                target->type.invalidate();
            target->data.invalidate();
            return;
        }

        

















        for (uint32 i = backing->trackerIndex() + 1; i < tracker.nentries; i++) {
            FrameEntry *fe = tracker[i];
            if (fe >= sp)
                continue;
            if (fe->isCopy() && fe->copyOf() == backing) {
                fe->setCopyOf(target);
                copied = true;
            }
        }
    }
    backing->setNotCopied();
    
    




    if (backing->trackerIndex() < target->trackerIndex())
        swapInTracker(backing, target);

    if (backing->isType(JSVAL_TYPE_DOUBLE)) {
        FPRegisterID fpreg = tempFPRegForData(backing);
        if (type == JSVAL_TYPE_UNKNOWN) {
            masm.storeDouble(fpreg, addressOf(target));
            target->resetSynced();

            
            forgetReg(fpreg);
        } else {
            JS_ASSERT(type == JSVAL_TYPE_DOUBLE);
            target->data.setFPRegister(fpreg);
            regstate(fpreg).reassociate(target);
        }

        target->setType(JSVAL_TYPE_DOUBLE);
    } else {
        



        RegisterID reg = tempRegForData(backing);
        target->data.setRegister(reg);
        regstate(reg).reassociate(target);

        if (type == JSVAL_TYPE_UNKNOWN) {
            if (backing->isTypeKnown()) {
                target->setType(backing->getKnownType());
            } else {
                RegisterID reg = tempRegForType(backing);
                target->type.setRegister(reg);
                regstate(reg).reassociate(target);
            }
        } else if (type != JSVAL_TYPE_DOUBLE || backing->isType(JSVAL_TYPE_INT32)) {
            




            if (type == JSVAL_TYPE_DOUBLE)
                type = JSVAL_TYPE_INT32;
            JS_ASSERT_IF(backing->isTypeKnown(), backing->isType(type));
            if (!backing->isTypeKnown())
                learnType(backing, type);
            target->setType(type);
        } else {
            FPRegisterID fpreg = allocFPReg();
            syncFe(backing);
            masm.moveInt32OrDouble(addressOf(backing), fpreg);

            forgetAllRegs(backing);

            backing->setType(JSVAL_TYPE_DOUBLE);
            target->setType(JSVAL_TYPE_DOUBLE);
            target->data.setFPRegister(fpreg);
            regstate(fpreg).associate(target, RematInfo::DATA);
        }
    }

    if (!backing->isTypeKnown())
        backing->type.invalidate();
    backing->data.invalidate();
    backing->setCopyOf(target);

    JS_ASSERT(top->copyOf() == target);

    








    if (copied || !popGuaranteed)
        target->setCopied();
}

void
FrameState::shimmy(uint32 n)
{
    JS_ASSERT(sp - n >= spBase);
    int32 depth = 0 - int32(n);
    storeTop(peek(depth - 1), JSVAL_TYPE_UNKNOWN, true);
    popn(n);
}

void
FrameState::shift(int32 n)
{
    JS_ASSERT(n < 0);
    JS_ASSERT(sp + n - 1 >= spBase);
    storeTop(peek(n - 1), JSVAL_TYPE_UNKNOWN, true);
    pop();
}

void
FrameState::forgetKnownDouble(FrameEntry *fe)
{
    




    JS_ASSERT(!fe->isConstant() && fe->isType(JSVAL_TYPE_DOUBLE));

    RegisterID typeReg = allocReg();
    RegisterID dataReg = allocReg();

    
    FPRegisterID fpreg = allocFPReg();
    masm.moveDouble(tempFPRegForData(fe), fpreg);
    masm.breakDouble(fpreg, typeReg, dataReg);

    forgetAllRegs(fe);
    fe->resetUnsynced();

    regstate(typeReg).associate(fe, RematInfo::TYPE);
    regstate(dataReg).associate(fe, RematInfo::DATA);
    fe->type.setRegister(typeReg);
    fe->data.setRegister(dataReg);
    freeReg(fpreg);
}

void
FrameState::pinEntry(FrameEntry *fe, ValueRemat &vr)
{
    if (fe->isConstant()) {
        vr = ValueRemat::FromConstant(fe->getValue());
    } else {
        if (fe->isType(JSVAL_TYPE_DOUBLE))
            forgetKnownDouble(fe);

        
        MaybeRegisterID maybePinnedType = maybePinType(fe);

        
        RegisterID dataReg = tempRegForData(fe);
        pinReg(dataReg);

        if (fe->isTypeKnown()) {
            vr = ValueRemat::FromKnownType(fe->getKnownType(), dataReg);
        } else {
            
            maybeUnpinReg(maybePinnedType);

            vr = ValueRemat::FromRegisters(tempRegForType(fe), dataReg);
            pinReg(vr.typeReg());
        }
    }

    
    vr.isDataSynced = fe->data.synced();
    vr.isTypeSynced = fe->type.synced();
}

void
FrameState::unpinEntry(const ValueRemat &vr)
{
    if (!vr.isConstant()) {
        if (!vr.isTypeKnown())
            unpinReg(vr.typeReg());
        unpinReg(vr.dataReg());
    }
}

void
FrameState::ensureValueSynced(Assembler &masm, FrameEntry *fe, const ValueRemat &vr)
{
#if defined JS_PUNBOX64
    if (!vr.isDataSynced || !vr.isTypeSynced)
        masm.storeValue(vr, addressOf(fe));
#elif defined JS_NUNBOX32
    if (vr.isConstant()) {
        if (!vr.isDataSynced || !vr.isTypeSynced)
            masm.storeValue(vr.value(), addressOf(fe));
    } else {
        if (!vr.isDataSynced)
            masm.storePayload(vr.dataReg(), addressOf(fe));
        if (!vr.isTypeSynced) {
            if (vr.isTypeKnown())
                masm.storeTypeTag(ImmType(vr.knownType()), addressOf(fe));
            else
                masm.storeTypeTag(vr.typeReg(), addressOf(fe));
        }
    }
#endif
}

static inline bool
AllocHelper(RematInfo &info, MaybeRegisterID &maybe)
{
    if (info.inRegister()) {
        maybe = info.reg();
        return true;
    }
    return false;
}

void
FrameState::allocForSameBinary(FrameEntry *fe, JSOp op, BinaryAlloc &alloc)
{
    alloc.rhsNeedsRemat = false;

    if (!fe->isTypeKnown()) {
        alloc.lhsType = tempRegForType(fe);
        pinReg(alloc.lhsType.reg());
    }

    alloc.lhsData = tempRegForData(fe);

    if (!freeRegs.empty(Registers::AvailRegs)) {
        alloc.result = allocReg();
        masm.move(alloc.lhsData.reg(), alloc.result);
        alloc.lhsNeedsRemat = false;
    } else {
        alloc.result = alloc.lhsData.reg();
        takeReg(alloc.result);
        alloc.lhsNeedsRemat = true;
    }

    if (alloc.lhsType.isSet())
        unpinReg(alloc.lhsType.reg());

    alloc.lhsFP = alloc.rhsFP = allocFPReg();
}

void
FrameState::ensureFullRegs(FrameEntry *fe, MaybeRegisterID *type, MaybeRegisterID *data)
{
    fe = fe->isCopy() ? fe->copyOf() : fe;

    JS_ASSERT(!data->isSet() && !type->isSet());
    if (!fe->type.inMemory()) {
        if (fe->type.inRegister())
            *type = fe->type.reg();
        if (fe->data.isConstant())
            return;
        if (fe->data.inRegister()) {
            *data = fe->data.reg();
            return;
        }
        if (fe->type.inRegister())
            pinReg(fe->type.reg());
        *data = tempRegForData(fe);
        if (fe->type.inRegister())
            unpinReg(fe->type.reg());
    } else if (!fe->data.inMemory()) {
        if (fe->data.inRegister())
            *data = fe->data.reg();
        if (fe->type.isConstant())
            return;
        if (fe->type.inRegister()) {
            *type = fe->type.reg();
            return;
        }
        if (fe->data.inRegister())
            pinReg(fe->data.reg());
        *type = tempRegForType(fe);
        if (fe->data.inRegister())
            unpinReg(fe->data.reg());
    } else {
        *data = tempRegForData(fe);
        pinReg(data->reg());
        *type = tempRegForType(fe);
        unpinReg(data->reg());
    }
}

inline bool
FrameState::binaryEntryLive(FrameEntry *fe) const
{
    






    if (fe >= sp - 2)
        return false;

    switch (JSOp(*PC)) {
      case JSOP_INCLOCAL:
      case JSOP_DECLOCAL:
      case JSOP_LOCALINC:
      case JSOP_LOCALDEC:
        if (fe - locals == (int) GET_SLOTNO(PC))
            return false;
      case JSOP_INCARG:
      case JSOP_DECARG:
      case JSOP_ARGINC:
      case JSOP_ARGDEC:
        if (fe - args == (int) GET_SLOTNO(PC))
            return false;
      default:;
    }

    JS_ASSERT(fe != callee_);

    
    return fe >= spBase || variableLive(fe, PC);
}

void
FrameState::allocForBinary(FrameEntry *lhs, FrameEntry *rhs, JSOp op, BinaryAlloc &alloc,
                           bool needsResult)
{
    FrameEntry *backingLeft = lhs;
    FrameEntry *backingRight = rhs;

    if (backingLeft->isCopy())
        backingLeft = backingLeft->copyOf();
    if (backingRight->isCopy())
        backingRight = backingRight->copyOf();

    



    if (AllocHelper(backingLeft->type, alloc.lhsType))
        pinReg(alloc.lhsType.reg());
    if (AllocHelper(backingLeft->data, alloc.lhsData))
        pinReg(alloc.lhsData.reg());
    if (AllocHelper(backingRight->type, alloc.rhsType))
        pinReg(alloc.rhsType.reg());
    if (AllocHelper(backingRight->data, alloc.rhsData))
        pinReg(alloc.rhsData.reg());

    
    if (!alloc.lhsType.isSet() && backingLeft->type.inMemory()) {
        alloc.lhsType = tempRegForType(lhs);
        pinReg(alloc.lhsType.reg());
    }
    if (!alloc.rhsType.isSet() && backingRight->type.inMemory()) {
        alloc.rhsType = tempRegForType(rhs);
        pinReg(alloc.rhsType.reg());
    }

    




    JS_ASSERT(!backingLeft->isType(JSVAL_TYPE_DOUBLE));
    JS_ASSERT(!backingRight->isType(JSVAL_TYPE_DOUBLE));
    alloc.lhsFP = allocFPReg();
    alloc.rhsFP = allocFPReg();

    bool commu;
    switch (op) {
      case JSOP_EQ:
      case JSOP_GT:
      case JSOP_GE:
      case JSOP_LT:
      case JSOP_LE:
        
      case JSOP_ADD:
      case JSOP_MUL:
      case JSOP_SUB:
        commu = true;
        break;

      case JSOP_DIV:
        commu = false;
        break;

      default:
        JS_NOT_REACHED("unknown op");
        return;
    }

    




    JS_ASSERT_IF(lhs->isConstant(), !rhs->isConstant());
    JS_ASSERT_IF(rhs->isConstant(), !lhs->isConstant());

    if (!alloc.lhsData.isSet()) {
        if (backingLeft->data.inMemory()) {
            alloc.lhsData = tempRegForData(lhs);
            pinReg(alloc.lhsData.reg());
        } else if (op == JSOP_MUL || !commu) {
            JS_ASSERT(lhs->isConstant());
            alloc.lhsData = allocReg();
            alloc.extraFree = alloc.lhsData;
            masm.move(Imm32(lhs->getValue().toInt32()), alloc.lhsData.reg());
        }
    }
    if (!alloc.rhsData.isSet()) {
        if (backingRight->data.inMemory()) {
            alloc.rhsData = tempRegForData(rhs);
            pinReg(alloc.rhsData.reg());
        } else if (op == JSOP_MUL) {
            JS_ASSERT(rhs->isConstant());
            alloc.rhsData = allocReg();
            alloc.extraFree = alloc.rhsData;
            masm.move(Imm32(rhs->getValue().toInt32()), alloc.rhsData.reg());
        }
    }

    alloc.lhsNeedsRemat = false;
    alloc.rhsNeedsRemat = false;
    alloc.resultHasRhs = false;
    alloc.undoResult = false;

    if (!needsResult)
        goto skip;

    






    



    if (backingLeft->data.inRegister() && !binaryEntryLive(backingLeft) &&
        (op == JSOP_ADD || (op == JSOP_SUB && backingRight->isConstant())) &&
        (lhs == backingLeft || hasOnlyCopy(backingLeft, lhs))) {
        alloc.result = backingLeft->data.reg();
        alloc.undoResult = true;
        alloc.resultHasRhs = false;
        goto skip;
    }

    if (!freeRegs.empty(Registers::AvailRegs)) {
        
        alloc.result = allocReg();
        if (!alloc.lhsData.isSet()) {
            JS_ASSERT(alloc.rhsData.isSet());
            JS_ASSERT(commu);
            masm.move(alloc.rhsData.reg(), alloc.result);
            alloc.resultHasRhs = true;
        } else {
            masm.move(alloc.lhsData.reg(), alloc.result);
            alloc.resultHasRhs = false;
        }
    } else {
        
        bool leftInReg = backingLeft->data.inRegister();
        bool rightInReg = backingRight->data.inRegister();

        
        uint32 mask = Registers::AvailRegs;
        if (backingLeft->type.inRegister())
            mask &= ~Registers::maskReg(backingLeft->type.reg());
        if (backingRight->type.inRegister())
            mask &= ~Registers::maskReg(backingRight->type.reg());

        RegisterID result = bestEvictReg(mask, true).reg();
        if (!commu && rightInReg && backingRight->data.reg() == result) {
            
            alloc.result = allocReg();
            masm.move(alloc.lhsData.reg(), alloc.result);
        } else {
            alloc.result = result;
            if (leftInReg && result == backingLeft->data.reg()) {
                alloc.lhsNeedsRemat = true;
                unpinReg(result);
                takeReg(result);
            } else if (rightInReg && result == backingRight->data.reg()) {
                alloc.rhsNeedsRemat = true;
                alloc.resultHasRhs = true;
                unpinReg(result);
                takeReg(result);
            } else {
                JS_ASSERT(!regstate(result).isPinned());
                takeReg(result);
                if (leftInReg) {
                    masm.move(alloc.lhsData.reg(), result);
                } else {
                    masm.move(alloc.rhsData.reg(), result);
                    alloc.resultHasRhs = true;
                }
            }
        }
    }

  skip:
    
    if (backingLeft->type.inRegister())
        unpinReg(backingLeft->type.reg());
    if (backingRight->type.inRegister())
        unpinReg(backingRight->type.reg());
    if (backingLeft->data.inRegister())
        unpinReg(backingLeft->data.reg());
    if (backingRight->data.inRegister())
        unpinReg(backingRight->data.reg());
}

void
FrameState::rematBinary(FrameEntry *lhs, FrameEntry *rhs, const BinaryAlloc &alloc, Assembler &masm)
{
    if (alloc.rhsNeedsRemat)
        masm.loadPayload(addressForDataRemat(rhs), alloc.rhsData.reg());
    if (alloc.lhsNeedsRemat)
        masm.loadPayload(addressForDataRemat(lhs), alloc.lhsData.reg());
}

MaybeRegisterID
FrameState::maybePinData(FrameEntry *fe)
{
    fe = fe->isCopy() ? fe->copyOf() : fe;
    if (fe->data.inRegister()) {
        pinReg(fe->data.reg());
        return fe->data.reg();
    }
    return MaybeRegisterID();
}

MaybeRegisterID
FrameState::maybePinType(FrameEntry *fe)
{
    fe = fe->isCopy() ? fe->copyOf() : fe;
    if (fe->type.inRegister()) {
        pinReg(fe->type.reg());
        return fe->type.reg();
    }
    return MaybeRegisterID();
}

void
FrameState::maybeUnpinReg(MaybeRegisterID reg)
{
    if (reg.isSet())
        unpinReg(reg.reg());
}

