






































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
        if (isCopy())
            return false;
        return data.isConstant();
    }

    const jsval_layout &getConstant() const {
        JS_ASSERT(isConstant());
        return v_;
    }

    Value getValue() const {
        JS_ASSERT(isConstant());
        return IMPL_TO_JSVAL(v_);
    }

#if defined JS_NUNBOX32
    uint32 getPayload() const {
        JS_ASSERT(isConstant());
        return v_.s.payload.u32;
    }
#elif defined JS_PUNBOX64
    uint64 getPayload() const {
        JS_ASSERT(isConstant());
        return v_.asBits & JSVAL_PAYLOAD_MASK;
    }
#endif

    
    void convertConstantDoubleToInt32(JSContext *cx) {
        JS_ASSERT(isType(JSVAL_TYPE_DOUBLE) && isConstant());
        int32 value;
        ValueToECMAInt32(cx, getValue(), &value);

        Value newValue = Int32Value(value);
        setConstant(newValue);
    }

    




    bool isTypeKnown() const {
        return backing()->type.isConstant();
    }

    



    JSValueType getKnownType() const {
        JS_ASSERT(isTypeKnown());
        return backing()->knownType;
    }

#if defined JS_NUNBOX32
    JSValueTag getKnownTag() const {
        JS_ASSERT(backing()->v_.s.tag != JSVAL_TAG_CLEAR);
        return backing()->v_.s.tag;
    }
#elif defined JS_PUNBOX64
    JSValueShiftedTag getKnownTag() const {
        return JSValueShiftedTag(backing()->v_.asBits & JSVAL_TAG_MASK);
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

    

    bool isCopy() const { return !!copy; }
    bool isCopied() const { return copied != 0; }

    const FrameEntry *backing() const {
        return isCopy() ? copyOf() : this;
    }

    bool hasSameBacking(const FrameEntry *other) const {
        return backing() == other->backing();
    }

  private:
    void setType(JSValueType type_) {
        JS_ASSERT(!isCopy() && type_ != JSVAL_TYPE_UNKNOWN);
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
        copied = 0;
        copy = NULL;
        index_ = index;
        tracked = true;
    }

    void clear() {
        JS_ASSERT(copied == 0);
        if (copy) {
            JS_ASSERT(copy->copied != 0);
            copy->copied--;
            copy = NULL;
        }
    }

    uint32 trackerIndex() {
        return index_;
    }

    


    void resetUnsynced() {
        clear();
        type.unsync();
        data.unsync();
        type.invalidate();
        data.invalidate();
    }

    


    void resetSynced() {
        clear();
        type.setMemory();
        data.setMemory();
    }

    


    void setConstant(const Value &v) {
        clear();
        type.unsync();
        data.unsync();
        type.setConstant();
        data.setConstant();
        v_ = JSVAL_TO_IMPL(v);
        if (v.isDouble())
            knownType = JSVAL_TYPE_DOUBLE;
        else
            knownType = v.extractNonDoubleType();
    }

    FrameEntry *copyOf() const {
        JS_ASSERT(isCopy());
        JS_ASSERT_IF(!copy->temporary, copy < this);
        return copy;
    }

    


    void setCopyOf(FrameEntry *fe) {
        clear();
        copy = fe;
        if (fe) {
            type.invalidate();
            data.invalidate();
            fe->copied++;
        }
    }

    inline bool isTracked() const {
        return tracked;
    }

    inline void untrack() {
        tracked = false;
    }

    inline bool dataInRegister(AnyRegisterID reg) const {
        JS_ASSERT(!copy);
        return reg.isReg()
            ? (data.inRegister() && data.reg() == reg.reg())
            : (data.inFPRegister() && data.fpreg() == reg.fpreg());
    }

  private:
    JSValueType knownType;
    jsval_layout v_;
    RematInfo  type;
    RematInfo  data;
    uint32     index_;
    FrameEntry *copy;
    bool       tracked;
    bool       temporary;

    
    uint32     copied;

    



    uint32     lastLoop;
};

} 
} 

#endif 

