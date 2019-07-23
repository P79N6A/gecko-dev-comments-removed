






































#ifndef nsPrintSettingsImpl_h__
#define nsPrintSettingsImpl_h__

#include "gfxCore.h"
#include "nsIPrintSettings.h"  
#include "nsMargin.h"  
#include "nsString.h"
#include "nsWeakReference.h"  

#define NUM_HEAD_FOOT 3





class nsPrintSettings : public nsIPrintSettings
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTSETTINGS

  nsPrintSettings();
  nsPrintSettings(const nsPrintSettings& aPS);
  virtual ~nsPrintSettings();

  virtual nsPrintSettings& operator=(const nsPrintSettings& rhs);

protected:
  
  virtual nsresult _Clone(nsIPrintSettings **_retval);
  virtual nsresult _Assign(nsIPrintSettings *aPS);
  
  typedef enum {
    eHeader,
    eFooter
  } nsHeaderFooterEnum;


  nsresult GetMarginStrs(PRUnichar * *aTitle, nsHeaderFooterEnum aType, PRInt16 aJust);
  nsresult SetMarginStrs(const PRUnichar * aTitle, nsHeaderFooterEnum aType, PRInt16 aJust);

  
  nsWeakPtr     mSession; 
 
  nsMargin      mMargin;
  PRInt32       mPrintOptions;

  
  PRInt16       mPrintRange;
  PRInt32       mStartPageNum; 
  PRInt32       mEndPageNum;
  double        mScaling;
  PRBool        mPrintBGColors;  
  PRBool        mPrintBGImages;  

  PRInt16       mPrintFrameTypeUsage;
  PRInt16       mPrintFrameType;
  PRBool        mHowToEnableFrameUI;
  PRBool        mIsCancelled;
  PRBool        mPrintSilent;
  PRBool        mPrintPreview;
  PRBool        mShrinkToFit;
  PRBool        mShowPrintProgress;
  PRInt32       mPrintPageDelay;

  nsString      mTitle;
  nsString      mURL;
  nsString      mPageNumberFormat;
  nsString      mHeaderStrs[NUM_HEAD_FOOT];
  nsString      mFooterStrs[NUM_HEAD_FOOT];

  nsString      mPaperName;
  nsString      mPlexName;
  PRInt16       mPaperData;
  PRInt16       mPaperSizeType;
  double        mPaperWidth;
  double        mPaperHeight;
  PRInt16       mPaperSizeUnit;

  PRBool        mPrintReversed;
  PRBool        mPrintInColor; 
  PRInt32       mOrientation;  
  nsString      mColorspace;
  nsString      mResolutionName;
  PRBool        mDownloadFonts;
  nsString      mPrintCommand;
  PRInt32       mNumCopies;
  nsXPIDLString mPrinter;
  PRBool        mPrintToFile;
  nsString      mToFileName;
  PRInt16       mOutputFormat;
  PRPackedBool  mIsInitedFromPrinter;
  PRPackedBool  mIsInitedFromPrefs;

};

#endif 
