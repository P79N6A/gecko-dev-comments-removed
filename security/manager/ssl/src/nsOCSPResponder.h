





































#ifndef __NSOCSPRESPONDER_H__
#define __NSOCSPRESPONDER_H__

#include "nsIOCSPResponder.h"
#include "nsString.h"

#include "certt.h"

class nsOCSPResponder : public nsIOCSPResponder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOCSPRESPONDER

  nsOCSPResponder();
  nsOCSPResponder(const PRUnichar*, const PRUnichar*);
  virtual ~nsOCSPResponder();
  
  static PRInt32 CmpCAName(nsIOCSPResponder *a, nsIOCSPResponder *b);
  static PRInt32 CompareEntries(nsIOCSPResponder *a, nsIOCSPResponder *b);
  static PRBool IncludeCert(CERTCertificate *aCert);
private:
  nsString mCA;
  nsString mURL;
};

#endif
