





#if !defined(DirectShowReader_h_)
#define DirectShowReader_h_

#include "windows.h" 
#include "MediaDecoderReader.h"
#include "mozilla/RefPtr.h"
#include "MP3FrameParser.h"

struct IGraphBuilder;
struct IMediaControl;
struct IMediaSeeking;

namespace mozilla {

class AudioSinkFilter;
class SourceFilter;
















class DirectShowReader : public MediaDecoderReader
{
public:
  DirectShowReader(AbstractMediaDecoder* aDecoder);

  virtual ~DirectShowReader();

  nsresult Init(MediaDecoderReader* aCloneDonor) override;

  bool DecodeAudioData() override;
  bool DecodeVideoFrame(bool &aKeyframeSkip,
                        int64_t aTimeThreshold) override;

  bool HasAudio() override;
  bool HasVideo() override;

  nsresult ReadMetadata(MediaInfo* aInfo,
                        MetadataTags** aTags) override;

  nsRefPtr<SeekPromise>
  Seek(int64_t aTime, int64_t aEndTime) override;

  void NotifyDataArrived(const char* aBuffer,
                         uint32_t aLength,
                         int64_t aOffset) override;

  bool IsMediaSeekable() override;

private:

  
  
  
  bool Finish(HRESULT aStatus);

  nsresult SeekInternal(int64_t aTime);

  
  
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
