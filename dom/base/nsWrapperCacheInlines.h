




#ifndef nsWrapperCacheInline_h___
#define nsWrapperCacheInline_h___

#include "nsWrapperCache.h"
#include "js/GCAPI.h"
#include "jsapi.h"

inline JSObject*
nsWrapperCache::GetWrapper() const
{
    JSObject* obj = GetWrapperPreserveColor();
    if (obj) {
      JS::ExposeObjectToActiveJS(obj);
    }
    return obj;
}

inline bool
nsWrapperCache::IsBlack()
{
  JSObject* o = GetWrapperPreserveColor();
  return o && !JS::GCThingIsMarkedGray(o);
}

static void
SearchGray(void* aGCThing, const char* aName, void* aClosure)
{
  bool* hasGrayObjects = static_cast<bool*>(aClosure);
  if (!*hasGrayObjects && aGCThing && JS::GCThingIsMarkedGray(aGCThing)) {
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
    participant->Trace(aThis, TraceCallbackFunc(SearchGray), &hasGrayObjects);
    return !hasGrayObjects;
  }
  return false;
}

inline void
nsWrapperCache::TraceWrapperJSObject(JSTracer* aTrc, const char* aName)
{
  JS_CallHeapObjectTracer(aTrc, &mWrapper, aName);
}

#endif 
