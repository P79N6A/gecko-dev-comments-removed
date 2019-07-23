




































#ifndef nsScreenGtk_h___
#define nsScreenGtk_h___

#include "nsIScreen.h"
#include "nsRect.h"

#ifdef MOZ_ENABLE_XINERAMA
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#endif 



class nsScreenGtk : public nsIScreen
{
public:
  nsScreenGtk();
  ~nsScreenGtk();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREEN

  void Init();
#ifdef MOZ_ENABLE_XINERAMA
  void Init(XineramaScreenInfo *aScreenInfo);
#endif

private:
  PRUint32 mScreenNum;
  nsRect mRect; 
  nsRect mAvailRect; 
};

#endif  
