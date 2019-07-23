





































#ifndef __NSDOMWORKERXHR_H__
#define __NSDOMWORKERXHR_H__


#include "nsIXMLHttpRequest.h"
#include "nsIClassInfo.h"




#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "prlock.h"


#include "nsDOMWorkerThread.h"

class nsDOMWorkerXHR;
class nsDOMWorkerXHREvent;
class nsDOMWorkerXHRProxy;

class nsDOMWorkerXHREventTarget : public nsIXMLHttpRequestEventTarget
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTTARGET
  NS_DECL_NSIXMLHTTPREQUESTEVENTTARGET

  static const char* const sListenerTypes[];
  static const PRUint32 sMaxXHREventTypes;
  static const PRUint32 sMaxUploadEventTypes;

  static PRUint32 GetListenerTypeFromString(const nsAString& aString);

  virtual nsresult SetEventListener(PRUint32 aType,
                                    nsIDOMEventListener* aListener,
                                    PRBool aOnXListener) = 0;

  virtual nsresult UnsetEventListener(PRUint32 aType,
                                      nsIDOMEventListener* aListener) = 0;

  virtual nsresult HandleWorkerEvent(nsIDOMEvent* aEvent) = 0;

  virtual already_AddRefed<nsIDOMEventListener>
    GetOnXListener(PRUint32 aType) = 0;

protected:
  virtual ~nsDOMWorkerXHREventTarget() { }
};

class nsDOMWorkerXHRUpload : public nsDOMWorkerXHREventTarget,
                             public nsIXMLHttpRequestUpload,
                             public nsIClassInfo
{
  friend class nsDOMWorkerXHR;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMWorkerXHREventTarget::)
  NS_FORWARD_NSIXMLHTTPREQUESTEVENTTARGET(nsDOMWorkerXHREventTarget::)
  NS_DECL_NSIXMLHTTPREQUESTUPLOAD
  NS_DECL_NSICLASSINFO

  nsDOMWorkerXHRUpload(nsDOMWorkerXHR* aWorkerXHR);

  virtual nsresult SetEventListener(PRUint32 aType,
                                    nsIDOMEventListener* aListener,
                                    PRBool aOnXListener);

  virtual nsresult UnsetEventListener(PRUint32 aType,
                                      nsIDOMEventListener* aListener);

  virtual nsresult HandleWorkerEvent(nsIDOMEvent* aEvent);

  virtual already_AddRefed<nsIDOMEventListener>
    GetOnXListener(PRUint32 aType);

protected:
  virtual ~nsDOMWorkerXHRUpload() { }

  nsRefPtr<nsDOMWorkerXHR> mWorkerXHR;
};

class nsDOMWorkerXHR : public nsDOMWorkerXHREventTarget,
                       public nsIXMLHttpRequest,
                       public nsIClassInfo
{
  friend class nsDOMWorkerXHREvent;
  friend class nsDOMWorkerXHRProxy;
  friend class nsDOMWorkerXHRUpload;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIXMLHTTPREQUEST
  NS_DECL_NSICLASSINFO

  nsDOMWorkerXHR(nsDOMWorkerThread* aWorker);

  nsresult Init();

  void Cancel();

  virtual nsresult SetEventListener(PRUint32 aType,
                                    nsIDOMEventListener* aListener,
                                    PRBool aOnXListener);

  virtual nsresult UnsetEventListener(PRUint32 aType,
                                      nsIDOMEventListener* aListener);

  virtual nsresult HandleWorkerEvent(nsIDOMEvent* aEvent);

  virtual already_AddRefed<nsIDOMEventListener>
    GetOnXListener(PRUint32 aType);

protected:
  virtual ~nsDOMWorkerXHR();

  PRLock* Lock() {
    return mWorker->Lock();
  }

  nsRefPtr<nsDOMWorkerThread> mWorker;
  nsRefPtr<nsDOMWorkerXHRProxy> mXHRProxy;
  nsRefPtr<nsDOMWorkerXHRUpload> mUpload;

  volatile PRBool mCanceled;
};

#endif 
