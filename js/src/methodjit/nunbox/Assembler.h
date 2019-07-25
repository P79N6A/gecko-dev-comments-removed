






































#if !defined jsjaeger_assembler_h__ && defined JS_METHODJIT
#define jsjaeger_assembler_h__

#include "methodjit/BaseAssembler.h"

namespace js {
namespace mjit {

class ImmTag : public JSC::MacroAssembler::Imm32
{
  public:
    ImmTag(JSValueTag mask)
      : Imm32(int32(mask))
    { }
};

struct ImmType : ImmTag
{
    ImmType(JSValueType type)
      : ImmTag(JSVAL_TYPE_TO_TAG(type))
    { }
};

class Assembler : public BaseAssembler
{
    static const uint32 PAYLOAD_OFFSET = 0;
    static const uint32 TAG_OFFSET     = 4;

  public:
    static const JSC::MacroAssembler::Scale JSVAL_SCALE = JSC::MacroAssembler::TimesEight;

    Address payloadOf(Address address) {
        return address;
    }

    BaseIndex payloadOf(BaseIndex address) {
        return address;
    }

    Address tagOf(Address address) {
        return Address(address.base, address.offset + TAG_OFFSET);
    }

    BaseIndex tagOf(BaseIndex address) {
        return BaseIndex(address.base, address.index, address.scale, address.offset + TAG_OFFSET);
    }

    void loadSlot(RegisterID obj, RegisterID clobber, uint32 slot, RegisterID type, RegisterID data) {
        JS_ASSERT(type != data);
        Address address(obj, offsetof(JSObject, fslots) + slot * sizeof(Value));
        if (slot >= JS_INITIAL_NSLOTS) {
            loadPtr(Address(obj, offsetof(JSObject, dslots)), clobber);
            address = Address(obj, (slot - JS_INITIAL_NSLOTS) * sizeof(Value));
        }
        if (obj == type) {
            loadData32(address, data);
            loadTypeTag(address, type);
        } else {
            loadTypeTag(address, type);
            loadData32(address, data);
        }
    }

    void loadTypeTag(Address address, RegisterID reg) {
        load32(Address(address.base, address.offset + TAG_OFFSET), reg);
    }

    void loadTypeTag(BaseIndex address, RegisterID reg) {
        load32(tagOf(address), reg);
    }

    void storeTypeTag(ImmTag imm, Address address) {
        store32(imm, Address(address.base, address.offset + TAG_OFFSET));
    }

    void storeTypeTag(ImmTag imm, BaseIndex address) {
        store32(imm, tagOf(address));
    }

    void storeTypeTag(RegisterID reg, Address address) {
        store32(reg, Address(address.base, address.offset + TAG_OFFSET));
    }

    void storeTypeTag(RegisterID reg, BaseIndex address) {
        store32(reg, tagOf(address));
    }

    void loadData32(Address address, RegisterID reg) {
        load32(Address(address.base, address.offset + PAYLOAD_OFFSET), reg);
    }

    void loadData32(BaseIndex address, RegisterID reg) {
        load32(payloadOf(address), reg);
    }

    void storeData32(Imm32 imm, Address address) {
        store32(imm, Address(address.base, address.offset + PAYLOAD_OFFSET));
    }

    void storeData32(RegisterID reg, Address address) {
        store32(reg, Address(address.base, address.offset + PAYLOAD_OFFSET));
    }

    void storeData32(RegisterID reg, BaseIndex address) {
        store32(reg, payloadOf(address));
    }

    void storeValue(const Value &v, Address address) {
        jsval_layout jv;
        jv.asBits = JSVAL_BITS(Jsvalify(v));

        store32(ImmTag(jv.s.tag), tagOf(address));
        if (!v.isUndefined())
            store32(Imm32(jv.s.payload.u32), payloadOf(address));
    }

    void storeValue(const Value &v, BaseIndex address) {
        jsval_layout jv;
        jv.asBits = JSVAL_BITS(Jsvalify(v));

        store32(ImmTag(jv.s.tag), tagOf(address));
        if (!v.isUndefined())
            store32(Imm32(jv.s.payload.u32), payloadOf(address));
    }

    Jump testNull(Assembler::Condition cond, RegisterID reg) {
        return branch32(cond, reg, ImmTag(JSVAL_TAG_NULL));
    }

    Jump testNull(Assembler::Condition cond, Address address) {
        return branch32(cond, tagOf(address), ImmTag(JSVAL_TAG_NULL));
    }

    Jump testInt32(Assembler::Condition cond, RegisterID reg) {
        return branch32(cond, reg, ImmTag(JSVAL_TAG_INT32));
    }

    Jump testInt32(Assembler::Condition cond, Address address) {
        return branch32(cond, tagOf(address), ImmTag(JSVAL_TAG_INT32));
    }

    Jump testNumber(Assembler::Condition cond, RegisterID reg) {
        cond = (cond == Assembler::Equal) ? Assembler::BelowOrEqual : Assembler::Above;
        return branch32(cond, reg, ImmTag(JSVAL_TAG_INT32));
    }

    Jump testNumber(Assembler::Condition cond, Address address) {
        cond = (cond == Assembler::Equal) ? Assembler::BelowOrEqual : Assembler::Above;
        return branch32(cond, tagOf(address), ImmTag(JSVAL_TAG_INT32));
    }

    Jump testPrimitive(Assembler::Condition cond, RegisterID reg) {
        cond = (cond == Assembler::NotEqual) ? Assembler::AboveOrEqual : Assembler::Below;
        return branch32(cond, reg, ImmTag(JSVAL_TAG_OBJECT));
    }

    Jump testPrimitive(Assembler::Condition cond, Address address) {
        cond = (cond == Assembler::NotEqual) ? Assembler::AboveOrEqual : Assembler::Below;
        return branch32(cond, tagOf(address), ImmTag(JSVAL_TAG_OBJECT));
    }

    Jump testObject(Assembler::Condition cond, RegisterID reg) {
        return branch32(cond, reg, ImmTag(JSVAL_TAG_OBJECT));
    }

    Jump testObject(Assembler::Condition cond, Address address) {
        return branch32(cond, tagOf(address), ImmTag(JSVAL_TAG_OBJECT));
    }

    Jump testDouble(Assembler::Condition cond, RegisterID reg) {
        Assembler::Condition opcond;
        if (cond == Assembler::Equal)
            opcond = Assembler::Below;
        else
            opcond = Assembler::AboveOrEqual;
        return branch32(opcond, reg, ImmTag(JSVAL_TAG_CLEAR));
    }

    Jump testDouble(Assembler::Condition cond, Address address) {
        Assembler::Condition opcond;
        if (cond == Assembler::Equal)
            opcond = Assembler::Below;
        else
            opcond = Assembler::AboveOrEqual;
        return branch32(opcond, tagOf(address), ImmTag(JSVAL_TAG_CLEAR));
    }

    Jump testBoolean(Assembler::Condition cond, RegisterID reg) {
        return branch32(cond, reg, ImmTag(JSVAL_TAG_BOOLEAN));
    }

    Jump testBoolean(Assembler::Condition cond, Address address) {
        return branch32(cond, tagOf(address), ImmTag(JSVAL_TAG_BOOLEAN));
    }

    Jump testString(Assembler::Condition cond, RegisterID reg) {
        return branch32(cond, reg, ImmTag(JSVAL_TAG_STRING));
    }

    Jump testString(Assembler::Condition cond, Address address) {
        return branch32(cond, tagOf(address), ImmTag(JSVAL_TAG_STRING));
    }
};

} 
} 

#endif

