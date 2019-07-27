





#if !defined(RtspMediaCodecReader_h_)
#define RtspMediaCodecReader_h_

#include "MediaCodecReader.h"

namespace mozilla {

class AbstractMediaDecoder;
class RtspMediaResource;





class RtspMediaCodecReader final : public MediaCodecReader
{
protected:
  
  virtual bool CreateExtractor() override;
  
  void EnsureActive();

public:
  RtspMediaCodecReader(AbstractMediaDecoder* aDecoder);

  virtual ~RtspMediaCodecReader();

  
  virtual nsRefPtr<SeekPromise>
  Seek(int64_t aTime, int64_t aEndTime) override;

  
  
  
  
  
  
  
  
  
  virtual media::TimeIntervals GetBuffered() override {
    return media::TimeIntervals::Invalid();
  }

  virtual void SetIdle() override;

  
  virtual nsRefPtr<VideoDataPromise>
  RequestVideoData(bool aSkipToNextKeyframe,
                   int64_t aTimeThreshold,
                   bool aForceDecodeAhead) override;

  
  virtual nsRefPtr<AudioDataPromise> RequestAudioData() override;

  virtual nsRefPtr<MediaDecoderReader::MetadataPromise> AsyncReadMetadata()
    override;

  virtual void HandleResourceAllocated() override;

private:
  
  
  
  
  RtspMediaResource* mRtspResource;
};

} 

#endif
