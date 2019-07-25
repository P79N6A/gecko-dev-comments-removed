





































#ifndef nsTArray_h__
#define nsTArray_h__

#include <string.h>

#include "prtypes.h"
#include "nsAlgorithm.h"
#include "nscore.h"
#include "nsQuickSort.h"
#include "nsDebug.h"
#include "nsTraceRefcnt.h"
#include NEW_H















struct nsTArrayFallibleAllocator
{
  static void* Malloc(size_t size) {
    return NS_Alloc(size);
  }

  static void* Realloc(void* ptr, size_t size) {
    return NS_Realloc(ptr, size);
  }

  static void Free(void* ptr) {
    NS_Free(ptr);
  }
};

#if defined(MOZALLOC_HAVE_XMALLOC)
struct nsTArrayInfallibleAllocator
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
};
#endif

#if defined(MOZALLOC_HAVE_XMALLOC)
struct nsTArrayDefaultAllocator : public nsTArrayInfallibleAllocator { };
#else
struct nsTArrayDefaultAllocator : public nsTArrayFallibleAllocator { };
#endif




struct NS_COM_GLUE nsTArrayHeader
{
  static nsTArrayHeader sEmptyHdr;

  PRUint32 mLength;
  PRUint32 mCapacity : 31;
  PRUint32 mIsAutoArray : 1;
};







template<class Alloc>
class nsTArray_base
{
  
  
  
  template<class Allocator>
  friend class nsTArray_base;

protected:
  typedef nsTArrayHeader Header;

public:
  typedef PRUint32 size_type;
  typedef PRUint32 index_type;

  
  size_type Length() const {
    return mHdr->mLength;
  }

  
  PRBool IsEmpty() const {
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

  
  
  
  
  PRBool EnsureCapacity(size_type capacity, size_type elemSize);

  
  
  void ShrinkCapacity(size_type elemSize);
    
  
  
  
  
  
  
  
  void ShiftData(index_type start, size_type oldLen, size_type newLen,
                 size_type elemSize);

  
  
  
  
  void IncrementLength(PRUint32 n) {
    NS_ASSERTION(mHdr != EmptyHdr() || n == 0, "bad data pointer");
    mHdr->mLength += n;
  }

  
  
  
  
  
  PRBool InsertSlotsAt(index_type index, size_type count,
                       size_type elementSize);

protected:
  
  
  template<class Allocator>
  PRBool SwapArrayElements(nsTArray_base<Allocator>& other,
                           size_type elemSize);

  
  
  PRBool EnsureNotUsingAutoArrayBuffer(size_type elemSize);

  
  PRBool IsAutoArray() {
    return mHdr->mIsAutoArray;
  }

  
  
  struct AutoArray {
    Header *mHdr;
    PRUint64 aligned;
  };

  
  Header* GetAutoArrayBuffer() {
    NS_ASSERTION(IsAutoArray(), "Should be an auto array to call this");

    return reinterpret_cast<Header*>(&(reinterpret_cast<AutoArray*>(&mHdr))->aligned);
  }

  
  
  PRBool UsesAutoArrayBuffer() {
    return mHdr->mIsAutoArray && mHdr == GetAutoArrayBuffer();
  }

  
  
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
    new (static_cast<void *>(e)) E(arg);
  }
  
  static inline void Destruct(E *e) {
    e->~E();
  }
};



template <class E, class Comparator>
class nsQuickSortComparator
{
public:
  typedef E elem_type;
  
  
  
  static int Compare(const void* e1, const void* e2, void *data) {
    const Comparator* c = reinterpret_cast<const Comparator*>(data);
    const elem_type* a = static_cast<const elem_type*>(e1);
    const elem_type* b = static_cast<const elem_type*>(e2);
    return c->LessThan(*a, *b) ? -1 : (c->Equals(*a, *b) ? 0 : 1);
  }
};


template<class A, class B>
class nsDefaultComparator
{
public:
  PRBool Equals(const A& a, const B& b) const {
    return a == b;
  }
  PRBool LessThan(const A& a, const B& b) const {
    return a < b;
  }
};





































template<class E, class Alloc=nsTArrayDefaultAllocator>
class nsTArray : public nsTArray_base<Alloc>
{
public:
  typedef nsTArray_base<Alloc>           base_type;
  typedef typename base_type::size_type  size_type;
  typedef typename base_type::index_type index_type;
  typedef E                              elem_type;
  typedef nsTArray<E, Alloc>             self_type;
  typedef nsTArrayElementTraits<E>       elem_traits;

  
  
  enum {
    NoIndex = index_type(-1)
  };

  using base_type::Length;

  
  
  

  ~nsTArray() { Clear(); }

  
  
  

  nsTArray() {}

  
  explicit nsTArray(size_type capacity) {
    SetCapacity(capacity);
  }

  
  
  nsTArray(const self_type& other) {
    AppendElements(other);
  }

  template<typename Allocator>
  nsTArray(const nsTArray<E, Allocator>& other) {
    AppendElements(other);
  }

  
  
  
  nsTArray& operator=(const self_type& other) {
    ReplaceElementsAt(0, Length(), other.Elements(), other.Length());
    return *this;
  }

  
  
  bool operator==(const self_type& other) const {
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
  nsTArray& operator=(const nsTArray<E, Allocator>& other) {
    ReplaceElementsAt(0, Length(), other.Elements(), other.Length());
    return *this;
  }

  
  
  

  
  
  
  elem_type* Elements() {
    return reinterpret_cast<elem_type *>(Hdr() + 1);
  }

  
  
  
  const elem_type* Elements() const {
    return reinterpret_cast<const elem_type *>(Hdr() + 1);
  }
    
  
  
  
  
  elem_type& ElementAt(index_type i) {
    NS_ASSERTION(i < Length(), "invalid array index");
    return Elements()[i];
  }

  
  
  
  
  const elem_type& ElementAt(index_type i) const {
    NS_ASSERTION(i < Length(), "invalid array index");
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

  
  
  

  
  
  
  
  
  template<class Item, class Comparator>
  PRBool Contains(const Item& item, const Comparator& comp) const {
    return IndexOf(item, 0, comp) != NoIndex;
  }

  
  
  
  
  
  template<class Item>
  PRBool Contains(const Item& item) const {
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
    if (start >= Length())
      start = Length() - 1;
    const elem_type* end = Elements() - 1, *iter = end + start + 1;
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

  
  
  

  
  
  
  
  
  
  
  
  
  
  template<class Item>
  elem_type *ReplaceElementsAt(index_type start, size_type count,
                               const Item* array, size_type arrayLen) {
    
    if (!this->EnsureCapacity(Length() + arrayLen - count, sizeof(elem_type)))
      return nsnull;
    DestructRange(start, count);
    this->ShiftData(start, count, arrayLen, sizeof(elem_type));
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
  elem_type *InsertElementsAt(index_type index, const Item* array,
                              size_type arrayLen) {
    return ReplaceElementsAt(index, 0, array, arrayLen);
  }

  
  template<class Item>
  elem_type *InsertElementsAt(index_type index, const nsTArray<Item>& array) {
    return ReplaceElementsAt(index, 0, array.Elements(), array.Length());
  }

  
  template<class Item>
  elem_type *InsertElementAt(index_type index, const Item& item) {
    return ReplaceElementsAt(index, 0, &item, 1);
  }

  
  
  
  elem_type* InsertElementAt(index_type index) {
    if (!this->EnsureCapacity(Length() + 1, sizeof(elem_type)))
      return nsnull;
    this->ShiftData(index, 0, 1, sizeof(elem_type));
    elem_type *elem = Elements() + index;
    elem_traits::Construct(elem);
    return elem;
  }

  
  
  
  
  
  
  
  
  
  
  template<class Item, class Comparator>
  PRBool
  GreatestIndexLtEq(const Item& item,
                    const Comparator& comp,
                    index_type* idx NS_OUTPARAM) const {
    
    
    

    
    index_type low = 0, high = Length();
    while (high > low) {
      index_type mid = (high + low) >> 1;
      if (comp.Equals(ElementAt(mid), item)) {
        
        
        
        
        do {
          --mid;
        } while (NoIndex != mid && comp.Equals(ElementAt(mid), item));
        *idx = ++mid;
        return PR_TRUE;
      }
      if (comp.LessThan(ElementAt(mid), item))
        
        low = mid + 1;
      else
        
        high = mid;
    }
    
    
    
    *idx = high;
    return PR_FALSE;
  }

  
  template<class Item, class Comparator>
  PRBool
  GreatestIndexLtEq(const Item& item,
                    index_type& idx,
                    const Comparator& comp) const {
    return GreatestIndexLtEq(item, comp, &idx);
  }

  
  template<class Item>
  PRBool
  GreatestIndexLtEq(const Item& item,
                    index_type& idx) const {
    return GreatestIndexLtEq(item, nsDefaultComparator<elem_type, Item>(), &idx);
  }

  
  
  
  template<class Item, class Comparator>
  elem_type *InsertElementSorted(const Item& item, const Comparator& comp) {
    index_type index;
    GreatestIndexLtEq(item, comp, &index);
    return InsertElementAt(index, item);
  }

  
  template<class Item>
  elem_type *InsertElementSorted(const Item& item) {
    return InsertElementSorted(item, nsDefaultComparator<elem_type, Item>());
  }

  
  
  
  
  
  template<class Item>
  elem_type *AppendElements(const Item* array, size_type arrayLen) {
    if (!this->EnsureCapacity(Length() + arrayLen, sizeof(elem_type)))
      return nsnull;
    index_type len = Length();
    AssignRange(len, arrayLen, array);
    this->IncrementLength(arrayLen);
    return Elements() + len;
  }

  
  template<class Item, class Allocator>
  elem_type *AppendElements(const nsTArray<Item, Allocator>& array) {
    return AppendElements(array.Elements(), array.Length());
  }

  
  template<class Item>
  elem_type *AppendElement(const Item& item) {
    return AppendElements(&item, 1);
  }

  
  
  
  elem_type *AppendElements(size_type count) {
    if (!this->EnsureCapacity(Length() + count, sizeof(elem_type)))
      return nsnull;
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
  elem_type *MoveElementsFrom(nsTArray<Item, Allocator>& array) {
    NS_PRECONDITION(&array != this, "argument must be different array");
    index_type len = Length();
    index_type otherLen = array.Length();
    if (!this->EnsureCapacity(len + otherLen, sizeof(elem_type)))
      return nsnull;
    memcpy(Elements() + len, array.Elements(), otherLen * sizeof(elem_type));
    this->IncrementLength(otherLen);      
    array.ShiftData(0, otherLen, 0, sizeof(elem_type));
    return Elements() + len;
  }

  
  
  
  void RemoveElementsAt(index_type start, size_type count) {
    NS_ASSERTION(count == 0 || start < Length(), "Invalid start index");
    NS_ASSERTION(start + count <= Length(), "Invalid length");
    DestructRange(start, count);
    this->ShiftData(start, count, 0, sizeof(elem_type));
  }

  
  void RemoveElementAt(index_type index) {
    RemoveElementsAt(index, 1);
  }

  
  void Clear() {
    RemoveElementsAt(0, Length());
  }

  
  
  
  
  
  template<class Item, class Comparator>
  PRBool RemoveElement(const Item& item, const Comparator& comp) {
    index_type i = IndexOf(item, 0, comp);
    if (i == NoIndex)
      return PR_FALSE;

    RemoveElementAt(i);
    return PR_TRUE;
  }

  
  
  template<class Item>
  PRBool RemoveElement(const Item& item) {
    return RemoveElement(item, nsDefaultComparator<elem_type, Item>());
  }

  
  
  
  
  
  
  template<class Item, class Comparator>
  PRBool RemoveElementSorted(const Item& item, const Comparator& comp) {
    index_type index;
    PRBool found = GreatestIndexLtEq(item, comp, &index);
    if (found)
      RemoveElementAt(index);
    return found;
  }

  
  template<class Item>
  PRBool RemoveElementSorted(const Item& item) {
    return RemoveElementSorted(item, nsDefaultComparator<elem_type, Item>());
  }

  
  
  
  
  template<class Allocator>
  PRBool SwapElements(nsTArray<E, Allocator>& other) {
    return this->SwapArrayElements(other, sizeof(elem_type));
  }

  
  
  

  
  
  
  
  
  
  PRBool SetCapacity(size_type capacity) {
    return this->EnsureCapacity(capacity, sizeof(elem_type));
  }

  
  
  
  
  
  
  
  
  PRBool SetLength(size_type newLen) {
    size_type oldLen = Length();
    if (newLen > oldLen) {
      return InsertElementsAt(oldLen, newLen - oldLen) != nsnull;
    }
      
    TruncateLength(newLen);
    return PR_TRUE;
  }

  
  
  
  
  
  
  void TruncateLength(size_type newLen) {
    size_type oldLen = Length();
    NS_ABORT_IF_FALSE(newLen <= oldLen,
                      "caller should use SetLength instead");
    RemoveElementsAt(newLen, oldLen - newLen);
  }

  
  
  
  
  
  
  PRBool EnsureLengthAtLeast(size_type minLen) {
    size_type oldLen = Length();
    if (minLen > oldLen) {
      return InsertElementsAt(oldLen, minLen - oldLen) != nsnull;
    }
    return PR_TRUE;
  }

  
  
  
  
  
  elem_type *InsertElementsAt(index_type index, size_type count) {
    if (!base_type::InsertSlotsAt(index, count, sizeof(elem_type))) {
      return nsnull;
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
    if (!base_type::InsertSlotsAt(index, count, sizeof(elem_type))) {
      return nsnull;
    }

    
    elem_type *iter = Elements() + index, *end = iter + count;
    for (; iter != end; ++iter) {
      elem_traits::Construct(iter, item);
    }

    return Elements() + index;
  }

  
  void Compact() {
    ShrinkCapacity(sizeof(elem_type));
  }

  
  
  

  
  
  
  template<class Comparator>
  void Sort(const Comparator& comp) {
    NS_QuickSort(Elements(), Length(), sizeof(elem_type),
                 nsQuickSortComparator<elem_type, Comparator>::Compare,
                 const_cast<Comparator*>(&comp));
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
    if (!base_type::InsertSlotsAt(Length(), 1, sizeof(elem_type))) {
      return nsnull;
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
    elem_type *iter = Elements() + start, *end = iter + count;
    for (; iter != end; ++iter, ++values) {
      elem_traits::Construct(iter, *values);
    }
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




template<class E>
class FallibleTArray : public nsTArray<E, nsTArrayFallibleAllocator>
{
public:
  typedef nsTArray<E, nsTArrayFallibleAllocator>   base_type;
  typedef typename base_type::size_type            size_type;

  FallibleTArray() {}
  explicit FallibleTArray(size_type capacity) : base_type(capacity) {}
  FallibleTArray(const FallibleTArray& other) : base_type(other) {}
};

#ifdef MOZALLOC_HAVE_XMALLOC
template<class E>
class InfallibleTArray : public nsTArray<E, nsTArrayInfallibleAllocator>
{
public:
  typedef nsTArray<E, nsTArrayInfallibleAllocator> base_type;
  typedef typename base_type::size_type            size_type;
 
  InfallibleTArray() {}
  explicit InfallibleTArray(size_type capacity) : base_type(capacity) {}
  InfallibleTArray(const InfallibleTArray& other) : base_type(other) {}
};
#endif


template<class TArrayBase, PRUint32 N>
class nsAutoArrayBase : public TArrayBase
{
public:
  typedef TArrayBase base_type;
  typedef typename base_type::Header Header;
  typedef typename base_type::elem_type elem_type;

  nsAutoArrayBase() {
    *base_type::PtrToHdr() = reinterpret_cast<Header*>(&mAutoBuf);
    base_type::Hdr()->mLength = 0;
    base_type::Hdr()->mCapacity = N;
    base_type::Hdr()->mIsAutoArray = 1;

    NS_ASSERTION(base_type::GetAutoArrayBuffer() ==
                 reinterpret_cast<Header*>(&mAutoBuf),
                 "GetAutoArrayBuffer needs to be fixed");
  }

protected:
  union {
    char mAutoBuf[sizeof(Header) + N * sizeof(elem_type)];
    PRUint64 dummy;
  };
};

template<class E, PRUint32 N, class Alloc=nsTArrayDefaultAllocator>
class nsAutoTArray : public nsAutoArrayBase<nsTArray<E, Alloc>, N>
{
public:
  nsAutoTArray() {}
};

template<class E, PRUint32 N>
class AutoFallibleTArray : public nsAutoArrayBase<FallibleTArray<E>, N>
{
public:
  AutoFallibleTArray() {}
};

#if defined(MOZALLOC_HAVE_XMALLOC)
template<class E, PRUint32 N>
class AutoInfallibleTArray : public nsAutoArrayBase<InfallibleTArray<E>, N>
{
public:
  AutoInfallibleTArray() {}
};
#endif



template<class E>
class nsAutoTArray<E, 0, nsTArrayDefaultAllocator> :
  public nsAutoArrayBase< nsTArray<E, nsTArrayDefaultAllocator>, 0>
{
public:
  nsAutoTArray() {}
};

template<class E>
class AutoFallibleTArray<E, 0> :
  public nsAutoArrayBase< FallibleTArray<E>, 0>
{
public:
  AutoFallibleTArray() {}
};

#if defined(MOZALLOC_HAVE_XMALLOC)
template<class E>
class AutoInfallibleTArray<E, 0> :
  public nsAutoArrayBase< InfallibleTArray<E>, 0>
{
public:
  AutoInfallibleTArray() {}
};
#endif
 

#include "nsTArray-inl.h"

#endif  
