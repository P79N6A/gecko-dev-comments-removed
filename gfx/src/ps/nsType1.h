

















































#ifndef TYPE1_H
#define TYPE1_H

#include <stdio.h>
#include "nspr.h"
#ifdef MOZ_ENABLE_XFT
#include "nsISupports.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#else
#include "nsIFreeType2.h"
#endif


inline int
toCS(double upm, double x)
{
  return (int)((x*1000.0)/upm);
}

inline int
fromCS(double upm, double x)
{
  return (int) (x*upm/1000.0);
}


#define T1_HSTEM      1  /* 0x01 */
#define T1_VSTEM      3  /* 0x03 */
#define T1_VMOVETO    4  /* 0x04 */
#define T1_RLINETO    5  /* 0x05 */
#define T1_HLINETO    6  /* 0x06 */
#define T1_VLINETO    7  /* 0x07 */
#define T1_RRCURVETO  8  /* 0x08 */
#define T1_CLOSEPATH  9  /* 0x09 */
#define T1_CALLSUBR  10  /* 0x0a */
#define T1_RETURN    11  /* 0x0b */
#define T1_ESC_CMD   12  /* 0x0c */
#define T1_HSBW      13  /* 0x0d */
#define T1_ENDCHAR   14  /* 0x0e */
#define T1_RMOVETO   21  /* 0x15 */
#define T1_HMOVETO   22  /* 0x16 */
#define T1_VHCURVETO 30  /* 0x1e */
#define T1_HVCURVETO 31  /* 0x1f */


#define T1_ESC_SBW    7  /* 0x07 */

#define TYPE1_ENCRYPTION_KEY 4330
#define TYPE1_ENCRYPTION_C1  52845
#define TYPE1_ENCRYPTION_C2  22719

#ifdef MOZ_ENABLE_XFT  
FT_Error FT2GlyphToType1CharString(FT_Face aFace,
#else 
FT_Error FT2GlyphToType1CharString(nsIFreeType2 *aFt2, FT_Face aFace,
#endif
                                   PRUint32 aGlyphID, int aWmode, int aLenIV,
                                   unsigned char *aBuf);
#ifdef MOZ_ENABLE_XFT

#define FT_REG_TO_16_16(x) ((x)<<16)
#ifndef FT_MulFix
#define FT_MulFix(v, s) (((v)*(s))>>16)
#endif
#define FT_ROUND(x) (((x) + 32) & ~63) // 63 = 2^6 - 1
#define FT_TRUNC(x) ((x) >> 6)
#define FT_DESIGN_UNITS_TO_PIXELS(v, s) FT_TRUNC(FT_ROUND(FT_MulFix((v) , (s))))
#endif 

class nsString;
class nsCString;

PRBool FT2SubsetToType1FontSet(FT_Face aFace, const nsString& aSubset,
                               int aWmode,  FILE *aFile);
nsresult FT2ToType1FontName(FT_Face aFace, int aWmode,
                            nsCString& aFontName);
  

#endif 
