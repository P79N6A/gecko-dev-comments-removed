






































#define __STDC_LIMIT_MACROS

#include "jsapi.h"
#include "jsnum.h"
#include "CodeGenerator.h"

using namespace js;
using namespace js::mjit;

CodeGenerator::CodeGenerator(MacroAssembler &masm, FrameState &frame)
  : masm(masm), frame(frame)
{
}

void
CodeGenerator::storeJsval(const Value &v, Address address)
{
    const jsval jv = Jsvalify(v);
    masm.store32(Imm32(jv.mask), Address(address.base, address.offset + offsetof(jsval, mask)));
    if (v.isNullOrUndefined())
        return;
#ifdef JS_CPU_X64
    masm.storePtr(ImmPtr(jv.data.ptr), Address(address.base, address.offset + offsetof(jsval, data)));
#elif defined(JS_CPU_ARM) || defined(JS_CPU_X86)
    if (v.isDouble()) {
        jsdpun u;
        u.d = jv.data.dbl;
        masm.store32(Imm32(u.s.hi), Address(address.base,
                                            address.offset + offsetof(jsval, data)));
        masm.store32(Imm32(u.s.lo), Address(address.base,
                                            address.offset + offsetof(jsval, data) + 4));
    } else {
        masm.storePtr(ImmPtr(jv.data.ptr), Address(address.base, address.offset + offsetof(jsval, data)));
    }
#else
# error "Unknown architecture"
#endif
    return;
}

void
CodeGenerator::storeValue(FrameEntry *fe, Address address, bool popped)
{
    if (fe->isConstant()) {
        storeJsval(Valueify(fe->getConstant()), address);
        return;
    }

    Address feAddr = frame.addressOf(fe);

    
#ifdef JS_32BIT
    RegisterID tempReg = frame.allocReg();
    masm.loadPtr(Address(feAddr.base, feAddr.offset + offsetof(jsval, data)), tempReg);
    masm.storePtr(tempReg, Address(address.base, address.offset + offsetof(jsval, data)));
    masm.loadPtr(Address(feAddr.base, feAddr.offset + offsetof(jsval, data) + 4), tempReg);
    masm.storePtr(tempReg, Address(address.base, address.offset + offsetof(jsval, data) + 4));
    frame.freeReg(tempReg);
#else
# error "IMPLEMENT ME"
#endif

    
    if (fe->isTypeConstant()) {
        masm.store32(Imm32(fe->getTypeTag()),
                     Address(address.base,
                             address.offset + offsetof(jsval, mask)));
    } else {
        RegisterID reg = frame.tempRegForType(fe);
        masm.store32(reg, Address(address.base, address.offset + offsetof(jsval, mask)));
    }
}

void
CodeGenerator::pushValueOntoFrame(Address address)
{
    RegisterID tempReg = frame.allocReg();

    Address tos = frame.topOfStack();

#ifdef JS_CPU_X64
    masm.loadPtr(Address(address.base, address.offset + offsetof(jsval, data)), tempReg);
    masm.storePtr(tempReg, Address(tos.base, tos.offset + offsetof(jsval, data)));
#elif defined(JS_CPU_ARM) || defined(JS_CPU_X86)
    masm.load32(Address(address.base, address.offset + offsetof(jsval, data)), tempReg);
    masm.store32(tempReg, Address(tos.base, tos.offset + offsetof(jsval, data)));
    masm.load32(Address(address.base, address.offset + offsetof(jsval, data) + 4), tempReg);
    masm.store32(tempReg, Address(tos.base, tos.offset + offsetof(jsval, data) + 4));
#endif
    masm.load32(Address(address.base, address.offset + offsetof(jsval, mask)), tempReg);

    frame.pushUnknownType(tempReg);
}

