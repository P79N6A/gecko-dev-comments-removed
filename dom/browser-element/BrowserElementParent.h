



#ifndef mozilla_BrowserElementHelpers_h
#define mozilla_BrowserElementHelpers_h

#include "nsAString.h"

class nsIDOMWindow;
class nsIURI;

namespace mozilla {

namespace dom {
class TabParent;
}












class BrowserElementParent
{
public:
  





























  static bool
  OpenWindowOOP(mozilla::dom::TabParent* aOpenerTabParent,
                mozilla::dom::TabParent* aPopupTabParent,
                const nsAString& aURL,
                const nsAString& aName,
                const nsAString& aFeatures);

  








  static bool
  OpenWindowInProcess(nsIDOMWindow* aOpenerWindow,
                      nsIURI* aURI,
                      const nsAString& aName,
                      const nsACString& aFeatures,
                      nsIDOMWindow** aReturnWindow);
};

} 

#endif
