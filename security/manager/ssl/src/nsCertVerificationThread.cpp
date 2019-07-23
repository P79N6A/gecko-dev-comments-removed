




































#include "nsMemory.h"
#include "nsAutoLock.h"
#include "nsAutoPtr.h"
#include "nsCertVerificationThread.h"

nsCertVerificationThread *nsCertVerificationThread::verification_thread_singleton;

NS_IMPL_THREADSAFE_ISUPPORTS1(nsCertVerificationResult, nsICertVerificationResult)

void nsCertVerificationJob::Run()
{
  if (!mListener || !mCert)
    return;

  PRUint32 verified;
  PRUint32 count;
  PRUnichar **usages;

  nsCOMPtr<nsICertVerificationResult> ires;
  nsRefPtr<nsCertVerificationResult> vres = new nsCertVerificationResult;
  if (vres)
  {
    nsresult rv = mCert->GetUsagesArray(PR_FALSE, 
                                        &verified,
                                        &count,
                                        &usages);
    vres->mRV = rv;
    if (NS_SUCCEEDED(rv))
    {
      vres->mVerified = verified;
      vres->mCount = count;
      vres->mUsages = usages;
    }

    ires = vres;
  }
  
  nsCOMPtr<nsIX509Cert3> c3 = do_QueryInterface(mCert);
  mListener->Notify(c3, ires);
}

void nsSMimeVerificationJob::Run()
{
  if (!mMessage || !mListener)
    return;
  
  nsresult rv;
  
  if (digest_data)
    rv = mMessage->VerifyDetachedSignature(digest_data, digest_len);
  else
    rv = mMessage->VerifySignature();
  
  nsCOMPtr<nsICMSMessage2> m2 = do_QueryInterface(mMessage);
  mListener->Notify(m2, rv);
}

nsCertVerificationThread::nsCertVerificationThread()
: mJobQ(nsnull)
{
  NS_ASSERTION(!verification_thread_singleton, 
               "nsCertVerificationThread is a singleton, caller attempts"
               " to create another instance!");
  
  verification_thread_singleton = this;
}

nsCertVerificationThread::~nsCertVerificationThread()
{
  verification_thread_singleton = nsnull;
}

nsresult nsCertVerificationThread::addJob(nsBaseVerificationJob *aJob)
{
  if (!aJob || !verification_thread_singleton)
    return NS_ERROR_FAILURE;
  
  if (!verification_thread_singleton->mThreadHandle)
    return NS_ERROR_OUT_OF_MEMORY;

  nsAutoLock threadLock(verification_thread_singleton->mMutex);

  verification_thread_singleton->mJobQ.Push(aJob);
  PR_NotifyAllCondVar(verification_thread_singleton->mCond);
  
  return NS_OK;
}

#define CONDITION_WAIT_TIME PR_TicksPerSecond() / 4
  
void nsCertVerificationThread::Run(void)
{
  const PRIntervalTime wait_time = CONDITION_WAIT_TIME;

  while (PR_TRUE) {

    nsBaseVerificationJob *job = nsnull;

    {
      nsAutoLock threadLock(verification_thread_singleton->mMutex);
      
      while (!mExitRequested && (0 == verification_thread_singleton->mJobQ.GetSize())) {
        

        PR_WaitCondVar(mCond, wait_time);
      }
      
      if (mExitRequested)
        break;
      
      job = NS_STATIC_CAST(nsBaseVerificationJob*, mJobQ.PopFront());
    }

    if (job)
    {
      job->Run();
      delete job;
    }
  }
  
  {
    nsAutoLock threadLock(verification_thread_singleton->mMutex);

    while (verification_thread_singleton->mJobQ.GetSize()) {
      nsCertVerificationJob *job = 
        NS_STATIC_CAST(nsCertVerificationJob*, mJobQ.PopFront());
      delete job;
    }
  }
}

nsCertVerificationResult::nsCertVerificationResult()
: mRV(0),
  mVerified(0),
  mCount(0),
  mUsages(0)
{
}

nsCertVerificationResult::~nsCertVerificationResult()
{
  if (mUsages)
  {
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(mCount, mUsages);
  }
}

NS_IMETHODIMP
nsCertVerificationResult::GetUsagesArrayResult(PRUint32 *aVerified,
                                               PRUint32 *aCount,
                                               PRUnichar ***aUsages)
{
  if (NS_FAILED(mRV))
    return mRV;
  
  
  
  *aVerified = mVerified;
  *aCount = mCount;
  *aUsages = mUsages;
  
  mVerified = 0;
  mCount = 0;
  mUsages = 0;
  
  nsresult rv = mRV;
  
  mRV = NS_ERROR_FAILURE; 
  
  return rv;
}
