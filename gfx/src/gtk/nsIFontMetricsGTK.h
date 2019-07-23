






































#ifndef __nsIFontMetricsGTK_h
#define __nsIFontMetricsGTK_h

#include "nsIFontMetrics.h"
#include "nsIRenderingContext.h"

#include "nsDrawingSurfaceGTK.h"

class nsRenderingContextGTK;

class nsIFontMetricsGTK : public nsIFontMetrics {
public:
    
    
    
    virtual nsresult GetWidth(const char* aString, PRUint32 aLength,
                              nscoord& aWidth,
                              nsRenderingContextGTK *aContext) = 0;
    
    virtual nsresult GetWidth(const PRUnichar* aString, PRUint32 aLength,
                              nscoord& aWidth, PRInt32 *aFontID,
                              nsRenderingContextGTK *aContext) = 0;

    
    virtual nsresult GetTextDimensions(const PRUnichar* aString,
                                       PRUint32 aLength,
                                       nsTextDimensions& aDimensions, 
                                       PRInt32* aFontID,
                                       nsRenderingContextGTK *aContext) = 0;
    virtual nsresult GetTextDimensions(const char*         aString,
                                       PRInt32             aLength,
                                       PRInt32             aAvailWidth,
                                       PRInt32*            aBreaks,
                                       PRInt32             aNumBreaks,
                                       nsTextDimensions&   aDimensions,
                                       PRInt32&            aNumCharsFit,
                                       nsTextDimensions&   aLastWordDimensions,
                                       PRInt32*            aFontID,
                                       nsRenderingContextGTK *aContext)=0;
    virtual nsresult GetTextDimensions(const PRUnichar*    aString,
                                       PRInt32             aLength,
                                       PRInt32             aAvailWidth,
                                       PRInt32*            aBreaks,
                                       PRInt32             aNumBreaks,
                                       nsTextDimensions&   aDimensions,
                                       PRInt32&            aNumCharsFit,
                                       nsTextDimensions&   aLastWordDimensions,
                                       PRInt32*            aFontID,
                                       nsRenderingContextGTK *aContext)=0;

    
    virtual nsresult DrawString(const char *aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                const nscoord* aSpacing,
                                nsRenderingContextGTK *aContext,
                                nsDrawingSurfaceGTK *aSurface) = 0;
    
    virtual nsresult DrawString(const PRUnichar* aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                PRInt32 aFontID,
                                const nscoord* aSpacing,
                                nsRenderingContextGTK *aContext,
                                nsDrawingSurfaceGTK *aSurface) = 0;

#ifdef MOZ_MATHML
    
    
    
    
    virtual nsresult GetBoundingMetrics(const char *aString, PRUint32 aLength,
                                        nsBoundingMetrics &aBoundingMetrics,
                                        nsRenderingContextGTK *aContext) = 0;
    
    virtual nsresult GetBoundingMetrics(const PRUnichar *aString,
                                        PRUint32 aLength,
                                        nsBoundingMetrics &aBoundingMetrics,
                                        PRInt32 *aFontID,
                                        nsRenderingContextGTK *aContext) = 0;
#endif 

    
    
    
    virtual GdkFont* GetCurrentGDKFont(void) = 0;

    
    virtual nsresult SetRightToLeftText(PRBool aIsRTL) = 0;
    virtual PRBool GetRightToLeftText() = 0;

    virtual nsresult GetClusterInfo(const PRUnichar *aText,
                                    PRUint32 aLength,
                                    PRUint8 *aClusterStarts) = 0;

    virtual PRInt32 GetPosition(const PRUnichar *aText,
                                PRUint32 aLength,
                                nsPoint aPt) = 0;

    virtual nsresult GetRangeWidth(const PRUnichar *aText,
                                   PRUint32 aLength,
                                   PRUint32 aStart,
                                   PRUint32 aEnd,
                                   PRUint32 &aWidth) = 0;
    virtual nsresult GetRangeWidth(const char *aText,
                                   PRUint32 aLength,
                                   PRUint32 aStart,
                                   PRUint32 aEnd,
                                   PRUint32 &aWidth) = 0;
    virtual PRInt32 GetMaxStringLength() = 0;
};

#endif 
