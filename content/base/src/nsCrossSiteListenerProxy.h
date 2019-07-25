




































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

class nsIURI;
class nsIParser;
class nsIPrincipal;

extern bool
IsValidHTTPToken(const nsCSubstring& aToken);

nsresult
NS_StartCORSPreflight(nsIChannel* aRequestChannel,
                      nsIStreamListener* aListener,
                      nsIPrincipal* aPrincipal,
                      bool aWithCredentials,
                      nsTArray<nsCString>& aACUnsafeHeaders,
                      nsIChannel** aPreflightChannel);

class nsCORSListenerProxy : public nsIStreamListener,
                            public nsIInterfaceRequestor,
                            public nsIChannelEventSink,
                            public nsIAsyncVerifyRedirectCallback
{
public:
  nsCORSListenerProxy(nsIStreamListener* aOuter,
                      nsIPrincipal* aRequestingPrincipal,
                      nsIChannel* aChannel,
                      bool aWithCredentials,
                      nsresult* aResult);
  nsCORSListenerProxy(nsIStreamListener* aOuter,
                      nsIPrincipal* aRequestingPrincipal,
                      nsIChannel* aChannel,
                      bool aWithCredentials,
                      const nsCString& aPreflightMethod,
                      const nsTArray<nsCString>& aPreflightHeaders,
                      nsresult* aResult);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIASYNCVERIFYREDIRECTCALLBACK

  
  static void Startup();

  static void Shutdown();

private:
  nsresult UpdateChannel(nsIChannel* aChannel);
  nsresult CheckRequestApproved(nsIRequest* aRequest);

  nsCOMPtr<nsIStreamListener> mOuterListener;
  nsCOMPtr<nsIPrincipal> mRequestingPrincipal;
  nsCOMPtr<nsIInterfaceRequestor> mOuterNotificationCallbacks;
  bool mWithCredentials;
  bool mRequestApproved;
  bool mHasBeenCrossSite;
  bool mIsPreflight;
  nsCString mPreflightMethod;
  nsTArray<nsCString> mPreflightHeaders;
  nsCOMPtr<nsIAsyncVerifyRedirectCallback> mRedirectCallback;
  nsCOMPtr<nsIChannel> mOldRedirectChannel;
  nsCOMPtr<nsIChannel> mNewRedirectChannel;
};

#endif
