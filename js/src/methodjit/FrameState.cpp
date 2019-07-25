





































#include "jscntxt.h"
#include "FrameState.h"
#include "FrameState-inl.h"

using namespace js;
using namespace js::mjit;


JS_STATIC_ASSERT(sizeof(FrameEntry) % 8 == 0);

FrameState::FrameState(JSContext *cx, JSScript *script, Assembler &masm)
  : cx(cx), script(script), masm(masm), entries(NULL), reifier(cx, *this)
{
}

FrameState::~FrameState()
{
    cx->free(entries);
}

bool
FrameState::init(uint32 nargs)
{
    this->nargs = nargs;

    uint32 nslots = script->nslots + nargs;
    if (!nslots) {
        sp = spBase = locals = args = base = NULL;
        return true;
    }

    uint32 nlocals = script->nslots;
    if ((eval = script->usesEval))
        nlocals = 0;

    uint8 *cursor = (uint8 *)cx->malloc(sizeof(FrameEntry) * nslots +       
                                        sizeof(FrameEntry *) * nslots +     
                                        sizeof(FrameEntry *) * nslots +     
                                        sizeof(uint32) * nlocals            
                                        );
    if (!cursor)
        return false;

    if (!reifier.init(nslots))
        return false;

    entries = (FrameEntry *)cursor;
    cursor += sizeof(FrameEntry) * nslots;

    base = (FrameEntry **)cursor;
    args = base;
    locals = base + nargs;
    spBase = locals + script->nfixed;
    sp = spBase;
    memset(base, 0, sizeof(FrameEntry *) * nslots);
    cursor += sizeof(FrameEntry *) * nslots;

    tracker.entries = (FrameEntry **)cursor;
    cursor += sizeof(FrameEntry *) * nslots;

    if (nlocals) {
        escaping = (uint32 *)cursor;
        memset(escaping, 0, sizeof(uint32) * nlocals);
    }

    return true;
}

void
FrameState::takeReg(RegisterID reg)
{
    if (freeRegs.hasReg(reg)) {
        freeRegs.takeReg(reg);
    } else {
        JS_ASSERT(regstate[reg].fe);
        evictReg(reg);
        regstate[reg].fe = NULL;
    }
}

void
FrameState::evictReg(RegisterID reg)
{
    FrameEntry *fe = regstate[reg].fe;

    if (regstate[reg].type == RematInfo::TYPE) {
        if (!fe->type.synced()) {
            syncType(fe, addressOf(fe), masm);
            fe->type.sync();
        }
        fe->type.setMemory();
    } else {
        if (!fe->data.synced()) {
            syncData(fe, addressOf(fe), masm);
            fe->data.sync();
        }
        fe->data.setMemory();
    }
}

JSC::MacroAssembler::RegisterID
FrameState::evictSomeReg(uint32 mask)
{
#ifdef DEBUG
    bool fallbackSet = false;
#endif
    RegisterID fallback = Registers::ReturnReg;

    for (uint32 i = 0; i < JSC::MacroAssembler::TotalRegisters; i++) {
        RegisterID reg = RegisterID(i);

        
        if (!(Registers::maskReg(reg) & mask))
            continue;

        
        FrameEntry *fe = regstate[i].fe;
        if (!fe)
            continue;

        
#ifdef DEBUG
        fallbackSet = true;
#endif
        fallback = reg;

        if (regstate[i].type == RematInfo::TYPE && fe->type.synced()) {
            fe->type.setMemory();
            return fallback;
        }
        if (regstate[i].type == RematInfo::DATA && fe->data.synced()) {
            fe->data.setMemory();
            return fallback;
        }
    }

    JS_ASSERT(fallbackSet);

    evictReg(fallback);
    return fallback;
}

void
FrameState::forgetEverything()
{
    syncAndKill(Registers::AvailRegs);

    throwaway();
}


void
FrameState::throwaway()
{
    for (uint32 i = 0; i < tracker.nentries; i++)
        base[indexOfFe(tracker[i])] = NULL;

    tracker.reset();
    freeRegs.reset();
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

    if (fe->data.inRegister()) {
        masm.storeData32(fe->data.reg(), address);
    } else {
        JS_ASSERT(fe->data.inMemory());
        RegisterID reg = popped ? allocReg() : allocReg(fe, RematInfo::DATA, true);
        masm.loadData32(addressOf(fe), reg);
        masm.storeData32(reg, address);
        if (popped)
            freeReg(reg);
        else
            fe->data.setRegister(reg);
    }

    if (fe->isTypeKnown()) {
        masm.storeTypeTag(ImmTag(fe->getTypeTag()), address);
    } else if (fe->type.inRegister()) {
        masm.storeTypeTag(fe->type.reg(), address);
    } else {
        JS_ASSERT(fe->type.inMemory());
        RegisterID reg = popped ? allocReg() : allocReg(fe, RematInfo::TYPE, true);
        masm.loadTypeTag(addressOf(fe), reg);
        masm.storeTypeTag(reg, address);
        if (popped)
            freeReg(reg);
        else
            fe->type.setRegister(reg);
    }
}

#ifdef DEBUG
void
FrameState::assertValidRegisterState() const
{
    Registers checkedFreeRegs;

    FrameEntry *tos = tosFe();
    for (uint32 i = 0; i < tracker.nentries; i++) {
        FrameEntry *fe = tracker[i];
        if (fe >= tos)
            continue;

        JS_ASSERT(i == fe->trackerIndex());
        JS_ASSERT_IF(fe->isCopy(),
                     fe->trackerIndex() > fe->copyOf()->trackerIndex());
        JS_ASSERT_IF(fe->isCopy(), !fe->type.inRegister() && !fe->data.inRegister());

        if (fe->isCopy())
            continue;
        if (fe->type.inRegister()) {
            checkedFreeRegs.takeReg(fe->type.reg());
            JS_ASSERT(regstate[fe->type.reg()].fe == fe);
        }
        if (fe->data.inRegister()) {
            checkedFreeRegs.takeReg(fe->data.reg());
            JS_ASSERT(regstate[fe->data.reg()].fe == fe);
        }
    }

    JS_ASSERT(checkedFreeRegs == freeRegs);
}
#endif

void
FrameState::syncFancy(Assembler &masm, Registers avail, uint32 resumeAt) const
{
    
    reifier.reset(&masm, avail, tracker.nentries);

    FrameEntry *tos = tosFe();
    for (uint32 i = resumeAt; i < tracker.nentries; i--) {
        FrameEntry *fe = tracker[i];
        if (fe >= tos)
            continue;

        reifier.sync(fe);
    }
}

void
FrameState::sync(Assembler &masm) const
{
    



    Registers avail(freeRegs);

    FrameEntry *tos = tosFe();
    for (uint32 i = tracker.nentries - 1; i < tracker.nentries; i--) {
        FrameEntry *fe = tracker[i];
        if (fe >= tos)
            continue;

        Address address = addressOf(fe);

        if (!fe->isCopy()) {
            
            if (fe->data.inRegister())
                avail.putReg(fe->data.reg());
            if (fe->type.inRegister())
                avail.putReg(fe->type.reg());

            
            if (!fe->data.synced()) {
                syncData(fe, address, masm);
                if (fe->isConstant())
                    continue;
            }
            if (!fe->type.synced())
                syncType(fe, addressOf(fe), masm);
        } else {
            FrameEntry *backing = fe->copyOf();
            JS_ASSERT(backing != fe);
            JS_ASSERT(!backing->isConstant() && !fe->isConstant());

            



            if ((!fe->type.synced() && !fe->type.inRegister()) ||
                (!fe->data.synced() && !fe->data.inRegister())) {
                syncFancy(masm, avail, i);
                return;
            }

            if (!fe->type.synced()) {
                
                if (fe->isTypeKnown()) {
                    
                    masm.storeTypeTag(ImmTag(fe->getTypeTag()), address);
                } else {
                    masm.storeTypeTag(backing->type.reg(), address);
                }
            }

            if (!fe->data.synced())
                masm.storeData32(backing->data.reg(), address);
        }
    }
}

void
FrameState::syncForCall(uint32 argc)
{
    syncAndKill(Registers::AvailRegs);
    freeRegs = Registers();
}

void
FrameState::syncAndKill(uint32 mask)
{
    Registers kill(mask);

    
    FrameEntry *tos = tosFe();
    for (uint32 i = tracker.nentries - 1; i < tracker.nentries; i--) {
        FrameEntry *fe = tracker[i];
        if (fe >= tos)
            continue;

        Address address = addressOf(fe);
        FrameEntry *backing = fe;
        if (fe->isCopy())
            backing = fe->copyOf();

        JS_ASSERT_IF(i == 0, !fe->isCopy());

        if (!fe->data.synced()) {
            if (backing != fe && backing->data.inMemory())
                tempRegForData(backing);
            syncData(backing, address, masm);
            fe->data.sync();
            if (fe->isConstant() && !fe->type.synced())
                fe->type.sync();
        }
        if (fe->data.inRegister() && kill.hasReg(fe->data.reg())) {
            JS_ASSERT(backing == fe);
            JS_ASSERT(fe->data.synced());
            if (regstate[fe->data.reg()].fe)
                forgetReg(fe->data.reg());
            fe->data.setMemory();
        }
        if (!fe->type.synced()) {
            if (backing != fe && backing->type.inMemory())
                tempRegForType(backing);
            syncType(backing, address, masm);
            fe->type.sync();
        }
        if (fe->type.inRegister() && kill.hasReg(fe->type.reg())) {
            JS_ASSERT(backing == fe);
            JS_ASSERT(fe->type.synced());
            if (regstate[fe->type.reg()].fe)
                forgetReg(fe->type.reg());
            fe->type.setMemory();
        }
    }
}

void
FrameState::merge(Assembler &masm, uint32 iVD) const
{
    FrameEntry *tos = tosFe();
    for (uint32 i = 0; i < tracker.nentries; i++) {
        FrameEntry *fe = tracker[i];
        if (fe >= tos)
            continue;

        
        if (fe->isCopy()) {
            JS_ASSERT(!fe->data.inRegister());
            JS_ASSERT(!fe->type.inRegister());
            continue;
        }

        if (fe->data.inRegister())
            masm.loadData32(addressOf(fe), fe->data.reg());
        if (fe->type.inRegister())
            masm.loadTypeTag(addressOf(fe), fe->type.reg());
    }
}

JSC::MacroAssembler::RegisterID
FrameState::copyDataIntoReg(FrameEntry *fe)
{
    JS_ASSERT(!fe->data.isConstant());

    if (fe->isCopy())
        fe = fe->copyOf();

    if (fe->data.inRegister()) {
        RegisterID reg = fe->data.reg();
        if (freeRegs.empty()) {
            if (!fe->data.synced())
                syncData(fe, addressOf(fe), masm);
            fe->data.setMemory();
            regstate[reg].fe = NULL;
        } else {
            RegisterID newReg = allocReg();
            masm.move(reg, newReg);
            reg = newReg;
        }
        return reg;
    }

    RegisterID reg = allocReg();

    if (!freeRegs.empty())
        masm.move(tempRegForData(fe), reg);
    else
        masm.loadData32(addressOf(fe),reg);

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
        if (freeRegs.empty()) {
            if (!fe->type.synced())
                syncType(fe, addressOf(fe), masm);
            fe->type.setMemory();
            regstate[reg].fe = NULL;
        } else {
            RegisterID newReg = allocReg();
            masm.move(reg, newReg);
            reg = newReg;
        }
        return reg;
    }

    RegisterID reg = allocReg();

    if (!freeRegs.empty())
        masm.move(tempRegForType(fe), reg);
    else
        masm.loadTypeTag(addressOf(fe), reg);

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

        if (freeRegs.empty()) {
            
            if (!backing->type.synced())
                syncType(backing, addressOf(backing), masm);
            reg = backing->type.reg();
            backing->type.setMemory();
            moveOwnership(reg, NULL);
        } else {
            reg = allocReg();
            masm.move(backing->type.reg(), reg);
        }
        return reg;
    }

    if (fe->type.inRegister()) {
        reg = fe->type.reg();
        
        JS_ASSERT(regstate[reg].fe == fe);
        JS_ASSERT(regstate[reg].type == RematInfo::TYPE);
        regstate[reg].fe = NULL;
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

    RegisterID reg;
    if (fe->isCopy()) {
        
        FrameEntry *backing = fe->copyOf();
        if (!backing->data.inRegister()) {
            JS_ASSERT(backing->data.inMemory());
            tempRegForData(backing);
        }

        if (freeRegs.empty()) {
            
            if (!backing->data.synced())
                syncData(backing, addressOf(backing), masm);
            reg = backing->data.reg();
            backing->data.setMemory();
            moveOwnership(reg, NULL);
        } else {
            reg = allocReg();
            masm.move(backing->data.reg(), reg);
        }
        return reg;
    }

    if (fe->isCopied()) {
        uncopy(fe);
        if (fe->isCopied()) {
            reg = allocReg();
            masm.loadData32(addressOf(fe), reg);
            return reg;
        }
    }
    
    if (fe->data.inRegister()) {
        reg = fe->data.reg();
        
        JS_ASSERT(regstate[reg].fe == fe);
        JS_ASSERT(regstate[reg].type == RematInfo::DATA);
        regstate[reg].fe = NULL;
        fe->data.invalidate();
    } else {
        JS_ASSERT(fe->data.inMemory());
        reg = allocReg();
        masm.loadData32(addressOf(fe), reg);
    }
    return reg;
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
            fe->setTypeTag(backing->getTypeTag());
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

void
FrameState::uncopy(FrameEntry *original)
{
    JS_ASSERT(original->isCopied());

    
    uint32 firstCopy = InvalidIndex;
    FrameEntry *tos = tosFe();
    for (uint32 i = 0; i < tracker.nentries; i++) {
        FrameEntry *fe = tracker[i];
        if (fe >= tos)
            continue;
        if (fe->isCopy() && fe->copyOf() == original) {
            firstCopy = i;
            break;
        }
    }

    if (firstCopy == InvalidIndex) {
        original->copied = false;
        return;
    }

    
    FrameEntry *fe = tracker[firstCopy];

    fe->setCopyOf(NULL);
    for (uint32 i = firstCopy + 1; i < tracker.nentries; i++) {
        FrameEntry *other = tracker[i];
        if (other >= tos)
            continue;

        
        JS_ASSERT(other != original);

        if (!other->isCopy() || other->copyOf() != original)
            continue;

        other->setCopyOf(fe);
        fe->setCopied();
    }

    




    if (!original->isTypeKnown()) {
        




        if (original->type.inMemory() && !fe->type.synced())
            tempRegForType(original);
        fe->type.inherit(original->type);
        if (fe->type.inRegister())
            moveOwnership(fe->type.reg(), fe);
    } else {
        JS_ASSERT(fe->isTypeKnown());
        JS_ASSERT(fe->getTypeTag() == original->getTypeTag());
    }
    if (original->data.inMemory() && !fe->data.synced())
        tempRegForData(original);
    fe->data.inherit(original->data);
    if (fe->data.inRegister())
        moveOwnership(fe->data.reg(), fe);
}

void
FrameState::storeLocal(uint32 n, bool popGuaranteed)
{
    if (!popGuaranteed && (eval || escaping[n])) {
        JS_ASSERT_IF(base[localIndex(n)] && (!eval || n < script->nfixed),
                     entries[localIndex(n)].type.inMemory() &&
                     entries[localIndex(n)].data.inMemory());
        Address local(JSFrameReg, sizeof(JSStackFrame) + n * sizeof(Value));
        storeTo(peek(-1), local, false);
        getLocal(n)->resetSynced();
        return;
    }

    FrameEntry *localFe = getLocal(n);

    
    FrameEntry *top = peek(-1);
    if (top->isCopy() && top->copyOf() == localFe) {
        JS_ASSERT(localFe->isCopied());
        return;
    }

    
    if (localFe->isCopied()) {
        uncopy(localFe);
        if (!localFe->isCopied())
            forgetAllRegs(localFe);
    } else {
        forgetAllRegs(localFe);
    }

    localFe->resetUnsynced();

    
    if (top->isConstant()) {
        localFe->setCopyOf(NULL);
        localFe->setNotCopied();
        localFe->setConstant(Jsvalify(top->getValue()));
        return;
    }

    











    FrameEntry *backing = top;
    if (top->isCopy()) {
        backing = top->copyOf();
        JS_ASSERT(backing->trackerIndex() < top->trackerIndex());

        uint32 backingIndex = indexOfFe(backing);
        uint32 tol = uint32(spBase - base);
        if (backingIndex < tol || backingIndex < localIndex(n)) {
            
            if (localFe->trackerIndex() < backing->trackerIndex())
                swapInTracker(backing, localFe);
            localFe->setNotCopied();
            localFe->setCopyOf(backing);
            if (backing->isTypeKnown())
                localFe->setTypeTag(backing->getTypeTag());
            else
                localFe->type.invalidate();
            localFe->data.invalidate();
            return;
        }

        

















        FrameEntry *tos = tosFe();
        for (uint32 i = backing->trackerIndex() + 1; i < tracker.nentries; i++) {
            FrameEntry *fe = tracker[i];
            if (fe >= tos)
                continue;
            if (fe->isCopy() && fe->copyOf() == backing)
                fe->setCopyOf(localFe);
        }
    }
    backing->setNotCopied();
    
    




    if (backing->trackerIndex() < localFe->trackerIndex())
        swapInTracker(backing, localFe);

    



    RegisterID reg = tempRegForData(backing);
    localFe->data.setRegister(reg);
    moveOwnership(reg, localFe);

    if (backing->isTypeKnown()) {
        localFe->setTypeTag(backing->getTypeTag());
    } else {
        RegisterID reg = tempRegForType(backing);
        localFe->type.setRegister(reg);
        moveOwnership(reg, localFe);
    }

    if (!backing->isTypeKnown())
        backing->type.invalidate();
    backing->data.invalidate();
    backing->setCopyOf(localFe);
    localFe->setCopied();

    JS_ASSERT(top->copyOf() == localFe);
}

void
FrameState::shimmy(uint32 n)
{
    JS_ASSERT(sp - n >= spBase);
    int32 depth = 0 - int32(n);
    storeLocal(uint32(&sp[depth - 1] - locals), true);
    popn(n);
}

void
FrameState::shift(int32 n)
{
    JS_ASSERT(n < 0);
    JS_ASSERT(sp + n - 1 >= spBase);
    storeLocal(uint32(&sp[n - 1] - locals), true);
    pop();
}

