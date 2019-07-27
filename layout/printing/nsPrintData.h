




#ifndef nsPrintData_h___
#define nsPrintData_h___

#include "mozilla/Attributes.h"


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

  explicit nsPrintData(ePrintDataType aType);
  ~nsPrintData(); 

  
  void OnEndPrinting();
  void OnStartPrinting();
  void DoOnProgressChange(int32_t      aProgress,
                          int32_t      aMaxProgress,
                          bool         aDoStartStop,
                          int32_t      aFlag);


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
  int16_t                     mPrintFrameType;
  int32_t                     mNumPrintablePages;
  int32_t                     mNumPagesPrinted;
  float                       mShrinkRatio;
  float                       mOrigDCScale;

  nsCOMPtr<nsIPrintSettings>  mPrintSettings;
  nsPrintPreviewListener*     mPPEventListeners;

  char16_t*            mBrandName; 

private:
  nsPrintData() MOZ_DELETE;
  nsPrintData& operator=(const nsPrintData& aOther) MOZ_DELETE;

};

#endif 

