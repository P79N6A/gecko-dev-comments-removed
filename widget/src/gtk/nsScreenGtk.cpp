





































#include "nsScreenGtk.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <X11/Xatom.h>

nsScreenGtk :: nsScreenGtk (  )
  : mScreenNum(0),
    mRect(0, 0, 0, 0),
    mAvailRect(0, 0, 0, 0)
{
}


nsScreenGtk :: ~nsScreenGtk()
{
}



NS_IMPL_ISUPPORTS1(nsScreenGtk, nsIScreen)


NS_IMETHODIMP
nsScreenGtk :: GetRect(PRInt32 *outLeft, PRInt32 *outTop, PRInt32 *outWidth, PRInt32 *outHeight)
{
  *outLeft = mRect.x;
  *outTop = mRect.y;
  *outWidth = mRect.width;
  *outHeight = mRect.height;

  return NS_OK;
  
} 


NS_IMETHODIMP
nsScreenGtk :: GetAvailRect(PRInt32 *outLeft, PRInt32 *outTop, PRInt32 *outWidth, PRInt32 *outHeight)
{
  *outLeft = mAvailRect.x;
  *outTop = mAvailRect.y;
  *outWidth = mAvailRect.width;
  *outHeight = mAvailRect.height;

  return NS_OK;
  
} 


NS_IMETHODIMP 
nsScreenGtk :: GetPixelDepth(PRInt32 *aPixelDepth)
{
  GdkVisual * rgb_visual = gdk_rgb_get_visual();
  *aPixelDepth = rgb_visual->depth;

  return NS_OK;

} 


NS_IMETHODIMP 
nsScreenGtk :: GetColorDepth(PRInt32 *aColorDepth)
{
  return GetPixelDepth ( aColorDepth );

} 


void
nsScreenGtk :: Init ()
{
  mAvailRect = mRect = nsRect(0, 0, gdk_screen_width(), gdk_screen_height());

  
  

  
  
  
  
  
  

#if GTK_CHECK_VERSION(2,2,0)
  GdkWindow *root_window = gdk_get_default_root_window();
#else
  GdkWindow *root_window = GDK_ROOT_PARENT();
#endif 

  long *workareas;
  GdkAtom type_returned;
  int format_returned;
  int length_returned;

#if GTK_CHECK_VERSION(2,0,0)
  GdkAtom cardinal_atom = gdk_x11_xatom_to_atom(XA_CARDINAL);
#else
  GdkAtom cardinal_atom = (GdkAtom) XA_CARDINAL;
#endif

  gdk_error_trap_push();

  
  if (!gdk_property_get(root_window,
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
      nsRect workarea(workareas[i],     workareas[i + 1],
                      workareas[i + 2], workareas[i + 3]);
      if (!mRect.Contains(workarea)) {
        NS_WARNING("Invalid bounds");
        continue;
      }

      mAvailRect.IntersectRect(mAvailRect, workarea);
    }
  }
  g_free (workareas);
}

#ifdef MOZ_ENABLE_XINERAMA
void
nsScreenGtk :: Init (XineramaScreenInfo *aScreenInfo)
{
  nsRect xineRect(aScreenInfo->x_org, aScreenInfo->y_org,
                  aScreenInfo->width, aScreenInfo->height);

  mScreenNum = aScreenInfo->screen_number;

  mAvailRect = mRect = xineRect;
}
#endif 
