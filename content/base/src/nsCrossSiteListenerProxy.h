




































#include "nsIStreamListener.h"
#include "nsIInterfaceRequestor.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIURI.h"
#include "nsTArray.h"
#include "nsIContentSink.h"
#include "nsIXMLContentSink.h"
#include "nsIExpatSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsIChannelEventSink.h"

class nsIURI;
class nsIParser;
class nsIPrincipal;

class nsCrossSiteListenerProxy : public nsIStreamListener,
                                 public nsIXMLContentSink,
                                 public nsIExpatSink,
                                 public nsIInterfaceRequestor,
                                 public nsIChannelEventSink
{
public:
  nsCrossSiteListenerProxy(nsIStreamListener* aOuter,
                           nsIPrincipal* aRequestingPrincipal,
                           nsIChannel* aChannel,
                           nsresult* aResult);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIEXPATSINK
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK

  
  NS_IMETHOD WillTokenize(void) { return NS_OK; }
  NS_IMETHOD WillBuildModel(void);
  NS_IMETHOD DidBuildModel()  { return NS_OK; }
  NS_IMETHOD WillInterrupt(void) { return NS_OK; }
  NS_IMETHOD WillResume(void) { return NS_OK; }
  NS_IMETHOD SetParser(nsIParser* aParser) { return NS_OK; }
  virtual void FlushPendingNotifications(mozFlushType aType) { }
  NS_IMETHOD SetDocumentCharset(nsACString& aCharset) { return NS_OK; }
  virtual nsISupports *GetTarget() { return nsnull; }

private:
  nsresult UpdateChannel(nsIChannel* aChannel);

  nsresult ForwardRequest(PRBool aCallStop);
  PRBool MatchPatternList(const char*& aIter, const char* aEnd);
  void CheckHeader(const nsCString& aHeader);
  PRBool VerifyAndMatchDomainPattern(const nsACString& aDomainPattern);

  nsCOMPtr<nsIStreamListener> mOuterListener;
  nsCOMPtr<nsIRequest> mOuterRequest;
  nsCOMPtr<nsISupports> mOuterContext;
  nsCOMPtr<nsIStreamListener> mParserListener;
  nsCOMPtr<nsIParser> mParser;
  nsCOMPtr<nsIURI> mRequestingURI;
  nsCOMPtr<nsIPrincipal> mRequestingPrincipal;
  nsCOMPtr<nsIInterfaceRequestor> mOuterNotificationCallbacks;
  nsTArray<nsCString> mReqSubdomains;
  nsCString mStoredData;
  enum {
    eAccept,
    eDeny,
    eNotSet
  } mAcceptState;
  PRBool mHasForwardedRequest;
  PRBool mHasBeenCrossSite;
};
