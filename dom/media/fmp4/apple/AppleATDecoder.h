





#ifndef mozilla_AppleATDecoder_h
#define mozilla_AppleATDecoder_h

#include <AudioToolbox/AudioToolbox.h>
#include "PlatformDecoderModule.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/Vector.h"
#include "nsIThread.h"

namespace mozilla {

class FlushableMediaTaskQueue;
class MediaDataDecoderCallback;

class AppleATDecoder : public MediaDataDecoder {
public:
  AppleATDecoder(const AudioInfo& aConfig,
                 FlushableMediaTaskQueue* aVideoTaskQueue,
                 MediaDataDecoderCallback* aCallback);
  virtual ~AppleATDecoder();

  virtual nsresult Init() override;
  virtual nsresult Input(MediaRawData* aSample) override;
  virtual nsresult Flush() override;
  virtual nsresult Drain() override;
  virtual nsresult Shutdown() override;

  
  const AudioInfo& mConfig;

  
  nsTArray<uint8_t> mMagicCookie;
  
  
  bool mFileStreamError;

private:
  nsRefPtr<FlushableMediaTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;
  AudioConverterRef mConverter;
  AudioStreamBasicDescription mOutputFormat;
  UInt32 mFormatID;
  AudioFileStreamID mStream;
  nsTArray<nsRefPtr<MediaRawData>> mQueuedSamples;

  void SubmitSample(MediaRawData* aSample);
  nsresult DecodeSample(MediaRawData* aSample);
  nsresult GetInputAudioDescription(AudioStreamBasicDescription& aDesc,
                                    const nsTArray<uint8_t>& aExtraData);
  
  
  nsresult SetupDecoder(MediaRawData* aSample);
  nsresult GetImplicitAACMagicCookie(const MediaRawData* aSample);
};

} 

#endif 
