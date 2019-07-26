





#ifndef mozilla_dom_EnableSpeechSynthesisCheck_h
#define mozilla_dom_EnableSpeechSynthesisCheck_h

#include "js/TypeDecls.h"

namespace mozilla {
namespace dom {



class EnableSpeechSynthesisCheck
{
public:
  static bool PrefEnabled(JSContext* aCx = nullptr, JSObject* aGlobal = nullptr);
};

}
}

#endif
