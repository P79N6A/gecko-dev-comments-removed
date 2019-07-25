




































#include "nsPrintObject.h"
#include "nsIContentViewer.h"
#include "nsIDOMDocument.h"
#include "nsContentUtils.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsPIDOMWindow.h"
#include "nsGkAtoms.h"
#include "nsComponentManagerUtils.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocShellTreeItem.h"




nsPrintObject::nsPrintObject() :
  mContent(nsnull), mFrameType(eFrame), mParent(nsnull),
  mHasBeenPrinted(PR_FALSE), mDontPrint(PR_TRUE), mPrintAsIs(PR_FALSE),
  mSharedPresShell(PR_FALSE), mInvisible(PR_FALSE),
  mShrinkRatio(1.0), mZoomRatio(1.0)
{
}


nsPrintObject::~nsPrintObject()
{
  for (PRUint32 i=0;i<mKids.Length();i++) {
    nsPrintObject* po = mKids[i];
    delete po;
  }

  DestroyPresentation();
}



nsresult 
nsPrintObject::Init(nsIDocShell* aDocShell, nsIDOMDocument* aDoc,
                    PRBool aPrintPreview)
{
  mPrintPreview = aPrintPreview;

  if (mPrintPreview || mParent) {
    mDocShell = aDocShell;
  } else {
    nsCOMPtr<nsIDocShellTreeOwner> owner = do_GetInterface(aDocShell);
    nsCOMPtr<nsIDocShellTreeItem> item = do_QueryInterface(aDocShell);
    PRInt32 itemType = 0;
    item->GetItemType(&itemType);
    
    mDocShell = do_CreateInstance("@mozilla.org/docshell;1");
    NS_ENSURE_TRUE(mDocShell, NS_ERROR_OUT_OF_MEMORY);
    nsCOMPtr<nsIDocShellTreeItem> newItem = do_QueryInterface(mDocShell);
    newItem->SetItemType(itemType);
    newItem->SetTreeOwner(owner);
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
  mWindow      = nsnull;
  mPresContext = nsnull;
  if (mPresShell) {
    mPresShell->EndObservingDocument();
    nsAutoScriptBlocker scriptBlocker;
    mPresShell->Destroy();
  }
  mPresShell   = nsnull;
  mViewManager = nsnull;
}

