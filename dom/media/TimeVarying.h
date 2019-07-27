




#ifndef MOZILLA_TIMEVARYING_H_
#define MOZILLA_TIMEVARYING_H_

#include "nsTArray.h"

namespace mozilla {






class TimeVaryingBase {
protected:
  TimeVaryingBase()
  {
    MOZ_COUNT_CTOR(TimeVaryingBase);
  }
  ~TimeVaryingBase()
  {
    MOZ_COUNT_DTOR(TimeVaryingBase);
  }
};






















template <typename Time, typename T, uint32_t ReservedChanges>
class TimeVarying : public TimeVaryingBase {
public:
  explicit TimeVarying(const T& aInitial) : mCurrent(aInitial) {}
  



  TimeVarying() : mCurrent() {}

  


  void SetAtAndAfter(Time aTime, const T& aValue)
  {
    for (int32_t i = mChanges.Length() - 1; i >= 0; --i) {
      NS_ASSERTION(i == int32_t(mChanges.Length() - 1),
                   "Always considering last element of array");
      if (aTime > mChanges[i].mTime) {
        if (mChanges[i].mValue != aValue) {
          mChanges.AppendElement(Entry(aTime, aValue));
        }
        return;
      }
      if (aTime == mChanges[i].mTime) {
        if ((i > 0 ? mChanges[i - 1].mValue : mCurrent) == aValue) {
          mChanges.RemoveElementAt(i);
          return;
        }
        mChanges[i].mValue = aValue;
        return;
      }
      mChanges.RemoveElementAt(i);
    }
    if (mCurrent == aValue) {
      return;
    }
    mChanges.InsertElementAt(0, Entry(aTime, aValue));
  }
  




  const T& GetLast(Time* aTime = nullptr) const
  {
    if (mChanges.IsEmpty()) {
      if (aTime) {
        *aTime = INT64_MIN;
      }
      return mCurrent;
    }
    if (aTime) {
      *aTime = mChanges[mChanges.Length() - 1].mTime;
    }
    return mChanges[mChanges.Length() - 1].mValue;
  }
  


  const T& GetBefore(Time aTime) const
  {
    if (mChanges.IsEmpty() || aTime <= mChanges[0].mTime) {
      return mCurrent;
    }
    int32_t changesLength = mChanges.Length();
    if (mChanges[changesLength - 1].mTime < aTime) {
      return mChanges[changesLength - 1].mValue;
    }
    for (uint32_t i = 1; ; ++i) {
      if (aTime <= mChanges[i].mTime) {
        NS_ASSERTION(mChanges[i].mValue != mChanges[i - 1].mValue,
                     "Only changed values appear in array");
        return mChanges[i - 1].mValue;
      }
    }
  }
  










  const T& GetAt(Time aTime, Time* aEnd = nullptr, Time* aStart = nullptr) const
  {
    if (mChanges.IsEmpty() || aTime < mChanges[0].mTime) {
      if (aStart) {
        *aStart = INT64_MIN;
      }
      if (aEnd) {
        *aEnd = mChanges.IsEmpty() ? INT64_MAX : mChanges[0].mTime;
      }
      return mCurrent;
    }
    int32_t changesLength = mChanges.Length();
    if (mChanges[changesLength - 1].mTime <= aTime) {
      if (aEnd) {
        *aEnd = INT64_MAX;
      }
      if (aStart) {
        *aStart = mChanges[changesLength - 1].mTime;
      }
      return mChanges[changesLength - 1].mValue;
    }

    for (uint32_t i = 1; ; ++i) {
      if (aTime < mChanges[i].mTime) {
        if (aEnd) {
          *aEnd = mChanges[i].mTime;
        }
        if (aStart) {
          *aStart = mChanges[i - 1].mTime;
        }
        NS_ASSERTION(mChanges[i].mValue != mChanges[i - 1].mValue,
                     "Only changed values appear in array");
        return mChanges[i - 1].mValue;
      }
    }
  }
  


  void AdvanceCurrentTime(Time aTime)
  {
    for (uint32_t i = 0; i < mChanges.Length(); ++i) {
      if (aTime < mChanges[i].mTime) {
        mChanges.RemoveElementsAt(0, i);
        return;
      }
      mCurrent = mChanges[i].mValue;
    }
    mChanges.Clear();
  }
  



  void InsertTimeAtStart(Time aDelta)
  {
    for (uint32_t i = 0; i < mChanges.Length(); ++i) {
      mChanges[i].mTime += aDelta;
    }
  }

  





  void Append(const TimeVarying& aOther, Time aTimeOffset)
  {
    NS_ASSERTION(aOther.mChanges.IsEmpty() || aOther.mChanges[0].mTime >= 0,
                 "Negative time not allowed here");
    NS_ASSERTION(&aOther != this, "Can't self-append");
    SetAtAndAfter(aTimeOffset, aOther.mCurrent);
    for (uint32_t i = 0; i < aOther.mChanges.Length(); ++i) {
      const Entry& e = aOther.mChanges[i];
      SetAtAndAfter(aTimeOffset + e.mTime, e.mValue);
    }
  }

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
  {
    return mChanges.SizeOfExcludingThis(aMallocSizeOf);
  }

private:
  struct Entry {
    Entry(Time aTime, const T& aValue) : mTime(aTime), mValue(aValue) {}

    
    Time mTime;
    T mValue;
  };
  nsAutoTArray<Entry, ReservedChanges> mChanges;
  T mCurrent;
};

}

#endif 
