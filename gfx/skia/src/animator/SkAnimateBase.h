








#ifndef SkAnimateBase_DEFINED
#define SkAnimateBase_DEFINED

#include "SkDisplayable.h"
#include "SkMath.h"
#include "SkMemberInfo.h"
#include "SkTypedArray.h"

class SkApply;
class SkDrawable;

class SkAnimateBase : public SkDisplayable {
public:
    DECLARE_MEMBER_INFO(AnimateBase);
    SkAnimateBase();
    virtual ~SkAnimateBase();
    virtual int components();
    virtual SkDisplayable* deepCopy(SkAnimateMaker* );
    virtual void dirty();
#ifdef SK_DUMP_ENABLED
    virtual void dump(SkAnimateMaker* );
#endif
    int entries() { return fValues.count() / components(); }
    virtual bool hasExecute() const;
    bool isDynamic() const { return SkToBool(fDynamic); }
    virtual SkDisplayable* getParent() const;
    virtual bool getProperty(int index, SkScriptValue* value) const;
    SkMSec getStart() const { return fStart; }
    SkOperand* getValues() { return fValues.begin(); }
    SkDisplayTypes getValuesType() { return fValues.getType(); }
    virtual void onEndElement(SkAnimateMaker& );
    void packARGB(SkScalar [], int count, SkTDOperandArray* );
    virtual void refresh(SkAnimateMaker& );
    void setChanged(bool changed) { fChanged = changed; }
    void setHasEndEvent() { fHasEndEvent = true; }
    virtual bool setParent(SkDisplayable* );
    virtual bool setProperty(int index, SkScriptValue& value);
    void setTarget(SkAnimateMaker& );
    virtual bool targetNeedsInitialization() const;
protected:
    SkMSec begin;
    SkTDScalarArray blend;
    SkMSec dur;
    
    SkString field; 
    SkString formula;
    SkString from;
    SkString lval;
    SkScalar repeat;
    SkString target;    
    SkString to;
    SkApply* fApply;
    const SkMemberInfo* fFieldInfo;
    int fFieldOffset;
    SkMSec fStart;  
    SkDrawable* fTarget;
    SkTypedArray fValues;
    unsigned fChanged : 1; 
    unsigned fDelayed : 1;  
    unsigned fDynamic : 1;
    unsigned fHasEndEvent : 1;
    unsigned fHasValues : 1;        
    unsigned fMirror : 1;
    unsigned fReset : 1;
    unsigned fResetPending : 1;
    unsigned fTargetIsScope : 1;
private:
    typedef SkDisplayable INHERITED;
    friend class SkActive;
    friend class SkApply;
    friend class SkDisplayList;
};

#endif 
