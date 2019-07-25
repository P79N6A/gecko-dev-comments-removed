





































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
}

void
FrameState::reset(FrameEntry *fe)
{
    fe->type.setMemory();
    fe->data.setMemory();
}

void
FrameState::flush()
{
    for (FrameEntry *fe = base; fe < sp; fe++)
        invalidate(fe);
}

void
FrameState::killSyncedRegs(uint32 mask)
{
    
    Registers regs(mask & ~(regalloc.freeMask));

    while (regs.anyRegsFree()) {
        RegisterID reg = regs.allocReg();
        if (!regstate[reg].tracked)
            continue;
        regstate[reg].tracked = false;
        regalloc.freeReg(reg);
        FrameEntry &fe = base[regstate[reg].index];
        RematInfo *mat;
        if (regstate[reg].part == RegState::Part_Type)
            mat = &fe.type;
        else
            mat = &fe.data;
        JS_ASSERT(mat->inRegister() && mat->reg() == reg);
        JS_ASSERT(mat->synced());
        mat->setMemory();
    }
}

void
FrameState::syncType(FrameEntry *fe, Assembler &masm) const
{
    JS_ASSERT(!fe->type.synced() && !fe->type.inMemory());
    if (fe->type.isConstant())
        masm.storeTypeTag(Imm32(fe->getTypeTag()), addressOf(fe));
    else
        masm.storeTypeTag(fe->type.reg(), addressOf(fe));
    fe->type.setSynced();
}

void
FrameState::syncData(FrameEntry *fe, Assembler &masm) const
{
    JS_ASSERT(!fe->data.synced() && !fe->data.inMemory());
    if (fe->data.isConstant())
        masm.storeData32(Imm32(fe->getPayload32()), addressOf(fe));
    else
        masm.storeData32(fe->data.reg(), addressOf(fe));
    fe->data.setSynced();
}

void
FrameState::sync(Assembler &masm, RegSnapshot &snapshot) const
{
    for (FrameEntry *fe = base; fe < sp; fe++) {
        if (fe->isConstant()) {
            JS_ASSERT(fe->type.synced() == fe->data.synced());
            if (!fe->data.synced())
                masm.storeValue(fe->getValue(), addressOf(fe));
        } else {
            if (fe->type.needsSync())
                syncType(fe, masm);
            if (fe->data.needsSync())
                syncData(fe, masm);
        }
    }

    JS_STATIC_ASSERT(sizeof(snapshot.regs) == sizeof(regstate));
    JS_STATIC_ASSERT(sizeof(RegState) == sizeof(uint32));
    memcpy(snapshot.regs, regstate, sizeof(regstate));
    snapshot.alloc = regalloc;
}

void
FrameState::merge(Assembler &masm, const RegSnapshot &snapshot, uint32 invalidationDepth) const
{
    uint32 threshold = uint32(sp - base) - invalidationDepth;

    











    Registers depends(snapshot.alloc);

    for (uint32 i = 0; i < MacroAssembler::TotalRegisters; i++) {
        if (!regstate[i].tracked)
            continue;

        if (regstate[i].index >= threshold) {
            





            FrameEntry *fe = &base[regstate[i].index];
            Address address = addressOf(fe);
            RegisterID reg = RegisterID(i);
            if (regstate[i].part == RegState::Part_Type) {
                masm.loadTypeTag(address, reg);
            } else {
                
                JS_ASSERT(fe->isTypeConstant());
                masm.loadData32(address, reg);
            }
        } else if (regstate[i].index != snapshot.regs[i].index ||
                   regstate[i].part != snapshot.regs[i].part) {
            JS_NOT_REACHED("say WAT");
        }
    }
}

