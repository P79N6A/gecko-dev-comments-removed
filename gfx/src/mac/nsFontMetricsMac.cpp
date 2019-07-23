




































#include "nsCarbonHelpers.h"

#include "nsFontMetricsMac.h"
#include "nsDeviceContextMac.h"
#include "nsUnicodeFontMappingMac.h"
#include "nsUnicodeMappingUtil.h"
#include "nsGfxUtils.h"
#include "nsFontUtils.h"


nsFontMetricsMac :: nsFontMetricsMac()
{
  mFontNum = BAD_FONT_NUM;
  mFontMapping = nsnull;
}
  
nsFontMetricsMac :: ~nsFontMetricsMac()
{
  if (mContext) {
    
    mContext->FontMetricsDeleted(this);
    mContext = nsnull;
  }
  if (mFontMapping) {
    delete mFontMapping;
  }
}



NS_IMPL_ISUPPORTS1(nsFontMetricsMac, nsIFontMetrics)


NS_IMETHODIMP nsFontMetricsMac::Init(const nsFont& aFont, nsIAtom* aLangGroup, nsIDeviceContext* aCX)
{
  NS_ASSERTION(!(nsnull == aCX), "attempt to init fontmetrics with null device context");

  mFont = aFont;
  mLangGroup = aLangGroup;
  mContext = aCX;
  RealizeFont();
	
	TextStyle		theStyle;
	nsFontUtils::GetNativeTextStyle(*this, *mContext, theStyle);
	
  StTextStyleSetter styleSetter(theStyle);
  
  FontInfo fInfo;
  GetFontInfo(&fInfo);
  
  float  dev2app;
  dev2app = mContext->DevUnitsToAppUnits();

  mLeading    = NSToCoordRound(float(fInfo.leading) * dev2app);
  mEmAscent   = NSToCoordRound(float(fInfo.ascent) * dev2app);
  mEmDescent  = NSToCoordRound(float(fInfo.descent) * dev2app);
  mEmHeight   = mEmAscent + mEmDescent;

  mMaxHeight  = mEmHeight;
  mMaxAscent  = mEmAscent;
  mMaxDescent = mEmDescent;

  float maxCharWidth = float(::CharWidth('M'));	
  mMaxAdvance = NSToCoordRound(maxCharWidth * dev2app);
  
  
  mMaxStringLength = PR_MAX(1, (PRInt32)floor(32767.0 / maxCharWidth));

  mAveCharWidth = NSToCoordRound(float(::CharWidth('x')) * dev2app);	
  mSpaceWidth = NSToCoordRound(float(::CharWidth(' ')) * dev2app);

  Point frac;
  frac.h = frac.v = 1;
  unsigned char x = 'x';
  short ascent;
  if (noErr == ::OutlineMetrics(1, &x, frac, frac, &ascent, 0, 0, 0, 0))
    mXHeight = NSToCoordRound(float(ascent) * dev2app);
  else
    mXHeight = NSToCoordRound(float(mMaxAscent) * 0.71f); 

  return NS_OK;
}

nsUnicodeFontMappingMac* nsFontMetricsMac::GetUnicodeFontMapping()
{
  if (!mFontMapping)
  {
  	
  	
  	
  	nsAutoString langGroup;
  	if (mLangGroup)
  		mLangGroup->ToString(langGroup);
    else
      langGroup.AssignLiteral("ja");
      
  	nsString lang;
    mFontMapping = new nsUnicodeFontMappingMac(&mFont, mContext, langGroup, lang);
  }
  
	return mFontMapping;
}


static void MapGenericFamilyToFont(const nsString& aGenericFamily, nsString& aFontFace, ScriptCode aScriptCode)
{
  
  nsUnicodeMappingUtil* unicodeMappingUtil = nsUnicodeMappingUtil::GetSingleton();
  if (unicodeMappingUtil)
  {
    nsString*   foundFont = unicodeMappingUtil->GenericFontNameForScript(
          aScriptCode,
          unicodeMappingUtil->MapGenericFontNameType(aGenericFamily));
    if (foundFont)
    {
      aFontFace = *foundFont;
      return;
    }
  }
  
  NS_ASSERTION(0, "Failed to find a font");
  aFontFace.AssignLiteral("Times");
	
  


























}

struct FontEnumData {
  FontEnumData(nsIDeviceContext* aDC, nsString& aFaceName, ScriptCode aScriptCode)
    : mContext(aDC), mFaceName(aFaceName), mScriptCode(aScriptCode)
  {}
  nsIDeviceContext* mContext;
  nsString&         mFaceName;
  ScriptCode		mScriptCode;
};

static PRBool FontEnumCallback(const nsString& aFamily, PRBool aGeneric, void *aData)
{
  FontEnumData* data = (FontEnumData*)aData;
  if (aGeneric)
  {
    nsAutoString realFace;
    MapGenericFamilyToFont(aFamily, realFace, data->mScriptCode);
    data->mFaceName = realFace;
    return PR_FALSE;  
  }
  else
  {
    nsAutoString realFace;
    PRBool  aliased;
    data->mContext->GetLocalFontName(aFamily, realFace, aliased);
    if (aliased || (NS_OK == data->mContext->CheckFontExistence(realFace)))
    {
    	data->mFaceName = realFace;
      return PR_FALSE;  
    }
  }
  return PR_TRUE;
}

void nsFontMetricsMac::RealizeFont()
{
	nsAutoString	fontName;
	nsUnicodeMappingUtil	*unicodeMappingUtil;
	ScriptCode				theScriptCode;

	unicodeMappingUtil = nsUnicodeMappingUtil::GetSingleton ();
	if (unicodeMappingUtil)
	{
		nsAutoString	theLangGroupString;

		if (mLangGroup)
			mLangGroup->ToString(theLangGroupString);
		else
			theLangGroupString.AssignLiteral("ja");

		theScriptCode = unicodeMappingUtil->MapLangGroupToScriptCode(
		    NS_ConvertUTF16toUTF8(theLangGroupString).get());

	}
	else
		theScriptCode = GetScriptManagerVariable (smSysScript);

	FontEnumData  fontData(mContext, fontName, theScriptCode);
	mFont.EnumerateFamilies(FontEnumCallback, &fontData);
  
	nsDeviceContextMac::GetMacFontNumber(fontName, mFontNum);
}


NS_IMETHODIMP
nsFontMetricsMac::Destroy()
{
  mContext = nsnull;
  return NS_OK;
}



NS_IMETHODIMP
nsFontMetricsMac :: GetXHeight(nscoord& aResult)
{
  aResult = mXHeight;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsMac :: GetSuperscriptOffset(nscoord& aResult)
{
  float  dev2app;
  dev2app = mContext->DevUnitsToAppUnits();
  aResult = NSToCoordRound(float(mMaxAscent / 2) - dev2app);
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsMac :: GetSubscriptOffset(nscoord& aResult)
{
  float  dev2app;
  dev2app = mContext->DevUnitsToAppUnits();
  aResult = NSToCoordRound(float(mMaxAscent / 2) - dev2app);
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsMac :: GetStrikeout(nscoord& aOffset, nscoord& aSize)
{
  float  dev2app;
  dev2app = mContext->DevUnitsToAppUnits();
  aOffset = NSToCoordRound(float(mMaxAscent / 2) - dev2app);
  aSize = NSToCoordRound(dev2app);
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsMac :: GetUnderline(nscoord& aOffset, nscoord& aSize)
{
  float  dev2app;
  dev2app = mContext->DevUnitsToAppUnits();
  aOffset = -NSToCoordRound( dev2app );
  aSize   = NSToCoordRound( dev2app );
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsMac :: GetHeight(nscoord &aHeight)
{
  aHeight = mMaxHeight;
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsMac :: GetNormalLineHeight(nscoord &aHeight)
{
  aHeight = mEmHeight + mLeading; 
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsMac :: GetLeading(nscoord &aLeading)
{
  aLeading = mLeading;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsMac :: GetEmHeight(nscoord &aHeight)
{
  aHeight = mEmHeight;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsMac :: GetEmAscent(nscoord &aAscent)
{
  aAscent = mEmAscent;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsMac :: GetEmDescent(nscoord &aDescent)
{
  aDescent = mEmDescent;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsMac :: GetMaxHeight(nscoord &aHeight)
{
  aHeight = mMaxHeight;
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsMac :: GetMaxAscent(nscoord &aAscent)
{
  aAscent = mMaxAscent;
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsMac :: GetMaxDescent(nscoord &aDescent)
{
  aDescent = mMaxDescent;
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsMac :: GetMaxAdvance(nscoord &aAdvance)
{
  aAdvance = mMaxAdvance;
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsMac :: GetAveCharWidth(nscoord &aAveCharWidth)
{
  aAveCharWidth = mAveCharWidth;
  return NS_OK;
}

PRInt32 nsFontMetricsMac::GetMaxStringLength()
{
  return mMaxStringLength;
}

nsresult nsFontMetricsMac :: GetSpaceWidth(nscoord &aSpaceWidth)
{
  aSpaceWidth = mSpaceWidth;
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsMac::GetLangGroup(nsIAtom** aLangGroup)
{
  if (!aLangGroup) {
    return NS_ERROR_NULL_POINTER;
  }

  *aLangGroup = mLangGroup;
  NS_IF_ADDREF(*aLangGroup);

  return NS_OK;
}


NS_IMETHODIMP nsFontMetricsMac :: GetWidths(const nscoord *&aWidths)
{
  return NS_ERROR_NOT_IMPLEMENTED;	
}

NS_IMETHODIMP nsFontMetricsMac :: GetFontHandle(nsFontHandle &aHandle)
{
	
	
	
	
	
	NS_PRECONDITION(mFontNum != BAD_FONT_NUM, "Font metrics have not been initialized");
	
	
	
	aHandle = (nsFontHandle)mFontNum;
	return NS_OK;
}

