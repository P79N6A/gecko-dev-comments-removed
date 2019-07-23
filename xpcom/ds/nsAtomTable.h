




































#ifndef nsAtomTable_h__
#define nsAtomTable_h__

#include "nsIAtom.h"







class AtomImpl : public nsIAtom {
public:
  AtomImpl();

protected:
  
  
  ~AtomImpl();

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIATOM

  enum { REFCNT_PERMANENT_SENTINEL = PR_UINT32_MAX };

  virtual PRBool IsPermanent();

  
  PRBool IsPermanentInDestructor() {
    return mRefCnt == REFCNT_PERMANENT_SENTINEL;
  }

  void* operator new(size_t size, const nsACString& aString) CPP_THROW_NEW;

  void operator delete(void* ptr) {
    ::operator delete(ptr);
  }

  
  nsrefcnt GetRefCount() { return mRefCnt; }

  
  
  char mString[1];
};





class PermanentAtomImpl : public AtomImpl {
public:
  PermanentAtomImpl();
  ~PermanentAtomImpl();
  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();

  virtual PRBool IsPermanent();

  void* operator new(size_t size, const nsACString& aString) CPP_THROW_NEW {
    return AtomImpl::operator new(size, aString);
  }
  void* operator new(size_t size, AtomImpl* aAtom) CPP_THROW_NEW;

};

void NS_PurgeAtomTable();

#endif
