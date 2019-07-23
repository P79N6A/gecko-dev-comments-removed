




































#ifndef nsWebShellWindow_h__
#define nsWebShellWindow_h__

#include "nsEvent.h"
#include "nsIWebProgressListener.h"
#include "nsITimer.h"


#include "nsIDOMDocument.h"

#include "nsCOMPtr.h"
#include "nsXULWindow.h"


class nsIURI;
class nsIAppShell;

class nsWebShellWindow : public nsXULWindow,
                         public nsIWebProgressListener
{
public:
  nsWebShellWindow(PRUint32 aChromeFlags);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  nsresult Initialize(nsIXULWindow * aParent, nsIXULWindow * aOpener,
                      nsIAppShell* aShell, nsIURI* aUrl,
                      PRInt32 aInitialWidth, PRInt32 aInitialHeight,
                      PRBool aIsHiddenWindow,
                      nsWidgetInitData& widgetInitData);

  nsresult Toolbar();

  
  NS_DECL_NSIWEBPROGRESSLISTENER

  
  NS_IMETHOD Destroy();

protected:
  
  virtual ~nsWebShellWindow();

  nsCOMPtr<nsIDOMDocument> GetNamedDOMDoc(const nsAString & aWebShellName);

  void                     LoadContentAreas();
  PRBool                   ExecuteCloseHandler();

  static nsEventStatus HandleEvent(nsGUIEvent *aEvent);

  nsCOMPtr<nsITimer>      mSPTimer;
  PRLock *                mSPTimerLock;

  void        SetPersistenceTimer(PRUint32 aDirtyFlags);
  static void FirePersistenceTimer(nsITimer *aTimer, void *aClosure);
};


#endif 
