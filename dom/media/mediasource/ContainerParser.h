





#ifndef MOZILLA_CONTAINERPARSER_H_
#define MOZILLA_CONTAINERPARSER_H_

#include "nsTArray.h"

namespace mozilla {

class ContainerParser {
public:
  ContainerParser() : mHasInitData(false) {}
  virtual ~ContainerParser() {}

  
  
  
  virtual bool IsInitSegmentPresent(const uint8_t* aData, uint32_t aLength);

  
  
  
  virtual bool IsMediaSegmentPresent(const uint8_t* aData, uint32_t aLength);

  
  
  
  virtual bool ParseStartAndEndTimestamps(const uint8_t* aData, uint32_t aLength,
                                          int64_t& aStart, int64_t& aEnd);

  
  
  
  bool TimestampsFuzzyEqual(int64_t aLhs, int64_t aRhs);

  virtual int64_t GetRoundingError();

  const nsTArray<uint8_t>& InitData();

  bool HasInitData()
  {
    return mHasInitData;
  }

  static ContainerParser* CreateForMIMEType(const nsACString& aType);

protected:
  nsTArray<uint8_t> mInitData;
  bool mHasInitData;
};

} 
#endif 
