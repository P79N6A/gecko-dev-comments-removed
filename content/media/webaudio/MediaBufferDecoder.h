





#ifndef MediaBufferDecoder_h_
#define MediaBufferDecoder_h_

#include "nsWrapperCache.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIThreadPool.h"
#include "nsString.h"
#include "nsTArray.h"
#include "mozilla/dom/TypedArray.h"
#include <utility>

namespace mozilla {

class MediaDecoderReader;
namespace dom {
class AudioBuffer;
class AudioContext;
class DecodeErrorCallback;
class DecodeSuccessCallback;
}

struct WebAudioDecodeJob
{
  WebAudioDecodeJob(const nsACString& aContentType,
                    const dom::ArrayBuffer& aBuffer,
                    dom::AudioContext* aContext,
                    dom::DecodeSuccessCallback* aSuccessCallback,
                    dom::DecodeErrorCallback* aFailureCallback);
  ~WebAudioDecodeJob();

  enum ErrorCode {
    NoError,
    UnknownContent,
    UnknownError,
    InvalidContent,
    NoAudio
  };

  typedef void (WebAudioDecodeJob::*ResultFn)(ErrorCode);
  typedef std::pair<void*, float*> ChannelBuffer;

  void OnSuccess(ErrorCode );
  void OnFailure(ErrorCode aErrorCode);

  bool AllocateBuffer();
  JSContext* GetJSContext() const;
  bool FinalizeBufferData();

  nsCString mContentType;
  uint8_t* mBuffer;
  uint32_t mLength;
  uint32_t mChannels;
  uint32_t mSourceSampleRate;
  uint32_t mFrames;
  uint32_t mResampledFrames; 
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

  void Shutdown();

private:
  bool EnsureThreadPoolInitialized();

private:
  nsCOMPtr<nsIThreadPool> mThreadPool;
};

}

#endif

