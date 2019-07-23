





































#ifndef __NSDOMWORKERXHRPROXY_H__
#define __NSDOMWORKERXHRPROXY_H__


#include "nsThreadUtils.h"
#include "nsIDOMEventListener.h"
#include "nsIRequestObserver.h"


#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "nsTArray.h"


#include "nsDOMWorkerXHR.h"

class nsIJSXMLHttpRequest;
class nsIThread;
class nsIVariant;
class nsIXMLHttpRequest;
class nsDOMWorkerXHREvent;
class nsDOMWorkerXHRWrappedListener;
class nsXMLHttpRequest;

class nsDOMWorkerXHRProxy : public nsRunnable,
                            public nsIDOMEventListener,
                            public nsIRequestObserver
{

  friend class nsDOMWorkerXHREvent;
  friend class nsDOMWorkerXHR;
  friend class nsDOMWorkerXHRUpload;

  typedef nsCOMPtr<nsIDOMEventListener> Listener;
  typedef nsTArray<Listener> ListenerArray;

  typedef nsRefPtr<nsDOMWorkerXHRWrappedListener> WrappedListener;

  typedef nsresult (NS_STDCALL nsIDOMEventTarget::*EventListenerFunction)
    (const nsAString&, nsIDOMEventListener*, PRBool);

public:
  typedef nsAutoTArray<nsCOMPtr<nsIRunnable>, 5> SyncEventQueue;

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMEVENTLISTENER
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSIREQUESTOBSERVER

  nsDOMWorkerXHRProxy(nsDOMWorkerXHR* aWorkerXHR);
  virtual ~nsDOMWorkerXHRProxy();

  nsresult Init();

  nsIXMLHttpRequest* GetXMLHttpRequest();

  nsresult Abort();

  nsresult OpenRequest(const nsACString& aMethod,
                       const nsACString& aUrl,
                       PRBool aAsync,
                       const nsAString& aUser,
                       const nsAString& aPassword);

  SyncEventQueue* SetSyncEventQueue(SyncEventQueue* aQueue);

  PRInt32 ChannelID() {
    return mChannelID;
  }

protected:
  nsresult InitInternal();
  void DestroyInternal();

  nsresult Destroy();

  void FlipOwnership();

  nsresult AddEventListener(PRUint32 aType,
                            nsIDOMEventListener* aListener,
                            PRBool aOnXListener,
                            PRBool aUploadListener);

  nsresult RemoveEventListener(PRUint32 aType,
                               nsIDOMEventListener* aListener,
                               PRBool aUploadListener);

  already_AddRefed<nsIDOMEventListener> GetOnXListener(PRUint32 aType,
                                                       PRBool aUploadListener);

  nsresult HandleWorkerEvent(nsDOMWorkerXHREvent* aEvent, PRBool aUploadEvent);

  nsresult HandleWorkerEvent(nsIDOMEvent* aEvent, PRBool aUploadEvent);

  nsresult HandleEventInternal(PRUint32 aType,
                               nsIDOMEvent* aEvent,
                               PRBool aUploadEvent);

  void ClearEventListeners();

  
  nsresult GetAllResponseHeaders(char** _retval);
  nsresult GetResponseHeader(const nsACString& aHeader,
                             nsACString& _retval);
  nsresult Send(nsIVariant* aBody);
  nsresult SendAsBinary(const nsAString& aBody);
  nsresult GetResponseText(nsAString& _retval);
  nsresult GetStatusText(nsACString& _retval);
  nsresult GetStatus(nsresult* _retval);
  nsresult GetReadyState(PRInt32* _retval);
  nsresult SetRequestHeader(const nsACString& aHeader,
                            const nsACString& aValue);
  nsresult OverrideMimeType(const nsACString& aMimetype);
  nsresult GetMultipart(PRBool* aMultipart);
  nsresult SetMultipart(PRBool aMultipart);

  
  nsDOMWorkerXHR* mWorkerXHR;

  
  nsIXMLHttpRequest* mXHR;

  
  nsXMLHttpRequest* mConcreteXHR;
  nsIXMLHttpRequestUpload* mUpload;

  nsCOMPtr<nsIThread> mMainThread;

  nsRefPtr<nsDOMWorkerXHREvent> mLastXHREvent;

  nsTArray<ListenerArray> mXHRListeners;
  nsTArray<WrappedListener> mXHROnXListeners;

  nsTArray<ListenerArray> mUploadListeners;
  nsTArray<WrappedListener> mUploadOnXListeners;

  SyncEventQueue* mSyncEventQueue;

  PRInt32 mChannelID;

  
  PRPackedBool mOwnedByXHR;

  PRPackedBool mCanceled;
};

#endif 
