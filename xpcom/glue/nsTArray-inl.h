





#ifndef nsTArray_h__
#  error "Don't include this file directly"
#endif

template<class Alloc, class Copy>
nsTArray_base<Alloc, Copy>::nsTArray_base()
  : mHdr(EmptyHdr()) {
  MOZ_COUNT_CTOR(nsTArray_base);
}

template<class Alloc, class Copy>
nsTArray_base<Alloc, Copy>::~nsTArray_base() {
  if (mHdr != EmptyHdr() && !UsesAutoArrayBuffer()) {
    Alloc::Free(mHdr);
  }
  MOZ_COUNT_DTOR(nsTArray_base);
}

template<class Alloc, class Copy>
const nsTArrayHeader* nsTArray_base<Alloc, Copy>::GetAutoArrayBufferUnsafe(size_t elemAlign) const {
  
  

  const void* autoBuf = &reinterpret_cast<const nsAutoArrayBase<nsTArray<uint32_t>, 1>*>(this)->mAutoBuf;

  
  

  static_assert(sizeof(void*) != 4 ||
                (MOZ_ALIGNOF(mozilla::AlignedElem<8>) == 8 &&
                 sizeof(nsAutoTArray<mozilla::AlignedElem<8>, 1>) ==
                   sizeof(void*) + sizeof(nsTArrayHeader) +
                   4 + sizeof(mozilla::AlignedElem<8>)),
                "auto array padding wasn't what we expected");

  
  NS_ABORT_IF_FALSE(elemAlign <= 4 || elemAlign == 8, "unsupported alignment.");
  if (sizeof(void*) == 4 && elemAlign == 8) {
    autoBuf = reinterpret_cast<const char*>(autoBuf) + 4;
  }

  return reinterpret_cast<const Header*>(autoBuf);
}

template<class Alloc, class Copy>
bool nsTArray_base<Alloc, Copy>::UsesAutoArrayBuffer() const {
  if (!mHdr->mIsAutoArray) {
    return false;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  static_assert(sizeof(nsTArrayHeader) > 4,
                "see comment above");

#ifdef DEBUG
  ptrdiff_t diff = reinterpret_cast<const char*>(GetAutoArrayBuffer(8)) -
                   reinterpret_cast<const char*>(GetAutoArrayBuffer(4));
  NS_ABORT_IF_FALSE(diff >= 0 && diff <= 4, "GetAutoArrayBuffer doesn't do what we expect.");
#endif

  return mHdr == GetAutoArrayBuffer(4) || mHdr == GetAutoArrayBuffer(8);
}


bool
IsTwiceTheRequiredBytesRepresentableAsUint32(size_t capacity, size_t elemSize);

template<class Alloc, class Copy>
void
nsTArray_base<Alloc, Copy>::GoodSizeForCapacity(size_t capacity,
                                                size_t elemSize,
                                                size_t& nbytes,
                                                size_t& newCapacity)
{
  
  
  
  
  const size_t pageSizeBytes = 12;
  const size_t pageSize = 1 << pageSizeBytes;

  nbytes = capacity * elemSize + sizeof(Header);
  if (nbytes >= pageSize) {
    
    nbytes = pageSize * ((nbytes + pageSize - 1) / pageSize);
  } else {
    
    
    nbytes = nbytes - 1;
    nbytes |= nbytes >> 1;
    nbytes |= nbytes >> 2;
    nbytes |= nbytes >> 4;
    nbytes |= nbytes >> 8;
    nbytes |= nbytes >> 16;
    nbytes++;

    MOZ_ASSERT((nbytes & (nbytes - 1)) == 0,
               "nsTArray's allocation size should be a power of two!");
  }

  
  newCapacity = (nbytes - sizeof(Header)) / elemSize;
  MOZ_ASSERT(newCapacity >= capacity, "Didn't enlarge the array enough!");
}

template<class Alloc, class Copy>
typename Alloc::ResultTypeProxy
nsTArray_base<Alloc, Copy>::EnsureCapacity(size_type capacity, size_type elemSize) {
  
  if (capacity <= mHdr->mCapacity)
    return Alloc::SuccessResult();

  
  
  
  
  
  if (!IsTwiceTheRequiredBytesRepresentableAsUint32(capacity, elemSize)) {
    Alloc::SizeTooBig((size_t)capacity * elemSize);
    return Alloc::FailureResult();
  }

  size_t nbytes, newCapacity;
  GoodSizeForCapacity(capacity, elemSize, nbytes, newCapacity);

  Header *header;
  if (mHdr == EmptyHdr()) {
    
    header = static_cast<Header*>(Alloc::Malloc(nbytes));
    if (!header) {
      return Alloc::FailureResult();
    }
    header->mLength = 0;
    header->mIsAutoArray = 0;

  } else if (UsesAutoArrayBuffer() || !Copy::allowRealloc) {
    
    header = static_cast<Header*>(Alloc::Malloc(nbytes));
    if (!header) {
      return Alloc::FailureResult();
    }
    Copy::CopyHeaderAndElements(header, mHdr, Length(), elemSize);

    if (!UsesAutoArrayBuffer()) {
      Alloc::Free(mHdr);
    }
  } else {
    
    header = static_cast<Header*>(Alloc::Realloc(mHdr, nbytes));
    if (!header) {
      return Alloc::FailureResult();
    }
  }

  header->mCapacity = newCapacity;

  mHdr = header;

  return Alloc::SuccessResult();
}

template<class Alloc, class Copy>
void
nsTArray_base<Alloc, Copy>::ShrinkCapacity(size_type elemSize, size_t elemAlign) {
  if (mHdr == EmptyHdr() || UsesAutoArrayBuffer())
    return;

  if (mHdr->mLength >= mHdr->mCapacity)  
    return;

  size_type length = Length();

  if (IsAutoArray() && GetAutoArrayBuffer(elemAlign)->mCapacity >= length) {
    Header* header = GetAutoArrayBuffer(elemAlign);

    
    header->mLength = length;
    Copy::CopyElements(header + 1, mHdr + 1, length, elemSize);

    Alloc::Free(mHdr);
    mHdr = header;
    return;
  }

  if (length == 0) {
    MOZ_ASSERT(!IsAutoArray(), "autoarray should have fit 0 elements");
    Alloc::Free(mHdr);
    mHdr = EmptyHdr();
    return;
  }

  
  size_t nbytes, newCapacity;
  GoodSizeForCapacity(length, elemSize, nbytes, newCapacity);
  if (newCapacity < Capacity()) {
    void *ptr = Alloc::Realloc(mHdr, nbytes);
    if (!ptr) {
      return;
    }
    mHdr = static_cast<Header*>(ptr);
    mHdr->mCapacity = newCapacity;
  }
}

template<class Alloc, class Copy>
void
nsTArray_base<Alloc, Copy>::ShiftData(index_type start,
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
    char *base = reinterpret_cast<char*>(mHdr + 1) + start;
    Copy::MoveElements(base + newLen, base + oldLen, num, elemSize);
  }
}

template<class Alloc, class Copy>
bool
nsTArray_base<Alloc, Copy>::InsertSlotsAt(index_type index, size_type count,
                                    size_type elementSize, size_t elemAlign)  {
  MOZ_ASSERT(index <= Length(), "Bogus insertion index");
  size_type newLen = Length() + count;

  EnsureCapacity(newLen, elementSize);

  
  if (Capacity() < newLen)
    return false;

  
  
  ShiftData(index, 0, count, elementSize, elemAlign);

  return true;
}









template<class Alloc, class Copy>
nsTArray_base<Alloc, Copy>::IsAutoArrayRestorer::IsAutoArrayRestorer(
  nsTArray_base<Alloc, Copy> &array,
  size_t elemAlign)
  : mArray(array),
    mElemAlign(elemAlign),
    mIsAuto(array.IsAutoArray())
{
}

template<class Alloc, class Copy>
nsTArray_base<Alloc, Copy>::IsAutoArrayRestorer::~IsAutoArrayRestorer() {
  
  if (mIsAuto && mArray.mHdr == mArray.EmptyHdr()) {
    
    
    mArray.mHdr = mArray.GetAutoArrayBufferUnsafe(mElemAlign);
    mArray.mHdr->mLength = 0;
  }
  else if (mArray.mHdr != mArray.EmptyHdr()) {
    mArray.mHdr->mIsAutoArray = mIsAuto;
  }
}

template<class Alloc, class Copy>
template<class Allocator>
typename Alloc::ResultTypeProxy
nsTArray_base<Alloc, Copy>::SwapArrayElements(nsTArray_base<Allocator, Copy>& other,
                                              size_type elemSize, size_t elemAlign) {

  
  
  
  

  IsAutoArrayRestorer ourAutoRestorer(*this, elemAlign);
  typename nsTArray_base<Allocator, Copy>::IsAutoArrayRestorer otherAutoRestorer(other, elemAlign);

  
  
  
  if ((!UsesAutoArrayBuffer() || Capacity() < other.Length()) &&
      (!other.UsesAutoArrayBuffer() || other.Capacity() < Length())) {

    if (!EnsureNotUsingAutoArrayBuffer(elemSize) ||
        !other.EnsureNotUsingAutoArrayBuffer(elemSize)) {
      return Alloc::FailureResult();
    }

    Header *temp = mHdr;
    mHdr = other.mHdr;
    other.mHdr = temp;

    return Alloc::SuccessResult();
  }

  
  
  
  
  
  
  
  
  
  

  if (!Alloc::Successful(EnsureCapacity(other.Length(), elemSize)) ||
      !Allocator::Successful(other.EnsureCapacity(Length(), elemSize))) {
    return Alloc::FailureResult();
  }

  
  
  NS_ABORT_IF_FALSE(UsesAutoArrayBuffer() ||
                    other.UsesAutoArrayBuffer(),
                    "One of the arrays should be using its auto buffer.");

  size_type smallerLength = XPCOM_MIN(Length(), other.Length());
  size_type largerLength = XPCOM_MAX(Length(), other.Length());
  void *smallerElements, *largerElements;
  if (Length() <= other.Length()) {
    smallerElements = Hdr() + 1;
    largerElements = other.Hdr() + 1;
  }
  else {
    smallerElements = other.Hdr() + 1;
    largerElements = Hdr() + 1;
  }

  
  
  
  
  
  nsAutoArrayBase<nsTArray_Impl<uint8_t, Alloc>, 64> temp;
  if (!Alloc::Successful(temp.EnsureCapacity(smallerLength, elemSize))) {
    return Alloc::FailureResult();
  }

  Copy::CopyElements(temp.Elements(), smallerElements, smallerLength, elemSize);
  Copy::CopyElements(smallerElements, largerElements, largerLength, elemSize);
  Copy::CopyElements(largerElements, temp.Elements(), smallerLength, elemSize);

  
  NS_ABORT_IF_FALSE((other.Length() == 0 || mHdr != EmptyHdr()) &&
                    (Length() == 0 || other.mHdr != EmptyHdr()),
                    "Don't set sEmptyHdr's length.");
  size_type tempLength = Length();
  mHdr->mLength = other.Length();
  other.mHdr->mLength = tempLength;

  return Alloc::SuccessResult();
}

template<class Alloc, class Copy>
bool
nsTArray_base<Alloc, Copy>::EnsureNotUsingAutoArrayBuffer(size_type elemSize) {
  if (UsesAutoArrayBuffer()) {

    
    
    
    
    if (Length() == 0) {
      mHdr = EmptyHdr();
      return true;
    }

    size_type size = sizeof(Header) + Length() * elemSize;

    Header* header = static_cast<Header*>(Alloc::Malloc(size));
    if (!header)
      return false;

    Copy::CopyHeaderAndElements(header, mHdr, Length(), elemSize);
    header->mCapacity = Length();
    mHdr = header;
  }

  return true;
}
