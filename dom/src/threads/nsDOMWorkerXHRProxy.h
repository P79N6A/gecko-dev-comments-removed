





































#ifndef __NSDOMWORKERXHRPROXY_H__
#define __NSDOMWORKERXHRPROXY_H__


#include "nsThreadUtils.h"
#include "nsIDOMEventListener.h"
#include "nsIRequestObserver.h"


#include "nsIDOMEventTarget.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "nsTArray.h"

class nsIJSXMLHttpRequest;
class nsIThread;
class nsIVariant;
class nsIXMLHttpRequest;
class nsIXMLHttpRequestUpload;
class nsIXPConnectWrappedNative;
class nsDOMWorkerXHR;
class nsDOMWorkerXHREvent;
class nsDOMWorkerXHRFinishSyncXHRRunnable;
class nsDOMWorkerXHRState;
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

  typedef NS_STDCALL_FUNCPROTO(nsresult, EventListenerFunction, nsIDOMEventTarget,
                               AddEventListener,
                               (const nsAString&, nsIDOMEventListener*, PRBool));

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

  nsresult Open(const nsACString& aMethod,
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
  struct ProgressInfo {
    ProgressInfo() : computable(PR_FALSE), loaded(0), total(0) { }

    PRBool computable;
    PRUint64 loaded;
    PRUint64 total;
  };

  nsresult InitInternal();
  void DestroyInternal();

  nsresult Destroy();

  void AddRemoveXHRListeners(PRBool aAdd);
  void FlipOwnership();

  nsresult UploadEventListenerAdded();

  nsresult HandleWorkerEvent(nsDOMWorkerXHREvent* aEvent,
                             PRBool aUploadEvent);

  nsresult HandleEventRunnable(nsIRunnable* aRunnable);

  
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

  PRBool IsUploadEvent(nsIDOMEvent* aEvent);

  nsresult DispatchPrematureAbortEvents(PRUint32 aType,
                                        nsIDOMEventTarget* aTarget,
                                        ProgressInfo* aProgressInfo);

  nsresult MaybeDispatchPrematureAbortEvents(PRBool aFromOpenRequest);

  
  nsDOMWorkerXHR* mWorkerXHR;
  nsCOMPtr<nsIXPConnectWrappedNative> mWorkerXHRWN;

  
  nsIXMLHttpRequest* mXHR;

  
  nsXMLHttpRequest* mConcreteXHR;
  nsIXMLHttpRequestUpload* mUpload;

  nsCOMPtr<nsIThread> mMainThread;

  nsRefPtr<nsDOMWorkerXHRState> mLastXHRState;
  nsRefPtr<nsDOMWorkerXHREvent> mLastProgressOrLoadEvent;

  SyncEventQueue* mSyncEventQueue;

  PRInt32 mChannelID;

  
  nsCOMPtr<nsIThread> mSyncXHRThread;

  
  nsRefPtr<nsDOMWorkerXHRFinishSyncXHRRunnable> mSyncFinishedRunnable;

  nsAutoPtr<ProgressInfo> mDownloadProgressInfo;
  nsAutoPtr<ProgressInfo> mUploadProgressInfo;

  
  PRPackedBool mOwnedByXHR;

  PRPackedBool mWantUploadListeners;
  PRPackedBool mCanceled;

  PRPackedBool mSyncRequest;

};

#endif 
