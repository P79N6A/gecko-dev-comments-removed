



#ifndef nsPrintObject_h___
#define nsPrintObject_h___

#include "mozilla/Attributes.h"


#include "nsCOMPtr.h"
#include "nsIPresShell.h"
#include "nsStyleSet.h"
#include "nsViewManager.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeOwner.h"

class nsIContent;
class nsIDocument;
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
  nsRefPtr<nsViewManager> mViewManager;

  nsCOMPtr<nsIContent>     mContent;
  PrintObjectType  mFrameType;
  
  nsTArray<nsPrintObject*> mKids;
  nsPrintObject*   mParent;
  bool             mHasBeenPrinted;
  bool             mDontPrint;
  bool             mPrintAsIs;
  bool             mInvisible;        
  bool             mPrintPreview;
  bool             mDidCreateDocShell;
  float            mShrinkRatio;
  float            mZoomRatio;

private:
  nsPrintObject& operator=(const nsPrintObject& aOther) MOZ_DELETE;
};



#endif 

