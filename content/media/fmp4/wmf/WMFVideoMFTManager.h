





#if !defined(WMFVideoMFTManager_h_)
#define WMFVideoMFTManager_h_

#include "WMF.h"
#include "MP4Reader.h"
#include "MFTDecoder.h"
#include "nsRect.h"
#include "WMFMediaDataDecoder.h"
#include "mozilla/RefPtr.h"

namespace mozilla {

class DXVA2Manager;

class WMFVideoMFTManager : public MFTManager {
public:
  WMFVideoMFTManager(const mp4_demuxer::VideoDecoderConfig& aConfig,
                     mozilla::layers::LayersBackend aLayersBackend,
                     mozilla::layers::ImageContainer* aImageContainer,
                     bool aDXVAEnabled);
  ~WMFVideoMFTManager();

  virtual TemporaryRef<MFTDecoder> Init() MOZ_OVERRIDE;

  virtual HRESULT Input(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE;

  virtual HRESULT Output(int64_t aStreamOffset,
                         nsAutoPtr<MediaData>& aOutput) MOZ_OVERRIDE;

  virtual void Shutdown() MOZ_OVERRIDE;

private:

  bool InitializeDXVA();

  HRESULT ConfigureVideoFrameGeometry();

  HRESULT CreateBasicVideoFrame(IMFSample* aSample,
                                int64_t aStreamOffset,
                                VideoData** aOutVideoData);

  HRESULT CreateD3DVideoFrame(IMFSample* aSample,
                              int64_t aStreamOffset,
                              VideoData** aOutVideoData);

  
  VideoInfo mVideoInfo;
  uint32_t mVideoStride;
  uint32_t mVideoWidth;
  uint32_t mVideoHeight;
  nsIntRect mPictureRegion;

  const mp4_demuxer::VideoDecoderConfig& mConfig;

  RefPtr<MFTDecoder> mDecoder;
  RefPtr<layers::ImageContainer> mImageContainer;
  nsAutoPtr<DXVA2Manager> mDXVA2Manager;
  RefPtr<MediaTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;

  const bool mDXVAEnabled;
  const layers::LayersBackend mLayersBackend;
  bool mUseHwAccel;
};

} 

#endif 
