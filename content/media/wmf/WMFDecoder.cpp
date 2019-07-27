





#include "WMF.h"
#include "WMFDecoder.h"
#include "WMFReader.h"
#include "WMFUtils.h"
#include "MediaDecoderStateMachine.h"
#include "mozilla/Preferences.h"
#include "mozilla/WindowsVersion.h"
#include "nsCharSeparatedTokenizer.h"

#ifdef MOZ_DIRECTSHOW
#include "DirectShowDecoder.h"
#endif

namespace mozilla {

MediaDecoderStateMachine* WMFDecoder::CreateStateMachine()
{
  return new MediaDecoderStateMachine(this, new WMFReader(this));
}


bool
WMFDecoder::IsMP3Supported()
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
  if (!MediaDecoder::IsWMFEnabled()) {
    return false;
  }
  
  if (!IsWin7OrLater()) {
    return true;
  }
  
  
  
  
  
  return IsWin7SP1OrLater();
}

static bool
IsSupportedH264Codec(const nsAString& aCodec)
{
  
  
  
  
  
  

  int16_t profile = 0, level = 0;
  if (!ExtractH264CodecDetails(aCodec, profile, level)) {
    return false;
  }

  return level >= H264_LEVEL_1 &&
         level <= H264_LEVEL_5_1 &&
         (profile == H264_PROFILE_BASE ||
          profile == H264_PROFILE_MAIN ||
          profile == H264_PROFILE_EXTENDED ||
          profile == H264_PROFILE_HIGH);
}

bool
WMFDecoder::CanPlayType(const nsACString& aType,
                        const nsAString& aCodecs)
{
  if (!MediaDecoder::IsWMFEnabled() ||
      NS_FAILED(LoadDLLs())) {
    return false;
  }

  
  
  if ((aType.EqualsASCII("audio/mpeg") || aType.EqualsASCII("audio/mp3")) &&
      IsMP3Supported()) {
    
    
    return !aCodecs.Length() || aCodecs.EqualsASCII("mp3");
  }

  
  if (aType.EqualsASCII("audio/mp4") || aType.EqualsASCII("audio/x-m4a")) {
    return !aCodecs.Length() ||
           aCodecs.EqualsASCII("mp4a.40.2") ||
           aCodecs.EqualsASCII("mp3");
  }

  if (!aType.EqualsASCII("video/mp4")) {
    return false;
  }

  
  
  nsCharSeparatedTokenizer tokenizer(aCodecs, ',');
  bool expectMoreTokens = false;
  while (tokenizer.hasMoreTokens()) {
    const nsSubstring& token = tokenizer.nextToken();
    expectMoreTokens = tokenizer.separatorAfterCurrentToken();
    if (token.EqualsASCII("mp4a.40.2") || 
        token.EqualsASCII("mp3") ||
        IsSupportedH264Codec(token)) {
      continue;
    }
    return false;
  }
  if (expectMoreTokens) {
    
    return false;
  }
  return true;
}

nsresult
WMFDecoder::LoadDLLs()
{
  return SUCCEEDED(wmf::LoadDLLs()) ? NS_OK : NS_ERROR_FAILURE;
}

void
WMFDecoder::UnloadDLLs()
{
  wmf::UnloadDLLs();
}


bool
WMFDecoder::IsEnabled()
{
  
  return IsVistaOrLater() &&
         Preferences::GetBool("media.windows-media-foundation.enabled");
}

} 

