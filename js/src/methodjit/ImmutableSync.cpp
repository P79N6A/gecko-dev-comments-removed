






































#if defined JS_NUNBOX32

#include "FrameEntry.h"
#include "FrameState.h"
#include "FrameState-inl.h"
#include "ImmutableSync.h"

using namespace js;
using namespace js::mjit;

ImmutableSync::ImmutableSync()
  : cx(NULL), entries(NULL), frame(NULL), avail(Registers::AvailRegs), generation(0)
{
}

ImmutableSync::~ImmutableSync()
{
    if (cx)
        cx->free_(entries);
}

bool
ImmutableSync::init(JSContext *cx, const FrameState &frame, uint32 nentries)
{
    this->cx = cx;
    this->frame = &frame;

    entries = (SyncEntry *)cx->calloc_(sizeof(SyncEntry) * nentries);
    return !!entries;
}

void
ImmutableSync::reset(Assembler *masm, Registers avail, FrameEntry *top, FrameEntry *bottom)
{
    this->avail = avail;
    this->masm = masm;
    this->top = top;
    this->bottom = bottom;
    this->generation++;
    memset(regs, 0, sizeof(regs));
}

JSC::MacroAssembler::RegisterID
ImmutableSync::allocReg()
{
    if (!avail.empty())
        return avail.takeAnyReg().reg();

    uint32 lastResort = FrameState::InvalidIndex;
    uint32 evictFromFrame = FrameState::InvalidIndex;

    
    for (uint32 i = 0; i < Registers::TotalRegisters; i++) {
        RegisterID reg = RegisterID(i);
        if (!(Registers::maskReg(reg) & Registers::AvailRegs))
            continue;

        lastResort = i;

        if (!regs[i]) {
            
            FrameEntry *fe = frame->regstate(reg).usedBy();
            if (!fe)
                return reg;

            evictFromFrame = i;

            



            if (!fe->isCopied())
                break;
        }
    }

    if (evictFromFrame != FrameState::InvalidIndex) {
        RegisterID evict = RegisterID(evictFromFrame);
        FrameEntry *fe = frame->regstate(evict).usedBy();
        SyncEntry &e = entryFor(fe);
        if (frame->regstate(evict).type() == RematInfo::TYPE) {
            JS_ASSERT(!e.typeClobbered);
            e.typeClobbered = true;
        } else {
            JS_ASSERT(!e.dataClobbered);
            e.dataClobbered = true;
        }
        return evict;
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
    JS_ASSERT(fe <= top || frame->isTemporary(fe));
    SyncEntry &e = entries[fe - frame->entries];
    if (e.generation != generation)
        e.reset(generation);
    return e;
}

void
ImmutableSync::sync(FrameEntry *fe)
{
    if (fe->isCopy())
        syncCopy(fe);
    else
        syncNormal(fe);
}

bool
ImmutableSync::shouldSyncType(FrameEntry *fe, SyncEntry &e)
{
    
    return !fe->type.synced() && !fe->type.inRegister();
}

bool
ImmutableSync::shouldSyncData(FrameEntry *fe, SyncEntry &e)
{
    
    return !fe->data.synced() && !fe->data.inRegister();
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
    masm->loadTypeTag(frame->addressOf(fe), e.typeReg);
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
    masm->loadPayload(frame->addressOf(fe), e.dataReg);
    return e.dataReg;
}

void
ImmutableSync::syncCopy(FrameEntry *fe)
{
    JS_ASSERT(fe >= bottom);

    FrameEntry *backing = fe->copyOf();
    SyncEntry &e = entryFor(backing);

    JS_ASSERT(!backing->isConstant());

    Address addr = frame->addressOf(fe);

    if (fe->isTypeKnown() && !fe->isType(JSVAL_TYPE_DOUBLE) && !e.learnedType) {
        e.learnedType = true;
        e.type = fe->getKnownType();
    }

    if (!fe->data.synced())
        masm->storePayload(ensureDataReg(backing, e), addr);

    if (!fe->type.synced()) {
        if (e.learnedType)
            masm->storeTypeTag(ImmType(e.type), addr);
        else
            masm->storeTypeTag(ensureTypeReg(backing, e), addr);
    }
}

void
ImmutableSync::syncNormal(FrameEntry *fe)
{
    SyncEntry &e = entryFor(fe);

    Address addr = frame->addressOf(fe);

    if (fe->isTypeKnown() && !fe->isType(JSVAL_TYPE_DOUBLE)) {
        e.learnedType = true;
        e.type = fe->getKnownType();
    }

    if (shouldSyncData(fe, e)) {
        if (fe->isConstant()) {
            masm->storeValue(fe->getValue(), addr);
            return;
        }
        masm->storePayload(ensureDataReg(fe, e), addr);
    }

    if (shouldSyncType(fe, e)) {
        if (e.learnedType)
            masm->storeTypeTag(ImmType(e.type), addr);
        else
            masm->storeTypeTag(ensureTypeReg(fe, e), addr);
    }

    if (e.hasDataReg) {
        avail.putReg(e.dataReg);
        regs[e.dataReg] = NULL;
    } else if (!e.dataClobbered &&
               fe->data.inRegister() &&
               frame->regstate(fe->data.reg()).usedBy()) {
        avail.putReg(fe->data.reg());
    }

    if (e.hasTypeReg) {
        avail.putReg(e.typeReg);
        regs[e.typeReg] = NULL;
    } else if (!e.typeClobbered &&
               fe->type.inRegister() &&
               frame->regstate(fe->type.reg()).usedBy()) {
        avail.putReg(fe->type.reg());
    }
}

#endif 

