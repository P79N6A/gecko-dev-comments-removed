







































#ifndef nsPrintSettingsImpl_h__
#define nsPrintSettingsImpl_h__

#include "nsIPrintSettings.h"  
#include "nsMargin.h"  
#include "nsString.h"  

#include <Pt.h>




class nsPrintSettings : public nsIPrintSettings
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTSETTINGS

  nsPrintSettings();
  virtual ~nsPrintSettings();

protected:
  typedef enum {
    eHeader,
    eFooter
  } nsHeaderFooterEnum;

  nsresult GetMarginStrs(PRUnichar * *aTitle, nsHeaderFooterEnum aType, PRInt16 aJust);
  nsresult SetMarginStrs(const PRUnichar * aTitle, nsHeaderFooterEnum aType, PRInt16 aJust);

  
  nsMargin      mMargin;
  PRInt32       mPrintOptions;

  
  PRInt16       mPrintRange;
  PRInt32       mStartPageNum; 
  PRInt32       mEndPageNum;
  double        mScaling;
  PRBool        mPrintBGColors;  
  PRBool        mPrintBGImages;  

  PRInt16       mPrintFrameType;
  PRBool        mPrintSilent;
  PRInt32       mPrintPageDelay;

  nsString      mTitle;
  nsString      mURL;
  nsString      mPageNumberFormat;
  nsString      mHeaderStrs[3];
  nsString      mFooterStrs[3];

  PRBool        mPrintReversed;
  PRBool        mPrintInColor; 
  PRInt32       mPaperSize;    
  PRInt32       mOrientation;  
  nsString      mPrintCommand;
  PRBool        mPrintToFile;
  nsString      mToFileName;
};



#endif 
