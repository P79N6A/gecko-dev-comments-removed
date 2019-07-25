





































#include "ListenerManager.h"

#include "jsapi.h"
#include "jscntxt.h"
#include "js/Vector.h"

#include "Events.h"

using mozilla::dom::workers::events::ListenerManager;

namespace {

struct Listener;

struct ListenerCollection : PRCList
{
  static ListenerCollection*
  Add(JSContext* aCx, ListenerCollection* aCollectionHead, jsid aTypeId)
  {
    ListenerCollection* collection =
      static_cast<ListenerCollection*>(JS_malloc(aCx,
                                                 sizeof(ListenerCollection)));
    if (!collection) {
      return NULL;
    }

    PR_APPEND_LINK(collection, aCollectionHead);

    collection->mTypeId = aTypeId;
    PR_INIT_CLIST(&collection->mListenerHead);
    return collection;
  }

  static void
  Remove(JSContext* aCx, ListenerCollection* aCollection)
  {
    PR_REMOVE_LINK(aCollection);
    JS_free(aCx, aCollection);
  }

  jsid mTypeId;
  PRCList mListenerHead;
};

struct Listener : PRCList
{
  static Listener*
  Add(JSContext* aCx, Listener* aListenerHead, jsval aListenerVal,
      ListenerManager::Phase aPhase, bool aWantsUntrusted)
  {
    Listener* listener =
      static_cast<Listener*>(JS_malloc(aCx, sizeof(Listener)));
    if (!listener) {
      return NULL;
    }

    PR_APPEND_LINK(listener, aListenerHead);

    listener->mListenerVal = aListenerVal;
    listener->mPhase = aPhase;
    listener->mWantsUntrusted = aWantsUntrusted;
    return listener;
  }

  static void
  Remove(JSContext* aCx, Listener* aListener)
  {
    PR_REMOVE_LINK(aListener);
    JS_free(aCx, aListener);
  }

  jsval mListenerVal;
  ListenerManager::Phase mPhase;
  bool mWantsUntrusted;
};

void
DestroyList(JSContext* aCx, PRCList* aListHead)
{
  for (PRCList* elem = PR_NEXT_LINK(aListHead); elem != aListHead; ) {
    PRCList* nextElem = PR_NEXT_LINK(elem);
    JS_free(aCx, elem);
    elem = nextElem;
  }
}

ListenerCollection*
GetCollectionForType(PRCList* aHead, jsid aTypeId)
{
  for (PRCList* elem = PR_NEXT_LINK(aHead);
       elem != aHead;
       elem = PR_NEXT_LINK(elem)) {
    ListenerCollection* collection = static_cast<ListenerCollection*>(elem);
    if (collection->mTypeId == aTypeId) {
      return collection;
    }
  }
  return NULL;
}

} 

#ifdef DEBUG
ListenerManager::~ListenerManager()
{
  JS_ASSERT(PR_CLIST_IS_EMPTY(&mCollectionHead));
}
#endif

void
ListenerManager::TraceInternal(JSTracer* aTrc)
{
  JS_ASSERT(!PR_CLIST_IS_EMPTY(&mCollectionHead));

  for (PRCList* collectionElem = PR_NEXT_LINK(&mCollectionHead);
       collectionElem != &mCollectionHead;
       collectionElem = PR_NEXT_LINK(collectionElem)) {
    ListenerCollection* collection =
      static_cast<ListenerCollection*>(collectionElem);

    for (PRCList* listenerElem = PR_NEXT_LINK(&collection->mListenerHead);
         listenerElem != &collection->mListenerHead;
         listenerElem = PR_NEXT_LINK(listenerElem)) {
      JS_CALL_VALUE_TRACER(aTrc,
                           static_cast<Listener*>(listenerElem)->mListenerVal,
                           "EventListenerManager listener value");
    }
  }
}

void
ListenerManager::FinalizeInternal(JSContext* aCx)
{
  JS_ASSERT(!PR_CLIST_IS_EMPTY(&mCollectionHead));

  for (PRCList* elem = PR_NEXT_LINK(&mCollectionHead);
       elem != &mCollectionHead;
       elem = PR_NEXT_LINK(elem)) {
    DestroyList(aCx, &static_cast<ListenerCollection*>(elem)->mListenerHead);
  }

  DestroyList(aCx, &mCollectionHead);

#ifdef DEBUG
  PR_INIT_CLIST(&mCollectionHead);
#endif
}

bool
ListenerManager::Add(JSContext* aCx, JSString* aType, jsval aListenerVal,
                     Phase aPhase, bool aWantsUntrusted)
{
  aType = JS_InternJSString(aCx, aType);
  if (!aType) {
    return false;
  }

  if (JSVAL_IS_PRIMITIVE(aListenerVal)) {
    JS_ReportError(aCx, "Bad listener!");
    return false;
  }

  jsid typeId = INTERNED_STRING_TO_JSID(aCx, aType);

  ListenerCollection* collection =
    GetCollectionForType(&mCollectionHead, typeId);
  if (!collection) {
    ListenerCollection* head =
      static_cast<ListenerCollection*>(&mCollectionHead);
    collection = ListenerCollection::Add(aCx, head, typeId);
    if (!collection) {
      return false;
    }
  }

  for (PRCList* elem = PR_NEXT_LINK(&collection->mListenerHead);
       elem != &collection->mListenerHead;
       elem = PR_NEXT_LINK(elem)) {
    Listener* listener = static_cast<Listener*>(elem);
    if (listener->mListenerVal == aListenerVal && listener->mPhase == aPhase) {
      return true;
    }
  }

  Listener* listener =
    Listener::Add(aCx, static_cast<Listener*>(&collection->mListenerHead),
                  aListenerVal, aPhase, aWantsUntrusted);
  if (!listener) {
    return false;
  }

  return true;
}

bool
ListenerManager::Remove(JSContext* aCx, JSString* aType, jsval aListenerVal,
                        Phase aPhase, bool aClearEmpty)
{
  aType = JS_InternJSString(aCx, aType);
  if (!aType) {
    return false;
  }

  ListenerCollection* collection =
    GetCollectionForType(&mCollectionHead, INTERNED_STRING_TO_JSID(aCx, aType));
  if (!collection) {
    return true;
  }

  for (PRCList* elem = PR_NEXT_LINK(&collection->mListenerHead);
       elem != &collection->mListenerHead;
       elem = PR_NEXT_LINK(elem)) {
    Listener* listener = static_cast<Listener*>(elem);
    if (listener->mListenerVal == aListenerVal && listener->mPhase == aPhase) {
      Listener::Remove(aCx, listener);
      if (aClearEmpty && PR_CLIST_IS_EMPTY(&collection->mListenerHead)) {
        ListenerCollection::Remove(aCx, collection);
      }
      break;
    }
  }

  return true;
}

bool
ListenerManager::SetEventListener(JSContext* aCx, JSString* aType,
                                  jsval aListener)
{
  jsval existing;
  if (!GetEventListener(aCx, aType, &existing)) {
    return false;
  }

  if (!JSVAL_IS_VOID(existing) &&
      !Remove(aCx, aType, existing, Onfoo, false)) {
    return false;
  }

  return JSVAL_IS_VOID(aListener) || JSVAL_IS_NULL(aListener) ?
         true :
         Add(aCx, aType, aListener, Onfoo, false);
}

bool
ListenerManager::GetEventListener(JSContext* aCx, JSString* aType,
                                  jsval* aListenerVal)
{
  if (PR_CLIST_IS_EMPTY(&mCollectionHead)) {
    *aListenerVal = JSVAL_VOID;
    return true;
  }

  aType = JS_InternJSString(aCx, aType);
  if (!aType) {
    return false;
  }

  jsid typeId = INTERNED_STRING_TO_JSID(aCx, aType);

  ListenerCollection* collection =
    GetCollectionForType(&mCollectionHead, typeId);
  if (collection) {
    for (PRCList* elem = PR_PREV_LINK(&collection->mListenerHead);
         elem != &collection->mListenerHead;
         elem = PR_NEXT_LINK(elem)) {
      Listener* listener = static_cast<Listener*>(elem);
      if (listener->mPhase == Onfoo) {
        *aListenerVal = listener->mListenerVal;
        return true;
      }
    }
  }
  *aListenerVal = JSVAL_VOID;
  return true;
}

bool
ListenerManager::DispatchEvent(JSContext* aCx, JSObject* aTarget,
                               JSObject* aEvent, bool* aPreventDefaultCalled)
{
  if (!events::IsSupportedEventClass(aCx, aEvent)) {
    JS_ReportErrorNumber(aCx, js_GetErrorMessage, NULL,
                         JSMSG_INCOMPATIBLE_METHOD,
                         "EventTarget", "dispatchEvent", "Event object");
    return false;
  }

  jsval val;
  if (!JS_GetProperty(aCx, aEvent, "target", &val)) {
    return false;
  }

  if (!JSVAL_IS_NULL(val)) {
    
    JS_ReportError(aCx, "Cannot recursively dispatch the same event!");
    return false;
  }

  if (PR_CLIST_IS_EMPTY(&mCollectionHead)) {
    *aPreventDefaultCalled = false;
    return true;
  }

  JSString* eventType;
  JSBool eventIsTrusted;

  if (!JS_GetProperty(aCx, aEvent, "type", &val) ||
      !(eventType = JS_ValueToString(aCx, val)) ||
      !(eventType = JS_InternJSString(aCx, eventType))) {
    return false;
  }

  
  
  if (!JS_GetProperty(aCx, aEvent, "isTrusted", &val) ||
      !JS_ValueToBoolean(aCx, val, &eventIsTrusted)) {
    return false;
  }

  ListenerCollection* collection =
    GetCollectionForType(&mCollectionHead,
                         INTERNED_STRING_TO_JSID(aCx, eventType));
  if (!collection) {
    *aPreventDefaultCalled = false;
    return true;
  }

  js::ContextAllocPolicy ap(aCx);
  js::Vector<jsval, 10, js::ContextAllocPolicy> listeners(ap);

  for (PRCList* elem = PR_NEXT_LINK(&collection->mListenerHead);
       elem != &collection->mListenerHead;
       elem = PR_NEXT_LINK(elem)) {
    Listener* listener = static_cast<Listener*>(elem);

    
    
    if ((eventIsTrusted || listener->mWantsUntrusted) &&
        !listeners.append(listener->mListenerVal)) {
      return false;
    }
  }

  if (listeners.empty()) {
    return true;
  }

  if (!events::SetEventTarget(aCx, aEvent, aTarget)) {
    return false;
  }

  for (size_t index = 0; index < listeners.length(); index++) {
    
    
    
    
    

    jsval listenerVal = listeners[index];

    JSObject* listenerObj;
    if (!JS_ValueToObject(aCx, listenerVal, &listenerObj)) {
      if (!JS_ReportPendingException(aCx)) {
        return false;
      }
      continue;
    }

    static const char sHandleEventChars[] = "handleEvent";

    JSBool hasHandleEvent;
    if (!JS_HasProperty(aCx, listenerObj, sHandleEventChars, &hasHandleEvent)) {
      if (!JS_ReportPendingException(aCx)) {
        return false;
      }
      continue;
    }

    if (hasHandleEvent) {
      if (!JS_GetProperty(aCx, listenerObj, sHandleEventChars, &listenerVal)) {
        if (!JS_ReportPendingException(aCx)) {
          return false;
        }
        continue;
      }
    }

    jsval argv[] = { OBJECT_TO_JSVAL(aEvent) };
    jsval rval = JSVAL_VOID;
    if (!JS_CallFunctionValue(aCx, aTarget, listenerVal, JS_ARRAY_LENGTH(argv),
                              argv, &rval)) {
      if (!JS_ReportPendingException(aCx)) {
        return false;
      }
      continue;
    }
  }

  if (!events::SetEventTarget(aCx, aEvent, NULL)) {
    return false;
  }

  *aPreventDefaultCalled = events::EventWasCanceled(aCx, aEvent);
  return true;
}

bool
ListenerManager::HasListenersForTypeInternal(JSContext* aCx, JSString* aType)
{
  JS_ASSERT(!PR_CLIST_IS_EMPTY(&mCollectionHead));

  aType = JS_InternJSString(aCx, aType);
  if (!aType) {
    return false;
  }

  jsid typeId = INTERNED_STRING_TO_JSID(aCx, aType);
  return !!GetCollectionForType(&mCollectionHead, typeId);
}
