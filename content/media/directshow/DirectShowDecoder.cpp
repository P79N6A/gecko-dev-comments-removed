





#include "DirectShowDecoder.h"
#include "DirectShowReader.h"
#include "MediaDecoderStateMachine.h"
#include "mozilla/Preferences.h"
#include "WinUtils.h"

using namespace mozilla::widget;

namespace mozilla {

MediaDecoderStateMachine* DirectShowDecoder::CreateStateMachine()
{
  return new MediaDecoderStateMachine(this, new DirectShowReader(this));
}


bool
DirectShowDecoder::GetSupportedCodecs(const nsACString& aType,
                                      char const *const ** aCodecList)
{
  if (!IsEnabled()) {
    return false;
  }

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

  return false;
}


bool
DirectShowDecoder::IsEnabled()
{
  return (WinUtils::GetWindowsVersion() < WinUtils::VISTA_VERSION) &&
          Preferences::GetBool("media.directshow.enabled");
}

DirectShowDecoder::DirectShowDecoder()
{
  MOZ_COUNT_CTOR(DirectShowDecoder);
}

DirectShowDecoder::~DirectShowDecoder()
{
  MOZ_COUNT_DTOR(DirectShowDecoder);
}

} 

