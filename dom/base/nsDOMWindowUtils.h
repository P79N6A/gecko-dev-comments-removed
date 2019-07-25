




































#include "nsAutoPtr.h"
#include "nsWeakReference.h"

#include "nsIDOMWindowUtils.h"

class nsGlobalWindow;
class nsIPresShell;

class nsDOMWindowUtils : public nsIDOMWindowUtils,
                         public nsIDOMWindowUtils_MOZILLA_2_0_BRANCH,
                         public nsSupportsWeakReference
{
public:
  nsDOMWindowUtils(nsGlobalWindow *aWindow);
  ~nsDOMWindowUtils();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMWINDOWUTILS
  NS_DECL_NSIDOMWINDOWUTILS_MOZILLA_2_0_BRANCH

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
                                  PRBool aIgnoreRootScrollFrame,
                                  PRBool aToWindow);
};
