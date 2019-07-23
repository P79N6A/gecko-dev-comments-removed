







































#include "prtypes.h"
#include <X11/Xlib.h>

PR_BEGIN_EXTERN_C

#ifdef HAVE_XIE
PRBool 
DrawScaledImageXIE(Display *display,
                   Drawable aDest,
                   GC aGC,
                   Drawable aSrc,
                   PRInt32 aSrcWidth,
                   PRInt32 aSrcHeight,
                   PRInt32 aSX,
                   PRInt32 aSY,
                   PRInt32 aSWidth,
                   PRInt32 aSHeight,
                   PRInt32 aDX,
                   PRInt32 aDY,
                   PRInt32 aDWidth,
                   PRInt32 aDHeight);
#endif

PR_END_EXTERN_C
