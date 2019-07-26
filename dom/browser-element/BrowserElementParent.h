



#ifndef mozilla_BrowserElementHelpers_h
#define mozilla_BrowserElementHelpers_h

#include "nsAString.h"
#include "mozilla/gfx/Point.h"
#include "mozilla/gfx/Rect.h"
#include "Units.h"

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
  OpenWindowOOP(dom::TabParent* aOpenerTabParent,
                dom::TabParent* aPopupTabParent,
                const nsAString& aURL,
                const nsAString& aName,
                const nsAString& aFeatures);

  










  static bool
  OpenWindowInProcess(nsIDOMWindow* aOpenerWindow,
                      nsIURI* aURI,
                      const nsAString& aName,
                      const nsACString& aFeatures,
                      nsIDOMWindow** aReturnWindow);

  














  static bool
  DispatchAsyncScrollEvent(dom::TabParent* aTabParent,
                           const CSSRect& aContentRect,
                           const CSSSize& aContentSize);
};

} 

#endif
