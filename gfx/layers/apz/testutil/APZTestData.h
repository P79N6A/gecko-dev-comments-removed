




#ifndef mozilla_layers_APZTestData_h
#define mozilla_layers_APZTestData_h

#include <map>

#include "FrameMetrics.h"
#include "nsDebug.h"             
#include "mozilla/Assertions.h"  
#include "mozilla/DebugOnly.h"   
#include "mozilla/ToString.h"    
#include "ipc/IPCMessageUtils.h"
#include "js/TypeDecls.h"

namespace mozilla {
namespace layers {

typedef uint32_t SequenceNumber;
















class APZTestData {
  typedef FrameMetrics::ViewID ViewID;
  friend struct IPC::ParamTraits<APZTestData>;
  friend class APZTestDataToJSConverter;
public:
  void StartNewPaint(SequenceNumber aSequenceNumber) {
    mPaints.insert(DataStore::value_type(aSequenceNumber, Bucket()));
    
    
    
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

  
  bool ToJS(JS::MutableHandleValue aOutValue, JSContext* aContext) const;

  
  
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
    scrollFrameData.insert(ScrollFrameData::value_type(aKey, aValue));
    
    
    
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

namespace IPC {

template <>
struct ParamTraits<mozilla::layers::APZTestData>
{
  typedef mozilla::layers::APZTestData paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.mPaints);
    WriteParam(aMsg, aParam.mRepaintRequests);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    return (ReadParam(aMsg, aIter, &aResult->mPaints) &&
            ReadParam(aMsg, aIter, &aResult->mRepaintRequests));
  }
};

template <>
struct ParamTraits<mozilla::layers::APZTestData::ScrollFrameData>
  : ParamTraits<mozilla::layers::APZTestData::ScrollFrameDataBase> {};

template <>
struct ParamTraits<mozilla::layers::APZTestData::Bucket>
  : ParamTraits<mozilla::layers::APZTestData::BucketBase> {};

template <>
struct ParamTraits<mozilla::layers::APZTestData::DataStore>
  : ParamTraits<mozilla::layers::APZTestData::DataStoreBase> {};

}


#endif 
