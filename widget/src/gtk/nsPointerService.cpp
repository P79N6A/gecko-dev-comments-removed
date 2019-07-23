



































#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include "nsPointerService.h"
#include "nsWindow.h"

NS_IMPL_ADDREF_INHERITED(nsPointerService, nsBasePointerService)
NS_IMPL_RELEASE_INHERITED(nsPointerService, nsBasePointerService)
NS_IMPL_QUERY_INTERFACE_INHERITED0(nsPointerService, nsBasePointerService)


NS_IMETHODIMP
nsPointerService::WidgetUnderPointer(nsIWidget **_retval,
				     PRUint32 *aXOffset, PRUint32 *aYOffset)
{
  *_retval = nsnull;
  *aXOffset = 0;
  *aYOffset = 0;

  Bool retval;
  Window root_return;
  Window child_return = None;
  int root_x_return, root_y_return;
  int win_x_return, win_y_return;
  unsigned int mask_return;

  
  
  
  
  
  XSync(GDK_DISPLAY(), False);

  
  retval = XQueryPointer(GDK_DISPLAY(),
			 GDK_WINDOW_XWINDOW(GDK_ROOT_PARENT()),
			 &root_return, &child_return,
			 &root_x_return, &root_y_return,
			 &win_x_return, &win_y_return,
			 &mask_return);

  
  if (!retval || child_return == None)
    return NS_OK;

  int done = 0;
  Window dest_w = child_return;
  int    xlate_x_return;
  int    xlate_y_return;

  
  while (!done) {
    Window xlate_return = None;
    retval = XTranslateCoordinates(GDK_DISPLAY(),
				   GDK_WINDOW_XWINDOW(GDK_ROOT_PARENT()),
				   dest_w,
				   win_x_return, win_y_return,
				   &xlate_x_return, &xlate_y_return,
				   &xlate_return);
    
    
    if (!retval)
      return NS_OK;

    
    if (xlate_return == None)
      done = 1;
    
    
    else
      dest_w = xlate_return;
  }

  GdkWindow *window;
  nsWindow  *widget;

  
  window = gdk_window_lookup(dest_w);
  
  if (!window)
    return NS_OK;
  
  
  gpointer data = NULL;
  gdk_window_get_user_data(window, &data);
  
  if (!data)
    return NS_OK;

  
  widget = (nsWindow *)gtk_object_get_data(GTK_OBJECT(data), "nsWindow");
  if (!widget)
    return NS_OK;

  *_retval = NS_STATIC_CAST(nsIWidget *, widget);
  *aXOffset = xlate_x_return;
  *aYOffset = xlate_y_return;

  NS_ADDREF(*_retval);

  return NS_OK;
}
