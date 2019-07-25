




































#ifndef nsWebShellWindow_h__
#define nsWebShellWindow_h__

#include "mozilla/Mutex.h"
#include "nsEvent.h"
#include "nsIWebProgressListener.h"
#include "nsITimer.h"


#include "nsIDOMDocument.h"

#include "nsCOMPtr.h"
#include "nsXULWindow.h"


class nsIURI;

class nsWebShellWindow : public nsXULWindow,
                         public nsIWebProgressListener
{
public:
  nsWebShellWindow(PRUint32 aChromeFlags);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  nsresult Initialize(nsIXULWindow * aParent, nsIXULWindow * aOpener,
                      nsIURI* aUrl,
                      PRInt32 aInitialWidth, PRInt32 aInitialHeight,
                      bool aIsHiddenWindow,
                      nsWidgetInitData& widgetInitData);

  nsresult Toolbar();

  
  NS_DECL_NSIWEBPROGRESSLISTENER

  
  NS_IMETHOD Destroy();

protected:
  
  virtual ~nsWebShellWindow();

  void                     LoadContentAreas();
  bool                     ExecuteCloseHandler();
  void                     ConstrainToOpenerScreen(PRInt32* aX, PRInt32* aY);

  static nsEventStatus HandleEvent(nsGUIEvent *aEvent);

  nsCOMPtr<nsITimer>      mSPTimer;
  mozilla::Mutex          mSPTimerLock;

  void        SetPersistenceTimer(PRUint32 aDirtyFlags);
  static void FirePersistenceTimer(nsITimer *aTimer, void *aClosure);
};


#endif 
