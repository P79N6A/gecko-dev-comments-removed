




#ifndef nsWebShellWindow_h__
#define nsWebShellWindow_h__

#include "mozilla/Mutex.h"
#include "nsEvent.h"
#include "nsIWebProgressListener.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsXULWindow.h"
#include "nsIWidgetListener.h"


class nsIURI;

namespace mozilla {
class WebShellWindowTimerCallback;
} 

class nsWebShellWindow : public nsXULWindow,
                         public nsIWebProgressListener,
                         public nsIWidgetListener
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

  
  virtual nsIXULWindow* GetXULWindow() { return this; }
  virtual nsIPresShell* GetPresShell();
  virtual bool WindowMoved(nsIWidget* aWidget, PRInt32 x, PRInt32 y);
  virtual bool WindowResized(nsIWidget* aWidget, PRInt32 aWidth, PRInt32 aHeight);
  virtual bool RequestWindowClose(nsIWidget* aWidget);
  virtual void SizeModeChanged(nsSizeMode sizeMode);
  virtual void OSToolbarButtonPressed();
  virtual bool ZLevelChanged(bool aImmediate, nsWindowZ *aPlacement,
                             nsIWidget* aRequestBelow, nsIWidget** aActualBelow);
  virtual void WindowActivated();
  virtual void WindowDeactivated();

protected:
  friend class mozilla::WebShellWindowTimerCallback;
  
  virtual ~nsWebShellWindow();

  void                     LoadContentAreas();
  bool                     ExecuteCloseHandler();
  void                     ConstrainToOpenerScreen(PRInt32* aX, PRInt32* aY);

  nsCOMPtr<nsITimer>      mSPTimer;
  mozilla::Mutex          mSPTimerLock;

  void        SetPersistenceTimer(PRUint32 aDirtyFlags);
  void        FirePersistenceTimer();
};


#endif 
