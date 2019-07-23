








































#include "nsWebBrowserFind.h"



#include "nsFind.h"

#include "nsIComponentManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsPIDOMWindow.h"
#include "nsIURI.h"
#include "nsIDocShell.h"
#include "nsIEnumerator.h"
#include "nsIDocShellTreeItem.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIEventStateManager.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIFocusController.h"
#include "nsISelectionController.h"
#include "nsISelection.h"
#include "nsIFrame.h"
#include "nsITextControlFrame.h"
#include "nsReadableUtils.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIContent.h"
#include "nsContentCID.h"
#include "nsIServiceManager.h"
#include "nsIObserverService.h"
#include "nsISupportsPrimitives.h"
#include "nsITimelineService.h"
#include "nsFind.h"

#if DEBUG
#include "nsIWebNavigation.h"
#include "nsXPIDLString.h"
#endif

#ifdef XP_MACOSX
#include "nsAutoPtr.h"
#include <Scrap.h>
#endif






nsWebBrowserFind::nsWebBrowserFind() :
    mFindBackwards(PR_FALSE),
    mWrapFind(PR_FALSE),
    mEntireWord(PR_FALSE),
    mMatchCase(PR_FALSE),
    mSearchSubFrames(PR_TRUE),
    mSearchParentFrames(PR_TRUE)
{
}

nsWebBrowserFind::~nsWebBrowserFind()
{
}

NS_IMPL_ISUPPORTS2(nsWebBrowserFind, nsIWebBrowserFind, nsIWebBrowserFindInFrames)



NS_IMETHODIMP nsWebBrowserFind::FindNext(PRBool *outDidFind)
{
    NS_ENSURE_ARG_POINTER(outDidFind);
    *outDidFind = PR_FALSE;

    NS_ENSURE_TRUE(CanFindNext(), NS_ERROR_NOT_INITIALIZED);

    nsresult rv = NS_OK;
    nsCOMPtr<nsIDOMWindow> searchFrame = do_QueryReferent(mCurrentSearchFrame);
    NS_ENSURE_TRUE(searchFrame, NS_ERROR_NOT_INITIALIZED);

    nsCOMPtr<nsIDOMWindow> rootFrame = do_QueryReferent(mRootSearchFrame);
    NS_ENSURE_TRUE(rootFrame, NS_ERROR_NOT_INITIALIZED);
    
    
    
    
    
    
    
    nsCOMPtr<nsIObserverService> observerSvc =
      do_GetService("@mozilla.org/observer-service;1");
    if (observerSvc) {
        nsCOMPtr<nsISupportsInterfacePointer> windowSupportsData = 
          do_CreateInstance(NS_SUPPORTS_INTERFACE_POINTER_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        nsCOMPtr<nsISupports> searchWindowSupports =
          do_QueryInterface(rootFrame);
        windowSupportsData->SetData(searchWindowSupports);
        NS_NAMED_LITERAL_STRING(dnStr, "down");
        NS_NAMED_LITERAL_STRING(upStr, "up");
        observerSvc->NotifyObservers(windowSupportsData, 
                                     "nsWebBrowserFind_FindAgain", 
                                     mFindBackwards? upStr.get(): dnStr.get());
        windowSupportsData->GetData(getter_AddRefs(searchWindowSupports));
        
        *outDidFind = searchWindowSupports == nsnull;
        if (*outDidFind)
            return NS_OK;
    }

    
    rv = SearchInFrame(searchFrame, PR_FALSE, outDidFind);
    if (NS_FAILED(rv)) return rv;
    if (*outDidFind)
        return OnFind(searchFrame);     

    
    if (!mSearchSubFrames && !mSearchParentFrames)
        return NS_OK;

    nsIDocShell *rootDocShell = GetDocShellFromWindow(rootFrame);
    if (!rootDocShell) return NS_ERROR_FAILURE;
    
    PRInt32 enumDirection;
    if (mFindBackwards)
        enumDirection = nsIDocShell::ENUMERATE_BACKWARDS;
    else
        enumDirection = nsIDocShell::ENUMERATE_FORWARDS;
        
    nsCOMPtr<nsISimpleEnumerator> docShellEnumerator;
    rv = rootDocShell->GetDocShellEnumerator(nsIDocShellTreeItem::typeAll,
            enumDirection, getter_AddRefs(docShellEnumerator));    
    if (NS_FAILED(rv)) return rv;
        
    
    nsCOMPtr<nsIDocShellTreeItem> startingItem =
        do_QueryInterface(GetDocShellFromWindow(searchFrame), &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIDocShellTreeItem> curItem;

    
    
    PRBool hasMore, doFind = PR_FALSE;
    while (NS_SUCCEEDED(docShellEnumerator->HasMoreElements(&hasMore)) && hasMore)
    {
        nsCOMPtr<nsISupports> curSupports;
        rv = docShellEnumerator->GetNext(getter_AddRefs(curSupports));
        if (NS_FAILED(rv)) break;
        curItem = do_QueryInterface(curSupports, &rv);
        if (NS_FAILED(rv)) break;

        if (doFind)
        {
            searchFrame = do_GetInterface(curItem, &rv);
            if (NS_FAILED(rv)) break;

            OnStartSearchFrame(searchFrame);

            rv = SearchInFrame(searchFrame, PR_FALSE, outDidFind);
            if (NS_FAILED(rv)) return rv;
            if (*outDidFind)
                return OnFind(searchFrame);     

            OnEndSearchFrame(searchFrame);
        }

        if (curItem.get() == startingItem.get())
            doFind = PR_TRUE;       
    };

    if (!mWrapFind)
    {
        
        SetCurrentSearchFrame(searchFrame);
        return NS_OK;
    }

    
    
    

    
    
    docShellEnumerator = nsnull;
    rv = rootDocShell->GetDocShellEnumerator(nsIDocShellTreeItem::typeAll,
            enumDirection, getter_AddRefs(docShellEnumerator));    
    if (NS_FAILED(rv)) return rv;
    
    while (NS_SUCCEEDED(docShellEnumerator->HasMoreElements(&hasMore)) && hasMore)
    {
        nsCOMPtr<nsISupports> curSupports;
        rv = docShellEnumerator->GetNext(getter_AddRefs(curSupports));
        if (NS_FAILED(rv)) break;
        curItem = do_QueryInterface(curSupports, &rv);
        if (NS_FAILED(rv)) break;

        if (curItem.get() == startingItem.get())
        {
            rv = SearchInFrame(searchFrame, PR_TRUE, outDidFind);
            if (NS_FAILED(rv)) return rv;
            if (*outDidFind)
                return OnFind(searchFrame);        
            break;
        }

        searchFrame = do_GetInterface(curItem, &rv);
        if (NS_FAILED(rv)) break;

        OnStartSearchFrame(searchFrame);

        rv = SearchInFrame(searchFrame, PR_FALSE, outDidFind);
        if (NS_FAILED(rv)) return rv;
        if (*outDidFind)
            return OnFind(searchFrame);        
        
        OnEndSearchFrame(searchFrame);
    }

    
    SetCurrentSearchFrame(searchFrame);
    
    NS_ASSERTION(NS_SUCCEEDED(rv), "Something failed");
    return rv;
}



NS_IMETHODIMP nsWebBrowserFind::GetSearchString(PRUnichar * *aSearchString)
{
    NS_ENSURE_ARG_POINTER(aSearchString);
#ifdef XP_MACOSX
    OSStatus err;
    ScrapRef scrap;
    err = ::GetScrapByName(kScrapFindScrap, kScrapGetNamedScrap, &scrap);
    if (err == noErr) {
        Size byteCount;
        err = ::GetScrapFlavorSize(scrap, kScrapFlavorTypeUnicode, &byteCount);
        if (err == noErr) {
            NS_ASSERTION(byteCount%2 == 0, "byteCount not a multiple of 2");
            nsAutoArrayPtr<PRUnichar> buffer(new PRUnichar[byteCount/2 + 1]);
            NS_ENSURE_TRUE(buffer, NS_ERROR_OUT_OF_MEMORY);
            err = ::GetScrapFlavorData(scrap, kScrapFlavorTypeUnicode, &byteCount, buffer.get());
            if (err == noErr) {
                buffer[byteCount/2] = PRUnichar('\0');
                mSearchString.Assign(buffer);
            }
        }
    }    
#endif
    *aSearchString = ToNewUnicode(mSearchString);
    return NS_OK;
}

NS_IMETHODIMP nsWebBrowserFind::SetSearchString(const PRUnichar * aSearchString)
{
    mSearchString.Assign(aSearchString);
#ifdef XP_MACOSX
    OSStatus err;
    ScrapRef scrap;
    err = ::GetScrapByName(kScrapFindScrap, kScrapClearNamedScrap, &scrap);
    if (err == noErr) {
        ::PutScrapFlavor(scrap, kScrapFlavorTypeUnicode, kScrapFlavorMaskNone,
        (mSearchString.Length()*2), aSearchString);
    }
#endif
    return NS_OK;
}


NS_IMETHODIMP nsWebBrowserFind::GetFindBackwards(PRBool *aFindBackwards)
{
    NS_ENSURE_ARG_POINTER(aFindBackwards);
    *aFindBackwards = mFindBackwards;
    return NS_OK;
}

NS_IMETHODIMP nsWebBrowserFind::SetFindBackwards(PRBool aFindBackwards)
{
    mFindBackwards = aFindBackwards;
    return NS_OK;
}


NS_IMETHODIMP nsWebBrowserFind::GetWrapFind(PRBool *aWrapFind)
{
    NS_ENSURE_ARG_POINTER(aWrapFind);
    *aWrapFind = mWrapFind;
    return NS_OK;
}
NS_IMETHODIMP nsWebBrowserFind::SetWrapFind(PRBool aWrapFind)
{
    mWrapFind = aWrapFind;
    return NS_OK;
}


NS_IMETHODIMP nsWebBrowserFind::GetEntireWord(PRBool *aEntireWord)
{
    NS_ENSURE_ARG_POINTER(aEntireWord);
    *aEntireWord = mEntireWord;
    return NS_OK;
}
NS_IMETHODIMP nsWebBrowserFind::SetEntireWord(PRBool aEntireWord)
{
    mEntireWord = aEntireWord;
    return NS_OK;
}


NS_IMETHODIMP nsWebBrowserFind::GetMatchCase(PRBool *aMatchCase)
{
    NS_ENSURE_ARG_POINTER(aMatchCase);
    *aMatchCase = mMatchCase;
    return NS_OK;
}
NS_IMETHODIMP nsWebBrowserFind::SetMatchCase(PRBool aMatchCase)
{
    mMatchCase = aMatchCase;
    return NS_OK;
}




static void
FocusElementButNotDocument(nsIDocument* aDocument, nsIContent* aContent)
{
  nsIFocusController *focusController = nsnull;
  nsPIDOMWindow *ourWindow = aDocument->GetWindow();
  if (ourWindow)
    focusController = ourWindow->GetRootFocusController();
  if (!focusController)
    return;

  
  nsCOMPtr<nsIDOMElement> oldFocusedElement;
  focusController->GetFocusedElement(getter_AddRefs(oldFocusedElement));
  nsCOMPtr<nsIContent> oldFocusedContent =
    do_QueryInterface(oldFocusedElement);

  
  nsCOMPtr<nsIDOMElement> newFocusedElement(do_QueryInterface(aContent));
  focusController->SetFocusedElement(newFocusedElement);

  nsIPresShell* presShell = aDocument->GetPrimaryShell();
  nsIEventStateManager* esm = presShell->GetPresContext()->EventStateManager();

  
  
  esm->SetFocusedContent(aContent);  
  aDocument->BeginUpdate(UPDATE_CONTENT_STATE);
  aDocument->ContentStatesChanged(oldFocusedContent, aContent, 
                                  NS_EVENT_STATE_FOCUS);
  aDocument->EndUpdate(UPDATE_CONTENT_STATE);

  
  
  
  esm->SetFocusedContent(nsnull);
}

void nsWebBrowserFind::SetSelectionAndScroll(nsIDOMWindow* aWindow,
                                             nsIDOMRange*  aRange)
{
  nsCOMPtr<nsIDOMDocument> domDoc;    
  aWindow->GetDocument(getter_AddRefs(domDoc));
  if (!domDoc) return;

  nsCOMPtr<nsIDocument> doc(do_QueryInterface(domDoc));
  nsIPresShell* presShell = doc->GetPrimaryShell();
  if (!presShell) return;

  
  
  nsIFrame *frame = nsnull;
  nsITextControlFrame *tcFrame = nsnull;
  nsCOMPtr<nsIDOMNode> node;
  aRange->GetStartContainer(getter_AddRefs(node));
  nsCOMPtr<nsIContent> content(do_QueryInterface(node));
  for ( ; content; content = content->GetParent()) {
    if (!content->IsNativeAnonymous()) {
      frame = presShell->GetPrimaryFrameFor(content);
      if (!frame)
        return;
      CallQueryInterface(frame, &tcFrame);
      break;
    }
  }

  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsISelectionController> selCon;
  frame->GetSelectionController(presShell->GetPresContext(),
                                getter_AddRefs(selCon));

  selCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
  selCon->GetSelection(nsISelectionController::SELECTION_NORMAL,
    getter_AddRefs(selection));
  if (selection) {
    selection->RemoveAllRanges();
    selection->AddRange(aRange);

    if (tcFrame) {
      FocusElementButNotDocument(doc, content);
    }
    else {
      nsCOMPtr<nsPresContext> presContext = presShell->GetPresContext();
      PRBool isSelectionWithFocus;
      presContext->EventStateManager()->
        MoveFocusToCaret(PR_TRUE, &isSelectionWithFocus);
    }

    
    
    selCon->ScrollSelectionIntoView
      (nsISelectionController::SELECTION_NORMAL,
       nsISelectionController::SELECTION_FOCUS_REGION, PR_TRUE);
  }
}


nsresult nsWebBrowserFind::GetRootNode(nsIDOMDocument* aDomDoc,
                                       nsIDOMNode **aNode)
{
  nsresult rv;

  NS_ENSURE_ARG_POINTER(aNode);
  *aNode = 0;

  nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(aDomDoc);
  if (htmlDoc)
  {
    
    nsCOMPtr<nsIDOMHTMLElement> bodyElement;
    rv = htmlDoc->GetBody(getter_AddRefs(bodyElement));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_ARG_POINTER(bodyElement);
    return bodyElement->QueryInterface(NS_GET_IID(nsIDOMNode),
                                       (void **)aNode);
  }

  
  nsCOMPtr<nsIDOMElement> docElement;
  rv = aDomDoc->GetDocumentElement(getter_AddRefs(docElement));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_ARG_POINTER(docElement);
  return docElement->QueryInterface(NS_GET_IID(nsIDOMNode), (void **)aNode);
}

nsresult nsWebBrowserFind::SetRangeAroundDocument(nsIDOMRange* aSearchRange,
                                                  nsIDOMRange* aStartPt,
                                                  nsIDOMRange* aEndPt,
                                                  nsIDOMDocument* aDoc)
{
    nsCOMPtr<nsIDOMNode> bodyNode;
    nsresult rv = GetRootNode(aDoc, getter_AddRefs(bodyNode));
    nsCOMPtr<nsIContent> bodyContent (do_QueryInterface(bodyNode));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_ARG_POINTER(bodyContent);

    PRUint32 childCount = bodyContent->GetChildCount();

    aSearchRange->SetStart(bodyNode, 0);
    aSearchRange->SetEnd(bodyNode, childCount);

    if (mFindBackwards)
    {
        aStartPt->SetStart(bodyNode, childCount);
        aStartPt->SetEnd(bodyNode, childCount);
        aEndPt->SetStart(bodyNode, 0);
        aEndPt->SetEnd(bodyNode, 0);
    }
    else
    {
        aStartPt->SetStart(bodyNode, 0);
        aStartPt->SetEnd(bodyNode, 0);
        aEndPt->SetStart(bodyNode, childCount);
        aEndPt->SetEnd(bodyNode, childCount);
    }

    return NS_OK;
}




nsresult
nsWebBrowserFind::GetSearchLimits(nsIDOMRange* aSearchRange,
                                  nsIDOMRange* aStartPt,
                                  nsIDOMRange* aEndPt,
                                  nsIDOMDocument* aDoc,
                                  nsISelection* aSel,
                                  PRBool aWrap)
{
    NS_ENSURE_ARG_POINTER(aSel);

    
    PRInt32 count = -1;
    nsresult rv = aSel->GetRangeCount(&count);
    if (count < 1)
        return SetRangeAroundDocument(aSearchRange, aStartPt, aEndPt, aDoc);

    
    nsCOMPtr<nsIDOMNode> bodyNode;
    rv = GetRootNode(aDoc, getter_AddRefs(bodyNode));
    nsCOMPtr<nsIContent> bodyContent (do_QueryInterface(bodyNode));
    NS_ENSURE_ARG_POINTER(bodyContent);

    PRUint32 childCount = bodyContent->GetChildCount();

    
    

    nsCOMPtr<nsIDOMRange> range;
    nsCOMPtr<nsIDOMNode> node;
    PRInt32 offset;

    
    if (!mFindBackwards && !aWrap)
    {
        
        
        aSel->GetRangeAt(count-1, getter_AddRefs(range));
        if (!range) return NS_ERROR_UNEXPECTED;
        range->GetEndContainer(getter_AddRefs(node));
        if (!node) return NS_ERROR_UNEXPECTED;
        range->GetEndOffset(&offset);

        aSearchRange->SetStart(node, offset);
        aSearchRange->SetEnd(bodyNode, childCount);
        aStartPt->SetStart(node, offset);
        aStartPt->SetEnd(node, offset);
        aEndPt->SetStart(bodyNode, childCount);
        aEndPt->SetEnd(bodyNode, childCount);
    }
    
    else if (mFindBackwards && !aWrap)
    {
        aSel->GetRangeAt(0, getter_AddRefs(range));
        if (!range) return NS_ERROR_UNEXPECTED;
        range->GetStartContainer(getter_AddRefs(node));
        if (!node) return NS_ERROR_UNEXPECTED;
        range->GetStartOffset(&offset);

        aSearchRange->SetStart(bodyNode, 0);
        aSearchRange->SetEnd(bodyNode, childCount);
        aStartPt->SetStart(node, offset);
        aStartPt->SetEnd(node, offset);
        aEndPt->SetStart(bodyNode, 0);
        aEndPt->SetEnd(bodyNode, 0);
    }
    
    else if (!mFindBackwards && aWrap)
    {
        aSel->GetRangeAt(count-1, getter_AddRefs(range));
        if (!range) return NS_ERROR_UNEXPECTED;
        range->GetEndContainer(getter_AddRefs(node));
        if (!node) return NS_ERROR_UNEXPECTED;
        range->GetEndOffset(&offset);

        aSearchRange->SetStart(bodyNode, 0);
        aSearchRange->SetEnd(bodyNode, childCount);
        aStartPt->SetStart(bodyNode, 0);
        aStartPt->SetEnd(bodyNode, 0);
        aEndPt->SetStart(node, offset);
        aEndPt->SetEnd(node, offset);
    }
    
    else if (mFindBackwards && aWrap)
    {
        aSel->GetRangeAt(0, getter_AddRefs(range));
        if (!range) return NS_ERROR_UNEXPECTED;
        range->GetStartContainer(getter_AddRefs(node));
        if (!node) return NS_ERROR_UNEXPECTED;
        range->GetStartOffset(&offset);

        aSearchRange->SetStart(bodyNode, 0);
        aSearchRange->SetEnd(bodyNode, childCount);
        aStartPt->SetStart(bodyNode, childCount);
        aStartPt->SetEnd(bodyNode, childCount);
        aEndPt->SetStart(node, offset);
        aEndPt->SetEnd(node, offset);
    }
    return NS_OK;
}


NS_IMETHODIMP nsWebBrowserFind::GetSearchFrames(PRBool *aSearchFrames)
{
    NS_ENSURE_ARG_POINTER(aSearchFrames);
    
    
    
    *aSearchFrames = mSearchSubFrames && mSearchParentFrames;
    return NS_OK;
}

NS_IMETHODIMP nsWebBrowserFind::SetSearchFrames(PRBool aSearchFrames)
{
    mSearchSubFrames = aSearchFrames;
    mSearchParentFrames = aSearchFrames;
    return NS_OK;
}


NS_IMETHODIMP nsWebBrowserFind::GetCurrentSearchFrame(nsIDOMWindow * *aCurrentSearchFrame)
{
    NS_ENSURE_ARG_POINTER(aCurrentSearchFrame);
    nsCOMPtr<nsIDOMWindow> searchFrame = do_QueryReferent(mCurrentSearchFrame);
    NS_IF_ADDREF(*aCurrentSearchFrame = searchFrame);
    return (*aCurrentSearchFrame) ? NS_OK : NS_ERROR_NOT_INITIALIZED;
}

NS_IMETHODIMP nsWebBrowserFind::SetCurrentSearchFrame(nsIDOMWindow * aCurrentSearchFrame)
{
    
    NS_ENSURE_ARG(aCurrentSearchFrame);
    mCurrentSearchFrame = do_GetWeakReference(aCurrentSearchFrame);
    return NS_OK;
}


NS_IMETHODIMP nsWebBrowserFind::GetRootSearchFrame(nsIDOMWindow * *aRootSearchFrame)
{
    NS_ENSURE_ARG_POINTER(aRootSearchFrame);
    nsCOMPtr<nsIDOMWindow> searchFrame = do_QueryReferent(mRootSearchFrame);
    NS_IF_ADDREF(*aRootSearchFrame = searchFrame);
    return (*aRootSearchFrame) ? NS_OK : NS_ERROR_NOT_INITIALIZED;
}

NS_IMETHODIMP nsWebBrowserFind::SetRootSearchFrame(nsIDOMWindow * aRootSearchFrame)
{
    
    NS_ENSURE_ARG(aRootSearchFrame);
    mRootSearchFrame = do_GetWeakReference(aRootSearchFrame);
    return NS_OK;
}


NS_IMETHODIMP nsWebBrowserFind::GetSearchSubframes(PRBool *aSearchSubframes)
{
    NS_ENSURE_ARG_POINTER(aSearchSubframes);
    *aSearchSubframes = mSearchSubFrames;
    return NS_OK;
}

NS_IMETHODIMP nsWebBrowserFind::SetSearchSubframes(PRBool aSearchSubframes)
{
    mSearchSubFrames = aSearchSubframes;
    return NS_OK;
}


NS_IMETHODIMP nsWebBrowserFind::GetSearchParentFrames(PRBool *aSearchParentFrames)
{
    NS_ENSURE_ARG_POINTER(aSearchParentFrames);
    *aSearchParentFrames = mSearchParentFrames;
    return NS_OK;
}

NS_IMETHODIMP nsWebBrowserFind::SetSearchParentFrames(PRBool aSearchParentFrames)
{
    mSearchParentFrames = aSearchParentFrames;
    return NS_OK;
}





nsresult nsWebBrowserFind::SearchInFrame(nsIDOMWindow* aWindow,
                                         PRBool aWrapping,
                                         PRBool* aDidFind)
{
    NS_ENSURE_ARG(aWindow);
    NS_ENSURE_ARG_POINTER(aDidFind);

    *aDidFind = PR_FALSE;

    nsCOMPtr<nsIDOMDocument> domDoc;    
    nsresult rv = aWindow->GetDocument(getter_AddRefs(domDoc));
    NS_ENSURE_SUCCESS(rv, rv);
    if (!domDoc) return NS_ERROR_FAILURE;

    
    
    

    
    nsCOMPtr<nsIDocument> theDoc = do_QueryInterface(domDoc);
    if (!theDoc) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIScriptSecurityManager> secMan =
      do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  
    PRBool hasCap = PR_FALSE;
    secMan->IsCapabilityEnabled("UniversalBrowserWrite", &hasCap);
    if (!hasCap)
      secMan->IsCapabilityEnabled("UniversalXPConnect", &hasCap);

    if (!hasCap) {
      nsCOMPtr<nsIPrincipal> subject;
      rv = secMan->GetSubjectPrincipal(getter_AddRefs(subject));
      NS_ENSURE_SUCCESS(rv, rv);
      if (subject) {
        rv = secMan->CheckSameOriginPrincipal(subject,
                                              theDoc->NodePrincipal());
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }

    if (!mFind) {
        mFind = do_CreateInstance(NS_FIND_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    (void) mFind->SetCaseSensitive(mMatchCase);
    (void) mFind->SetFindBackwards(mFindBackwards);

    
    (void) mFind->SetWordBreaker(0);

    
    
    theDoc->FlushPendingNotifications(Flush_Frames);

    nsCOMPtr<nsISelection> sel;
    GetFrameSelection(aWindow, getter_AddRefs(sel));
    NS_ENSURE_ARG_POINTER(sel);

    nsCOMPtr<nsIDOMRange> searchRange = nsFind::CreateRange();
    NS_ENSURE_ARG_POINTER(searchRange);
    nsCOMPtr<nsIDOMRange> startPt  = nsFind::CreateRange();
    NS_ENSURE_ARG_POINTER(startPt);
    nsCOMPtr<nsIDOMRange> endPt  = nsFind::CreateRange();
    NS_ENSURE_ARG_POINTER(endPt);

    nsCOMPtr<nsIDOMRange> foundRange;

    
    if (!aWrapping)
        rv = GetSearchLimits(searchRange, startPt, endPt, domDoc, sel,
                             PR_FALSE);

    
    
    else
        rv = GetSearchLimits(searchRange, startPt, endPt, domDoc, sel,
                             PR_TRUE);

    NS_ENSURE_SUCCESS(rv, rv);

    rv =  mFind->Find(mSearchString.get(), searchRange, startPt, endPt,
                      getter_AddRefs(foundRange));

    if (NS_SUCCEEDED(rv) && foundRange)
    {
        *aDidFind = PR_TRUE;
        sel->RemoveAllRanges();
        SetSelectionAndScroll(aWindow, foundRange);
    }

    return rv;
}






nsresult nsWebBrowserFind::OnStartSearchFrame(nsIDOMWindow *aWindow)
{
    return ClearFrameSelection(aWindow);
}



nsresult nsWebBrowserFind::OnEndSearchFrame(nsIDOMWindow *aWindow)
{
    return NS_OK;
}

void
nsWebBrowserFind::GetFrameSelection(nsIDOMWindow* aWindow, 
                                    nsISelection** aSel)
{
    *aSel = nsnull;

    nsCOMPtr<nsIDOMDocument> domDoc;    
    aWindow->GetDocument(getter_AddRefs(domDoc));
    if (!domDoc) return;

    nsCOMPtr<nsIDocument> doc(do_QueryInterface(domDoc));
    nsIPresShell* presShell = doc->GetPrimaryShell();
    if (!presShell) return;

    
    
    nsPresContext *presContext = presShell->GetPresContext();

    nsIFrame *frame = nsnull;
    presContext->EventStateManager()->GetFocusedFrame(&frame);
    if (!frame) {
        nsPIDOMWindow *ourWindow = doc->GetWindow();
        if (ourWindow) {
            nsIFocusController *focusController =
                ourWindow->GetRootFocusController();
            if (focusController) {
                nsCOMPtr<nsIDOMElement> focusedElement;
                focusController->GetFocusedElement(getter_AddRefs(focusedElement));
                if (focusedElement) {
                    nsCOMPtr<nsIContent> content(do_QueryInterface(focusedElement));
                    frame = presShell->GetPrimaryFrameFor(content);
                }
            }
        }
    }

    nsCOMPtr<nsISelectionController> selCon;
    if (frame) {
        frame->GetSelectionController(presContext, getter_AddRefs(selCon));
        selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, aSel);
        if (*aSel) {
            PRInt32 count = -1;
            (*aSel)->GetRangeCount(&count);
            if (count > 0) {
                return;
            }
            NS_RELEASE(*aSel);
        }
    }

    selCon = do_QueryInterface(presShell);
    selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, aSel);
}

nsresult nsWebBrowserFind::ClearFrameSelection(nsIDOMWindow *aWindow)
{
    NS_ENSURE_ARG(aWindow);
    nsCOMPtr<nsISelection> selection;
    GetFrameSelection(aWindow, getter_AddRefs(selection));
    if (selection)
        selection->RemoveAllRanges();
    
    return NS_OK;
}

nsresult nsWebBrowserFind::OnFind(nsIDOMWindow *aFoundWindow)
{
    SetCurrentSearchFrame(aFoundWindow);

    
    nsCOMPtr<nsIDOMWindow> lastFocusedWindow = do_QueryReferent(mLastFocusedWindow);
    if (lastFocusedWindow && lastFocusedWindow != aFoundWindow)
        ClearFrameSelection(lastFocusedWindow);

    
    nsCOMPtr<nsPIDOMWindow> ourWindow = do_QueryInterface(aFoundWindow);
    nsIFocusController *focusController = nsnull;
    if (ourWindow)
        focusController = ourWindow->GetRootFocusController();
    if (focusController)
    {
        nsCOMPtr<nsIDOMWindowInternal> windowInt = do_QueryInterface(aFoundWindow);
        focusController->SetFocusedWindow(windowInt);
        mLastFocusedWindow = do_GetWeakReference(aFoundWindow);
    }

    return NS_OK;
}








nsIDocShell *
nsWebBrowserFind::GetDocShellFromWindow(nsIDOMWindow *inWindow)
{
    nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(inWindow));
    if (!window) return nsnull;

    return window->GetDocShell();
}

