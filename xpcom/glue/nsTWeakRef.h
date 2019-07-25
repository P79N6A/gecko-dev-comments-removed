





#ifndef nsTWeakRef_h__
#define nsTWeakRef_h__

#ifndef nsDebug_h___
#include "nsDebug.h"
#endif

















































template <class Type>
class nsTWeakRef {
public:
  ~nsTWeakRef() {
    if (mRef)
      mRef->Release();
  }

  


  explicit
  nsTWeakRef(Type *obj = nullptr) {
    if (obj) {
      mRef = new Inner(obj);
    } else {
      mRef = nullptr;
    }
  }

  


  explicit
  nsTWeakRef(const nsTWeakRef<Type> &other) : mRef(other.mRef) {
    if (mRef)
      mRef->AddRef();
  }

  


  nsTWeakRef<Type> &operator=(Type *obj) {
    if (mRef)  
      mRef->Release();
    if (obj) {
      mRef = new Inner(obj);
    } else {
      mRef = nullptr;
    }
    return *this;
  }

  

 
  nsTWeakRef<Type> &operator=(const nsTWeakRef<Type> &other) {
    if (mRef)  
      mRef->Release();
    mRef = other.mRef;
    if (mRef)
      mRef->AddRef();
    return *this;
  }

  



  Type *get() const {
    return mRef ? mRef->mObj : nullptr;
  }

  




  Type *forget() {
    Type *obj;
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

  


  operator Type *() const {
    return get();
  }

  



  Type *operator->() const {
    NS_ASSERTION(mRef && mRef->mObj,
        "You can't dereference a null weak reference with operator->().");
    return get();
  }

private:

  struct Inner {
    int     mCnt;
    Type   *mObj;

    Inner(Type *obj) : mCnt(1), mObj(obj) {}
    void AddRef() { ++mCnt; }
    void Release() { if (--mCnt == 0) delete this; }
  };

  Inner *mRef;
};

#endif  
