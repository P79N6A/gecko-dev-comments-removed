




































#ifndef nsTObserverArray_h___
#define nsTObserverArray_h___

#include "nsVoidArray.h"

class nsTObserverArray_base {
  public:
    class Iterator_base;
    friend class Iterator_base;

    class Iterator_base {
      protected:
        friend class nsTObserverArray_base;

        Iterator_base(PRInt32 aPosition, nsTObserverArray_base& aArray)
          : mPosition(aPosition),
            mNext(aArray.mIterators),
            mArray(aArray) {
          aArray.mIterators = this;
        }

        ~Iterator_base() {
          NS_ASSERTION(mArray.mIterators == this,
                       "Iterators must currently be destroyed in opposite order "
                       "from the construction order. It is suggested that you "
                       "simply put them on the stack");
          mArray.mIterators = mNext;
        }

        
        
        void* GetSafeElementAt(PRInt32 aIndex) {
          return mArray.mObservers.SafeElementAt(aIndex);
        }

        
        
        
        
        PRInt32 mPosition;

        
        Iterator_base* mNext;

        
        nsTObserverArray_base& mArray;
    };

    




    void Clear();

  protected:
    nsTObserverArray_base()
      : mIterators(nsnull) {
    }

    






    void AdjustIterators(PRInt32 aModPos, PRInt32 aAdjustment);

    Iterator_base* mIterators;
    nsVoidArray mObservers;
};








template<class T>
class nsTObserverArray : public nsTObserverArray_base {
  public:

    



    PRBool PrependObserver(T* aObserver) {
      NS_PRECONDITION(!Contains(aObserver),
                      "Don't prepend if the observer is already in the list");

      PRBool res = mObservers.InsertElementAt(aObserver, 0);
      if (res) {
        AdjustIterators(0, 1);
      }
      return res;
    }

    





    PRBool AppendObserver(T* aObserver) {
      return Contains(aObserver) || mObservers.AppendElement(aObserver);
    }

    




    PRBool RemoveObserver(T* aObserver) {
      PRInt32 index = mObservers.IndexOf(aObserver);
      if (index < 0) {
        return PR_FALSE;
      }

      mObservers.RemoveElementAt(index);
      AdjustIterators(index, -1);

      return PR_TRUE;
    }

    PRBool Contains(T* aObserver) const {
      return mObservers.IndexOf(aObserver) >= 0;
    }

    PRBool IsEmpty() const {
      return mObservers.Count() == 0;
    }

    T* SafeObserverAt(PRInt32 aIndex) {
      return NS_STATIC_CAST(T*, mObservers.SafeElementAt(aIndex));
    }

    



    
    
    
    class ForwardIterator : public nsTObserverArray_base::Iterator_base {
      public:
        ForwardIterator(nsTObserverArray<T>& aArray)
          : Iterator_base(0, aArray) {
        }

        





        T* GetNext() {
          return NS_STATIC_CAST(T*, GetSafeElementAt(mPosition++));
        }
    };
};



#define NS_OBSERVER_ARRAY_NOTIFY_OBSERVERS(array_, obstype_, func_, params_) \
  PR_BEGIN_MACRO                                                             \
    nsTObserverArray<obstype_>::ForwardIterator iter_(array_);               \
    nsCOMPtr<obstype_> obs_;                                                 \
    while ((obs_ = iter_.GetNext())) {                                       \
      obs_ -> func_ params_ ;                                                \
    }                                                                        \
  PR_END_MACRO

#endif 
