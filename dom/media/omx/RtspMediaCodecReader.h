





#if !defined(RtspMediaCodecReader_h_)
#define RtspMediaCodecReader_h_

#include "MediaCodecReader.h"

namespace mozilla {

namespace dom {
  class TimeRanges;
}

class AbstractMediaDecoder;
class RtspMediaResource;





class RtspMediaCodecReader MOZ_FINAL : public MediaCodecReader
{
protected:
  
  virtual bool CreateExtractor() MOZ_OVERRIDE;
  
  void EnsureActive();

public:
  RtspMediaCodecReader(AbstractMediaDecoder* aDecoder);

  virtual ~RtspMediaCodecReader();

  
  virtual nsresult Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime,
                        int64_t aCurrentTime) MOZ_OVERRIDE;

  
  
  
  
  
  
  
  
  
  virtual nsresult GetBuffered(dom::TimeRanges* aBuffered,
                               int64_t aStartTime) MOZ_OVERRIDE {
    return NS_OK;
  }

  virtual void SetIdle() MOZ_OVERRIDE;

  
  virtual void RequestVideoData(bool aSkipToNextKeyframe,
                                int64_t aTimeThreshold) MOZ_OVERRIDE;

  
  virtual void RequestAudioData() MOZ_OVERRIDE;

  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags) MOZ_OVERRIDE;

  virtual void codecReserved(Track& aTrack) MOZ_OVERRIDE;

private:
  
  
  
  
  RtspMediaResource* mRtspResource;
};

} 

#endif
