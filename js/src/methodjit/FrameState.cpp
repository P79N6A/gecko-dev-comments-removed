





































#include "jscntxt.h"
#include "FrameState.h"
#include "FrameState-inl.h"

using namespace js;
using namespace js::mjit;


JS_STATIC_ASSERT(sizeof(FrameEntry) % 8 == 0);

FrameState::FrameState(JSContext *cx, JSScript *script, Assembler &masm)
  : cx(cx), script(script), masm(masm), entries(NULL), reifier(cx, *this),
    inTryBlock(false)
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
        sp = spBase = locals = args = NULL;
        return true;
    }

    uint32 nlocals = script->nslots;
    if ((eval = script->usesEval || cx->compartment->debugMode))
        nlocals = 0;

    size_t totalBytes = sizeof(FrameEntry) * nslots +       
                        sizeof(FrameEntry *) * nslots +     
                        sizeof(uint32) * nlocals;           

    uint8 *cursor = (uint8 *)cx->calloc(totalBytes);
    if (!cursor)
        return false;

    if (!reifier.init(nslots))
        return false;

    entries = (FrameEntry *)cursor;
    cursor += sizeof(FrameEntry) * nslots;

    args = entries;
    locals = args + nargs;
    spBase = locals + script->nfixed;
    sp = spBase;

    tracker.entries = (FrameEntry **)cursor;
    cursor += sizeof(FrameEntry *) * nslots;

    if (nlocals) {
        escaping = (uint32 *)cursor;
        memset(escaping, 0, sizeof(uint32) * nlocals);
        cursor += sizeof(uint32) * nlocals;
    }

    JS_ASSERT(reinterpret_cast<uint8 *>(entries) + totalBytes == cursor);

    return true;
}

void
FrameState::takeReg(RegisterID reg)
{
    if (freeRegs.hasReg(reg)) {
        freeRegs.takeReg(reg);
        JS_ASSERT(!regstate[reg].usedBy());
    } else {
        JS_ASSERT(regstate[reg].fe());
        evictReg(reg);
        regstate[reg].forget();
    }
}

void
FrameState::evictReg(RegisterID reg)
{
    FrameEntry *fe = regstate[reg].fe();

    if (regstate[reg].type() == RematInfo::TYPE) {
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

        
        FrameEntry *fe = regstate[i].fe();
        if (!fe)
            continue;

        
#ifdef DEBUG
        fallbackSet = true;
#endif
        fallback = reg;

        if (regstate[i].type() == RematInfo::TYPE && fe->type.synced()) {
            fe->type.setMemory();
            return fallback;
        }
        if (regstate[i].type() == RematInfo::DATA && fe->data.synced()) {
            fe->data.setMemory();
            return fallback;
        }
    }

    JS_ASSERT(fallbackSet);

    evictReg(fallback);
    return fallback;
}


void
FrameState::syncAndForgetEverything()
{
    syncAndKill(Registers(Registers::AvailRegs), Uses(frameDepth()));
    forgetEverything();
}

void
FrameState::resetInternalState()
{
    for (uint32 i = 0; i < tracker.nentries; i++)
        tracker[i]->untrack();

    tracker.reset();
    freeRegs.reset();
}

void
FrameState::discardFrame()
{
    resetInternalState();

    memset(regstate, 0, sizeof(regstate));
}

void
FrameState::forgetEverything()
{
    resetInternalState();

#ifdef DEBUG
    for (uint32 i = 0; i < JSC::MacroAssembler::TotalRegisters; i++) {
        JS_ASSERT(!regstate[i].usedBy());
    }
#endif
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
        masm.storePayload(fe->data.reg(), address);
    } else {
        JS_ASSERT(fe->data.inMemory());
        RegisterID reg = popped ? allocReg() : allocReg(fe, RematInfo::DATA);

        JS_ASSERT(addressOf(fe).base != address.base ||
                  addressOf(fe).offset != address.offset);
        masm.loadPayload(addressOf(fe), reg);
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
        JS_ASSERT(addressOf(fe).base != address.base ||
                  addressOf(fe).offset != address.offset);
        RegisterID reg = popped ? allocReg() : allocReg(fe, RematInfo::TYPE);
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

    for (uint32 i = 0; i < tracker.nentries; i++) {
        FrameEntry *fe = tracker[i];
        if (fe >= sp)
            continue;

        JS_ASSERT(i == fe->trackerIndex());
        JS_ASSERT_IF(fe->isCopy(),
                     fe->trackerIndex() > fe->copyOf()->trackerIndex());
        JS_ASSERT_IF(fe->isCopy(), fe > fe->copyOf());
        JS_ASSERT_IF(fe->isCopy(), !fe->type.inRegister() && !fe->data.inRegister());
        JS_ASSERT_IF(fe->isCopy(), fe->copyOf() < sp);
        JS_ASSERT_IF(fe->isCopy(), fe->copyOf()->isCopied());

        if (fe->isCopy())
            continue;
        if (fe->type.inRegister()) {
            checkedFreeRegs.takeReg(fe->type.reg());
            JS_ASSERT(regstate[fe->type.reg()].fe() == fe);
        }
        if (fe->data.inRegister()) {
            checkedFreeRegs.takeReg(fe->data.reg());
            JS_ASSERT(regstate[fe->data.reg()].fe() == fe);
        }
    }

    JS_ASSERT(checkedFreeRegs == freeRegs);

    for (uint32 i = 0; i < JSC::MacroAssembler::TotalRegisters; i++) {
        JS_ASSERT(!regstate[i].isPinned());
        JS_ASSERT_IF(regstate[i].fe(), !freeRegs.hasReg(RegisterID(i)));
        JS_ASSERT_IF(regstate[i].fe(), regstate[i].fe()->isTracked());
    }
}
#endif

void
FrameState::syncFancy(Assembler &masm, Registers avail, uint32 resumeAt,
                      FrameEntry *bottom) const
{
    
    reifier.reset(&masm, avail, tracker.nentries, bottom);

    for (uint32 i = resumeAt; i < tracker.nentries; i--) {
        FrameEntry *fe = tracker[i];
        if (fe >= sp)
            continue;

        reifier.sync(fe);
    }
}

void
FrameState::sync(Assembler &masm, Uses uses) const
{
    



    Registers avail(freeRegs);
    Registers temp(Registers::TempRegs);

    FrameEntry *bottom = sp - uses.nuses;

    if (inTryBlock)
        bottom = NULL;

    for (uint32 i = tracker.nentries - 1; i < tracker.nentries; i--) {
        FrameEntry *fe = tracker[i];
        if (fe >= sp)
            continue;

        Address address = addressOf(fe);

        if (!fe->isCopy()) {
            
            if (fe->data.inRegister())
                avail.putReg(fe->data.reg());
            if (fe->type.inRegister())
                avail.putReg(fe->type.reg());

            
            if (!fe->data.synced() && (fe->data.inRegister() || fe >= bottom)) {
                syncData(fe, address, masm);
                if (fe->isConstant())
                    continue;
            }
            if (!fe->type.synced() && (fe->type.inRegister() || fe >= bottom))
                syncType(fe, addressOf(fe), masm);
        } else if (fe >= bottom) {
            FrameEntry *backing = fe->copyOf();
            JS_ASSERT(backing != fe);
            JS_ASSERT(!backing->isConstant() && !fe->isConstant());

            



            if ((!fe->type.synced() && !backing->type.inRegister()) ||
                (!fe->data.synced() && !backing->data.inRegister())) {
                syncFancy(masm, avail, i, bottom);
                return;
            }

            if (!fe->type.synced()) {
                
                if (fe->isTypeKnown()) {
                    
                    masm.storeTypeTag(ImmType(fe->getKnownType()), address);
                } else {
                    masm.storeTypeTag(backing->type.reg(), address);
                }
            }

            if (!fe->data.synced())
                masm.storePayload(backing->data.reg(), address);
        }
    }
}

void
FrameState::syncAndKill(Registers kill, Uses uses)
{
    
    FrameEntry *bottom = sp - uses.nuses;

    if (inTryBlock)
        bottom = NULL;

    for (uint32 i = tracker.nentries - 1; i < tracker.nentries; i--) {
        FrameEntry *fe = tracker[i];
        if (fe >= sp)
            continue;

        Address address = addressOf(fe);
        FrameEntry *backing = fe;
        if (fe->isCopy()) {
            if (!inTryBlock && fe < bottom)
                continue;
            backing = fe->copyOf();
        }

        JS_ASSERT_IF(i == 0, !fe->isCopy());

        bool killData = fe->data.inRegister() && kill.hasReg(fe->data.reg());
        if (!fe->data.synced() && (killData || fe >= bottom)) {
            if (backing != fe && backing->data.inMemory())
                tempRegForData(backing);
            syncData(backing, address, masm);
            fe->data.sync();
            if (fe->isConstant() && !fe->type.synced())
                fe->type.sync();
        }
        if (killData) {
            JS_ASSERT(backing == fe);
            JS_ASSERT(fe->data.synced());
            if (regstate[fe->data.reg()].fe())
                forgetReg(fe->data.reg());
            fe->data.setMemory();
        }
        bool killType = fe->type.inRegister() && kill.hasReg(fe->type.reg());
        if (!fe->type.synced() && (killType || fe >= bottom)) {
            if (backing != fe && backing->type.inMemory())
                tempRegForType(backing);
            syncType(backing, address, masm);
            fe->type.sync();
        }
        if (killType) {
            JS_ASSERT(backing == fe);
            JS_ASSERT(fe->type.synced());
            if (regstate[fe->type.reg()].fe())
                forgetReg(fe->type.reg());
            fe->type.setMemory();
        }
    }
}

void
FrameState::merge(Assembler &masm, Changes changes) const
{
    Registers temp(Registers::TempRegs);

    for (uint32 i = 0; i < tracker.nentries; i++) {
        FrameEntry *fe = tracker[i];
        if (fe >= sp)
            continue;

        
        if (fe->isCopy()) {
            JS_ASSERT(!fe->data.inRegister());
            JS_ASSERT(!fe->type.inRegister());
            continue;
        }

#if defined JS_PUNBOX64
        if (fe->data.inRegister() && fe->type.inRegister()) {
            masm.loadValueAsComponents(addressOf(fe), fe->type.reg(), fe->data.reg());
        } else
#endif
        {
            if (fe->data.inRegister())
                masm.loadPayload(addressOf(fe), fe->data.reg());
            if (fe->type.inRegister())
                masm.loadTypeTag(addressOf(fe), fe->type.reg());
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

    if (fe->isCopy())
        fe = fe->copyOf();

    if (!fe->data.inRegister())
        tempRegForData(fe);

    RegisterID reg = fe->data.reg();
    if (reg == hint) {
        if (freeRegs.empty()) {
            if (!fe->data.synced())
                syncData(fe, addressOf(fe), masm);
            fe->data.setMemory();
        } else {
            reg = allocReg();
            masm.move(hint, reg);
            fe->data.setRegister(reg);
            regstate[reg].associate(regstate[hint].fe(), RematInfo::DATA);
        }
        regstate[hint].forget();
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
        if (freeRegs.empty()) {
            if (!fe->data.synced())
                syncData(fe, addressOf(fe), masm);
            fe->data.setMemory();
            regstate[reg].forget();
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
        if (freeRegs.empty()) {
            if (!fe->type.synced())
                syncType(fe, addressOf(fe), masm);
            fe->type.setMemory();
            regstate[reg].forget();
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

JSC::MacroAssembler::FPRegisterID
FrameState::copyEntryIntoFPReg(FrameEntry *fe, FPRegisterID fpreg)
{
    return copyEntryIntoFPReg(this->masm, fe, fpreg);
}

JSC::MacroAssembler::FPRegisterID
FrameState::copyEntryIntoFPReg(Assembler &masm, FrameEntry *fe, FPRegisterID fpreg)
{
    if (fe->isCopy())
        fe = fe->copyOf();

    
    if (fe->data.isConstant()) {
        if (!fe->data.synced())
            syncData(fe, addressOf(fe), masm);
        if (!fe->type.synced())
            syncType(fe, addressOf(fe), masm);
    } else {
        if (fe->data.inRegister() && !fe->data.synced())
            syncData(fe, addressOf(fe), masm);
        if (fe->type.inRegister() && !fe->type.synced())
            syncType(fe, addressOf(fe), masm);
    }

    masm.loadDouble(addressOf(fe), fpreg);
    return fpreg;
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
            regstate[reg].forget();
        } else {
            reg = allocReg();
            masm.move(backing->type.reg(), reg);
        }
        return reg;
    }

    if (fe->type.inRegister()) {
        reg = fe->type.reg();

        
        JS_ASSERT(regstate[reg].fe() == fe);
        JS_ASSERT(regstate[reg].type() == RematInfo::TYPE);
        regstate[reg].forget();
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
            regstate[reg].forget();
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
        
        JS_ASSERT(regstate[reg].fe() == fe);
        JS_ASSERT(regstate[reg].type() == RematInfo::DATA);
        regstate[reg].forget();
        fe->data.invalidate();
    } else {
        JS_ASSERT(fe->data.inMemory());
        reg = allocReg();
        masm.loadPayload(addressOf(fe), reg);
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
            fe->setType(backing->getKnownType());
        else
            fe->type.invalidate();
        fe->isNumber = backing->isNumber;
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
FrameState::uncopy(FrameEntry *original)
{
    JS_ASSERT(original->isCopied());

    





















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
        original->copied = false;
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

    FrameEntry *fe = bestFe;

    




    if (!original->isTypeKnown()) {
        




        if (original->type.inMemory() && !fe->type.synced())
            tempRegForType(original);
        fe->type.inherit(original->type);
        if (fe->type.inRegister())
            regstate[fe->type.reg()].reassociate(fe);
    } else {
        JS_ASSERT(fe->isTypeKnown());
        JS_ASSERT(fe->getKnownType() == original->getKnownType());
    }
    if (original->data.inMemory() && !fe->data.synced())
        tempRegForData(original);
    fe->data.inherit(original->data);
    if (fe->data.inRegister())
        regstate[fe->data.reg()].reassociate(fe);

    return fe;
}

void
FrameState::storeLocal(uint32 n, bool popGuaranteed, bool typeChange)
{
    FrameEntry *localFe = getLocal(n);
    bool cacheable = !eval && !escaping[n];

    if (!popGuaranteed && !cacheable) {
        JS_ASSERT_IF(locals[n].isTracked() && (!eval || n < script->nfixed),
                     locals[n].type.inMemory() &&
                     locals[n].data.inMemory());
        Address local(JSFrameReg, sizeof(JSStackFrame) + n * sizeof(Value));
        storeTo(peek(-1), local, false);
        forgetAllRegs(getLocal(n));
        localFe->resetSynced();
        return;
    }

    bool wasSynced = localFe->type.synced();

    
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

        if (backing < localFe) {
            
            if (localFe->trackerIndex() < backing->trackerIndex())
                swapInTracker(backing, localFe);
            localFe->setNotCopied();
            localFe->setCopyOf(backing);
            if (backing->isTypeKnown())
                localFe->setType(backing->getKnownType());
            else
                localFe->type.invalidate();
            localFe->data.invalidate();
            localFe->isNumber = backing->isNumber;
            return;
        }

        

















        for (uint32 i = backing->trackerIndex() + 1; i < tracker.nentries; i++) {
            FrameEntry *fe = tracker[i];
            if (fe >= sp)
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
    regstate[reg].reassociate(localFe);

    if (typeChange) {
        if (backing->isTypeKnown()) {
            localFe->setType(backing->getKnownType());
        } else {
            RegisterID reg = tempRegForType(backing);
            localFe->type.setRegister(reg);
            regstate[reg].reassociate(localFe);
        }
    } else {
        if (!wasSynced)
            masm.storeTypeTag(ImmType(backing->getKnownType()), addressOf(localFe));
        localFe->type.setMemory();
    }

    if (!backing->isTypeKnown())
        backing->type.invalidate();
    backing->data.invalidate();
    backing->setCopyOf(localFe);
    backing->isNumber = localFe->isNumber;
    localFe->setCopied();

    if (!cacheable) {
        
        if (!localFe->type.synced())
            syncType(localFe, addressOf(localFe), masm);
        if (!localFe->data.synced())
            syncData(localFe, addressOf(localFe), masm);
        forgetAllRegs(localFe);
        localFe->type.setMemory();
        localFe->data.setMemory();
    }

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
    if (!fe->isTypeKnown()) {
        alloc.lhsType = tempRegForType(fe);
        pinReg(alloc.lhsType.reg());
    }

    alloc.lhsData = tempRegForData(fe);

    if (!freeRegs.empty()) {
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

    bool commu;
    switch (op) {
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

    if (!needsResult)
        goto skip;

    





    if (!freeRegs.empty()) {
        
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

