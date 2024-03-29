





#ifndef mozilla__CryptoTask_h
#define mozilla__CryptoTask_h

#include "mozilla/Attributes.h"
#include "nsThreadUtils.h"
#include "nsNSSShutDown.h"

namespace mozilla {





























class CryptoTask : public nsRunnable,
                   public nsNSSShutDownObject
{
public:
  template <size_t LEN>
  nsresult Dispatch(const char (&taskThreadName)[LEN])
  {
    static_assert(LEN <= 15,
                  "Thread name must be no more than 15 characters");
    return Dispatch(nsDependentCString(taskThreadName, LEN - 1));
  }

  nsresult Dispatch(const nsACString& taskThreadName);

  void Skip()
  {
    virtualDestroyNSSReference();
  }

protected:
  CryptoTask()
    : mRv(NS_ERROR_NOT_INITIALIZED),
      mReleasedNSSResources(false)
  {
  }

  virtual ~CryptoTask();

  




  virtual nsresult CalculateResult() = 0;

  




  virtual void ReleaseNSSResources() = 0;

  




  virtual void CallCallback(nsresult rv) = 0;

private:
  NS_IMETHOD Run() override final;
  virtual void virtualDestroyNSSReference() override final;

  nsresult mRv;
  bool mReleasedNSSResources;

  nsCOMPtr<nsIThread> mThread;
};

} 

#endif 
