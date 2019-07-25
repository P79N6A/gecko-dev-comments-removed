







































#if !defined jsjaeger_assembler_h__ && defined JS_METHODJIT && defined JS_NUNBOX32
#define jsjaeger_assembler_h__

#include "assembler/assembler/MacroAssembler.h"
#include "methodjit/CodeGenIncludes.h"
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

class NunboxAssembler : public JSC::MacroAssembler
{
    static const uint32 PAYLOAD_OFFSET = 0;
    static const uint32 TAG_OFFSET     = 4;

  public:
    static const JSC::MacroAssembler::Scale JSVAL_SCALE = JSC::MacroAssembler::TimesEight;

    template <typename T>
    T payloadOf(T address) {
        JS_ASSERT(PAYLOAD_OFFSET == 0);
        return address;
    }

    Address tagOf(Address address) {
        return Address(address.base, address.offset + TAG_OFFSET);
    }

    BaseIndex tagOf(BaseIndex address) {
        return BaseIndex(address.base, address.index, address.scale, address.offset + TAG_OFFSET);
    }

    void loadInlineSlot(RegisterID objReg, uint32 slot,
                        RegisterID typeReg, RegisterID dataReg) {
        Address address(objReg, JSObject::getFixedSlotOffset(slot));
        if (objReg == typeReg) {
            loadPayload(address, dataReg);
            loadTypeTag(address, typeReg);
        } else {
            loadTypeTag(address, typeReg);
            loadPayload(address, dataReg);
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

    bool addressUsesRegister(BaseIndex address, RegisterID reg) {
        return (address.base == reg) || (address.index == reg);
    }

    bool addressUsesRegister(Address address, RegisterID reg) {
        return address.base == reg;
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

    








    Label loadValueWithAddressOffsetPatch(Address address, RegisterID treg, RegisterID dreg) {
        JS_ASSERT(address.base != treg); 

        Label start = label();
#if defined JS_CPU_X86
        



        loadTypeTag(address, treg);
        DBGLABEL_NOMASM(endType);
        loadPayload(address, dreg);
        DBGLABEL_NOMASM(endPayload);
        JS_ASSERT(differenceBetween(start, endType) == 6);
        JS_ASSERT(differenceBetween(endType, endPayload) == 6);
        return start;
#elif defined JS_CPU_ARM
        



        DataLabel32 load = load64WithAddressOffsetPatch(address, treg, dreg);
        JS_ASSERT(differenceBetween(start, load) == 0);
        (void) load;
        return start;
#endif
    }

    





    DataLabel32 storeValueWithAddressOffsetPatch(RegisterID treg, RegisterID dreg, Address address) {
        DataLabel32 start = dataLabel32();
#if defined JS_CPU_X86
        



        storeTypeTag(treg, address);
        DBGLABEL_NOMASM(endType);
        storePayload(dreg, address);
        DBGLABEL_NOMASM(endPayload);
        JS_ASSERT(differenceBetween(start, endType) == 6);
        JS_ASSERT(differenceBetween(endType, endPayload) == 6);
        return start;
#elif defined JS_CPU_ARM
        return store64WithAddressOffsetPatch(treg, dreg, address);
#endif
    }

    
    DataLabel32 storeValueWithAddressOffsetPatch(ImmType type, RegisterID dreg, Address address) {
        DataLabel32 start = dataLabel32();
#if defined JS_CPU_X86
        storeTypeTag(type, address);
        DBGLABEL_NOMASM(endType);
        storePayload(dreg, address);
        DBGLABEL_NOMASM(endPayload);
        JS_ASSERT(differenceBetween(start, endType) == 10);
        JS_ASSERT(differenceBetween(endType, endPayload) == 6);
        return start;
#elif defined JS_CPU_ARM
        return store64WithAddressOffsetPatch(type, dreg, address);
#endif
    }

    
    DataLabel32 storeValueWithAddressOffsetPatch(const Value &v, Address address) {
        jsval_layout jv;
        jv.asBits = JSVAL_BITS(Jsvalify(v));
        ImmTag type(jv.s.tag);
        Imm32 payload(jv.s.payload.u32);
        DataLabel32 start = dataLabel32();
#if defined JS_CPU_X86
        store32(type, tagOf(address));
        DBGLABEL_NOMASM(endType);
        store32(payload, payloadOf(address));
        DBGLABEL_NOMASM(endPayload);
        JS_ASSERT(differenceBetween(start, endType) == 10);
        JS_ASSERT(differenceBetween(endType, endPayload) == 10);
        return start;
#elif defined JS_CPU_ARM
        return store64WithAddressOffsetPatch(type, payload, address);
#endif
    }

    
    DataLabel32 storeValueWithAddressOffsetPatch(const ValueRemat &vr, Address address) {
        if (vr.isConstant()) {
            return storeValueWithAddressOffsetPatch(vr.value(), address);
        } else if (vr.isTypeKnown()) {
            ImmType type(vr.knownType());
            RegisterID data(vr.dataReg());
            return storeValueWithAddressOffsetPatch(type, data, address);
        } else {
            RegisterID type(vr.typeReg());
            RegisterID data(vr.dataReg());
            return storeValueWithAddressOffsetPatch(type, data, address);
        }
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
        if (vr.isConstant()) {
            return storeValue(vr.value(), address);
        } else {
            if (vr.isTypeKnown())
                storeTypeTag(ImmType(vr.knownType()), address);
            else
                storeTypeTag(vr.typeReg(), address);
            Label l = label();
            storePayload(vr.dataReg(), address);
            return l;
        }
    }

    template <typename T>
    Jump guardNotHole(T address) {
        return branch32(Equal, tagOf(address), ImmType(JSVAL_TYPE_MAGIC));
    }

    void loadPrivate(Address privAddr, RegisterID to) {
        loadPtr(privAddr, to);
    }

    void loadObjPrivate(RegisterID base, RegisterID to) {
        Address priv(base, offsetof(JSObject, privateData));
        loadPtr(priv, to);
    }

    Jump testNull(Condition cond, RegisterID reg) {
        return branch32(cond, reg, ImmTag(JSVAL_TAG_NULL));
    }

    Jump testNull(Condition cond, Address address) {
        return branch32(cond, tagOf(address), ImmTag(JSVAL_TAG_NULL));
    }

    Jump testUndefined(Condition cond, RegisterID reg) {
        return branch32(cond, reg, ImmTag(JSVAL_TAG_UNDEFINED));
    }

    Jump testUndefined(Condition cond, Address address) {
        return branch32(cond, tagOf(address), ImmTag(JSVAL_TAG_UNDEFINED));
    }

    Jump testInt32(Condition cond, RegisterID reg) {
        return branch32(cond, reg, ImmTag(JSVAL_TAG_INT32));
    }

    Jump testInt32(Condition cond, Address address) {
        return branch32(cond, tagOf(address), ImmTag(JSVAL_TAG_INT32));
    }

    Jump testNumber(Condition cond, RegisterID reg) {
        cond = (cond == Equal) ? BelowOrEqual : Above;
        return branch32(cond, reg, ImmTag(JSVAL_TAG_INT32));
    }

    Jump testNumber(Condition cond, Address address) {
        cond = (cond == Equal) ? BelowOrEqual : Above;
        return branch32(cond, tagOf(address), ImmTag(JSVAL_TAG_INT32));
    }

    Jump testPrimitive(Condition cond, RegisterID reg) {
        cond = (cond == NotEqual) ? AboveOrEqual : Below;
        return branch32(cond, reg, ImmTag(JSVAL_TAG_OBJECT));
    }

    Jump testPrimitive(Condition cond, Address address) {
        cond = (cond == NotEqual) ? AboveOrEqual : Below;
        return branch32(cond, tagOf(address), ImmTag(JSVAL_TAG_OBJECT));
    }

    Jump testObject(Condition cond, RegisterID reg) {
        return branch32(cond, reg, ImmTag(JSVAL_TAG_OBJECT));
    }

    Jump testObject(Condition cond, Address address) {
        return branch32(cond, tagOf(address), ImmTag(JSVAL_TAG_OBJECT));
    }

    Jump testDouble(Condition cond, RegisterID reg) {
        Condition opcond;
        if (cond == Equal)
            opcond = Below;
        else
            opcond = AboveOrEqual;
        return branch32(opcond, reg, ImmTag(JSVAL_TAG_CLEAR));
    }

    Jump testDouble(Condition cond, Address address) {
        Condition opcond;
        if (cond == Equal)
            opcond = Below;
        else
            opcond = AboveOrEqual;
        return branch32(opcond, tagOf(address), ImmTag(JSVAL_TAG_CLEAR));
    }

    Jump testBoolean(Condition cond, RegisterID reg) {
        return branch32(cond, reg, ImmTag(JSVAL_TAG_BOOLEAN));
    }

    Jump testBoolean(Condition cond, Address address) {
        return branch32(cond, tagOf(address), ImmTag(JSVAL_TAG_BOOLEAN));
    }

    Jump testString(Condition cond, RegisterID reg) {
        return branch32(cond, reg, ImmTag(JSVAL_TAG_STRING));
    }

    Jump testString(Condition cond, Address address) {
        return branch32(cond, tagOf(address), ImmTag(JSVAL_TAG_STRING));
    }

#ifdef JS_CPU_X86
    void fastLoadDouble(RegisterID lo, RegisterID hi, FPRegisterID fpReg) {
        if (MacroAssemblerX86Common::getSSEState() >= HasSSE4_1) {
            m_assembler.movd_rr(lo, fpReg);
            m_assembler.pinsrd_rr(hi, fpReg);
        } else {
            m_assembler.movd_rr(lo, fpReg);
            m_assembler.movd_rr(hi, FPRegisters::Temp0);
            m_assembler.unpcklps_rr(FPRegisters::Temp0, fpReg);
        }
    }
#endif

    void breakDouble(FPRegisterID srcDest, RegisterID typeReg, RegisterID dataReg) {
#ifdef JS_CPU_X86
        
        
        
        m_assembler.movd_rr(srcDest, dataReg);
        m_assembler.psrldq_rr(srcDest, 4);
        m_assembler.movd_rr(srcDest, typeReg);
#else
        JS_NOT_REACHED("implement this - push double, pop pop is easiest");
#endif
    }

    void loadStaticDouble(const double *dp, FPRegisterID dest, RegisterID scratch) {
        move(ImmPtr(dp), scratch);
        loadDouble(Address(scratch), dest);
    }

    template <typename T>
    Jump fastArrayLoadSlot(T address, RegisterID typeReg, RegisterID dataReg) {
        loadTypeTag(address, typeReg);
        Jump notHole = branch32(Equal, typeReg, ImmType(JSVAL_TYPE_MAGIC));
        loadPayload(address, dataReg);
        return notHole;
    }
};

typedef NunboxAssembler ValueAssembler;

} 
} 

#endif

