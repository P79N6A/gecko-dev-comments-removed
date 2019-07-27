





#ifndef nsPACMan_h__
#define nsPACMan_h__

#include "nsIStreamLoader.h"
#include "nsIInterfaceRequestor.h"
#include "nsIChannelEventSink.h"
#include "ProxyAutoConfig.h"
#include "nsThreadUtils.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "mozilla/Attributes.h"
#include "mozilla/LinkedList.h"
#include "nsAutoPtr.h"
#include "mozilla/TimeStamp.h"
#include "prlog.h"

class nsPACMan;
class nsISystemProxySettings;
class nsIThread;
class WaitForThreadShutdown;




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
  NS_DECL_THREADSAFE_ISUPPORTS

  nsPACMan();

  




  void Shutdown();

  












  nsresult AsyncGetProxyForURI(nsIURI *uri, nsPACManCallback *callback,
                               bool mustCallbackOnMainThread);

  








  nsresult LoadPACFromURI(const nsCString &pacSpec);

  


  bool IsLoading() { return mLoader != nullptr; }

  







  bool IsPACURI(const nsACString &spec)
  {
    return mPACURISpec.Equals(spec) || mPACURIRedirectSpec.Equals(spec) ||
      mNormalPACURISpec.Equals(spec);
  }

  bool IsPACURI(nsIURI *uri) {
    if (mPACURISpec.IsEmpty() && mPACURIRedirectSpec.IsEmpty())
      return false;

    nsAutoCString tmp;
    uri->GetSpec(tmp);
    return IsPACURI(tmp);
  }

  nsresult Init(nsISystemProxySettings *);
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
  friend class WaitForThreadShutdown;

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

  
  
  
  nsCString                    mPACURISpec;
  nsCString                    mPACURIRedirectSpec;
  nsCString                    mNormalPACURISpec;

  nsCOMPtr<nsIStreamLoader>    mLoader;
  bool                         mLoadPending;
  bool                         mShutdown;
  mozilla::TimeStamp           mScheduledReload;
  uint32_t                     mLoadFailureCount;

  bool                         mInProgress;
};

namespace mozilla {
namespace net {
PRLogModuleInfo* GetProxyLog();
}
}

#endif  
