


















#ifndef AUDIOOUTPUT_H_
#define AUDIOOUTPUT_H_

#include <stagefright/foundation/ABase.h>
#include <utils/Mutex.h>
#include <AudioTrack.h>

#include "GonkAudioSink.h"

namespace mozilla {







class AudioOutput : public GonkAudioSink
{
  typedef android::Mutex Mutex;
  typedef android::String8 String8;
  typedef android::status_t status_t;
  typedef android::AudioTrack AudioTrack;

  class CallbackData;

public:
  AudioOutput(int aSessionId, int aUid);
  virtual ~AudioOutput();

  virtual ssize_t FrameSize() const;
  virtual status_t GetPosition(uint32_t* aPosition) const;
  virtual status_t SetVolume(float aVolume) const;
  virtual status_t SetParameters(const String8& aKeyValuePairs);

  
  
  
  virtual status_t Open(uint32_t aSampleRate,
                        int aChannelCount,
                        audio_channel_mask_t aChannelMask,
                        audio_format_t aFormat,
                        AudioCallback aCb,
                        void* aCookie,
                        audio_output_flags_t aFlags = AUDIO_OUTPUT_FLAG_NONE,
                        const audio_offload_info_t* aOffloadInfo = nullptr);

  virtual status_t Start();
  virtual void Stop();
  virtual void Flush();
  virtual void Pause();
  virtual void Close();

private:
  static void CallbackWrapper(int aEvent, void* aMe, void* aInfo);

  android::sp<AudioTrack> mTrack;
  void* mCallbackCookie;
  AudioCallback mCallback;
  CallbackData* mCallbackData;

  
  int mUid;

  
  int mSessionId;

  
  
  
  class CallbackData
  {
    public:
      CallbackData(AudioOutput* aCookie)
      {
        mData = aCookie;
      }
      AudioOutput* GetOutput() { return mData;}
      void SetOutput(AudioOutput* aNewcookie) { mData = aNewcookie; }
      
      
      void Lock() { mLock.lock(); }
      void Unlock() { mLock.unlock(); }
    private:
      AudioOutput* mData;
      mutable Mutex mLock;
      DISALLOW_EVIL_CONSTRUCTORS(CallbackData);
  };
}; 

} 

#endif 
