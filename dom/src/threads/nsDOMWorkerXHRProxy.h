





































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
class nsDOMWorkerXHRFinishSyncXHRRunnable;
class nsDOMWorkerXHRWrappedListener;
class nsXMLHttpRequest;

class nsDOMWorkerXHRProxy : public nsIRunnable,
                            public nsIDOMEventListener,
                            public nsIRequestObserver
{
  friend class nsDOMWorkerXHRAttachUploadListenersRunnable;
  friend class nsDOMWorkerXHREvent;
  friend class nsDOMWorkerXHRFinishSyncXHRRunnable;
  friend class nsDOMWorkerXHRLastProgressOrLoadEvent;
  friend class nsDOMWorkerXHR;
  friend class nsDOMWorkerXHRUpload;

  typedef nsCOMPtr<nsIDOMEventListener> Listener;
  typedef nsTArray<Listener> ListenerArray;

  typedef nsRefPtr<nsDOMWorkerXHRWrappedListener> WrappedListener;

  typedef nsresult (NS_STDCALL nsIDOMEventTarget::*EventListenerFunction)
    (const nsAString&, nsIDOMEventListener*, PRBool);

public:
  typedef nsAutoTArray<nsCOMPtr<nsIRunnable>, 5> SyncEventQueue;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSIREQUESTOBSERVER

  nsDOMWorkerXHRProxy(nsDOMWorkerXHR* aWorkerXHR);
  virtual ~nsDOMWorkerXHRProxy();

  nsresult Init();

  nsIXMLHttpRequest* GetXMLHttpRequest();

  nsresult OpenRequest(const nsACString& aMethod,
                       const nsACString& aUrl,
                       PRBool aAsync,
                       const nsAString& aUser,
                       const nsAString& aPassword);

  nsresult Abort();

  SyncEventQueue* SetSyncEventQueue(SyncEventQueue* aQueue);

  PRInt32 ChannelID() {
    return mChannelID;
  }

protected:
  nsresult InitInternal();
  void DestroyInternal();

  nsresult Destroy();

  void AddRemoveXHRListeners(PRBool aAdd);
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
  nsresult GetWithCredentials(PRBool* aWithCredentials);
  nsresult SetWithCredentials(PRBool aWithCredentials);

  nsresult RunSyncEventLoop();

  
  
  PRBool HasListenersForType(PRUint32 aType, nsIDOMEvent* aEvent = nsnull);

  
  nsDOMWorkerXHR* mWorkerXHR;

  
  nsIXMLHttpRequest* mXHR;

  
  nsXMLHttpRequest* mConcreteXHR;
  nsIXMLHttpRequestUpload* mUpload;

  nsCOMPtr<nsIThread> mMainThread;

  nsRefPtr<nsDOMWorkerXHREvent> mLastXHREvent;
  nsRefPtr<nsDOMWorkerXHREvent> mLastProgressOrLoadEvent;

  nsTArray<ListenerArray> mXHRListeners;
  nsTArray<WrappedListener> mXHROnXListeners;

  nsTArray<ListenerArray> mUploadListeners;
  nsTArray<WrappedListener> mUploadOnXListeners;

  SyncEventQueue* mSyncEventQueue;

  PRInt32 mChannelID;

  
  nsCOMPtr<nsIThread> mSyncXHRThread;

  
  nsRefPtr<nsDOMWorkerXHRFinishSyncXHRRunnable> mSyncFinishedRunnable;

  
  PRPackedBool mOwnedByXHR;

  PRPackedBool mWantUploadListeners;
  PRPackedBool mCanceled;

  PRPackedBool mSyncRequest;
};

#endif 
