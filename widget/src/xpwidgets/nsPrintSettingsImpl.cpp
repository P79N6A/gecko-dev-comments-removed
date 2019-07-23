




































#include "nsPrintSettingsImpl.h"
#include "nsCoord.h"
#include "nsUnitConversion.h"
#include "nsReadableUtils.h"
#include "nsIPrintSession.h"

NS_IMPL_ISUPPORTS1(nsPrintSettings, nsIPrintSettings)





nsPrintSettings::nsPrintSettings() :
  mPrintOptions(0L),
  mPrintRange(kRangeAllPages),
  mStartPageNum(1),
  mEndPageNum(1),
  mScaling(1.0),
  mPrintBGColors(PR_FALSE),
  mPrintBGImages(PR_FALSE),
  mPrintFrameTypeUsage(kUseInternalDefault),
  mPrintFrameType(kFramesAsIs),
  mHowToEnableFrameUI(kFrameEnableNone),
  mIsCancelled(PR_FALSE),
  mPrintSilent(PR_FALSE),
	mPrintPreview(PR_FALSE),
  mShrinkToFit(PR_TRUE),
  mShowPrintProgress(PR_TRUE),
  mPrintPageDelay(500),
  mPaperData(0),
  mPaperSizeType(kPaperSizeDefined),
  mPaperWidth(8.5),
  mPaperHeight(11.0),
  mPaperSizeUnit(kPaperSizeInches),
  mPrintReversed(PR_FALSE),
  mPrintInColor(PR_TRUE),
  mOrientation(kPortraitOrientation),
  mNumCopies(1),
  mPrintToFile(PR_FALSE),
  mOutputFormat(kOutputFormatNative),
  mIsInitedFromPrinter(PR_FALSE),
  mIsInitedFromPrefs(PR_FALSE)
{

  
  nscoord halfInch = NS_INCHES_TO_TWIPS(0.5);
  mMargin.SizeTo(halfInch, halfInch, halfInch, halfInch);

  mPrintOptions = kPrintOddPages | kPrintEvenPages;

  mHeaderStrs[0].AssignLiteral("&T");
  mHeaderStrs[2].AssignLiteral("&U");

  mFooterStrs[0].AssignLiteral("&PT"); 
  mFooterStrs[2].AssignLiteral("&D");

}





nsPrintSettings::nsPrintSettings(const nsPrintSettings& aPS)
{
  *this = aPS;
}





nsPrintSettings::~nsPrintSettings()
{
}


NS_IMETHODIMP nsPrintSettings::GetPrintSession(nsIPrintSession **aPrintSession)
{
  NS_ENSURE_ARG_POINTER(aPrintSession);
  *aPrintSession = nsnull;
  
  nsCOMPtr<nsIPrintSession> session = do_QueryReferent(mSession);
  if (!session)
    return NS_ERROR_NOT_INITIALIZED;
  *aPrintSession = session;
  NS_ADDREF(*aPrintSession);
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPrintSession(nsIPrintSession *aPrintSession)
{
  
  
  NS_ENSURE_ARG(aPrintSession);
  
  mSession = do_GetWeakReference(aPrintSession);
  if (!mSession) {
    
    
    NS_ERROR("Could not get a weak reference from aPrintSession");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetStartPageRange(PRInt32 *aStartPageRange)
{
  
  *aStartPageRange = mStartPageNum;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetStartPageRange(PRInt32 aStartPageRange)
{
  mStartPageNum = aStartPageRange;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetEndPageRange(PRInt32 *aEndPageRange)
{
  
  *aEndPageRange = mEndPageNum;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetEndPageRange(PRInt32 aEndPageRange)
{
  mEndPageNum = aEndPageRange;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetPrintReversed(PRBool *aPrintReversed)
{
  
  *aPrintReversed = mPrintReversed;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPrintReversed(PRBool aPrintReversed)
{
  mPrintReversed = aPrintReversed;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetPrintInColor(PRBool *aPrintInColor)
{
  
  *aPrintInColor = mPrintInColor;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPrintInColor(PRBool aPrintInColor)
{
  mPrintInColor = aPrintInColor;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetOrientation(PRInt32 *aOrientation)
{
  NS_ENSURE_ARG_POINTER(aOrientation);
  *aOrientation = mOrientation;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetOrientation(PRInt32 aOrientation)
{
  mOrientation = aOrientation;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetColorspace(PRUnichar * *aColorspace)
{
  NS_ENSURE_ARG_POINTER(aColorspace);
  if (!mColorspace.IsEmpty()) {
    *aColorspace = ToNewUnicode(mColorspace);
  } else {
    *aColorspace = nsnull;
  }
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetColorspace(const PRUnichar * aColorspace)
{
  if (aColorspace) {
    mColorspace = aColorspace;
  } else {
    mColorspace.SetLength(0);
  }
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetResolutionName(PRUnichar * *aResolutionName)
{
  NS_ENSURE_ARG_POINTER(aResolutionName);
  if (!mResolutionName.IsEmpty()) {
    *aResolutionName = ToNewUnicode(mResolutionName);
  } else {
    *aResolutionName = nsnull;
  }
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetResolutionName(const PRUnichar * aResolutionName)
{
  if (aResolutionName) {
    mResolutionName = aResolutionName;
  } else {
    mResolutionName.SetLength(0);
  }
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetDownloadFonts(PRBool *aDownloadFonts)
{
  
  *aDownloadFonts = mDownloadFonts;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetDownloadFonts(PRBool aDownloadFonts)
{
  mDownloadFonts = aDownloadFonts;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetPrinterName(PRUnichar * *aPrinter)
{
   NS_ENSURE_ARG_POINTER(aPrinter);

   *aPrinter = ToNewUnicode(mPrinter);
   NS_ENSURE_TRUE(*aPrinter, NS_ERROR_OUT_OF_MEMORY);

   return NS_OK;
}

NS_IMETHODIMP nsPrintSettings::SetPrinterName(const PRUnichar * aPrinter)
{
  if (!aPrinter || !mPrinter.Equals(aPrinter)) {
    mIsInitedFromPrinter = PR_FALSE;
    mIsInitedFromPrefs   = PR_FALSE;
  }

  mPrinter.Assign(aPrinter);
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetNumCopies(PRInt32 *aNumCopies)
{
  NS_ENSURE_ARG_POINTER(aNumCopies);
  *aNumCopies = mNumCopies;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetNumCopies(PRInt32 aNumCopies)
{
  mNumCopies = aNumCopies;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetPrintCommand(PRUnichar * *aPrintCommand)
{
  
  *aPrintCommand = ToNewUnicode(mPrintCommand);
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPrintCommand(const PRUnichar * aPrintCommand)
{
  if (aPrintCommand) {
    mPrintCommand = aPrintCommand;
  } else {
    mPrintCommand.SetLength(0);
  }
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetPrintToFile(PRBool *aPrintToFile)
{
  
  *aPrintToFile = mPrintToFile;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPrintToFile(PRBool aPrintToFile)
{
  mPrintToFile = aPrintToFile;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetToFileName(PRUnichar * *aToFileName)
{
  
  *aToFileName = ToNewUnicode(mToFileName);
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetToFileName(const PRUnichar * aToFileName)
{
  if (aToFileName) {
    mToFileName = aToFileName;
  } else {
    mToFileName.SetLength(0);
  }
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetOutputFormat(PRInt16 *aOutputFormat)
{
  NS_ENSURE_ARG_POINTER(aOutputFormat);
  *aOutputFormat = mOutputFormat;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetOutputFormat(PRInt16 aOutputFormat)
{
  mOutputFormat = aOutputFormat;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetPrintPageDelay(PRInt32 *aPrintPageDelay)
{
  *aPrintPageDelay = mPrintPageDelay;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPrintPageDelay(PRInt32 aPrintPageDelay)
{
  mPrintPageDelay = aPrintPageDelay;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetIsInitializedFromPrinter(PRBool *aIsInitializedFromPrinter)
{
  NS_ENSURE_ARG_POINTER(aIsInitializedFromPrinter);
  *aIsInitializedFromPrinter = (PRBool)mIsInitedFromPrinter;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetIsInitializedFromPrinter(PRBool aIsInitializedFromPrinter)
{
  mIsInitedFromPrinter = (PRPackedBool)aIsInitializedFromPrinter;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetIsInitializedFromPrefs(PRBool *aInitializedFromPrefs)
{
  NS_ENSURE_ARG_POINTER(aInitializedFromPrefs);
  *aInitializedFromPrefs = (PRBool)mIsInitedFromPrefs;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetIsInitializedFromPrefs(PRBool aInitializedFromPrefs)
{
  mIsInitedFromPrefs = (PRPackedBool)aInitializedFromPrefs;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetMarginTop(double *aMarginTop)
{
  NS_ENSURE_ARG_POINTER(aMarginTop);
  *aMarginTop = NS_TWIPS_TO_INCHES(mMargin.top);
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetMarginTop(double aMarginTop)
{
  mMargin.top = NS_INCHES_TO_TWIPS(float(aMarginTop));
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetMarginLeft(double *aMarginLeft)
{
  NS_ENSURE_ARG_POINTER(aMarginLeft);
  *aMarginLeft = NS_TWIPS_TO_INCHES(mMargin.left);
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetMarginLeft(double aMarginLeft)
{
  mMargin.left = NS_INCHES_TO_TWIPS(float(aMarginLeft));
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetMarginBottom(double *aMarginBottom)
{
  NS_ENSURE_ARG_POINTER(aMarginBottom);
  *aMarginBottom = NS_TWIPS_TO_INCHES(mMargin.bottom);
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetMarginBottom(double aMarginBottom)
{
  mMargin.bottom = NS_INCHES_TO_TWIPS(float(aMarginBottom));
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetMarginRight(double *aMarginRight)
{
  NS_ENSURE_ARG_POINTER(aMarginRight);
  *aMarginRight = NS_TWIPS_TO_INCHES(mMargin.right);
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetMarginRight(double aMarginRight)
{
  mMargin.right = NS_INCHES_TO_TWIPS(float(aMarginRight));
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetScaling(double *aScaling)
{
  NS_ENSURE_ARG_POINTER(aScaling);
  *aScaling = mScaling;
  return NS_OK;
}

NS_IMETHODIMP nsPrintSettings::SetScaling(double aScaling)
{
  mScaling = aScaling;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetPrintBGColors(PRBool *aPrintBGColors)
{
  NS_ENSURE_ARG_POINTER(aPrintBGColors);
  *aPrintBGColors = mPrintBGColors;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPrintBGColors(PRBool aPrintBGColors)
{
  mPrintBGColors = aPrintBGColors;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetPrintBGImages(PRBool *aPrintBGImages)
{
  NS_ENSURE_ARG_POINTER(aPrintBGImages);
  *aPrintBGImages = mPrintBGImages;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPrintBGImages(PRBool aPrintBGImages)
{
  mPrintBGImages = aPrintBGImages;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetPrintRange(PRInt16 *aPrintRange)
{
  NS_ENSURE_ARG_POINTER(aPrintRange);
  *aPrintRange = mPrintRange;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPrintRange(PRInt16 aPrintRange)
{
  mPrintRange = aPrintRange;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetTitle(PRUnichar * *aTitle)
{
  NS_ENSURE_ARG_POINTER(aTitle);
  if (!mTitle.IsEmpty()) {
    *aTitle = ToNewUnicode(mTitle);
  } else {
    *aTitle = nsnull;
  }
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetTitle(const PRUnichar * aTitle)
{
  if (aTitle) {
    mTitle = aTitle;
  } else {
    mTitle.SetLength(0);
  }
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetDocURL(PRUnichar * *aDocURL)
{
  NS_ENSURE_ARG_POINTER(aDocURL);
  if (!mURL.IsEmpty()) {
    *aDocURL = ToNewUnicode(mURL);
  } else {
    *aDocURL = nsnull;
  }
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetDocURL(const PRUnichar * aDocURL)
{
  if (aDocURL) {
    mURL = aDocURL;
  } else {
    mURL.SetLength(0);
  }
  return NS_OK;
}





NS_IMETHODIMP 
nsPrintSettings::GetPrintOptions(PRInt32 aType, PRBool *aTurnOnOff)
{
  NS_ENSURE_ARG_POINTER(aTurnOnOff);
  *aTurnOnOff = mPrintOptions & aType ? PR_TRUE : PR_FALSE;
  return NS_OK;
}




NS_IMETHODIMP 
nsPrintSettings::SetPrintOptions(PRInt32 aType, PRBool aTurnOnOff)
{
  if (aTurnOnOff) {
    mPrintOptions |=  aType;
  } else {
    mPrintOptions &= ~aType;
  }
  return NS_OK;
}





NS_IMETHODIMP 
nsPrintSettings::GetPrintOptionsBits(PRInt32 *aBits)
{
  NS_ENSURE_ARG_POINTER(aBits);
  *aBits = mPrintOptions;
  return NS_OK;
}


nsresult 
nsPrintSettings::GetMarginStrs(PRUnichar * *aTitle, 
                              nsHeaderFooterEnum aType, 
                              PRInt16 aJust)
{
  NS_ENSURE_ARG_POINTER(aTitle);
  *aTitle = nsnull;
  if (aType == eHeader) {
    switch (aJust) {
      case kJustLeft:   *aTitle = ToNewUnicode(mHeaderStrs[0]);break;
      case kJustCenter: *aTitle = ToNewUnicode(mHeaderStrs[1]);break;
      case kJustRight:  *aTitle = ToNewUnicode(mHeaderStrs[2]);break;
    } 
  } else {
    switch (aJust) {
      case kJustLeft:   *aTitle = ToNewUnicode(mFooterStrs[0]);break;
      case kJustCenter: *aTitle = ToNewUnicode(mFooterStrs[1]);break;
      case kJustRight:  *aTitle = ToNewUnicode(mFooterStrs[2]);break;
    } 
  }
  return NS_OK;
}

nsresult
nsPrintSettings::SetMarginStrs(const PRUnichar * aTitle, 
                              nsHeaderFooterEnum aType, 
                              PRInt16 aJust)
{
  NS_ENSURE_ARG_POINTER(aTitle);
  if (aType == eHeader) {
    switch (aJust) {
      case kJustLeft:   mHeaderStrs[0] = aTitle;break;
      case kJustCenter: mHeaderStrs[1] = aTitle;break;
      case kJustRight:  mHeaderStrs[2] = aTitle;break;
    } 
  } else {
    switch (aJust) {
      case kJustLeft:   mFooterStrs[0] = aTitle;break;
      case kJustCenter: mFooterStrs[1] = aTitle;break;
      case kJustRight:  mFooterStrs[2] = aTitle;break;
    } 
  }
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetHeaderStrLeft(PRUnichar * *aTitle)
{
  return GetMarginStrs(aTitle, eHeader, kJustLeft);
}
NS_IMETHODIMP nsPrintSettings::SetHeaderStrLeft(const PRUnichar * aTitle)
{
  return SetMarginStrs(aTitle, eHeader, kJustLeft);
}


NS_IMETHODIMP nsPrintSettings::GetHeaderStrCenter(PRUnichar * *aTitle)
{
  return GetMarginStrs(aTitle, eHeader, kJustCenter);
}
NS_IMETHODIMP nsPrintSettings::SetHeaderStrCenter(const PRUnichar * aTitle)
{
  return SetMarginStrs(aTitle, eHeader, kJustCenter);
}


NS_IMETHODIMP nsPrintSettings::GetHeaderStrRight(PRUnichar * *aTitle)
{
  return GetMarginStrs(aTitle, eHeader, kJustRight);
}
NS_IMETHODIMP nsPrintSettings::SetHeaderStrRight(const PRUnichar * aTitle)
{
  return SetMarginStrs(aTitle, eHeader, kJustRight);
}



NS_IMETHODIMP nsPrintSettings::GetFooterStrLeft(PRUnichar * *aTitle)
{
  return GetMarginStrs(aTitle, eFooter, kJustLeft);
}
NS_IMETHODIMP nsPrintSettings::SetFooterStrLeft(const PRUnichar * aTitle)
{
  return SetMarginStrs(aTitle, eFooter, kJustLeft);
}


NS_IMETHODIMP nsPrintSettings::GetFooterStrCenter(PRUnichar * *aTitle)
{
  return GetMarginStrs(aTitle, eFooter, kJustCenter);
}
NS_IMETHODIMP nsPrintSettings::SetFooterStrCenter(const PRUnichar * aTitle)
{
  return SetMarginStrs(aTitle, eFooter, kJustCenter);
}


NS_IMETHODIMP nsPrintSettings::GetFooterStrRight(PRUnichar * *aTitle)
{
  return GetMarginStrs(aTitle, eFooter, kJustRight);
}
NS_IMETHODIMP nsPrintSettings::SetFooterStrRight(const PRUnichar * aTitle)
{
  return SetMarginStrs(aTitle, eFooter, kJustRight);
}


NS_IMETHODIMP nsPrintSettings::GetPrintFrameTypeUsage(PRInt16 *aPrintFrameTypeUsage)
{
  NS_ENSURE_ARG_POINTER(aPrintFrameTypeUsage);
  *aPrintFrameTypeUsage = mPrintFrameTypeUsage;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPrintFrameTypeUsage(PRInt16 aPrintFrameTypeUsage)
{
  mPrintFrameTypeUsage = aPrintFrameTypeUsage;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetPrintFrameType(PRInt16 *aPrintFrameType)
{
  NS_ENSURE_ARG_POINTER(aPrintFrameType);
  *aPrintFrameType = (PRInt32)mPrintFrameType;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPrintFrameType(PRInt16 aPrintFrameType)
{
  mPrintFrameType = aPrintFrameType;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetPrintSilent(PRBool *aPrintSilent)
{
  NS_ENSURE_ARG_POINTER(aPrintSilent);
  *aPrintSilent = mPrintSilent;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPrintSilent(PRBool aPrintSilent)
{
  mPrintSilent = aPrintSilent;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetShrinkToFit(PRBool *aShrinkToFit)
{
  NS_ENSURE_ARG_POINTER(aShrinkToFit);
  *aShrinkToFit = mShrinkToFit;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetShrinkToFit(PRBool aShrinkToFit)
{
  mShrinkToFit = aShrinkToFit;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetShowPrintProgress(PRBool *aShowPrintProgress)
{
  NS_ENSURE_ARG_POINTER(aShowPrintProgress);
  *aShowPrintProgress = mShowPrintProgress;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetShowPrintProgress(PRBool aShowPrintProgress)
{
  mShowPrintProgress = aShowPrintProgress;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetPaperName(PRUnichar * *aPaperName)
{
  NS_ENSURE_ARG_POINTER(aPaperName);
  if (!mPaperName.IsEmpty()) {
    *aPaperName = ToNewUnicode(mPaperName);
  } else {
    *aPaperName = nsnull;
  }
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPaperName(const PRUnichar * aPaperName)
{
  if (aPaperName) {
    mPaperName = aPaperName;
  } else {
    mPaperName.SetLength(0);
  }
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetPlexName(PRUnichar * *aPlexName)
{
  NS_ENSURE_ARG_POINTER(aPlexName);
  if (!mPlexName.IsEmpty()) {
    *aPlexName = ToNewUnicode(mPlexName);
  } else {
    *aPlexName = nsnull;
  }
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPlexName(const PRUnichar * aPlexName)
{
  if (aPlexName) {
    mPlexName = aPlexName;
  } else {
    mPlexName.SetLength(0);
  }
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetHowToEnableFrameUI(PRInt16 *aHowToEnableFrameUI)
{
  NS_ENSURE_ARG_POINTER(aHowToEnableFrameUI);
  *aHowToEnableFrameUI = (PRInt32)mHowToEnableFrameUI;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetHowToEnableFrameUI(PRInt16 aHowToEnableFrameUI)
{
  mHowToEnableFrameUI = aHowToEnableFrameUI;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetIsCancelled(PRBool *aIsCancelled)
{
  NS_ENSURE_ARG_POINTER(aIsCancelled);
  *aIsCancelled = mIsCancelled;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetIsCancelled(PRBool aIsCancelled)
{
  mIsCancelled = aIsCancelled;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetPaperWidth(double *aPaperWidth)
{
  NS_ENSURE_ARG_POINTER(aPaperWidth);
  *aPaperWidth = mPaperWidth;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPaperWidth(double aPaperWidth)
{
  mPaperWidth = aPaperWidth;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetPaperHeight(double *aPaperHeight)
{
  NS_ENSURE_ARG_POINTER(aPaperHeight);
  *aPaperHeight = mPaperHeight;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPaperHeight(double aPaperHeight)
{
  mPaperHeight = aPaperHeight;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetPaperSizeUnit(PRInt16 *aPaperSizeUnit)
{
  NS_ENSURE_ARG_POINTER(aPaperSizeUnit);
  *aPaperSizeUnit = mPaperSizeUnit;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPaperSizeUnit(PRInt16 aPaperSizeUnit)
{
  mPaperSizeUnit = aPaperSizeUnit;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetPaperSizeType(PRInt16 *aPaperSizeType)
{
  NS_ENSURE_ARG_POINTER(aPaperSizeType);
  *aPaperSizeType = mPaperSizeType;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPaperSizeType(PRInt16 aPaperSizeType)
{
  mPaperSizeType = aPaperSizeType;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettings::GetPaperData(PRInt16 *aPaperData)
{
  NS_ENSURE_ARG_POINTER(aPaperData);
  *aPaperData = mPaperData;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettings::SetPaperData(PRInt16 aPaperData)
{
  mPaperData = aPaperData;
  return NS_OK;
}






NS_IMETHODIMP 
nsPrintSettings::SetMarginInTwips(nsMargin& aMargin)
{
  mMargin = aMargin;
  return NS_OK;
}





NS_IMETHODIMP 
nsPrintSettings::GetMarginInTwips(nsMargin& aMargin)
{
  aMargin = mMargin;
  return NS_OK;
}





NS_IMETHODIMP 
nsPrintSettings::GetPageSizeInTwips(PRInt32 *aWidth, PRInt32 *aHeight)
{
  if (mPaperSizeUnit == kPaperSizeInches) {
    *aWidth  = NS_INCHES_TO_TWIPS(float(mPaperWidth));
    *aHeight = NS_INCHES_TO_TWIPS(float(mPaperHeight));
  } else {
    *aWidth  = NS_MILLIMETERS_TO_TWIPS(float(mPaperWidth));
    *aHeight = NS_MILLIMETERS_TO_TWIPS(float(mPaperHeight));
  }
  return NS_OK;
}

nsresult 
nsPrintSettings::_Clone(nsIPrintSettings **_retval)
{
  nsPrintSettings* printSettings = new nsPrintSettings(*this);
  return printSettings->QueryInterface(NS_GET_IID(nsIPrintSettings), (void**)_retval); 
}


NS_IMETHODIMP 
nsPrintSettings::Clone(nsIPrintSettings **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  return _Clone(_retval);
}


nsresult 
nsPrintSettings::_Assign(nsIPrintSettings *aPS)
{
  nsPrintSettings *ps = static_cast<nsPrintSettings*>(aPS);
  *this = *ps;
  return NS_OK;
}


NS_IMETHODIMP 
nsPrintSettings::Assign(nsIPrintSettings *aPS)
{
  NS_ENSURE_ARG(aPS);
  return _Assign(aPS);
}


nsPrintSettings& nsPrintSettings::operator=(const nsPrintSettings& rhs)
{
  if (this == &rhs) {
    return *this;
  }

  mStartPageNum        = rhs.mStartPageNum;
  mEndPageNum          = rhs.mEndPageNum;
  mMargin              = rhs.mMargin;
  mScaling             = rhs.mScaling;
  mPrintBGColors       = rhs.mPrintBGColors;
  mPrintBGImages       = rhs.mPrintBGImages;
  mPrintRange          = rhs.mPrintRange;
  mTitle               = rhs.mTitle;
  mURL                 = rhs.mURL;
  mHowToEnableFrameUI  = rhs.mHowToEnableFrameUI;
  mIsCancelled         = rhs.mIsCancelled;
  mPrintFrameTypeUsage = rhs.mPrintFrameTypeUsage;
  mPrintFrameType      = rhs.mPrintFrameType;
  mPrintSilent         = rhs.mPrintSilent;
  mShrinkToFit         = rhs.mShrinkToFit;
  mShowPrintProgress   = rhs.mShowPrintProgress;
  mPaperName           = rhs.mPaperName;
  mPlexName            = rhs.mPlexName;
  mPaperSizeType       = rhs.mPaperSizeType;
  mPaperData           = rhs.mPaperData;
  mPaperWidth          = rhs.mPaperWidth;
  mPaperHeight         = rhs.mPaperHeight;
  mPaperSizeUnit       = rhs.mPaperSizeUnit;
  mPrintReversed       = rhs.mPrintReversed;
  mPrintInColor        = rhs.mPrintInColor;
  mOrientation         = rhs.mOrientation;
  mPrintCommand        = rhs.mPrintCommand;
  mNumCopies           = rhs.mNumCopies;
  mPrinter             = rhs.mPrinter;
  mPrintToFile         = rhs.mPrintToFile;
  mToFileName          = rhs.mToFileName;
  mOutputFormat        = rhs.mOutputFormat;
  mPrintPageDelay      = rhs.mPrintPageDelay;

  for (PRInt32 i=0;i<NUM_HEAD_FOOT;i++) {
    mHeaderStrs[i] = rhs.mHeaderStrs[i];
    mFooterStrs[i] = rhs.mFooterStrs[i];
  }

  return *this;
}

