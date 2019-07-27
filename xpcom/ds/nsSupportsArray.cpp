





#include <string.h>
#include "mozilla/MathAlgorithms.h"
#include "nsSupportsArray.h"
#include "nsSupportsArrayEnumerator.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"

#if DEBUG_SUPPORTSARRAY
#define MAXSUPPORTS 20

class SupportsStats
{
public:
  SupportsStats();
  ~SupportsStats();

};

static int sizesUsed; 
static int sizesAlloced[MAXSUPPORTS]; 
static int NumberOfSize[MAXSUPPORTS]; 
static int AllocedOfSize[MAXSUPPORTS]; 
static int GrowInPlace[MAXSUPPORTS];


static int MaxElements[3000];


#define ADD_TO_STATS(x,size) do {int i; for (i = 0; i < sizesUsed; i++) \
                                  { \
                                    if (sizesAlloced[i] == (int)(size)) \
                                    { ((x)[i])++; break; } \
                                  } \
                                  if (i >= sizesUsed && sizesUsed < MAXSUPPORTS) \
                                  { sizesAlloced[sizesUsed] = (size); \
                                    ((x)[sizesUsed++])++; break; \
                                  } \
                                } while (0);

#define SUB_FROM_STATS(x,size) do {int i; for (i = 0; i < sizesUsed; i++) \
                                    { \
                                      if (sizesAlloced[i] == (int)(size)) \
                                      { ((x)[i])--; break; } \
                                    } \
                                  } while (0);


SupportsStats::SupportsStats()
{
  sizesUsed = 1;
  sizesAlloced[0] = 0;
}

SupportsStats::~SupportsStats()
{
  int i;
  for (i = 0; i < sizesUsed; ++i) {
    printf("Size %d:\n", sizesAlloced[i]);
    printf("\tNumber of SupportsArrays this size (max):     %d\n", NumberOfSize[i]);
    printf("\tNumber of allocations this size (total):  %d\n", AllocedOfSize[i]);
    printf("\tNumber of GrowsInPlace this size (total): %d\n", GrowInPlace[i]);
  }
  printf("Max Size of SupportsArray:\n");
  for (i = 0; i < (int)(sizeof(MaxElements) / sizeof(MaxElements[0])); ++i) {
    if (MaxElements[i]) {
      printf("\t%d: %d\n", i, MaxElements[i]);
    }
  }
}


SupportsStats gSupportsStats;
#endif

nsresult
nsQueryElementAt::operator()(const nsIID& aIID, void** aResult) const
{
  nsresult status =
    mCollection ? mCollection->QueryElementAt(mIndex, aIID, aResult) :
                  NS_ERROR_NULL_POINTER;

  if (mErrorPtr) {
    *mErrorPtr = status;
  }

  return status;
}

static const int32_t kGrowArrayBy = 8;
static const int32_t kLinearThreshold = 16 * sizeof(nsISupports*);

nsSupportsArray::nsSupportsArray()
{
  mArray = mAutoArray;
  mArraySize = kAutoArraySize;
  mCount = 0;
#if DEBUG_SUPPORTSARRAY
  mMaxCount = 0;
  mMaxSize = 0;
  ADD_TO_STATS(NumberOfSize, kAutoArraySize * sizeof(mArray[0]));
  MaxElements[0]++;
#endif
}

nsSupportsArray::~nsSupportsArray()
{
  DeleteArray();
}

void
nsSupportsArray::GrowArrayBy(int32_t aGrowBy)
{
  
  
  
  
  if (aGrowBy < kGrowArrayBy) {
    aGrowBy = kGrowArrayBy;
  }

  uint32_t newCount = mArraySize + aGrowBy;  
  uint32_t newSize = sizeof(mArray[0]) * newCount;

  if (newSize >= (uint32_t)kLinearThreshold) {
    
    
    
    if (newSize & (newSize - 1)) {
      newSize = 1u << mozilla::CeilingLog2(newSize);
    }

    newCount = newSize / sizeof(mArray[0]);
  }
  
  
  nsISupports** oldArray = mArray;

  mArray = new nsISupports*[newCount];
  mArraySize = newCount;

#if DEBUG_SUPPORTSARRAY
  if (oldArray == mArray) { 
    ADD_TO_STATS(GrowInPlace, mCount);
  }
  ADD_TO_STATS(AllocedOfSize, mArraySize * sizeof(mArray[0]));
  if (mArraySize > mMaxSize) {
    ADD_TO_STATS(NumberOfSize, mArraySize * sizeof(mArray[0]));
    if (oldArray != &(mAutoArray[0])) {
      SUB_FROM_STATS(NumberOfSize, mCount * sizeof(mArray[0]));
    }
    mMaxSize = mArraySize;
  }
#endif
  if (oldArray) {                   
    if (0 < mCount) {
      ::memcpy(mArray, oldArray, mCount * sizeof(nsISupports*));
    }
    if (oldArray != &(mAutoArray[0])) {
      delete[] oldArray;
    }
  }
}

nsresult
nsSupportsArray::Create(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  if (aOuter) {
    return NS_ERROR_NO_AGGREGATION;
  }

  nsCOMPtr<nsISupportsArray> it = new nsSupportsArray();

  return it->QueryInterface(aIID, aResult);
}

NS_IMPL_ISUPPORTS(nsSupportsArray, nsISupportsArray, nsICollection,
                  nsISerializable)

NS_IMETHODIMP
nsSupportsArray::Read(nsIObjectInputStream* aStream)
{
  nsresult rv;

  uint32_t newArraySize;
  rv = aStream->Read32(&newArraySize);

  if (newArraySize <= kAutoArraySize) {
    if (mArray != mAutoArray) {
      delete[] mArray;
      mArray = mAutoArray;
    }
    newArraySize = kAutoArraySize;
  } else {
    if (newArraySize <= mArraySize) {
      
      newArraySize = mArraySize;
    } else {
      nsISupports** array = new nsISupports*[newArraySize];
      if (mArray != mAutoArray) {
        delete[] mArray;
      }
      mArray = array;
    }
  }
  mArraySize = newArraySize;

  rv = aStream->Read32(&mCount);
  if (NS_FAILED(rv)) {
    return rv;
  }

  NS_ASSERTION(mCount <= mArraySize, "overlarge mCount!");
  if (mCount > mArraySize) {
    mCount = mArraySize;
  }

  for (uint32_t i = 0; i < mCount; i++) {
    rv = aStream->ReadObject(true, &mArray[i]);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsSupportsArray::Write(nsIObjectOutputStream* aStream)
{
  nsresult rv;

  rv = aStream->Write32(mArraySize);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = aStream->Write32(mCount);
  if (NS_FAILED(rv)) {
    return rv;
  }

  for (uint32_t i = 0; i < mCount; i++) {
    rv = aStream->WriteObject(mArray[i], true);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  return NS_OK;
}

void
nsSupportsArray::DeleteArray(void)
{
  Clear();
  if (mArray != &(mAutoArray[0])) {
    delete[] mArray;
    mArray = mAutoArray;
    mArraySize = kAutoArraySize;
  }
}


NS_IMETHODIMP_(bool)
nsSupportsArray::Equals(const nsISupportsArray* aOther)
{
  if (aOther) {
    uint32_t countOther;
    nsISupportsArray* other = const_cast<nsISupportsArray*>(aOther);
    nsresult rv = other->Count(&countOther);
    if (NS_FAILED(rv)) {
      return false;
    }

    if (mCount == countOther) {
      uint32_t index = mCount;
      nsCOMPtr<nsISupports> otherElem;
      while (index--) {
        if (NS_FAILED(other->GetElementAt(index, getter_AddRefs(otherElem)))) {
          return false;
        }
        if (mArray[index] != otherElem) {
          return false;
        }
      }
      return true;
    }
  }
  return false;
}

NS_IMETHODIMP
nsSupportsArray::GetElementAt(uint32_t aIndex, nsISupports** aOutPtr)
{
  *aOutPtr = nullptr;
  if (aIndex < mCount) {
    NS_IF_ADDREF(*aOutPtr = mArray[aIndex]);
  }
  return NS_OK;
}

NS_IMETHODIMP_(int32_t)
nsSupportsArray::IndexOf(const nsISupports* aPossibleElement)
{
  return IndexOfStartingAt(aPossibleElement, 0);
}

NS_IMETHODIMP_(int32_t)
nsSupportsArray::IndexOfStartingAt(const nsISupports* aPossibleElement,
                                   uint32_t aStartIndex)
{
  if (aStartIndex < mCount) {
    const nsISupports** start = (const nsISupports**)mArray;  
    const nsISupports** ep = (start + aStartIndex);
    const nsISupports** end = (start + mCount);
    while (ep < end) {
      if (aPossibleElement == *ep) {
        return (ep - start);
      }
      ep++;
    }
  }
  return -1;
}

NS_IMETHODIMP_(int32_t)
nsSupportsArray::LastIndexOf(const nsISupports* aPossibleElement)
{
  if (0 < mCount) {
    const nsISupports** start = (const nsISupports**)mArray;  
    const nsISupports** ep = (start + mCount);
    while (start <= --ep) {
      if (aPossibleElement == *ep) {
        return (ep - start);
      }
    }
  }
  return -1;
}

NS_IMETHODIMP_(bool)
nsSupportsArray::InsertElementAt(nsISupports* aElement, uint32_t aIndex)
{
  if (aIndex <= mCount) {
    if (mArraySize < (mCount + 1)) {
      
      GrowArrayBy(1);
    }

    
    
    uint32_t slide = (mCount - aIndex);
    if (0 < slide) {
      ::memmove(mArray + aIndex + 1, mArray + aIndex,
                slide * sizeof(nsISupports*));
    }

    mArray[aIndex] = aElement;
    NS_IF_ADDREF(aElement);
    mCount++;

#if DEBUG_SUPPORTSARRAY
    if (mCount > mMaxCount &&
        mCount < (int32_t)(sizeof(MaxElements) / sizeof(MaxElements[0]))) {
      MaxElements[mCount]++;
      MaxElements[mMaxCount]--;
      mMaxCount = mCount;
    }
#endif
    return true;
  }
  return false;
}

NS_IMETHODIMP_(bool)
nsSupportsArray::InsertElementsAt(nsISupportsArray* aElements, uint32_t aIndex)
{
  if (!aElements) {
    return false;
  }
  uint32_t countElements;
  if (NS_FAILED(aElements->Count(&countElements))) {
    return false;
  }

  if (aIndex <= mCount) {
    if (mArraySize < (mCount + countElements)) {
      
      GrowArrayBy(countElements);
    }

    
    
    uint32_t slide = (mCount - aIndex);
    if (0 < slide) {
      ::memmove(mArray + aIndex + countElements, mArray + aIndex,
                slide * sizeof(nsISupports*));
    }

    for (uint32_t i = 0; i < countElements; ++i, ++mCount) {
      
      if (NS_FAILED(aElements->GetElementAt(i, mArray + aIndex + i))) {
        return false;
      }
    }

#if DEBUG_SUPPORTSARRAY
    if (mCount > mMaxCount &&
        mCount < (int32_t)(sizeof(MaxElements) / sizeof(MaxElements[0]))) {
      MaxElements[mCount]++;
      MaxElements[mMaxCount]--;
      mMaxCount = mCount;
    }
#endif
    return true;
  }
  return false;
}

NS_IMETHODIMP_(bool)
nsSupportsArray::ReplaceElementAt(nsISupports* aElement, uint32_t aIndex)
{
  if (aIndex < mCount) {
    NS_IF_ADDREF(aElement);  
    NS_IF_RELEASE(mArray[aIndex]);
    mArray[aIndex] = aElement;
    return true;
  }
  return false;
}

NS_IMETHODIMP_(bool)
nsSupportsArray::RemoveElementsAt(uint32_t aIndex, uint32_t aCount)
{
  if (aIndex + aCount <= mCount) {
    for (uint32_t i = 0; i < aCount; i++) {
      NS_IF_RELEASE(mArray[aIndex + i]);
    }
    mCount -= aCount;
    int32_t slide = (mCount - aIndex);
    if (0 < slide) {
      ::memmove(mArray + aIndex, mArray + aIndex + aCount,
                slide * sizeof(nsISupports*));
    }
    return true;
  }
  return false;
}

NS_IMETHODIMP
nsSupportsArray::RemoveElement(nsISupports* aElement)
{
  int32_t theIndex = IndexOfStartingAt(aElement, 0);
  if (theIndex >= 0) {
    return RemoveElementAt(theIndex) ? NS_OK : NS_ERROR_FAILURE;
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP_(bool)
nsSupportsArray::RemoveLastElement(const nsISupports* aElement)
{
  int32_t theIndex = LastIndexOf(aElement);
  if (theIndex >= 0) {
    return RemoveElementAt(theIndex);
  }

  return false;
}

NS_IMETHODIMP_(bool)
nsSupportsArray::MoveElement(int32_t aFrom, int32_t aTo)
{
  nsISupports* tempElement;

  if (aTo == aFrom) {
    return true;
  }

  if (aTo < 0 || aFrom < 0 ||
      (uint32_t)aTo >= mCount || (uint32_t)aFrom >= mCount) {
    
    return false;
  }
  tempElement = mArray[aFrom];

  if (aTo < aFrom) {
    
    ::memmove(mArray + aTo + 1, mArray + aTo,
              (aFrom - aTo) * sizeof(mArray[0]));
    mArray[aTo] = tempElement;
  } else { 
    
    ::memmove(mArray + aFrom, mArray + aFrom + 1,
              (aTo - aFrom) * sizeof(mArray[0]));
    mArray[aTo] = tempElement;
  }

  return true;
}

NS_IMETHODIMP
nsSupportsArray::Clear(void)
{
  if (0 < mCount) {
    do {
      --mCount;
      NS_IF_RELEASE(mArray[mCount]);
    } while (0 != mCount);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSupportsArray::Compact(void)
{
#if DEBUG_SUPPORTSARRAY
  uint32_t oldArraySize = mArraySize;
#endif
  if ((mArraySize != mCount) && (kAutoArraySize < mArraySize)) {
    nsISupports** oldArray = mArray;
    if (mCount <= kAutoArraySize) {
      mArray = mAutoArray;
      mArraySize = kAutoArraySize;
    } else {
      mArray = new nsISupports*[mCount];
      if (!mArray) {
        mArray = oldArray;
        return NS_OK;
      }
      mArraySize = mCount;
    }
#if DEBUG_SUPPORTSARRAY
    if (oldArray == mArray &&
        oldArray != &(mAutoArray[0])) { 
      ADD_TO_STATS(GrowInPlace, oldArraySize);
    }
    if (oldArray != &(mAutoArray[0])) {
      ADD_TO_STATS(AllocedOfSize, mArraySize * sizeof(mArray[0]));
    }
#endif
    ::memcpy(mArray, oldArray, mCount * sizeof(nsISupports*));
    delete[] oldArray;
  }
  return NS_OK;
}

NS_IMETHODIMP_(bool)
nsSupportsArray::SizeTo(int32_t aSize)
{
#if DEBUG_SUPPORTSARRAY
  uint32_t oldArraySize = mArraySize;
#endif
  NS_ASSERTION(aSize >= 0, "negative aSize!");

  
  if (mArraySize == (uint32_t)aSize || (uint32_t)aSize < mCount) {
    return true;  
  }

  
  nsISupports** oldArray = mArray;
  if ((uint32_t)aSize <= kAutoArraySize) {
    mArray = mAutoArray;
    mArraySize = kAutoArraySize;
  } else {
    mArray = new nsISupports*[aSize];
    if (!mArray) {
      mArray = oldArray;
      return false;
    }
    mArraySize = aSize;
  }
#if DEBUG_SUPPORTSARRAY
  if (oldArray == mArray &&
      oldArray != &(mAutoArray[0])) { 
    ADD_TO_STATS(GrowInPlace, oldArraySize);
  }
  if (oldArray != &(mAutoArray[0])) {
    ADD_TO_STATS(AllocedOfSize, mArraySize * sizeof(mArray[0]));
  }
#endif
  ::memcpy(mArray, oldArray, mCount * sizeof(nsISupports*));
  if (oldArray != mAutoArray) {
    delete[] oldArray;
  }

  return true;
}

NS_IMETHODIMP
nsSupportsArray::Enumerate(nsIEnumerator** aResult)
{
  nsSupportsArrayEnumerator* e = new nsSupportsArrayEnumerator(this);
  if (!e) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aResult = e;
  NS_ADDREF(e);
  return NS_OK;
}

NS_IMETHODIMP
nsSupportsArray::Clone(nsISupportsArray** aResult)
{
  nsCOMPtr<nsISupportsArray> newArray;
  nsresult rv = NS_NewISupportsArray(getter_AddRefs(newArray));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  uint32_t count = 0;
  Count(&count);
  for (uint32_t i = 0; i < count; i++) {
    if (!newArray->InsertElementAt(mArray[i], i)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  newArray.forget(aResult);
  return NS_OK;
}

nsresult
NS_NewISupportsArray(nsISupportsArray** aInstancePtrResult)
{
  nsresult rv;
  rv = nsSupportsArray::Create(nullptr, NS_GET_IID(nsISupportsArray),
                               (void**)aInstancePtrResult);
  return rv;
}
