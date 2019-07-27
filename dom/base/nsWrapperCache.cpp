





#include "nsWrapperCacheInlines.h"

#include "jsproxy.h"
#include "mozilla/dom/DOMJSProxyHandler.h"
#include "mozilla/HoldDropJSObjects.h"
#include "nsCycleCollectionTraversalCallback.h"
#include "nsCycleCollector.h"

using namespace mozilla;
using namespace mozilla::dom;

 void
nsWrapperCache::HoldJSObjects(void* aScriptObjectHolder,
                              nsScriptObjectTracer* aTracer)
{
  cyclecollector::HoldJSObjectsImpl(aScriptObjectHolder, aTracer);
}

void
nsWrapperCache::ReleaseWrapper(void* aScriptObjectHolder)
{
  if (PreservingWrapper()) {
    
    
    
    JSObject* obj = GetWrapperPreserveColor();
    if (IsDOMBinding() && obj && js::IsProxy(obj)) {
      DOMProxyHandler::GetAndClearExpandoObject(obj);
    }
    SetPreservingWrapper(false);
    cyclecollector::DropJSObjectsImpl(aScriptObjectHolder);
  }
}

#ifdef DEBUG

class DebugWrapperTraversalCallback : public nsCycleCollectionTraversalCallback
{
public:
  explicit DebugWrapperTraversalCallback(void* aWrapper)
    : mFound(false)
    , mWrapper(aWrapper)
  {
    mFlags = WANT_ALL_TRACES;
  }

  NS_IMETHOD_(void) DescribeRefCountedNode(nsrefcnt aRefCount,
                                           const char* aObjName)
  {
  }
  NS_IMETHOD_(void) DescribeGCedNode(bool aIsMarked,
                                     const char* aObjName,
                                     uint64_t aCompartmentAddress)
  {
  }

  NS_IMETHOD_(void) NoteJSChild(void* aChild)
  {
    if (aChild == mWrapper) {
      mFound = true;
    }
  }
  NS_IMETHOD_(void) NoteXPCOMChild(nsISupports* aChild)
  {
  }
  NS_IMETHOD_(void) NoteNativeChild(void* aChild,
                                    nsCycleCollectionParticipant* aHelper)
  {
  }

  NS_IMETHOD_(void) NoteNextEdgeName(const char* aName)
  {
  }

  bool mFound;

private:
  void* mWrapper;
};

static void
DebugWrapperTraceCallback(void* aP, const char* aName, void* aClosure)
{
  DebugWrapperTraversalCallback* callback =
    static_cast<DebugWrapperTraversalCallback*>(aClosure);
  callback->NoteJSChild(aP);
}

void
nsWrapperCache::CheckCCWrapperTraversal(void* aScriptObjectHolder,
                                        nsScriptObjectTracer* aTracer)
{
  JSObject* wrapper = GetWrapper();
  if (!wrapper) {
    return;
  }

  DebugWrapperTraversalCallback callback(wrapper);

  aTracer->Traverse(aScriptObjectHolder, callback);
  MOZ_ASSERT(callback.mFound,
             "Cycle collection participant didn't traverse to preserved "
             "wrapper! This will probably crash.");

  callback.mFound = false;
  aTracer->Trace(aScriptObjectHolder,
                 TraceCallbackFunc(DebugWrapperTraceCallback), &callback);
  MOZ_ASSERT(callback.mFound,
             "Cycle collection participant didn't trace preserved wrapper! "
             "This will probably crash.");
}

#endif 
