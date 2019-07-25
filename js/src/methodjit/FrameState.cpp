





































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

