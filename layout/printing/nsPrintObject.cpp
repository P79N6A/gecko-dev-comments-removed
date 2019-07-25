




































#include "nsPrintObject.h"
#include "nsIContentViewer.h"
#include "nsIDOMDocument.h"
#include "nsContentUtils.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsPIDOMWindow.h"
#include "nsGkAtoms.h"
#include "nsComponentManagerUtils.h"
#include "nsIDocShellTreeItem.h"
#include "nsIBaseWindow.h"
                                                   



nsPrintObject::nsPrintObject() :
  mContent(nsnull), mFrameType(eFrame), mParent(nsnull),
  mHasBeenPrinted(PR_FALSE), mDontPrint(PR_TRUE), mPrintAsIs(PR_FALSE),
  mSharedPresShell(PR_FALSE), mInvisible(PR_FALSE), mDidCreateDocShell(PR_FALSE),
  mShrinkRatio(1.0), mZoomRatio(1.0)
{
  MOZ_COUNT_CTOR(nsPrintObject);
}


nsPrintObject::~nsPrintObject()
{
  MOZ_COUNT_DTOR(nsPrintObject);
  for (PRUint32 i=0;i<mKids.Length();i++) {
    nsPrintObject* po = mKids[i];
    delete po;
  }

  DestroyPresentation();
  if (mDidCreateDocShell && mDocShell) {
    nsCOMPtr<nsIBaseWindow> baseWin(do_QueryInterface(mDocShell));
    if (baseWin) {
      baseWin->Destroy();
    }
  }                            
  mDocShell = nsnull;
  mTreeOwner = nsnull; 
}


nsresult 
nsPrintObject::Init(nsIDocShell* aDocShell, nsIDOMDocument* aDoc,
                    PRBool aPrintPreview)
{
  mPrintPreview = aPrintPreview;

  if (mPrintPreview || mParent) {
    mDocShell = aDocShell;
  } else {
    mTreeOwner = do_GetInterface(aDocShell);
    nsCOMPtr<nsIDocShellTreeItem> item = do_QueryInterface(aDocShell);
    PRInt32 itemType = 0;
    item->GetItemType(&itemType);
    
    mDocShell = do_CreateInstance("@mozilla.org/docshell;1");
    NS_ENSURE_TRUE(mDocShell, NS_ERROR_OUT_OF_MEMORY);
    mDidCreateDocShell = PR_TRUE;
    nsCOMPtr<nsIDocShellTreeItem> newItem = do_QueryInterface(mDocShell);
    newItem->SetItemType(itemType);
    newItem->SetTreeOwner(mTreeOwner);
  }
  NS_ENSURE_TRUE(mDocShell, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMDocument> dummy = do_GetInterface(mDocShell);
  nsCOMPtr<nsIContentViewer> viewer;
  mDocShell->GetContentViewer(getter_AddRefs(viewer));
  NS_ENSURE_STATE(viewer);

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDoc);
  NS_ENSURE_STATE(doc);

  if (mParent) {
    nsCOMPtr<nsPIDOMWindow> window = doc->GetWindow();
    if (window) {
      mContent = do_QueryInterface(window->GetFrameElementInternal());
    }
    mDocument = doc;
    return NS_OK;
  }

  mDocument = doc->CreateStaticClone(mDocShell);
  nsCOMPtr<nsIDOMDocument> clonedDOMDoc = do_QueryInterface(mDocument);
  NS_ENSURE_STATE(clonedDOMDoc);

  viewer->SetDOMDocument(clonedDOMDoc);
  return NS_OK;
}



void 
nsPrintObject::DestroyPresentation()
{
  mPresContext = nsnull;
  if (mPresShell) {
    mPresShell->EndObservingDocument();
    nsAutoScriptBlocker scriptBlocker;
    mPresShell->Destroy();
  }
  mPresShell   = nsnull;
  mViewManager = nsnull;
}

