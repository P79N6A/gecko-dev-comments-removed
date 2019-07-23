




































#include "nsAutoPtr.h"
#include "nsWeakReference.h"

#include "nsIDOMWindowUtils.h"

class nsGlobalWindow;

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

  nsPresContext* GetPresContext();
};
