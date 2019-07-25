






































#if !defined jsjaeger_assembler64_h__ && defined JS_METHODJIT && defined JS_PUNBOX64
#define jsjaeger_assembler64_h__

#include "methodjit/BaseAssembler.h"
#include "methodjit/MachineRegs.h"

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

    Address payloadOf(Address address) {
        return address;
    }

    BaseIndex payloadOf(BaseIndex address) {
        return address;
    }

    Address valueOf(Address address) {
        return address;
    }

    BaseIndex valueOf(BaseIndex address) {
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

    void loadValue(Address address, RegisterID dst) {
        loadPtr(address, dst);
    }

    void loadValue(BaseIndex address, RegisterID dst) {
        loadPtr(address, dst);
    }

    void convertValueToType(RegisterID val) {
        andPtr(Registers::TypeMaskReg, val);
    }

    void convertValueToPayload(RegisterID val) {
        andPtr(Registers::PayloadMaskReg, val);
    }

    
    Label loadValueAsComponents(Address address, RegisterID type, RegisterID payload) {
        loadValue(address, type);
        Label l = label();

        move(Registers::PayloadMaskReg, payload);
        andPtr(type, payload);
        xorPtr(payload, type);

        return l;
    }

    Label loadValueAsComponents(BaseIndex address, RegisterID type, RegisterID payload) {
        loadValue(address, type);
        Label l = label();

        move(Registers::PayloadMaskReg, payload);
        andPtr(type, payload);
        xorPtr(payload, type);

        return l;
    }

    void loadTypeTag(Address address, RegisterID reg) {
        loadValue(address, reg);
        convertValueToType(reg);
    }

    void loadTypeTag(BaseIndex address, RegisterID reg) {
        loadValue(address, reg);
        convertValueToType(reg);
    }

    void storeTypeTag(ImmShiftedTag imm, Address address) {
        loadValue(address, Registers::ValueReg);
        convertValueToPayload(Registers::ValueReg);
        orPtr(imm, Registers::ValueReg);
        storePtr(Registers::ValueReg, valueOf(address));
    }

    void storeTypeTag(ImmShiftedTag imm, BaseIndex address) {
        loadValue(address, Registers::ValueReg);
        convertValueToPayload(Registers::ValueReg);
        orPtr(imm, Registers::ValueReg);
        storePtr(Registers::ValueReg, valueOf(address));
    }

    void storeTypeTag(RegisterID reg, Address address) {
        
        loadValue(address, Registers::ValueReg);
        convertValueToPayload(Registers::ValueReg);
        orPtr(reg, Registers::ValueReg);
        storePtr(Registers::ValueReg, valueOf(address));
    }

    void storeTypeTag(RegisterID reg, BaseIndex address) {
        
        loadValue(address, Registers::ValueReg);
        convertValueToPayload(Registers::ValueReg);
        orPtr(reg, Registers::ValueReg);
        storePtr(Registers::ValueReg, valueOf(address));
    }

    void loadPayload(Address address, RegisterID reg) {
        loadValue(address, reg);
        convertValueToPayload(reg);
    }

    void loadPayload(BaseIndex address, RegisterID reg) {
        loadValue(address, reg);
        convertValueToPayload(reg);
    }

    void storePayload(RegisterID reg, Address address) {
        
        loadValue(address, Registers::ValueReg);
        convertValueToType(Registers::ValueReg);
        orPtr(reg, Registers::ValueReg);
        storePtr(Registers::ValueReg, valueOf(address));
    }

    void storePayload(RegisterID reg, BaseIndex address) {
        
        loadValue(address, Registers::ValueReg);
        convertValueToType(Registers::ValueReg);
        orPtr(reg, Registers::ValueReg);
        storePtr(Registers::ValueReg, valueOf(address));
    }
    
    void storePayload(Imm64 imm, Address address) {
        
        storePtr(imm, valueOf(address));
    }

    void storeValue(const Value &v, Address address) {
        jsval_layout jv;
        jv.asBits = JSVAL_BITS(Jsvalify(v));

        storePtr(Imm64(jv.asBits), valueOf(address));
    }

    void storeValue(const Value &v, BaseIndex address) {
        jsval_layout jv;
        jv.asBits = JSVAL_BITS(Jsvalify(v));

        storePtr(Imm64(jv.asBits), valueOf(address));        
    }

    



    Label storeValueForIC(const Value &v, Address address) {
        jsval_layout jv;
        jv.asBits = JSVAL_BITS(Jsvalify(v));

        storePtr(Imm64(jv.asBits), valueOf(address));
        return label();
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

