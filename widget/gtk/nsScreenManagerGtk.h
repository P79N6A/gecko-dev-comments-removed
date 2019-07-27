




#ifndef nsScreenManagerGtk_h___
#define nsScreenManagerGtk_h___

#include "nsIScreenManager.h"
#include "nsIScreen.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "prlink.h"
#include "gdk/gdk.h"
#ifdef MOZ_X11
#include <X11/Xlib.h>
#endif



class nsScreenManagerGtk : public nsIScreenManager
{
public:
  nsScreenManagerGtk ( );

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREENMANAGER

#ifdef MOZ_X11
  Atom NetWorkareaAtom() { return mNetWorkareaAtom; }
#endif
  
  
  nsresult Init();

private:
  virtual ~nsScreenManagerGtk();

  nsresult EnsureInit();
  nsresult ScreenForRectPix(int32_t aX, int32_t aY,
                            int32_t aWidth, int32_t aHeight,
                            nsIScreen **aOutScreen);

  
  nsCOMArray<nsIScreen> mCachedScreenArray;

  PRLibrary *mXineramalib;

  GdkWindow *mRootWindow;
#ifdef MOZ_X11
  Atom mNetWorkareaAtom;
#endif
};

#endif  
