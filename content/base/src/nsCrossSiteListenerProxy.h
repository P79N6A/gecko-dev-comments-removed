




































#ifndef nsCrossSiteListenerProxy_h__
#define nsCrossSiteListenerProxy_h__

#include "nsIStreamListener.h"
#include "nsIInterfaceRequestor.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIURI.h"
#include "nsTArray.h"
#include "nsIInterfaceRequestor.h"
#include "nsIChannelEventSink.h"

class nsIURI;
class nsIParser;
class nsIPrincipal;

extern PRBool
IsValidHTTPToken(const nsCSubstring& aToken);

class nsCrossSiteListenerProxy : public nsIStreamListener,
                                 public nsIInterfaceRequestor,
                                 public nsIChannelEventSink
{
public:
  nsCrossSiteListenerProxy(nsIStreamListener* aOuter,
                           nsIPrincipal* aRequestingPrincipal,
                           nsIChannel* aChannel,
                           PRBool aWithCredentials,
                           nsresult* aResult);
  nsCrossSiteListenerProxy(nsIStreamListener* aOuter,
                           nsIPrincipal* aRequestingPrincipal,
                           nsIChannel* aChannel,
                           PRBool aWithCredentials,
                           const nsCString& aPreflightMethod,
                           const nsTArray<nsCString>& aPreflightHeaders,
                           nsresult* aResult);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK

  
  static void Startup();

  void AllowHTTPResult(PRUint32 aResultCode)
  {
    mAllowedHTTPErrors.AppendElement(aResultCode);
  }

private:
  nsresult UpdateChannel(nsIChannel* aChannel);
  nsresult CheckRequestApproved(nsIRequest* aRequest, PRBool aIsRedirect);

  nsCOMPtr<nsIStreamListener> mOuterListener;
  nsCOMPtr<nsIPrincipal> mRequestingPrincipal;
  nsCOMPtr<nsIInterfaceRequestor> mOuterNotificationCallbacks;
  PRBool mWithCredentials;
  PRBool mRequestApproved;
  PRBool mHasBeenCrossSite;
  PRBool mIsPreflight;
  nsCString mPreflightMethod;
  nsTArray<nsCString> mPreflightHeaders;
  nsTArray<PRUint32> mAllowedHTTPErrors;
};

#endif
