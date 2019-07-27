




#ifndef nsCORSListenerProxy_h__
#define nsCORSListenerProxy_h__

#include "nsIStreamListener.h"
#include "nsIInterfaceRequestor.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIURI.h"
#include "nsTArray.h"
#include "nsIInterfaceRequestor.h"
#include "nsIChannelEventSink.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "mozilla/Attributes.h"

class nsIURI;
class nsIPrincipal;
class nsINetworkInterceptController;

nsresult
NS_StartCORSPreflight(nsIChannel* aRequestChannel,
                      nsIStreamListener* aListener,
                      nsIPrincipal* aPrincipal,
                      bool aWithCredentials,
                      nsTArray<nsCString>& aACUnsafeHeaders,
                      nsIChannel** aPreflightChannel);

enum class DataURIHandling
{
  Allow,
  Disallow
};

class nsCORSListenerProxy final : public nsIStreamListener,
                                  public nsIInterfaceRequestor,
                                  public nsIChannelEventSink,
                                  public nsIAsyncVerifyRedirectCallback
{
public:
  nsCORSListenerProxy(nsIStreamListener* aOuter,
                      nsIPrincipal* aRequestingPrincipal,
                      bool aWithCredentials);
  nsCORSListenerProxy(nsIStreamListener* aOuter,
                      nsIPrincipal* aRequestingPrincipal,
                      bool aWithCredentials,
                      const nsCString& aPreflightMethod,
                      const nsTArray<nsCString>& aPreflightHeaders);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIASYNCVERIFYREDIRECTCALLBACK

  
  static void Startup();

  static void Shutdown();

  nsresult Init(nsIChannel* aChannel, DataURIHandling aAllowDataURI);

  void SetInterceptController(nsINetworkInterceptController* aInterceptController);

private:
  ~nsCORSListenerProxy();

  nsresult UpdateChannel(nsIChannel* aChannel, DataURIHandling aAllowDataURI);
  nsresult CheckRequestApproved(nsIRequest* aRequest);

  nsCOMPtr<nsIStreamListener> mOuterListener;
  
  nsCOMPtr<nsIPrincipal> mRequestingPrincipal;
  
  
  nsCOMPtr<nsIPrincipal> mOriginHeaderPrincipal;
  nsCOMPtr<nsIInterfaceRequestor> mOuterNotificationCallbacks;
  nsCOMPtr<nsINetworkInterceptController> mInterceptController;
  bool mWithCredentials;
  bool mRequestApproved;
  bool mHasBeenCrossSite;
  bool mIsPreflight;
#ifdef DEBUG
  bool mInited;
#endif
  nsCString mPreflightMethod;
  nsTArray<nsCString> mPreflightHeaders;
  nsCOMPtr<nsIAsyncVerifyRedirectCallback> mRedirectCallback;
  nsCOMPtr<nsIChannel> mOldRedirectChannel;
  nsCOMPtr<nsIChannel> mNewRedirectChannel;
};

#endif
