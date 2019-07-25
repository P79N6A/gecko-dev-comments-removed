



































#include "mozilla/dom/TabParent.h"

#include "nsFocusManager.h"

#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIServiceManager.h"
#include "nsIEnumerator.h"
#include "nsGkAtoms.h"
#include "nsContentUtils.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMElement.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsIDOMHTMLLegendElement.h"
#include "nsIDOMDocument.h"
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
#include "nsEventStateManager.h"
#include "nsIMEStateManager.h"
#include "nsIWebNavigation.h"
#include "nsCaret.h"
#include "nsIWidget.h"
#include "nsIBaseWindow.h"
#include "nsIViewManager.h"
#include "nsFrameSelection.h"
#include "nsXULPopupManager.h"
#include "nsIDOMNodeFilter.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIPrincipal.h"
#include "mozilla/dom/Element.h"
#include "mozAutoDocUpdate.h"
#include "mozilla/Preferences.h"
#include "mozilla/LookAndFeel.h"

#ifdef MOZ_XUL
#include "nsIDOMXULTextboxElement.h"
#include "nsIDOMXULMenuListElement.h"
#endif

#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif

using namespace mozilla;
using namespace mozilla::dom;



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
                            nsIDOMEventTarget* aTarget)
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
  nsCOMPtr<nsIDOMEventTarget> mTarget;
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

nsFocusManager* nsFocusManager::sInstance = nsnull;
bool nsFocusManager::sMouseFocusesFormControl = false;

static const char* kObservedPrefs[] = {
  "accessibility.browsewithcaret",
  "accessibility.tabfocus_applies_to_xul",
  "accessibility.mouse_focuses_formcontrol",
  NULL
};

nsFocusManager::nsFocusManager()
{ }

nsFocusManager::~nsFocusManager()
{
  Preferences::RemoveObservers(this, kObservedPrefs);

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->RemoveObserver(this, "xpcom-shutdown");
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
    Preferences::GetBool("accessibility.tabfocus_applies_to_xul",
                         nsIContent::sTabFocusModelAppliesToXUL);

  sMouseFocusesFormControl =
    Preferences::GetBool("accessibility.mouse_focuses_formcontrol", false);

  Preferences::AddWeakObservers(fm, kObservedPrefs);

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->AddObserver(fm, "xpcom-shutdown", true);
  }

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
  if (!nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    nsDependentString data(aData);
    if (data.EqualsLiteral("accessibility.browsewithcaret")) {
      UpdateCaret(false, true, mFocusedContent);
    }
    else if (data.EqualsLiteral("accessibility.tabfocus_applies_to_xul")) {
      nsIContent::sTabFocusModelAppliesToXUL =
        Preferences::GetBool("accessibility.tabfocus_applies_to_xul",
                             nsIContent::sTabFocusModelAppliesToXUL);
    }
    else if (data.EqualsLiteral("accessibility.mouse_focuses_formcontrol")) {
      sMouseFocusesFormControl =
        Preferences::GetBool("accessibility.mouse_focuses_formcontrol",
                             false);
    }
  } else if (!nsCRT::strcmp(aTopic, "xpcom-shutdown")) {
    mActiveWindow = nsnull;
    mFocusedWindow = nsnull;
    mFocusedContent = nsnull;
    mFirstBlurEvent = nsnull;
    mFirstFocusEvent = nsnull;
    mWindowBeingLowered = nsnull;
    mDelayedBlurFocusEvents.Clear();
    mMouseDownEventHandlingDocument = nsnull;
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
nsFocusManager::GetFocusedDescendant(nsPIDOMWindow* aWindow, bool aDeep,
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
  if (aContent->IsXUL()) {
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


PRUint32
nsFocusManager::GetFocusMoveReason(PRUint32 aFlags)
{
  PRUint32 reason = IMEContext::FOCUS_MOVED_UNKNOWN;
  if (aFlags & nsIFocusManager::FLAG_BYMOUSE) {
    reason = IMEContext::FOCUS_MOVED_BY_MOUSE;
  } else if (aFlags & nsIFocusManager::FLAG_BYKEY) {
    reason = IMEContext::FOCUS_MOVED_BY_KEY;
  } else if (aFlags & nsIFocusManager::FLAG_BYMOVEFOCUS) {
    reason = IMEContext::FOCUS_MOVED_BY_MOVEFOCUS;
  }

  return reason;
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
  if (piWindow)
      piWindow = piWindow->GetOuterWindow();

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
    
    
    SetFocusInner(frameContent, 0, false, true);
  }
  else {
    
    
    
    
    nsIContent* content = windowToFocus->GetFocusedNode();
    if (content) {
      nsCOMPtr<nsIDOMWindow> childWindow = GetContentWindow(content);
      if (childWindow)
        ClearFocus(windowToFocus);
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

  SetFocusInner(newFocus, aFlags, true, true);

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::MoveFocus(nsIDOMWindow* aWindow, nsIDOMElement* aStartElement,
                          PRUint32 aType, PRUint32 aFlags, nsIDOMElement** aElement)
{
  *aElement = nsnull;

#ifdef DEBUG_FOCUS
  printf("<<MoveFocus Type: %d Flags: %x>>\n<<", aType, aFlags);

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

  
  
  
  if (aType != MOVEFOCUS_ROOT && aType != MOVEFOCUS_CARET &&
      (aFlags & FOCUSMETHOD_MASK) == 0) {
    aFlags |= FLAG_BYMOVEFOCUS;
  }

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
    
    
    
    
    SetFocusInner(newFocus, aFlags, aType != MOVEFOCUS_CARET, true);
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
    bool isAncestor = (window != mFocusedWindow);
    if (Blur(window, nsnull, isAncestor, true)) {
      
      
      if (isAncestor)
        Focus(window, nsnull, 0, true, false, false, true);
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
                                           bool aDeep,
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
        bool isEditable;
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
    bool isEnabled = true;
    if (NS_SUCCEEDED(baseWindow->GetEnabled(&isEnabled)) && !isEnabled) {
      return NS_ERROR_FAILURE;
    }

    baseWindow->SetVisibility(true);
  }

  
  
  window->ActivateOrDeactivate(true);

  
  nsCOMPtr<nsIDocument> document = do_QueryInterface(window->GetExtantDocument());
  nsContentUtils::DispatchTrustedEvent(document,
                                       window,
                                       NS_LITERAL_STRING("activate"),
                                       true, true, nsnull);

  
  nsCOMPtr<nsPIDOMWindow> currentWindow;
  nsCOMPtr<nsIContent> currentFocus =
    GetFocusedDescendant(window, true, getter_AddRefs(currentWindow));

  NS_ASSERTION(currentWindow, "window raised with no window current");
  if (!currentWindow)
    return NS_OK;

  nsCOMPtr<nsIDocShell> currentDocShell = currentWindow->GetDocShell();

  nsCOMPtr<nsIPresShell> presShell;
  currentDocShell->GetPresShell(getter_AddRefs(presShell));
  if (presShell) {
    
    
    nsRefPtr<nsFrameSelection> frameSelection = presShell->FrameSelection();
    frameSelection->SetMouseDownState(false);
  }

  Focus(currentWindow, currentFocus, 0, true, false, true, true);

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

  
  nsIPresShell::SetCapturingContent(nsnull, 0);

  
  
  window->ActivateOrDeactivate(false);

  
  nsCOMPtr<nsIDocument> document = do_QueryInterface(window->GetExtantDocument());
  nsContentUtils::DispatchTrustedEvent(document,
                                       window,
                                       NS_LITERAL_STRING("deactivate"),
                                       true, true, nsnull);

  
  
  
  mWindowBeingLowered = mActiveWindow;
  mActiveWindow = nsnull;

  if (mFocusedWindow)
    Blur(nsnull, nsnull, true, true);

  mWindowBeingLowered = nsnull;

  return NS_OK;
}

nsresult
nsFocusManager::ContentRemoved(nsIDocument* aDocument, nsIContent* aContent)
{
  NS_ENSURE_ARG(aDocument);
  NS_ENSURE_ARG(aContent);

  nsPIDOMWindow *window = aDocument->GetWindow();
  if (!window)
    return NS_OK;

  
  
  nsIContent* content = window->GetFocusedNode();
  if (content && nsContentUtils::ContentIsDescendantOf(content, aContent)) {
    bool shouldShowFocusRing = window->ShouldShowFocusRing();
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

    NotifyFocusStateChange(aContent, shouldShowFocusRing, false);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::WindowShown(nsIDOMWindow* aWindow, bool aNeedsFocus)
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

  if (aNeedsFocus) {
    nsCOMPtr<nsPIDOMWindow> currentWindow;
    nsCOMPtr<nsIContent> currentFocus =
      GetFocusedDescendant(window, true, getter_AddRefs(currentWindow));
    if (currentWindow)
      Focus(currentWindow, currentFocus, 0, true, false, false, true);
  }
  else {
    
    
    
    EnsureCurrentWidgetFocused();
  }

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

  
  
  

  nsIContent* oldFocusedContent = mFocusedContent;
  mFocusedContent = nsnull;

  if (oldFocusedContent && oldFocusedContent->IsInDoc()) {
    NotifyFocusStateChange(oldFocusedContent,
                           mFocusedWindow->ShouldShowFocusRing(),
                           false);
  }

  nsCOMPtr<nsIDocShell> focusedDocShell = mFocusedWindow->GetDocShell();
  nsCOMPtr<nsIPresShell> presShell;
  focusedDocShell->GetPresShell(getter_AddRefs(presShell));

  nsIMEStateManager::OnTextStateBlur(nsnull, nsnull);
  if (presShell) {
    nsIMEStateManager::OnChangeFocus(presShell->GetPresContext(), nsnull, IMEContext::FOCUS_REMOVED);
    SetCaretVisible(presShell, false, nsnull);
  }

  
  
  
  
  
  
  bool beingDestroyed;
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
    nsCOMPtr<nsIWebNavigation> webnav(do_GetInterface(mFocusedWindow));
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
      nsCOMPtr<nsIDOMEventTarget> target = mDelayedBlurFocusEvents[i].mTarget;
      nsCOMPtr<nsIPresShell> presShell = mDelayedBlurFocusEvents[i].mPresShell;
      mDelayedBlurFocusEvents.RemoveElementAt(i);
      SendFocusOrBlurEvent(type, presShell, aDocument, target, 0, false);
      --i;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::FocusPlugin(nsIContent* aContent)
{
  NS_ENSURE_ARG(aContent);
  SetFocusInner(aContent, 0, true, false);
  return NS_OK;
}


void
nsFocusManager::NotifyFocusStateChange(nsIContent* aContent,
                                       bool aWindowShouldShowFocusRing,
                                       bool aGettingFocus)
{
  if (!aContent->IsElement()) {
    return;
  }
  nsEventStates eventState = NS_EVENT_STATE_FOCUS;
  if (aWindowShouldShowFocusRing) {
    eventState |= NS_EVENT_STATE_FOCUSRING;
  }
  if (aGettingFocus) {
    aContent->AsElement()->AddStates(eventState);
  } else {
    aContent->AsElement()->RemoveStates(eventState);
  }
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
          widget->SetFocus(false);
      }
    }
  }
}

void
nsFocusManager::SetFocusInner(nsIContent* aNewContent, PRInt32 aFlags,
                              bool aFocusChanged, bool aAdjustWidget)
{
  
  nsCOMPtr<nsIContent> contentToFocus = CheckIfFocusable(aNewContent, aFlags);
  if (!contentToFocus)
    return;

  
  
  
  
  nsCOMPtr<nsPIDOMWindow> newWindow;
  nsCOMPtr<nsPIDOMWindow> subWindow = GetContentWindow(contentToFocus);
  if (subWindow) {
    contentToFocus = GetFocusedDescendant(subWindow, true, getter_AddRefs(newWindow));
    
    
    aFocusChanged = false;
  }

  
  if (!newWindow)
    newWindow = GetCurrentWindow(contentToFocus);

  
  
  
  if (!newWindow || (newWindow == mFocusedWindow && contentToFocus == mFocusedContent))
    return;

  
  
  
  nsCOMPtr<nsIDocShell> newDocShell = newWindow->GetDocShell();
  nsCOMPtr<nsIDocShell> docShell = newDocShell;
  while (docShell) {
    bool inUnload;
    docShell->GetIsInUnload(&inUnload);
    if (inUnload)
      return;

    bool beingDestroyed;
    docShell->IsBeingDestroyed(&beingDestroyed);
    if (beingDestroyed)
      return;

    nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(docShell);
    nsCOMPtr<nsIDocShellTreeItem> parentDsti;
    dsti->GetParent(getter_AddRefs(parentDsti));
    docShell = do_QueryInterface(parentDsti);
  }

  
  bool isElementInFocusedWindow = (mFocusedWindow == newWindow);

  if (!isElementInFocusedWindow && mFocusedWindow && newWindow &&
      nsContentUtils::IsHandlingKeyBoardEvent()) {
    nsCOMPtr<nsIScriptObjectPrincipal> focused =
      do_QueryInterface(mFocusedWindow);
    nsCOMPtr<nsIScriptObjectPrincipal> newFocus =
      do_QueryInterface(newWindow);
    nsIPrincipal* focusedPrincipal = focused->GetPrincipal();
    nsIPrincipal* newPrincipal = newFocus->GetPrincipal();
    if (!focusedPrincipal || !newPrincipal) {
      return;
    }
    bool subsumes = false;
    focusedPrincipal->Subsumes(newPrincipal, &subsumes);
    if (!subsumes && !nsContentUtils::IsCallerTrustedForWrite()) {
      NS_WARNING("Not allowed to focus the new window!");
      return;
    }
  }

  
  
  bool isElementInActiveWindow = false;

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
  printf(" Flags: %x Current Window: %p New Window: %p Current Element: %p",
         aFlags, mFocusedWindow.get(), newWindow.get(), mFocusedContent.get());
  printf(" In Active Window: %d In Focused Window: %d\n",
         isElementInActiveWindow, isElementInFocusedWindow);
#endif

  
  
  
  bool allowFrameSwitch = !(aFlags & FLAG_NOSWITCHFRAME) ||
                            IsSameOrAncestor(newWindow, mFocusedWindow);

  
  
  bool sendFocusEvent =
    isElementInActiveWindow && allowFrameSwitch && IsWindowVisible(newWindow);

  
  
  
  
  
  if (sendFocusEvent && mFocusedContent &&
      mFocusedContent->OwnerDoc() != aNewContent->OwnerDoc()) {
    
    
    
    nsCOMPtr<nsIDOMNode> domNode(do_QueryInterface(mFocusedContent));
    sendFocusEvent = nsContentUtils::CanCallerAccess(domNode);
    if (!sendFocusEvent && mMouseDownEventHandlingDocument) {
      
      
      domNode = do_QueryInterface(mMouseDownEventHandlingDocument);
      sendFocusEvent = nsContentUtils::CanCallerAccess(domNode);
    }
  }

  if (sendFocusEvent) {
    
    if (mFocusedWindow) {
      
      
      
      
      
      bool currentIsSameOrAncestor = IsSameOrAncestor(mFocusedWindow, newWindow);
      
      
      
      
      
      
      
      
      
      
      
      
      nsCOMPtr<nsPIDOMWindow> commonAncestor;
      if (!isElementInFocusedWindow)
        commonAncestor = GetCommonAncestor(newWindow, mFocusedWindow);

      if (!Blur(currentIsSameOrAncestor ? mFocusedWindow.get() : nsnull,
                commonAncestor, !isElementInFocusedWindow, aAdjustWidget))
        return;
    }

    Focus(newWindow, contentToFocus, aFlags, !isElementInFocusedWindow,
          aFocusChanged, false, aAdjustWidget);
  }
  else {
    
    
    if (allowFrameSwitch)
      AdjustWindowFocus(newWindow, true);

    
    PRUint32 focusMethod = aFocusChanged ? aFlags & FOCUSMETHODANDRING_MASK :
                           newWindow->GetFocusMethod() | (aFlags & FLAG_SHOWRING);
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

bool
nsFocusManager::IsSameOrAncestor(nsPIDOMWindow* aPossibleAncestor,
                                 nsPIDOMWindow* aWindow)
{
  nsCOMPtr<nsIWebNavigation> awebnav(do_GetInterface(aPossibleAncestor));
  nsCOMPtr<nsIDocShellTreeItem> ancestordsti = do_QueryInterface(awebnav);

  nsCOMPtr<nsIWebNavigation> fwebnav(do_GetInterface(aWindow));
  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(fwebnav);
  while (dsti) {
    if (dsti == ancestordsti)
      return true;
    nsCOMPtr<nsIDocShellTreeItem> parentDsti;
    dsti->GetParent(getter_AddRefs(parentDsti));
    dsti.swap(parentDsti);
  }

  return false;
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

  nsAutoTArray<nsIDocShellTreeItem*, 30> parents1, parents2;
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
  for (len = NS_MIN(pos1, pos2); len > 0; --len) {
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
nsFocusManager::AdjustWindowFocus(nsPIDOMWindow* aWindow,
                                  bool aCheckPermission)
{
  bool isVisible = IsWindowVisible(aWindow);

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

      
      
      
      if (aCheckPermission && !nsContentUtils::CanCallerAccess(window))
        break;

      window->SetFocusedNode(frameContent);
    }
  }
}

bool
nsFocusManager::IsWindowVisible(nsPIDOMWindow* aWindow)
{
  if (!aWindow)
    return false;

  nsCOMPtr<nsIDocShell> docShell = aWindow->GetDocShell();
  nsCOMPtr<nsIBaseWindow> baseWin(do_QueryInterface(docShell));
  if (!baseWin)
    return false;

  bool visible = false;
  baseWin->GetVisibility(&visible);
  return visible;
}

bool
nsFocusManager::IsNonFocusableRoot(nsIContent* aContent)
{
  NS_PRECONDITION(aContent, "aContent must not be NULL");
  NS_PRECONDITION(aContent->IsInDoc(), "aContent must be in a document");

  
  
  
  
  
  nsIDocument* doc = aContent->GetCurrentDoc();
  NS_ASSERTION(doc, "aContent must have current document");
  return aContent == doc->GetRootElement() &&
           (doc->HasFlag(NODE_IS_EDITABLE) || !aContent->IsEditable());
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

  
  doc->FlushPendingNotifications(Flush_Layout);

  nsIPresShell *shell = doc->GetShell();
  if (!shell)
    return nsnull;

  
  if (aContent == doc->GetRootElement())
    return aContent;

  
  nsPresContext* presContext = shell->GetPresContext();
  if (presContext && presContext->Type() == nsPresContext::eContext_PrintPreview)
    return nsnull;

  nsIFrame* frame = aContent->GetPrimaryFrame();
  if (!frame)
    return nsnull;

  if (aContent->Tag() == nsGkAtoms::area && aContent->IsHTML()) {
    
    
    
    return frame->IsVisibleConsideringAncestors() &&
           aContent->IsFocusable() ? aContent : nsnull;
  }

  
  
  
  
  nsIDocument* subdoc = doc->GetSubDocumentFor(aContent);
  if (subdoc && IsWindowVisible(subdoc->GetWindow())) {
    const nsStyleUserInterface* ui = frame->GetStyleUserInterface();
    PRInt32 tabIndex = (ui->mUserFocus == NS_STYLE_USER_FOCUS_IGNORE ||
                        ui->mUserFocus == NS_STYLE_USER_FOCUS_NONE) ? -1 : 0;
    return aContent->IsFocusable(&tabIndex, aFlags & FLAG_BYMOUSE) ? aContent : nsnull;
  }
  
  return frame->IsFocusable(nsnull, aFlags & FLAG_BYMOUSE) ? aContent : nsnull;
}

bool
nsFocusManager::Blur(nsPIDOMWindow* aWindowToClear,
                     nsPIDOMWindow* aAncestorWindowToFocus,
                     bool aIsLeavingDocument,
                     bool aAdjustWidgets)
{
  
  nsCOMPtr<nsIContent> content = mFocusedContent;
  if (content) {
    if (!content->IsInDoc()) {
      mFocusedContent = nsnull;
      return true;
    }
    if (content == mFirstBlurEvent)
      return true;
  }

  
  nsCOMPtr<nsPIDOMWindow> window = mFocusedWindow;
  if (!window) {
    mFocusedContent = nsnull;
    return true;
  }

  nsCOMPtr<nsIDocShell> docShell = window->GetDocShell();
  if (!docShell) {
    mFocusedContent = nsnull;
    return true;
  }

  
  
  nsCOMPtr<nsIPresShell> presShell;
  docShell->GetPresShell(getter_AddRefs(presShell));
  if (!presShell) {
    mFocusedContent = nsnull;
    return true;
  }

  bool clearFirstBlurEvent = false;
  if (!mFirstBlurEvent) {
    mFirstBlurEvent = content;
    clearFirstBlurEvent = true;
  }

  
  
  
  nsIMEStateManager::OnTextStateBlur(nsnull, nsnull);
  if (mActiveWindow)
    nsIMEStateManager::OnChangeFocus(presShell->GetPresContext(), nsnull, IMEContext::FOCUS_REMOVED);

  
  
  mFocusedContent = nsnull;
  bool shouldShowFocusRing = window->ShouldShowFocusRing();
  if (aWindowToClear)
    aWindowToClear->SetFocusedNode(nsnull);

#ifdef DEBUG_FOCUS
  PRINTTAGF("**Element %s has been blurred\n", content);
#endif

  
  bool sendBlurEvent =
    content && content->IsInDoc() && !IsNonFocusableRoot(content);
  if (content) {
    if (sendBlurEvent) {
      NotifyFocusStateChange(content, shouldShowFocusRing, false);
    }

    
    
    
    
    if (mActiveWindow) {
      nsIFrame* contentFrame = content->GetPrimaryFrame();
      nsIObjectFrame* objectFrame = do_QueryFrame(contentFrame);
      if (aAdjustWidgets && objectFrame) {
        
        
        nsIViewManager* vm = presShell->GetViewManager();
        if (vm) {
          nsCOMPtr<nsIWidget> widget;
          vm->GetRootWidget(getter_AddRefs(widget));
          if (widget)
            widget->SetFocus(false);
        }
      }

      
      TabParent* remote = GetRemoteForContent(content);
      if (remote) {
        remote->Deactivate();
  #ifdef DEBUG_FOCUS
      printf("*Remote browser deactivated\n");
  #endif
      }
    }
  }

  bool result = true;
  if (sendBlurEvent) {
    
    
    
    if (mActiveWindow)
      window->UpdateCommands(NS_LITERAL_STRING("focus"));

    SendFocusOrBlurEvent(NS_BLUR_CONTENT, presShell,
                         content->GetCurrentDoc(), content, 1, false);
  }

  
  
  if (aIsLeavingDocument || !mActiveWindow)
    SetCaretVisible(presShell, false, nsnull);

  
  
  
  
  
  
  if (mFocusedWindow != window ||
      (mFocusedContent != nsnull && !aIsLeavingDocument)) {
    result = false;
  }
  else if (aIsLeavingDocument) {
    window->TakeFocus(false, 0);

    
    
    
    if (aAncestorWindowToFocus)
      aAncestorWindowToFocus->SetFocusedNode(nsnull, 0, true);

    mFocusedWindow = nsnull;
    mFocusedContent = nsnull;

    
    
    
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(window->GetExtantDocument());
    if (doc)
      SendFocusOrBlurEvent(NS_BLUR_CONTENT, presShell, doc, doc, 1, false);
    if (mFocusedWindow == nsnull)
      SendFocusOrBlurEvent(NS_BLUR_CONTENT, presShell, doc, window, 1, false);

    
    result = (mFocusedWindow == nsnull && mActiveWindow);
  }
  else if (mActiveWindow) {
    
    
    
    
    
    UpdateCaret(false, true, nsnull);
  }

  if (clearFirstBlurEvent)
    mFirstBlurEvent = nsnull;

  return result;
}

void
nsFocusManager::Focus(nsPIDOMWindow* aWindow,
                      nsIContent* aContent,
                      PRUint32 aFlags,
                      bool aIsNewDocument,
                      bool aFocusChanged,
                      bool aWindowRaised,
                      bool aAdjustWidgets)
{
  if (!aWindow)
    return;

  if (aContent && (aContent == mFirstFocusEvent || aContent == mFirstBlurEvent))
    return;

  
  
  nsCOMPtr<nsIDocShell> docShell = aWindow->GetDocShell();
  if (!docShell)
    return;

  nsCOMPtr<nsIPresShell> presShell;
  docShell->GetPresShell(getter_AddRefs(presShell));
  if (!presShell)
    return;

  
  
  
  PRUint32 focusMethod = aFocusChanged ? aFlags & FOCUSMETHODANDRING_MASK :
                         aWindow->GetFocusMethod() | (aFlags & FLAG_SHOWRING);

  if (!IsWindowVisible(aWindow)) {
    
    
    if (CheckIfFocusable(aContent, aFlags)) {
      aWindow->SetFocusedNode(aContent, focusMethod);
      if (aFocusChanged)
        ScrollIntoView(presShell, aContent, aFlags);
    }
    return;
  }

  bool clearFirstFocusEvent = false;
  if (!mFirstFocusEvent) {
    mFirstFocusEvent = aContent;
    clearFirstFocusEvent = true;
  }

#ifdef DEBUG_FOCUS
  PRINTTAGF("**Element %s has been focused", aContent);
  nsCOMPtr<nsIDocument> docm = do_QueryInterface(aWindow->GetExtantDocument());
  if (docm)
    PRINTTAGF(" from %s", docm->GetRootElement());
  printf(" [Newdoc: %d FocusChanged: %d Raised: %d Flags: %x]\n",
         aIsNewDocument, aFocusChanged, aWindowRaised, aFlags);
#endif

  if (aIsNewDocument) {
    
    
    
    AdjustWindowFocus(aWindow, false);

    
    
    aWindow->UpdateTouchState();
  }

  
  if (aWindow->TakeFocus(true, focusMethod))
    aIsNewDocument = true;

  mFocusedWindow = aWindow;

  
  
  
  nsCOMPtr<nsIWidget> objectFrameWidget;
  if (aContent) {
    nsIFrame* contentFrame = aContent->GetPrimaryFrame();
    nsIObjectFrame* objectFrame = do_QueryFrame(contentFrame);
    if (objectFrame)
      objectFrameWidget = objectFrame->GetWidget();
  }
  if (aAdjustWidgets && !objectFrameWidget) {
    nsIViewManager* vm = presShell->GetViewManager();
    if (vm) {
      nsCOMPtr<nsIWidget> widget;
      vm->GetRootWidget(getter_AddRefs(widget));
      if (widget)
        widget->SetFocus(false);
    }
  }

  
  
  if (aIsNewDocument) {
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(aWindow->GetExtantDocument());
    if (doc)
      SendFocusOrBlurEvent(NS_FOCUS_CONTENT, presShell, doc,
                           doc, aFlags & FOCUSMETHOD_MASK, aWindowRaised);
    if (mFocusedWindow == aWindow && mFocusedContent == nsnull)
      SendFocusOrBlurEvent(NS_FOCUS_CONTENT, presShell, doc,
                           aWindow, aFlags & FOCUSMETHOD_MASK, aWindowRaised);
  }

  
  
  if (CheckIfFocusable(aContent, aFlags) &&
      mFocusedWindow == aWindow && mFocusedContent == nsnull) {
    mFocusedContent = aContent;

    nsIContent* focusedNode = aWindow->GetFocusedNode();
    bool isRefocus = focusedNode && focusedNode->IsEqualTo(aContent);

    aWindow->SetFocusedNode(aContent, focusMethod);

    bool sendFocusEvent =
      aContent && aContent->IsInDoc() && !IsNonFocusableRoot(aContent);
    nsPresContext* presContext = presShell->GetPresContext();
    if (sendFocusEvent) {
      
      if (aFocusChanged)
        ScrollIntoView(presShell, aContent, aFlags);

      NotifyFocusStateChange(aContent, aWindow->ShouldShowFocusRing(), true);

      
      
      
      if (presShell->GetDocument() == aContent->GetDocument()) {
        if (aAdjustWidgets && objectFrameWidget)
          objectFrameWidget->SetFocus(false);

        
        TabParent* remote = GetRemoteForContent(aContent);
        if (remote) {
          remote->Activate();
#ifdef DEBUG_FOCUS
          printf("*Remote browser activated\n");
#endif
        }
      }

      PRUint32 reason = GetFocusMoveReason(aFlags);
      nsIMEStateManager::OnChangeFocus(presContext, aContent, reason);

      
      
      
      if (!aWindowRaised)
        aWindow->UpdateCommands(NS_LITERAL_STRING("focus"));

      SendFocusOrBlurEvent(NS_FOCUS_CONTENT, presShell,
                           aContent->GetCurrentDoc(),
                           aContent, aFlags & FOCUSMETHOD_MASK,
                           aWindowRaised, isRefocus);

      nsIMEStateManager::OnTextStateFocus(presContext, aContent);
    } else {
      nsIMEStateManager::OnTextStateBlur(presContext, nsnull);
      nsIMEStateManager::OnChangeFocus(presContext, nsnull, IMEContext::FOCUS_REMOVED);
      if (!aWindowRaised) {
        aWindow->UpdateCommands(NS_LITERAL_STRING("focus"));
      }
    }
  }
  else {
    
    
    
    if (aAdjustWidgets && objectFrameWidget &&
        mFocusedWindow == aWindow && mFocusedContent == nsnull) {
      nsIViewManager* vm = presShell->GetViewManager();
      if (vm) {
        nsCOMPtr<nsIWidget> widget;
        vm->GetRootWidget(getter_AddRefs(widget));
        if (widget)
          widget->SetFocus(false);
      }
    }

    nsPresContext* presContext = presShell->GetPresContext();
    nsIMEStateManager::OnTextStateBlur(presContext, nsnull);
    nsIMEStateManager::OnChangeFocus(presContext, nsnull, IMEContext::FOCUS_REMOVED);

    if (!aWindowRaised)
      aWindow->UpdateCommands(NS_LITERAL_STRING("focus"));
  }

  
  
  
  
  
  
  if (mFocusedContent == aContent)
    UpdateCaret(aFocusChanged && !(aFlags & FLAG_BYMOUSE), aIsNewDocument,
                mFocusedContent);

  if (clearFirstFocusEvent)
    mFirstFocusEvent = nsnull;
}

class FocusBlurEvent : public nsRunnable
{
public:
  FocusBlurEvent(nsISupports* aTarget, PRUint32 aType,
                 nsPresContext* aContext, bool aWindowRaised,
                 bool aIsRefocus)
  : mTarget(aTarget), mType(aType), mContext(aContext),
    mWindowRaised(aWindowRaised), mIsRefocus(aIsRefocus) {}

  NS_IMETHOD Run()
  {
    nsFocusEvent event(true, mType);
    event.flags |= NS_EVENT_FLAG_CANT_BUBBLE;
    event.fromRaise = mWindowRaised;
    event.isRefocus = mIsRefocus;
    return nsEventDispatcher::Dispatch(mTarget, mContext, &event);
  }

  nsCOMPtr<nsISupports>   mTarget;
  PRUint32                mType;
  nsRefPtr<nsPresContext> mContext;
  bool                    mWindowRaised;
  bool                    mIsRefocus;
};

void
nsFocusManager::SendFocusOrBlurEvent(PRUint32 aType,
                                     nsIPresShell* aPresShell,
                                     nsIDocument* aDocument,
                                     nsISupports* aTarget,
                                     PRUint32 aFocusMethod,
                                     bool aWindowRaised,
                                     bool aIsRefocus)
{
  NS_ASSERTION(aType == NS_FOCUS_CONTENT || aType == NS_BLUR_CONTENT,
               "Wrong event type for SendFocusOrBlurEvent");

  nsCOMPtr<nsIDOMEventTarget> eventTarget = do_QueryInterface(aTarget);

  
  
  
  if (aFocusMethod && aDocument && aDocument->EventHandlingSuppressed()) {
    
    
    NS_ASSERTION(!aWindowRaised, "aWindowRaised should not be set");

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

#ifdef ACCESSIBILITY
  nsAccessibilityService* accService = GetAccService();
  if (accService) {
    if (aType == NS_FOCUS_CONTENT)
      accService->NotifyOfDOMFocus(aTarget);
    else
      accService->NotifyOfDOMBlur(aTarget);
  }
#endif

  nsContentUtils::AddScriptRunner(
    new FocusBlurEvent(aTarget, aType, aPresShell->GetPresContext(),
                       aWindowRaised, aIsRefocus));
}

void
nsFocusManager::ScrollIntoView(nsIPresShell* aPresShell,
                               nsIContent* aContent,
                               PRUint32 aFlags)
{
  
  if (!(aFlags & FLAG_NOSCROLL))
    aPresShell->ScrollContentIntoView(aContent,
                                      NS_PRESSHELL_SCROLL_IF_NOT_VISIBLE,
                                      NS_PRESSHELL_SCROLL_IF_NOT_VISIBLE,
                                      nsIPresShell::SCROLL_OVERFLOW_HIDDEN);
}


void
nsFocusManager::RaiseWindow(nsPIDOMWindow* aWindow)
{
  
  
  if (!aWindow || aWindow == mActiveWindow || aWindow == mWindowBeingLowered)
    return;

#if defined(XP_WIN) || defined(XP_OS2)
  
  
  
  
  
  nsCOMPtr<nsPIDOMWindow> childWindow;
  GetFocusedDescendant(aWindow, true, getter_AddRefs(childWindow));
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
      widget->SetFocus(true);
  }
#else
  nsCOMPtr<nsIWebNavigation> webnav = do_GetInterface(aWindow);
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin = do_QueryInterface(webnav);
  if (treeOwnerAsWin) {
    nsCOMPtr<nsIWidget> widget;
    treeOwnerAsWin->GetMainWidget(getter_AddRefs(widget));
    if (widget)
      widget->SetFocus(true);
  }
#endif
}

void
nsFocusManager::UpdateCaret(bool aMoveCaretToFocus,
                            bool aUpdateVisibility,
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

  bool browseWithCaret =
    Preferences::GetBool("accessibility.browsewithcaret");

  nsCOMPtr<nsIPresShell> presShell;
  focusedDocShell->GetPresShell(getter_AddRefs(presShell));
  if (!presShell)
    return;

  
  
  
  bool isEditable = false;
  nsCOMPtr<nsIEditorDocShell> editorDocShell(do_QueryInterface(dsti));
  if (editorDocShell) {
    editorDocShell->GetEditable(&isEditable);

    if (isEditable) {
      nsCOMPtr<nsIHTMLDocument> doc =
        do_QueryInterface(presShell->GetDocument());

      bool isContentEditableDoc =
        doc && doc->GetEditingState() == nsIHTMLDocument::eContentEditable;

      bool isFocusEditable =
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
  
  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(aPresShell->GetDocument());
  if (domDoc) {
    nsRefPtr<nsFrameSelection> frameSelection = aPresShell->FrameSelection();
    nsCOMPtr<nsISelection> domSelection = frameSelection->
      GetSelection(nsISelectionController::SELECTION_NORMAL);
    if (domSelection) {
      nsCOMPtr<nsIDOMNode> currentFocusNode(do_QueryInterface(aContent));
      
      
      domSelection->RemoveAllRanges();
      if (currentFocusNode) {
        nsCOMPtr<nsIDOMRange> newRange;
        nsresult rv = domDoc->CreateRange(getter_AddRefs(newRange));
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
                                bool aVisible,
                                nsIContent* aContent)
{
  
  
  
  nsRefPtr<nsCaret> caret = aPresShell->GetCaret();
  if (!caret)
    return NS_OK;

  bool caretVisible = false;
  caret->GetCaretVisible(&caretVisible);
  if (!aVisible && !caretVisible)
    return NS_OK;

  nsRefPtr<nsFrameSelection> frameSelection;
  if (aContent) {
    NS_ASSERTION(aContent->GetDocument() == aPresShell->GetDocument(),
                 "Wrong document?");
    nsIFrame *focusFrame = aContent->GetPrimaryFrame();
    if (focusFrame)
      frameSelection = focusFrame->GetFrameSelection();
  }

  nsRefPtr<nsFrameSelection> docFrameSelection = aPresShell->FrameSelection();

  if (docFrameSelection && caret &&
     (frameSelection == docFrameSelection || !aContent)) {
    nsISelection* domSelection = docFrameSelection->
      GetSelection(nsISelectionController::SELECTION_NORMAL);
    if (domSelection) {
      
      caret->SetCaretVisible(false);

      
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

  nsRefPtr<nsFrameSelection> frameSelection = aPresShell->FrameSelection();

  nsCOMPtr<nsISelection> domSelection;
  if (frameSelection) {
    domSelection = frameSelection->
      GetSelection(nsISelectionController::SELECTION_NORMAL);
  }

  nsCOMPtr<nsIDOMNode> startNode, endNode;
  bool isCollapsed = false;
  nsCOMPtr<nsIContent> startContent, endContent;
  PRInt32 startOffset = 0;
  if (domSelection) {
    domSelection->GetIsCollapsed(&isCollapsed);
    nsCOMPtr<nsIDOMRange> domRange;
    rv = domSelection->GetRangeAt(0, getter_AddRefs(domRange));
    if (domRange) {
      domRange->GetStartContainer(getter_AddRefs(startNode));
      domRange->GetEndContainer(getter_AddRefs(endNode));
      domRange->GetStartOffset(&startOffset);

      nsIContent *childContent = nsnull;

      startContent = do_QueryInterface(startNode);
      if (startContent && startContent->IsElement()) {
        NS_ASSERTION(startOffset >= 0, "Start offset cannot be negative");  
        childContent = startContent->GetChildAt(startOffset);
        if (childContent) {
          startContent = childContent;
        }
      }

      endContent = do_QueryInterface(endNode);
      if (endContent && endContent->IsElement()) {
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
    startFrame = startContent->GetPrimaryFrame();
    if (isCollapsed) {
      
      
      
      

      if (startContent->NodeType() == nsIDOMNode::TEXT_NODE) {
        nsAutoString nodeValue;
        startContent->AppendTextTo(nodeValue);

        bool isFormControl =
          startContent->IsNodeOfType(nsINode::eHTML_FORM_CONTROL);

        if (nodeValue.Length() == (PRUint32)startOffset && !isFormControl &&
            startContent != aDocument->GetRootElement()) {
          
          nsCOMPtr<nsIFrameEnumerator> frameTraversal;
          nsresult rv = NS_NewFrameTraversal(getter_AddRefs(frameTraversal),
                                             presContext, startFrame,
                                             eLeaf,
                                             false, 
                                             false, 
                                             true      
                                             );
          NS_ENSURE_SUCCESS(rv, rv);

          nsIFrame *newCaretFrame = nsnull;
          nsCOMPtr<nsIContent> newCaretContent = startContent;
          bool endOfSelectionInStartNode(startContent == endContent);
          do {
            
            
            frameTraversal->Next();
            newCaretFrame = static_cast<nsIFrame*>(frameTraversal->CurrentItem());
            if (nsnull == newCaretFrame)
              break;
            newCaretContent = newCaretFrame->GetContent();            
          } while (!newCaretContent || newCaretContent == startContent);

          if (newCaretFrame && newCaretContent) {
            
            
            nsRefPtr<nsCaret> caret = aPresShell->GetCaret();
            nsRect caretRect;
            nsIFrame *frame = caret->GetGeometry(domSelection, &caretRect);
            if (frame) {
              nsPoint caretWidgetOffset;
              nsIWidget *widget = frame->GetNearestWidget(caretWidgetOffset);
              caretRect.MoveBy(caretWidgetOffset);
              nsPoint newCaretOffset;
              nsIWidget *newCaretWidget = newCaretFrame->GetNearestWidget(newCaretOffset);
              if (widget == newCaretWidget && caretRect.y == newCaretOffset.y &&
                  caretRect.x == newCaretOffset.x) {
                
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

  LookAndFeel::GetInt(LookAndFeel::eIntID_TabFocusModel,
                      &nsIContent::sTabFocusModel);

  if (aType == MOVEFOCUS_ROOT) {
    NS_IF_ADDREF(*aNextContent = GetRootForFocus(aWindow, doc, false, false));
    return NS_OK;
  }
  if (aType == MOVEFOCUS_FORWARDDOC) {
    NS_IF_ADDREF(*aNextContent = GetNextTabbableDocument(true));
    return NS_OK;
  }
  if (aType == MOVEFOCUS_BACKWARDDOC) {
    NS_IF_ADDREF(*aNextContent = GetNextTabbableDocument(false));
    return NS_OK;
  }
  
  nsIContent* rootContent = doc->GetRootElement();
  NS_ENSURE_TRUE(rootContent, NS_OK);

  nsIPresShell *presShell = doc->GetShell();
  NS_ENSURE_TRUE(presShell, NS_OK);

  if (aType == MOVEFOCUS_FIRST) {
    if (!aStartContent)
      startContent = rootContent;
    return GetNextTabbableContent(presShell, startContent,
                                  nsnull, startContent,
                                  true, 1, false, aNextContent);
  }
  if (aType == MOVEFOCUS_LAST) {
    if (!aStartContent)
      startContent = rootContent;
    return GetNextTabbableContent(presShell, startContent,
                                  nsnull, startContent,
                                  false, 0, false, aNextContent);
  }

  bool forward = (aType == MOVEFOCUS_FORWARD || aType == MOVEFOCUS_CARET);
  bool doNavigation = true;
  bool ignoreTabIndex = false;
  
  
  
  nsIFrame* popupFrame = nsnull;

  PRInt32 tabIndex = forward ? 1 : 0;
  if (startContent) {
    nsIFrame* frame = startContent->GetPrimaryFrame();
    if (startContent->Tag() == nsGkAtoms::area &&
        startContent->IsHTML())
      startContent->IsFocusable(&tabIndex);
    else if (frame)
      frame->IsFocusable(&tabIndex, 0);
    else
      startContent->IsFocusable(&tabIndex);

    
    
    
    if (tabIndex < 0) {
      tabIndex = 1;
      if (startContent != rootContent)
        ignoreTabIndex = true;
    }

    
    
    
    if (frame) {
      popupFrame = nsLayoutUtils::GetClosestFrameOfType(frame,
                                                        nsGkAtoms::menuPopupFrame);
    }

    if (popupFrame) {
      
      
      rootContent = popupFrame->GetContent();
      NS_ASSERTION(rootContent, "Popup frame doesn't have a content node");
    }
    else if (!forward) {
      
      
      
      if (startContent == rootContent) {
        doNavigation = false;
      } else {
        nsIDocument* doc = startContent->GetCurrentDoc();
        if (startContent ==
              nsLayoutUtils::GetEditableRootContentByContentEditable(doc)) {
          doNavigation = false;
        }
      }
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

        if (aType == MOVEFOCUS_CARET) {
          
          
          
          if (startContent) {
            GetFocusInSelection(aWindow, startContent,
                                endSelectionContent, aNextContent);
          }
          return NS_OK;
        }

        if (startContent) {
          
          
          
          ignoreTabIndex = true;
        }
      }

      if (!startContent) {
        
        startContent = rootContent;
        NS_ENSURE_TRUE(startContent, NS_OK);
      }
    }
  }

  NS_ASSERTION(startContent, "starting content not set");

  
  
  
  
  
  
  
  
  
  
  
  bool skipOriginalContentCheck = true;
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

    doNavigation = true;
    skipOriginalContentCheck = false;
    ignoreTabIndex = false;

    
    
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

      presShell = doc->GetShell();

      rootContent = doc->GetRootElement();
      startContent = do_QueryInterface(piWindow->GetFrameElementInternal());
      if (startContent) {
        nsIFrame* frame = startContent->GetPrimaryFrame();
        if (!frame)
          return NS_OK;

        frame->IsFocusable(&tabIndex, 0);
        if (tabIndex < 0) {
          tabIndex = 1;
          ignoreTabIndex = true;
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
      
      
      bool tookFocus;
      docShell->TabToTreeOwner(forward, &tookFocus);
      
      if (tookFocus) {
        nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(docShell);
        if (window->GetFocusedNode() == mFocusedContent)
          Blur(mFocusedWindow, nsnull, true, true);
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
                                       bool aForward,
                                       PRInt32 aCurrentTabIndex,
                                       bool aIgnoreTabIndex,
                                       nsIContent** aResultContent)
{
  *aResultContent = nsnull;

  nsCOMPtr<nsIContent> startContent = aStartContent;
  if (!startContent)
    return NS_OK;

#ifdef DEBUG_FOCUS_NAVIGATION
  PRINTTAGF("GetNextTabbable: %s", aStartContent);
  printf(" tabindex: %d\n", aCurrentTabIndex);
#endif

  nsPresContext* presContext = aPresShell->GetPresContext();

  bool getNextFrame = true;
  nsCOMPtr<nsIContent> iterStartContent = aStartContent;
  while (1) {
    nsIFrame* startFrame = iterStartContent->GetPrimaryFrame();
    
    if (!startFrame) {
      
      if (iterStartContent == aRootContent)
        return NS_OK;

      
      iterStartContent = aForward ? iterStartContent->GetNextNode() : iterStartContent->GetPreviousContent();
      
      
      getNextFrame = false;
      if (iterStartContent)
        continue;

      
      iterStartContent = aRootContent;
      continue;
    }

    nsCOMPtr<nsIFrameEnumerator> frameTraversal;
    nsresult rv = NS_NewFrameTraversal(getter_AddRefs(frameTraversal),
                                       presContext, startFrame,
                                       ePreOrder,
                                       false, 
                                       false, 
                                       true      
                                       );
    NS_ENSURE_SUCCESS(rv, rv);

    if (iterStartContent == aRootContent) {
      if (!aForward) {
        frameTraversal->Last();
      } else if (aRootContent->IsFocusable()) {
        frameTraversal->Next();
      }
    }
    else if (getNextFrame &&
             (!iterStartContent || iterStartContent->Tag() != nsGkAtoms::area ||
              !iterStartContent->IsHTML())) {
      
      
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
                                   currentContent, iterStartContent);
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
                  
                  
                  
                  
                  *aResultContent =
                    nsLayoutUtils::GetEditableRootContentByContentEditable(subdoc);
                  if (!*aResultContent ||
                      !((*aResultContent)->GetPrimaryFrame())) {
                    *aResultContent =
                      GetRootForFocus(subframe, subdoc, false, true);
                  }
                  if (*aResultContent) {
                    NS_ADDREF(*aResultContent);
                    return NS_OK;
                  }
                }
              }
              Element* rootElement = subdoc->GetRootElement();
              nsIPresShell* subShell = subdoc->GetShell();
              if (rootElement && subShell) {
                rv = GetNextTabbableContent(subShell, rootElement,
                                            aOriginalStartContent, rootElement,
                                            aForward, (aForward ? 1 : 0),
                                            false, aResultContent);
                NS_ENSURE_SUCCESS(rv, rv);
                if (*aResultContent)
                  return NS_OK;
              }
            }
          }
          
          
          
          
          
          
          
          
          
          
          
          else if (currentContent == aRootContent ||
                   (currentContent != startContent &&
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
                     GetRootForFocus(window, aRootContent->GetCurrentDoc(), false, true));
      }
      break;
    }

    
    aCurrentTabIndex = GetNextTabIndex(aRootContent, aCurrentTabIndex, aForward);
    startContent = iterStartContent = aRootContent;
  }

  return NS_OK;
}

nsIContent*
nsFocusManager::GetNextTabbableMapArea(bool aForward,
                                       PRInt32 aCurrentTabIndex,
                                       nsIContent* aImageContent,
                                       nsIContent* aStartContent)
{
  nsAutoString useMap;
  aImageContent->GetAttr(kNameSpaceID_None, nsGkAtoms::usemap, useMap);

  nsCOMPtr<nsIDocument> doc = aImageContent->GetDocument();
  if (doc) {
    nsCOMPtr<nsIContent> mapContent = doc->FindImageMap(useMap);
    if (!mapContent)
      return nsnull;
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
                                bool aForward)
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
                                bool aIsForDocNavigation,
                                bool aCheckVisibility)
{
  
  
  if (aIsForDocNavigation) {
    nsCOMPtr<nsIContent> docContent =
      do_QueryInterface(aWindow->GetFrameElementInternal());
    
    if (docContent) {
      if (docContent->Tag() == nsGkAtoms::iframe)
        return nsnull;

      nsIFrame* frame = docContent->GetPrimaryFrame();
      if (!frame || !frame->IsFocusable(nsnull, 0))
        return nsnull;
    }
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

  Element *rootElement = aDocument->GetRootElement();
  if (rootElement) {
    if (aCheckVisibility && !rootElement->GetPrimaryFrame()) {
      return nsnull;
    }

    
    nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(aDocument);
    if (htmlDoc) {
      PRUint32 childCount = rootElement->GetChildCount();
      for (PRUint32 i = 0; i < childCount; ++i) {
        nsIContent *childContent = rootElement->GetChildAt(i);
        nsINodeInfo *ni = childContent->NodeInfo();
        if (childContent->IsHTML() &&
            ni->Equals(nsGkAtoms::frameset))
          return nsnull;
      }
    }
  }

  return rootElement;
}

TabParent*
nsFocusManager::GetRemoteForContent(nsIContent* aContent) {
  if (!aContent ||
      (aContent->Tag() != nsGkAtoms::browser &&
       aContent->Tag() != nsGkAtoms::iframe) ||
      !aContent->IsXUL() ||
      !aContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::Remote,
                             nsGkAtoms::_true, eIgnoreCase))
    return nsnull;

  nsCOMPtr<nsIFrameLoaderOwner> loaderOwner = do_QueryInterface(aContent);
  if (!loaderOwner)
    return nsnull;

  nsRefPtr<nsFrameLoader> frameLoader = loaderOwner->GetFrameLoader();
  if (!frameLoader)
    return nsnull;

  PBrowserParent* remoteBrowser = frameLoader->GetRemoteBrowser();
  TabParent* remote = static_cast<TabParent*>(remoteBrowser);
  return remote;
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
nsFocusManager::GetNextTabbableDocument(bool aForward)
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
      content = GetRootForFocus(nextFrame, doc, true, true);
      if (content && !GetRootForFocus(nextFrame, doc, false, false)) {
        
        
        
        nsCOMPtr<nsIContent> nextFocus;
        Element* rootElement = doc->GetRootElement();
        nsIPresShell* presShell = doc->GetShell();
        if (presShell) {
          nsresult rv = GetNextTabbableContent(presShell, rootElement,
                                               nsnull, rootElement,
                                               true, 1, false,
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
    } while (true);
  }
  while (selectionNode && selectionNode != endSelectionNode);
}

nsresult
NS_NewFocusManager(nsIFocusManager** aResult)
{
  NS_IF_ADDREF(*aResult = nsFocusManager::GetFocusManager());
  return NS_OK;
}
