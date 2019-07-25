






































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

    JSValueMask32 getTypeTag() const {
        return v_.s.u.mask32;
    }

    uint32 getPayload32() const {
        
        return v_.s.payload.u32;
    }

  private:
    void setTypeTag(JSValueMask32 u32) {
        type.setConstant();
        v_.s.u.mask32 = u32;
    }

    void track(uint32 index) {
        clear();
        index_ = index;
    }

    void clear() {
        copied = false;
        copy = NULL;
    }

    uint32 trackerIndex() {
        return index_;
    }

    


    void resetUnsynced() {
        clear();
        type.unsync();
        data.unsync();
#ifdef DEBUG
        type.invalidate();
        data.invalidate();
#endif
    }

    


    void resetSynced() {
        clear();
        type.setMemory();
        data.setMemory();
    }

    


    void setConstant(const jsval &v) {
        clear();
        type.unsync();
        data.unsync();
        type.setConstant();
        data.setConstant();
        v_.asBits = v;
    }

    bool isCopied() const {
        return copied;
    }

    void setCopied() {
        JS_ASSERT(!isCopy());
        copied = true;
    }

    bool isCopy() const {
        return !!copy;
    }

    FrameEntry *copyOf() const {
        JS_ASSERT(isCopy());
        return copy;
    }

    void setNotCopied() {
        copied = false;
    }

    


    void setCopyOf(FrameEntry *fe) {
        JS_ASSERT(!isCopied());
        copy = fe;
    }

  private:
    jsval_layout v_;
    RematInfo  type;
    RematInfo  data;
    uint32     index_;
    FrameEntry *copy;
    bool       copied;
    char       padding[7];
};

} 
} 

#endif 

