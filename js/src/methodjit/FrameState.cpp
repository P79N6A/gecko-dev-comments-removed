





































#include "jscntxt.h"
#include "FrameState.h"
#include "FrameState-inl.h"

using namespace js;
using namespace js::mjit;


JS_STATIC_ASSERT(sizeof(FrameEntry) % 8 == 0);

FrameState::FrameState(JSContext *cx, JSScript *script, JSFunction *fun, Assembler &masm)
  : cx(cx), script(script), fun(fun),
    nargs(fun ? fun->nargs : 0),
    masm(masm), entries(NULL),
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
    cx->free_(entries);
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
                        (eval
                         ? 0
                         : sizeof(JSPackedBool) * script->nslots) +         
                        (eval || usesArguments
                         ? 0
                         : sizeof(JSPackedBool) * nargs);                   

    uint8 *cursor = (uint8 *)cx->calloc_(totalBytes);
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
        ensureTypeSynced(fe, masm);
        fe->type.setMemory();
    } else {
        ensureDataSynced(fe, masm);
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
    syncAndKill(Registers(Registers::AvailRegs), Uses(frameSlots()));
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

    
    JS_ASSERT_IF((fe->type.inMemory() || fe->data.inMemory()),
                 addressOf(fe).base != address.base ||
                 addressOf(fe).offset != address.offset);

#if defined JS_PUNBOX64
    if (fe->type.inMemory() && fe->data.inMemory()) {
        
        RegisterID vreg = Registers::ValueReg;
        masm.loadPtr(addressOf(fe), vreg);
        masm.storePtr(vreg, address);
        return;
    }

    





    bool canPinDreg = true;
    bool wasInRegister = fe->data.inRegister();

    
    MaybeRegisterID dreg;
    if (fe->data.inRegister()) {
        dreg = fe->data.reg();
    } else {
        JS_ASSERT(fe->data.inMemory());
        if (popped) {
            dreg = allocReg();
            canPinDreg = false;
        } else {
            dreg = allocReg(fe, RematInfo::DATA);
            fe->data.setRegister(dreg.reg());
        }
        masm.loadPayload(addressOf(fe), dreg.reg());
    }
    
    
    if (fe->type.inRegister()) {
        masm.storeValueFromComponents(fe->type.reg(), dreg.reg(), address);
    } else if (fe->isTypeKnown()) {
        masm.storeValueFromComponents(ImmType(fe->getKnownType()), dreg.reg(), address);
    } else {
        JS_ASSERT(fe->type.inMemory());
        if (canPinDreg)
            pinReg(dreg.reg());

        RegisterID treg = popped ? allocReg() : allocReg(fe, RematInfo::TYPE);
        masm.loadTypeTag(addressOf(fe), treg);
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
        RegisterID reg = popped ? allocReg() : allocReg(fe, RematInfo::DATA);
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
        RegisterID reg = popped ? allocReg() : allocReg(fe, RematInfo::TYPE);
        masm.loadTypeTag(addressOf(fe), reg);
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

    
    Registers allRegs(Registers::AvailRegs);
    while (!allRegs.empty()) {
        RegisterID reg = allRegs.takeAnyReg();
        FrameEntry *fe = regstate[reg].usedBy();
        if (!fe)
            continue;

        JS_ASSERT(fe->isTracked());

#if defined JS_PUNBOX64
        
        ensureFeSynced(fe, masm);

        
        if (regstate[reg].type() == RematInfo::DATA && fe->type.inRegister())
            allRegs.takeReg(fe->type.reg());
        else if (regstate[reg].type() == RematInfo::TYPE && fe->data.inRegister())
            allRegs.takeReg(fe->data.reg());
#elif defined JS_NUNBOX32
        
        if (regstate[reg].type() == RematInfo::DATA) {
            JS_ASSERT(fe->data.reg() == reg);
            ensureDataSynced(fe, masm);
        } else {
            JS_ASSERT(fe->type.reg() == reg);
            ensureTypeSynced(fe, masm);
        }
#endif
    }

    



    Registers avail(freeRegs);
    Registers temp(Registers::TempRegs);

    FrameEntry *bottom = sp - uses.nuses;

    for (FrameEntry *fe = sp - 1; fe >= bottom; fe--) {
        if (!fe->isTracked())
            continue;

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
    FrameEntry *spStop = sp - ignore.nuses;

    
    Registers search(kill.freeMask & ~freeRegs.freeMask);
    while (!search.empty()) {
        RegisterID reg = search.takeAnyReg();
        FrameEntry *fe = regstate[reg].usedBy();
        if (!fe || fe >= spStop)
            continue;

        JS_ASSERT(fe->isTracked());

#if defined JS_PUNBOX64
        
        ensureFeSynced(fe, masm);

        if (!fe->type.synced())
            fe->type.sync();
        if (!fe->data.synced())
            fe->data.sync();

        
        if (regstate[reg].type() == RematInfo::DATA) {
            JS_ASSERT(fe->data.reg() == reg);
            if (fe->type.inRegister() && search.hasReg(fe->type.reg()))
                search.takeReg(fe->type.reg());
        } else {
            JS_ASSERT(fe->type.reg() == reg);
            if (fe->data.inRegister() && search.hasReg(fe->data.reg()))
                search.takeReg(fe->data.reg());
        }
#elif defined JS_NUNBOX32
        
        if (regstate[reg].type() == RematInfo::DATA) {
            JS_ASSERT(fe->data.reg() == reg);
            syncData(fe);
        } else {
            JS_ASSERT(fe->type.reg() == reg);
            syncType(fe);
        }
#endif
    }

    uint32 maxvisits = tracker.nentries;
    FrameEntry *bottom = sp - uses.nuses;

    for (FrameEntry *fe = sp - 1; fe >= bottom && maxvisits; fe--) {
        if (!fe->isTracked())
            continue;

        maxvisits--;

        if (fe >= spStop)
            continue;

        syncFe(fe);

        
        if (fe->data.inRegister() && kill.hasReg(fe->data.reg()) &&
            !regstate[fe->data.reg()].isPinned()) {
            forgetReg(fe->data.reg());
            fe->data.setMemory();
        }
        if (fe->type.inRegister() && kill.hasReg(fe->type.reg()) &&
            !regstate[fe->type.reg()].isPinned()) {
            forgetReg(fe->type.reg());
            fe->type.setMemory();
        }
    }

    



    search = Registers(kill.freeMask & ~freeRegs.freeMask);
    while (!search.empty()) {
        RegisterID reg = search.takeAnyReg();
        FrameEntry *fe = regstate[reg].usedBy();
        if (!fe || fe >= spStop)
            continue;

        JS_ASSERT(fe->isTracked());

        if (regstate[reg].type() == RematInfo::DATA) {
            JS_ASSERT(fe->data.reg() == reg);
            JS_ASSERT(fe->data.synced());
            fe->data.setMemory();
        } else {
            JS_ASSERT(fe->type.reg() == reg);
            JS_ASSERT(fe->type.synced());
            fe->type.setMemory();
        }

        forgetReg(reg);
    }
}

void
FrameState::merge(Assembler &masm, Changes changes) const
{
    Registers search(Registers::AvailRegs & ~freeRegs.freeMask);

    while (!search.empty()) {
        RegisterID reg = search.peekReg();
        FrameEntry *fe = regstate[reg].usedBy();

        if (!fe) {
            search.takeReg(reg);
            continue;
        }

        if (fe->data.inRegister() && fe->type.inRegister()) {
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

    if (fe->isCopy())
        fe = fe->copyOf();

    if (!fe->data.inRegister())
        tempRegForData(fe);

    RegisterID reg = fe->data.reg();
    if (reg == hint) {
        if (freeRegs.empty()) {
            ensureDataSynced(fe, masm);
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
            ensureDataSynced(fe, masm);
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
            ensureTypeSynced(fe, masm);
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

    ensureFeSynced(fe, masm);
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
            
            ensureTypeSynced(backing, masm);
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
            
            ensureDataSynced(backing, masm);
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
FrameState::discardFe(FrameEntry *fe)
{
    forgetEntry(fe);
    fe->type.setMemory();
    fe->data.setMemory();
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
FrameState::finishStore(FrameEntry *fe, bool closed)
{
    
    
    
    syncFe(fe);
    if (closed) {
        if (!fe->isCopy())
            forgetEntry(fe);
        fe->resetSynced();
    }
}

void
FrameState::storeLocal(uint32 n, bool popGuaranteed, bool typeChange)
{
    FrameEntry *local = getLocal(n);
    storeTop(local, popGuaranteed, typeChange);

    bool closed = isClosedVar(n);
    if (!closed && !inTryBlock)
        return;

    finishStore(local, closed);
}

void
FrameState::storeArg(uint32 n, bool popGuaranteed)
{
    
    
    FrameEntry *arg = getArg(n);
    storeTop(arg, popGuaranteed, true);
    finishStore(arg, isClosedArg(n));
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
}

void
FrameState::storeTop(FrameEntry *target, bool popGuaranteed, bool typeChange)
{
    bool wasSynced = target->type.synced();
    
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
            target->isNumber = backing->isNumber;
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

    



    RegisterID reg = tempRegForData(backing);
    target->data.setRegister(reg);
    regstate[reg].reassociate(target);

    if (typeChange) {
        if (backing->isTypeKnown()) {
            target->setType(backing->getKnownType());
        } else {
            RegisterID reg = tempRegForType(backing);
            target->type.setRegister(reg);
            regstate[reg].reassociate(target);
        }
    } else {
        if (!wasSynced)
            masm.storeTypeTag(ImmType(backing->getKnownType()), addressOf(target));
        target->type.setMemory();
    }

    if (!backing->isTypeKnown())
        backing->type.invalidate();
    backing->data.invalidate();
    backing->setCopyOf(target);
    backing->isNumber = target->isNumber;

    JS_ASSERT(top->copyOf() == target);

    








    if (copied || !popGuaranteed)
        target->setCopied();
}

void
FrameState::shimmy(uint32 n)
{
    JS_ASSERT(sp - n >= spBase);
    int32 depth = 0 - int32(n);
    storeTop(peek(depth - 1), true);
    popn(n);
}

void
FrameState::shift(int32 n)
{
    JS_ASSERT(n < 0);
    JS_ASSERT(sp + n - 1 >= spBase);
    storeTop(peek(n - 1), true);
    pop();
}

void
FrameState::pinEntry(FrameEntry *fe, ValueRemat &vr)
{
    if (fe->isConstant()) {
        vr = ValueRemat::FromConstant(fe->getValue());
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

