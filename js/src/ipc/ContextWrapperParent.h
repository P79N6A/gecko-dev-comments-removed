







































#ifndef mozilla_jsipc_ContextWrapperParent_h__
#define mozilla_jsipc_ContextWrapperParent_h__

#include "mozilla/jsipc/PContextWrapperParent.h"
#include "mozilla/jsipc/ObjectWrapperParent.h"

#include "jsapi.h"
#include "nsAutoJSValHolder.h"

namespace mozilla {
namespace jsipc {
    
class ContextWrapperParent
    : public PContextWrapperParent
{
public:

    ContextWrapperParent() : mGlobal(NULL) {}

    bool GetGlobalJSObject(JSContext* cx, JSObject** globalp) {
        if (!mGlobal)
            return false;
        mGlobalHolder.Hold(cx);
        mGlobalHolder = *globalp = mGlobal->GetJSObject(cx);
        return true;
    }

    ObjectWrapperParent* GetGlobalObjectWrapper() const {
        return mGlobal;
    }

private:

    ObjectWrapperParent* mGlobal;
    nsAutoJSValHolder mGlobalHolder;

    PObjectWrapperParent* AllocPObjectWrapper(const bool&) {
        return new ObjectWrapperParent();
    }

    bool RecvPObjectWrapperConstructor(PObjectWrapperParent* actor,
                                       const bool& makeGlobal)
    {
        if (makeGlobal) {
            mGlobalHolder.Release();
            mGlobal = static_cast<ObjectWrapperParent*>(actor);
        }
        return true;
    }

    bool DeallocPObjectWrapper(PObjectWrapperParent* actor)
    {
        if (mGlobal &&
            mGlobal == static_cast<ObjectWrapperParent*>(actor)) {
            mGlobalHolder.Release();
            mGlobal = NULL;
        }
        delete actor;
        return true;
    }

};

}}

#endif
