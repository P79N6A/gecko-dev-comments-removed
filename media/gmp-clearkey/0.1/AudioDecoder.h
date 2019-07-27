















#ifndef __AudioDecoder_h__
#define __AudioDecoder_h__

#include "gmp-audio-decode.h"
#include "gmp-audio-host.h"
#include "gmp-task-utils.h"
#include "WMFAACDecoder.h"
#include "RefCounted.h"

#include "mfobjects.h"

class AudioDecoder : public GMPAudioDecoder
                   , public RefCounted
{
public:
  AudioDecoder(GMPAudioHost *aHostAPI);

  virtual void InitDecode(const GMPAudioCodec& aCodecSettings,
                          GMPAudioDecoderCallback* aCallback) override;

  virtual void Decode(GMPAudioSamples* aEncodedSamples);

  virtual void Reset() override;

  virtual void Drain() override;

  virtual void DecodingComplete() override;

  bool HasShutdown() { return mHasShutdown; }

private:
  virtual ~AudioDecoder();

  void EnsureWorker();

  void DecodeTask(GMPAudioSamples* aEncodedSamples);
  void DrainTask();

  void ReturnOutput(IMFSample* aSample);

  HRESULT MFToGMPSample(IMFSample* aSample,
                        GMPAudioSamples* aAudioFrame);

  void MaybeRunOnMainThread(GMPTask* aTask);

  GMPAudioHost *mHostAPI; 
  GMPAudioDecoderCallback* mCallback; 
  GMPThread* mWorkerThread;
  GMPMutex* mMutex;
  wmf::AutoPtr<wmf::WMFAACDecoder> mDecoder;

  int32_t mNumInputTasks;

  bool mHasShutdown;
};

#endif 
