





































#ifndef mozilla_dom_workers_xmlhttprequestprivate_h__
#define mozilla_dom_workers_xmlhttprequestprivate_h__

#include "Workers.h"

#include "jspubtd.h"
#include "nsAutoPtr.h"

#include "EventTarget.h"
#include "WorkerFeature.h"
#include "WorkerPrivate.h"

BEGIN_WORKERS_NAMESPACE

namespace xhr {

class Proxy;

class XMLHttpRequestPrivate : public events::EventTarget,
                              public WorkerFeature
{
  JSObject* mJSObject;
  JSObject* mUploadJSObject;
  WorkerPrivate* mWorkerPrivate;
  nsRefPtr<Proxy> mProxy;
  PRUint32 mJSObjectRootCount;

  bool mMultipart;
  bool mBackgroundRequest;
  bool mWithCredentials;
  bool mCanceled;

public:
  XMLHttpRequestPrivate(JSObject* aObj, WorkerPrivate* aWorkerPrivate);
  ~XMLHttpRequestPrivate();

  void
  FinalizeInstance(JSContext* aCx)
  {
    ReleaseProxy();
    events::EventTarget::FinalizeInstance(aCx);
  }

  void
  UnrootJSObject(JSContext* aCx);

  JSObject*
  GetJSObject()
  {
    mWorkerPrivate->AssertIsOnWorkerThread();
    return mJSObject;
  }

  JSObject*
  GetUploadJSObject()
  {
    mWorkerPrivate->AssertIsOnWorkerThread();
    return mUploadJSObject;
  }

  void
  SetUploadObject(JSObject* aUploadObj)
  {
    mWorkerPrivate->AssertIsOnWorkerThread();
    mUploadJSObject = aUploadObj;
  }

  using events::EventTarget::TraceInstance;
  using events::EventTarget::GetEventListenerOnEventTarget;
  using events::EventTarget::SetEventListenerOnEventTarget;

  bool
  Notify(JSContext* aCx, Status aStatus);

  bool
  SetMultipart(JSContext* aCx, jsval *aVp);

  bool
  SetMozBackgroundRequest(JSContext* aCx, jsval *aVp);

  bool
  SetWithCredentials(JSContext* aCx, jsval *aVp);

  bool
  Abort(JSContext* aCx);

  JSString*
  GetAllResponseHeaders(JSContext* aCx);

  JSString*
  GetResponseHeader(JSContext* aCx, JSString* aHeader);

  bool
  Open(JSContext* aCx, JSString* aMethod, JSString* aURL, bool aAsync,
       JSString* aUser, JSString* aPassword);

  bool
  Send(JSContext* aCx, bool aHasBody, jsval aBody);

  bool
  SendAsBinary(JSContext* aCx, JSString* aBody);

  bool
  SetRequestHeader(JSContext* aCx, JSString* aHeader, JSString* aValue);

private:
  void
  ReleaseProxy();

  bool
  RootJSObject(JSContext* aCx);

  bool
  MaybeDispatchPrematureAbortEvents(JSContext* aCx, bool aFromOpen);

  bool
  DispatchPrematureAbortEvent(JSContext* aCx, JSObject* aTarget,
                              PRUint64 aEventType, bool aUploadTarget);
};

}  

END_WORKERS_NAMESPACE

#endif 
