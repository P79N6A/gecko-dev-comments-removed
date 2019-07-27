















#ifndef __VideoDecoder_h__
#define __VideoDecoder_h__

#include "gmp-video-decode.h"
#include "gmp-video-host.h"
#include "WMFH264Decoder.h"

#include "mfobjects.h"

class VideoDecoder : public GMPVideoDecoder
{
public:
  VideoDecoder(GMPVideoHost *aHostAPI);

  virtual ~VideoDecoder();

  virtual void InitDecode(const GMPVideoCodec& aCodecSettings,
                          const uint8_t* aCodecSpecific,
                          uint32_t aCodecSpecificLength,
                          GMPVideoDecoderCallback* aCallback,
                          int32_t aCoreCount) override;

  virtual void Decode(GMPVideoEncodedFrame* aInputFrame,
                      bool aMissingFrames,
                      const uint8_t* aCodecSpecific,
                      uint32_t aCodecSpecificLength,
                      int64_t aRenderTimeMs = -1);

  virtual void Reset() override;

  virtual void Drain() override;

  virtual void DecodingComplete() override;

private:

  void EnsureWorker();

  void DrainTask();

  void DecodeTask(GMPVideoEncodedFrame* aInputFrame);

  void ReturnOutput(IMFSample* aSample,
                    int32_t aWidth,
                    int32_t aHeight,
                    int32_t aStride);

  HRESULT SampleToVideoFrame(IMFSample* aSample,
                             int32_t aWidth,
                             int32_t aHeight,
                             int32_t aStride,
                             GMPVideoi420Frame* aVideoFrame);

  GMPVideoHost *mHostAPI; 
  GMPVideoDecoderCallback* mCallback; 
  GMPThread* mWorkerThread;
  GMPMutex* mMutex;
  wmf::AutoPtr<wmf::WMFH264Decoder> mDecoder;

  std::vector<uint8_t> mExtraData;
  std::vector<uint8_t> mAnnexB;

  int32_t mNumInputTasks;
  bool mSentExtraData;
};

#endif 
