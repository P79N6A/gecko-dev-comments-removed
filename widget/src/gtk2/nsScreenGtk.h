




































#ifndef nsScreenGtk_h___
#define nsScreenGtk_h___

#include "nsIScreen.h"
#include "nsRect.h"
#include "gdk/gdk.h"
#include <X11/Xlib.h>

#ifdef MOZ_ENABLE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif 



class nsScreenGtk : public nsIScreen
{
public:
  nsScreenGtk();
  ~nsScreenGtk();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREEN

  void Init(PRBool aReInit = PR_FALSE);
  void ReInit() { Init(PR_TRUE); }
#ifdef MOZ_ENABLE_XINERAMA
  void Init(XineramaScreenInfo *aScreenInfo);
#endif

  Atom NetWorkareaAtom() { return mNetWorkareaAtom; }

private:
  GdkWindow *mRootWindow;
  PRUint32 mScreenNum;
  Atom mNetWorkareaAtom;
  nsRect mRect; 
  nsRect mAvailRect; 
};

#endif  
