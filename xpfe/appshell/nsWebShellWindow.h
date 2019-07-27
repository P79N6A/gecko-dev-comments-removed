




#ifndef nsWebShellWindow_h__
#define nsWebShellWindow_h__

#include "mozilla/Mutex.h"
#include "nsIWebProgressListener.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsXULWindow.h"
#include "nsIWidgetListener.h"
#include "nsITabParent.h"


class nsIURI;

struct nsWidgetInitData;

namespace mozilla {
class WebShellWindowTimerCallback;
} 

class nsWebShellWindow MOZ_FINAL : public nsXULWindow,
                                   public nsIWebProgressListener,
                                   public nsIWidgetListener
{
public:
  nsWebShellWindow(uint32_t aChromeFlags);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  nsresult Initialize(nsIXULWindow * aParent, nsIXULWindow * aOpener,
                      nsIURI* aUrl,
                      int32_t aInitialWidth, int32_t aInitialHeight,
                      bool aIsHiddenWindow,
                      nsITabParent *aOpeningTab,
                      nsWidgetInitData& widgetInitData);

  nsresult Toolbar();

  
  NS_DECL_NSIWEBPROGRESSLISTENER

  
  NS_IMETHOD Destroy();

  
  virtual nsIXULWindow* GetXULWindow() { return this; }
  virtual nsIPresShell* GetPresShell();
  virtual bool WindowMoved(nsIWidget* aWidget, int32_t x, int32_t y);
  virtual bool WindowResized(nsIWidget* aWidget, int32_t aWidth, int32_t aHeight);
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
  void                     ConstrainToOpenerScreen(int32_t* aX, int32_t* aY);

  nsCOMPtr<nsITimer>      mSPTimer;
  mozilla::Mutex          mSPTimerLock;

  void        SetPersistenceTimer(uint32_t aDirtyFlags);
  void        FirePersistenceTimer();
};


#endif 
