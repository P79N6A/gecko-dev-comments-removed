





































#ifndef nsTObserverArray_h___
#define nsTObserverArray_h___

#include "nsTArray.h"











class NS_COM_GLUE nsTObserverArray_base {
  public:
    typedef PRUint32 index_type;
    typedef PRUint32 size_type;
    typedef PRInt32  diff_type;

  protected:
    class Iterator_base {
      protected:
        friend class nsTObserverArray_base;

        Iterator_base(index_type aPosition, Iterator_base* aNext)
          : mPosition(aPosition),
            mNext(aNext) {
        }

        
        
        index_type mPosition;

        
        Iterator_base* mNext;
    };

    nsTObserverArray_base()
      : mIterators(nsnull) {
    }

    ~nsTObserverArray_base() {
      NS_ASSERTION(mIterators == nsnull, "iterators outlasting array");
    }

    






    void AdjustIterators(index_type aModPos, diff_type aAdjustment);

    


    void ClearIterators();

    mutable Iterator_base* mIterators;
};

template<class T, PRUint32 N>
class nsAutoTObserverArray : protected nsTObserverArray_base {
  public:
    typedef T           elem_type;
    typedef nsTArray<T> array_type;

    nsAutoTObserverArray() {
    }

    
    
    

    
    size_type Length() const {
      return mArray.Length();
    }

    
    bool IsEmpty() const {
      return mArray.IsEmpty();
    }

    
    
    
    
    elem_type& ElementAt(index_type i) {
      return mArray.ElementAt(i);
    }

    
    const elem_type& ElementAt(index_type i) const {
      return mArray.ElementAt(i);
    }

    
    
    
    
    
    elem_type& SafeElementAt(index_type i, elem_type& def) {
      return mArray.SafeElementAt(i, def);
    }

    
    const elem_type& SafeElementAt(index_type i, const elem_type& def) const {
      return mArray.SafeElementAt(i, def);
    }

    
    
    

    
    
    
    
    
    template<class Item>
    bool Contains(const Item& item) const {
      return IndexOf(item) != array_type::NoIndex;
    }

    
    
    
    
    
    
    template<class Item>
    index_type IndexOf(const Item& item, index_type start = 0) const {
      return mArray.IndexOf(item, start);
    }

    
    
    

    
    
    
    
    template<class Item>
    bool PrependElementUnlessExists(const Item& item) {
      return Contains(item) || mArray.InsertElementAt(0, item) != nsnull;
    }

    
    
    
    template<class Item>
    elem_type* AppendElement(const Item& item) {
      return mArray.AppendElement(item);
    }

    
    
    elem_type* AppendElement() {
      return mArray.AppendElement();
    }

    
    
    
    
    template<class Item>
    bool AppendElementUnlessExists(const Item& item) {
      return Contains(item) || AppendElement(item) != nsnull;
    }

    
    
    void RemoveElementAt(index_type index) {
      NS_ASSERTION(index < mArray.Length(), "invalid index");
      mArray.RemoveElementAt(index);
      AdjustIterators(index, -1);
    }

    
    
    
    
    
    template<class Item>
    bool RemoveElement(const Item& item) {
      index_type index = mArray.IndexOf(item, 0);
      if (index == array_type::NoIndex)
        return false;

      mArray.RemoveElementAt(index);
      AdjustIterators(index, -1);
      return true;
    }

    
    
    
    void Clear() {
      mArray.Clear();
      ClearIterators();
    }

    
    
    PRUint64 SizeOf() {
      return mArray.SizeOf();
    }

    
    
    

    
    class Iterator : public Iterator_base {
      protected:
        friend class nsAutoTObserverArray;
        typedef nsAutoTObserverArray<T, N> array_type;

        Iterator(index_type aPosition, const array_type& aArray)
          : Iterator_base(aPosition, aArray.mIterators),
            mArray(const_cast<array_type&>(aArray)) {
          aArray.mIterators = this;
        }

        ~Iterator() {
          NS_ASSERTION(mArray.mIterators == this,
                       "Iterators must currently be destroyed in opposite order "
                       "from the construction order. It is suggested that you "
                       "simply put them on the stack");
          mArray.mIterators = mNext;
        }

        
        array_type& mArray;
    };

    
    
    
    
    
    
    
    class ForwardIterator : protected Iterator {
      public:
        typedef nsAutoTObserverArray<T, N> array_type;
        typedef Iterator                   base_type;

        ForwardIterator(const array_type& aArray)
          : Iterator(0, aArray) {
        }

        ForwardIterator(const array_type& aArray, index_type aPos)
          : Iterator(aPos, aArray) {
        }

        bool operator <(const ForwardIterator& aOther) const {
          NS_ASSERTION(&this->mArray == &aOther.mArray,
                       "not iterating the same array");
          return base_type::mPosition < aOther.mPosition;
        }

        
        
        
        bool HasMore() const {
          return base_type::mPosition < base_type::mArray.Length();
        }

        
        
        
        elem_type& GetNext() {
          NS_ASSERTION(HasMore(), "iterating beyond end of array");
          return base_type::mArray.ElementAt(base_type::mPosition++);
        }
    };

    
    
    class EndLimitedIterator : protected ForwardIterator {
      public:
        typedef nsAutoTObserverArray<T, N> array_type;
        typedef Iterator                   base_type;

        EndLimitedIterator(const array_type& aArray)
          : ForwardIterator(aArray),
            mEnd(aArray, aArray.Length()) {
        }

        
        
        
        bool HasMore() const {
          return *this < mEnd;
        }

        
        
        
        elem_type& GetNext() {
          NS_ASSERTION(HasMore(), "iterating beyond end of array");
          return base_type::mArray.ElementAt(base_type::mPosition++);
        }

      private:
        ForwardIterator mEnd;
    };

  protected:
    nsAutoTArray<T, N> mArray;
};

template<class T>
class nsTObserverArray : public nsAutoTObserverArray<T, 0> {
  public:
    typedef nsAutoTObserverArray<T, 0>       base_type;
    typedef nsTObserverArray_base::size_type size_type;

    
    
    

    nsTObserverArray() {}

    
    explicit nsTObserverArray(size_type capacity) {
      base_type::mArray.SetCapacity(capacity);
    }
};




#define NS_OBSERVER_ARRAY_NOTIFY_XPCOM_OBSERVERS(array_, obstype_, func_, params_) \
  PR_BEGIN_MACRO                                                             \
    nsTObserverArray<obstype_ *>::ForwardIterator iter_(array_);             \
    nsCOMPtr<obstype_> obs_;                                                 \
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
