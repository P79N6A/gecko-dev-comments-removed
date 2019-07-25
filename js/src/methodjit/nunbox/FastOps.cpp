






































#include "methodjit/MethodJIT.h"
#include "methodjit/Compiler.h"
#include "methodjit/StubCalls.h"

#include "jsautooplen.h"

using namespace js;
using namespace js::mjit;

static const uint32 TAG_OFFSET     = 4;
static const uint32 PAYLOAD_OFFSET = 0;

void
mjit::Compiler::jsop_bindname(uint32 index)
{
    RegisterID reg = frame.allocReg();
    masm.loadPtr(Address(FrameState::FpReg, offsetof(JSStackFrame, scopeChain)), reg);

    Address address(reg, offsetof(JSObject, fslots) + JSSLOT_PARENT * sizeof(jsval));

    masm.load32(Address(address.base, address.offset + PAYLOAD_OFFSET), reg);
    Jump j = masm.branchTestPtr(Assembler::Zero, reg, reg);

    {
        stubcc.linkExit(j);
        stubcc.syncAndSpill();
        stubcc.call(stubs::BindName);
    }

    frame.pushObject(reg);
}

