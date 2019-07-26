





#include "AudioBuffer.h"
#include "mozilla/dom/AudioBufferBinding.h"
#include "nsContentUtils.h"
#include "AudioContext.h"
#include "jsfriendapi.h"
#include "mozilla/ErrorResult.h"
#include "AudioSegment.h"
#include "nsIScriptError.h"
#include "nsPIDOMWindow.h"
#include "AudioChannelFormat.h"
#include "mozilla/PodOperations.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(AudioBuffer)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mContext)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mJSChannels)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
  tmp->ClearJSChannels();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(AudioBuffer)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(AudioBuffer)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
  for (uint32_t i = 0; i < tmp->mJSChannels.Length(); ++i) {
    NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mJSChannels[i])
  }
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(AudioBuffer)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AudioBuffer)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AudioBuffer)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

AudioBuffer::AudioBuffer(AudioContext* aContext, uint32_t aLength,
                         float aSampleRate)
  : mContext(aContext),
    mLength(aLength),
    mSampleRate(aSampleRate)
{
  SetIsDOMBinding();

  NS_HOLD_JS_OBJECTS(this, AudioBuffer);
}

AudioBuffer::~AudioBuffer()
{
  ClearJSChannels();
}

void
AudioBuffer::ClearJSChannels()
{
  mJSChannels.Clear();
  NS_DROP_JS_OBJECTS(this, AudioBuffer);
}

bool
AudioBuffer::InitializeBuffers(uint32_t aNumberOfChannels, JSContext* aJSContext)
{
  if (!mJSChannels.SetCapacity(aNumberOfChannels)) {
    return false;
  }
  for (uint32_t i = 0; i < aNumberOfChannels; ++i) {
    JS::RootedObject array(aJSContext, JS_NewFloat32Array(aJSContext, mLength));
    if (!array) {
      return false;
    }
    mJSChannels.AppendElement(array);
  }

  return true;
}

JSObject*
AudioBuffer::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return AudioBufferBinding::Wrap(aCx, aScope, this);
}

bool
AudioBuffer::RestoreJSChannelData(JSContext* aJSContext)
{
  if (mSharedChannels) {
    for (uint32_t i = 0; i < mJSChannels.Length(); ++i) {
      const float* data = mSharedChannels->GetData(i);
      
      
      
      JS::RootedObject array(aJSContext, JS_NewFloat32Array(aJSContext, mLength));
      if (!array) {
        return false;
      }
      memcpy(JS_GetFloat32ArrayData(array), data, sizeof(float)*mLength);
      mJSChannels[i] = array;
    }

    mSharedChannels = nullptr;
  }

  return true;
}

void
AudioBuffer::SetRawChannelContents(JSContext* aJSContext, uint32_t aChannel,
                                   float* aContents)
{
  PodCopy(JS_GetFloat32ArrayData(mJSChannels[aChannel]), aContents, mLength);
}

JSObject*
AudioBuffer::GetChannelData(JSContext* aJSContext, uint32_t aChannel,
                            ErrorResult& aRv)
{
  if (aChannel >= NumberOfChannels()) {
    aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return nullptr;
  }

  if (!RestoreJSChannelData(aJSContext)) {
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return nullptr;
  }

  return mJSChannels[aChannel];
}

bool
AudioBuffer::SetChannelDataFromArrayBufferContents(JSContext* aJSContext,
                                                   uint32_t aChannel,
                                                   void* aContents)
{
  if (!RestoreJSChannelData(aJSContext)) {
    return false;
  }

  MOZ_ASSERT(aChannel < NumberOfChannels());
  JS::RootedObject arrayBuffer(aJSContext, JS_NewArrayBufferWithContents(aJSContext, aContents));
  if (!arrayBuffer) {
    return false;
  }
  mJSChannels[aChannel] = JS_NewFloat32ArrayWithBuffer(aJSContext, arrayBuffer,
                                                       0, -1);
  if (!mJSChannels[aChannel]) {
    return false;
  }
  MOZ_ASSERT(mLength == JS_GetTypedArrayLength(mJSChannels[aChannel]));

  return true;
}

static already_AddRefed<ThreadSharedFloatArrayBufferList>
StealJSArrayDataIntoThreadSharedFloatArrayBufferList(JSContext* aJSContext,
                                                     const nsTArray<JSObject*>& aJSArrays)
{
  nsRefPtr<ThreadSharedFloatArrayBufferList> result =
    new ThreadSharedFloatArrayBufferList(aJSArrays.Length());
  for (uint32_t i = 0; i < aJSArrays.Length(); ++i) {
    JS::RootedObject arrayBuffer(aJSContext, JS_GetArrayBufferViewBuffer(aJSArrays[i]));
    void* dataToFree = nullptr;
    uint8_t* stolenData = nullptr;
    if (arrayBuffer &&
        JS_StealArrayBufferContents(aJSContext, arrayBuffer, &dataToFree,
                                    &stolenData)) {
      result->SetData(i, dataToFree, reinterpret_cast<float*>(stolenData));
    } else {
      result->Clear();
      return result.forget();
    }
  }
  return result.forget();
}

ThreadSharedFloatArrayBufferList*
AudioBuffer::GetThreadSharedChannelsForRate(JSContext* aJSContext)
{
  if (!mSharedChannels) {
    
    mSharedChannels =
      StealJSArrayDataIntoThreadSharedFloatArrayBufferList(aJSContext, mJSChannels);
  }

  return mSharedChannels;
}

void
AudioBuffer::MixToMono(JSContext* aJSContext)
{
  if (mJSChannels.Length() == 1) {
    
    return;
  }

  
  nsAutoTArray<const void*, GUESS_AUDIO_CHANNELS> channels;
  channels.SetLength(mJSChannels.Length());
  for (uint32_t i = 0; i < mJSChannels.Length(); ++i) {
    channels[i] = JS_GetFloat32ArrayData(mJSChannels[i]);
  }

  
  float* downmixBuffer = new float[mLength];

  
  AudioChannelsDownMix(channels, &downmixBuffer, 1, mLength);

  
  mJSChannels.SetLength(1);
  SetRawChannelContents(aJSContext, 0, downmixBuffer);
  delete[] downmixBuffer;
}

}
}
