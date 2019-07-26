




#include "nsAutoPtr.h"
#include "nsWeakReference.h"

#include "nsIDOMWindowUtils.h"
#include "nsEvent.h"
#include "mozilla/Attributes.h"

class nsGlobalWindow;
class nsIPresShell;

class nsDOMWindowUtils MOZ_FINAL : public nsIDOMWindowUtils,
                                   public nsSupportsWeakReference
{
public:
  nsDOMWindowUtils(nsGlobalWindow *aWindow);
  ~nsDOMWindowUtils();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMWINDOWUTILS

protected:
  nsWeakPtr mWindow;

  
  
  
  
  nsIWidget* GetWidget(nsPoint* aOffset = nullptr);
  nsIWidget* GetWidgetForElement(nsIDOMElement* aElement);

  nsIPresShell* GetPresShell();
  nsPresContext* GetPresContext();

  NS_IMETHOD SendMouseEventCommon(const nsAString& aType,
                                  float aX,
                                  float aY,
                                  int32_t aButton,
                                  int32_t aClickCount,
                                  int32_t aModifiers,
                                  bool aIgnoreRootScrollFrame,
                                  float aPressure,
                                  unsigned short aInputSourceArg,
                                  bool aToWindow);

  static mozilla::widget::Modifiers GetWidgetModifiers(int32_t aModifiers);
};
