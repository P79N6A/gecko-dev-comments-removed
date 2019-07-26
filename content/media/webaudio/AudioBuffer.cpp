





#include "AudioBuffer.h"
#include "mozilla/dom/AudioBufferBinding.h"
#include "jsfriendapi.h"
#include "mozilla/ErrorResult.h"
#include "AudioSegment.h"
#include "AudioChannelFormat.h"
#include "mozilla/PodOperations.h"
#include "mozilla/CheckedInt.h"
#include "AudioNodeEngine.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(AudioBuffer)

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

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(AudioBuffer, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(AudioBuffer, Release)

AudioBuffer::AudioBuffer(AudioContext* aContext, uint32_t aNumberOfChannels,
                         uint32_t aLength, float aSampleRate)
  : mContext(aContext),
    mLength(aLength),
    mSampleRate(aSampleRate)
{
  mJSChannels.SetCapacity(aNumberOfChannels);
  SetIsDOMBinding();
  mozilla::HoldJSObjects(this);
}

AudioBuffer::~AudioBuffer()
{
  ClearJSChannels();
}

void
AudioBuffer::ClearJSChannels()
{
  mJSChannels.Clear();
  mozilla::DropJSObjects(this);
}

 already_AddRefed<AudioBuffer>
AudioBuffer::Create(AudioContext* aContext, uint32_t aNumberOfChannels,
                    uint32_t aLength, float aSampleRate,
                    JSContext* aJSContext, ErrorResult& aRv)
{
  
  
  
  if (aSampleRate < WebAudioUtils::MinSampleRate ||
      aSampleRate > WebAudioUtils::MaxSampleRate ||
      aNumberOfChannels > WebAudioUtils::MaxChannelCount ||
      !aLength || aLength > INT32_MAX) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsRefPtr<AudioBuffer> buffer =
    new AudioBuffer(aContext, aNumberOfChannels, aLength, aSampleRate);

  for (uint32_t i = 0; i < aNumberOfChannels; ++i) {
    JS::Rooted<JSObject*> array(aJSContext,
                                JS_NewFloat32Array(aJSContext, aLength));
    if (!array) {
      aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return nullptr;
    }
    buffer->mJSChannels.AppendElement(array.get());
  }

  return buffer.forget();
}

JSObject*
AudioBuffer::WrapObject(JSContext* aCx)
{
  return AudioBufferBinding::Wrap(aCx, this);
}

bool
AudioBuffer::RestoreJSChannelData(JSContext* aJSContext)
{
  if (mSharedChannels) {
    for (uint32_t i = 0; i < mJSChannels.Length(); ++i) {
      const float* data = mSharedChannels->GetData(i);
      
      
      
      JS::Rooted<JSObject*> array(aJSContext,
                                  JS_NewFloat32Array(aJSContext, mLength));
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
AudioBuffer::CopyFromChannel(const Float32Array& aDestination, uint32_t aChannelNumber,
                             uint32_t aStartInChannel, ErrorResult& aRv)
{
  uint32_t length = aDestination.Length();
  CheckedInt<uint32_t> end = aStartInChannel;
  end += length;
  if (aChannelNumber >= NumberOfChannels() ||
      !end.isValid() || end.value() > mLength) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  if (!mSharedChannels && JS_GetTypedArrayLength(mJSChannels[aChannelNumber]) != mLength) {
    
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  const float* sourceData = mSharedChannels ?
    mSharedChannels->GetData(aChannelNumber) :
    JS_GetFloat32ArrayData(mJSChannels[aChannelNumber]);
  PodMove(aDestination.Data(), sourceData + aStartInChannel, length);
}

void
AudioBuffer::CopyToChannel(JSContext* aJSContext, const Float32Array& aSource,
                           uint32_t aChannelNumber, uint32_t aStartInChannel,
                           ErrorResult& aRv)
{
  uint32_t length = aSource.Length();
  CheckedInt<uint32_t> end = aStartInChannel;
  end += length;
  if (aChannelNumber >= NumberOfChannels() ||
      !end.isValid() || end.value() > mLength) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  if (!mSharedChannels && JS_GetTypedArrayLength(mJSChannels[aChannelNumber]) != mLength) {
    
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  if (!RestoreJSChannelData(aJSContext)) {
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return;
  }

  PodMove(JS_GetFloat32ArrayData(mJSChannels[aChannelNumber]) + aStartInChannel,
          aSource.Data(), length);
}

void
AudioBuffer::SetRawChannelContents(uint32_t aChannel, float* aContents)
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

static already_AddRefed<ThreadSharedFloatArrayBufferList>
StealJSArrayDataIntoThreadSharedFloatArrayBufferList(JSContext* aJSContext,
                                                     const nsTArray<JSObject*>& aJSArrays)
{
  nsRefPtr<ThreadSharedFloatArrayBufferList> result =
    new ThreadSharedFloatArrayBufferList(aJSArrays.Length());
  for (uint32_t i = 0; i < aJSArrays.Length(); ++i) {
    JS::Rooted<JSObject*> arrayBufferView(aJSContext, aJSArrays[i]);
    JS::Rooted<JSObject*> arrayBuffer(aJSContext,
                                      JS_GetArrayBufferViewBuffer(aJSContext, arrayBufferView));
    uint8_t* stolenData = arrayBuffer
                          ? (uint8_t*) JS_StealArrayBufferContents(aJSContext, arrayBuffer)
                          : nullptr;
    if (stolenData) {
      result->SetData(i, stolenData, reinterpret_cast<float*>(stolenData));
    } else {
      return nullptr;
    }
  }
  return result.forget();
}

ThreadSharedFloatArrayBufferList*
AudioBuffer::GetThreadSharedChannelsForRate(JSContext* aJSContext)
{
  if (!mSharedChannels) {
    for (uint32_t i = 0; i < mJSChannels.Length(); ++i) {
      if (mLength != JS_GetTypedArrayLength(mJSChannels[i])) {
        
        return nullptr;
      }
    }

    mSharedChannels =
      StealJSArrayDataIntoThreadSharedFloatArrayBufferList(aJSContext, mJSChannels);
  }

  return mSharedChannels;
}

size_t
AudioBuffer::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t amount = aMallocSizeOf(this);
  amount += mJSChannels.SizeOfExcludingThis(aMallocSizeOf);
  if (mSharedChannels) {
    amount += mSharedChannels->SizeOfIncludingThis(aMallocSizeOf);
  }
  return amount;
}

}
}
