






#ifndef mozilla_dom_CSS_h_
#define mozilla_dom_CSS_h_

#include "mozilla/Attributes.h"
#include "mozilla/Preferences.h"

namespace mozilla {

class ErrorResult;

namespace dom {

class GlobalObject;

class CSS {
private:
  CSS() MOZ_DELETE;

public:
  static bool Supports(const GlobalObject& aGlobal,
                       const nsAString& aProperty,
                       const nsAString& aValue,
                       ErrorResult& aRv);

  static bool Supports(const GlobalObject& aGlobal,
                       const nsAString& aDeclaration,
                       ErrorResult& aRv);

  static bool PrefEnabled()
  {
    return Preferences::GetBool("layout.css.supports-rule.enabled");
  }
};

} 
} 

#endif 
