






































#if !defined jsjaeger_assembler_h__ && defined JS_METHODJIT
#define jsjaeger_assembler_h__

#include "methodjit/BaseAssembler.h"

namespace js {
namespace mjit {

class ImmTag : public JSC::MacroAssembler::Imm32
{
  public:
    ImmTag(JSValueMask32 mask)
      : Imm32(int32(mask))
    { }
};

class Assembler : public BaseAssembler
{
    static const uint32 PAYLOAD_OFFSET = 0;
    static const uint32 TAG_OFFSET     = 4;

  public:
    Address payloadOf(Address address) {
        return address;
    }

    Address tagOf(Address address) {
        return Address(address.base, address.offset + TAG_OFFSET);
    }

    void loadTypeTag(Address address, RegisterID reg) {
        load32(Address(address.base, address.offset + TAG_OFFSET), reg);
    }

    void storeTypeTag(ImmTag imm, Address address) {
        store32(imm, Address(address.base, address.offset + TAG_OFFSET));
    }

    void storeTypeTag(RegisterID reg, Address address) {
        store32(reg, Address(address.base, address.offset + TAG_OFFSET));
    }

    void loadData32(Address address, RegisterID reg) {
        load32(Address(address.base, address.offset + PAYLOAD_OFFSET), reg);
    }

    void storeData32(Imm32 imm, Address address) {
        store32(imm, Address(address.base, address.offset + PAYLOAD_OFFSET));
    }

    void storeData32(RegisterID reg, Address address) {
        store32(reg, Address(address.base, address.offset + PAYLOAD_OFFSET));
    }

    void storeValue(const Value &v, Address address) {
        jsval_layout jv;
        jv.asBits = Jsvalify(v);

        store32(ImmTag(jv.s.u.mask32), tagOf(address));
        if (!v.isUndefined())
            store32(Imm32(jv.s.payload.u32), payloadOf(address));
    }

    Jump testInt32(Assembler::Condition cond, RegisterID reg) {
        return branch32(cond, reg, ImmTag(JSVAL_MASK32_INT32));
    }

    Jump testInt32(Assembler::Condition cond, Address address) {
        return branch32(cond, tagOf(address), ImmTag(JSVAL_MASK32_INT32));
    }

    Jump testDouble(Assembler::Condition cond, RegisterID reg) {
        Assembler::Condition opcond;
        if (cond == Assembler::Equal)
            opcond = Assembler::LessThan;
        else
            opcond = Assembler::GreaterThanOrEqual;
        return branch32(opcond, reg, ImmTag(JSVAL_MASK32_CLEAR));
    }

    Jump testDouble(Assembler::Condition cond, Address address) {
        Assembler::Condition opcond;
        if (cond == Assembler::Equal)
            opcond = Assembler::LessThan;
        else
            opcond = Assembler::GreaterThanOrEqual;
        return branch32(opcond, tagOf(address), ImmTag(JSVAL_MASK32_CLEAR));
    }

    Jump testBoolean(Assembler::Condition cond, RegisterID reg) {
        return branch32(cond, reg, ImmTag(JSVAL_MASK32_BOOLEAN));
    }

    Jump testBoolean(Assembler::Condition cond, Address address) {
        return branch32(cond, tagOf(address), ImmTag(JSVAL_MASK32_BOOLEAN));
    }
};

} 
} 

#endif

