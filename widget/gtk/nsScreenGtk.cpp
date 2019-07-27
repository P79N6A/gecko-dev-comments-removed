




#include "nsScreenGtk.h"

#include <gdk/gdk.h>
#ifdef MOZ_X11
#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#endif
#include <gtk/gtk.h>
#include <dlfcn.h>
#include "gfxPlatformGtk.h"

static uint32_t sScreenId = 0;


nsScreenGtk :: nsScreenGtk (  )
  : mScreenNum(0),
    mRect(0, 0, 0, 0),
    mAvailRect(0, 0, 0, 0),
    mId(++sScreenId)
{
}


nsScreenGtk :: ~nsScreenGtk()
{
}


NS_IMETHODIMP
nsScreenGtk :: GetId(uint32_t *aId)
{
  *aId = mId;
  return NS_OK;
} 


NS_IMETHODIMP
nsScreenGtk :: GetRect(int32_t *outLeft, int32_t *outTop, int32_t *outWidth, int32_t *outHeight)
{
  *outLeft = mRect.x;
  *outTop = mRect.y;
  *outWidth = mRect.width;
  *outHeight = mRect.height;

  return NS_OK;
  
} 


NS_IMETHODIMP
nsScreenGtk :: GetAvailRect(int32_t *outLeft, int32_t *outTop, int32_t *outWidth, int32_t *outHeight)
{
  *outLeft = mAvailRect.x;
  *outTop = mAvailRect.y;
  *outWidth = mAvailRect.width;
  *outHeight = mAvailRect.height;

  return NS_OK;
  
} 

gint
nsScreenGtk :: GetGtkMonitorScaleFactor()
{
#if (MOZ_WIDGET_GTK >= 3)
  
  static auto sGdkScreenGetMonitorScaleFactorPtr = (gint (*)(GdkScreen*, gint))
      dlsym(RTLD_DEFAULT, "gdk_screen_get_monitor_scale_factor");
  if (sGdkScreenGetMonitorScaleFactorPtr) {
      
      
      GdkScreen *screen = gdk_screen_get_default();
      return sGdkScreenGetMonitorScaleFactorPtr(screen, 0);
  }
#endif
    return 1;
}

double
nsScreenGtk :: GetDPIScale()
{
  double dpiScale = nsIWidget::DefaultScaleOverride();
  if (dpiScale <= 0.0) {
    dpiScale = GetGtkMonitorScaleFactor() * gfxPlatformGtk::GetDPIScale();
  }
  return dpiScale;
}

NS_IMETHODIMP
nsScreenGtk :: GetRectDisplayPix(int32_t *outLeft,  int32_t *outTop, int32_t *outWidth, int32_t *outHeight)
{
  int32_t left, top, width, height;

  GetRect(&left, &top, &width, &height);

  double scaleFactor = 1.0 / GetDPIScale();
  *outLeft = NSToIntRound(left * scaleFactor);
  *outTop = NSToIntRound(top * scaleFactor);
  *outWidth = NSToIntRound(width * scaleFactor);
  *outHeight = NSToIntRound(height * scaleFactor);

  return NS_OK;
}

NS_IMETHODIMP
nsScreenGtk :: GetAvailRectDisplayPix(int32_t *outLeft,  int32_t *outTop,  int32_t *outWidth, int32_t *outHeight)
{
  int32_t left, top, width, height;

  GetAvailRect(&left, &top, &width, &height);

  double scaleFactor = 1.0 / GetDPIScale();
  *outLeft = NSToIntRound(left * scaleFactor);
  *outTop = NSToIntRound(top * scaleFactor);
  *outWidth = NSToIntRound(width * scaleFactor);
  *outHeight = NSToIntRound(height * scaleFactor);

  return NS_OK;
}

NS_IMETHODIMP 
nsScreenGtk :: GetPixelDepth(int32_t *aPixelDepth)
{
  GdkVisual * visual = gdk_screen_get_system_visual(gdk_screen_get_default());
  *aPixelDepth = gdk_visual_get_depth(visual);

  return NS_OK;

} 


NS_IMETHODIMP 
nsScreenGtk :: GetColorDepth(int32_t *aColorDepth)
{
  return GetPixelDepth ( aColorDepth );

} 


void
nsScreenGtk :: Init (GdkWindow *aRootWindow)
{
  gint scale = nsScreenGtk::GetGtkMonitorScaleFactor();
  gint width = gdk_screen_width()*scale;
  gint height = gdk_screen_height()*scale;

  
  
  
  
  mAvailRect = mRect = nsIntRect(0, 0, width, height);

#ifdef MOZ_X11
  
  

  
  
  
  

  long *workareas;
  GdkAtom type_returned;
  int format_returned;
  int length_returned;

  GdkAtom cardinal_atom = gdk_x11_xatom_to_atom(XA_CARDINAL);

  gdk_error_trap_push();

  
  if (!gdk_property_get(aRootWindow,
                        gdk_atom_intern ("_NET_WORKAREA", FALSE),
                        cardinal_atom,
                        0, G_MAXLONG - 3, FALSE,
                        &type_returned,
                        &format_returned,
                        &length_returned,
                        (guchar **) &workareas)) {
    
    
    return;
  }

  
  gdk_flush();

  if (!gdk_error_trap_pop() &&
      type_returned == cardinal_atom &&
      length_returned && (length_returned % 4) == 0 &&
      format_returned == 32) {
    int num_items = length_returned / sizeof(long);

    for (int i = 0; i < num_items; i += 4) {
      nsIntRect workarea(workareas[i],     workareas[i + 1],
                         workareas[i + 2], workareas[i + 3]);
      if (!mRect.Contains(workarea)) {
        
        
        
        
        
        
        
        NS_WARNING("Invalid bounds");
        continue;
      }

      mAvailRect.IntersectRect(mAvailRect, workarea);
    }
  }
  g_free (workareas);
#endif
}

#ifdef MOZ_X11
void
nsScreenGtk :: Init (XineramaScreenInfo *aScreenInfo)
{
  nsIntRect xineRect(aScreenInfo->x_org, aScreenInfo->y_org,
                     aScreenInfo->width, aScreenInfo->height);

  mScreenNum = aScreenInfo->screen_number;

  mAvailRect = mRect = xineRect;
}
#endif
