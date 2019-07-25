





































#include "jscntxt.h"
#include "FrameState.h"

using namespace js;
using namespace js::mjit;

bool
FrameState::init(uint32 nargs)
{
    base = (FrameEntry *)cx->malloc(sizeof(FrameEntry) * (script->nslots + nargs));
    if (!base)
        return false;

    memset(base, 0, sizeof(FrameEntry) * (script->nslots + nargs));
    memset(regstate, 0, sizeof(regstate));

    args = base;
    locals = args + nargs;
    sp = locals + script->nfixed;

    return true;
}

void
FrameState::evictSomething()
{
}

FrameState::~FrameState()
{
    cx->free(base);
}

void
FrameState::assertValidRegisterState()
{
    Registers temp;

    RegisterID reg;
    uint32 index = 0;
    for (FrameEntry *fe = base; fe < sp; fe++, index++) {
        if (fe->type.inRegister()) {
            reg = fe->type.reg();
            temp.allocSpecific(reg);
            JS_ASSERT(regstate[reg].tracked);
            JS_ASSERT(regstate[reg].index == index);
            JS_ASSERT(regstate[reg].part == RegState::Part_Type);
        }
        if (fe->data.inRegister()) {
            reg = fe->data.reg();
            temp.allocSpecific(reg);
            JS_ASSERT(regstate[reg].tracked);
            JS_ASSERT(regstate[reg].index == index);
            JS_ASSERT(regstate[reg].part == RegState::Part_Data);
        }
    }

    JS_ASSERT(temp == regalloc);
}

void
FrameState::invalidate(FrameEntry *fe)
{
    if (!fe->type.synced()) {
        JS_NOT_REACHED("wat");
    }
    if (!fe->data.synced()) {
        JS_NOT_REACHED("wat2");
    }
    fe->type.setMemory();
    fe->data.setMemory();
    fe->copies = 0;
}

void
FrameState::flush()
{
    for (FrameEntry *fe = base; fe < sp; fe++)
        invalidate(fe);
}

void
FrameState::syncRegister(Assembler &masm, RegisterID reg, const RegState &state) const
{
    JS_NOT_REACHED("bleh");
}

void
FrameState::syncType(FrameEntry *fe, Assembler &masm) const
{
}

void
FrameState::syncData(FrameEntry *fe, Assembler &masm) const
{
}

void
FrameState::sync(Assembler &masm) const
{
    for (FrameEntry *fe = base; fe < sp; fe++) {
        if (fe->type.needsSync())
            syncType(fe, masm);
        if (fe->data.needsSync())
            syncData(fe, masm);
    }
}

void
FrameState::restoreTempRegs(Assembler &masm) const
{
#if 0
    
    Registers temps = regalloc.freeMask & Registers::TempRegs;

    while (temps.anyRegsFree()) {
        RegisterID reg = temps.allocReg();
        if (!regstate[reg].tracked)
            continue;
        syncRegister(masm, reg, regstate[reg]);
    }
#endif
}

