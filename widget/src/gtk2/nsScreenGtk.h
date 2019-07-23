




































#ifndef nsScreenGtk_h___
#define nsScreenGtk_h___

#include "nsIScreen.h"
#include "nsRect.h"
#include "gdk/gdk.h"
#include <X11/Xlib.h>


typedef struct {
   int   screen_number;
   short x_org;
   short y_org;
   short width;
   short height;
} XineramaScreenInfo;



class nsScreenGtk : public nsIScreen
{
public:
  nsScreenGtk();
  ~nsScreenGtk();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREEN

  void Init(PRBool aReInit = PR_FALSE);
  void ReInit() { Init(PR_TRUE); }
  void Init(XineramaScreenInfo *aScreenInfo);

  Atom NetWorkareaAtom() { return mNetWorkareaAtom; }

private:
  GdkWindow *mRootWindow;
  PRUint32 mScreenNum;
  Atom mNetWorkareaAtom;
  nsRect mRect; 
  nsRect mAvailRect; 
};

#endif  
