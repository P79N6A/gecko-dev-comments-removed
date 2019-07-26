




#include <stdlib.h>

#include "nsVoidArray.h"
#include "nsQuickSort.h"
#include "prbit.h"
#include "nsISupportsImpl.h" 
#include "nsAlgorithm.h"




static const PRInt32 kMinGrowArrayBy = 8;
static const PRInt32 kMaxGrowArrayBy = 1024;
static const PRInt32 kAutoClearCompactSizeFactor = 4;





static const PRInt32 kLinearThreshold = 24 * sizeof(void *);





#define SIZEOF_IMPL(n_) (sizeof(Impl) + sizeof(void *) * ((n_) - 1))






#define CAPACITYOF_IMPL(n_) ((((n_) - sizeof(Impl)) / sizeof(void *)) + 1)

#if DEBUG_VOIDARRAY
#define MAXVOID 10

class VoidStats {
public:
  VoidStats();
  ~VoidStats();

};

static int sizesUsed; 
static int sizesAlloced[MAXVOID]; 
static int NumberOfSize[MAXVOID]; 
static int AllocedOfSize[MAXVOID]; 
static int MaxAuto[MAXVOID];      
static int GrowInPlace[MAXVOID];  


static int MaxElements[2000];     


#define ADD_TO_STATS(x,size) do {int i; for (i = 0; i < sizesUsed; i++) \
                                  { \
                                    if (sizesAlloced[i] == (int)(size)) \
                                    { ((x)[i])++; break; } \
                                  } \
                                  if (i >= sizesUsed && sizesUsed < MAXVOID) \
                                  { sizesAlloced[sizesUsed] = (size); \
                                    ((x)[sizesUsed++])++; break; \
                                  } \
                                } while (0)

#define SUB_FROM_STATS(x,size) do {int i; for (i = 0; i < sizesUsed; i++) \
                                    { \
                                      if (sizesAlloced[i] == (int)(size)) \
                                      { ((x)[i])--; break; } \
                                    } \
                                  } while (0)


VoidStats::VoidStats()
{
  sizesUsed = 1;
  sizesAlloced[0] = 0;
}

VoidStats::~VoidStats()
{
  int i;
  for (i = 0; i < sizesUsed; i++)
  {
    printf("Size %d:\n",sizesAlloced[i]);
    printf("\tNumber of VoidArrays this size (max):     %d\n",NumberOfSize[i]-MaxAuto[i]);
    printf("\tNumber of AutoVoidArrays this size (max): %d\n",MaxAuto[i]);
    printf("\tNumber of allocations this size (total):  %d\n",AllocedOfSize[i]);
    printf("\tNumber of GrowsInPlace this size (total): %d\n",GrowInPlace[i]);
  }
  printf("Max Size of VoidArray:\n");
  for (i = 0; i < (int)(sizeof(MaxElements)/sizeof(MaxElements[0])); i++)
  {
    if (MaxElements[i])
      printf("\t%d: %d\n",i,MaxElements[i]);
  }
}


VoidStats gVoidStats;
#endif

void
nsVoidArray::SetArray(Impl *newImpl, PRInt32 aSize, PRInt32 aCount,
                      bool aOwner, bool aHasAuto)
{
  
  NS_PRECONDITION(newImpl, "can't set size");
  mImpl = newImpl;
  mImpl->mCount = aCount;
  mImpl->mBits = static_cast<PRUint32>(aSize & kArraySizeMask) |
                 (aOwner ? kArrayOwnerMask : 0) |
                 (aHasAuto ? kArrayHasAutoBufferMask : 0);
}




bool nsVoidArray::SizeTo(PRInt32 aSize)
{
  PRUint32 oldsize = GetArraySize();
  bool isOwner = IsArrayOwner();
  bool hasAuto = HasAutoBuffer();

  if (aSize == (PRInt32) oldsize)
    return true; 

  if (aSize <= 0)
  {
    
    if (mImpl)
    {
      if (isOwner)
      {
        free(reinterpret_cast<char *>(mImpl));
        if (hasAuto) {
          static_cast<nsAutoVoidArray*>(this)->ResetToAutoBuffer();
        }
        else {
          mImpl = nsnull;
        }
      }
      else
      {
        mImpl->mCount = 0; 
      }
    }
    return true;
  }

  if (mImpl && isOwner)
  {
    
    if (aSize < mImpl->mCount)
    {
      
      return true;  
    }

    char* bytes = (char *) realloc(mImpl,SIZEOF_IMPL(aSize));
    Impl* newImpl = reinterpret_cast<Impl*>(bytes);
    if (!newImpl)
      return false;

#if DEBUG_VOIDARRAY
    if (mImpl == newImpl)
      ADD_TO_STATS(GrowInPlace,oldsize);
    ADD_TO_STATS(AllocedOfSize,SIZEOF_IMPL(aSize));
    if (aSize > mMaxSize)
    {
      ADD_TO_STATS(NumberOfSize,SIZEOF_IMPL(aSize));
      if (oldsize)
        SUB_FROM_STATS(NumberOfSize,oldsize);
      mMaxSize = aSize;
      if (mIsAuto)
      {
        ADD_TO_STATS(MaxAuto,SIZEOF_IMPL(aSize));
        SUB_FROM_STATS(MaxAuto,oldsize);
      }
    }
#endif
    SetArray(newImpl, aSize, newImpl->mCount, true, hasAuto);
    return true;
  }

  if ((PRUint32) aSize < oldsize) {
    
    return true;
  }

  
  
  char* bytes = (char *) malloc(SIZEOF_IMPL(aSize));
  Impl* newImpl = reinterpret_cast<Impl*>(bytes);
  if (!newImpl)
    return false;

#if DEBUG_VOIDARRAY
  ADD_TO_STATS(AllocedOfSize,SIZEOF_IMPL(aSize));
  if (aSize > mMaxSize)
  {
    ADD_TO_STATS(NumberOfSize,SIZEOF_IMPL(aSize));
    if (oldsize && !mImpl)
      SUB_FROM_STATS(NumberOfSize,oldsize);
    mMaxSize = aSize;
  }
#endif
  if (mImpl)
  {
#if DEBUG_VOIDARRAY
    ADD_TO_STATS(MaxAuto,SIZEOF_IMPL(aSize));
    SUB_FROM_STATS(MaxAuto,0);
    SUB_FROM_STATS(NumberOfSize,0);
    mIsAuto = true;
#endif
    
    
    memcpy(newImpl->mArray, mImpl->mArray,
                  mImpl->mCount * sizeof(mImpl->mArray[0]));
  }

  SetArray(newImpl, aSize, mImpl ? mImpl->mCount : 0, true, hasAuto);
  
  return true;
}

bool nsVoidArray::GrowArrayBy(PRInt32 aGrowBy)
{
  
  
  
  
  if (aGrowBy < kMinGrowArrayBy)
    aGrowBy = kMinGrowArrayBy;

  PRUint32 newCapacity = GetArraySize() + aGrowBy;  
  PRUint32 newSize = SIZEOF_IMPL(newCapacity);

  if (newSize >= (PRUint32) kLinearThreshold)
  {
    
    
    
    
    if (GetArraySize() >= kMaxGrowArrayBy)
    {
      newCapacity = GetArraySize() + NS_MAX(kMaxGrowArrayBy,aGrowBy);
      newSize = SIZEOF_IMPL(newCapacity);
    }
    else
    {
      PR_CEILING_LOG2(newSize, newSize);
      newCapacity = CAPACITYOF_IMPL(PR_BIT(newSize));
    }
  }
  
  if (!SizeTo(newCapacity))
    return false;

  return true;
}

nsVoidArray::nsVoidArray()
  : mImpl(nsnull)
{
  MOZ_COUNT_CTOR(nsVoidArray);
#if DEBUG_VOIDARRAY
  mMaxCount = 0;
  mMaxSize = 0;
  mIsAuto = false;
  ADD_TO_STATS(NumberOfSize,0);
  MaxElements[0]++;
#endif
}

nsVoidArray::nsVoidArray(PRInt32 aCount)
  : mImpl(nsnull)
{
  MOZ_COUNT_CTOR(nsVoidArray);
#if DEBUG_VOIDARRAY
  mMaxCount = 0;
  mMaxSize = 0;
  mIsAuto = false;
  MaxElements[0]++;
#endif
  SizeTo(aCount);
}

nsVoidArray& nsVoidArray::operator=(const nsVoidArray& other)
{
  PRInt32 otherCount = other.Count();
  PRInt32 maxCount = GetArraySize();
  if (otherCount)
  {
    if (otherCount > maxCount)
    {
      
      if (!GrowArrayBy(otherCount-maxCount))
        return *this;      

      memcpy(mImpl->mArray, other.mImpl->mArray, otherCount * sizeof(mImpl->mArray[0]));
      mImpl->mCount = otherCount;
    }
    else
    {
      
      memcpy(mImpl->mArray, other.mImpl->mArray, otherCount * sizeof(mImpl->mArray[0]));
      mImpl->mCount = otherCount;
      
      if ((otherCount*2) < maxCount && maxCount > 100)
      {
        Compact();  
      }
    }
#if DEBUG_VOIDARRAY
     if (mImpl->mCount > mMaxCount &&
         mImpl->mCount < (PRInt32)(sizeof(MaxElements)/sizeof(MaxElements[0])))
     {
       MaxElements[mImpl->mCount]++;
       MaxElements[mMaxCount]--;
       mMaxCount = mImpl->mCount;
     }
#endif
  }
  else
  {
    
    SizeTo(0);
  }

  return *this;
}

nsVoidArray::~nsVoidArray()
{
  MOZ_COUNT_DTOR(nsVoidArray);
  if (mImpl && IsArrayOwner())
    free(reinterpret_cast<char*>(mImpl));
}

bool nsVoidArray::SetCount(PRInt32 aNewCount)
{
  NS_ASSERTION(aNewCount >= 0,"SetCount(negative index)");
  if (aNewCount < 0)
    return false;

  if (aNewCount == 0)
  {
    Clear();
    return true;
  }

  if (PRUint32(aNewCount) > PRUint32(GetArraySize()))
  {
    PRInt32 oldCount = Count();
    PRInt32 growDelta = aNewCount - oldCount;

    
    if (!GrowArrayBy(growDelta))
      return false;
  }

  if (aNewCount > mImpl->mCount)
  {
    
    
    
    memset(&mImpl->mArray[mImpl->mCount], 0,
           (aNewCount - mImpl->mCount) * sizeof(mImpl->mArray[0]));
  }

  mImpl->mCount = aNewCount;

#if DEBUG_VOIDARRAY
  if (mImpl->mCount > mMaxCount &&
      mImpl->mCount < (PRInt32)(sizeof(MaxElements)/sizeof(MaxElements[0])))
  {
    MaxElements[mImpl->mCount]++;
    MaxElements[mMaxCount]--;
    mMaxCount = mImpl->mCount;
  }
#endif

  return true;
}

PRInt32 nsVoidArray::IndexOf(void* aPossibleElement) const
{
  if (mImpl)
  {
    void** ap = mImpl->mArray;
    void** end = ap + mImpl->mCount;
    while (ap < end)
    {
      if (*ap == aPossibleElement)
      {
        return ap - mImpl->mArray;
      }
      ap++;
    }
  }
  return -1;
}

bool nsVoidArray::InsertElementAt(void* aElement, PRInt32 aIndex)
{
  PRInt32 oldCount = Count();
  NS_ASSERTION(aIndex >= 0,"InsertElementAt(negative index)");
  if (PRUint32(aIndex) > PRUint32(oldCount))
  {
    
    
    
    return false;
  }

  if (oldCount >= GetArraySize())
  {
    if (!GrowArrayBy(1))
      return false;
  }
  

  PRInt32 slide = oldCount - aIndex;
  if (0 != slide)
  {
    
    memmove(mImpl->mArray + aIndex + 1, mImpl->mArray + aIndex,
            slide * sizeof(mImpl->mArray[0]));
  }

  mImpl->mArray[aIndex] = aElement;
  mImpl->mCount++;

#if DEBUG_VOIDARRAY
  if (mImpl->mCount > mMaxCount &&
      mImpl->mCount < (PRInt32)(sizeof(MaxElements)/sizeof(MaxElements[0])))
  {
    MaxElements[mImpl->mCount]++;
    MaxElements[mMaxCount]--;
    mMaxCount = mImpl->mCount;
  }
#endif

  return true;
}

bool nsVoidArray::InsertElementsAt(const nsVoidArray& other, PRInt32 aIndex)
{
  PRInt32 oldCount = Count();
  PRInt32 otherCount = other.Count();

  NS_ASSERTION(aIndex >= 0,"InsertElementsAt(negative index)");
  if (PRUint32(aIndex) > PRUint32(oldCount))
  {
    
    
    
    return false;
  }

  if (oldCount + otherCount > GetArraySize())
  {
    if (!GrowArrayBy(otherCount))
      return false;;
  }
  

  PRInt32 slide = oldCount - aIndex;
  if (0 != slide)
  {
    
    memmove(mImpl->mArray + aIndex + otherCount, mImpl->mArray + aIndex,
            slide * sizeof(mImpl->mArray[0]));
  }

  for (PRInt32 i = 0; i < otherCount; i++)
  {
    
    mImpl->mArray[aIndex++] = other.mImpl->mArray[i];
    mImpl->mCount++;
  }

#if DEBUG_VOIDARRAY
  if (mImpl->mCount > mMaxCount &&
      mImpl->mCount < (PRInt32)(sizeof(MaxElements)/sizeof(MaxElements[0])))
  {
    MaxElements[mImpl->mCount]++;
    MaxElements[mMaxCount]--;
    mMaxCount = mImpl->mCount;
  }
#endif

  return true;
}

bool nsVoidArray::ReplaceElementAt(void* aElement, PRInt32 aIndex)
{
  NS_ASSERTION(aIndex >= 0,"ReplaceElementAt(negative index)");
  if (aIndex < 0)
    return false;

  
  
  if (PRUint32(aIndex) >= PRUint32(GetArraySize()))
  {
    PRInt32 oldCount = Count();
    PRInt32 requestedCount = aIndex + 1;
    PRInt32 growDelta = requestedCount - oldCount;

    
    if (!GrowArrayBy(growDelta))
      return false;
  }

  mImpl->mArray[aIndex] = aElement;
  if (aIndex >= mImpl->mCount)
  {
    
    
    
    if (aIndex > mImpl->mCount) 
    {
      
      
      
      memset(&mImpl->mArray[mImpl->mCount], 0,
             (aIndex - mImpl->mCount) * sizeof(mImpl->mArray[0]));
    }
    
     mImpl->mCount = aIndex + 1;

#if DEBUG_VOIDARRAY
     if (mImpl->mCount > mMaxCount &&
         mImpl->mCount < (PRInt32)(sizeof(MaxElements)/sizeof(MaxElements[0])))
     {
       MaxElements[mImpl->mCount]++;
       MaxElements[mMaxCount]--;
       mMaxCount = mImpl->mCount;
     }
#endif
  }

  return true;
}


bool nsVoidArray::MoveElement(PRInt32 aFrom, PRInt32 aTo)
{
  void *tempElement;

  if (aTo == aFrom)
    return true;

  NS_ASSERTION(aTo >= 0 && aFrom >= 0,"MoveElement(negative index)");
  if (aTo >= Count() || aFrom >= Count())
  {
    
    return false;
  }
  tempElement = mImpl->mArray[aFrom];

  if (aTo < aFrom)
  {
    
    memmove(mImpl->mArray + aTo + 1, mImpl->mArray + aTo,
            (aFrom-aTo) * sizeof(mImpl->mArray[0]));
    mImpl->mArray[aTo] = tempElement;
  }
  else 
  {
    
    memmove(mImpl->mArray + aFrom, mImpl->mArray + aFrom + 1,
            (aTo-aFrom) * sizeof(mImpl->mArray[0]));
    mImpl->mArray[aTo] = tempElement;
  }

  return true;
}

bool nsVoidArray::RemoveElementsAt(PRInt32 aIndex, PRInt32 aCount)
{
  PRInt32 oldCount = Count();
  NS_ASSERTION(aIndex >= 0,"RemoveElementsAt(negative index)");
  if (PRUint32(aIndex) >= PRUint32(oldCount))
  {
    
    return false;
  }
  
  if (aCount + aIndex > oldCount)
    aCount = oldCount - aIndex;

  
  
  if (aIndex < (oldCount - aCount))
  {
    memmove(mImpl->mArray + aIndex, mImpl->mArray + aIndex + aCount,
            (oldCount - (aIndex + aCount)) * sizeof(mImpl->mArray[0]));
  }

  mImpl->mCount -= aCount;
  return true;
}

bool nsVoidArray::RemoveElement(void* aElement)
{
  PRInt32 theIndex = IndexOf(aElement);
  if (theIndex != -1)
    return RemoveElementAt(theIndex);

  return false;
}

void nsVoidArray::Clear()
{
  if (mImpl)
  {
    mImpl->mCount = 0;
    
    
    if (HasAutoBuffer() && IsArrayOwner() &&
        GetArraySize() > kAutoClearCompactSizeFactor * kAutoBufSize) {
      SizeTo(0);
    }
  }
}

void nsVoidArray::Compact()
{
  if (mImpl)
  {
    
    
    PRInt32 count = Count();
    if (HasAutoBuffer() && count <= kAutoBufSize)
    {
      Impl* oldImpl = mImpl;
      static_cast<nsAutoVoidArray*>(this)->ResetToAutoBuffer();
      memcpy(mImpl->mArray, oldImpl->mArray,
             count * sizeof(mImpl->mArray[0]));
      free(reinterpret_cast<char *>(oldImpl));
    }
    else if (GetArraySize() > count)
    {
      SizeTo(Count());
    }
  }
}



struct VoidArrayComparatorContext {
  nsVoidArrayComparatorFunc mComparatorFunc;
  void* mData;
};

static int
VoidArrayComparator(const void* aElement1, const void* aElement2, void* aData)
{
  VoidArrayComparatorContext* ctx = static_cast<VoidArrayComparatorContext*>(aData);
  return (*ctx->mComparatorFunc)(*static_cast<void* const*>(aElement1),
                                 *static_cast<void* const*>(aElement2),
                                  ctx->mData);
}

void nsVoidArray::Sort(nsVoidArrayComparatorFunc aFunc, void* aData)
{
  if (mImpl && mImpl->mCount > 1)
  {
    VoidArrayComparatorContext ctx = {aFunc, aData};
    NS_QuickSort(mImpl->mArray, mImpl->mCount, sizeof(mImpl->mArray[0]),
                 VoidArrayComparator, &ctx);
  }
}

bool nsVoidArray::EnumerateForwards(nsVoidArrayEnumFunc aFunc, void* aData)
{
  PRInt32 index = -1;
  bool    running = true;

  if (mImpl) {
    while (running && (++index < mImpl->mCount)) {
      running = (*aFunc)(mImpl->mArray[index], aData);
    }
  }
  return running;
}

bool nsVoidArray::EnumerateForwards(nsVoidArrayEnumFuncConst aFunc,
                                    void* aData) const
{
  PRInt32 index = -1;
  bool    running = true;

  if (mImpl) {
    while (running && (++index < mImpl->mCount)) {
      running = (*aFunc)(mImpl->mArray[index], aData);
    }
  }
  return running;
}

bool nsVoidArray::EnumerateBackwards(nsVoidArrayEnumFunc aFunc, void* aData)
{
  bool    running = true;

  if (mImpl)
  {
    PRInt32 index = Count();
    while (running && (0 <= --index))
    {
      running = (*aFunc)(mImpl->mArray[index], aData);
    }
  }
  return running;
}

struct SizeOfElementIncludingThisData
{
  size_t mSize;
  nsVoidArraySizeOfElementIncludingThisFunc mSizeOfElementIncludingThis;
  nsMallocSizeOfFun mMallocSizeOf;
  void *mData;      
};

static bool
SizeOfElementIncludingThisEnumerator(const void *aElement, void *aData)
{
  SizeOfElementIncludingThisData *d = (SizeOfElementIncludingThisData *)aData;
  d->mSize += d->mSizeOfElementIncludingThis(aElement, d->mMallocSizeOf, d->mData);
  return true;
}

size_t
nsVoidArray::SizeOfExcludingThis(
  nsVoidArraySizeOfElementIncludingThisFunc aSizeOfElementIncludingThis,
  nsMallocSizeOfFun aMallocSizeOf, void* aData) const
{
  size_t n = 0;
  
  if (mImpl) {
    n += aMallocSizeOf(mImpl);
  }
  
  if (aSizeOfElementIncludingThis) { 
    SizeOfElementIncludingThisData data2 =
      { 0, aSizeOfElementIncludingThis, aMallocSizeOf, aData };
    EnumerateForwards(SizeOfElementIncludingThisEnumerator, &data2);
    n += data2.mSize;
  }
  return n;
}




nsAutoVoidArray::nsAutoVoidArray()
  : nsVoidArray()
{
  
  
#if DEBUG_VOIDARRAY
  mIsAuto = true;
  ADD_TO_STATS(MaxAuto,0);
#endif
  ResetToAutoBuffer();
}





nsSmallVoidArray::~nsSmallVoidArray()
{
  if (HasSingle())
  {
    
    mImpl = nsnull;
  }
}

nsSmallVoidArray& 
nsSmallVoidArray::operator=(nsSmallVoidArray& other)
{
  PRInt32 count = other.Count();
  switch (count) {
    case 0:
      Clear();
      break;
    case 1:
      Clear();
      AppendElement(other.ElementAt(0));
      break;
    default:
      if (GetArraySize() >= count || SizeTo(count)) {
        *AsArray() = *other.AsArray();
      }
  }
    
  return *this;
}

PRInt32
nsSmallVoidArray::GetArraySize() const
{
  if (HasSingle()) {
    return 1;
  }

  return AsArray()->GetArraySize();
}

PRInt32
nsSmallVoidArray::Count() const
{
  if (HasSingle()) {
    return 1;
  }

  return AsArray()->Count();
}

void*
nsSmallVoidArray::FastElementAt(PRInt32 aIndex) const
{
  NS_ASSERTION(0 <= aIndex && aIndex < Count(), "nsSmallVoidArray::FastElementAt: index out of range");

  if (HasSingle()) {
    return GetSingle();
  }

  return AsArray()->FastElementAt(aIndex);
}

PRInt32
nsSmallVoidArray::IndexOf(void* aPossibleElement) const
{
  if (HasSingle()) {
    return aPossibleElement == GetSingle() ? 0 : -1;
  }

  return AsArray()->IndexOf(aPossibleElement);
}

bool
nsSmallVoidArray::InsertElementAt(void* aElement, PRInt32 aIndex)
{
  NS_ASSERTION(!(NS_PTR_TO_INT32(aElement) & 0x1),
               "Attempt to add element with 0x1 bit set to nsSmallVoidArray");

  if (aIndex == 0 && IsEmpty()) {
    SetSingle(aElement);

    return true;
  }

  if (!EnsureArray()) {
    return false;
  }

  return AsArray()->InsertElementAt(aElement, aIndex);
}

bool nsSmallVoidArray::InsertElementsAt(const nsVoidArray &aOther, PRInt32 aIndex)
{
#ifdef DEBUG  
  for (int i = 0; i < aOther.Count(); i++) {
    NS_ASSERTION(!(NS_PTR_TO_INT32(aOther.ElementAt(i)) & 0x1),
                 "Attempt to add element with 0x1 bit set to nsSmallVoidArray");
  }
#endif

  if (aIndex == 0 && IsEmpty() && aOther.Count() == 1) {
    SetSingle(aOther.FastElementAt(0));
    
    return true;
  }

  if (!EnsureArray()) {
    return false;
  }

  return AsArray()->InsertElementsAt(aOther, aIndex);
}

bool
nsSmallVoidArray::ReplaceElementAt(void* aElement, PRInt32 aIndex)
{
  NS_ASSERTION(!(NS_PTR_TO_INT32(aElement) & 0x1),
               "Attempt to add element with 0x1 bit set to nsSmallVoidArray");

  if (aIndex == 0 && (IsEmpty() || HasSingle())) {
    SetSingle(aElement);
    
    return true;
  }

  if (!EnsureArray()) {
    return false;
  }

  return AsArray()->ReplaceElementAt(aElement, aIndex);
}

bool
nsSmallVoidArray::AppendElement(void* aElement)
{
  NS_ASSERTION(!(NS_PTR_TO_INT32(aElement) & 0x1),
               "Attempt to add element with 0x1 bit set to nsSmallVoidArray");

  if (IsEmpty()) {
    SetSingle(aElement);
    
    return true;
  }

  if (!EnsureArray()) {
    return false;
  }

  return AsArray()->AppendElement(aElement);
}

bool
nsSmallVoidArray::RemoveElement(void* aElement)
{
  if (HasSingle()) {
    if (aElement == GetSingle()) {
      mImpl = nsnull;
      return true;
    }
    
    return false;
  }

  return AsArray()->RemoveElement(aElement);
}

bool
nsSmallVoidArray::RemoveElementAt(PRInt32 aIndex)
{
  if (HasSingle()) {
    if (aIndex == 0) {
      mImpl = nsnull;

      return true;
    }
    
    return false;
  }

  return AsArray()->RemoveElementAt(aIndex);
}

bool
nsSmallVoidArray::RemoveElementsAt(PRInt32 aIndex, PRInt32 aCount)
{
  if (HasSingle()) {
    if (aIndex == 0) {
      if (aCount > 0) {
        mImpl = nsnull;
      }

      return true;
    }

    return false;
  }

  return AsArray()->RemoveElementsAt(aIndex, aCount);
}

void
nsSmallVoidArray::Clear()
{
  if (HasSingle()) {
    mImpl = nsnull;
  }
  else {
    AsArray()->Clear();
  }
}

bool
nsSmallVoidArray::SizeTo(PRInt32 aMin)
{
  if (!HasSingle()) {
    return AsArray()->SizeTo(aMin);
  }

  if (aMin <= 0) {
    mImpl = nsnull;

    return true;
  }

  if (aMin == 1) {
    return true;
  }

  void* single = GetSingle();
  mImpl = nsnull;
  if (!AsArray()->SizeTo(aMin)) {
    SetSingle(single);

    return false;
  }

  AsArray()->AppendElement(single);

  return true;
}

void
nsSmallVoidArray::Compact()
{
  if (!HasSingle()) {
    AsArray()->Compact();
  }
}

void
nsSmallVoidArray::Sort(nsVoidArrayComparatorFunc aFunc, void* aData)
{
  if (!HasSingle()) {
    AsArray()->Sort(aFunc,aData);
  }
}

bool
nsSmallVoidArray::EnumerateForwards(nsVoidArrayEnumFunc aFunc, void* aData)
{
  if (HasSingle()) {
    return (*aFunc)(GetSingle(), aData);
  }
  return AsArray()->EnumerateForwards(aFunc,aData);
}

bool
nsSmallVoidArray::EnumerateBackwards(nsVoidArrayEnumFunc aFunc, void* aData)
{
  if (HasSingle()) {
    return (*aFunc)(GetSingle(), aData);
  }
  return AsArray()->EnumerateBackwards(aFunc,aData);
}

bool
nsSmallVoidArray::EnsureArray()
{
  if (!HasSingle()) {
    return true;
  }

  void* single = GetSingle();
  mImpl = nsnull;
  if (!AsArray()->AppendElement(single)) {
    SetSingle(single);

    return false;
  }

  return true;
}
