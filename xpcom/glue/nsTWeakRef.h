





#ifndef nsTWeakRef_h__
#define nsTWeakRef_h__

#ifndef nsDebug_h___
#include "nsDebug.h"
#endif

















































template<class Type>
class nsTWeakRef
{
public:
  ~nsTWeakRef()
  {
    if (mRef) {
      mRef->Release();
    }
  }

  


  explicit nsTWeakRef(Type* aObj = nullptr)
  {
    if (aObj) {
      mRef = new Inner(aObj);
    } else {
      mRef = nullptr;
    }
  }

  


  explicit nsTWeakRef(const nsTWeakRef<Type>& aOther) : mRef(aOther.mRef)
  {
    if (mRef) {
      mRef->AddRef();
    }
  }

  


  nsTWeakRef<Type>& operator=(Type* aObj)
  {
    if (mRef) {
      mRef->Release();
    }
    if (aObj) {
      mRef = new Inner(aObj);
    } else {
      mRef = nullptr;
    }
    return *this;
  }

  


  nsTWeakRef<Type>& operator=(const nsTWeakRef<Type>& aOther)
  {
    if (mRef) {
      mRef->Release();
    }
    mRef = aOther.mRef;
    if (mRef) {
      mRef->AddRef();
    }
    return *this;
  }

  



  Type* get() const { return mRef ? mRef->mObj : nullptr; }

  




  Type* forget()
  {
    Type* obj;
    if (mRef) {
      obj = mRef->mObj;
      mRef->mObj = nullptr;
      mRef->Release();
      mRef = nullptr;
    } else {
      obj = nullptr;
    }
    return obj;
  }

  


  operator Type*() const { return get(); }

  



  Type* operator->() const
  {
    NS_ASSERTION(mRef && mRef->mObj,
                 "You can't dereference a null weak reference with operator->().");
    return get();
  }

private:

  struct Inner
  {
    int     mCnt;
    Type*   mObj;

    explicit Inner(Type* aObj)
      : mCnt(1)
      , mObj(aObj)
    {
    }
    void AddRef()
    {
      ++mCnt;
    }
    void Release()
    {
      if (--mCnt == 0) {
        delete this;
      }
    }
  };

  Inner* mRef;
};

#endif  
