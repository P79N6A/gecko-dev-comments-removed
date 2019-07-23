





































#ifndef nsX11AlphaBlend_h__
#define nsX11AlphaBlend_h__

#include <X11/Xlib.h>
#include "nsColor.h"

class nsAntiAliasedGlyph;

#ifdef DEBUG
#ifndef DEBUG_SHOW_GLYPH_BOX
# define DEBUG_SHOW_GLYPH_BOX 0
#endif
void AADrawBox(XImage *, PRInt32, PRInt32, PRInt32, PRInt32, nscolor, PRUint8);
#if DEBUG_SHOW_GLYPH_BOX
# define DEBUG_AADRAWBOX(i,x,y,w,h,r,g,b,a) \
    PR_BEGIN_MACRO \
      nscolor color NS_RGB((r),(g),(b)); \
      AADrawBox((i), (x), (y), (w), (h), color, (a)); \
    PR_END_MACRO
#else
# define DEBUG_AADRAWBOX(i,x,y,w,h,r,g,b,a)
#endif
#endif

void     nsX11AlphaBlendFreeGlobals(void);
nsresult nsX11AlphaBlendInitGlobals(Display *dsp);


typedef void    (*blendGlyph)(XImage *, nsAntiAliasedGlyph *, PRUint8*,
                              nscolor, int, int);
typedef void    (*blendPixel)(XImage *, int, int, nscolor, int);
typedef nscolor (*pixelToNSColor)(unsigned long aPixel);






class nsX11AlphaBlend {
public:
  inline static PRBool     CanAntiAlias()      { return sAvailable; };
  inline static blendPixel GetBlendPixel()     { return sBlendPixel; };
  inline static blendGlyph GetBlendGlyph()     { return sBlendMonoImage; };

  static XImage*  GetXImage(PRUint32 width, PRUint32 height);
  static void     FreeGlobals();
  static nsresult InitGlobals(Display *dsp);
  static XImage*  GetBackground(Display *, int, Drawable,
                                PRInt32, PRInt32, PRUint32, PRUint32);
  static nscolor  PixelToNSColor(unsigned long aPixel);

protected:
  static void ClearGlobals();
  static void ClearFunctions();
  static PRBool InitLibrary(Display *dsp);

  static PRBool         sAvailable;
  static PRUint16       sBitmapPad;
  static PRUint16       sBitsPerPixel;
  static blendGlyph     sBlendMonoImage;
  static blendPixel     sBlendPixel;
  static PRUint16       sBytesPerPixel;
  static int            sDepth;
  static PRBool         sInited;
  static pixelToNSColor sPixelToNSColor;
};

#endif 
