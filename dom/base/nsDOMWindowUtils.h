




































#include "nsAutoPtr.h"
#include "nsWeakReference.h"

#include "nsIDOMWindowUtils.h"

class nsGlobalWindow;
class nsIPresShell;

class nsDOMWindowUtils : public nsIDOMWindowUtils,
                         public nsSupportsWeakReference
{
public:
  nsDOMWindowUtils(nsGlobalWindow *aWindow);
  ~nsDOMWindowUtils();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMWINDOWUTILS

protected:
  nsRefPtr<nsGlobalWindow> mWindow;

  
  
  
  
  nsIWidget* GetWidget(nsPoint* aOffset = nsnull);
  nsIWidget* GetWidgetForElement(nsIDOMElement* aElement);

  nsIPresShell* GetPresShell();
  nsPresContext* GetPresContext();

  NS_IMETHOD SendMouseEventCommon(const nsAString& aType,
                                  float aX,
                                  float aY,
                                  PRInt32 aButton,
                                  PRInt32 aClickCount,
                                  PRInt32 aModifiers,
                                  bool aIgnoreRootScrollFrame,
                                  bool aToWindow);
};
