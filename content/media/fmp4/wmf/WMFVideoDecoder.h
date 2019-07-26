





#if !defined(WMFVideoDecoder_h_)
#define WMFVideoDecoder_h_

#include "WMF.h"
#include "MP4Reader.h"
#include "MFTDecoder.h"
#include "nsRect.h"

#include "mozilla/RefPtr.h"

namespace mozilla {

class DXVA2Manager;

class WMFVideoDecoder : public MediaDataDecoder {
public:
  WMFVideoDecoder(mozilla::layers::LayersBackend aLayersBackend,
                  mozilla::layers::ImageContainer* aImageContainer,
                  bool aDXVAEnabled);
  ~WMFVideoDecoder();

  
  virtual nsresult Init() MOZ_OVERRIDE;

  virtual nsresult Shutdown() MOZ_OVERRIDE;

  
  virtual DecoderStatus Input(nsAutoPtr<mp4_demuxer::MP4Sample>& aSample) MOZ_OVERRIDE;

  
  virtual DecoderStatus Output(nsAutoPtr<MediaData>& aOutData) MOZ_OVERRIDE;

  virtual DecoderStatus Flush() MOZ_OVERRIDE;

private:

  bool InitializeDXVA();

  HRESULT ConfigureVideoFrameGeometry();

  HRESULT CreateBasicVideoFrame(IMFSample* aSample,
                                VideoData** aOutVideoData);

  HRESULT CreateD3DVideoFrame(IMFSample* aSample,
                              VideoData** aOutVideoData);
  
  VideoInfo mVideoInfo;
  uint32_t mVideoStride;
  uint32_t mVideoWidth;
  uint32_t mVideoHeight;
  nsIntRect mPictureRegion;

  
  
  int64_t mLastStreamOffset;

  nsAutoPtr<MFTDecoder> mDecoder;
  RefPtr<layers::ImageContainer> mImageContainer;
  nsAutoPtr<DXVA2Manager> mDXVA2Manager;

  const bool mDXVAEnabled;
  const layers::LayersBackend mLayersBackend;
  bool mUseHwAccel;
};



} 

#endif
