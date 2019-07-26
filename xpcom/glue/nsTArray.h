





#ifndef nsTArray_h__
#define nsTArray_h__

#include "nsTArrayForwardDeclare.h"
#include "mozilla/Alignment.h"
#include "mozilla/Assertions.h"
#include "mozilla/MemoryReporting.h"
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
template <class T>
class Heap;
} 

class nsRegion;
class nsIntRegion;





































































struct nsTArrayFallibleResult
{
  
  nsTArrayFallibleResult(bool result)
    : mResult(result)
  {}

  operator bool() {
    return mResult;
  }

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

  static ResultType Result(ResultTypeProxy result) {
    return result;
  }

  static bool Successful(ResultTypeProxy result) {
    return result;
  }

  static ResultTypeProxy SuccessResult() {
    return true;
  }

  static ResultTypeProxy FailureResult() {
    return false;
  }

  static ResultType ConvertBoolToResultType(bool aValue) {
    return aValue;
  }
};

struct nsTArrayInfallibleAllocatorBase
{
  typedef void ResultType;
  typedef nsTArrayInfallibleResult ResultTypeProxy;

  static ResultType Result(ResultTypeProxy result) {
  }

  static bool Successful(ResultTypeProxy) {
    return true;
  }

  static ResultTypeProxy SuccessResult() {
    return ResultTypeProxy();
  }

  static ResultTypeProxy FailureResult() {
    NS_RUNTIMEABORT("Infallible nsTArray should never fail");
    return ResultTypeProxy();
  }

  static ResultType ConvertBoolToResultType(bool aValue) {
    if (!aValue) {
      NS_RUNTIMEABORT("infallible nsTArray should never convert false to ResultType");
    }
  }
};

#if defined(MOZALLOC_HAVE_XMALLOC)
#include "mozilla/mozalloc_abort.h"

struct nsTArrayFallibleAllocator : nsTArrayFallibleAllocatorBase
{
  static void* Malloc(size_t size) {
    return moz_malloc(size);
  }

  static void* Realloc(void* ptr, size_t size) {
    return moz_realloc(ptr, size);
  }

  static void Free(void* ptr) {
    moz_free(ptr);
  }

  static void SizeTooBig(size_t) {
  }
};

struct nsTArrayInfallibleAllocator : nsTArrayInfallibleAllocatorBase
{
  static void* Malloc(size_t size) {
    return moz_xmalloc(size);
  }

  static void* Realloc(void* ptr, size_t size) {
    return moz_xrealloc(ptr, size);
  }

  static void Free(void* ptr) {
    moz_free(ptr);
  }

  static void SizeTooBig(size_t size) {
    NS_ABORT_OOM(size);
  }
};

#else
#include <stdlib.h>

struct nsTArrayFallibleAllocator : nsTArrayFallibleAllocatorBase
{
  static void* Malloc(size_t size) {
    return malloc(size);
  }

  static void* Realloc(void* ptr, size_t size) {
    return realloc(ptr, size);
  }

  static void Free(void* ptr) {
    free(ptr);
  }

  static void SizeTooBig(size_t) {
  }
};

struct nsTArrayInfallibleAllocator : nsTArrayInfallibleAllocatorBase
{
  static void* Malloc(size_t size) {
    void* ptr = malloc(size);
    if (MOZ_UNLIKELY(!ptr)) {
      NS_ABORT_OOM(size);
    }
    return ptr;
  }

  static void* Realloc(void* ptr, size_t size) {
    void* newptr = realloc(ptr, size);
    if (MOZ_UNLIKELY(!ptr && size)) {
      NS_ABORT_OOM(size);
    }
    return newptr;
  }

  static void Free(void* ptr) {
    free(ptr);
  }

  static void SizeTooBig(size_t size) {
    NS_ABORT_OOM(size);
  }
};

#endif




struct NS_COM_GLUE nsTArrayHeader
{
  static nsTArrayHeader sEmptyHdr;

  uint32_t mLength;
  uint32_t mCapacity : 31;
  uint32_t mIsAutoArray : 1;
};



template <class E, class Derived>
struct nsTArray_SafeElementAtHelper
{
  typedef E*       elem_type;
  typedef size_t   index_type;

  
  
  
  elem_type& SafeElementAt(index_type i);
  const elem_type& SafeElementAt(index_type i) const;
};

template <class E, class Derived>
struct nsTArray_SafeElementAtHelper<E*, Derived>
{
  typedef E*       elem_type;
  typedef size_t   index_type;

  elem_type SafeElementAt(index_type i) {
    return static_cast<Derived*> (this)->SafeElementAt(i, nullptr);
  }

  const elem_type SafeElementAt(index_type i) const {
    return static_cast<const Derived*> (this)->SafeElementAt(i, nullptr);
  }
};



template <class E, class Derived>
struct nsTArray_SafeElementAtSmartPtrHelper
{
  typedef E*       elem_type;
  typedef size_t   index_type;

  elem_type SafeElementAt(index_type i) {
    return static_cast<Derived*> (this)->SafeElementAt(i, nullptr);
  }

  const elem_type SafeElementAt(index_type i) const {
    return static_cast<const Derived*> (this)->SafeElementAt(i, nullptr);
  }
};

template <class T> class nsCOMPtr;

template <class E, class Derived>
struct nsTArray_SafeElementAtHelper<nsCOMPtr<E>, Derived> :
  public nsTArray_SafeElementAtSmartPtrHelper<E, Derived>
{
};

template <class T> class nsRefPtr;

template <class E, class Derived>
struct nsTArray_SafeElementAtHelper<nsRefPtr<E>, Derived> :
  public nsTArray_SafeElementAtSmartPtrHelper<E, Derived>
{
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

  
  size_type Length() const {
    return mHdr->mLength;
  }

  
  bool IsEmpty() const {
    return Length() == 0;
  }

  
  
  
  size_type Capacity() const {
    return mHdr->mCapacity;
  }

#ifdef DEBUG
  void* DebugGetHeader() const {
    return mHdr;
  }
#endif

protected:
  nsTArray_base();

  ~nsTArray_base();

  
  
  
  
  typename Alloc::ResultTypeProxy EnsureCapacity(size_type capacity, size_type elemSize);

  
  
  
  void ShrinkCapacity(size_type elemSize, size_t elemAlign);

  
  
  
  
  
  
  
  
  void ShiftData(index_type start, size_type oldLen, size_type newLen,
                 size_type elemSize, size_t elemAlign);

  
  
  
  
  void IncrementLength(size_t n) {
    if (mHdr == EmptyHdr()) {
      if (MOZ_UNLIKELY(n != 0)) {
        
        MOZ_CRASH();
      }
    } else {
      mHdr->mLength += n;
    }
  }

  
  
  
  
  
  
  bool InsertSlotsAt(index_type index, size_type count,
                       size_type elementSize, size_t elemAlign);

protected:
  template<class Allocator>
  typename Alloc::ResultTypeProxy
  SwapArrayElements(nsTArray_base<Allocator, Copy>& other,
                    size_type elemSize,
                    size_t elemAlign);

  
  class IsAutoArrayRestorer {
    public:
      IsAutoArrayRestorer(nsTArray_base<Alloc, Copy> &array, size_t elemAlign);
      ~IsAutoArrayRestorer();

    private:
      nsTArray_base<Alloc, Copy> &mArray;
      size_t mElemAlign;
      bool mIsAuto;
  };

  
  
  bool EnsureNotUsingAutoArrayBuffer(size_type elemSize);

  
  bool IsAutoArray() const {
    return mHdr->mIsAutoArray;
  }

  
  Header* GetAutoArrayBuffer(size_t elemAlign) {
    MOZ_ASSERT(IsAutoArray(), "Should be an auto array to call this");
    return GetAutoArrayBufferUnsafe(elemAlign);
  }
  const Header* GetAutoArrayBuffer(size_t elemAlign) const {
    MOZ_ASSERT(IsAutoArray(), "Should be an auto array to call this");
    return GetAutoArrayBufferUnsafe(elemAlign);
  }

  
  
  Header* GetAutoArrayBufferUnsafe(size_t elemAlign) {
    return const_cast<Header*>(static_cast<const nsTArray_base<Alloc, Copy>*>(this)->
                               GetAutoArrayBufferUnsafe(elemAlign));
  }
  const Header* GetAutoArrayBufferUnsafe(size_t elemAlign) const;

  
  
  bool UsesAutoArrayBuffer() const;

  
  
  Header *mHdr;

  Header* Hdr() const {
    return mHdr;
  }

  Header** PtrToHdr() {
    return &mHdr;
  }

  static Header* EmptyHdr() {
    return &Header::sEmptyHdr;
  }
};





template<class E>
class nsTArrayElementTraits
{
public:
  
  static inline void Construct(E *e) {
    
    
    
    
    
    new (static_cast<void *>(e)) E;
  }
  
  template<class A>
  static inline void Construct(E *e, const A &arg) {
    typedef typename mozilla::RemoveCV<E>::Type E_NoCV;
    typedef typename mozilla::RemoveCV<A>::Type A_NoCV;
    static_assert(!mozilla::IsSame<E_NoCV*, A_NoCV>::value,
                  "For safety, we disallow constructing nsTArray<E> elements "
                  "from E* pointers. See bug 960591.");
    new (static_cast<void *>(e)) E(arg);
  }
  
  static inline void Destruct(E *e) {
    e->~E();
  }
};


template<class A, class B>
class nsDefaultComparator
{
public:
  bool Equals(const A& a, const B& b) const {
    return a == b;
  }
  bool LessThan(const A& a, const B& b) const {
    return a < b;
  }
};

template <class E> class InfallibleTArray;
template <class E> class FallibleTArray;

template<bool IsPod, bool IsSameType>
struct AssignRangeAlgorithm {
  template<class Item, class ElemType, class IndexType, class SizeType>
  static void implementation(ElemType* elements, IndexType start,
                             SizeType count, const Item *values) {
    ElemType *iter = elements + start, *end = iter + count;
    for (; iter != end; ++iter, ++values)
      nsTArrayElementTraits<ElemType>::Construct(iter, *values);
  }
};

template<>
struct AssignRangeAlgorithm<true, true> {
  template<class Item, class ElemType, class IndexType, class SizeType>
  static void implementation(ElemType* elements, IndexType start,
                             SizeType count, const Item *values) {
    memcpy(elements + start, values, count * sizeof(ElemType));
  }
};











struct nsTArray_CopyWithMemutils
{
  const static bool allowRealloc = true;

  static void CopyElements(void* dest, const void* src, size_t count, size_t elemSize) {
    memcpy(dest, src, count * elemSize);
  }

  static void CopyHeaderAndElements(void* dest, const void* src, size_t count, size_t elemSize) {
    memcpy(dest, src, sizeof(nsTArrayHeader) + count * elemSize);
  }

  static void MoveElements(void* dest, const void* src, size_t count, size_t elemSize) {
    memmove(dest, src, count * elemSize);
  }
};





template <class ElemType>
struct nsTArray_CopyWithConstructors
{
  typedef nsTArrayElementTraits<ElemType> traits;

  const static bool allowRealloc = false;

  static void CopyElements(void* dest, void* src, size_t count, size_t elemSize) {
    ElemType* destElem = static_cast<ElemType*>(dest);
    ElemType* srcElem = static_cast<ElemType*>(src);
    ElemType* destElemEnd = destElem + count;
#ifdef DEBUG
    ElemType* srcElemEnd = srcElem + count;
    MOZ_ASSERT(srcElemEnd <= destElem || srcElemEnd > destElemEnd);
#endif
    while (destElem != destElemEnd) {
      traits::Construct(destElem, *srcElem);
      traits::Destruct(srcElem);
      ++destElem;
      ++srcElem;
    }
  }

  static void CopyHeaderAndElements(void* dest, void* src, size_t count, size_t elemSize) {
    nsTArrayHeader* destHeader = static_cast<nsTArrayHeader*>(dest);
    nsTArrayHeader* srcHeader = static_cast<nsTArrayHeader*>(src);
    *destHeader = *srcHeader;
    CopyElements(static_cast<uint8_t*>(dest) + sizeof(nsTArrayHeader),
                 static_cast<uint8_t*>(src) + sizeof(nsTArrayHeader),
                 count, elemSize);
  }

  static void MoveElements(void* dest, void* src, size_t count, size_t elemSize) {
    ElemType* destElem = static_cast<ElemType*>(dest);
    ElemType* srcElem = static_cast<ElemType*>(src);
    ElemType* destElemEnd = destElem + count;
    ElemType* srcElemEnd = srcElem + count;
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
      CopyElements(dest, src, count, elemSize);
    }
  }
};




template <class E>
struct nsTArray_CopyChooser {
  typedef nsTArray_CopyWithMemutils Type;
};





template <class E>
struct nsTArray_CopyChooser<JS::Heap<E> > {
  typedef nsTArray_CopyWithConstructors<JS::Heap<E> > Type;
};

template<>
struct nsTArray_CopyChooser<nsRegion> {
  typedef nsTArray_CopyWithConstructors<nsRegion> Type;
};

template<>
struct nsTArray_CopyChooser<nsIntRegion> {
  typedef nsTArray_CopyWithConstructors<nsIntRegion> Type;
};






template <class E, class Derived>
struct nsTArray_TypedBase : public nsTArray_SafeElementAtHelper<E, Derived> {};












template <class E, class Derived>
struct nsTArray_TypedBase<JS::Heap<E>, Derived>
 : public nsTArray_SafeElementAtHelper<JS::Heap<E>, Derived>
{
  operator const nsTArray<E>& () {
    static_assert(sizeof(E) == sizeof(JS::Heap<E>),
                  "JS::Heap<E> must be binary compatible with E.");
    Derived* self = static_cast<Derived*>(this);
    return *reinterpret_cast<nsTArray<E> *>(self);
  }

  operator const FallibleTArray<E>& () {
    Derived* self = static_cast<Derived*>(this);
    return *reinterpret_cast<FallibleTArray<E> *>(self);
  }
};














template<class E, class Alloc>
class nsTArray_Impl : public nsTArray_base<Alloc, typename nsTArray_CopyChooser<E>::Type>,
                      public nsTArray_TypedBase<E, nsTArray_Impl<E, Alloc> >
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

  using safeelementat_helper_type::SafeElementAt;
  using base_type::EmptyHdr;

  
  
  enum {
    NoIndex = index_type(-1)
  };

  using base_type::Length;

  
  
  

  ~nsTArray_Impl() { Clear(); }

  
  
  

  nsTArray_Impl() {}

  
  explicit nsTArray_Impl(size_type capacity) {
    SetCapacity(capacity);
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  explicit nsTArray_Impl(const self_type& other) {
    AppendElements(other);
  }

  
  
  template<typename Allocator>
  operator const nsTArray_Impl<E, Allocator>&() const {
    return *reinterpret_cast<const nsTArray_Impl<E, Allocator>*>(this);
  }
  
  operator const nsTArray<E>&() const {
    return *reinterpret_cast<const InfallibleTArray<E>*>(this);
  }
  operator const FallibleTArray<E>&() const {
    return *reinterpret_cast<const FallibleTArray<E>*>(this);
  }

  
  
  
  self_type& operator=(const self_type& other) {
    ReplaceElementsAt(0, Length(), other.Elements(), other.Length());
    return *this;
  }

  
  
  template<typename Allocator>
  bool operator==(const nsTArray_Impl<E, Allocator>& other) const {
    size_type len = Length();
    if (len != other.Length())
      return false;

    
    for (index_type i = 0; i < len; ++i)
      if (!(operator[](i) == other[i]))
        return false;

    return true;
  }

  
  
  bool operator!=(const self_type& other) const {
    return !operator==(other);
  }

  template<typename Allocator>
  self_type& operator=(const nsTArray_Impl<E, Allocator>& other) {
    ReplaceElementsAt(0, Length(), other.Elements(), other.Length());
    return *this;
  }

  
  
  size_t SizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const {
    if (this->UsesAutoArrayBuffer() || Hdr() == EmptyHdr())
      return 0;
    return mallocSizeOf(this->Hdr());
  }

  
  
  size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const {
    return mallocSizeOf(this) + SizeOfExcludingThis(mallocSizeOf);
  }

  
  
  

  
  
  
  elem_type* Elements() {
    return reinterpret_cast<elem_type *>(Hdr() + 1);
  }

  
  
  
  const elem_type* Elements() const {
    return reinterpret_cast<const elem_type *>(Hdr() + 1);
  }

  
  
  
  
  elem_type& ElementAt(index_type i) {
    MOZ_ASSERT(i < Length(), "invalid array index");
    return Elements()[i];
  }

  
  
  
  
  const elem_type& ElementAt(index_type i) const {
    MOZ_ASSERT(i < Length(), "invalid array index");
    return Elements()[i];
  }

  
  
  
  
  
  elem_type& SafeElementAt(index_type i, elem_type& def) {
    return i < Length() ? Elements()[i] : def;
  }

  
  
  
  
  
  const elem_type& SafeElementAt(index_type i, const elem_type& def) const {
    return i < Length() ? Elements()[i] : def;
  }

  
  elem_type& operator[](index_type i) {
    return ElementAt(i);
  }

  
  const elem_type& operator[](index_type i) const {
    return ElementAt(i);
  }

  
  elem_type& LastElement() {
    return ElementAt(Length() - 1);
  }

  
  const elem_type& LastElement() const {
    return ElementAt(Length() - 1);
  }

  
  elem_type& SafeLastElement(elem_type& def) {
    return SafeElementAt(Length() - 1, def);
  }

  
  const elem_type& SafeLastElement(const elem_type& def) const {
    return SafeElementAt(Length() - 1, def);
  }

  
  
  

  
  
  
  
  
  template<class Item, class Comparator>
  bool Contains(const Item& item, const Comparator& comp) const {
    return IndexOf(item, 0, comp) != NoIndex;
  }

  
  
  
  
  
  template<class Item>
  bool Contains(const Item& item) const {
    return IndexOf(item) != NoIndex;
  }

  
  
  
  
  
  
  template<class Item, class Comparator>
  index_type IndexOf(const Item& item, index_type start,
                     const Comparator& comp) const {
    const elem_type* iter = Elements() + start, *end = Elements() + Length();
    for (; iter != end; ++iter) {
      if (comp.Equals(*iter, item))
        return index_type(iter - Elements());
    }
    return NoIndex;
  }

  
  
  
  
  
  
  template<class Item>
  index_type IndexOf(const Item& item, index_type start = 0) const {
    return IndexOf(item, start, nsDefaultComparator<elem_type, Item>());
  }

  
  
  
  
  
  
  
  template<class Item, class Comparator>
  index_type LastIndexOf(const Item& item, index_type start,
                         const Comparator& comp) const {
    size_type endOffset = start >= Length() ? Length() : start + 1;
    const elem_type* end = Elements() - 1, *iter = end + endOffset;
    for (; iter != end; --iter) {
      if (comp.Equals(*iter, item))
        return index_type(iter - Elements());
    }
    return NoIndex;
  }

  
  
  
  
  
  
  
  template<class Item>
  index_type LastIndexOf(const Item& item,
                         index_type start = NoIndex) const {
    return LastIndexOf(item, start, nsDefaultComparator<elem_type, Item>());
  }

  
  
  
  
  
  template<class Item, class Comparator>
  index_type BinaryIndexOf(const Item& item, const Comparator& comp) const {
    index_type low = 0, high = Length();
    while (high > low) {
      index_type mid = (high + low) >> 1;
      if (comp.Equals(ElementAt(mid), item))
        return mid;
      if (comp.LessThan(ElementAt(mid), item))
        low = mid + 1;
      else
        high = mid;
    }
    return NoIndex;
  }

  
  
  
  
  
  template<class Item>
  index_type BinaryIndexOf(const Item& item) const {
    return BinaryIndexOf(item, nsDefaultComparator<elem_type, Item>());
  }

  
  
  
  
  
  
  
  
  void ClearAndRetainStorage() {
    if (base_type::mHdr == EmptyHdr()) {
      return;
    }

    DestructRange(0, Length());
    base_type::mHdr->mLength = 0;
  }

  
  
  
  
  
  
  
  
  void SetLengthAndRetainStorage(size_type newLen) {
    MOZ_ASSERT(newLen <= base_type::Capacity());
    size_type oldLen = Length();
    if (newLen > oldLen) {
      InsertElementsAt(oldLen, newLen - oldLen);
      return;
    }
    if (newLen < oldLen) {
      DestructRange(newLen, oldLen - newLen);
      base_type::mHdr->mLength = newLen;
    }
  }

  
  
  
  
  
  
  
  
  
  
  template<class Item>
  elem_type *ReplaceElementsAt(index_type start, size_type count,
                               const Item* array, size_type arrayLen) {
    
    if (!Alloc::Successful(this->EnsureCapacity(Length() + arrayLen - count, sizeof(elem_type))))
      return nullptr;
    DestructRange(start, count);
    this->ShiftData(start, count, arrayLen, sizeof(elem_type), MOZ_ALIGNOF(elem_type));
    AssignRange(start, arrayLen, array);
    return Elements() + start;
  }

  
  template<class Item>
  elem_type *ReplaceElementsAt(index_type start, size_type count,
                               const nsTArray<Item>& array) {
    return ReplaceElementsAt(start, count, array.Elements(), array.Length());
  }

  
  template<class Item>
  elem_type *ReplaceElementsAt(index_type start, size_type count,
                               const Item& item) {
    return ReplaceElementsAt(start, count, &item, 1);
  }

  
  template<class Item>
  elem_type *ReplaceElementAt(index_type index, const Item& item) {
    return ReplaceElementsAt(index, 1, &item, 1);
  }

  
  template<class Item>
  elem_type *InsertElementsAt(index_type index, const Item* array,
                              size_type arrayLen) {
    return ReplaceElementsAt(index, 0, array, arrayLen);
  }

  
  template<class Item, class Allocator>
  elem_type *InsertElementsAt(index_type index, const nsTArray_Impl<Item, Allocator>& array) {
    return ReplaceElementsAt(index, 0, array.Elements(), array.Length());
  }

  
  template<class Item>
  elem_type *InsertElementAt(index_type index, const Item& item) {
    return ReplaceElementsAt(index, 0, &item, 1);
  }

  
  
  
  elem_type* InsertElementAt(index_type index) {
    if (!Alloc::Successful(this->EnsureCapacity(Length() + 1, sizeof(elem_type))))
      return nullptr;
    this->ShiftData(index, 0, 1, sizeof(elem_type), MOZ_ALIGNOF(elem_type));
    elem_type *elem = Elements() + index;
    elem_traits::Construct(elem);
    return elem;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  template<class Item, class Comparator>
  index_type
  IndexOfFirstElementGt(const Item& item,
                        const Comparator& comp) const {
    
    index_type low = 0, high = Length();
    while (high > low) {
      index_type mid = (high + low) >> 1;
      
      
      if (comp.LessThan(ElementAt(mid), item) ||
          comp.Equals(ElementAt(mid), item)) {
        
        low = mid + 1;
      } else {
        
        high = mid;
      }
    }
    MOZ_ASSERT(high == low);
    return low;
  }

  
  template<class Item>
  index_type
  IndexOfFirstElementGt(const Item& item) const {
    return IndexOfFirstElementGt(item, nsDefaultComparator<elem_type, Item>());
  }

  
  
  
  template<class Item, class Comparator>
  elem_type *InsertElementSorted(const Item& item, const Comparator& comp) {
    index_type index = IndexOfFirstElementGt(item, comp);
    return InsertElementAt(index, item);
  }

  
  template<class Item>
  elem_type *InsertElementSorted(const Item& item) {
    return InsertElementSorted(item, nsDefaultComparator<elem_type, Item>());
  }

  
  
  
  
  
  template<class Item>
  elem_type *AppendElements(const Item* array, size_type arrayLen) {
    if (!Alloc::Successful(this->EnsureCapacity(Length() + arrayLen, sizeof(elem_type))))
      return nullptr;
    index_type len = Length();
    AssignRange(len, arrayLen, array);
    this->IncrementLength(arrayLen);
    return Elements() + len;
  }

  
  template<class Item, class Allocator>
  elem_type *AppendElements(const nsTArray_Impl<Item, Allocator>& array) {
    return AppendElements(array.Elements(), array.Length());
  }

  
  template<class Item>
  elem_type *AppendElement(const Item& item) {
    return AppendElements(&item, 1);
  }

  
  
  
  elem_type *AppendElements(size_type count) {
    if (!Alloc::Successful(this->EnsureCapacity(Length() + count, sizeof(elem_type))))
      return nullptr;
    elem_type *elems = Elements() + Length();
    size_type i;
    for (i = 0; i < count; ++i) {
      elem_traits::Construct(elems + i);
    }
    this->IncrementLength(count);
    return elems;
  }

  
  
  
  elem_type *AppendElement() {
    return AppendElements(1);
  }

  
  
  
  template<class Item, class Allocator>
  elem_type *MoveElementsFrom(nsTArray_Impl<Item, Allocator>& array) {
    MOZ_ASSERT(&array != this, "argument must be different array");
    index_type len = Length();
    index_type otherLen = array.Length();
    if (!Alloc::Successful(this->EnsureCapacity(len + otherLen, sizeof(elem_type))))
      return nullptr;
    copy_type::CopyElements(Elements() + len, array.Elements(), otherLen, sizeof(elem_type));
    this->IncrementLength(otherLen);
    array.ShiftData(0, otherLen, 0, sizeof(elem_type), MOZ_ALIGNOF(elem_type));
    return Elements() + len;
  }

  
  
  
  void RemoveElementsAt(index_type start, size_type count) {
    MOZ_ASSERT(count == 0 || start < Length(), "Invalid start index");
    MOZ_ASSERT(start + count <= Length(), "Invalid length");
    
    MOZ_ASSERT(start <= start + count, "Start index plus length overflows");
    DestructRange(start, count);
    this->ShiftData(start, count, 0, sizeof(elem_type), MOZ_ALIGNOF(elem_type));
  }

  
  void RemoveElementAt(index_type index) {
    RemoveElementsAt(index, 1);
  }

  
  void Clear() {
    RemoveElementsAt(0, Length());
  }

  
  
  
  
  
  template<class Item, class Comparator>
  bool RemoveElement(const Item& item, const Comparator& comp) {
    index_type i = IndexOf(item, 0, comp);
    if (i == NoIndex)
      return false;

    RemoveElementAt(i);
    return true;
  }

  
  
  template<class Item>
  bool RemoveElement(const Item& item) {
    return RemoveElement(item, nsDefaultComparator<elem_type, Item>());
  }

  
  
  
  
  
  
  template<class Item, class Comparator>
  bool RemoveElementSorted(const Item& item, const Comparator& comp) {
    index_type index = IndexOfFirstElementGt(item, comp);
    if (index > 0 && comp.Equals(ElementAt(index - 1), item)) {
      RemoveElementAt(index - 1);
      return true;
    }
    return false;
  }

  
  template<class Item>
  bool RemoveElementSorted(const Item& item) {
    return RemoveElementSorted(item, nsDefaultComparator<elem_type, Item>());
  }

  
  
  template<class Allocator>
  typename Alloc::ResultType
  SwapElements(nsTArray_Impl<E, Allocator>& other) {
    return Alloc::Result(this->SwapArrayElements(other, sizeof(elem_type),
                                                 MOZ_ALIGNOF(elem_type)));
  }

  
  
  

  
  
  
  
  
  
  typename Alloc::ResultType SetCapacity(size_type capacity) {
    return Alloc::Result(this->EnsureCapacity(capacity, sizeof(elem_type)));
  }

  
  
  
  
  
  
  
  
  typename Alloc::ResultType SetLength(size_type newLen) {
    size_type oldLen = Length();
    if (newLen > oldLen) {
      return Alloc::ConvertBoolToResultType(InsertElementsAt(oldLen, newLen - oldLen) != nullptr);
    }

    TruncateLength(newLen);
    return Alloc::ConvertBoolToResultType(true);
  }

  
  
  
  
  
  
  void TruncateLength(size_type newLen) {
    size_type oldLen = Length();
    NS_ABORT_IF_FALSE(newLen <= oldLen,
                      "caller should use SetLength instead");
    RemoveElementsAt(newLen, oldLen - newLen);
  }

  
  
  
  
  
  
typename Alloc::ResultType EnsureLengthAtLeast(size_type minLen) {
    size_type oldLen = Length();
    if (minLen > oldLen) {
      return Alloc::ConvertBoolToResultType(!!InsertElementsAt(oldLen, minLen - oldLen));
    }
    return Alloc::ConvertBoolToResultType(true);
  }

  
  
  
  
  
  elem_type *InsertElementsAt(index_type index, size_type count) {
    if (!base_type::InsertSlotsAt(index, count, sizeof(elem_type), MOZ_ALIGNOF(elem_type))) {
      return nullptr;
    }

    
    elem_type *iter = Elements() + index, *end = iter + count;
    for (; iter != end; ++iter) {
      elem_traits::Construct(iter);
    }

    return Elements() + index;
  }

  
  
  
  
  
  
  
  template<class Item>
  elem_type *InsertElementsAt(index_type index, size_type count,
                              const Item& item) {
    if (!base_type::InsertSlotsAt(index, count, sizeof(elem_type), MOZ_ALIGNOF(elem_type))) {
      return nullptr;
    }

    
    elem_type *iter = Elements() + index, *end = iter + count;
    for (; iter != end; ++iter) {
      elem_traits::Construct(iter, item);
    }

    return Elements() + index;
  }

  
  void Compact() {
    ShrinkCapacity(sizeof(elem_type), MOZ_ALIGNOF(elem_type));
  }

  
  
  

  
  
  
  template<class Comparator>
  static int Compare(const void* e1, const void* e2, void *data) {
    const Comparator* c = reinterpret_cast<const Comparator*>(data);
    const elem_type* a = static_cast<const elem_type*>(e1);
    const elem_type* b = static_cast<const elem_type*>(e2);
    return c->LessThan(*a, *b) ? -1 : (c->Equals(*a, *b) ? 0 : 1);
  }

  
  
  
  template<class Comparator>
  void Sort(const Comparator& comp) {
    NS_QuickSort(Elements(), Length(), sizeof(elem_type),
                 Compare<Comparator>, const_cast<Comparator*>(&comp));
  }

  
  
  void Sort() {
    Sort(nsDefaultComparator<elem_type, elem_type>());
  }

  
  
  

  
  
  template<class Comparator>
  void MakeHeap(const Comparator& comp) {
    if (!Length()) {
      return;
    }
    index_type index = (Length() - 1) / 2;
    do {
      SiftDown(index, comp);
    } while (index--);
  }

  
  void MakeHeap() {
    MakeHeap(nsDefaultComparator<elem_type, elem_type>());
  }

  
  
  
  template<class Item, class Comparator>
  elem_type *PushHeap(const Item& item, const Comparator& comp) {
    if (!base_type::InsertSlotsAt(Length(), 1, sizeof(elem_type), MOZ_ALIGNOF(elem_type))) {
      return nullptr;
    }
    
    elem_type *elem = Elements();
    index_type index = Length() - 1;
    index_type parent_index = (index - 1) / 2;
    while (index && comp.LessThan(elem[parent_index], item)) {
      elem[index] = elem[parent_index];
      index = parent_index;
      parent_index = (index - 1) / 2;
    }
    elem[index] = item;
    return &elem[index];
  }

  
  template<class Item>
  elem_type *PushHeap(const Item& item) {
    return PushHeap(item, nsDefaultComparator<elem_type, Item>());
  }

  
  
  template<class Comparator>
  void PopHeap(const Comparator& comp) {
    if (!Length()) {
      return;
    }
    index_type last_index = Length() - 1;
    elem_type *elem = Elements();
    elem[0] = elem[last_index];
    TruncateLength(last_index);
    if (Length()) {
      SiftDown(0, comp);
    }
  }

  
  void PopHeap() {
    PopHeap(nsDefaultComparator<elem_type, elem_type>());
  }

protected:
  using base_type::Hdr;
  using base_type::ShrinkCapacity;

  
  
  
  void DestructRange(index_type start, size_type count) {
    elem_type *iter = Elements() + start, *end = iter + count;
    for (; iter != end; ++iter) {
      elem_traits::Destruct(iter);
    }
  }

  
  
  
  
  template<class Item>
  void AssignRange(index_type start, size_type count,
                   const Item *values) {
    AssignRangeAlgorithm<mozilla::IsPod<Item>::value,
                         mozilla::IsSame<Item, elem_type>::value>
      ::implementation(Elements(), start, count, values);
  }

  
  
  
  template<class Comparator>
  void SiftDown(index_type index, const Comparator& comp) {
    elem_type *elem = Elements();
    elem_type item = elem[index];
    index_type end = Length() - 1;
    while ((index * 2) < end) {
      const index_type left = (index * 2) + 1;
      const index_type right = (index * 2) + 2;
      const index_type parent_index = index;
      if (comp.LessThan(item, elem[left])) {
        if (left < end &&
            comp.LessThan(elem[left], elem[right])) {
          index = right;
        } else {
          index = left;
        }
      } else if (left < end &&
                 comp.LessThan(item, elem[right])) {
        index = right;
      } else {
        break;
      }
      elem[parent_index] = elem[index];
    }
    elem[index] = item;
  }
};

template <typename E, typename Alloc>
inline void
ImplCycleCollectionUnlink(nsTArray_Impl<E, Alloc>& aField)
{
  aField.Clear();
}

template <typename E, typename Alloc>
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





template <class E>
class nsTArray : public nsTArray_Impl<E, nsTArrayInfallibleAllocator>
{
public:
  typedef nsTArray_Impl<E, nsTArrayInfallibleAllocator> base_type;
  typedef nsTArray<E>                                   self_type;
  typedef typename base_type::size_type                 size_type;

  nsTArray() {}
  explicit nsTArray(size_type capacity) : base_type(capacity) {}
  explicit nsTArray(const nsTArray& other) : base_type(other) {}

  template<class Allocator>
  explicit nsTArray(const nsTArray_Impl<E, Allocator>& other) : base_type(other) {}
};




template <class E>
class FallibleTArray : public nsTArray_Impl<E, nsTArrayFallibleAllocator>
{
public:
  typedef nsTArray_Impl<E, nsTArrayFallibleAllocator>   base_type;
  typedef FallibleTArray<E>                             self_type;
  typedef typename base_type::size_type                 size_type;

  FallibleTArray() {}
  explicit FallibleTArray(size_type capacity) : base_type(capacity) {}
  explicit FallibleTArray(const FallibleTArray<E>& other) : base_type(other) {}

  template<class Allocator>
  explicit FallibleTArray(const nsTArray_Impl<E, Allocator>& other) : base_type(other) {}
};





template <class TArrayBase, size_t N>
class nsAutoArrayBase : public TArrayBase
{
public:
  typedef nsAutoArrayBase<TArrayBase, N> self_type;
  typedef TArrayBase base_type;
  typedef typename base_type::Header Header;
  typedef typename base_type::elem_type elem_type;

  template<typename Allocator>
  self_type& operator=(const nsTArray_Impl<elem_type, Allocator>& other) {
    base_type::operator=(other);
    return *this;
  }

protected:
  nsAutoArrayBase() {
    Init();
  }

  
  
  
  
  nsAutoArrayBase(const self_type &aOther) {
    Init();
    this->AppendElements(aOther);
  }

private:
  
  
  template<class Allocator, class Copier>
  friend class nsTArray_base;

  void Init() {
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

  
  
  
  
  union {
    char mAutoBuf[sizeof(nsTArrayHeader) + N * sizeof(elem_type)];
    
    mozilla::AlignedElem<(MOZ_ALIGNOF(Header) > MOZ_ALIGNOF(elem_type))
                         ? MOZ_ALIGNOF(Header) : MOZ_ALIGNOF(elem_type)> mAlign;
  };
};









template<class E, size_t N>
class nsAutoTArray : public nsAutoArrayBase<nsTArray<E>, N>
{
  typedef nsAutoTArray<E, N> self_type;
  typedef nsAutoArrayBase<nsTArray<E>, N> Base;

public:
  nsAutoTArray() {}

  template<typename Allocator>
  explicit nsAutoTArray(const nsTArray_Impl<E, Allocator>& other) {
    Base::AppendElements(other);
  }

  operator const AutoFallibleTArray<E, N>&() const {
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
  explicit AutoFallibleTArray(const nsTArray_Impl<E, Allocator>& other) {
    Base::AppendElements(other);
  }

  operator const nsAutoTArray<E, N>&() const {
    return *reinterpret_cast<const nsAutoTArray<E, N>*>(this);
  }
};














static_assert(sizeof(nsAutoTArray<uint32_t, 2>) ==
              sizeof(void*) + sizeof(nsTArrayHeader) + sizeof(uint32_t) * 2,
              "nsAutoTArray shouldn't contain any extra padding, "
              "see the comment");


#include "nsTArray-inl.h"

#endif  
