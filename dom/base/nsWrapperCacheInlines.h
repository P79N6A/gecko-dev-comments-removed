




#ifndef nsWrapperCacheInline_h___
#define nsWrapperCacheInline_h___

#include "nsWrapperCache.h"
#include "xpcpublic.h"
#include "jsapi.h"







MOZ_STATIC_ASSERT(sizeof(js::shadow::Object) % 8 == 0 && sizeof(JS::Value) == 8,
                  "We want to rely on JSObject being aligned on 8 byte "
                  "boundaries.");

inline JSObject*
nsWrapperCache::GetWrapper() const
{
    JSObject* obj = GetWrapperPreserveColor();
    xpc_UnmarkGrayObject(obj);
    return obj;
}

inline bool
nsWrapperCache::IsBlack()
{
  JSObject* o = GetWrapperPreserveColor();
  return o && !xpc_IsGrayGCThing(o);
}

static void
SearchGray(void* aGCThing, const char* aName, void* aClosure)
{
  bool* hasGrayObjects = static_cast<bool*>(aClosure);
  if (!*hasGrayObjects && aGCThing && xpc_IsGrayGCThing(aGCThing)) {
    *hasGrayObjects = true;
  }
}

inline bool
nsWrapperCache::IsBlackAndDoesNotNeedTracing(nsISupports* aThis)
{
  if (IsBlack()) {
    nsXPCOMCycleCollectionParticipant* participant = nullptr;
    CallQueryInterface(aThis, &participant);
    bool hasGrayObjects = false;
    participant->Trace(aThis, SearchGray, &hasGrayObjects);
    return !hasGrayObjects;
  }
  return false;
}

inline void
nsWrapperCache::TraceJSObjectFromBits(JSTracer* aTrc, const char* aName)
{
  JS_CallMaskedObjectTracer(aTrc, &mWrapperPtrBits, kWrapperBitMask, aName);
}

#endif 
