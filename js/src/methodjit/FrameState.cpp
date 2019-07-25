





































#include "jscntxt.h"
#include "FrameState.h"
#include "FrameState-inl.h"

using namespace js;
using namespace js::mjit;

FrameState::FrameState(JSContext *cx, JSScript *script, Assembler &masm)
  : cx(cx), script(script), masm(masm)
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
    if (!nslots)
        return true;

    uint8 *cursor = (uint8 *)cx->malloc(sizeof(FrameEntry) * nslots +       
                                        sizeof(FrameEntry *) * nslots +     
                                        sizeof(uint32 *) * nslots           
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

    tracker.entries = (uint32 *)cursor;

    return true;
}

void
FrameState::evictSomething()
{
    JS_NOT_REACHED("NYI");
}

void
FrameState::forgetEverything()
{
    for (uint32 i = 0; i < tracker.nentries; i++) {
        uint32 index = tracker[i];
        base[index] = NULL;

        if (index >= tos())
            continue;

        FrameEntry *fe = &entries[index];
        if (!fe->data.synced())
            syncData(fe, masm);
        if (!fe->type.synced())
            syncType(fe, masm);
    }

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

    if (fe->data.inRegister()) {
        masm.storeData32(fe->data.reg(), addressOf(fe));
    } else {
        RegisterID reg = popped ? alloc() : alloc(fe, RematInfo::DATA, true);
        masm.loadData32(addressOf(fe), reg);
        masm.storeData32(reg, addressOf(fe));
        if (popped)
            freeReg(reg);
        else
            fe->data.setRegister(reg);
    }

    if (fe->isTypeKnown()) {
        masm.storeTypeTag(Imm32(fe->getTypeTag()), addressOf(fe));
    } else if (fe->type.inRegister()) {
        masm.storeTypeTag(fe->type.reg(), addressOf(fe));
    } else {
        RegisterID reg = popped ? alloc() : alloc(fe, RematInfo::TYPE, true);
        masm.loadTypeTag(addressOf(fe), reg);
        masm.storeTypeTag(reg, addressOf(fe));
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

    uint32 tos = uint32(sp - base);

    for (uint32 i = 0; i < tracker.nentries; i++) {
        uint32 index = tracker[i];

        if (index >= tos)
            continue;

        JS_ASSERT(base[index]);
        FrameEntry *fe = &entries[index];
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
    for (uint32 i = 0; i < tracker.nentries; i++) {
        uint32 index = tracker[i];

        if (index >= tos())
            continue;

        FrameEntry *fe = &entries[index];
        if (!fe->data.synced()) {
            syncData(fe, masm);
            if (fe->isConstant())
                continue;
        }
        if (!fe->type.synced())
            syncType(fe, masm);
    }
}

void
FrameState::syncAndKill(uint32 mask)
{
    Registers kill(mask);

    for (uint32 i = 0; i < tracker.nentries; i++) {
        uint32 index = tracker[i];

        if (index >= tos())
            continue;

        FrameEntry *fe = &entries[index];
        if (!fe->data.synced()) {
            syncData(fe, masm);
            fe->data.sync();
            if (fe->isConstant())
                fe->type.sync();
        }
        if (fe->data.inRegister() && kill.hasReg(fe->data.reg())) {
            JS_ASSERT(fe->data.synced());
            forgetReg(fe->data.reg());
            fe->data.setMemory();
        }
        if (!fe->type.synced()) {
            syncType(fe, masm);
            fe->type.sync();
        }
        if (fe->type.inRegister() && kill.hasReg(fe->type.reg())) {
            JS_ASSERT(fe->type.synced());
            forgetReg(fe->type.reg());
            fe->type.setMemory();
        }
    }
}

void
FrameState::merge(Assembler &masm, uint32 iVD) const
{
    for (uint32 i = 0; i < tracker.nentries; i++) {
        uint32 index = tracker[i];

        if (index >= tos())
            continue;

        FrameEntry *fe = &entries[index];
        if (fe->data.inRegister())
            masm.loadData32(addressOf(fe), fe->data.reg());
        if (fe->type.inRegister())
            masm.loadTypeTag(addressOf(fe), fe->type.reg());
    }
}

