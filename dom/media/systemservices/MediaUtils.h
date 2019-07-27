





#ifndef mozilla_MediaUtils_h
#define mozilla_MediaUtils_h

#include "nsAutoPtr.h"

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
  explicit Pledge();

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
private:
  bool mDone;
  nsresult mResult;
  nsAutoPtr<FunctorsBase> mFunctors;
};

}
}

#endif 
