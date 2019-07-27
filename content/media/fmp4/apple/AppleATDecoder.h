





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


  
  void MetadataCallback(AudioFileStreamID aFileStream,
                        AudioFileStreamPropertyID aPropertyID,
                        uint32_t* aFlags);
  void SampleCallback(uint32_t aNumBytes,
                      uint32_t aNumPackets,
                      const void* aData,
                      AudioStreamPacketDescription* aPackets);

  
  const mp4_demuxer::AudioDecoderConfig& mConfig;

private:
  RefPtr<MediaTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;
  AudioConverterRef mConverter;
  AudioFileStreamID mStream;
  uint64_t mCurrentAudioFrame;
  int64_t mSamplePosition;
  bool mHaveOutput;

  void SetupDecoder();
  void SubmitSample(nsAutoPtr<mp4_demuxer::MP4Sample> aSample);
};

} 

#endif 
