






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
  CSS() = delete;

public:
  static bool Supports(const GlobalObject& aGlobal,
                       const nsAString& aProperty,
                       const nsAString& aValue,
                       ErrorResult& aRv);

  static bool Supports(const GlobalObject& aGlobal,
                       const nsAString& aDeclaration,
                       ErrorResult& aRv);

  static void Escape(const GlobalObject& aGlobal,
                     const nsAString& aIdent,
                     nsAString& aReturn,
                     ErrorResult& aRv);
};

} 
} 

#endif 
