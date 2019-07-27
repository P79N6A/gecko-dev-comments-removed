





#ifndef MediaBufferDecoder_h_
#define MediaBufferDecoder_h_

#include "nsWrapperCache.h"
#include "nsCOMPtr.h"
#include "nsIThreadPool.h"
#include "nsString.h"
#include "nsTArray.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/MemoryReporting.h"

namespace mozilla {

namespace dom {
class AudioBuffer;
class AudioContext;
class DecodeErrorCallback;
class DecodeSuccessCallback;
class Promise;
}

struct WebAudioDecodeJob MOZ_FINAL
{
  
  
  WebAudioDecodeJob(const nsACString& aContentType,
                    dom::AudioContext* aContext,
                    dom::Promise* aPromise,
                    dom::DecodeSuccessCallback* aSuccessCallback = nullptr,
                    dom::DecodeErrorCallback* aFailureCallback = nullptr);

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

  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  nsCString mContentType;
  uint32_t mWriteIndex;
  nsRefPtr<dom::AudioContext> mContext;
  nsRefPtr<dom::Promise> mPromise;
  nsRefPtr<dom::DecodeSuccessCallback> mSuccessCallback;
  nsRefPtr<dom::DecodeErrorCallback> mFailureCallback; 
  nsRefPtr<dom::AudioBuffer> mOutput;
  FallibleTArray<ChannelBuffer> mChannelBuffers;

private:
  ~WebAudioDecodeJob();
};







class MediaBufferDecoder
{
public:
  void AsyncDecodeMedia(const char* aContentType, uint8_t* aBuffer,
                        uint32_t aLength, WebAudioDecodeJob& aDecodeJob);

  ~MediaBufferDecoder() { Shutdown(); }
  void Shutdown();

  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return 0;
  }

private:
  bool EnsureThreadPoolInitialized();

private:
  nsCOMPtr<nsIThreadPool> mThreadPool;
};

}

#endif

