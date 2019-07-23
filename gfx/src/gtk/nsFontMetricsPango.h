






































#include "nsIFontMetrics.h"
#include "nsIFontEnumerator.h"
#include "nsCRT.h"
#include "nsIAtom.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "nsIFontMetricsGTK.h"

#include <pango/pango.h>

class nsFontMetricsPango : public nsIFontMetricsGTK
{
public:
    nsFontMetricsPango();
    virtual ~nsFontMetricsPango();

    NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

    
    NS_DECL_ISUPPORTS

    
    NS_IMETHOD  Init                 (const nsFont& aFont, nsIAtom* aLangGroup,
                                      nsIDeviceContext *aContext);
    NS_IMETHOD  Destroy();
    NS_IMETHOD  GetLangGroup         (nsIAtom** aLangGroup);
    NS_IMETHOD  GetFontHandle        (nsFontHandle &aHandle);

    NS_IMETHOD  GetXHeight           (nscoord& aResult)
                                     { aResult = mXHeight; return NS_OK; };

    NS_IMETHOD GetSuperscriptOffset  (nscoord& aResult)
                                     { aResult = mSuperscriptOffset;
                                       return NS_OK; };

    NS_IMETHOD GetSubscriptOffset    (nscoord& aResult)
                                     { aResult = mSubscriptOffset;
                                       return NS_OK; };
                              
    NS_IMETHOD GetStrikeout          (nscoord& aOffset, nscoord& aSize)
                                     { aOffset = mStrikeoutOffset;
                                       aSize = mStrikeoutSize; 
                                       return NS_OK; };

    NS_IMETHOD GetUnderline          (nscoord& aOffset, nscoord& aSize)
                                     { aOffset = mUnderlineOffset;
                                       aSize = mUnderlineSize; 
                                       return NS_OK; };

    NS_IMETHOD GetHeight             (nscoord &aHeight)
                                     { aHeight = mMaxHeight; 
                                       return NS_OK; };

    NS_IMETHOD GetNormalLineHeight   (nscoord &aHeight)
                                     { aHeight = mEmHeight + mLeading;
                                       return NS_OK; };

    NS_IMETHOD GetLeading            (nscoord &aLeading)
                                     { aLeading = mLeading; 
                                       return NS_OK; };

    NS_IMETHOD GetEmHeight           (nscoord &aHeight)
                                     { aHeight = mEmHeight; 
                                       return NS_OK; };

    NS_IMETHOD GetEmAscent           (nscoord &aAscent)
                                     { aAscent = mEmAscent;
                                       return NS_OK; };

    NS_IMETHOD GetEmDescent          (nscoord &aDescent)
                                     { aDescent = mEmDescent;
                                       return NS_OK; };

    NS_IMETHOD GetMaxHeight          (nscoord &aHeight)
                                     { aHeight = mMaxHeight;
                                       return NS_OK; };

    NS_IMETHOD GetMaxAscent          (nscoord &aAscent)
                                     { aAscent = mMaxAscent;
                                       return NS_OK; };

    NS_IMETHOD GetMaxDescent         (nscoord &aDescent)
                                     { aDescent = mMaxDescent;
                                       return NS_OK; };

    NS_IMETHOD GetMaxAdvance         (nscoord &aAdvance)
                                     { aAdvance = mMaxAdvance;
                                       return NS_OK; };

    NS_IMETHOD GetSpaceWidth         (nscoord &aSpaceCharWidth)
                                     { aSpaceCharWidth = mSpaceWidth;
                                       return NS_OK; };

    NS_IMETHOD GetAveCharWidth       (nscoord &aAveCharWidth)
                                     { aAveCharWidth = mAveCharWidth;
                                       return NS_OK; };

    PRInt32 GetMaxStringLength() { return mMaxStringLength; }

    
    virtual nsresult GetWidth(const char* aString, PRUint32 aLength,
                              nscoord& aWidth,
                              nsRenderingContextGTK *aContext);
    virtual nsresult GetWidth(const PRUnichar* aString, PRUint32 aLength,
                              nscoord& aWidth, PRInt32 *aFontID,
                              nsRenderingContextGTK *aContext);

    virtual nsresult GetTextDimensions(const PRUnichar* aString,
                                       PRUint32 aLength,
                                       nsTextDimensions& aDimensions, 
                                       PRInt32* aFontID,
                                       nsRenderingContextGTK *aContext);
    virtual nsresult GetTextDimensions(const char*         aString,
                                       PRInt32             aLength,
                                       PRInt32             aAvailWidth,
                                       PRInt32*            aBreaks,
                                       PRInt32             aNumBreaks,
                                       nsTextDimensions&   aDimensions,
                                       PRInt32&            aNumCharsFit,
                                       nsTextDimensions&   aLastWordDimensions,
                                       PRInt32*            aFontID,
                                       nsRenderingContextGTK *aContext);
    virtual nsresult GetTextDimensions(const PRUnichar*    aString,
                                       PRInt32             aLength,
                                       PRInt32             aAvailWidth,
                                       PRInt32*            aBreaks,
                                       PRInt32             aNumBreaks,
                                       nsTextDimensions&   aDimensions,
                                       PRInt32&            aNumCharsFit,
                                       nsTextDimensions&   aLastWordDimensions,
                                       PRInt32*            aFontID,
                                       nsRenderingContextGTK *aContext);

    virtual nsresult DrawString(const char *aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                const nscoord* aSpacing,
                                nsRenderingContextGTK *aContext,
                                nsDrawingSurfaceGTK *aSurface);
    virtual nsresult DrawString(const PRUnichar* aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                PRInt32 aFontID,
                                const nscoord* aSpacing,
                                nsRenderingContextGTK *aContext,
                                nsDrawingSurfaceGTK *aSurface);

#ifdef MOZ_MATHML
    virtual nsresult GetBoundingMetrics(const char *aString, PRUint32 aLength,
                                        nsBoundingMetrics &aBoundingMetrics,
                                        nsRenderingContextGTK *aContext);
    virtual nsresult GetBoundingMetrics(const PRUnichar *aString,
                                        PRUint32 aLength,
                                        nsBoundingMetrics &aBoundingMetrics,
                                        PRInt32 *aFontID,
                                        nsRenderingContextGTK *aContext);
#endif 

    virtual GdkFont* GetCurrentGDKFont(void);

    virtual nsresult SetRightToLeftText(PRBool aIsRTL);
    virtual PRBool GetRightToLeftText();

    virtual nsresult GetClusterInfo(const PRUnichar *aText,
                                    PRUint32 aLength,
                                    PRUint8 *aClusterStarts);

    virtual PRInt32 GetPosition(const PRUnichar *aText,
                                PRUint32 aLength,
                                nsPoint aPt);

    virtual nsresult GetRangeWidth(const PRUnichar *aText,
                                   PRUint32 aLength,
                                   PRUint32 aStart,
                                   PRUint32 aEnd,
                                   PRUint32 &aWidth);

    virtual nsresult GetRangeWidth(const char *aText,
                                   PRUint32 aLength,
                                   PRUint32 aStart,
                                   PRUint32 aEnd,
                                   PRUint32 &aWidth);

    
    static PRUint32    GetHints     (void);

    
    static nsresult FamilyExists    (nsIDeviceContext *aDevice,
                                     const nsString &aName);

private:

    
    nsCStringArray       mFontList;
    nsAutoVoidArray      mFontIsGeneric;

    nsIDeviceContext    *mDeviceContext;
    nsCOMPtr<nsIAtom>    mLangGroup;
    nsCString           *mGenericFont;
    float                mPointSize;

    nsCAutoString        mDefaultFont;

    
    PangoFontDescription *mPangoFontDesc;
    PangoContext         *mPangoContext;
    PangoContext         *mLTRPangoContext;
    PangoContext         *mRTLPangoContext;
    PangoAttrList        *mPangoAttrList;
    PRBool                mIsRTL;

    
    nscoord                  mXHeight;
    nscoord                  mSuperscriptOffset;
    nscoord                  mSubscriptOffset;
    nscoord                  mStrikeoutOffset;
    nscoord                  mStrikeoutSize;
    nscoord                  mUnderlineOffset;
    nscoord                  mUnderlineSize;
    nscoord                  mMaxHeight;
    nscoord                  mLeading;
    nscoord                  mEmHeight;
    nscoord                  mEmAscent;
    nscoord                  mEmDescent;
    nscoord                  mMaxAscent;
    nscoord                  mMaxDescent;
    nscoord                  mMaxAdvance;
    nscoord                  mSpaceWidth;
    nscoord                  mPangoSpaceWidth;
    nscoord                  mAveCharWidth;
    PRInt32                  mMaxStringLength;

    
    nsresult RealizeFont(void);
    nsresult CacheFontMetrics(void);

    static PRBool EnumFontCallback(const nsString &aFamily,
                                   PRBool aIsGeneric, void *aData);

    void     DrawStringSlowly(const gchar *aText,
                              const PRUnichar *aOrigString,
                              PRUint32 aLength,
                              GdkDrawable *aDrawable,
                              GdkGC *aGC, gint aX, gint aY,
                              PangoLayoutLine *aLine,
                              const nscoord *aSpacing);

    nsresult GetTextDimensionsInternal(const gchar*        aString,
                                       PRInt32             aLength,
                                       PRInt32             aAvailWidth,
                                       PRInt32*            aBreaks,
                                       PRInt32             aNumBreaks,
                                       nsTextDimensions&   aDimensions,
                                       PRInt32&            aNumCharsFit,
                                       nsTextDimensions&   aLastWordDimensions,
                                       nsRenderingContextGTK *aContext);

    void FixupSpaceWidths (PangoLayout *aLayout, const char *aString);
};

class nsFontEnumeratorPango : public nsIFontEnumerator
{
public:
    nsFontEnumeratorPango();
    NS_DECL_ISUPPORTS
    NS_DECL_NSIFONTENUMERATOR
};
