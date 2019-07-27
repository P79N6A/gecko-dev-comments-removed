




#ifndef VP8TrackEncoder_h_
#define VP8TrackEncoder_h_

#include "TrackEncoder.h"
#include "vpx/vpx_codec.h"

namespace mozilla {

typedef struct vpx_codec_ctx vpx_codec_ctx_t;
typedef struct vpx_codec_enc_cfg vpx_codec_enc_cfg_t;
typedef struct vpx_image vpx_image_t;







class VP8TrackEncoder : public VideoTrackEncoder
{
  enum EncodeOperation {
    ENCODE_NORMAL_FRAME, 
    ENCODE_I_FRAME, 
    SKIP_FRAME, 
  };
public:
  VP8TrackEncoder();
  virtual ~VP8TrackEncoder();

  already_AddRefed<TrackMetadataBase> GetMetadata() MOZ_FINAL MOZ_OVERRIDE;

  nsresult GetEncodedTrack(EncodedFrameContainer& aData) MOZ_FINAL MOZ_OVERRIDE;

protected:
  nsresult Init(int32_t aWidth, int32_t aHeight,
                int32_t aDisplayWidth, int32_t aDisplayHeight,
                TrackRate aTrackRate) MOZ_FINAL MOZ_OVERRIDE;

private:
  
  TrackTicks CalculateEncodedDuration(TrackTicks aDurationCopied);

  
  TrackTicks CalculateRemainingTicks(TrackTicks aDurationCopied,
                                     TrackTicks aEncodedDuration);

  
  EncodeOperation GetNextEncodeOperation(TimeDuration aTimeElapsed,
                                         TrackTicks aProcessedDuration);

  
  nsresult GetEncodedPartitions(EncodedFrameContainer& aData);

  
  nsresult PrepareRawFrame(VideoChunk &aChunk);

  
  uint32_t mEncodedFrameRate;
  
  TrackTicks mEncodedFrameDuration;
  
  TrackTicks mEncodedTimestamp;
  
  TrackTicks mRemainingTicks;

  
  nsRefPtr<layers::Image> mMuteFrame;

  
  nsTArray<uint8_t> mI420Frame;

  





  VideoSegment mSourceSegment;

  
  
  nsAutoPtr<vpx_codec_ctx_t> mVPXContext;
  
  nsAutoPtr<vpx_image_t> mVPXImageWrapper;
};

} 

#endif
