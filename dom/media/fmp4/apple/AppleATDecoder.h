





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
  AppleATDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                 FlushableMediaTaskQueue* aVideoTaskQueue,
                 MediaDataDecoderCallback* aCallback);
  virtual ~AppleATDecoder();

  virtual nsresult Init() MOZ_OVERRIDE;
  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE;
  virtual nsresult Flush() MOZ_OVERRIDE;
  virtual nsresult Drain() MOZ_OVERRIDE;
  virtual nsresult Shutdown() MOZ_OVERRIDE;

  
  const mp4_demuxer::AudioDecoderConfig& mConfig;

  
  nsTArray<uint8_t> mMagicCookie;
  
  
  bool mFileStreamError;

private:
  nsRefPtr<FlushableMediaTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;
  AudioConverterRef mConverter;
  AudioStreamBasicDescription mOutputFormat;
  UInt32 mFormatID;
  AudioFileStreamID mStream;
  nsTArray<nsAutoPtr<mp4_demuxer::MP4Sample>> mQueuedSamples;

  void SubmitSample(nsAutoPtr<mp4_demuxer::MP4Sample> aSample);
  nsresult DecodeSample(mp4_demuxer::MP4Sample* aSample);
  nsresult GetInputAudioDescription(AudioStreamBasicDescription& aDesc,
                                    const nsTArray<uint8_t>& aExtraData);
  
  
  nsresult SetupDecoder(mp4_demuxer::MP4Sample* aSample);
  nsresult GetImplicitAACMagicCookie(const mp4_demuxer::MP4Sample* aSample);
};

} 

#endif 
