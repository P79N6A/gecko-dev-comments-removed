






































#if !defined jsjaeger_remat_h__ && defined JS_METHODJIT
#define jsjaeger_remat_h__

#include "assembler/assembler/MacroAssembler.h"




struct RematInfo {
    typedef JSC::MacroAssembler::RegisterID RegisterID;

    
    enum PhysLoc {
        
        PhysLoc_Memory = 0,

        
        PhysLoc_Copy,

        
        PhysLoc_Constant,

        
        PhysLoc_Register
    };

    void setRegister(RegisterID reg) {
        reg_ = reg;
        location_ = PhysLoc_Register;
        synced_ = false;
    }

    void setMemory() {
        synced_ = true;
        location_ = PhysLoc_Memory;
    }

    void setSynced() { synced_ = true; }
    void setConstant() { location_ = PhysLoc_Constant; }

    bool isCopy() { return location_ == PhysLoc_Copy; }
    bool isConstant() { return location_ == PhysLoc_Constant; }
    bool inRegister() { return location_ == PhysLoc_Register; }
    bool inMemory() { return location_ == PhysLoc_Memory; }
    RegisterID reg() { return reg_; }

    void unsync() { synced_ = false; }
    bool synced() { return synced_; }
    bool needsSync() { return !inMemory() && !synced(); }

    
    RegisterID reg_;

    
    PhysLoc    location_;

    
    bool       synced_;
};

#endif

