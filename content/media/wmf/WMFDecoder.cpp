





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
#ifdef MOZ_DIRECTSHOW
  if (DirectShowDecoder::IsEnabled()) {
    
    
    return false;
  }
#endif
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
  
  
  
  
  
  

  
  
  
  
  
  
  if (aCodec.Length() != strlen("avc1.PPCCLL")) {
    return false;
  }

  
  const nsAString& sample = Substring(aCodec, 0, 5);
  if (!sample.EqualsASCII("avc1.")) {
    return false;
  }

  
  
  
  nsresult rv = NS_OK;
  const int32_t profile = PromiseFlatString(Substring(aCodec, 5, 2)).ToInteger(&rv, 16);
  NS_ENSURE_SUCCESS(rv, false);

  const int32_t level = PromiseFlatString(Substring(aCodec, 9, 2)).ToInteger(&rv, 16);
  NS_ENSURE_SUCCESS(rv, false);

  return level >= eAVEncH264VLevel1 &&
         level <= eAVEncH264VLevel5_1 &&
         (profile == eAVEncH264VProfile_Base ||
          profile == eAVEncH264VProfile_Main ||
          profile == eAVEncH264VProfile_Extended ||
          profile == eAVEncH264VProfile_High);
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
    return !aCodecs.Length() || aCodecs.EqualsASCII("mp4a.40.2");
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

