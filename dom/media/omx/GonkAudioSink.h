


















#ifndef GONK_AUDIO_SINK_H_
#define GONK_AUDIO_SINK_H_

#include <utils/Errors.h>
#include <utils/String8.h>
#include <system/audio.h>

#define DEFAULT_AUDIOSINK_BUFFERCOUNT 4
#define DEFAULT_AUDIOSINK_BUFFERSIZE 1200
#define DEFAULT_AUDIOSINK_SAMPLERATE 44100



#define CHANNEL_MASK_USE_CHANNEL_ORDER 0

namespace mozilla {






class GonkAudioSink : public android::RefBase
{
  typedef android::String8 String8;
  typedef android::status_t status_t;

public:
  enum cb_event_t {
    CB_EVENT_FILL_BUFFER,   
    CB_EVENT_STREAM_END,    
                            
    CB_EVENT_TEAR_DOWN      
                            
  };

  
  typedef size_t (*AudioCallback)(GonkAudioSink* aAudioSink,
                                  void* aBuffer,
                                  size_t aSize,
                                  void* aCookie,
                                  cb_event_t aEvent);
  virtual ~GonkAudioSink() {}
  virtual ssize_t FrameSize() const = 0;
  virtual status_t GetPosition(uint32_t* aPosition) const = 0;
  virtual status_t SetVolume(float aVolume) const = 0;
  virtual status_t SetParameters(const String8& aKeyValuePairs)
  {
    return android::NO_ERROR;
  }

  virtual status_t Open(uint32_t aSampleRate,
                        int aChannelCount,
                        audio_channel_mask_t aChannelMask,
                        audio_format_t aFormat=AUDIO_FORMAT_PCM_16_BIT,
                        AudioCallback aCb = nullptr,
                        void* aCookie = nullptr,
                        audio_output_flags_t aFlags = AUDIO_OUTPUT_FLAG_NONE,
                        const audio_offload_info_t* aOffloadInfo = nullptr) = 0;

  virtual status_t Start() = 0;
  virtual void Stop() = 0;
  virtual void Flush() = 0;
  virtual void Pause() = 0;
  virtual void Close() = 0;
};

} 

#endif
