








































#include "nsULE.h"
#include "nsString.h"

#include "pango-types.h"
#include "pango-glyph.h"
#include "pango-modules.h"
#include "pango-utils.h"

#define GLYPH_COMBINING 256





nsULE::nsULE() {
}

nsULE::~nsULE() {
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsULE, nsILE)



const char*
nsULE::GetDefaultFont(const PRUnichar aString)
{
  if ((aString >= 0x0e01) && (aString <= 0x0e5b))
    return "tis620-2";
  if ((aString >= 0x0901) && (aString <= 0x0970))
    return "sun.unicode.india-0";
  return "iso8859-1";
}

PRInt32
nsULE::GetGlyphInfo(const PRUnichar      *aSrcBuf,
                    PRInt32              aSrcLen,
                    PangoliteGlyphString *aGlyphData,
                    const char           *aFontCharset)
{
  int                  ct=0, start=0, i, index, startgid, lastCluster=0;
  PRBool               sameCtlRun=PR_FALSE;
  PangoliteEngineShape *curShaper=NULL, *prevShaper=NULL;
  PangoliteMap         *pngMap=NULL;
  PangoliteAnalysis    pngAnalysis;
  guint                enginetypeId=0, rendertypeId=0;

  pngAnalysis.aDir = PANGO_DIRECTION_LTR;
 
  
  if (aFontCharset == NULL)
    pngAnalysis.fontCharset = strdup(GetDefaultFont(aSrcBuf[0]));
  else
    pngAnalysis.fontCharset = strdup(aFontCharset);

  enginetypeId = g_quark_from_static_string(PANGO_ENGINE_TYPE_SHAPE);
  rendertypeId = g_quark_from_static_string(PANGO_RENDER_TYPE_X);
  pngMap = pangolite_find_map("en_US", enginetypeId, rendertypeId);

  for (ct=0; ct < aSrcLen;) {
    start = ct;
    curShaper = (PangoliteEngineShape*)
      pangolite_map_get_engine(pngMap, (PRUint32)aSrcBuf[ct++]);
    sameCtlRun = (curShaper != NULL);
    prevShaper = curShaper;

    if (sameCtlRun) {
      while (sameCtlRun && ct < aSrcLen) {
        curShaper = (PangoliteEngineShape*)
          pangolite_map_get_engine(pngMap, (PRUint32)aSrcBuf[ct]);
        sameCtlRun = ((curShaper != NULL) && (curShaper == prevShaper));
        if (sameCtlRun)
          ct++;
      }
      startgid = aGlyphData->num_glyphs;
      pngAnalysis.shape_engine = curShaper;
      prevShaper->script_shape(pngAnalysis.fontCharset,
                               &aSrcBuf[start], (ct-start),
                               &pngAnalysis, aGlyphData);
      if (lastCluster > 0) {
         for (i=startgid; i < aGlyphData->num_glyphs; i++)
           aGlyphData->log_clusters[i] += lastCluster;
      }
    }
    else {
      while (!sameCtlRun && ct < aSrcLen) {
        curShaper = (PangoliteEngineShape*)
          pangolite_map_get_engine(pngMap, (PRUint32)aSrcBuf[ct]);
        sameCtlRun = (curShaper != NULL);
        if (!sameCtlRun)
          ct++;
      }
      index = aGlyphData->num_glyphs;
      for (i=0; i < (ct-start); i++) {
        pangolite_glyph_string_set_size(aGlyphData, index+1);
        aGlyphData->glyphs[index].glyph = aSrcBuf[start+i];
        aGlyphData->glyphs[index].is_cluster_start = (gint)1;
        aGlyphData->log_clusters[index] = i+lastCluster;
        index++;
      }
    }
    lastCluster = aGlyphData->log_clusters[aGlyphData->num_glyphs-1];
  }
  nsMemory::Free(pngAnalysis.fontCharset);
  return aGlyphData->num_glyphs;
}

NS_IMETHODIMP
nsULE::NeedsCTLFix(const PRUnichar *aString,
                   const PRInt32   aBeg,
                   const PRInt32   aEnd,
                   PRBool          *aCTLNeeded)
{
  PangoliteEngineShape *BegShaper=NULL, *EndShaper=NULL;
  PangoliteMap         *pngMap=NULL;
  guint                enginetypeId=0, rendertypeId=0;

  enginetypeId = g_quark_from_static_string(PANGO_ENGINE_TYPE_SHAPE);
  rendertypeId = g_quark_from_static_string(PANGO_RENDER_TYPE_X);
  pngMap = pangolite_find_map("en_US", enginetypeId, rendertypeId);

  *aCTLNeeded = PR_FALSE;
  if (aBeg >= 0)
    BegShaper = (PangoliteEngineShape*)
      pangolite_map_get_engine(pngMap, (PRUint32)aString[aBeg]);

  if (!BegShaper) {

    if ((aEnd < 0) && ((aBeg+aEnd) >= 0)) {
      EndShaper = (PangoliteEngineShape*)
        pangolite_map_get_engine(pngMap, (PRUint32)aString[aBeg+aEnd]);
    }
    else {
      EndShaper = (PangoliteEngineShape*)
        pangolite_map_get_engine(pngMap, (PRUint32)aString[aEnd]);
    }
  }

  if (BegShaper || EndShaper)
    *aCTLNeeded = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsULE::GetPresentationForm(const PRUnichar *aString,
                           PRUint32        aLength,
                           const char      *aFontCharset,
                           char            *aGlyphs,
                           PRSize          *aOutLength,
                           PRBool          aIsWide)
{
  PangoliteGlyphString *tmpGlyphs=pangolite_glyph_string_new();

  GetGlyphInfo(aString, aLength, tmpGlyphs, aFontCharset);

  if (tmpGlyphs->num_glyphs > 0) {
    gint i=0, glyphCt=0;
    for (i=0; i < tmpGlyphs->num_glyphs; i++, glyphCt++) {
      if (aIsWide)
         aGlyphs[glyphCt++]=(unsigned char)
                            ((tmpGlyphs->glyphs[i].glyph & 0xFF00) >> 8);
      aGlyphs[glyphCt]=(unsigned char)(tmpGlyphs->glyphs[i].glyph & 0x00FF);
    }
    *aOutLength = (PRSize)glyphCt;
  }
  pangolite_glyph_string_free(tmpGlyphs);
  return NS_OK;
}



NS_IMETHODIMP
nsULE::NextCluster(const PRUnichar *aString,
                   PRUint32        aLength,
                   const PRInt32   aIndex,
                   PRInt32         *aNextOffset)
{
  int mStart, mEnd;

  if (aIndex < 0) {
    *aNextOffset = 0;
    return NS_OK;
  }

  if (PRUint32(aIndex) >= aLength) {
    *aNextOffset = aLength;
    return NS_OK;
  }
  this->GetRangeOfCluster(aString, aLength, aIndex, &mStart, &mEnd);
  *aNextOffset = mEnd;
  return NS_OK;
}



NS_IMETHODIMP
nsULE::PrevCluster(const PRUnichar *aString,
                   PRUint32        aLength,
                   const PRInt32   aIndex,
                   PRInt32         *aPrevOffset)
{
  int                  gCt, pCluster, cCluster;
  PangoliteGlyphString *GlyphInfo=pangolite_glyph_string_new();

  if (aIndex <= 1) {
    *aPrevOffset = 0;
    return NS_OK;
  }
  pCluster=cCluster=0;
  GetGlyphInfo(aString, aLength, GlyphInfo, NULL);
  for (gCt=0; gCt < GlyphInfo->num_glyphs; gCt++) {

    if (GlyphInfo->glyphs[gCt].is_cluster_start != GLYPH_COMBINING)
       cCluster += GlyphInfo->glyphs[gCt].is_cluster_start;

    if (cCluster >= aIndex) {
       *aPrevOffset = pCluster;
       pangolite_glyph_string_free(GlyphInfo);
       return NS_OK;
    }
    pCluster = cCluster;
  }
  *aPrevOffset = pCluster;
  pangolite_glyph_string_free(GlyphInfo);
  return NS_OK;
}


NS_IMETHODIMP
nsULE::GetRangeOfCluster(const PRUnichar *aString,
                         PRUint32        aLength,
                         const PRInt32   aIndex,
                         PRInt32         *aStart,
                         PRInt32         *aEnd)
{
  PangoliteGlyphString *GlyphInfo=pangolite_glyph_string_new();
  int                  gCt=0;

  GetGlyphInfo(aString, aLength, GlyphInfo, NULL);

  *aStart=*aEnd=0;
  for (gCt=0; gCt < GlyphInfo->num_glyphs; gCt++) {

    if (GlyphInfo->glyphs[gCt].is_cluster_start != GLYPH_COMBINING)
       *aEnd += GlyphInfo->glyphs[gCt].is_cluster_start;

    if (*aEnd >= aIndex+1) {
      pangolite_glyph_string_free(GlyphInfo);
      return NS_OK;
    }
    *aStart = *aEnd;
  }
  *aEnd = aLength;
  pangolite_glyph_string_free(GlyphInfo);
  return NS_OK;
}
