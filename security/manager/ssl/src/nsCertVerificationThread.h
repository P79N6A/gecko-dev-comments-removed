




































#ifndef _NSCERTVERIFICATIONTHREAD_H_
#define _NSCERTVERIFICATIONTHREAD_H_

#include "nsCOMPtr.h"
#include "nsDeque.h"
#include "nsPSMBackgroundThread.h"
#include "nsVerificationJob.h"

class nsCertVerificationThread : public nsPSMBackgroundThread
{
private:
  nsDeque mJobQ;

  virtual void Run(void);

public:
  nsCertVerificationThread();
  ~nsCertVerificationThread();

  static nsCertVerificationThread *verification_thread_singleton;
  
  static nsresult addJob(nsBaseVerificationJob *aJob);
};

#endif
