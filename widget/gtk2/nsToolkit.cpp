






































#include "nscore.h"  
#include "nsGTKToolkit.h"

nsGTKToolkit* nsGTKToolkit::gToolkit = nsnull;






nsGTKToolkit::nsGTKToolkit()
  : mSharedGC(nsnull), mFocusTimestamp(0)
{
    CreateSharedGC();
}






nsGTKToolkit::~nsGTKToolkit()
{
    if (mSharedGC) {
        g_object_unref(mSharedGC);
    }
}

void nsGTKToolkit::CreateSharedGC(void)
{
    GdkPixmap *pixmap;

    if (mSharedGC)
        return;

    pixmap = gdk_pixmap_new(NULL, 1, 1, gdk_rgb_get_visual()->depth);
    mSharedGC = gdk_gc_new(pixmap);
    g_object_unref(pixmap);
}

GdkGC *nsGTKToolkit::GetSharedGC(void)
{
    return (GdkGC *)g_object_ref(mSharedGC);
}





nsGTKToolkit* nsGTKToolkit::GetToolkit()
{
    if (!gToolkit) {
        gToolkit = new nsGTKToolkit();
    }
 
    return gToolkit;
}
