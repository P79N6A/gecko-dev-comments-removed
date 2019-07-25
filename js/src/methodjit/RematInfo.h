






































#if !defined jsjaeger_remat_h__ && defined JS_METHODJIT
#define jsjaeger_remat_h__

#include "jscntxt.h"
#include "MachineRegs.h"
#include "assembler/assembler/MacroAssembler.h"

namespace js {
namespace mjit {


struct StateRemat {
    typedef JSC::MacroAssembler::RegisterID RegisterID;
    typedef JSC::MacroAssembler::Address Address;

    static const int32 CONSTANT = -int(UINT16_LIMIT * sizeof(Value));

    
    
    
    
    
    
    union {
        RegisterID  reg_;
        int32       offset_;
    };

    static StateRemat FromInt32(int32 i32) {
        StateRemat sr;
        sr.offset_ = i32;
        return sr;
    }
    static StateRemat FromRegister(RegisterID reg) {
        StateRemat sr;
        sr.reg_ = reg;
        JS_ASSERT(sr.inRegister());
        return sr;
    }
    static StateRemat FromAddress(Address address) {
        JS_ASSERT(address.base == JSFrameReg);
        StateRemat sr;
        sr.offset_ = address.offset;
        JS_ASSERT(sr.inMemory());
        return sr;
    }

    
    
    
    
    
    
#define MIN_STATE_REMAT_BITS        21

    bool isConstant() const { return offset_ == CONSTANT; }
    bool inRegister() const { return offset_ >= 0 &&
                                     offset_ <= int32(JSC::MacroAssembler::TotalRegisters); }
    bool inMemory() const {
        return offset_ >= int32(sizeof(StackFrame)) ||
               offset_ < 0;
    }

    int32 toInt32() const { return offset_; }
    Address address() const {
        JS_ASSERT(inMemory());
        return Address(JSFrameReg, offset_);
    }
    RegisterID reg() const {
        JS_ASSERT(inRegister());
        return reg_;
    }
};


struct ValueRemat {
    typedef JSC::MacroAssembler::RegisterID RegisterID;
    union {
        struct {
            union {
                int32       typeRemat_;
                JSValueType knownType_;
            } type;
            int32   dataRemat_   : MIN_STATE_REMAT_BITS;
            bool    isTypeKnown_ : 1;
        } s;
        jsval v_;
    } u;
    bool isConstant_    : 1;
    bool isDataSynced   : 1;
    bool isTypeSynced   : 1;

    static ValueRemat FromConstant(const Value &v) {
        ValueRemat vr;
        vr.isConstant_ = true;
        vr.u.v_ = Jsvalify(v);
        return vr;
    }
    static ValueRemat FromKnownType(JSValueType type, RegisterID dataReg) {
        ValueRemat vr;
        vr.isConstant_ = false;
        vr.u.s.type.knownType_ = type;
        vr.u.s.isTypeKnown_ = true;
        vr.u.s.dataRemat_ = StateRemat::FromRegister(dataReg).toInt32();

        
        JS_ASSERT(vr.dataReg() == dataReg);
        return vr;
    }
    static ValueRemat FromRegisters(RegisterID typeReg, RegisterID dataReg) {
        ValueRemat vr;
        vr.isConstant_ = false;
        vr.u.s.isTypeKnown_ = false;
        vr.u.s.type.typeRemat_ = StateRemat::FromRegister(typeReg).toInt32();
        vr.u.s.dataRemat_ = StateRemat::FromRegister(dataReg).toInt32();

        
        JS_ASSERT(vr.dataReg() == dataReg);
        JS_ASSERT(vr.typeReg() == typeReg);
        return vr;
    }

    RegisterID dataReg() const {
        JS_ASSERT(!isConstant());
        return dataRemat().reg();
    }
    RegisterID typeReg() const {
        JS_ASSERT(!isTypeKnown());
        return typeRemat().reg();
    }

    bool isConstant() const { return isConstant_; }
    bool isTypeKnown() const { return isConstant() || u.s.isTypeKnown_; }

    StateRemat dataRemat() const {
        JS_ASSERT(!isConstant());
        return StateRemat::FromInt32(u.s.dataRemat_);
    }
    StateRemat typeRemat() const {
        JS_ASSERT(!isTypeKnown());
        return StateRemat::FromInt32(u.s.type.typeRemat_);
    }
    Value value() const {
        JS_ASSERT(isConstant());
        return Valueify(u.v_);
    }
    JSValueType knownType() const {
        JS_ASSERT(isTypeKnown());
        if (isConstant()) {
            const Value v = value();
            if (v.isDouble())
                return JSVAL_TYPE_DOUBLE;
            return v.extractNonDoubleType();
        }
        return u.s.type.knownType_;
    }
    bool isType(JSValueType type_) const {
        return isTypeKnown() && knownType() == type_;
    }
};




struct RematInfo {
    typedef JSC::MacroAssembler::RegisterID RegisterID;

    enum SyncState {
        SYNCED,
        UNSYNCED
    };

    enum RematType {
        TYPE,
        DATA
    };

    
    enum PhysLoc {
        


        PhysLoc_Memory = 0,

        
        PhysLoc_Constant,

        
        PhysLoc_Register,

        
        PhysLoc_Invalid
    };

    void setRegister(RegisterID reg) {
        reg_ = reg;
        location_ = PhysLoc_Register;
    }

    RegisterID reg() const {
        JS_ASSERT(inRegister());
        return reg_;
    }

    void setMemory() {
        location_ = PhysLoc_Memory;
        sync_ = SYNCED;
    }

    void invalidate() {
        location_ = PhysLoc_Invalid;
    }

    void setConstant() { location_ = PhysLoc_Constant; }

    bool isConstant() const { return location_ == PhysLoc_Constant; }
    bool inRegister() const { return location_ == PhysLoc_Register; }
    bool inMemory() const { return location_ == PhysLoc_Memory; }
    bool synced() const { return sync_ == SYNCED; }
    void sync() {
        JS_ASSERT(!synced());
        sync_ = SYNCED;
    }
    void unsync() {
        sync_ = UNSYNCED;
    }

    void inherit(const RematInfo &other) {
        reg_ = other.reg_;
        location_ = other.location_;
    }

  private:
    
    RegisterID reg_;

    
    PhysLoc location_;

    
    SyncState sync_;
};

} 
} 

#endif

