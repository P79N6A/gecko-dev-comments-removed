





#include "EnableSpeechSynthesisCheck.h"
#include "mozilla/Preferences.h"

namespace {

bool gPrefInitialized = false;
bool gWebSpeechEnabled = false;

}

namespace mozilla {
namespace dom {

 bool
EnableSpeechSynthesisCheck::PrefEnabled()
{
  if (!gPrefInitialized) {
    Preferences::AddBoolVarCache(&gWebSpeechEnabled, "media.webspeech.synth.enabled");
    gPrefInitialized = true;
  }

  return gWebSpeechEnabled;
}

}
}

