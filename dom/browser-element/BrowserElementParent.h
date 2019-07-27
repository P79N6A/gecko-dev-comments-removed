





#ifndef mozilla_BrowserElementHelpers_h
#define mozilla_BrowserElementHelpers_h

#include "nsAString.h"
#include "mozilla/gfx/Point.h"
#include "mozilla/gfx/Rect.h"
#include "Units.h"
#include "mozilla/dom/Element.h"

class nsIDOMWindow;
class nsIURI;

namespace mozilla {

namespace dom {
class TabParent;
} 












class BrowserElementParent
{
public:

  








  enum OpenWindowResult {
    OPEN_WINDOW_ADDED,
    OPEN_WINDOW_IGNORED,
    OPEN_WINDOW_CANCELLED
  };

  































  static OpenWindowResult
  OpenWindowOOP(dom::TabParent* aOpenerTabParent,
                dom::TabParent* aPopupTabParent,
                const nsAString& aURL,
                const nsAString& aName,
                const nsAString& aFeatures);

  










  static OpenWindowResult
  OpenWindowInProcess(nsIDOMWindow* aOpenerWindow,
                      nsIURI* aURI,
                      const nsAString& aName,
                      const nsACString& aFeatures,
                      nsIDOMWindow** aReturnWindow);

  














  static bool
  DispatchAsyncScrollEvent(dom::TabParent* aTabParent,
                           const CSSRect& aContentRect,
                           const CSSSize& aContentSize);

private:
  static OpenWindowResult
  DispatchOpenWindowEvent(dom::Element* aOpenerFrameElement,
                          dom::Element* aPopupFrameElement,
                          const nsAString& aURL,
                          const nsAString& aName,
                          const nsAString& aFeatures);
};

} 

#endif
