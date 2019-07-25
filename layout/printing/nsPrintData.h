



































#ifndef nsPrintData_h___
#define nsPrintData_h___


#include "nsIDOMWindow.h"
#include "nsDeviceContext.h"
#include "nsIPrintProgressParams.h"
#include "nsIPrintOptions.h"
#include "nsTArray.h"
#include "nsCOMArray.h"
#include "nsAutoPtr.h"


class nsPrintObject;
class nsPrintPreviewListener;
class nsIWebProgressListener;















class nsPrintData {
public:

  typedef enum {eIsPrinting, eIsPrintPreview } ePrintDataType;

  
  
  enum eDocTitleDefault {
    eDocTitleDefNone,
    eDocTitleDefBlank,
    eDocTitleDefURLDoc
  };


  nsPrintData(ePrintDataType aType);
  ~nsPrintData(); 

  
  void OnEndPrinting();
  void OnStartPrinting();
  void DoOnProgressChange(PRInt32      aProgress,
                          PRInt32      aMaxProgress,
                          bool         aDoStartStop,
                          PRInt32      aFlag);


  ePrintDataType               mType;            
  nsRefPtr<nsDeviceContext>   mPrintDC;
  FILE                        *mDebugFilePtr;    

  nsPrintObject *                mPrintObject;
  nsPrintObject *                mSelectedPO;

  nsCOMArray<nsIWebProgressListener> mPrintProgressListeners;
  nsCOMPtr<nsIPrintProgressParams> mPrintProgressParams;

  nsCOMPtr<nsIDOMWindow> mCurrentFocusWin; 

  nsTArray<nsPrintObject*>    mPrintDocList;
  bool                        mIsIFrameSelected;
  bool                        mIsParentAFrameSet;
  bool                        mOnStartSent;
  bool                        mIsAborted;           
  bool                        mPreparingForPrint;   
  bool                        mDocWasToBeDestroyed; 
  bool                        mShrinkToFit;
  PRInt16                     mPrintFrameType;
  PRInt32                     mNumPrintablePages;
  PRInt32                     mNumPagesPrinted;
  float                       mShrinkRatio;
  float                       mOrigDCScale;

  nsCOMPtr<nsIPrintSettings>  mPrintSettings;
  nsPrintPreviewListener*     mPPEventListeners;

  PRUnichar*            mBrandName; 

private:
  nsPrintData(); 
  nsPrintData& operator=(const nsPrintData& aOther); 

};

#endif 

