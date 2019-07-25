






































#if !defined jsjaeger_valueinfo_h__ && defined JS_METHODJIT
#define jsjaeger_valueinfo_h__

#include "jsapi.h"
#include "jsnum.h"
#include "jstypes.h"
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

    



    JSValueType getKnownType() const {
        JS_ASSERT(isTypeKnown());
        return knownType;
    }

#if defined JS_NUNBOX32
    JSValueTag getKnownTag() const {
        JS_ASSERT(v_.s.tag != JSVAL_TAG_CLEAR);
        return v_.s.tag;
    }
#elif defined JS_PUNBOX64
    JSValueShiftedTag getKnownTag() const {
        return JSValueShiftedTag(v_.asBits & JSVAL_TAG_MASK);
    }
#endif

    
    bool isType(JSValueType type_) const {
        return isTypeKnown() && getKnownType() == type_;
    }

    
    bool isNotType(JSValueType type_) const {
        return isTypeKnown() && getKnownType() != type_;
    }

    
    
    bool mightBeType(JSValueType type_) const {
        return !isNotType(type_);
    }

#if defined JS_NUNBOX32
    uint32 getPayload() const {
        
        return v_.s.payload.u32;
    }
#elif defined JS_PUNBOX64
    uint64 getPayload() const {
        return v_.asBits & JSVAL_PAYLOAD_MASK;
    }
#endif

    bool hasSameBacking(const FrameEntry *other) const {
        return backing() == other->backing();
    }

    
    void convertConstantDoubleToInt32(JSContext *cx) {
        JS_ASSERT(isType(JSVAL_TYPE_DOUBLE) && isConstant());
        int32 value;
        ValueToECMAInt32(cx, getValue(), &value);

        Value newValue = Int32Value(value);
        setConstant(Jsvalify(newValue));
    }

    bool isCopy() const { return !!copy; }
    bool isCopied() const { return copied; }

  private:
    void setType(JSValueType type_) {
        type.setConstant();
#if defined JS_NUNBOX32
        v_.s.tag = JSVAL_TYPE_TO_TAG(type_);
#elif defined JS_PUNBOX64
        v_.asBits &= JSVAL_PAYLOAD_MASK;
        v_.asBits |= JSVAL_TYPE_TO_SHIFTED_TAG(type_);
#endif
        knownType = type_;
    }

    void track(uint32 index) {
        clear();
        index_ = index;
        tracked = true;
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

    void setCopied() {
        JS_ASSERT(!isCopy());
        copied = true;
    }

    FrameEntry *copyOf() const {
        JS_ASSERT(isCopy());
        JS_ASSERT(copy < this);
        return copy;
    }

    const FrameEntry *backing() const {
        return isCopy() ? copyOf() : this;
    }

    void setNotCopied() {
        copied = false;
    }

    


    void setCopyOf(FrameEntry *fe) {
        JS_ASSERT_IF(fe, !fe->isConstant());
        JS_ASSERT(!isCopied());
        copy = fe;
    }

    inline bool isTracked() const {
        return tracked;
    }

    inline void untrack() {
        tracked = false;
    }

    inline bool dataInRegister(AnyRegisterID reg) const {
        JS_ASSERT(!copy);
        return (data.inRegister() && reg.isReg() && data.reg() == reg.reg())
            || (data.inFPRegister() && !reg.isReg() && data.fpreg() == reg.fpreg());
    }

  private:
    JSValueType knownType;
    jsval_layout v_;
    RematInfo  type;
    RematInfo  data;
    uint32     index_;
    FrameEntry *copy;
    bool       copied;
    bool       tracked;
    bool       inlined;
    bool       initArray;
    JSObject   *initObject;
    jsbytecode *lastLoop;
};

} 
} 

#endif 

