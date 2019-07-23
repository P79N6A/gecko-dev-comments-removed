




































#include "nsIStreamListener.h"
#include "nsIInterfaceRequestor.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsIChannelEventSink.h"

class nsIURI;
class nsIParser;
class nsIPrincipal;
class nsIHttpChannel;

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

  static nsresult CheckPreflight(nsIHttpChannel* aRequestChannel,
                                 nsIStreamListener* aRequestListener,
                                 nsISupports* aRequestContext,
                                 nsIPrincipal* aRequestingPrincipal,
                                 PRBool aForcePreflight,
                                 nsTArray<nsCString>& aUnsafeHeaders,
                                 PRBool aWithCredentials,
                                 PRBool* aPreflighted,
                                 nsIChannel** aPreflightChannel,
                                 nsIStreamListener** aPreflightListener);

  static void ShutdownPreflightCache();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK

private:
  nsresult UpdateChannel(nsIChannel* aChannel);
  nsresult CheckRequestApproved(nsIRequest* aRequest);

  nsCOMPtr<nsIStreamListener> mOuterListener;
  nsCOMPtr<nsIPrincipal> mRequestingPrincipal;
  nsCOMPtr<nsIInterfaceRequestor> mOuterNotificationCallbacks;
  PRBool mWithCredentials;
  PRBool mRequestApproved;
  PRBool mHasBeenCrossSite;
  PRBool mIsPreflight;
  nsCString mPreflightMethod;
  nsTArray<nsCString> mPreflightHeaders;
};
