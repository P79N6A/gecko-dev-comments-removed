





#ifndef mozStorageBindingParamsArray_h
#define mozStorageBindingParamsArray_h

#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "mozilla/Attributes.h"

#include "mozIStorageBindingParamsArray.h"

namespace mozilla {
namespace storage {

class StorageBaseStatementInternal;

class BindingParamsArray MOZ_FINAL : public mozIStorageBindingParamsArray
{
  typedef nsTArray< nsCOMPtr<mozIStorageBindingParams> > array_type;

  ~BindingParamsArray() {}

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_MOZISTORAGEBINDINGPARAMSARRAY

  explicit BindingParamsArray(StorageBaseStatementInternal *aOwningStatement);

  typedef array_type::size_type size_type;

  



  void lock();

  


  const StorageBaseStatementInternal *getOwner() const;

  


  const size_type length() const { return mArray.Length(); }

  class iterator {
  public:
    iterator(BindingParamsArray *aArray,
             uint32_t aIndex)
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
    mozIStorageBindingParams *operator*()
    {
      NS_ASSERTION(mIndex < mArray->length(),
                   "Dereferenceing an invalid value!");
      return mArray->mArray[mIndex].get();
    }
  private:
    void operator--() { }
    BindingParamsArray *mArray;
    uint32_t mIndex;
  };

  


  inline iterator begin()
  {
    NS_ASSERTION(length() != 0,
                 "Obtaining an iterator to the beginning with no elements!");
    return iterator(this, 0);
  }

  


  inline iterator end()
  {
    NS_ASSERTION(mLocked,
                 "Obtaining an iterator to the end when we are not locked!");
    return iterator(this, length());
  }
private:
  nsCOMPtr<StorageBaseStatementInternal> mOwningStatement;
  array_type mArray;
  bool mLocked;

  friend class iterator;
};

} 
} 

#endif 
