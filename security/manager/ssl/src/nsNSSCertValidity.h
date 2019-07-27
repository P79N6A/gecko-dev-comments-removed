



#ifndef _NSX509CERTVALIDITY_H_
#define _NSX509CERTVALIDITY_H_

#include "nsIX509CertValidity.h"

#include "certt.h"

class nsX509CertValidity : public nsIX509CertValidity
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIX509CERTVALIDITY

  nsX509CertValidity();
  explicit nsX509CertValidity(CERTCertificate *cert);

protected:
  virtual ~nsX509CertValidity();
  

private:
  PRTime mNotBefore, mNotAfter;
  bool mTimesInitialized;
};

#endif
