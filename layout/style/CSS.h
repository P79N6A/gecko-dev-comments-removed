






#ifndef mozilla_dom_CSS_h_
#define mozilla_dom_CSS_h_

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/Preferences.h"

namespace mozilla {
namespace dom {

class CSS {
private:
  CSS() MOZ_DELETE;

public:
  static bool Supports(nsISupports* aGlobal,
                       const nsAString& aProperty,
                       const nsAString& aValue,
                       ErrorResult& aRv);

  static bool Supports(nsISupports* aGlobal,
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
