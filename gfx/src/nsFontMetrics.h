





































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

    


    nsresult GetAveCharWidth(nscoord& aAveCharWidth);

    


    nsresult GetSpaceWidth(nscoord& aSpaceCharWidth);

    PRInt32 GetMaxStringLength();

    
    
    
    nsresult GetWidth(const char* aString, PRUint32 aLength, nscoord& aWidth,
                      nsRenderingContext *aContext);
    nsresult GetWidth(const PRUnichar* aString, PRUint32 aLength,
                      nscoord& aWidth, PRInt32 *aFontID,
                      nsRenderingContext *aContext);

    
    nsresult DrawString(const char *aString, PRUint32 aLength,
                        nscoord aX, nscoord aY,
                        const nscoord* aSpacing,
                        nsRenderingContext *aContext);
    nsresult DrawString(const PRUnichar* aString, PRUint32 aLength,
                        nscoord aX, nscoord aY,
                        nsRenderingContext *aContext,
                        nsRenderingContext *aTextRunConstructionContext);

#ifdef MOZ_MATHML
    nsresult GetBoundingMetrics(const PRUnichar *aString,
                                PRUint32 aLength,
                                nsRenderingContext *aContext,
                                nsBoundingMetrics &aBoundingMetrics);
#endif 

    
    void SetRightToLeftText(PRBool aIsRTL) { mIsRightToLeft = aIsRTL; }
    PRBool GetRightToLeftText() { return mIsRightToLeft; }

    void SetTextRunRTL(PRBool aIsRTL) { mTextRunRTL = aIsRTL; }
    PRBool GetRightToLeftTextRunMode() { return mTextRunRTL; }

    gfxFontGroup* GetThebesFontGroup() { return mFontGroup; }
    gfxUserFontSet* GetUserFontSet() { return mFontGroup->GetUserFontSet(); }

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
