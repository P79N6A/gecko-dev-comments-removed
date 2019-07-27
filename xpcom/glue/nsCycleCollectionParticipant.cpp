





#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "jsapi.h"

#ifdef MOZILLA_INTERNAL_API
#include "nsString.h"
#else
#include "nsStringAPI.h"
#endif

void
nsScriptObjectTracer::NoteJSChild(void* aScriptThing, const char* aName,
                                  void* aClosure)
{
  nsCycleCollectionTraversalCallback* cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aClosure);
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, aName);
  cb->NoteJSChild(aScriptThing);
}

NS_IMETHODIMP_(void)
nsXPCOMCycleCollectionParticipant::Root(void* aPtr)
{
  nsISupports* s = static_cast<nsISupports*>(aPtr);
  NS_ADDREF(s);
}

NS_IMETHODIMP_(void)
nsXPCOMCycleCollectionParticipant::Unroot(void* aPtr)
{
  nsISupports* s = static_cast<nsISupports*>(aPtr);
  NS_RELEASE(s);
}



NS_IMETHODIMP_(void)
nsXPCOMCycleCollectionParticipant::Trace(void* aPtr, const TraceCallbacks& aCb,
                                         void* aClosure)
{
}

bool
nsXPCOMCycleCollectionParticipant::CheckForRightISupports(nsISupports* aSupports)
{
  nsISupports* foo;
  aSupports->QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                            reinterpret_cast<void**>(&foo));
  return aSupports == foo;
}

void
CycleCollectionNoteEdgeNameImpl(nsCycleCollectionTraversalCallback& aCallback,
                                const char* aName,
                                uint32_t aFlags)
{
  nsAutoCString arrayEdgeName(aName);
  if (aFlags & CycleCollectionEdgeNameArrayFlag) {
    arrayEdgeName.AppendLiteral("[i]");
  }
  aCallback.NoteNextEdgeName(arrayEdgeName.get());
}

void
TraceCallbackFunc::Trace(JS::Heap<JS::Value>* aPtr, const char* aName,
                         void* aClosure) const
{
  if (aPtr->isMarkable()) {
    mCallback(aPtr->toGCThing(), aName, aClosure);
  }
}

void
TraceCallbackFunc::Trace(JS::Heap<jsid>* aPtr, const char* aName,
                         void* aClosure) const
{
  void* thing = JSID_TO_GCTHING(*aPtr);
  if (thing) {
    mCallback(thing, aName, aClosure);
  }
}

void
TraceCallbackFunc::Trace(JS::Heap<JSObject*>* aPtr, const char* aName,
                         void* aClosure) const
{
  mCallback(*aPtr, aName, aClosure);
}

void
TraceCallbackFunc::Trace(JS::TenuredHeap<JSObject*>* aPtr, const char* aName,
                         void* aClosure) const
{
  mCallback(*aPtr, aName, aClosure);
}

void
TraceCallbackFunc::Trace(JS::Heap<JSFunction*>* aPtr, const char* aName,
                         void* aClosure) const
{
  mCallback(*aPtr, aName, aClosure);
}

void
TraceCallbackFunc::Trace(JS::Heap<JSString*>* aPtr, const char* aName,
                         void* aClosure) const
{
  mCallback(*aPtr, aName, aClosure);
}

void
TraceCallbackFunc::Trace(JS::Heap<JSScript*>* aPtr, const char* aName,
                         void* aClosure) const
{
  mCallback(aPtr->get(), aName, aClosure);
}
