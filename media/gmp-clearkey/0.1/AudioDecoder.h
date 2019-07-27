















#include "stdafx.h"

#ifdef TEST_DECODING

class AudioDecoder : public GMPAudioDecoder
{
public:
  AudioDecoder(GMPAudioHost *aHostAPI);

  virtual ~AudioDecoder();

  virtual void InitDecode(const GMPAudioCodec& aCodecSettings,
                          GMPAudioDecoderCallback* aCallback) override;

  virtual void Decode(GMPAudioSamples* aEncodedSamples);

  virtual void Reset() override;

  virtual void Drain() override;

  virtual void DecodingComplete() override;

private:

  void DecodeTask(GMPAudioSamples* aEncodedSamples);
  void DrainTask();

  void ReturnOutput(IMFSample* aSample);

  HRESULT MFToGMPSample(IMFSample* aSample,
                        GMPAudioSamples* aAudioFrame);

  GMPAudioHost *mHostAPI; 
  GMPAudioDecoderCallback* mCallback; 
  GMPThread* mWorkerThread;
  GMPMutex* mMutex;
  AutoPtr<WMFAACDecoder> mDecoder;

  int32_t mNumInputTasks;
};

#endif 
