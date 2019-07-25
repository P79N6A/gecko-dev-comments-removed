





































#ifndef NSFONTMETRICS__H__
#define NSFONTMETRICS__H__

#include "nsCOMPtr.h"
#include "nsCoord.h"
#include "nsFont.h"
#include "gfxFont.h"

class nsIAtom;
class nsIDeviceContext;
class nsRenderingContext;
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

    



    void Destroy();

    


    nscoord XHeight();

    




    nscoord SuperscriptOffset();

    




    nscoord SubscriptOffset();

    




    void GetStrikeout(nscoord& aOffset, nscoord& aSize);

    




    void GetUnderline(nscoord& aOffset, nscoord& aSize);

    




    nscoord InternalLeading();

    




    nscoord ExternalLeading();

    



    nscoord EmHeight();

    


    nscoord EmAscent();

    


    nscoord EmDescent();

    



    nscoord MaxHeight();

    



    nscoord MaxAscent();

    



    nscoord MaxDescent();

    


    nscoord MaxAdvance();

    


    nscoord AveCharWidth();

    


    nscoord SpaceWidth();

    



    const nsFont &Font() { return mFont; }

    


    already_AddRefed<nsIAtom> GetLanguage();

    PRInt32 GetMaxStringLength();

    
    
    
    nscoord GetWidth(const char* aString, PRUint32 aLength,
                     nsRenderingContext *aContext);
    nscoord GetWidth(const PRUnichar* aString, PRUint32 aLength,
                     nsRenderingContext *aContext);

    
    void DrawString(const char *aString, PRUint32 aLength,
                    nscoord aX, nscoord aY,
                    nsRenderingContext *aContext);
    void DrawString(const PRUnichar* aString, PRUint32 aLength,
                    nscoord aX, nscoord aY,
                    nsRenderingContext *aContext,
                    nsRenderingContext *aTextRunConstructionContext);

#ifdef MOZ_MATHML
    nsBoundingMetrics GetBoundingMetrics(const PRUnichar *aString,
                                         PRUint32 aLength,
                                         nsRenderingContext *aContext);
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
