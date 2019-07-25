





































#include "FrameEntry.h"
#include "FrameState.h"
#include "FrameState-inl.h"
#include "ImmutableSync.h"

using namespace js;
using namespace js::mjit;

ImmutableSync::ImmutableSync(JSContext *cx, const FrameState &frame)
  : cx(cx), entries(NULL), frame(frame)
{
}

ImmutableSync::~ImmutableSync()
{
    cx->free(entries);
}

bool
ImmutableSync::init(uint32 nentries)
{
    entries = (SyncEntry *)cx->malloc(sizeof(SyncEntry) * nentries);
    return !!entries;
}

void
ImmutableSync::reset(Assembler *masm, Registers avail, uint32 n)
{
    this->avail = avail;
    this->nentries = n;
    this->masm = masm;
    memset(entries, 0, sizeof(SyncEntry) * nentries);
    memset(regs, 0, sizeof(regs));
}

JSC::MacroAssembler::RegisterID
ImmutableSync::allocReg()
{
    if (!avail.empty())
        return avail.takeAnyReg();

    uint32 lastResort = FrameState::InvalidIndex;
    uint32 evictFromFrame = FrameState::InvalidIndex;

    
    for (uint32 i = 0; i < JSC::MacroAssembler::TotalRegisters; i++) {
        RegisterID reg = RegisterID(i);
        if (!(Registers::maskReg(reg) & Registers::AvailRegs))
            continue;

        lastResort = 0;

        if (!regs[i]) {
            
            FrameEntry *fe = frame.regstate[i].fe;
            if (!fe)
                return reg;

            




            JS_ASSERT(fe->trackerIndex() < nentries);

            evictFromFrame = i;

            



            if (!fe->isCopied())
                break;
        }
    }

    if (evictFromFrame != FrameState::InvalidIndex) {
        FrameEntry *fe = frame.regstate[evictFromFrame].fe;
        SyncEntry &e = entryFor(fe);
        if (frame.regstate[evictFromFrame].type == RematInfo::TYPE) {
            JS_ASSERT(!e.typeClobbered);
            e.typeSynced = true;
            e.typeClobbered = true;
            masm->storeTypeTag(fe->type.reg(), frame.addressOf(fe));
        } else {
            JS_ASSERT(!e.dataClobbered);
            e.dataSynced = true;
            e.dataClobbered = true;
            masm->storeData32(fe->data.reg(), frame.addressOf(fe));
        }
        return RegisterID(evictFromFrame);
    }

    JS_ASSERT(lastResort != FrameState::InvalidIndex);
    JS_ASSERT(regs[lastResort]);

    SyncEntry *e = regs[lastResort];
    RegisterID reg = RegisterID(lastResort);
    if (e->hasDataReg && e->dataReg == reg) {
        e->hasDataReg = false;
    } else if (e->hasTypeReg && e->typeReg == reg) {
        e->hasTypeReg = false;
    } else {
        JS_NOT_REACHED("no way");
    }

    return reg;
}

inline ImmutableSync::SyncEntry &
ImmutableSync::entryFor(FrameEntry *fe)
{
    JS_ASSERT(fe->trackerIndex() < nentries);
    return entries[fe->trackerIndex()];
}

void
ImmutableSync::sync(FrameEntry *fe)
{
    JS_ASSERT(nentries);
    if (fe->isCopy())
        syncCopy(fe);
    else
        syncNormal(fe);
    nentries--;
}

JSC::MacroAssembler::RegisterID
ImmutableSync::ensureTypeReg(FrameEntry *fe, SyncEntry &e)
{
    if (fe->type.inRegister() && !e.typeClobbered)
        return fe->type.reg();
    if (e.hasTypeReg)
        return e.typeReg;
    e.typeReg = allocReg();
    e.hasTypeReg = true;
    regs[e.typeReg] = &e;
    masm->loadTypeTag(frame.addressOf(fe), e.typeReg);
    return e.typeReg;
}

JSC::MacroAssembler::RegisterID
ImmutableSync::ensureDataReg(FrameEntry *fe, SyncEntry &e)
{
    if (fe->data.inRegister() && !e.dataClobbered)
        return fe->data.reg();
    if (e.hasDataReg)
        return e.dataReg;
    e.dataReg = allocReg();
    e.hasDataReg = true;
    regs[e.dataReg] = &e;
    masm->loadData32(frame.addressOf(fe), e.dataReg);
    return e.dataReg;
}

void
ImmutableSync::syncCopy(FrameEntry *fe)
{
    FrameEntry *backing = fe->copyOf();
    SyncEntry &e = entryFor(backing);

    JS_ASSERT(!backing->isConstant());

    Address addr = frame.addressOf(fe);

    if (fe->isTypeKnown() && !e.learnedType) {
        e.learnedType = true;
        e.typeTag = fe->getKnownTag();
    }

    if (!fe->data.synced())
        masm->storeData32(ensureDataReg(backing, e), addr);

    if (!fe->type.synced()) {
        if (e.learnedType)
            masm->storeTypeTag(ImmTag(e.typeTag), addr);
        else
            masm->storeTypeTag(ensureTypeReg(backing, e), addr);
    }
}

void
ImmutableSync::syncNormal(FrameEntry *fe)
{
    SyncEntry &e = entryFor(fe);

    Address addr = frame.addressOf(fe);

    if (fe->isTypeKnown()) {
        e.learnedType = true;
        e.typeTag = fe->getKnownTag();
    }

    if (!fe->data.synced() && !e.dataSynced) {
        if (fe->isConstant()) {
            masm->storeValue(fe->getValue(), addr);
            return;
        }
        masm->storeData32(ensureDataReg(fe, e), addr);
    }

    if (!fe->type.synced() && !e.typeSynced) {
        if (e.learnedType)
            masm->storeTypeTag(ImmTag(e.typeTag), addr);
        else
            masm->storeTypeTag(ensureTypeReg(fe, e), addr);
    }

    if (e.hasDataReg) {
        avail.putReg(e.dataReg);
        regs[e.dataReg] = NULL;
    } else if (!e.dataClobbered && fe->data.inRegister()) {
        avail.putReg(fe->data.reg());
    }

    if (e.hasTypeReg) {
        avail.putReg(e.typeReg);
        regs[e.typeReg] = NULL;
    } else if (!e.typeClobbered && fe->type.inRegister()) {
        avail.putReg(fe->type.reg());
    }
}

