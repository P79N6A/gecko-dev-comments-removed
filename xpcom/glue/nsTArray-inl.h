





#ifndef nsTArray_h__
#  error "Don't include this file directly"
#endif

template<class Alloc, class Copy>
nsTArray_base<Alloc, Copy>::nsTArray_base()
  : mHdr(EmptyHdr())
{
  MOZ_COUNT_CTOR(nsTArray_base);
}

template<class Alloc, class Copy>
nsTArray_base<Alloc, Copy>::~nsTArray_base()
{
  if (mHdr != EmptyHdr() && !UsesAutoArrayBuffer()) {
    Alloc::Free(mHdr);
  }
  MOZ_COUNT_DTOR(nsTArray_base);
}

template<class Alloc, class Copy>
const nsTArrayHeader*
nsTArray_base<Alloc, Copy>::GetAutoArrayBufferUnsafe(size_t aElemAlign) const
{
  
  

  const void* autoBuf =
    &reinterpret_cast<const nsAutoArrayBase<nsTArray<uint32_t>, 1>*>(this)->mAutoBuf;

  
  

  static_assert(sizeof(void*) != 4 ||
                (MOZ_ALIGNOF(mozilla::AlignedElem<8>) == 8 &&
                 sizeof(nsAutoTArray<mozilla::AlignedElem<8>, 1>) ==
                   sizeof(void*) + sizeof(nsTArrayHeader) +
                   4 + sizeof(mozilla::AlignedElem<8>)),
                "auto array padding wasn't what we expected");

  
  MOZ_ASSERT(aElemAlign <= 4 || aElemAlign == 8,
             "unsupported alignment.");
  if (sizeof(void*) == 4 && aElemAlign == 8) {
    autoBuf = reinterpret_cast<const char*>(autoBuf) + 4;
  }

  return reinterpret_cast<const Header*>(autoBuf);
}

template<class Alloc, class Copy>
bool
nsTArray_base<Alloc, Copy>::UsesAutoArrayBuffer() const
{
  if (!mHdr->mIsAutoArray) {
    return false;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  static_assert(sizeof(nsTArrayHeader) > 4,
                "see comment above");

#ifdef DEBUG
  ptrdiff_t diff = reinterpret_cast<const char*>(GetAutoArrayBuffer(8)) -
                   reinterpret_cast<const char*>(GetAutoArrayBuffer(4));
  MOZ_ASSERT(diff >= 0 && diff <= 4,
             "GetAutoArrayBuffer doesn't do what we expect.");
#endif

  return mHdr == GetAutoArrayBuffer(4) || mHdr == GetAutoArrayBuffer(8);
}


bool IsTwiceTheRequiredBytesRepresentableAsUint32(size_t aCapacity,
                                                  size_t aElemSize);

template<class Alloc, class Copy>
typename Alloc::ResultTypeProxy
nsTArray_base<Alloc, Copy>::EnsureCapacity(size_type aCapacity,
                                           size_type aElemSize)
{
  
  if (aCapacity <= mHdr->mCapacity) {
    return Alloc::SuccessResult();
  }

  
  
  
  
  
  if (!IsTwiceTheRequiredBytesRepresentableAsUint32(aCapacity, aElemSize)) {
    Alloc::SizeTooBig((size_t)aCapacity * aElemSize);
    return Alloc::FailureResult();
  }

  size_t reqSize = sizeof(Header) + aCapacity * aElemSize;

  if (mHdr == EmptyHdr()) {
    
    Header* header = static_cast<Header*>(Alloc::Malloc(reqSize));
    if (!header) {
      return Alloc::FailureResult();
    }
    header->mLength = 0;
    header->mCapacity = aCapacity;
    header->mIsAutoArray = 0;
    mHdr = header;

    return Alloc::SuccessResult();
  }

  
  
  
  
  const size_t slowGrowthThreshold = 8 * 1024 * 1024;

  size_t bytesToAlloc;
  if (reqSize >= slowGrowthThreshold) {
    size_t currSize = sizeof(Header) + Capacity() * aElemSize;
    size_t minNewSize = currSize + (currSize >> 3); 
    bytesToAlloc = reqSize > minNewSize ? reqSize : minNewSize;

    
    const size_t MiB = 1 << 20;
    bytesToAlloc = MiB * ((bytesToAlloc + MiB - 1) / MiB);
  } else {
    
    bytesToAlloc = mozilla::RoundUpPow2(reqSize);
  }

  Header* header;
  if (UsesAutoArrayBuffer() || !Copy::allowRealloc) {
    
    header = static_cast<Header*>(Alloc::Malloc(bytesToAlloc));
    if (!header) {
      return Alloc::FailureResult();
    }

    Copy::CopyHeaderAndElements(header, mHdr, Length(), aElemSize);

    if (!UsesAutoArrayBuffer()) {
      Alloc::Free(mHdr);
    }
  } else {
    
    header = static_cast<Header*>(Alloc::Realloc(mHdr, bytesToAlloc));
    if (!header) {
      return Alloc::FailureResult();
    }
  }

  
  size_t newCapacity = (bytesToAlloc - sizeof(Header)) / aElemSize;
  MOZ_ASSERT(newCapacity >= aCapacity, "Didn't enlarge the array enough!");
  header->mCapacity = newCapacity;

  mHdr = header;

  return Alloc::SuccessResult();
}

template<class Alloc, class Copy>
void
nsTArray_base<Alloc, Copy>::ShrinkCapacity(size_type aElemSize,
                                           size_t aElemAlign)
{
  if (mHdr == EmptyHdr() || UsesAutoArrayBuffer()) {
    return;
  }

  if (mHdr->mLength >= mHdr->mCapacity) { 
    return;
  }

  size_type length = Length();

  if (IsAutoArray() && GetAutoArrayBuffer(aElemAlign)->mCapacity >= length) {
    Header* header = GetAutoArrayBuffer(aElemAlign);

    
    header->mLength = length;
    Copy::CopyElements(header + 1, mHdr + 1, length, aElemSize);

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

  size_type size = sizeof(Header) + length * aElemSize;
  void* ptr = Alloc::Realloc(mHdr, size);
  if (!ptr) {
    return;
  }
  mHdr = static_cast<Header*>(ptr);
  mHdr->mCapacity = length;
}

template<class Alloc, class Copy>
void
nsTArray_base<Alloc, Copy>::ShiftData(index_type aStart,
                                      size_type aOldLen, size_type aNewLen,
                                      size_type aElemSize, size_t aElemAlign)
{
  if (aOldLen == aNewLen) {
    return;
  }

  
  size_type num = mHdr->mLength - (aStart + aOldLen);

  
  mHdr->mLength += aNewLen - aOldLen;
  if (mHdr->mLength == 0) {
    ShrinkCapacity(aElemSize, aElemAlign);
  } else {
    
    if (num == 0) {
      return;
    }
    
    aStart *= aElemSize;
    aNewLen *= aElemSize;
    aOldLen *= aElemSize;
    char* base = reinterpret_cast<char*>(mHdr + 1) + aStart;
    Copy::MoveElements(base + aNewLen, base + aOldLen, num, aElemSize);
  }
}

template<class Alloc, class Copy>
bool
nsTArray_base<Alloc, Copy>::InsertSlotsAt(index_type aIndex, size_type aCount,
                                          size_type aElemSize,
                                          size_t aElemAlign)
{
  MOZ_ASSERT(aIndex <= Length(), "Bogus insertion index");
  size_type newLen = Length() + aCount;

  EnsureCapacity(newLen, aElemSize);

  
  if (Capacity() < newLen) {
    return false;
  }

  
  
  ShiftData(aIndex, 0, aCount, aElemSize, aElemAlign);

  return true;
}









template<class Alloc, class Copy>
nsTArray_base<Alloc, Copy>::IsAutoArrayRestorer::IsAutoArrayRestorer(
      nsTArray_base<Alloc, Copy>& aArray,
      size_t aElemAlign)
  : mArray(aArray)
  , mElemAlign(aElemAlign)
  , mIsAuto(aArray.IsAutoArray())
{
}

template<class Alloc, class Copy>
nsTArray_base<Alloc, Copy>::IsAutoArrayRestorer::~IsAutoArrayRestorer()
{
  
  if (mIsAuto && mArray.mHdr == mArray.EmptyHdr()) {
    
    
    mArray.mHdr = mArray.GetAutoArrayBufferUnsafe(mElemAlign);
    mArray.mHdr->mLength = 0;
  } else if (mArray.mHdr != mArray.EmptyHdr()) {
    mArray.mHdr->mIsAutoArray = mIsAuto;
  }
}

template<class Alloc, class Copy>
template<class Allocator>
typename Alloc::ResultTypeProxy
nsTArray_base<Alloc, Copy>::SwapArrayElements(nsTArray_base<Allocator,
                                                            Copy>& aOther,
                                              size_type aElemSize,
                                              size_t aElemAlign)
{

  
  
  
  

  IsAutoArrayRestorer ourAutoRestorer(*this, aElemAlign);
  typename nsTArray_base<Allocator, Copy>::IsAutoArrayRestorer
    otherAutoRestorer(aOther, aElemAlign);

  
  
  
  if ((!UsesAutoArrayBuffer() || Capacity() < aOther.Length()) &&
      (!aOther.UsesAutoArrayBuffer() || aOther.Capacity() < Length())) {

    if (!EnsureNotUsingAutoArrayBuffer(aElemSize) ||
        !aOther.EnsureNotUsingAutoArrayBuffer(aElemSize)) {
      return Alloc::FailureResult();
    }

    Header* temp = mHdr;
    mHdr = aOther.mHdr;
    aOther.mHdr = temp;

    return Alloc::SuccessResult();
  }

  
  
  
  
  
  
  
  
  
  

  if (!Alloc::Successful(EnsureCapacity(aOther.Length(), aElemSize)) ||
      !Allocator::Successful(aOther.EnsureCapacity(Length(), aElemSize))) {
    return Alloc::FailureResult();
  }

  
  
  MOZ_ASSERT(UsesAutoArrayBuffer() || aOther.UsesAutoArrayBuffer(),
             "One of the arrays should be using its auto buffer.");

  size_type smallerLength = XPCOM_MIN(Length(), aOther.Length());
  size_type largerLength = XPCOM_MAX(Length(), aOther.Length());
  void* smallerElements;
  void* largerElements;
  if (Length() <= aOther.Length()) {
    smallerElements = Hdr() + 1;
    largerElements = aOther.Hdr() + 1;
  } else {
    smallerElements = aOther.Hdr() + 1;
    largerElements = Hdr() + 1;
  }

  
  
  
  
  
  nsAutoArrayBase<nsTArray_Impl<uint8_t, Alloc>, 64> temp;
  if (!Alloc::Successful(temp.EnsureCapacity(smallerLength, aElemSize))) {
    return Alloc::FailureResult();
  }

  Copy::CopyElements(temp.Elements(), smallerElements, smallerLength, aElemSize);
  Copy::CopyElements(smallerElements, largerElements, largerLength, aElemSize);
  Copy::CopyElements(largerElements, temp.Elements(), smallerLength, aElemSize);

  
  MOZ_ASSERT((aOther.Length() == 0 || mHdr != EmptyHdr()) &&
             (Length() == 0 || aOther.mHdr != EmptyHdr()),
             "Don't set sEmptyHdr's length.");
  size_type tempLength = Length();

  
  
  if (mHdr != EmptyHdr()) {
    mHdr->mLength = aOther.Length();
  }
  if (aOther.mHdr != EmptyHdr()) {
    aOther.mHdr->mLength = tempLength;
  }

  return Alloc::SuccessResult();
}

template<class Alloc, class Copy>
bool
nsTArray_base<Alloc, Copy>::EnsureNotUsingAutoArrayBuffer(size_type aElemSize)
{
  if (UsesAutoArrayBuffer()) {

    
    
    
    
    if (Length() == 0) {
      mHdr = EmptyHdr();
      return true;
    }

    size_type size = sizeof(Header) + Length() * aElemSize;

    Header* header = static_cast<Header*>(Alloc::Malloc(size));
    if (!header) {
      return false;
    }

    Copy::CopyHeaderAndElements(header, mHdr, Length(), aElemSize);
    header->mCapacity = Length();
    mHdr = header;
  }

  return true;
}
