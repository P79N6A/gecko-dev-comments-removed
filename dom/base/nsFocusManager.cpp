



































#include "nsFocusManager.h"

#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIServiceManager.h"
#include "nsIEnumerator.h"
#include "nsTPtrArray.h"
#include "nsGkAtoms.h"
#include "nsIPrefBranch2.h"
#include "nsContentUtils.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMElement.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMNSHTMLFrameElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsIDOMHTMLLegendElement.h"
#include "nsIDOMDocumentRange.h"
#include "nsIDOMRange.h"
#include "nsIHTMLDocument.h"
#include "nsIFormControlFrame.h"
#include "nsGenericHTMLElement.h"
#include "nsIDocShell.h"
#include "nsIEditorDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsLayoutUtils.h"
#include "nsIPresShell.h"
#include "nsIContentViewer.h"
#include "nsFrameTraversal.h"
#include "nsObjectFrame.h"
#include "nsEventDispatcher.h"
#include "nsIEventStateManager.h"
#include "nsIMEStateManager.h"
#include "nsIWebNavigation.h"
#include "nsCaret.h"
#include "nsWidgetsCID.h"
#include "nsILookAndFeel.h"
#include "nsIWidget.h"
#include "nsIBaseWindow.h"
#include "nsIViewManager.h"
#include "nsFrameSelection.h"
#include "nsXULPopupManager.h"
#include "nsImageMapUtils.h"

#ifdef MOZ_XUL
#include "nsIDOMXULTextboxElement.h"
#include "nsIDOMXULMenuListElement.h"
#endif



#define PRINTTAGF(format, content)                     \
  {                                                    \
    nsAutoString tag(NS_LITERAL_STRING("(none)"));     \
    if (content)                                       \
      content->Tag()->ToString(tag);                   \
    printf(format, NS_ConvertUTF16toUTF8(tag).get());  \
  }

struct nsDelayedBlurOrFocusEvent
{
  nsDelayedBlurOrFocusEvent(PRUint32 aType,
                            nsIPresShell* aPresShell,
                            nsIDocument* aDocument,
                            nsPIDOMEventTarget* aTarget)
   : mType(aType),
     mPresShell(aPresShell),
     mDocument(aDocument),
     mTarget(aTarget) { }

  nsDelayedBlurOrFocusEvent(const nsDelayedBlurOrFocusEvent& aOther)
   : mType(aOther.mType),
     mPresShell(aOther.mPresShell),
     mDocument(aOther.mDocument),
     mTarget(aOther.mTarget) { }

  PRUint32 mType;
  nsCOMPtr<nsIPresShell> mPresShell;
  nsCOMPtr<nsIDocument> mDocument;
  nsCOMPtr<nsPIDOMEventTarget> mTarget;
};

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsFocusManager)
  NS_INTERFACE_MAP_ENTRY(nsIFocusManager)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIFocusManager)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsFocusManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsFocusManager)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsFocusManager)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsFocusManager)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mActiveWindow)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFocusedWindow)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFocusedContent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFirstBlurEvent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFirstFocusEvent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mWindowBeingLowered)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsFocusManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mActiveWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFocusedWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFocusedContent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFirstBlurEvent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFirstFocusEvent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mWindowBeingLowered)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

static NS_DEFINE_CID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);

nsIFocusManager* nsFocusManager::sInstance = nsnull;

nsFocusManager::nsFocusManager()
{ }

nsFocusManager::~nsFocusManager()
{
  nsCOMPtr<nsIPrefBranch2> prefBranch =
    do_QueryInterface(nsContentUtils::GetPrefBranch());

  if (prefBranch) {
    prefBranch->RemoveObserver("accessibility.browsewithcaret", this);
    prefBranch->RemoveObserver("accessibility.tabfocus_applies_to_xul", this);
  }
}


nsresult
nsFocusManager::Init()
{
  nsFocusManager* fm = new nsFocusManager();
  NS_ENSURE_TRUE(fm, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(fm);
  sInstance = fm;

  nsIContent::sTabFocusModelAppliesToXUL =
    nsContentUtils::GetBoolPref("accessibility.tabfocus_applies_to_xul",
                                nsIContent::sTabFocusModelAppliesToXUL);

  nsCOMPtr<nsIPrefBranch2> prefBranch =
    do_QueryInterface(nsContentUtils::GetPrefBranch());
  prefBranch->AddObserver("accessibility.browsewithcaret", fm, PR_TRUE);
  prefBranch->AddObserver("accessibility.tabfocus_applies_to_xul", fm, PR_TRUE);

  return NS_OK;
}


void
nsFocusManager::Shutdown()
{
  NS_IF_RELEASE(sInstance);
}

NS_IMETHODIMP
nsFocusManager::Observe(nsISupports *aSubject,
                        const char *aTopic,
                        const PRUnichar *aData)
{
  nsDependentString data(aData);
  if (!nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    if (data.EqualsLiteral("accessibility.browsewithcaret")) {
      UpdateCaret(PR_FALSE, PR_TRUE, mFocusedContent);
    }
    else if (data.EqualsLiteral("accessibility.tabfocus_applies_to_xul")) {
      nsIContent::sTabFocusModelAppliesToXUL =
        nsContentUtils::GetBoolPref("accessibility.tabfocus_applies_to_xul",
                                    nsIContent::sTabFocusModelAppliesToXUL);
    }
  }

  return NS_OK;
}


static nsPIDOMWindow*
GetContentWindow(nsIContent* aContent)
{
  nsIDocument* doc = aContent->GetCurrentDoc();
  if (doc) {
    nsIDocument* subdoc = doc->GetSubDocumentFor(aContent);
    if (subdoc)
      return subdoc->GetWindow();
  }

  return nsnull;
}


static nsPIDOMWindow*
GetCurrentWindow(nsIContent* aContent)
{
  nsIDocument *doc = aContent->GetCurrentDoc();
  return doc ? doc->GetWindow() : nsnull;
}


nsIContent*
nsFocusManager::GetFocusedDescendant(nsPIDOMWindow* aWindow, PRBool aDeep,
                                     nsPIDOMWindow** aFocusedWindow)
{
  NS_ENSURE_TRUE(aWindow, nsnull);

  *aFocusedWindow = nsnull;

  nsIContent* currentContent = nsnull;
  nsPIDOMWindow* window = aWindow->GetOuterWindow();
  while (window) {
    *aFocusedWindow = window;
    currentContent = window->GetFocusedNode();
    if (!currentContent || !aDeep)
      break;

    window = GetContentWindow(currentContent);
  }

  NS_IF_ADDREF(*aFocusedWindow);

  return currentContent;
}


nsIContent*
nsFocusManager::GetRedirectedFocus(nsIContent* aContent)
{
#ifdef MOZ_XUL
  if (aContent->IsNodeOfType(nsINode::eXUL)) {
    nsCOMPtr<nsIDOMNode> inputField;

    nsCOMPtr<nsIDOMXULTextBoxElement> textbox = do_QueryInterface(aContent);
    if (textbox) {
      textbox->GetInputField(getter_AddRefs(inputField));
    }
    else {
      nsCOMPtr<nsIDOMXULMenuListElement> menulist = do_QueryInterface(aContent);
      if (menulist) {
        menulist->GetInputField(getter_AddRefs(inputField));
      }
      else if (aContent->Tag() == nsGkAtoms::scale) {
        nsCOMPtr<nsIDocument> doc = aContent->GetCurrentDoc();
        if (!doc)
          return nsnull;

        nsINodeList* children = doc->BindingManager()->GetXBLChildNodesFor(aContent);
        if (children) {
          nsIContent* child = children->GetNodeAt(0);
          if (child && child->Tag() == nsGkAtoms::slider)
            return child;
        }
      }
    }

    if (inputField) {
      nsCOMPtr<nsIContent> retval = do_QueryInterface(inputField);
      return retval;
    }
  }
#endif

  return nsnull;
}

NS_IMETHODIMP
nsFocusManager::GetActiveWindow(nsIDOMWindow** aWindow)
{
  NS_IF_ADDREF(*aWindow = mActiveWindow);
  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::SetActiveWindow(nsIDOMWindow* aWindow)
{
  
  nsCOMPtr<nsPIDOMWindow> piWindow = do_QueryInterface(aWindow);
  NS_ASSERTION(!piWindow || piWindow->IsOuterWindow(), "outer window expected");

  NS_ENSURE_TRUE(piWindow && (piWindow == piWindow->GetPrivateRoot()),
                 NS_ERROR_INVALID_ARG);

  RaiseWindow(piWindow);
  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::GetFocusedWindow(nsIDOMWindow** aFocusedWindow)
{
  NS_IF_ADDREF(*aFocusedWindow = mFocusedWindow);
  return NS_OK;
}

NS_IMETHODIMP nsFocusManager::SetFocusedWindow(nsIDOMWindow* aWindowToFocus)
{
#ifdef DEBUG_FOCUS
  printf("<<SetFocusedWindow begin>>\n");
#endif

  nsCOMPtr<nsPIDOMWindow> windowToFocus(do_QueryInterface(aWindowToFocus));
  NS_ENSURE_TRUE(windowToFocus, NS_ERROR_FAILURE);

  windowToFocus = windowToFocus->GetOuterWindow();

  nsCOMPtr<nsIContent> frameContent =
    do_QueryInterface(windowToFocus->GetFrameElementInternal());
  if (frameContent) {
    
    
    SetFocusInner(frameContent, 0, PR_FALSE);
  }
  else {
    
    
    
    
    nsIContent* content = windowToFocus->GetFocusedNode();
    if (content) {
      nsCOMPtr<nsIDOMWindow> childWindow = GetContentWindow(content);
      if (childWindow)
        ClearFocus(childWindow);
    }
  }

  nsCOMPtr<nsPIDOMWindow> rootWindow = windowToFocus->GetPrivateRoot();
  if (rootWindow)
    RaiseWindow(rootWindow);

#ifdef DEBUG_FOCUS
  printf("<<SetFocusedWindow end>>\n");
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::GetFocusedElement(nsIDOMElement** aFocusedElement)
{
  if (mFocusedContent)
    CallQueryInterface(mFocusedContent, aFocusedElement);
  else
    *aFocusedElement = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::GetLastFocusMethod(nsIDOMWindow* aWindow, PRUint32* aLastFocusMethod)
{
  
  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aWindow));
  if (window)
    window = window->GetCurrentInnerWindow();
  if (!window)
    window = mFocusedWindow;

  *aLastFocusMethod = window ? window->GetFocusMethod() : 0;

  NS_ASSERTION((*aLastFocusMethod & FOCUSMETHOD_MASK) == *aLastFocusMethod,
               "invalid focus method");
  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::SetFocus(nsIDOMElement* aElement, PRUint32 aFlags)
{
#ifdef DEBUG_FOCUS
  printf("<<SetFocus>>\n");
#endif

  nsCOMPtr<nsIContent> newFocus = do_QueryInterface(aElement);
  NS_ENSURE_ARG(newFocus);

  SetFocusInner(newFocus, aFlags, PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::MoveFocus(nsIDOMWindow* aWindow, nsIDOMElement* aStartElement,
                          PRUint32 aType, PRUint32 aFlags, nsIDOMElement** aElement)
{
  *aElement = nsnull;

#ifdef DEBUG_FOCUS
  printf("<<MoveFocus Type: %d Flags: %d>>\n<<", aType, aFlags);

  nsCOMPtr<nsPIDOMWindow> focusedWindow = mFocusedWindow;
  if (focusedWindow) {
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(focusedWindow->GetExtantDocument());
    if (doc) {
      nsCAutoString spec;
      doc->GetDocumentURI()->GetSpec(spec);
      printf(" [%p] Focused Window: %s", mFocusedWindow.get(), spec.get());
    }
  }
  PRINTTAGF(">> $[[%s]]\n", mFocusedContent);
#endif

  nsCOMPtr<nsPIDOMWindow> window;
  nsCOMPtr<nsIContent> startContent;
  if (aStartElement) {
    startContent = do_QueryInterface(aStartElement);
    NS_ENSURE_TRUE(startContent, NS_ERROR_INVALID_ARG);

    window = GetCurrentWindow(startContent);
  }
  else {
    window = aWindow ? do_QueryInterface(aWindow) : mFocusedWindow;
    NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);
    window = window->GetOuterWindow();
  }

  NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);

  nsCOMPtr<nsIContent> newFocus;
  nsresult rv = DetermineElementToMoveFocus(window, startContent, aType,
                                            getter_AddRefs(newFocus));
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG_FOCUS_NAVIGATION
  PRINTTAGF("-> Element to be focused: %s\n", newFocus);
#endif

  if (newFocus) {
    
    
    
    
    SetFocusInner(newFocus, aFlags, aType != MOVEFOCUS_CARET);
    CallQueryInterface(newFocus, aElement);
  }
  else if (aType == MOVEFOCUS_ROOT || aType == MOVEFOCUS_CARET) {
    
    ClearFocus(window);
  }

#ifdef DEBUG_FOCUS
  printf("<<MoveFocus end>>\n");
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::ClearFocus(nsIDOMWindow* aWindow)
{
#ifdef DEBUG_FOCUS
  printf("<<ClearFocus begin>>\n");
#endif

  
  
  
  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aWindow));
  NS_ENSURE_TRUE(window, NS_ERROR_INVALID_ARG);

  window = window->GetOuterWindow();
  NS_ENSURE_TRUE(window, NS_ERROR_INVALID_ARG);

  if (IsSameOrAncestor(window, mFocusedWindow)) {
    PRBool isAncestor = (window != mFocusedWindow);
    if (Blur(window, nsnull, isAncestor)) {
      
      
      if (isAncestor)
        Focus(window, nsnull, 0, PR_TRUE, PR_FALSE, PR_FALSE);
    }
  }
  else {
    window->SetFocusedNode(nsnull);
  }

#ifdef DEBUG_FOCUS
  printf("<<ClearFocus end>>\n");
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::GetFocusedElementForWindow(nsIDOMWindow* aWindow,
                                           PRBool aDeep,
                                           nsIDOMWindow** aFocusedWindow,
                                           nsIDOMElement** aElement)
{
  *aElement = nsnull;
  if (aFocusedWindow)
    *aFocusedWindow = nsnull;

  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aWindow));
  NS_ENSURE_TRUE(window, NS_ERROR_INVALID_ARG);

  window = window->GetOuterWindow();
  NS_ENSURE_TRUE(window, NS_ERROR_INVALID_ARG);

  nsCOMPtr<nsPIDOMWindow> focusedWindow;
  nsCOMPtr<nsIContent> focusedContent =
    GetFocusedDescendant(window, aDeep, getter_AddRefs(focusedWindow));
  if (focusedContent)
    CallQueryInterface(focusedContent, aElement);

  if (aFocusedWindow)
    NS_IF_ADDREF(*aFocusedWindow = focusedWindow);

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::MoveCaretToFocus(nsIDOMWindow* aWindow)
{
  PRInt32 itemType = nsIDocShellTreeItem::typeChrome;

  nsCOMPtr<nsIWebNavigation> webnav = do_GetInterface(aWindow);
  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(webnav);
  if (dsti) {
    dsti->GetItemType(&itemType);
    if (itemType != nsIDocShellTreeItem::typeChrome) {
      
      nsCOMPtr<nsIEditorDocShell> editorDocShell(do_QueryInterface(dsti));
      if (editorDocShell) {
        PRBool isEditable;
        editorDocShell->GetEditable(&isEditable);
        if (isEditable)
          return NS_OK;
      }

      nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(dsti);
      NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

      nsCOMPtr<nsIPresShell> presShell;
      docShell->GetPresShell(getter_AddRefs(presShell));
      NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

      nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aWindow));
      nsCOMPtr<nsIContent> content = window->GetFocusedNode();
      if (content)
        MoveCaretToFocus(presShell, content);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::WindowRaised(nsIDOMWindow* aWindow)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aWindow);
  NS_ENSURE_TRUE(window && window->IsOuterWindow(), NS_ERROR_INVALID_ARG);

#ifdef DEBUG_FOCUS
  printf("Window %p Raised [Currently: %p %p] <<", aWindow, mActiveWindow.get(), mFocusedWindow.get());
  nsCAutoString spec;
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(window->GetExtantDocument());
  if (doc) {
    doc->GetDocumentURI()->GetSpec(spec);
    printf("[%p] Raised Window: %s", aWindow, spec.get());
  }
  if (mActiveWindow) {
    doc = do_QueryInterface(mActiveWindow->GetExtantDocument());
    if (doc) {
      doc->GetDocumentURI()->GetSpec(spec);
      printf(" [%p] Active Window: %s", mActiveWindow.get(), spec.get());
    }
  }
  printf(">>\n");
#endif

  if (mActiveWindow == window) {
    
    
    
    
    
    
    
    EnsureCurrentWidgetFocused();
    return NS_OK;
  }

  
  if (mActiveWindow)
    WindowLowered(mActiveWindow);

  nsCOMPtr<nsIWebNavigation> webnav(do_GetInterface(aWindow));
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(webnav));
  
  
  NS_ENSURE_TRUE(docShellAsItem, NS_OK);

  
  mActiveWindow = window;

  
  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  docShellAsItem->GetTreeOwner(getter_AddRefs(treeOwner));
  nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(treeOwner);
  if (baseWindow) {
    PRBool isEnabled = PR_TRUE;
    if (NS_SUCCEEDED(baseWindow->GetEnabled(&isEnabled)) && !isEnabled) {
      return NS_ERROR_FAILURE;
    }

    baseWindow->SetVisibility(PR_TRUE);
  }

  
  
  window->ActivateOrDeactivate(PR_TRUE);

  
  nsCOMPtr<nsPIDOMWindow> currentWindow;
  nsCOMPtr<nsIContent> currentFocus =
    GetFocusedDescendant(window, PR_TRUE, getter_AddRefs(currentWindow));

  NS_ASSERTION(currentWindow, "window raised with no window current");
  if (!currentWindow)
    return NS_OK;

  nsCOMPtr<nsIDocShell> currentDocShell = currentWindow->GetDocShell();

  nsCOMPtr<nsIPresShell> presShell;
  currentDocShell->GetPresShell(getter_AddRefs(presShell));
  if (presShell) {
    
    
    nsCOMPtr<nsFrameSelection> frameSelection = presShell->FrameSelection();
    frameSelection->SetMouseDownState(PR_FALSE);
  }

  Focus(currentWindow, currentFocus, 0, PR_TRUE, PR_FALSE, PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::WindowLowered(nsIDOMWindow* aWindow)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aWindow);
  NS_ENSURE_TRUE(window && window->IsOuterWindow(), NS_ERROR_INVALID_ARG);

#ifdef DEBUG_FOCUS
  printf("Window %p Lowered [Currently: %p %p] <<", aWindow, mActiveWindow.get(), mFocusedWindow.get());
  nsCAutoString spec;
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(window->GetExtantDocument());
  if (doc) {
    doc->GetDocumentURI()->GetSpec(spec);
    printf("[%p] Lowered Window: %s", aWindow, spec.get());
  }
  if (mActiveWindow) {
    doc = do_QueryInterface(mActiveWindow->GetExtantDocument());
    if (doc) {
      doc->GetDocumentURI()->GetSpec(spec);
      printf(" [%p] Active Window: %s", mActiveWindow.get(), spec.get());
    }
  }
  printf(">>\n");
#endif

  if (mActiveWindow != window)
    return NS_OK;

  
  
  window->ActivateOrDeactivate(PR_FALSE);

  
  
  
  mWindowBeingLowered = mActiveWindow;
  mActiveWindow = nsnull;

  if (mFocusedWindow)
    Blur(nsnull, nsnull, PR_TRUE);

  mWindowBeingLowered = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::ContentRemoved(nsIDocument* aDocument, nsIContent* aContent)
{
  NS_ENSURE_ARG(aDocument);
  NS_ENSURE_ARG(aContent);

  nsPIDOMWindow *window = aDocument->GetWindow();
  if (!window)
    return NS_OK;

  
  
  nsCOMPtr<nsIContent> content = window->GetFocusedNode();
  if (content && nsContentUtils::ContentIsDescendantOf(content, aContent)) {
    window->SetFocusedNode(nsnull);

    nsCOMPtr<nsIDocShell> docShell = window->GetDocShell();
    if (docShell) {
      nsCOMPtr<nsIPresShell> presShell;
      docShell->GetPresShell(getter_AddRefs(presShell));
      nsIMEStateManager::OnRemoveContent(presShell->GetPresContext(), content);
    }

    
    
    if (window == mFocusedWindow) {
      mFocusedContent = nsnull;
    }
    else {
      
      
      
      
      
      nsIDocument* subdoc = aDocument->GetSubDocumentFor(content);
      if (subdoc) {
        nsCOMPtr<nsISupports> container = subdoc->GetContainer();
        nsCOMPtr<nsPIDOMWindow> childWindow = do_GetInterface(container);
        if (childWindow && IsSameOrAncestor(childWindow, mFocusedWindow)) {
          ClearFocus(mActiveWindow);
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::WindowShown(nsIDOMWindow* aWindow)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aWindow);
  NS_ENSURE_TRUE(window, NS_ERROR_INVALID_ARG);

  window = window->GetOuterWindow();

#ifdef DEBUG_FOCUS
  printf("Window %p Shown [Currently: %p %p] <<", window.get(), mActiveWindow.get(), mFocusedWindow.get());
  nsCAutoString spec;
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(window->GetExtantDocument());
  if (doc) {
    doc->GetDocumentURI()->GetSpec(spec);
    printf("Shown Window: %s", spec.get());
  }

  if (mFocusedWindow) {
    doc = do_QueryInterface(mFocusedWindow->GetExtantDocument());
    if (doc) {
      doc->GetDocumentURI()->GetSpec(spec);
      printf(" Focused Window: %s", spec.get());
    }
  }
  printf(">>\n");
#endif

  if (mFocusedWindow != window)
    return NS_OK;

  nsCOMPtr<nsPIDOMWindow> currentWindow;
  nsCOMPtr<nsIContent> currentFocus =
    GetFocusedDescendant(window, PR_TRUE, getter_AddRefs(currentWindow));
  if (currentWindow)
    Focus(currentWindow, currentFocus, 0, PR_TRUE, PR_FALSE, PR_FALSE);

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::WindowHidden(nsIDOMWindow* aWindow)
{
  
  
  

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aWindow);
  NS_ENSURE_TRUE(window, NS_ERROR_INVALID_ARG);

  window = window->GetOuterWindow();

#ifdef DEBUG_FOCUS
  printf("Window %p Hidden [Currently: %p %p] <<", window.get(), mActiveWindow.get(), mFocusedWindow.get());
  nsCAutoString spec;
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(window->GetExtantDocument());
  if (doc) {
    doc->GetDocumentURI()->GetSpec(spec);
    printf("Hide Window: %s", spec.get());
  }

  if (mFocusedWindow) {
    doc = do_QueryInterface(mFocusedWindow->GetExtantDocument());
    if (doc) {
      doc->GetDocumentURI()->GetSpec(spec);
      printf(" Focused Window: %s", spec.get());
    }
  }

  if (mActiveWindow) {
    doc = do_QueryInterface(mActiveWindow->GetExtantDocument());
    if (doc) {
      doc->GetDocumentURI()->GetSpec(spec);
      printf(" Active Window: %s", spec.get());
    }
  }
  printf(">>\n");
#endif

  if (!IsSameOrAncestor(window, mFocusedWindow))
    return NS_OK;

  
  
  

  nsCOMPtr<nsIDocShell> focusedDocShell = mFocusedWindow->GetDocShell();
  nsCOMPtr<nsIPresShell> presShell;
  focusedDocShell->GetPresShell(getter_AddRefs(presShell));
  if (presShell) {
    nsPresContext* presContext = presShell->GetPresContext();
    presContext->EventStateManager()->SetContentState(mFocusedContent, NS_EVENT_STATE_FOCUS);  
  }

  mFocusedContent = nsnull;

  nsIMEStateManager::OnTextStateBlur(nsnull, nsnull);
  if (presShell) {
    nsIMEStateManager::OnChangeFocus(presShell->GetPresContext(), nsnull);
    SetCaretVisible(presShell, PR_FALSE, nsnull);
  }

  
  
  
  
  
  
  PRBool beingDestroyed;
  nsCOMPtr<nsIDocShell> docShellBeingHidden = window->GetDocShell();
  docShellBeingHidden->IsBeingDestroyed(&beingDestroyed);
  if (beingDestroyed) {
    
    
    
    
    
    NS_ASSERTION(mFocusedWindow->IsOuterWindow(), "outer window expected");
    if (mActiveWindow == mFocusedWindow || mActiveWindow == window)
      WindowLowered(mActiveWindow);
    else
      ClearFocus(mActiveWindow);
    return NS_OK;
  }

  
  
  
  
  if (window != mFocusedWindow) {
    nsCOMPtr<nsIWebNavigation> webnav(do_GetInterface(window));
    nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(webnav);
    if (dsti) {
      nsCOMPtr<nsIDocShellTreeItem> parentDsti;
      dsti->GetParent(getter_AddRefs(parentDsti));
      nsCOMPtr<nsPIDOMWindow> parentWindow = do_GetInterface(parentDsti);
      if (parentWindow)
        parentWindow->SetFocusedNode(nsnull);
    }

    mFocusedWindow = window;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::FireDelayedEvents(nsIDocument* aDocument)
{
  NS_ENSURE_ARG(aDocument);

  
  for (PRUint32 i = 0; i < mDelayedBlurFocusEvents.Length(); i++)
  {
    if (mDelayedBlurFocusEvents[i].mDocument == aDocument &&
        !aDocument->EventHandlingSuppressed()) {
      PRUint32 type = mDelayedBlurFocusEvents[i].mType;
      nsCOMPtr<nsPIDOMEventTarget> target = mDelayedBlurFocusEvents[i].mTarget;
      nsCOMPtr<nsIPresShell> presShell = mDelayedBlurFocusEvents[i].mPresShell;
      mDelayedBlurFocusEvents.RemoveElementAt(i);
      SendFocusOrBlurEvent(type, presShell, aDocument, target, 0);
      --i;
    }
  }

  return NS_OK;
}


void
nsFocusManager::EnsureCurrentWidgetFocused()
{
  if (!mFocusedWindow)
    return;

  
  
  nsCOMPtr<nsIDocShell> docShell = mFocusedWindow->GetDocShell();
  if (docShell) {
    nsCOMPtr<nsIPresShell> presShell;
    docShell->GetPresShell(getter_AddRefs(presShell));
    if (presShell) {
      nsIViewManager* vm = presShell->GetViewManager();
      if (vm) {
        nsCOMPtr<nsIWidget> widget;
        vm->GetRootWidget(getter_AddRefs(widget));
        if (widget)
          widget->SetFocus(PR_TRUE);
      }
    }
  }
}

void
nsFocusManager::SetFocusInner(nsIContent* aNewContent, PRInt32 aFlags,
                              PRBool aFocusChanged)
{
  
  nsCOMPtr<nsIContent> contentToFocus = CheckIfFocusable(aNewContent, aFlags);
  if (!contentToFocus)
    return;

  
  
  
  
  nsCOMPtr<nsPIDOMWindow> newWindow;
  nsCOMPtr<nsPIDOMWindow> subWindow = GetContentWindow(contentToFocus);
  if (subWindow) {
    contentToFocus = GetFocusedDescendant(subWindow, PR_TRUE, getter_AddRefs(newWindow));
    
    
    aFocusChanged = PR_FALSE;
  }

  
  if (!newWindow)
    newWindow = GetCurrentWindow(contentToFocus);

  
  
  
  if (!newWindow || newWindow == mFocusedWindow && contentToFocus == mFocusedContent)
    return;

  
  
  
  nsCOMPtr<nsIDocShell> newDocShell = newWindow->GetDocShell();
  nsCOMPtr<nsIDocShell> docShell = newDocShell;
  while (docShell) {
    PRBool inUnload;
    docShell->GetIsInUnload(&inUnload);
    if (inUnload)
      return;

    PRBool beingDestroyed;
    docShell->IsBeingDestroyed(&beingDestroyed);
    if (beingDestroyed)
      return;

    nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(docShell);
    nsCOMPtr<nsIDocShellTreeItem> parentDsti;
    dsti->GetParent(getter_AddRefs(parentDsti));
    docShell = do_QueryInterface(parentDsti);
  }

  
  PRBool isElementInFocusedWindow = (mFocusedWindow == newWindow);

  
  
  PRBool isElementInActiveWindow = PR_FALSE;

  nsCOMPtr<nsIWebNavigation> webnav = do_GetInterface(newWindow);
  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(webnav);
  nsCOMPtr<nsPIDOMWindow> newRootWindow;
  if (dsti) {
    nsCOMPtr<nsIDocShellTreeItem> root;
    dsti->GetRootTreeItem(getter_AddRefs(root));
    newRootWindow = do_GetInterface(root);

    isElementInActiveWindow = (mActiveWindow && newRootWindow == mActiveWindow);
  }

#ifdef DEBUG_FOCUS
  PRINTTAGF("Shift Focus: %s", contentToFocus);
  printf(" Flags: %d Current Window: %p New Window: %p Current Element: %p",
         aFlags, mFocusedWindow.get(), newWindow.get(), mFocusedContent.get());
  printf(" In Active Window: %d In Focused Window: %d\n",
         isElementInActiveWindow, isElementInFocusedWindow);
#endif

  
  
  
  PRBool allowFrameSwitch = !(aFlags & FLAG_NOSWITCHFRAME) ||
                            IsSameOrAncestor(newWindow, mFocusedWindow);

  
  
  if (isElementInActiveWindow && allowFrameSwitch && IsWindowVisible(newWindow)) {
    
    if (mFocusedWindow) {
      
      
      
      
      
      PRBool currentIsSameOrAncestor = IsSameOrAncestor(mFocusedWindow, newWindow);
      
      
      
      
      
      
      
      
      
      
      
      
      nsCOMPtr<nsPIDOMWindow> commonAncestor;
      if (!isElementInFocusedWindow)
        commonAncestor = GetCommonAncestor(newWindow, mFocusedWindow);

      if (!Blur(currentIsSameOrAncestor ? mFocusedWindow.get() : nsnull,
                commonAncestor, !isElementInFocusedWindow))
        return;
    }

    Focus(newWindow, contentToFocus, aFlags, !isElementInFocusedWindow,
          aFocusChanged, PR_FALSE);
  }
  else {
    
    
    if (allowFrameSwitch)
      AdjustWindowFocus(newWindow);

    
    PRUint32 focusMethod = aFocusChanged ? aFlags & FOCUSMETHOD_MASK :
                                           newWindow->GetFocusMethod();
    newWindow->SetFocusedNode(contentToFocus, focusMethod);
    if (aFocusChanged) {
      nsCOMPtr<nsIDocShell> docShell = newWindow->GetDocShell();

      nsCOMPtr<nsIPresShell> presShell;
      docShell->GetPresShell(getter_AddRefs(presShell));
      if (presShell)
        ScrollIntoView(presShell, contentToFocus, aFlags);
    }

    
    
    if (allowFrameSwitch)
      newWindow->UpdateCommands(NS_LITERAL_STRING("focus"));

    if (aFlags & FLAG_RAISE)
      RaiseWindow(newRootWindow);
  }
}

PRBool
nsFocusManager::IsSameOrAncestor(nsPIDOMWindow* aPossibleAncestor,
                                 nsPIDOMWindow* aWindow)
{
  nsCOMPtr<nsIWebNavigation> awebnav(do_GetInterface(aPossibleAncestor));
  nsCOMPtr<nsIDocShellTreeItem> ancestordsti = do_QueryInterface(awebnav);

  nsCOMPtr<nsIWebNavigation> fwebnav(do_GetInterface(aWindow));
  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(fwebnav);
  while (dsti) {
    if (dsti == ancestordsti)
      return PR_TRUE;
    nsCOMPtr<nsIDocShellTreeItem> parentDsti;
    dsti->GetParent(getter_AddRefs(parentDsti));
    dsti.swap(parentDsti);
  }

  return PR_FALSE;
}

already_AddRefed<nsPIDOMWindow>
nsFocusManager::GetCommonAncestor(nsPIDOMWindow* aWindow1,
                                  nsPIDOMWindow* aWindow2)
{
  nsCOMPtr<nsIWebNavigation> webnav(do_GetInterface(aWindow1));
  nsCOMPtr<nsIDocShellTreeItem> dsti1 = do_QueryInterface(webnav);
  NS_ENSURE_TRUE(dsti1, nsnull);

  webnav = do_GetInterface(aWindow2);
  nsCOMPtr<nsIDocShellTreeItem> dsti2 = do_QueryInterface(webnav);
  NS_ENSURE_TRUE(dsti2, nsnull);

  nsAutoTPtrArray<nsIDocShellTreeItem, 30> parents1, parents2;
  do {
    parents1.AppendElement(dsti1);
    nsCOMPtr<nsIDocShellTreeItem> parentDsti1;
    dsti1->GetParent(getter_AddRefs(parentDsti1));
    dsti1.swap(parentDsti1);
  } while (dsti1);
  do {
    parents2.AppendElement(dsti2);
    nsCOMPtr<nsIDocShellTreeItem> parentDsti2;
    dsti2->GetParent(getter_AddRefs(parentDsti2));
    dsti2.swap(parentDsti2);
  } while (dsti2);

  PRUint32 pos1 = parents1.Length();
  PRUint32 pos2 = parents2.Length();
  nsIDocShellTreeItem* parent = nsnull;
  PRUint32 len;
  for (len = PR_MIN(pos1, pos2); len > 0; --len) {
    nsIDocShellTreeItem* child1 = parents1.ElementAt(--pos1);
    nsIDocShellTreeItem* child2 = parents2.ElementAt(--pos2);
    if (child1 != child2) {
      break;
    }
    parent = child1;
  }

  nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(parent);
  return window.forget();
}

void
nsFocusManager::AdjustWindowFocus(nsPIDOMWindow* aWindow)
{
  PRBool isVisible = IsWindowVisible(aWindow);

  nsCOMPtr<nsPIDOMWindow> window(aWindow);
  while (window) {
    
    
    nsCOMPtr<nsIContent> frameContent =
      do_QueryInterface(window->GetFrameElementInternal());

    nsCOMPtr<nsIWebNavigation> webnav(do_GetInterface(window));
    nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(webnav);
    if (!dsti) 
      return;
    nsCOMPtr<nsIDocShellTreeItem> parentDsti;
    dsti->GetParent(getter_AddRefs(parentDsti));

    window = do_GetInterface(parentDsti);
    if (window) {
      
      
      
      if (IsWindowVisible(window) != isVisible)
        break;

      window->SetFocusedNode(frameContent);
    }
  }
}

PRBool
nsFocusManager::IsWindowVisible(nsPIDOMWindow* aWindow)
{
  if (!aWindow)
    return PR_FALSE;

  nsCOMPtr<nsIDocShell> docShell = aWindow->GetDocShell();
  nsCOMPtr<nsIBaseWindow> baseWin(do_QueryInterface(docShell));
  if (!baseWin)
    return PR_FALSE;

  PRBool visible = PR_FALSE;
  baseWin->GetVisibility(&visible);
  return visible;
}

nsIContent*
nsFocusManager::CheckIfFocusable(nsIContent* aContent, PRUint32 aFlags)
{
  if (!aContent)
    return nsnull;

  
  
  nsIContent* redirectedFocus = GetRedirectedFocus(aContent);
  if (redirectedFocus)
    return CheckIfFocusable(redirectedFocus, aFlags);

  nsCOMPtr<nsIDocument> doc = aContent->GetCurrentDoc();
  
  if (!doc)
    return nsnull;

  
  if (doc)
    doc->FlushPendingNotifications(Flush_Frames);

  nsIPresShell *shell = doc->GetPrimaryShell();
  if (!shell)
    return nsnull;

  
  if (aContent == doc->GetRootContent())
    return aContent;

  
  nsPresContext* presContext = shell->GetPresContext();
  if (presContext && presContext->Type() == nsPresContext::eContext_PrintPreview)
    return nsnull;

  nsIFrame* frame = shell->GetPrimaryFrameFor(aContent);
  if (!frame)
    return nsnull;

  if (aContent->Tag() == nsGkAtoms::area && aContent->IsNodeOfType(nsINode::eHTML)) {
    
    
    
    return frame->AreAncestorViewsVisible() &&
           frame->GetStyleVisibility()->IsVisible() &&
           aContent->IsFocusable() ? aContent : nsnull;
  }

  
  
  
  
  nsIDocument* subdoc = doc->GetSubDocumentFor(aContent);
  if (subdoc && IsWindowVisible(subdoc->GetWindow())) {
    const nsStyleUserInterface* ui = frame->GetStyleUserInterface();
    PRInt32 tabIndex = (ui->mUserFocus == NS_STYLE_USER_FOCUS_IGNORE ||
                        ui->mUserFocus == NS_STYLE_USER_FOCUS_NONE) ? -1 : 0;
    return aContent->IsFocusable(&tabIndex) ? aContent : nsnull;
  }
  
  return frame->IsFocusable(nsnull, aFlags & FLAG_BYMOUSE) ? aContent : nsnull;
}

PRBool
nsFocusManager::Blur(nsPIDOMWindow* aWindowToClear,
                     nsPIDOMWindow* aAncestorWindowToFocus,
                     PRBool aIsLeavingDocument)
{
  
  nsCOMPtr<nsIContent> content = mFocusedContent;
  if (content) {
    if (!content->IsInDoc()) {
      mFocusedContent = nsnull;
      return PR_TRUE;
    }
    if (content == mFirstBlurEvent)
      return PR_TRUE;
  }

  
  nsCOMPtr<nsPIDOMWindow> window = mFocusedWindow;
  if (!window) {
    mFocusedContent = nsnull;
    return PR_TRUE;
  }

  nsCOMPtr<nsIDocShell> docShell = window->GetDocShell();
  if (!docShell) {
    mFocusedContent = nsnull;
    return PR_TRUE;
  }

  
  
  nsCOMPtr<nsIPresShell> presShell;
  docShell->GetPresShell(getter_AddRefs(presShell));
  if (!presShell) {
    mFocusedContent = nsnull;
    return PR_TRUE;
  }

  PRBool clearFirstBlurEvent = PR_FALSE;
  if (!mFirstBlurEvent) {
    mFirstBlurEvent = content;
    clearFirstBlurEvent = PR_TRUE;
  }

  
  
  
  nsIMEStateManager::OnTextStateBlur(nsnull, nsnull);
  nsIMEStateManager::OnChangeFocus(presShell->GetPresContext(), nsnull);

  
  
  mFocusedContent = nsnull;
  if (aWindowToClear)
    aWindowToClear->SetFocusedNode(nsnull);

#ifdef DEBUG_FOCUS
  PRINTTAGF("**Element %s has been blurred\n", content);
#endif

  PRBool isRootContent = content && content == content->GetCurrentDoc()->GetRootContent();
  if (content) {
    if (!isRootContent) {
      
      
      nsPresContext* presContext = presShell->GetPresContext();
      presContext->EventStateManager()->SetContentState(content, NS_EVENT_STATE_FOCUS);  
    }

    
    
    
    
    if (mActiveWindow) {
      nsIFrame* contentFrame = presShell->GetPrimaryFrameFor(content);
      nsIObjectFrame* objectFrame = do_QueryFrame(contentFrame);
      if (objectFrame) {
        
        
        nsIViewManager* vm = presShell->GetViewManager();
        if (vm) {
          nsCOMPtr<nsIWidget> widget;
          vm->GetRootWidget(getter_AddRefs(widget));
          if (widget)
            widget->SetFocus(PR_TRUE);
        }
      }
    }
  }

  PRBool result = PR_TRUE;
  if (content && !isRootContent) {
    
    
    
    if (mActiveWindow)
      window->UpdateCommands(NS_LITERAL_STRING("focus"));

    SendFocusOrBlurEvent(NS_BLUR_CONTENT, presShell,
                         content->GetCurrentDoc(), content, 1);
  }

  
  
  if (aIsLeavingDocument || !mActiveWindow)
    SetCaretVisible(presShell, PR_FALSE, nsnull);

  
  
  
  
  
  
  if (mFocusedWindow != window ||
      (mFocusedContent != nsnull && !aIsLeavingDocument)) {
    result = PR_FALSE;
  }
  else if (aIsLeavingDocument) {
    window->TakeFocus(PR_FALSE, 0);

    
    
    
    if (aAncestorWindowToFocus)
      aAncestorWindowToFocus->SetFocusedNode(nsnull, 0, PR_TRUE);

    mFocusedWindow = nsnull;
    mFocusedContent = nsnull;

    
    
    
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(window->GetExtantDocument());
    if (doc)
      SendFocusOrBlurEvent(NS_BLUR_CONTENT, presShell, doc, doc, 1);
    if (mFocusedWindow == nsnull)
      SendFocusOrBlurEvent(NS_BLUR_CONTENT, presShell, doc, window, 1);

    
    result = (mFocusedWindow == nsnull && mActiveWindow);
  }
  else if (mActiveWindow) {
    
    
    
    
    
    UpdateCaret(PR_FALSE, PR_TRUE, nsnull);
  }

  if (clearFirstBlurEvent)
    mFirstBlurEvent = nsnull;

  return result;
}

void
nsFocusManager::Focus(nsPIDOMWindow* aWindow,
                      nsIContent* aContent,
                      PRUint32 aFlags,
                      PRBool aIsNewDocument,
                      PRBool aFocusChanged,
                      PRBool aWindowRaised)
{
  if (!aWindow)
    return;

  if (aContent && aContent == mFirstFocusEvent)
    return;

  
  
  nsCOMPtr<nsIDocShell> docShell = aWindow->GetDocShell();
  if (!docShell)
    return;

  nsCOMPtr<nsIPresShell> presShell;
  docShell->GetPresShell(getter_AddRefs(presShell));
  if (!presShell)
    return;

  
  
  
  PRUint32 focusMethod = aFocusChanged ? aFlags & FOCUSMETHOD_MASK :
                                         aWindow->GetFocusMethod();

  if (!IsWindowVisible(aWindow)) {
    
    
    if (CheckIfFocusable(aContent, aFlags)) {
      aWindow->SetFocusedNode(aContent, focusMethod);
      if (aFocusChanged)
        ScrollIntoView(presShell, aContent, aFlags);
    }
    return;
  }

  PRBool clearFirstFocusEvent = PR_FALSE;
  if (!mFirstFocusEvent) {
    mFirstFocusEvent = aContent;
    clearFirstFocusEvent = PR_TRUE;
  }

#ifdef DEBUG_FOCUS
  PRINTTAGF("**Element %s has been focused", aContent);
  nsCOMPtr<nsIDocument> docm = do_QueryInterface(aWindow->GetExtantDocument());
  if (docm)
    PRINTTAGF(" from %s", docm->GetRootContent());
  printf(" [Newdoc: %d FocusChanged: %d Raised: %d Flags: %d]\n",
         aIsNewDocument, aFocusChanged, aWindowRaised, aFlags);
#endif

  
  
  
  if (aIsNewDocument)
    AdjustWindowFocus(aWindow);

  
  if (aWindow->TakeFocus(PR_TRUE, focusMethod))
    aIsNewDocument = PR_TRUE;

  mFocusedWindow = aWindow;

  
  nsIViewManager* vm = presShell->GetViewManager();
  if (vm) {
    nsCOMPtr<nsIWidget> widget;
    vm->GetRootWidget(getter_AddRefs(widget));
    if (widget)
      widget->SetFocus(PR_TRUE);
  }

  
  
  if (aIsNewDocument) {
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(aWindow->GetExtantDocument());
    if (doc)
      SendFocusOrBlurEvent(NS_FOCUS_CONTENT, presShell, doc,
                           doc, aFlags & FOCUSMETHOD_MASK);
    if (mFocusedWindow == aWindow && mFocusedContent == nsnull)
      SendFocusOrBlurEvent(NS_FOCUS_CONTENT, presShell, doc,
                           aWindow, aFlags & FOCUSMETHOD_MASK);
  }

  
  
  if (CheckIfFocusable(aContent, aFlags) &&
      mFocusedWindow == aWindow && mFocusedContent == nsnull) {
    mFocusedContent = aContent;
    aWindow->SetFocusedNode(aContent, focusMethod);

    
    PRBool isRootContent = aContent &&
                           aContent->IsInDoc() &&
                           aContent == aContent->GetCurrentDoc()->GetRootContent();
    if (!isRootContent) {
      
      if (aFocusChanged)
        ScrollIntoView(presShell, aContent, aFlags);

      
      
      nsPresContext* presContext = presShell->GetPresContext();
      presContext->EventStateManager()->SetContentState(aContent, NS_EVENT_STATE_FOCUS);  

      
      nsIFrame* contentFrame = presShell->GetPrimaryFrameFor(aContent);
      nsIObjectFrame* objectFrame = do_QueryFrame(contentFrame);
      if (objectFrame) {
        nsIWidget* widget = objectFrame->GetWidget();
        if (widget)
          widget->SetFocus(PR_TRUE);
      }

      nsIMEStateManager::OnChangeFocus(presContext, aContent);

      
      
      
      if (!aWindowRaised)
        aWindow->UpdateCommands(NS_LITERAL_STRING("focus"));

      SendFocusOrBlurEvent(NS_FOCUS_CONTENT, presShell, aContent->GetCurrentDoc(),
                           aContent, aFlags & FOCUSMETHOD_MASK);

      nsIMEStateManager::OnTextStateFocus(presContext, aContent);
    }
  }
  else {
    nsPresContext* presContext = presShell->GetPresContext();
    nsIMEStateManager::OnTextStateBlur(presContext, nsnull);
    nsIMEStateManager::OnChangeFocus(presContext, nsnull);

    if (!aWindowRaised)
      aWindow->UpdateCommands(NS_LITERAL_STRING("focus"));
  }

  
  
  
  
  
  
  if (mFocusedContent == aContent)
    UpdateCaret(aFocusChanged && !(aFlags & FLAG_BYMOUSE), aIsNewDocument,
                mFocusedContent);

  if (clearFirstFocusEvent)
    mFirstFocusEvent = nsnull;
}

void
nsFocusManager::SendFocusOrBlurEvent(PRUint32 aType,
                                     nsIPresShell* aPresShell,
                                     nsIDocument* aDocument,
                                     nsISupports* aTarget,
                                     PRUint32 aFocusMethod)
{
  NS_ASSERTION(aType == NS_FOCUS_CONTENT || aType == NS_BLUR_CONTENT,
               "Wrong event type for SendFocusOrBlurEvent");

  nsCOMPtr<nsPIDOMEventTarget> eventTarget = do_QueryInterface(aTarget);

  
  
  
  if (aFocusMethod && aDocument && aDocument->EventHandlingSuppressed()) {
    for (PRUint32 i = mDelayedBlurFocusEvents.Length(); i > 0; --i) {
      
      if (mDelayedBlurFocusEvents[i - 1].mType == aType &&
          mDelayedBlurFocusEvents[i - 1].mPresShell == aPresShell &&
          mDelayedBlurFocusEvents[i - 1].mDocument == aDocument &&
          mDelayedBlurFocusEvents[i - 1].mTarget == eventTarget) {
        mDelayedBlurFocusEvents.RemoveElementAt(i - 1);
      }
    }

    mDelayedBlurFocusEvents.AppendElement(
      nsDelayedBlurOrFocusEvent(aType, aPresShell, aDocument, eventTarget));
    return;
  }

  nsCOMPtr<nsPresContext> presContext = aPresShell->GetPresContext();

  nsEventStatus status = nsEventStatus_eIgnore;
  nsEvent event(PR_TRUE, aType);
  event.flags |= NS_EVENT_FLAG_CANT_BUBBLE;

  nsEventDispatcher::Dispatch(aTarget, presContext, &event, nsnull, &status);
}

void
nsFocusManager::ScrollIntoView(nsIPresShell* aPresShell,
                               nsIContent* aContent,
                               PRUint32 aFlags)
{
  
  if (!(aFlags & FLAG_NOSCROLL))
    aPresShell->ScrollContentIntoView(aContent,
                                      NS_PRESSHELL_SCROLL_IF_NOT_VISIBLE,
                                      NS_PRESSHELL_SCROLL_IF_NOT_VISIBLE);
}


void
nsFocusManager::RaiseWindow(nsPIDOMWindow* aWindow)
{
  
  
  if (!aWindow || aWindow == mActiveWindow || aWindow == mWindowBeingLowered)
    return;

#ifdef XP_WIN
  
  
  
  
  
  nsCOMPtr<nsPIDOMWindow> childWindow;
  GetFocusedDescendant(aWindow, PR_TRUE, getter_AddRefs(childWindow));
  if (!childWindow)
    childWindow = aWindow;

  nsCOMPtr<nsIDocShell> docShell = aWindow->GetDocShell();
  if (!docShell)
    return;

  nsCOMPtr<nsIPresShell> presShell;
  docShell->GetPresShell(getter_AddRefs(presShell));
  if (!presShell)
    return;

  nsIViewManager* vm = presShell->GetViewManager();
  if (vm) {
    nsCOMPtr<nsIWidget> widget;
    vm->GetRootWidget(getter_AddRefs(widget));
    if (widget)
      widget->SetFocus(PR_TRUE);
  }
#else
  nsCOMPtr<nsIWebNavigation> webnav = do_GetInterface(aWindow);
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin = do_QueryInterface(webnav);
  if (treeOwnerAsWin) {
    nsCOMPtr<nsIWidget> widget;
    treeOwnerAsWin->GetMainWidget(getter_AddRefs(widget));
    if (widget)
      widget->SetFocus(PR_TRUE);
  }
#endif
}

void
nsFocusManager::UpdateCaret(PRBool aMoveCaretToFocus,
                            PRBool aUpdateVisibility,
                            nsIContent* aContent)
{
#ifdef DEBUG_FOCUS
  printf("Update Caret: %d %d\n", aMoveCaretToFocus, aUpdateVisibility);
#endif

  if (!mFocusedWindow)
    return;

  
  
  nsCOMPtr<nsIDocShell> focusedDocShell = mFocusedWindow->GetDocShell();
  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(focusedDocShell);
  if (!dsti)
    return;

  PRInt32 itemType;
  dsti->GetItemType(&itemType);
  if (itemType == nsIDocShellTreeItem::typeChrome)
    return;  

  PRPackedBool browseWithCaret =
    nsContentUtils::GetBoolPref("accessibility.browsewithcaret");

  nsCOMPtr<nsIPresShell> presShell;
  focusedDocShell->GetPresShell(getter_AddRefs(presShell));
  if (!presShell)
    return;

  
  
  
  PRBool isEditable = PR_FALSE;
  nsCOMPtr<nsIEditorDocShell> editorDocShell(do_QueryInterface(dsti));
  if (editorDocShell) {
    editorDocShell->GetEditable(&isEditable);

    if (isEditable) {
      nsCOMPtr<nsIHTMLDocument> doc =
        do_QueryInterface(presShell->GetDocument());

      PRBool isContentEditableDoc =
        doc && doc->GetEditingState() == nsIHTMLDocument::eContentEditable;

      PRBool isFocusEditable =
        aContent && aContent->HasFlag(NODE_IS_EDITABLE);
      if (!isContentEditableDoc || isFocusEditable)
        return;
    }
  }

  if (!isEditable && aMoveCaretToFocus)
    MoveCaretToFocus(presShell, aContent);

  if (!aUpdateVisibility)
    return;

  
  
  
  if (!browseWithCaret) {
    nsCOMPtr<nsIContent> docContent =
      do_QueryInterface(mFocusedWindow->GetFrameElementInternal());
    if (docContent)
      browseWithCaret = docContent->AttrValueIs(kNameSpaceID_None,
                                                nsGkAtoms::showcaret,
                                                NS_LITERAL_STRING("true"),
                                                eCaseMatters);
  }

  SetCaretVisible(presShell, browseWithCaret, aContent);
}

void
nsFocusManager::MoveCaretToFocus(nsIPresShell* aPresShell, nsIContent* aContent)
{
  
  nsCOMPtr<nsIDOMDocumentRange> rangeDoc(do_QueryInterface(aPresShell->GetDocument()));
  if (rangeDoc) {
    nsCOMPtr<nsFrameSelection> frameSelection = aPresShell->FrameSelection();
    nsCOMPtr<nsISelection> domSelection = frameSelection->
      GetSelection(nsISelectionController::SELECTION_NORMAL);
    if (domSelection) {
      nsCOMPtr<nsIDOMNode> currentFocusNode(do_QueryInterface(aContent));
      
      
      domSelection->RemoveAllRanges();
      if (currentFocusNode) {
        nsCOMPtr<nsIDOMRange> newRange;
        nsresult rv = rangeDoc->CreateRange(getter_AddRefs(newRange));
        if (NS_SUCCEEDED(rv)) {
          
          
          newRange->SelectNodeContents(currentFocusNode);
          nsCOMPtr<nsIDOMNode> firstChild;
          currentFocusNode->GetFirstChild(getter_AddRefs(firstChild));
          if (!firstChild ||
              aContent->IsNodeOfType(nsINode::eHTML_FORM_CONTROL)) {
            
            
            
            newRange->SetStartBefore(currentFocusNode);
            newRange->SetEndBefore(currentFocusNode);
          }
          domSelection->AddRange(newRange);
          domSelection->CollapseToStart();
        }
      }
    }
  }
}

nsresult
nsFocusManager::SetCaretVisible(nsIPresShell* aPresShell,
                                PRBool aVisible,
                                nsIContent* aContent)
{
  
  
  
  nsRefPtr<nsCaret> caret;
  aPresShell->GetCaret(getter_AddRefs(caret));
  if (!caret)
    return NS_OK;

  PRBool caretVisible = PR_FALSE;
  caret->GetCaretVisible(&caretVisible);
  if (!aVisible && !caretVisible)
    return NS_OK;

  nsCOMPtr<nsFrameSelection> frameSelection;
  if (aContent) {
    nsIFrame *focusFrame = aPresShell->GetPrimaryFrameFor(aContent);
    if (focusFrame)
      frameSelection = focusFrame->GetFrameSelection();
  }

  nsCOMPtr<nsFrameSelection> docFrameSelection = aPresShell->FrameSelection();

  if (docFrameSelection && caret &&
     (frameSelection == docFrameSelection || !aContent)) {
    nsISelection* domSelection = docFrameSelection->
      GetSelection(nsISelectionController::SELECTION_NORMAL);
    if (domSelection) {
      
      caret->SetCaretDOMSelection(domSelection);

      
      
      

      nsCOMPtr<nsISelectionController> selCon(do_QueryInterface(aPresShell));
      if (!selCon)
        return NS_ERROR_FAILURE;

      selCon->SetCaretEnabled(aVisible);
      caret->SetCaretVisible(aVisible);
    }
  }

  return NS_OK;
}

nsresult
nsFocusManager::GetSelectionLocation(nsIDocument* aDocument,
                                     nsIPresShell* aPresShell,
                                     nsIContent **aStartContent,
                                     nsIContent **aEndContent)
{
  *aStartContent = *aEndContent = nsnull;
  nsresult rv = NS_ERROR_FAILURE;

  nsPresContext* presContext = aPresShell->GetPresContext();
  NS_ASSERTION(presContext, "mPresContent is null!!");

  nsCOMPtr<nsFrameSelection> frameSelection;
  frameSelection = aPresShell->FrameSelection();

  nsCOMPtr<nsISelection> domSelection;
  if (frameSelection) {
    domSelection = frameSelection->
      GetSelection(nsISelectionController::SELECTION_NORMAL);
  }

  nsCOMPtr<nsIDOMNode> startNode, endNode;
  PRBool isCollapsed = PR_FALSE;
  nsCOMPtr<nsIContent> startContent, endContent;
  PRInt32 startOffset = 0;
  if (domSelection) {
    domSelection->GetIsCollapsed(&isCollapsed);
    nsCOMPtr<nsIDOMRange> domRange;
    rv = domSelection->GetRangeAt(0, getter_AddRefs(domRange));
    if (domRange) {
      PRInt32 startOffset;
      domRange->GetStartContainer(getter_AddRefs(startNode));
      domRange->GetEndContainer(getter_AddRefs(endNode));
      domRange->GetStartOffset(&startOffset);

      nsIContent *childContent = nsnull;

      startContent = do_QueryInterface(startNode);
      if (startContent && startContent->IsNodeOfType(nsINode::eELEMENT)) {
        NS_ASSERTION(startOffset >= 0, "Start offset cannot be negative");  
        childContent = startContent->GetChildAt(startOffset);
        if (childContent) {
          startContent = childContent;
        }
      }

      endContent = do_QueryInterface(endNode);
      if (endContent && endContent->IsNodeOfType(nsINode::eELEMENT)) {
        PRInt32 endOffset = 0;
        domRange->GetEndOffset(&endOffset);
        NS_ASSERTION(endOffset >= 0, "End offset cannot be negative");
        childContent = endContent->GetChildAt(endOffset);
        if (childContent) {
          endContent = childContent;
        }
      }
    }
  }
  else {
    rv = NS_ERROR_INVALID_ARG;
  }

  nsIFrame *startFrame = nsnull;
  if (startContent) {
    startFrame = aPresShell->GetPrimaryFrameFor(startContent);
    if (isCollapsed) {
      
      
      
      

      nsCOMPtr<nsIDOMNode> domNode(do_QueryInterface(startContent));
      PRUint16 nodeType;
      domNode->GetNodeType(&nodeType);

      if (nodeType == nsIDOMNode::TEXT_NODE) {
        nsAutoString nodeValue;
        domNode->GetNodeValue(nodeValue);

        PRBool isFormControl =
          startContent->IsNodeOfType(nsINode::eHTML_FORM_CONTROL);

        if (nodeValue.Length() == (PRUint32)startOffset && !isFormControl &&
            startContent != aDocument->GetRootContent()) {
          
          nsCOMPtr<nsIFrameEnumerator> frameTraversal;
          nsresult rv = NS_NewFrameTraversal(getter_AddRefs(frameTraversal),
                                             presContext, startFrame,
                                             eLeaf,
                                             PR_FALSE, 
                                             PR_FALSE, 
                                             PR_TRUE   
                                             );
          NS_ENSURE_SUCCESS(rv, rv);

          nsIFrame *newCaretFrame = nsnull;
          nsCOMPtr<nsIContent> newCaretContent = startContent;
          PRBool endOfSelectionInStartNode(startContent == endContent);
          do {
            
            
            frameTraversal->Next();
            newCaretFrame = static_cast<nsIFrame*>(frameTraversal->CurrentItem());
            if (nsnull == newCaretFrame)
              break;
            newCaretContent = newCaretFrame->GetContent();            
          } while (!newCaretContent || newCaretContent == startContent);

          if (newCaretFrame && newCaretContent) {
            
            
            nsRefPtr<nsCaret> caret;
            aPresShell->GetCaret(getter_AddRefs(caret));
            nsRect caretRect;
            nsIView *caretView;
            caret->GetCaretCoordinates(nsCaret::eClosestViewCoordinates, 
                                       domSelection, &caretRect,
                                       &isCollapsed, &caretView);
            nsPoint framePt;
            nsIView *frameClosestView = newCaretFrame->GetClosestView(&framePt);
            if (caretView == frameClosestView && caretRect.y == framePt.y &&
                caretRect.x == framePt.x) {
              
              startFrame = newCaretFrame;
              startContent = newCaretContent;
              if (endOfSelectionInStartNode) {
                endContent = newCaretContent; 
              }
            }
          }
        }
      }
    }
  }

  *aStartContent = startContent;
  *aEndContent = endContent;
  NS_IF_ADDREF(*aStartContent);
  NS_IF_ADDREF(*aEndContent);

  return rv;
}

nsresult
nsFocusManager::DetermineElementToMoveFocus(nsPIDOMWindow* aWindow,
                                            nsIContent* aStartContent,
                                            PRInt32 aType,
                                            nsIContent** aNextContent)
{
  *aNextContent = nsnull;

  nsCOMPtr<nsIDocShell> docShell = aWindow->GetDocShell();
  if (!docShell)
    return NS_OK;

  nsCOMPtr<nsIContent> startContent = aStartContent;
  if (!startContent && aType != MOVEFOCUS_CARET)
    startContent = aWindow->GetFocusedNode();

  nsCOMPtr<nsIDocument> doc;
  if (startContent)
    doc = startContent->GetCurrentDoc();
  else
    doc = do_QueryInterface(aWindow->GetExtantDocument());
  if (!doc)
    return NS_OK;

  nsCOMPtr<nsILookAndFeel> lookNFeel(do_GetService(kLookAndFeelCID));
  lookNFeel->GetMetric(nsILookAndFeel::eMetric_TabFocusModel,
                       nsIContent::sTabFocusModel);

  if (aType == MOVEFOCUS_ROOT) {
    NS_IF_ADDREF(*aNextContent = GetRootForFocus(aWindow, doc, PR_FALSE, PR_FALSE));
    return NS_OK;
  }
  if (aType == MOVEFOCUS_FORWARDDOC) {
    NS_IF_ADDREF(*aNextContent = GetNextTabbableDocument(PR_TRUE));
    return NS_OK;
  }
  if (aType == MOVEFOCUS_BACKWARDDOC) {
    NS_IF_ADDREF(*aNextContent = GetNextTabbableDocument(PR_FALSE));
    return NS_OK;
  }
  
  nsIContent* rootContent = doc->GetRootContent();
  NS_ENSURE_TRUE(rootContent, NS_OK);

  nsIPresShell *presShell = doc->GetPrimaryShell();
  NS_ENSURE_TRUE(presShell, NS_OK);

  if (aType == MOVEFOCUS_FIRST) {
    if (!aStartContent)
      startContent = rootContent;
    return GetNextTabbableContent(presShell, startContent,
                                  nsnull, startContent,
                                  PR_TRUE, 1, PR_FALSE, aNextContent);
  }
  if (aType == MOVEFOCUS_LAST) {
    if (!aStartContent)
      startContent = rootContent;
    return GetNextTabbableContent(presShell, startContent,
                                  nsnull, startContent,
                                  PR_FALSE, 0, PR_FALSE, aNextContent);
  }

  PRBool forward = (aType == MOVEFOCUS_FORWARD);
  PRBool doNavigation = PR_TRUE;
  PRBool ignoreTabIndex = PR_FALSE;
  
  
  
  nsIFrame* popupFrame = nsnull;

  PRInt32 tabIndex = forward ? 1 : 0;
  if (startContent) {
    nsIFrame* frame = presShell->GetPrimaryFrameFor(startContent);
    if (startContent->Tag() == nsGkAtoms::area &&
        startContent->IsNodeOfType(nsINode::eHTML))
      startContent->IsFocusable(&tabIndex);
    else if (frame)
      frame->IsFocusable(&tabIndex, 0);

    
    
    
    if (tabIndex < 0) {
      tabIndex = 1;
      if (startContent != rootContent)
        ignoreTabIndex = PR_TRUE;
    }

    
    
    
    if (frame) {
      popupFrame = nsLayoutUtils::GetClosestFrameOfType(frame,
                                                        nsGkAtoms::menuPopupFrame);
    }

    if (popupFrame) {
      
      
      rootContent = popupFrame->GetContent();
      NS_ASSERTION(rootContent, "Popup frame doesn't have a content node");
    }
    else if (!forward && startContent == rootContent) {
      doNavigation = PR_FALSE;
    }
  }
  else {
#ifdef MOZ_XUL
    
    
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm)
      popupFrame = pm->GetTopPopup(ePopupTypePanel);
#endif
    if (popupFrame) {
      rootContent = popupFrame->GetContent();
      NS_ASSERTION(rootContent, "Popup frame doesn't have a content node");
      startContent = rootContent;
    }
    else {
      
      PRInt32 itemType;
      nsCOMPtr<nsIDocShellTreeItem> shellItem = do_QueryInterface(docShell);
      shellItem->GetItemType(&itemType);
      if (itemType != nsIDocShellTreeItem::typeChrome) {
        nsCOMPtr<nsIContent> endSelectionContent;
        GetSelectionLocation(doc, presShell,
                             getter_AddRefs(startContent),
                             getter_AddRefs(endSelectionContent));
        
        if (startContent == rootContent) {
          startContent = nsnull;
        }
        else if (startContent && startContent->HasFlag(NODE_IS_EDITABLE)) {
          
          
          nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(doc);
          if (htmlDoc &&
              htmlDoc->GetEditingState() == nsIHTMLDocument::eContentEditable)
            startContent = nsnull;
        }

        if (startContent) {
          if (aType == MOVEFOCUS_CARET) {
            GetFocusInSelection(aWindow, startContent,
                                endSelectionContent, aNextContent);
            return NS_OK;
          }
          
          
          
          
          ignoreTabIndex = PR_TRUE;
        }
      }

      if (!startContent) {
        
        startContent = rootContent;
        NS_ENSURE_TRUE(startContent, NS_OK);
      }
    }
  }

  NS_ASSERTION(startContent, "starting content not set");

  
  
  
  
  
  
  
  
  
  
  
  PRBool skipOriginalContentCheck = PR_TRUE;
  nsIContent* originalStartContent = startContent;

#ifdef DEBUG_FOCUS_NAVIGATION
  PRINTTAGF("Focus Navigation Start Content %s\n", startContent);
  printf("[Tabindex: %d Ignore: %d]", tabIndex, ignoreTabIndex);
#endif

  while (doc) {
    if (doNavigation) {
      nsCOMPtr<nsIContent> nextFocus;
      nsresult rv = GetNextTabbableContent(presShell, rootContent,
                                           skipOriginalContentCheck ? nsnull : originalStartContent,
                                           startContent, forward,
                                           tabIndex, ignoreTabIndex,
                                           getter_AddRefs(nextFocus));
      NS_ENSURE_SUCCESS(rv, rv);

      
      if (nextFocus) {
#ifdef DEBUG_FOCUS_NAVIGATION
        PRINTTAGF("Next Content: %s\n", nextFocus);
#endif
        
        
        if (nextFocus != originalStartContent)
          NS_ADDREF(*aNextContent = nextFocus);
        return NS_OK;
      }

      if (popupFrame) {
        
        
        
        if (startContent != rootContent) {
          startContent = rootContent;
          tabIndex = forward ? 1 : 0;
          continue;
        }
        return NS_OK;
      }
    }

    doNavigation = PR_TRUE;
    skipOriginalContentCheck = PR_FALSE;
    ignoreTabIndex = PR_FALSE;

    
    
    nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(docShell);

    nsCOMPtr<nsIDocShellTreeItem> docShellParent;
    dsti->GetParent(getter_AddRefs(docShellParent));
    if (docShellParent) {
      

      
      nsCOMPtr<nsPIDOMWindow> piWindow = do_GetInterface(docShell);
      NS_ENSURE_TRUE(piWindow, NS_ERROR_FAILURE);

      
      docShell = do_QueryInterface(docShellParent);
      NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

      nsCOMPtr<nsPIDOMWindow> piParentWindow = do_GetInterface(docShellParent);
      NS_ENSURE_TRUE(piParentWindow, NS_ERROR_FAILURE);
      doc = do_QueryInterface(piParentWindow->GetExtantDocument());
      NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

      presShell = doc->GetPrimaryShell();

      rootContent = doc->GetRootContent();
      startContent = do_QueryInterface(piWindow->GetFrameElementInternal());
      if (startContent) {
        nsIFrame* frame = presShell->GetPrimaryFrameFor(startContent);
        if (!frame)
          return NS_OK;

        frame->IsFocusable(&tabIndex, 0);
        if (tabIndex < 0) {
          tabIndex = 1;
          ignoreTabIndex = PR_TRUE;
        }

        
        
        
        
        
        popupFrame = nsLayoutUtils::GetClosestFrameOfType(frame,
                                                          nsGkAtoms::menuPopupFrame);
        if (popupFrame) {
          rootContent = popupFrame->GetContent();
          NS_ASSERTION(rootContent, "Popup frame doesn't have a content node");
        }
      }
      else {
        startContent = rootContent;
        tabIndex = forward ? 1 : 0;
      }
    }
    else {
      
      
      PRBool tookFocus;
      docShell->TabToTreeOwner(forward, &tookFocus);
      
      if (tookFocus) {
        nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(docShell);
        if (window->GetFocusedNode() == mFocusedContent)
          Blur(mFocusedWindow, nsnull, PR_TRUE);
        else
          window->SetFocusedNode(nsnull);
        return NS_OK;
      }

      
      startContent = rootContent;
      tabIndex = forward ? 1 : 0;
    }

    
    
    if (startContent == originalStartContent)
      break;
  }

  return NS_OK;
}

nsresult
nsFocusManager::GetNextTabbableContent(nsIPresShell* aPresShell,
                                       nsIContent* aRootContent,
                                       nsIContent* aOriginalStartContent,
                                       nsIContent* aStartContent,
                                       PRBool aForward,
                                       PRInt32 aCurrentTabIndex,
                                       PRBool aIgnoreTabIndex,
                                       nsIContent** aResultContent)
{
  *aResultContent = nsnull;

  if (!aStartContent)
    return NS_OK;

#ifdef DEBUG_FOCUS_NAVIGATION
  PRINTTAGF("GetNextTabbable: %s", aStartContent);
  printf(" tabindex: %d\n", aCurrentTabIndex);
#endif

  nsPresContext* presContext = aPresShell->GetPresContext();

  while (1) {
    nsIFrame* aStartFrame = aPresShell->GetPrimaryFrameFor(aStartContent);
    if (!aStartFrame) {
      
      aStartFrame = aPresShell->GetPrimaryFrameFor(aRootContent);
      if (!aStartFrame)
        return NS_OK;
      aStartContent = aRootContent;
    }

    nsCOMPtr<nsIFrameEnumerator> frameTraversal;
    nsresult rv = NS_NewFrameTraversal(getter_AddRefs(frameTraversal),
                                       presContext, aStartFrame,
                                       ePreOrder,
                                       PR_FALSE, 
                                       PR_FALSE, 
                                       PR_TRUE   
                                       );
    NS_ENSURE_SUCCESS(rv, rv);

    if (aStartContent == aRootContent) {
      if (!aForward)
        frameTraversal->Last();
    }
    else if (!aStartContent || aStartContent->Tag() != nsGkAtoms::area ||
             !aStartContent->IsNodeOfType(nsINode::eHTML)) {
      
      
      if (aForward)
        frameTraversal->Next();
      else
        frameTraversal->Prev();
    }

    
    nsIFrame* frame = static_cast<nsIFrame*>(frameTraversal->CurrentItem());
    while (frame) {
      
      
      
      
      
      
      

      PRInt32 tabIndex;
      frame->IsFocusable(&tabIndex, 0);

#ifdef DEBUG_FOCUS_NAVIGATION
      if (frame->GetContent()) {
        PRINTTAGF("Next Tabbable %s:", frame->GetContent());
        printf(" with tabindex: %d expected: %d\n", tabIndex, aCurrentTabIndex);
      }
#endif

      nsIContent* currentContent = frame->GetContent();
      if (tabIndex >= 0) {
        NS_ASSERTION(currentContent, "IsFocusable set a tabindex for a frame with no content");
        if (currentContent->Tag() == nsGkAtoms::img &&
            currentContent->HasAttr(kNameSpaceID_None, nsGkAtoms::usemap)) {
          
          
          nsIContent *areaContent =
            GetNextTabbableMapArea(aForward, aCurrentTabIndex,
                                   currentContent, aStartContent);
          if (areaContent) {
            NS_ADDREF(*aResultContent = areaContent);
            return NS_OK;
          }
        }
        else if (aIgnoreTabIndex || aCurrentTabIndex == tabIndex) {
          
          if (aOriginalStartContent && currentContent == aOriginalStartContent) {
            NS_ADDREF(*aResultContent = currentContent);
            return NS_OK;
          }

          
          
          nsIDocument* doc = currentContent->GetCurrentDoc();
          NS_ASSERTION(doc, "content not in document");
          nsIDocument* subdoc = doc->GetSubDocumentFor(currentContent);
          if (subdoc) {
            if (!subdoc->EventHandlingSuppressed()) {
              if (aForward) {
                
                
                nsCOMPtr<nsPIDOMWindow> subframe = subdoc->GetWindow();
                if (subframe) {
                  *aResultContent = GetRootForFocus(subframe, subdoc, PR_FALSE, PR_TRUE);
                  if (*aResultContent) {
                    NS_ADDREF(*aResultContent);
                    return NS_OK;
                  }
                }
              }
              nsIContent* rootContent = subdoc->GetRootContent();
              nsIPresShell* subShell = subdoc->GetPrimaryShell();
              if (rootContent && subShell) {
                rv = GetNextTabbableContent(subShell, rootContent,
                                            aOriginalStartContent, rootContent,
                                            aForward, (aForward ? 1 : 0),
                                            PR_FALSE, aResultContent);
                NS_ENSURE_SUCCESS(rv, rv);
                if (*aResultContent)
                  return NS_OK;
              }
            }
          }
          
          
          
          
          
          
          
          
          
          
          
          else if (currentContent == aRootContent ||
                   (currentContent != aStartContent &&
                    (aForward || !GetRedirectedFocus(currentContent)))) {
            NS_ADDREF(*aResultContent = currentContent);
            return NS_OK;
          }
        }
      }
      else if (aOriginalStartContent && currentContent == aOriginalStartContent) {
        
        
        
        NS_ADDREF(*aResultContent = currentContent);
        return NS_OK;
      }

      
      
      
      
      
      
      
      
      do {
        if (aForward)
          frameTraversal->Next();
        else
          frameTraversal->Prev();
        frame = static_cast<nsIFrame*>(frameTraversal->CurrentItem());
      } while (frame && frame->GetPrevContinuation());
    }

    
    
    if (aCurrentTabIndex == (aForward ? 0 : 1)) {
      
      
      if (!aForward) {
        nsCOMPtr<nsPIDOMWindow> window = GetCurrentWindow(aRootContent);
        NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);
        NS_IF_ADDREF(*aResultContent =
                     GetRootForFocus(window, aRootContent->GetCurrentDoc(), PR_FALSE, PR_TRUE));
      }
      break;
    }

    
    aCurrentTabIndex = GetNextTabIndex(aRootContent, aCurrentTabIndex, aForward);
    aStartContent = aRootContent;
  }

  return NS_OK;
}

nsIContent*
nsFocusManager::GetNextTabbableMapArea(PRBool aForward,
                                       PRInt32 aCurrentTabIndex,
                                       nsIContent* aImageContent,
                                       nsIContent* aStartContent)
{
  nsAutoString useMap;
  aImageContent->GetAttr(kNameSpaceID_None, nsGkAtoms::usemap, useMap);

  nsCOMPtr<nsIDocument> doc = aImageContent->GetDocument();
  if (doc) {
    nsCOMPtr<nsIDOMHTMLMapElement> imageMap = nsImageMapUtils::FindImageMap(doc, useMap);
    if (!imageMap)
      return nsnull;
    nsCOMPtr<nsIContent> mapContent = do_QueryInterface(imageMap);
    PRUint32 count = mapContent->GetChildCount();
    

    PRInt32 index = mapContent->IndexOf(aStartContent);
    PRInt32 tabIndex;
    if (index < 0 || (aStartContent->IsFocusable(&tabIndex) &&
                      tabIndex != aCurrentTabIndex)) {
      
      
      
      
      index = aForward ? -1 : (PRInt32)count;
    }

    
    nsCOMPtr<nsIContent> areaContent;
    while ((areaContent = mapContent->GetChildAt(aForward ? ++index : --index)) != nsnull) {
      if (areaContent->IsFocusable(&tabIndex) && tabIndex == aCurrentTabIndex) {
        return areaContent;
      }
    }
  }

  return nsnull;
}

PRInt32
nsFocusManager::GetNextTabIndex(nsIContent* aParent,
                                PRInt32 aCurrentTabIndex,
                                PRBool aForward)
{
  PRInt32 tabIndex, childTabIndex;
  nsIContent *child;

  PRUint32 count = aParent->GetChildCount();

  if (aForward) {
    tabIndex = 0;
    for (PRUint32 index = 0; index < count; index++) {
      child = aParent->GetChildAt(index);
      childTabIndex = GetNextTabIndex(child, aCurrentTabIndex, aForward);
      if (childTabIndex > aCurrentTabIndex && childTabIndex != tabIndex) {
        tabIndex = (tabIndex == 0 || childTabIndex < tabIndex) ? childTabIndex : tabIndex;
      }

      nsAutoString tabIndexStr;
      child->GetAttr(kNameSpaceID_None, nsGkAtoms::tabindex, tabIndexStr);
      PRInt32 ec, val = tabIndexStr.ToInteger(&ec);
      if (NS_SUCCEEDED (ec) && val > aCurrentTabIndex && val != tabIndex) {
        tabIndex = (tabIndex == 0 || val < tabIndex) ? val : tabIndex;
      }
    }
  }
  else { 
    tabIndex = 1;
    for (PRUint32 index = 0; index < count; index++) {
      child = aParent->GetChildAt(index);
      childTabIndex = GetNextTabIndex(child, aCurrentTabIndex, aForward);
      if ((aCurrentTabIndex == 0 && childTabIndex > tabIndex) ||
          (childTabIndex < aCurrentTabIndex && childTabIndex > tabIndex)) {
        tabIndex = childTabIndex;
      }

      nsAutoString tabIndexStr;
      child->GetAttr(kNameSpaceID_None, nsGkAtoms::tabindex, tabIndexStr);
      PRInt32 ec, val = tabIndexStr.ToInteger(&ec);
      if (NS_SUCCEEDED (ec)) {
        if ((aCurrentTabIndex == 0 && val > tabIndex) ||
            (val < aCurrentTabIndex && val > tabIndex) ) {
          tabIndex = val;
        }
      }
    }
  }

  return tabIndex;
}

nsIContent*
nsFocusManager::GetRootForFocus(nsPIDOMWindow* aWindow,
                                nsIDocument* aDocument,
                                PRBool aIsForDocNavigation,
                                PRBool aCheckVisibility)
{
  
  
  if (aIsForDocNavigation) {
    nsCOMPtr<nsIContent> docContent =
      do_QueryInterface(aWindow->GetFrameElementInternal());
    if (docContent && docContent->Tag() == nsGkAtoms::iframe)
      return nsnull;
  }
  else  {
    PRInt32 itemType;
    nsCOMPtr<nsIDocShellTreeItem> shellItem = do_QueryInterface(aWindow->GetDocShell());
    shellItem->GetItemType(&itemType);

    if (itemType == nsIDocShellTreeItem::typeChrome)
      return nsnull;
  }

  if (aCheckVisibility && !IsWindowVisible(aWindow))
    return nsnull;

  nsIContent *rootContent = aDocument->GetRootContent();
  if (rootContent) {
    if (aCheckVisibility) {
      nsIPresShell* presShell = aDocument->GetPrimaryShell();
      if (!presShell || !presShell->GetPrimaryFrameFor(rootContent))
        return nsnull;
    }

    
    nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(aDocument);
    if (htmlDoc) {
      PRUint32 childCount = rootContent->GetChildCount();
      for (PRUint32 i = 0; i < childCount; ++i) {
        nsIContent *childContent = rootContent->GetChildAt(i);
        nsINodeInfo *ni = childContent->NodeInfo();
        if (childContent->IsNodeOfType(nsINode::eHTML) &&
            ni->Equals(nsGkAtoms::frameset))
          return nsnull;
      }
    }
  }

  return rootContent;
}

void
nsFocusManager::GetLastDocShell(nsIDocShellTreeItem* aItem,
                                nsIDocShellTreeItem** aResult)
{
  *aResult = nsnull;

  nsCOMPtr<nsIDocShellTreeItem> curItem = aItem;
  while (curItem) {
    PRInt32 childCount = 0;
    curItem->GetChildCount(&childCount);
    if (!childCount) {
      *aResult = curItem;
      NS_ADDREF(*aResult);
      return;
    }

    
    curItem->GetChildAt(childCount - 1, getter_AddRefs(curItem));
  }
}

void
nsFocusManager::GetNextDocShell(nsIDocShellTreeItem* aItem,
                                nsIDocShellTreeItem** aResult)
{
  *aResult = nsnull;

  PRInt32 childCount = 0;
  aItem->GetChildCount(&childCount);
  if (childCount) {
    aItem->GetChildAt(0, aResult);
    if (*aResult)
      return;
  }

  nsCOMPtr<nsIDocShellTreeItem> curItem = aItem;
  while (curItem) {
    nsCOMPtr<nsIDocShellTreeItem> parentItem;
    curItem->GetParent(getter_AddRefs(parentItem));
    if (!parentItem)
      return;

    
    
    nsCOMPtr<nsIDocShellTreeItem> iterItem;
    childCount = 0;
    parentItem->GetChildCount(&childCount);
    for (PRInt32 index = 0; index < childCount; ++index) {
      parentItem->GetChildAt(index, getter_AddRefs(iterItem));
      if (iterItem == curItem) {
        ++index;
        if (index < childCount) {
          parentItem->GetChildAt(index, aResult);
          if (*aResult)
            return;
        }
        break;
      }
    }

    curItem = parentItem;
  }
}

void
nsFocusManager::GetPreviousDocShell(nsIDocShellTreeItem* aItem,
                                    nsIDocShellTreeItem** aResult)
{
  *aResult = nsnull;

  nsCOMPtr<nsIDocShellTreeItem> parentItem;
  aItem->GetParent(getter_AddRefs(parentItem));
  if (!parentItem)
    return;

  
  
  PRInt32 childCount = 0;
  parentItem->GetChildCount(&childCount);
  nsCOMPtr<nsIDocShellTreeItem> prevItem, iterItem;
  for (PRInt32 index = 0; index < childCount; ++index) {
    parentItem->GetChildAt(index, getter_AddRefs(iterItem));
    if (iterItem == aItem)
      break;
    prevItem = iterItem;
  }

  if (prevItem)
    GetLastDocShell(prevItem, aResult);
  else
    NS_ADDREF(*aResult = parentItem);
}

nsIContent*
nsFocusManager::GetNextTabbableDocument(PRBool aForward)
{
  nsCOMPtr<nsIDocShellTreeItem> startItem;
  if (mFocusedWindow) {
    startItem = do_QueryInterface(mFocusedWindow->GetDocShell());
  }
  else {
    nsCOMPtr<nsIWebNavigation> webnav = do_GetInterface(mActiveWindow);
    startItem = do_QueryInterface(webnav);
  }
  if (!startItem)
    return nsnull;

  
  
  nsIContent* content = nsnull;
  nsCOMPtr<nsIDocShellTreeItem> curItem = startItem;
  nsCOMPtr<nsIDocShellTreeItem> nextItem;
  do {
    if (aForward) {
      GetNextDocShell(curItem, getter_AddRefs(nextItem));
      if (!nextItem) {
        
        startItem->GetRootTreeItem(getter_AddRefs(nextItem));
      }
    }
    else {
      GetPreviousDocShell(curItem, getter_AddRefs(nextItem));
      if (!nextItem) {
        
        nsCOMPtr<nsIDocShellTreeItem> rootItem;
        startItem->GetRootTreeItem(getter_AddRefs(rootItem));
        GetLastDocShell(rootItem, getter_AddRefs(nextItem));
      }
    }

    curItem = nextItem;
    nsCOMPtr<nsPIDOMWindow> nextFrame = do_GetInterface(nextItem);
    if (!nextFrame)
      return nsnull;

    nsCOMPtr<nsIDocument> doc = do_QueryInterface(nextFrame->GetExtantDocument());
    if (doc && !doc->EventHandlingSuppressed()) {
      content = GetRootForFocus(nextFrame, doc, PR_TRUE, PR_TRUE);
      if (content && !GetRootForFocus(nextFrame, doc, PR_FALSE, PR_FALSE)) {
        
        
        
        nsCOMPtr<nsIContent> nextFocus;
        nsIContent* rootContent = doc->GetRootContent();
        nsIPresShell* presShell = doc->GetPrimaryShell();
        if (presShell) {
          nsresult rv = GetNextTabbableContent(presShell, rootContent,
                                               nsnull, rootContent,
                                               PR_TRUE, 1, PR_FALSE,
                                               getter_AddRefs(nextFocus));
          return NS_SUCCEEDED(rv) ? nextFocus.get() : nsnull;
        }
      }
    }
  } while (!content);

  return content;
}

void
nsFocusManager::GetFocusInSelection(nsPIDOMWindow* aWindow,
                                    nsIContent* aStartSelection,
                                    nsIContent* aEndSelection,
                                    nsIContent** aFocusedContent)
{
  *aFocusedContent = nsnull;

  nsCOMPtr<nsIContent> testContent = aStartSelection;
  nsCOMPtr<nsIContent> nextTestContent = aEndSelection;

  nsCOMPtr<nsIContent> currentFocus = aWindow->GetFocusedNode();

  
  

  
  
  
  
  while (testContent) {
    
    

    nsCOMPtr<nsIURI> uri;
    if (testContent == currentFocus ||
        testContent->IsLink(getter_AddRefs(uri))) {
      NS_ADDREF(*aFocusedContent = testContent);
      return;
    }

    
    testContent = testContent->GetParent();

    if (!testContent) {
      
      testContent = nextTestContent;
      nextTestContent = nsnull;
    }
  }

  
  

  
  nsCOMPtr<nsIDOMNode> selectionNode(do_QueryInterface(aStartSelection));
  nsCOMPtr<nsIDOMNode> endSelectionNode(do_QueryInterface(aEndSelection));
  nsCOMPtr<nsIDOMNode> testNode;

  do {
    testContent = do_QueryInterface(selectionNode);

    
    
    nsCOMPtr<nsIURI> uri;
    if (testContent == currentFocus ||
        testContent->IsLink(getter_AddRefs(uri))) {
      NS_ADDREF(*aFocusedContent = testContent);
      return;
    }

    selectionNode->GetFirstChild(getter_AddRefs(testNode));
    if (testNode) {
      selectionNode = testNode;
      continue;
    }

    if (selectionNode == endSelectionNode)
      break;
    selectionNode->GetNextSibling(getter_AddRefs(testNode));
    if (testNode) {
      selectionNode = testNode;
      continue;
    }

    do {
      selectionNode->GetParentNode(getter_AddRefs(testNode));
      if (!testNode || testNode == endSelectionNode) {
        selectionNode = nsnull;
        break;
      }
      testNode->GetNextSibling(getter_AddRefs(selectionNode));
      if (selectionNode)
        break;
      selectionNode = testNode;
    } while (PR_TRUE);
  }
  while (selectionNode && selectionNode != endSelectionNode);
}

nsresult
NS_NewFocusManager(nsIFocusManager** aResult)
{
  NS_IF_ADDREF(*aResult = nsFocusManager::GetFocusManager());
  return NS_OK;
}
