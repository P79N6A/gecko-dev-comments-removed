






































#include "nsScreenManagerGtk.h"
#include "nsScreenGtk.h"
#include "nsIComponentManager.h"
#include "nsRect.h"
#include "nsAutoPtr.h"

#define SCREEN_MANAGER_LIBRARY_LOAD_FAILED ((PRLibrary*)1)

#ifdef MOZ_DFB
#include <directfb.h>
#endif

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

#ifdef MOZ_DFB
  DFBWindowEvent * dfbEvent = static_cast<DFBWindowEvent *> (aGdkXEvent);

  switch (dfbEvent->type)
  {
      case DWET_POSITION :
      case DWET_SIZE :
          manager->Init();
      break;

          

      default :
      break;
  }
#endif

  return GDK_FILTER_CONTINUE;
}

nsScreenManagerGtk :: nsScreenManagerGtk ( )
  : mXineramalib(nsnull)
  , mRootWindow(nsnull)
{
  
  
  
}


nsScreenManagerGtk :: ~nsScreenManagerGtk()
{
  if (mRootWindow) {
    gdk_window_remove_filter(mRootWindow, root_window_event_filter, this);
    g_object_unref(mRootWindow);
    mRootWindow = nsnull;
  }

  






}



NS_IMPL_ISUPPORTS1(nsScreenManagerGtk, nsIScreenManager)



nsresult
nsScreenManagerGtk :: EnsureInit()
{
  if (mCachedScreenArray.Count() > 0)
    return NS_OK;

#if GTK_CHECK_VERSION(2,2,0)
  mRootWindow = gdk_get_default_root_window();
#else
  mRootWindow = GDK_ROOT_PARENT();
#endif 
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
  XineramaScreenInfo *screenInfo = NULL;
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
        
    
    if (_XnrmIsActive && _XnrmQueryScreens &&
        _XnrmIsActive(GDK_DISPLAY())) {
      screenInfo = _XnrmQueryScreens(GDK_DISPLAY(), &numScreens);
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
nsScreenManagerGtk :: ScreenForRect ( PRInt32 aX, PRInt32 aY,
                                      PRInt32 aWidth, PRInt32 aHeight,
                                      nsIScreen **aOutScreen )
{
  nsresult rv;
  rv = EnsureInit();
  if (NS_FAILED(rv)) {
    NS_ERROR("nsScreenManagerGtk::EnsureInit() failed from ScreenForRect\n");
    return rv;
  }
  
  PRUint32 which = 0;
  
  
  
  if (mCachedScreenArray.Count() > 1) {
    
    
    PRUint32 area = 0;
    nsIntRect windowRect(aX, aY, aWidth, aHeight);
    for (PRInt32 i = 0, i_end = mCachedScreenArray.Count(); i < i_end; ++i) {
      PRInt32  x, y, width, height;
      x = y = width = height = 0;
      mCachedScreenArray[i]->GetRect(&x, &y, &width, &height);
      
      nsIntRect screenRect(x, y, width, height);
      screenRect.IntersectRect(screenRect, windowRect);
      PRUint32 tempArea = screenRect.width * screenRect.height;
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
    NS_ERROR("nsScreenManagerGtk::EnsureInit() failed from GetPrimaryScreen\n");
    return rv;
  }
  *aPrimaryScreen = mCachedScreenArray.SafeObjectAt(0);
  NS_IF_ADDREF(*aPrimaryScreen);
  return NS_OK;
  
} 







NS_IMETHODIMP
nsScreenManagerGtk :: GetNumberOfScreens(PRUint32 *aNumberOfScreens)
{
  nsresult rv;
  rv = EnsureInit();
  if (NS_FAILED(rv)) {
    NS_ERROR("nsScreenManagerGtk::EnsureInit() failed from GetNumberOfScreens\n");
    return rv;
  }
  *aNumberOfScreens = mCachedScreenArray.Count();
  return NS_OK;
  
} 

NS_IMETHODIMP
nsScreenManagerGtk :: ScreenForNativeWidget (void *aWidget, nsIScreen **outScreen)
{
  nsresult rv;
  rv = EnsureInit();
  if (NS_FAILED(rv)) {
    NS_ERROR("nsScreenManagerGtk::EnsureInit() failed from ScreenForNativeWidget\n");
    return rv;
  }

  if (mCachedScreenArray.Count() > 1) {
    
    
    gint x, y, width, height, depth;
    x = y = width = height = 0;

    gdk_window_get_geometry(GDK_WINDOW(aWidget), &x, &y, &width, &height,
                            &depth);
    gdk_window_get_origin(GDK_WINDOW(aWidget), &x, &y);
    rv = ScreenForRect(x, y, width, height, outScreen);
  } else {
    rv = GetPrimaryScreen(outScreen);
  }

  return rv;
}
