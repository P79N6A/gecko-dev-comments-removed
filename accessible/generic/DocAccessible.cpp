




#include "Accessible-inl.h"
#include "AccIterator.h"
#include "DocAccessible-inl.h"
#include "HTMLImageMapAccessible.h"
#include "nsAccCache.h"
#include "nsAccessiblePivot.h"
#include "nsAccUtils.h"
#include "nsEventShell.h"
#include "nsTextEquivUtils.h"
#include "Role.h"
#include "RootAccessible.h"
#include "TreeWalker.h"

#include "nsIMutableArray.h"
#include "nsICommandManager.h"
#include "nsIDocShell.h"
#include "nsIDocument.h"
#include "nsIDOMAttr.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMMutationEvent.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMXULPopupElement.h"
#include "nsIEditingSession.h"
#include "nsIFrame.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsImageFrame.h"
#include "nsIPersistentProperties2.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"
#include "nsViewManager.h"
#include "nsIScrollableFrame.h"
#include "nsUnicharUtils.h"
#include "nsIURI.h"
#include "nsIWebNavigation.h"
#include "nsFocusManager.h"
#include "nsNameSpaceManager.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/Assertions.h"
#include "mozilla/EventStates.h"
#include "mozilla/dom/DocumentType.h"
#include "mozilla/dom/Element.h"

#ifdef MOZ_XUL
#include "nsIXULDocument.h"
#endif

using namespace mozilla;
using namespace mozilla::a11y;




static nsIAtom** kRelationAttrs[] =
{
  &nsGkAtoms::aria_labelledby,
  &nsGkAtoms::aria_describedby,
  &nsGkAtoms::aria_owns,
  &nsGkAtoms::aria_controls,
  &nsGkAtoms::aria_flowto,
  &nsGkAtoms::_for,
  &nsGkAtoms::control
};

static const uint32_t kRelationAttrsLen = ArrayLength(kRelationAttrs);




DocAccessible::
  DocAccessible(nsIDocument* aDocument, nsIContent* aRootContent,
                  nsIPresShell* aPresShell) :
  HyperTextAccessibleWrap(aRootContent, this),
  
  mAccessibleCache(kDefaultCacheLength),
  mNodeToAccessibleMap(kDefaultCacheLength),
  mDocumentNode(aDocument),
  mScrollPositionChangedTicks(0),
  mLoadState(eTreeConstructionPending), mDocFlags(0), mLoadEventType(0),
  mVirtualCursor(nullptr),
  mPresShell(aPresShell)
{
  mGenericTypes |= eDocument;
  mStateFlags |= eNotNodeMapEntry;

  MOZ_ASSERT(mPresShell, "should have been given a pres shell");
  mPresShell->SetDocAccessible(this);

  
  if (mDocumentNode && mDocumentNode->IsXUL())
    mGenericTypes &= ~eHyperText;
}

DocAccessible::~DocAccessible()
{
  NS_ASSERTION(!mPresShell, "LastRelease was never called!?!");
}





NS_IMPL_CYCLE_COLLECTION_CLASS(DocAccessible)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(DocAccessible, Accessible)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mNotificationController)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mVirtualCursor)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mChildDocuments)
  tmp->mDependentIDsHash.EnumerateRead(CycleCollectorTraverseDepIDsEntry, &cb);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAccessibleCache)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAnchorJumpElm)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(DocAccessible, Accessible)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mNotificationController)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mVirtualCursor)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mChildDocuments)
  tmp->mDependentIDsHash.Clear();
  tmp->mNodeToAccessibleMap.Clear();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mAccessibleCache)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mAnchorJumpElm)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(DocAccessible)
  NS_INTERFACE_MAP_ENTRY(nsIAccessibleDocument)
  NS_INTERFACE_MAP_ENTRY(nsIDocumentObserver)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsIAccessiblePivotObserver)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIAccessibleDocument)
    foundInterface = 0;

  nsresult status;
  if (!foundInterface) {
    
    
    
    

    status = IsHyperText() ? 
      HyperTextAccessible::QueryInterface(aIID, (void**)&foundInterface) :
      Accessible::QueryInterface(aIID, (void**)&foundInterface);
  } else {
    NS_ADDREF(foundInterface);
    status = NS_OK;
  }

  *aInstancePtr = foundInterface;
  return status;
}

NS_IMPL_ADDREF_INHERITED(DocAccessible, HyperTextAccessible)
NS_IMPL_RELEASE_INHERITED(DocAccessible, HyperTextAccessible)




ENameValueFlag
DocAccessible::Name(nsString& aName)
{
  aName.Truncate();

  if (mParent) {
    mParent->Name(aName); 
  }
  if (aName.IsEmpty()) {
    
    Accessible::Name(aName);
  }
  if (aName.IsEmpty()) {
    GetTitle(aName);   
  }
  if (aName.IsEmpty()) {   
    GetURL(aName);
  }
 
  return eNameOK;
}


role
DocAccessible::NativeRole()
{
  nsCOMPtr<nsIDocShell> docShell = nsCoreUtils::GetDocShellFor(mDocumentNode);
  if (docShell) {
    nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
    docShell->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));
    int32_t itemType = docShell->ItemType();
    if (sameTypeRoot == docShell) {
      
      if (itemType == nsIDocShellTreeItem::typeChrome)
        return roles::CHROME_WINDOW;

      if (itemType == nsIDocShellTreeItem::typeContent) {
#ifdef MOZ_XUL
        nsCOMPtr<nsIXULDocument> xulDoc(do_QueryInterface(mDocumentNode));
        if (xulDoc)
          return roles::APPLICATION;
#endif
        return roles::DOCUMENT;
      }
    }
    else if (itemType == nsIDocShellTreeItem::typeContent) {
      return roles::DOCUMENT;
    }
  }

  return roles::PANE; 
}

void
DocAccessible::Description(nsString& aDescription)
{
  if (mParent)
    mParent->Description(aDescription);

  if (HasOwnContent() && aDescription.IsEmpty()) {
    nsTextEquivUtils::
      GetTextEquivFromIDRefs(this, nsGkAtoms::aria_describedby,
                             aDescription);
  }
}


uint64_t
DocAccessible::NativeState()
{
  
  uint64_t state = states::FOCUSABLE; 
  if (FocusMgr()->IsFocused(this))
    state |= states::FOCUSED;

  
  
  if (!HasLoadState(eReady))
    state |= states::STALE;

  
  
  if (!HasLoadState(eCompletelyLoaded))
    state |= states::BUSY;

  nsIFrame* frame = GetFrame();
  if (!frame ||
      !frame->IsVisibleConsideringAncestors(nsIFrame::VISIBILITY_CROSS_CHROME_CONTENT_BOUNDARY)) {
    state |= states::INVISIBLE | states::OFFSCREEN;
  }

  nsCOMPtr<nsIEditor> editor = GetEditor();
  state |= editor ? states::EDITABLE : states::READONLY;

  return state;
}

uint64_t
DocAccessible::NativeInteractiveState() const
{
  
  return states::FOCUSABLE;
}

bool
DocAccessible::NativelyUnavailable() const
{
  return false;
}


void
DocAccessible::ApplyARIAState(uint64_t* aState) const
{
  
  if (mContent)
    Accessible::ApplyARIAState(aState);

  
  if (mParent)
    mParent->ApplyARIAState(aState);
}

already_AddRefed<nsIPersistentProperties>
DocAccessible::Attributes()
{
  nsCOMPtr<nsIPersistentProperties> attributes =
    HyperTextAccessibleWrap::Attributes();

  
  
  if (!mParent || IsRoot())
    return attributes.forget();

  
  aria::AttrIterator attribIter(mParent->GetContent());
  nsAutoString name, value, unused;
  while(attribIter.Next(name, value))
    attributes->SetStringProperty(NS_ConvertUTF16toUTF8(name), value, unused);

  return attributes.forget();
}

Accessible*
DocAccessible::FocusedChild()
{
  
  
  return FocusMgr()->FocusedAccessible();
}

void
DocAccessible::TakeFocus()
{
  
  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  nsCOMPtr<nsIDOMElement> newFocus;
  fm->MoveFocus(mDocumentNode->GetWindow(), nullptr,
                nsFocusManager::MOVEFOCUS_ROOT, 0, getter_AddRefs(newFocus));
}





NS_IMETHODIMP
DocAccessible::GetURL(nsAString& aURL)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsISupports> container = mDocumentNode->GetContainer();
  nsCOMPtr<nsIWebNavigation> webNav(do_GetInterface(container));
  nsAutoCString theURL;
  if (webNav) {
    nsCOMPtr<nsIURI> pURI;
    webNav->GetCurrentURI(getter_AddRefs(pURI));
    if (pURI)
      pURI->GetSpec(theURL);
  }
  CopyUTF8toUTF16(theURL, aURL);
  return NS_OK;
}

NS_IMETHODIMP
DocAccessible::GetTitle(nsAString& aTitle)
{
  if (!mDocumentNode) {
    return NS_ERROR_FAILURE;
  }
  nsString title;
  mDocumentNode->GetTitle(title);
  aTitle = title;
  return NS_OK;
}

NS_IMETHODIMP
DocAccessible::GetMimeType(nsAString& aMimeType)
{
  if (!mDocumentNode) {
    return NS_ERROR_FAILURE;
  }
  return mDocumentNode->GetContentType(aMimeType);
}

NS_IMETHODIMP
DocAccessible::GetDocType(nsAString& aDocType)
{
#ifdef MOZ_XUL
  nsCOMPtr<nsIXULDocument> xulDoc(do_QueryInterface(mDocumentNode));
  if (xulDoc) {
    aDocType.AssignLiteral("window"); 
    return NS_OK;
  } else
#endif
  if (mDocumentNode) {
    dom::DocumentType* docType = mDocumentNode->GetDoctype();
    if (docType) {
      return docType->GetPublicId(aDocType);
    }
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
DocAccessible::GetNameSpaceURIForID(int16_t aNameSpaceID, nsAString& aNameSpaceURI)
{
  if (mDocumentNode) {
    nsNameSpaceManager* nameSpaceManager = nsNameSpaceManager::GetInstance();
    if (nameSpaceManager)
      return nameSpaceManager->GetNameSpaceURI(aNameSpaceID, aNameSpaceURI);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
DocAccessible::GetWindowHandle(void** aWindow)
{
  NS_ENSURE_ARG_POINTER(aWindow);
  *aWindow = GetNativeWindow();
  return NS_OK;
}

NS_IMETHODIMP
DocAccessible::GetWindow(nsIDOMWindow** aDOMWin)
{
  *aDOMWin = nullptr;
  if (!mDocumentNode) {
    return NS_ERROR_FAILURE;  
  }
  *aDOMWin = mDocumentNode->GetWindow();

  if (!*aDOMWin)
    return NS_ERROR_FAILURE;  

  NS_ADDREF(*aDOMWin);

  return NS_OK;
}

NS_IMETHODIMP
DocAccessible::GetDOMDocument(nsIDOMDocument** aDOMDocument)
{
  NS_ENSURE_ARG_POINTER(aDOMDocument);
  *aDOMDocument = nullptr;

  if (mDocumentNode)
    CallQueryInterface(mDocumentNode, aDOMDocument);

  return NS_OK;
}

NS_IMETHODIMP
DocAccessible::GetParentDocument(nsIAccessibleDocument** aDocument)
{
  NS_ENSURE_ARG_POINTER(aDocument);
  *aDocument = nullptr;

  if (!IsDefunct())
    NS_IF_ADDREF(*aDocument = ParentDocument());

  return NS_OK;
}

NS_IMETHODIMP
DocAccessible::GetChildDocumentCount(uint32_t* aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);
  *aCount = 0;

  if (!IsDefunct())
    *aCount = ChildDocumentCount();

  return NS_OK;
}

NS_IMETHODIMP
DocAccessible::GetChildDocumentAt(uint32_t aIndex,
                                  nsIAccessibleDocument** aDocument)
{
  NS_ENSURE_ARG_POINTER(aDocument);
  *aDocument = nullptr;

  if (IsDefunct())
    return NS_OK;

  NS_IF_ADDREF(*aDocument = GetChildDocumentAt(aIndex));
  return *aDocument ? NS_OK : NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
DocAccessible::GetVirtualCursor(nsIAccessiblePivot** aVirtualCursor)
{
  NS_ENSURE_ARG_POINTER(aVirtualCursor);
  *aVirtualCursor = nullptr;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (!mVirtualCursor) {
    mVirtualCursor = new nsAccessiblePivot(this);
    mVirtualCursor->AddObserver(this);
  }

  NS_ADDREF(*aVirtualCursor = mVirtualCursor);
  return NS_OK;
}


already_AddRefed<nsIEditor>
DocAccessible::GetEditor() const
{
  
  
  if (!mDocumentNode->HasFlag(NODE_IS_EDITABLE) &&
      (!mContent || !mContent->HasFlag(NODE_IS_EDITABLE)))
    return nullptr;

  nsCOMPtr<nsISupports> container = mDocumentNode->GetContainer();
  nsCOMPtr<nsIEditingSession> editingSession(do_GetInterface(container));
  if (!editingSession)
    return nullptr; 

  nsCOMPtr<nsIEditor> editor;
  editingSession->GetEditorForWindow(mDocumentNode->GetWindow(), getter_AddRefs(editor));
  if (!editor)
    return nullptr;

  bool isEditable = false;
  editor->GetIsDocumentEditable(&isEditable);
  if (isEditable)
    return editor.forget();

  return nullptr;
}


Accessible*
DocAccessible::GetAccessible(nsINode* aNode) const
{
  Accessible* accessible = mNodeToAccessibleMap.Get(aNode);

  
  
  if (!accessible) {
    if (GetNode() != aNode)
      return nullptr;

    accessible = const_cast<DocAccessible*>(this);
  }

#ifdef DEBUG
  
  
  
  
  Accessible* parent = accessible->Parent();
  if (parent)
    parent->TestChildCache(accessible);
#endif

  return accessible;
}




void
DocAccessible::Init()
{
#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eDocCreate))
    logging::DocCreate("document initialize", mDocumentNode, this);
#endif

  
  mNotificationController = new NotificationController(this, mPresShell);

  
  
  
  if (mDocumentNode->GetReadyStateEnum() == nsIDocument::READYSTATE_COMPLETE)
    mLoadState |= eDOMLoaded;

  AddEventListeners();
}

void
DocAccessible::Shutdown()
{
  if (!mPresShell) 
    return;

#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eDocDestroy))
    logging::DocDestroy("document shutdown", mDocumentNode, this);
#endif

  if (mNotificationController) {
    mNotificationController->Shutdown();
    mNotificationController = nullptr;
  }

  RemoveEventListeners();

  
  
  
  mStateFlags |= eIsDefunct;
  nsCOMPtr<nsIDocument> kungFuDeathGripDoc = mDocumentNode;
  mDocumentNode = nullptr;

  if (mParent) {
    DocAccessible* parentDocument = mParent->Document();
    if (parentDocument)
      parentDocument->RemoveChildDocument(this);

    mParent->RemoveChild(this);
  }

  
  
  int32_t childDocCount = mChildDocuments.Length();
  for (int32_t idx = childDocCount - 1; idx >= 0; idx--)
    mChildDocuments[idx]->Shutdown();

  mChildDocuments.Clear();

  if (mVirtualCursor) {
    mVirtualCursor->RemoveObserver(this);
    mVirtualCursor = nullptr;
  }

  mPresShell->SetDocAccessible(nullptr);
  mPresShell = nullptr;  

  mDependentIDsHash.Clear();
  mNodeToAccessibleMap.Clear();
  ClearCache(mAccessibleCache);

  HyperTextAccessibleWrap::Shutdown();

  GetAccService()->NotifyOfDocumentShutdown(kungFuDeathGripDoc);
}

nsIFrame*
DocAccessible::GetFrame() const
{
  nsIFrame* root = nullptr;
  if (mPresShell)
    root = mPresShell->GetRootFrame();

  return root;
}


nsRect
DocAccessible::RelativeBounds(nsIFrame** aRelativeFrame) const
{
  *aRelativeFrame = GetFrame();

  nsIDocument *document = mDocumentNode;
  nsIDocument *parentDoc = nullptr;

  nsRect bounds;
  while (document) {
    nsIPresShell *presShell = document->GetShell();
    if (!presShell)
      return nsRect();

    nsRect scrollPort;
    nsIScrollableFrame* sf = presShell->GetRootScrollFrameAsScrollableExternal();
    if (sf) {
      scrollPort = sf->GetScrollPortRect();
    } else {
      nsIFrame* rootFrame = presShell->GetRootFrame();
      if (!rootFrame)
        return nsRect();

      scrollPort = rootFrame->GetRect();
    }

    if (parentDoc) {  
      
      
      
      
      bounds.IntersectRect(scrollPort, bounds);
    }
    else {  
      bounds = scrollPort;
    }

    document = parentDoc = document->GetParentDocument();
  }

  return bounds;
}


nsresult
DocAccessible::AddEventListeners()
{
  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem(mDocumentNode->GetDocShell());

  
  
  if (docShellTreeItem->ItemType() == nsIDocShellTreeItem::typeContent) {
    nsCOMPtr<nsICommandManager> commandManager = do_GetInterface(docShellTreeItem);
    if (commandManager)
      commandManager->AddCommandObserver(this, "obs_documentCreated");
  }

  SelectionMgr()->AddDocSelectionListener(mPresShell);

  
  mDocumentNode->AddObserver(this);
  return NS_OK;
}


nsresult
DocAccessible::RemoveEventListeners()
{
  
  
  RemoveScrollListener();

  NS_ASSERTION(mDocumentNode, "No document during removal of listeners.");

  if (mDocumentNode) {
    mDocumentNode->RemoveObserver(this);

    nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem(mDocumentNode->GetDocShell());
    NS_ASSERTION(docShellTreeItem, "doc should support nsIDocShellTreeItem.");

    if (docShellTreeItem) {
      if (docShellTreeItem->ItemType() == nsIDocShellTreeItem::typeContent) {
        nsCOMPtr<nsICommandManager> commandManager = do_GetInterface(docShellTreeItem);
        if (commandManager) {
          commandManager->RemoveCommandObserver(this, "obs_documentCreated");
        }
      }
    }
  }

  if (mScrollWatchTimer) {
    mScrollWatchTimer->Cancel();
    mScrollWatchTimer = nullptr;
    NS_RELEASE_THIS(); 
  }

  SelectionMgr()->RemoveDocSelectionListener(mPresShell);
  return NS_OK;
}

void
DocAccessible::ScrollTimerCallback(nsITimer* aTimer, void* aClosure)
{
  DocAccessible* docAcc = reinterpret_cast<DocAccessible*>(aClosure);

  if (docAcc && docAcc->mScrollPositionChangedTicks &&
      ++docAcc->mScrollPositionChangedTicks > 2) {
    
    
    
    
    nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_SCROLLING_END, docAcc);

    docAcc->mScrollPositionChangedTicks = 0;
    if (docAcc->mScrollWatchTimer) {
      docAcc->mScrollWatchTimer->Cancel();
      docAcc->mScrollWatchTimer = nullptr;
      NS_RELEASE(docAcc); 
    }
  }
}




void
DocAccessible::ScrollPositionDidChange(nscoord aX, nscoord aY)
{
  
  
  const uint32_t kScrollPosCheckWait = 50;
  if (mScrollWatchTimer) {
    mScrollWatchTimer->SetDelay(kScrollPosCheckWait);  
  }
  else {
    mScrollWatchTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (mScrollWatchTimer) {
      NS_ADDREF_THIS(); 
      mScrollWatchTimer->InitWithFuncCallback(ScrollTimerCallback, this,
                                              kScrollPosCheckWait,
                                              nsITimer::TYPE_REPEATING_SLACK);
    }
  }
  mScrollPositionChangedTicks = 1;
}




NS_IMETHODIMP
DocAccessible::Observe(nsISupports* aSubject, const char* aTopic,
                       const char16_t* aData)
{
  if (!nsCRT::strcmp(aTopic,"obs_documentCreated")) {    
    
    
    
    
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(this, states::EDITABLE, true);
    FireDelayedEvent(event);
  }

  return NS_OK;
}




NS_IMETHODIMP
DocAccessible::OnPivotChanged(nsIAccessiblePivot* aPivot,
                              nsIAccessible* aOldAccessible,
                              int32_t aOldStart, int32_t aOldEnd,
                              PivotMoveReason aReason,
                              bool aIsFromUserInput)
{
  nsRefPtr<AccEvent> event = new AccVCChangeEvent(
    this, aOldAccessible, aOldStart, aOldEnd, aReason,
    aIsFromUserInput ? eFromUserInput : eNoUserInput);
  nsEventShell::FireEvent(event);

  return NS_OK;
}




NS_IMPL_NSIDOCUMENTOBSERVER_CORE_STUB(DocAccessible)
NS_IMPL_NSIDOCUMENTOBSERVER_LOAD_STUB(DocAccessible)
NS_IMPL_NSIDOCUMENTOBSERVER_STYLE_STUB(DocAccessible)

void
DocAccessible::AttributeWillChange(nsIDocument* aDocument,
                                   dom::Element* aElement,
                                   int32_t aNameSpaceID,
                                   nsIAtom* aAttribute, int32_t aModType)
{
  Accessible* accessible = GetAccessible(aElement);
  if (!accessible) {
    if (aElement != mContent)
      return;

    accessible = this;
  }

  
  
  
  if (aModType != nsIDOMMutationEvent::ADDITION)
    RemoveDependentIDsFor(aElement, aAttribute);

  
  
  
  

  
  
  
  
  if (aAttribute == nsGkAtoms::aria_checked ||
      aAttribute == nsGkAtoms::aria_pressed) {
    mARIAAttrOldValue = (aModType != nsIDOMMutationEvent::ADDITION) ?
      nsAccUtils::GetARIAToken(aElement, aAttribute) : nullptr;
    return;
  }

  if (aAttribute == nsGkAtoms::aria_disabled ||
      aAttribute == nsGkAtoms::disabled)
    mStateBitWasOn = accessible->Unavailable();
}

void
DocAccessible::AttributeChanged(nsIDocument* aDocument,
                                dom::Element* aElement,
                                int32_t aNameSpaceID, nsIAtom* aAttribute,
                                int32_t aModType)
{
  NS_ASSERTION(!IsDefunct(),
               "Attribute changed called on defunct document accessible!");

  
  
  if (UpdateAccessibleOnAttrChange(aElement, aAttribute))
    return;

  
  
  
  
  
  Accessible* accessible = GetAccessible(aElement);
  if (!accessible) {
    if (mContent != aElement)
      return;

    accessible = this;
  }

  
  
  AttributeChangedImpl(accessible, aNameSpaceID, aAttribute);

  
  
  
  
  
  if (aModType == nsIDOMMutationEvent::MODIFICATION ||
      aModType == nsIDOMMutationEvent::ADDITION) {
    AddDependentIDsFor(aElement, aAttribute);
  }
}


void
DocAccessible::AttributeChangedImpl(Accessible* aAccessible,
                                    int32_t aNameSpaceID, nsIAtom* aAttribute)
{
  
  
  

  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  if (aAttribute == nsGkAtoms::disabled ||
      aAttribute == nsGkAtoms::aria_disabled) {
    
    
    if (aAccessible->Unavailable() == mStateBitWasOn)
      return;

    nsRefPtr<AccEvent> enabledChangeEvent =
      new AccStateChangeEvent(aAccessible, states::ENABLED, mStateBitWasOn);
    FireDelayedEvent(enabledChangeEvent);

    nsRefPtr<AccEvent> sensitiveChangeEvent =
      new AccStateChangeEvent(aAccessible, states::SENSITIVE, mStateBitWasOn);
    FireDelayedEvent(sensitiveChangeEvent);
    return;
  }

  
  if (aNameSpaceID == kNameSpaceID_None) {
    
    if (StringBeginsWith(nsDependentAtomString(aAttribute),
                         NS_LITERAL_STRING("aria-"))) {
      ARIAAttributeChanged(aAccessible, aAttribute);
    }
  }

  
  
  
  if (aAttribute == nsGkAtoms::aria_label) {
    FireDelayedEvent(nsIAccessibleEvent::EVENT_NAME_CHANGE, aAccessible);
    return;
  }

  if (aAttribute == nsGkAtoms::aria_describedby) {
    FireDelayedEvent(nsIAccessibleEvent::EVENT_DESCRIPTION_CHANGE, aAccessible);
    return;
  }

  nsIContent* elm = aAccessible->GetContent();
  if (aAttribute == nsGkAtoms::aria_labelledby &&
      !elm->HasAttr(kNameSpaceID_None, nsGkAtoms::aria_label)) {
    FireDelayedEvent(nsIAccessibleEvent::EVENT_NAME_CHANGE, aAccessible);
    return;
  }

  if (aAttribute == nsGkAtoms::alt &&
      !elm->HasAttr(kNameSpaceID_None, nsGkAtoms::aria_label) &&
      !elm->HasAttr(kNameSpaceID_None, nsGkAtoms::aria_labelledby)) {
    FireDelayedEvent(nsIAccessibleEvent::EVENT_NAME_CHANGE, aAccessible);
    return;
  }

  if (aAttribute == nsGkAtoms::title) {
    if (!elm->HasAttr(kNameSpaceID_None, nsGkAtoms::aria_label) &&
        !elm->HasAttr(kNameSpaceID_None, nsGkAtoms::aria_labelledby) &&
        !elm->HasAttr(kNameSpaceID_None, nsGkAtoms::alt)) {
      FireDelayedEvent(nsIAccessibleEvent::EVENT_NAME_CHANGE, aAccessible);
      return;
    }

    if (!elm->HasAttr(kNameSpaceID_None, nsGkAtoms::aria_describedby))
      FireDelayedEvent(nsIAccessibleEvent::EVENT_DESCRIPTION_CHANGE, aAccessible);

    return;
  }

  if (aAttribute == nsGkAtoms::aria_busy) {
    bool isOn = elm->AttrValueIs(aNameSpaceID, aAttribute, nsGkAtoms::_true,
                                 eCaseMatters);
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(aAccessible, states::BUSY, isOn);
    FireDelayedEvent(event);
    return;
  }

  
  if ((aAccessible->GetContent()->IsXUL() && aAttribute == nsGkAtoms::selected) ||
      aAttribute == nsGkAtoms::aria_selected) {
    Accessible* widget =
      nsAccUtils::GetSelectableContainer(aAccessible, aAccessible->State());
    if (widget) {
      AccSelChangeEvent::SelChangeType selChangeType =
        elm->AttrValueIs(aNameSpaceID, aAttribute, nsGkAtoms::_true, eCaseMatters) ?
          AccSelChangeEvent::eSelectionAdd : AccSelChangeEvent::eSelectionRemove;

      nsRefPtr<AccEvent> event =
        new AccSelChangeEvent(widget, aAccessible, selChangeType);
      FireDelayedEvent(event);
    }

    return;
  }

  if (aAttribute == nsGkAtoms::contenteditable) {
    nsRefPtr<AccEvent> editableChangeEvent =
      new AccStateChangeEvent(aAccessible, states::EDITABLE);
    FireDelayedEvent(editableChangeEvent);
    return;
  }

  if (aAttribute == nsGkAtoms::value) {
    if (aAccessible->IsProgress())
      FireDelayedEvent(nsIAccessibleEvent::EVENT_VALUE_CHANGE, aAccessible);
  }
}


void
DocAccessible::ARIAAttributeChanged(Accessible* aAccessible, nsIAtom* aAttribute)
{
  
  

  if (aAttribute == nsGkAtoms::aria_required) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(aAccessible, states::REQUIRED);
    FireDelayedEvent(event);
    return;
  }

  if (aAttribute == nsGkAtoms::aria_invalid) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(aAccessible, states::INVALID);
    FireDelayedEvent(event);
    return;
  }

  
  
  
  if (aAttribute == nsGkAtoms::aria_activedescendant) {
    mNotificationController->HandleNotification<DocAccessible, Accessible>
      (this, &DocAccessible::ARIAActiveDescendantChanged, aAccessible);

    return;
  }

  
  if (aAttribute == nsGkAtoms::aria_expanded) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(aAccessible, states::EXPANDED);
    FireDelayedEvent(event);
    return;
  }

  
  
  uint8_t attrFlags = aria::AttrCharacteristicsFor(aAttribute);
  if (!(attrFlags & ATTR_BYPASSOBJ)) {
    nsRefPtr<AccEvent> event =
      new AccObjectAttrChangedEvent(aAccessible, aAttribute);
    FireDelayedEvent(event);
  }

  nsIContent* elm = aAccessible->GetContent();

  if (aAttribute == nsGkAtoms::aria_checked ||
      (aAccessible->IsButton() &&
       aAttribute == nsGkAtoms::aria_pressed)) {
    const uint64_t kState = (aAttribute == nsGkAtoms::aria_checked) ?
                            states::CHECKED : states::PRESSED;
    nsRefPtr<AccEvent> event = new AccStateChangeEvent(aAccessible, kState);
    FireDelayedEvent(event);

    bool wasMixed = (mARIAAttrOldValue == nsGkAtoms::mixed);
    bool isMixed = elm->AttrValueIs(kNameSpaceID_None, aAttribute,
                                    nsGkAtoms::mixed, eCaseMatters);
    if (isMixed != wasMixed) {
      nsRefPtr<AccEvent> event =
        new AccStateChangeEvent(aAccessible, states::MIXED, isMixed);
      FireDelayedEvent(event);
    }
    return;
  }

  if (aAttribute == nsGkAtoms::aria_readonly) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(aAccessible, states::READONLY);
    FireDelayedEvent(event);
    return;
  }

  
  
  if (aAttribute == nsGkAtoms::aria_valuetext ||
      (aAttribute == nsGkAtoms::aria_valuenow &&
       (!elm->HasAttr(kNameSpaceID_None, nsGkAtoms::aria_valuetext) ||
        elm->AttrValueIs(kNameSpaceID_None, nsGkAtoms::aria_valuetext,
                         nsGkAtoms::_empty, eCaseMatters)))) {
    FireDelayedEvent(nsIAccessibleEvent::EVENT_VALUE_CHANGE, aAccessible);
    return;
  }
}

void
DocAccessible::ARIAActiveDescendantChanged(Accessible* aAccessible)
{
  nsIContent* elm = aAccessible->GetContent();
  if (elm && aAccessible->IsActiveWidget()) {
    nsAutoString id;
    if (elm->GetAttr(kNameSpaceID_None, nsGkAtoms::aria_activedescendant, id)) {
      dom::Element* activeDescendantElm = elm->OwnerDoc()->GetElementById(id);
      if (activeDescendantElm) {
        Accessible* activeDescendant = GetAccessible(activeDescendantElm);
        if (activeDescendant) {
          FocusMgr()->ActiveItemChanged(activeDescendant, false);
#ifdef A11Y_LOG
          if (logging::IsEnabled(logging::eFocus))
            logging::ActiveItemChangeCausedBy("ARIA activedescedant changed",
                                              activeDescendant);
#endif
        }
      }
    }
  }
}

void
DocAccessible::ContentAppended(nsIDocument* aDocument,
                               nsIContent* aContainer,
                               nsIContent* aFirstNewContent,
                               int32_t )
{
}

void
DocAccessible::ContentStateChanged(nsIDocument* aDocument,
                                   nsIContent* aContent,
                                   EventStates aStateMask)
{
  Accessible* accessible = GetAccessible(aContent);
  if (!accessible)
    return;

  if (aStateMask.HasState(NS_EVENT_STATE_CHECKED)) {
    Accessible* widget = accessible->ContainerWidget();
    if (widget && widget->IsSelect()) {
      AccSelChangeEvent::SelChangeType selChangeType =
        aContent->AsElement()->State().HasState(NS_EVENT_STATE_CHECKED) ?
          AccSelChangeEvent::eSelectionAdd : AccSelChangeEvent::eSelectionRemove;
      nsRefPtr<AccEvent> event =
        new AccSelChangeEvent(widget, accessible, selChangeType);
      FireDelayedEvent(event);
      return;
    }

    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(accessible, states::CHECKED,
                              aContent->AsElement()->State().HasState(NS_EVENT_STATE_CHECKED));
    FireDelayedEvent(event);
  }

  if (aStateMask.HasState(NS_EVENT_STATE_INVALID)) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(accessible, states::INVALID, true);
    FireDelayedEvent(event);
  }

  if (aStateMask.HasState(NS_EVENT_STATE_VISITED)) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(accessible, states::TRAVERSED, true);
    FireDelayedEvent(event);
  }
}

void
DocAccessible::DocumentStatesChanged(nsIDocument* aDocument,
                                     EventStates aStateMask)
{
}

void
DocAccessible::CharacterDataWillChange(nsIDocument* aDocument,
                                       nsIContent* aContent,
                                       CharacterDataChangeInfo* aInfo)
{
}

void
DocAccessible::CharacterDataChanged(nsIDocument* aDocument,
                                    nsIContent* aContent,
                                    CharacterDataChangeInfo* aInfo)
{
}

void
DocAccessible::ContentInserted(nsIDocument* aDocument, nsIContent* aContainer,
                               nsIContent* aChild, int32_t )
{
}

void
DocAccessible::ContentRemoved(nsIDocument* aDocument, nsIContent* aContainer,
                              nsIContent* aChild, int32_t ,
                              nsIContent* aPreviousSibling)
{
}

void
DocAccessible::ParentChainChanged(nsIContent* aContent)
{
}





#ifdef A11Y_LOG
nsresult
DocAccessible::HandleAccEvent(AccEvent* aEvent)
{
  if (logging::IsEnabled(logging::eDocLoad))
    logging::DocLoadEventHandled(aEvent);

  return HyperTextAccessible::HandleAccEvent(aEvent);
}
#endif




void*
DocAccessible::GetNativeWindow() const
{
  if (!mPresShell)
    return nullptr;

  nsViewManager* vm = mPresShell->GetViewManager();
  if (!vm)
    return nullptr;

  nsCOMPtr<nsIWidget> widget;
  vm->GetRootWidget(getter_AddRefs(widget));
  if (widget)
    return widget->GetNativeData(NS_NATIVE_WINDOW);

  return nullptr;
}

Accessible*
DocAccessible::GetAccessibleByUniqueIDInSubtree(void* aUniqueID)
{
  Accessible* child = GetAccessibleByUniqueID(aUniqueID);
  if (child)
    return child;

  uint32_t childDocCount = mChildDocuments.Length();
  for (uint32_t childDocIdx= 0; childDocIdx < childDocCount; childDocIdx++) {
    DocAccessible* childDocument = mChildDocuments.ElementAt(childDocIdx);
    child = childDocument->GetAccessibleByUniqueIDInSubtree(aUniqueID);
    if (child)
      return child;
  }

  return nullptr;
}

Accessible*
DocAccessible::GetAccessibleOrContainer(nsINode* aNode) const
{
  if (!aNode || !aNode->GetCrossShadowCurrentDoc())
    return nullptr;

  nsINode* currNode = aNode;
  Accessible* accessible = nullptr;
  while (!(accessible = GetAccessible(currNode))) {
    nsINode* parent = nullptr;

    
    
    
    if (currNode->IsContent())
      parent = currNode->AsContent()->GetFlattenedTreeParent();

    
    
    if (!parent)
      parent = currNode->GetParentNode();

    if (!(currNode = parent)) break;
  }

  return accessible;
}

Accessible*
DocAccessible::GetAccessibleOrDescendant(nsINode* aNode) const
{
  Accessible* acc = GetAccessible(aNode);
  if (acc)
    return acc;

  acc = GetContainerAccessible(aNode);
  if (acc) {
    uint32_t childCnt = acc->ChildCount();
    for (uint32_t idx = 0; idx < childCnt; idx++) {
      Accessible* child = acc->GetChildAt(idx);
      for (nsIContent* elm = child->GetContent();
           elm && elm != acc->GetContent();
           elm = elm->GetFlattenedTreeParent()) {
        if (elm == aNode)
          return child;
      }
    }
  }

  return nullptr;
}

void
DocAccessible::BindToDocument(Accessible* aAccessible,
                              nsRoleMapEntry* aRoleMapEntry)
{
  
  if (aAccessible->IsNodeMapEntry())
    mNodeToAccessibleMap.Put(aAccessible->GetNode(), aAccessible);

  
  mAccessibleCache.Put(aAccessible->UniqueID(), aAccessible);

  aAccessible->SetRoleMapEntry(aRoleMapEntry);

  nsIContent* content = aAccessible->GetContent();
  if (content && content->IsElement())
    AddDependentIDsFor(content->AsElement());
}

void
DocAccessible::UnbindFromDocument(Accessible* aAccessible)
{
  NS_ASSERTION(mAccessibleCache.GetWeak(aAccessible->UniqueID()),
               "Unbinding the unbound accessible!");

  
  
  if (FocusMgr()->IsActiveItem(aAccessible)) {
    FocusMgr()->ActiveItemChanged(nullptr);
#ifdef A11Y_LOG
          if (logging::IsEnabled(logging::eFocus))
            logging::ActiveItemChangeCausedBy("tree shutdown", aAccessible);
#endif
  }

  
  if (aAccessible->IsNodeMapEntry() &&
      mNodeToAccessibleMap.Get(aAccessible->GetNode()) == aAccessible)
    mNodeToAccessibleMap.Remove(aAccessible->GetNode());

  void* uniqueID = aAccessible->UniqueID();

  NS_ASSERTION(!aAccessible->IsDefunct(), "Shutdown the shutdown accessible!");
  aAccessible->Shutdown();

  mAccessibleCache.Remove(uniqueID);
}

void
DocAccessible::ContentInserted(nsIContent* aContainerNode,
                               nsIContent* aStartChildNode,
                               nsIContent* aEndChildNode)
{
  
  
  if (mNotificationController && HasLoadState(eTreeConstructed)) {
    
    
    Accessible* container = aContainerNode ?
      GetAccessibleOrContainer(aContainerNode) : this;

    mNotificationController->ScheduleContentInsertion(container,
                                                      aStartChildNode,
                                                      aEndChildNode);
  }
}

void
DocAccessible::ContentRemoved(nsIContent* aContainerNode,
                              nsIContent* aChildNode)
{
  
  
  Accessible* container = aContainerNode ?
    GetAccessibleOrContainer(aContainerNode) : this;

  UpdateTree(container, aChildNode, false);
}

void
DocAccessible::RecreateAccessible(nsIContent* aContent)
{
#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eTree)) {
    logging::MsgBegin("TREE", "accessible recreated");
    logging::Node("content", aContent);
    logging::MsgEnd();
  }
#endif

  
  
  
  

  nsIContent* parent = aContent->GetFlattenedTreeParent();
  ContentRemoved(parent, aContent);
  ContentInserted(parent, aContent, aContent->GetNextSibling());
}

void
DocAccessible::ProcessInvalidationList()
{
  
  
  
  for (uint32_t idx = 0; idx < mInvalidationList.Length(); idx++) {
    nsIContent* content = mInvalidationList[idx];
    Accessible* accessible = GetAccessible(content);
    if (!accessible) {
      Accessible* container = GetContainerAccessible(content);
      if (container) {
        container->UpdateChildren();
        accessible = GetAccessible(content);
      }
    }

    
    if (accessible)
      CacheChildrenInSubtree(accessible);
  }

  mInvalidationList.Clear();
}

Accessible*
DocAccessible::GetAccessibleEvenIfNotInMap(nsINode* aNode) const
{
if (!aNode->IsContent() || !aNode->AsContent()->IsHTML(nsGkAtoms::area))
    return GetAccessible(aNode);

  
  nsIFrame* frame = aNode->AsContent()->GetPrimaryFrame();
  nsImageFrame* imageFrame = do_QueryFrame(frame);
  if (imageFrame) {
    Accessible* parent = GetAccessible(imageFrame->GetContent());
    if (parent) {
      Accessible* area =
        parent->AsImageMap()->GetChildAccessibleFor(aNode);
      if (area)
        return area;

      return nullptr;
    }
  }

  return GetAccessible(aNode);
}




void
DocAccessible::CacheChildren()
{
  
  
  dom::Element* rootElm = mDocumentNode->GetRootElement();
  if (!rootElm)
    return;

  
  TreeWalker walker(this, rootElm);
  Accessible* lastChild = nullptr;
  while (Accessible* child = walker.NextChild()) {
    if (lastChild)
      AppendChild(lastChild);

    lastChild = child;
  }

  if (lastChild) {
    if (lastChild->IsHTMLBr())
      Document()->UnbindFromDocument(lastChild);
    else
      AppendChild(lastChild);
  }
}




void
DocAccessible::NotifyOfLoading(bool aIsReloading)
{
  
  
  mLoadState &= ~eDOMLoaded;

  if (!IsLoadEventTarget())
    return;

  if (aIsReloading) {
    
    
    
    nsRefPtr<AccEvent> reloadEvent =
      new AccEvent(nsIAccessibleEvent::EVENT_DOCUMENT_RELOAD, this);
    nsEventShell::FireEvent(reloadEvent);
  }

  
  
  nsRefPtr<AccEvent> stateEvent =
    new AccStateChangeEvent(this, states::BUSY, true);
  FireDelayedEvent(stateEvent);
}

void
DocAccessible::DoInitialUpdate()
{
  if (nsCoreUtils::IsTabDocument(mDocumentNode))
    mDocFlags |= eTabDocument;

  mLoadState |= eTreeConstructed;

  
  
  
  nsIContent* contentElm = nsCoreUtils::GetRoleContent(mDocumentNode);
  if (mContent != contentElm) {
    mContent = contentElm;
    SetRoleMapEntry(aria::GetRoleMap(mContent));
  }

  
  CacheChildrenInSubtree(this);

  
  
  
  
  if (!IsRoot()) {
    nsRefPtr<AccReorderEvent> reorderEvent = new AccReorderEvent(Parent());
    ParentDocument()->FireDelayedEvent(reorderEvent);
  }
}

void
DocAccessible::ProcessLoad()
{
  mLoadState |= eCompletelyLoaded;

#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eDocLoad))
    logging::DocCompleteLoad(this, IsLoadEventTarget());
#endif

  
  
  
  
  
  
  if (!IsLoadEventTarget())
    return;

  
  if (mLoadEventType) {
    nsRefPtr<AccEvent> loadEvent = new AccEvent(mLoadEventType, this);
    FireDelayedEvent(loadEvent);

    mLoadEventType = 0;
  }

  
  nsRefPtr<AccEvent> stateEvent =
    new AccStateChangeEvent(this, states::BUSY, false);
  FireDelayedEvent(stateEvent);
}

void
DocAccessible::AddDependentIDsFor(dom::Element* aRelProviderElm,
                                  nsIAtom* aRelAttr)
{
  for (uint32_t idx = 0; idx < kRelationAttrsLen; idx++) {
    nsIAtom* relAttr = *kRelationAttrs[idx];
    if (aRelAttr && aRelAttr != relAttr)
      continue;

    if (relAttr == nsGkAtoms::_for) {
      if (!aRelProviderElm->IsHTML() ||
          (aRelProviderElm->Tag() != nsGkAtoms::label &&
           aRelProviderElm->Tag() != nsGkAtoms::output))
        continue;

    } else if (relAttr == nsGkAtoms::control) {
      if (!aRelProviderElm->IsXUL() ||
          (aRelProviderElm->Tag() != nsGkAtoms::label &&
           aRelProviderElm->Tag() != nsGkAtoms::description))
        continue;
    }

    IDRefsIterator iter(this, aRelProviderElm, relAttr);
    while (true) {
      const nsDependentSubstring id = iter.NextID();
      if (id.IsEmpty())
        break;

      AttrRelProviderArray* providers = mDependentIDsHash.Get(id);
      if (!providers) {
        providers = new AttrRelProviderArray();
        if (providers) {
          mDependentIDsHash.Put(id, providers);
        }
      }

      if (providers) {
        AttrRelProvider* provider =
          new AttrRelProvider(relAttr, aRelProviderElm);
        if (provider) {
          providers->AppendElement(provider);

          
          
          
          
          nsIContent* dependentContent = iter.GetElem(id);
          if (dependentContent && !HasAccessible(dependentContent)) {
            mInvalidationList.AppendElement(dependentContent);
          }
        }
      }
    }

    
    
    if (aRelAttr)
      break;
  }
}

void
DocAccessible::RemoveDependentIDsFor(dom::Element* aRelProviderElm,
                                     nsIAtom* aRelAttr)
{
  for (uint32_t idx = 0; idx < kRelationAttrsLen; idx++) {
    nsIAtom* relAttr = *kRelationAttrs[idx];
    if (aRelAttr && aRelAttr != *kRelationAttrs[idx])
      continue;

    IDRefsIterator iter(this, aRelProviderElm, relAttr);
    while (true) {
      const nsDependentSubstring id = iter.NextID();
      if (id.IsEmpty())
        break;

      AttrRelProviderArray* providers = mDependentIDsHash.Get(id);
      if (providers) {
        for (uint32_t jdx = 0; jdx < providers->Length(); ) {
          AttrRelProvider* provider = (*providers)[jdx];
          if (provider->mRelAttr == relAttr &&
              provider->mContent == aRelProviderElm)
            providers->RemoveElement(provider);
          else
            jdx++;
        }
        if (providers->Length() == 0)
          mDependentIDsHash.Remove(id);
      }
    }

    
    
    if (aRelAttr)
      break;
  }
}

bool
DocAccessible::UpdateAccessibleOnAttrChange(dom::Element* aElement,
                                            nsIAtom* aAttribute)
{
  if (aAttribute == nsGkAtoms::role) {
    
    
    if (mContent == aElement) {
      SetRoleMapEntry(aria::GetRoleMap(aElement));
      return true;
    }

    
    
    
    RecreateAccessible(aElement);

    return true;
  }

  if (aAttribute == nsGkAtoms::href ||
      aAttribute == nsGkAtoms::onclick) {
    
    
    

    
    
    RecreateAccessible(aElement);
    return true;
  }

  if (aAttribute == nsGkAtoms::aria_multiselectable &&
      aElement->HasAttr(kNameSpaceID_None, nsGkAtoms::role)) {
    
    
    
    RecreateAccessible(aElement);

    return true;
  }

  return false;
}

void
DocAccessible::ProcessContentInserted(Accessible* aContainer,
                                      const nsTArray<nsCOMPtr<nsIContent> >* aInsertedContent)
{
  
  if (!HasAccessible(aContainer->GetNode()))
    return;

  bool containerNotUpdated = true;

  for (uint32_t idx = 0; idx < aInsertedContent->Length(); idx++) {
    
    
    
    
    
    
    

    Accessible* presentContainer =
      GetContainerAccessible(aInsertedContent->ElementAt(idx));
    if (presentContainer != aContainer)
      continue;

    if (containerNotUpdated) {
      containerNotUpdated = false;

      if (aContainer == this) {
        
        nsIContent* rootContent = nsCoreUtils::GetRoleContent(mDocumentNode);
        if (rootContent != mContent) {
          mContent = rootContent;
          SetRoleMapEntry(aria::GetRoleMap(mContent));
        }

        
        
        
      }

      
      
      
      
      
      aContainer->InvalidateChildren();
      CacheChildrenInSubtree(aContainer);
    }

    UpdateTree(aContainer, aInsertedContent->ElementAt(idx), true);
  }
}

void
DocAccessible::UpdateTree(Accessible* aContainer, nsIContent* aChildNode,
                          bool aIsInsert)
{
  uint32_t updateFlags = eNoAccessible;

  
  Accessible* child = GetAccessible(aChildNode);
#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eTree)) {
    logging::MsgBegin("TREE", "process content %s",
                      (aIsInsert ? "insertion" : "removal"));
    logging::Node("container", aContainer->GetNode());
    logging::Node("child", aChildNode);
    if (child)
      logging::Address("child", child);
    else
      logging::MsgEntry("child accessible: null");

    logging::MsgEnd();
  }
#endif

  nsRefPtr<AccReorderEvent> reorderEvent = new AccReorderEvent(aContainer);

  if (child) {
    updateFlags |= UpdateTreeInternal(child, aIsInsert, reorderEvent);
  } else {
    if (aIsInsert) {
      TreeWalker walker(aContainer, aChildNode, TreeWalker::eWalkCache);

      while ((child = walker.NextChild()))
        updateFlags |= UpdateTreeInternal(child, aIsInsert, reorderEvent);
    } else {
      
      
      
      
      
      
      
      nsINode* containerNode = aContainer->GetNode();
      for (uint32_t idx = 0; idx < aContainer->ContentChildCount();) {
        Accessible* child = aContainer->ContentChildAt(idx);

        
        
        
        if (!child->HasOwnContent() || child->IsDoc()) {
          idx++;
          continue;
        }

        nsINode* childNode = child->GetContent();
        while (childNode != aChildNode && childNode != containerNode &&
               (childNode = childNode->GetParentNode()));

        if (childNode != containerNode) {
          updateFlags |= UpdateTreeInternal(child, false, reorderEvent);
        } else {
          idx++;
        }
      }
    }
  }

  
  if (updateFlags == eNoAccessible)
    return;

  
  
  if (aIsInsert && !(updateFlags & eAlertAccessible)) {
    
    
    Accessible* ancestor = aContainer;
    while (ancestor) {
      if (ancestor->ARIARole() == roles::ALERT) {
        FireDelayedEvent(nsIAccessibleEvent::EVENT_ALERT, ancestor);
        break;
      }

      
      if (ancestor == this)
        break;

      ancestor = ancestor->Parent();
    }
  }

  MaybeNotifyOfValueChange(aContainer);

  
  
  FireDelayedEvent(reorderEvent);
}

uint32_t
DocAccessible::UpdateTreeInternal(Accessible* aChild, bool aIsInsert,
                                  AccReorderEvent* aReorderEvent)
{
  uint32_t updateFlags = eAccessible;

  
  
  
  
  Accessible* focusedAcc = nullptr;

  nsINode* node = aChild->GetNode();
  if (aIsInsert) {
    
    CacheChildrenInSubtree(aChild, &focusedAcc);

  } else {
    

    
    
    
    
    
    
    
    if (aChild->ARIARole() == roles::MENUPOPUP)
      FireDelayedEvent(nsIAccessibleEvent::EVENT_MENUPOPUP_END, aChild);
  }

  
  nsRefPtr<AccMutationEvent> event;
  if (aIsInsert)
    event = new AccShowEvent(aChild, node);
  else
    event = new AccHideEvent(aChild, node);

  FireDelayedEvent(event);
  aReorderEvent->AddSubMutationEvent(event);

  if (aIsInsert) {
    roles::Role ariaRole = aChild->ARIARole();
    if (ariaRole == roles::MENUPOPUP) {
      
      FireDelayedEvent(nsIAccessibleEvent::EVENT_MENUPOPUP_START, aChild);

    } else if (ariaRole == roles::ALERT) {
      
      updateFlags = eAlertAccessible;
      FireDelayedEvent(nsIAccessibleEvent::EVENT_ALERT, aChild);
    }
  } else {
    
    
    
    
    Accessible* parent = aChild->Parent();
    NS_ASSERTION(parent, "No accessible parent?!");
    if (parent)
      parent->RemoveChild(aChild);

    UncacheChildrenInSubtree(aChild);
  }

  
  
  if (focusedAcc) {
    FocusMgr()->DispatchFocusEvent(this, focusedAcc);
    SelectionMgr()->SetControlSelectionListener(focusedAcc->GetNode()->AsElement());
  }

  return updateFlags;
}

void
DocAccessible::CacheChildrenInSubtree(Accessible* aRoot,
                                      Accessible** aFocusedAcc)
{
  
  
  if (aFocusedAcc && !*aFocusedAcc &&
      FocusMgr()->HasDOMFocus(aRoot->GetContent()))
    *aFocusedAcc = aRoot;

  aRoot->EnsureChildren();

  
  
  
  uint32_t count = aRoot->ContentChildCount();
  for (uint32_t idx = 0; idx < count; idx++) {
    Accessible* child = aRoot->ContentChildAt(idx);
    NS_ASSERTION(child, "Illicit tree change while tree is created!");
    
    if (child && child->IsContent())
      CacheChildrenInSubtree(child, aFocusedAcc);
  }

  
  
  if (aRoot->HasARIARole() && !aRoot->IsDoc()) {
    a11y::role role = aRoot->ARIARole();
    if (role == roles::DIALOG || role == roles::DOCUMENT)
      FireDelayedEvent(nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_COMPLETE, aRoot);
  }
}

void
DocAccessible::UncacheChildrenInSubtree(Accessible* aRoot)
{
  aRoot->mStateFlags |= eIsNotInDocument;

  nsIContent* rootContent = aRoot->GetContent();
  if (rootContent && rootContent->IsElement())
    RemoveDependentIDsFor(rootContent->AsElement());

  uint32_t count = aRoot->ContentChildCount();
  for (uint32_t idx = 0; idx < count; idx++)
    UncacheChildrenInSubtree(aRoot->ContentChildAt(idx));

  if (aRoot->IsNodeMapEntry() &&
      mNodeToAccessibleMap.Get(aRoot->GetNode()) == aRoot)
    mNodeToAccessibleMap.Remove(aRoot->GetNode());
}

void
DocAccessible::ShutdownChildrenInSubtree(Accessible* aAccessible)
{
  
  
  
  
  uint32_t count = aAccessible->ContentChildCount();
  for (uint32_t idx = 0, jdx = 0; idx < count; idx++) {
    Accessible* child = aAccessible->ContentChildAt(jdx);
    if (!child->IsBoundToParent()) {
      NS_ERROR("Parent refers to a child, child doesn't refer to parent!");
      jdx++;
    }

    
    
    if (!child->IsDoc())
      ShutdownChildrenInSubtree(child);
  }

  UnbindFromDocument(aAccessible);
}

bool
DocAccessible::IsLoadEventTarget() const
{
  nsCOMPtr<nsIDocShellTreeItem> treeItem = mDocumentNode->GetDocShell();
  NS_ASSERTION(treeItem, "No document shell for document!");

  nsCOMPtr<nsIDocShellTreeItem> parentTreeItem;
  treeItem->GetParent(getter_AddRefs(parentTreeItem));

  
  if (parentTreeItem) {
    
    
    nsCOMPtr<nsIDocShellTreeItem> rootTreeItem;
    treeItem->GetRootTreeItem(getter_AddRefs(rootTreeItem));
    if (parentTreeItem == rootTreeItem)
      return true;

    
    
    
    DocAccessible* parentDoc = ParentDocument();
    return parentDoc && parentDoc->HasLoadState(eCompletelyLoaded);
  }

  
  return (treeItem->ItemType() == nsIDocShellTreeItem::typeContent);
}

PLDHashOperator
DocAccessible::CycleCollectorTraverseDepIDsEntry(const nsAString& aKey,
                                                 AttrRelProviderArray* aProviders,
                                                 void* aUserArg)
{
  nsCycleCollectionTraversalCallback* cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aUserArg);

  for (int32_t jdx = aProviders->Length() - 1; jdx >= 0; jdx--) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb,
                                       "content of dependent ids hash entry of document accessible");

    AttrRelProvider* provider = (*aProviders)[jdx];
    cb->NoteXPCOMChild(provider->mContent);

    NS_ASSERTION(provider->mContent->IsInDoc(),
                 "Referred content is not in document!");
  }

  return PL_DHASH_NEXT;
}

