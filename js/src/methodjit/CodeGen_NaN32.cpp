






































#define __STDC_LIMIT_MACROS

#include "jsapi.h"
#include "jsnum.h"
#include "CodeGenerator.h"

using namespace js;
using namespace js::mjit;

static const uint32 TAG_OFFSET     = 4;
static const uint32 PAYLOAD_OFFSET = 0;

class ImmIntPtr : public JSC::MacroAssembler::ImmPtr
{
  public:
    ImmIntPtr(int64_t i64)
      : ImmPtr(reinterpret_cast<void*>(i64))
    { }
};

CodeGenerator::CodeGenerator(MacroAssembler &masm, FrameState &frame)
  : masm(masm), frame(frame)
{
}

void
CodeGenerator::storeJsval(const Value &v, Address address)
{
    jsval_layout jv;
    jv.asBits = Jsvalify(v);

    masm.store32(Imm32(jv.s.mask32), Address(address.base, address.offset + TAG_OFFSET));
    if (!v.isNullOrUndefined())
        masm.store32(Imm32(jv.s.payload.u32), Address(address.base, address.offset + PAYLOAD_OFFSET));
}

void
CodeGenerator::storeValue(FrameEntry *fe, Address address, bool popped)
{
    if (fe->isConstant()) {
        storeJsval(fe->getValue(), address);
        return;
    }

    Address feAddr = frame.addressOf(fe);
    if (fe->isTypeConstant()) {
        masm.store32(Imm32(fe->getTypeTag()),
                     Address(address.base, address.offset + TAG_OFFSET));
    } else {
        RegisterID typeReg = frame.tempRegForType(fe);
        masm.store32(typeReg, Address(address.base, address.offset + TAG_OFFSET));
    }
    RegisterID tempReg = frame.allocReg();
    masm.load32(Address(feAddr.base, feAddr.offset + PAYLOAD_OFFSET), tempReg);
    masm.store32(tempReg, Address(address.base, address.offset + PAYLOAD_OFFSET));
    frame.freeReg(tempReg);
}

void
CodeGenerator::pushValueOntoFrame(Address address)
{
    RegisterID tempReg = frame.allocReg();

    Address tos = frame.topOfStack();

    masm.load32(Address(address.base, address.offset + PAYLOAD_OFFSET), tempReg);
    masm.store32(tempReg, Address(tos.base, tos.offset + PAYLOAD_OFFSET));
    masm.load32(Address(address.base, address.offset + TAG_OFFSET), tempReg);
    masm.store32(tempReg, Address(tos.base, tos.offset + TAG_OFFSET));

    frame.pushUnknownType(tempReg);
}

