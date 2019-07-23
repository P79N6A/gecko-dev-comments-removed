





































#ifndef NSTHEBESFONTMETRICS__H__
#define NSTHEBESFONTMETRICS__H__

#include "nsIThebesFontMetrics.h"
#include "nsThebesRenderingContext.h"
#include "nsCOMPtr.h"
#include "nsThebesDeviceContext.h"
#include "nsIAtom.h"

#include "gfxFont.h"
#include "gfxTextRunCache.h"

class nsThebesFontMetrics : public nsIThebesFontMetrics
{
public:
    nsThebesFontMetrics();
    virtual ~nsThebesFontMetrics();

    NS_DECL_ISUPPORTS

    NS_IMETHOD  Init(const nsFont& aFont, nsIAtom* aLangGroup,
                     nsIDeviceContext *aContext);
    NS_IMETHOD  Destroy();
    NS_IMETHOD  GetXHeight(nscoord& aResult);
    NS_IMETHOD  GetSuperscriptOffset(nscoord& aResult);
    NS_IMETHOD  GetSubscriptOffset(nscoord& aResult);
    NS_IMETHOD  GetStrikeout(nscoord& aOffset, nscoord& aSize);
    NS_IMETHOD  GetUnderline(nscoord& aOffset, nscoord& aSize);
    NS_IMETHOD  GetHeight(nscoord &aHeight);
    NS_IMETHOD  GetInternalLeading(nscoord &aLeading);
    NS_IMETHOD  GetExternalLeading(nscoord &aLeading);
    NS_IMETHOD  GetEmHeight(nscoord &aHeight);
    NS_IMETHOD  GetEmAscent(nscoord &aAscent);
    NS_IMETHOD  GetEmDescent(nscoord &aDescent);
    NS_IMETHOD  GetMaxHeight(nscoord &aHeight);
    NS_IMETHOD  GetMaxAscent(nscoord &aAscent);
    NS_IMETHOD  GetMaxDescent(nscoord &aDescent);
    NS_IMETHOD  GetMaxAdvance(nscoord &aAdvance);
    NS_IMETHOD  GetLangGroup(nsIAtom** aLangGroup);
    NS_IMETHOD  GetFontHandle(nsFontHandle &aHandle);
    NS_IMETHOD  GetAveCharWidth(nscoord& aAveCharWidth);
    NS_IMETHOD  GetSpaceWidth(nscoord& aSpaceCharWidth);
    NS_IMETHOD  GetLeading(nscoord& aLeading);
    NS_IMETHOD  GetNormalLineHeight(nscoord& aLineHeight);
    virtual PRInt32 GetMaxStringLength();


    virtual nsresult GetWidth(const char* aString, PRUint32 aLength, nscoord& aWidth,
                              nsThebesRenderingContext *aContext);
    
    virtual nsresult GetWidth(const PRUnichar* aString, PRUint32 aLength,
                              nscoord& aWidth, PRInt32 *aFontID,
                              nsThebesRenderingContext *aContext);

    
    virtual nsresult GetTextDimensions(const PRUnichar* aString,
                                       PRUint32 aLength,
                                       nsTextDimensions& aDimensions, 
                                       PRInt32* aFontID);
    virtual nsresult GetTextDimensions(const char*         aString,
                                       PRInt32             aLength,
                                       PRInt32             aAvailWidth,
                                       PRInt32*            aBreaks,
                                       PRInt32             aNumBreaks,
                                       nsTextDimensions&   aDimensions,
                                       PRInt32&            aNumCharsFit,
                                       nsTextDimensions&   aLastWordDimensions,
                                       PRInt32*            aFontID);
    virtual nsresult GetTextDimensions(const PRUnichar*    aString,
                                       PRInt32             aLength,
                                       PRInt32             aAvailWidth,
                                       PRInt32*            aBreaks,
                                       PRInt32             aNumBreaks,
                                       nsTextDimensions&   aDimensions,
                                       PRInt32&            aNumCharsFit,
                                       nsTextDimensions&   aLastWordDimensions,
                                       PRInt32*            aFontID);

    
    virtual nsresult DrawString(const char *aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                const nscoord* aSpacing,
                                nsThebesRenderingContext *aContext);
    
    virtual nsresult DrawString(const PRUnichar* aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                PRInt32 aFontID,
                                const nscoord* aSpacing,
                                nsThebesRenderingContext *aContext);

#ifdef MOZ_MATHML
    
    
    
    
    virtual nsresult GetBoundingMetrics(const char *aString, PRUint32 aLength,
                                        nsBoundingMetrics &aBoundingMetrics);
    
    virtual nsresult GetBoundingMetrics(const PRUnichar *aString,
                                        PRUint32 aLength,
                                        nsBoundingMetrics &aBoundingMetrics,
                                        PRInt32 *aFontID);
#endif 

    
    virtual nsresult SetRightToLeftText(PRBool aIsRTL);
    virtual PRBool GetRightToLeftText();
    virtual void SetTextRunRTL(PRBool aIsRTL) { mTextRunRTL = aIsRTL; }

    virtual gfxFontGroup* GetThebesFontGroup() { return mFontGroup; }
    
    PRBool GetRightToLeftTextRunMode() {
        return mTextRunRTL;
    }

protected:

    const gfxFont::Metrics& GetMetrics() const;

    class AutoTextRun {
    public:
        AutoTextRun(nsThebesFontMetrics* aMetrics, nsIRenderingContext* aRC,
                    const char* aString, PRInt32 aLength, PRBool aEnableSpacing) {
            mTextRun = gfxTextRunCache::MakeTextRun(
                NS_REINTERPRET_CAST(const PRUint8*, aString), aLength,
                aMetrics->mFontGroup,
                NS_STATIC_CAST(gfxContext*, aRC->GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT)),
                aMetrics->mP2A,
                ComputeFlags(aMetrics, aEnableSpacing));
        }
        AutoTextRun(nsThebesFontMetrics* aMetrics, nsIRenderingContext* aRC,
                    const PRUnichar* aString, PRInt32 aLength, PRBool aEnableSpacing) {
            mTextRun = gfxTextRunCache::MakeTextRun(
                aString, aLength, aMetrics->mFontGroup,
                NS_STATIC_CAST(gfxContext*, aRC->GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT)),
                aMetrics->mP2A,
                ComputeFlags(aMetrics, aEnableSpacing));
        }
        gfxTextRun* operator->() { return mTextRun.get(); }
        gfxTextRun* get() { return mTextRun.get(); }

    private:
        gfxTextRunCache::AutoTextRun mTextRun;
        
        static PRUint32 ComputeFlags(nsThebesFontMetrics* aMetrics,
                                     PRBool aEnableSpacing) {
            PRUint32 flags = 0;
            if (aMetrics->GetRightToLeftTextRunMode()) {
                flags |= gfxTextRunFactory::TEXT_IS_RTL;
            }
            if (aEnableSpacing) {
                flags |= gfxTextRunFactory::TEXT_ENABLE_SPACING |
                         gfxTextRunFactory::TEXT_ABSOLUTE_SPACING |
                         gfxTextRunFactory::TEXT_ENABLE_NEGATIVE_SPACING;
            }
            return flags;
        }
    };
    friend class AutoTextRun;

    nsRefPtr<gfxFontGroup> mFontGroup;
    gfxFontStyle *mFontStyle;

private:
    nsThebesDeviceContext *mDeviceContext;
    nsCOMPtr<nsIAtom> mLangGroup;
    PRInt32 mP2A;
    PRPackedBool mIsRightToLeft;
    PRPackedBool mTextRunRTL;
};

#endif 
