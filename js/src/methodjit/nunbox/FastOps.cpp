






































#include "methodjit/MethodJIT.h"
#include "methodjit/Compiler.h"
#include "methodjit/StubCalls.h"

#include "jsautooplen.h"

using namespace js;
using namespace js::mjit;

void
mjit::Compiler::jsop_bindname(uint32 index)
{
    RegisterID reg = frame.allocReg();
    masm.loadPtr(Address(Assembler::FpReg, offsetof(JSStackFrame, scopeChain)), reg);

    Address address(reg, offsetof(JSObject, fslots) + JSSLOT_PARENT * sizeof(jsval));

    Jump j = masm.branch32(Assembler::NotEqual, masm.payloadOf(address), Imm32(0));

    stubcc.linkExit(j);
    stubcc.leave();
    stubcc.call(stubs::BindName);

    frame.pushTypedPayload(JSVAL_MASK32_NONFUNOBJ, reg);

    stubcc.rejoin(1);
}

void
mjit::Compiler::jsop_bitop(JSOp op)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    
    if ((rhs->isTypeKnown() && rhs->getTypeTag() != JSVAL_MASK32_INT32) ||
        (lhs->isTypeKnown() && lhs->getTypeTag() != JSVAL_MASK32_INT32)) {
        prepareStubCall();
        stubCall(stubs::BitAnd, Uses(2), Defs(1));
        frame.popn(2);
        frame.pushSyncedType(JSVAL_MASK32_INT32);
        return;
    }
           
    
    if (!rhs->isTypeKnown()) {
        RegisterID reg = frame.tempRegForType(rhs);
        Jump rhsFail = masm.branch32(Assembler::NotEqual, reg, Imm32(JSVAL_MASK32_INT32));
        stubcc.linkExit(rhsFail);
        if (lhs->isTypeKnown())
            stubcc.leave();
        frame.freeReg(reg);
        
    }
    if (!lhs->isTypeKnown()) {
        RegisterID reg = frame.tempRegForType(lhs);
        Jump lhsFail = masm.branch32(Assembler::NotEqual, reg, Imm32(JSVAL_MASK32_INT32));
        stubcc.linkExit(lhsFail);
        stubcc.leave();
        frame.freeReg(reg);
    }

    stubcc.call(stubs::BitAnd);

    frame.pop();
    frame.pop();
}

