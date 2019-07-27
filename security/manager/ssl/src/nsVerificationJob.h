





#ifndef _INC_NSVERIFICATIONJOB_H
#define _INC_NSVERIFICATIONJOB_H

#include "nspr.h"

#include "nsIX509Cert.h"
#include "nsProxyRelease.h"

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
  nsMainThreadPtrHandle<nsICertVerificationListener> mListener;

  void Run();
};

class nsCertVerificationResult : public nsICertVerificationResult
{
public:
  nsCertVerificationResult();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSICERTVERIFICATIONRESULT

protected:
  virtual ~nsCertVerificationResult();

private:
  nsresult mRV;
  uint32_t mVerified;
  uint32_t mCount;
  char16_t **mUsages;

friend class nsCertVerificationJob;
};

#endif
