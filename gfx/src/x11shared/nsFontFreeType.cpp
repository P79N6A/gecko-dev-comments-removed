







































#ifdef DEBUG

#define DEBUG_SHOW_GLYPH_BOX 0
#endif

#include "gfx-config.h"
#include "nsFontFreeType.h"
#include "nsUnicharUtils.h"

#if (!defined(MOZ_ENABLE_FREETYPE2))

nsFreeTypeFont *
nsFreeTypeFont::NewFont(nsITrueTypeFontCatalogEntry *, PRUint16, const char *)
{
  return nsnull;
}

#else

#include "nsX11AlphaBlend.h"
#include "nsAntiAliasedGlyph.h"
#include "nsFontDebug.h"
#include "nsIServiceManager.h"


#define FT_16_16_TO_REG(x) ((x)>>16)

#define IMAGE_BUFFER_SIZE 2048

PRUint32 deltaMicroSeconds(PRTime aStartTime, PRTime aEndTime);
void GetFallbackGlyphMetrics(FT_BBox *aBoundingBox, FT_Face aFace);

PRUint8 nsFreeTypeFont::sLinearWeightTable[256];




class nsFreeTypeXImage : public nsFreeTypeFont {
public:
  nsFreeTypeXImage(nsITrueTypeFontCatalogEntry *aFaceID, PRUint16 aPixelSize,
                   const char *aName);
  gint DrawString(nsRenderingContextGTK* aContext,
                            nsDrawingSurfaceGTK* aSurface, nscoord aX,
                            nscoord aY, const PRUnichar* aString,
                            PRUint32 aLength);
protected:
  nsFreeTypeXImage();
};




class nsFreeTypeXImageSBC : public nsFreeTypeXImage {
public:
  nsFreeTypeXImageSBC(nsITrueTypeFontCatalogEntry *aFaceID,
                      PRUint16 aPixelSize, const char *aName);
#ifdef MOZ_MATHML
  virtual nsresult GetBoundingMetrics(const PRUnichar*   aString,
                                      PRUint32           aLength,
                                      nsBoundingMetrics& aBoundingMetrics);
#endif

  virtual gint GetWidth(const PRUnichar* aString, PRUint32 aLength);
  virtual gint DrawString(nsRenderingContextGTK* aContext,
                          nsDrawingSurfaceGTK* aSurface, nscoord aX,
                          nscoord aY, const PRUnichar* aString,
                          PRUint32 aLength);
protected:
  nsFreeTypeXImageSBC();
};

#ifdef ENABLE_TIME_MACROS
PRUint32
deltaMicroSeconds(PRTime aStartTime, PRTime aEndTime)
{
  PRUint32 delta;
  PRUint64 loadTime64;

  LL_SUB(loadTime64, aEndTime, aStartTime);
  LL_L2UI(delta, loadTime64);

  return delta;
}
#endif




nsFreeTypeFont::nsFreeTypeFont()
{
  NS_ERROR("should never call nsFreeTypeFont::nsFreeTypeFont");
}

nsFreeTypeFont *
nsFreeTypeFont::NewFont(nsITrueTypeFontCatalogEntry *aFaceID,
                        PRUint16 aPixelSize, const char *aName)
{
  
  nsresult rv;
  nsCOMPtr<nsIFreeType2> ft2 = do_GetService(NS_FREETYPE2_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    NS_ERROR("FreeType2 routines not available");
    return nsnull;
  }

  
  
  
  PRBool ximage = PR_TRUE;
  PRBool render = PR_FALSE;
  nsFreeTypeFont *ftfont;
  nsCAutoString familyName;
  aFaceID->GetFamilyName(familyName);
  nsTTFontFamilyEncoderInfo *ffei =
    nsFreeType2::GetCustomEncoderInfo(familyName.get());
  if (ximage) {
    if (ffei) {
      ftfont = new nsFreeTypeXImageSBC(aFaceID, aPixelSize, aName);
    }
    else {
      ftfont = new nsFreeTypeXImage(aFaceID, aPixelSize, aName);
    }
    return ftfont;
  }
  else if (render) {
    NS_ERROR("need to construct a render type FreeType object");
    return nsnull;
  }
  NS_ERROR("need to construct other type FreeType objects");
  return nsnull;
}

FT_Face
nsFreeTypeFont::getFTFace()
{
  FT_Face face = nsnull;
  FTC_Manager mgr;
  nsresult rv;
  mFt2->GetFTCacheManager(&mgr);
  rv = mFt2->ManagerLookupSize(mgr, &mImageDesc.font, &face, nsnull);
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to get face/size");
  if (NS_FAILED(rv))
    return nsnull;
  return face;
}

nsFreeTypeFont::nsFreeTypeFont(nsITrueTypeFontCatalogEntry *aFaceID,
                               PRUint16 aPixelSize, const char *aName)
{
  PRBool anti_alias = PR_TRUE;
  PRBool embedded_bimap = PR_FALSE;
  mFaceID = aFaceID;
  mPixelSize = aPixelSize;
  mImageDesc.font.face_id    = (void*)mFaceID;
  mImageDesc.font.pix_width  = aPixelSize;
  mImageDesc.font.pix_height = aPixelSize;
  mImageDesc.image_type = 0;

  if (aPixelSize < nsFreeType2::gAntiAliasMinimum) {
    mImageDesc.image_type |= ftc_image_mono;
    anti_alias = PR_FALSE;
  }

  if (nsFreeType2::gFreeType2Autohinted)
    mImageDesc.image_type |= ftc_image_flag_autohinted;

  if (nsFreeType2::gFreeType2Unhinted)
    mImageDesc.image_type |= ftc_image_flag_unhinted;

  PRUint32  num_embedded_bitmaps, i;
  PRInt32*  embedded_bitmapheights;
  mFaceID->GetEmbeddedBitmapHeights(&num_embedded_bitmaps,
                                    &embedded_bitmapheights);
  
  if (aPixelSize <= nsFreeType2::gEmbeddedBitmapMaximumHeight) {
    if (num_embedded_bitmaps) {
      for (i=0; i<num_embedded_bitmaps; i++) {
        if (embedded_bitmapheights[i] == aPixelSize) {
          embedded_bimap = PR_TRUE;
          
          mImageDesc.image_type |= ftc_image_flag_unhinted;
          break;
        }
      }
    }
  }

  nsresult rv;
  
  mFt2 = do_GetService(NS_FREETYPE2_CONTRACTID, &rv);
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to find FreeType routines");

  FREETYPE_FONT_PRINTF(("anti_alias=%d, embedded_bitmap=%d, "
                        "AutoHinted=%d, gFreeType2Unhinted = %d, "
                        "size=%dpx, \"%s\"",
                        anti_alias, embedded_bimap,
                        nsFreeType2::gFreeType2Autohinted,
                        nsFreeType2::gFreeType2Unhinted,
                        aPixelSize, aName));
}

void
nsFreeTypeFont::LoadFont()
{
  if (mAlreadyCalledLoadFont) {
    return;
  }

  mAlreadyCalledLoadFont = PR_TRUE;
  PRUint32 size;
  mFaceID->GetCCMap(&size, &mCCMap);
#ifdef NS_FONT_DEBUG_LOAD_FONT
  nsCAutoString fileName;
  mFaceID->GetFileName(fileName);
  if (gFontDebug & NS_FONT_DEBUG_LOAD_FONT) {
    printf("loaded \"%s\", size=%d, filename=%s\n",
                 mName, mSize, fileName.get());
  }
#endif
}

nsFreeTypeFont::~nsFreeTypeFont()
{
}

#ifdef MOZ_MATHML
nsresult
nsFreeTypeFont::GetBoundingMetrics(const PRUnichar*   aString,
                                   PRUint32           aLength,
                                   nsBoundingMetrics& aBoundingMetrics)
{
  return doGetBoundingMetrics(aString, aLength,
                              &aBoundingMetrics.leftBearing,
                              &aBoundingMetrics.rightBearing,
                              &aBoundingMetrics.ascent,
                              &aBoundingMetrics.descent,
                              &aBoundingMetrics.width);
}
#endif



nsresult
nsFreeTypeFont::doGetBoundingMetrics(const PRUnichar* aString, PRUint32 aLength,
                                     PRInt32* aLeftBearing,
                                     PRInt32* aRightBearing,
                                     PRInt32* aAscent,
                                     PRInt32* aDescent,
                                     PRInt32* aWidth)
{
  nsresult rv;

  *aLeftBearing = 0;
  *aRightBearing = 0;
  *aAscent = 0;
  *aDescent = 0;
  *aWidth = 0;

  if (aLength < 1) {
    return NS_ERROR_FAILURE;
  }

  FT_Pos pos = 0;
  FT_BBox bbox;
  
  bbox.xMin = bbox.yMin = 32000;
  bbox.xMax = bbox.yMax = -32000;

  
  FT_Face face = getFTFace();
  NS_ASSERTION(face, "failed to get face/size");
  if (!face)
    return NS_ERROR_FAILURE;

  FTC_Image_Cache icache;
  mFt2->GetImageCache(&icache);
  if (!icache)
    return NS_ERROR_FAILURE;

  
  PRUint32 i, extraSurrogateLength;
  for (i=0; i<aLength; i+=1+extraSurrogateLength) {
    FT_UInt glyph_index;
    FT_Glyph glyph;
    FT_BBox glyph_bbox;
    FT_Pos advance;
    extraSurrogateLength=0;

    FT_ULong code_point = aString[i];
    if(i<aLength-1 && NS_IS_HIGH_SURROGATE(code_point) && NS_IS_LOW_SURROGATE(aString[i+1])) {
      
      
      code_point = SURROGATE_TO_UCS4(code_point, aString[i+1]);

      
      extraSurrogateLength = 1;
    }
    mFt2->GetCharIndex(face, code_point, &glyph_index);

    
    if (glyph_index) {
      rv = mFt2->ImageCacheLookup(icache, &mImageDesc, glyph_index, &glyph);
      NS_ASSERTION(NS_SUCCEEDED(rv),"error loading glyph");
    }
    if ((glyph_index) && (NS_SUCCEEDED(rv))) {
      mFt2->GlyphGetCBox(glyph, ft_glyph_bbox_pixels, &glyph_bbox);
      advance = FT_16_16_TO_REG(glyph->advance.x);
    }
    else {
      
      GetFallbackGlyphMetrics(&glyph_bbox, face);
      advance = glyph_bbox.xMax + 1;
    }
    bbox.xMin = PR_MIN(pos+glyph_bbox.xMin, bbox.xMin);
    bbox.xMax = PR_MAX(pos+glyph_bbox.xMax, bbox.xMax);
    bbox.yMin = PR_MIN(glyph_bbox.yMin, bbox.yMin);
    bbox.yMax = PR_MAX(glyph_bbox.yMax, bbox.yMax);
    pos += advance;
  }

  
  if (bbox.xMin > bbox.xMax)
    bbox.xMin = bbox.xMax = bbox.yMin = bbox.yMax = 0;

  *aLeftBearing  = bbox.xMin;
  *aRightBearing = bbox.xMax;
  *aAscent       = bbox.yMax;
  *aDescent      = -bbox.yMin;
  *aWidth        = pos;
  return NS_OK;
}

GdkFont*
nsFreeTypeFont::GetGDKFont()
{
  return nsnull;
}

PRBool
nsFreeTypeFont::GetGDKFontIs10646()
{
  return PR_TRUE;
}

PRBool
nsFreeTypeFont::IsFreeTypeFont()
{
  return PR_TRUE;
}

gint
nsFreeTypeFont::GetWidth(const PRUnichar* aString, PRUint32 aLength)
{
  FT_UInt glyph_index;
  FT_Glyph glyph;
  FT_Pos origin_x = 0;

  
  FT_Face face = getFTFace();
  NS_ASSERTION(face, "failed to get face/size");
  if (!face)
    return 0;

  FTC_Image_Cache icache;
  mFt2->GetImageCache(&icache);
  if (!icache)
    return 0;

  PRUint32 i, extraSurrogateLength;
  for (i=0; i<aLength; i+=1+extraSurrogateLength) {
    extraSurrogateLength=0;
    FT_ULong code_point = aString[i];
    if(i<aLength-1 && NS_IS_HIGH_SURROGATE(code_point) && NS_IS_LOW_SURROGATE(aString[i+1])) {
      
      
      code_point = SURROGATE_TO_UCS4(code_point, aString[i+1]);

      
      extraSurrogateLength = 1;
    }
    mFt2->GetCharIndex((FT_Face)face, code_point, &glyph_index);
    nsresult rv;
    rv = mFt2->ImageCacheLookup(icache, &mImageDesc, glyph_index, &glyph);
    NS_ASSERTION(NS_SUCCEEDED(rv),"error loading glyph");
    if (NS_FAILED(rv)) {
      origin_x += face->size->metrics.x_ppem/2 + 2;
      continue;
    }
    origin_x += FT_16_16_TO_REG(glyph->advance.x);
  }

  return origin_x;
}

gint
nsFreeTypeFont::DrawString(nsRenderingContextGTK* aContext,
                            nsDrawingSurfaceGTK* aSurface, nscoord aX,
                            nscoord aY, const PRUnichar* aString,
                            PRUint32 aLength)
{
  NS_ERROR("should never call nsFreeTypeFont::DrawString");
  return 0;
}

PRUint32
nsFreeTypeFont::Convert(const PRUnichar* aSrc, PRUint32 aSrcLen,
                           PRUnichar* aDest, PRUint32 aDestLen)
{
  NS_ERROR("should not be calling nsFreeTypeFont::Convert");
  return 0;
}

int
nsFreeTypeFont::ascent()
{
  FT_Face face = getFTFace();
  NS_ASSERTION(face, "failed to get face/size");
  if (!face)
    return 0;
  return FT_DESIGN_UNITS_TO_PIXELS(face->ascender, face->size->metrics.y_scale);
}

int
nsFreeTypeFont::descent()
{
  FT_Face face = getFTFace();
  NS_ASSERTION(face, "failed to get face/size");
  if (!face)
    return 0;
  return FT_DESIGN_UNITS_TO_PIXELS(-face->descender, face->size->metrics.y_scale);
}

int
nsFreeTypeFont::max_ascent()
{
  FT_Face face = getFTFace();
  NS_ASSERTION(face, "failed to get face/size");
  if (!face)
    return 0;

  TT_OS2 * tt_os2;
  mFt2->GetSfntTable(face, ft_sfnt_os2, (void**)&tt_os2);
  NS_ASSERTION(tt_os2, "unable to get OS2 table");
  if (tt_os2)
     return FT_DESIGN_UNITS_TO_PIXELS(tt_os2->sTypoAscender,
                                      face->size->metrics.y_scale);
  else
     return FT_DESIGN_UNITS_TO_PIXELS(face->bbox.yMax,
                                      face->size->metrics.y_scale);
}

int
nsFreeTypeFont::max_descent()
{
  FT_Face face = getFTFace();
  NS_ASSERTION(face, "failed to get face/size");
  if (!face)
    return 0;

  TT_OS2 *tt_os2;
  mFt2->GetSfntTable(face, ft_sfnt_os2, (void**)&tt_os2);
  NS_ASSERTION(tt_os2, "unable to get OS2 table");
  if (tt_os2)
     return FT_DESIGN_UNITS_TO_PIXELS(-tt_os2->sTypoDescender,
                                      face->size->metrics.y_scale);
  else
     return FT_DESIGN_UNITS_TO_PIXELS(-face->bbox.yMin,
                                      face->size->metrics.y_scale);
}

int
nsFreeTypeFont::max_width()
{
  FT_Face face = getFTFace();
  NS_ASSERTION(face, "failed to get face/size");
  if (!face)
    return 0;
  return FT_DESIGN_UNITS_TO_PIXELS(face->max_advance_width,
                                   face->size->metrics.x_scale);
}

PRBool
nsFreeTypeFont::getXHeight(unsigned long &val)
{
  FT_Face face = getFTFace();
  NS_ASSERTION(face, "failed to get face/size");
  if (!face || !val)
    return PR_FALSE;
  val = FT_DESIGN_UNITS_TO_PIXELS(face->height, face->size->metrics.y_scale);

  return PR_TRUE;
}

PRBool
nsFreeTypeFont::underlinePosition(long &val)
{
  FT_Face face = getFTFace();
  NS_ASSERTION(face, "failed to get face/size");
  if (!face)
    return PR_FALSE;
  val = FT_DESIGN_UNITS_TO_PIXELS(-face->underline_position,
                                   face->size->metrics.y_scale);
  return PR_TRUE;
}

PRBool
nsFreeTypeFont::underline_thickness(unsigned long &val)
{
  FT_Face face = getFTFace();
  NS_ASSERTION(face, "failed to get face/size");
  if (!face)
    return PR_FALSE;
  val = FT_DESIGN_UNITS_TO_PIXELS(face->underline_thickness,
                                  face->size->metrics.y_scale);
  return PR_TRUE;
}

PRBool
nsFreeTypeFont::superscript_y(long &val)
{
  FT_Face face = getFTFace();
  NS_ASSERTION(face, "failed to get face/size");
  if (!face)
    return PR_FALSE;

  TT_OS2 *tt_os2;
  mFt2->GetSfntTable(face, ft_sfnt_os2, (void**)&tt_os2);
  NS_ASSERTION(tt_os2, "unable to get OS2 table");
  if (!tt_os2)
    return PR_FALSE;

  val = FT_DESIGN_UNITS_TO_PIXELS(tt_os2->ySuperscriptYOffset,
                                  face->size->metrics.y_scale);
  return PR_TRUE;
}

PRBool
nsFreeTypeFont::subscript_y(long &val)
{
  FT_Face face = getFTFace();
  NS_ASSERTION(face, "failed to get face/size");
  if (!face)
    return PR_FALSE;

  TT_OS2 *tt_os2;
  mFt2->GetSfntTable(face, ft_sfnt_os2, (void**)&tt_os2);
  NS_ASSERTION(tt_os2, "unable to get OS2 table");
  if (!tt_os2)
    return PR_FALSE;

  val = FT_DESIGN_UNITS_TO_PIXELS(tt_os2->ySubscriptYOffset,
                                  face->size->metrics.y_scale);

  
  val = (val < 0) ? -val : val;
  return PR_TRUE;
}






class nsFreeTypeRender : nsFreeTypeFont {
private:
  nsFreeTypeRender();
};




nsFreeTypeXImage::nsFreeTypeXImage()
{
  NS_ERROR("should never call nsFreeTypeXImage::nsFreeTypeXImage");
}

nsFreeTypeXImage::nsFreeTypeXImage(nsITrueTypeFontCatalogEntry *aFaceID,
                                   PRUint16 aPixelSize, const char *aName)
: nsFreeTypeFont(aFaceID, aPixelSize, aName)
{
  
}

gint
nsFreeTypeXImage::DrawString(nsRenderingContextGTK* aContext,
                            nsDrawingSurfaceGTK* aSurface, nscoord aX,
                            nscoord aY, const PRUnichar* aString,
                            PRUint32 aLength)
{

#if DEBUG_SHOW_GLYPH_BOX
  PRUint32 x, y;
  
  
  
#endif

  if (aLength < 1) {
    return 0;
  }

  
  FT_Face face = getFTFace();
  NS_ASSERTION(face, "failed to get face/size");
  if (!face)
    return 0;

  nsresult rslt;
  PRInt32 leftBearing, rightBearing, ascent, descent, width;
  rslt = doGetBoundingMetrics(aString, aLength, &leftBearing, &rightBearing,
                              &ascent, &descent, &width);
  if (NS_FAILED(rslt))
    return 0;

  
  rightBearing = PR_MAX(rightBearing, width+1);

  
  PRInt32 x_origin = PR_MAX(0, -leftBearing);
  
  PRInt32 y_origin = ascent;
  PRInt32 x_pos = x_origin;

  int image_width  = x_origin + rightBearing;
  int image_height = y_origin + PR_MAX(descent, 0);
  if ((image_width<=0) || (image_height<=0)) {
    
    
    NS_ASSERTION(width>=0, "Negative width");
    return width;
  }
  Display *dpy = GDK_DISPLAY();
  Drawable win = GDK_WINDOW_XWINDOW(aSurface->GetDrawable());
  GC gc = GDK_GC_XGC(aContext->GetGC());
  XGCValues values;
  if (!XGetGCValues(dpy, gc, GCForeground, &values)) {
    NS_ERROR("failed to get foreground pixel");
    return 0;
  }
  nscolor color = nsX11AlphaBlend::PixelToNSColor(values.foreground);

#if DEBUG_SHOW_GLYPH_BOX
  
  XDrawLine(dpy, win, DefaultGC(dpy, 0), aX-2, aY, aX+2, aY);
  XDrawLine(dpy, win, DefaultGC(dpy, 0), aX, aY-2, aX, aY+2);
  
  XDrawLine(dpy, win, DefaultGC(dpy, 0), aX-x_origin,  aY-y_origin-2,
                                         aX+rightBearing, aY-y_origin-2);
#endif

  
  
  
  XImage *sub_image = nsX11AlphaBlend::GetBackground(dpy, DefaultScreen(dpy),
                                 win, aX-x_origin, aY-y_origin,
                                 image_width, image_height);
  if (sub_image==nsnull) {
#ifdef DEBUG
    int screen = DefaultScreen(dpy);
    
    int win_width = DisplayWidth(dpy, screen);
    int win_height = DisplayHeight(dpy, screen);
    if (((int)(aX-leftBearing+image_width) > 0)  
        && ((int)(aX-leftBearing) < win_width)   
        && ((int)(aY-ascent+image_height) > 0)
        && ((int)(aY-ascent) < win_height))   
    {
      NS_ASSERTION(sub_image, "failed to get the image");
    }
#endif
    return 0;
  }

#if DEBUG_SHOW_GLYPH_BOX
  DEBUG_AADRAWBOX(sub_image,0,0,image_width,image_height,0,0,0,255/4);
  nscolor black NS_RGB(0,255,0);
  blendPixel blendPixelFunc = nsX11AlphaBlend::GetBlendPixel();
  
  for (x=0; x<(unsigned int)image_height; x++)
    if (x%4==0) (*blendPixelFunc)(sub_image, x_origin, x, black, 255/2);
  
  for (y=0; y<(unsigned int)image_width; y++)
    if (y%4==0) (*blendPixelFunc)(sub_image, y, ascent-1, black, 255/2);
#endif

  FTC_Image_Cache icache;
  mFt2->GetImageCache(&icache);
  if (!icache)
    return 0;

  
  
  
  blendGlyph blendGlyph = nsX11AlphaBlend::GetBlendGlyph();
  PRUint32 i, extraSurrogateLength;
  for (i=0; i<aLength; i+=1+extraSurrogateLength) {
    FT_UInt glyph_index;
    FT_Glyph glyph;
    nsresult rv;
    FT_BBox glyph_bbox;
    FT_ULong code_point = aString[i];
    extraSurrogateLength = 0;

    if(i<aLength-1 && NS_IS_HIGH_SURROGATE(code_point) && NS_IS_LOW_SURROGATE(aString[i+1])) {
      
      
      code_point = SURROGATE_TO_UCS4(code_point, aString[i+1]);

      
      extraSurrogateLength = 1;
    }

    mFt2->GetCharIndex(face, code_point, &glyph_index);
    if (glyph_index) {
      rv = mFt2->ImageCacheLookup(icache, &mImageDesc, glyph_index, &glyph);
    }
    if ((glyph_index) && (NS_SUCCEEDED(rv))) {
      mFt2->GlyphGetCBox(glyph, ft_glyph_bbox_pixels, &glyph_bbox);
    }
    else {
      
      GetFallbackGlyphMetrics(&glyph_bbox, face);
      int x, y, w = glyph_bbox.xMax, h = glyph_bbox.yMax;
      for (x=1; x<w; x++) {
        XPutPixel(sub_image, x_pos+x, ascent-1,   values.foreground);
        XPutPixel(sub_image, x_pos+x, ascent-h, values.foreground);
      }
      for (y=1; y<h; y++) {
        XPutPixel(sub_image, x_pos+1, ascent-y, values.foreground);
        XPutPixel(sub_image, x_pos+w-1, ascent-y, values.foreground);
        x = (y*(w-2))/h;
        XPutPixel(sub_image, x_pos+x+1, ascent-y,   values.foreground);
      }
      x_pos += w + 1;
      continue;
    }

    FT_BitmapGlyph slot = (FT_BitmapGlyph)glyph;
    nsAntiAliasedGlyph aaglyph(glyph_bbox.xMax-glyph_bbox.xMin,
                               glyph_bbox.yMax-glyph_bbox.yMin, 0);
    PRUint8 buf[IMAGE_BUFFER_SIZE]; 
    if (!aaglyph.WrapFreeType(&glyph_bbox, slot, buf, IMAGE_BUFFER_SIZE)) {
      NS_ERROR("failed to wrap freetype image");
      XDestroyImage(sub_image);
      return 0;
    }

    
    
    
    NS_ASSERTION(ascent>=glyph_bbox.yMax,"glyph too tall");
    NS_ASSERTION(x_pos>=-aaglyph.GetLBearing(),"glyph extends too far to left");

#if DEBUG_SHOW_GLYPH_BOX
  
  
  if (aaglyph.GetLBearing() < 0) {
    DEBUG_AADRAWBOX(sub_image, x_pos + aaglyph.GetLBearing(),
                    ascent-glyph_bbox.yMax,
                    -aaglyph.GetLBearing(), glyph_bbox.yMax, 255,0,0, 255/4);
  }
  
  DEBUG_AADRAWBOX(sub_image, x_pos, ascent-glyph_bbox.yMax,
                  aaglyph.GetAdvance(), glyph_bbox.yMax, 0,255,0, 255/4);
  
  
  if (aaglyph.GetRBearing() > (int)aaglyph.GetAdvance()) {
    DEBUG_AADRAWBOX(sub_image, x_pos + aaglyph.GetAdvance(),
                    ascent-glyph_bbox.yMax,
                    aaglyph.GetRBearing()-aaglyph.GetAdvance(),
                    glyph_bbox.yMax, 0,0,255, 255/4);
  }
#endif
    (*blendGlyph)(sub_image, &aaglyph, sLinearWeightTable, color,
                  x_pos + aaglyph.GetLBearing(), ascent-glyph_bbox.yMax);

    x_pos += aaglyph.GetAdvance();
  }

  
  
  
  XPutImage(dpy, win, gc, sub_image, 0, 0, aX-x_origin , aY-ascent,
            image_width, image_height);
  XDestroyImage(sub_image);

  return width;
}







nsFreeTypeXImageSBC::nsFreeTypeXImageSBC()
{
  NS_ERROR("should never call nsFreeTypeXImageSBC::nsFreeTypeXImageSBC");
}

nsFreeTypeXImageSBC::nsFreeTypeXImageSBC(nsITrueTypeFontCatalogEntry *aFaceID,
                                         PRUint16 aPixelSize,
                                         const char *aName)
: nsFreeTypeXImage(aFaceID, aPixelSize, aName)
{
}

#ifdef MOZ_MATHML
nsresult
nsFreeTypeXImageSBC::GetBoundingMetrics(const PRUnichar*   aString,
                                        PRUint32           aLength,
                                        nsBoundingMetrics& aBoundingMetrics)
{
  nsresult res;
  char buf[512];
  PRInt32 bufLen = sizeof(buf);
  PRInt32 stringLen = aLength;
  nsCAutoString familyName;
  mFaceID->GetFamilyName(familyName);
  nsTTFontFamilyEncoderInfo *ffei =
    nsFreeType2::GetCustomEncoderInfo(familyName.get());
  NS_ASSERTION(ffei,"failed to find font encoder info");
  if (!ffei)
    return NS_ERROR_FAILURE;
  res = ffei->mEncodingInfo->mConverter->Convert(aString, &stringLen,
                                                 buf, &bufLen);
  NS_ASSERTION((aLength&&bufLen)||(!aLength&&!bufLen), "converter failed");

  
  
  
  PRUnichar unibuf[512];
  int i;
  for (i=0; i<bufLen; i++) {
    unibuf[i] = (unsigned char)buf[i];
  }

  res = nsFreeTypeXImage::GetBoundingMetrics(unibuf, bufLen, aBoundingMetrics);
  return res;
}
#endif

gint
nsFreeTypeXImageSBC::GetWidth(const PRUnichar* aString, PRUint32 aLength)
{
  char buf[512];
  PRInt32 bufLen = sizeof(buf);
  PRInt32 stringLen = aLength;
  nsCAutoString familyName;
  mFaceID->GetFamilyName(familyName);
  nsTTFontFamilyEncoderInfo *ffei =
    nsFreeType2::GetCustomEncoderInfo(familyName.get());
  NS_ASSERTION(ffei,"failed to find font encoder info");
  if (!ffei)
    return 0;
  ffei->mEncodingInfo->mConverter->Convert(aString, &stringLen,
                                                 buf, &bufLen);
  NS_ASSERTION((aLength&&bufLen)||(!aLength&&!bufLen), "converter failed");

  
  
  
  PRUnichar unibuf[512];
  int i;
  for (i=0; i<bufLen; i++) {
    unibuf[i] = (unsigned char)buf[i];
  }

  return nsFreeTypeXImage::GetWidth(unibuf, bufLen);
}

gint
nsFreeTypeXImageSBC::DrawString(nsRenderingContextGTK* aContext,
                                nsDrawingSurfaceGTK* aSurface, nscoord aX,
                                nscoord aY, const PRUnichar* aString,
                                PRUint32 aLength)
{
  char buf[512];
  PRInt32 bufLen = sizeof(buf);
  PRInt32 stringLen = aLength;
  nsCAutoString familyName;
  mFaceID->GetFamilyName(familyName);
  nsTTFontFamilyEncoderInfo *ffei =
    nsFreeType2::GetCustomEncoderInfo(familyName.get());
  NS_ASSERTION(ffei,"failed to find font encoder info");
  if (!ffei)
    return 0;
  ffei->mEncodingInfo->mConverter->Convert(aString, &stringLen,
                                           buf, &bufLen);
  NS_ASSERTION((aLength&&bufLen)||(!aLength&&!bufLen), "converter failed");

  
  
  
  PRUnichar unibuf[512];
  int i;
  for (i=0; i<bufLen; i++) {
    unibuf[i] = (unsigned char)buf[i];
  }

  return nsFreeTypeXImage::DrawString(aContext, aSurface, aX, aY,
                                      unibuf, bufLen);
}

void
GetFallbackGlyphMetrics(FT_BBox *aBoundingBox, FT_Face aFace) {
  aBoundingBox->xMin = 0;
  aBoundingBox->yMin = 0;
  aBoundingBox->xMax = PR_MAX(aFace->size->metrics.x_ppem/2 - 1, 0);
  aBoundingBox->yMax = PR_MAX(aFace->size->metrics.y_ppem/2, 1);
}

void
WeightTableInitCorrection(PRUint8* aTable, PRUint8 aMinValue,
                                double aGain)
{
  
  for (int i=0; i<256; i++) {
    int val = i + (int)rint((double)(i-aMinValue)*aGain);
    val = PR_MAX(0, val);
    val = PR_MIN(val, 255);
    aTable[i] = (PRUint8)val;
  }
}

#endif

