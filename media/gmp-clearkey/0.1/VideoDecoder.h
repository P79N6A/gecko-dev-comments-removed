















#include "stdafx.h"

#ifdef TEST_DECODING

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

  void DrainTask();

  void DecodeTask(GMPVideoEncodedFrame* aInputFrame);

  void ReturnOutput(IMFSample* aSample);

  HRESULT SampleToVideoFrame(IMFSample* aSample,
                             GMPVideoi420Frame* aVideoFrame);

  GMPVideoHost *mHostAPI; 
  GMPVideoDecoderCallback* mCallback; 
  GMPThread* mWorkerThread;
  GMPMutex* mMutex;
  AutoPtr<WMFH264Decoder> mDecoder;

  std::vector<uint8_t> mExtraData;
  AVCDecoderConfigurationRecord mAVCC;
  std::vector<uint8_t> mAnnexB;

  int32_t mNumInputTasks;
  bool mSentExtraData;
};

#endif
