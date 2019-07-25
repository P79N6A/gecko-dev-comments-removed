





































#ifndef nsTArray_h__
#  error "Don't include this file directly"
#endif

template<class Alloc>
nsTArray_base<Alloc>::nsTArray_base()
  : mHdr(EmptyHdr()) {
  MOZ_COUNT_CTOR(nsTArray_base);
}

template<class Alloc>
nsTArray_base<Alloc>::~nsTArray_base() {
  if (mHdr != EmptyHdr() && !UsesAutoArrayBuffer()) {
    Alloc::Free(mHdr);
  }
  MOZ_COUNT_DTOR(nsTArray_base);
}

template<class Alloc>
nsTArrayHeader* nsTArray_base<Alloc>::GetAutoArrayBufferUnsafe(size_t elemAlign) {
  
  

  void* autoBuf = &reinterpret_cast<nsAutoArrayBase<nsTArray<PRUint32>, 1>*>(this)->mAutoBuf;

  
  

  
  PR_STATIC_ASSERT(sizeof(void*) != 4 ||
                   (MOZ_ALIGNOF(mozilla::AlignedElem<8>) == 8 &&
                    sizeof(nsAutoTArray<mozilla::AlignedElem<8>, 1>) ==
                      sizeof(void*) + sizeof(nsTArrayHeader) +
                      4 + sizeof(mozilla::AlignedElem<8>)));

  
  NS_ABORT_IF_FALSE(elemAlign <= 4 || elemAlign == 8, "unsupported alignment.");
  if (sizeof(void*) == 4 && elemAlign == 8) {
    autoBuf = reinterpret_cast<char*>(autoBuf) + 4;
  }

  return reinterpret_cast<Header*>(autoBuf);
}

template<class Alloc>
bool nsTArray_base<Alloc>::UsesAutoArrayBuffer() {
  if (!mHdr->mIsAutoArray) {
    return PR_FALSE;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  PR_STATIC_ASSERT(sizeof(nsTArrayHeader) > 4);

#ifdef DEBUG
  PRPtrdiff diff = reinterpret_cast<char*>(GetAutoArrayBuffer(8)) -
                   reinterpret_cast<char*>(GetAutoArrayBuffer(4));
  NS_ABORT_IF_FALSE(diff >= 0 && diff <= 4, "GetAutoArrayBuffer doesn't do what we expect.");
#endif

  return mHdr == GetAutoArrayBuffer(4) || mHdr == GetAutoArrayBuffer(8);
}


template<class Alloc>
bool
nsTArray_base<Alloc>::EnsureCapacity(size_type capacity, size_type elemSize) {
  
  if (capacity <= mHdr->mCapacity)
    return PR_TRUE;

  
  
  
  
  
  if ((PRUint64)capacity * elemSize > size_type(-1)/2) {
    NS_ERROR("Attempting to allocate excessively large array");
    return PR_FALSE;
  }

  if (mHdr == EmptyHdr()) {
    
    Header *header = static_cast<Header*>
                     (Alloc::Malloc(sizeof(Header) + capacity * elemSize));
    if (!header)
      return PR_FALSE;
    header->mLength = 0;
    header->mCapacity = capacity;
    header->mIsAutoArray = 0;
    mHdr = header;

    return PR_TRUE;
  }

  
  
  
  const PRUint32 pageSizeBytes = 12;
  const PRUint32 pageSize = 1 << pageSizeBytes;

  PRUint32 minBytes = capacity * elemSize + sizeof(Header);
  PRUint32 bytesToAlloc;
  if (minBytes >= pageSize) {
    
    bytesToAlloc = pageSize * ((minBytes + pageSize - 1) / pageSize);
  }
  else {
    
    
    bytesToAlloc = minBytes - 1;
    bytesToAlloc |= bytesToAlloc >> 1;
    bytesToAlloc |= bytesToAlloc >> 2;
    bytesToAlloc |= bytesToAlloc >> 4;
    bytesToAlloc |= bytesToAlloc >> 8;
    bytesToAlloc |= bytesToAlloc >> 16;
    bytesToAlloc++;

    NS_ASSERTION((bytesToAlloc & (bytesToAlloc - 1)) == 0,
                 "nsTArray's allocation size should be a power of two!");
  }

  Header *header;
  if (UsesAutoArrayBuffer()) {
    
    header = static_cast<Header*>(Alloc::Malloc(bytesToAlloc));
    if (!header)
      return PR_FALSE;

    memcpy(header, mHdr, sizeof(Header) + Length() * elemSize);
  } else {
    
    header = static_cast<Header*>(Alloc::Realloc(mHdr, bytesToAlloc));
    if (!header)
      return PR_FALSE;
  }

  
  PRUint32 newCapacity = (bytesToAlloc - sizeof(Header)) / elemSize;
  NS_ASSERTION(newCapacity >= capacity, "Didn't enlarge the array enough!");
  header->mCapacity = newCapacity;

  mHdr = header;

  return PR_TRUE;
}

template<class Alloc>
void
nsTArray_base<Alloc>::ShrinkCapacity(size_type elemSize, size_t elemAlign) {
  if (mHdr == EmptyHdr() || UsesAutoArrayBuffer())
    return;

  if (mHdr->mLength >= mHdr->mCapacity)  
    return;

  size_type length = Length();

  if (IsAutoArray() && GetAutoArrayBuffer(elemAlign)->mCapacity >= length) {
    Header* header = GetAutoArrayBuffer(elemAlign);

    
    header->mLength = length;
    memcpy(header + 1, mHdr + 1, length * elemSize);

    Alloc::Free(mHdr);
    mHdr = header;
    return;
  }

  if (length == 0) {
    NS_ASSERTION(!IsAutoArray(), "autoarray should have fit 0 elements");
    Alloc::Free(mHdr);
    mHdr = EmptyHdr();
    return;
  }

  size_type size = sizeof(Header) + length * elemSize;
  void *ptr = Alloc::Realloc(mHdr, size);
  if (!ptr)
    return;
  mHdr = static_cast<Header*>(ptr);
  mHdr->mCapacity = length;
}

template<class Alloc>
void
nsTArray_base<Alloc>::ShiftData(index_type start,
                                size_type oldLen, size_type newLen,
                                size_type elemSize, size_t elemAlign) {
  if (oldLen == newLen)
    return;

  
  size_type num = mHdr->mLength - (start + oldLen);

  
  mHdr->mLength += newLen - oldLen;
  if (mHdr->mLength == 0) {
    ShrinkCapacity(elemSize, elemAlign);
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

template<class Alloc>
bool
nsTArray_base<Alloc>::InsertSlotsAt(index_type index, size_type count,
                                    size_type elementSize, size_t elemAlign)  {
  NS_ASSERTION(index <= Length(), "Bogus insertion index");
  size_type newLen = Length() + count;

  EnsureCapacity(newLen, elementSize);

  
  if (Capacity() < newLen)
    return PR_FALSE;

  
  
  ShiftData(index, 0, count, elementSize, elemAlign);
      
  return PR_TRUE;
}









template<class Alloc>
nsTArray_base<Alloc>::IsAutoArrayRestorer::IsAutoArrayRestorer(
  nsTArray_base<Alloc> &array,
  size_t elemAlign) 
  : mArray(array),
    mElemAlign(elemAlign),
    mIsAuto(array.IsAutoArray())
{
}

template<class Alloc>
nsTArray_base<Alloc>::IsAutoArrayRestorer::~IsAutoArrayRestorer() {
  
  if (mIsAuto && mArray.mHdr == mArray.EmptyHdr()) {
    
    
    mArray.mHdr = mArray.GetAutoArrayBufferUnsafe(mElemAlign);
    mArray.mHdr->mLength = 0;
  }
  else {
    mArray.mHdr->mIsAutoArray = mIsAuto;
  }
}

template<class Alloc>
template<class Allocator>
bool
nsTArray_base<Alloc>::SwapArrayElements(nsTArray_base<Allocator>& other,
                                        size_type elemSize,
                                        size_t elemAlign) {

  
  
  
  

  IsAutoArrayRestorer ourAutoRestorer(*this, elemAlign);
  typename nsTArray_base<Allocator>::IsAutoArrayRestorer otherAutoRestorer(other, elemAlign);

  
  
  
  if ((!UsesAutoArrayBuffer() || Capacity() < other.Length()) &&
      (!other.UsesAutoArrayBuffer() || other.Capacity() < Length())) {

    if (!EnsureNotUsingAutoArrayBuffer(elemSize) ||
        !other.EnsureNotUsingAutoArrayBuffer(elemSize)) {
      return PR_FALSE;
    }

    Header *temp = mHdr;
    mHdr = other.mHdr;
    other.mHdr = temp;

    return PR_TRUE;
  }

  
  
  
  
  
  
  
  
  
  

  if (!EnsureCapacity(other.Length(), elemSize) ||
      !other.EnsureCapacity(Length(), elemSize)) {
    return PR_FALSE;
  }

  
  
  NS_ABORT_IF_FALSE(UsesAutoArrayBuffer() ||
                    other.UsesAutoArrayBuffer(),
                    "One of the arrays should be using its auto buffer.");

  size_type smallerLength = NS_MIN(Length(), other.Length());
  size_type largerLength = NS_MAX(Length(), other.Length());
  void *smallerElements, *largerElements;
  if (Length() <= other.Length()) {
    smallerElements = Hdr() + 1;
    largerElements = other.Hdr() + 1;
  }
  else {
    smallerElements = other.Hdr() + 1;
    largerElements = Hdr() + 1;
  }

  
  
  
  
  
  nsAutoTArray<PRUint8, 8192, Alloc> temp;
  if (!temp.SetCapacity(smallerLength * elemSize)) {
    return PR_FALSE;
  }

  memcpy(temp.Elements(), smallerElements, smallerLength * elemSize);
  memcpy(smallerElements, largerElements, largerLength * elemSize);
  memcpy(largerElements, temp.Elements(), smallerLength * elemSize);

  
  NS_ABORT_IF_FALSE((other.Length() == 0 || mHdr != EmptyHdr()) &&
                    (Length() == 0 || other.mHdr != EmptyHdr()),
                    "Don't set sEmptyHdr's length.");
  size_type tempLength = Length();
  mHdr->mLength = other.Length();
  other.mHdr->mLength = tempLength;

  return PR_TRUE;
}

template<class Alloc>
bool
nsTArray_base<Alloc>::EnsureNotUsingAutoArrayBuffer(size_type elemSize) {
  if (UsesAutoArrayBuffer()) {

    
    
    
    
    if (Length() == 0) {
      mHdr = EmptyHdr();
      return PR_TRUE;
    }

    size_type size = sizeof(Header) + Length() * elemSize;

    Header* header = static_cast<Header*>(Alloc::Malloc(size));
    if (!header)
      return PR_FALSE;

    memcpy(header, mHdr, size);
    header->mCapacity = Length();
    mHdr = header;
  }
  
  return PR_TRUE;
}
