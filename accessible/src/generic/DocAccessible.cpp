




#include "Accessible-inl.h"
#include "AccIterator.h"
#include "DocAccessible-inl.h"
#include "nsAccCache.h"
#include "nsAccessibilityService.h"
#include "nsAccessiblePivot.h"
#include "nsAccTreeWalker.h"
#include "nsAccUtils.h"
#include "nsEventShell.h"
#include "nsTextEquivUtils.h"
#include "Role.h"
#include "RootAccessible.h"
#include "States.h"

#include "nsIMutableArray.h"
#include "nsICommandManager.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocument.h"
#include "nsIDOMAttr.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentType.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMMutationEvent.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMXULPopupElement.h"
#include "nsIEditingSession.h"
#include "nsEventStateManager.h"
#include "nsIFrame.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsINameSpaceManager.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"
#include "nsIViewManager.h"
#include "nsIScrollableFrame.h"
#include "nsUnicharUtils.h"
#include "nsIURI.h"
#include "nsIWebNavigation.h"
#include "nsFocusManager.h"
#include "mozilla/dom/Element.h"

#ifdef DEBUG
#include "Logging.h"
#endif

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

static const uint32_t kRelationAttrsLen = NS_ARRAY_LENGTH(kRelationAttrs);




DocAccessible::
  DocAccessible(nsIDocument* aDocument, nsIContent* aRootContent,
                  nsIPresShell* aPresShell) :
  HyperTextAccessibleWrap(aRootContent, this),
  mDocument(aDocument), mScrollPositionChangedTicks(0),
  mLoadState(eTreeConstructionPending), mLoadEventType(0),
  mVirtualCursor(nullptr),
  mPresShell(aPresShell)
{
  mFlags |= eDocAccessible;
  if (mPresShell)
    mPresShell->SetAccDocument(this);

  mDependentIDsHash.Init();
  
  mAccessibleCache.Init(kDefaultCacheSize);
  mNodeToAccessibleMap.Init(kDefaultCacheSize);

  
  if (mDocument && mDocument->IsXUL())
    mFlags &= ~eHyperTextAccessible;

  
  if (!mDocument)
    return;

  
  
  AddScrollListener();

  
  mIsCursorable = (!(mDocument->GetParentDocument()) ||
                   nsCoreUtils::IsTabDocument(mDocument));
}

DocAccessible::~DocAccessible()
{
  NS_ASSERTION(!mPresShell, "LastRelease was never called!?!");
}





NS_IMPL_CYCLE_COLLECTION_CLASS(DocAccessible)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(DocAccessible, Accessible)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_MEMBER(mNotificationController,
                                                  NotificationController)

  if (tmp->mVirtualCursor) {
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_MEMBER(mVirtualCursor,
                                                    nsAccessiblePivot)
  }

  uint32_t i, length = tmp->mChildDocuments.Length();
  for (i = 0; i < length; ++i) {
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mChildDocuments[i],
                                                         nsIAccessible)
  }

  CycleCollectorTraverseCache(tmp->mAccessibleCache, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(DocAccessible, Accessible)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mNotificationController)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mVirtualCursor)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSTARRAY(mChildDocuments)
  tmp->mDependentIDsHash.Clear();
  tmp->mNodeToAccessibleMap.Clear();
  ClearCache(tmp->mAccessibleCache);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(DocAccessible)
  NS_INTERFACE_MAP_ENTRY(nsIAccessibleDocument)
  NS_INTERFACE_MAP_ENTRY(nsIDocumentObserver)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsIAccessiblePivotObserver)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIAccessibleDocument)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIAccessibleCursorable,
                                     mIsCursorable)
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
  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
    nsCoreUtils::GetDocShellTreeItemFor(mDocument);
  if (docShellTreeItem) {
    nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
    docShellTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));
    int32_t itemType;
    docShellTreeItem->GetItemType(&itemType);
    if (sameTypeRoot == docShellTreeItem) {
      
      if (itemType == nsIDocShellTreeItem::typeChrome)
        return roles::CHROME_WINDOW;

      if (itemType == nsIDocShellTreeItem::typeContent) {
#ifdef MOZ_XUL
        nsCOMPtr<nsIXULDocument> xulDoc(do_QueryInterface(mDocument));
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
DocAccessible::SetRoleMapEntry(nsRoleMapEntry* aRoleMapEntry)
{
  NS_ASSERTION(mDocument, "No document during initialization!");
  if (!mDocument)
    return;

  mRoleMapEntry = aRoleMapEntry;

  nsIDocument *parentDoc = mDocument->GetParentDocument();
  if (!parentDoc)
    return; 

  
  nsIContent *ownerContent = parentDoc->FindContentForSubDocument(mDocument);
  if (ownerContent) {
    nsRoleMapEntry* roleMapEntry = aria::GetRoleMap(ownerContent);
    if (roleMapEntry)
      mRoleMapEntry = roleMapEntry; 
  }
}

void
DocAccessible::Description(nsString& aDescription)
{
  if (mParent)
    mParent->Description(aDescription);

  if (aDescription.IsEmpty())
    nsTextEquivUtils::
      GetTextEquivFromIDRefs(this, nsGkAtoms::aria_describedby,
                             aDescription);
}


uint64_t
DocAccessible::NativeState()
{
  
  
  uint64_t state = (mContent->GetCurrentDoc() == mDocument) ?
    0 : states::STALE;

  
  state |= states::FOCUSABLE; 
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
  
  
  Accessible::ApplyARIAState(aState);

  
  if (mParent)
    mParent->ApplyARIAState(aState);

}

NS_IMETHODIMP
DocAccessible::GetAttributes(nsIPersistentProperties** aAttributes)
{
  Accessible::GetAttributes(aAttributes);
  if (mParent) {
    mParent->GetAttributes(aAttributes); 
  }
  return NS_OK;
}

Accessible*
DocAccessible::FocusedChild()
{
  
  
  return FocusMgr()->FocusedAccessible();
}

NS_IMETHODIMP
DocAccessible::TakeFocus()
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ENSURE_STATE(fm);

  nsCOMPtr<nsIDOMElement> newFocus;
  return fm->MoveFocus(mDocument->GetWindow(), nullptr,
                       nsIFocusManager::MOVEFOCUS_ROOT, 0,
                       getter_AddRefs(newFocus));
}





NS_IMETHODIMP
DocAccessible::GetURL(nsAString& aURL)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsISupports> container = mDocument->GetContainer();
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
  nsCOMPtr<nsIDOMDocument> domDocument = do_QueryInterface(mDocument);
  if (!domDocument) {
    return NS_ERROR_FAILURE;
  }
  return domDocument->GetTitle(aTitle);
}

NS_IMETHODIMP
DocAccessible::GetMimeType(nsAString& aMimeType)
{
  nsCOMPtr<nsIDOMDocument> domDocument = do_QueryInterface(mDocument);
  if (!domDocument) {
    return NS_ERROR_FAILURE;
  }
  return domDocument->GetContentType(aMimeType);
}

NS_IMETHODIMP
DocAccessible::GetDocType(nsAString& aDocType)
{
  nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(mDocument));
  nsCOMPtr<nsIDOMDocumentType> docType;

#ifdef MOZ_XUL
  nsCOMPtr<nsIXULDocument> xulDoc(do_QueryInterface(mDocument));
  if (xulDoc) {
    aDocType.AssignLiteral("window"); 
    return NS_OK;
  } else
#endif
  if (domDoc && NS_SUCCEEDED(domDoc->GetDoctype(getter_AddRefs(docType))) && docType) {
    return docType->GetPublicId(aDocType);
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
DocAccessible::GetNameSpaceURIForID(int16_t aNameSpaceID, nsAString& aNameSpaceURI)
{
  if (mDocument) {
    nsCOMPtr<nsINameSpaceManager> nameSpaceManager =
        do_GetService(NS_NAMESPACEMANAGER_CONTRACTID);
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
  if (!mDocument) {
    return NS_ERROR_FAILURE;  
  }
  *aDOMWin = mDocument->GetWindow();

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

  if (mDocument)
    CallQueryInterface(mDocument, aDOMDocument);

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

  NS_ENSURE_TRUE(mIsCursorable, NS_ERROR_NOT_IMPLEMENTED);

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
  
  
  if (!mDocument->HasFlag(NODE_IS_EDITABLE) &&
      !mContent->HasFlag(NODE_IS_EDITABLE))
    return nullptr;

  nsCOMPtr<nsISupports> container = mDocument->GetContainer();
  nsCOMPtr<nsIEditingSession> editingSession(do_GetInterface(container));
  if (!editingSession)
    return nullptr; 

  nsCOMPtr<nsIEditor> editor;
  editingSession->GetEditorForWindow(mDocument->GetWindow(), getter_AddRefs(editor));
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
#ifdef DEBUG
  if (logging::IsEnabled(logging::eDocCreate))
    logging::DocCreate("document initialize", mDocument, this);
#endif

  
  mNotificationController = new NotificationController(this, mPresShell);

  
  
  
  if (mDocument->GetReadyStateEnum() == nsIDocument::READYSTATE_COMPLETE)
    mLoadState |= eDOMLoaded;

  AddEventListeners();
}

void
DocAccessible::Shutdown()
{
  if (!mPresShell) 
    return;

#ifdef DEBUG
  if (logging::IsEnabled(logging::eDocDestroy))
    logging::DocDestroy("document shutdown", mDocument, this);
#endif

  mPresShell->SetAccDocument(nullptr);

  if (mNotificationController) {
    mNotificationController->Shutdown();
    mNotificationController = nullptr;
  }

  RemoveEventListeners();

  
  
  
  mFlags |= eIsDefunct;
  nsCOMPtr<nsIDocument> kungFuDeathGripDoc = mDocument;
  mDocument = nullptr;

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


void
DocAccessible::GetBoundsRect(nsRect& aBounds, nsIFrame** aRelativeFrame)
{
  *aRelativeFrame = GetFrame();

  nsIDocument *document = mDocument;
  nsIDocument *parentDoc = nullptr;

  while (document) {
    nsIPresShell *presShell = document->GetShell();
    if (!presShell) {
      return;
    }

    nsRect scrollPort;
    nsIScrollableFrame* sf = presShell->GetRootScrollFrameAsScrollableExternal();
    if (sf) {
      scrollPort = sf->GetScrollPortRect();
    } else {
      nsIFrame* rootFrame = presShell->GetRootFrame();
      if (!rootFrame) {
        return;
      }
      scrollPort = rootFrame->GetRect();
    }

    if (parentDoc) {  
      
      
      
      
      aBounds.IntersectRect(scrollPort, aBounds);
    }
    else {  
      aBounds = scrollPort;
    }

    document = parentDoc = document->GetParentDocument();
  }
}


nsresult
DocAccessible::AddEventListeners()
{
  
  

  NS_ENSURE_TRUE(mPresShell, NS_ERROR_FAILURE);

  nsCOMPtr<nsISupports> container = mDocument->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem(do_QueryInterface(container));
  NS_ENSURE_TRUE(docShellTreeItem, NS_ERROR_FAILURE);

  
  
  int32_t itemType;
  docShellTreeItem->GetItemType(&itemType);

  bool isContent = (itemType == nsIDocShellTreeItem::typeContent);

  if (isContent) {
    
    nsCOMPtr<nsICommandManager> commandManager = do_GetInterface(docShellTreeItem);
    if (commandManager) {
      commandManager->AddCommandObserver(this, "obs_documentCreated");
    }
  }

  nsCOMPtr<nsIDocShellTreeItem> rootTreeItem;
  docShellTreeItem->GetRootTreeItem(getter_AddRefs(rootTreeItem));
  if (rootTreeItem) {
    a11y::RootAccessible* rootAccessible = RootAccessible();
    NS_ENSURE_TRUE(rootAccessible, NS_ERROR_FAILURE);
    nsRefPtr<nsCaretAccessible> caretAccessible = rootAccessible->GetCaretAccessible();
    if (caretAccessible) {
      caretAccessible->AddDocSelectionListener(mPresShell);
    }
  }

  
  mDocument->AddObserver(this);
  return NS_OK;
}


nsresult
DocAccessible::RemoveEventListeners()
{
  
  
  RemoveScrollListener();

  NS_ASSERTION(mDocument, "No document during removal of listeners.");

  if (mDocument) {
    mDocument->RemoveObserver(this);

    nsCOMPtr<nsISupports> container = mDocument->GetContainer();
    nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem(do_QueryInterface(container));
    NS_ASSERTION(docShellTreeItem, "doc should support nsIDocShellTreeItem.");

    if (docShellTreeItem) {
      int32_t itemType;
      docShellTreeItem->GetItemType(&itemType);
      if (itemType == nsIDocShellTreeItem::typeContent) {
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

  a11y::RootAccessible* rootAccessible = RootAccessible();
  if (rootAccessible) {
    nsRefPtr<nsCaretAccessible> caretAccessible = rootAccessible->GetCaretAccessible();
    if (caretAccessible)
      caretAccessible->RemoveDocSelectionListener(mPresShell);
  }

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
DocAccessible::AddScrollListener()
{
  if (!mPresShell)
    return;

  nsIScrollableFrame* sf = mPresShell->GetRootScrollFrameAsScrollableExternal();
  if (sf) {
    sf->AddScrollPositionListener(this);
#ifdef DEBUG
    if (logging::IsEnabled(logging::eDocCreate))
      logging::Text("add scroll listener");
#endif
  }
}


void
DocAccessible::RemoveScrollListener()
{
  if (!mPresShell)
    return;
 
  nsIScrollableFrame* sf = mPresShell->GetRootScrollFrameAsScrollableExternal();
  if (sf) {
    sf->RemoveScrollPositionListener(this);
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
                       const PRUnichar* aData)
{
  if (!nsCRT::strcmp(aTopic,"obs_documentCreated")) {    
    
    
    
    
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(this, states::EDITABLE, true);
    FireDelayedAccessibleEvent(event);
  }

  return NS_OK;
}




NS_IMETHODIMP
DocAccessible::OnPivotChanged(nsIAccessiblePivot* aPivot,
                              nsIAccessible* aOldAccessible,
                              int32_t aOldStart, int32_t aOldEnd,
                              PivotMoveReason aReason)
{
  nsRefPtr<AccEvent> event = new AccVCChangeEvent(this, aOldAccessible,
                                                  aOldStart, aOldEnd,
                                                  aReason);
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
    RemoveDependentIDsFor(accessible, aAttribute);

  
  
  
  

  
  
  
  
  if (aAttribute == nsGkAtoms::aria_checked ||
      aAttribute == nsGkAtoms::aria_pressed) {
    mARIAAttrOldValue = (aModType != nsIDOMMutationEvent::ADDITION) ?
      nsAccUtils::GetARIAToken(aElement, aAttribute) : nullptr;
  }
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

  
  
  AttributeChangedImpl(aElement, aNameSpaceID, aAttribute);

  
  
  
  
  
  if (aModType == nsIDOMMutationEvent::MODIFICATION ||
      aModType == nsIDOMMutationEvent::ADDITION) {
    AddDependentIDsFor(accessible, aAttribute);
  }
}


void
DocAccessible::AttributeChangedImpl(nsIContent* aContent, int32_t aNameSpaceID, nsIAtom* aAttribute)
{
  
  
  

  
  
  
  
  
  
  
  
  
  
  

  
  
  if (aAttribute == nsGkAtoms::disabled ||
      aAttribute == nsGkAtoms::aria_disabled) {

    
    
    

    
    

    nsRefPtr<AccEvent> enabledChangeEvent =
      new AccStateChangeEvent(aContent, states::ENABLED);

    FireDelayedAccessibleEvent(enabledChangeEvent);

    nsRefPtr<AccEvent> sensitiveChangeEvent =
      new AccStateChangeEvent(aContent, states::SENSITIVE);

    FireDelayedAccessibleEvent(sensitiveChangeEvent);
    return;
  }

  
  if (aNameSpaceID == kNameSpaceID_None) {
    
    if (StringBeginsWith(nsDependentAtomString(aAttribute),
                         NS_LITERAL_STRING("aria-"))) {
      ARIAAttributeChanged(aContent, aAttribute);
    }
  }

  if (aAttribute == nsGkAtoms::alt ||
      aAttribute == nsGkAtoms::title ||
      aAttribute == nsGkAtoms::aria_label ||
      aAttribute == nsGkAtoms::aria_labelledby) {
    FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_NAME_CHANGE,
                               aContent);
    return;
  }

  if (aAttribute == nsGkAtoms::aria_busy) {
    bool isOn = aContent->AttrValueIs(aNameSpaceID, aAttribute,
                                      nsGkAtoms::_true, eCaseMatters);
    nsRefPtr<AccEvent> event = new AccStateChangeEvent(aContent, states::BUSY, isOn);
    FireDelayedAccessibleEvent(event);
    return;
  }

  
  if ((aContent->IsXUL() && aAttribute == nsGkAtoms::selected) ||
      aAttribute == nsGkAtoms::aria_selected) {
    Accessible* item = GetAccessible(aContent);
    if (!item)
      return;

    Accessible* widget =
      nsAccUtils::GetSelectableContainer(item, item->State());
    if (widget) {
      AccSelChangeEvent::SelChangeType selChangeType =
        aContent->AttrValueIs(aNameSpaceID, aAttribute,
                              nsGkAtoms::_true, eCaseMatters) ?
          AccSelChangeEvent::eSelectionAdd : AccSelChangeEvent::eSelectionRemove;

      nsRefPtr<AccEvent> event =
        new AccSelChangeEvent(widget, item, selChangeType);
      FireDelayedAccessibleEvent(event);
    }
    return;
  }

  if (aAttribute == nsGkAtoms::contenteditable) {
    nsRefPtr<AccEvent> editableChangeEvent =
      new AccStateChangeEvent(aContent, states::EDITABLE);
    FireDelayedAccessibleEvent(editableChangeEvent);
    return;
  }
}


void
DocAccessible::ARIAAttributeChanged(nsIContent* aContent, nsIAtom* aAttribute)
{
  
  

  if (aAttribute == nsGkAtoms::aria_required) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(aContent, states::REQUIRED);
    FireDelayedAccessibleEvent(event);
    return;
  }

  if (aAttribute == nsGkAtoms::aria_invalid) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(aContent, states::INVALID);
    FireDelayedAccessibleEvent(event);
    return;
  }

  
  
  
  if (aAttribute == nsGkAtoms::aria_activedescendant) {
    mNotificationController->HandleNotification<DocAccessible, nsIContent>
      (this, &DocAccessible::ARIAActiveDescendantChanged, aContent);

    return;
  }

  
  if (aAttribute == nsGkAtoms::aria_expanded) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(aContent, states::EXPANDED);
    FireDelayedAccessibleEvent(event);
    return;
  }

  
  
  uint8_t attrFlags = nsAccUtils::GetAttributeCharacteristics(aAttribute);
  if (!(attrFlags & ATTR_BYPASSOBJ))
    FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_OBJECT_ATTRIBUTE_CHANGED,
                               aContent);

  if (!aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::role)) {
    
    
    
    
    return;
  }

  
  if (aAttribute == nsGkAtoms::aria_checked ||
      aAttribute == nsGkAtoms::aria_pressed) {
    const uint32_t kState = (aAttribute == nsGkAtoms::aria_checked) ?
                            states::CHECKED : states::PRESSED;
    nsRefPtr<AccEvent> event = new AccStateChangeEvent(aContent, kState);
    FireDelayedAccessibleEvent(event);

    Accessible* accessible = event->GetAccessible();
    if (accessible) {
      bool wasMixed = (mARIAAttrOldValue == nsGkAtoms::mixed);
      bool isMixed = aContent->AttrValueIs(kNameSpaceID_None, aAttribute,
                                           nsGkAtoms::mixed, eCaseMatters);
      if (isMixed != wasMixed) {
        nsRefPtr<AccEvent> event =
          new AccStateChangeEvent(aContent, states::MIXED, isMixed);
        FireDelayedAccessibleEvent(event);
      }
    }
    return;
  }

  if (aAttribute == nsGkAtoms::aria_readonly) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(aContent, states::READONLY);
    FireDelayedAccessibleEvent(event);
    return;
  }

  
  
  if (aAttribute == nsGkAtoms::aria_valuetext ||
      (aAttribute == nsGkAtoms::aria_valuenow &&
       (!aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::aria_valuetext) ||
        aContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::aria_valuetext,
                              nsGkAtoms::_empty, eCaseMatters)))) {
    FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_VALUE_CHANGE,
                               aContent);
    return;
  }
}

void
DocAccessible::ARIAActiveDescendantChanged(nsIContent* aElm)
{
  if (FocusMgr()->HasDOMFocus(aElm)) {
    nsAutoString id;
    if (aElm->GetAttr(kNameSpaceID_None, nsGkAtoms::aria_activedescendant, id)) {
      nsIDocument* DOMDoc = aElm->OwnerDoc();
      dom::Element* activeDescendantElm = DOMDoc->GetElementById(id);
      if (activeDescendantElm) {
        Accessible* activeDescendant = GetAccessible(activeDescendantElm);
        if (activeDescendant) {
          FocusMgr()->ActiveItemChanged(activeDescendant, false);
          A11YDEBUG_FOCUS_ACTIVEITEMCHANGE_CAUSE("ARIA activedescedant changed",
                                                 activeDescendant)
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
                                   nsEventStates aStateMask)
{
  if (aStateMask.HasState(NS_EVENT_STATE_CHECKED)) {
    Accessible* item = GetAccessible(aContent);
    if (item) {
      Accessible* widget = item->ContainerWidget();
      if (widget && widget->IsSelect()) {
        AccSelChangeEvent::SelChangeType selChangeType =
          aContent->AsElement()->State().HasState(NS_EVENT_STATE_CHECKED) ?
            AccSelChangeEvent::eSelectionAdd : AccSelChangeEvent::eSelectionRemove;
        nsRefPtr<AccEvent> event = new AccSelChangeEvent(widget, item,
                                                         selChangeType);
        FireDelayedAccessibleEvent(event);
      }
    }
  }

  if (aStateMask.HasState(NS_EVENT_STATE_INVALID)) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(aContent, states::INVALID, true);
    FireDelayedAccessibleEvent(event);
   }
}

void
DocAccessible::DocumentStatesChanged(nsIDocument* aDocument,
                                     nsEventStates aStateMask)
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





#ifdef DEBUG
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

  nsIViewManager* vm = mPresShell->GetViewManager();
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
DocAccessible::GetAccessibleOrContainer(nsINode* aNode)
{
  if (!aNode || !aNode->IsInDoc())
    return nullptr;

  nsINode* currNode = aNode;
  Accessible* accessible = nullptr;
  while (!(accessible = GetAccessible(currNode)) &&
         (currNode = currNode->GetNodeParent()));

  return accessible;
}

bool
DocAccessible::BindToDocument(Accessible* aAccessible,
                              nsRoleMapEntry* aRoleMapEntry)
{
  if (!aAccessible)
    return false;

  
  if (aAccessible->IsPrimaryForNode())
    mNodeToAccessibleMap.Put(aAccessible->GetNode(), aAccessible);

  
  mAccessibleCache.Put(aAccessible->UniqueID(), aAccessible);

  
  aAccessible->Init();

  aAccessible->SetRoleMapEntry(aRoleMapEntry);
  if (aAccessible->IsElement())
    AddDependentIDsFor(aAccessible);

  return true;
}

void
DocAccessible::UnbindFromDocument(Accessible* aAccessible)
{
  NS_ASSERTION(mAccessibleCache.GetWeak(aAccessible->UniqueID()),
               "Unbinding the unbound accessible!");

  
  
  if (FocusMgr()->IsActiveItem(aAccessible)) {
    FocusMgr()->ActiveItemChanged(nullptr);
    A11YDEBUG_FOCUS_ACTIVEITEMCHANGE_CAUSE("tree shutdown", aAccessible)
  }

  
  if (aAccessible->IsPrimaryForNode() &&
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
  
  
  
  

  ContentRemoved(aContent->GetParent(), aContent);
  ContentInserted(aContent->GetParent(), aContent, aContent->GetNextSibling());
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




void
DocAccessible::CacheChildren()
{
  
  
  nsAccTreeWalker walker(this, mDocument->GetRootElement(),
                         CanHaveAnonChildren());

  Accessible* child = nullptr;
  while ((child = walker.NextChild()) && AppendChild(child));
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
    new AccStateChangeEvent(mDocument, states::BUSY, true);
  FireDelayedAccessibleEvent(stateEvent);
}

void
DocAccessible::DoInitialUpdate()
{
  mLoadState |= eTreeConstructed;

  
  
  
  nsIContent* contentElm = nsCoreUtils::GetRoleContent(mDocument);
  if (contentElm && mContent != contentElm)
    mContent = contentElm;

  
  CacheChildrenInSubtree(this);

  
  
  
  
  if (!IsRoot()) {
    nsRefPtr<AccEvent> reorderEvent =
      new AccEvent(nsIAccessibleEvent::EVENT_REORDER, Parent(), eAutoDetect,
                   AccEvent::eCoalesceFromSameSubtree);
    ParentDocument()->FireDelayedAccessibleEvent(reorderEvent);
  }
}

void
DocAccessible::ProcessLoad()
{
  mLoadState |= eCompletelyLoaded;

  
  
  
  
  
  
  if (!IsLoadEventTarget())
    return;

  
  if (mLoadEventType) {
    nsRefPtr<AccEvent> loadEvent = new AccEvent(mLoadEventType, this);
    nsEventShell::FireEvent(loadEvent);

    mLoadEventType = 0;
  }

  
  nsRefPtr<AccEvent> stateEvent =
    new AccStateChangeEvent(this, states::BUSY, false);
  nsEventShell::FireEvent(stateEvent);
}

void
DocAccessible::AddDependentIDsFor(Accessible* aRelProvider,
                                  nsIAtom* aRelAttr)
{
  for (uint32_t idx = 0; idx < kRelationAttrsLen; idx++) {
    nsIAtom* relAttr = *kRelationAttrs[idx];
    if (aRelAttr && aRelAttr != relAttr)
      continue;

    if (relAttr == nsGkAtoms::_for) {
      if (!aRelProvider->GetContent()->IsHTML() ||
          (aRelProvider->GetContent()->Tag() != nsGkAtoms::label &&
           aRelProvider->GetContent()->Tag() != nsGkAtoms::output))
        continue;

    } else if (relAttr == nsGkAtoms::control) {
      if (!aRelProvider->GetContent()->IsXUL() ||
          (aRelProvider->GetContent()->Tag() != nsGkAtoms::label &&
           aRelProvider->GetContent()->Tag() != nsGkAtoms::description))
        continue;
    }

    IDRefsIterator iter(this, aRelProvider->GetContent(), relAttr);
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
          new AttrRelProvider(relAttr, aRelProvider->GetContent());
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
DocAccessible::RemoveDependentIDsFor(Accessible* aRelProvider,
                                     nsIAtom* aRelAttr)
{
  for (uint32_t idx = 0; idx < kRelationAttrsLen; idx++) {
    nsIAtom* relAttr = *kRelationAttrs[idx];
    if (aRelAttr && aRelAttr != *kRelationAttrs[idx])
      continue;

    IDRefsIterator iter(this, aRelProvider->GetContent(), relAttr);
    while (true) {
      const nsDependentSubstring id = iter.NextID();
      if (id.IsEmpty())
        break;

      AttrRelProviderArray* providers = mDependentIDsHash.Get(id);
      if (providers) {
        for (uint32_t jdx = 0; jdx < providers->Length(); ) {
          AttrRelProvider* provider = (*providers)[jdx];
          if (provider->mRelAttr == relAttr &&
              provider->mContent == aRelProvider->GetContent())
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


nsresult
DocAccessible::FireDelayedAccessibleEvent(uint32_t aEventType, nsINode* aNode,
                                          AccEvent::EEventRule aAllowDupes,
                                          EIsFromUserInput aIsFromUserInput)
{
  nsRefPtr<AccEvent> event =
    new AccEvent(aEventType, aNode, aIsFromUserInput, aAllowDupes);
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  return FireDelayedAccessibleEvent(event);
}


nsresult
DocAccessible::FireDelayedAccessibleEvent(AccEvent* aEvent)
{
  NS_ENSURE_ARG(aEvent);

#ifdef DEBUG
  if (logging::IsEnabled(logging::eDocLoad))
    logging::DocLoadEventFired(aEvent);
#endif

  if (mNotificationController)
    mNotificationController->QueueEvent(aEvent);

  return NS_OK;
}

void
DocAccessible::ProcessPendingEvent(AccEvent* aEvent)
{
  uint32_t eventType = aEvent->GetEventType();
  if (eventType == nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED) {
    HyperTextAccessible* hyperText = aEvent->GetAccessible()->AsHyperText();
    int32_t caretOffset;
    if (hyperText &&
        NS_SUCCEEDED(hyperText->GetCaretOffset(&caretOffset))) {
      nsRefPtr<AccEvent> caretMoveEvent =
        new AccCaretMoveEvent(hyperText, caretOffset);
      nsEventShell::FireEvent(caretMoveEvent);

      int32_t selectionCount;
      hyperText->GetSelectionCount(&selectionCount);
      if (selectionCount) {  
        nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_TEXT_SELECTION_CHANGED,
                                hyperText);
      }
    }
  }
  else {
    nsEventShell::FireEvent(aEvent);

    
    if (eventType == nsIAccessibleEvent::EVENT_HIDE)
      ShutdownChildrenInSubtree(aEvent->GetAccessible());
  }
}

void
DocAccessible::ProcessContentInserted(Accessible* aContainer,
                                      const nsTArray<nsCOMPtr<nsIContent> >* aInsertedContent)
{
  
  if (!HasAccessible(aContainer->GetNode()))
    return;

  if (aContainer == this) {
    
    nsIContent* rootContent = nsCoreUtils::GetRoleContent(mDocument);
    if (rootContent && rootContent != mContent)
      mContent = rootContent;

    
    
    
  }

  
  
  
  
  
  aContainer->UpdateChildren();

  
  
  
  
  
  
  
  
  for (uint32_t idx = 0; idx < aInsertedContent->Length(); idx++) {
    Accessible* directContainer =
      GetContainerAccessible(aInsertedContent->ElementAt(idx));
    if (directContainer)
      UpdateTree(directContainer, aInsertedContent->ElementAt(idx), true);
  }
}

void
DocAccessible::UpdateTree(Accessible* aContainer, nsIContent* aChildNode,
                          bool aIsInsert)
{
  uint32_t updateFlags = eNoAccessible;

  
  Accessible* child = GetAccessible(aChildNode);
#ifdef DEBUG
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

  if (child) {
    updateFlags |= UpdateTreeInternal(child, aIsInsert);

  } else {
    nsAccTreeWalker walker(this, aChildNode,
                           aContainer->CanHaveAnonChildren(), true);

    while ((child = walker.NextChild()))
      updateFlags |= UpdateTreeInternal(child, aIsInsert);
  }

  
  if (updateFlags == eNoAccessible)
    return;

  
  
  if (aIsInsert && !(updateFlags & eAlertAccessible)) {
    
    
    Accessible* ancestor = aContainer;
    while (ancestor) {
      if (ancestor->ARIARole() == roles::ALERT) {
        FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_ALERT,
                                   ancestor->GetNode());
        break;
      }

      
      if (ancestor == this)
        break;

      ancestor = ancestor->Parent();
    }
  }

  MaybeNotifyOfValueChange(aContainer);

  
  
  nsRefPtr<AccEvent> reorderEvent =
    new AccEvent(nsIAccessibleEvent::EVENT_REORDER, aContainer->GetNode(),
                 eAutoDetect, AccEvent::eCoalesceFromSameSubtree);
  if (reorderEvent)
    FireDelayedAccessibleEvent(reorderEvent);
}

uint32_t
DocAccessible::UpdateTreeInternal(Accessible* aChild, bool aIsInsert)
{
  uint32_t updateFlags = eAccessible;

  nsINode* node = aChild->GetNode();
  if (aIsInsert) {
    
    CacheChildrenInSubtree(aChild);

  } else {
    

    
    
    
    
    
    
    
    if (aChild->ARIARole() == roles::MENUPOPUP) {
      nsRefPtr<AccEvent> event =
        new AccEvent(nsIAccessibleEvent::EVENT_MENUPOPUP_END, aChild);

      if (event)
        FireDelayedAccessibleEvent(event);
    }
  }

  
  nsRefPtr<AccEvent> event;
  if (aIsInsert)
    event = new AccShowEvent(aChild, node);
  else
    event = new AccHideEvent(aChild, node);

  if (event)
    FireDelayedAccessibleEvent(event);

  if (aIsInsert) {
    roles::Role ariaRole = aChild->ARIARole();
    if (ariaRole == roles::MENUPOPUP) {
      
      FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_MENUPOPUP_START,
                                 node, AccEvent::eRemoveDupes);

    } else if (ariaRole == roles::ALERT) {
      
      updateFlags = eAlertAccessible;
      FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_ALERT, node,
                                 AccEvent::eRemoveDupes);
    }

    
    
    
    
    
    
    if (FocusMgr()->IsFocused(aChild))
      FocusMgr()->DispatchFocusEvent(this, aChild);

  } else {
    
    
    
    
    Accessible* parent = aChild->Parent();
    NS_ASSERTION(parent, "No accessible parent?!");
    if (parent)
      parent->RemoveChild(aChild);

    UncacheChildrenInSubtree(aChild);
  }

  return updateFlags;
}

void
DocAccessible::CacheChildrenInSubtree(Accessible* aRoot)
{
  aRoot->EnsureChildren();

  
  
  
  uint32_t count = aRoot->ContentChildCount();
  for (uint32_t idx = 0; idx < count; idx++) {
    Accessible* child = aRoot->ContentChildAt(idx);
    NS_ASSERTION(child, "Illicit tree change while tree is created!");
    
    if (child && child->IsContent())
      CacheChildrenInSubtree(child);
  }

  
  
  if (aRoot->HasARIARole() && !aRoot->IsDoc()) {
    a11y::role role = aRoot->ARIARole();
    if (role == roles::DIALOG || role == roles::DOCUMENT)
      FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_COMPLETE,
                                 aRoot->GetContent());
  }
}

void
DocAccessible::UncacheChildrenInSubtree(Accessible* aRoot)
{
  aRoot->mFlags |= eIsNotInDocument;

  if (aRoot->IsElement())
    RemoveDependentIDsFor(aRoot);

  uint32_t count = aRoot->ContentChildCount();
  for (uint32_t idx = 0; idx < count; idx++)
    UncacheChildrenInSubtree(aRoot->ContentChildAt(idx));

  if (aRoot->IsPrimaryForNode() &&
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

    ShutdownChildrenInSubtree(child);
  }

  UnbindFromDocument(aAccessible);
}

bool
DocAccessible::IsLoadEventTarget() const
{
  nsCOMPtr<nsISupports> container = mDocument->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
    do_QueryInterface(container);
  NS_ASSERTION(docShellTreeItem, "No document shell for document!");

  nsCOMPtr<nsIDocShellTreeItem> parentTreeItem;
  docShellTreeItem->GetParent(getter_AddRefs(parentTreeItem));

  
  
  
  
  if (parentTreeItem) {
    DocAccessible* parentDoc = ParentDocument();
    return parentDoc && parentDoc->HasLoadState(eCompletelyLoaded);
  }

  
  int32_t contentType;
  docShellTreeItem->GetItemType(&contentType);
  return (contentType == nsIDocShellTreeItem::typeContent);
}

