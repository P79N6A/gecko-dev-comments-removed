





#ifndef MediaBufferDecoder_h_
#define MediaBufferDecoder_h_

#include "nsWrapperCache.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIThreadPool.h"
#include "nsString.h"
#include "nsTArray.h"
#include "mozilla/dom/TypedArray.h"

namespace mozilla {

namespace dom {
class AudioBuffer;
class AudioContext;
class DecodeErrorCallback;
class DecodeSuccessCallback;
}

struct WebAudioDecodeJob MOZ_FINAL
{
  
  
  WebAudioDecodeJob(const nsACString& aContentType,
                    dom::AudioContext* aContext,
                    const dom::ArrayBuffer& aBuffer,
                    dom::DecodeSuccessCallback* aSuccessCallback = nullptr,
                    dom::DecodeErrorCallback* aFailureCallback = nullptr);
  ~WebAudioDecodeJob();

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebAudioDecodeJob)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebAudioDecodeJob)

  enum ErrorCode {
    NoError,
    UnknownContent,
    UnknownError,
    InvalidContent,
    NoAudio
  };

  typedef void (WebAudioDecodeJob::*ResultFn)(ErrorCode);
  typedef nsAutoArrayPtr<float> ChannelBuffer;

  void OnSuccess(ErrorCode );
  void OnFailure(ErrorCode aErrorCode);

  bool AllocateBuffer();


  JS::Heap<JSObject*> mArrayBuffer;
  nsCString mContentType;
  uint32_t mWriteIndex;
  nsRefPtr<dom::AudioContext> mContext;
  nsRefPtr<dom::DecodeSuccessCallback> mSuccessCallback;
  nsRefPtr<dom::DecodeErrorCallback> mFailureCallback; 
  nsRefPtr<dom::AudioBuffer> mOutput;
  FallibleTArray<ChannelBuffer> mChannelBuffers;
};







class MediaBufferDecoder
{
public:
  void AsyncDecodeMedia(const char* aContentType, uint8_t* aBuffer,
                        uint32_t aLength, WebAudioDecodeJob& aDecodeJob);

  bool SyncDecodeMedia(const char* aContentType, uint8_t* aBuffer,
                       uint32_t aLength, WebAudioDecodeJob& aDecodeJob);

  void Shutdown();

private:
  bool EnsureThreadPoolInitialized();

private:
  nsCOMPtr<nsIThreadPool> mThreadPool;
};

}

#endif

