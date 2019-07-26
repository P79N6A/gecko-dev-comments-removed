




#ifndef __nsAppShellService_h
#define __nsAppShellService_h

#include "nsIAppShellService.h"
#include "nsIObserver.h"


#include "nsWebShellWindow.h"
#include "nsStringFwd.h"
#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"


#define NS_APPSHELLSERVICE_CID \
{ 0x99907d, 0x123c, 0x4853, { 0xa4, 0x6a, 0x43, 0x9, 0x8b, 0x5f, 0xb6, 0x8c } }

class nsAppShellService MOZ_FINAL : public nsIAppShellService,
                                    public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAPPSHELLSERVICE
  NS_DECL_NSIOBSERVER

  nsAppShellService();

protected:
  ~nsAppShellService();

  nsresult JustCreateTopWindow(nsIXULWindow *aParent,
                               nsIURI *aUrl, 
                               uint32_t aChromeMask,
                               int32_t aInitialWidth, int32_t aInitialHeight,
                               bool aIsHiddenWindow,
                               nsWebShellWindow **aResult);
  uint32_t CalculateWindowZLevel(nsIXULWindow *aParent, uint32_t aChromeMask);

  nsRefPtr<nsWebShellWindow>  mHiddenWindow;
#ifdef MOZ_PER_WINDOW_PRIVATE_BROWSING
  nsRefPtr<nsWebShellWindow>  mHiddenPrivateWindow;
#endif
  bool                        mXPCOMWillShutDown;
  bool                        mXPCOMShuttingDown;
  uint16_t                    mModalWindowCount;
  bool                        mApplicationProvidedHiddenWindow;
};

#endif
