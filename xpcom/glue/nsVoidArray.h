



































#ifndef nsVoidArray_h___
#define nsVoidArray_h___



#include "nscore.h"
#include "nsStringGlue.h"
#include "nsDebug.h"


typedef int (* nsVoidArrayComparatorFunc)
            (const void* aElement1, const void* aElement2, void* aData);


typedef bool (* nsVoidArrayEnumFunc)(void* aElement, void *aData);


class NS_COM_GLUE nsVoidArray {
public:
  nsVoidArray();
  nsVoidArray(PRInt32 aCount);  
  ~nsVoidArray();

  nsVoidArray& operator=(const nsVoidArray& other);

  inline PRInt32 Count() const {
    return mImpl ? mImpl->mCount : 0;
  }
  
  bool SetCount(PRInt32 aNewCount);
  
  inline PRInt32 GetArraySize() const {
    return mImpl ? (PRInt32(mImpl->mBits) & kArraySizeMask) : 0;
  }

  void* FastElementAt(PRInt32 aIndex) const
  {
    NS_ASSERTION(0 <= aIndex && aIndex < Count(), "nsVoidArray::FastElementAt: index out of range");
    return mImpl->mArray[aIndex];
  }

  
  
  
  void* ElementAt(PRInt32 aIndex) const
  {
    NS_ASSERTION(0 <= aIndex && aIndex < Count(), "nsVoidArray::ElementAt: index out of range");
    return SafeElementAt(aIndex);
  }

  
  void* SafeElementAt(PRInt32 aIndex) const
  {
    if (PRUint32(aIndex) >= PRUint32(Count())) 
    {
      return nsnull;
    }
    
    return mImpl->mArray[aIndex];
  }

  void* operator[](PRInt32 aIndex) const { return ElementAt(aIndex); }

  PRInt32 IndexOf(void* aPossibleElement) const;

  bool InsertElementAt(void* aElement, PRInt32 aIndex);
  bool InsertElementsAt(const nsVoidArray &other, PRInt32 aIndex);

  bool ReplaceElementAt(void* aElement, PRInt32 aIndex);

  
  bool MoveElement(PRInt32 aFrom, PRInt32 aTo);

  bool AppendElement(void* aElement) {
    return InsertElementAt(aElement, Count());
  }

  bool AppendElements(nsVoidArray& aElements) {
    return InsertElementsAt(aElements, Count());
  }

  bool RemoveElement(void* aElement);
  bool RemoveElementsAt(PRInt32 aIndex, PRInt32 aCount);
  bool RemoveElementAt(PRInt32 aIndex) { return RemoveElementsAt(aIndex,1); }

  void   Clear();

  bool SizeTo(PRInt32 aMin);
  
  
  void Compact();

  void Sort(nsVoidArrayComparatorFunc aFunc, void* aData);

  bool EnumerateForwards(nsVoidArrayEnumFunc aFunc, void* aData);
  bool EnumerateBackwards(nsVoidArrayEnumFunc aFunc, void* aData);

protected:
  bool GrowArrayBy(PRInt32 aGrowBy);

  struct Impl {
    





    PRUint32 mBits;

    


    PRInt32 mCount;

    


    void*   mArray[1];
  };

  Impl* mImpl;
#if DEBUG_VOIDARRAY
  PRInt32 mMaxCount;
  PRInt32 mMaxSize;
  bool    mIsAuto;
#endif

  enum {
    kArrayOwnerMask = 1 << 31,
    kArrayHasAutoBufferMask = 1 << 30,
    kArraySizeMask = ~(kArrayOwnerMask | kArrayHasAutoBufferMask)
  };
  enum { kAutoBufSize = 8 };


  
  void SetArray(Impl *newImpl, PRInt32 aSize, PRInt32 aCount, bool aOwner,
                bool aHasAuto);
  inline bool IsArrayOwner() const {
    return mImpl && (mImpl->mBits & kArrayOwnerMask);
  }
  inline bool HasAutoBuffer() const {
    return mImpl && (mImpl->mBits & kArrayHasAutoBufferMask);
  }

private:
  
  nsVoidArray(const nsVoidArray& other);
};



class NS_COM_GLUE nsAutoVoidArray : public nsVoidArray {
public:
  nsAutoVoidArray();

  void ResetToAutoBuffer()
  {
    SetArray(reinterpret_cast<Impl*>(mAutoBuf), kAutoBufSize, 0, false,
             true);
  }

  nsAutoVoidArray& operator=(const nsVoidArray& other)
  {
    nsVoidArray::operator=(other);
    return *this;
  }
  
protected:
  
  char mAutoBuf[sizeof(Impl) + (kAutoBufSize - 1) * sizeof(void*)];
};


class nsCString;

typedef int (* nsCStringArrayComparatorFunc)
            (const nsCString* aElement1, const nsCString* aElement2, void* aData);

typedef bool (*nsCStringArrayEnumFunc)(nsCString& aElement, void *aData);

class NS_COM_GLUE nsCStringArray: private nsVoidArray
{
public:
  nsCStringArray(void);
  nsCStringArray(PRInt32 aCount); 
  ~nsCStringArray(void);

  nsCStringArray& operator=(const nsCStringArray& other);

  PRInt32 Count(void) const {
    return nsVoidArray::Count();
  }

  void CStringAt(PRInt32 aIndex, nsACString& aCString) const;
  nsCString* CStringAt(PRInt32 aIndex) const;
  nsCString* operator[](PRInt32 aIndex) const { return CStringAt(aIndex); }

  PRInt32 IndexOf(const nsACString& aPossibleString) const;

#ifdef MOZILLA_INTERNAL_API
  PRInt32 IndexOfIgnoreCase(const nsACString& aPossibleString) const;
#endif

  bool InsertCStringAt(const nsACString& aCString, PRInt32 aIndex);

  bool ReplaceCStringAt(const nsACString& aCString, PRInt32 aIndex);

  bool AppendCString(const nsACString& aCString) {
    return InsertCStringAt(aCString, Count());
  }

  bool RemoveCString(const nsACString& aCString);

#ifdef MOZILLA_INTERNAL_API
  bool RemoveCStringIgnoreCase(const nsACString& aCString);
#endif

  bool RemoveCStringAt(PRInt32 aIndex);
  void   Clear(void);

  void Compact(void) {
    nsVoidArray::Compact();
  }

  void Sort(void);

#ifdef MOZILLA_INTERNAL_API
  void SortIgnoreCase(void);
#endif

  void Sort(nsCStringArrayComparatorFunc aFunc, void* aData);

  bool EnumerateForwards(nsCStringArrayEnumFunc aFunc, void* aData);
  bool EnumerateBackwards(nsCStringArrayEnumFunc aFunc, void* aData);

private:
  
  nsCStringArray(const nsCStringArray& other);
};


























class NS_COM_GLUE nsSmallVoidArray : private nsVoidArray
{
public:
  ~nsSmallVoidArray();

  nsSmallVoidArray& operator=(nsSmallVoidArray& other);
  void* operator[](PRInt32 aIndex) const { return ElementAt(aIndex); }

  PRInt32 GetArraySize() const;

  PRInt32 Count() const;
  void* FastElementAt(PRInt32 aIndex) const;
  
  
  
  void* ElementAt(PRInt32 aIndex) const
  {
    NS_ASSERTION(0 <= aIndex && aIndex < Count(), "nsSmallVoidArray::ElementAt: index out of range");
    return SafeElementAt(aIndex);
  }
  void* SafeElementAt(PRInt32 aIndex) const {
    
    if (PRUint32(aIndex) >= PRUint32(Count())) 
    {
      return nsnull;
    }
    return FastElementAt(aIndex);
  }
  PRInt32 IndexOf(void* aPossibleElement) const;
  bool InsertElementAt(void* aElement, PRInt32 aIndex);
  bool InsertElementsAt(const nsVoidArray &other, PRInt32 aIndex);
  bool ReplaceElementAt(void* aElement, PRInt32 aIndex);
  bool MoveElement(PRInt32 aFrom, PRInt32 aTo);
  bool AppendElement(void* aElement);
  bool AppendElements(nsVoidArray& aElements) {
    return InsertElementsAt(aElements, Count());
  }
  bool RemoveElement(void* aElement);
  bool RemoveElementsAt(PRInt32 aIndex, PRInt32 aCount);
  bool RemoveElementAt(PRInt32 aIndex);

  void Clear();
  bool SizeTo(PRInt32 aMin);
  void Compact();
  void Sort(nsVoidArrayComparatorFunc aFunc, void* aData);

  bool EnumerateForwards(nsVoidArrayEnumFunc aFunc, void* aData);
  bool EnumerateBackwards(nsVoidArrayEnumFunc aFunc, void* aData);

private:

  bool HasSingle() const
  {
    return !!(reinterpret_cast<PRWord>(mImpl) & 0x1);
  }
  void* GetSingle() const
  {
    NS_ASSERTION(HasSingle(), "wrong type");
    return reinterpret_cast<void*>
                           (reinterpret_cast<PRWord>(mImpl) & ~0x1);
  }
  void SetSingle(void *aChild)
  {
    NS_ASSERTION(HasSingle() || !mImpl, "overwriting array");
    mImpl = reinterpret_cast<Impl*>
                            (reinterpret_cast<PRWord>(aChild) | 0x1);
  }
  bool IsEmpty() const
  {
    
    return !mImpl;
  }
  const nsVoidArray* AsArray() const
  {
    NS_ASSERTION(!HasSingle(), "This is a single");
    return this;
  }
  nsVoidArray* AsArray()
  {
    NS_ASSERTION(!HasSingle(), "This is a single");
    return this;
  }
  bool EnsureArray();
};

#endif 
