





































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
  nsTWeakRef(Type *obj = nsnull) {
    if (obj) {
      mRef = new Inner(obj);
    } else {
      mRef = nsnull;
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
      mRef = nsnull;
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
    return mRef ? mRef->mObj : nsnull;
  }

  




  Type *forget() {
    Type *obj;
    if (mRef) {
      obj = mRef->mObj;
      mRef->mObj = nsnull;
      mRef->Release();
      mRef = nsnull;
    } else {
      obj = nsnull;
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
