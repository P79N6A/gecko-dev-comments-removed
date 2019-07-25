





































#ifndef NSFONTMETRICS__H__
#define NSFONTMETRICS__H__

#include "nsCOMPtr.h"
#include "nsCoord.h"
#include "nsFont.h"
#include "gfxFont.h"
#include "gfxTextRunCache.h"

class gfxFontGroup;
class gfxUserFontSet;
class nsIAtom;
class nsIDeviceContext;
class nsRenderingContext;
class nsString;
class nsThebesDeviceContext;
struct nsBoundingMetrics;
struct nsTextDimensions;




typedef void* nsFontHandle;



















class nsFontMetrics
{
public:
    nsFontMetrics();
    ~nsFontMetrics();

    NS_INLINE_DECL_REFCOUNTING(nsFontMetrics)

    





    nsresult Init(const nsFont& aFont, nsIAtom* aLanguage,
                  nsIDeviceContext *aContext,
                  gfxUserFontSet *aUserFontSet = nsnull);
    



    nsresult Destroy();
    


    nsresult GetXHeight(nscoord& aResult);
    




    nsresult GetSuperscriptOffset(nscoord& aResult);
    




    nsresult GetSubscriptOffset(nscoord& aResult);
    




    nsresult GetStrikeout(nscoord& aOffset, nscoord& aSize);
    




    nsresult GetUnderline(nscoord& aOffset, nscoord& aSize);
    






    nsresult GetHeight(nscoord &aHeight);
    



    nsresult GetInternalLeading(nscoord &aLeading);
    




    nsresult GetExternalLeading(nscoord &aLeading);
    



    nsresult GetEmHeight(nscoord &aHeight);
    


    nsresult GetEmAscent(nscoord &aAscent);
    


    nsresult GetEmDescent(nscoord &aDescent);
    



    nsresult GetMaxHeight(nscoord &aHeight);
    



    nsresult GetMaxAscent(nscoord &aAscent);
    



    nsresult GetMaxDescent(nscoord &aDescent);
    


    nsresult GetMaxAdvance(nscoord &aAdvance);
    



    const nsFont &Font() { return mFont; }
    


    nsresult GetLanguage(nsIAtom** aLanguage);
    


    nsresult GetFontHandle(nsFontHandle &aHandle);
    


    nsresult GetAveCharWidth(nscoord& aAveCharWidth);
    


    nsresult GetSpaceWidth(nscoord& aSpaceCharWidth);

    PRInt32 GetMaxStringLength();

    
    
    
    nsresult GetWidth(const char* aString, PRUint32 aLength, nscoord& aWidth,
                      nsRenderingContext *aContext);
    nsresult GetWidth(const PRUnichar* aString, PRUint32 aLength,
                      nscoord& aWidth, PRInt32 *aFontID,
                      nsRenderingContext *aContext);

    
    nsresult GetTextDimensions(const PRUnichar* aString,
                               PRUint32 aLength,
                               nsTextDimensions& aDimensions,
                               PRInt32* aFontID);
    nsresult GetTextDimensions(const char*         aString,
                               PRInt32             aLength,
                               PRInt32             aAvailWidth,
                               PRInt32*            aBreaks,
                               PRInt32             aNumBreaks,
                               nsTextDimensions&   aDimensions,
                               PRInt32&            aNumCharsFit,
                               nsTextDimensions&   aLastWordDimensions,
                               PRInt32*            aFontID);
    nsresult GetTextDimensions(const PRUnichar*    aString,
                               PRInt32             aLength,
                               PRInt32             aAvailWidth,
                               PRInt32*            aBreaks,
                               PRInt32             aNumBreaks,
                               nsTextDimensions&   aDimensions,
                               PRInt32&            aNumCharsFit,
                               nsTextDimensions&   aLastWordDimensions,
                               PRInt32*            aFontID);

    
    nsresult DrawString(const char *aString, PRUint32 aLength,
                        nscoord aX, nscoord aY,
                        const nscoord* aSpacing,
                        nsRenderingContext *aContext);
    nsresult DrawString(const PRUnichar* aString, PRUint32 aLength,
                        nscoord aX, nscoord aY,
                        PRInt32 aFontID,
                        const nscoord* aSpacing,
                        nsRenderingContext *aContext)
    {
        NS_ASSERTION(!aSpacing, "Spacing not supported here");
        return DrawString(aString, aLength, aX, aY, aContext, aContext);
    }
    nsresult DrawString(const PRUnichar* aString, PRUint32 aLength,
                        nscoord aX, nscoord aY,
                        nsRenderingContext *aContext,
                        nsRenderingContext *aTextRunConstructionContext);

#ifdef MOZ_MATHML
    
    
    nsresult GetBoundingMetrics(const char *aString, PRUint32 aLength,
                                nsRenderingContext *aContext,
                                nsBoundingMetrics &aBoundingMetrics);
    nsresult GetBoundingMetrics(const PRUnichar *aString,
                                PRUint32 aLength,
                                nsRenderingContext *aContext,
                                nsBoundingMetrics &aBoundingMetrics);
#endif 

    
    nsresult SetRightToLeftText(PRBool aIsRTL);
    PRBool GetRightToLeftText();
    void SetTextRunRTL(PRBool aIsRTL) { mTextRunRTL = aIsRTL; }

    gfxFontGroup* GetThebesFontGroup() { return mFontGroup; }

    gfxUserFontSet* GetUserFontSet();

    PRBool GetRightToLeftTextRunMode() {
        return mTextRunRTL;
    }

    PRInt32 AppUnitsPerDevPixel() { return mP2A; }

protected:
    const gfxFont::Metrics& GetMetrics() const;

    nsFont mFont;		
    nsRefPtr<gfxFontGroup> mFontGroup;
    gfxFontStyle *mFontStyle;
    nsThebesDeviceContext *mDeviceContext;
    nsCOMPtr<nsIAtom> mLanguage;
    PRInt32 mP2A;
    PRPackedBool mIsRightToLeft;
    PRPackedBool mTextRunRTL;
};

#endif 
