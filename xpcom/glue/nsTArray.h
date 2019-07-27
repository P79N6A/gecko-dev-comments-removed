





#ifndef nsTArray_h__
#define nsTArray_h__

#include "nsTArrayForwardDeclare.h"
#include "mozilla/Alignment.h"
#include "mozilla/Assertions.h"
#include "mozilla/BinarySearch.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Move.h"
#include "mozilla/ReverseIterator.h"
#include "mozilla/TypeTraits.h"

#include <string.h>

#include "nsCycleCollectionNoteChild.h"
#include "nsAlgorithm.h"
#include "nscore.h"
#include "nsQuickSort.h"
#include "nsDebug.h"
#include "nsISupportsImpl.h"
#include <new>

namespace JS {
template<class T>
class Heap;
} 

class nsRegion;
class nsIntRegion;
namespace mozilla {
namespace layers {
struct TileClient;
}
}




































































struct nsTArrayFallibleResult
{
  
  MOZ_IMPLICIT nsTArrayFallibleResult(bool aResult) : mResult(aResult) {}

  MOZ_IMPLICIT operator bool() { return mResult; }

private:
  bool mResult;
};

struct nsTArrayInfallibleResult
{
};






struct nsTArrayFallibleAllocatorBase
{
  typedef bool ResultType;
  typedef nsTArrayFallibleResult ResultTypeProxy;

  static ResultType Result(ResultTypeProxy aResult) { return aResult; }
  static bool Successful(ResultTypeProxy aResult) { return aResult; }
  static ResultTypeProxy SuccessResult() { return true; }
  static ResultTypeProxy FailureResult() { return false; }
  static ResultType ConvertBoolToResultType(bool aValue) { return aValue; }
};

struct nsTArrayInfallibleAllocatorBase
{
  typedef void ResultType;
  typedef nsTArrayInfallibleResult ResultTypeProxy;

  static ResultType Result(ResultTypeProxy aResult) {}
  static bool Successful(ResultTypeProxy) { return true; }
  static ResultTypeProxy SuccessResult() { return ResultTypeProxy(); }

  static ResultTypeProxy FailureResult()
  {
    NS_RUNTIMEABORT("Infallible nsTArray should never fail");
    return ResultTypeProxy();
  }

  static ResultType ConvertBoolToResultType(bool aValue)
  {
    if (!aValue) {
      NS_RUNTIMEABORT("infallible nsTArray should never convert false to ResultType");
    }
  }
};

#if defined(MOZALLOC_HAVE_XMALLOC)
#include "mozilla/mozalloc_abort.h"

struct nsTArrayFallibleAllocator : nsTArrayFallibleAllocatorBase
{
  static void* Malloc(size_t aSize) { return malloc(aSize); }
  static void* Realloc(void* aPtr, size_t aSize)
  {
    return realloc(aPtr, aSize);
  }

  static void Free(void* aPtr) { free(aPtr); }
  static void SizeTooBig(size_t) {}
};

struct nsTArrayInfallibleAllocator : nsTArrayInfallibleAllocatorBase
{
  static void* Malloc(size_t aSize) { return moz_xmalloc(aSize); }
  static void* Realloc(void* aPtr, size_t aSize)
  {
    return moz_xrealloc(aPtr, aSize);
  }

  static void Free(void* aPtr) { free(aPtr); }
  static void SizeTooBig(size_t aSize) { NS_ABORT_OOM(aSize); }
};

#else
#include <stdlib.h>

struct nsTArrayFallibleAllocator : nsTArrayFallibleAllocatorBase
{
  static void* Malloc(size_t aSize) { return malloc(aSize); }
  static void* Realloc(void* aPtr, size_t aSize) { return realloc(aPtr, aSize); }

  static void Free(void* aPtr) { free(aPtr); }
  static void SizeTooBig(size_t) {}
};

struct nsTArrayInfallibleAllocator : nsTArrayInfallibleAllocatorBase
{
  static void* Malloc(size_t aSize)
  {
    void* ptr = malloc(aSize);
    if (MOZ_UNLIKELY(!ptr)) {
      NS_ABORT_OOM(aSize);
    }
    return ptr;
  }

  static void* Realloc(void* aPtr, size_t aSize)
  {
    void* newptr = realloc(aPtr, aSize);
    if (MOZ_UNLIKELY(!newptr && aSize)) {
      NS_ABORT_OOM(aSize);
    }
    return newptr;
  }

  static void Free(void* aPtr) { free(aPtr); }
  static void SizeTooBig(size_t aSize) { NS_ABORT_OOM(aSize); }
};

#endif




struct nsTArrayHeader
{
  static nsTArrayHeader sEmptyHdr;

  uint32_t mLength;
  uint32_t mCapacity : 31;
  uint32_t mIsAutoArray : 1;
};



template<class E, class Derived>
struct nsTArray_SafeElementAtHelper
{
  typedef E*       elem_type;
  typedef size_t   index_type;

  
  
  
  elem_type& SafeElementAt(index_type aIndex);
  const elem_type& SafeElementAt(index_type aIndex) const;
};

template<class E, class Derived>
struct nsTArray_SafeElementAtHelper<E*, Derived>
{
  typedef E*       elem_type;
  typedef size_t   index_type;

  elem_type SafeElementAt(index_type aIndex)
  {
    return static_cast<Derived*>(this)->SafeElementAt(aIndex, nullptr);
  }

  const elem_type SafeElementAt(index_type aIndex) const
  {
    return static_cast<const Derived*>(this)->SafeElementAt(aIndex, nullptr);
  }
};



template<class E, class Derived>
struct nsTArray_SafeElementAtSmartPtrHelper
{
  typedef E*       elem_type;
  typedef size_t   index_type;

  elem_type SafeElementAt(index_type aIndex)
  {
    return static_cast<Derived*>(this)->SafeElementAt(aIndex, nullptr);
  }

  const elem_type SafeElementAt(index_type aIndex) const
  {
    return static_cast<const Derived*>(this)->SafeElementAt(aIndex, nullptr);
  }
};

template<class T> class nsCOMPtr;

template<class E, class Derived>
struct nsTArray_SafeElementAtHelper<nsCOMPtr<E>, Derived>
  : public nsTArray_SafeElementAtSmartPtrHelper<E, Derived>
{
};

template<class T> class nsRefPtr;

template<class E, class Derived>
struct nsTArray_SafeElementAtHelper<nsRefPtr<E>, Derived>
  : public nsTArray_SafeElementAtSmartPtrHelper<E, Derived>
{
};

namespace mozilla {
namespace dom {
template<class T> class OwningNonNull;
}
}

template<class E, class Derived>
struct nsTArray_SafeElementAtHelper<mozilla::dom::OwningNonNull<E>, Derived>
{
  typedef E*     elem_type;
  typedef size_t index_type;

  elem_type SafeElementAt(index_type aIndex)
  {
    if (aIndex < static_cast<Derived*>(this)->Length()) {
      return static_cast<Derived*>(this)->ElementAt(aIndex);
    }
    return nullptr;
  }

  const elem_type SafeElementAt(index_type aIndex) const
  {
    if (aIndex < static_cast<const Derived*>(this)->Length()) {
      return static_cast<const Derived*>(this)->ElementAt(aIndex);
    }
    return nullptr;
  }
};






template<class Alloc, class Copy>
class nsTArray_base
{
  
  
  
  template<class Allocator, class Copier>
  friend class nsTArray_base;

protected:
  typedef nsTArrayHeader Header;

public:
  typedef size_t size_type;
  typedef size_t index_type;

  
  size_type Length() const { return mHdr->mLength; }

  
  bool IsEmpty() const { return Length() == 0; }

  
  
  
  size_type Capacity() const {  return mHdr->mCapacity; }

#ifdef DEBUG
  void* DebugGetHeader() const { return mHdr; }
#endif

protected:
  nsTArray_base();

  ~nsTArray_base();

  
  
  
  
  typename Alloc::ResultTypeProxy EnsureCapacity(size_type aCapacity,
                                                 size_type aElemSize);

  
  
  
  void ShrinkCapacity(size_type aElemSize, size_t aElemAlign);

  
  
  
  
  
  
  
  
  void ShiftData(index_type aStart, size_type aOldLen, size_type aNewLen,
                 size_type aElemSize, size_t aElemAlign);

  
  
  
  
  void IncrementLength(size_t aNum)
  {
    if (mHdr == EmptyHdr()) {
      if (MOZ_UNLIKELY(aNum != 0)) {
        
        MOZ_CRASH();
      }
    } else {
      mHdr->mLength += aNum;
    }
  }

  
  
  
  
  
  
  bool InsertSlotsAt(index_type aIndex, size_type aCount,
                     size_type aElementSize, size_t aElemAlign);

protected:
  template<class Allocator>
  typename Alloc::ResultTypeProxy
  SwapArrayElements(nsTArray_base<Allocator, Copy>& aOther,
                    size_type aElemSize,
                    size_t aElemAlign);

  
  class IsAutoArrayRestorer
  {
  public:
    IsAutoArrayRestorer(nsTArray_base<Alloc, Copy>& aArray, size_t aElemAlign);
    ~IsAutoArrayRestorer();

  private:
    nsTArray_base<Alloc, Copy>& mArray;
    size_t mElemAlign;
    bool mIsAuto;
  };

  
  
  bool EnsureNotUsingAutoArrayBuffer(size_type aElemSize);

  
  bool IsAutoArray() const { return mHdr->mIsAutoArray; }

  
  Header* GetAutoArrayBuffer(size_t aElemAlign)
  {
    MOZ_ASSERT(IsAutoArray(), "Should be an auto array to call this");
    return GetAutoArrayBufferUnsafe(aElemAlign);
  }
  const Header* GetAutoArrayBuffer(size_t aElemAlign) const
  {
    MOZ_ASSERT(IsAutoArray(), "Should be an auto array to call this");
    return GetAutoArrayBufferUnsafe(aElemAlign);
  }

  
  
  Header* GetAutoArrayBufferUnsafe(size_t aElemAlign)
  {
    return const_cast<Header*>(static_cast<const nsTArray_base<Alloc, Copy>*>(
      this)->GetAutoArrayBufferUnsafe(aElemAlign));
  }
  const Header* GetAutoArrayBufferUnsafe(size_t aElemAlign) const;

  
  
  bool UsesAutoArrayBuffer() const;

  
  
  Header* mHdr;

  Header* Hdr() const { return mHdr; }
  Header** PtrToHdr() { return &mHdr; }
  static Header* EmptyHdr() { return &Header::sEmptyHdr; }
};





template<class E>
class nsTArrayElementTraits
{
public:
  
  static inline void Construct(E* aE)
  {
    
    
    
    
    
    new (static_cast<void*>(aE)) E;
  }
  
  template<class A>
  static inline void Construct(E* aE, A&& aArg)
  {
    typedef typename mozilla::RemoveCV<E>::Type E_NoCV;
    typedef typename mozilla::RemoveCV<A>::Type A_NoCV;
    static_assert(!mozilla::IsSame<E_NoCV*, A_NoCV>::value,
                  "For safety, we disallow constructing nsTArray<E> elements "
                  "from E* pointers. See bug 960591.");
    new (static_cast<void*>(aE)) E(mozilla::Forward<A>(aArg));
  }
  
  static inline void Destruct(E* aE) { aE->~E(); }
};


template<class A, class B>
class nsDefaultComparator
{
public:
  bool Equals(const A& aA, const B& aB) const { return aA == aB; }
  bool LessThan(const A& aA, const B& aB) const { return aA < aB; }
};

template<class E> class InfallibleTArray;
template<class E> class FallibleTArray;

template<bool IsPod, bool IsSameType>
struct AssignRangeAlgorithm
{
  template<class Item, class ElemType, class IndexType, class SizeType>
  static void implementation(ElemType* aElements, IndexType aStart,
                             SizeType aCount, const Item* aValues)
  {
    ElemType* iter = aElements + aStart;
    ElemType* end = iter + aCount;
    for (; iter != end; ++iter, ++aValues) {
      nsTArrayElementTraits<ElemType>::Construct(iter, *aValues);
    }
  }
};

template<>
struct AssignRangeAlgorithm<true, true>
{
  template<class Item, class ElemType, class IndexType, class SizeType>
  static void implementation(ElemType* aElements, IndexType aStart,
                             SizeType aCount, const Item* aValues)
  {
    memcpy(aElements + aStart, aValues, aCount * sizeof(ElemType));
  }
};











struct nsTArray_CopyWithMemutils
{
  const static bool allowRealloc = true;

  static void CopyElements(void* aDest, const void* aSrc, size_t aCount,
                           size_t aElemSize)
  {
    memcpy(aDest, aSrc, aCount * aElemSize);
  }

  static void CopyHeaderAndElements(void* aDest, const void* aSrc,
                                    size_t aCount, size_t aElemSize)
  {
    memcpy(aDest, aSrc, sizeof(nsTArrayHeader) + aCount * aElemSize);
  }

  static void MoveElements(void* aDest, const void* aSrc, size_t aCount,
                           size_t aElemSize)
  {
    memmove(aDest, aSrc, aCount * aElemSize);
  }
};





template<class ElemType>
struct nsTArray_CopyWithConstructors
{
  typedef nsTArrayElementTraits<ElemType> traits;

  const static bool allowRealloc = false;

  static void CopyElements(void* aDest, void* aSrc, size_t aCount,
                           size_t aElemSize)
  {
    ElemType* destElem = static_cast<ElemType*>(aDest);
    ElemType* srcElem = static_cast<ElemType*>(aSrc);
    ElemType* destElemEnd = destElem + aCount;
#ifdef DEBUG
    ElemType* srcElemEnd = srcElem + aCount;
    MOZ_ASSERT(srcElemEnd <= destElem || srcElemEnd > destElemEnd);
#endif
    while (destElem != destElemEnd) {
      traits::Construct(destElem, *srcElem);
      traits::Destruct(srcElem);
      ++destElem;
      ++srcElem;
    }
  }

  static void CopyHeaderAndElements(void* aDest, void* aSrc, size_t aCount,
                                    size_t aElemSize)
  {
    nsTArrayHeader* destHeader = static_cast<nsTArrayHeader*>(aDest);
    nsTArrayHeader* srcHeader = static_cast<nsTArrayHeader*>(aSrc);
    *destHeader = *srcHeader;
    CopyElements(static_cast<uint8_t*>(aDest) + sizeof(nsTArrayHeader),
                 static_cast<uint8_t*>(aSrc) + sizeof(nsTArrayHeader),
                 aCount, aElemSize);
  }

  static void MoveElements(void* aDest, void* aSrc, size_t aCount,
                           size_t aElemSize)
  {
    ElemType* destElem = static_cast<ElemType*>(aDest);
    ElemType* srcElem = static_cast<ElemType*>(aSrc);
    ElemType* destElemEnd = destElem + aCount;
    ElemType* srcElemEnd = srcElem + aCount;
    if (destElem == srcElem) {
      return;  
    } else if (srcElemEnd > destElem && srcElemEnd < destElemEnd) {
      while (destElemEnd != destElem) {
        --destElemEnd;
        --srcElemEnd;
        traits::Construct(destElemEnd, *srcElemEnd);
        traits::Destruct(srcElem);
      }
    } else {
      CopyElements(aDest, aSrc, aCount, aElemSize);
    }
  }
};




template<class E>
struct nsTArray_CopyChooser
{
  typedef nsTArray_CopyWithMemutils Type;
};





template<class E>
struct nsTArray_CopyChooser<JS::Heap<E>>
{
  typedef nsTArray_CopyWithConstructors<JS::Heap<E>> Type;
};

template<>
struct nsTArray_CopyChooser<nsRegion>
{
  typedef nsTArray_CopyWithConstructors<nsRegion> Type;
};

template<>
struct nsTArray_CopyChooser<nsIntRegion>
{
  typedef nsTArray_CopyWithConstructors<nsIntRegion> Type;
};

template<>
struct nsTArray_CopyChooser<mozilla::layers::TileClient>
{
  typedef nsTArray_CopyWithConstructors<mozilla::layers::TileClient> Type;
};







template<class E, class Derived>
struct nsTArray_TypedBase : public nsTArray_SafeElementAtHelper<E, Derived>
{
};












template<class E, class Derived>
struct nsTArray_TypedBase<JS::Heap<E>, Derived>
  : public nsTArray_SafeElementAtHelper<JS::Heap<E>, Derived>
{
  operator const nsTArray<E>&()
  {
    static_assert(sizeof(E) == sizeof(JS::Heap<E>),
                  "JS::Heap<E> must be binary compatible with E.");
    Derived* self = static_cast<Derived*>(this);
    return *reinterpret_cast<nsTArray<E> *>(self);
  }

  operator const FallibleTArray<E>&()
  {
    Derived* self = static_cast<Derived*>(this);
    return *reinterpret_cast<FallibleTArray<E> *>(self);
  }
};

namespace detail {

template<class Item, class Comparator>
struct ItemComparatorEq
{
  const Item& mItem;
  const Comparator& mComp;
  ItemComparatorEq(const Item& aItem, const Comparator& aComp)
    : mItem(aItem)
    , mComp(aComp)
  {}
  template<class T>
  int operator()(const T& aElement) const {
    if (mComp.Equals(aElement, mItem)) {
      return 0;
    }

    return mComp.LessThan(aElement, mItem) ? 1 : -1;
  }
};

template<class Item, class Comparator>
struct ItemComparatorFirstElementGT
{
  const Item& mItem;
  const Comparator& mComp;
  ItemComparatorFirstElementGT(const Item& aItem, const Comparator& aComp)
    : mItem(aItem)
    , mComp(aComp)
  {}
  template<class T>
  int operator()(const T& aElement) const {
    if (mComp.LessThan(aElement, mItem) ||
        mComp.Equals(aElement, mItem)) {
      return 1;
    } else {
      return -1;
    }
  }
};

} 













template<class E, class Alloc>
class nsTArray_Impl
  : public nsTArray_base<Alloc, typename nsTArray_CopyChooser<E>::Type>
  , public nsTArray_TypedBase<E, nsTArray_Impl<E, Alloc>>
{
public:
  typedef typename nsTArray_CopyChooser<E>::Type     copy_type;
  typedef nsTArray_base<Alloc, copy_type>            base_type;
  typedef typename base_type::size_type              size_type;
  typedef typename base_type::index_type             index_type;
  typedef E                                          elem_type;
  typedef nsTArray_Impl<E, Alloc>                    self_type;
  typedef nsTArrayElementTraits<E>                   elem_traits;
  typedef nsTArray_SafeElementAtHelper<E, self_type> safeelementat_helper_type;
  typedef elem_type*                                 iterator;
  typedef const elem_type*                           const_iterator;
  typedef mozilla::ReverseIterator<elem_type*>       reverse_iterator;
  typedef mozilla::ReverseIterator<const elem_type*> const_reverse_iterator;

  using safeelementat_helper_type::SafeElementAt;
  using base_type::EmptyHdr;

  
  
  static const index_type NoIndex = index_type(-1);

  using base_type::Length;

  
  
  

  ~nsTArray_Impl() { Clear(); }

  
  
  

  nsTArray_Impl() {}

  
  explicit nsTArray_Impl(size_type aCapacity) { SetCapacity(aCapacity); }

  
  
  template<typename Allocator>
  explicit nsTArray_Impl(nsTArray_Impl<E, Allocator>&& aOther)
  {
    SwapElements(aOther);
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  explicit nsTArray_Impl(const self_type& aOther) { AppendElements(aOther); }

  
  
  template<typename Allocator>
  operator const nsTArray_Impl<E, Allocator>&() const
  {
    return *reinterpret_cast<const nsTArray_Impl<E, Allocator>*>(this);
  }
  
  operator const nsTArray<E>&() const
  {
    return *reinterpret_cast<const InfallibleTArray<E>*>(this);
  }
  operator const FallibleTArray<E>&() const
  {
    return *reinterpret_cast<const FallibleTArray<E>*>(this);
  }

  
  
  
  self_type& operator=(const self_type& aOther)
  {
    ReplaceElementsAt(0, Length(), aOther.Elements(), aOther.Length());
    return *this;
  }

  
  
  
  self_type& operator=(self_type&& aOther)
  {
    Clear();
    SwapElements(aOther);
    return *this;
  }

  
  
  template<typename Allocator>
  bool operator==(const nsTArray_Impl<E, Allocator>& aOther) const
  {
    size_type len = Length();
    if (len != aOther.Length()) {
      return false;
    }

    
    for (index_type i = 0; i < len; ++i) {
      if (!(operator[](i) == aOther[i])) {
        return false;
      }
    }

    return true;
  }

  
  
  bool operator!=(const self_type& aOther) const { return !operator==(aOther); }

  template<typename Allocator>
  self_type& operator=(const nsTArray_Impl<E, Allocator>& aOther)
  {
    ReplaceElementsAt(0, Length(), aOther.Elements(), aOther.Length());
    return *this;
  }

  template<typename Allocator>
  self_type& operator=(nsTArray_Impl<E, Allocator>&& aOther)
  {
    Clear();
    SwapElements(aOther);
    return *this;
  }

  
  
  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    if (this->UsesAutoArrayBuffer() || Hdr() == EmptyHdr()) {
      return 0;
    }
    return aMallocSizeOf(this->Hdr());
  }

  
  
  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  
  
  

  
  
  
  elem_type* Elements() { return reinterpret_cast<elem_type*>(Hdr() + 1); }

  
  
  
  const elem_type* Elements() const
  {
    return reinterpret_cast<const elem_type*>(Hdr() + 1);
  }

  
  
  
  
  elem_type& ElementAt(index_type aIndex)
  {
    MOZ_ASSERT(aIndex < Length(), "invalid array index");
    return Elements()[aIndex];
  }

  
  
  
  
  const elem_type& ElementAt(index_type aIndex) const
  {
    MOZ_ASSERT(aIndex < Length(), "invalid array index");
    return Elements()[aIndex];
  }

  
  
  
  
  
  elem_type& SafeElementAt(index_type aIndex, elem_type& aDef)
  {
    return aIndex < Length() ? Elements()[aIndex] : aDef;
  }

  
  
  
  
  
  const elem_type& SafeElementAt(index_type aIndex, const elem_type& aDef) const
  {
    return aIndex < Length() ? Elements()[aIndex] : aDef;
  }

  
  elem_type& operator[](index_type aIndex) { return ElementAt(aIndex); }

  
  const elem_type& operator[](index_type aIndex) const { return ElementAt(aIndex); }

  
  elem_type& LastElement() { return ElementAt(Length() - 1); }

  
  const elem_type& LastElement() const { return ElementAt(Length() - 1); }

  
  elem_type& SafeLastElement(elem_type& aDef)
  {
    return SafeElementAt(Length() - 1, aDef);
  }

  
  const elem_type& SafeLastElement(const elem_type& aDef) const
  {
    return SafeElementAt(Length() - 1, aDef);
  }

  
  iterator begin() { return Elements(); }
  const_iterator begin() const { return Elements(); }
  const_iterator cbegin() const { return begin(); }
  iterator end() { return Elements() + Length(); }
  const_iterator end() const { return Elements() + Length(); }
  const_iterator cend() const { return end(); }

  
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  const_reverse_iterator crbegin() const { return rbegin(); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
  const_reverse_iterator crend() const { return rend(); }

  
  
  

  
  
  
  
  
  template<class Item, class Comparator>
  bool Contains(const Item& aItem, const Comparator& aComp) const
  {
    return IndexOf(aItem, 0, aComp) != NoIndex;
  }

  
  
  
  
  
  template<class Item>
  bool Contains(const Item& aItem) const
  {
    return IndexOf(aItem) != NoIndex;
  }

  
  
  
  
  
  
  template<class Item, class Comparator>
  index_type IndexOf(const Item& aItem, index_type aStart,
                     const Comparator& aComp) const
  {
    const elem_type* iter = Elements() + aStart;
    const elem_type* iend = Elements() + Length();
    for (; iter != iend; ++iter) {
      if (aComp.Equals(*iter, aItem)) {
        return index_type(iter - Elements());
      }
    }
    return NoIndex;
  }

  
  
  
  
  
  
  template<class Item>
  index_type IndexOf(const Item& aItem, index_type aStart = 0) const
  {
    return IndexOf(aItem, aStart, nsDefaultComparator<elem_type, Item>());
  }

  
  
  
  
  
  
  
  template<class Item, class Comparator>
  index_type LastIndexOf(const Item& aItem, index_type aStart,
                         const Comparator& aComp) const
  {
    size_type endOffset = aStart >= Length() ? Length() : aStart + 1;
    const elem_type* iend = Elements() - 1;
    const elem_type* iter = iend + endOffset;
    for (; iter != iend; --iter) {
      if (aComp.Equals(*iter, aItem)) {
        return index_type(iter - Elements());
      }
    }
    return NoIndex;
  }

  
  
  
  
  
  
  
  template<class Item>
  index_type LastIndexOf(const Item& aItem,
                         index_type aStart = NoIndex) const
  {
    return LastIndexOf(aItem, aStart, nsDefaultComparator<elem_type, Item>());
  }

  
  
  
  
  
  
  
  template<class Item, class Comparator>
  index_type BinaryIndexOf(const Item& aItem, const Comparator& aComp) const
  {
    using mozilla::BinarySearchIf;
    typedef ::detail::ItemComparatorEq<Item, Comparator> Cmp;

    size_t index;
    bool found = BinarySearchIf(*this, 0, Length(), Cmp(aItem, aComp), &index);
    return found ? index : NoIndex;
  }

  
  
  
  
  
  template<class Item>
  index_type BinaryIndexOf(const Item& aItem) const
  {
    return BinaryIndexOf(aItem, nsDefaultComparator<elem_type, Item>());
  }

  
  
  
  
  
  
  
  
  void ClearAndRetainStorage()
  {
    if (base_type::mHdr == EmptyHdr()) {
      return;
    }

    DestructRange(0, Length());
    base_type::mHdr->mLength = 0;
  }

  
  
  
  
  
  
  
  
  void SetLengthAndRetainStorage(size_type aNewLen)
  {
    MOZ_ASSERT(aNewLen <= base_type::Capacity());
    size_type oldLen = Length();
    if (aNewLen > oldLen) {
      InsertElementsAt(oldLen, aNewLen - oldLen);
      return;
    }
    if (aNewLen < oldLen) {
      DestructRange(aNewLen, oldLen - aNewLen);
      base_type::mHdr->mLength = aNewLen;
    }
  }

  
  
  
  
  
  
  
  
  
  
  template<class Item>
  elem_type* ReplaceElementsAt(index_type aStart, size_type aCount,
                               const Item* aArray, size_type aArrayLen)
  {
    
    if (!Alloc::Successful(this->EnsureCapacity(Length() + aArrayLen - aCount,
                                                sizeof(elem_type)))) {
      return nullptr;
    }
    DestructRange(aStart, aCount);
    this->ShiftData(aStart, aCount, aArrayLen,
                    sizeof(elem_type), MOZ_ALIGNOF(elem_type));
    AssignRange(aStart, aArrayLen, aArray);
    return Elements() + aStart;
  }

  
  template<class Item>
  elem_type* ReplaceElementsAt(index_type aStart, size_type aCount,
                               const nsTArray<Item>& aArray)
  {
    return ReplaceElementsAt(aStart, aCount, aArray.Elements(), aArray.Length());
  }

  
  template<class Item>
  elem_type* ReplaceElementsAt(index_type aStart, size_type aCount,
                               const Item& aItem)
  {
    return ReplaceElementsAt(aStart, aCount, &aItem, 1);
  }

  
  template<class Item>
  elem_type* ReplaceElementAt(index_type aIndex, const Item& aItem)
  {
    return ReplaceElementsAt(aIndex, 1, &aItem, 1);
  }

  
  template<class Item>
  elem_type* InsertElementsAt(index_type aIndex, const Item* aArray,
                              size_type aArrayLen)
  {
    return ReplaceElementsAt(aIndex, 0, aArray, aArrayLen);
  }

  
  template<class Item, class Allocator>
  elem_type* InsertElementsAt(index_type aIndex,
                              const nsTArray_Impl<Item, Allocator>& aArray)
  {
    return ReplaceElementsAt(aIndex, 0, aArray.Elements(), aArray.Length());
  }

  
  
  
  elem_type* InsertElementAt(index_type aIndex)
  {
    if (!Alloc::Successful(this->EnsureCapacity(Length() + 1,
                                                sizeof(elem_type)))) {
      return nullptr;
    }
    this->ShiftData(aIndex, 0, 1, sizeof(elem_type), MOZ_ALIGNOF(elem_type));
    elem_type* elem = Elements() + aIndex;
    elem_traits::Construct(elem);
    return elem;
  }

  
  template<class Item>
  elem_type* InsertElementAt(index_type aIndex, Item&& aItem)
  {
    if (!Alloc::Successful(this->EnsureCapacity(Length() + 1,
                                                sizeof(elem_type)))) {
      return nullptr;
    }
    this->ShiftData(aIndex, 0, 1, sizeof(elem_type), MOZ_ALIGNOF(elem_type));
    elem_type* elem = Elements() + aIndex;
    elem_traits::Construct(elem, mozilla::Forward<Item>(aItem));
    return elem;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  template<class Item, class Comparator>
  index_type IndexOfFirstElementGt(const Item& aItem,
                                   const Comparator& aComp) const
  {
    using mozilla::BinarySearchIf;
    typedef ::detail::ItemComparatorFirstElementGT<Item, Comparator> Cmp;

    size_t index;
    BinarySearchIf(*this, 0, Length(), Cmp(aItem, aComp), &index);
    return index;
  }

  
  template<class Item>
  index_type
  IndexOfFirstElementGt(const Item& aItem) const
  {
    return IndexOfFirstElementGt(aItem, nsDefaultComparator<elem_type, Item>());
  }

  
  
  
  template<class Item, class Comparator>
  elem_type* InsertElementSorted(const Item& aItem, const Comparator& aComp)
  {
    index_type index = IndexOfFirstElementGt(aItem, aComp);
    return InsertElementAt(index, aItem);
  }

  
  template<class Item>
  elem_type* InsertElementSorted(const Item& aItem)
  {
    return InsertElementSorted(aItem, nsDefaultComparator<elem_type, Item>());
  }

  
  
  
  
  
  template<class Item>
  elem_type* AppendElements(const Item* aArray, size_type aArrayLen)
  {
    if (!Alloc::Successful(this->EnsureCapacity(Length() + aArrayLen,
                                                sizeof(elem_type)))) {
      return nullptr;
    }
    index_type len = Length();
    AssignRange(len, aArrayLen, aArray);
    this->IncrementLength(aArrayLen);
    return Elements() + len;
  }

  
  template<class Item, class Allocator>
  elem_type* AppendElements(const nsTArray_Impl<Item, Allocator>& aArray)
  {
    return AppendElements(aArray.Elements(), aArray.Length());
  }

  
  template<class Item>
  elem_type* AppendElement(Item&& aItem)
  {
    if (!Alloc::Successful(this->EnsureCapacity(Length() + 1,
                                                sizeof(elem_type)))) {
      return nullptr;
    }
    elem_type* elem = Elements() + Length();
    elem_traits::Construct(elem, mozilla::Forward<Item>(aItem));
    this->IncrementLength(1);
    return elem;
  }

  
  
  
  elem_type* AppendElements(size_type aCount)
  {
    if (!Alloc::Successful(this->EnsureCapacity(Length() + aCount,
                                                sizeof(elem_type)))) {
      return nullptr;
    }
    elem_type* elems = Elements() + Length();
    size_type i;
    for (i = 0; i < aCount; ++i) {
      elem_traits::Construct(elems + i);
    }
    this->IncrementLength(aCount);
    return elems;
  }

  
  
  
  elem_type* AppendElement() { return AppendElements(1); }

  
  
  
  template<class Item, class Allocator>
  elem_type* MoveElementsFrom(nsTArray_Impl<Item, Allocator>& aArray)
  {
    MOZ_ASSERT(&aArray != this, "argument must be different aArray");
    index_type len = Length();
    index_type otherLen = aArray.Length();
    if (!Alloc::Successful(this->EnsureCapacity(len + otherLen,
                                                sizeof(elem_type)))) {
      return nullptr;
    }
    copy_type::CopyElements(Elements() + len, aArray.Elements(), otherLen,
                            sizeof(elem_type));
    this->IncrementLength(otherLen);
    aArray.ShiftData(0, otherLen, 0, sizeof(elem_type), MOZ_ALIGNOF(elem_type));
    return Elements() + len;
  }
  template<class Item, class Allocator>
  elem_type* MoveElementsFrom(nsTArray_Impl<Item, Allocator>&& aArray)
  {
    return MoveElementsFrom<Item, Allocator>(aArray);
  }

  
  
  
  void RemoveElementsAt(index_type aStart, size_type aCount)
  {
    MOZ_ASSERT(aCount == 0 || aStart < Length(), "Invalid aStart index");
    MOZ_ASSERT(aStart + aCount <= Length(), "Invalid length");
    
    MOZ_ASSERT(aStart <= aStart + aCount, "Start index plus length overflows");
    DestructRange(aStart, aCount);
    this->ShiftData(aStart, aCount, 0,
                    sizeof(elem_type), MOZ_ALIGNOF(elem_type));
  }

  
  void RemoveElementAt(index_type aIndex) { RemoveElementsAt(aIndex, 1); }

  
  void Clear() { RemoveElementsAt(0, Length()); }

  
  
  
  
  
  template<class Item, class Comparator>
  bool RemoveElement(const Item& aItem, const Comparator& aComp)
  {
    index_type i = IndexOf(aItem, 0, aComp);
    if (i == NoIndex) {
      return false;
    }

    RemoveElementAt(i);
    return true;
  }

  
  
  template<class Item>
  bool RemoveElement(const Item& aItem)
  {
    return RemoveElement(aItem, nsDefaultComparator<elem_type, Item>());
  }

  
  
  
  
  
  
  template<class Item, class Comparator>
  bool RemoveElementSorted(const Item& aItem, const Comparator& aComp)
  {
    index_type index = IndexOfFirstElementGt(aItem, aComp);
    if (index > 0 && aComp.Equals(ElementAt(index - 1), aItem)) {
      RemoveElementAt(index - 1);
      return true;
    }
    return false;
  }

  
  template<class Item>
  bool RemoveElementSorted(const Item& aItem)
  {
    return RemoveElementSorted(aItem, nsDefaultComparator<elem_type, Item>());
  }

  
  
  template<class Allocator>
  typename Alloc::ResultType SwapElements(nsTArray_Impl<E, Allocator>& aOther)
  {
    return Alloc::Result(this->SwapArrayElements(aOther, sizeof(elem_type),
                                                 MOZ_ALIGNOF(elem_type)));
  }

  
  
  

  
  
  
  
  
  
  typename Alloc::ResultType SetCapacity(size_type aCapacity)
  {
    return Alloc::Result(this->EnsureCapacity(aCapacity, sizeof(elem_type)));
  }

  
  
  
  
  
  
  
  
  typename Alloc::ResultType SetLength(size_type aNewLen)
  {
    size_type oldLen = Length();
    if (aNewLen > oldLen) {
      return Alloc::ConvertBoolToResultType(
        InsertElementsAt(oldLen, aNewLen - oldLen) != nullptr);
    }

    TruncateLength(aNewLen);
    return Alloc::ConvertBoolToResultType(true);
  }

  
  
  
  
  
  
  void TruncateLength(size_type aNewLen)
  {
    size_type oldLen = Length();
    MOZ_ASSERT(aNewLen <= oldLen,
               "caller should use SetLength instead");
    RemoveElementsAt(aNewLen, oldLen - aNewLen);
  }

  
  
  
  
  
  
  typename Alloc::ResultType EnsureLengthAtLeast(size_type aMinLen)
  {
    size_type oldLen = Length();
    if (aMinLen > oldLen) {
      return Alloc::ConvertBoolToResultType(!!InsertElementsAt(oldLen,
                                                               aMinLen - oldLen));
    }
    return Alloc::ConvertBoolToResultType(true);
  }

  
  
  
  
  
  elem_type* InsertElementsAt(index_type aIndex, size_type aCount)
  {
    if (!base_type::InsertSlotsAt(aIndex, aCount, sizeof(elem_type),
                                  MOZ_ALIGNOF(elem_type))) {
      return nullptr;
    }

    
    elem_type* iter = Elements() + aIndex;
    elem_type* iend = iter + aCount;
    for (; iter != iend; ++iter) {
      elem_traits::Construct(iter);
    }

    return Elements() + aIndex;
  }

  
  
  
  
  
  
  
  template<class Item>
  elem_type* InsertElementsAt(index_type aIndex, size_type aCount,
                              const Item& aItem)
  {
    if (!base_type::InsertSlotsAt(aIndex, aCount, sizeof(elem_type),
                                  MOZ_ALIGNOF(elem_type))) {
      return nullptr;
    }

    
    elem_type* iter = Elements() + aIndex;
    elem_type* iend = iter + aCount;
    for (; iter != iend; ++iter) {
      elem_traits::Construct(iter, aItem);
    }

    return Elements() + aIndex;
  }

  
  void Compact()
  {
    ShrinkCapacity(sizeof(elem_type), MOZ_ALIGNOF(elem_type));
  }

  
  
  

  
  
  
  template<class Comparator>
  static int Compare(const void* aE1, const void* aE2, void* aData)
  {
    const Comparator* c = reinterpret_cast<const Comparator*>(aData);
    const elem_type* a = static_cast<const elem_type*>(aE1);
    const elem_type* b = static_cast<const elem_type*>(aE2);
    return c->LessThan(*a, *b) ? -1 : (c->Equals(*a, *b) ? 0 : 1);
  }

  
  
  
  template<class Comparator>
  void Sort(const Comparator& aComp)
  {
    NS_QuickSort(Elements(), Length(), sizeof(elem_type),
                 Compare<Comparator>, const_cast<Comparator*>(&aComp));
  }

  
  
  void Sort() { Sort(nsDefaultComparator<elem_type, elem_type>()); }

  
  
  

  
  
  template<class Comparator>
  void MakeHeap(const Comparator& aComp)
  {
    if (!Length()) {
      return;
    }
    index_type index = (Length() - 1) / 2;
    do {
      SiftDown(index, aComp);
    } while (index--);
  }

  
  void MakeHeap()
  {
    MakeHeap(nsDefaultComparator<elem_type, elem_type>());
  }

  
  
  
  template<class Item, class Comparator>
  elem_type* PushHeap(const Item& aItem, const Comparator& aComp)
  {
    if (!base_type::InsertSlotsAt(Length(), 1, sizeof(elem_type),
                                  MOZ_ALIGNOF(elem_type))) {
      return nullptr;
    }
    
    elem_type* elem = Elements();
    index_type index = Length() - 1;
    index_type parent_index = (index - 1) / 2;
    while (index && aComp.LessThan(elem[parent_index], aItem)) {
      elem[index] = elem[parent_index];
      index = parent_index;
      parent_index = (index - 1) / 2;
    }
    elem[index] = aItem;
    return &elem[index];
  }

  
  template<class Item>
  elem_type* PushHeap(const Item& aItem)
  {
    return PushHeap(aItem, nsDefaultComparator<elem_type, Item>());
  }

  
  
  template<class Comparator>
  void PopHeap(const Comparator& aComp)
  {
    if (!Length()) {
      return;
    }
    index_type last_index = Length() - 1;
    elem_type* elem = Elements();
    elem[0] = elem[last_index];
    TruncateLength(last_index);
    if (Length()) {
      SiftDown(0, aComp);
    }
  }

  
  void PopHeap()
  {
    PopHeap(nsDefaultComparator<elem_type, elem_type>());
  }

protected:
  using base_type::Hdr;
  using base_type::ShrinkCapacity;

  
  
  
  void DestructRange(index_type aStart, size_type aCount)
  {
    elem_type* iter = Elements() + aStart;
    elem_type *iend = iter + aCount;
    for (; iter != iend; ++iter) {
      elem_traits::Destruct(iter);
    }
  }

  
  
  
  
  template<class Item>
  void AssignRange(index_type aStart, size_type aCount, const Item* aValues)
  {
    AssignRangeAlgorithm<mozilla::IsPod<Item>::value,
                         mozilla::IsSame<Item, elem_type>::value>
                         ::implementation(Elements(), aStart, aCount, aValues);
  }

  
  
  
  template<class Comparator>
  void SiftDown(index_type aIndex, const Comparator& aComp)
  {
    elem_type* elem = Elements();
    elem_type item = elem[aIndex];
    index_type iend = Length() - 1;
    while ((aIndex * 2) < iend) {
      const index_type left = (aIndex * 2) + 1;
      const index_type right = (aIndex * 2) + 2;
      const index_type parent_index = aIndex;
      if (aComp.LessThan(item, elem[left])) {
        if (left < iend &&
            aComp.LessThan(elem[left], elem[right])) {
          aIndex = right;
        } else {
          aIndex = left;
        }
      } else if (left < iend &&
                 aComp.LessThan(item, elem[right])) {
        aIndex = right;
      } else {
        break;
      }
      elem[parent_index] = elem[aIndex];
    }
    elem[aIndex] = item;
  }
};

template<typename E, typename Alloc>
inline void
ImplCycleCollectionUnlink(nsTArray_Impl<E, Alloc>& aField)
{
  aField.Clear();
}

template<typename E, typename Alloc>
inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            nsTArray_Impl<E, Alloc>& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  aFlags |= CycleCollectionEdgeNameArrayFlag;
  size_t length = aField.Length();
  for (size_t i = 0; i < length; ++i) {
    ImplCycleCollectionTraverse(aCallback, aField[i], aName, aFlags);
  }
}





template<class E>
class nsTArray : public nsTArray_Impl<E, nsTArrayInfallibleAllocator>
{
public:
  typedef nsTArray_Impl<E, nsTArrayInfallibleAllocator> base_type;
  typedef nsTArray<E>                                   self_type;
  typedef typename base_type::size_type                 size_type;

  nsTArray() {}
  explicit nsTArray(size_type aCapacity) : base_type(aCapacity) {}
  explicit nsTArray(const nsTArray& aOther) : base_type(aOther) {}
  MOZ_IMPLICIT nsTArray(nsTArray&& aOther) : base_type(mozilla::Move(aOther)) {}

  template<class Allocator>
  explicit nsTArray(const nsTArray_Impl<E, Allocator>& aOther)
    : base_type(aOther)
  {
  }
  template<class Allocator>
  MOZ_IMPLICIT nsTArray(nsTArray_Impl<E, Allocator>&& aOther)
    : base_type(mozilla::Move(aOther))
  {
  }

  self_type& operator=(const self_type& aOther)
  {
    base_type::operator=(aOther);
    return *this;
  }
  template<class Allocator>
  self_type& operator=(const nsTArray_Impl<E, Allocator>& aOther)
  {
    base_type::operator=(aOther);
    return *this;
  }
  self_type& operator=(self_type&& aOther)
  {
    base_type::operator=(mozilla::Move(aOther));
    return *this;
  }
  template<class Allocator>
  self_type& operator=(nsTArray_Impl<E, Allocator>&& aOther)
  {
    base_type::operator=(mozilla::Move(aOther));
    return *this;
  }
};




template<class E>
class FallibleTArray : public nsTArray_Impl<E, nsTArrayFallibleAllocator>
{
public:
  typedef nsTArray_Impl<E, nsTArrayFallibleAllocator>   base_type;
  typedef FallibleTArray<E>                             self_type;
  typedef typename base_type::size_type                 size_type;

  FallibleTArray() {}
  explicit FallibleTArray(size_type aCapacity) : base_type(aCapacity) {}
  explicit FallibleTArray(const FallibleTArray<E>& aOther) : base_type(aOther) {}
  explicit FallibleTArray(FallibleTArray<E>&& aOther)
    : base_type(mozilla::Move(aOther))
  {
  }

  template<class Allocator>
  explicit FallibleTArray(const nsTArray_Impl<E, Allocator>& aOther)
    : base_type(aOther)
  {
  }
  template<class Allocator>
  explicit FallibleTArray(nsTArray_Impl<E, Allocator>&& aOther)
    : base_type(mozilla::Move(aOther))
  {
  }

  self_type& operator=(const self_type& aOther)
  {
    base_type::operator=(aOther);
    return *this;
  }
  template<class Allocator>
  self_type& operator=(const nsTArray_Impl<E, Allocator>& aOther)
  {
    base_type::operator=(aOther);
    return *this;
  }
  self_type& operator=(self_type&& aOther)
  {
    base_type::operator=(mozilla::Move(aOther));
    return *this;
  }
  template<class Allocator>
  self_type& operator=(nsTArray_Impl<E, Allocator>&& aOther)
  {
    base_type::operator=(mozilla::Move(aOther));
    return *this;
  }
};





template<class TArrayBase, size_t N>
class nsAutoArrayBase : public TArrayBase
{
  static_assert(N != 0, "nsAutoArrayBase<TArrayBase, 0> should be specialized");
public:
  typedef nsAutoArrayBase<TArrayBase, N> self_type;
  typedef TArrayBase base_type;
  typedef typename base_type::Header Header;
  typedef typename base_type::elem_type elem_type;

  template<typename Allocator>
  self_type& operator=(const nsTArray_Impl<elem_type, Allocator>& aOther)
  {
    base_type::operator=(aOther);
    return *this;
  }

protected:
  nsAutoArrayBase() { Init(); }

  
  
  
  
  nsAutoArrayBase(const self_type& aOther)
  {
    Init();
    this->AppendElements(aOther);
  }

  explicit nsAutoArrayBase(const TArrayBase &aOther)
  {
    Init();
    this->AppendElements(aOther);
  }

  template<typename Allocator>
  nsAutoArrayBase(nsTArray_Impl<elem_type, Allocator>&& aOther)
  {
    Init();
    this->SwapElements(aOther);
  }

private:
  
  
  template<class Allocator, class Copier>
  friend class nsTArray_base;

  void Init()
  {
    static_assert(MOZ_ALIGNOF(elem_type) <= 8,
                  "can't handle alignments greater than 8, "
                  "see nsTArray_base::UsesAutoArrayBuffer()");
    
    Header** phdr = base_type::PtrToHdr();
    *phdr = reinterpret_cast<Header*>(&mAutoBuf);
    (*phdr)->mLength = 0;
    (*phdr)->mCapacity = N;
    (*phdr)->mIsAutoArray = 1;

    MOZ_ASSERT(base_type::GetAutoArrayBuffer(MOZ_ALIGNOF(elem_type)) ==
               reinterpret_cast<Header*>(&mAutoBuf),
               "GetAutoArrayBuffer needs to be fixed");
  }

  
  
  
  
  union
  {
    char mAutoBuf[sizeof(nsTArrayHeader) + N * sizeof(elem_type)];
    
    mozilla::AlignedElem<(MOZ_ALIGNOF(Header) > MOZ_ALIGNOF(elem_type)) ?
                         MOZ_ALIGNOF(Header) : MOZ_ALIGNOF(elem_type)> mAlign;
  };
};















template<class TArrayBase>
class nsAutoArrayBase<TArrayBase, 0> : public TArrayBase
{
};









template<class E, size_t N>
class nsAutoTArray : public nsAutoArrayBase<nsTArray<E>, N>
{
  typedef nsAutoTArray<E, N> self_type;
  typedef nsAutoArrayBase<nsTArray<E>, N> Base;

public:
  nsAutoTArray() {}

  template<typename Allocator>
  explicit nsAutoTArray(const nsTArray_Impl<E, Allocator>& aOther)
  {
    Base::AppendElements(aOther);
  }
  template<typename Allocator>
  explicit nsAutoTArray(nsTArray_Impl<E, Allocator>&& aOther)
    : Base(mozilla::Move(aOther))
  {
  }

  template<typename Allocator>
  self_type& operator=(const nsTArray_Impl<E, Allocator>& other)
  {
    Base::operator=(other);
    return *this;
  }

  operator const AutoFallibleTArray<E, N>&() const
  {
    return *reinterpret_cast<const AutoFallibleTArray<E, N>*>(this);
  }
};





template<class E, size_t N>
class AutoFallibleTArray : public nsAutoArrayBase<FallibleTArray<E>, N>
{
  typedef AutoFallibleTArray<E, N> self_type;
  typedef nsAutoArrayBase<FallibleTArray<E>, N> Base;

public:
  AutoFallibleTArray() {}

  template<typename Allocator>
  explicit AutoFallibleTArray(const nsTArray_Impl<E, Allocator>& aOther)
  {
    Base::AppendElements(aOther);
  }
  template<typename Allocator>
  explicit AutoFallibleTArray(nsTArray_Impl<E, Allocator>&& aOther)
    : Base(mozilla::Move(aOther))
  {
  }

  template<typename Allocator>
  self_type& operator=(const nsTArray_Impl<E, Allocator>& other)
  {
    Base::operator=(other);
    return *this;
  }

  operator const nsAutoTArray<E, N>&() const
  {
    return *reinterpret_cast<const nsAutoTArray<E, N>*>(this);
  }
};














static_assert(sizeof(nsAutoTArray<uint32_t, 2>) ==
              sizeof(void*) + sizeof(nsTArrayHeader) + sizeof(uint32_t) * 2,
              "nsAutoTArray shouldn't contain any extra padding, "
              "see the comment");


#include "nsTArray-inl.h"

#endif  
