





#include "ContainerParser.h"

#include "WebMBufferedParser.h"
#include "mozilla/Endian.h"
#include "mp4_demuxer/MoofParser.h"
#include "prlog.h"
#include "MediaData.h"
#ifdef MOZ_FMP4
#include "MP4Stream.h"
#endif
#include "SourceBufferResource.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetMediaSourceLog();
extern PRLogModuleInfo* GetMediaSourceAPILog();

#define MSE_DEBUG(...) PR_LOG(GetMediaSourceLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#define MSE_DEBUGV(...) PR_LOG(GetMediaSourceLog(), PR_LOG_DEBUG+1, (__VA_ARGS__))
#define MSE_API(...) PR_LOG(GetMediaSourceAPILog(), PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define MSE_DEBUG(...)
#define MSE_DEBUGV(...)
#define MSE_API(...)
#endif

namespace mozilla {

ContainerParser::ContainerParser()
  : mHasInitData(false)
{
}

bool
ContainerParser::IsInitSegmentPresent(LargeDataBuffer* aData)
{
MSE_DEBUG("ContainerParser(%p)::IsInitSegmentPresent aLength=%u [%x%x%x%x]",
            this, aData->Length(),
            aData->Length() > 0 ? (*aData)[0] : 0,
            aData->Length() > 1 ? (*aData)[1] : 0,
            aData->Length() > 2 ? (*aData)[2] : 0,
            aData->Length() > 3 ? (*aData)[3] : 0);
return false;
}

bool
ContainerParser::IsMediaSegmentPresent(LargeDataBuffer* aData)
{
  MSE_DEBUG("ContainerParser(%p)::IsMediaSegmentPresent aLength=%u [%x%x%x%x]",
            this, aData->Length(),
            aData->Length() > 0 ? (*aData)[0] : 0,
            aData->Length() > 1 ? (*aData)[1] : 0,
            aData->Length() > 2 ? (*aData)[2] : 0,
            aData->Length() > 3 ? (*aData)[3] : 0);
  return false;
}

bool
ContainerParser::ParseStartAndEndTimestamps(LargeDataBuffer* aData,
                                            int64_t& aStart, int64_t& aEnd)
{
  return false;
}

bool
ContainerParser::TimestampsFuzzyEqual(int64_t aLhs, int64_t aRhs)
{
  return llabs(aLhs - aRhs) <= GetRoundingError();
}

int64_t
ContainerParser::GetRoundingError()
{
  NS_WARNING("Using default ContainerParser::GetRoundingError implementation");
  return 0;
}

bool
ContainerParser::HasCompleteInitData()
{
  return mHasInitData && !!mInitData->Length();
}

LargeDataBuffer*
ContainerParser::InitData()
{
  return mInitData;
}

class WebMContainerParser : public ContainerParser {
public:
  WebMContainerParser()
    : mParser(0), mOffset(0)
  {}

  static const unsigned NS_PER_USEC = 1000;
  static const unsigned USEC_PER_SEC = 1000000;

  bool IsInitSegmentPresent(LargeDataBuffer* aData)
  {
    ContainerParser::IsInitSegmentPresent(aData);
    
    
    
    
    
    
    
    
    
    
    if (aData->Length() >= 4 &&
        (*aData)[0] == 0x1a && (*aData)[1] == 0x45 && (*aData)[2] == 0xdf &&
        (*aData)[3] == 0xa3) {
      return true;
    }
    return false;
  }

  bool IsMediaSegmentPresent(LargeDataBuffer* aData)
  {
    ContainerParser::IsMediaSegmentPresent(aData);
    
    
    
    
    
    
    
    
    
    
    if (aData->Length() >= 4 &&
        (*aData)[0] == 0x1f && (*aData)[1] == 0x43 && (*aData)[2] == 0xb6 &&
        (*aData)[3] == 0x75) {
      return true;
    }
    return false;
  }

  bool ParseStartAndEndTimestamps(LargeDataBuffer* aData,
                                  int64_t& aStart, int64_t& aEnd)
  {
    bool initSegment = IsInitSegmentPresent(aData);
    if (initSegment) {
      mOffset = 0;
      mParser = WebMBufferedParser(0);
      mOverlappedMapping.Clear();
      mInitData = new LargeDataBuffer();
    }

    
    
    nsTArray<WebMTimeDataOffset> mapping;
    mapping.AppendElements(mOverlappedMapping);
    mOverlappedMapping.Clear();
    ReentrantMonitor dummy("dummy");
    mParser.Append(aData->Elements(), aData->Length(), mapping, dummy);

    
    
    
    if (initSegment) {
      uint32_t length = aData->Length();
      if (!mapping.IsEmpty()) {
        length = mapping[0].mSyncOffset;
        MOZ_ASSERT(length <= aData->Length());
      }
      MSE_DEBUG("WebMContainerParser(%p)::ParseStartAndEndTimestamps: Stashed init of %u bytes.",
                this, length);
      if (!mInitData->ReplaceElementsAt(0, mInitData->Length(),
                                        aData->Elements(), length)) {
        
        return false;
      }
      mHasInitData = true;
    }
    mOffset += aData->Length();

    if (mapping.IsEmpty()) {
      return false;
    }

    
    uint32_t endIdx = mapping.Length() - 1;
    while (mOffset < mapping[endIdx].mEndOffset && endIdx > 0) {
      endIdx -= 1;
    }

    if (endIdx == 0) {
      return false;
    }

    uint64_t frameDuration = mapping[endIdx].mTimecode - mapping[endIdx - 1].mTimecode;
    aStart = mapping[0].mTimecode / NS_PER_USEC;
    aEnd = (mapping[endIdx].mTimecode + frameDuration) / NS_PER_USEC;

    MSE_DEBUG("WebMContainerParser(%p)::ParseStartAndEndTimestamps: [%lld, %lld] [fso=%lld, leo=%lld, l=%u endIdx=%u]",
              this, aStart, aEnd, mapping[0].mSyncOffset, mapping[endIdx].mEndOffset, mapping.Length(), endIdx);

    mapping.RemoveElementsAt(0, endIdx + 1);
    mOverlappedMapping.AppendElements(mapping);

    return true;
  }

  int64_t GetRoundingError()
  {
    int64_t error = mParser.GetTimecodeScale() / NS_PER_USEC;
    return error * 2;
  }

private:
  WebMBufferedParser mParser;
  nsTArray<WebMTimeDataOffset> mOverlappedMapping;
  int64_t mOffset;
};

#ifdef MOZ_FMP4
class MP4ContainerParser : public ContainerParser {
public:
  MP4ContainerParser() :mMonitor("MP4ContainerParser Index Monitor") {}

  bool IsInitSegmentPresent(LargeDataBuffer* aData)
  {
    ContainerParser::IsInitSegmentPresent(aData);
    
    
    

    if (aData->Length() < 8) {
      return false;
    }

    uint32_t chunk_size = BigEndian::readUint32(aData->Elements());
    if (chunk_size < 8) {
      return false;
    }

    return (*aData)[4] == 'f' && (*aData)[5] == 't' && (*aData)[6] == 'y' &&
           (*aData)[7] == 'p';
  }

  bool IsMediaSegmentPresent(LargeDataBuffer* aData)
  {
    ContainerParser::IsMediaSegmentPresent(aData);
    if (aData->Length() < 8) {
      return false;
    }

    uint32_t chunk_size = BigEndian::readUint32(aData->Elements());
    if (chunk_size < 8) {
      return false;
    }

    return ((*aData)[4] == 'm' && (*aData)[5] == 'o' && (*aData)[6] == 'o' &&
            (*aData)[7] == 'f') ||
           ((*aData)[4] == 's' && (*aData)[5] == 't' && (*aData)[6] == 'y' &&
            (*aData)[7] == 'p');
  }

  bool ParseStartAndEndTimestamps(LargeDataBuffer* aData,
                                  int64_t& aStart, int64_t& aEnd)
  {
    MonitorAutoLock mon(mMonitor); 
                                   
    bool initSegment = IsInitSegmentPresent(aData);
    if (initSegment) {
      mResource = new SourceBufferResource(NS_LITERAL_CSTRING("video/mp4"));
      mStream = new MP4Stream(mResource);
      
      
      
      
      mParser = new mp4_demuxer::MoofParser(mStream, 0, 0, &mMonitor);
      mInitData = new LargeDataBuffer();
    } else if (!mStream || !mParser) {
      return false;
    }

    mResource->AppendData(aData);
    nsTArray<MediaByteRange> byteRanges;
    MediaByteRange mbr =
      MediaByteRange(mParser->mOffset, mResource->GetLength());
    byteRanges.AppendElement(mbr);
    mParser->RebuildFragmentedIndex(byteRanges);

    if (initSegment || !HasCompleteInitData()) {
      const MediaByteRange& range = mParser->mInitRange;
      uint32_t length = range.mEnd - range.mStart;
      if (length) {
        if (!mInitData->SetLength(length)) {
          
          return false;
        }
        char* buffer = reinterpret_cast<char*>(mInitData->Elements());
        mResource->ReadFromCache(buffer, range.mStart, length);
        MSE_DEBUG("MP4ContainerParser(%p)::ParseStartAndEndTimestamps: Stashed init of %u bytes.",
                  this, length);
      } else {
        MSE_DEBUG("MP4ContainerParser(%p)::ParseStartAndEndTimestamps: Incomplete init found.");
      }
      mHasInitData = true;
    }

    mp4_demuxer::Interval<mp4_demuxer::Microseconds> compositionRange =
      mParser->GetCompositionRange(byteRanges);
    mResource->EvictData(mParser->mOffset, mParser->mOffset);

    if (compositionRange.IsNull()) {
      return false;
    }
    aStart = compositionRange.start;
    aEnd = compositionRange.end;
    MSE_DEBUG("MP4ContainerParser(%p)::ParseStartAndEndTimestamps: [%lld, %lld]",
              this, aStart, aEnd);
    return true;
  }

  
  
  int64_t GetRoundingError()
  {
    return 20000;
  }

private:
  nsRefPtr<MP4Stream> mStream;
  nsAutoPtr<mp4_demuxer::MoofParser> mParser;
  nsRefPtr<SourceBufferResource> mResource;
  Monitor mMonitor;
};
#endif

 ContainerParser*
ContainerParser::CreateForMIMEType(const nsACString& aType)
{
  if (aType.LowerCaseEqualsLiteral("video/webm") || aType.LowerCaseEqualsLiteral("audio/webm")) {
    return new WebMContainerParser();
  }

#ifdef MOZ_FMP4
  if (aType.LowerCaseEqualsLiteral("video/mp4") || aType.LowerCaseEqualsLiteral("audio/mp4")) {
    return new MP4ContainerParser();
  }
#endif
  return new ContainerParser();
}

} 
