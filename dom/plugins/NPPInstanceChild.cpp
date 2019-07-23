





































#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdk.h>

#include "gtk2xtbin.h"

#include "NPPInstanceChild.h"

namespace mozilla {
namespace plugins {


static const char*
NPNVariableToString(NPNVariable aVar)
{
#define VARSTR(v_)  case v_: return #v_

    switch(aVar) {
        VARSTR(NPNVxDisplay);
        VARSTR(NPNVxtAppContext);
        VARSTR(NPNVnetscapeWindow);
        VARSTR(NPNVjavascriptEnabledBool);
        VARSTR(NPNVasdEnabledBool);
        VARSTR(NPNVisOfflineBool);

        VARSTR(NPNVserviceManager);
        VARSTR(NPNVDOMElement);
        VARSTR(NPNVDOMWindow);
        VARSTR(NPNVToolkit);
        VARSTR(NPNVSupportsXEmbedBool);

        VARSTR(NPNVWindowNPObject);

        VARSTR(NPNVPluginElementNPObject);

        VARSTR(NPNVSupportsWindowless);

        VARSTR(NPNVprivateModeBool);

    default: return "???";
    }
#undef VARSTR
}

NPError
NPPInstanceChild::NPN_GetValue(NPNVariable aVar, void* aValue)
{
    printf ("[NPPInstanceChild] NPN_GetValue(%s)\n",
            NPNVariableToString(aVar));

#ifdef OS_LINUX
    

    switch(aVar) {
    case NPNVSupportsXEmbedBool:
        *((PRBool*)aValue) = PR_TRUE;
        return NPERR_NO_ERROR;

    case NPNVToolkit:
        *((NPNToolkitType*)aValue) = NPNVGtk2;
        return NPERR_NO_ERROR;

    case NPNVSupportsWindowless:
        
        
        *((PRBool*)aValue) = PR_FALSE;
        return NPERR_NO_ERROR;

    default:
        printf("  unhandled var %s\n", NPNVariableToString(aVar));
        return NPERR_GENERIC_ERROR;   
    }
#else
#  error Add support for your OS
#endif

}

NPError
NPPInstanceChild::NPP_SetWindow(XID aWindow, int32_t aWidth, int32_t aHeight)
{
    printf("[NPPInstanceChild] NPP_SetWindow(%lx, %d, %d)\n",
           aWindow, aWidth, aHeight);

#ifdef OS_LINUX
    
    
    

    GdkNativeWindow handle = (GdkNativeWindow) aWindow;
    mPlug = gtk_plug_new(handle);
    
    mWindow.window = (void*) handle;
    mWindow.width = aWidth;
    mWindow.height = aHeight;
    mWindow.type = NPWindowTypeWindow;

    GdkWindow* gdkWindow = gdk_window_lookup(handle);

    fprintf (stderr, "GDK_WINDOW\n");
    mWsInfo.display = GDK_WINDOW_XDISPLAY(gdkWindow);
    fprintf (stderr, "GDK_COLORMAP\n");
    mWsInfo.colormap = GDK_COLORMAP_XCOLORMAP(gdk_drawable_get_colormap(gdkWindow));
    fprintf (stderr, "gdk_get_visual\n");
    GdkVisual* gdkVisual = gdk_drawable_get_visual(gdkWindow);
    fprintf (stderr, "GDK_VISUAL_XVISUAL\n");
    mWsInfo.visual = GDK_VISUAL_XVISUAL(gdkVisual);
    fprintf (stderr, "gdkVisual->depth\n");
    mWsInfo.depth = gdkVisual->depth;

    mWindow.ws_info = (void*) &mWsInfo;

    return mPluginIface->setwindow(&mData, &mWindow);

#else
#  error Implement me for your OS
#endif
}


} 
} 
