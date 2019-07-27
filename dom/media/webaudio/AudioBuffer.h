





#ifndef AudioBuffer_h_
#define AudioBuffer_h_

#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "AudioContext.h"
#include "js/TypeDecls.h"
#include "mozilla/MemoryReporting.h"

namespace mozilla {

class ErrorResult;
class ThreadSharedFloatArrayBufferList;

namespace dom {

class AudioContext;






class AudioBuffer final : public nsWrapperCache
{
public:
  static already_AddRefed<AudioBuffer>
  Create(AudioContext* aContext, uint32_t aNumberOfChannels,
         uint32_t aLength, float aSampleRate,
         JSContext* aJSContext, ErrorResult& aRv);

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(AudioBuffer)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(AudioBuffer)

  AudioContext* GetParentObject() const
  {
    return mContext;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

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

  



  void GetChannelData(JSContext* aJSContext, uint32_t aChannel,
                      JS::MutableHandle<JSObject*> aRetval,
                      ErrorResult& aRv);

  void CopyFromChannel(const Float32Array& aDestination, uint32_t aChannelNumber,
                       uint32_t aStartInChannel, ErrorResult& aRv);
  void CopyToChannel(JSContext* aJSContext, const Float32Array& aSource,
                     uint32_t aChannelNumber, uint32_t aStartInChannel,
                     ErrorResult& aRv);

  



  ThreadSharedFloatArrayBufferList* GetThreadSharedChannelsForRate(JSContext* aContext);

  
  
  
  
  void SetRawChannelContents(uint32_t aChannel, float* aContents);

protected:
  AudioBuffer(AudioContext* aContext, uint32_t aNumberOfChannels,
              uint32_t aLength, float aSampleRate);
  ~AudioBuffer();

  bool RestoreJSChannelData(JSContext* aJSContext);
  void ClearJSChannels();

  nsRefPtr<AudioContext> mContext;
  
  nsAutoTArray<JS::Heap<JSObject*>, 2> mJSChannels;

  
  
  nsRefPtr<ThreadSharedFloatArrayBufferList> mSharedChannels;

  uint32_t mLength;
  float mSampleRate;
};

} 
} 

#endif

