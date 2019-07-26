



#ifndef nsVoidArray_h___
#define nsVoidArray_h___



#include "nsDebug.h"

#include "mozilla/StandardInteger.h"


typedef int (* nsVoidArrayComparatorFunc)
            (const void* aElement1, const void* aElement2, void* aData);


typedef bool (* nsVoidArrayEnumFunc)(void* aElement, void *aData);
typedef bool (* nsVoidArrayEnumFuncConst)(const void* aElement, void *aData);


typedef size_t (* nsVoidArraySizeOfElementIncludingThisFunc)(const void* aElement,
                                                             nsMallocSizeOfFun aMallocSizeOf,
                                                             void *aData);


class NS_COM_GLUE nsVoidArray {
public:
  nsVoidArray();
  nsVoidArray(int32_t aCount);  
  ~nsVoidArray();

  nsVoidArray& operator=(const nsVoidArray& other);

  inline int32_t Count() const {
    return mImpl ? mImpl->mCount : 0;
  }
  
  bool SetCount(int32_t aNewCount);
  
  inline int32_t GetArraySize() const {
    return mImpl ? (int32_t(mImpl->mBits) & kArraySizeMask) : 0;
  }

  void* FastElementAt(int32_t aIndex) const
  {
    NS_ASSERTION(0 <= aIndex && aIndex < Count(), "nsVoidArray::FastElementAt: index out of range");
    return mImpl->mArray[aIndex];
  }

  
  
  
  void* ElementAt(int32_t aIndex) const
  {
    NS_ASSERTION(0 <= aIndex && aIndex < Count(), "nsVoidArray::ElementAt: index out of range");
    return SafeElementAt(aIndex);
  }

  
  void* SafeElementAt(int32_t aIndex) const
  {
    if (uint32_t(aIndex) >= uint32_t(Count())) 
    {
      return nullptr;
    }
    
    return mImpl->mArray[aIndex];
  }

  void* operator[](int32_t aIndex) const { return ElementAt(aIndex); }

  int32_t IndexOf(void* aPossibleElement) const;

  bool InsertElementAt(void* aElement, int32_t aIndex);
  bool InsertElementsAt(const nsVoidArray &other, int32_t aIndex);

  bool ReplaceElementAt(void* aElement, int32_t aIndex);

  
  bool MoveElement(int32_t aFrom, int32_t aTo);

  bool AppendElement(void* aElement) {
    return InsertElementAt(aElement, Count());
  }

  bool AppendElements(nsVoidArray& aElements) {
    return InsertElementsAt(aElements, Count());
  }

  bool RemoveElement(void* aElement);
  bool RemoveElementsAt(int32_t aIndex, int32_t aCount);
  bool RemoveElementAt(int32_t aIndex) { return RemoveElementsAt(aIndex,1); }

  void   Clear();

  bool SizeTo(int32_t aMin);
  
  
  void Compact();

  void Sort(nsVoidArrayComparatorFunc aFunc, void* aData);

  bool EnumerateForwards(nsVoidArrayEnumFunc aFunc, void* aData);
  bool EnumerateForwards(nsVoidArrayEnumFuncConst aFunc, void* aData) const;
  bool EnumerateBackwards(nsVoidArrayEnumFunc aFunc, void* aData);

  
  
  
  size_t SizeOfExcludingThis(
           nsVoidArraySizeOfElementIncludingThisFunc aSizeOfElementIncludingThis,
           nsMallocSizeOfFun aMallocSizeOf, void* aData = NULL) const;

protected:
  bool GrowArrayBy(int32_t aGrowBy);

  struct Impl {
    





    uint32_t mBits;

    


    int32_t mCount;

    


    void*   mArray[1];
  };

  Impl* mImpl;
#if DEBUG_VOIDARRAY
  int32_t mMaxCount;
  int32_t mMaxSize;
  bool    mIsAuto;
#endif

  enum {
    kArrayOwnerMask = 1 << 31,
    kArrayHasAutoBufferMask = 1 << 30,
    kArraySizeMask = ~(kArrayOwnerMask | kArrayHasAutoBufferMask)
  };
  enum { kAutoBufSize = 8 };


  
  void SetArray(Impl *newImpl, int32_t aSize, int32_t aCount, bool aOwner,
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


























class NS_COM_GLUE nsSmallVoidArray : private nsVoidArray
{
public:
  ~nsSmallVoidArray();

  nsSmallVoidArray& operator=(nsSmallVoidArray& other);
  void* operator[](int32_t aIndex) const { return ElementAt(aIndex); }

  int32_t GetArraySize() const;

  int32_t Count() const;
  void* FastElementAt(int32_t aIndex) const;
  
  
  
  void* ElementAt(int32_t aIndex) const
  {
    NS_ASSERTION(0 <= aIndex && aIndex < Count(), "nsSmallVoidArray::ElementAt: index out of range");
    return SafeElementAt(aIndex);
  }
  void* SafeElementAt(int32_t aIndex) const {
    
    if (uint32_t(aIndex) >= uint32_t(Count())) 
    {
      return nullptr;
    }
    return FastElementAt(aIndex);
  }
  int32_t IndexOf(void* aPossibleElement) const;
  bool InsertElementAt(void* aElement, int32_t aIndex);
  bool InsertElementsAt(const nsVoidArray &other, int32_t aIndex);
  bool ReplaceElementAt(void* aElement, int32_t aIndex);
  bool MoveElement(int32_t aFrom, int32_t aTo);
  bool AppendElement(void* aElement);
  bool AppendElements(nsVoidArray& aElements) {
    return InsertElementsAt(aElements, Count());
  }
  bool RemoveElement(void* aElement);
  bool RemoveElementsAt(int32_t aIndex, int32_t aCount);
  bool RemoveElementAt(int32_t aIndex);

  void Clear();
  bool SizeTo(int32_t aMin);
  void Compact();
  void Sort(nsVoidArrayComparatorFunc aFunc, void* aData);

  bool EnumerateForwards(nsVoidArrayEnumFunc aFunc, void* aData);
  bool EnumerateBackwards(nsVoidArrayEnumFunc aFunc, void* aData);

private:

  bool HasSingle() const
  {
    return !!(reinterpret_cast<intptr_t>(mImpl) & 0x1);
  }
  void* GetSingle() const
  {
    NS_ASSERTION(HasSingle(), "wrong type");
    return reinterpret_cast<void*>
                           (reinterpret_cast<intptr_t>(mImpl) & ~0x1);
  }
  void SetSingle(void *aChild)
  {
    NS_ASSERTION(HasSingle() || !mImpl, "overwriting array");
    mImpl = reinterpret_cast<Impl*>
                            (reinterpret_cast<intptr_t>(aChild) | 0x1);
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
