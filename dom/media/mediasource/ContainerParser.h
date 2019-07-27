





#ifndef MOZILLA_CONTAINERPARSER_H_
#define MOZILLA_CONTAINERPARSER_H_

#include "nsRefPtr.h"

namespace mozilla {

class LargeDataBuffer;

class ContainerParser {
public:
  ContainerParser();
  virtual ~ContainerParser() {}

  
  
  
  virtual bool IsInitSegmentPresent(LargeDataBuffer* aData);

  
  
  
  virtual bool IsMediaSegmentPresent(LargeDataBuffer* aData);

  
  
  
  virtual bool ParseStartAndEndTimestamps(LargeDataBuffer* aData,
                                          int64_t& aStart, int64_t& aEnd);

  
  
  
  bool TimestampsFuzzyEqual(int64_t aLhs, int64_t aRhs);

  virtual int64_t GetRoundingError();

  LargeDataBuffer* InitData();

  bool HasInitData()
  {
    return mHasInitData;
  }

  bool HasCompleteInitData();

  static ContainerParser* CreateForMIMEType(const nsACString& aType);

protected:
  nsRefPtr<LargeDataBuffer> mInitData;
  bool mHasInitData;
};

} 
#endif 
