





































#include <string.h>
#include "nsTArray.h"
#include "nsXPCOM.h"
#include "nsDebug.h"

nsTArray_base::Header nsTArray_base::sEmptyHdr = { 0, 0, 0 };

#ifdef NS_BUILD_REFCNT_LOGGING
nsTArray_base::nsTArray_base()
  : mHdr(&sEmptyHdr) {
  MOZ_COUNT_CTOR(nsTArray_base);
}

nsTArray_base::~nsTArray_base() {
  MOZ_COUNT_DTOR(nsTArray_base);
}
#endif 

PRBool
nsTArray_base::EnsureCapacity(size_type capacity, size_type elemSize) {
  
  if (capacity <= mHdr->mCapacity)
    return PR_TRUE;

  
  
  
  
  if (capacity * elemSize > size_type(-1)/2) {
    NS_ERROR("Attempting to allocate excessively large array");
    return PR_FALSE;
  }

  if (mHdr == &sEmptyHdr) {
    
    Header *header = static_cast<Header*>
                                (NS_Alloc(sizeof(Header) + capacity * elemSize));
    if (!header)
      return PR_FALSE;
    header->mLength = 0;
    header->mCapacity = capacity;
    header->mIsAutoArray = 0;
    mHdr = header;
    
    return PR_TRUE;
  }

  
  NS_ASSERTION(mHdr->mCapacity > 0, "should not have buffer of zero size");
  size_type temp = mHdr->mCapacity;
  while (temp < capacity)
    temp <<= 1;
  capacity = temp;

  Header *header;
  if (UsesAutoArrayBuffer()) {
    
    header = static_cast<Header*>
                        (NS_Alloc(sizeof(Header) + capacity * elemSize));
    if (!header)
      return PR_FALSE;

    memcpy(header, mHdr, sizeof(Header) + Length() * elemSize);
  } else {
    
    size_type size = sizeof(Header) + capacity * elemSize;
    header = static_cast<Header*>(NS_Realloc(mHdr, size));
    if (!header)
      return PR_FALSE;
  }

  header->mCapacity = capacity;
  mHdr = header;

  return PR_TRUE;
}

void
nsTArray_base::ShrinkCapacity(size_type elemSize) {
  if (mHdr == &sEmptyHdr || UsesAutoArrayBuffer())
    return;

  if (mHdr->mLength >= mHdr->mCapacity)  
    return;

  size_type length = Length();

  if (IsAutoArray() && GetAutoArrayBuffer()->mCapacity >= length) {
    Header* header = GetAutoArrayBuffer();

    
    header->mLength = length;
    memcpy(header + 1, mHdr + 1, length * elemSize);

    NS_Free(mHdr);
    mHdr = header;
    return;
  }

  if (length == 0) {
    NS_ASSERTION(!IsAutoArray(), "autoarray should have fit 0 elements");
    NS_Free(mHdr);
    mHdr = &sEmptyHdr;
    return;
  }

  size_type size = sizeof(Header) + length * elemSize;
  void *ptr = NS_Realloc(mHdr, size);
  if (!ptr)
    return;
  mHdr = static_cast<Header*>(ptr);
  mHdr->mCapacity = length;
}

void
nsTArray_base::ShiftData(index_type start, size_type oldLen, size_type newLen,
                         size_type elemSize) {
  if (oldLen == newLen)
    return;

  
  size_type num = mHdr->mLength - (start + oldLen);

  
  mHdr->mLength += newLen - oldLen;
  if (mHdr->mLength == 0) {
    ShrinkCapacity(elemSize);
  } else {
    
    if (num == 0)
      return;
    
    start *= elemSize;
    newLen *= elemSize;
    oldLen *= elemSize;
    num *= elemSize;
    char *base = reinterpret_cast<char*>(mHdr + 1) + start;
    memmove(base + newLen, base + oldLen, num);
  }
}

PRBool
nsTArray_base::InsertSlotsAt(index_type index, size_type count,
                             size_type elementSize) {
  NS_ASSERTION(index <= Length(), "Bogus insertion index");
  size_type newLen = Length() + count;

  EnsureCapacity(newLen, elementSize);

  
  if (Capacity() < newLen)
    return PR_FALSE;

  
  
  ShiftData(index, 0, count, elementSize);
      
  return PR_TRUE;
}

PRBool
nsTArray_base::SwapArrayElements(nsTArray_base& other, size_type elemSize)
{
#ifdef DEBUG
  PRBool isAuto = IsAutoArray();
  PRBool otherIsAuto = other.IsAutoArray();
#endif

  if (!EnsureNotUsingAutoArrayBuffer(elemSize) ||
      !other.EnsureNotUsingAutoArrayBuffer(elemSize)) {
    return PR_FALSE;
  }

  NS_ASSERTION(isAuto == IsAutoArray(), "lost auto info");
  NS_ASSERTION(otherIsAuto == other.IsAutoArray(), "lost auto info");
  NS_ASSERTION(!UsesAutoArrayBuffer() && !other.UsesAutoArrayBuffer(),
               "both should be using an alloced buffer now");

  
  
  
  
  
  
  
  
  

  
  if (IsAutoArray() && !other.IsAutoArray()) {
    if (other.mHdr == &sEmptyHdr) {
      
      
      other.mHdr = GetAutoArrayBuffer();
      other.mHdr->mLength = 0;
    }
    else {
      other.mHdr->mIsAutoArray = 1;
    }
    mHdr->mIsAutoArray = 0;
  }
  else if (!IsAutoArray() && other.IsAutoArray()) {
    if (mHdr == &sEmptyHdr) {
      
      
      mHdr = other.GetAutoArrayBuffer();
      mHdr->mLength = 0;
    }
    else {
      mHdr->mIsAutoArray = 1;
    }
    other.mHdr->mIsAutoArray = 0;
  }

  
  Header *h = other.mHdr;
  other.mHdr = mHdr;
  mHdr = h;

  NS_ASSERTION(isAuto == IsAutoArray(), "lost auto info");
  NS_ASSERTION(otherIsAuto == other.IsAutoArray(), "lost auto info");

  return PR_TRUE;
}

PRBool
nsTArray_base::EnsureNotUsingAutoArrayBuffer(size_type elemSize)
{
  if (UsesAutoArrayBuffer()) {
    size_type size = sizeof(Header) + Length() * elemSize;

    Header* header = static_cast<Header*>(NS_Alloc(size));
    if (!header)
      return PR_FALSE;

    memcpy(header, mHdr, size);
    header->mCapacity = Length();
    mHdr = header;
  }
  
  return PR_TRUE;
}
