




#ifndef mozilla_layers_APZTestData_h
#define mozilla_layers_APZTestData_h

#include <map>

#include "FrameMetrics.h"
#include "nsDebug.h"             
#include "mozilla/Assertions.h"  
#include "mozilla/DebugOnly.h"   
#include "mozilla/ToString.h"    

namespace mozilla {
namespace layers {

typedef uint32_t SequenceNumber;
















class APZTestData {
  typedef FrameMetrics::ViewID ViewID;
public:
  void StartNewPaint(SequenceNumber aSequenceNumber) {
    auto insertResult = mPaints.insert(DataStore::value_type(aSequenceNumber, Bucket()));
    if (!insertResult.second) {
      
      
      
      NS_WARNING("Already have a paint with this sequence number");
    }
  }
  void LogTestDataForPaint(SequenceNumber aSequenceNumber,
                           ViewID aScrollId,
                           const std::string& aKey,
                           const std::string& aValue) {
    LogTestDataImpl(mPaints, aSequenceNumber, aScrollId, aKey, aValue);
  }

  void StartNewRepaintRequest(SequenceNumber aSequenceNumber) {
    typedef std::pair<DataStore::iterator, bool> InsertResultT;
    DebugOnly<InsertResultT> insertResult = mRepaintRequests.insert(DataStore::value_type(aSequenceNumber, Bucket()));
    MOZ_ASSERT(((InsertResultT&)insertResult).second, "Already have a repaint request with this sequence number");
  }
  void LogTestDataForRepaintRequest(SequenceNumber aSequenceNumber,
                                    ViewID aScrollId,
                                    const std::string& aKey,
                                    const std::string& aValue) {
    LogTestDataImpl(mRepaintRequests, aSequenceNumber, aScrollId, aKey, aValue);
  }

  
  
  typedef std::map<std::string, std::string> ScrollFrameDataBase;
  struct ScrollFrameData : ScrollFrameDataBase {};
  typedef std::map<ViewID, ScrollFrameData> BucketBase;
  struct Bucket : BucketBase {};
  typedef std::map<SequenceNumber, Bucket> DataStoreBase;
  struct DataStore : DataStoreBase {};
private:
  DataStore mPaints;
  DataStore mRepaintRequests;

  void LogTestDataImpl(DataStore& aDataStore,
                       SequenceNumber aSequenceNumber,
                       ViewID aScrollId,
                       const std::string& aKey,
                       const std::string& aValue) {
    auto bucketIterator = aDataStore.find(aSequenceNumber);
    if (bucketIterator == aDataStore.end()) {
      MOZ_ASSERT(false, "LogTestDataImpl called with nonexistent sequence number");
      return;
    }
    Bucket& bucket = bucketIterator->second;
    ScrollFrameData& scrollFrameData = bucket[aScrollId];  
    auto insertResult = scrollFrameData.insert(ScrollFrameData::value_type(aKey, aValue));
    if (!insertResult.second) {
      
      
      
      NS_WARNING("Key already present in test data, not overwriting");
    }
  }
};


class APZPaintLogHelper {
public:
  APZPaintLogHelper(APZTestData* aTestData, SequenceNumber aPaintSequenceNumber)
    : mTestData(aTestData),
      mPaintSequenceNumber(aPaintSequenceNumber)
  {}

  template <typename Value>
  void LogTestData(FrameMetrics::ViewID aScrollId,
                   const std::string& aKey,
                   const Value& aValue) const {
    if (mTestData) {  
      LogTestData(aScrollId, aKey, ToString(aValue));
    }
  }

  void LogTestData(FrameMetrics::ViewID aScrollId,
                   const std::string& aKey,
                   const std::string& aValue) const {
    if (mTestData) {
      mTestData->LogTestDataForPaint(mPaintSequenceNumber, aScrollId, aKey, aValue);
    }
  }
private:
  APZTestData* mTestData;
  SequenceNumber mPaintSequenceNumber;
};

}
}

#endif 
