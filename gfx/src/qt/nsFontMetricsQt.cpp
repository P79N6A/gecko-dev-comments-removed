







































#include "nsQuickSort.h"
#include "nsFontMetricsQt.h"
#include "nsFont.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsIServiceManager.h"
#include "nsISaveAsCharset.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsCOMPtr.h"
#include "nspr.h"
#include "nsHashtable.h"
#include "nsDrawingSurfaceQt.h"
#include "nsRenderingContextQt.h"
#include "nsILanguageAtomService.h"
#include "nsDrawingSurfaceQt.h"
#include "nsRenderingContextQt.h"

#include <qapplication.h>
#include <qfont.h>
#include <qfontdatabase.h>

#include "qtlog.h"

nsFontQt::nsFontQt(const nsFont &aFont, nsIAtom *aLangGroup, nsIDeviceContext *aContext)
{
    mLangGroup = aLangGroup;
    mDeviceContext = aContext;

    float a2d = aContext->AppUnitsToDevUnits();
    mPixelSize = NSToIntRound(a2d * aFont.size);

    if (mLangGroup) {
        nsCAutoString name("font.min-size.variable.");
        const char* langGroup = nsnull;
        mLangGroup->GetUTF8String(&langGroup);

        PRInt32 minimum = 0;
        nsresult res;
        nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &res);
        if (NS_SUCCEEDED(res)) {
            nsCOMPtr<nsIPrefBranch> branch;
            prefs->GetBranch(name.get(), getter_AddRefs(branch));
            branch->GetIntPref(langGroup, &minimum);
        }
        if (minimum < 0) {
            minimum = 0;
        }
        if (mPixelSize < minimum) {
            mPixelSize = minimum;
        }
    }

    font.setFamily(QString::fromUcs2(aFont.name.get()));
    font.setPixelSize(mPixelSize);
    font.setWeight(aFont.weight/10);
    font.setItalic(aFont.style != NS_FONT_STYLE_NORMAL);

    RealizeFont();
}

void nsFontQt::RealizeFont()
{
    QFontMetrics fm(font);

    float f = mDeviceContext->DevUnitsToAppUnits();

    mMaxAscent = nscoord(fm.ascent() * f) ;
    mMaxDescent = nscoord(fm.descent() * f);
    mMaxHeight = mMaxAscent + mMaxDescent;

    mEmHeight = nscoord(fm.height() * f);
    mMaxAdvance = nscoord(fm.maxWidth() * f);

    mAveCharWidth = nscoord(fm.width(QChar('x')) * f);

    mEmAscent = mMaxAscent;
    mEmDescent = mMaxDescent;

    mXHeight = NSToCoordRound(fm.boundingRect('x').height() * f);

    mUnderlineOffset = - nscoord(fm.underlinePos() * f);

    mUnderlineSize = NSToIntRound(fm.lineWidth() * f);

    mSuperscriptOffset = mXHeight;
    mSubscriptOffset = mXHeight;

    mStrikeoutOffset = nscoord(fm.strikeOutPos() * f);
    mStrikeoutSize = mUnderlineSize;

    mLeading = nscoord(fm.leading() * f);
    mSpaceWidth = nscoord(fm.width(QChar(' ')) * f);
}

nsFontMetricsQt::nsFontMetricsQt()
{
    qFont = 0;
}

nsFontMetricsQt::~nsFontMetricsQt()
{
    NS_ASSERTION(qFont == 0, "deleting non destroyed nsFontMetricsQt");
}

NS_IMPL_ISUPPORTS1(nsFontMetricsQt,nsIFontMetrics)

    NS_IMETHODIMP nsFontMetricsQt::Init(const nsFont &aFont, nsIAtom *aLangGroup,
                                        nsIDeviceContext *aContext)
{
    NS_ASSERTION(!(nsnull == aContext), "attempt to init fontmetrics with null device context");

    mFont = aFont;
    qFont = new nsFontQt(aFont, aLangGroup, aContext);
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::Destroy()
{
    delete qFont;
    qFont = 0;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::GetXHeight(nscoord &aResult)
{
    aResult = qFont->mXHeight;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::GetSuperscriptOffset(nscoord &aResult)
{
    aResult = qFont->mSuperscriptOffset;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::GetSubscriptOffset(nscoord &aResult)
{
    aResult = qFont->mSubscriptOffset;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::GetStrikeout(nscoord &aOffset,nscoord &aSize)
{
    aOffset = qFont->mStrikeoutOffset;
    aSize = qFont->mStrikeoutSize;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::GetUnderline(nscoord &aOffset,nscoord &aSize)
{
    aOffset = qFont->mUnderlineOffset;
    aSize = qFont->mUnderlineSize;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::GetHeight(nscoord &aHeight)
{
    aHeight = qFont->mMaxHeight;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::GetNormalLineHeight(nscoord &aHeight)
{
    aHeight = qFont->mEmHeight + qFont->mLeading;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::GetLeading(nscoord &aLeading)
{
    aLeading = qFont->mLeading;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::GetEmHeight(nscoord &aHeight)
{
    aHeight = qFont->mEmHeight;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::GetEmAscent(nscoord &aAscent)
{
    aAscent = qFont->mEmAscent;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::GetEmDescent(nscoord &aDescent)
{
    aDescent = qFont->mEmDescent;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::GetMaxHeight(nscoord &aHeight)
{
    aHeight = qFont->mMaxHeight;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::GetMaxAscent(nscoord &aAscent)
{
    aAscent = qFont->mMaxAscent;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::GetMaxDescent(nscoord &aDescent)
{
    aDescent = qFont->mMaxDescent;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::GetMaxAdvance(nscoord &aAdvance)
{
    aAdvance = qFont->mMaxAdvance;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::GetAveCharWidth(nscoord &aAveCharWidth)
{
    aAveCharWidth = qFont->mAveCharWidth;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::GetSpaceWidth(nscoord &aSpaceWidth)
{
    aSpaceWidth = qFont->mSpaceWidth;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQt::GetLangGroup(nsIAtom **aLangGroup)
{
    if (!aLangGroup) {
        return NS_ERROR_NULL_POINTER;
    }
    *aLangGroup = qFont->mLangGroup;
    NS_IF_ADDREF(*aLangGroup);
    return NS_OK;
}


NS_IMETHODIMP nsFontMetricsQt::GetFontHandle(nsFontHandle &aHandle)
{
    aHandle = (nsFontHandle)qFont;
    return NS_OK;
}

static nsresult EnumFonts(nsIAtom *aLangGroup,const char *aGeneric,
                          PRUint32 *aCount,PRUnichar ***aResult)
{
    *aResult = nsnull;
    *aCount = 0;
    
    QStringList qFamilies = QFontDatabase().families();
    int count = qFamilies.count();

    PRUnichar **array = (PRUnichar**)nsMemory::Alloc(count * sizeof(PRUnichar*));
    NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);

    int i = 0;
    for (QStringList::ConstIterator famIt = qFamilies.begin(); famIt != qFamilies.end(); ++famIt) {
        QString family = *famIt;
        array[i] = new PRUnichar[family.length()];
        if (array[i]) {
            memcpy(array[i], family.unicode(), family.length()*sizeof(PRUnichar));
            i++;
        }
        else {
            
            count--;
        }
    }

    *aCount = count;
    *aResult = array;
    return NS_OK;
}

nsFontEnumeratorQt::nsFontEnumeratorQt()
{
}

NS_IMPL_ISUPPORTS1(nsFontEnumeratorQt, nsIFontEnumerator)

NS_IMETHODIMP
nsFontEnumeratorQt::EnumerateAllFonts(PRUint32 *aCount,PRUnichar ***aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = nsnull;
    NS_ENSURE_ARG_POINTER(aCount);
    *aCount = 0;

    return EnumFonts(nsnull,nsnull,aCount,aResult);
}

NS_IMETHODIMP
nsFontEnumeratorQt::EnumerateFonts(const char *aLangGroup,const char *aGeneric,
                                   PRUint32 *aCount,PRUnichar ***aResult)
{
    nsresult res;

    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = nsnull;
    NS_ENSURE_ARG_POINTER(aCount);
    *aCount = 0;
    NS_ENSURE_ARG_POINTER(aGeneric);
    NS_ENSURE_ARG_POINTER(aLangGroup);

    nsIAtom *langGroup = NS_NewAtom(aLangGroup);
    res = EnumFonts(langGroup,aGeneric,aCount,aResult);
    NS_IF_RELEASE(langGroup);
    return(res);
}

NS_IMETHODIMP
nsFontEnumeratorQt::HaveFontFor(const char *aLangGroup,PRBool *aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(aLangGroup);

    *aResult = PR_TRUE; 
    
    return NS_OK;
}

NS_IMETHODIMP
nsFontEnumeratorQt::GetDefaultFont(const char *aLangGroup,
                                   const char *aGeneric,
                                   PRUnichar **aResult)
{
    
    

    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = nsnull;

    return NS_OK;
}

NS_IMETHODIMP
nsFontEnumeratorQt::UpdateFontList(PRBool *updateFontList)
{
    *updateFontList = PR_FALSE; 
    return NS_OK;
}
