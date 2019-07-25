




































#ifndef __nsIThebesFontMetrics_h
#define __nsIThebesFontMetrics_h

#include "nsIFontMetrics.h"

struct nsTextDimensions;
struct nsBoundingMetrics;
class gfxFontGroup;
class nsRenderingContext;

class nsIThebesFontMetrics : public nsIFontMetrics {
public:
    
    
    
    virtual nsresult GetWidth(const char* aString, PRUint32 aLength,
                              nscoord& aWidth, nsThebesRenderingContext *aContext) = 0;
    virtual nsresult GetWidth(const PRUnichar* aString, PRUint32 aLength,
                              nscoord& aWidth, PRInt32 *aFontID,
                              nsThebesRenderingContext *aContext) = 0;

    
    virtual nsresult GetTextDimensions(const PRUnichar* aString,
                                       PRUint32 aLength,
                                       nsTextDimensions& aDimensions, 
                                       PRInt32* aFontID) = 0;
    virtual nsresult GetTextDimensions(const char*         aString,
                                       PRInt32             aLength,
                                       PRInt32             aAvailWidth,
                                       PRInt32*            aBreaks,
                                       PRInt32             aNumBreaks,
                                       nsTextDimensions&   aDimensions,
                                       PRInt32&            aNumCharsFit,
                                       nsTextDimensions&   aLastWordDimensions,
                                       PRInt32*            aFontID) = 0;
    virtual nsresult GetTextDimensions(const PRUnichar*    aString,
                                       PRInt32             aLength,
                                       PRInt32             aAvailWidth,
                                       PRInt32*            aBreaks,
                                       PRInt32             aNumBreaks,
                                       nsTextDimensions&   aDimensions,
                                       PRInt32&            aNumCharsFit,
                                       nsTextDimensions&   aLastWordDimensions,
                                       PRInt32*            aFontID) = 0;

    
    virtual nsresult DrawString(const char *aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                const nscoord* aSpacing,
                                nsThebesRenderingContext *aContext) = 0;
    virtual nsresult DrawString(const PRUnichar* aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                PRInt32 aFontID,
                                const nscoord* aSpacing,
                                nsThebesRenderingContext *aContext) = 0;
    virtual nsresult DrawString(const PRUnichar* aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                nsIRenderingContext *aContext,
                                nsIRenderingContext *aTextRunConstructionContext) = 0;

#ifdef MOZ_MATHML
    
    
    
    
    virtual nsresult GetBoundingMetrics(const char *aString, PRUint32 aLength,
                                        nsThebesRenderingContext *aContext,
                                        nsBoundingMetrics &aBoundingMetrics) = 0;
    
    virtual nsresult GetBoundingMetrics(const PRUnichar *aString,
                                        PRUint32 aLength,
                                        nsThebesRenderingContext *aContext,
                                        nsBoundingMetrics &aBoundingMetrics) = 0;
#endif 

    
    virtual nsresult SetRightToLeftText(PRBool aIsRTL) = 0;
    virtual PRBool GetRightToLeftText() = 0;
    virtual void SetTextRunRTL(PRBool aIsRTL) = 0;

    virtual PRInt32 GetMaxStringLength() = 0;

    virtual gfxFontGroup* GetThebesFontGroup() = 0;

    
    
    virtual gfxUserFontSet* GetUserFontSet() = 0;
};

#endif 
