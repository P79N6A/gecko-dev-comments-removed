




































#ifndef nsAtomTable_h__
#define nsAtomTable_h__

#include "nsIAtom.h"
#include "nsStringBuffer.h"






class AtomImpl : public nsIAtom {
public:
  AtomImpl(const nsAString& aString);

  
  
  AtomImpl(nsStringBuffer* aData, PRUint32 aLength);

protected:
  
  
  AtomImpl() {
    
    
    NS_ASSERTION((mLength + 1) * sizeof(PRUnichar) <=
                 nsStringBuffer::FromData(mString)->StorageSize() &&
                 mString[mLength] == 0,
                 "Not initialized atom");
  }

  
  
  ~AtomImpl();

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIATOM

  enum { REFCNT_PERMANENT_SENTINEL = PR_UINT32_MAX };

  virtual bool IsPermanent();

  
  bool IsPermanentInDestructor() {
    return mRefCnt == REFCNT_PERMANENT_SENTINEL;
  }

  
  nsrefcnt GetRefCount() { return mRefCnt; }
};





class PermanentAtomImpl : public AtomImpl {
public:
  PermanentAtomImpl(const nsAString& aString)
    : AtomImpl(aString)
  {}
  PermanentAtomImpl(nsStringBuffer* aData, PRUint32 aLength)
    : AtomImpl(aData, aLength)
  {}
  PermanentAtomImpl()
  {}

  ~PermanentAtomImpl();
  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();

  virtual bool IsPermanent();

  void* operator new(size_t size, AtomImpl* aAtom) CPP_THROW_NEW;
  void* operator new(size_t size) CPP_THROW_NEW
  {
    return ::operator new(size);
  }

};

void NS_PurgeAtomTable();

#endif
