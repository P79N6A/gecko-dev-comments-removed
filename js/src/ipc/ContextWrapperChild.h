







































#ifndef mozilla_jsipc_ContextWrapperChild_h__
#define mozilla_jsipc_ContextWrapperChild_h__

#include "mozilla/jsipc/PContextWrapperChild.h"
#include "mozilla/jsipc/ObjectWrapperChild.h"

#include "jsapi.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"

namespace mozilla {
namespace jsipc {

class ContextWrapperChild
    : public PContextWrapperChild
{
public:

    ContextWrapperChild(JSContext* cx)
        : mContext(cx)
    {
        mResidentObjectTable.Init();
    }

    JSContext* GetContext() { return mContext; }

    PObjectWrapperChild* GetOrCreateWrapper(JSObject* obj,
                                            bool makeGlobal = false)
    {
        if (!obj) 
            return NULL;
        PObjectWrapperChild* wrapper;
        while (!mResidentObjectTable.Get(obj, &wrapper)) {
            wrapper = SendPObjectWrapperConstructor(AllocPObjectWrapper(obj),
                                                    makeGlobal);
            if (wrapper)
                mResidentObjectTable.Put(obj, wrapper);
            else
                return NULL;
        }
        return wrapper;
    }

protected:

    PObjectWrapperChild* AllocPObjectWrapper(JSObject* obj) {
        return new ObjectWrapperChild(mContext, obj);
    }
    
    PObjectWrapperChild* AllocPObjectWrapper(const bool&) {
        return AllocPObjectWrapper(JS_GetGlobalObject(mContext));
    }

    bool DeallocPObjectWrapper(PObjectWrapperChild* actor) {
        ObjectWrapperChild* owc = static_cast<ObjectWrapperChild*>(actor);
        mResidentObjectTable.Remove(owc->GetJSObject());
        return true;
    }

private:

    JSContext* const mContext;

    nsClassHashtable<nsPtrHashKey<JSObject>,
                     PObjectWrapperChild> mResidentObjectTable;

};

}}

#endif
