





































#include "jscntxt.h"
#include "FrameState.h"
#include "FrameState-inl.h"

using namespace js;
using namespace js::mjit;


JS_STATIC_ASSERT(sizeof(FrameEntry) % 8 == 0);

FrameState::FrameState(JSContext *cx, JSScript *script, Assembler &masm)
  : cx(cx), script(script), masm(masm), entries(NULL)
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

    uint8 *cursor = (uint8 *)cx->malloc(sizeof(FrameEntry) * nslots +       
                                        sizeof(FrameEntry *) * nslots +     
                                        sizeof(FrameEntry *) * nslots       
                                        );
    if (!cursor)
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
        syncType(fe, addressOf(fe), masm);
        fe->type.sync();
        fe->type.setMemory();
    } else {
        syncData(fe, addressOf(fe), masm);
        fe->data.sync();
        fe->data.setMemory();
    }
}

JSC::MacroAssembler::RegisterID
FrameState::evictSomething(uint32 mask)
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
        RegisterID reg = popped ? alloc() : alloc(fe, RematInfo::DATA, true);
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
        RegisterID reg = popped ? alloc() : alloc(fe, RematInfo::TYPE, true);
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
FrameState::sync(Assembler &masm) const
{
    Registers avail(freeRegs);

    FrameEntry *tos = tosFe();
    for (uint32 i = 0; i < tracker.nentries; i++) {
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

            bool allocd = false;
            RegisterID reg = Registers::ReturnReg;
            if (backing->type.inMemory() || backing->data.inMemory()) {
                
                JS_ASSERT(!avail.empty());

                




                reg = avail.takeAnyReg();
                allocd = true;
            }

            if (!fe->type.synced()) {
                
                if (fe->isTypeKnown()) {
                    
                    masm.storeTypeTag(ImmTag(fe->getTypeTag()), address);
                } else {
                    RegisterID r;
                    if (backing->type.inRegister()) {
                        r = backing->type.reg();
                    } else {
                        masm.loadTypeTag(addressOf(backing), reg);
                        r = reg;
                    }
                    masm.storeTypeTag(r, address);
                }
            }

            if (!fe->data.synced()) {
                RegisterID r;
                if (backing->data.inRegister()) {
                    r = backing->data.reg();
                } else {
                    masm.loadData32(addressOf(backing), reg);
                    r = reg;
                }
                masm.storeData32(r, address);
            }

            if (allocd)
                avail.putReg(reg);
        }
    }
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
FrameState::copyData(FrameEntry *fe)
{
    JS_ASSERT(!fe->data.isConstant());
    JS_ASSERT(!fe->isCopy());

    if (fe->data.inRegister()) {
        RegisterID reg = fe->data.reg();
        if (freeRegs.empty()) {
            if (!fe->data.synced())
                syncData(fe, addressOf(fe), masm);
            fe->data.setMemory();
            regstate[reg].fe = NULL;
        } else {
            RegisterID newReg = alloc();
            masm.move(reg, newReg);
            reg = newReg;
        }
        return reg;
    }

    RegisterID reg = alloc();

    if (!freeRegs.empty())
        masm.move(tempRegForData(fe), reg);
    else
        masm.loadData32(addressOf(fe),reg);

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
            reg = alloc();
            masm.move(backing->data.reg(), reg);
        }
    } else if (fe->data.inRegister()) {
        reg = fe->data.reg();
        
        JS_ASSERT(regstate[reg].fe == fe);
        JS_ASSERT(regstate[reg].type == RematInfo::DATA);
        regstate[reg].fe = NULL;
        fe->data.invalidate();
    } else {
        JS_ASSERT(fe->data.inMemory());
        reg = alloc();
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
FrameState::storeLocal(uint32 n)
{
    FrameEntry *localFe = getLocal(n);

    
    FrameEntry *top = peek(-1);
    if (top->isCopy() && top->copyOf() == localFe) {
        JS_ASSERT(localFe->isCopied());
        return;
    }

    
    if (localFe->isCopied()) {
        uncopy(localFe);
        if (!localFe->isCopied())
            forgetRegs(localFe);
    } else {
        forgetRegs(localFe);
    }

    localFe->resetUnsynced();

    
    if (top->isConstant()) {
        localFe->setCopyOf(NULL);
        localFe->setNotCopied();
        localFe->setConstant(Jsvalify(top->getValue()));
        return;
    }

    









    FrameEntry *backing = top;
    uint32 searchPoint = InvalidIndex;
    if (top->isCopy()) {
        backing = top->copyOf();
        JS_ASSERT(backing->trackerIndex() < top->trackerIndex());

        if (indexOfFe(backing) < uint32(spBase - base)) {
            
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

        searchPoint = backing->trackerIndex();
    } else if (top->trackerIndex() < localFe->trackerIndex()) {
        swapInTracker(top, localFe);
    }

    



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
    backing->setNotCopied();
    backing->setCopyOf(localFe);

    JS_ASSERT(searchPoint == InvalidIndex);
    JS_ASSERT(top->copyOf() == localFe);
}

void
FrameState::popAfterSet()
{
    FrameEntry *top = peek(-1);
    FrameEntry *down = peek(-2);

    forgetRegs(down);

    if (top->isConstant()) {
        down->setConstant(Jsvalify(top->getValue()));
    } else {
        down->type.unsync();
        if (top->isTypeKnown()) {
            down->setTypeTag(top->getTypeTag());
        } else {
            down->type.inherit(top->type);
            if (top->type.inRegister())
                moveOwnership(top->type.reg(), down);
        }
        down->data.unsync();
        down->data.inherit(top->data);
        if (top->data.inRegister())
            moveOwnership(top->data.reg(), down);
    }

    
    sp--;
}

