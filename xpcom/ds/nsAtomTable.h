




































#ifndef nsAtomTable_h__
#define nsAtomTable_h__

#include "nsIAtom.h"
#include "nsStringBuffer.h"






class AtomImpl : public nsIAtom {
public:
  AtomImpl(const nsACString& aString);

  
  
  AtomImpl(nsStringBuffer* aData, PRUint32 aLength);

protected:
  
  
  AtomImpl() {
    
    
    NS_ASSERTION((mLength + 1) * sizeof(char) <=
                 nsStringBuffer::FromData(mString)->StorageSize() &&
                 mString[mLength] == 0,
                 "Not initialized atom");
  }

  
  
  ~AtomImpl();

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIATOM

  enum { REFCNT_PERMANENT_SENTINEL = PR_UINT32_MAX };

  virtual PRBool IsPermanent();

  
  PRBool IsPermanentInDestructor() {
    return mRefCnt == REFCNT_PERMANENT_SENTINEL;
  }

  
  nsrefcnt GetRefCount() { return mRefCnt; }

  
  PRUint32 mLength;

  
  char* mString;
};





class PermanentAtomImpl : public AtomImpl {
public:
  PermanentAtomImpl(const nsACString& aString)
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

  virtual PRBool IsPermanent();

  void* operator new(size_t size, AtomImpl* aAtom) CPP_THROW_NEW;
  void* operator new(size_t size) CPP_THROW_NEW
  {
    return ::operator new(size);
  }

};

void NS_PurgeAtomTable();

#endif
