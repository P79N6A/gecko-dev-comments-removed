





































#include "AccIterator.h"
#include "States.h"
#include "nsAccCache.h"
#include "nsAccessibilityService.h"
#include "nsAccTreeWalker.h"
#include "nsAccUtils.h"
#include "nsRootAccessible.h"
#include "nsTextEquivUtils.h"

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
#include "nsHTMLSelectAccessible.h"
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

static const PRUint32 kRelationAttrsLen = NS_ARRAY_LENGTH(kRelationAttrs);




nsDocAccessible::
  nsDocAccessible(nsIDocument *aDocument, nsIContent *aRootContent,
                  nsIWeakReference *aShell) :
  nsHyperTextAccessibleWrap(aRootContent, aShell),
  mDocument(aDocument), mScrollPositionChangedTicks(0),
  mLoadState(eTreeConstructionPending), mLoadEventType(0)
{
  mFlags |= eDocAccessible;

  mDependentIDsHash.Init();
  
  mAccessibleCache.Init(kDefaultCacheSize);
  mNodeToAccessibleMap.Init(kDefaultCacheSize);

  
  if (mDocument && mDocument->IsXUL())
    mFlags &= ~eHyperTextAccessible;

  
  if (!mDocument)
    return;

  
  
  AddScrollListener();
}

nsDocAccessible::~nsDocAccessible()
{
}





NS_IMPL_CYCLE_COLLECTION_CLASS(nsDocAccessible)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDocAccessible, nsAccessible)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_MEMBER(mNotificationController,
                                                  NotificationController)

  PRUint32 i, length = tmp->mChildDocuments.Length();
  for (i = 0; i < length; ++i) {
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mChildDocuments[i],
                                                         nsIAccessible)
  }

  CycleCollectorTraverseCache(tmp->mAccessibleCache, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDocAccessible, nsAccessible)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mNotificationController)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSTARRAY(mChildDocuments)
  tmp->mDependentIDsHash.Clear();
  tmp->mNodeToAccessibleMap.Clear();
  ClearCache(tmp->mAccessibleCache);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDocAccessible)
  NS_INTERFACE_MAP_STATIC_AMBIGUOUS(nsDocAccessible)
  NS_INTERFACE_MAP_ENTRY(nsIAccessibleDocument)
  NS_INTERFACE_MAP_ENTRY(nsIDocumentObserver)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIAccessibleDocument)
    foundInterface = 0;

  nsresult status;
  if (!foundInterface) {
    
    
    
    

    status = IsHyperText() ? 
      nsHyperTextAccessible::QueryInterface(aIID,
                                            (void**)&foundInterface) :
      nsAccessible::QueryInterface(aIID, (void**)&foundInterface);
  } else {
    NS_ADDREF(foundInterface);
    status = NS_OK;
  }

  *aInstancePtr = foundInterface;
  return status;
}

NS_IMPL_ADDREF_INHERITED(nsDocAccessible, nsHyperTextAccessible)
NS_IMPL_RELEASE_INHERITED(nsDocAccessible, nsHyperTextAccessible)





NS_IMETHODIMP
nsDocAccessible::GetName(nsAString& aName)
{
  nsresult rv = NS_OK;
  aName.Truncate();
  if (mParent) {
    rv = mParent->GetName(aName); 
  }
  if (aName.IsEmpty()) {
    
    rv = nsAccessible::GetName(aName);
  }
  if (aName.IsEmpty()) {
    rv = GetTitle(aName);   
  }
  if (aName.IsEmpty()) {   
    rv = GetURL(aName);
  }

  return rv;
}


PRUint32
nsDocAccessible::NativeRole()
{
  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
    nsCoreUtils::GetDocShellTreeItemFor(mDocument);
  if (docShellTreeItem) {
    nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
    docShellTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));
    PRInt32 itemType;
    docShellTreeItem->GetItemType(&itemType);
    if (sameTypeRoot == docShellTreeItem) {
      
      if (itemType == nsIDocShellTreeItem::typeChrome)
        return nsIAccessibleRole::ROLE_CHROME_WINDOW;

      if (itemType == nsIDocShellTreeItem::typeContent) {
#ifdef MOZ_XUL
        nsCOMPtr<nsIXULDocument> xulDoc(do_QueryInterface(mDocument));
        if (xulDoc)
          return nsIAccessibleRole::ROLE_APPLICATION;
#endif
        return nsIAccessibleRole::ROLE_DOCUMENT;
      }
    }
    else if (itemType == nsIDocShellTreeItem::typeContent) {
      return nsIAccessibleRole::ROLE_DOCUMENT;
    }
  }

  return nsIAccessibleRole::ROLE_PANE; 
}


void
nsDocAccessible::SetRoleMapEntry(nsRoleMapEntry* aRoleMapEntry)
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
    nsRoleMapEntry *roleMapEntry = nsAccUtils::GetRoleMapEntry(ownerContent);
    if (roleMapEntry)
      mRoleMapEntry = roleMapEntry; 
  }
}

void
nsDocAccessible::Description(nsString& aDescription)
{
  if (mParent)
    mParent->Description(aDescription);

  if (aDescription.IsEmpty())
    nsTextEquivUtils::
      GetTextEquivFromIDRefs(this, nsGkAtoms::aria_describedby,
                             aDescription);
}


PRUint64
nsDocAccessible::NativeState()
{
  
  
  PRUint64 state = (mContent->GetCurrentDoc() == mDocument) ?
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

  nsCOMPtr<nsIEditor> editor;
  GetAssociatedEditor(getter_AddRefs(editor));
  state |= editor ? states::EDITABLE : states::READONLY;

  return state;
}


void
nsDocAccessible::ApplyARIAState(PRUint64* aState)
{
  
  
  nsAccessible::ApplyARIAState(aState);

  
  if (mParent)
    mParent->ApplyARIAState(aState);

}

NS_IMETHODIMP
nsDocAccessible::GetAttributes(nsIPersistentProperties **aAttributes)
{
  nsAccessible::GetAttributes(aAttributes);
  if (mParent) {
    mParent->GetAttributes(aAttributes); 
  }
  return NS_OK;
}

nsAccessible*
nsDocAccessible::FocusedChild()
{
  
  
  return FocusMgr()->FocusedAccessible();
}

NS_IMETHODIMP nsDocAccessible::TakeFocus()
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
  NS_ENSURE_STATE(fm);

  nsCOMPtr<nsIDOMElement> newFocus;
  return fm->MoveFocus(mDocument->GetWindow(), nsnull,
                       nsIFocusManager::MOVEFOCUS_ROOT, 0,
                       getter_AddRefs(newFocus));
}





NS_IMETHODIMP nsDocAccessible::GetURL(nsAString& aURL)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsISupports> container = mDocument->GetContainer();
  nsCOMPtr<nsIWebNavigation> webNav(do_GetInterface(container));
  nsCAutoString theURL;
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
nsDocAccessible::GetTitle(nsAString& aTitle)
{
  nsCOMPtr<nsIDOMDocument> domDocument = do_QueryInterface(mDocument);
  if (!domDocument) {
    return NS_ERROR_FAILURE;
  }
  return domDocument->GetTitle(aTitle);
}

NS_IMETHODIMP
nsDocAccessible::GetMimeType(nsAString& aMimeType)
{
  nsCOMPtr<nsIDOMDocument> domDocument = do_QueryInterface(mDocument);
  if (!domDocument) {
    return NS_ERROR_FAILURE;
  }
  return domDocument->GetContentType(aMimeType);
}

NS_IMETHODIMP nsDocAccessible::GetDocType(nsAString& aDocType)
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

NS_IMETHODIMP nsDocAccessible::GetNameSpaceURIForID(PRInt16 aNameSpaceID, nsAString& aNameSpaceURI)
{
  if (mDocument) {
    nsCOMPtr<nsINameSpaceManager> nameSpaceManager =
        do_GetService(NS_NAMESPACEMANAGER_CONTRACTID);
    if (nameSpaceManager)
      return nameSpaceManager->GetNameSpaceURI(aNameSpaceID, aNameSpaceURI);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsDocAccessible::GetWindowHandle(void **aWindow)
{
  NS_ENSURE_ARG_POINTER(aWindow);
  *aWindow = GetNativeWindow();
  return NS_OK;
}

NS_IMETHODIMP nsDocAccessible::GetWindow(nsIDOMWindow **aDOMWin)
{
  *aDOMWin = nsnull;
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
nsDocAccessible::GetDOMDocument(nsIDOMDocument **aDOMDocument)
{
  NS_ENSURE_ARG_POINTER(aDOMDocument);
  *aDOMDocument = nsnull;

  if (mDocument)
    CallQueryInterface(mDocument, aDOMDocument);

  return NS_OK;
}

NS_IMETHODIMP
nsDocAccessible::GetParentDocument(nsIAccessibleDocument** aDocument)
{
  NS_ENSURE_ARG_POINTER(aDocument);
  *aDocument = nsnull;

  if (!IsDefunct())
    NS_IF_ADDREF(*aDocument = ParentDocument());

  return NS_OK;
}

NS_IMETHODIMP
nsDocAccessible::GetChildDocumentCount(PRUint32* aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);
  *aCount = 0;

  if (!IsDefunct())
    *aCount = ChildDocumentCount();

  return NS_OK;
}

NS_IMETHODIMP
nsDocAccessible::GetChildDocumentAt(PRUint32 aIndex,
                                    nsIAccessibleDocument** aDocument)
{
  NS_ENSURE_ARG_POINTER(aDocument);
  *aDocument = nsnull;

  if (IsDefunct())
    return NS_OK;

  NS_IF_ADDREF(*aDocument = GetChildDocumentAt(aIndex));
  return *aDocument ? NS_OK : NS_ERROR_INVALID_ARG;
}


NS_IMETHODIMP nsDocAccessible::GetAssociatedEditor(nsIEditor **aEditor)
{
  NS_ENSURE_ARG_POINTER(aEditor);
  *aEditor = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  
  if (!mDocument->HasFlag(NODE_IS_EDITABLE) &&
      !mContent->HasFlag(NODE_IS_EDITABLE))
    return NS_OK;

  nsCOMPtr<nsISupports> container = mDocument->GetContainer();
  nsCOMPtr<nsIEditingSession> editingSession(do_GetInterface(container));
  if (!editingSession)
    return NS_OK; 

  nsCOMPtr<nsIEditor> editor;
  editingSession->GetEditorForWindow(mDocument->GetWindow(), getter_AddRefs(editor));
  if (!editor) {
    return NS_OK;
  }
  bool isEditable;
  editor->GetIsDocumentEditable(&isEditable);
  if (isEditable) {
    NS_ADDREF(*aEditor = editor);
  }
  return NS_OK;
}


nsAccessible*
nsDocAccessible::GetAccessible(nsINode* aNode) const
{
  nsAccessible* accessible = mNodeToAccessibleMap.Get(aNode);

  
  
  if (!accessible) {
    if (GetNode() != aNode)
      return nsnull;

    accessible = const_cast<nsDocAccessible*>(this);
  }

#ifdef DEBUG
  
  
  
  
  nsAccessible* parent = accessible->Parent();
  if (parent)
    parent->TestChildCache(accessible);
#endif

  return accessible;
}




bool
nsDocAccessible::Init()
{
  NS_LOG_ACCDOCCREATE_FOR("document initialize", mDocument, this)

  
  nsCOMPtr<nsIPresShell> shell(GetPresShell());
  mNotificationController = new NotificationController(this, shell);
  if (!mNotificationController)
    return false;

  
  
  
  if (mDocument->GetReadyStateEnum() == nsIDocument::READYSTATE_COMPLETE)
    mLoadState |= eDOMLoaded;

  AddEventListeners();
  return true;
}

void
nsDocAccessible::Shutdown()
{
  if (!mWeakShell) 
    return;

  NS_LOG_ACCDOCDESTROY_FOR("document shutdown", mDocument, this)

  if (mNotificationController) {
    mNotificationController->Shutdown();
    mNotificationController = nsnull;
  }

  RemoveEventListeners();

  
  
  nsCOMPtr<nsIDocument> kungFuDeathGripDoc = mDocument;
  mDocument = nsnull;

  if (mParent) {
    nsDocAccessible* parentDocument = mParent->GetDocAccessible();
    if (parentDocument)
      parentDocument->RemoveChildDocument(this);

    mParent->RemoveChild(this);
  }

  
  
  PRInt32 childDocCount = mChildDocuments.Length();
  for (PRInt32 idx = childDocCount - 1; idx >= 0; idx--)
    mChildDocuments[idx]->Shutdown();

  mChildDocuments.Clear();

  mWeakShell = nsnull;  

  mDependentIDsHash.Clear();
  mNodeToAccessibleMap.Clear();
  ClearCache(mAccessibleCache);

  nsHyperTextAccessibleWrap::Shutdown();

  GetAccService()->NotifyOfDocumentShutdown(kungFuDeathGripDoc);
}

nsIFrame*
nsDocAccessible::GetFrame() const
{
  nsCOMPtr<nsIPresShell> shell(do_QueryReferent(mWeakShell));

  nsIFrame* root = nsnull;
  if (shell)
    root = shell->GetRootFrame();

  return root;
}

bool
nsDocAccessible::IsDefunct() const
{
  return nsHyperTextAccessibleWrap::IsDefunct() || !mDocument;
}


void nsDocAccessible::GetBoundsRect(nsRect& aBounds, nsIFrame** aRelativeFrame)
{
  *aRelativeFrame = GetFrame();

  nsIDocument *document = mDocument;
  nsIDocument *parentDoc = nsnull;

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


nsresult nsDocAccessible::AddEventListeners()
{
  
  

  nsCOMPtr<nsIPresShell> presShell(GetPresShell());
  NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

  nsCOMPtr<nsISupports> container = mDocument->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem(do_QueryInterface(container));
  NS_ENSURE_TRUE(docShellTreeItem, NS_ERROR_FAILURE);

  
  
  PRInt32 itemType;
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
    nsRootAccessible* rootAccessible = RootAccessible();
    NS_ENSURE_TRUE(rootAccessible, NS_ERROR_FAILURE);
    nsRefPtr<nsCaretAccessible> caretAccessible = rootAccessible->GetCaretAccessible();
    if (caretAccessible) {
      caretAccessible->AddDocSelectionListener(presShell);
    }
  }

  
  mDocument->AddObserver(this);
  return NS_OK;
}


nsresult nsDocAccessible::RemoveEventListeners()
{
  
  
  RemoveScrollListener();

  NS_ASSERTION(mDocument, "No document during removal of listeners.");

  if (mDocument) {
    mDocument->RemoveObserver(this);

    nsCOMPtr<nsISupports> container = mDocument->GetContainer();
    nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem(do_QueryInterface(container));
    NS_ASSERTION(docShellTreeItem, "doc should support nsIDocShellTreeItem.");

    if (docShellTreeItem) {
      PRInt32 itemType;
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
    mScrollWatchTimer = nsnull;
    NS_RELEASE_THIS(); 
  }

  nsRootAccessible* rootAccessible = RootAccessible();
  if (rootAccessible) {
    nsRefPtr<nsCaretAccessible> caretAccessible = rootAccessible->GetCaretAccessible();
    if (caretAccessible) {
      
      nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));
      caretAccessible->RemoveDocSelectionListener(presShell);
    }
  }

  return NS_OK;
}

void nsDocAccessible::ScrollTimerCallback(nsITimer *aTimer, void *aClosure)
{
  nsDocAccessible *docAcc = reinterpret_cast<nsDocAccessible*>(aClosure);

  if (docAcc && docAcc->mScrollPositionChangedTicks &&
      ++docAcc->mScrollPositionChangedTicks > 2) {
    
    
    
    
    nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_SCROLLING_END, docAcc);

    docAcc->mScrollPositionChangedTicks = 0;
    if (docAcc->mScrollWatchTimer) {
      docAcc->mScrollWatchTimer->Cancel();
      docAcc->mScrollWatchTimer = nsnull;
      NS_RELEASE(docAcc); 
    }
  }
}


void nsDocAccessible::AddScrollListener()
{
  nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));
  if (!presShell)
    return;

  nsIScrollableFrame* sf = presShell->GetRootScrollFrameAsScrollableExternal();
  if (sf) {
    sf->AddScrollPositionListener(this);
    NS_LOG_ACCDOCCREATE_TEXT("add scroll listener")
  }
}


void nsDocAccessible::RemoveScrollListener()
{
  nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));
  if (!presShell)
    return;
 
  nsIScrollableFrame* sf = presShell->GetRootScrollFrameAsScrollableExternal();
  if (sf) {
    sf->RemoveScrollPositionListener(this);
  }
}




void nsDocAccessible::ScrollPositionDidChange(nscoord aX, nscoord aY)
{
  
  
  const PRUint32 kScrollPosCheckWait = 50;
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




NS_IMETHODIMP nsDocAccessible::Observe(nsISupports *aSubject, const char *aTopic,
                                       const PRUnichar *aData)
{
  if (!nsCRT::strcmp(aTopic,"obs_documentCreated")) {    
    
    
    
    
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(this, states::EDITABLE, true);
    FireDelayedAccessibleEvent(event);
  }

  return NS_OK;
}




NS_IMPL_NSIDOCUMENTOBSERVER_CORE_STUB(nsDocAccessible)
NS_IMPL_NSIDOCUMENTOBSERVER_LOAD_STUB(nsDocAccessible)
NS_IMPL_NSIDOCUMENTOBSERVER_STYLE_STUB(nsDocAccessible)

void
nsDocAccessible::AttributeWillChange(nsIDocument *aDocument,
                                     dom::Element* aElement,
                                     PRInt32 aNameSpaceID,
                                     nsIAtom* aAttribute, PRInt32 aModType)
{
  nsAccessible* accessible = GetAccessible(aElement);
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
      nsAccUtils::GetARIAToken(aElement, aAttribute) : nsnull;
  }
}

void
nsDocAccessible::AttributeChanged(nsIDocument *aDocument,
                                  dom::Element* aElement,
                                  PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                                  PRInt32 aModType)
{
  NS_ASSERTION(!IsDefunct(),
               "Attribute changed called on defunct document accessible!");

  
  
  if (UpdateAccessibleOnAttrChange(aElement, aAttribute))
    return;

  
  
  
  
  
  nsAccessible* accessible = GetAccessible(aElement);
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
nsDocAccessible::AttributeChangedImpl(nsIContent* aContent, PRInt32 aNameSpaceID, nsIAtom* aAttribute)
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
    nsAccessible* item = GetAccessible(aContent);
    nsAccessible* widget =
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
nsDocAccessible::ARIAAttributeChanged(nsIContent* aContent, nsIAtom* aAttribute)
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
    mNotificationController->HandleNotification<nsDocAccessible, nsIContent>
      (this, &nsDocAccessible::ARIAActiveDescendantChanged, aContent);

    return;
  }

  
  
  if (aAttribute == nsGkAtoms::aria_grabbed ||
      aAttribute == nsGkAtoms::aria_dropeffect ||
      aAttribute == nsGkAtoms::aria_hidden ||
      aAttribute == nsGkAtoms::aria_sort) {
    FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_OBJECT_ATTRIBUTE_CHANGED,
                               aContent);
  }

  
  if (aAttribute == nsGkAtoms::aria_expanded) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(aContent, states::EXPANDED);
    FireDelayedAccessibleEvent(event);
    return;
  }

  if (!aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::role)) {
    
    
    
    
    return;
  }

  
  if (aAttribute == nsGkAtoms::aria_checked ||
      aAttribute == nsGkAtoms::aria_pressed) {
    const PRUint32 kState = (aAttribute == nsGkAtoms::aria_checked) ?
                            states::CHECKED : states::PRESSED;
    nsRefPtr<AccEvent> event = new AccStateChangeEvent(aContent, kState);
    FireDelayedAccessibleEvent(event);

    nsAccessible* accessible = event->GetAccessible();
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
nsDocAccessible::ARIAActiveDescendantChanged(nsIContent* aElm)
{
  if (FocusMgr()->HasDOMFocus(aElm)) {
    nsAutoString id;
    if (aElm->GetAttr(kNameSpaceID_None, nsGkAtoms::aria_activedescendant, id)) {
      nsIDocument* DOMDoc = aElm->OwnerDoc();
      dom::Element* activeDescendantElm = DOMDoc->GetElementById(id);
      if (activeDescendantElm) {
        nsAccessible* activeDescendant = GetAccessible(activeDescendantElm);
        if (activeDescendant) {
          FocusMgr()->ActiveItemChanged(activeDescendant, false);
          A11YDEBUG_FOCUS_ACTIVEITEMCHANGE_CAUSE("ARIA activedescedant changed",
                                                 activeDescendant)
        }
      }
    }
  }
}

void nsDocAccessible::ContentAppended(nsIDocument *aDocument,
                                      nsIContent* aContainer,
                                      nsIContent* aFirstNewContent,
                                      PRInt32 )
{
}

void nsDocAccessible::ContentStateChanged(nsIDocument* aDocument,
                                          nsIContent* aContent,
                                          nsEventStates aStateMask)
{
  if (aStateMask.HasState(NS_EVENT_STATE_CHECKED)) {
    nsAccessible* item = GetAccessible(aContent);
    if (item) {
      nsAccessible* widget = item->ContainerWidget();
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

void nsDocAccessible::DocumentStatesChanged(nsIDocument* aDocument,
                                            nsEventStates aStateMask)
{
}

void nsDocAccessible::CharacterDataWillChange(nsIDocument *aDocument,
                                              nsIContent* aContent,
                                              CharacterDataChangeInfo* aInfo)
{
}

void nsDocAccessible::CharacterDataChanged(nsIDocument *aDocument,
                                           nsIContent* aContent,
                                           CharacterDataChangeInfo* aInfo)
{
}

void
nsDocAccessible::ContentInserted(nsIDocument *aDocument, nsIContent* aContainer,
                                 nsIContent* aChild, PRInt32 )
{
}

void
nsDocAccessible::ContentRemoved(nsIDocument *aDocument, nsIContent* aContainer,
                                nsIContent* aChild, PRInt32 ,
                                nsIContent* aPreviousSibling)
{
}

void
nsDocAccessible::ParentChainChanged(nsIContent *aContent)
{
}





#ifdef DEBUG_ACCDOCMGR
nsresult
nsDocAccessible::HandleAccEvent(AccEvent* aAccEvent)
{
  NS_LOG_ACCDOCLOAD_HANDLEEVENT(aAccEvent)

  return nsHyperTextAccessible::HandleAccEvent(aAccEvent);

}
#endif




void*
nsDocAccessible::GetNativeWindow() const
{
  nsCOMPtr<nsIPresShell> shell(do_QueryReferent(mWeakShell));
  nsIViewManager* vm = shell->GetViewManager();
  if (vm) {
    nsCOMPtr<nsIWidget> widget;
    vm->GetRootWidget(getter_AddRefs(widget));
    if (widget)
      return widget->GetNativeData(NS_NATIVE_WINDOW);
  }
  return nsnull;
}

nsAccessible*
nsDocAccessible::GetAccessibleByUniqueIDInSubtree(void* aUniqueID)
{
  nsAccessible* child = GetAccessibleByUniqueID(aUniqueID);
  if (child)
    return child;

  PRUint32 childDocCount = mChildDocuments.Length();
  for (PRUint32 childDocIdx= 0; childDocIdx < childDocCount; childDocIdx++) {
    nsDocAccessible* childDocument = mChildDocuments.ElementAt(childDocIdx);
    child = childDocument->GetAccessibleByUniqueIDInSubtree(aUniqueID);
    if (child)
      return child;
  }

  return nsnull;
}

nsAccessible*
nsDocAccessible::GetAccessibleOrContainer(nsINode* aNode)
{
  if (!aNode || !aNode->IsInDoc())
    return nsnull;

  nsINode* currNode = aNode;
  nsAccessible* accessible = nsnull;
  while (!(accessible = GetAccessible(currNode)) &&
         (currNode = currNode->GetNodeParent()));

  return accessible;
}

bool
nsDocAccessible::BindToDocument(nsAccessible* aAccessible,
                                nsRoleMapEntry* aRoleMapEntry)
{
  if (!aAccessible)
    return false;

  
  if (aAccessible->IsPrimaryForNode() &&
      !mNodeToAccessibleMap.Put(aAccessible->GetNode(), aAccessible))
    return false;

  
  if (!mAccessibleCache.Put(aAccessible->UniqueID(), aAccessible)) {
    if (aAccessible->IsPrimaryForNode())
      mNodeToAccessibleMap.Remove(aAccessible->GetNode());

    return false;
  }

  
  if (!aAccessible->Init()) {
    NS_ERROR("Failed to initialize an accessible!");

    UnbindFromDocument(aAccessible);
    return false;
  }

  aAccessible->SetRoleMapEntry(aRoleMapEntry);
  if (aAccessible->IsElement())
    AddDependentIDsFor(aAccessible);

  return true;
}

void
nsDocAccessible::UnbindFromDocument(nsAccessible* aAccessible)
{
  NS_ASSERTION(mAccessibleCache.GetWeak(aAccessible->UniqueID()),
               "Unbinding the unbound accessible!");

  
  
  if (FocusMgr()->IsActiveItem(aAccessible)) {
    FocusMgr()->ActiveItemChanged(nsnull);
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
nsDocAccessible::ContentInserted(nsIContent* aContainerNode,
                                 nsIContent* aStartChildNode,
                                 nsIContent* aEndChildNode)
{
  
  
  if (mNotificationController && HasLoadState(eTreeConstructed)) {
    
    
    nsAccessible* container = aContainerNode ?
      GetAccessibleOrContainer(aContainerNode) : this;

    mNotificationController->ScheduleContentInsertion(container,
                                                      aStartChildNode,
                                                      aEndChildNode);
  }
}

void
nsDocAccessible::ContentRemoved(nsIContent* aContainerNode,
                                nsIContent* aChildNode)
{
  
  
  nsAccessible* container = aContainerNode ?
    GetAccessibleOrContainer(aContainerNode) : this;

  UpdateTree(container, aChildNode, false);
}

void
nsDocAccessible::RecreateAccessible(nsIContent* aContent)
{
  
  
  
  

  
  nsAccessible* container = GetContainerAccessible(aContent);
  if (container) {
    
    UpdateTree(container, aContent, false);
    container->UpdateChildren();
    UpdateTree(container, aContent, true);
  }
}

void
nsDocAccessible::ProcessInvalidationList()
{
  
  
  
  for (PRUint32 idx = 0; idx < mInvalidationList.Length(); idx++) {
    nsIContent* content = mInvalidationList[idx];
    nsAccessible* accessible = GetAccessible(content);
    if (!accessible) {
      nsAccessible* container = GetContainerAccessible(content);
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
nsDocAccessible::CacheChildren()
{
  
  
  nsAccTreeWalker walker(mWeakShell, mDocument->GetRootElement(),
                         GetAllowsAnonChildAccessibles());

  nsAccessible* child = nsnull;
  while ((child = walker.NextChild()) && AppendChild(child));
}




void
nsDocAccessible::NotifyOfLoading(bool aIsReloading)
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
nsDocAccessible::DoInitialUpdate()
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
nsDocAccessible::ProcessLoad()
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
nsDocAccessible::AddDependentIDsFor(nsAccessible* aRelProvider,
                                    nsIAtom* aRelAttr)
{
  for (PRUint32 idx = 0; idx < kRelationAttrsLen; idx++) {
    nsIAtom* relAttr = *kRelationAttrs[idx];
    if (aRelAttr && aRelAttr != relAttr)
      continue;

    if (relAttr == nsGkAtoms::_for) {
      if (!aRelProvider->GetContent()->IsHTML() ||
          aRelProvider->GetContent()->Tag() != nsGkAtoms::label &&
          aRelProvider->GetContent()->Tag() != nsGkAtoms::output)
        continue;

    } else if (relAttr == nsGkAtoms::control) {
      if (!aRelProvider->GetContent()->IsXUL() ||
          aRelProvider->GetContent()->Tag() != nsGkAtoms::label &&
          aRelProvider->GetContent()->Tag() != nsGkAtoms::description)
        continue;
    }

    IDRefsIterator iter(aRelProvider->GetContent(), relAttr);
    while (true) {
      const nsDependentSubstring id = iter.NextID();
      if (id.IsEmpty())
        break;

      AttrRelProviderArray* providers = mDependentIDsHash.Get(id);
      if (!providers) {
        providers = new AttrRelProviderArray();
        if (providers) {
          if (!mDependentIDsHash.Put(id, providers)) {
            delete providers;
            providers = nsnull;
          }
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
nsDocAccessible::RemoveDependentIDsFor(nsAccessible* aRelProvider,
                                       nsIAtom* aRelAttr)
{
  for (PRUint32 idx = 0; idx < kRelationAttrsLen; idx++) {
    nsIAtom* relAttr = *kRelationAttrs[idx];
    if (aRelAttr && aRelAttr != *kRelationAttrs[idx])
      continue;

    IDRefsIterator iter(aRelProvider->GetContent(), relAttr);
    while (true) {
      const nsDependentSubstring id = iter.NextID();
      if (id.IsEmpty())
        break;

      AttrRelProviderArray* providers = mDependentIDsHash.Get(id);
      if (providers) {
        for (PRUint32 jdx = 0; jdx < providers->Length(); ) {
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
nsDocAccessible::UpdateAccessibleOnAttrChange(dom::Element* aElement,
                                              nsIAtom* aAttribute)
{
  if (aAttribute == nsGkAtoms::role) {
    
    
    if (mContent == aElement) {
      SetRoleMapEntry(nsAccUtils::GetRoleMapEntry(aElement));
      return true;
    }

    
    
    
    HandleNotification<nsDocAccessible, nsIContent>
      (this, &nsDocAccessible::RecreateAccessible, aElement);

    return true;
  }

  if (aAttribute == nsGkAtoms::href ||
      aAttribute == nsGkAtoms::onclick) {
    
    
    

    
    
    mNotificationController->ScheduleNotification<nsDocAccessible, nsIContent>
      (this, &nsDocAccessible::RecreateAccessible, aElement);

    return true;
  }

  if (aAttribute == nsGkAtoms::aria_multiselectable &&
      aElement->HasAttr(kNameSpaceID_None, nsGkAtoms::role)) {
    
    
    
    HandleNotification<nsDocAccessible, nsIContent>
      (this, &nsDocAccessible::RecreateAccessible, aElement);

    return true;
  }

  return false;
}


nsresult
nsDocAccessible::FireDelayedAccessibleEvent(PRUint32 aEventType, nsINode *aNode,
                                            AccEvent::EEventRule aAllowDupes,
                                            EIsFromUserInput aIsFromUserInput)
{
  nsRefPtr<AccEvent> event =
    new AccEvent(aEventType, aNode, aIsFromUserInput, aAllowDupes);
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  return FireDelayedAccessibleEvent(event);
}


nsresult
nsDocAccessible::FireDelayedAccessibleEvent(AccEvent* aEvent)
{
  NS_ENSURE_ARG(aEvent);
  NS_LOG_ACCDOCLOAD_FIREEVENT(aEvent)

  if (mNotificationController)
    mNotificationController->QueueEvent(aEvent);

  return NS_OK;
}

void
nsDocAccessible::ProcessPendingEvent(AccEvent* aEvent)
{
  PRUint32 eventType = aEvent->GetEventType();
  if (eventType == nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED) {
    nsHyperTextAccessible* hyperText = aEvent->GetAccessible()->AsHyperText();
    PRInt32 caretOffset;
    if (hyperText &&
        NS_SUCCEEDED(hyperText->GetCaretOffset(&caretOffset))) {
#ifdef DEBUG_A11Y
      PRUnichar chAtOffset;
      hyperText->GetCharacterAtOffset(caretOffset, &chAtOffset);
      printf("\nCaret moved to %d with char %c", caretOffset, chAtOffset);
#endif
      nsRefPtr<AccEvent> caretMoveEvent =
        new AccCaretMoveEvent(hyperText, caretOffset);
      nsEventShell::FireEvent(caretMoveEvent);

      PRInt32 selectionCount;
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
nsDocAccessible::ProcessContentInserted(nsAccessible* aContainer,
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

  
  
  
  
  
  
  
  
  for (PRUint32 idx = 0; idx < aInsertedContent->Length(); idx++) {
    nsAccessible* directContainer =
      GetContainerAccessible(aInsertedContent->ElementAt(idx));
    if (directContainer)
      UpdateTree(directContainer, aInsertedContent->ElementAt(idx), true);
  }
}

void
nsDocAccessible::UpdateTree(nsAccessible* aContainer, nsIContent* aChildNode,
                            bool aIsInsert)
{
  PRUint32 updateFlags = eNoAccessible;

  
  nsAccessible* child = GetAccessible(aChildNode);
  if (child) {
    updateFlags |= UpdateTreeInternal(child, aIsInsert);

  } else {
    nsAccTreeWalker walker(mWeakShell, aChildNode,
                           aContainer->GetAllowsAnonChildAccessibles(), true);

    while ((child = walker.NextChild()))
      updateFlags |= UpdateTreeInternal(child, aIsInsert);
  }

  
  if (updateFlags == eNoAccessible)
    return;

  
  
  if (aIsInsert && !(updateFlags & eAlertAccessible)) {
    
    
    nsAccessible* ancestor = aContainer;
    while (ancestor) {
      if (ancestor->ARIARole() == nsIAccessibleRole::ROLE_ALERT) {
        FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_ALERT,
                                   ancestor->GetNode());
        break;
      }

      
      if (ancestor == this)
        break;

      ancestor = ancestor->Parent();
    }
  }

  
  if (aContainer->Role() == nsIAccessibleRole::ROLE_ENTRY) {
    FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_VALUE_CHANGE,
                               aContainer->GetNode());
  }

  
  
  nsRefPtr<AccEvent> reorderEvent =
    new AccEvent(nsIAccessibleEvent::EVENT_REORDER, aContainer->GetNode(),
                 eAutoDetect, AccEvent::eCoalesceFromSameSubtree);
  if (reorderEvent)
    FireDelayedAccessibleEvent(reorderEvent);
}

PRUint32
nsDocAccessible::UpdateTreeInternal(nsAccessible* aChild, bool aIsInsert)
{
  PRUint32 updateFlags = eAccessible;

  nsINode* node = aChild->GetNode();
  if (aIsInsert) {
    
    CacheChildrenInSubtree(aChild);

  } else {
    

    
    
    
    
    
    
    
    if (aChild->ARIARole() == nsIAccessibleRole::ROLE_MENUPOPUP) {
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
    PRUint32 ariaRole = aChild->ARIARole();
    if (ariaRole == nsIAccessibleRole::ROLE_MENUPOPUP) {
      
      FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_MENUPOPUP_START,
                                 node, AccEvent::eRemoveDupes);

    } else if (ariaRole == nsIAccessibleRole::ROLE_ALERT) {
      
      updateFlags = eAlertAccessible;
      FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_ALERT, node,
                                 AccEvent::eRemoveDupes);
    }

    
    
    
    
    
    
    if (FocusMgr()->IsFocused(aChild))
      FocusMgr()->DispatchFocusEvent(this, aChild);

  } else {
    
    
    
    
    nsAccessible* parent = aChild->Parent();
    NS_ASSERTION(parent, "No accessible parent?!");
    if (parent)
      parent->RemoveChild(aChild);

    UncacheChildrenInSubtree(aChild);
  }

  return updateFlags;
}

void
nsDocAccessible::CacheChildrenInSubtree(nsAccessible* aRoot)
{
  aRoot->EnsureChildren();

  
  
  
  PRUint32 count = aRoot->ContentChildCount();
  for (PRUint32 idx = 0; idx < count; idx++) {
    nsAccessible* child = aRoot->ContentChildAt(idx);
    NS_ASSERTION(child, "Illicit tree change while tree is created!");
    
    if (child && child->IsContent())
      CacheChildrenInSubtree(child);
  }
}

void
nsDocAccessible::UncacheChildrenInSubtree(nsAccessible* aRoot)
{
  if (aRoot->IsElement())
    RemoveDependentIDsFor(aRoot);

  PRUint32 count = aRoot->ContentChildCount();
  for (PRUint32 idx = 0; idx < count; idx++)
    UncacheChildrenInSubtree(aRoot->ContentChildAt(idx));

  if (aRoot->IsPrimaryForNode() &&
      mNodeToAccessibleMap.Get(aRoot->GetNode()) == aRoot)
    mNodeToAccessibleMap.Remove(aRoot->GetNode());
}

void
nsDocAccessible::ShutdownChildrenInSubtree(nsAccessible* aAccessible)
{
  
  
  
  
  PRUint32 count = aAccessible->ContentChildCount();
  for (PRUint32 idx = 0, jdx = 0; idx < count; idx++) {
    nsAccessible* child = aAccessible->ContentChildAt(jdx);
    if (!child->IsBoundToParent()) {
      NS_ERROR("Parent refers to a child, child doesn't refer to parent!");
      jdx++;
    }

    ShutdownChildrenInSubtree(child);
  }

  UnbindFromDocument(aAccessible);
}

bool
nsDocAccessible::IsLoadEventTarget() const
{
  nsCOMPtr<nsISupports> container = mDocument->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
    do_QueryInterface(container);
  NS_ASSERTION(docShellTreeItem, "No document shell for document!");

  nsCOMPtr<nsIDocShellTreeItem> parentTreeItem;
  docShellTreeItem->GetParent(getter_AddRefs(parentTreeItem));

  
  if (parentTreeItem) {
    nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
    docShellTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));

    
    return (sameTypeRoot == docShellTreeItem);
  }

  
  PRInt32 contentType;
  docShellTreeItem->GetItemType(&contentType);
  return (contentType == nsIDocShellTreeItem::typeContent);
}

