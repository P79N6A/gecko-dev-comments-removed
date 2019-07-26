





#include "EnableWebSpeechRecognitionCheck.h"
#include "mozilla/Preferences.h"

namespace {

bool gPrefInitialized = false;
bool gWebSpeechEnabled = false;

}

namespace mozilla {
namespace dom {

 bool
EnableWebSpeechRecognitionCheck::PrefEnabled()
{
  if (!gPrefInitialized) {
    Preferences::AddBoolVarCache(&gWebSpeechEnabled, "media.webspeech.recognition.enable");
    gPrefInitialized = true;
  }
  return gWebSpeechEnabled;
}

}
}

