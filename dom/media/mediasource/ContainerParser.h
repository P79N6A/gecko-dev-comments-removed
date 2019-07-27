





#ifndef MOZILLA_CONTAINERPARSER_H_
#define MOZILLA_CONTAINERPARSER_H_

#include "nsRefPtr.h"
#include "nsString.h"
#include "MediaResource.h"

namespace mozilla {

class MediaByteBuffer;
class SourceBufferResource;

class ContainerParser {
public:
  explicit ContainerParser(const nsACString& aType);
  virtual ~ContainerParser() {}

  
  
  
  virtual bool IsInitSegmentPresent(MediaByteBuffer* aData);

  
  
  
  virtual bool IsMediaSegmentPresent(MediaByteBuffer* aData);

  
  
  
  virtual bool ParseStartAndEndTimestamps(MediaByteBuffer* aData,
                                          int64_t& aStart, int64_t& aEnd);

  
  
  
  bool TimestampsFuzzyEqual(int64_t aLhs, int64_t aRhs);

  virtual int64_t GetRoundingError();

  MediaByteBuffer* InitData();

  bool HasInitData()
  {
    return mHasInitData;
  }

  bool HasCompleteInitData();
  
  
  MediaByteRange InitSegmentRange();
  
  
  MediaByteRange MediaHeaderRange();
  
  
  MediaByteRange MediaSegmentRange();

  static ContainerParser* CreateForMIMEType(const nsACString& aType);

protected:
  nsRefPtr<MediaByteBuffer> mInitData;
  nsRefPtr<SourceBufferResource> mResource;
  bool mHasInitData;
  MediaByteRange mCompleteInitSegmentRange;
  MediaByteRange mCompleteMediaHeaderRange;
  MediaByteRange mCompleteMediaSegmentRange;
  const nsCString mType;
};

} 
#endif 
