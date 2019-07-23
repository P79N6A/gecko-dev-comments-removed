





































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

#endif 
