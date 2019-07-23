









































#include "drawers.h"

#ifdef HAVE_XIE

#include <stdio.h>
#include <string.h>
#include "prenv.h"
#include "xlibrgb.h"
#include <X11/extensions/XIElib.h>


static PRBool useXIE = PR_TRUE;
static PRBool inited = PR_FALSE;
static XiePhotospace gPhotospace;
static XiePhotoElement *photoElement;

static void
DoFlo(Display *display,
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
      PRInt32 aDHeight)
{
  XieExtensionInfo *info;
  float coeffs[6];
  XieConstant constant;
  XiePhototag idx = 0, src;

  


  
  XieFloImportDrawable(&photoElement[idx], aSrc,
                       aSX, aSY, aSWidth, aSHeight,
                       0, PR_FALSE);
  ++idx;
  src = idx;

  
  coeffs[0] = (float)aSrcWidth / (float)aDWidth;
  coeffs[1] = 0.0;
  coeffs[2] = 0.0;
  coeffs[3] = (float)aSrcHeight / (float)aDHeight;
  coeffs[4] = 0.0;
  coeffs[5] = 0.0;

  constant[0] = 128.0;
  constant[1] = 128.0;
  constant[2] = 128.0;

  XieFloGeometry(&photoElement[idx], src, aDWidth, aDHeight,
                 coeffs,
                 constant,
                 0x07,
                 xieValGeomNearestNeighbor,
                 NULL);
  ++idx;

  
  XieFloExportDrawable(&photoElement[idx], idx, aDest, aGC,
                       (aDX - aSX),
                       (aDY - aSY));
#ifdef DEBUG_XIE
  printf("export to %d, %d (%dx%d)\n", (aDX - aSX), (aDY - aSY),
      aDWidth, aDHeight);
#endif
  ++idx;

  
  XieExecuteImmediate(display, gPhotospace, 1, PR_FALSE, photoElement, idx);

  


}

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
                   PRInt32 aDHeight)
{
#ifdef DEBUG_XIE
  printf("DrawScaledImageXIE\n");
#endif

  if (!useXIE) {
#ifdef DEBUG_XIE
    fprintf(stderr, "useXIE is false.\n");
#endif
    return PR_FALSE;
  }

  if (!inited) {
    XieExtensionInfo *info;

    if (useXIE) {
      char *text = PR_GetEnv("MOZ_DISABLE_XIE");
      if (text) {
#ifdef DEBUG_XIE
        fprintf(stderr, "MOZ_DISABLE_XIE set, disabling use of XIE.\n");
#endif
        useXIE = PR_FALSE;
        return PR_FALSE;
      }
    }

    if (!XieInitialize(display, &info)) {
      useXIE = PR_FALSE;
      return PR_FALSE;
    }

    inited = PR_TRUE;

    
    gPhotospace = XieCreatePhotospace(display);

    photoElement = XieAllocatePhotofloGraph(3);

    


  }

  
  DoFlo(display, aDest, aGC, aSrc,
        aSrcWidth, aSrcHeight,
        aSX, aSY, aSWidth, aSHeight,
        aDX, aDY, aDWidth, aDHeight);

  return PR_TRUE;
}
#endif
