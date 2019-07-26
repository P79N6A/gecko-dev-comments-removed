





#if !defined(DirectShowReader_h_)
#define DirectShowReader_h_

#include "windows.h" 
#include "MediaDecoderReader.h"
#include "mozilla/RefPtr.h"
#include "MP3FrameParser.h"

class IGraphBuilder;
class IMediaControl;
class IMediaSeeking;
class IMediaEventEx;

namespace mozilla {

class AudioSinkFilter;
class SourceFilter;

namespace dom {
class TimeRanges;
}
















class DirectShowReader : public MediaDecoderReader
{
public:
  DirectShowReader(AbstractMediaDecoder* aDecoder);

  virtual ~DirectShowReader();

  nsresult Init(MediaDecoderReader* aCloneDonor) MOZ_OVERRIDE;

  bool DecodeAudioData() MOZ_OVERRIDE;
  bool DecodeVideoFrame(bool &aKeyframeSkip,
                        int64_t aTimeThreshold) MOZ_OVERRIDE;

  bool HasAudio() MOZ_OVERRIDE;
  bool HasVideo() MOZ_OVERRIDE;

  nsresult ReadMetadata(MediaInfo* aInfo,
                        MetadataTags** aTags) MOZ_OVERRIDE;

  nsresult Seek(int64_t aTime,
                int64_t aStartTime,
                int64_t aEndTime,
                int64_t aCurrentTime) MOZ_OVERRIDE;

  nsresult GetBuffered(mozilla::dom::TimeRanges* aBuffered,
                       int64_t aStartTime) MOZ_OVERRIDE;

  void OnDecodeThreadStart() MOZ_OVERRIDE;
  void OnDecodeThreadFinish() MOZ_OVERRIDE;

  void NotifyDataArrived(const char* aBuffer,
                         uint32_t aLength,
                         int64_t aOffset) MOZ_OVERRIDE;

private:

  
  
  
  bool Finish(HRESULT aStatus);

  
  
  RefPtr<IGraphBuilder> mGraph;
  RefPtr<IMediaControl> mControl;
  RefPtr<IMediaSeeking> mMediaSeeking;

  
  RefPtr<SourceFilter> mSourceFilter;

  
  
  RefPtr<AudioSinkFilter> mAudioSinkFilter;

  
  
  
  MP3FrameParser mMP3FrameParser;

#ifdef DEBUG
  
  
  
  DWORD mRotRegister;
#endif

  
  uint32_t mNumChannels;

  
  uint32_t mAudioRate;

  
  uint32_t mBytesPerSample;

  
  int64_t mDuration;
};

} 

#endif
