






































#if !defined jsjaeger_valueinfo_h__ && defined JS_METHODJIT
#define jsjaeger_valueinfo_h__

#include "jsapi.h"
#include "methodjit/MachineRegs.h"
#include "assembler/assembler/MacroAssembler.h"

namespace js {
namespace mjit {

struct RematInfo {
    typedef JSC::MacroAssembler::RegisterID RegisterID;

    
    enum PhysLoc {
        
        PhysLoc_Copy,

        
        PhysLoc_Constant,

        
        PhysLoc_Register,

        
        PhysLoc_Memory
    };

    void setRegister(RegisterID reg) {
        reg_ = reg;
        location_ = PhysLoc_Register;
        synced_ = false;
    }

    bool isCopy() { return location_ == PhysLoc_Copy; }
    void setConstant() { location_ = PhysLoc_Constant; }
    void unsync() { synced_ = false; }
    bool isConstant() { return location_ == PhysLoc_Constant; }
    bool inRegister() { return location_ == PhysLoc_Register; }
    RegisterID reg() { return reg_; }
    void setMemory() {
        synced_ = true;
        location_ = PhysLoc_Memory;
    }
    bool synced() { return synced_; }

    RegisterID reg_;
    PhysLoc    location_;
    bool       synced_;
};

class FrameEntry
{
    friend class FrameState;

  public:
    bool isConstant() {
        return data.isConstant();
    }

    const jsval_layout &getConstant() {
        JS_ASSERT(isConstant());
        return v_;
    }

    const Value &getValue() {
        JS_ASSERT(isConstant());
        return Valueify(v_.asBits);
    }

    bool isTypeConstant() {
        return type.isConstant();
    }

    uint32 getTypeTag() {
#if 0
        return v_.mask;
#else
        return v_.s.mask32;
#endif
    }

    uint32 copyOf() {
        JS_ASSERT(type.isCopy() || data.isCopy());
        return index_;
    }

  private:
    void setConstant(const jsval &v) {
        type.setConstant();
        type.unsync();
        data.setConstant();
        data.unsync();
        v_.asBits = v;
    }

  private:
    RematInfo  type;
    RematInfo  data;
    jsval_layout v_;
    uint32     index_;
    uint32     copies;
};

} 
} 

#endif 

