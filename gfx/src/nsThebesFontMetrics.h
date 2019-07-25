





































#ifndef NSTHEBESFONTMETRICS__H__
#define NSTHEBESFONTMETRICS__H__

#include "nsIThebesFontMetrics.h"
#include "nsCOMPtr.h"
#include "nsRenderingContext.h"
#include "gfxFont.h"
#include "gfxTextRunCache.h"

class nsIAtom;
class nsIDeviceContext;
class nsThebesDeviceContext;

class nsThebesFontMetrics : public nsIThebesFontMetrics
{
public:
    nsThebesFontMetrics();
    virtual ~nsThebesFontMetrics();

    NS_DECL_ISUPPORTS

    NS_IMETHOD  Init(const nsFont& aFont, nsIAtom* aLanguage,
                     nsIDeviceContext *aContext,
                     gfxUserFontSet *aUserFontSet = nsnull);
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
    NS_IMETHOD  GetLanguage(nsIAtom** aLanguage);
    NS_IMETHOD  GetFontHandle(nsFontHandle &aHandle);
    NS_IMETHOD  GetAveCharWidth(nscoord& aAveCharWidth);
    NS_IMETHOD  GetSpaceWidth(nscoord& aSpaceCharWidth);
    virtual PRInt32 GetMaxStringLength();


    virtual nsresult GetWidth(const char* aString, PRUint32 aLength, nscoord& aWidth,
                              nsRenderingContext *aContext);
    virtual nsresult GetWidth(const PRUnichar* aString, PRUint32 aLength,
                              nscoord& aWidth, PRInt32 *aFontID,
                              nsRenderingContext *aContext);

    
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
                                nsRenderingContext *aContext);
    virtual nsresult DrawString(const PRUnichar* aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                PRInt32 aFontID,
                                const nscoord* aSpacing,
                                nsRenderingContext *aContext)
    {
      NS_ASSERTION(!aSpacing, "Spacing not supported here");
      return DrawString(aString, aLength, aX, aY, aContext, aContext);
    }
    virtual nsresult DrawString(const PRUnichar* aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                nsRenderingContext *aContext,
                                nsRenderingContext *aTextRunConstructionContext);

#ifdef MOZ_MATHML
    
    
    virtual nsresult GetBoundingMetrics(const char *aString, PRUint32 aLength,
                                        nsRenderingContext *aContext,
                                        nsBoundingMetrics &aBoundingMetrics);
    virtual nsresult GetBoundingMetrics(const PRUnichar *aString,
                                        PRUint32 aLength,
                                        nsRenderingContext *aContext,
                                        nsBoundingMetrics &aBoundingMetrics);
#endif 

    
    virtual nsresult SetRightToLeftText(PRBool aIsRTL);
    virtual PRBool GetRightToLeftText();
    virtual void SetTextRunRTL(PRBool aIsRTL) { mTextRunRTL = aIsRTL; }

    virtual gfxFontGroup* GetThebesFontGroup() { return mFontGroup; }

    virtual gfxUserFontSet* GetUserFontSet();
    
    PRBool GetRightToLeftTextRunMode() {
        return mTextRunRTL;
    }

protected:

    const gfxFont::Metrics& GetMetrics() const;

    class AutoTextRun {
    public:
        AutoTextRun(nsThebesFontMetrics* aMetrics, nsRenderingContext* aRC,
                    const char* aString, PRInt32 aLength) {
            mTextRun = gfxTextRunCache::MakeTextRun(
                reinterpret_cast<const PRUint8*>(aString), aLength,
                aMetrics->mFontGroup, aRC->ThebesContext(),
                aMetrics->mP2A,
                ComputeFlags(aMetrics));
        }
        AutoTextRun(nsThebesFontMetrics* aMetrics, nsRenderingContext* aRC,
                    const PRUnichar* aString, PRInt32 aLength) {
            mTextRun = gfxTextRunCache::MakeTextRun(
                aString, aLength, aMetrics->mFontGroup,
                aRC->ThebesContext(),
                aMetrics->mP2A,
                ComputeFlags(aMetrics));
        }
        gfxTextRun* operator->() { return mTextRun.get(); }
        gfxTextRun* get() { return mTextRun.get(); }

    private:
        gfxTextRunCache::AutoTextRun mTextRun;
        
        static PRUint32 ComputeFlags(nsThebesFontMetrics* aMetrics) {
            PRUint32 flags = 0;
            if (aMetrics->GetRightToLeftTextRunMode()) {
                flags |= gfxTextRunFactory::TEXT_IS_RTL;
            }
            return flags;
        }
    };
    friend class AutoTextRun;

    nsRefPtr<gfxFontGroup> mFontGroup;
    gfxFontStyle *mFontStyle;

private:
    nsThebesDeviceContext *mDeviceContext;
    nsCOMPtr<nsIAtom> mLanguage;
    PRInt32 mP2A;
    PRPackedBool mIsRightToLeft;
    PRPackedBool mTextRunRTL;
};

#endif 
