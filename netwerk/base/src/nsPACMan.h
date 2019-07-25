





































#ifndef nsPACMan_h__
#define nsPACMan_h__

#include "nsIStreamLoader.h"
#include "nsIInterfaceRequestor.h"
#include "nsIChannelEventSink.h"
#include "nsIProxyAutoConfig.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "prclist.h"




class NS_NO_VTABLE nsPACManCallback : public nsISupports
{
public:
  








  virtual void OnQueryComplete(nsresult status, const nsCString &pacString) = 0;
};





class nsPACMan : public nsIStreamLoaderObserver
               , public nsIInterfaceRequestor
               , public nsIChannelEventSink
{
public:
  NS_DECL_ISUPPORTS

  nsPACMan();

  




  void Shutdown();

  










  nsresult GetProxyForURI(nsIURI *uri, nsACString &result);

  










  nsresult AsyncGetProxyForURI(nsIURI *uri, nsPACManCallback *callback);

  








  nsresult LoadPACFromURI(nsIURI *pacURI);

  


  bool IsLoading() { return mLoader != nsnull; }

  


  bool IsPACURI(nsIURI *uri) {
    bool result;
    return mPACURI && NS_SUCCEEDED(mPACURI->Equals(uri, &result)) && result;
  }

private:
  NS_DECL_NSISTREAMLOADEROBSERVER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK

  ~nsPACMan();

  


  void CancelExistingLoad();

  




  void ProcessPendingQ(nsresult status);

  


  void StartLoading();

  


  void MaybeReloadPAC();

  


  void OnLoadFailure();

private:
  nsCOMPtr<nsIProxyAutoConfig> mPAC;
  nsCOMPtr<nsIURI>             mPACURI;
  PRCList                      mPendingQ;
  nsCOMPtr<nsIStreamLoader>    mLoader;
  bool                         mLoadPending;
  bool                         mShutdown;
  PRTime                       mScheduledReload;
  PRUint32                     mLoadFailureCount;
};

#endif  
