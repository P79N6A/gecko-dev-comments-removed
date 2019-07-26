



#include "VideoUtils.h"
#include "MediaResource.h"
#include "MP3FrameParser.h"
#include "mozilla/dom/TimeRanges.h"
#include "nsMathUtils.h"
#include "nsSize.h"

#include <stdint.h>



CheckedInt64 FramesToUsecs(int64_t aFrames, uint32_t aRate) {
  return (CheckedInt64(aFrames) * USECS_PER_S) / aRate;
}



CheckedInt64 UsecsToFrames(int64_t aUsecs, uint32_t aRate) {
  return (CheckedInt64(aUsecs) * aRate) / USECS_PER_S;
}

static int32_t ConditionDimension(float aValue)
{
  
  if (aValue > 1.0 && aValue <= INT32_MAX)
    return int32_t(NS_round(aValue));
  return 0;
}

void ScaleDisplayByAspectRatio(nsIntSize& aDisplay, float aAspectRatio)
{
  if (aAspectRatio > 1.0) {
    
    aDisplay.width = ConditionDimension(aAspectRatio * aDisplay.width);
  } else {
    
    aDisplay.height = ConditionDimension(aDisplay.height / aAspectRatio);
  }
}

static int64_t BytesToTime(int64_t offset, int64_t length, int64_t durationUs) {
  NS_ASSERTION(length > 0, "Must have positive length");
  double r = double(offset) / double(length);
  if (r > 1.0)
    r = 1.0;
  return int64_t(double(durationUs) * r);
}

void GetEstimatedBufferedTimeRanges(mozilla::MediaResource* aStream,
                                    int64_t aDurationUsecs,
                                    mozilla::dom::TimeRanges* aOutBuffered)
{
  
  if (aDurationUsecs <= 0 || !aStream || !aOutBuffered)
    return;

  
  if (aStream->IsDataCachedToEndOfResource(0)) {
    aOutBuffered->Add(0, double(aDurationUsecs) / USECS_PER_S);
    return;
  }

  int64_t totalBytes = aStream->GetLength();

  
  
  
  if (totalBytes <= 0)
    return;

  int64_t startOffset = aStream->GetNextCachedData(0);
  while (startOffset >= 0) {
    int64_t endOffset = aStream->GetCachedDataEnd(startOffset);
    
    NS_ASSERTION(startOffset >= 0, "Integer underflow in GetBuffered");
    NS_ASSERTION(endOffset >= 0, "Integer underflow in GetBuffered");

    int64_t startUs = BytesToTime(startOffset, totalBytes, aDurationUsecs);
    int64_t endUs = BytesToTime(endOffset, totalBytes, aDurationUsecs);
    if (startUs != endUs) {
      aOutBuffered->Add(double(startUs) / USECS_PER_S,
                        double(endUs) / USECS_PER_S);
    }
    startOffset = aStream->GetNextCachedData(endOffset);
  }
  return;
}

static const uint32_t READ_SIZE = 4096;

int64_t
GetEstimatedMP3Duration(mozilla::MediaResource *aResource,
                        mozilla::MP3FrameParser *aParser)
{
  if (!aResource || !aParser) {
    return -1;
  }

  while (aParser->IsMP3()) {
    
    uint64_t parserOffset = aParser->GetLastStreamOffset();
    int64_t readOffset = aResource->GetNextCachedData(parserOffset);

    if (readOffset < 0) {
      
      break;
    }

    char buffer[READ_SIZE];
    uint64_t readLength = aResource->GetCachedDataEnd(readOffset) - readOffset;
    readLength = readLength > READ_SIZE ? READ_SIZE : readLength;

    nsresult rv = aResource->ReadFromCache(buffer, readOffset, readLength);
    if (NS_FAILED(rv)) {
      return -1;
    }

    aParser->Parse(buffer, readLength, readOffset);
  }

  return aParser->GetDuration();
}
