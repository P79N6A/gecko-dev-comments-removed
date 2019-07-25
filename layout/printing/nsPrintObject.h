



































#ifndef nsPrintObject_h___
#define nsPrintObject_h___


#include "nsCOMPtr.h"
#include "nsIContent.h"
#include "nsIPresShell.h"
#include "nsStyleSet.h"
#include "nsIViewManager.h"
#include "nsIDocShell.h"
#include "nsIDocument.h"
#include "nsIDocShellTreeOwner.h"

class nsPresContext;


enum PrintObjectType  {eDoc = 0, eFrame = 1, eIFrame = 2, eFrameSet = 3};




class nsPrintObject
{

public:
  nsPrintObject();
  ~nsPrintObject(); 

  
  nsresult Init(nsIDocShell* aDocShell, nsIDOMDocument* aDoc,
                bool aPrintPreview);

  bool IsPrintable()  { return !mDontPrint; }
  void   DestroyPresentation();

  
  nsCOMPtr<nsIDocShell>    mDocShell;
  nsCOMPtr<nsIDocShellTreeOwner> mTreeOwner;
  nsCOMPtr<nsIDocument>    mDocument;

  nsRefPtr<nsPresContext>  mPresContext;
  nsCOMPtr<nsIPresShell>   mPresShell;
  nsCOMPtr<nsIViewManager> mViewManager;

  nsCOMPtr<nsIContent>     mContent;
  PrintObjectType  mFrameType;
  
  nsTArray<nsPrintObject*> mKids;
  nsPrintObject*   mParent;
  bool             mHasBeenPrinted;
  bool             mDontPrint;
  bool             mPrintAsIs;
  bool             mSharedPresShell;
  bool             mInvisible;        
  bool             mPrintPreview;
  bool             mDidCreateDocShell;
  float            mShrinkRatio;
  float            mZoomRatio;

private:
  nsPrintObject& operator=(const nsPrintObject& aOther); 

};



#endif 

