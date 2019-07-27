




#include "mozilla/dom/TabParent.h"

#include "nsFocusManager.h"

#include "nsIInterfaceRequestorUtils.h"
#include "nsGkAtoms.h"
#include "nsContentUtils.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"
#include "nsIEditor.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIDOMRange.h"
#include "nsIHTMLDocument.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeOwner.h"
#include "nsLayoutUtils.h"
#include "nsIPresShell.h"
#include "nsFrameTraversal.h"
#include "nsIWebNavigation.h"
#include "nsCaret.h"
#include "nsIBaseWindow.h"
#include "nsIXULWindow.h"
#include "nsViewManager.h"
#include "nsFrameSelection.h"
#include "mozilla/dom/Selection.h"
#include "nsXULPopupManager.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIPrincipal.h"
#include "nsIObserverService.h"
#include "nsIObjectFrame.h"
#include "nsBindingManager.h"
#include "nsStyleCoord.h"
#include "SelectionCarets.h"
#include "TabChild.h"

#include "mozilla/ContentEvents.h"
#include "mozilla/dom/Element.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/EventStates.h"
#include "mozilla/IMEStateManager.h"
#include "mozilla/LookAndFeel.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/unused.h"
#include <algorithm>

#ifdef MOZ_XUL
#include "nsIDOMXULTextboxElement.h"
#include "nsIDOMXULMenuListElement.h"
#endif

#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif

#ifndef XP_MACOSX
#include "nsIScriptError.h"
#endif

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::widget;

#ifdef PR_LOGGING




PRLogModuleInfo* gFocusLog;
PRLogModuleInfo* gFocusNavigationLog;

#define LOGFOCUS(args) PR_LOG(gFocusLog, 4, args)
#define LOGFOCUSNAVIGATION(args) PR_LOG(gFocusNavigationLog, 4, args)

#define LOGTAG(log, format, content)                            \
  {                                                             \
    nsAutoCString tag(NS_LITERAL_CSTRING("(none)"));            \
    if (content) {                                              \
      content->NodeInfo()->NameAtom()->ToUTF8String(tag);       \
    }                                                           \
    PR_LOG(log, 4, (format, tag.get()));                        \
  }

#define LOGCONTENT(format, content) LOGTAG(gFocusLog, format, content)
#define LOGCONTENTNAVIGATION(format, content) LOGTAG(gFocusNavigationLog, format, content)

#else

#define LOGFOCUS(args)
#define LOGFOCUSNAVIGATION(args)
#define LOGCONTENT(format, content)
#define LOGCONTENTNAVIGATION(format, content)

#endif

struct nsDelayedBlurOrFocusEvent
{
  nsDelayedBlurOrFocusEvent(uint32_t aType,
                            nsIPresShell* aPresShell,
                            nsIDocument* aDocument,
                            EventTarget* aTarget)
   : mType(aType),
     mPresShell(aPresShell),
     mDocument(aDocument),
     mTarget(aTarget) { }

  nsDelayedBlurOrFocusEvent(const nsDelayedBlurOrFocusEvent& aOther)
   : mType(aOther.mType),
     mPresShell(aOther.mPresShell),
     mDocument(aOther.mDocument),
     mTarget(aOther.mTarget) { }

  uint32_t mType;
  nsCOMPtr<nsIPresShell> mPresShell;
  nsCOMPtr<nsIDocument> mDocument;
  nsCOMPtr<EventTarget> mTarget;
};

inline void ImplCycleCollectionUnlink(nsDelayedBlurOrFocusEvent& aField)
{
  aField.mPresShell = nullptr;
  aField.mDocument = nullptr;
  aField.mTarget = nullptr;
}

inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            nsDelayedBlurOrFocusEvent& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  CycleCollectionNoteChild(aCallback, aField.mPresShell.get(), aName, aFlags);
  CycleCollectionNoteChild(aCallback, aField.mDocument.get(), aName, aFlags);
  CycleCollectionNoteChild(aCallback, aField.mTarget.get(), aName, aFlags);
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsFocusManager)
  NS_INTERFACE_MAP_ENTRY(nsIFocusManager)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIFocusManager)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsFocusManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsFocusManager)

NS_IMPL_CYCLE_COLLECTION(nsFocusManager,
                         mActiveWindow,
                         mFocusedWindow,
                         mFocusedContent,
                         mFirstBlurEvent,
                         mFirstFocusEvent,
                         mWindowBeingLowered,
                         mDelayedBlurFocusEvents,
                         mMouseButtonEventHandlingDocument)

nsFocusManager* nsFocusManager::sInstance = nullptr;
bool nsFocusManager::sMouseFocusesFormControl = false;
bool nsFocusManager::sTestMode = false;

static const char* kObservedPrefs[] = {
  "accessibility.browsewithcaret",
  "accessibility.tabfocus_applies_to_xul",
  "accessibility.mouse_focuses_formcontrol",
  "focusmanager.testmode",
  nullptr
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

#ifdef PR_LOGGING
  gFocusLog = PR_NewLogModule("Focus");
  gFocusNavigationLog = PR_NewLogModule("FocusNavigation");
#endif

  nsIContent::sTabFocusModelAppliesToXUL =
    Preferences::GetBool("accessibility.tabfocus_applies_to_xul",
                         nsIContent::sTabFocusModelAppliesToXUL);

  sMouseFocusesFormControl =
    Preferences::GetBool("accessibility.mouse_focuses_formcontrol", false);

  sTestMode = Preferences::GetBool("focusmanager.testmode", false);

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
                        const char16_t *aData)
{
  if (!nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    nsDependentString data(aData);
    if (data.EqualsLiteral("accessibility.browsewithcaret")) {
      UpdateCaretForCaretBrowsingMode();
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
    else if (data.EqualsLiteral("focusmanager.testmode")) {
      sTestMode = Preferences::GetBool("focusmanager.testmode", false);
    }
  } else if (!nsCRT::strcmp(aTopic, "xpcom-shutdown")) {
    mActiveWindow = nullptr;
    mFocusedWindow = nullptr;
    mFocusedContent = nullptr;
    mFirstBlurEvent = nullptr;
    mFirstFocusEvent = nullptr;
    mWindowBeingLowered = nullptr;
    mDelayedBlurFocusEvents.Clear();
    mMouseButtonEventHandlingDocument = nullptr;
  }

  return NS_OK;
}


static nsPIDOMWindow*
GetContentWindow(nsIContent* aContent)
{
  nsIDocument* doc = aContent->GetComposedDoc();
  if (doc) {
    nsIDocument* subdoc = doc->GetSubDocumentFor(aContent);
    if (subdoc)
      return subdoc->GetWindow();
  }

  return nullptr;
}


static nsPIDOMWindow*
GetCurrentWindow(nsIContent* aContent)
{
  nsIDocument* doc = aContent->GetComposedDoc();
  return doc ? doc->GetWindow() : nullptr;
}


nsIContent*
nsFocusManager::GetFocusedDescendant(nsPIDOMWindow* aWindow, bool aDeep,
                                     nsPIDOMWindow** aFocusedWindow)
{
  NS_ENSURE_TRUE(aWindow, nullptr);

  *aFocusedWindow = nullptr;

  nsIContent* currentContent = nullptr;
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
  if (aContent->IsXULElement()) {
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
      else if (aContent->IsXULElement(nsGkAtoms::scale)) {
        nsCOMPtr<nsIDocument> doc = aContent->GetComposedDoc();
        if (!doc)
          return nullptr;

        nsINodeList* children = doc->BindingManager()->GetAnonymousNodesFor(aContent);
        if (children) {
          nsIContent* child = children->Item(0);
          if (child && child->IsXULElement(nsGkAtoms::slider))
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

  return nullptr;
}


InputContextAction::Cause
nsFocusManager::GetFocusMoveActionCause(uint32_t aFlags)
{
  if (aFlags & nsIFocusManager::FLAG_BYMOUSE) {
    return InputContextAction::CAUSE_MOUSE;
  } else if (aFlags & nsIFocusManager::FLAG_BYKEY) {
    return InputContextAction::CAUSE_KEY;
  }
  return InputContextAction::CAUSE_UNKNOWN;
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
  LOGFOCUS(("<<SetFocusedWindow begin>>"));

  nsCOMPtr<nsPIDOMWindow> windowToFocus(do_QueryInterface(aWindowToFocus));
  NS_ENSURE_TRUE(windowToFocus, NS_ERROR_FAILURE);

  windowToFocus = windowToFocus->GetOuterWindow();

  nsCOMPtr<Element> frameElement = windowToFocus->GetFrameElementInternal();
  if (frameElement) {
    
    
    SetFocusInner(frameElement, 0, false, true);
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

  LOGFOCUS(("<<SetFocusedWindow end>>"));

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::GetFocusedElement(nsIDOMElement** aFocusedElement)
{
  if (mFocusedContent)
    CallQueryInterface(mFocusedContent, aFocusedElement);
  else
    *aFocusedElement = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::GetLastFocusMethod(nsIDOMWindow* aWindow, uint32_t* aLastFocusMethod)
{
  
  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aWindow));
  if (window && window->IsOuterWindow())
    window = window->GetCurrentInnerWindow();
  if (!window)
    window = mFocusedWindow;

  *aLastFocusMethod = window ? window->GetFocusMethod() : 0;

  NS_ASSERTION((*aLastFocusMethod & FOCUSMETHOD_MASK) == *aLastFocusMethod,
               "invalid focus method");
  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::SetFocus(nsIDOMElement* aElement, uint32_t aFlags)
{
  LOGFOCUS(("<<SetFocus begin>>"));

  nsCOMPtr<nsIContent> newFocus = do_QueryInterface(aElement);
  NS_ENSURE_ARG(newFocus);

  SetFocusInner(newFocus, aFlags, true, true);

  LOGFOCUS(("<<SetFocus end>>"));

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::ElementIsFocusable(nsIDOMElement* aElement, uint32_t aFlags,
                                   bool* aIsFocusable)
{
  NS_ENSURE_TRUE(aElement, NS_ERROR_INVALID_ARG);

  nsCOMPtr<nsIContent> aContent = do_QueryInterface(aElement);

  *aIsFocusable = CheckIfFocusable(aContent, aFlags) != nullptr;

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::MoveFocus(nsIDOMWindow* aWindow, nsIDOMElement* aStartElement,
                          uint32_t aType, uint32_t aFlags, nsIDOMElement** aElement)
{
  *aElement = nullptr;

#ifdef PR_LOGGING
  LOGFOCUS(("<<MoveFocus begin Type: %d Flags: %x>>", aType, aFlags));

  if (PR_LOG_TEST(gFocusLog, PR_LOG_DEBUG) && mFocusedWindow) {
    nsIDocument* doc = mFocusedWindow->GetExtantDoc();
    if (doc && doc->GetDocumentURI()) {
      nsAutoCString spec;
      doc->GetDocumentURI()->GetSpec(spec);
      LOGFOCUS((" Focused Window: %p %s", mFocusedWindow.get(), spec.get()));
    }
  }

  LOGCONTENT("  Current Focus: %s", mFocusedContent.get());
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

  bool noParentTraversal = aFlags & FLAG_NOPARENTFRAME;
  nsCOMPtr<nsIContent> newFocus;
  nsresult rv = DetermineElementToMoveFocus(window, startContent, aType, noParentTraversal,
                                            getter_AddRefs(newFocus));
  NS_ENSURE_SUCCESS(rv, rv);

  LOGCONTENTNAVIGATION("Element to be focused: %s", newFocus.get());

  if (newFocus) {
    
    
    
    
    SetFocusInner(newFocus, aFlags, aType != MOVEFOCUS_CARET, true);
    CallQueryInterface(newFocus, aElement);
  }
  else if (aType == MOVEFOCUS_ROOT || aType == MOVEFOCUS_CARET) {
    
    ClearFocus(window);
  }

  LOGFOCUS(("<<MoveFocus end>>"));

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::ClearFocus(nsIDOMWindow* aWindow)
{
  LOGFOCUS(("<<ClearFocus begin>>"));

  
  
  
  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aWindow));
  NS_ENSURE_TRUE(window, NS_ERROR_INVALID_ARG);

  window = window->GetOuterWindow();
  NS_ENSURE_TRUE(window, NS_ERROR_INVALID_ARG);

  if (IsSameOrAncestor(window, mFocusedWindow)) {
    bool isAncestor = (window != mFocusedWindow);
    if (Blur(window, nullptr, isAncestor, true)) {
      
      
      if (isAncestor)
        Focus(window, nullptr, 0, true, false, false, true);
    }
  }
  else {
    window->SetFocusedNode(nullptr);
  }

  LOGFOCUS(("<<ClearFocus end>>"));

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::GetFocusedElementForWindow(nsIDOMWindow* aWindow,
                                           bool aDeep,
                                           nsIDOMWindow** aFocusedWindow,
                                           nsIDOMElement** aElement)
{
  *aElement = nullptr;
  if (aFocusedWindow)
    *aFocusedWindow = nullptr;

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
  nsCOMPtr<nsIWebNavigation> webnav = do_GetInterface(aWindow);
  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(webnav);
  if (dsti) {
    if (dsti->ItemType() != nsIDocShellTreeItem::typeChrome) {
      nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(dsti);
      NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

      
      bool isEditable;
      docShell->GetEditable(&isEditable);
      if (isEditable)
        return NS_OK;

      nsCOMPtr<nsIPresShell> presShell = docShell->GetPresShell();
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

#ifdef PR_LOGGING
  if (PR_LOG_TEST(gFocusLog, PR_LOG_DEBUG)) {
    LOGFOCUS(("Window %p Raised [Currently: %p %p]", aWindow, mActiveWindow.get(), mFocusedWindow.get()));
    nsAutoCString spec;
    nsIDocument* doc = window->GetExtantDoc();
    if (doc && doc->GetDocumentURI()) {
      doc->GetDocumentURI()->GetSpec(spec);
      LOGFOCUS(("  Raised Window: %p %s", aWindow, spec.get()));
    }
    if (mActiveWindow) {
      doc = mActiveWindow->GetExtantDoc();
      if (doc && doc->GetDocumentURI()) {
        doc->GetDocumentURI()->GetSpec(spec);
        LOGFOCUS(("  Active Window: %p %s", mActiveWindow.get(), spec.get()));
      }
    }
  }
#endif

  if (mActiveWindow == window) {
    
    
    
    
    
    
    
    EnsureCurrentWidgetFocused();
    return NS_OK;
  }

  
  if (mActiveWindow)
    WindowLowered(mActiveWindow);

  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem = window->GetDocShell();
  
  
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

    if (!sTestMode) {
      baseWindow->SetVisibility(true);
    }
  }

  
  
  
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    ActivateOrDeactivate(window, true);
  }

  
  nsCOMPtr<nsPIDOMWindow> currentWindow;
  nsCOMPtr<nsIContent> currentFocus =
    GetFocusedDescendant(window, true, getter_AddRefs(currentWindow));

  NS_ASSERTION(currentWindow, "window raised with no window current");
  if (!currentWindow)
    return NS_OK;

  nsCOMPtr<nsIDocShell> currentDocShell = currentWindow->GetDocShell();

  nsCOMPtr<nsIPresShell> presShell = currentDocShell->GetPresShell();
  if (presShell) {
    
    
    nsRefPtr<nsFrameSelection> frameSelection = presShell->FrameSelection();
    frameSelection->SetDragState(false);
  }

  
  
  nsCOMPtr<nsIXULWindow> xulWin(do_GetInterface(baseWindow));
  Focus(currentWindow, currentFocus, 0, true, false, xulWin != nullptr, true);

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::WindowLowered(nsIDOMWindow* aWindow)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aWindow);
  NS_ENSURE_TRUE(window && window->IsOuterWindow(), NS_ERROR_INVALID_ARG);

#ifdef PR_LOGGING
  if (PR_LOG_TEST(gFocusLog, PR_LOG_DEBUG)) {
    LOGFOCUS(("Window %p Lowered [Currently: %p %p]", aWindow, mActiveWindow.get(), mFocusedWindow.get()));
    nsAutoCString spec;
    nsIDocument* doc = window->GetExtantDoc();
    if (doc && doc->GetDocumentURI()) {
      doc->GetDocumentURI()->GetSpec(spec);
      LOGFOCUS(("  Lowered Window: %s", spec.get()));
    }
    if (mActiveWindow) {
      doc = mActiveWindow->GetExtantDoc();
      if (doc && doc->GetDocumentURI()) {
        doc->GetDocumentURI()->GetSpec(spec);
        LOGFOCUS(("  Active Window: %s", spec.get()));
      }
    }
  }
#endif

  if (mActiveWindow != window)
    return NS_OK;

  
  nsIPresShell::SetCapturingContent(nullptr, 0);

  
  
  
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    ActivateOrDeactivate(window, false);
  }

  
  
  
  mWindowBeingLowered = mActiveWindow;
  mActiveWindow = nullptr;

  if (mFocusedWindow)
    Blur(nullptr, nullptr, true, true);

  mWindowBeingLowered = nullptr;

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
    window->SetFocusedNode(nullptr);

    
    
    if (window == mFocusedWindow) {
      mFocusedContent = nullptr;
    } else {
      
      
      
      
      
      nsIDocument* subdoc = aDocument->GetSubDocumentFor(content);
      if (subdoc) {
        nsCOMPtr<nsIDocShell> docShell = subdoc->GetDocShell();
        if (docShell) {
          nsCOMPtr<nsPIDOMWindow> childWindow = docShell->GetWindow();
          if (childWindow && IsSameOrAncestor(childWindow, mFocusedWindow)) {
            ClearFocus(mActiveWindow);
          }
        }
      }
    }

    
    if (content->IsEditable()) {
      nsCOMPtr<nsIDocShell> docShell = aDocument->GetDocShell();
      if (docShell) {
        nsCOMPtr<nsIEditor> editor;
        docShell->GetEditor(getter_AddRefs(editor));
        if (editor) {
          nsCOMPtr<nsISelection> s;
          editor->GetSelection(getter_AddRefs(s));
          nsCOMPtr<nsISelectionPrivate> selection = do_QueryInterface(s);
          if (selection) {
            nsCOMPtr<nsIContent> limiter;
            selection->GetAncestorLimiter(getter_AddRefs(limiter));
            if (limiter == content) {
              editor->FinalizeSelection();
            }
          }
        }
      }
    }

    NotifyFocusStateChange(content, shouldShowFocusRing, false);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::WindowShown(nsIDOMWindow* aWindow, bool aNeedsFocus)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aWindow);
  NS_ENSURE_TRUE(window, NS_ERROR_INVALID_ARG);

  window = window->GetOuterWindow();

#ifdef PR_LOGGING
  if (PR_LOG_TEST(gFocusLog, PR_LOG_DEBUG)) {
    LOGFOCUS(("Window %p Shown [Currently: %p %p]", window.get(), mActiveWindow.get(), mFocusedWindow.get()));
    nsAutoCString spec;
    nsIDocument* doc = window->GetExtantDoc();
    if (doc && doc->GetDocumentURI()) {
      doc->GetDocumentURI()->GetSpec(spec);
      LOGFOCUS(("Shown Window: %s", spec.get()));
    }

    if (mFocusedWindow) {
      doc = mFocusedWindow->GetExtantDoc();
      if (doc && doc->GetDocumentURI()) {
        doc->GetDocumentURI()->GetSpec(spec);
        LOGFOCUS((" Focused Window: %s", spec.get()));
      }
    }
  }
#endif

  if (nsCOMPtr<nsITabChild> child = do_GetInterface(window->GetDocShell())) {
    bool active = static_cast<TabChild*>(child.get())->ParentIsActive();
    ActivateOrDeactivate(window, active);
  }

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

#ifdef PR_LOGGING
  if (PR_LOG_TEST(gFocusLog, PR_LOG_DEBUG)) {
    LOGFOCUS(("Window %p Hidden [Currently: %p %p]", window.get(), mActiveWindow.get(), mFocusedWindow.get()));
    nsAutoCString spec;
    nsIDocument* doc = window->GetExtantDoc();
    if (doc && doc->GetDocumentURI()) {
      doc->GetDocumentURI()->GetSpec(spec);
      LOGFOCUS(("  Hide Window: %s", spec.get()));
    }

    if (mFocusedWindow) {
      doc = mFocusedWindow->GetExtantDoc();
      if (doc && doc->GetDocumentURI()) {
        doc->GetDocumentURI()->GetSpec(spec);
        LOGFOCUS(("  Focused Window: %s", spec.get()));
      }
    }

    if (mActiveWindow) {
      doc = mActiveWindow->GetExtantDoc();
      if (doc && doc->GetDocumentURI()) {
        doc->GetDocumentURI()->GetSpec(spec);
        LOGFOCUS(("  Active Window: %s", spec.get()));
      }
    }
  }
#endif

  if (!IsSameOrAncestor(window, mFocusedWindow))
    return NS_OK;

  
  
  

  nsCOMPtr<nsIContent> oldFocusedContent = mFocusedContent.forget();

  nsCOMPtr<nsIDocShell> focusedDocShell = mFocusedWindow->GetDocShell();
  nsCOMPtr<nsIPresShell> presShell = focusedDocShell->GetPresShell();

  if (oldFocusedContent && oldFocusedContent->IsInComposedDoc()) {
    NotifyFocusStateChange(oldFocusedContent,
                           mFocusedWindow->ShouldShowFocusRing(),
                           false);
    window->UpdateCommands(NS_LITERAL_STRING("focus"), nullptr, 0);

    if (presShell) {
      SendFocusOrBlurEvent(NS_BLUR_CONTENT, presShell,
                           oldFocusedContent->GetComposedDoc(),
                           oldFocusedContent, 1, false);
    }
  }

  nsPresContext* focusedPresContext =
    presShell ? presShell->GetPresContext() : nullptr;
  IMEStateManager::OnChangeFocus(focusedPresContext, nullptr,
                                 GetFocusMoveActionCause(0));
  if (presShell) {
    SetCaretVisible(presShell, false, nullptr);
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
    nsCOMPtr<nsIDocShellTreeItem> dsti =
      mFocusedWindow ? mFocusedWindow->GetDocShell() : nullptr;
    if (dsti) {
      nsCOMPtr<nsIDocShellTreeItem> parentDsti;
      dsti->GetParent(getter_AddRefs(parentDsti));
      if (parentDsti) {
        nsCOMPtr<nsPIDOMWindow> parentWindow = parentDsti->GetWindow();
        if (parentWindow)
          parentWindow->SetFocusedNode(nullptr);
      }
    }

    SetFocusedWindowInternal(window);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsFocusManager::FireDelayedEvents(nsIDocument* aDocument)
{
  NS_ENSURE_ARG(aDocument);

  
  for (uint32_t i = 0; i < mDelayedBlurFocusEvents.Length(); i++) {
    if (mDelayedBlurFocusEvents[i].mDocument == aDocument) {
      if (!aDocument->GetInnerWindow() ||
          !aDocument->GetInnerWindow()->IsCurrentInnerWindow()) {
        
        
        
        mDelayedBlurFocusEvents.RemoveElementAt(i);
        --i;
      } else if (!aDocument->EventHandlingSuppressed()) {
        uint32_t type = mDelayedBlurFocusEvents[i].mType;
        nsCOMPtr<EventTarget> target = mDelayedBlurFocusEvents[i].mTarget;
        nsCOMPtr<nsIPresShell> presShell = mDelayedBlurFocusEvents[i].mPresShell;
        mDelayedBlurFocusEvents.RemoveElementAt(i);
        SendFocusOrBlurEvent(type, presShell, aDocument, target, 0, false);
        --i;
      }
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

NS_IMETHODIMP
nsFocusManager::ParentActivated(nsIDOMWindow* aWindow, bool aActive)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aWindow);
  NS_ENSURE_TRUE(window, NS_ERROR_INVALID_ARG);

  window = window->GetOuterWindow();

  ActivateOrDeactivate(window, aActive);
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
  EventStates eventState = NS_EVENT_STATE_FOCUS;
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
  if (!mFocusedWindow || sTestMode)
    return;

  
  
  nsCOMPtr<nsIDocShell> docShell = mFocusedWindow->GetDocShell();
  if (docShell) {
    nsCOMPtr<nsIPresShell> presShell = docShell->GetPresShell();
    if (presShell) {
      nsViewManager* vm = presShell->GetViewManager();
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
ActivateOrDeactivateChild(TabParent* aParent, void* aArg)
{
  bool active = static_cast<bool>(aArg);
  unused << aParent->SendParentActivated(active);
}

void
nsFocusManager::ActivateOrDeactivate(nsPIDOMWindow* aWindow, bool aActive)
{
  if (!aWindow) {
    return;
  }

  
  
  aWindow->ActivateOrDeactivate(aActive);

  
  nsContentUtils::DispatchEventOnlyToChrome(aWindow->GetExtantDoc(),
                                            aWindow,
                                            aActive ?
                                              NS_LITERAL_STRING("activate") :
                                              NS_LITERAL_STRING("deactivate"),
                                            true, true, nullptr);

  
  nsContentUtils::CallOnAllRemoteChildren(aWindow, ActivateOrDeactivateChild,
                                          (void *)aActive);
}

void
nsFocusManager::SetFocusInner(nsIContent* aNewContent, int32_t aFlags,
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

    nsCOMPtr<nsIDocShellTreeItem> parentDsti;
    docShell->GetParent(getter_AddRefs(parentDsti));
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
    if (!subsumes && !nsContentUtils::IsCallerChrome()) {
      NS_WARNING("Not allowed to focus the new window!");
      return;
    }
  }

  
  
  bool isElementInActiveWindow = false;

  nsCOMPtr<nsIDocShellTreeItem> dsti = newWindow->GetDocShell();
  nsCOMPtr<nsPIDOMWindow> newRootWindow;
  if (dsti) {
    nsCOMPtr<nsIDocShellTreeItem> root;
    dsti->GetRootTreeItem(getter_AddRefs(root));
    newRootWindow = root ? root->GetWindow() : nullptr;

    isElementInActiveWindow = (mActiveWindow && newRootWindow == mActiveWindow);
  }

  
  
  
  
  
#ifndef XP_MACOSX
  nsIDocument* fullscreenAncestor;
  if (contentToFocus &&
      (fullscreenAncestor = nsContentUtils::GetFullscreenAncestor(contentToFocus->OwnerDoc())) &&
      nsContentUtils::HasPluginWithUncontrolledEventDispatch(contentToFocus)) {
    nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                    NS_LITERAL_CSTRING("DOM"),
                                    contentToFocus->OwnerDoc(),
                                    nsContentUtils::eDOM_PROPERTIES,
                                    "FocusedWindowedPluginWhileFullScreen");
    nsIDocument::ExitFullscreen(fullscreenAncestor,  true);
  }
#endif

  
  
  
  bool allowFrameSwitch = !(aFlags & FLAG_NOSWITCHFRAME) ||
                            IsSameOrAncestor(newWindow, mFocusedWindow);

  
  
  bool sendFocusEvent =
    isElementInActiveWindow && allowFrameSwitch && IsWindowVisible(newWindow);

  
  
  
  
  
  if (sendFocusEvent && mFocusedContent &&
      mFocusedContent->OwnerDoc() != aNewContent->OwnerDoc()) {
    
    
    
    nsCOMPtr<nsIDOMNode> domNode(do_QueryInterface(mFocusedContent));
    sendFocusEvent = nsContentUtils::CanCallerAccess(domNode);
    if (!sendFocusEvent && mMouseButtonEventHandlingDocument) {
      
      
      domNode = do_QueryInterface(mMouseButtonEventHandlingDocument);
      sendFocusEvent = nsContentUtils::CanCallerAccess(domNode);
    }
  }

  LOGCONTENT("Shift Focus: %s", contentToFocus.get());
  LOGFOCUS((" Flags: %x Current Window: %p New Window: %p Current Element: %p",
           aFlags, mFocusedWindow.get(), newWindow.get(), mFocusedContent.get()));
  LOGFOCUS((" In Active Window: %d In Focused Window: %d SendFocus: %d",
           isElementInActiveWindow, isElementInFocusedWindow, sendFocusEvent));

  if (sendFocusEvent) {
    
    if (mFocusedWindow) {
      
      
      
      
      
      bool currentIsSameOrAncestor = IsSameOrAncestor(mFocusedWindow, newWindow);
      
      
      
      
      
      
      
      
      
      
      
      
      nsCOMPtr<nsPIDOMWindow> commonAncestor;
      if (!isElementInFocusedWindow)
        commonAncestor = GetCommonAncestor(newWindow, mFocusedWindow);

      if (!Blur(currentIsSameOrAncestor ? mFocusedWindow.get() : nullptr,
                commonAncestor, !isElementInFocusedWindow, aAdjustWidget))
        return;
    }

    Focus(newWindow, contentToFocus, aFlags, !isElementInFocusedWindow,
          aFocusChanged, false, aAdjustWidget);
  }
  else {
    
    
    if (allowFrameSwitch)
      AdjustWindowFocus(newWindow, true);

    
    uint32_t focusMethod = aFocusChanged ? aFlags & FOCUSMETHODANDRING_MASK :
                           newWindow->GetFocusMethod() | (aFlags & FLAG_SHOWRING);
    newWindow->SetFocusedNode(contentToFocus, focusMethod);
    if (aFocusChanged) {
      nsCOMPtr<nsIDocShell> docShell = newWindow->GetDocShell();

      nsCOMPtr<nsIPresShell> presShell = docShell->GetPresShell();
      if (presShell)
        ScrollIntoView(presShell, contentToFocus, aFlags);
    }

    
    
    if (allowFrameSwitch)
      newWindow->UpdateCommands(NS_LITERAL_STRING("focus"), nullptr, 0);

    if (aFlags & FLAG_RAISE)
      RaiseWindow(newRootWindow);
  }
}

bool
nsFocusManager::IsSameOrAncestor(nsPIDOMWindow* aPossibleAncestor,
                                 nsPIDOMWindow* aWindow)
{
  if (!aWindow || !aPossibleAncestor) {
    return false;
  }

  nsCOMPtr<nsIDocShellTreeItem> ancestordsti = aPossibleAncestor->GetDocShell();
  nsCOMPtr<nsIDocShellTreeItem> dsti = aWindow->GetDocShell();
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
  NS_ENSURE_TRUE(aWindow1 && aWindow2, nullptr);

  nsCOMPtr<nsIDocShellTreeItem> dsti1 = aWindow1->GetDocShell();
  NS_ENSURE_TRUE(dsti1, nullptr);

  nsCOMPtr<nsIDocShellTreeItem> dsti2 = aWindow2->GetDocShell();
  NS_ENSURE_TRUE(dsti2, nullptr);

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

  uint32_t pos1 = parents1.Length();
  uint32_t pos2 = parents2.Length();
  nsIDocShellTreeItem* parent = nullptr;
  uint32_t len;
  for (len = std::min(pos1, pos2); len > 0; --len) {
    nsIDocShellTreeItem* child1 = parents1.ElementAt(--pos1);
    nsIDocShellTreeItem* child2 = parents2.ElementAt(--pos2);
    if (child1 != child2) {
      break;
    }
    parent = child1;
  }

  nsCOMPtr<nsPIDOMWindow> window = parent ? parent->GetWindow() : nullptr;
  return window.forget();
}

void
nsFocusManager::AdjustWindowFocus(nsPIDOMWindow* aWindow,
                                  bool aCheckPermission)
{
  bool isVisible = IsWindowVisible(aWindow);

  nsCOMPtr<nsPIDOMWindow> window(aWindow);
  while (window) {
    
    
    nsCOMPtr<Element> frameElement = window->GetFrameElementInternal();

    nsCOMPtr<nsIDocShellTreeItem> dsti = window->GetDocShell();
    if (!dsti) 
      return;
    nsCOMPtr<nsIDocShellTreeItem> parentDsti;
    dsti->GetParent(getter_AddRefs(parentDsti));
    if (!parentDsti) {
      return;
    }

    window = parentDsti->GetWindow();
    if (window) {
      
      
      
      if (IsWindowVisible(window) != isVisible)
        break;

      
      
      
      if (aCheckPermission && !nsContentUtils::CanCallerAccess(window))
        break;

      window->SetFocusedNode(frameElement);
    }
  }
}

bool
nsFocusManager::IsWindowVisible(nsPIDOMWindow* aWindow)
{
  if (!aWindow || aWindow->IsFrozen())
    return false;

  
  
  nsPIDOMWindow* innerWindow = aWindow->GetCurrentInnerWindow();
  if (!innerWindow || innerWindow->IsFrozen())
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
  NS_PRECONDITION(aContent->IsInComposedDoc(), "aContent must be in a document");

  
  
  
  
  
  
  nsIDocument* doc = aContent->GetComposedDoc();
  NS_ASSERTION(doc, "aContent must have current document");
  return aContent == doc->GetRootElement() &&
           (doc->HasFlag(NODE_IS_EDITABLE) || !aContent->IsEditable() ||
            nsContentUtils::IsUserFocusIgnored(aContent));
}

nsIContent*
nsFocusManager::CheckIfFocusable(nsIContent* aContent, uint32_t aFlags)
{
  if (!aContent)
    return nullptr;

  
  
  nsIContent* redirectedFocus = GetRedirectedFocus(aContent);
  if (redirectedFocus)
    return CheckIfFocusable(redirectedFocus, aFlags);

  nsCOMPtr<nsIDocument> doc = aContent->GetComposedDoc();
  
  if (!doc) {
    LOGCONTENT("Cannot focus %s because content not in document", aContent)
    return nullptr;
  }

  
  doc->FlushPendingNotifications(Flush_Layout);

  nsIPresShell *shell = doc->GetShell();
  if (!shell)
    return nullptr;

  
  
  if (aContent == doc->GetRootElement())
    return nsContentUtils::IsUserFocusIgnored(aContent) ? nullptr : aContent;

  
  nsPresContext* presContext = shell->GetPresContext();
  if (presContext && presContext->Type() == nsPresContext::eContext_PrintPreview) {
    LOGCONTENT("Cannot focus %s while in print preview", aContent)
    return nullptr;
  }

  nsIFrame* frame = aContent->GetPrimaryFrame();
  if (!frame) {
    LOGCONTENT("Cannot focus %s as it has no frame", aContent)
    return nullptr;
  }

  if (aContent->IsHTMLElement(nsGkAtoms::area)) {
    
    
    
    return frame->IsVisibleConsideringAncestors() &&
           aContent->IsFocusable() ? aContent : nullptr;
  }

  
  
  
  
  nsIDocument* subdoc = doc->GetSubDocumentFor(aContent);
  if (subdoc && IsWindowVisible(subdoc->GetWindow())) {
    const nsStyleUserInterface* ui = frame->StyleUserInterface();
    int32_t tabIndex = (ui->mUserFocus == NS_STYLE_USER_FOCUS_IGNORE ||
                        ui->mUserFocus == NS_STYLE_USER_FOCUS_NONE) ? -1 : 0;
    return aContent->IsFocusable(&tabIndex, aFlags & FLAG_BYMOUSE) ? aContent : nullptr;
  }
  
  return frame->IsFocusable(nullptr, aFlags & FLAG_BYMOUSE) ? aContent : nullptr;
}

bool
nsFocusManager::Blur(nsPIDOMWindow* aWindowToClear,
                     nsPIDOMWindow* aAncestorWindowToFocus,
                     bool aIsLeavingDocument,
                     bool aAdjustWidgets)
{
  LOGFOCUS(("<<Blur begin>>"));

  
  nsCOMPtr<nsIContent> content = mFocusedContent;
  if (content) {
    if (!content->IsInComposedDoc()) {
      mFocusedContent = nullptr;
      return true;
    }
    if (content == mFirstBlurEvent)
      return true;
  }

  
  nsCOMPtr<nsPIDOMWindow> window = mFocusedWindow;
  if (!window) {
    mFocusedContent = nullptr;
    return true;
  }

  nsCOMPtr<nsIDocShell> docShell = window->GetDocShell();
  if (!docShell) {
    mFocusedContent = nullptr;
    return true;
  }

  
  
  nsCOMPtr<nsIPresShell> presShell = docShell->GetPresShell();
  if (!presShell) {
    mFocusedContent = nullptr;
    return true;
  }

  bool clearFirstBlurEvent = false;
  if (!mFirstBlurEvent) {
    mFirstBlurEvent = content;
    clearFirstBlurEvent = true;
  }

  nsPresContext* focusedPresContext =
    mActiveWindow ? presShell->GetPresContext() : nullptr;
  IMEStateManager::OnChangeFocus(focusedPresContext, nullptr,
                                 GetFocusMoveActionCause(0));

  
  
  mFocusedContent = nullptr;
  bool shouldShowFocusRing = window->ShouldShowFocusRing();
  if (aWindowToClear)
    aWindowToClear->SetFocusedNode(nullptr);

  LOGCONTENT("Element %s has been blurred", content.get());

  
  bool sendBlurEvent =
    content && content->IsInComposedDoc() && !IsNonFocusableRoot(content);
  if (content) {
    if (sendBlurEvent) {
      NotifyFocusStateChange(content, shouldShowFocusRing, false);
    }

    
    
    
    
    if (mActiveWindow) {
      nsIFrame* contentFrame = content->GetPrimaryFrame();
      nsIObjectFrame* objectFrame = do_QueryFrame(contentFrame);
      if (aAdjustWidgets && objectFrame && !sTestMode) {
        
        
        nsViewManager* vm = presShell->GetViewManager();
        if (vm) {
          nsCOMPtr<nsIWidget> widget;
          vm->GetRootWidget(getter_AddRefs(widget));
          if (widget)
            widget->SetFocus(false);
        }
      }
    }

      
    if (TabParent* remote = TabParent::GetFrom(content)) {
      remote->Deactivate();
      LOGFOCUS(("Remote browser deactivated"));
    }
  }

  bool result = true;
  if (sendBlurEvent) {
    
    
    
    if (mActiveWindow)
      window->UpdateCommands(NS_LITERAL_STRING("focus"), nullptr, 0);

    SendFocusOrBlurEvent(NS_BLUR_CONTENT, presShell,
                         content->GetComposedDoc(), content, 1, false);
  }

  
  
  if (aIsLeavingDocument || !mActiveWindow) {
    SetCaretVisible(presShell, false, nullptr);
  }

  nsRefPtr<SelectionCarets> selectionCarets = presShell->GetSelectionCarets();
  if (selectionCarets) {
    selectionCarets->NotifyBlur(aIsLeavingDocument || !mActiveWindow);
  }

  
  
  
  
  
  
  if (mFocusedWindow != window ||
      (mFocusedContent != nullptr && !aIsLeavingDocument)) {
    result = false;
  }
  else if (aIsLeavingDocument) {
    window->TakeFocus(false, 0);

    
    
    
    if (aAncestorWindowToFocus)
      aAncestorWindowToFocus->SetFocusedNode(nullptr, 0, true);

    SetFocusedWindowInternal(nullptr);
    mFocusedContent = nullptr;

    
    
    
    nsIDocument* doc = window->GetExtantDoc();
    if (doc)
      SendFocusOrBlurEvent(NS_BLUR_CONTENT, presShell, doc, doc, 1, false);
    if (mFocusedWindow == nullptr)
      SendFocusOrBlurEvent(NS_BLUR_CONTENT, presShell, doc, window, 1, false);

    
    result = (mFocusedWindow == nullptr && mActiveWindow);
  }
  else if (mActiveWindow) {
    
    
    
    
    
    UpdateCaret(false, true, nullptr);
  }

  if (clearFirstBlurEvent)
    mFirstBlurEvent = nullptr;

  return result;
}

void
nsFocusManager::Focus(nsPIDOMWindow* aWindow,
                      nsIContent* aContent,
                      uint32_t aFlags,
                      bool aIsNewDocument,
                      bool aFocusChanged,
                      bool aWindowRaised,
                      bool aAdjustWidgets)
{
  LOGFOCUS(("<<Focus begin>>"));

  if (!aWindow)
    return;

  if (aContent && (aContent == mFirstFocusEvent || aContent == mFirstBlurEvent))
    return;

  
  
  nsCOMPtr<nsIDocShell> docShell = aWindow->GetDocShell();
  if (!docShell)
    return;

  nsCOMPtr<nsIPresShell> presShell = docShell->GetPresShell();
  if (!presShell)
    return;

  
  
  
  uint32_t focusMethod = aFocusChanged ? aFlags & FOCUSMETHODANDRING_MASK :
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

#ifdef PR_LOGGING
  LOGCONTENT("Element %s has been focused", aContent);

  if (PR_LOG_TEST(gFocusLog, PR_LOG_DEBUG)) {
    nsIDocument* docm = aWindow->GetExtantDoc();
    if (docm) {
      LOGCONTENT(" from %s", docm->GetRootElement());
    }
    LOGFOCUS((" [Newdoc: %d FocusChanged: %d Raised: %d Flags: %x]",
             aIsNewDocument, aFocusChanged, aWindowRaised, aFlags));
  }
#endif

  if (aIsNewDocument) {
    
    
    
    AdjustWindowFocus(aWindow, false);
  }

  
  if (aWindow->TakeFocus(true, focusMethod))
    aIsNewDocument = true;

  SetFocusedWindowInternal(aWindow);

  
  
  
  nsCOMPtr<nsIWidget> objectFrameWidget;
  if (aContent) {
    nsIFrame* contentFrame = aContent->GetPrimaryFrame();
    nsIObjectFrame* objectFrame = do_QueryFrame(contentFrame);
    if (objectFrame)
      objectFrameWidget = objectFrame->GetWidget();
  }
  if (aAdjustWidgets && !objectFrameWidget && !sTestMode) {
    nsViewManager* vm = presShell->GetViewManager();
    if (vm) {
      nsCOMPtr<nsIWidget> widget;
      vm->GetRootWidget(getter_AddRefs(widget));
      if (widget)
        widget->SetFocus(false);
    }
  }

  
  
  if (aIsNewDocument) {
    nsIDocument* doc = aWindow->GetExtantDoc();
    
    
    
    if (doc && doc->HasFlag(NODE_IS_EDITABLE)) {
      IMEStateManager::OnChangeFocus(presShell->GetPresContext(), nullptr,
                                     GetFocusMoveActionCause(aFlags));
    }
    if (doc)
      SendFocusOrBlurEvent(NS_FOCUS_CONTENT, presShell, doc,
                           doc, aFlags & FOCUSMETHOD_MASK, aWindowRaised);
    if (mFocusedWindow == aWindow && mFocusedContent == nullptr)
      SendFocusOrBlurEvent(NS_FOCUS_CONTENT, presShell, doc,
                           aWindow, aFlags & FOCUSMETHOD_MASK, aWindowRaised);
  }

  
  
  if (CheckIfFocusable(aContent, aFlags) &&
      mFocusedWindow == aWindow && mFocusedContent == nullptr) {
    mFocusedContent = aContent;

    nsIContent* focusedNode = aWindow->GetFocusedNode();
    bool isRefocus = focusedNode && focusedNode->IsEqualNode(aContent);

    aWindow->SetFocusedNode(aContent, focusMethod);

    bool sendFocusEvent =
      aContent && aContent->IsInComposedDoc() && !IsNonFocusableRoot(aContent);
    nsPresContext* presContext = presShell->GetPresContext();
    if (sendFocusEvent) {
      
      if (aFocusChanged)
        ScrollIntoView(presShell, aContent, aFlags);

      NotifyFocusStateChange(aContent, aWindow->ShouldShowFocusRing(), true);

      
      
      
      if (presShell->GetDocument() == aContent->GetComposedDoc()) {
        if (aAdjustWidgets && objectFrameWidget && !sTestMode)
          objectFrameWidget->SetFocus(false);

        
        if (TabParent* remote = TabParent::GetFrom(aContent)) {
          remote->Activate();
          LOGFOCUS(("Remote browser activated"));
        }
      }

      IMEStateManager::OnChangeFocus(presContext, aContent,
                                     GetFocusMoveActionCause(aFlags));

      
      
      
      if (!aWindowRaised)
        aWindow->UpdateCommands(NS_LITERAL_STRING("focus"), nullptr, 0);

      SendFocusOrBlurEvent(NS_FOCUS_CONTENT, presShell,
                           aContent->GetComposedDoc(),
                           aContent, aFlags & FOCUSMETHOD_MASK,
                           aWindowRaised, isRefocus);
    } else {
      IMEStateManager::OnChangeFocus(presContext, nullptr,
                                     GetFocusMoveActionCause(aFlags));
      if (!aWindowRaised) {
        aWindow->UpdateCommands(NS_LITERAL_STRING("focus"), nullptr, 0);
      }
    }
  }
  else {
    
    
    
    if (aAdjustWidgets && objectFrameWidget &&
        mFocusedWindow == aWindow && mFocusedContent == nullptr &&
        !sTestMode) {
      nsViewManager* vm = presShell->GetViewManager();
      if (vm) {
        nsCOMPtr<nsIWidget> widget;
        vm->GetRootWidget(getter_AddRefs(widget));
        if (widget)
          widget->SetFocus(false);
      }
    }

    if (!mFocusedContent) {
      
      
      nsPresContext* presContext = presShell->GetPresContext();
      IMEStateManager::OnChangeFocus(presContext, nullptr,
                                     GetFocusMoveActionCause(aFlags));
    }

    if (!aWindowRaised)
      aWindow->UpdateCommands(NS_LITERAL_STRING("focus"), nullptr, 0);
  }

  
  
  
  
  
  
  if (mFocusedContent == aContent)
    UpdateCaret(aFocusChanged && !(aFlags & FLAG_BYMOUSE), aIsNewDocument,
                mFocusedContent);

  if (clearFirstFocusEvent)
    mFirstFocusEvent = nullptr;
}

class FocusBlurEvent : public nsRunnable
{
public:
  FocusBlurEvent(nsISupports* aTarget, uint32_t aType,
                 nsPresContext* aContext, bool aWindowRaised,
                 bool aIsRefocus)
  : mTarget(aTarget), mType(aType), mContext(aContext),
    mWindowRaised(aWindowRaised), mIsRefocus(aIsRefocus) {}

  NS_IMETHOD Run()
  {
    InternalFocusEvent event(true, mType);
    event.mFlags.mBubbles = false;
    event.fromRaise = mWindowRaised;
    event.isRefocus = mIsRefocus;
    return EventDispatcher::Dispatch(mTarget, mContext, &event);
  }

  nsCOMPtr<nsISupports>   mTarget;
  uint32_t                mType;
  nsRefPtr<nsPresContext> mContext;
  bool                    mWindowRaised;
  bool                    mIsRefocus;
};

void
nsFocusManager::SendFocusOrBlurEvent(uint32_t aType,
                                     nsIPresShell* aPresShell,
                                     nsIDocument* aDocument,
                                     nsISupports* aTarget,
                                     uint32_t aFocusMethod,
                                     bool aWindowRaised,
                                     bool aIsRefocus)
{
  NS_ASSERTION(aType == NS_FOCUS_CONTENT || aType == NS_BLUR_CONTENT,
               "Wrong event type for SendFocusOrBlurEvent");

  nsCOMPtr<EventTarget> eventTarget = do_QueryInterface(aTarget);

  nsCOMPtr<nsINode> n = do_QueryInterface(aTarget);
  if (!n) {
    nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(aTarget);
    n = win ? win->GetExtantDoc() : nullptr;
  }
  bool dontDispatchEvent = n && nsContentUtils::IsUserFocusIgnored(n);

  
  
  
  if (aFocusMethod && !dontDispatchEvent &&
      aDocument && aDocument->EventHandlingSuppressed()) {
    
    
    NS_ASSERTION(!aWindowRaised, "aWindowRaised should not be set");

    for (uint32_t i = mDelayedBlurFocusEvents.Length(); i > 0; --i) {
      
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

  if (!dontDispatchEvent) {
    nsContentUtils::AddScriptRunner(
      new FocusBlurEvent(aTarget, aType, aPresShell->GetPresContext(),
                         aWindowRaised, aIsRefocus));
  }
}

void
nsFocusManager::ScrollIntoView(nsIPresShell* aPresShell,
                               nsIContent* aContent,
                               uint32_t aFlags)
{
  
  if (!(aFlags & FLAG_NOSCROLL))
    aPresShell->ScrollContentIntoView(aContent,
                                      nsIPresShell::ScrollAxis(
                                        nsIPresShell::SCROLL_MINIMUM,
                                        nsIPresShell::SCROLL_IF_NOT_VISIBLE),
                                      nsIPresShell::ScrollAxis(
                                        nsIPresShell::SCROLL_MINIMUM,
                                        nsIPresShell::SCROLL_IF_NOT_VISIBLE),
                                      nsIPresShell::SCROLL_OVERFLOW_HIDDEN);
}


void
nsFocusManager::RaiseWindow(nsPIDOMWindow* aWindow)
{
  
  
  if (!aWindow || aWindow == mActiveWindow || aWindow == mWindowBeingLowered)
    return;

  if (sTestMode) {
    
    
    if (mActiveWindow)
      WindowLowered(mActiveWindow);
    WindowRaised(aWindow);
    return;
  }

#if defined(XP_WIN)
  
  
  
  
  
  nsCOMPtr<nsPIDOMWindow> childWindow;
  GetFocusedDescendant(aWindow, true, getter_AddRefs(childWindow));
  if (!childWindow)
    childWindow = aWindow;

  nsCOMPtr<nsIDocShell> docShell = aWindow->GetDocShell();
  if (!docShell)
    return;

  nsCOMPtr<nsIPresShell> presShell = docShell->GetPresShell();
  if (!presShell)
    return;

  nsViewManager* vm = presShell->GetViewManager();
  if (vm) {
    nsCOMPtr<nsIWidget> widget;
    vm->GetRootWidget(getter_AddRefs(widget));
    if (widget)
      widget->SetFocus(true);
  }
#else
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin =
    do_QueryInterface(aWindow->GetDocShell());
  if (treeOwnerAsWin) {
    nsCOMPtr<nsIWidget> widget;
    treeOwnerAsWin->GetMainWidget(getter_AddRefs(widget));
    if (widget)
      widget->SetFocus(true);
  }
#endif
}

void
nsFocusManager::UpdateCaretForCaretBrowsingMode()
{
  UpdateCaret(false, true, mFocusedContent);
}

void
nsFocusManager::UpdateCaret(bool aMoveCaretToFocus,
                            bool aUpdateVisibility,
                            nsIContent* aContent)
{
  LOGFOCUS(("Update Caret: %d %d", aMoveCaretToFocus, aUpdateVisibility));

  if (!mFocusedWindow)
    return;

  
  
  nsCOMPtr<nsIDocShell> focusedDocShell = mFocusedWindow->GetDocShell();
  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(focusedDocShell);
  if (!dsti)
    return;

  if (dsti->ItemType() == nsIDocShellTreeItem::typeChrome) {
    return;  
  }

  bool browseWithCaret =
    Preferences::GetBool("accessibility.browsewithcaret");

  nsCOMPtr<nsIPresShell> presShell = focusedDocShell->GetPresShell();
  if (!presShell)
    return;

  
  
  
  bool isEditable = false;
  focusedDocShell->GetEditable(&isEditable);

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

  if (!isEditable && aMoveCaretToFocus)
    MoveCaretToFocus(presShell, aContent);

  if (!aUpdateVisibility)
    return;

  
  
  
  if (!browseWithCaret) {
    nsCOMPtr<Element> docElement =
      mFocusedWindow->GetFrameElementInternal();
    if (docElement)
      browseWithCaret = docElement->AttrValueIs(kNameSpaceID_None,
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

  bool caretVisible = caret->IsVisible();
  if (!aVisible && !caretVisible)
    return NS_OK;

  nsRefPtr<nsFrameSelection> frameSelection;
  if (aContent) {
    NS_ASSERTION(aContent->GetComposedDoc() == aPresShell->GetDocument(),
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
      nsCOMPtr<nsISelectionController> selCon(do_QueryInterface(aPresShell));
      if (!selCon) {
        return NS_ERROR_FAILURE;
      }
      
      selCon->SetCaretEnabled(false);

      
      caret->SetIgnoreUserModify(true);
      
      caret->SetSelection(domSelection);

      
      
      

      selCon->SetCaretReadOnly(false);
      selCon->SetCaretEnabled(aVisible);
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
  *aStartContent = *aEndContent = nullptr;
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
  int32_t startOffset = 0;
  if (domSelection) {
    domSelection->GetIsCollapsed(&isCollapsed);
    nsCOMPtr<nsIDOMRange> domRange;
    rv = domSelection->GetRangeAt(0, getter_AddRefs(domRange));
    if (domRange) {
      domRange->GetStartContainer(getter_AddRefs(startNode));
      domRange->GetEndContainer(getter_AddRefs(endNode));
      domRange->GetStartOffset(&startOffset);

      nsIContent *childContent = nullptr;

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
        int32_t endOffset = 0;
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

  nsIFrame *startFrame = nullptr;
  if (startContent) {
    startFrame = startContent->GetPrimaryFrame();
    if (isCollapsed) {
      
      
      
      

      if (startContent->NodeType() == nsIDOMNode::TEXT_NODE) {
        nsAutoString nodeValue;
        startContent->AppendTextTo(nodeValue);

        bool isFormControl =
          startContent->IsNodeOfType(nsINode::eHTML_FORM_CONTROL);

        if (nodeValue.Length() == (uint32_t)startOffset && !isFormControl &&
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

          nsIFrame *newCaretFrame = nullptr;
          nsCOMPtr<nsIContent> newCaretContent = startContent;
          bool endOfSelectionInStartNode(startContent == endContent);
          do {
            
            
            frameTraversal->Next();
            newCaretFrame = static_cast<nsIFrame*>(frameTraversal->CurrentItem());
            if (nullptr == newCaretFrame)
              break;
            newCaretContent = newCaretFrame->GetContent();
          } while (!newCaretContent || newCaretContent == startContent);

          if (newCaretFrame && newCaretContent) {
            
            
            nsRect caretRect;
            nsIFrame *frame = nsCaret::GetGeometry(domSelection, &caretRect);
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
                                            int32_t aType, bool aNoParentTraversal,
                                            nsIContent** aNextContent)
{
  *aNextContent = nullptr;

  nsCOMPtr<nsIDocShell> docShell = aWindow->GetDocShell();
  if (!docShell)
    return NS_OK;

  nsCOMPtr<nsIContent> startContent = aStartContent;
  if (!startContent && aType != MOVEFOCUS_CARET) {
    if (aType == MOVEFOCUS_FORWARDDOC || aType == MOVEFOCUS_BACKWARDDOC) {
      
      
      nsCOMPtr<nsPIDOMWindow> focusedWindow;
      startContent = GetFocusedDescendant(aWindow, true, getter_AddRefs(focusedWindow));
    }
    else {
      startContent = aWindow->GetFocusedNode();
    }
  }

  nsCOMPtr<nsIDocument> doc;
  if (startContent)
    doc = startContent->GetComposedDoc();
  else
    doc = aWindow->GetExtantDoc();
  if (!doc)
    return NS_OK;

  LookAndFeel::GetInt(LookAndFeel::eIntID_TabFocusModel,
                      &nsIContent::sTabFocusModel);

  if (aType == MOVEFOCUS_ROOT) {
    NS_IF_ADDREF(*aNextContent = GetRootForFocus(aWindow, doc, false, false));
    return NS_OK;
  }
  if (aType == MOVEFOCUS_FORWARDDOC) {
    NS_IF_ADDREF(*aNextContent = GetNextTabbableDocument(startContent, true));
    return NS_OK;
  }
  if (aType == MOVEFOCUS_BACKWARDDOC) {
    NS_IF_ADDREF(*aNextContent = GetNextTabbableDocument(startContent, false));
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
                                  nullptr, startContent,
                                  true, 1, false, aNextContent);
  }
  if (aType == MOVEFOCUS_LAST) {
    if (!aStartContent)
      startContent = rootContent;
    return GetNextTabbableContent(presShell, startContent,
                                  nullptr, startContent,
                                  false, 0, false, aNextContent);
  }

  bool forward = (aType == MOVEFOCUS_FORWARD || aType == MOVEFOCUS_CARET);
  bool doNavigation = true;
  bool ignoreTabIndex = false;
  
  
  
  nsIFrame* popupFrame = nullptr;

  int32_t tabIndex = forward ? 1 : 0;
  if (startContent) {
    nsIFrame* frame = startContent->GetPrimaryFrame();
    if (startContent->IsHTMLElement(nsGkAtoms::area))
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
        nsIDocument* doc = startContent->GetComposedDoc();
        if (startContent ==
              nsLayoutUtils::GetEditableRootContentByContentEditable(doc)) {
          doNavigation = false;
        }
      }
    }
  }
  else {
#ifdef MOZ_XUL
    if (aType != MOVEFOCUS_CARET) {
      
      
      nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
      if (pm)
        popupFrame = pm->GetTopPopup(ePopupTypePanel);
    }
#endif
    if (popupFrame) {
      rootContent = popupFrame->GetContent();
      NS_ASSERTION(rootContent, "Popup frame doesn't have a content node");
      startContent = rootContent;
    }
    else {
      
      if (docShell->ItemType() != nsIDocShellTreeItem::typeChrome) {
        nsCOMPtr<nsIContent> endSelectionContent;
        GetSelectionLocation(doc, presShell,
                             getter_AddRefs(startContent),
                             getter_AddRefs(endSelectionContent));
        
        if (startContent == rootContent) {
          startContent = nullptr;
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

  LOGCONTENTNAVIGATION("Focus Navigation Start Content %s", startContent.get());
  LOGFOCUSNAVIGATION(("  Tabindex: %d Ignore: %d", tabIndex, ignoreTabIndex));

  while (doc) {
    if (doNavigation) {
      nsCOMPtr<nsIContent> nextFocus;
      nsresult rv = GetNextTabbableContent(presShell, rootContent,
                                           skipOriginalContentCheck ? nullptr : originalStartContent,
                                           startContent, forward,
                                           tabIndex, ignoreTabIndex,
                                           getter_AddRefs(nextFocus));
      NS_ENSURE_SUCCESS(rv, rv);

      
      if (nextFocus) {
        LOGCONTENTNAVIGATION("Next Content: %s", nextFocus.get());

        
        
        if (nextFocus != originalStartContent) {
          nextFocus.forget(aNextContent);
        }
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

    if (aNoParentTraversal) {
      if (startContent == rootContent)
        return NS_OK;

      startContent = rootContent;
      tabIndex = forward ? 1 : 0;
      continue;
    }

    
    
    nsCOMPtr<nsIDocShellTreeItem> docShellParent;
    docShell->GetParent(getter_AddRefs(docShellParent));
    if (docShellParent) {
      

      
      nsCOMPtr<nsPIDOMWindow> piWindow = docShell->GetWindow();
      NS_ENSURE_TRUE(piWindow, NS_ERROR_FAILURE);

      
      docShell = do_QueryInterface(docShellParent);
      NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

      nsCOMPtr<nsPIDOMWindow> piParentWindow = docShellParent->GetWindow();
      NS_ENSURE_TRUE(piParentWindow, NS_ERROR_FAILURE);
      doc = piParentWindow->GetExtantDoc();
      NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

      presShell = doc->GetShell();

      rootContent = doc->GetRootElement();
      startContent = piWindow->GetFrameElementInternal();
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
        nsCOMPtr<nsPIDOMWindow> window = docShell->GetWindow();
        if (window->GetFocusedNode() == mFocusedContent)
          Blur(mFocusedWindow, nullptr, true, true);
        else
          window->SetFocusedNode(nullptr);
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
                                       int32_t aCurrentTabIndex,
                                       bool aIgnoreTabIndex,
                                       nsIContent** aResultContent)
{
  *aResultContent = nullptr;

  nsCOMPtr<nsIContent> startContent = aStartContent;
  if (!startContent)
    return NS_OK;

  LOGCONTENTNAVIGATION("GetNextTabbable: %s", aStartContent);
  LOGFOCUSNAVIGATION(("  tabindex: %d", aCurrentTabIndex));

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
             (!iterStartContent ||
              !iterStartContent->IsHTMLElement(nsGkAtoms::area))) {
      
      
      if (aForward)
        frameTraversal->Next();
      else
        frameTraversal->Prev();
    }

    
    nsIFrame* frame = static_cast<nsIFrame*>(frameTraversal->CurrentItem());
    while (frame) {
      
      
      
      
      
      
      

      int32_t tabIndex;
      frame->IsFocusable(&tabIndex, 0);

      LOGCONTENTNAVIGATION("Next Tabbable %s:", frame->GetContent());
      LOGFOCUSNAVIGATION(("  with tabindex: %d expected: %d", tabIndex, aCurrentTabIndex));

      nsIContent* currentContent = frame->GetContent();
      if (tabIndex >= 0) {
        NS_ASSERTION(currentContent, "IsFocusable set a tabindex for a frame with no content");
        if (currentContent->IsHTMLElement(nsGkAtoms::img) &&
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

          
          
          nsIDocument* doc = currentContent->GetComposedDoc();
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
                     GetRootForFocus(window, aRootContent->GetComposedDoc(),
                                     false, true));
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
                                       int32_t aCurrentTabIndex,
                                       nsIContent* aImageContent,
                                       nsIContent* aStartContent)
{
  nsAutoString useMap;
  aImageContent->GetAttr(kNameSpaceID_None, nsGkAtoms::usemap, useMap);

  nsCOMPtr<nsIDocument> doc = aImageContent->GetComposedDoc();
  if (doc) {
    nsCOMPtr<nsIContent> mapContent = doc->FindImageMap(useMap);
    if (!mapContent)
      return nullptr;
    uint32_t count = mapContent->GetChildCount();
    

    int32_t index = mapContent->IndexOf(aStartContent);
    int32_t tabIndex;
    if (index < 0 || (aStartContent->IsFocusable(&tabIndex) &&
                      tabIndex != aCurrentTabIndex)) {
      
      
      
      
      index = aForward ? -1 : (int32_t)count;
    }

    
    nsCOMPtr<nsIContent> areaContent;
    while ((areaContent = mapContent->GetChildAt(aForward ? ++index : --index)) != nullptr) {
      if (areaContent->IsFocusable(&tabIndex) && tabIndex == aCurrentTabIndex) {
        return areaContent;
      }
    }
  }

  return nullptr;
}

int32_t
nsFocusManager::GetNextTabIndex(nsIContent* aParent,
                                int32_t aCurrentTabIndex,
                                bool aForward)
{
  int32_t tabIndex, childTabIndex;

  if (aForward) {
    tabIndex = 0;
    for (nsIContent* child = aParent->GetFirstChild();
         child;
         child = child->GetNextSibling()) {
      childTabIndex = GetNextTabIndex(child, aCurrentTabIndex, aForward);
      if (childTabIndex > aCurrentTabIndex && childTabIndex != tabIndex) {
        tabIndex = (tabIndex == 0 || childTabIndex < tabIndex) ? childTabIndex : tabIndex;
      }

      nsAutoString tabIndexStr;
      child->GetAttr(kNameSpaceID_None, nsGkAtoms::tabindex, tabIndexStr);
      nsresult ec;
      int32_t val = tabIndexStr.ToInteger(&ec);
      if (NS_SUCCEEDED (ec) && val > aCurrentTabIndex && val != tabIndex) {
        tabIndex = (tabIndex == 0 || val < tabIndex) ? val : tabIndex;
      }
    }
  }
  else { 
    tabIndex = 1;
    for (nsIContent* child = aParent->GetFirstChild();
         child;
         child = child->GetNextSibling()) {
      childTabIndex = GetNextTabIndex(child, aCurrentTabIndex, aForward);
      if ((aCurrentTabIndex == 0 && childTabIndex > tabIndex) ||
          (childTabIndex < aCurrentTabIndex && childTabIndex > tabIndex)) {
        tabIndex = childTabIndex;
      }

      nsAutoString tabIndexStr;
      child->GetAttr(kNameSpaceID_None, nsGkAtoms::tabindex, tabIndexStr);
      nsresult ec;
      int32_t val = tabIndexStr.ToInteger(&ec);
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
    nsCOMPtr<Element> docElement = aWindow->GetFrameElementInternal();
    
    if (docElement) {
      if (docElement->NodeInfo()->NameAtom() == nsGkAtoms::iframe)
        return nullptr;

      nsIFrame* frame = docElement->GetPrimaryFrame();
      if (!frame || !frame->IsFocusable(nullptr, 0))
        return nullptr;
    }
  } else {
    nsCOMPtr<nsIDocShell> docShell = aWindow->GetDocShell();
    if (docShell->ItemType() == nsIDocShellTreeItem::typeChrome) {
      return nullptr;
    }
  }

  if (aCheckVisibility && !IsWindowVisible(aWindow))
    return nullptr;

  Element *rootElement = aDocument->GetRootElement();
  if (!rootElement) {
    return nullptr;
  }

  if (aCheckVisibility && !rootElement->GetPrimaryFrame()) {
    return nullptr;
  }

  
  nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(aDocument);
  if (htmlDoc && aDocument->GetHtmlChildElement(nsGkAtoms::frameset)) {
    return nullptr;
  }

  return rootElement;
}

void
nsFocusManager::GetLastDocShell(nsIDocShellTreeItem* aItem,
                                nsIDocShellTreeItem** aResult)
{
  *aResult = nullptr;

  nsCOMPtr<nsIDocShellTreeItem> curItem = aItem;
  while (curItem) {
    int32_t childCount = 0;
    curItem->GetChildCount(&childCount);
    if (!childCount) {
      curItem.forget(aResult);
      return;
    }

    
    curItem->GetChildAt(childCount - 1, getter_AddRefs(curItem));
  }
}

void
nsFocusManager::GetNextDocShell(nsIDocShellTreeItem* aItem,
                                nsIDocShellTreeItem** aResult)
{
  *aResult = nullptr;

  int32_t childCount = 0;
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
    for (int32_t index = 0; index < childCount; ++index) {
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
  *aResult = nullptr;

  nsCOMPtr<nsIDocShellTreeItem> parentItem;
  aItem->GetParent(getter_AddRefs(parentItem));
  if (!parentItem)
    return;

  
  
  int32_t childCount = 0;
  parentItem->GetChildCount(&childCount);
  nsCOMPtr<nsIDocShellTreeItem> prevItem, iterItem;
  for (int32_t index = 0; index < childCount; ++index) {
    parentItem->GetChildAt(index, getter_AddRefs(iterItem));
    if (iterItem == aItem)
      break;
    prevItem = iterItem;
  }

  if (prevItem)
    GetLastDocShell(prevItem, aResult);
  else
    parentItem.forget(aResult);
}

nsIContent*
nsFocusManager::GetNextTabbablePanel(nsIDocument* aDocument, nsIFrame* aCurrentPopup, bool aForward)
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (!pm)
    return nullptr;

  
  nsTArray<nsIFrame *> popups;
  pm->GetVisiblePopups(popups);
  int32_t i = aForward ? 0 : popups.Length() - 1;
  int32_t end = aForward ? popups.Length() : -1;

  for (; i != end; aForward ? i++ : i--) {
    nsIFrame* popupFrame = popups[i];
    if (aCurrentPopup) {
      
      
      
      if (aCurrentPopup == popupFrame)
        aCurrentPopup = nullptr;
      continue;
    }

    
    if (!popupFrame->GetContent()->IsXULElement(nsGkAtoms::panel) ||
        (aDocument && popupFrame->GetContent()->GetComposedDoc() != aDocument)) {
      continue;
    }

    
    
    nsIPresShell* presShell = popupFrame->PresContext()->GetPresShell();
    if (presShell) {
      nsCOMPtr<nsIContent> nextFocus;
      nsIContent* popup = popupFrame->GetContent();
      nsresult rv = GetNextTabbableContent(presShell, popup,
                                           nullptr, popup,
                                           true, 1, false,
                                           getter_AddRefs(nextFocus));
      if (NS_SUCCEEDED(rv) && nextFocus) {
        return nextFocus.get();
      }
    }
  }

  return nullptr;
}

nsIContent*
nsFocusManager::GetNextTabbableDocument(nsIContent* aStartContent, bool aForward)
{
  
  nsIFrame* currentPopup = nullptr;
  nsCOMPtr<nsIDocument> doc;
  nsCOMPtr<nsIDocShell> startDocShell;

  if (aStartContent) {
    doc = aStartContent->GetComposedDoc();
    if (doc) {
      startDocShell = doc->GetWindow()->GetDocShell();
    }

    
    
    nsIContent* content = aStartContent;
    while (content) {
      if (content->NodeInfo()->Equals(nsGkAtoms::panel, kNameSpaceID_XUL)) {
        currentPopup = content->GetPrimaryFrame();
        break;
      }
      content = content->GetParent();
    }
  }
  else if (mFocusedWindow) {
    startDocShell = mFocusedWindow->GetDocShell();
    doc = mFocusedWindow->GetExtantDoc();
  } else if (mActiveWindow) {
    startDocShell = mActiveWindow->GetDocShell();
    doc = mActiveWindow->GetExtantDoc();
  }

  if (!startDocShell)
    return nullptr;

  
  
  nsIContent* content = aStartContent;
  nsCOMPtr<nsIDocShellTreeItem> curItem = startDocShell.get();
  nsCOMPtr<nsIDocShellTreeItem> nextItem;
  do {
    
    
    
    
    
    
    
    

    bool checkPopups = false;
    nsCOMPtr<nsPIDOMWindow> nextFrame = nullptr;

    if (doc && (aForward || currentPopup)) {
      nsIContent* popupContent = GetNextTabbablePanel(doc, currentPopup, aForward);
      if (popupContent)
        return popupContent;

      if (!aForward && currentPopup) {
        
        
        nextFrame = doc->GetWindow();
      }
    }

    
    if (!nextFrame) {
      if (aForward) {
        GetNextDocShell(curItem, getter_AddRefs(nextItem));
        if (!nextItem) {
          
          startDocShell->GetRootTreeItem(getter_AddRefs(nextItem));
        }
      }
      else {
        GetPreviousDocShell(curItem, getter_AddRefs(nextItem));
        if (!nextItem) {
          
          nsCOMPtr<nsIDocShellTreeItem> rootItem;
          startDocShell->GetRootTreeItem(getter_AddRefs(rootItem));
          GetLastDocShell(rootItem, getter_AddRefs(nextItem));
        }

        
        
        checkPopups = true;
      }

      curItem = nextItem;
      nextFrame = nextItem ? nextItem->GetWindow() : nullptr;
    }

    if (!nextFrame)
      return nullptr;

    
    currentPopup = nullptr;

    
    
    
    doc = nextFrame->GetExtantDoc();
    if (!doc || doc->EventHandlingSuppressed()) {
      content = nullptr;
      continue;
    }

    if (checkPopups) {
      
      
      
      nsIContent* popupContent = GetNextTabbablePanel(doc, nullptr, false);
      if (popupContent)
        return popupContent;
    }

    content = GetRootForFocus(nextFrame, doc, true, true);
    if (content && !GetRootForFocus(nextFrame, doc, false, false)) {
      
      
      
      nsCOMPtr<nsIContent> nextFocus;
      Element* rootElement = doc->GetRootElement();
      nsIPresShell* presShell = doc->GetShell();
      if (presShell) {
        nsresult rv = GetNextTabbableContent(presShell, rootElement,
                                             nullptr, rootElement,
                                             true, 1, false,
                                             getter_AddRefs(nextFocus));
        return NS_SUCCEEDED(rv) ? nextFocus.get() : nullptr;
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
  *aFocusedContent = nullptr;

  nsCOMPtr<nsIContent> testContent = aStartSelection;
  nsCOMPtr<nsIContent> nextTestContent = aEndSelection;

  nsCOMPtr<nsIContent> currentFocus = aWindow->GetFocusedNode();

  
  

  
  
  
  
  while (testContent) {
    
    

    nsCOMPtr<nsIURI> uri;
    if (testContent == currentFocus ||
        testContent->IsLink(getter_AddRefs(uri))) {
      testContent.forget(aFocusedContent);
      return;
    }

    
    testContent = testContent->GetParent();

    if (!testContent) {
      
      testContent = nextTestContent;
      nextTestContent = nullptr;
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
      testContent.forget(aFocusedContent);
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
        selectionNode = nullptr;
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

class PointerUnlocker : public nsRunnable
{
public:
  PointerUnlocker()
  {
    MOZ_ASSERT(!PointerUnlocker::sActiveUnlocker);
    PointerUnlocker::sActiveUnlocker = this;
  }

  ~PointerUnlocker()
  {
    if (PointerUnlocker::sActiveUnlocker == this) {
      PointerUnlocker::sActiveUnlocker = nullptr;
    }
  }

  NS_IMETHOD Run()
  {
    if (PointerUnlocker::sActiveUnlocker == this) {
      PointerUnlocker::sActiveUnlocker = nullptr;
    }
    NS_ENSURE_STATE(nsFocusManager::GetFocusManager());
    nsPIDOMWindow* focused =
      nsFocusManager::GetFocusManager()->GetFocusedWindow();
    nsCOMPtr<nsIDocument> pointerLockedDoc =
      do_QueryReferent(EventStateManager::sPointerLockedDoc);
    if (pointerLockedDoc &&
        !nsContentUtils::IsInPointerLockContext(focused)) {
      nsIDocument::UnlockPointer();
    }
    return NS_OK;
  }

  static PointerUnlocker* sActiveUnlocker;
};

PointerUnlocker*
PointerUnlocker::sActiveUnlocker = nullptr;

void
nsFocusManager::SetFocusedWindowInternal(nsPIDOMWindow* aWindow)
{
  if (!PointerUnlocker::sActiveUnlocker &&
      nsContentUtils::IsInPointerLockContext(mFocusedWindow) &&
      !nsContentUtils::IsInPointerLockContext(aWindow)) {
    nsCOMPtr<nsIRunnable> runnable = new PointerUnlocker();
    NS_DispatchToCurrentThread(runnable);
  }
  mFocusedWindow = aWindow;
}

void
nsFocusManager::MarkUncollectableForCCGeneration(uint32_t aGeneration)
{
  if (!sInstance) {
    return;
  }

  if (sInstance->mActiveWindow) {
    sInstance->mActiveWindow->
      MarkUncollectableForCCGeneration(aGeneration);
  }
  if (sInstance->mFocusedWindow) {
    sInstance->mFocusedWindow->
      MarkUncollectableForCCGeneration(aGeneration);
  }
  if (sInstance->mWindowBeingLowered) {
    sInstance->mWindowBeingLowered->
      MarkUncollectableForCCGeneration(aGeneration);
  }
  if (sInstance->mFocusedContent) {
    sInstance->mFocusedContent->OwnerDoc()->
      MarkUncollectableForCCGeneration(aGeneration);
  }
  if (sInstance->mFirstBlurEvent) {
    sInstance->mFirstBlurEvent->OwnerDoc()->
      MarkUncollectableForCCGeneration(aGeneration);
  }
  if (sInstance->mFirstFocusEvent) {
    sInstance->mFirstFocusEvent->OwnerDoc()->
      MarkUncollectableForCCGeneration(aGeneration);
  }
  if (sInstance->mMouseButtonEventHandlingDocument) {
    sInstance->mMouseButtonEventHandlingDocument->
      MarkUncollectableForCCGeneration(aGeneration);
  }
}

nsresult
NS_NewFocusManager(nsIFocusManager** aResult)
{
  NS_IF_ADDREF(*aResult = nsFocusManager::GetFocusManager());
  return NS_OK;
}
