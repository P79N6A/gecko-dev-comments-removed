





#ifndef mozilla_MediaUtils_h
#define mozilla_MediaUtils_h

#include "nsAutoPtr.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace media {











template<typename ValueType>
class Pledge
{
  
  class FunctorsBase
  {
  public:
    FunctorsBase() {}
    virtual void Succeed(const ValueType& result) = 0;
    virtual void Fail(nsresult rv) = 0;
    virtual ~FunctorsBase() {};
  };

public:
  explicit Pledge() : mDone(false), mResult(NS_OK) {}

  template<typename OnSuccessType>
  void Then(OnSuccessType aOnSuccess)
  {
    Then(aOnSuccess, [](nsresult){});
  }

  template<typename OnSuccessType, typename OnFailureType>
  void Then(OnSuccessType aOnSuccess, OnFailureType aOnFailure)
  {
    class F : public FunctorsBase
    {
    public:
      F(OnSuccessType& aOnSuccess, OnFailureType& aOnFailure)
        : mOnSuccess(aOnSuccess), mOnFailure(aOnFailure) {}

      void Succeed(const ValueType& result)
      {
        mOnSuccess(result);
      }
      void Fail(nsresult rv)
      {
        mOnFailure(rv);
      };

      OnSuccessType mOnSuccess;
      OnFailureType mOnFailure;
    };
    mFunctors = new F(aOnSuccess, aOnFailure);

    if (mDone) {
      if (mResult == NS_OK) {
        mFunctors->Succeed(mValue);
      } else {
        mFunctors->Fail(mResult);
      }
    }
  }

protected:
  void Resolve(const ValueType& aValue)
  {
    mValue = aValue;
    Resolve();
  }

  void Resolve()
  {
    if (!mDone) {
      mDone = true;
      MOZ_ASSERT(mResult == NS_OK);
      if (mFunctors) {
        mFunctors->Succeed(mValue);
      }
    }
  }

  void Reject(nsresult rv)
  {
    if (!mDone) {
      mDone = true;
      mResult = rv;
      if (mFunctors) {
        mFunctors->Fail(mResult);
      }
    }
  }

  ValueType mValue;
protected:
  bool mDone;
  nsresult mResult;
private:
  nsAutoPtr<FunctorsBase> mFunctors;
};




template<typename ValueType>
class PledgeRunnable : public Pledge<ValueType>, public nsRunnable
{
public:
  template<typename OnRunType>
  static PledgeRunnable<ValueType>*
  New(OnRunType aOnRun)
  {
    class P : public PledgeRunnable<ValueType>
    {
    public:
      explicit P(OnRunType& aOnRun)
      : mOriginThread(NS_GetCurrentThread())
      , mOnRun(aOnRun)
      , mHasRun(false) {}
    private:
      virtual ~P() {}
      NS_IMETHODIMP
      Run()
      {
        if (!mHasRun) {
          P::mResult = mOnRun(P::mValue);
          mHasRun = true;
          return mOriginThread->Dispatch(this, NS_DISPATCH_NORMAL);
        }
        bool on;
        MOZ_RELEASE_ASSERT(NS_SUCCEEDED(mOriginThread->IsOnCurrentThread(&on)));
        MOZ_RELEASE_ASSERT(on);

        if (NS_SUCCEEDED(P::mResult)) {
          P::Resolve();
        } else {
          P::Reject(P::mResult);
        }
        return NS_OK;
      }
      nsCOMPtr<nsIThread> mOriginThread;
      OnRunType mOnRun;
      bool mHasRun;
    };

    return new P(aOnRun);
  }

protected:
  virtual ~PledgeRunnable() {}
};



namespace CallbackRunnable
{
template<typename OnRunType>
class Impl : public nsRunnable
{
public:
  explicit Impl(OnRunType& aOnRun) : mOnRun(aOnRun) {}
private:
  NS_IMETHODIMP
  Run()
  {
    return mOnRun();
  }
  OnRunType mOnRun;
};

template<typename OnRunType>
Impl<OnRunType>*
New(OnRunType aOnRun)
{
  return new Impl<OnRunType>(aOnRun);
}
}

}
}

#endif 
