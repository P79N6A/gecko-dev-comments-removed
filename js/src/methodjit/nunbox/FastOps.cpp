






































#include "methodjit/MethodJIT.h"
#include "methodjit/Compiler.h"

#include "jsautooplen.h"

using namespace js;

static const uint32 TAG_OFFSET     = 4;
static const uint32 PAYLOAD_OFFSET = 0;

void
mjit::Compiler::jsop_bindname(uint32 index)
{
    RegisterID reg = frame.allocReg();
    masm.loadPtr(Address(FrameState::FpReg, offsetof(JSStackFrame, scopeChain)), reg);

    Address address(reg, offsetof(JSObject, fslots) + JSSLOT_PARENT * sizeof(jsval));

    masm.load32(Address(address.base, address.offset + PAYLOAD_OFFSET), reg);
#ifdef JS_64BIT
# error "Bleh!"
#endif

    Jump j = masm.branchTestPtr(MacroAssembler::Zero, reg, reg);

    frame.pushObject(reg);
}

