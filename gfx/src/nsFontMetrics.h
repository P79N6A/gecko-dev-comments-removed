




#ifndef NSFONTMETRICS__H__
#define NSFONTMETRICS__H__

#include <stdint.h>                     
#include <sys/types.h>                  
#include "gfxFont.h"                    
#include "mozilla/Assertions.h"         
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsCoord.h"                    
#include "nsError.h"                    
#include "nsFont.h"                     
#include "nsISupports.h"                
#include "nscore.h"                     

class gfxUserFontSet;
class gfxTextPerfMetrics;
class nsDeviceContext;
class nsIAtom;
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
                  gfxUserFontSet *aUserFontSet,
                  gfxTextPerfMetrics *aTextPerf);

    



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

    int32_t GetMaxStringLength();

    
    
    
    nscoord GetWidth(const char* aString, uint32_t aLength,
                     nsRenderingContext *aContext);
    nscoord GetWidth(const PRUnichar* aString, uint32_t aLength,
                     nsRenderingContext *aContext);

    
    void DrawString(const char *aString, uint32_t aLength,
                    nscoord aX, nscoord aY,
                    nsRenderingContext *aContext);
    void DrawString(const PRUnichar* aString, uint32_t aLength,
                    nscoord aX, nscoord aY,
                    nsRenderingContext *aContext,
                    nsRenderingContext *aTextRunConstructionContext);

    nsBoundingMetrics GetBoundingMetrics(const PRUnichar *aString,
                                         uint32_t aLength,
                                         nsRenderingContext *aContext);

    
    
    nsBoundingMetrics GetInkBoundsForVisualOverflow(const PRUnichar *aString,
                                                    uint32_t aLength,
                                                    nsRenderingContext *aContext);

    void SetTextRunRTL(bool aIsRTL) { mTextRunRTL = aIsRTL; }
    bool GetTextRunRTL() { return mTextRunRTL; }

    gfxFontGroup* GetThebesFontGroup() { return mFontGroup; }
    gfxUserFontSet* GetUserFontSet() { return mFontGroup->GetUserFontSet(); }

    int32_t AppUnitsPerDevPixel() { return mP2A; }

protected:
    const gfxFont::Metrics& GetMetrics() const;

    nsFont mFont;
    nsRefPtr<gfxFontGroup> mFontGroup;
    nsCOMPtr<nsIAtom> mLanguage;
    nsDeviceContext *mDeviceContext;
    int32_t mP2A;
    bool mTextRunRTL;
};

#endif 
