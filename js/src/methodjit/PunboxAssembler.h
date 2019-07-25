






































#if !defined jsjaeger_assembler64_h__ && defined JS_METHODJIT && defined JS_PUNBOX64
#define jsjaeger_assembler64_h__

#include "methodjit/BaseAssembler.h"
#include "methodjit/MachineRegs.h"
#include "methodjit/RematInfo.h"

namespace js {
namespace mjit {

class Imm64 : public JSC::MacroAssembler::ImmPtr
{
  public:
    Imm64(uint64 u)
      : ImmPtr((const void *)u)
    { }
};

class ImmShiftedTag : public JSC::MacroAssembler::ImmPtr
{
  public:
    ImmShiftedTag(JSValueShiftedTag shtag)
      : ImmPtr((const void *)shtag)
    { }
};

class ImmType : public ImmShiftedTag
{
  public:
    ImmType(JSValueType type)
      : ImmShiftedTag(JSValueShiftedTag(JSVAL_TYPE_TO_SHIFTED_TAG(type)))
    { }
};

class Assembler : public BaseAssembler
{
    static const uint32 PAYLOAD_OFFSET = 0;

  public:
    static const JSC::MacroAssembler::Scale JSVAL_SCALE = JSC::MacroAssembler::TimesEight;

    template <typename T>
    T payloadOf(T address) {
        return address;
    }

    template <typename T>
    T valueOf(T address) {
        return address;
    }

    void loadSlot(RegisterID obj, RegisterID clobber, uint32 slot, RegisterID type, RegisterID data) {
        JS_ASSERT(type != data);
        Address address(obj, offsetof(JSObject, fslots) + slot * sizeof(Value));
        if (slot >= JS_INITIAL_NSLOTS) {
            loadPtr(Address(obj, offsetof(JSObject, dslots)), clobber);
            address = Address(clobber, (slot - JS_INITIAL_NSLOTS) * sizeof(Value));
        }
        
        loadValueAsComponents(address, type, data);
    }

    template <typename T>
    void loadValue(T address, RegisterID dst) {
        loadPtr(address, dst);
    }

    void convertValueToType(RegisterID val) {
        andPtr(Registers::TypeMaskReg, val);
    }

    void convertValueToPayload(RegisterID val) {
        andPtr(Registers::PayloadMaskReg, val);
    }

    
    template <typename T>
    Label loadValueAsComponents(T address, RegisterID type, RegisterID payload) {
        loadValue(address, type);
        Label l = label();

        move(Registers::PayloadMaskReg, payload);
        andPtr(type, payload);
        xorPtr(payload, type);

        return l;
    }

    template <typename T>
    void storeValueFromComponents(RegisterID type, RegisterID payload, T address) {
        move(type, Registers::ValueReg);
        orPtr(payload, Registers::ValueReg);
        storeValue(Registers::ValueReg, address);
    }

    template <typename T>
    void storeValueFromComponents(ImmShiftedTag type, RegisterID payload, T address) {
        move(type, Registers::ValueReg);
        orPtr(payload, Registers::ValueReg);
        storeValue(Registers::ValueReg, address);
    }

    template <typename T>
    void loadTypeTag(T address, RegisterID reg) {
        loadValue(address, reg);
        convertValueToType(reg);
    }

    template <typename T>
    void storeTypeTag(ImmShiftedTag imm, T address) {
        loadValue(address, Registers::ValueReg);
        convertValueToPayload(Registers::ValueReg);
        orPtr(imm, Registers::ValueReg);
        storePtr(Registers::ValueReg, valueOf(address));
    }

    template <typename T>
    void storeTypeTag(RegisterID reg, T address) {
        
        loadValue(address, Registers::ValueReg);
        convertValueToPayload(Registers::ValueReg);
        orPtr(reg, Registers::ValueReg);
        storePtr(Registers::ValueReg, valueOf(address));
    }

    template <typename T>
    void loadPayload(T address, RegisterID reg) {
        loadValue(address, reg);
        convertValueToPayload(reg);
    }

    template <typename T>
    void storePayload(RegisterID reg, T address) {
        
        loadValue(address, Registers::ValueReg);
        convertValueToType(Registers::ValueReg);
        orPtr(reg, Registers::ValueReg);
        storePtr(Registers::ValueReg, valueOf(address));
    }
    
    template <typename T>
    void storePayload(Imm64 imm, T address) {
        
        storePtr(imm, valueOf(address));
    }

    template <typename T>
    void storeValue(RegisterID reg, T address) {
        storePtr(reg, valueOf(address));
    }

    template <typename T>
    void storeValue(const Value &v, T address) {
        jsval_layout jv;
        jv.asBits = JSVAL_BITS(Jsvalify(v));

        storePtr(Imm64(jv.asBits), valueOf(address));
    }

    template <typename T>
    void storeValue(const ValueRemat &vr, T address) {
        if (vr.isConstant)
            storeValue(Valueify(vr.u.v), address);
        else if (vr.u.s.isTypeKnown)
            storeValueFromComponents(ImmType(vr.u.s.type.knownType), vr.u.s.data, address);
        else
            storeValueFromComponents(vr.u.s.type.reg, vr.u.s.data, address);
    }

    void loadPrivate(Address privAddr, RegisterID to) {
        loadPtr(privAddr, to);
        lshiftPtr(Imm32(1), to);
    }

    void loadFunctionPrivate(RegisterID base, RegisterID to) {
        Address privSlot(base, offsetof(JSObject, fslots) +
                               JSSLOT_PRIVATE * sizeof(Value));
        loadPtr(privSlot, to);
    }

    Jump testNull(Assembler::Condition cond, RegisterID reg) {
        return branchPtr(cond, reg, ImmShiftedTag(JSVAL_SHIFTED_TAG_NULL));
    }

    Jump testNull(Assembler::Condition cond, Address address) {
        loadValue(address, Registers::ValueReg);
        convertValueToType(Registers::ValueReg);
        return branchPtr(cond, Registers::ValueReg, ImmShiftedTag(JSVAL_SHIFTED_TAG_NULL));
    }

    Jump testInt32(Assembler::Condition cond, RegisterID reg) {
        return branchPtr(cond, reg, ImmShiftedTag(JSVAL_SHIFTED_TAG_INT32));
    }

    Jump testInt32(Assembler::Condition cond, Address address) {
        loadValue(address, Registers::ValueReg);
        convertValueToType(Registers::ValueReg);
        return branchPtr(cond, Registers::ValueReg, ImmShiftedTag(JSVAL_SHIFTED_TAG_INT32));
    }

    Jump testNumber(Assembler::Condition cond, RegisterID reg) {
        cond = (cond == Assembler::Equal) ? Assembler::BelowOrEqual : Assembler::Above;
        return branchPtr(cond, reg, ImmShiftedTag(JSVAL_SHIFTED_TAG_INT32));
    }

    Jump testNumber(Assembler::Condition cond, Address address) {
        cond = (cond == Assembler::Equal) ? Assembler::BelowOrEqual : Assembler::Above;
        loadValue(address, Registers::ValueReg);
        convertValueToType(Registers::ValueReg);
        return branchPtr(cond, Registers::ValueReg, ImmShiftedTag(JSVAL_SHIFTED_TAG_INT32));
    }

    Jump testPrimitive(Assembler::Condition cond, RegisterID reg) {
        cond = (cond == Assembler::NotEqual) ? Assembler::AboveOrEqual : Assembler::Below;
        return branchPtr(cond, reg, ImmShiftedTag(JSVAL_SHIFTED_TAG_OBJECT));
    }

    Jump testPrimitive(Assembler::Condition cond, Address address) {
        cond = (cond == Assembler::NotEqual) ? Assembler::AboveOrEqual : Assembler::Below;
        loadValue(address, Registers::ValueReg);
        convertValueToType(Registers::ValueReg);
        return branchPtr(cond, Registers::ValueReg, ImmShiftedTag(JSVAL_SHIFTED_TAG_OBJECT));
    }

    Jump testObject(Assembler::Condition cond, RegisterID reg) {
        return branchPtr(cond, reg, ImmShiftedTag(JSVAL_SHIFTED_TAG_OBJECT));
    }

    Jump testObject(Assembler::Condition cond, Address address) {
        loadValue(address, Registers::ValueReg);
        convertValueToType(Registers::ValueReg);
        return branchPtr(cond, Registers::ValueReg, ImmShiftedTag(JSVAL_SHIFTED_TAG_OBJECT));
    }

    Jump testDouble(Assembler::Condition cond, RegisterID reg) {
        Assembler::Condition opcond;
        if (cond == Assembler::Equal)
            opcond = Assembler::Below;
        else
            opcond = Assembler::AboveOrEqual;
        return branchPtr(opcond, reg, ImmShiftedTag(JSVAL_SHIFTED_TAG_MAX_DOUBLE));
    }

    Jump testDouble(Assembler::Condition cond, Address address) {
        Assembler::Condition opcond;
        if (cond == Assembler::Equal)
            opcond = Assembler::Below;
        else
            opcond = Assembler::AboveOrEqual;
        loadValue(address, Registers::ValueReg);
        convertValueToType(Registers::ValueReg);
        return branchPtr(opcond, Registers::ValueReg, ImmShiftedTag(JSVAL_SHIFTED_TAG_MAX_DOUBLE));
    }

    Jump testBoolean(Assembler::Condition cond, RegisterID reg) {
        return branchPtr(cond, reg, ImmShiftedTag(JSVAL_SHIFTED_TAG_BOOLEAN));
    }

    Jump testBoolean(Assembler::Condition cond, Address address) {
        loadValue(address, Registers::ValueReg);
        convertValueToType(Registers::ValueReg);
        return branchPtr(cond, Registers::ValueReg, ImmShiftedTag(JSVAL_SHIFTED_TAG_BOOLEAN));
    }

    Jump testString(Assembler::Condition cond, RegisterID reg) {
        return branchPtr(cond, reg, ImmShiftedTag(JSVAL_SHIFTED_TAG_STRING));
    }

    Jump testString(Assembler::Condition cond, Address address) {
        loadValue(address, Registers::ValueReg);
        convertValueToType(Registers::ValueReg);
        return branchPtr(cond, Registers::ValueReg, ImmShiftedTag(JSVAL_SHIFTED_TAG_BOOLEAN));
    }
};

} 
} 

#endif

