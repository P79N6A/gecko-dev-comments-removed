





































#ifndef __NSDOMWORKERXHR_H__
#define __NSDOMWORKERXHR_H__


#include "nsIClassInfo.h"
#include "nsIXMLHttpRequest.h"
#include "nsIXPCScriptable.h"


#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"


#include "nsDOMWorker.h"
#include "nsDOMWorkerMacros.h"
#include "nsDOMWorkerXHRProxy.h"


#define LISTENER_TYPE_ABORT 0
#define LISTENER_TYPE_ERROR 1
#define LISTENER_TYPE_LOAD 2
#define LISTENER_TYPE_LOADSTART 3
#define LISTENER_TYPE_PROGRESS 4
#define LISTENER_TYPE_READYSTATECHANGE 5
#define LISTENER_TYPE_LOADEND 6

class nsIXPConnectWrappedNative;

class nsDOMWorkerXHREventTarget : public nsDOMWorkerMessageHandler,
                                  public nsIXMLHttpRequestEventTarget
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMWorkerMessageHandler::)
  NS_FORWARD_NSIDOMNSEVENTTARGET(nsDOMWorkerMessageHandler::)
  NS_DECL_NSIXMLHTTPREQUESTEVENTTARGET

  static const char* const sListenerTypes[];
  static const PRUint32 sMaxXHREventTypes;
  static const PRUint32 sMaxUploadEventTypes;

  static PRUint32 GetListenerTypeFromString(const nsAString& aString);

protected:
  virtual ~nsDOMWorkerXHREventTarget() { }
};

class nsDOMWorkerXHRUpload;

class nsDOMWorkerXHR : public nsDOMWorkerFeature,
                       public nsDOMWorkerXHREventTarget,
                       public nsIXMLHttpRequest,
                       public nsIXPCScriptable
{
  typedef mozilla::Mutex Mutex;

  friend class nsDOMWorkerXHREvent;
  friend class nsDOMWorkerXHRLastProgressOrLoadEvent;
  friend class nsDOMWorkerXHRProxy;
  friend class nsDOMWorkerXHRUpload;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIXMLHTTPREQUEST
  NS_FORWARD_NSICLASSINFO_NOGETINTERFACES(nsDOMWorkerXHREventTarget::)
  NS_DECL_NSICLASSINFO_GETINTERFACES
  NS_DECL_NSIXPCSCRIPTABLE

  nsDOMWorkerXHR(nsDOMWorker* aWorker);

  nsresult Init();

  virtual void Cancel();

  virtual nsresult SetOnXListener(const nsAString& aType,
                                  nsIDOMEventListener* aListener);

private:
  virtual ~nsDOMWorkerXHR();

  Mutex& GetLock() {
    return mWorker->GetLock();
  }

  already_AddRefed<nsIXPConnectWrappedNative> GetWrappedNative() {
    nsCOMPtr<nsIXPConnectWrappedNative> wrappedNative(mWrappedNative);
    return wrappedNative.forget();
  }

  nsRefPtr<nsDOMWorkerXHRProxy> mXHRProxy;
  nsRefPtr<nsDOMWorkerXHRUpload> mUpload;

  nsIXPConnectWrappedNative* mWrappedNative;

  volatile PRBool mCanceled;
};

class nsDOMWorkerXHRUpload : public nsDOMWorkerXHREventTarget,
                             public nsIXMLHttpRequestUpload
{
  friend class nsDOMWorkerXHR;

public:
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_INTERNAL_NSIDOMEVENTTARGET(nsDOMWorkerMessageHandler::)
  NS_IMETHOD AddEventListener(const nsAString& aType,
                              nsIDOMEventListener* aListener,
                              PRBool aUseCapture);
  NS_IMETHOD RemoveEventListener(const nsAString& aType,
                                 nsIDOMEventListener* aListener,
                                 PRBool aUseCapture);
  NS_IMETHOD DispatchEvent(nsIDOMEvent* aEvent,
                           PRBool* _retval);
  NS_IMETHOD AddEventListener(const nsAString& aType,
                              nsIDOMEventListener* aListener,
                              PRBool aUseCapture,
                              PRBool aWantsUntrusted,
                              PRUint8 optional_argc);
  NS_FORWARD_NSIXMLHTTPREQUESTEVENTTARGET(nsDOMWorkerXHREventTarget::)
  NS_DECL_NSIXMLHTTPREQUESTUPLOAD
  NS_FORWARD_NSICLASSINFO_NOGETINTERFACES(nsDOMWorkerXHREventTarget::)
  NS_DECL_NSICLASSINFO_GETINTERFACES

  nsDOMWorkerXHRUpload(nsDOMWorkerXHR* aWorkerXHR);

  virtual nsresult SetOnXListener(const nsAString& aType,
                                  nsIDOMEventListener* aListener);

protected:
  virtual ~nsDOMWorkerXHRUpload() { }

  nsRefPtr<nsDOMWorkerXHR> mWorkerXHR;
};

#endif 
