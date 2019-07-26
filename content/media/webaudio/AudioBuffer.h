





#ifndef AudioBuffer_h_
#define AudioBuffer_h_

#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "EnableWebAudioCheck.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "AudioContext.h"
#include "AudioSegment.h"
#include "AudioNodeEngine.h"

struct JSContext;
class JSObject;

namespace mozilla {

class ErrorResult;

namespace dom {






class AudioBuffer MOZ_FINAL : public nsISupports,
                              public nsWrapperCache,
                              public EnableWebAudioCheck
{
public:
  AudioBuffer(AudioContext* aContext, uint32_t aLength,
              float aSampleRate);
  ~AudioBuffer();

  
  
  bool InitializeBuffers(uint32_t aNumberOfChannels,
                         JSContext* aJSContext);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(AudioBuffer)

  AudioContext* GetParentObject() const
  {
    return mContext;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  float SampleRate() const
  {
    return mSampleRate;
  }

  int32_t Length() const
  {
    return mLength;
  }

  double Duration() const
  {
    return mLength / static_cast<double> (mSampleRate);
  }

  uint32_t NumberOfChannels() const
  {
    return mJSChannels.Length();
  }

  



  JSObject* GetChannelData(JSContext* aJSContext, uint32_t aChannel,
                           ErrorResult& aRv);

  JSObject* GetChannelData(uint32_t aChannel) const {
    
    MOZ_ASSERT(aChannel < mJSChannels.Length());
    return mJSChannels[aChannel];
  }

  



  ThreadSharedFloatArrayBufferList* GetThreadSharedChannelsForRate(JSContext* aContext,
                                                                   uint32_t* aRate,
                                                                   uint32_t* aLength);

  
  
  void SetChannelDataFromArrayBufferContents(JSContext* aJSContext,
                                             uint32_t aChannel,
                                             void* aContents);

  
  
  
  
  void SetRawChannelContents(JSContext* aJSContext,
                             uint32_t aChannel,
                             float* aContents);

protected:
  void RestoreJSChannelData(JSContext* aJSContext);
  void ClearJSChannels();

  nsRefPtr<AudioContext> mContext;
  
  AutoFallibleTArray<JSObject*,2> mJSChannels;

  
  
  nsRefPtr<ThreadSharedFloatArrayBufferList> mSharedChannels;

  uint32_t mLength;
  float mSampleRate;
};

}
}

#endif

