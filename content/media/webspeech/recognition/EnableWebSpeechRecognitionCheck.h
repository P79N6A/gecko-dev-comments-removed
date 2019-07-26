





#ifndef EnableWebSpeechRecognitionCheck_h
#define EnableWebSpeechRecognitionCheck_h

#include "js/TypeDecls.h"

namespace mozilla {
namespace dom {

class EnableWebSpeechRecognitionCheck
{
public:
  static bool PrefEnabled(JSContext* aCx, JSObject* aGlobal);
};

}
}

#endif
