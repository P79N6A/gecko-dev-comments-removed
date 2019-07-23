






































#ifndef nsFontFreeType_h__
#define nsFontFreeType_h__

#include "gfx-config.h"
#include "nsFontMetricsGTK.h"
#include "nsFreeType.h"

#if (!defined(MOZ_ENABLE_FREETYPE2))
class nsFreeTypeFont : public nsFontGTK {
public:
  static nsFreeTypeFont *NewFont(nsITrueTypeFontCatalogEntry*,
                                 PRUint16, const char *);
};
#else

class nsFreeTypeFont : public nsFontGTK
{
public:

  nsFreeTypeFont();
  nsFreeTypeFont(nsITrueTypeFontCatalogEntry *, PRUint16, const char *);
  virtual ~nsFreeTypeFont(void);
  static nsFreeTypeFont *NewFont(nsITrueTypeFontCatalogEntry*,
                                 PRUint16, const char *);

  void LoadFont(void);

  virtual GdkFont* GetGDKFont(void);
  virtual PRBool   GetGDKFontIs10646(void);
  virtual PRBool   IsFreeTypeFont(void);

  virtual gint GetWidth(const PRUnichar* aString, PRUint32 aLength);
  virtual gint DrawString(nsRenderingContextGTK* aContext,
                          nsDrawingSurfaceGTK* aSurface, nscoord aX,
                          nscoord aY, const PRUnichar* aString,
                          PRUint32 aLength);
#ifdef MOZ_MATHML
  virtual nsresult GetBoundingMetrics(const PRUnichar*   aString,
                                      PRUint32           aLength,
                                      nsBoundingMetrics& aBoundingMetrics);
#endif
  virtual nsresult doGetBoundingMetrics(const PRUnichar*   aString,
                                        PRUint32 aLength,
                                        PRInt32* aLeftBearing,
                                        PRInt32* aRightBearing,
                                        PRInt32* aAscent,
                                        PRInt32* aDescent,
                                        PRInt32* aWidth);

  virtual PRUint32 Convert(const PRUnichar* aSrc, PRUint32 aSrcLen,
                           PRUnichar* aDest, PRUint32 aDestLen);

  FT_Face getFTFace();
  int     ascent();
  int     descent();
  PRBool  getXHeight(unsigned long &val);
  int     max_ascent();
  int     max_descent();
  int     max_width();
  PRBool  superscript_y(long &val);
  PRBool  subscript_y(long &val);
  PRBool  underlinePosition(long &val);
  PRBool  underline_thickness(unsigned long &val);

  FT_Error FaceRequester(FT_Face* aface);
  static void FreeGlobals();

  static PRUint8 sLinearWeightTable[256];

protected:
  XImage *GetXImage(PRUint32 width, PRUint32 height);
  nsITrueTypeFontCatalogEntry *mFaceID;
  PRUint16        mPixelSize;
  FTC_Image_Desc  mImageDesc;
  nsCOMPtr<nsIFreeType2> mFt2;
};

void WeightTableInitCorrection(PRUint8*, PRUint8, double);

#endif
#endif

