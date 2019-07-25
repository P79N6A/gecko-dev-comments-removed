





#include "EnableWebAudioCheck.h"
#include "mozilla/Preferences.h"

namespace {

bool gPrefInitialized = false;
bool gWebAudioEnabled = false;

}

namespace mozilla {
namespace dom {

 bool
EnableWebAudioCheck::PrefEnabled()
{
  if (!gPrefInitialized) {
    Preferences::AddBoolVarCache(&gWebAudioEnabled, "media.webaudio.enabled");
    gPrefInitialized = true;
  }
  return gWebAudioEnabled;
}

}
}

