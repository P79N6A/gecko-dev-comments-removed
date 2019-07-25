






































#if !defined jsjaeger_remat_h__ && defined JS_METHODJIT
#define jsjaeger_remat_h__

#include "jscntxt.h"
#include "assembler/assembler/MacroAssembler.h"


struct ValueRemat {
    typedef JSC::MacroAssembler::RegisterID RegisterID;
    union {
        struct {
            union {
                RegisterID reg;
                uint32 mask;
            } type;
            RegisterID data : 5;
            bool isTypeKnown : 1;
        } s;
        jsval v;
    } u;
    bool isConstant;
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

#endif

