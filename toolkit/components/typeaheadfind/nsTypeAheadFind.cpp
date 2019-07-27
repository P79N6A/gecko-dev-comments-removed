




#include "nsCOMPtr.h"
#include "nsMemory.h"
#include "nsIServiceManager.h"
#include "mozilla/ModuleUtils.h"
#include "mozilla/Services.h"
#include "nsIWebBrowserChrome.h"
#include "nsCURILoader.h"
#include "nsCycleCollectionParticipant.h"
#include "nsNetUtil.h"
#include "nsIURL.h"
#include "nsIURI.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeOwner.h"
#include "nsISimpleEnumerator.h"
#include "nsPIDOMWindow.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsString.h"
#include "nsCRT.h"

#include "nsIDOMNode.h"
#include "mozilla/dom/Element.h"
#include "nsIFrame.h"
#include "nsFrameTraversal.h"
#include "nsIImageDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDocument.h"
#include "nsISelection.h"
#include "nsTextFragment.h"
#include "nsIDOMNSEditableElement.h"
#include "nsIEditor.h"

#include "nsIDocShellTreeItem.h"
#include "nsIWebNavigation.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsContentCID.h"
#include "nsLayoutCID.h"
#include "nsWidgetsCID.h"
#include "nsIFormControl.h"
#include "nsNameSpaceManager.h"
#include "nsIWindowWatcher.h"
#include "nsIObserverService.h"
#include "nsFocusManager.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/Link.h"
#include "nsRange.h"
#include "nsXBLBinding.h"

#include "nsTypeAheadFind.h"

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsTypeAheadFind)
  NS_INTERFACE_MAP_ENTRY(nsITypeAheadFind)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsITypeAheadFind)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsTypeAheadFind)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsTypeAheadFind)

NS_IMPL_CYCLE_COLLECTION(nsTypeAheadFind, mFoundLink, mFoundEditable,
                         mCurrentWindow, mStartFindRange, mSearchRange,
                         mStartPointRange, mEndPointRange, mSoundInterface,
                         mFind, mFoundRange)

static NS_DEFINE_CID(kFrameTraversalCID, NS_FRAMETRAVERSAL_CID);

#define NS_FIND_CONTRACTID "@mozilla.org/embedcomp/rangefind;1"

nsTypeAheadFind::nsTypeAheadFind():
  mStartLinksOnlyPref(false),
  mCaretBrowsingOn(false),
  mDidAddObservers(false),
  mLastFindLength(0),
  mIsSoundInitialized(false),
  mCaseSensitive(false)
{
}

nsTypeAheadFind::~nsTypeAheadFind()
{
  nsCOMPtr<nsIPrefBranch> prefInternal(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (prefInternal) {
    prefInternal->RemoveObserver("accessibility.typeaheadfind", this);
    prefInternal->RemoveObserver("accessibility.browsewithcaret", this);
  }
}

nsresult
nsTypeAheadFind::Init(nsIDocShell* aDocShell)
{
  nsCOMPtr<nsIPrefBranch> prefInternal(do_GetService(NS_PREFSERVICE_CONTRACTID));

  mSearchRange = nullptr;
  mStartPointRange = nullptr;
  mEndPointRange = nullptr;
  if (!prefInternal || !EnsureFind())
    return NS_ERROR_FAILURE;

  SetDocShell(aDocShell);

  if (!mDidAddObservers) {
    mDidAddObservers = true;
    
    nsresult rv = prefInternal->AddObserver("accessibility.browsewithcaret", this, true);
    NS_ENSURE_SUCCESS(rv, rv);

    
    PrefsReset();

    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (os) {
      os->AddObserver(this, DOM_WINDOW_DESTROYED_TOPIC, true);
    }
  }
  return NS_OK;
}

nsresult
nsTypeAheadFind::PrefsReset()
{
  nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
  NS_ENSURE_TRUE(prefBranch, NS_ERROR_FAILURE);

  prefBranch->GetBoolPref("accessibility.typeaheadfind.startlinksonly",
                          &mStartLinksOnlyPref);

  bool isSoundEnabled = true;
  prefBranch->GetBoolPref("accessibility.typeaheadfind.enablesound",
                           &isSoundEnabled);
  nsXPIDLCString soundStr;
  if (isSoundEnabled)
    prefBranch->GetCharPref("accessibility.typeaheadfind.soundURL", getter_Copies(soundStr));

  mNotFoundSoundURL = soundStr;

  prefBranch->GetBoolPref("accessibility.browsewithcaret",
                          &mCaretBrowsingOn);

  return NS_OK;
}

NS_IMETHODIMP
nsTypeAheadFind::SetCaseSensitive(bool isCaseSensitive)
{
  mCaseSensitive = isCaseSensitive;

  if (mFind) {
    mFind->SetCaseSensitive(mCaseSensitive);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsTypeAheadFind::GetCaseSensitive(bool* isCaseSensitive)
{
  *isCaseSensitive = mCaseSensitive;

  return NS_OK;
}

NS_IMETHODIMP
nsTypeAheadFind::SetDocShell(nsIDocShell* aDocShell)
{
  mDocShell = do_GetWeakReference(aDocShell);

  mWebBrowserFind = do_GetInterface(aDocShell);
  NS_ENSURE_TRUE(mWebBrowserFind, NS_ERROR_FAILURE);

  nsCOMPtr<nsIPresShell> presShell;
  presShell = aDocShell->GetPresShell();
  mPresShell = do_GetWeakReference(presShell);

  ReleaseStrongMemberVariables();
  return NS_OK;
}

void
nsTypeAheadFind::ReleaseStrongMemberVariables()
{
  mStartFindRange = nullptr;
  mStartPointRange = nullptr;
  mSearchRange = nullptr;
  mEndPointRange = nullptr;

  mFoundLink = nullptr;
  mFoundEditable = nullptr;
  mFoundRange = nullptr;
  mCurrentWindow = nullptr;

  mSelectionController = nullptr;

  mFind = nullptr;
}

NS_IMETHODIMP
nsTypeAheadFind::SetSelectionModeAndRepaint(int16_t aToggle)
{
  nsCOMPtr<nsISelectionController> selectionController = 
    do_QueryReferent(mSelectionController);
  if (!selectionController) {
    return NS_OK;
  }

  selectionController->SetDisplaySelection(aToggle);
  selectionController->RepaintSelection(nsISelectionController::SELECTION_NORMAL);

  return NS_OK;
}

NS_IMETHODIMP
nsTypeAheadFind::CollapseSelection()
{
  nsCOMPtr<nsISelectionController> selectionController = 
    do_QueryReferent(mSelectionController);
  if (!selectionController) {
    return NS_OK;
  }

  nsCOMPtr<nsISelection> selection;
  selectionController->GetSelection(nsISelectionController::SELECTION_NORMAL,
                                     getter_AddRefs(selection));
  if (selection)
    selection->CollapseToStart();

  return NS_OK;
}

NS_IMETHODIMP
nsTypeAheadFind::Observe(nsISupports *aSubject, const char *aTopic,
                         const char16_t *aData)
{
  if (!nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    return PrefsReset();
  } else if (!nsCRT::strcmp(aTopic, DOM_WINDOW_DESTROYED_TOPIC) &&
             SameCOMIdentity(aSubject, mCurrentWindow)) {
    ReleaseStrongMemberVariables();
  }

  return NS_OK;
}

void
nsTypeAheadFind::SaveFind()
{
  if (mWebBrowserFind)
    mWebBrowserFind->SetSearchString(mTypeAheadBuffer.get());
  
  
  mLastFindLength = mTypeAheadBuffer.Length();
}

void
nsTypeAheadFind::PlayNotFoundSound()
{
  if (mNotFoundSoundURL.IsEmpty())    
    return;

  if (!mSoundInterface)
    mSoundInterface = do_CreateInstance("@mozilla.org/sound;1");

  if (mSoundInterface) {
    mIsSoundInitialized = true;

    if (mNotFoundSoundURL.EqualsLiteral("beep")) {
      mSoundInterface->Beep();
      return;
    }

    nsCOMPtr<nsIURI> soundURI;
    if (mNotFoundSoundURL.EqualsLiteral("default"))
      NS_NewURI(getter_AddRefs(soundURI), NS_LITERAL_CSTRING(TYPEAHEADFIND_NOTFOUND_WAV_URL));
    else
      NS_NewURI(getter_AddRefs(soundURI), mNotFoundSoundURL);

    nsCOMPtr<nsIURL> soundURL(do_QueryInterface(soundURI));
    if (soundURL)
      mSoundInterface->Play(soundURL);
  }
}

nsresult
nsTypeAheadFind::FindItNow(nsIPresShell *aPresShell, bool aIsLinksOnly,
                           bool aIsFirstVisiblePreferred, bool aFindPrev,
                           uint16_t* aResult)
{
  *aResult = FIND_NOTFOUND;
  mFoundLink = nullptr;
  mFoundEditable = nullptr;
  mFoundRange = nullptr;
  mCurrentWindow = nullptr;
  nsCOMPtr<nsIPresShell> startingPresShell (GetPresShell());
  if (!startingPresShell) {    
    nsCOMPtr<nsIDocShell> ds = do_QueryReferent(mDocShell);
    NS_ENSURE_TRUE(ds, NS_ERROR_FAILURE);

    startingPresShell = ds->GetPresShell();
    mPresShell = do_GetWeakReference(startingPresShell);    
  }  

  nsCOMPtr<nsIPresShell> presShell(aPresShell);

  if (!presShell) {
    presShell = startingPresShell;  

    if (!presShell)
      return NS_ERROR_FAILURE;
  }

  nsRefPtr<nsPresContext> presContext = presShell->GetPresContext();

  if (!presContext)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsISelectionController> selectionController = 
    do_QueryReferent(mSelectionController);
  if (!selectionController) {
    GetSelection(presShell, getter_AddRefs(selectionController),
                 getter_AddRefs(selection)); 
    mSelectionController = do_GetWeakReference(selectionController);
  } else {
    selectionController->GetSelection(
      nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));
  }
 
  nsCOMPtr<nsIDocShell> startingDocShell(presContext->GetDocShell());
  NS_ASSERTION(startingDocShell, "Bug 175321 Crashes with Type Ahead Find [@ nsTypeAheadFind::FindItNow]");
  if (!startingDocShell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocShellTreeItem> rootContentTreeItem;
  nsCOMPtr<nsIDocShell> currentDocShell;

  startingDocShell->GetSameTypeRootTreeItem(getter_AddRefs(rootContentTreeItem));
  nsCOMPtr<nsIDocShell> rootContentDocShell =
    do_QueryInterface(rootContentTreeItem);

  if (!rootContentDocShell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsISimpleEnumerator> docShellEnumerator;
  rootContentDocShell->GetDocShellEnumerator(nsIDocShellTreeItem::typeContent,
                                             nsIDocShell::ENUMERATE_FORWARDS,
                                             getter_AddRefs(docShellEnumerator));

  
  nsCOMPtr<nsISupports> currentContainer =
    do_QueryInterface(rootContentDocShell);

  
  
  bool hasMoreDocShells;

  while (NS_SUCCEEDED(docShellEnumerator->HasMoreElements(&hasMoreDocShells)) && hasMoreDocShells) {
    docShellEnumerator->GetNext(getter_AddRefs(currentContainer));
    currentDocShell = do_QueryInterface(currentContainer);
    if (!currentDocShell || currentDocShell == startingDocShell || aIsFirstVisiblePreferred)
      break;    
  }

  
  nsCOMPtr<nsIDOMRange> returnRange;
  nsCOMPtr<nsIPresShell> focusedPS;
  if (NS_FAILED(GetSearchContainers(currentContainer,
                                    (!aIsFirstVisiblePreferred ||
                                     mStartFindRange) ?
                                    selectionController.get() : nullptr,
                                    aIsFirstVisiblePreferred,  aFindPrev,
                                    getter_AddRefs(presShell),
                                    getter_AddRefs(presContext)))) {
    return NS_ERROR_FAILURE;
  }

  int16_t rangeCompareResult = 0;
  if (!mStartPointRange) {
    mStartPointRange = new nsRange(presShell->GetDocument());
  }

  mStartPointRange->CompareBoundaryPoints(nsIDOMRange::START_TO_START, mSearchRange, &rangeCompareResult);
  
  bool hasWrapped = (rangeCompareResult < 0);

  if (mTypeAheadBuffer.IsEmpty() || !EnsureFind())
    return NS_ERROR_FAILURE;

  mFind->SetFindBackwards(aFindPrev);

  while (true) {    
    while (true) {  
      mFind->Find(mTypeAheadBuffer.get(), mSearchRange, mStartPointRange,
                  mEndPointRange, getter_AddRefs(returnRange));
      
      if (!returnRange)
        break;  

      
      bool isInsideLink = false, isStartingLink = false;

      if (aIsLinksOnly) {
        
        RangeStartsInsideLink(returnRange, presShell, &isInsideLink,
                              &isStartingLink);
      }

      bool usesIndependentSelection;
      if (!IsRangeVisible(presShell, presContext, returnRange,
                          aIsFirstVisiblePreferred, false,
                          getter_AddRefs(mStartPointRange), 
                          &usesIndependentSelection) ||
          (aIsLinksOnly && !isInsideLink) ||
          (mStartLinksOnlyPref && aIsLinksOnly && !isStartingLink)) {
        
        
        
        
        if (aFindPrev) {
          
          
          
          int16_t compareResult;
          nsresult rv =
            mStartPointRange->CompareBoundaryPoints(nsIDOMRange::START_TO_END,
                                                    returnRange, &compareResult);
          if (NS_SUCCEEDED(rv) && compareResult <= 0) {
            
            mStartPointRange->Collapse(false);
          } else {
            
            returnRange->CloneRange(getter_AddRefs(mStartPointRange));
            mStartPointRange->Collapse(true);
          }
        } else {
          
          
          
          int16_t compareResult;
          nsresult rv =
            mStartPointRange->CompareBoundaryPoints(nsIDOMRange::END_TO_START,
                                                    returnRange, &compareResult);
          if (NS_SUCCEEDED(rv) && compareResult >= 0) {
            
            mStartPointRange->Collapse(true);
          } else {
            
            returnRange->CloneRange(getter_AddRefs(mStartPointRange));
            mStartPointRange->Collapse(false);
          }
        }
        continue;
      }

      mFoundRange = returnRange;

      
      
      if (selection) {
        selection->CollapseToStart();
        SetSelectionModeAndRepaint(nsISelectionController::SELECTION_ON);
      }

      
      if (presShell != startingPresShell) {
        
        mPresShell = do_GetWeakReference(presShell);
      }

      nsCOMPtr<nsIDocument> document =
        do_QueryInterface(presShell->GetDocument());
      NS_ASSERTION(document, "Wow, presShell doesn't have document!");
      if (!document)
        return NS_ERROR_UNEXPECTED;

      nsCOMPtr<nsPIDOMWindow> window = document->GetInnerWindow();
      NS_ASSERTION(window, "document has no window");
      if (!window)
        return NS_ERROR_UNEXPECTED;

      nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
      if (usesIndependentSelection) {
        


        bool shouldFocusEditableElement = false;
        if (fm) {
          nsCOMPtr<nsIDOMWindow> focusedWindow;
          nsresult rv = fm->GetFocusedWindow(getter_AddRefs(focusedWindow));
          if (NS_SUCCEEDED(rv)) {
            nsCOMPtr<nsPIDOMWindow> fwPI(do_QueryInterface(focusedWindow, &rv));
            if (NS_SUCCEEDED(rv)) {
              nsCOMPtr<nsIDocShellTreeItem> fwTreeItem
                (do_QueryInterface(fwPI->GetDocShell(), &rv));
              if (NS_SUCCEEDED(rv)) {
                nsCOMPtr<nsIDocShellTreeItem> fwRootTreeItem;
                rv = fwTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(fwRootTreeItem));
                if (NS_SUCCEEDED(rv) && fwRootTreeItem == rootContentTreeItem)
                  shouldFocusEditableElement = true;
              }
            }
          }
        }

        
        
        
        nsCOMPtr<nsIDOMNode> node;
        returnRange->GetStartContainer(getter_AddRefs(node));
        while (node) {
          nsCOMPtr<nsIDOMNSEditableElement> editable = do_QueryInterface(node);
          if (editable) {
            
            
            nsCOMPtr<nsIEditor> editor;
            editable->GetEditor(getter_AddRefs(editor));
            NS_ASSERTION(editor, "Editable element has no editor!");
            if (!editor) {
              break;
            }
            editor->GetSelectionController(
              getter_AddRefs(selectionController));
            if (selectionController) {
              selectionController->GetSelection(
                nsISelectionController::SELECTION_NORMAL, 
                getter_AddRefs(selection));
            }
            mFoundEditable = do_QueryInterface(node);

            if (!shouldFocusEditableElement)
              break;

            
            if (fm)
              fm->SetFocus(mFoundEditable, 0);
            break;
          }
          nsIDOMNode* tmp = node;
          tmp->GetParentNode(getter_AddRefs(node));
        }

        
        
        
        
        
        
        NS_ASSERTION(mFoundEditable, "Independent selection controller on "
                     "non-editable element!");
      }

      if (!mFoundEditable) {
        
        
        GetSelection(presShell, getter_AddRefs(selectionController), 
                     getter_AddRefs(selection));
      }
      mSelectionController = do_GetWeakReference(selectionController);

      
      if (selection) {
        selection->RemoveAllRanges();
        selection->AddRange(returnRange);
      }

      if (!mFoundEditable && fm) {
        nsCOMPtr<nsIDOMWindow> win = do_QueryInterface(window);
        fm->MoveFocus(win, nullptr, nsIFocusManager::MOVEFOCUS_CARET,
                      nsIFocusManager::FLAG_NOSCROLL | nsIFocusManager::FLAG_NOSWITCHFRAME,
                      getter_AddRefs(mFoundLink));
      }

      
      
      
      
      if (selectionController) {
        
        
        SetSelectionModeAndRepaint(nsISelectionController::SELECTION_ATTENTION);
        selectionController->ScrollSelectionIntoView(
          nsISelectionController::SELECTION_NORMAL, 
          nsISelectionController::SELECTION_WHOLE_SELECTION,
          nsISelectionController::SCROLL_CENTER_VERTICALLY |
          nsISelectionController::SCROLL_SYNCHRONOUS);
      }

      mCurrentWindow = window;
      *aResult = hasWrapped ? FIND_WRAPPED : FIND_FOUND;
      return NS_OK;
    }

    

    
    bool hasTriedFirstDoc = false;
    do {
      
      if (NS_SUCCEEDED(docShellEnumerator->HasMoreElements(&hasMoreDocShells))
          && hasMoreDocShells) {
        docShellEnumerator->GetNext(getter_AddRefs(currentContainer));
        NS_ASSERTION(currentContainer, "HasMoreElements lied to us!");
        currentDocShell = do_QueryInterface(currentContainer);

        if (currentDocShell)
          break;
      }
      else if (hasTriedFirstDoc)  
        return NS_ERROR_FAILURE;  

      
      rootContentDocShell->GetDocShellEnumerator(nsIDocShellTreeItem::typeContent,
                                                 nsIDocShell::ENUMERATE_FORWARDS,
                                                 getter_AddRefs(docShellEnumerator));
      hasTriedFirstDoc = true;      
    } while (docShellEnumerator);  

    bool continueLoop = false;
    if (currentDocShell != startingDocShell)
      continueLoop = true;  
    else if (!hasWrapped || aIsFirstVisiblePreferred) {
      
      
      
      
      aIsFirstVisiblePreferred = false;
      hasWrapped = true;
      continueLoop = true; 
    }

    if (continueLoop) {
      if (NS_FAILED(GetSearchContainers(currentContainer, nullptr,
                                        aIsFirstVisiblePreferred, aFindPrev,
                                        getter_AddRefs(presShell),
                                        getter_AddRefs(presContext)))) {
        continue;
      }

      if (aFindPrev) {
        
        
        nsCOMPtr<nsIDOMRange> tempRange;
        mStartPointRange->CloneRange(getter_AddRefs(tempRange));
        if (!mEndPointRange) {
          mEndPointRange = new nsRange(presShell->GetDocument());
        }

        mStartPointRange = mEndPointRange;
        mEndPointRange = tempRange;
      }

      continue;
    }

    
    break;
  }   

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsTypeAheadFind::GetSearchString(nsAString& aSearchString)
{
  aSearchString = mTypeAheadBuffer;
  return NS_OK;
}

NS_IMETHODIMP
nsTypeAheadFind::GetFoundLink(nsIDOMElement** aFoundLink)
{
  NS_ENSURE_ARG_POINTER(aFoundLink);
  *aFoundLink = mFoundLink;
  NS_IF_ADDREF(*aFoundLink);
  return NS_OK;
}

NS_IMETHODIMP
nsTypeAheadFind::GetFoundEditable(nsIDOMElement** aFoundEditable)
{
  NS_ENSURE_ARG_POINTER(aFoundEditable);
  *aFoundEditable = mFoundEditable;
  NS_IF_ADDREF(*aFoundEditable);
  return NS_OK;
}

NS_IMETHODIMP
nsTypeAheadFind::GetCurrentWindow(nsIDOMWindow** aCurrentWindow)
{
  NS_ENSURE_ARG_POINTER(aCurrentWindow);
  *aCurrentWindow = mCurrentWindow;
  NS_IF_ADDREF(*aCurrentWindow);
  return NS_OK;
}

nsresult
nsTypeAheadFind::GetSearchContainers(nsISupports *aContainer,
                                     nsISelectionController *aSelectionController,
                                     bool aIsFirstVisiblePreferred,
                                     bool aFindPrev,
                                     nsIPresShell **aPresShell,
                                     nsPresContext **aPresContext)
{
  NS_ENSURE_ARG_POINTER(aContainer);
  NS_ENSURE_ARG_POINTER(aPresShell);
  NS_ENSURE_ARG_POINTER(aPresContext);

  *aPresShell = nullptr;
  *aPresContext = nullptr;

  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aContainer));
  if (!docShell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresShell> presShell = docShell->GetPresShell();

  nsRefPtr<nsPresContext> presContext;
  docShell->GetPresContext(getter_AddRefs(presContext));

  if (!presShell || !presContext)
    return NS_ERROR_FAILURE;

  nsIDocument* doc = presShell->GetDocument();

  if (!doc)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> rootContent;
  nsCOMPtr<nsIDOMHTMLDocument> htmlDoc(do_QueryInterface(doc));
  if (htmlDoc) {
    nsCOMPtr<nsIDOMHTMLElement> bodyEl;
    htmlDoc->GetBody(getter_AddRefs(bodyEl));
    rootContent = do_QueryInterface(bodyEl);
  }

  if (!rootContent)
    rootContent = doc->GetRootElement();
 
  nsCOMPtr<nsIDOMNode> rootNode(do_QueryInterface(rootContent));

  if (!rootNode)
    return NS_ERROR_FAILURE;

  if (!mSearchRange) {
    mSearchRange = new nsRange(doc);
  }
  nsCOMPtr<nsIDOMNode> searchRootNode = rootNode;

  
  
  
  
  
  nsXBLBinding* binding = rootContent->GetXBLBinding();
  if (binding) {
    nsIContent* anonContent = binding->GetAnonymousContent();
    if (anonContent) {
      searchRootNode = do_QueryInterface(anonContent->GetFirstChild());
    }
  }
  mSearchRange->SelectNodeContents(searchRootNode);

  if (!mStartPointRange) {
    mStartPointRange = new nsRange(doc);
  }
  mStartPointRange->SetStart(searchRootNode, 0);
  mStartPointRange->Collapse(true); 

  if (!mEndPointRange) {
    mEndPointRange = new nsRange(doc);
  }
  nsCOMPtr<nsINode> searchRootTmp = do_QueryInterface(searchRootNode);
  mEndPointRange->SetEnd(searchRootNode, searchRootTmp->Length());
  mEndPointRange->Collapse(false); 

  
  
  nsCOMPtr<nsIDOMRange> currentSelectionRange;
  nsCOMPtr<nsIPresShell> selectionPresShell = GetPresShell();
  if (aSelectionController && selectionPresShell && selectionPresShell == presShell) {
    nsCOMPtr<nsISelection> selection;
    aSelectionController->GetSelection(
      nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));
    if (selection)
      selection->GetRangeAt(0, getter_AddRefs(currentSelectionRange));
  }

  if (!currentSelectionRange) {
    
    
    
    IsRangeVisible(presShell, presContext, mSearchRange, 
                   aIsFirstVisiblePreferred, true, 
                   getter_AddRefs(mStartPointRange), nullptr);
  }
  else {
    int32_t startOffset;
    nsCOMPtr<nsIDOMNode> startNode;
    if (aFindPrev) {
      currentSelectionRange->GetStartContainer(getter_AddRefs(startNode));
      currentSelectionRange->GetStartOffset(&startOffset);
    } else {
      currentSelectionRange->GetEndContainer(getter_AddRefs(startNode));
      currentSelectionRange->GetEndOffset(&startOffset);
    }
    if (!startNode)
      startNode = rootNode;    

    
    mStartPointRange->SelectNode(startNode);
    mStartPointRange->SetStart(startNode, startOffset);
  }

  mStartPointRange->Collapse(true); 

  presShell.forget(aPresShell);
  presContext.forget(aPresContext);

  return NS_OK;
}

void
nsTypeAheadFind::RangeStartsInsideLink(nsIDOMRange *aRange,
                                       nsIPresShell *aPresShell,
                                       bool *aIsInsideLink,
                                       bool *aIsStartingLink)
{
  *aIsInsideLink = false;
  *aIsStartingLink = true;

  
  nsCOMPtr<nsIDOMNode> startNode;
  nsCOMPtr<nsIContent> startContent, origContent;
  aRange->GetStartContainer(getter_AddRefs(startNode));
  int32_t startOffset;
  aRange->GetStartOffset(&startOffset);

  startContent = do_QueryInterface(startNode);
  if (!startContent) {
    NS_NOTREACHED("startContent should never be null");
    return;
  }
  origContent = startContent;

  if (startContent->IsElement()) {
    nsIContent *childContent = startContent->GetChildAt(startOffset);
    if (childContent) {
      startContent = childContent;
    }
  }
  else if (startOffset > 0) {
    const nsTextFragment *textFrag = startContent->GetText();
    if (textFrag) {
      
      for (int32_t index = 0; index < startOffset; index++) {
        
        if (!mozilla::dom::IsSpaceCharacter(textFrag->CharAt(index))) {
          *aIsStartingLink = false;  

          break;
        }
      }
    }
  }

  

  
  

  nsCOMPtr<nsIAtom> tag, hrefAtom(do_GetAtom("href"));
  nsCOMPtr<nsIAtom> typeAtom(do_GetAtom("type"));

  while (true) {
    
    

    if (startContent->IsHTMLElement()) {
      nsCOMPtr<mozilla::dom::Link> link(do_QueryInterface(startContent));
      if (link) {
        
        *aIsInsideLink = startContent->HasAttr(kNameSpaceID_None, hrefAtom);
        return;
      }
    }
    else {
      
      *aIsInsideLink = startContent->HasAttr(kNameSpaceID_XLink, hrefAtom);
      if (*aIsInsideLink) {
        if (!startContent->AttrValueIs(kNameSpaceID_XLink, typeAtom,
                                       NS_LITERAL_STRING("simple"),
                                       eCaseMatters)) {
          *aIsInsideLink = false;  
        }

        return;
      }
    }

    
    nsCOMPtr<nsIContent> parent = startContent->GetParent();
    if (!parent)
      break;

    nsIContent* parentsFirstChild = parent->GetFirstChild();

    
    if (parentsFirstChild && parentsFirstChild->TextIsOnlyWhitespace()) {
      parentsFirstChild = parentsFirstChild->GetNextSibling();
    }

    if (parentsFirstChild != startContent) {
      
      
      *aIsStartingLink = false;
    }

    startContent = parent;
  }

  *aIsStartingLink = false;
}


NS_IMETHODIMP
nsTypeAheadFind::FindAgain(bool aFindBackwards, bool aLinksOnly,
                           uint16_t* aResult)

{
  *aResult = FIND_NOTFOUND;

  if (!mTypeAheadBuffer.IsEmpty())
    
    
    FindItNow(nullptr, aLinksOnly, false, aFindBackwards, aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsTypeAheadFind::Find(const nsAString& aSearchString, bool aLinksOnly,
                      uint16_t* aResult)
{
  *aResult = FIND_NOTFOUND;

  nsCOMPtr<nsIPresShell> presShell (GetPresShell());
  if (!presShell) {
    nsCOMPtr<nsIDocShell> ds (do_QueryReferent(mDocShell));
    NS_ENSURE_TRUE(ds, NS_ERROR_FAILURE);

    presShell = ds->GetPresShell();
    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);
    mPresShell = do_GetWeakReference(presShell);
  }

  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsISelectionController> selectionController =
    do_QueryReferent(mSelectionController);
  if (!selectionController) {
    GetSelection(presShell, getter_AddRefs(selectionController),
                 getter_AddRefs(selection)); 
    mSelectionController = do_GetWeakReference(selectionController);
  } else {
    selectionController->GetSelection(
      nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));
  }

  if (selection)
    selection->CollapseToStart();

  if (aSearchString.IsEmpty()) {
    mTypeAheadBuffer.Truncate();

    
    
    mStartFindRange = nullptr;
    mSelectionController = nullptr;

    *aResult = FIND_FOUND;
    return NS_OK;
  }

  bool atEnd = false;    
  if (mTypeAheadBuffer.Length()) {
    const nsAString& oldStr = Substring(mTypeAheadBuffer, 0, mTypeAheadBuffer.Length());
    const nsAString& newStr = Substring(aSearchString, 0, mTypeAheadBuffer.Length());
    if (oldStr.Equals(newStr))
      atEnd = true;
  
    const nsAString& newStr2 = Substring(aSearchString, 0, aSearchString.Length());
    const nsAString& oldStr2 = Substring(mTypeAheadBuffer, 0, aSearchString.Length());
    if (oldStr2.Equals(newStr2))
      atEnd = true;
    
    if (!atEnd)
      mStartFindRange = nullptr;
  }

  if (!mIsSoundInitialized && !mNotFoundSoundURL.IsEmpty()) {
    
    
    
    mIsSoundInitialized = true;
    mSoundInterface = do_CreateInstance("@mozilla.org/sound;1");
    if (mSoundInterface && !mNotFoundSoundURL.EqualsLiteral("beep")) {
      mSoundInterface->Init();
    }
  }

#ifdef XP_WIN
  
  
  
  mSoundInterface = nullptr;
#endif

  int32_t bufferLength = mTypeAheadBuffer.Length();

  mTypeAheadBuffer = aSearchString;

  bool isFirstVisiblePreferred = false;

  
  if (bufferLength == 0) {
    
    
    
    bool isSelectionCollapsed = true;
    if (selection)
      selection->GetIsCollapsed(&isSelectionCollapsed);

    
    
    isFirstVisiblePreferred = !atEnd && !mCaretBrowsingOn && isSelectionCollapsed;
    if (isFirstVisiblePreferred) {
      
      
      
      nsPresContext* presContext = presShell->GetPresContext();
      NS_ENSURE_TRUE(presContext, NS_OK);

      nsCOMPtr<nsIDocument> document =
        do_QueryInterface(presShell->GetDocument());
      if (!document)
        return NS_ERROR_UNEXPECTED;

      nsCOMPtr<nsIDOMWindow> window = do_QueryInterface(document->GetWindow());

      nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
      if (fm) {
        nsCOMPtr<nsIDOMElement> focusedElement;
        nsCOMPtr<nsIDOMWindow> focusedWindow;
        fm->GetFocusedElementForWindow(window, false, getter_AddRefs(focusedWindow),
                                       getter_AddRefs(focusedElement));
        
        
        if (focusedElement &&
            !SameCOMIdentity(focusedElement, document->GetRootElement())) {
          fm->MoveCaretToFocus(window);
          isFirstVisiblePreferred = false;
        }
      }
    }
  }

  
  
  
  nsresult rv = FindItNow(nullptr, aLinksOnly, isFirstVisiblePreferred,
                          false, aResult);

  
  if (NS_SUCCEEDED(rv)) {
    if (mTypeAheadBuffer.Length() == 1) {
      
      

      mStartFindRange = nullptr;
      if (selection) {
        nsCOMPtr<nsIDOMRange> startFindRange;
        selection->GetRangeAt(0, getter_AddRefs(startFindRange));
        if (startFindRange)
          startFindRange->CloneRange(getter_AddRefs(mStartFindRange));
      }
    }
  }
  else {
    
    if (mTypeAheadBuffer.Length() > mLastFindLength)
      PlayNotFoundSound();
  }

  SaveFind();
  return NS_OK;
}

void
nsTypeAheadFind::GetSelection(nsIPresShell *aPresShell,
                              nsISelectionController **aSelCon,
                              nsISelection **aDOMSel)
{
  if (!aPresShell)
    return;

  
  *aDOMSel = nullptr;

  nsPresContext* presContext = aPresShell->GetPresContext();

  nsIFrame *frame = aPresShell->GetRootFrame();

  if (presContext && frame) {
    frame->GetSelectionController(presContext, aSelCon);
    if (*aSelCon) {
      (*aSelCon)->GetSelection(nsISelectionController::SELECTION_NORMAL,
                               aDOMSel);
    }
  }
}

NS_IMETHODIMP
nsTypeAheadFind::GetFoundRange(nsIDOMRange** aFoundRange)
{
  NS_ENSURE_ARG_POINTER(aFoundRange);
  if (mFoundRange == nullptr) {
    *aFoundRange = nullptr;
    return NS_OK;
  }

  mFoundRange->CloneRange(aFoundRange);
  return NS_OK;
}

NS_IMETHODIMP
nsTypeAheadFind::IsRangeVisible(nsIDOMRange *aRange,
                                bool aMustBeInViewPort,
                                bool *aResult)
{
  
  nsCOMPtr<nsIDOMNode> node;
  aRange->GetStartContainer(getter_AddRefs(node));
  nsCOMPtr<nsIDOMDocument> document;
  node->GetOwnerDocument(getter_AddRefs(document));
  nsCOMPtr<nsIDOMWindow> window;
  document->GetDefaultView(getter_AddRefs(window));
  nsCOMPtr<nsIWebNavigation> navNav (do_GetInterface(window));
  nsCOMPtr<nsIDocShell> docShell (do_GetInterface(navNav));

  
  nsCOMPtr<nsIPresShell> presShell (docShell->GetPresShell());
  nsRefPtr<nsPresContext> presContext = presShell->GetPresContext();
  nsCOMPtr<nsIDOMRange> startPointRange = new nsRange(presShell->GetDocument());
  *aResult = IsRangeVisible(presShell, presContext, aRange,
                            aMustBeInViewPort, false,
                            getter_AddRefs(startPointRange),
                            nullptr);
  return NS_OK;
}

bool
nsTypeAheadFind::IsRangeVisible(nsIPresShell *aPresShell,
                                nsPresContext *aPresContext,
                                nsIDOMRange *aRange, bool aMustBeInViewPort,
                                bool aGetTopVisibleLeaf,
                                nsIDOMRange **aFirstVisibleRange,
                                bool *aUsesIndependentSelection)
{
  NS_ASSERTION(aPresShell && aPresContext && aRange && aFirstVisibleRange, 
               "params are invalid");

  
  
  

  aRange->CloneRange(aFirstVisibleRange);
  nsCOMPtr<nsIDOMNode> node;
  aRange->GetStartContainer(getter_AddRefs(node));

  nsCOMPtr<nsIContent> content(do_QueryInterface(node));
  if (!content)
    return false;

  nsIFrame *frame = content->GetPrimaryFrame();
  if (!frame)    
    return false;  

  if (!frame->StyleVisibility()->IsVisible())
    return false;

  
  
  if (aUsesIndependentSelection) {
    *aUsesIndependentSelection = 
      (frame->GetStateBits() & NS_FRAME_INDEPENDENT_SELECTION);
  }

  
  if (!aMustBeInViewPort)   
    return true; 

  
  int32_t startRangeOffset, startFrameOffset, endFrameOffset;
  aRange->GetStartOffset(&startRangeOffset);
  while (true) {
    frame->GetOffsets(startFrameOffset, endFrameOffset);
    if (startRangeOffset < endFrameOffset)
      break;

    nsIFrame *nextContinuationFrame = frame->GetNextContinuation();
    if (nextContinuationFrame)
      frame = nextContinuationFrame;
    else
      break;
  }

  
  const uint16_t kMinPixels  = 12;
  nscoord minDistance = nsPresContext::CSSPixelsToAppUnits(kMinPixels);

  
  
  
  
  nsRectVisibility rectVisibility = nsRectVisibility_kAboveViewport;

  if (!aGetTopVisibleLeaf && !frame->GetRect().IsEmpty()) {
    rectVisibility =
      aPresShell->GetRectVisibility(frame,
                                    nsRect(nsPoint(0,0), frame->GetSize()),
                                    minDistance);

    if (rectVisibility != nsRectVisibility_kAboveViewport) {
      return true;
    }
  }

  
  
  
  nsCOMPtr<nsIFrameEnumerator> frameTraversal;
  nsCOMPtr<nsIFrameTraversal> trav(do_CreateInstance(kFrameTraversalCID));
  if (trav)
    trav->NewFrameTraversal(getter_AddRefs(frameTraversal),
                            aPresContext, frame,
                            eLeaf,
                            false, 
                            false, 
                            false     
                            );

  if (!frameTraversal)
    return false;

  while (rectVisibility == nsRectVisibility_kAboveViewport) {
    frameTraversal->Next();
    frame = frameTraversal->CurrentItem();
    if (!frame)
      return false;

    if (!frame->GetRect().IsEmpty()) {
      rectVisibility =
        aPresShell->GetRectVisibility(frame,
                                      nsRect(nsPoint(0,0), frame->GetSize()),
                                      minDistance);
    }
  }

  if (frame) {
    nsCOMPtr<nsIDOMNode> firstVisibleNode = do_QueryInterface(frame->GetContent());

    if (firstVisibleNode) {
      frame->GetOffsets(startFrameOffset, endFrameOffset);
      (*aFirstVisibleRange)->SetStart(firstVisibleNode, startFrameOffset);
      (*aFirstVisibleRange)->SetEnd(firstVisibleNode, endFrameOffset);
    }
  }

  return false;
}

already_AddRefed<nsIPresShell>
nsTypeAheadFind::GetPresShell()
{
  if (!mPresShell)
    return nullptr;

  nsCOMPtr<nsIPresShell> shell = do_QueryReferent(mPresShell);
  if (shell) {
    nsPresContext *pc = shell->GetPresContext();
    if (!pc || !pc->GetContainerWeak()) {
      return nullptr;
    }
  }

  return shell.forget();
}
