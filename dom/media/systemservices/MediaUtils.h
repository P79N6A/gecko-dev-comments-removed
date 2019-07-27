





#ifndef mozilla_MediaUtils_h
#define mozilla_MediaUtils_h

#include "nsAutoPtr.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace media {




























class PledgeBase
{
public:
  NS_INLINE_DECL_REFCOUNTING(PledgeBase);
protected:
  virtual ~PledgeBase() {};
};

template<typename ValueType, typename ErrorType = nsresult>
class Pledge : public PledgeBase
{
  
  
  class FunctorsBase
  {
  public:
    FunctorsBase() {}
    virtual void Succeed(ValueType& result) = 0;
    virtual void Fail(ErrorType& error) = 0;
    virtual ~FunctorsBase() {};
  };

public:
  explicit Pledge() : mDone(false), mError(nullptr) {}
  Pledge(const Pledge& aOther) = delete;
  Pledge& operator = (const Pledge&) = delete;

  template<typename OnSuccessType>
  void Then(OnSuccessType aOnSuccess)
  {
    Then(aOnSuccess, [](ErrorType&){});
  }

  template<typename OnSuccessType, typename OnFailureType>
  void Then(OnSuccessType aOnSuccess, OnFailureType aOnFailure)
  {
    class Functors : public FunctorsBase
    {
    public:
      Functors(OnSuccessType& aOnSuccess, OnFailureType& aOnFailure)
        : mOnSuccess(aOnSuccess), mOnFailure(aOnFailure) {}

      void Succeed(ValueType& result)
      {
        mOnSuccess(result);
      }
      void Fail(ErrorType& error)
      {
        mOnFailure(error);
      };

      OnSuccessType mOnSuccess;
      OnFailureType mOnFailure;
    };
    mFunctors = new Functors(aOnSuccess, aOnFailure);

    if (mDone) {
      if (!mError) {
        mFunctors->Succeed(mValue);
      } else {
        mFunctors->Fail(*mError);
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
      MOZ_ASSERT(!mError);
      if (mFunctors) {
        mFunctors->Succeed(mValue);
      }
    }
  }

  void Reject(ErrorType rv)
  {
    if (!mDone) {
      mDone = true;
      mError = rv;
      if (mFunctors) {
        mFunctors->Fail(mError);
      }
    }
  }

  ValueType mValue;
private:
  ~Pledge() {};
  bool mDone;
  nsRefPtr<ErrorType> mError;
  ScopedDeletePtr<FunctorsBase> mFunctors;
};

template<typename ValueType>
class Pledge<ValueType, nsresult>  : public PledgeBase
{
  
  
  class FunctorsBase
  {
  public:
    FunctorsBase() {}
    virtual void Succeed(ValueType& result) = 0;
    virtual void Fail(nsresult error) = 0;
    virtual ~FunctorsBase() {};
  };

public:
  explicit Pledge() : mDone(false), mError(NS_OK) {}
  Pledge(const Pledge& aOther) = delete;
  Pledge& operator = (const Pledge&) = delete;

  template<typename OnSuccessType>
  void Then(OnSuccessType aOnSuccess)
  {
    Then(aOnSuccess, [](nsresult){});
  }

  template<typename OnSuccessType, typename OnFailureType>
  void Then(OnSuccessType aOnSuccess, OnFailureType aOnFailure)
  {
    class Functors : public FunctorsBase
    {
    public:
      Functors(OnSuccessType& aOnSuccess, OnFailureType& aOnFailure)
        : mOnSuccess(aOnSuccess), mOnFailure(aOnFailure) {}

      void Succeed(ValueType& result)
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
    mFunctors = new Functors(aOnSuccess, aOnFailure);

    if (mDone) {
      if (mError == NS_OK) {
        mFunctors->Succeed(mValue);
      } else {
        mFunctors->Fail(mError);
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
      MOZ_ASSERT(mError == NS_OK);
      if (mFunctors) {
        mFunctors->Succeed(mValue);
      }
    }
  }

  void Reject(nsresult error)
  {
    if (!mDone) {
      mDone = true;
      mError = error;
      if (mFunctors) {
        mFunctors->Fail(mError);
      }
    }
  }

  ValueType mValue;
private:
  ~Pledge() {};
  bool mDone;
  nsresult mError;
  ScopedDeletePtr<FunctorsBase> mFunctors;
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

template<typename OnRunType>
class LambdaTask : public Task
{
public:
  explicit LambdaTask(OnRunType& aOnRun) : mOnRun(aOnRun) {}
private:
  void
  Run()
  {
    return mOnRun();
  }
  OnRunType mOnRun;
};

template<typename OnRunType>
LambdaTask<OnRunType>*
NewTaskFrom(OnRunType aOnRun)
{
  return new LambdaTask<OnRunType>(aOnRun);
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
