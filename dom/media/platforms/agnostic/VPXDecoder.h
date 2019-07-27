




#if !defined(VPXDecoder_h_)
#define VPXDecoder_h_

#include "PlatformDecoderModule.h"

#include <stdint.h>
#define VPX_DONT_DEFINE_STDINT_TYPES
#include "vpx/vp8dx.h"
#include "vpx/vpx_codec.h"
#include "vpx/vpx_decoder.h"

namespace mozilla {

  using namespace layers;

class VPXDecoder : public MediaDataDecoder
{
public:
  VPXDecoder(const VideoInfo& aConfig,
             ImageContainer* aImageContainer,
             FlushableTaskQueue* aTaskQueue,
             MediaDataDecoderCallback* aCallback);

  ~VPXDecoder();

  nsresult Init() override;
  nsresult Input(MediaRawData* aSample) override;
  nsresult Flush() override;
  nsresult Drain() override;
  nsresult Shutdown() override;

  
  static bool IsVPX(const nsACString& aMimeType);

  enum Codec {
    VP8,
    VP9
  };

private:
  void DecodeFrame (MediaRawData* aSample);
  int DoDecodeFrame (MediaRawData* aSample);
  void DoDrain ();
  void OutputDelayedFrames ();

  nsRefPtr<ImageContainer> mImageContainer;
  RefPtr<FlushableTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;

  
  vpx_codec_ctx_t mVPX;
  vpx_codec_iter_t mIter;

  uint32_t mDisplayWidth;
  uint32_t mDisplayHeight;

  int mCodec;
};

} 

#endif
