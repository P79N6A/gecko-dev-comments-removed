




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
  explicit nsWebShellWindow(uint32_t aChromeFlags);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  nsresult Initialize(nsIXULWindow * aParent, nsIXULWindow * aOpener,
                      nsIURI* aUrl,
                      int32_t aInitialWidth, int32_t aInitialHeight,
                      bool aIsHiddenWindow,
                      nsITabParent *aOpeningTab,
                      nsWidgetInitData& widgetInitData);

  nsresult Toolbar();

  
  NS_DECL_NSIWEBPROGRESSLISTENER

  
  NS_IMETHOD Destroy() MOZ_OVERRIDE;

  
  virtual nsIXULWindow* GetXULWindow() MOZ_OVERRIDE { return this; }
  virtual nsIPresShell* GetPresShell() MOZ_OVERRIDE;
  virtual bool WindowMoved(nsIWidget* aWidget, int32_t x, int32_t y) MOZ_OVERRIDE;
  virtual bool WindowResized(nsIWidget* aWidget, int32_t aWidth, int32_t aHeight) MOZ_OVERRIDE;
  virtual bool RequestWindowClose(nsIWidget* aWidget) MOZ_OVERRIDE;
  virtual void SizeModeChanged(nsSizeMode sizeMode) MOZ_OVERRIDE;
  virtual void OSToolbarButtonPressed() MOZ_OVERRIDE;
  virtual bool ZLevelChanged(bool aImmediate, nsWindowZ *aPlacement,
                             nsIWidget* aRequestBelow, nsIWidget** aActualBelow) MOZ_OVERRIDE;
  virtual void WindowActivated() MOZ_OVERRIDE;
  virtual void WindowDeactivated() MOZ_OVERRIDE;

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
