



































#ifndef nsPrintObject_h___
#define nsPrintObject_h___


#include "nsCOMPtr.h"
#include "nsIContent.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsStyleSet.h"
#include "nsIViewManager.h"
#include "nsIDocShell.h"
#include "nsIDocument.h"
#include "nsIWidget.h"


enum PrintObjectType  {eDoc = 0, eFrame = 1, eIFrame = 2, eFrameSet = 3};




class nsPrintObject
{

public:
  nsPrintObject();
  ~nsPrintObject(); 

  
  nsresult Init(nsIDocShell* aDocShell);

  PRBool IsPrintable()  { return !mDontPrint; }
  void   DestroyPresentation();

  
  nsCOMPtr<nsIDocShell>    mDocShell;
  nsCOMPtr<nsIDocument>    mDocument;

  nsCOMPtr<nsPresContext>  mPresContext;
  nsCOMPtr<nsIPresShell>   mPresShell;
  nsCOMPtr<nsIViewManager> mViewManager;
  nsCOMPtr<nsIWidget>      mWindow;

  nsIContent*      mContent;
  PrintObjectType  mFrameType;
  
  nsVoidArray      mKids;
  nsPrintObject*   mParent;
  PRPackedBool     mHasBeenPrinted;
  PRPackedBool     mDontPrint;
  PRPackedBool     mPrintAsIs;
  PRPackedBool     mSharedPresShell;
  PRPackedBool     mInvisible;        

  float            mShrinkRatio;
  float            mZoomRatio;

private:
  nsPrintObject& operator=(const nsPrintObject& aOther); 

};



#endif 

