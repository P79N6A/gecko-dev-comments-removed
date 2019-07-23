


































#include "nsCOMPtr.h"
#include "nsIStreamListener.h"
#include "nsString.h"

class nsISignatureVerifier;
class nsIPrincipal;
class nsIURI;
class nsPICertNotification;


class CertReader : public nsIStreamListener
{
public:
  CertReader(nsIURI* uri, nsISupports* aContext, nsPICertNotification* aObs);
  virtual ~CertReader();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

private:
  nsCString mLeftoverBuffer;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<nsISignatureVerifier> mVerifier;

  nsCOMPtr<nsISupports> mContext;
  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<nsPICertNotification> mObserver;
};
