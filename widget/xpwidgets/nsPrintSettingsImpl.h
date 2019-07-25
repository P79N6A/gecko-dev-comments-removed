






































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

  nsPrintSettings& operator=(const nsPrintSettings& rhs);

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
 
  
  nsIntMargin   mMargin;
  nsIntMargin   mEdge;
  nsIntMargin   mUnwriteableMargin;

  PRInt32       mPrintOptions;

  
  PRInt16       mPrintRange;
  PRInt32       mStartPageNum; 
  PRInt32       mEndPageNum;
  double        mScaling;
  bool          mPrintBGColors;  
  bool          mPrintBGImages;  

  PRInt16       mPrintFrameTypeUsage;
  PRInt16       mPrintFrameType;
  PRInt16       mHowToEnableFrameUI;
  bool          mIsCancelled;
  bool          mPrintSilent;
  bool          mPrintPreview;
  bool          mShrinkToFit;
  bool          mShowPrintProgress;
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

  bool          mPrintReversed;
  bool          mPrintInColor; 
  PRInt32       mOrientation;  
  nsString      mColorspace;
  nsString      mResolutionName;
  bool          mDownloadFonts;
  nsString      mPrintCommand;
  PRInt32       mNumCopies;
  nsXPIDLString mPrinter;
  bool          mPrintToFile;
  nsString      mToFileName;
  PRInt16       mOutputFormat;
  bool          mIsInitedFromPrinter;
  bool          mIsInitedFromPrefs;

};

#endif 
