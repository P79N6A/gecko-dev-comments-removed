




#include "nsScreenManagerGtk.h"
#include "nsScreenGtk.h"
#include "nsIComponentManager.h"
#include "nsRect.h"
#include "nsAutoPtr.h"

#define SCREEN_MANAGER_LIBRARY_LOAD_FAILED ((PRLibrary*)1)

#ifdef MOZ_X11
#include <gdk/gdkx.h>

typedef Bool (*_XnrmIsActive_fn)(Display *dpy);
typedef XineramaScreenInfo* (*_XnrmQueryScreens_fn)(Display *dpy, int *number);
#endif

#include <gtk/gtk.h>


static GdkFilterReturn
root_window_event_filter(GdkXEvent *aGdkXEvent, GdkEvent *aGdkEvent,
                         gpointer aClosure)
{
  nsScreenManagerGtk *manager = static_cast<nsScreenManagerGtk*>(aClosure);
#ifdef MOZ_X11
  XEvent *xevent = static_cast<XEvent*>(aGdkXEvent);

  
  switch (xevent->type) {
    case ConfigureNotify:
      manager->Init();
      break;
    case PropertyNotify:
      {
        XPropertyEvent *propertyEvent = &xevent->xproperty;
        if (propertyEvent->atom == manager->NetWorkareaAtom()) {
          manager->Init();
        }
      }
      break;
    default:
      break;
  }
#endif

  return GDK_FILTER_CONTINUE;
}

nsScreenManagerGtk :: nsScreenManagerGtk ( )
  : mXineramalib(nullptr)
  , mRootWindow(nullptr)
{
  
  
  
}


nsScreenManagerGtk :: ~nsScreenManagerGtk()
{
  if (mRootWindow) {
    gdk_window_remove_filter(mRootWindow, root_window_event_filter, this);
    g_object_unref(mRootWindow);
    mRootWindow = nullptr;
  }

  






}



NS_IMPL_ISUPPORTS(nsScreenManagerGtk, nsIScreenManager)



nsresult
nsScreenManagerGtk :: EnsureInit()
{
  if (mCachedScreenArray.Count() > 0)
    return NS_OK;

  mRootWindow = gdk_get_default_root_window();
  g_object_ref(mRootWindow);

  
  
  gdk_window_set_events(mRootWindow,
                        GdkEventMask(gdk_window_get_events(mRootWindow) |
                                     GDK_STRUCTURE_MASK |
                                     GDK_PROPERTY_CHANGE_MASK));
  gdk_window_add_filter(mRootWindow, root_window_event_filter, this);
#ifdef MOZ_X11
  mNetWorkareaAtom =
    XInternAtom(GDK_WINDOW_XDISPLAY(mRootWindow), "_NET_WORKAREA", False);
#endif

  return Init();
}

nsresult
nsScreenManagerGtk :: Init()
{
#ifdef MOZ_X11
  XineramaScreenInfo *screenInfo = nullptr;
  int numScreens;

  if (!mXineramalib) {
    mXineramalib = PR_LoadLibrary("libXinerama.so.1");
    if (!mXineramalib) {
      mXineramalib = SCREEN_MANAGER_LIBRARY_LOAD_FAILED;
    }
  }
  if (mXineramalib && mXineramalib != SCREEN_MANAGER_LIBRARY_LOAD_FAILED) {
    _XnrmIsActive_fn _XnrmIsActive = (_XnrmIsActive_fn)
        PR_FindFunctionSymbol(mXineramalib, "XineramaIsActive");

    _XnrmQueryScreens_fn _XnrmQueryScreens = (_XnrmQueryScreens_fn)
        PR_FindFunctionSymbol(mXineramalib, "XineramaQueryScreens");
        
    
    Display *display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    if (_XnrmIsActive && _XnrmQueryScreens && _XnrmIsActive(display)) {
      screenInfo = _XnrmQueryScreens(display, &numScreens);
    }
  }

  
  
  if (!screenInfo || numScreens == 1) {
    numScreens = 1;
#endif
    nsRefPtr<nsScreenGtk> screen;

    if (mCachedScreenArray.Count() > 0) {
      screen = static_cast<nsScreenGtk*>(mCachedScreenArray[0]);
    } else {
      screen = new nsScreenGtk();
      if (!screen || !mCachedScreenArray.AppendObject(screen)) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }

    screen->Init(mRootWindow);
#ifdef MOZ_X11
  }
  
  
  
  else {
#ifdef DEBUG
    printf("Xinerama superpowers activated for %d screens!\n", numScreens);
#endif
    for (int i = 0; i < numScreens; ++i) {
      nsRefPtr<nsScreenGtk> screen;
      if (mCachedScreenArray.Count() > i) {
        screen = static_cast<nsScreenGtk*>(mCachedScreenArray[i]);
      } else {
        screen = new nsScreenGtk();
        if (!screen || !mCachedScreenArray.AppendObject(screen)) {
          return NS_ERROR_OUT_OF_MEMORY;
        }
      }

      
      screen->Init(&screenInfo[i]);
    }
  }
  
  while (mCachedScreenArray.Count() > numScreens) {
    mCachedScreenArray.RemoveObjectAt(mCachedScreenArray.Count() - 1);
  }

  if (screenInfo) {
    XFree(screenInfo);
  }
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsScreenManagerGtk :: ScreenForId ( uint32_t aId, nsIScreen **outScreen )
{
  *outScreen = nullptr;

  nsresult rv;
  rv = EnsureInit();
  if (NS_FAILED(rv)) {
    NS_ERROR("nsScreenManagerGtk::EnsureInit() failed from ScreenForId");
    return rv;
  }

  for (int32_t i = 0, i_end = mCachedScreenArray.Count(); i < i_end; ++i) {
    uint32_t id;
    rv = mCachedScreenArray[i]->GetId(&id);
    if (NS_SUCCEEDED(rv) && id == aId) {
      NS_IF_ADDREF(*outScreen = mCachedScreenArray[i]);
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}










NS_IMETHODIMP
nsScreenManagerGtk :: ScreenForRect( int32_t aX, int32_t aY,
                                     int32_t aWidth, int32_t aHeight,
                                     nsIScreen **aOutScreen )
{
  uint32_t scale = nsScreenGtk::GetDPIScale();
  return ScreenForRectPix(aX*scale, aY*scale, aWidth*scale, aHeight*scale,
                          aOutScreen);
}









nsresult
nsScreenManagerGtk :: ScreenForRectPix( int32_t aX, int32_t aY,
                                        int32_t aWidth, int32_t aHeight,
                                        nsIScreen **aOutScreen )
{
  nsresult rv;
  rv = EnsureInit();
  if (NS_FAILED(rv)) {
    NS_ERROR("nsScreenManagerGtk::EnsureInit() failed from ScreenForRect");
    return rv;
  }

  
  uint32_t which = 0;
  
  
  
  if (mCachedScreenArray.Count() > 1) {
    
    
    uint32_t area = 0;
    nsIntRect windowRect(aX, aY, aWidth, aHeight);
    for (int32_t i = 0, i_end = mCachedScreenArray.Count(); i < i_end; ++i) {
      int32_t  x, y, width, height;
      x = y = width = height = 0;
      mCachedScreenArray[i]->GetRect(&x, &y, &width, &height);
      
      nsIntRect screenRect(x, y, width, height);
      screenRect.IntersectRect(screenRect, windowRect);
      uint32_t tempArea = screenRect.width * screenRect.height;
      if (tempArea >= area) {
        which = i;
        area = tempArea;
      }
    }
  }
  *aOutScreen = mCachedScreenArray.SafeObjectAt(which);
  NS_IF_ADDREF(*aOutScreen);
  return NS_OK;
    
} 








NS_IMETHODIMP 
nsScreenManagerGtk :: GetPrimaryScreen(nsIScreen * *aPrimaryScreen) 
{
  nsresult rv;
  rv =  EnsureInit();
  if (NS_FAILED(rv)) {
    NS_ERROR("nsScreenManagerGtk::EnsureInit() failed from GetPrimaryScreen");
    return rv;
  }
  *aPrimaryScreen = mCachedScreenArray.SafeObjectAt(0);
  NS_IF_ADDREF(*aPrimaryScreen);
  return NS_OK;
  
} 







NS_IMETHODIMP
nsScreenManagerGtk :: GetNumberOfScreens(uint32_t *aNumberOfScreens)
{
  nsresult rv;
  rv = EnsureInit();
  if (NS_FAILED(rv)) {
    NS_ERROR("nsScreenManagerGtk::EnsureInit() failed from GetNumberOfScreens");
    return rv;
  }
  *aNumberOfScreens = mCachedScreenArray.Count();
  return NS_OK;
  
} 

NS_IMETHODIMP
nsScreenManagerGtk::GetSystemDefaultScale(float *aDefaultScale)
{
  *aDefaultScale = nsScreenGtk::GetDPIScale();
  return NS_OK;
}

NS_IMETHODIMP
nsScreenManagerGtk :: ScreenForNativeWidget (void *aWidget, nsIScreen **outScreen)
{
  nsresult rv;
  rv = EnsureInit();
  if (NS_FAILED(rv)) {
    NS_ERROR("nsScreenManagerGtk::EnsureInit() failed from ScreenForNativeWidget");
    return rv;
  }

  if (mCachedScreenArray.Count() > 1) {
    
    
    gint x, y, width, height;
#if (MOZ_WIDGET_GTK == 2)
    gint depth;
#endif
    x = y = width = height = 0;

#if (MOZ_WIDGET_GTK == 2)
    gdk_window_get_geometry(GDK_WINDOW(aWidget), &x, &y, &width, &height,
                            &depth);
#else
    gdk_window_get_geometry(GDK_WINDOW(aWidget), &x, &y, &width, &height);
#endif
    gdk_window_get_origin(GDK_WINDOW(aWidget), &x, &y);
    rv = ScreenForRectPix(x, y, width, height, outScreen);
  } else {
    rv = GetPrimaryScreen(outScreen);
  }

  return rv;
}
