




































#include "nsIStreamListener.h"
#include "nsIInterfaceRequestor.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIURI.h"
#include "nsTArray.h"
#include "nsIContentSink.h"
#include "nsIXMLContentSink.h"
#include "nsIExpatSink.h"

class nsIURI;
class nsIParser;
class nsIPrincipal;

class nsCrossSiteListenerProxy : public nsIStreamListener,
                                 public nsIXMLContentSink,
                                 public nsIExpatSink
{
public:
  nsCrossSiteListenerProxy(nsIStreamListener* aOuter,
                           nsIPrincipal* aRequestingPrincipal);
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIEXPATSINK

  
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

  nsresult ForwardRequest(PRBool aCallStop);
  PRBool MatchPatternList(const char*& aIter, const char* aEnd);
  void CheckHeader(const nsCString& aHeader);
  PRBool VerifyAndMatchDomainPattern(const nsACString& aDomainPattern);

  nsCOMPtr<nsIStreamListener> mOuter;
  nsCOMPtr<nsIRequest> mOuterRequest;
  nsCOMPtr<nsISupports> mOuterContext;
  nsCOMPtr<nsIStreamListener> mParserListener;
  nsCOMPtr<nsIParser> mParser;
  nsCOMPtr<nsIURI> mRequestingURI;
  nsTArray<nsCString> mReqSubdomains;
  nsCString mStoredData;
  enum {
    eAccept,
    eDeny,
    eNotSet
  } mAcceptState;
  PRBool mHasForwardedRequest;
};
