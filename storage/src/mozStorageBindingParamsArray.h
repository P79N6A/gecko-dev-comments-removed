






































#ifndef _mozStorageBindingParamsArray_h_
#define _mozStorageBindingParamsArray_h_

#include "nsAutoPtr.h"
#include "nsTArray.h"

#include "mozIStorageBindingParamsArray.h"

namespace mozilla {
namespace storage {

class BindingParams;
class Statement;

class BindingParamsArray : public mozIStorageBindingParamsArray
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEBINDINGPARAMSARRAY

  BindingParamsArray(Statement *aOwningStatement);

  



  void lock();

  


  const Statement *getOwner() const;

  class iterator {
  public:
    iterator(BindingParamsArray *aArray,
             PRUint32 aIndex)
    : mArray(aArray)
    , mIndex(aIndex)
    {
    }

    iterator &operator++(int)
    {
      mIndex++;
      return *this;
    }

    bool operator==(const iterator &aOther) const
    {
      return mIndex == aOther.mIndex;
    }
    bool operator!=(const iterator &aOther) const
    {
      return !(*this == aOther);
    }
    BindingParams *operator*()
    {
      NS_ASSERTION(mIndex < mArray->mArray.Length(),
                   "Dereferenceing an invalid value!");
      return mArray->mArray[mIndex].get();
    }
  private:
    void operator--() { }
    BindingParamsArray *mArray;
    PRUint32 mIndex;
  };

  


  inline iterator begin()
  {
    NS_ASSERTION(mLocked, "Obtaining an iterator when we are not locked!");
    return iterator(this, 0);
  }

  


  inline iterator end()
  {
    NS_ASSERTION(mLocked, "Obtaining an iterator when we are not locked!");
    return iterator(this, mArray.Length());
  }
private:
  nsRefPtr<Statement> mOwningStatement;
  nsTArray< nsRefPtr<BindingParams> > mArray;
  bool mLocked;

  friend class iterator;
};

} 
} 

#endif 
