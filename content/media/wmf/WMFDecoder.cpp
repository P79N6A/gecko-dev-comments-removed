





#include "WMF.h"
#include "WMFDecoder.h"
#include "WMFReader.h"
#include "WMFUtils.h"
#include "MediaDecoderStateMachine.h"
#include "mozilla/Preferences.h"
#include "WinUtils.h"

using namespace mozilla::widget;

namespace mozilla {

MediaDecoderStateMachine* WMFDecoder::CreateStateMachine()
{
  return new MediaDecoderStateMachine(this, new WMFReader(this));
}

bool
WMFDecoder::GetSupportedCodecs(const nsACString& aType,
                               char const *const ** aCodecList)
{
  if (!MediaDecoder::IsWMFEnabled() ||
      NS_FAILED(LoadDLLs()))
    return false;

  
  
  static char const *const mp3AudioCodecs[] = {
    "mp3",
    nullptr
  };
  if (aType.EqualsASCII("audio/mpeg") ||
      aType.EqualsASCII("audio/mp3")) {
    if (aCodecList) {
      *aCodecList = mp3AudioCodecs;
    }
    return true;
  }

  
  static char const *const aacAudioCodecs[] = {
    "mp4a.40.2",    
    nullptr
  };
  if (aType.EqualsASCII("audio/mp4") ||
      aType.EqualsASCII("audio/x-m4a")) {
    if (aCodecList) {
      *aCodecList = aacAudioCodecs;
    }
    return true;
  }

  
  static char const *const H264Codecs[] = {
    "avc1.42E01E",  
    "avc1.42001E",  
    "avc1.58A01E",  
    "avc1.4D401E",  
    "avc1.64001E",  
    "avc1.64001F",  
    "mp4a.40.2",    
    nullptr
  };
  if (aType.EqualsASCII("video/mp4")) {
    if (aCodecList) {
      *aCodecList = H264Codecs;
    }
    return true;
  }

  return false;
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
  
  return WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION &&
         Preferences::GetBool("media.windows-media-foundation.enabled");
}

} 

