







































#if !defined jsjaeger_assembler_h__ && defined JS_METHODJIT && defined JS_NUNBOX32
#define jsjaeger_assembler_h__

#include "methodjit/BaseAssembler.h"
#include "methodjit/RematInfo.h"

namespace js {
namespace mjit {


struct ImmTag : JSC::MacroAssembler::Imm32
{
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

struct ImmPayload : JSC::MacroAssembler::Imm32
{
    ImmPayload(uint32 payload)
      : Imm32(payload)
    { }
};

class Assembler : public BaseAssembler
{
    static const uint32 PAYLOAD_OFFSET = 0;
    static const uint32 TAG_OFFSET     = 4;

  public:
    static const JSC::MacroAssembler::Scale JSVAL_SCALE = JSC::MacroAssembler::TimesEight;

    template <typename T>
    T payloadOf(T address) {
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
        RegisterID activeAddressReg = obj;
        if (slot >= JS_INITIAL_NSLOTS) {
            loadPtr(Address(obj, offsetof(JSObject, dslots)), clobber);
            address = Address(clobber, (slot - JS_INITIAL_NSLOTS) * sizeof(Value));
            activeAddressReg = clobber;
        }
        if (activeAddressReg == type) {
            loadPayload(address, data);
            loadTypeTag(address, type);
        } else {
            loadTypeTag(address, type);
            loadPayload(address, data);
        }
    }

    template <typename T>
    void loadTypeTag(T address, RegisterID reg) {
        load32(tagOf(address), reg);
    }

    template <typename T>
    void storeTypeTag(ImmTag imm, T address) {
        store32(imm, tagOf(address));
    }

    template <typename T>
    void storeTypeTag(RegisterID reg, T address) {
        store32(reg, tagOf(address));
    }

    template <typename T>
    void loadPayload(T address, RegisterID reg) {
        load32(payloadOf(address), reg);
    }

    template <typename T>
    void storePayload(RegisterID reg, T address) {
        store32(reg, payloadOf(address));
    }

    template <typename T>
    void storePayload(ImmPayload imm, T address) {
        store32(imm, payloadOf(address));
    }

    
    template <typename T>
    Label loadValueAsComponents(T address, RegisterID type, RegisterID payload) {
        JS_ASSERT(!addressUsesRegister(address, type));
        loadTypeTag(address, type);
        Label l = label();
        loadPayload(address, payload);
        return l;
    }

    void loadValueAsComponents(const Value &val, RegisterID type, RegisterID payload) {
        jsval_layout jv;
        jv.asBits = JSVAL_BITS(Jsvalify(val));

        move(ImmTag(jv.s.tag), type);
        move(Imm32(jv.s.payload.u32), payload);
    }

    


    template <typename T>
    Label storeValue(const Value &v, T address) {
        jsval_layout jv;
        jv.asBits = JSVAL_BITS(Jsvalify(v));

        store32(ImmTag(jv.s.tag), tagOf(address));
        Label l = label();
        store32(Imm32(jv.s.payload.u32), payloadOf(address));
        return l;
    }

    template <typename T>
    void storeValueFromComponents(RegisterID type, RegisterID payload, T address) {
        storeTypeTag(type, address);
        storePayload(payload, address);
    }

    template <typename T>
    void storeValueFromComponents(ImmType type, RegisterID payload, T address) {
        storeTypeTag(type, address);
        storePayload(payload, address);
    }

    template <typename T>
    Label storeValue(const ValueRemat &vr, T address) {
        if (vr.isConstant) {
            return storeValue(Valueify(vr.u.v), address);
        } else {
            if (vr.u.s.isTypeKnown)
                storeTypeTag(ImmType(vr.u.s.type.knownType), address);
            else
                storeTypeTag(vr.u.s.type.reg, address);
            Label l = label();
            storePayload(vr.u.s.data, address);
            return l;
        }
    }

    void loadPrivate(Address privAddr, RegisterID to) {
        loadPtr(privAddr, to);
    }

    void loadFunctionPrivate(RegisterID base, RegisterID to) {
        Address privSlot(base, offsetof(JSObject, fslots) +
                               JSSLOT_PRIVATE * sizeof(Value));
        loadPtr(privSlot, to);
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

