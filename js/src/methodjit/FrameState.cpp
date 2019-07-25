





































#include "jscntxt.h"
#include "FrameState.h"
#include "FrameState-inl.h"
#include "StubCompiler.h"

using namespace js;
using namespace js::mjit;
using namespace js::analyze;


JS_STATIC_ASSERT(sizeof(FrameEntry) % 8 == 0);

FrameState::FrameState(JSContext *cx, mjit::Compiler &cc,
                       Assembler &masm, StubCompiler &stubcc)
  : cx(cx),
    masm(masm), cc(cc), stubcc(stubcc),
    a(NULL), entries(NULL), nentries(0), freeRegs(Registers::AvailAnyRegs),
    loop(NULL), inTryBlock(false)
{
}

FrameState::~FrameState()
{
    while (a) {
        ActiveFrame *parent = a->parent;
        a->script->analysis()->clearAllocations();
        cx->free_(a);
        a = parent;
    }
    cx->free_(entries);
}

void
FrameState::pruneDeadEntries()
{
    unsigned shift = 0;
    for (unsigned i = 0; i < tracker.nentries; i++) {
        FrameEntry *fe = tracker[i];
        if (deadEntry(fe)) {
            fe->untrack();
            shift++;
        } else if (shift) {
            fe->index_ -= shift;
            tracker.entries[fe->index_] = fe;
        }
    }
    tracker.nentries -= shift;
}

bool
FrameState::pushActiveFrame(JSScript *script, uint32 argc)
{
    if (!a) {
        this->nentries = analyze::TotalSlots(script) + (script->nslots - script->nfixed) +
            StackSpace::STACK_JIT_EXTRA - VALUES_PER_STACK_FRAME;
        size_t totalBytes = sizeof(FrameEntry) * nentries +       
                            sizeof(FrameEntry *) * nentries +     
                            sizeof(StackEntryExtra) * nentries;   
        uint8 *cursor = (uint8 *)cx->calloc_(totalBytes);
        if (!cursor)
            return false;

        this->entries = (FrameEntry *) cursor;
        cursor += sizeof(FrameEntry) * nentries;

        this->tracker.entries = (FrameEntry **)cursor;
        cursor += sizeof(FrameEntry *) * nentries;

        this->extraArray = (StackEntryExtra *)cursor;
        cursor += sizeof(StackEntryExtra) * nentries;

        JS_ASSERT(reinterpret_cast<uint8 *>(this->entries) + totalBytes == cursor);

#if defined JS_NUNBOX32
        if (!reifier.init(cx, *this, nentries))
            return false;
#endif

        this->temporaries = this->temporariesTop = this->entries + nentries - TEMPORARY_LIMIT;
    }

    
    JS_ASSERT_IF(a, argc == script->function()->nargs);

    ActiveFrame *newa = cx->new_<ActiveFrame>();
    if (!newa)
        return false;

    newa->parent = a;
    newa->depth = a ? (totalDepth() + VALUES_PER_STACK_FRAME) : 0;

    newa->script = script;
    newa->PC = script->code;
    newa->analysis = script->analysis();

    



    FrameEntry *entriesStart = a ? a->sp - (argc + 2) : entries;
    newa->callee_ = entriesStart + analyze::CalleeSlot();
    newa->this_   = entriesStart + analyze::ThisSlot();
    newa->args    = entriesStart + analyze::ArgSlot(0);
    newa->locals  = entriesStart + analyze::LocalSlot(script, 0);
    newa->spBase  = entriesStart + analyze::TotalSlots(script);
    newa->sp      = newa->spBase;

    this->a = newa;

    return true;
}

void
FrameState::associateReg(FrameEntry *fe, RematInfo::RematType type, AnyRegisterID reg)
{
    freeRegs.takeReg(reg);

    if (type == RematInfo::TYPE)
        fe->type.setRegister(reg.reg());
    else if (reg.isReg())
        fe->data.setRegister(reg.reg());
    else
        fe->data.setFPRegister(reg.fpreg());
    regstate(reg).associate(fe, type);
}

void
FrameState::popActiveFrame()
{
    a->analysis->clearAllocations();

    if (a->parent) {
        
        for (FrameEntry *fe = a->sp - 1; fe >= a->locals; fe--) {
            if (!fe->isTracked())
                continue;
            forgetAllRegs(fe);
            fe->clear();
        }
    }

    ActiveFrame *parent = a->parent;
    cx->delete_(a);
    a = parent;
}

void
FrameState::takeReg(AnyRegisterID reg)
{
    modifyReg(reg);
    if (freeRegs.hasReg(reg)) {
        freeRegs.takeReg(reg);
        JS_ASSERT(!regstate(reg).usedBy());
    } else {
        JS_ASSERT(regstate(reg).fe());
        evictReg(reg);
    }
}

#ifdef DEBUG
const char *
FrameState::entryName(const FrameEntry *fe) const
{
    static char bufs[4][50];
    static unsigned which = 0;
    which = (which + 1) & 3;
    char *buf = bufs[which];

    if (isTemporary(fe)) {
        JS_snprintf(buf, 50, "temp%d", fe - temporaries);
        return buf;
    }

    if (fe < a->callee_)
        return "parent";

    JS_ASSERT(fe >= a->callee_ && fe < a->sp);

    if (fe == a->callee_)
        return "callee";
    if (fe == a->this_)
        return "'this'";

    if (isArg(fe))
        JS_snprintf(buf, 50, "arg%d", fe - a->args);
    else if (isLocal(fe))
        JS_snprintf(buf, 50, "local%d", fe - a->locals);
    else
        JS_snprintf(buf, 50, "slot%d", fe - a->spBase);
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

    regstate(reg).forget();
}

inline Lifetime *
FrameState::variableLive(FrameEntry *fe, jsbytecode *pc) const
{
    




    JS_ASSERT(cx->typeInferenceEnabled());
    JS_ASSERT(fe > a->callee_ && fe < a->spBase);

    uint32 offset = pc - a->script->code;
    return a->analysis->liveness(entrySlot(fe)).live(offset);
}

AnyRegisterID
FrameState::bestEvictReg(uint32 mask, bool includePinned) const
{
    JS_ASSERT(cx->typeInferenceEnabled());

    
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

        






        if (fe == a->callee_) {
            JaegerSpew(JSpew_Regalloc, "result: %s is callee\n", reg.name());
            return reg;
        }

        if (fe >= a->spBase && !isTemporary(fe)) {
            if (!fallback.isSet()) {
                fallback = reg;
                fallbackOffset = 0;
            }
            JaegerSpew(JSpew_Regalloc, "    %s is on stack\n", reg.name());
            continue;
        }

        
        if (fe->isCopied()) {
            if (!fallback.isSet()) {
                fallback = reg;
                fallbackOffset = 0;
            }
            JaegerSpew(JSpew_Regalloc, "    %s has copies\n", reg.name());
            continue;
        }

        if (isTemporary(fe) || (a->parent && fe < a->locals)) {
            





            uint32 offset = a->parent ? a->script->length : loop->backedgeOffset();
            if (!fallback.isSet() || offset > fallbackOffset) {
                fallback = reg;
                fallbackOffset = offset;
            }
            JaegerSpew(JSpew_Regalloc, "    %s is a LICM or inline parent entry\n", reg.name());
            continue;
        }

        



        Lifetime *lifetime = variableLive(fe, a->PC);

        if (!lifetime) {
            JS_ASSERT(isConstructorThis(fe));
            fallback = reg;
            fallbackOffset = a->script->length;
            JaegerSpew(JSpew_Regalloc, "    %s is 'this' in a constructor\n", reg.name());
            continue;
        }

        



        JS_ASSERT_IF(lifetime->loopTail, loop);
        if (lifetime->loopTail && !loop->carriesLoopReg(fe)) {
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

void
FrameState::evictDeadEntries(bool includePinned)
{
    for (uint32 i = 0; i < Registers::TotalAnyRegisters; i++) {
        AnyRegisterID reg = AnyRegisterID::fromRaw(i);

        

        if (!(Registers::maskReg(reg) & Registers::AvailAnyRegs))
            continue;

        FrameEntry *fe = includePinned ? regstate(reg).usedBy() : regstate(reg).fe();
        if (!fe)
            continue;

        if (fe == a->callee_ || isConstructorThis(fe) ||
            fe >= a->spBase || fe->isCopied() || (a->parent && fe < a->locals)) {
            continue;
        }

        Lifetime *lifetime = variableLive(fe, a->PC);
        if (lifetime)
            continue;

        



        if (!fe->type.synced() && fe->isTypeKnown())
            fe->type.setMemory();

        



        fakeSync(fe);
        if (regstate(reg).type() == RematInfo::DATA)
            fe->data.setMemory();
        else
            fe->type.setMemory();
        forgetReg(reg);
    }
}

AnyRegisterID
FrameState::evictSomeReg(uint32 mask)
{
    JS_ASSERT(!freeRegs.hasRegInMask(mask));

    if (cx->typeInferenceEnabled()) {
        evictDeadEntries(false);

        if (freeRegs.hasRegInMask(mask)) {
            
            AnyRegisterID reg = freeRegs.takeAnyReg(mask);
            modifyReg(reg);
            return reg;
        }

        AnyRegisterID reg = bestEvictReg(mask, false);
        evictReg(reg);
        return reg;
    }

    
    JS_ASSERT((mask & ~Registers::AvailRegs) == 0);

    MaybeRegisterID fallback;

    for (uint32 i = 0; i < JSC::MacroAssembler::TotalRegisters; i++) {
        RegisterID reg = RegisterID(i);

        
        if (!(Registers::maskReg(reg) & mask))
            continue;

        
        FrameEntry *fe = regstate(reg).fe();
        if (!fe)
            continue;

        
        fallback = reg;

        if (regstate(reg).type() == RematInfo::TYPE && fe->type.synced()) {
            fe->type.setMemory();
            regstate(reg).forget();
            return reg;
        }
        if (regstate(reg).type() == RematInfo::DATA && fe->data.synced()) {
            fe->data.setMemory();
            regstate(reg).forget();
            return reg;
        }
    }

    evictReg(fallback.reg());
    return fallback.reg();
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

FrameEntry *
FrameState::snapshotState()
{
    
    FrameEntry *snapshot = cx->array_new<FrameEntry>(nentries);
    if (!snapshot)
        return NULL;
    PodCopy(snapshot, entries, nentries);
    return snapshot;
}

void
FrameState::restoreFromSnapshot(FrameEntry *snapshot)
{
    discardFrame();
    PodCopy(entries, snapshot, nentries);

    for (unsigned i = 0; i < nentries; i++) {
        FrameEntry *fe = entries + i;
        if (!fe->isTracked())
            continue;
        tracker.entries[fe->index_] = fe;
        tracker.nentries = Max(tracker.nentries, fe->index_ + 1);
        if (fe->isCopy())
            continue;
        if (fe->type.inRegister()) {
            freeRegs.takeReg(fe->type.reg());
            regstate(fe->type.reg()).associate(fe, RematInfo::TYPE);
        }
        if (fe->data.inRegister()) {
            freeRegs.takeReg(fe->data.reg());
            regstate(fe->data.reg()).associate(fe, RematInfo::DATA);
        }
        if (fe->data.inFPRegister()) {
            freeRegs.takeReg(fe->data.fpreg());
            regstate(fe->data.fpreg()).associate(fe, RematInfo::DATA);
        }
    }
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

#ifdef DEBUG
void
FrameState::dumpAllocation(RegisterAllocation *alloc)
{
    JS_ASSERT(cx->typeInferenceEnabled());
    for (unsigned i = 0; i < Registers::TotalAnyRegisters; i++) {
        AnyRegisterID reg = AnyRegisterID::fromRaw(i);
        if (alloc->assigned(reg)) {
            printf(" (%s: %s%s)", reg.name(), entryName(entries + alloc->index(reg)),
                   alloc->synced(reg) ? "" : " unsynced");
        }
    }
    printf("\n");
}
#endif

RegisterAllocation *
FrameState::computeAllocation(jsbytecode *target)
{
    JS_ASSERT(cx->typeInferenceEnabled());
    RegisterAllocation *alloc = ArenaNew<RegisterAllocation>(cx->compartment->pool, false);
    if (!alloc)
        return NULL;

    if (a->analysis->getCode(target).exceptionEntry || a->analysis->getCode(target).switchTarget ||
        JSOp(*target) == JSOP_TRAP) {
        
#ifdef DEBUG
        if (IsJaegerSpewChannelActive(JSpew_Regalloc)) {
            JaegerSpew(JSpew_Regalloc, "allocation at %u:", unsigned(target - a->script->code));
            dumpAllocation(alloc);
        }
#endif
        return alloc;
    }

    



    Registers regs = Registers::AvailAnyRegs;
    while (!regs.empty()) {
        AnyRegisterID reg = regs.takeAnyReg();
        if (freeRegs.hasReg(reg) || regstate(reg).type() == RematInfo::TYPE)
            continue;
        FrameEntry *fe = regstate(reg).fe();
        if (fe < a->callee_ ||
            isConstructorThis(fe) ||
            (fe > a->callee_ && fe < a->spBase && variableLive(fe, target)) ||
            (isTemporary(fe) && (a->parent || uint32(target - a->script->code) <= loop->backedgeOffset()))) {
            





            if (!reg.isReg() && !isTemporary(fe) && fe >= a->callee_ && fe < a->spBase) {
                if (!a->analysis->trackSlot(entrySlot(fe)))
                    continue;
                bool nonDoubleTarget = false;
                const SlotValue *newv = a->analysis->newValues(target);
                while (newv && newv->slot) {
                    if (newv->value.kind() == SSAValue::PHI &&
                        newv->value.phiOffset() == uint32(target - a->script->code) &&
                        newv->slot == entrySlot(fe)) {
                        types::TypeSet *types = a->analysis->getValueTypes(newv->value);
                        if (types->getKnownTypeTag(cx) != JSVAL_TYPE_DOUBLE)
                            nonDoubleTarget = true;
                    }
                    newv++;
                }
                if (nonDoubleTarget)
                    continue;
            }
            alloc->set(reg, fe - entries, fe->data.synced());
        }
    }

#ifdef DEBUG
    if (IsJaegerSpewChannelActive(JSpew_Regalloc)) {
        JaegerSpew(JSpew_Regalloc, "allocation at %u:", unsigned(target - a->script->code));
        dumpAllocation(alloc);
    }
#endif

    return alloc;
}

void
FrameState::relocateReg(AnyRegisterID reg, RegisterAllocation *alloc, Uses uses)
{
    JS_ASSERT(cx->typeInferenceEnabled());

    






    JS_ASSERT(!freeRegs.hasReg(reg));

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

    if (!cx->typeInferenceEnabled()) {
        syncAndForgetEverything();
        return true;
    }

    RegisterAllocation *&alloc = a->analysis->getAllocation(target);
    if (!alloc) {
        alloc = computeAllocation(target);
        if (!alloc)
            return false;
    }

    syncForAllocation(alloc, false, uses);

    return true;
}

void
FrameState::syncForAllocation(RegisterAllocation *alloc, bool inlineReturn, Uses uses)
{
    





    FrameEntry *topEntry = NULL;
    if (inlineReturn)
        topEntry = a->parent->sp - (GET_ARGC(a->parent->PC) + 2);

    for (uint32 i = tracker.nentries - 1; i < tracker.nentries; i--) {
        FrameEntry *fe = tracker[i];

        if (deadEntry(fe, uses.nuses))
            continue;
        if (inlineReturn && fe >= topEntry && !isTemporary(fe)) {
            






            forgetAllRegs(fe);
            fe->resetSynced();
            continue;
        }

        
        if (isLocal(fe) && !a->analysis->slotEscapes(entrySlot(fe))) {
            Lifetime *lifetime = a->analysis->liveness(entrySlot(fe)).live(a->PC - a->script->code);
            if (!lifetime)
                fakeSync(fe);
        }

        
        if (inlineReturn && fe >= a->parent->locals &&
            fe - a->parent->locals < a->parent->script->nfixed &&
            !a->parent->analysis->slotEscapes(frameSlot(a->parent, fe))) {
            const LifetimeVariable &var = a->parent->analysis->liveness(frameSlot(a->parent, fe));
            Lifetime *lifetime = var.live(a->parent->PC - a->parent->script->code);
            if (!lifetime)
                fakeSync(fe);
        }

        if (!fe->isCopy() && alloc->hasAnyReg(fe - entries)) {
            
            if (!fe->isType(JSVAL_TYPE_DOUBLE))
                syncType(fe);
        } else {
            syncFe(fe);
            if (fe->isCopy())
                fe->resetSynced();
        }
    }

    





    Registers regs = Registers(Registers::AvailAnyRegs);
    while (!regs.empty()) {
        AnyRegisterID reg = regs.takeAnyReg();
        if (!alloc->assigned(reg))
            continue;
        FrameEntry *fe = getOrTrack(alloc->index(reg));
        JS_ASSERT(!fe->isCopy());

        JS_ASSERT_IF(!fe->isType(JSVAL_TYPE_DOUBLE), fe->type.synced());
        if (!fe->data.synced() && alloc->synced(reg))
            syncFe(fe);

        if (fe->dataInRegister(reg))
            continue;

        if (!freeRegs.hasReg(reg))
            relocateReg(reg, alloc, uses);

        if (reg.isReg()) {
            RegisterID nreg = reg.reg();
            if (fe->isType(JSVAL_TYPE_DOUBLE)) {
                JS_ASSERT(!a->analysis->trackSlot(entrySlot(fe)));
                syncFe(fe);
                forgetAllRegs(fe);
                fe->resetSynced();
            }
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
            FPRegisterID nreg = reg.fpreg();
            JS_ASSERT(!fe->isNotType(JSVAL_TYPE_DOUBLE));
            if (!fe->isTypeKnown())
                learnType(fe, JSVAL_TYPE_DOUBLE, false);
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
}

bool
FrameState::discardForJoin(RegisterAllocation *&alloc, uint32 stackDepth)
{
    if (!cx->typeInferenceEnabled()) {
        resetInternalState();
        PodArrayZero(regstate_);
        a->sp = a->spBase + stackDepth;
        return true;
    }

    if (!alloc) {
        



        alloc = ArenaNew<RegisterAllocation>(cx->compartment->pool, false);
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
        FrameEntry *fe = getOrTrack(alloc->index(reg));

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

    a->sp = a->spBase + stackDepth;

    for (unsigned i = 0; i < stackDepth; i++)
        extraArray[a->spBase + i - entries].reset();

    return true;
}

bool
FrameState::consistentRegisters(jsbytecode *target)
{
    if (!cx->typeInferenceEnabled()) {
        JS_ASSERT(freeRegs.freeMask == Registers::AvailAnyRegs);
        return true;
    }

    





    RegisterAllocation *alloc = a->analysis->getAllocation(target);
    JS_ASSERT(alloc);

    Registers regs(Registers::AvailAnyRegs);
    while (!regs.empty()) {
        AnyRegisterID reg = regs.takeAnyReg();
        if (alloc->assigned(reg)) {
            FrameEntry *needed = getOrTrack(alloc->index(reg));
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
    if (!cx->typeInferenceEnabled())
        return;

    JS_ASSERT_IF(!synced, !consistentRegisters(target));

    RegisterAllocation *alloc = a->analysis->getAllocation(target);
    JS_ASSERT(alloc);

    Registers regs = 0;

    regs = Registers(Registers::AvailAnyRegs);
    while (!regs.empty()) {
        AnyRegisterID reg = regs.takeAnyReg();
        if (!alloc->assigned(reg))
            continue;

        const FrameEntry *fe = getOrTrack(alloc->index(reg));
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

    
    bool pinAddressReg = !!regstate(address.base).fe();
    if (pinAddressReg)
        pinReg(address.base);

#if defined JS_PUNBOX64
    if (fe->type.inMemory() && fe->data.inMemory()) {
        
        RegisterID vreg = Registers::ValueReg;
        masm.loadPtr(addressOf(fe), vreg);
        masm.storePtr(vreg, address);
        if (pinAddressReg)
            unpinReg(address.base);
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
    if (pinAddressReg)
        unpinReg(address.base);
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

    
    int32 copyCount = 0;

    for (uint32 i = 0; i < tracker.nentries; i++) {
        FrameEntry *fe = tracker[i];
        if (deadEntry(fe))
            continue;

        JS_ASSERT(i == fe->trackerIndex());

        if (fe->isCopy()) {
            JS_ASSERT_IF(!fe->copyOf()->temporary, fe > fe->copyOf());
            JS_ASSERT(fe->trackerIndex() > fe->copyOf()->trackerIndex());
            JS_ASSERT(!deadEntry(fe->copyOf()));
            JS_ASSERT(fe->copyOf()->isCopied());
            JS_ASSERT(!fe->isCopied());
            copyCount--;
            continue;
        }

        copyCount += fe->copied;

        if (fe->type.inRegister()) {
            checkedFreeRegs.takeReg(fe->type.reg());
            JS_ASSERT(regstate(fe->type.reg()).fe() == fe);
            JS_ASSERT(!fe->isType(JSVAL_TYPE_DOUBLE));
        }
        if (fe->data.inRegister()) {
            checkedFreeRegs.takeReg(fe->data.reg());
            JS_ASSERT(regstate(fe->data.reg()).fe() == fe);
            JS_ASSERT(!fe->isType(JSVAL_TYPE_DOUBLE));
        }
        if (fe->data.inFPRegister()) {
            JS_ASSERT(fe->isType(JSVAL_TYPE_DOUBLE));
            checkedFreeRegs.takeReg(fe->data.fpreg());
            JS_ASSERT(regstate(fe->data.fpreg()).fe() == fe);
        }
    }

    JS_ASSERT(copyCount == 0);
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

    FrameEntry *bottom = (cx->typeInferenceEnabled() || cx->compartment->debugMode())
        ? entries
        : a->sp - uses.nuses;

    for (FrameEntry *fe = a->sp - 1; fe >= bottom; fe--) {
        if (!fe->isTracked())
            continue;

        if (fe->isType(JSVAL_TYPE_DOUBLE)) {
            
            if (fe->isCopy() || !fe->data.inFPRegister())
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
                syncFancy(masm, avail, fe, bottom);
                return;
            }
#endif
        }

        bool copy = fe->isCopy();

        
#if defined JS_PUNBOX64
        
        if (copy || (!fe->type.inRegister() && !fe->data.inRegister()))
            ensureFeSynced(fe, masm);
#elif defined JS_NUNBOX32
        
        if (copy || !fe->data.inRegister())
            ensureDataSynced(fe, masm);
        if (copy || !fe->type.inRegister())
            ensureTypeSynced(fe, masm);
#endif
    }
}

void
FrameState::syncAndKill(Registers kill, Uses uses, Uses ignore)
{
    if (loop) {
        



        loop->clearLoopRegisters();
    }

    
    Registers search(kill.freeMask & ~freeRegs.freeMask);
    while (!search.empty()) {
        AnyRegisterID reg = search.takeAnyReg();
        FrameEntry *fe = regstate(reg).usedBy();
        if (!fe || deadEntry(fe, ignore.nuses))
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

    FrameEntry *bottom = (cx->typeInferenceEnabled() || cx->compartment->debugMode())
        ? entries
        : a->sp - uses.nuses;

    for (FrameEntry *fe = a->sp - 1; fe >= bottom && maxvisits; fe--) {
        if (!fe->isTracked())
            continue;

        maxvisits--;

        if (deadEntry(fe, ignore.nuses))
            continue;

        syncFe(fe);

        if (fe->isCopy())
            continue;

        
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
        if (!fe || deadEntry(fe, ignore.nuses))
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
        FrameEntry *fe = a->sp - 1 - i;
        if (fe->isTracked() && fe->isType(JSVAL_TYPE_DOUBLE))
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
    JS_ASSERT(!fe->isConstant());
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

    modifyReg(hint);
}

JSC::MacroAssembler::RegisterID
FrameState::copyDataIntoReg(Assembler &masm, FrameEntry *fe)
{
    JS_ASSERT(!fe->isConstant());

    if (fe->isCopy())
        fe = fe->copyOf();

    if (fe->data.inRegister()) {
        RegisterID reg = fe->data.reg();
        if (freeRegs.empty(Registers::AvailRegs)) {
            ensureDataSynced(fe, masm);
            fe->data.setMemory();
            regstate(reg).forget();
            modifyReg(reg);
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
    if (fe->isCopy())
        fe = fe->copyOf();

    JS_ASSERT(!fe->type.isConstant());

    if (fe->type.inRegister()) {
        RegisterID reg = fe->type.reg();
        if (freeRegs.empty(Registers::AvailRegs)) {
            ensureTypeSynced(fe, masm);
            fe->type.setMemory();
            regstate(reg).forget();
            modifyReg(reg);
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
    JS_ASSERT(!fe->isTypeKnown());

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
            modifyReg(reg);
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
        fe->type.setMemory();
        modifyReg(reg);
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
    JS_ASSERT(!fe->isConstant());
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
            modifyReg(reg);
        } else {
            reg = allocReg();
            masm.move(backing->data.reg(), reg);
        }
        return reg;
    }

    if (fe->isCopied())
        uncopy(fe);

    if (fe->data.inRegister()) {
        reg = fe->data.reg();
        
        JS_ASSERT(regstate(reg).fe() == fe);
        JS_ASSERT(regstate(reg).type() == RematInfo::DATA);
        regstate(reg).forget();
        fe->data.setMemory();
        modifyReg(reg);
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
    fe->clear();
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
    if (fe->isType(JSVAL_TYPE_DOUBLE))
        return;

    if (fe->isConstant()) {
        JS_ASSERT(fe->getValue().isInt32());
        Value newValue = DoubleValue(double(fe->getValue().toInt32()));
        fe->setConstant(Jsvalify(newValue));
        return;
    }

    FrameEntry *backing = fe;
    if (fe->isCopy()) {
        
        backing = fe->copyOf();
        fe->clear();
    } else if (fe->isCopied()) {
        
        for (uint32 i = fe->trackerIndex() + 1; i < tracker.nentries; i++) {
            FrameEntry *nfe = tracker[i];
            if (!deadEntry(nfe) && nfe->isCopy() && nfe->copyOf() == fe) {
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

    if (fe == backing)
        forgetAllRegs(fe);
    fe->resetUnsynced();
    fe->setType(JSVAL_TYPE_DOUBLE);
    fe->data.setFPRegister(fpreg);
    regstate(fpreg).associate(fe, RematInfo::DATA);

    fe->data.unsync();
    fe->type.unsync();
}

void
FrameState::ensureInteger(FrameEntry *fe)
{
    




    if (fe->isConstant()) {
        Value newValue = Int32Value(int32(fe->getValue().toDouble()));
        fe->setConstant(Jsvalify(newValue));
        return;
    }

    JS_ASSERT(!fe->isCopy() && !fe->isCopied());
    JS_ASSERT_IF(fe->isTypeKnown(), fe->isType(JSVAL_TYPE_DOUBLE));

    if (!fe->isType(JSVAL_TYPE_DOUBLE)) {
        



        if (fe->data.inRegister()) {
            syncFe(fe);
            forgetReg(fe->data.reg());
            fe->data.setMemory();
        }
        learnType(fe, JSVAL_TYPE_DOUBLE, false);
    }

    RegisterID reg = allocReg();
    FPRegisterID fpreg = tempFPRegForData(fe);
    Jump j = masm.branchTruncateDoubleToInt32(fpreg, reg);
    j.linkTo(masm.label(), &masm);

    forgetAllRegs(fe);
    fe->resetUnsynced();
    fe->setType(JSVAL_TYPE_INT32);
    fe->data.setRegister(reg);
    regstate(reg).associate(fe, RematInfo::DATA);

    fe->data.unsync();
    fe->type.unsync();
}

void
FrameState::ensureInMemoryDoubles(Assembler &masm)
{
    JS_ASSERT(!a->parent);
    for (uint32 i = 0; i < tracker.nentries; i++) {
        FrameEntry *fe = tracker[i];
        if (!deadEntry(fe) && fe->isType(JSVAL_TYPE_DOUBLE) &&
            !fe->isCopy() && !fe->isConstant()) {
            masm.ensureInMemoryDouble(addressOf(fe));
        }
    }
}

void
FrameState::pushCopyOf(FrameEntry *backing)
{
    JS_ASSERT(backing->isTracked());
    FrameEntry *fe = rawPush();
    fe->resetUnsynced();
    if (backing->isConstant()) {
        fe->setConstant(Jsvalify(backing->getValue()));
    } else {
        if (backing->isCopy())
            backing = backing->copyOf();
        fe->setCopyOf(backing);

        
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
        if (deadEntry(fe))
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
    JS_ASSERT_IF(!isTemporary(original), bestFe > original);

    
    bestFe->setCopyOf(NULL);
    if (ncopies > 1) {
        for (uint32 i = firstCopy; i < tracker.nentries; i++) {
            FrameEntry *other = tracker[i];
            if (deadEntry(other) || other == bestFe)
                continue;

            
            JS_ASSERT(other != original);

            if (!other->isCopy() || other->copyOf() != original)
                continue;

            other->setCopyOf(bestFe);

            






            if (other->trackerIndex() < bestFe->trackerIndex())
                swapInTracker(bestFe, other);
        }
    }

    return bestFe;
}

FrameEntry *
FrameState::walkFrameForUncopy(FrameEntry *original)
{
    FrameEntry *bestFe = NULL;
    uint32 ncopies = 0;

    
    uint32 maxvisits = tracker.nentries;

    for (FrameEntry *fe = original + 1; fe < a->sp && maxvisits; fe++) {
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

    return bestFe;
}

FrameEntry *
FrameState::uncopy(FrameEntry *original)
{
    JS_ASSERT(original->isCopied());

    






















    FrameEntry *fe;
    if ((tracker.nentries - original->trackerIndex()) * 2 > uint32(a->sp - original))
        fe = walkFrameForUncopy(original);
    else
        fe = walkTrackerForUncopy(original);
    JS_ASSERT(fe);

    




    if (!original->isTypeKnown()) {
        




        if (original->type.inMemory() && !fe->type.synced())
            tempRegForType(original);
        fe->type.inherit(original->type);
        if (fe->type.inRegister())
            regstate(fe->type.reg()).reassociate(fe);
    } else {
        fe->setType(original->getKnownType());
    }
    if (original->isType(JSVAL_TYPE_DOUBLE)) {
        if (original->data.inMemory() && !fe->data.synced())
            tempFPRegForData(original);
        fe->data.inherit(original->data);
        if (fe->data.inFPRegister())
            regstate(fe->data.fpreg()).reassociate(fe);
    } else {
        if (fe->type.inRegister())
            pinReg(fe->type.reg());
        if (original->data.inMemory() && !fe->data.synced())
            tempRegForData(original);
        if (fe->type.inRegister())
            unpinReg(fe->type.reg());
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
        if (nfe != fe && !deadEntry(nfe) && nfe->isCopy() && nfe->copyOf() == backing)
            return false;
    }

    return true;
}

void
FrameState::separateBinaryEntries(FrameEntry *lhs, FrameEntry *rhs)
{
    JS_ASSERT(lhs == a->sp - 2 && rhs == a->sp - 1);
    if (rhs->isCopy() && rhs->copyOf() == lhs) {
        syncAndForgetFe(rhs);
        syncAndForgetFe(lhs);
        uncopy(lhs);
    }
}

void
FrameState::storeLocal(uint32 n, bool popGuaranteed)
{
    FrameEntry *local = getLocal(n);

    if (a->analysis->slotEscapes(entrySlot(local))) {
        JS_ASSERT(local->data.inMemory());
        storeTo(peek(-1), addressOf(local), popGuaranteed);
        return;
    }

    storeTop(local);

    if (loop)
        local->lastLoop = loop->headOffset();

    if (inTryBlock)
        syncFe(local);
}

void
FrameState::storeArg(uint32 n, bool popGuaranteed)
{
    
    
    FrameEntry *arg = getArg(n);

    if (a->analysis->slotEscapes(entrySlot(arg))) {
        JS_ASSERT(arg->data.inMemory());
        storeTo(peek(-1), addressOf(arg), popGuaranteed);
        return;
    }

    storeTop(arg);

    if (loop)
        arg->lastLoop = loop->headOffset();

    syncFe(arg);
}

void
FrameState::forgetEntry(FrameEntry *fe)
{
    if (fe->isCopied()) {
        uncopy(fe);
        fe->resetUnsynced();
    } else {
        forgetAllRegs(fe);
    }

    extraArray[fe - entries].reset();
}

void
FrameState::storeTop(FrameEntry *target)
{
    JS_ASSERT(!isTemporary(target));

    
    FrameEntry *top = peek(-1);
    if (top->isCopy() && top->copyOf() == target) {
        JS_ASSERT(target->isCopied());
        return;
    }

    




    bool wasSynced = target->type.synced();
    JSValueType oldType = target->isTypeKnown() ? target->getKnownType() : JSVAL_TYPE_UNKNOWN;
    bool trySyncType = wasSynced && oldType != JSVAL_TYPE_UNKNOWN && oldType != JSVAL_TYPE_DOUBLE;

    
    forgetEntry(target);
    target->resetUnsynced();

    
    if (top->isConstant()) {
        target->clear();
        target->setConstant(Jsvalify(top->getValue()));
        if (trySyncType && target->isType(oldType))
            target->type.sync();
        return;
    }

    












    FrameEntry *backing = top;
    if (top->isCopy()) {
        backing = top->copyOf();
        JS_ASSERT(backing->trackerIndex() < top->trackerIndex());

        if (backing < target || isTemporary(backing)) {
            
            if (target->trackerIndex() < backing->trackerIndex())
                swapInTracker(backing, target);
            target->setCopyOf(backing);
            if (trySyncType && target->isType(oldType))
                target->type.sync();
            return;
        }

        

















        for (uint32 i = backing->trackerIndex() + 1; i < tracker.nentries; i++) {
            FrameEntry *fe = tracker[i];
            if (deadEntry(fe))
                continue;
            if (fe->isCopy() && fe->copyOf() == backing)
                fe->setCopyOf(target);
        }
    }
    
    




    if (backing->trackerIndex() < target->trackerIndex())
        swapInTracker(backing, target);

    if (backing->isType(JSVAL_TYPE_DOUBLE)) {
        FPRegisterID fpreg = tempFPRegForData(backing);
        target->setType(JSVAL_TYPE_DOUBLE);
        target->data.setFPRegister(fpreg);
        regstate(fpreg).reassociate(target);
    } else {
        





        if (backing->type.inRegister())
            pinReg(backing->type.reg());
        RegisterID reg = tempRegForData(backing);
        if (backing->type.inRegister())
            unpinReg(backing->type.reg());
        target->data.setRegister(reg);
        regstate(reg).reassociate(target);

        if (backing->isTypeKnown()) {
            target->setType(backing->getKnownType());
        } else {
            pinReg(reg);
            RegisterID typeReg = tempRegForType(backing);
            unpinReg(reg);
            target->type.setRegister(typeReg);
            regstate(typeReg).reassociate(target);
        }
    }

    backing->setCopyOf(target);
    JS_ASSERT(top->copyOf() == target);

    if (trySyncType && target->isType(oldType))
        target->type.sync();
}

void
FrameState::shimmy(uint32 n)
{
    JS_ASSERT(a->sp - n >= a->spBase);
    int32 depth = 0 - int32(n);
    storeTop(peek(depth - 1));
    popn(n);
}

void
FrameState::shift(int32 n)
{
    JS_ASSERT(n < 0);
    JS_ASSERT(a->sp + n - 1 >= a->spBase);
    storeTop(peek(n - 1));
    pop();
}

void
FrameState::swap()
{
    

    dupAt(-2);
    

    dupAt(-2);
    

    shift(-3);
    

    shimmy(1);
    
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
    fe->clear();

    regstate(typeReg).associate(fe, RematInfo::TYPE);
    regstate(dataReg).associate(fe, RematInfo::DATA);
    fe->type.setRegister(typeReg);
    fe->data.setRegister(dataReg);
    freeReg(fpreg);
}

void
FrameState::pinEntry(FrameEntry *fe, ValueRemat &vr, bool breakDouble)
{
    if (breakDouble && !fe->isConstant() && fe->isType(JSVAL_TYPE_DOUBLE))
        forgetKnownDouble(fe);

    if (fe->isConstant()) {
        vr = ValueRemat::FromConstant(fe->getValue());
    } else if (fe->isType(JSVAL_TYPE_DOUBLE)) {
        FPRegisterID fpreg = tempFPRegForData(fe);
        pinReg(fpreg);
        vr = ValueRemat::FromFPRegister(fpreg);
    } else {
        
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
    if (vr.isFPRegister()) {
        unpinReg(vr.fpReg());
    } else if (!vr.isConstant()) {
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
    if (vr.isConstant() || vr.isFPRegister()) {
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
    





    JS_ASSERT(cx->typeInferenceEnabled());

    if (deadEntry(fe, 2))
        return false;

    switch (JSOp(*a->PC)) {
      case JSOP_INCLOCAL:
      case JSOP_DECLOCAL:
      case JSOP_LOCALINC:
      case JSOP_LOCALDEC:
        if (fe - a->locals == (int) GET_SLOTNO(a->PC))
            return false;
      case JSOP_INCARG:
      case JSOP_DECARG:
      case JSOP_ARGINC:
      case JSOP_ARGDEC:
        if (fe - a->args == (int) GET_SLOTNO(a->PC))
            return false;
      default:;
    }

    JS_ASSERT(fe != a->callee_);

    
    if (a->parent && fe < a->locals)
        return true;

    
    return fe >= a->spBase || variableLive(fe, a->PC);
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
        } else if (!commu) {
            JS_ASSERT(lhs->isConstant());
            alloc.lhsData = allocReg();
            alloc.extraFree = alloc.lhsData;
            masm.move(Imm32(lhs->getValue().toInt32()), alloc.lhsData.reg());
        }
    }
    if (!alloc.rhsData.isSet() && backingRight->data.inMemory()) {
        alloc.rhsData = tempRegForData(rhs);
        pinReg(alloc.rhsData.reg());
    }

    alloc.lhsNeedsRemat = false;
    alloc.rhsNeedsRemat = false;
    alloc.resultHasRhs = false;
    alloc.undoResult = false;

    if (!needsResult)
        goto skip;

    






    



    if (cx->typeInferenceEnabled() &&
        backingLeft->data.inRegister() && !binaryEntryLive(backingLeft) &&
        (op == JSOP_ADD || (op == JSOP_SUB && backingRight->isConstant())) &&
        (lhs == backingLeft || hasOnlyCopy(backingLeft, lhs))) {
        alloc.result = backingLeft->data.reg();
        alloc.undoResult = true;
        alloc.resultHasRhs = false;
        goto skip;
    }

    if (cx->typeInferenceEnabled())
        evictDeadEntries(true);

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
    } else if (cx->typeInferenceEnabled()) {
        
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
    } else {
        



        bool leftInReg = backingLeft->data.inRegister();
        bool rightInReg = backingRight->data.inRegister();
        bool leftSynced = backingLeft->data.synced();
        bool rightSynced = backingRight->data.synced();
        if (!commu || (leftInReg && (leftSynced || (!rightInReg || !rightSynced)))) {
            JS_ASSERT(backingLeft->data.inRegister() || !commu);
            JS_ASSERT_IF(backingLeft->data.inRegister(),
                         backingLeft->data.reg() == alloc.lhsData.reg());
            if (backingLeft->data.inRegister()) {
                alloc.result = backingLeft->data.reg();
                unpinReg(alloc.result);
                takeReg(alloc.result);
                alloc.lhsNeedsRemat = true;
            } else {
                
                alloc.result = allocReg();
                masm.move(alloc.lhsData.reg(), alloc.result);
            }
            alloc.resultHasRhs = false;
        } else {
            JS_ASSERT(commu);
            JS_ASSERT(!leftInReg || (rightInReg && rightSynced));
            alloc.result = backingRight->data.reg();
            unpinReg(alloc.result);
            takeReg(alloc.result);
            alloc.resultHasRhs = true;
            alloc.rhsNeedsRemat = true;
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

uint32
FrameState::allocTemporary()
{
    if (temporariesTop == temporaries + TEMPORARY_LIMIT)
        return uint32(-1);
    FrameEntry *fe = temporariesTop++;
    fe->lastLoop = 0;
    fe->temporary = true;
    return fe - temporaries;
}

void
FrameState::clearTemporaries()
{
    JS_ASSERT(!a->parent);

    for (FrameEntry *fe = temporaries; fe < temporariesTop; fe++) {
        if (!fe->isTracked())
            continue;
        if (fe->isCopied())
            uncopy(fe);
        forgetAllRegs(fe);
        fe->resetSynced();
    }

    temporariesTop = temporaries;
}

Vector<TemporaryCopy> *
FrameState::getTemporaryCopies()
{
    
    Vector<TemporaryCopy> *res = NULL;

    for (FrameEntry *fe = temporaries; fe < temporariesTop; fe++) {
        if (!fe->isTracked())
            continue;
        if (fe->isCopied()) {
            for (uint32 i = fe->trackerIndex() + 1; i < tracker.nentries; i++) {
                FrameEntry *nfe = tracker[i];
                if (!deadEntry(nfe) && nfe->isCopy() && nfe->copyOf() == fe) {
                    if (!res)
                        res = cx->new_< Vector<TemporaryCopy> >(cx);
                    res->append(TemporaryCopy(addressOf(nfe), addressOf(fe)));
                }
            }
        }
    }

    return res;
}
