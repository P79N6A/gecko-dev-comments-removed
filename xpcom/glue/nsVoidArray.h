




#ifndef nsVoidArray_h___
#define nsVoidArray_h___



#include "nsDebug.h"

#include "mozilla/MemoryReporting.h"
#include <stdint.h>


typedef int (*nsVoidArrayComparatorFunc)(const void* aElement1,
                                         const void* aElement2, void* aData);


typedef bool (*nsVoidArrayEnumFunc)(void* aElement, void* aData);
typedef bool (*nsVoidArrayEnumFuncConst)(const void* aElement, void* aData);


typedef size_t (*nsVoidArraySizeOfElementIncludingThisFunc)(
  const void* aElement, mozilla::MallocSizeOf aMallocSizeOf, void* aData);


class nsVoidArray
{
public:
  nsVoidArray();
  explicit nsVoidArray(int32_t aCount);  
  ~nsVoidArray();

  nsVoidArray& operator=(const nsVoidArray& aOther);

  inline int32_t Count() const { return mImpl ? mImpl->mCount : 0; }
  
  bool SetCount(int32_t aNewCount);
  
  inline int32_t GetArraySize() const { return mImpl ? mImpl->mSize : 0; }

  void* FastElementAt(int32_t aIndex) const
  {
    NS_ASSERTION(aIndex >= 0 && aIndex < Count(),
                 "nsVoidArray::FastElementAt: index out of range");
    return mImpl->mArray[aIndex];
  }

  
  
  
  void* ElementAt(int32_t aIndex) const
  {
    NS_ASSERTION(aIndex >= 0 && aIndex < Count(),
                 "nsVoidArray::ElementAt: index out of range");
    return SafeElementAt(aIndex);
  }

  
  void* SafeElementAt(int32_t aIndex) const
  {
    if (uint32_t(aIndex) >= uint32_t(Count())) { 
      return nullptr;
    }
    
    return mImpl->mArray[aIndex];
  }

  void* operator[](int32_t aIndex) const { return ElementAt(aIndex); }

  int32_t IndexOf(void* aPossibleElement) const;

  bool InsertElementAt(void* aElement, int32_t aIndex);
  bool InsertElementsAt(const nsVoidArray& aOther, int32_t aIndex);

  bool ReplaceElementAt(void* aElement, int32_t aIndex);

  
  bool MoveElement(int32_t aFrom, int32_t aTo);

  bool AppendElement(void* aElement)
  {
    return InsertElementAt(aElement, Count());
  }

  bool AppendElements(nsVoidArray& aElements)
  {
    return InsertElementsAt(aElements, Count());
  }

  bool RemoveElement(void* aElement);
  void RemoveElementsAt(int32_t aIndex, int32_t aCount);
  void RemoveElementAt(int32_t aIndex)
  {
    return RemoveElementsAt(aIndex, 1);
  }

  void Clear();

  bool SizeTo(int32_t aMin);
  
  
  void Compact();

  void Sort(nsVoidArrayComparatorFunc aFunc, void* aData);

  bool EnumerateForwards(nsVoidArrayEnumFunc aFunc, void* aData);
  bool EnumerateForwards(nsVoidArrayEnumFuncConst aFunc, void* aData) const;
  bool EnumerateBackwards(nsVoidArrayEnumFunc aFunc, void* aData);

  
  
  
  size_t SizeOfExcludingThis(
    nsVoidArraySizeOfElementIncludingThisFunc aSizeOfElementIncludingThis,
    mozilla::MallocSizeOf aMallocSizeOf, void* aData = nullptr) const;

protected:
  bool GrowArrayBy(int32_t aGrowBy);

  struct Impl
  {
    


    int32_t mSize;

    


    int32_t mCount;

    


    void*   mArray[1];
  };

  Impl* mImpl;
#if DEBUG_VOIDARRAY
  int32_t mMaxCount;
  int32_t mMaxSize;
  bool    mIsAuto;
#endif

  
  void SetArray(Impl* aNewImpl, int32_t aSize, int32_t aCount);

private:
  
  nsVoidArray(const nsVoidArray& aOther);
};

























class nsSmallVoidArray : private nsVoidArray
{
public:
  ~nsSmallVoidArray();

  nsSmallVoidArray& operator=(nsSmallVoidArray& aOther);
  void* operator[](int32_t aIndex) const { return ElementAt(aIndex); }

  int32_t GetArraySize() const;

  int32_t Count() const;
  void* FastElementAt(int32_t aIndex) const;
  
  
  
  void* ElementAt(int32_t aIndex) const
  {
    NS_ASSERTION(aIndex >= 0 && aIndex < Count(),
                 "nsSmallVoidArray::ElementAt: index out of range");
    return SafeElementAt(aIndex);
  }
  void* SafeElementAt(int32_t aIndex) const
  {
    
    if (uint32_t(aIndex) >= uint32_t(Count())) { 
      return nullptr;
    }
    return FastElementAt(aIndex);
  }
  int32_t IndexOf(void* aPossibleElement) const;
  bool InsertElementAt(void* aElement, int32_t aIndex);
  bool InsertElementsAt(const nsVoidArray& aOther, int32_t aIndex);
  bool ReplaceElementAt(void* aElement, int32_t aIndex);
  bool MoveElement(int32_t aFrom, int32_t aTo);
  bool AppendElement(void* aElement);
  bool AppendElements(nsVoidArray& aElements)
  {
    return InsertElementsAt(aElements, Count());
  }
  bool RemoveElement(void* aElement);
  void RemoveElementsAt(int32_t aIndex, int32_t aCount);
  void RemoveElementAt(int32_t aIndex);

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
    return reinterpret_cast<void*>(reinterpret_cast<intptr_t>(mImpl) & ~0x1);
  }
  void SetSingle(void* aChild)
  {
    NS_ASSERTION(HasSingle() || !mImpl, "overwriting array");
    mImpl = reinterpret_cast<Impl*>(reinterpret_cast<intptr_t>(aChild) | 0x1);
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
