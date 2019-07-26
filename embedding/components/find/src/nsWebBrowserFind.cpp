





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
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
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
#include "nsFind.h"
#include "nsError.h"
#include "nsFocusManager.h"
#include "mozilla/Services.h"

#if DEBUG
#include "nsIWebNavigation.h"
#include "nsXPIDLString.h"
#endif

#if defined(XP_MACOSX) && !defined(__LP64__)
#include "nsAutoPtr.h"
#include <Carbon/Carbon.h>
#endif






nsWebBrowserFind::nsWebBrowserFind() :
    mFindBackwards(false),
    mWrapFind(false),
    mEntireWord(false),
    mMatchCase(false),
    mSearchSubFrames(true),
    mSearchParentFrames(true)
{
}

nsWebBrowserFind::~nsWebBrowserFind()
{
}

NS_IMPL_ISUPPORTS2(nsWebBrowserFind, nsIWebBrowserFind, nsIWebBrowserFindInFrames)



NS_IMETHODIMP nsWebBrowserFind::FindNext(bool *outDidFind)
{
    NS_ENSURE_ARG_POINTER(outDidFind);
    *outDidFind = false;

    NS_ENSURE_TRUE(CanFindNext(), NS_ERROR_NOT_INITIALIZED);

    nsresult rv = NS_OK;
    nsCOMPtr<nsIDOMWindow> searchFrame = do_QueryReferent(mCurrentSearchFrame);
    NS_ENSURE_TRUE(searchFrame, NS_ERROR_NOT_INITIALIZED);

    nsCOMPtr<nsIDOMWindow> rootFrame = do_QueryReferent(mRootSearchFrame);
    NS_ENSURE_TRUE(rootFrame, NS_ERROR_NOT_INITIALIZED);
    
    
    
    
    
    
    
    nsCOMPtr<nsIObserverService> observerSvc =
      mozilla::services::GetObserverService();
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
        
        *outDidFind = searchWindowSupports == nullptr;
        if (*outDidFind)
            return NS_OK;
    }

    

    
    
    rv = SearchInFrame(searchFrame, false, outDidFind);
    if (NS_FAILED(rv)) return rv;
    if (*outDidFind)
        return OnFind(searchFrame);     

    
    if (!mSearchSubFrames && !mSearchParentFrames)
        return NS_OK;

    nsIDocShell *rootDocShell = GetDocShellFromWindow(rootFrame);
    if (!rootDocShell) return NS_ERROR_FAILURE;
    
    int32_t enumDirection;
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

    
    
    bool hasMore, doFind = false;
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

            
            
            rv = SearchInFrame(searchFrame, false, outDidFind);
            if (NS_FAILED(rv)) return rv;
            if (*outDidFind)
                return OnFind(searchFrame);     

            OnEndSearchFrame(searchFrame);
        }

        if (curItem.get() == startingItem.get())
            doFind = true;       
    };

    if (!mWrapFind)
    {
        
        SetCurrentSearchFrame(searchFrame);
        return NS_OK;
    }

    
    
    

    
    
    docShellEnumerator = nullptr;
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

        searchFrame = do_GetInterface(curItem, &rv);
        if (NS_FAILED(rv)) break;

        if (curItem.get() == startingItem.get())
        {
            
            
            rv = SearchInFrame(searchFrame, true, outDidFind);
            if (NS_FAILED(rv)) return rv;
            if (*outDidFind)
                return OnFind(searchFrame);        
            break;
        }

        OnStartSearchFrame(searchFrame);

        
        
        rv = SearchInFrame(searchFrame, false, outDidFind);
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
#if defined(XP_MACOSX) && !defined(__LP64__)
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
#if defined(XP_MACOSX) && !defined(__LP64__)
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


NS_IMETHODIMP nsWebBrowserFind::GetFindBackwards(bool *aFindBackwards)
{
    NS_ENSURE_ARG_POINTER(aFindBackwards);
    *aFindBackwards = mFindBackwards;
    return NS_OK;
}

NS_IMETHODIMP nsWebBrowserFind::SetFindBackwards(bool aFindBackwards)
{
    mFindBackwards = aFindBackwards;
    return NS_OK;
}


NS_IMETHODIMP nsWebBrowserFind::GetWrapFind(bool *aWrapFind)
{
    NS_ENSURE_ARG_POINTER(aWrapFind);
    *aWrapFind = mWrapFind;
    return NS_OK;
}
NS_IMETHODIMP nsWebBrowserFind::SetWrapFind(bool aWrapFind)
{
    mWrapFind = aWrapFind;
    return NS_OK;
}


NS_IMETHODIMP nsWebBrowserFind::GetEntireWord(bool *aEntireWord)
{
    NS_ENSURE_ARG_POINTER(aEntireWord);
    *aEntireWord = mEntireWord;
    return NS_OK;
}
NS_IMETHODIMP nsWebBrowserFind::SetEntireWord(bool aEntireWord)
{
    mEntireWord = aEntireWord;
    return NS_OK;
}


NS_IMETHODIMP nsWebBrowserFind::GetMatchCase(bool *aMatchCase)
{
    NS_ENSURE_ARG_POINTER(aMatchCase);
    *aMatchCase = mMatchCase;
    return NS_OK;
}
NS_IMETHODIMP nsWebBrowserFind::SetMatchCase(bool aMatchCase)
{
    mMatchCase = aMatchCase;
    return NS_OK;
}

static bool
IsInNativeAnonymousSubtree(nsIContent* aContent)
{
    while (aContent) {
        nsIContent* bindingParent = aContent->GetBindingParent();
        if (bindingParent == aContent) {
            return true;
        }

        aContent = bindingParent;
    }

    return false;
}

void nsWebBrowserFind::SetSelectionAndScroll(nsIDOMWindow* aWindow,
                                             nsIDOMRange*  aRange)
{
  nsCOMPtr<nsIDOMDocument> domDoc;    
  aWindow->GetDocument(getter_AddRefs(domDoc));
  if (!domDoc) return;

  nsCOMPtr<nsIDocument> doc(do_QueryInterface(domDoc));
  nsIPresShell* presShell = doc->GetShell();
  if (!presShell) return;

  nsCOMPtr<nsIDOMNode> node;
  aRange->GetStartContainer(getter_AddRefs(node));
  nsCOMPtr<nsIContent> content(do_QueryInterface(node));
  nsIFrame* frame = content->GetPrimaryFrame();
  if (!frame)
      return;
  nsCOMPtr<nsISelectionController> selCon;
  frame->GetSelectionController(presShell->GetPresContext(),
                                getter_AddRefs(selCon));
  
  
  
  nsITextControlFrame *tcFrame = nullptr;
  for ( ; content; content = content->GetParent()) {
    if (!IsInNativeAnonymousSubtree(content)) {
      nsIFrame* f = content->GetPrimaryFrame();
      if (!f)
        return;
      tcFrame = do_QueryFrame(f);
      break;
    }
  }

  nsCOMPtr<nsISelection> selection;

  selCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
  selCon->GetSelection(nsISelectionController::SELECTION_NORMAL,
    getter_AddRefs(selection));
  if (selection) {
    selection->RemoveAllRanges();
    selection->AddRange(aRange);

    nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
    if (fm) {
      if (tcFrame) {
        nsCOMPtr<nsIDOMElement> newFocusedElement(do_QueryInterface(content));
        fm->SetFocus(newFocusedElement, nsIFocusManager::FLAG_NOSCROLL);
      }
      else  {
        nsCOMPtr<nsIDOMElement> result;
        fm->MoveFocus(aWindow, nullptr, nsIFocusManager::MOVEFOCUS_CARET,
                      nsIFocusManager::FLAG_NOSCROLL,
                      getter_AddRefs(result));
      }
    }

    
    

    
    
    selCon->ScrollSelectionIntoView
      (nsISelectionController::SELECTION_NORMAL,
       nsISelectionController::SELECTION_WHOLE_SELECTION,
       nsISelectionController::SCROLL_CENTER_VERTICALLY |
       nsISelectionController::SCROLL_SYNCHRONOUS);
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

    uint32_t childCount = bodyContent->GetChildCount();

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
                                  bool aWrap)
{
    NS_ENSURE_ARG_POINTER(aSel);

    
    int32_t count = -1;
    nsresult rv = aSel->GetRangeCount(&count);
    NS_ENSURE_SUCCESS(rv, rv);
    if (count < 1)
        return SetRangeAroundDocument(aSearchRange, aStartPt, aEndPt, aDoc);

    
    nsCOMPtr<nsIDOMNode> bodyNode;
    rv = GetRootNode(aDoc, getter_AddRefs(bodyNode));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIContent> bodyContent (do_QueryInterface(bodyNode));
    NS_ENSURE_ARG_POINTER(bodyContent);

    uint32_t childCount = bodyContent->GetChildCount();

    
    

    nsCOMPtr<nsIDOMRange> range;
    nsCOMPtr<nsIDOMNode> node;
    int32_t offset;

    
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


NS_IMETHODIMP nsWebBrowserFind::GetSearchFrames(bool *aSearchFrames)
{
    NS_ENSURE_ARG_POINTER(aSearchFrames);
    
    
    
    *aSearchFrames = mSearchSubFrames && mSearchParentFrames;
    return NS_OK;
}

NS_IMETHODIMP nsWebBrowserFind::SetSearchFrames(bool aSearchFrames)
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


NS_IMETHODIMP nsWebBrowserFind::GetSearchSubframes(bool *aSearchSubframes)
{
    NS_ENSURE_ARG_POINTER(aSearchSubframes);
    *aSearchSubframes = mSearchSubFrames;
    return NS_OK;
}

NS_IMETHODIMP nsWebBrowserFind::SetSearchSubframes(bool aSearchSubframes)
{
    mSearchSubFrames = aSearchSubframes;
    return NS_OK;
}


NS_IMETHODIMP nsWebBrowserFind::GetSearchParentFrames(bool *aSearchParentFrames)
{
    NS_ENSURE_ARG_POINTER(aSearchParentFrames);
    *aSearchParentFrames = mSearchParentFrames;
    return NS_OK;
}

NS_IMETHODIMP nsWebBrowserFind::SetSearchParentFrames(bool aSearchParentFrames)
{
    mSearchParentFrames = aSearchParentFrames;
    return NS_OK;
}





nsresult nsWebBrowserFind::SearchInFrame(nsIDOMWindow* aWindow,
                                         bool aWrapping,
                                         bool* aDidFind)
{
    NS_ENSURE_ARG(aWindow);
    NS_ENSURE_ARG_POINTER(aDidFind);

    *aDidFind = false;

    nsCOMPtr<nsIDOMDocument> domDoc;    
    nsresult rv = aWindow->GetDocument(getter_AddRefs(domDoc));
    NS_ENSURE_SUCCESS(rv, rv);
    if (!domDoc) return NS_ERROR_FAILURE;

    
    

    
    nsCOMPtr<nsIDocument> theDoc = do_QueryInterface(domDoc);
    if (!theDoc) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIScriptSecurityManager> secMan =
      do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  
    nsCOMPtr<nsIPrincipal> subject;
    rv = secMan->GetSubjectPrincipal(getter_AddRefs(subject));
    NS_ENSURE_SUCCESS(rv, rv);

    if (subject) {
        bool subsumes;
        rv = subject->Subsumes(theDoc->NodePrincipal(), &subsumes);
        NS_ENSURE_SUCCESS(rv, rv);
        if (!subsumes) {
            return NS_ERROR_DOM_PROP_ACCESS_DENIED;
        }
    }

    nsCOMPtr<nsIFind> find = do_CreateInstance(NS_FIND_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    (void) find->SetCaseSensitive(mMatchCase);
    (void) find->SetFindBackwards(mFindBackwards);

    
    (void) find->SetWordBreaker(0);

    
    
    theDoc->FlushPendingNotifications(Flush_Frames);

    nsCOMPtr<nsISelection> sel;
    GetFrameSelection(aWindow, getter_AddRefs(sel));
    NS_ENSURE_ARG_POINTER(sel);

    nsCOMPtr<nsIDOMRange> searchRange = nsFind::CreateRange(theDoc);
    NS_ENSURE_ARG_POINTER(searchRange);
    nsCOMPtr<nsIDOMRange> startPt  = nsFind::CreateRange(theDoc);
    NS_ENSURE_ARG_POINTER(startPt);
    nsCOMPtr<nsIDOMRange> endPt  = nsFind::CreateRange(theDoc);
    NS_ENSURE_ARG_POINTER(endPt);

    nsCOMPtr<nsIDOMRange> foundRange;

    
    if (!aWrapping)
        rv = GetSearchLimits(searchRange, startPt, endPt, domDoc, sel,
                             false);

    
    
    else
        rv = GetSearchLimits(searchRange, startPt, endPt, domDoc, sel,
                             true);

    NS_ENSURE_SUCCESS(rv, rv);

    rv =  find->Find(mSearchString.get(), searchRange, startPt, endPt,
                     getter_AddRefs(foundRange));

    if (NS_SUCCEEDED(rv) && foundRange)
    {
        *aDidFind = true;
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
    *aSel = nullptr;

    nsCOMPtr<nsIDOMDocument> domDoc;    
    aWindow->GetDocument(getter_AddRefs(domDoc));
    if (!domDoc) return;

    nsCOMPtr<nsIDocument> doc(do_QueryInterface(domDoc));
    nsIPresShell* presShell = doc->GetShell();
    if (!presShell) return;

    
    
    nsPresContext *presContext = presShell->GetPresContext();

    nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aWindow));

    nsCOMPtr<nsPIDOMWindow> focusedWindow;
    nsCOMPtr<nsIContent> focusedContent =
      nsFocusManager::GetFocusedDescendant(window, false, getter_AddRefs(focusedWindow));

    nsIFrame *frame = focusedContent ? focusedContent->GetPrimaryFrame() : nullptr;

    nsCOMPtr<nsISelectionController> selCon;
    if (frame) {
        frame->GetSelectionController(presContext, getter_AddRefs(selCon));
        selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, aSel);
        if (*aSel) {
            int32_t count = -1;
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

    nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
    if (fm) {
      
      
      nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aFoundWindow));
      NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);

      nsCOMPtr<nsIDOMElement> frameElement = window->GetFrameElementInternal();
      if (frameElement)
        fm->SetFocus(frameElement, 0);

      mLastFocusedWindow = do_GetWeakReference(aFoundWindow);
    }

    return NS_OK;
}








nsIDocShell *
nsWebBrowserFind::GetDocShellFromWindow(nsIDOMWindow *inWindow)
{
    nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(inWindow));
    if (!window) return nullptr;

    return window->GetDocShell();
}

