






































#ifndef _INC_NSVERIFICATIONJOB_H
#define _INC_NSVERIFICATIONJOB_H

#include "nspr.h"

#include "nsIX509Cert.h"
#include "nsIX509Cert3.h"
#include "nsICMSMessage.h"
#include "nsICMSMessage2.h"

class nsBaseVerificationJob
{
public:
  virtual ~nsBaseVerificationJob() {}
  virtual void Run() = 0;
};

class nsCertVerificationJob : public nsBaseVerificationJob
{
public:
  nsCOMPtr<nsIX509Cert> mCert;
  nsCOMPtr<nsICertVerificationListener> mListener;

  void Run();
};

class nsCertVerificationResult : public nsICertVerificationResult
{
public:
  nsCertVerificationResult();
  virtual ~nsCertVerificationResult();

  NS_DECL_ISUPPORTS
  NS_DECL_NSICERTVERIFICATIONRESULT

private:
  nsresult mRV;
  PRUint32 mVerified;
  PRUint32 mCount;
  PRUnichar **mUsages;

friend class nsCertVerificationJob;
};

class nsSMimeVerificationJob : public nsBaseVerificationJob
{
public:
  nsSMimeVerificationJob() { digest_data = nsnull; digest_len = 0; }
  ~nsSMimeVerificationJob() { delete [] digest_data; }

  nsCOMPtr<nsICMSMessage> mMessage;
  nsCOMPtr<nsISMimeVerificationListener> mListener;

  unsigned char *digest_data;
  PRUint32 digest_len;

  void Run();
};



#endif
