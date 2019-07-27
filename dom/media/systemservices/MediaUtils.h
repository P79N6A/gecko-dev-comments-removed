





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
  NS_INLINE_DECL_REFCOUNTING(Pledge);
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

  void Resolve(const ValueType& aValue)
  {
    mValue = aValue;
    Resolve();
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
protected:
  ~Pledge() {};
  bool mDone;
  nsresult mResult;
private:
  nsAutoPtr<FunctorsBase> mFunctors;
};







































template<typename OnRunType>
class LambdaRunnable : public nsRunnable
{
public:
  explicit LambdaRunnable(OnRunType& aOnRun) : mOnRun(aOnRun) {}
private:
  NS_IMETHODIMP
  Run()
  {
    return mOnRun();
  }
  OnRunType mOnRun;
};

template<typename OnRunType>
LambdaRunnable<OnRunType>*
NewRunnableFrom(OnRunType aOnRun)
{
  return new LambdaRunnable<OnRunType>(aOnRun);
}































































template<class T>
class CoatCheck
{
public:
  typedef std::pair<uint32_t, nsRefPtr<T>> Element;

  uint32_t Append(T& t)
  {
    uint32_t id = GetNextId();
    mElements.AppendElement(Element(id, nsRefPtr<T>(&t)));
    return id;
  }

  already_AddRefed<T> Remove(uint32_t aId)
  {
    for (auto& element : mElements) {
      if (element.first == aId) {
        nsRefPtr<T> ref;
        ref.swap(element.second);
        mElements.RemoveElement(element);
        return ref.forget();
      }
    }
    MOZ_ASSERT_UNREACHABLE("Received id with no matching parked object!");
    return nullptr;
  }

private:
  static uint32_t GetNextId()
  {
    static uint32_t counter = 0;
    return ++counter;
  };
  nsAutoTArray<Element, 3> mElements;
};

}
}

#endif
