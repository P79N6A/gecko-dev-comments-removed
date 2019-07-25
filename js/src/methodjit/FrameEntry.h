






































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
    friend class ImmutableSync;

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
        return Valueify(JSVAL_FROM_LAYOUT(v_));
    }

    bool isTypeKnown() const {
        return type.isConstant();
    }

    JSValueTag getKnownTag() const {
        return v_.s.tag;
    }

    JSValueType getKnownType() const {
        JS_ASSERT(isTypeKnown());
        return knownType;
    }

    
    bool isType(JSValueType type_) const {
        return isTypeKnown() && getKnownType() == type_;
    }

    
    bool isNotType(JSValueType type_) const {
        return isTypeKnown() && getKnownType() != type_;
    }

    uint32 getPayload32() const {
        
        return v_.s.payload.u32;
    }

  private:
    void setType(JSValueType type_) {
        type.setConstant();
        v_.s.tag = JSVAL_TYPE_TO_TAG(type_);
        knownType = type_;
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
        v_.asBits = JSVAL_BITS(v);
        Value cv = Valueify(v);
        if (cv.isDouble())
            knownType = JSVAL_TYPE_DOUBLE;
        else
            knownType = cv.extractNonDoubleType();
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
        JS_ASSERT_IF(fe, !fe->isConstant());
        JS_ASSERT(!isCopied());
        copy = fe;
    }

  private:
    JSValueType knownType;
    jsval_layout v_;
    RematInfo  type;
    RematInfo  data;
    uint32     index_;
    FrameEntry *copy;
    bool       copied;
    char       padding[3];
};

} 
} 

#endif 

