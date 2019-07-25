





































#ifndef __NSDOMWORKERMESSAGEHANDLER_H__
#define __NSDOMWORKERMESSAGEHANDLER_H__

#include "nsIClassInfo.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMWorkers.h"

#include "nsIProgrammingLanguage.h"

#include "jsapi.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIClassInfoImpl.h"
#include "nsStringGlue.h"
#include "nsTArray.h"
#include "nsIWeakReference.h"

class nsDOMWorkerEventListenerBase
{
public:
  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();

  virtual already_AddRefed<nsIDOMEventListener> GetListener() = 0;
  virtual JSObject* GetJSObject() = 0;

protected:
  virtual ~nsDOMWorkerEventListenerBase() { }

  nsAutoRefCnt mRefCnt;
};

class nsDOMWorkerWeakEventListener : public nsDOMWorkerEventListenerBase
{
public:
  nsDOMWorkerWeakEventListener()
  : mObj(NULL) { }

  nsresult Init(nsIDOMEventListener* aListener);

  already_AddRefed<nsIDOMEventListener> GetListener();

  virtual JSObject* GetJSObject() {
    return mObj;
  }

private:
  JSObject* mObj;
};

class nsDOMWorkerWrappedWeakEventListener : public nsDOMWorkerEventListenerBase
{
public:
  nsDOMWorkerWrappedWeakEventListener(nsDOMWorkerWeakEventListener* aInner);

  already_AddRefed<nsIDOMEventListener> GetListener() {
    return mInner->GetListener();
  }

  virtual JSObject* GetJSObject() {
    return mInner->GetJSObject();
  }

private:
  nsRefPtr<nsDOMWorkerWeakEventListener> mInner;
};

class nsDOMWorkerMessageHandler : public nsIDOMEventTarget,
                                  public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTTARGET
  NS_DECL_NSICLASSINFO

  virtual nsresult SetOnXListener(const nsAString& aType,
                                  nsIDOMEventListener* aListener);

  already_AddRefed<nsIDOMEventListener>
    GetOnXListener(const nsAString& aType) const;

  void ClearListeners(const nsAString& aType);

  PRBool HasListeners(const nsAString& aType);

  void ClearAllListeners();

  void Trace(JSTracer* aTracer);

protected:
  virtual ~nsDOMWorkerMessageHandler() { }

private:

  typedef nsCOMPtr<nsIDOMEventListener> Listener;
  typedef nsTArray<Listener> ListenerArray;

  typedef nsRefPtr<nsDOMWorkerEventListenerBase> WeakListener;
  typedef nsTArray<WeakListener> WeakListenerArray;

  struct ListenerCollection {
    PRBool operator==(const ListenerCollection& aOther) const {
      return this == &aOther;
    }

    ListenerCollection(const nsAString& aType)
    : type(aType) { }

    nsString type;
    WeakListenerArray listeners;
    nsRefPtr<nsDOMWorkerWrappedWeakEventListener> onXListener;
  };

  const ListenerCollection* GetListenerCollection(const nsAString& aType) const;

  void GetListenersForType(const nsAString& aType,
                           ListenerArray& _retval) const;

  nsTArray<ListenerCollection> mCollections;
};

#define NS_FORWARD_INTERNAL_NSIDOMEVENTTARGET(_to) \
  virtual nsIDOMEventTarget * GetTargetForDOMEvent(void) { return _to GetTargetForDOMEvent(); } \
  virtual nsIDOMEventTarget * GetTargetForEventTargetChain(void) { return _to GetTargetForEventTargetChain(); } \
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor & aVisitor) { return _to PreHandleEvent(aVisitor); } \
  virtual nsresult WillHandleEvent(nsEventChainPostVisitor & aVisitor) { return _to WillHandleEvent(aVisitor); } \
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor & aVisitor) { return _to PostHandleEvent(aVisitor); } \
  virtual nsresult DispatchDOMEvent(nsEvent *aEvent, nsIDOMEvent *aDOMEvent, nsPresContext *aPresContext, nsEventStatus *aEventStatus) { return _to DispatchDOMEvent(aEvent, aDOMEvent, aPresContext, aEventStatus); } \
  virtual nsIEventListenerManager * GetListenerManager(PRBool aMayCreate) { return _to GetListenerManager(aMayCreate); } \
  virtual nsresult AddEventListenerByIID(nsIDOMEventListener *aListener, const nsIID & aIID) { return _to AddEventListenerByIID(aListener, aIID); } \
  virtual nsresult RemoveEventListenerByIID(nsIDOMEventListener *aListener, const nsIID & aIID) { return _to RemoveEventListenerByIID(aListener, aIID); } \
  virtual nsresult GetSystemEventGroup(nsIDOMEventGroup **_retval NS_OUTPARAM) { return _to GetSystemEventGroup(_retval); } \
  virtual nsIScriptContext * GetContextForEventHandlers(nsresult *aRv NS_OUTPARAM) { return _to GetContextForEventHandlers(aRv); } \
  virtual JSContext * GetJSContextForEventHandlers(void) { return _to GetJSContextForEventHandlers(); } 


#endif 
