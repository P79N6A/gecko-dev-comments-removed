






#ifndef mozilla_jsipc_CrossProcessObjectWrappers_h__
#define mozilla_jsipc_CrossProcessObjectWrappers_h__

#include "js/TypeDecls.h"
#include "mozilla/jsipc/CpowHolder.h"
#include "mozilla/jsipc/JavaScriptTypes.h"
#include "nsID.h"
#include "nsString.h"
#include "nsTArray.h"

#ifdef XP_WIN
#undef GetClassName
#undef GetClassInfo
#endif

namespace mozilla {

namespace dom {
class CPOWManagerGetter;
} 

namespace jsipc {

class PJavaScriptParent;
class PJavaScriptChild;

class CPOWManager
{
  public:
    virtual bool Unwrap(JSContext* cx,
                        const InfallibleTArray<CpowEntry>& aCpows,
                        JS::MutableHandleObject objp) = 0;

    virtual bool Wrap(JSContext* cx,
                      JS::HandleObject aObj,
                      InfallibleTArray<CpowEntry>* outCpows) = 0;
};

class CrossProcessCpowHolder : public CpowHolder
{
  public:
    CrossProcessCpowHolder(dom::CPOWManagerGetter* managerGetter,
                           const InfallibleTArray<CpowEntry>& cpows);

    bool ToObject(JSContext* cx, JS::MutableHandleObject objp);

  private:
    CPOWManager* js_;
    const InfallibleTArray<CpowEntry>& cpows_;
};

CPOWManager*
CPOWManagerFor(PJavaScriptParent* aParent);

CPOWManager*
CPOWManagerFor(PJavaScriptChild* aChild);

bool
IsCPOW(JSObject* obj);

bool
IsWrappedCPOW(JSObject* obj);

nsresult
InstanceOf(JSObject* obj, const nsID* id, bool* bp);

bool
DOMInstanceOf(JSContext* cx, JSObject* obj, int prototypeID, int depth, bool* bp);

void
GetWrappedCPOWTag(JSObject* obj, nsACString& out);

PJavaScriptParent*
NewJavaScriptParent(JSRuntime* rt);

void
ReleaseJavaScriptParent(PJavaScriptParent* parent);

PJavaScriptChild*
NewJavaScriptChild(JSRuntime* rt);

void
ReleaseJavaScriptChild(PJavaScriptChild* child);

} 
} 

#endif 
