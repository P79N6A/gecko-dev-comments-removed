





#ifndef nsTObserverArray_h___
#define nsTObserverArray_h___

#include "mozilla/MemoryReporting.h"
#include "nsTArray.h"
#include "nsCycleCollectionNoteChild.h"











class nsTObserverArray_base
{
public:
  typedef size_t index_type;
  typedef size_t size_type;
  typedef ptrdiff_t diff_type;

protected:
  class Iterator_base
  {
  protected:
    friend class nsTObserverArray_base;

    Iterator_base(index_type aPosition, Iterator_base* aNext)
      : mPosition(aPosition)
      , mNext(aNext)
    {
    }

    
    
    index_type mPosition;

    
    Iterator_base* mNext;
  };

  nsTObserverArray_base() : mIterators(nullptr) {}

  ~nsTObserverArray_base()
  {
    NS_ASSERTION(mIterators == nullptr, "iterators outlasting array");
  }

  






  void AdjustIterators(index_type aModPos, diff_type aAdjustment);

  


  void ClearIterators();

  mutable Iterator_base* mIterators;
};

template<class T, size_t N>
class nsAutoTObserverArray : protected nsTObserverArray_base
{
public:
  typedef T           elem_type;
  typedef nsTArray<T> array_type;

  nsAutoTObserverArray() {}

  
  
  

  
  size_type Length() const { return mArray.Length(); }

  
  bool IsEmpty() const { return mArray.IsEmpty(); }

  
  
  
  
  
  elem_type& ElementAt(index_type aIndex)
  {
    return mArray.ElementAt(aIndex);
  }

  
  const elem_type& ElementAt(index_type aIndex) const
  {
    return mArray.ElementAt(aIndex);
  }

  
  
  
  
  
  elem_type& SafeElementAt(index_type aIndex, elem_type& aDef)
  {
    return mArray.SafeElementAt(aIndex, aDef);
  }

  
  const elem_type& SafeElementAt(index_type aIndex, const elem_type& aDef) const
  {
    return mArray.SafeElementAt(aIndex, aDef);
  }

  
  
  

  
  
  

  
  
  
  
  
  template<class Item>
  bool Contains(const Item& aItem) const
  {
    return IndexOf(aItem) != array_type::NoIndex;
  }

  
  
  
  
  
  
  template<class Item>
  index_type IndexOf(const Item& aItem, index_type aStart = 0) const
  {
    return mArray.IndexOf(aItem, aStart);
  }

  
  
  

  
  
  
  
  template<class Item>
  elem_type* InsertElementAt(index_type aIndex, const Item& aItem)
  {
    elem_type* item = mArray.InsertElementAt(aIndex, aItem);
    AdjustIterators(aIndex, 1);
    return item;
  }

  
  
  elem_type* InsertElementAt(index_type aIndex)
  {
    elem_type* item = mArray.InsertElementAt(aIndex);
    AdjustIterators(aIndex, 1);
    return item;
  }

  
  
  
  
  template<class Item>
  bool PrependElementUnlessExists(const Item& aItem)
  {
    if (Contains(aItem)) {
      return true;
    }

    bool inserted = mArray.InsertElementAt(0, aItem) != nullptr;
    AdjustIterators(0, 1);
    return inserted;
  }

  
  
  
  template<class Item>
  elem_type* AppendElement(const Item& aItem)
  {
    return mArray.AppendElement(aItem);
  }

  
  
  elem_type* AppendElement()
  {
    return mArray.AppendElement();
  }

  
  
  
  
  template<class Item>
  bool AppendElementUnlessExists(const Item& aItem)
  {
    return Contains(aItem) || AppendElement(aItem) != nullptr;
  }

  
  
  void RemoveElementAt(index_type aIndex)
  {
    NS_ASSERTION(aIndex < mArray.Length(), "invalid index");
    mArray.RemoveElementAt(aIndex);
    AdjustIterators(aIndex, -1);
  }

  
  
  
  
  
  template<class Item>
  bool RemoveElement(const Item& aItem)
  {
    index_type index = mArray.IndexOf(aItem, 0);
    if (index == array_type::NoIndex) {
      return false;
    }

    mArray.RemoveElementAt(index);
    AdjustIterators(index, -1);
    return true;
  }

  
  
  
  void Clear()
  {
    mArray.Clear();
    ClearIterators();
  }

  
  void Compact() { mArray.Compact(); }

  
  
  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return mArray.SizeOfExcludingThis(aMallocSizeOf);
  }

  
  
  

  
  class Iterator : public Iterator_base
  {
  protected:
    friend class nsAutoTObserverArray;
    typedef nsAutoTObserverArray<T, N> array_type;

    Iterator(index_type aPosition, const array_type& aArray)
      : Iterator_base(aPosition, aArray.mIterators)
      , mArray(const_cast<array_type&>(aArray))
    {
      aArray.mIterators = this;
    }

    ~Iterator()
    {
      NS_ASSERTION(mArray.mIterators == this,
                   "Iterators must currently be destroyed in opposite order "
                   "from the construction order. It is suggested that you "
                   "simply put them on the stack");
      mArray.mIterators = mNext;
    }

    
    array_type& mArray;
  };

  
  
  
  
  
  
  
  class ForwardIterator : protected Iterator
  {
  public:
    typedef nsAutoTObserverArray<T, N> array_type;
    typedef Iterator                   base_type;

    explicit ForwardIterator(const array_type& aArray)
      : Iterator(0, aArray)
    {
    }

    ForwardIterator(const array_type& aArray, index_type aPos)
      : Iterator(aPos, aArray)
    {
    }

    bool operator<(const ForwardIterator& aOther) const
    {
      NS_ASSERTION(&this->mArray == &aOther.mArray,
                   "not iterating the same array");
      return base_type::mPosition < aOther.mPosition;
    }

    
    
    
    bool HasMore() const
    {
      return base_type::mPosition < base_type::mArray.Length();
    }

    
    
    
    elem_type& GetNext()
    {
      NS_ASSERTION(HasMore(), "iterating beyond end of array");
      return base_type::mArray.ElementAt(base_type::mPosition++);
    }
  };

  
  
  class EndLimitedIterator : protected ForwardIterator
  {
  public:
    typedef nsAutoTObserverArray<T, N> array_type;
    typedef Iterator                   base_type;

    explicit EndLimitedIterator(const array_type& aArray)
      : ForwardIterator(aArray)
      , mEnd(aArray, aArray.Length())
    {
    }

    
    
    
    bool HasMore() const { return *this < mEnd; }

    
    
    
    elem_type& GetNext()
    {
      NS_ASSERTION(HasMore(), "iterating beyond end of array");
      return base_type::mArray.ElementAt(base_type::mPosition++);
    }

  private:
    ForwardIterator mEnd;
  };

  
  
  
  
  
  
  
  class BackwardIterator : protected Iterator
  {
  public:
    typedef nsAutoTObserverArray<T, N> array_type;
    typedef Iterator                   base_type;

    explicit BackwardIterator(const array_type& aArray)
      : Iterator(aArray.Length(), aArray)
    {
    }

    
    
    
    bool HasMore() const { return base_type::mPosition > 0; }

    
    
    
    elem_type& GetNext()
    {
      NS_ASSERTION(HasMore(), "iterating beyond start of array");
      return base_type::mArray.ElementAt(--base_type::mPosition);
    }

    
    
    
    void Remove()
    {
      return base_type::mArray.RemoveElementAt(base_type::mPosition);
    }
  };

protected:
  nsAutoTArray<T, N> mArray;
};

template<class T>
class nsTObserverArray : public nsAutoTObserverArray<T, 0>
{
public:
  typedef nsAutoTObserverArray<T, 0>       base_type;
  typedef nsTObserverArray_base::size_type size_type;

  
  
  

  nsTObserverArray() {}

  
  explicit nsTObserverArray(size_type aCapacity)
  {
    base_type::mArray.SetCapacity(aCapacity);
  }
};

template<typename T, size_t N>
inline void
ImplCycleCollectionUnlink(nsAutoTObserverArray<T, N>& aField)
{
  aField.Clear();
}

template<typename T, size_t N>
inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            nsAutoTObserverArray<T, N>& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  aFlags |= CycleCollectionEdgeNameArrayFlag;
  size_t length = aField.Length();
  for (size_t i = 0; i < length; ++i) {
    ImplCycleCollectionTraverse(aCallback, aField.ElementAt(i), aName, aFlags);
  }
}




#define NS_OBSERVER_ARRAY_NOTIFY_XPCOM_OBSERVERS(array_, obstype_, func_, params_) \
  PR_BEGIN_MACRO                                                             \
    nsTObserverArray<obstype_ *>::ForwardIterator iter_(array_);             \
    nsRefPtr<obstype_> obs_;                                                 \
    while (iter_.HasMore()) {                                                 \
      obs_ = iter_.GetNext();                                                \
      obs_ -> func_ params_ ;                                                \
    }                                                                        \
  PR_END_MACRO


#define NS_OBSERVER_ARRAY_NOTIFY_OBSERVERS(array_, obstype_, func_, params_) \
  PR_BEGIN_MACRO                                                             \
    nsTObserverArray<obstype_ *>::ForwardIterator iter_(array_);             \
    obstype_* obs_;                                                          \
    while (iter_.HasMore()) {                                                \
      obs_ = iter_.GetNext();                                                \
      obs_ -> func_ params_ ;                                                \
    }                                                                        \
  PR_END_MACRO
#endif 
