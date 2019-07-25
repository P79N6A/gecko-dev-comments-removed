





































#ifndef NSFONTMETRICS__H__
#define NSFONTMETRICS__H__

#include "nsCOMPtr.h"
#include "nsCoord.h"
#include "nsFont.h"
#include "gfxFont.h"

class nsIAtom;
class nsDeviceContext;
class nsRenderingContext;
struct nsBoundingMetrics;



















class nsFontMetrics
{
public:
    nsFontMetrics();
    ~nsFontMetrics();

    NS_INLINE_DECL_REFCOUNTING(nsFontMetrics)

    





    nsresult Init(const nsFont& aFont, nsIAtom* aLanguage,
                  nsDeviceContext *aContext,
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

    


    nsIAtom* Language() { return mLanguage; }

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

    nsBoundingMetrics GetBoundingMetrics(const PRUnichar *aString,
                                         PRUint32 aLength,
                                         nsRenderingContext *aContext);

    void SetTextRunRTL(PRBool aIsRTL) { mTextRunRTL = aIsRTL; }
    PRBool GetTextRunRTL() { return mTextRunRTL; }

    gfxFontGroup* GetThebesFontGroup() { return mFontGroup; }
    gfxUserFontSet* GetUserFontSet() { return mFontGroup->GetUserFontSet(); }

    PRInt32 AppUnitsPerDevPixel() { return mP2A; }

protected:
    const gfxFont::Metrics& GetMetrics() const;

    nsFont mFont;
    nsRefPtr<gfxFontGroup> mFontGroup;
    nsCOMPtr<nsIAtom> mLanguage;
    nsDeviceContext *mDeviceContext;
    PRInt32 mP2A;
    PRPackedBool mTextRunRTL;
};

#endif 
