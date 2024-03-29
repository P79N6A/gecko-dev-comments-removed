





#include "ContainerParser.h"

#include "WebMBufferedParser.h"
#include "mozilla/Endian.h"
#include "mozilla/ErrorResult.h"
#include "mp4_demuxer/MoofParser.h"
#include "mozilla/Logging.h"
#include "MediaData.h"
#ifdef MOZ_FMP4
#include "MP4Stream.h"
#include "mp4_demuxer/AtomType.h"
#include "mp4_demuxer/ByteReader.h"
#endif
#include "SourceBufferResource.h"

extern PRLogModuleInfo* GetMediaSourceSamplesLog();


#ifdef _MSC_VER
#define __func__ __FUNCTION__
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define MSE_DEBUG(name, arg, ...) MOZ_LOG(GetMediaSourceSamplesLog(), mozilla::LogLevel::Debug, (TOSTRING(name) "(%p:%s)::%s: " arg, this, mType.get(), __func__, ##__VA_ARGS__))
#define MSE_DEBUGV(name, arg, ...) MOZ_LOG(GetMediaSourceSamplesLog(), mozilla::LogLevel::Verbose, (TOSTRING(name) "(%p:%s)::%s: " arg, this, mType.get(), __func__, ##__VA_ARGS__))

namespace mozilla {

ContainerParser::ContainerParser(const nsACString& aType)
  : mHasInitData(false)
  , mType(aType)
{
}

bool
ContainerParser::IsInitSegmentPresent(MediaByteBuffer* aData)
{
MSE_DEBUG(ContainerParser, "aLength=%u [%x%x%x%x]",
            aData->Length(),
            aData->Length() > 0 ? (*aData)[0] : 0,
            aData->Length() > 1 ? (*aData)[1] : 0,
            aData->Length() > 2 ? (*aData)[2] : 0,
            aData->Length() > 3 ? (*aData)[3] : 0);
return false;
}

bool
ContainerParser::IsMediaSegmentPresent(MediaByteBuffer* aData)
{
  MSE_DEBUG(ContainerParser, "aLength=%u [%x%x%x%x]",
            aData->Length(),
            aData->Length() > 0 ? (*aData)[0] : 0,
            aData->Length() > 1 ? (*aData)[1] : 0,
            aData->Length() > 2 ? (*aData)[2] : 0,
            aData->Length() > 3 ? (*aData)[3] : 0);
  return false;
}

bool
ContainerParser::ParseStartAndEndTimestamps(MediaByteBuffer* aData,
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

MediaByteBuffer*
ContainerParser::InitData()
{
  return mInitData;
}

MediaByteRange
ContainerParser::InitSegmentRange()
{
  return mCompleteInitSegmentRange;
}

MediaByteRange
ContainerParser::MediaHeaderRange()
{
  return mCompleteMediaHeaderRange;
}

MediaByteRange
ContainerParser::MediaSegmentRange()
{
  return mCompleteMediaSegmentRange;
}

class WebMContainerParser : public ContainerParser {
public:
  explicit WebMContainerParser(const nsACString& aType)
    : ContainerParser(aType)
    , mParser(0)
    , mOffset(0)
  {}

  static const unsigned NS_PER_USEC = 1000;
  static const unsigned USEC_PER_SEC = 1000000;

  bool IsInitSegmentPresent(MediaByteBuffer* aData) override
  {
    ContainerParser::IsInitSegmentPresent(aData);
    
    
    
    
    
    
    
    
                  
                  
    
    
    if (aData->Length() >= 4 &&
        (*aData)[0] == 0x1a && (*aData)[1] == 0x45 && (*aData)[2] == 0xdf &&
        (*aData)[3] == 0xa3) {
      return true;
    }
    return false;
  }

  bool IsMediaSegmentPresent(MediaByteBuffer* aData) override
  {
    ContainerParser::IsMediaSegmentPresent(aData);
    
    
    
    
    
    
    
    
    
    
    if (aData->Length() >= 4 &&
        (*aData)[0] == 0x1f && (*aData)[1] == 0x43 && (*aData)[2] == 0xb6 &&
        (*aData)[3] == 0x75) {
      return true;
    }
    return false;
  }

  bool ParseStartAndEndTimestamps(MediaByteBuffer* aData,
                                  int64_t& aStart, int64_t& aEnd) override
  {
    bool initSegment = IsInitSegmentPresent(aData);
    if (initSegment) {
      mOffset = 0;
      mParser = WebMBufferedParser(0);
      mOverlappedMapping.Clear();
      mInitData = new MediaByteBuffer();
      mResource = new SourceBufferResource(NS_LITERAL_CSTRING("video/webm"));
    }

    
    
    nsTArray<WebMTimeDataOffset> mapping;
    mapping.AppendElements(mOverlappedMapping);
    mOverlappedMapping.Clear();
    ReentrantMonitor dummy("dummy");
    mParser.Append(aData->Elements(), aData->Length(), mapping, dummy);
    if (mResource) {
      mResource->AppendData(aData);
    }

    
    
    
    if (initSegment || !HasCompleteInitData()) {
      if (mParser.mInitEndOffset > 0) {
        MOZ_ASSERT(mParser.mInitEndOffset <= mResource->GetLength());
        if (!mInitData->SetLength(mParser.mInitEndOffset, fallible)) {
          
          return false;
        }
        mCompleteInitSegmentRange = MediaByteRange(0, mParser.mInitEndOffset);
        char* buffer = reinterpret_cast<char*>(mInitData->Elements());
        mResource->ReadFromCache(buffer, 0, mParser.mInitEndOffset);
        MSE_DEBUG(WebMContainerParser, "Stashed init of %u bytes.",
                  mParser.mInitEndOffset);
        mResource = nullptr;
      } else {
        MSE_DEBUG(WebMContainerParser, "Incomplete init found.");
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

    MSE_DEBUG(WebMContainerParser, "[%lld, %lld] [fso=%lld, leo=%lld, l=%u endIdx=%u]",
              aStart, aEnd, mapping[0].mSyncOffset, mapping[endIdx].mEndOffset, mapping.Length(), endIdx);

    mapping.RemoveElementsAt(0, endIdx + 1);
    mOverlappedMapping.AppendElements(mapping);

    return true;
  }

  int64_t GetRoundingError() override
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
  explicit MP4ContainerParser(const nsACString& aType)
    : ContainerParser(aType)
    , mMonitor("MP4ContainerParser Index Monitor")
  {}

  bool HasAtom(const mp4_demuxer::AtomType& aAtom, const MediaByteBuffer* aData) {
    mp4_demuxer::ByteReader reader(aData);

    while (reader.Remaining() >= 8) {
      uint64_t size = reader.ReadU32();
      const uint8_t* typec = reader.Peek(4);
      uint32_t type = reader.ReadU32();
      MSE_DEBUGV(MP4ContainerParser ,"Checking atom:'%c%c%c%c'",
                typec[0], typec[1], typec[2], typec[3]);
      if (mp4_demuxer::AtomType(type) == aAtom) {
        reader.DiscardRemaining();
        return true;
      }
      if (size == 1) {
        
        if (!reader.CanReadType<uint64_t>()) {
          break;
        }
        size = reader.ReadU64();
      } else if (size == 0) {
        
        
        break;
      }
      if (reader.Remaining() < size - 8) {
        
        break;
      }
      reader.Read(size - 8);
    }
    reader.DiscardRemaining();
    return false;
  }

  bool IsInitSegmentPresent(MediaByteBuffer* aData) override
  {
    ContainerParser::IsInitSegmentPresent(aData);
    
    
    
    return HasAtom(mp4_demuxer::AtomType("ftyp"), aData);
  }

  bool IsMediaSegmentPresent(MediaByteBuffer* aData) override
  {
    ContainerParser::IsMediaSegmentPresent(aData);
    return HasAtom(mp4_demuxer::AtomType("moof"), aData);
  }

  bool ParseStartAndEndTimestamps(MediaByteBuffer* aData,
                                  int64_t& aStart, int64_t& aEnd) override
  {
    MonitorAutoLock mon(mMonitor); 
                                   
    bool initSegment = IsInitSegmentPresent(aData);
    if (initSegment) {
      mResource = new SourceBufferResource(NS_LITERAL_CSTRING("video/mp4"));
      mStream = new MP4Stream(mResource);
      
      
      
      
      mParser = new mp4_demuxer::MoofParser(mStream, 0,  false, &mMonitor);
      mInitData = new MediaByteBuffer();
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
      MediaByteRange& range = mParser->mInitRange;
      if (range.Length()) {
        mCompleteInitSegmentRange = range;
        if (!mInitData->SetLength(range.Length(), fallible)) {
          
          return false;
        }
        char* buffer = reinterpret_cast<char*>(mInitData->Elements());
        mResource->ReadFromCache(buffer, range.mStart, range.Length());
        MSE_DEBUG(MP4ContainerParser ,"Stashed init of %u bytes.",
                  range.Length());
      } else {
        MSE_DEBUG(MP4ContainerParser, "Incomplete init found.");
      }
      mHasInitData = true;
    }

    mp4_demuxer::Interval<mp4_demuxer::Microseconds> compositionRange =
      mParser->GetCompositionRange(byteRanges);

    mCompleteMediaHeaderRange = mParser->FirstCompleteMediaHeader();
    mCompleteMediaSegmentRange = mParser->FirstCompleteMediaSegment();
    ErrorResult rv;
    if (HasCompleteInitData()) {
      mResource->EvictData(mParser->mOffset, mParser->mOffset, rv);
    }
    if (NS_WARN_IF(rv.Failed())) {
      rv.SuppressException();
      return false;
    }

    if (compositionRange.IsNull()) {
      return false;
    }
    aStart = compositionRange.start;
    aEnd = compositionRange.end;
    MSE_DEBUG(MP4ContainerParser, "[%lld, %lld]",
              aStart, aEnd);
    return true;
  }

  
  
  int64_t GetRoundingError() override
  {
    return 35000;
  }

private:
  nsRefPtr<MP4Stream> mStream;
  nsAutoPtr<mp4_demuxer::MoofParser> mParser;
  Monitor mMonitor;
};
#endif

 ContainerParser*
ContainerParser::CreateForMIMEType(const nsACString& aType)
{
  if (aType.LowerCaseEqualsLiteral("video/webm") || aType.LowerCaseEqualsLiteral("audio/webm")) {
    return new WebMContainerParser(aType);
  }

#ifdef MOZ_FMP4
  if (aType.LowerCaseEqualsLiteral("video/mp4") || aType.LowerCaseEqualsLiteral("audio/mp4")) {
    return new MP4ContainerParser(aType);
  }
#endif
  return new ContainerParser(aType);
}

#undef MSE_DEBUG
#undef MSE_DEBUGV

} 
