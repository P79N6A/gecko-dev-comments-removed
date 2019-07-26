




#ifndef FRAMEPROPERTYTABLE_H_
#define FRAMEPROPERTYTABLE_H_

#include "nsTArray.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"

class nsIFrame;

namespace mozilla {

struct FramePropertyDescriptor;

typedef void (*FramePropertyDestructor)(void* aPropertyValue);
typedef void (*FramePropertyDestructorWithFrame)(nsIFrame* aFrame,
                                                 void* aPropertyValue);












struct FramePropertyDescriptor {
  


  FramePropertyDestructor          mDestructor;
  







  FramePropertyDestructorWithFrame mDestructorWithFrame;
  



};












class FramePropertyTable {
public:
  FramePropertyTable() : mLastFrame(nullptr), mLastEntry(nullptr)
  {
    mEntries.Init();
  }
  ~FramePropertyTable()
  {
    DeleteAll();
  }

  





  void Set(nsIFrame* aFrame, const FramePropertyDescriptor* aProperty,
           void* aValue);
  









  void* Get(const nsIFrame* aFrame, const FramePropertyDescriptor* aProperty,
            bool* aFoundResult = nullptr);
  










  void* Remove(nsIFrame* aFrame, const FramePropertyDescriptor* aProperty,
               bool* aFoundResult = nullptr);
  





  void Delete(nsIFrame* aFrame, const FramePropertyDescriptor* aProperty);
  



  void DeleteAllFor(nsIFrame* aFrame);
  


  void DeleteAll();

  size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

protected:
  



  struct PropertyValue {
    PropertyValue() : mProperty(nullptr), mValue(nullptr) {}
    PropertyValue(const FramePropertyDescriptor* aProperty, void* aValue)
      : mProperty(aProperty), mValue(aValue) {}

    bool IsArray() { return !mProperty && mValue; }
    nsTArray<PropertyValue>* ToArray()
    {
      NS_ASSERTION(IsArray(), "Must be array");
      return reinterpret_cast<nsTArray<PropertyValue>*>(&mValue);
    }

    void DestroyValueFor(nsIFrame* aFrame) {
      if (mProperty->mDestructor) {
        mProperty->mDestructor(mValue);
      } else if (mProperty->mDestructorWithFrame) {
        mProperty->mDestructorWithFrame(aFrame, mValue);
      }
    }

    size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) {
      size_t n = 0;
      
      
      
      
      
      if (IsArray()) {
        nsTArray<PropertyValue>* array = ToArray();
        n += array->SizeOfExcludingThis(aMallocSizeOf);
      }
      return n;
    }

    const FramePropertyDescriptor* mProperty;
    void* mValue;
  };

  



  class PropertyComparator {
  public:
    bool Equals(const PropertyValue& a, const PropertyValue& b) const {
      return a.mProperty == b.mProperty;
    }
    bool Equals(const FramePropertyDescriptor* a, const PropertyValue& b) const {
      return a == b.mProperty;
    }
    bool Equals(const PropertyValue& a, const FramePropertyDescriptor* b) const {
      return a.mProperty == b;
    }
  };

  



  class Entry : public nsPtrHashKey<nsIFrame>
  {
  public:
    Entry(KeyTypePointer aKey) : nsPtrHashKey<nsIFrame>(aKey) {}
    Entry(const Entry &toCopy) :
      nsPtrHashKey<nsIFrame>(toCopy), mProp(toCopy.mProp) {}

    PropertyValue mProp;
  };

  static void DeleteAllForEntry(Entry* aEntry);
  static PLDHashOperator DeleteEnumerator(Entry* aEntry, void* aArg);

  static size_t SizeOfPropertyTableEntryExcludingThis(Entry* aEntry,
                  nsMallocSizeOfFun aMallocSizeOf, void *);

  nsTHashtable<Entry> mEntries;
  nsIFrame* mLastFrame;
  Entry* mLastEntry;
};




class FrameProperties {
public:
  FrameProperties(FramePropertyTable* aTable, nsIFrame* aFrame)
    : mTable(aTable), mFrame(aFrame) {}
  FrameProperties(FramePropertyTable* aTable, const nsIFrame* aFrame)
    : mTable(aTable), mFrame(const_cast<nsIFrame*>(aFrame)) {}

  void Set(const FramePropertyDescriptor* aProperty, void* aValue) const
  {
    mTable->Set(mFrame, aProperty, aValue);
  }
  void* Get(const FramePropertyDescriptor* aProperty,
            bool* aFoundResult = nullptr) const
  {
    return mTable->Get(mFrame, aProperty, aFoundResult);
  }
  void* Remove(const FramePropertyDescriptor* aProperty,
               bool* aFoundResult = nullptr) const
  {
    return mTable->Remove(mFrame, aProperty, aFoundResult);
  }
  void Delete(const FramePropertyDescriptor* aProperty)
  {
    mTable->Delete(mFrame, aProperty);
  }

private:
  FramePropertyTable* mTable;
  nsIFrame* mFrame;
};

}

#endif 
