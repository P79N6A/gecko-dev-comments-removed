





































#ifndef _NSX509CERTVALIDITY_H_
#define _NSX509CERTVALIDITY_H_

#include "nsIX509CertValidity.h"

#include "certt.h"

class nsX509CertValidity : public nsIX509CertValidity
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIX509CERTVALIDITY

  nsX509CertValidity();
  nsX509CertValidity(CERTCertificate *cert);
  virtual ~nsX509CertValidity();
  

private:
  PRTime mNotBefore, mNotAfter;
  PRBool mTimesInitialized;
};

#endif
