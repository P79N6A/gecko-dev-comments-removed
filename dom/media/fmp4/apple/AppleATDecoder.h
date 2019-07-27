





#ifndef mozilla_AppleATDecoder_h
#define mozilla_AppleATDecoder_h

#include <AudioToolbox/AudioToolbox.h>
#include "PlatformDecoderModule.h"
#include "mozilla/RefPtr.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsIThread.h"

namespace mozilla {

class MediaTaskQueue;
class MediaDataDecoderCallback;

class AppleATDecoder : public MediaDataDecoder {
public:
  AppleATDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                 MediaTaskQueue* aVideoTaskQueue,
                 MediaDataDecoderCallback* aCallback);
  ~AppleATDecoder();

  virtual nsresult Init() MOZ_OVERRIDE;
  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE;
  virtual nsresult Flush() MOZ_OVERRIDE;
  virtual nsresult Drain() MOZ_OVERRIDE;
  virtual nsresult Shutdown() MOZ_OVERRIDE;

  
  const mp4_demuxer::AudioDecoderConfig& mConfig;

  
  Vector<uint8_t> mMagicCookie;
  
  
  bool mFileStreamError;

private:
  RefPtr<MediaTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;
  AudioConverterRef mConverter;
  AudioStreamBasicDescription mOutputFormat;
  UInt32 mFormatID;
  AudioFileStreamID mStream;
  nsTArray<nsAutoPtr<mp4_demuxer::MP4Sample>> mQueuedSamples;

  void SubmitSample(nsAutoPtr<mp4_demuxer::MP4Sample> aSample);
  nsresult DecodeSample(mp4_demuxer::MP4Sample* aSample);
  nsresult GetInputAudioDescription(AudioStreamBasicDescription& aDesc,
                                    const Vector<uint8_t>& aExtraData);
  
  
  nsresult SetupDecoder(mp4_demuxer::MP4Sample* aSample);
  nsresult GetImplicitAACMagicCookie(const mp4_demuxer::MP4Sample* aSample);
};

} 

#endif 
