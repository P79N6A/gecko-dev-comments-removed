





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


  nsresult GetMarginStrs(PRUnichar * *aTitle, nsHeaderFooterEnum aType, int16_t aJust);
  nsresult SetMarginStrs(const PRUnichar * aTitle, nsHeaderFooterEnum aType, int16_t aJust);

  
  nsWeakPtr     mSession; 
 
  
  nsIntMargin   mMargin;
  nsIntMargin   mEdge;
  nsIntMargin   mUnwriteableMargin;

  int32_t       mPrintOptions;

  
  int16_t       mPrintRange;
  int32_t       mStartPageNum; 
  int32_t       mEndPageNum;
  double        mScaling;
  bool          mPrintBGColors;  
  bool          mPrintBGImages;  

  int16_t       mPrintFrameTypeUsage;
  int16_t       mPrintFrameType;
  int16_t       mHowToEnableFrameUI;
  bool          mIsCancelled;
  bool          mPrintSilent;
  bool          mPrintPreview;
  bool          mShrinkToFit;
  bool          mShowPrintProgress;
  int32_t       mPrintPageDelay;

  nsString      mTitle;
  nsString      mURL;
  nsString      mPageNumberFormat;
  nsString      mHeaderStrs[NUM_HEAD_FOOT];
  nsString      mFooterStrs[NUM_HEAD_FOOT];

  nsString      mPaperName;
  nsString      mPlexName;
  int16_t       mPaperData;
  int16_t       mPaperSizeType;
  double        mPaperWidth;
  double        mPaperHeight;
  int16_t       mPaperSizeUnit;

  bool          mPrintReversed;
  bool          mPrintInColor; 
  int32_t       mOrientation;  
  nsString      mColorspace;
  nsString      mResolutionName;
  bool          mDownloadFonts;
  nsString      mPrintCommand;
  int32_t       mNumCopies;
  nsXPIDLString mPrinter;
  bool          mPrintToFile;
  nsString      mToFileName;
  int16_t       mOutputFormat;
  bool          mIsInitedFromPrinter;
  bool          mIsInitedFromPrefs;
  bool          mPersistMarginBoxSettings;
};

#endif 
