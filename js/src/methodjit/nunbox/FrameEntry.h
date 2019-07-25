






































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
        return v_.s.mask32;
    }

    uint32 getPayload32() {
        JS_ASSERT(!Valueify(v_.asBits).isDouble());
        return v_.s.payload.u32;
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

