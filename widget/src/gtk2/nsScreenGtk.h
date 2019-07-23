




































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

  void Init(GdkWindow *aRootWindow);
  void Init(XineramaScreenInfo *aScreenInfo);

private:
  PRUint32 mScreenNum;
  nsRect mRect; 
  nsRect mAvailRect; 
};

#endif  
