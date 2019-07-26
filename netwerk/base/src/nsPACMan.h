





#ifndef nsPACMan_h__
#define nsPACMan_h__

#include "nsIStreamLoader.h"
#include "nsIInterfaceRequestor.h"
#include "nsIChannelEventSink.h"
#include "ProxyAutoConfig.h"
#include "nsICancelable.h"
#include "nsThreadUtils.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "mozilla/Attributes.h"
#include "mozilla/LinkedList.h"
#include "nsIThread.h"
#include "nsAutoPtr.h"
#include "nsISystemProxySettings.h"
#include "mozilla/TimeStamp.h"

class nsPACMan;




class NS_NO_VTABLE nsPACManCallback : public nsISupports
{
public:
  












  virtual void OnQueryComplete(nsresult status,
                               const nsCString &pacString,
                               const nsCString &newPACURL) = 0;
};

class PendingPACQuery MOZ_FINAL : public nsRunnable,
                                  public mozilla::LinkedListElement<PendingPACQuery>
{
public:
  PendingPACQuery(nsPACMan *pacMan, nsIURI *uri,
                  nsPACManCallback *callback, bool mainThreadResponse);

  
  void Complete(nsresult status, const nsCString &pacString);
  void UseAlternatePACFile(const nsCString &pacURL);

  nsCString                  mSpec;
  nsCString                  mScheme;
  nsCString                  mHost;
  int32_t                    mPort;

  NS_IMETHOD Run(void);     

private:
  nsPACMan                  *mPACMan;  
  nsRefPtr<nsPACManCallback> mCallback;
  bool                       mOnMainThreadOnly;
};






class nsPACMan MOZ_FINAL : public nsIStreamLoaderObserver
                         , public nsIInterfaceRequestor
                         , public nsIChannelEventSink
{
public:
  NS_DECL_ISUPPORTS

  nsPACMan();

  




  void Shutdown();

  












  nsresult AsyncGetProxyForURI(nsIURI *uri, nsPACManCallback *callback,
                               bool mustCallbackOnMainThread);

  








  nsresult LoadPACFromURI(nsIURI *pacURI);

  


  bool IsLoading() { return mLoader != nullptr; }

  


  bool IsPACURI(nsIURI *uri) {
    bool result;
    return mPACURI && NS_SUCCEEDED(mPACURI->Equals(uri, &result)) && result;
  }

  bool IsPACURI(nsACString &spec)
  {
    nsAutoCString tmp;
    return (mPACURI && NS_SUCCEEDED(mPACURI->GetSpec(tmp)) && tmp.Equals(spec));
  }

  NS_HIDDEN_(nsresult) Init(nsISystemProxySettings *);
  static nsPACMan *sInstance;

  
  void ProcessPendingQ();
  void CancelPendingQ(nsresult);

private:
  NS_DECL_NSISTREAMLOADEROBSERVER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK

  friend class PendingPACQuery;
  friend class PACLoadComplete;
  friend class ExecutePACThreadAction;

  ~nsPACMan();

  


  void CancelExistingLoad();

  


  void StartLoading();

  


  void MaybeReloadPAC();

  


  void OnLoadFailure();

  




  nsresult PostQuery(PendingPACQuery *query);

  
  void PostProcessPendingQ();
  void PostCancelPendingQ(nsresult);
  bool ProcessPending();
  void NamePACThread();

private:
  mozilla::net::ProxyAutoConfig mPAC;
  nsCOMPtr<nsIThread>           mPACThread;
  nsCOMPtr<nsISystemProxySettings> mSystemProxySettings;

  mozilla::LinkedList<PendingPACQuery> mPendingQ; 

  nsCOMPtr<nsIURI>             mPACURI;
  nsCString                    mPACURISpec; 
  nsCOMPtr<nsIStreamLoader>    mLoader;
  bool                         mLoadPending;
  bool                         mShutdown;
  mozilla::TimeStamp           mScheduledReload;
  uint32_t                     mLoadFailureCount;

  bool                         mInProgress;
};

#endif  
