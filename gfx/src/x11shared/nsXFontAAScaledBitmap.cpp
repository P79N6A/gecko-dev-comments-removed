






































#include <X11/Xatom.h>
#include "gfx-config.h"
#include "nscore.h"
#include "nsXFontAAScaledBitmap.h"
#include "nsRenderingContextGTK.h"
#include "nsX11AlphaBlend.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include "nsHashtable.h"

#define IMAGE_BUFFER_SIZE 2048

#ifdef DEBUG
  static PRBool dodump = 0;
# define DEBUG_DUMP(x) if (dodump) {dodump=0; x; }
#else
# define DEBUG_DUMP(x)
#endif

#define DEBUG_SHOW_GLYPH_BOX 0
#if DEBUG_SHOW_GLYPH_BOX
# define DEBUG_AADRAWBOX(i,x,y,w,h,r,g,b,a) \
    PR_BEGIN_MACRO \
      nscolor color NS_RGB((r),(g),(b)); \
      AADrawBox((i), (x), (y), (w), (h), color, (a)); \
    PR_END_MACRO
#else
# define DEBUG_AADRAWBOX(i,x,y,w,h,r,g,b,a)
#endif

void dump_byte_table(PRUint8 *table, int width, int height);

static void scale_image(nsAntiAliasedGlyph *, nsAntiAliasedGlyph *);
static void scale_imageAntiJag(nsAntiAliasedGlyph *, nsAntiAliasedGlyph *);
static void WeightTableInitLinearCorrection(PRUint8*, PRUint8, double);

Display *nsXFontAAScaledBitmap::sDisplay       = nsnull;
GC       nsXFontAAScaledBitmap::sBackgroundGC  = nsnull;
PRUint8  nsXFontAAScaledBitmap::sWeightedScaleDarkText[256];
PRUint8  nsXFontAAScaledBitmap::sWeightedScaleLightText[256];


































PRUint8 gAASBDarkTextMinValue = 64;
double gAASBDarkTextGain = 0.6;
PRUint8 gAASBLightTextMinValue = 64;
double gAASBLightTextGain = 1.3;

PRBool
nsXFontAAScaledBitmap::DisplayIsLocal(Display *aDisplay)
{
  
  if (gdk_get_use_xshm())
    return PR_TRUE;

  return PR_FALSE;

}

void
nsXFontAAScaledBitmap::DrawText8(GdkDrawable *aDrawable, GdkGC *aGC,
                                 PRInt32 aX, PRInt32 aY,
                                 const char *aString, PRUint32 aLength)
{
  DrawText8or16(aDrawable, aGC, aX, aY, (void*)aString, aLength);
}

void
nsXFontAAScaledBitmap::DrawText16(GdkDrawable *aDrawable, GdkGC *aGC,
                         PRInt32 aX, PRInt32 aY,
                        const XChar2b *aString, PRUint32 aLength)
{
  DrawText8or16(aDrawable, aGC, aX, aY, (void*)aString, aLength);
}







void
nsXFontAAScaledBitmap::DrawText8or16(GdkDrawable *aDrawable, GdkGC *aGC,
                                     PRInt32 aX, PRInt32 aY,
                                     void *a8or16String, PRUint32 aLength)
{
  
  const char    *string8  = (const char    *)a8or16String;
  const XChar2b *string16 = (const XChar2b *)a8or16String;

#if DEBUG_SHOW_GLYPH_BOX
  
  
  
#endif

  if (aLength < 1) {
    return;
  }

  
  
  
  
  
  
  
  
  
  PRUint32 image_width  = (mScaledMax.width * aLength) + mScaledMax.lbearing;
  PRUint32 image_height = mScaledMax.ascent+mScaledMax.descent;
  PRInt32 x_pos = mScaledMax.lbearing;

  Drawable win = GDK_WINDOW_XWINDOW(aDrawable);
  GC gc = GDK_GC_XGC(aGC);
  XGCValues values;
  if (!XGetGCValues(mDisplay, gc, GCForeground, &values)) {
    NS_ASSERTION(0, "failed to get foreground pixel");
    return;
  }
  nscolor color = nsX11AlphaBlend::PixelToNSColor(values.foreground);

  
  PRUint32 color_val = NS_GET_R(color) + NS_GET_G(color) + NS_GET_B(color);
  PRUint8 *weight_table;
  if ((NS_GET_R(color)>200) || (NS_GET_R(color)>200) || (NS_GET_R(color)>200)
      || (color_val>3*128))
    weight_table = sWeightedScaleLightText;
  else
    weight_table = sWeightedScaleDarkText;

  
  
  
  XImage *sub_image = nsX11AlphaBlend::GetBackground(mDisplay, mScreen, win,
                                 aX-mScaledMax.lbearing, aY-mScaledMax.ascent,
                                 image_width, image_height);
  if (sub_image==nsnull) {
#ifdef DEBUG
    
    int win_width = DisplayWidth(mDisplay, mScreen);
    int win_height = DisplayHeight(mDisplay, mScreen);
    if (((int)(aX-mScaledMax.lbearing+image_width) > 0)  
        && ((int)(aX-mScaledMax.lbearing) < win_width)   
        && ((int)(aY-mScaledMax.ascent+image_height) > 0)
        && ((int)(aY-mScaledMax.ascent) < win_height))   
    {
      NS_ASSERTION(sub_image, "failed to get the image");
    }
#endif
    return;
  }

#if DEBUG_SHOW_GLYPH_BOX
  DEBUG_AADRAWBOX(sub_image,0,0,image_width,image_height,0,0,0,255/4);
  int lbearing, rbearing, width, ascent, descent;
  if (mIsSingleByte)
    TextExtents8(aString8, aLength, &lbearing, &rbearing, &width,
                 &ascent, &descent);
  else
    TextExtents16(aString16, aLength, &lbearing, &rbearing, &width,
                  &ascent, &descent);

  DEBUG_AADRAWBOX(sub_image, x_pos+lbearing, mScaledMax.ascent-ascent,
                  rbearing-lbearing, ascent+descent, 0,255,0, 255/2);
#endif

  
  
  
  blendGlyph blendGlyph = nsX11AlphaBlend::GetBlendGlyph();
  for (PRUint32 i=0; i<aLength; i++) {
#if DEBUG_SHOW_GLYPH_BOX
    PRUint32 box_width;
    if (mIsSingleByte)
      box_width = TextWidth8(&aString8[i], 1);
    else
      box_width = TextWidth16(&aString16[i], 1);
    nscolor red = NS_RGB(255,0,0);
    AADrawBox(sub_image, x_pos, 0, box_width, mScaledMax.ascent, red, 255/4);
    AADrawBox(sub_image, x_pos, mScaledMax.ascent, box_width,
              mScaledMax.descent, red,255/4);
#endif
    nsAntiAliasedGlyph *scaled_glyph;
    PRBool got_image;
    if (mIsSingleByte)
      got_image = GetScaledGreyImage(&string8[i], &scaled_glyph);
    else
      got_image = GetScaledGreyImage((const char*)&string16[i], &scaled_glyph);
    if (!got_image) {
      PRUint32 char_width;
      if (mIsSingleByte)
        char_width = XTextWidth(mUnscaledFontInfo, &string8[i], 1);
      else
        char_width = XTextWidth16(mUnscaledFontInfo, &string16[i], 1);
      x_pos += SCALED_SIZE(char_width);
      continue;
    }
    NS_ASSERTION(scaled_glyph->GetBorder()==0,
                 "do not support non-zero border");
    DEBUG_DUMP((dump_byte_table(scaled_glyph->GetBuffer(),
                    scaled_glyph->GetBufferWidth(),
                    scaled_glyph->GetBufferHeight())));
    
    
    
    (*blendGlyph)(sub_image, scaled_glyph, weight_table, color,
                  x_pos + scaled_glyph->GetLBearing(), 0);

    DEBUG_DUMP((dump_XImage_blue_data(sub_image)));
    x_pos += scaled_glyph->GetAdvance();
  }
  DEBUG_DUMP((dump_XImage_blue_data(sub_image)));

  
  
  
  XPutImage(mDisplay, win, gc, sub_image,
            0, 0, aX-mScaledMax.lbearing, aY-mScaledMax.ascent,
            image_width, image_height);
  XDestroyImage(sub_image);
}

void
nsXFontAAScaledBitmap::FreeGlobals()
{
  if (sBackgroundGC) {
    XFreeGC(sDisplay, sBackgroundGC);
    sBackgroundGC = nsnull;
  }
  sDisplay = nsnull;
}

static PRBool
FreeGlyphHash(nsHashKey* aKey, void* aData, void* aClosure)
{
  delete (nsAntiAliasedGlyph *)aData;

  return PR_TRUE;
}

PRBool
nsXFontAAScaledBitmap::GetScaledGreyImage(const char *aChar,
                                         nsAntiAliasedGlyph **aGreyImage)
{
  XChar2b *aChar2b = nsnull;
  PRUint32 antiJagPadding;
  XImage *ximage;
  nsAntiAliasedGlyph *scaled_image;
  PRUnichar charKey[2];

  
  if (mIsSingleByte)
    charKey[0] = (PRUnichar)*aChar;
  else {
    aChar2b = (XChar2b *)aChar;
    charKey[0] = aChar2b->byte1<<8 | aChar2b->byte2;
  }
  charKey[1] = 0;
  nsStringKey key(charKey, 1);

  
  scaled_image = (nsAntiAliasedGlyph*)mGlyphHash->Get(&key);
  if (!scaled_image) {
    
    int direction, font_ascent, font_descent;
    XCharStruct charMetrics;
    if (mIsSingleByte)
      XTextExtents(mUnscaledFontInfo, aChar, 1, &direction,
                   &font_ascent, &font_descent, &charMetrics);
    else
      XTextExtents16(mUnscaledFontInfo, aChar2b,1, &direction,
                     &font_ascent, &font_descent, &charMetrics);

    
    PRInt32 left_edge  = GLYPH_LEFT_EDGE(&charMetrics);
    PRInt32 right_edge = GLYPH_RIGHT_EDGE(&charMetrics);
    PRUint32 unscaled_width = right_edge - left_edge;
    NS_ASSERTION(unscaled_width<=mUnscaledMax.width, "unexpected glyph width");

    
    XFillRectangle(mDisplay, mUnscaledBitmap, sBackgroundGC, 0, 0,
                                unscaled_width, mUnscaledMax.height);
    
    if (mIsSingleByte)
      XDrawString(mDisplay, mUnscaledBitmap, mForegroundGC,
               -left_edge, mUnscaledMax.ascent, aChar, 1);
    else
      XDrawString16(mDisplay, mUnscaledBitmap, mForegroundGC,
               -left_edge, mUnscaledMax.ascent, aChar2b, 1);
    
    ximage = XGetImage(mDisplay, mUnscaledBitmap,
                     0, 0, unscaled_width, mUnscaledMax.height,
                     AllPlanes, ZPixmap);
    NS_ASSERTION(ximage, "failed to XGetSubImage()");
    if (!ximage) {
      return PR_FALSE;
    }
    DEBUG_DUMP((dump_XImage_blue_data(ximage)));

    
    if (mRatio < 1.25)
      antiJagPadding = 0;
    else
      antiJagPadding = 2; 

    
    nsAntiAliasedGlyph unscaled_image(unscaled_width, mUnscaledMax.height,
                                      antiJagPadding);
    PRUint8 buf[IMAGE_BUFFER_SIZE]; 
    if (!unscaled_image.Init(buf, IMAGE_BUFFER_SIZE)) {
      NS_ASSERTION(0, "failed to Init() unscaled_image");
      XDestroyImage(ximage);
      return PR_FALSE;
    }

    
    
    
    unscaled_image.SetImage(&charMetrics, ximage);
    DEBUG_DUMP((dump_byte_table(unscaled_image.GetBuffer(),
                    unscaled_image.GetBufferWidth(),
                    unscaled_image.GetBufferHeight())));
    XDestroyImage(ximage);

    
    
    
    GlyphMetrics glyphMetrics;
    glyphMetrics.width    = SCALED_SIZE(unscaled_width);
    glyphMetrics.height   = SCALED_SIZE(mUnscaledMax.height);
    glyphMetrics.lbearing = SCALED_SIZE(left_edge);
    glyphMetrics.rbearing = SCALED_SIZE(right_edge);
    glyphMetrics.advance  = SCALED_SIZE(charMetrics.width);
    glyphMetrics.ascent   = SCALED_SIZE(charMetrics.ascent);
    glyphMetrics.descent  = SCALED_SIZE(charMetrics.descent);

    scaled_image = new nsAntiAliasedGlyph(SCALED_SIZE(unscaled_width),
                                          SCALED_SIZE(mUnscaledMax.height), 0);
    NS_ASSERTION(scaled_image, "failed to create scaled_image");
    if (!scaled_image) {
      return PR_FALSE;
    }
    if (!scaled_image->Init()) {
      NS_ASSERTION(0, "failed to initialize scaled_image");
      delete scaled_image;
      return PR_FALSE;
    }
    scaled_image->SetSize(&glyphMetrics);

    
    
    
    if (antiJagPadding==0)
      scale_image(&unscaled_image, scaled_image);
    else
      scale_imageAntiJag(&unscaled_image, scaled_image);

    DEBUG_DUMP((dump_byte_table(scaled_image->GetBuffer(),
                    scaled_image->GetBufferWidth(),
                    scaled_image->GetBufferHeight())));

    
    mGlyphHash->Put(&key, scaled_image);
  }
  *aGreyImage = scaled_image;
  return PR_TRUE;
}

PRBool
nsXFontAAScaledBitmap::GetXFontProperty(Atom aAtom, unsigned long *aValue)
{
  unsigned long val;
  PRBool rslt = ::XGetFontProperty(mUnscaledFontInfo, aAtom, &val);
  if (!rslt)
    return PR_FALSE;

  switch (aAtom) {
    case XA_X_HEIGHT:
      if (val >= 0x00ffffff) {
        return PR_FALSE;
      }
    case XA_SUBSCRIPT_Y:
    case XA_SUPERSCRIPT_Y:
    case XA_UNDERLINE_POSITION:
    case XA_UNDERLINE_THICKNESS:
      *aValue = (unsigned long)SCALED_SIZE(val);
      break;
    default:
      *aValue = val;
  }
  return rslt;
}

XFontStruct *
nsXFontAAScaledBitmap::GetXFontStruct()
{
  NS_ASSERTION(mGdkFont, "GetXFontStruct called before font loaded");
  if (mGdkFont==nsnull)
    return nsnull;

  return &mScaledFontInfo;
}

PRBool
nsXFontAAScaledBitmap::InitGlobals(Display *aDisplay, int aScreen)
{
  Window root_win;

  sDisplay = aDisplay; 

  
  if (!DisplayIsLocal(aDisplay)) {
    goto cleanup_and_return;
  }

  root_win = RootWindow(sDisplay, aScreen);
  sBackgroundGC = XCreateGC(sDisplay, root_win, 0, NULL);
  NS_ASSERTION(sBackgroundGC, "failed to create sBackgroundGC");
  if (!sBackgroundGC) {
    goto cleanup_and_return;
  }
  XSetForeground(sDisplay, sBackgroundGC, 0);

  WeightTableInitLinearCorrection(sWeightedScaleDarkText,
                                  gAASBDarkTextMinValue, gAASBDarkTextGain);
  WeightTableInitLinearCorrection(sWeightedScaleLightText,
                                  gAASBLightTextMinValue, gAASBLightTextGain);
  return PR_TRUE;

cleanup_and_return:
  if (sBackgroundGC) {
    XFreeGC(sDisplay, sBackgroundGC);
    sBackgroundGC = nsnull;
  }

  return PR_FALSE;
}

PRBool
nsXFontAAScaledBitmap::LoadFont()
{
  NS_ASSERTION(!mAlreadyLoaded, "LoadFont called more than once");
  mAlreadyLoaded = PR_TRUE;

  if (!mGdkFont)
    return PR_FALSE;
  mUnscaledFontInfo = (XFontStruct *)GDK_FONT_XFONT(mGdkFont);

  XFontStruct *usfi = mUnscaledFontInfo;
  XFontStruct *sfi  = &mScaledFontInfo;

  mIsSingleByte = (usfi->min_byte1 == 0) && (usfi->max_byte1 == 0);

  mUnscaledMax.width    = MAX(usfi->max_bounds.rbearing,usfi->max_bounds.width);
  mUnscaledMax.width   -= MIN(usfi->min_bounds.lbearing, 0);
  mUnscaledMax.height   = usfi->max_bounds.ascent   + usfi->max_bounds.descent;
  mUnscaledMax.lbearing = usfi->max_bounds.lbearing;
  mUnscaledMax.rbearing = usfi->max_bounds.rbearing;
  mUnscaledMax.advance  = usfi->max_bounds.width;
  mUnscaledMax.ascent   = usfi->max_bounds.ascent;
  mUnscaledMax.descent  = usfi->max_bounds.descent;

  mScaledMax.width    = SCALED_SIZE(mUnscaledMax.width);
  mScaledMax.lbearing = SCALED_SIZE(mUnscaledMax.lbearing);
  mScaledMax.rbearing = SCALED_SIZE(mUnscaledMax.rbearing);
  mScaledMax.advance  = SCALED_SIZE(mUnscaledMax.width);
  mScaledMax.ascent   = SCALED_SIZE(mUnscaledMax.ascent);
  mScaledMax.descent  = SCALED_SIZE(mUnscaledMax.ascent + mUnscaledMax.descent)
                        - SCALED_SIZE(mUnscaledMax.ascent);
  mScaledMax.height   = mScaledMax.ascent + mScaledMax.descent;

  
  
  
  sfi->fid               = 0;
  sfi->direction         = usfi->direction;
  sfi->min_char_or_byte2 = usfi->min_char_or_byte2;
  sfi->max_char_or_byte2 = usfi->max_char_or_byte2;
  sfi->min_byte1         = usfi->min_byte1;
  sfi->max_byte1         = usfi->max_byte1;
  sfi->all_chars_exist   = usfi->all_chars_exist;
  sfi->default_char      = usfi->default_char;
  sfi->n_properties      = 0;
  sfi->properties        = nsnull;
  sfi->ext_data          = nsnull;

  sfi->min_bounds.lbearing = SCALED_SIZE(usfi->min_bounds.lbearing);
  sfi->min_bounds.rbearing = SCALED_SIZE(usfi->min_bounds.rbearing);
  sfi->min_bounds.width    = SCALED_SIZE(usfi->min_bounds.width);
  sfi->min_bounds.ascent   = SCALED_SIZE(usfi->min_bounds.ascent);
  sfi->min_bounds.descent  =
            SCALED_SIZE(usfi->min_bounds.ascent + usfi->min_bounds.descent)
            - SCALED_SIZE(usfi->min_bounds.ascent);
  sfi->min_bounds.attributes = usfi->min_bounds.attributes;

  sfi->max_bounds.lbearing = SCALED_SIZE(usfi->max_bounds.lbearing);
  sfi->max_bounds.rbearing = SCALED_SIZE(usfi->max_bounds.rbearing);
  sfi->max_bounds.width    = SCALED_SIZE(usfi->max_bounds.width);
  sfi->max_bounds.ascent   = SCALED_SIZE(usfi->max_bounds.ascent);
  sfi->max_bounds.descent  =
            SCALED_SIZE(usfi->max_bounds.ascent + usfi->max_bounds.descent)
            - SCALED_SIZE(usfi->max_bounds.ascent);
  sfi->max_bounds.attributes = usfi->max_bounds.attributes;

  sfi->per_char = nsnull;
  sfi->ascent   = SCALED_SIZE(usfi->ascent);
  sfi->descent  = SCALED_SIZE(usfi->descent);

  
  
  
  
  mForegroundGC = XCreateGC(mDisplay, RootWindow(mDisplay, mScreen), 0, NULL);
  NS_ASSERTION(mForegroundGC, "failed to create mForegroundGC");
  if (!mForegroundGC) {
    goto cleanup_and_return;
  }

  XSetFont(mDisplay, mForegroundGC, usfi->fid);
  XSetForeground(mDisplay, mForegroundGC, 0xFFFFFF);

  mUnscaledBitmap = XCreatePixmap(mDisplay,
                          RootWindow(mDisplay,DefaultScreen(mDisplay)),
                          mUnscaledMax.width, mUnscaledMax.height,
                          DefaultDepth(mDisplay, mScreen));
  if (!mUnscaledBitmap)
    goto cleanup_and_return;

  mGlyphHash = new nsHashtable();
  if (!mGlyphHash)
    goto cleanup_and_return;

  if (mGdkFont) {
#ifdef NS_FONT_DEBUG_LOAD_FONT
    if (gFontDebug & NS_FONT_DEBUG_LOAD_FONT) {
      printf("loaded %s\n", mName);
    }
#endif
    return PR_TRUE;
  }
  else
    return PR_FALSE;

cleanup_and_return:
  if (mUnscaledFontInfo) {
    mUnscaledFontInfo = nsnull;
  }
  if (mForegroundGC) {
    XFreeGC(mDisplay, mForegroundGC);
    mForegroundGC = nsnull;
  }
  if (mUnscaledBitmap) {
    XFreePixmap(mDisplay, mUnscaledBitmap);
    mUnscaledBitmap = nsnull;
  }
  if (mGlyphHash) {
    delete mGlyphHash;
    mGlyphHash = nsnull;
  }
  memset(&mScaledFontInfo, 0, sizeof(mScaledFontInfo));
  memset(&mUnscaledMax,    0, sizeof(mUnscaledMax));
  memset(&mScaledMax,      0, sizeof(mScaledMax));
  return PR_FALSE;
}

nsXFontAAScaledBitmap::nsXFontAAScaledBitmap(Display *aDisplay,
                                             int aScreen,
                                             GdkFont *aGdkFont,
                                             PRUint16 aSize,
                                             PRUint16 aUnscaledSize)
{
  mAlreadyLoaded       = PR_FALSE;
  mDisplay             = aDisplay;
  mScreen              = aScreen;
  mGdkFont             = ::gdk_font_ref(aGdkFont);
  mUnscaledSize        = aUnscaledSize;
  mRatio               = ((double)aSize)/((double)aUnscaledSize);
  mIsSingleByte        = 0;
  mForegroundGC        = nsnull;
  mGlyphHash           = nsnull;
  mUnscaledBitmap      = nsnull;
  memset(&mScaledFontInfo, 0, sizeof(mScaledFontInfo));
  memset(&mUnscaledMax,    0, sizeof(mUnscaledMax));
  memset(&mScaledMax,      0, sizeof(mScaledMax));
}

void
nsXFontAAScaledBitmap::TextExtents8(const char *aString, PRUint32 aLength,
                                    PRInt32* aLBearing, PRInt32* aRBearing,
                                    PRInt32* aWidth, PRInt32* aAscent,
                                    PRInt32* aDescent)
{
  TextExtents8or16((void *)aString, aLength, aLBearing, aRBearing, aWidth,
                   aAscent, aDescent);
}

void
nsXFontAAScaledBitmap::TextExtents16(const XChar2b *aString, PRUint32 aLength,
                            PRInt32* aLBearing, PRInt32* aRBearing,
                            PRInt32* aWidth, PRInt32* aAscent,
                            PRInt32* aDescent)
{
  TextExtents8or16((void *)aString, aLength, aLBearing, aRBearing, aWidth,
                   aAscent, aDescent);
}







void
nsXFontAAScaledBitmap::TextExtents8or16(void *a8or16String, PRUint32 aLength,
                            PRInt32* aLBearing, PRInt32* aRBearing,
                            PRInt32* aWidth, PRInt32* aAscent,
                            PRInt32* aDescent)
{
  
  const char    *string8  = (const char    *)a8or16String;
  const XChar2b *string16 = (const XChar2b *)a8or16String;

  int dir, unscaled_ascent, unscaled_descent;
  XCharStruct char_metrics;
  int leftBearing  = 0;
  int rightBearing = 0;
  int width        = 0;
  int ascent       = 0;
  int descent      = 0;

  
  if (aLength >= 1) {
    if (mIsSingleByte)
      XTextExtents(mUnscaledFontInfo, string8++, 1,
                     &dir, &unscaled_ascent, &unscaled_descent, &char_metrics);
    else
      XTextExtents16(mUnscaledFontInfo, string16++, 1,
                     &dir, &unscaled_ascent, &unscaled_descent, &char_metrics);
    leftBearing  = SCALED_SIZE(char_metrics.lbearing);
    rightBearing = SCALED_SIZE(char_metrics.rbearing);
    ascent       = SCALED_SIZE(char_metrics.ascent);
    descent      = SCALED_SIZE(mUnscaledMax.ascent+char_metrics.descent)
                   - SCALED_SIZE(mUnscaledMax.ascent);
    width        = SCALED_SIZE(char_metrics.width);
  }

  
  
  
  
  
  
  for (PRUint32 i=1; i<aLength; i++) {
    if (mIsSingleByte)
      XTextExtents(mUnscaledFontInfo, string8++, 1,
                   &dir, &unscaled_ascent, &unscaled_descent, &char_metrics);
    else
      XTextExtents16(mUnscaledFontInfo, string16++, 1,
                   &dir, &unscaled_ascent, &unscaled_descent, &char_metrics);
    
    
    
    
    
    leftBearing  = MIN(leftBearing,  width+SCALED_SIZE(char_metrics.lbearing));
    rightBearing = MAX(rightBearing, width+SCALED_SIZE(char_metrics.rbearing));
    ascent       = MAX(ascent,  SCALED_SIZE(char_metrics.ascent));
    descent      = MAX(descent,
                       SCALED_SIZE(mUnscaledMax.ascent+char_metrics.descent)
                       - SCALED_SIZE(mUnscaledMax.ascent));
    width        += SCALED_SIZE(char_metrics.width);
  }
  *aLBearing     = leftBearing;
  *aRBearing     = rightBearing;
  *aWidth        = width;
  *aAscent       = ascent;
  *aDescent      = descent;
}

PRInt32
nsXFontAAScaledBitmap::TextWidth8(const char *aString, PRUint32 aLength)
{
  int width = 0;
  
  for (PRUint32 i=0; i<aLength; i++) {
    int unscaled_width = XTextWidth(mUnscaledFontInfo, aString+i, 1);
    width += SCALED_SIZE(unscaled_width);
  }

  return width;
}

PRInt32
nsXFontAAScaledBitmap::TextWidth16(const XChar2b *aString, PRUint32 aLength)
{
  int width = 0;
  
  for (PRUint32 i=0; i<aLength; i++) {
    int unscaled_width = XTextWidth16(mUnscaledFontInfo, aString+i, 1);
    width += SCALED_SIZE(unscaled_width);
  }
  return width;
}

void
nsXFontAAScaledBitmap::UnloadFont()
{
  NS_ASSERTION(mGdkFont, "UnloadFont called when font not loaded");
  delete this;
}

nsXFontAAScaledBitmap::~nsXFontAAScaledBitmap()
{
  if (mGlyphHash) {
    mGlyphHash->Reset(FreeGlyphHash, nsnull);
    delete mGlyphHash;
  }
  if (mForegroundGC) {
    XFreeGC(mDisplay, mForegroundGC);
  }
  if (mGdkFont) {
    ::gdk_font_unref(mGdkFont);
  }
  if (mUnscaledBitmap) {
    XFreePixmap(mDisplay, mUnscaledBitmap);
  }
}
































static void
scale_image(nsAntiAliasedGlyph *aSrc, nsAntiAliasedGlyph *aDst)
{
  PRUint32 x, y, col;
  PRUint8 buffer[65536];
  PRUint8 *horizontally_scaled_data = buffer;
  PRUint8 *pHsd, *pDst;
  PRUint32 dst_width = aDst->GetWidth();
  PRUint32 dst_buffer_width = aDst->GetBufferWidth();
  PRUint32 dst_height = aDst->GetHeight();
  PRUint8 *dst = aDst->GetBuffer();

  if (aDst->GetBorder() != 0) {
    NS_ASSERTION(aDst->GetBorder()!=0,"border not supported");
    return;
  }

  PRUint32 ratio;

  PRUint8 *src = aSrc->GetBuffer();
  PRUint32 src_width = aSrc->GetWidth();
  NS_ASSERTION(src_width,"zero width glyph");
  if (src_width==0)
    return;

  PRUint32 src_height = aSrc->GetHeight();
  NS_ASSERTION(src_height,"zero height glyph");
  if (src_height==0)
    return;

  
  
  

  
  
  
  ratio = (dst_width<<8)/src_width;

  PRUint32 hsd_len = dst_buffer_width * src_height;
  
  if (hsd_len > sizeof(buffer)) {
    horizontally_scaled_data = (PRUint8*)nsMemory::Alloc(hsd_len);
    memset(horizontally_scaled_data, 0, hsd_len);
  }
  for (y=0; y<(dst_buffer_width*src_height); y++)
    horizontally_scaled_data[y] = 0;

  pHsd = horizontally_scaled_data;
  for (y=0; y<src_height; y++,pHsd+=dst_buffer_width) {
    for (x=0; x<src_width; x++) {
      
      PRUint8 src_val = src[x + (y*src_width)];
      if (!src_val)
        continue;
      
      
      PRUint32 area_begin = x * ratio; 
      PRUint32 area_end   = (x+1) * ratio; 
      PRUint32 end_pixel  = (area_end+255)&0xffffff00; 
      
      
      for (col=(area_begin&0xffffff00); col<end_pixel; col+=256) {
        
        
        PRUint32 this_begin = MAX(col,area_begin);
        PRUint32 this_end   = MIN((col+256), area_end);
        
        pHsd[col>>8] += (PRUint8)(((this_end-this_begin)*src_val)>>8);
      }
      DEBUG_DUMP((dump_byte_table(horizontally_scaled_data,
                                  dst_width, src_height)));
    }
  }

  
  
  

  
  
  
  ratio = (dst_height<<8)/src_height;

  for (x=0; x<dst_width; x++) {
    pHsd = horizontally_scaled_data + x;
    pDst = dst + x;
    for (y=0; y<src_height; y++,pHsd+=dst_buffer_width) {
      
      PRUint8 src_val = *pHsd;
      if (src_val == 0)
        continue;
      
      
      PRUint32 area_begin = y * ratio; 
      PRUint32 area_end   = area_begin + ratio; 
      PRUint32 end_pixel  = (area_end+255)&0xffffff00; 
      PRUint32 c;
      PRUint32 col;
      
      
      for (c=(area_begin>>8)*dst_buffer_width,col=(area_begin&0xffffff00);
                col<end_pixel; c+=dst_buffer_width,col+=256) {
        
        
        PRUint32 this_begin = MAX(col,area_begin);
        PRUint32 this_end   = MIN((col+256), area_end);
        
        pDst[c] += (((this_end-this_begin)*src_val)>>8);
      }
      DEBUG_DUMP((dump_byte_table(dst, dst_width, dst_height)));
    }
  }
  if (horizontally_scaled_data != buffer)
    free(horizontally_scaled_data);
}









































static void
scale_imageAntiJag(nsAntiAliasedGlyph *aSrc, nsAntiAliasedGlyph *aDst)
{
  PRUint32 x, y, col;
  PRUint8 buffer[65536];
  PRUint8 *padded_src = aSrc->GetBuffer();
  PRUint8 exp_buffer[65536];
  PRUint8 *horizontally_scaled_data = buffer;
  PRUint8 *pHsd, *pDst;
  PRUint32 dst_width = aDst->GetWidth();
  PRUint32 dst_buffer_width = aDst->GetBufferWidth();
  PRUint32 dst_height = aDst->GetHeight();
  PRUint8 *dst = aDst->GetBuffer();

  if (aDst->GetBorder() != 0) {
    NS_ASSERTION(aDst->GetBorder()==0, "non zero dest border not supported");
    return;
  }
  PRUint32 expand = (((dst_width<<8)/aSrc->GetWidth())+255)>>8;

  PRUint32 src_width     = aSrc->GetWidth();
  PRUint32 src_height    = aSrc->GetHeight();
  PRUint32 border_width  = aSrc->GetBorder();
  PRUint32 padded_width  = aSrc->GetWidth()  + (2*border_width);
  PRUint32 padded_height = aSrc->GetHeight() + (2*border_width);

  
  
  
  PRUint32 expanded_width  = padded_width  * expand;
  PRUint32 expanded_height = padded_height * expand;

  PRUint32 start_x = border_width * expand;
  PRUint32 start_y = border_width * expand;

  PRUint8 *expanded_data = exp_buffer;
  PRUint32 exp_len = expanded_width*expanded_height;
  if (exp_len > sizeof(exp_buffer))
    expanded_data = (PRUint8*)malloc(expanded_width*expanded_height);
  for (y=0; y<padded_height; y++) {
    for (int i=0; i<expand; i++) {
      for (x=0; x<padded_width; x++) {
        PRUint32 padded_index = x+(y*padded_width);
        PRUint32 exp_index = (x*expand) + ((i+(y*expand))*expanded_width);
        for (int j=0; j<expand; j++) {
          expanded_data[exp_index+j] = padded_src[padded_index];
        }
      }
    }
  }
  DEBUG_DUMP((dump_byte_table(expanded_data, expanded_width, expanded_height)));


















#define SRC_UP_LEFT(ps)    *(ps-padded_width-1)
#define SRC_UP(ps)         *(ps-padded_width)
#define SRC_UP_RIGHT(ps)   *(ps-padded_width+1)
#define SRC_LEFT(ps)       *(ps-1)
#define SRC(ps)            *(ps)
#define SRC_RIGHT(ps)      *(ps+1)
#define SRC_DOWN_LEFT(ps)  *(ps+padded_width-1)
#define SRC_DOWN(ps)       *(ps+padded_width)
#define SRC_DOWN_RIGHT(ps) *(ps+padded_width+1)
#define FILL_VALUE 255

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  DEBUG_DUMP((dump_byte_table(expanded_data, expanded_width,expanded_height)));
  PRUint8 *ps, *es;
  PRUint32 jag_len = (expand)/2; 
  PRUint32 i, j;
  for (y=0; y<src_height; y++) {
    ps = padded_src + (border_width + (border_width+y)*padded_width);
    es = expanded_data
         + (border_width+((border_width+y)*expanded_width))*expand;
    for (x=0; x<src_width; x++,ps++,es+=expand) {
      
      
      
      if (SRC(ps)==0) {
        jag_len = ((expand+1)/2);
        if ((SRC_RIGHT(ps)==255) && (SRC_DOWN(ps)==255)) {
          
          for (i=0; i<jag_len; i++)
            for (j=0; j<jag_len-i; j++)
              es[expand-1-j+((expand-1-i)*expanded_width)] =
                            255-(((((i+j)<<8)/jag_len)*FILL_VALUE)>>8);
        }
        if ((SRC_DOWN(ps)==255) && (SRC_LEFT(ps)==255)) {
          
          for (i=0; i<jag_len; i++)
            for (j=0; j<jag_len-i; j++)
              es[j+((expand-1-i)*expanded_width)] =
                            255-(((((i+j)<<8)/jag_len)*FILL_VALUE)>>8);
        }
        if ((SRC_LEFT(ps)==255) && (SRC_UP(ps)==255)) {
          
          for (i=0; i<jag_len; i++)
            for (j=0; j<jag_len-i; j++)
              es[j+(i*expanded_width)] =
                            255-(((((i+j)<<8)/jag_len)*FILL_VALUE)>>8);
        }
        if ((SRC_UP(ps)==255) && (SRC_RIGHT(ps)==255)) {
          
          for (i=0; i<jag_len; i++)
            for (j=0; j<jag_len-i; j++)
              es[expand-1-j+(i*expanded_width)] =
                            255-(((((i+j)<<8)/jag_len)*FILL_VALUE)>>8);
        }
      }
      
      
      else {
        jag_len = ((expand+1)/2);
        if ((SRC_UP_LEFT(ps)==0) && (SRC_UP(ps)==0) && (SRC_LEFT(ps)==0)) {
          
          for (i=0; i<jag_len; i++)
            for (j=0; j<jag_len-i; j++)
              es[j+(i*expanded_width)] =
                            (((((i+j)<<8)/jag_len)*FILL_VALUE)>>8);
        }
        if ((SRC_UP(ps)==0) && (SRC_UP_RIGHT(ps)==0) && (SRC_RIGHT(ps)==0)) {
          
          for (i=0; i<jag_len; i++)
            for (j=0; j<jag_len-i; j++)
              es[expand-1-j+(i*expanded_width)] =
                            (((((i+j)<<8)/jag_len)*FILL_VALUE)>>8);
        }
        if ((SRC_LEFT(ps)==0) && (SRC_DOWN_LEFT(ps)==0) && (SRC_DOWN(ps)==0)) {
          
          for (i=0; i<jag_len; i++)
            for (j=0; j<jag_len-i; j++)
              es[j+((expand-1-i)*expanded_width)] =
                            (((((i+j)<<8)/jag_len)*FILL_VALUE)>>8);
        }
        if ((SRC_RIGHT(ps)==0) && (SRC_DOWN_RIGHT(ps)==0) && (SRC_DOWN(ps)==0)){
          
          for (i=0; i<jag_len; i++)
            for (j=0; j<jag_len-i; j++)
              es[(expand-1-j)+((expand-1-i)*expanded_width)] =
                            (((((i+j)<<8)/jag_len)*FILL_VALUE)>>8);
        }
      }
    }
  }
  DEBUG_DUMP((dump_byte_table(expanded_data, expanded_width,expanded_height)));

  
  
  
  
  
  PRUint32 ratio = ((dst_width<<8)/expand)/src_width;

  PRUint32 hsd_len = (dst_buffer_width+1) * expanded_height;
  if (hsd_len > sizeof(buffer)) {
    horizontally_scaled_data = (PRUint8*)nsMemory::Alloc(hsd_len);
    memset(horizontally_scaled_data, 0, hsd_len);
  }
  for (i=0; i<hsd_len; i++)
    horizontally_scaled_data[i] = 0;

  PRUint32 len_x   = src_width * expand;
  pHsd = horizontally_scaled_data;
  for (y=0; y<expanded_height; y++,pHsd+=dst_buffer_width) {
    for (x=0; x<len_x; x++) {
      PRUint32 exp_index = start_x + x + (y*expanded_width);
      PRUint8 src_val = expanded_data[exp_index];
      if (!src_val)
        continue;
      PRUint32 area_begin = x * ratio;
      PRUint32 area_end   = (x+1) * ratio;
      PRUint32 end_pixel   = (area_end+255)&0xffffff00;
      for (col=(area_begin&0xffffff00); col<end_pixel; col+=256) {
        PRUint32 this_begin = MAX(col,area_begin);
        PRUint32 this_end   = MIN((col+256), area_end);
        pHsd[col>>8] += (PRUint8)(((this_end-this_begin)*src_val)>>8);
        if ((&pHsd[col>>8]-horizontally_scaled_data) > (int)hsd_len) {
          NS_ASSERTION(0, "buffer too small");
          return;
        }
      }
    }

    DEBUG_DUMP((dump_byte_table(horizontally_scaled_data,
                dst_width, expanded_height)));
  }

  
  
  
  ratio = ((dst_height<<8)/expand)/src_height;
  PRUint32 len_y   = src_height * expand;
  for (x=0; x<dst_width; x++) {
    pHsd = horizontally_scaled_data + x + (start_y*dst_buffer_width);
    pDst = dst + x;
    for (y=0; y<len_y; y++,pHsd+=dst_buffer_width) {
      PRUint8 src_val = *pHsd;
      if (src_val == 0)
        continue;
      PRUint32 area_begin = y * ratio;
      PRUint32 area_end   = area_begin + ratio;
      PRUint32 end_pixel   = (area_end+255)&0xffffff00;
      PRUint32 c;
      PRUint32 col;
      for (c=(area_begin>>8)*dst_buffer_width,col=(area_begin&0xffffff00);
                  col<end_pixel; c+=dst_buffer_width,col+=256) {
        PRUint32 this_begin = MAX(col,area_begin);
        PRUint32 this_end   = MIN((col+256), area_end);
        PRUint32 val = (((this_end-this_begin)*src_val)>>8);
        pDst[c] += val;
      }
    }
    DEBUG_DUMP((dump_byte_table(dst, dst_width, dst_height)));
  }
  if (expanded_data != exp_buffer)
    free(expanded_data);
  if (horizontally_scaled_data != buffer)
    free(horizontally_scaled_data);
}

#ifdef DEBUG
void
nsXFontAAScaledBitmap::dump_XImage_blue_data(XImage *ximage)
{
  int x, y;
  int width  = ximage->width;
  int height = ximage->height;
  int pitch = ximage->bytes_per_line;
  int depth  = DefaultDepth(sDisplay, DefaultScreen(sDisplay));
  PRUint8 *lineStart = (PRUint8 *)ximage->data;
  printf("dump_byte_table: width=%d, height=%d\n", width, height);
  printf("    ");
  for (x=0; (x<width)&&(x<75); x++) {
    if ((x%10) == 0)
      printf("+ ");
    else
      printf("- ");
  }
  printf("\n");
  if ((depth == 16) || (depth == 15)) {
    short *data = (short *)ximage->data;
    for (y=0; y<height; y++) {
      printf("%2d: ", y);
      data = (short *)lineStart;
      for (x=0; (x<width)&&(x<75); x++, data++) {
        printf("%02x", *data & 0x1F);
      }
      printf("\n");
      lineStart += ximage->bytes_per_line;
    }
  }
  else if ((depth == 24) || (depth == 32)) {
    long *data = (long *)ximage->data;
    for (y=0; y<height; y++) {
      printf("%2d: ", y);
      for (x=0; (x<width)&&(x<75); x++) {
        printf("%02x", (short)(data[x+(y*pitch)] & 0xFF));
      }
      printf("\n");
    }
  }
  else {
    printf("depth %d not supported\n", DefaultDepth(sDisplay, DefaultScreen(sDisplay)));
  }
}

void
dump_byte_table(PRUint8 *table, int width, int height)
{
  int x, y;
  printf("dump_byte_table: width=%d, height=%d\n", width, height);
  printf("    ");
  for (x=0; x<width; x++) {
    if ((x%10) == 0)
      printf("+ ");
    else
      printf("- ");
  }
  printf("\n");
  for (y=0; y<height; y++) {
    printf("%2d: ", y);
    for (x=0; x<width; x++) {
      PRUint8 val = table[x+(y*width)];
      printf("%02x", val);
    }
    printf("\n");
  }
  printf("---\n");
}
#endif

static void
WeightTableInitLinearCorrection(PRUint8* aTable, PRUint8 aMinValue,
                                double aGain)
{
  
  for (int i=0; i<256; i++) {
    int val = i;
    if (i>=aMinValue)
      val += (int)rint((double)(i-aMinValue)*aGain);
    val = MAX(0, val);
    val = MIN(val, 255);
    aTable[i] = (PRUint8)val;
  }
}

