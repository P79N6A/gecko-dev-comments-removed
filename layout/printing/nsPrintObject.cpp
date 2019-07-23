




































#include "nsPrintObject.h"
#include "nsIContentViewer.h"
#include "nsIDOMDocument.h"




nsPrintObject::nsPrintObject() :
  mContent(nsnull), mFrameType(eFrame), mParent(nsnull),
  mHasBeenPrinted(PR_FALSE), mDontPrint(PR_TRUE), mPrintAsIs(PR_FALSE),
  mSharedPresShell(PR_FALSE), mInvisible(PR_FALSE),
  mShrinkRatio(1.0), mZoomRatio(1.0)
{
}


nsPrintObject::~nsPrintObject()
{
  for (PRInt32 i=0;i<mKids.Count();i++) {
    nsPrintObject* po = (nsPrintObject*)mKids[i];
    delete po;
  }

  DestroyPresentation();
}



nsresult 
nsPrintObject::Init(nsIDocShell* aDocShell)
{
  mDocShell = aDocShell;
  NS_ENSURE_TRUE(mDocShell, NS_ERROR_FAILURE);
  
  nsresult rv;
  nsCOMPtr<nsIContentViewer> viewer;
  rv = mDocShell->GetContentViewer(getter_AddRefs(viewer));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIDOMDocument> doc;
  viewer->GetDOMDocument(getter_AddRefs(doc));
  NS_ENSURE_SUCCESS(rv, rv);
  
  mDocument = do_QueryInterface(doc);
  NS_ENSURE_TRUE(mDocument, NS_ERROR_FAILURE);

  return NS_OK;
}



void 
nsPrintObject::DestroyPresentation()
{
  mWindow      = nsnull;
  mPresContext = nsnull;
  if (mPresShell) {
    mPresShell->EndObservingDocument();
    mPresShell->Destroy();
  }
  mPresShell   = nsnull;
  mViewManager = nsnull;
}

