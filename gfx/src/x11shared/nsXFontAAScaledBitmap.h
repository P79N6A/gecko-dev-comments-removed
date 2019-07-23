






































#ifndef nsXFontAAScaledBitmap_h__
#define nsXFontAAScaledBitmap_h__

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include "nspr.h"
#include "nsXFont.h"
#include "nsAntiAliasedGlyph.h"

extern PRUint8 gAASBDarkTextMinValue;
extern double  gAASBDarkTextGain;
extern PRUint8 gAASBLightTextMinValue;
extern double  gAASBLightTextGain;


#define SCALED_SIZE(x) (PRInt32)(rint(((double)(x))*mRatio))
class nsHashtable;

class nsXFontAAScaledBitmap : public nsXFont {
public:
  
  
  
  
  
  
  nsXFontAAScaledBitmap(Display *aDisplay, int aScreen, GdkFont *,
                        PRUint16, PRUint16);
  ~nsXFontAAScaledBitmap();

  void         DrawText8(GdkDrawable *Drawable, GdkGC *GC, PRInt32, PRInt32,
                         const char *, PRUint32);
  void         DrawText16(GdkDrawable *Drawable, GdkGC *GC, PRInt32, PRInt32,
                          const XChar2b *, PRUint32);
  PRBool       GetXFontProperty(Atom, unsigned long *);
  XFontStruct *GetXFontStruct();
  PRBool       LoadFont();
  void         TextExtents8(const char *, PRUint32, PRInt32*, PRInt32*,
                            PRInt32*, PRInt32*, PRInt32*);
  void         TextExtents16(const XChar2b *, PRUint32, PRInt32*, PRInt32*,
                             PRInt32*, PRInt32*, PRInt32*);
  PRInt32      TextWidth8(const char *, PRUint32);
  PRInt32      TextWidth16(const XChar2b *, PRUint32);
  void         UnloadFont();

public:
  static PRBool InitGlobals(Display *aDisplay, int aScreen);
  static void   FreeGlobals();

protected:
  void         DrawText8or16(GdkDrawable *Drawable, GdkGC *GC, PRInt32,
                             PRInt32, void *, PRUint32);
  void         TextExtents8or16(void *, PRUint32, PRInt32*, PRInt32*,
                             PRInt32*, PRInt32*, PRInt32*);
  PRBool GetScaledGreyImage(const char *, nsAntiAliasedGlyph **);
#ifdef DEBUG
  void dump_XImage_blue_data(XImage *ximage);
#endif
  static PRBool DisplayIsLocal(Display *);

protected:
  PRBool       mAlreadyLoaded;
  Display     *mDisplay;
  GC           mForegroundGC;
  GdkFont     *mGdkFont;
  nsHashtable* mGlyphHash;
  double       mRatio;
  XFontStruct  mScaledFontInfo;
  GlyphMetrics mScaledMax;
  int          mScreen;
  Pixmap       mUnscaledBitmap;
  XFontStruct *mUnscaledFontInfo;
  GlyphMetrics mUnscaledMax;
  PRUint16     mUnscaledSize;


protected:
  static Display *sDisplay;
  static GC       sBackgroundGC; 
                                 
  static PRUint8  sWeightedScaleDarkText[256];
  static PRUint8  sWeightedScaleLightText[256];
};

#endif 
