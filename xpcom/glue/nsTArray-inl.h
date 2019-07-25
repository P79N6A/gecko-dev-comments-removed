





































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
PRBool
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

  
  
  
  
  capacity = NS_MAX<size_type>(capacity, mHdr->mCapacity * 2U);

  Header *header;
  if (UsesAutoArrayBuffer()) {
    
    header = static_cast<Header*>
             (Alloc::Malloc(sizeof(Header) + capacity * elemSize));
    if (!header)
      return PR_FALSE;

    memcpy(header, mHdr, sizeof(Header) + Length() * elemSize);
  } else {
    
    size_type size = sizeof(Header) + capacity * elemSize;
    header = static_cast<Header*>(Alloc::Realloc(mHdr, size));
    if (!header)
      return PR_FALSE;
  }

  header->mCapacity = capacity;
  mHdr = header;

  return PR_TRUE;
}

template<class Alloc>
void
nsTArray_base<Alloc>::ShrinkCapacity(size_type elemSize) {
  if (mHdr == EmptyHdr() || UsesAutoArrayBuffer())
    return;

  if (mHdr->mLength >= mHdr->mCapacity)  
    return;

  size_type length = Length();

  if (IsAutoArray() && GetAutoArrayBuffer()->mCapacity >= length) {
    Header* header = GetAutoArrayBuffer();

    
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

template<class Alloc>
PRBool
nsTArray_base<Alloc>::InsertSlotsAt(index_type index, size_type count,
                                    size_type elementSize)  {
  NS_ASSERTION(index <= Length(), "Bogus insertion index");
  size_type newLen = Length() + count;

  EnsureCapacity(newLen, elementSize);

  
  if (Capacity() < newLen)
    return PR_FALSE;

  
  
  ShiftData(index, 0, count, elementSize);
      
  return PR_TRUE;
}

template<class Alloc>
template<class Allocator>
PRBool
nsTArray_base<Alloc>::SwapArrayElements(nsTArray_base<Allocator>& other,
                                        size_type elemSize) {
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
    if (other.mHdr == EmptyHdr()) {
      
      
      other.mHdr = GetAutoArrayBuffer();
      other.mHdr->mLength = 0;
    }
    else {
      other.mHdr->mIsAutoArray = 1;
    }
    mHdr->mIsAutoArray = 0;
  }
  else if (!IsAutoArray() && other.IsAutoArray()) {
    if (mHdr == EmptyHdr()) {
      
      
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

template<class Alloc>
PRBool
nsTArray_base<Alloc>::EnsureNotUsingAutoArrayBuffer(size_type elemSize) {
  if (UsesAutoArrayBuffer()) {
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
