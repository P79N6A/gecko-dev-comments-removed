





#ifndef MOZILLA_CONTAINERPARSER_H_
#define MOZILLA_CONTAINERPARSER_H_

#include "nsRefPtr.h"
#include "nsString.h"

namespace mozilla {

class MediaLargeByteBuffer;
class SourceBufferResource;

class ContainerParser {
public:
  explicit ContainerParser(const nsACString& aType);
  virtual ~ContainerParser() {}

  
  
  
  virtual bool IsInitSegmentPresent(MediaLargeByteBuffer* aData);

  
  
  
  virtual bool IsMediaSegmentPresent(MediaLargeByteBuffer* aData);

  
  
  
  virtual bool ParseStartAndEndTimestamps(MediaLargeByteBuffer* aData,
                                          int64_t& aStart, int64_t& aEnd);

  
  
  
  bool TimestampsFuzzyEqual(int64_t aLhs, int64_t aRhs);

  virtual int64_t GetRoundingError();

  MediaLargeByteBuffer* InitData();

  bool HasInitData()
  {
    return mHasInitData;
  }

  bool HasCompleteInitData();

  static ContainerParser* CreateForMIMEType(const nsACString& aType);

protected:
  nsRefPtr<MediaLargeByteBuffer> mInitData;
  nsRefPtr<SourceBufferResource> mResource;
  bool mHasInitData;
  const nsCString mType;
};

} 
#endif 
