






































#if !defined jsjaeger_assembler64_h__ && defined JS_METHODJIT && defined JS_PUNBOX64
#define jsjaeger_assembler64_h__

#include "assembler/assembler/MacroAssembler.h"
#include "methodjit/MachineRegs.h"
#include "methodjit/RematInfo.h"

namespace js {
namespace mjit {

struct Imm64 : JSC::MacroAssembler::ImmPtr
{
    Imm64(uint64 u)
      : ImmPtr((const void *)u)
    { }
};


struct ImmTag : JSC::MacroAssembler::ImmPtr
{
    ImmTag(JSValueShiftedTag shtag)
      : ImmPtr((const void *)shtag)
    { }
};

struct ImmType : ImmTag
{
    ImmType(JSValueType type)
      : ImmTag(JSValueShiftedTag(JSVAL_TYPE_TO_SHIFTED_TAG(type)))
    { }
};

struct ImmPayload : Imm64
{
    ImmPayload(uint64 payload)
      : Imm64(payload)
    { }
};

class PunboxAssembler : public JSC::MacroAssembler
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

    void loadInlineSlot(RegisterID objReg, uint32 slot,
                        RegisterID typeReg, RegisterID dataReg) {
        Address address(objReg, JSObject::getFixedSlotOffset(slot));
        loadValueAsComponents(address, typeReg, dataReg);
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

    void loadValueAsComponents(const Value &val, RegisterID type, RegisterID payload) {
        move(Imm64(val.asRawBits() & JSVAL_TAG_MASK), type);
        move(Imm64(val.asRawBits() & JSVAL_PAYLOAD_MASK), payload);
    }

    template <typename T>
    void storeValueFromComponents(RegisterID type, RegisterID payload, T address) {
        move(type, Registers::ValueReg);
        orPtr(payload, Registers::ValueReg);
        storeValue(Registers::ValueReg, address);
    }

    template <typename T>
    void storeValueFromComponents(ImmTag type, RegisterID payload, T address) {
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
    void storeTypeTag(ImmTag imm, T address) {
        loadPayload(address, Registers::ValueReg);
        orPtr(imm, Registers::ValueReg);
        storePtr(Registers::ValueReg, valueOf(address));
    }

    template <typename T>
    void storeTypeTag(RegisterID reg, T address) {
        
        loadPayload(address, Registers::ValueReg);
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
        
        loadTypeTag(address, Registers::ValueReg);
        orPtr(reg, Registers::ValueReg);
        storePtr(Registers::ValueReg, valueOf(address));
    }
    
    template <typename T>
    void storePayload(ImmPayload imm, T address) {
        
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
        if (vr.isConstant())
            storeValue(vr.value(), address);
        else if (vr.isTypeKnown())
            storeValueFromComponents(ImmType(vr.knownType()), vr.dataReg(), address);
        else
            storeValueFromComponents(vr.typeReg(), vr.dataReg(), address);
    }

    void loadPrivate(Address privAddr, RegisterID to) {
        loadPtr(privAddr, to);
        lshiftPtr(Imm32(1), to);
    }

    void loadFunctionPrivate(RegisterID base, RegisterID to) {
        Address priv(base, offsetof(JSObject, privateData));
        loadPtr(priv, to);
    }

    Jump testNull(Condition cond, RegisterID reg) {
        return branchPtr(cond, reg, ImmTag(JSVAL_SHIFTED_TAG_NULL));
    }

    Jump testNull(Condition cond, Address address) {
        loadValue(address, Registers::ValueReg);
        return testNull(cond, Registers::ValueReg);
    }

    Jump testUndefined(Condition cond, RegisterID reg) {
        return branchPtr(cond, reg, ImmTag(JSVAL_SHIFTED_TAG_UNDEFINED));
    }

    Jump testUndefined(Condition cond, Address address) {
        loadValue(address, Registers::ValueReg);
        return testUndefined(cond, Registers::ValueReg);
    }

    Jump testInt32(Condition cond, RegisterID reg) {
        return branchPtr(cond, reg, ImmTag(JSVAL_SHIFTED_TAG_INT32));
    }

    Jump testInt32(Condition cond, Address address) {
        loadTypeTag(address, Registers::ValueReg);
        return testInt32(cond, Registers::ValueReg);
    }

    Jump testNumber(Condition cond, RegisterID reg) {
        cond = (cond == Equal) ? Below : AboveOrEqual;
        return branchPtr(cond, reg,
                         ImmTag(JSVAL_UPPER_EXCL_SHIFTED_TAG_OF_NUMBER_SET));
    }

    Jump testNumber(Condition cond, Address address) {
        loadValue(address, Registers::ValueReg);
        return testNumber(cond, Registers::ValueReg);
    }

    Jump testPrimitive(Condition cond, RegisterID reg) {
        cond = (cond == Equal) ? Below : AboveOrEqual;
        return branchPtr(cond, reg,
                         ImmTag(JSVAL_UPPER_EXCL_SHIFTED_TAG_OF_PRIMITIVE_SET));
    }

    Jump testPrimitive(Condition cond, Address address) {
        loadValue(address, Registers::ValueReg);
        return testPrimitive(cond, Registers::ValueReg);
    }

    Jump testObject(Condition cond, RegisterID reg) {
        cond = (cond == Equal) ? AboveOrEqual : Below;
        return branchPtr(cond, reg, ImmTag(JSVAL_SHIFTED_TAG_OBJECT));
    }

    Jump testObject(Condition cond, Address address) {
        loadValue(address, Registers::ValueReg);
        return testObject(cond, Registers::ValueReg);
    }

    Jump testDouble(Condition cond, RegisterID reg) {
        cond = (cond == Equal) ? BelowOrEqual : Above;
        return branchPtr(cond, reg, ImmTag(JSVAL_SHIFTED_TAG_MAX_DOUBLE));
    }

    Jump testDouble(Condition cond, Address address) {
        loadValue(address, Registers::ValueReg);
        return testDouble(cond, Registers::ValueReg);
    }

    Jump testBoolean(Condition cond, RegisterID reg) {
        return branchPtr(cond, reg, ImmTag(JSVAL_SHIFTED_TAG_BOOLEAN));
    }

    Jump testBoolean(Condition cond, Address address) {
        loadTypeTag(address, Registers::ValueReg);
        return testBoolean(cond, Registers::ValueReg);
    }

    Jump testString(Condition cond, RegisterID reg) {
        return branchPtr(cond, reg, ImmTag(JSVAL_SHIFTED_TAG_STRING));
    }

    Jump testString(Condition cond, Address address) {
        loadTypeTag(address, Registers::ValueReg);
        return testString(cond, Registers::ValueReg);
    }

    template <typename T>
    Jump fastArrayLoadSlot(T address, RegisterID typeReg, RegisterID dataReg) {
        loadValueAsComponents(address, typeReg, dataReg);
        return branchPtr(Equal, typeReg, ImmType(JSVAL_TYPE_MAGIC));
    }
};

typedef PunboxAssembler ValueAssembler;

} 
} 

#endif

