



































#ifndef nsVoidArray_h___
#define nsVoidArray_h___



#include "nscore.h"
#include "nsStringGlue.h"
#include "nsDebug.h"


typedef int (* PR_CALLBACK nsVoidArrayComparatorFunc)
            (const void* aElement1, const void* aElement2, void* aData);


typedef PRBool (* PR_CALLBACK nsVoidArrayEnumFunc)(void* aElement, void *aData);


class NS_COM_GLUE nsVoidArray {
public:
  nsVoidArray();
  nsVoidArray(PRInt32 aCount);  
  ~nsVoidArray();

  nsVoidArray& operator=(const nsVoidArray& other);

  inline PRInt32 Count() const {
    return mImpl ? mImpl->mCount : 0;
  }
  
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

  PRBool InsertElementAt(void* aElement, PRInt32 aIndex);
  PRBool InsertElementsAt(const nsVoidArray &other, PRInt32 aIndex);

  PRBool ReplaceElementAt(void* aElement, PRInt32 aIndex);

  
  PRBool MoveElement(PRInt32 aFrom, PRInt32 aTo);

  PRBool AppendElement(void* aElement) {
    return InsertElementAt(aElement, Count());
  }

  PRBool AppendElements(nsVoidArray& aElements) {
    return InsertElementsAt(aElements, Count());
  }

  PRBool RemoveElement(void* aElement);
  PRBool RemoveElementsAt(PRInt32 aIndex, PRInt32 aCount);
  PRBool RemoveElementAt(PRInt32 aIndex) { return RemoveElementsAt(aIndex,1); }

  void   Clear();

  PRBool SizeTo(PRInt32 aMin);
  
  
  void Compact();

  void Sort(nsVoidArrayComparatorFunc aFunc, void* aData);

  PRBool EnumerateForwards(nsVoidArrayEnumFunc aFunc, void* aData);
  PRBool EnumerateBackwards(nsVoidArrayEnumFunc aFunc, void* aData);

protected:
  PRBool GrowArrayBy(PRInt32 aGrowBy);

  struct Impl {
    





    PRUint32 mBits;

    


    PRInt32 mCount;

    


    void*   mArray[1];
  };

  Impl* mImpl;
#if DEBUG_VOIDARRAY
  PRInt32 mMaxCount;
  PRInt32 mMaxSize;
  PRBool  mIsAuto;
#endif

  enum {
    kArrayOwnerMask = 1 << 31,
    kArrayHasAutoBufferMask = 1 << 30,
    kArraySizeMask = ~(kArrayOwnerMask | kArrayHasAutoBufferMask)
  };
  enum { kAutoBufSize = 8 };


  
  void SetArray(Impl *newImpl, PRInt32 aSize, PRInt32 aCount, PRBool aOwner,
                PRBool aHasAuto);
  inline PRBool IsArrayOwner() const {
    return mImpl && (mImpl->mBits & kArrayOwnerMask);
  }
  inline PRBool HasAutoBuffer() const {
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
    SetArray(reinterpret_cast<Impl*>(mAutoBuf), kAutoBufSize, 0, PR_FALSE,
             PR_TRUE);
  }
  
protected:
  
  char mAutoBuf[sizeof(Impl) + (kAutoBufSize - 1) * sizeof(void*)];
};


class nsString;

typedef int (* PR_CALLBACK nsStringArrayComparatorFunc)
            (const nsString* aElement1, const nsString* aElement2, void* aData);

typedef PRBool (*nsStringArrayEnumFunc)(nsString& aElement, void *aData);

class NS_COM_GLUE nsStringArray: private nsVoidArray
{
public:
  nsStringArray(void);
  nsStringArray(PRInt32 aCount);  
  ~nsStringArray(void);

  nsStringArray& operator=(const nsStringArray& other);

  PRInt32 Count(void) const {
    return nsVoidArray::Count();
  }

  void StringAt(PRInt32 aIndex, nsAString& aString) const;
  nsString* StringAt(PRInt32 aIndex) const;
  nsString* operator[](PRInt32 aIndex) const { return StringAt(aIndex); }

  PRInt32 IndexOf(const nsAString& aPossibleString) const;

  PRBool InsertStringAt(const nsAString& aString, PRInt32 aIndex);

  PRBool ReplaceStringAt(const nsAString& aString, PRInt32 aIndex);

  PRBool AppendString(const nsAString& aString) {
    return InsertStringAt(aString, Count());
  }

  PRBool RemoveString(const nsAString& aString);
  PRBool RemoveStringAt(PRInt32 aIndex);
  void   Clear(void);

  void Compact(void) {
    nsVoidArray::Compact();
  }

  void Sort(void);
  void Sort(nsStringArrayComparatorFunc aFunc, void* aData);

  PRBool EnumerateForwards(nsStringArrayEnumFunc aFunc, void* aData);
  PRBool EnumerateBackwards(nsStringArrayEnumFunc aFunc, void* aData);

private:
  
  nsStringArray(const nsStringArray& other);
};


class nsCString;

typedef int (* PR_CALLBACK nsCStringArrayComparatorFunc)
            (const nsCString* aElement1, const nsCString* aElement2, void* aData);

typedef PRBool (*nsCStringArrayEnumFunc)(nsCString& aElement, void *aData);

class NS_COM_GLUE nsCStringArray: private nsVoidArray
{
public:
  nsCStringArray(void);
  nsCStringArray(PRInt32 aCount); 
  ~nsCStringArray(void);

  nsCStringArray& operator=(const nsCStringArray& other);

  
  
  
  
  
  void ParseString(const char* string, const char* delimiter);

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

  PRBool InsertCStringAt(const nsACString& aCString, PRInt32 aIndex);

  PRBool ReplaceCStringAt(const nsACString& aCString, PRInt32 aIndex);

  PRBool AppendCString(const nsACString& aCString) {
    return InsertCStringAt(aCString, Count());
  }

  PRBool RemoveCString(const nsACString& aCString);

#ifdef MOZILLA_INTERNAL_API
  PRBool RemoveCStringIgnoreCase(const nsACString& aCString);
#endif

  PRBool RemoveCStringAt(PRInt32 aIndex);
  void   Clear(void);

  void Compact(void) {
    nsVoidArray::Compact();
  }

  void Sort(void);

#ifdef MOZILLA_INTERNAL_API
  void SortIgnoreCase(void);
#endif

  void Sort(nsCStringArrayComparatorFunc aFunc, void* aData);

  PRBool EnumerateForwards(nsCStringArrayEnumFunc aFunc, void* aData);
  PRBool EnumerateBackwards(nsCStringArrayEnumFunc aFunc, void* aData);

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
  PRBool InsertElementAt(void* aElement, PRInt32 aIndex);
  PRBool InsertElementsAt(const nsVoidArray &other, PRInt32 aIndex);
  PRBool ReplaceElementAt(void* aElement, PRInt32 aIndex);
  PRBool MoveElement(PRInt32 aFrom, PRInt32 aTo);
  PRBool AppendElement(void* aElement);
  PRBool AppendElements(nsVoidArray& aElements) {
    return InsertElementsAt(aElements, Count());
  }
  PRBool RemoveElement(void* aElement);
  PRBool RemoveElementsAt(PRInt32 aIndex, PRInt32 aCount);
  PRBool RemoveElementAt(PRInt32 aIndex);

  void Clear();
  PRBool SizeTo(PRInt32 aMin);
  void Compact();
  void Sort(nsVoidArrayComparatorFunc aFunc, void* aData);

  PRBool EnumerateForwards(nsVoidArrayEnumFunc aFunc, void* aData);
  PRBool EnumerateBackwards(nsVoidArrayEnumFunc aFunc, void* aData);

private:

  PRBool HasSingle() const
  {
    return reinterpret_cast<PRWord>(mImpl) & 0x1;
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
  PRBool IsEmpty() const
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
  PRBool EnsureArray();
};

#endif 
