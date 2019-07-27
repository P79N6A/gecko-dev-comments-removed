




#ifndef nsScreenGtk_h___
#define nsScreenGtk_h___

#include "nsBaseScreen.h"
#include "nsRect.h"
#include "gdk/gdk.h"
#ifdef MOZ_X11
#include <X11/Xlib.h>


typedef struct {
   int   screen_number;
   short x_org;
   short y_org;
   short width;
   short height;
} XineramaScreenInfo;
#endif 



class nsScreenGtk : public nsBaseScreen
{
public:
  nsScreenGtk();
  ~nsScreenGtk();

  NS_IMETHOD GetId(uint32_t* aId);
  NS_IMETHOD GetRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
  NS_IMETHOD GetAvailRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
  NS_IMETHOD GetRectDisplayPix(int32_t *outLeft, int32_t *outTop, int32_t *outWidth, int32_t *outHeight);
  NS_IMETHOD GetAvailRectDisplayPix(int32_t *outLeft, int32_t *outTop, int32_t *outWidth, int32_t *outHeight);
  NS_IMETHOD GetPixelDepth(int32_t* aPixelDepth);
  NS_IMETHOD GetColorDepth(int32_t* aColorDepth);

  void Init(GdkWindow *aRootWindow);
#ifdef MOZ_X11
  void Init(XineramaScreenInfo *aScreenInfo);
#endif 

  static gint    GetGtkMonitorScaleFactor();
  static double  GetDPIScale();

private:
  uint32_t mScreenNum;
  nsIntRect mRect;
  nsIntRect mAvailRect;
  uint32_t mId;
};

#endif  
