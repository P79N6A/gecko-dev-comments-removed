






































#if !defined jsjaeger_valueinfo_h__ && defined JS_METHODJIT
#define jsjaeger_valueinfo_h__

#include "jsapi.h"
#include "methodjit/MachineRegs.h"
#include "methodjit/RematInfo.h"
#include "assembler/assembler/MacroAssembler.h"

namespace js {
namespace mjit {

class FrameEntry
{
    friend class FrameState;

  public:
    bool isConstant() const {
        return data.isConstant();
    }

    const jsval_layout &getConstant() const {
        JS_ASSERT(isConstant());
        return v_;
    }

    const Value &getValue() const {
        JS_ASSERT(isConstant());
        return Valueify(v_.asBits);
    }

    bool isTypeKnown() const {
        return type.isConstant();
    }

    uint32 getTypeTag() const {
        return v_.s.mask32;
    }

    uint32 getPayload32() const {
        
        return v_.s.payload.u32;
    }

  private:
    uint32 copyOf() {
        JS_ASSERT(type.isCopy() || data.isCopy());
        return index_;
    }

    void setTypeTag(uint32 u32) {
        type.setConstant();
        v_.s.mask32 = u32;
    }

    


    void resetUnsynced() {
        type.unsync();
        data.unsync();
#ifdef DEBUG
        type.invalidate();
        data.invalidate();
#endif
        copies = 0;
    }

    


    void resetSynced() {
        type.setMemory();
        data.setMemory();
        copies = 0;
    }

    


    void setConstant(const jsval &v) {
        type.setConstant();
        data.setConstant();
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

