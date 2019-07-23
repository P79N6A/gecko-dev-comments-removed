




































#ifndef nsUnicodeRenderingToolkit_h__
#define nsUnicodeRenderingToolkit_h__

#include <UnicodeConverter.h>

#include "nsATSUIUtils.h"

#include "nsCOMPtr.h"
#include "nsISaveAsCharset.h"
#include "nsIRenderingContext.h"

class nsUnicodeFallbackCache;
class nsIDeviceContext;
class nsGraphicState;
class nsUnicodeFontMappingMac;

class nsUnicodeRenderingToolkit 
{
public:
  nsUnicodeRenderingToolkit() {};
  virtual ~nsUnicodeRenderingToolkit() {};

  nsresult PrepareToDraw(float aP2T, nsIDeviceContext* aContext, nsGraphicState* aGS, 
                         CGrafPtr aPort, PRBool aRightToLeftText);
  nsresult GetTextDimensions(const PRUnichar *aString, PRUint32 aLength, 
                             nsTextDimensions &aDimension, PRInt32 *aFontID);
  nsresult GetWidth(const PRUnichar *aString, PRUint32 aLength, nscoord &aWidth,
                    PRInt32 *aFontID);

  nsresult DrawString(const PRUnichar *aString, PRUint32 aLength, nscoord aX, nscoord aY,
                      PRInt32 aFontID,
                      const nscoord* aSpacing);
#ifdef MOZ_MATHML
  nsresult GetTextBoundingMetrics(const PRUnichar *aString, PRUint32 aLength,
                                  nsBoundingMetrics &aBoundingMetrics, PRInt32 *aFontID);
#endif 

private:  
  
  PRBool TECFallbackGetDimensions(const PRUnichar *pChar, nsTextDimensions& oWidth,
                                  short fontNum, nsUnicodeFontMappingMac& fontMapping);
  PRBool TECFallbackDrawChar(const PRUnichar *pChar, PRInt32 x, PRInt32 y, short& oWidth,
                             short fontNum, nsUnicodeFontMappingMac& fontMapping);
                  
  PRBool ATSUIFallbackGetDimensions(const PRUnichar *pChar, nsTextDimensions& oWidth, short fontNum,  
                                    short aSize, PRBool aBold, PRBool aItalic, nscolor aColor);
  PRBool ATSUIFallbackDrawChar(const PRUnichar *pChar, PRInt32 x, PRInt32 y, short& oWidth, short fontNum, 
                               short aSize, PRBool aBold, PRBool aItalic, nscolor aColor);
  PRBool SurrogateGetDimensions(const PRUnichar *aSurrogatePt, nsTextDimensions& oWidth, short fontNum,  
                                    short aSize, PRBool aBold, PRBool aItalic, nscolor aColor);
  PRBool SurrogateDrawChar(const PRUnichar *aSurrogatePt, PRInt32 x, PRInt32 y, short& oWidth, short fontNum, 
                               short aSize, PRBool aBold, PRBool aItalic, nscolor aColor);
  
  PRBool UPlusFallbackGetWidth(const PRUnichar *pChar, short& oWidth);
  PRBool UPlusFallbackDrawChar(const PRUnichar *pChar, PRInt32 x, PRInt32 y, short& oWidth);

  PRBool QuestionMarkFallbackGetWidth(const PRUnichar *pChar, short& oWidth);
  PRBool QuestionMarkFallbackDrawChar(const PRUnichar *pChar, PRInt32 x, PRInt32 y, short& oWidth);

  PRBool LatinFallbackGetWidth(const PRUnichar *pChar, short& oWidth);
  PRBool LatinFallbackDrawChar(const PRUnichar *pChar, PRInt32 x, PRInt32 y, short& oWidth);
  PRBool PrecomposeHangulFallbackGetWidth(const PRUnichar *pChar, short& oWidth,short koreanFont, short origFont);
  PRBool PrecomposeHangulFallbackDrawChar(const PRUnichar *pChar, PRInt32 x, PRInt32 y, short& oWidth,
                                          short koreanFont, short origFont);
  PRBool TransliterateFallbackGetWidth(const PRUnichar *pChar, short& oWidth);
  PRBool TransliterateFallbackDrawChar(const PRUnichar *pChar, PRInt32 x, PRInt32 y, short& oWidth);
  PRBool LoadTransliterator();
  
  void GetScriptTextWidth(const char* aText, ByteCount aLen, short& aWidth);
  void DrawScriptText(const char* aText, ByteCount aLen, PRInt32 x, PRInt32 y, short& aWidth);
  
  nsresult GetTextSegmentWidth(const PRUnichar *aString, PRUint32 aLength, short fontNum, 
                               nsUnicodeFontMappingMac& fontMapping, PRUint32& oWidth);
  nsresult GetTextSegmentDimensions(const PRUnichar *aString, PRUint32 aLength, short fontNum, 
                                    nsUnicodeFontMappingMac& fontMapping, nsTextDimensions& aDimension);
  nsresult DrawTextSegment(const PRUnichar *aString, PRUint32 aLength, short fontNum, 
                           nsUnicodeFontMappingMac& fontMapping, PRInt32 x, PRInt32 y, PRUint32& oWidth);

#ifdef MOZ_MATHML
  PRBool TECFallbackGetBoundingMetrics(const PRUnichar *pChar, nsBoundingMetrics& oBoundingMetrics,
  										short fontNum, nsUnicodeFontMappingMac& fontMapping);
  PRBool ATSUIFallbackGetBoundingMetrics(const PRUnichar *pChar, nsBoundingMetrics& oBoundingMetrics, short fontNum,
                                         short aSize, PRBool aBold, PRBool aItalic, nscolor aColor);
  PRBool SurrogateGetBoundingMetrics(const PRUnichar *aSurrogatePt, nsBoundingMetrics& oBoundingMetrics, short fontNum,
                                         short aSize, PRBool aBold, PRBool aItalic, nscolor aColor);
  void GetScriptTextBoundingMetrics(const char* aText, ByteCount aLen, ScriptCode aScript, nsBoundingMetrics& oBoundingMetrics);
  nsresult GetTextSegmentBoundingMetrics(const PRUnichar *aString, PRUint32 aLength, short fontNum,
                                         nsUnicodeFontMappingMac& fontMapping, nsBoundingMetrics& oBoundingMetrics);
#endif 

  nsUnicodeFallbackCache* GetTECFallbackCache();    
private:
  float mP2T; 
  nsIDeviceContext *mContext;
  nsGraphicState *mGS; 

  CGrafPtr mPort; 
  nsATSUIToolkit mATSUIToolkit;
  nsCOMPtr<nsISaveAsCharset> mTrans;
  PRBool mRightToLeftText;

};
#endif 
