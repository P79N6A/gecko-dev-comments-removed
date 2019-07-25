





































#include "nsAccCache.h"
#include "nsAccessibilityAtoms.h"
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
#include "nsIDOMNSDocument.h"
#include "nsIDOMNSHTMLDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMMutationEvent.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMXULPopupElement.h"
#include "nsIEditingSession.h"
#include "nsIEventStateManager.h"
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

namespace dom = mozilla::dom;




PRUint32 nsDocAccessible::gLastFocusedAccessiblesState = 0;





nsDocAccessible::
  nsDocAccessible(nsIDocument *aDocument, nsIContent *aRootContent,
                  nsIWeakReference *aShell) :
  nsHyperTextAccessibleWrap(aRootContent, aShell),
  mDocument(aDocument), mScrollPositionChangedTicks(0), mIsLoaded(PR_FALSE)
{
  
  mAccessibleCache.Init(kDefaultCacheSize);
  mNodeToAccessibleMap.Init(kDefaultCacheSize);

  
  if (!mDocument)
    return;

  
  
  AddScrollListener();
}

nsDocAccessible::~nsDocAccessible()
{
}





NS_IMPL_CYCLE_COLLECTION_CLASS(nsDocAccessible)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDocAccessible, nsAccessible)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mEventQueue");
  cb.NoteXPCOMChild(tmp->mEventQueue.get());

  PRUint32 i, length = tmp->mChildDocuments.Length();
  for (i = 0; i < length; ++i) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mChildDocuments[i]");
    cb.NoteXPCOMChild(static_cast<nsIAccessible*>(tmp->mChildDocuments[i].get()));
  }

  CycleCollectorTraverseCache(tmp->mAccessibleCache, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDocAccessible, nsAccessible)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mEventQueue)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSTARRAY(mChildDocuments)
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
    
    
    
    

    nsCOMPtr<nsIDOMXULDocument> xulDoc(do_QueryInterface(mDocument));
    if (xulDoc)
      status = nsAccessible::QueryInterface(aIID, (void**)&foundInterface);
    else
      status = nsHyperTextAccessible::QueryInterface(aIID,
                                                     (void**)&foundInterface);
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

NS_IMETHODIMP 
nsDocAccessible::GetDescription(nsAString& aDescription)
{
  if (mParent)
    mParent->GetDescription(aDescription);

  if (aDescription.IsEmpty()) {
    nsAutoString description;
    nsTextEquivUtils::
      GetTextEquivFromIDRefs(this, nsAccessibilityAtoms::aria_describedby,
                             description);
    aDescription = description;
  }

  return NS_OK;
}


nsresult
nsDocAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  *aState = 0;

  if (IsDefunct()) {
    if (aExtraState)
      *aExtraState = nsIAccessibleStates::EXT_STATE_DEFUNCT;

    return NS_OK_DEFUNCT_OBJECT;
  }

  if (aExtraState) {
    
    
    *aExtraState = (mContent->GetCurrentDoc() == mDocument) ?
      0 : nsIAccessibleStates::EXT_STATE_STALE;
  }

#ifdef MOZ_XUL
  nsCOMPtr<nsIXULDocument> xulDoc(do_QueryInterface(mDocument));
  if (!xulDoc)
#endif
  {
    
    
    
    *aState |= nsIAccessibleStates::STATE_FOCUSABLE;
    if (gLastFocusedNode == mDocument)
      *aState |= nsIAccessibleStates::STATE_FOCUSED;
  }

  if (nsCoreUtils::IsDocumentBusy(mDocument)) {
    *aState |= nsIAccessibleStates::STATE_BUSY;
    if (aExtraState) {
      *aExtraState |= nsIAccessibleStates::EXT_STATE_STALE;
    }
  }
 
  nsIFrame* frame = GetFrame();
  while (frame != nsnull && !frame->HasView()) {
    frame = frame->GetParent();
  }
 
  if (frame == nsnull ||
      !CheckVisibilityInParentChain(mDocument, frame->GetViewExternal())) {
    *aState |= nsIAccessibleStates::STATE_INVISIBLE |
               nsIAccessibleStates::STATE_OFFSCREEN;
  }

  nsCOMPtr<nsIEditor> editor;
  GetAssociatedEditor(getter_AddRefs(editor));
  if (!editor) {
    *aState |= nsIAccessibleStates::STATE_READONLY;
  }
  else if (aExtraState) {
    *aExtraState |= nsIAccessibleStates::EXT_STATE_EDITABLE;
  }

  return NS_OK;
}


nsresult
nsDocAccessible::GetARIAState(PRUint32 *aState, PRUint32 *aExtraState)
{
  
  NS_ENSURE_ARG_POINTER(aState);
  nsresult rv = nsAccessible::GetARIAState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mParent)  
    return mParent->GetARIAState(aState, aExtraState);

  return rv;
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

NS_IMETHODIMP nsDocAccessible::GetFocusedChild(nsIAccessible **aFocusedChild)
{
  
  if (!gLastFocusedNode) {
    *aFocusedChild = nsnull;
    return NS_OK;
  }

  
  
  NS_IF_ADDREF(*aFocusedChild = GetAccService()->GetAccessible(gLastFocusedNode));
  return NS_OK;
}

NS_IMETHODIMP nsDocAccessible::TakeFocus()
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRUint32 state;
  GetStateInternal(&state, nsnull);
  if (0 == (state & nsIAccessibleStates::STATE_FOCUSABLE)) {
    return NS_ERROR_FAILURE; 
  }

  
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

NS_IMETHODIMP nsDocAccessible::GetTitle(nsAString& aTitle)
{
  nsCOMPtr<nsIDOMNSDocument> domnsDocument(do_QueryInterface(mDocument));
  if (domnsDocument) {
    return domnsDocument->GetTitle(aTitle);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsDocAccessible::GetMimeType(nsAString& aMimeType)
{
  nsCOMPtr<nsIDOMNSDocument> domnsDocument(do_QueryInterface(mDocument));
  if (domnsDocument) {
    return domnsDocument->GetContentType(aMimeType);
  }
  return NS_ERROR_FAILURE;
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
  PRBool isEditable;
  editor->GetIsDocumentEditable(&isEditable);
  if (isEditable) {
    NS_ADDREF(*aEditor = editor);
  }
  return NS_OK;
}


nsAccessible *
nsDocAccessible::GetCachedAccessible(nsINode *aNode)
{
  nsAccessible* accessible = mNodeToAccessibleMap.Get(aNode);

  
  
  if (!accessible) {
    if (GetNode() != aNode)
      return nsnull;

    accessible = this;
  }

#ifdef DEBUG
  
  
  
  
  nsAccessible* parent(accessible->GetCachedParent());
  if (parent)
    parent->TestChildCache(accessible);
#endif

  return accessible;
}




PRBool
nsDocAccessible::Init()
{
  NS_LOG_ACCDOCCREATE_FOR("document initialize", mDocument, this)

  
  mEventQueue = new nsAccEventQueue(this);
  if (!mEventQueue)
    return PR_FALSE;

  AddEventListeners();

  nsDocAccessible* parentDocument = mParent->GetDocAccessible();
  if (parentDocument)
    parentDocument->AppendChildDocument(this);

  
  
  nsRefPtr<AccEvent> reorderEvent =
    new AccEvent(nsIAccessibleEvent::EVENT_REORDER, mParent, eAutoDetect,
                 AccEvent::eCoalesceFromSameSubtree);
  if (reorderEvent) {
    FireDelayedAccessibleEvent(reorderEvent);
    return PR_TRUE;
  }

  return PR_FALSE;
}

void
nsDocAccessible::Shutdown()
{
  if (!mWeakShell) 
    return;

  NS_LOG_ACCDOCDESTROY_FOR("document shutdown", mDocument, this)

  if (mEventQueue) {
    mEventQueue->Shutdown();
    mEventQueue = nsnull;
  }

  RemoveEventListeners();

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

  mNodeToAccessibleMap.Clear();
  ClearCache(mAccessibleCache);

  nsCOMPtr<nsIDocument> kungFuDeathGripDoc = mDocument;
  mDocument = nsnull;

  nsHyperTextAccessibleWrap::Shutdown();

  GetAccService()->NotifyOfDocumentShutdown(kungFuDeathGripDoc);
}

nsIFrame*
nsDocAccessible::GetFrame()
{
  nsCOMPtr<nsIPresShell> shell(do_QueryReferent(mWeakShell));

  nsIFrame* root = nsnull;
  if (shell)
    root = shell->GetRootFrame();

  return root;
}

PRBool
nsDocAccessible::IsDefunct()
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

  PRBool isContent = (itemType == nsIDocShellTreeItem::typeContent);

  if (isContent) {
    
    nsCOMPtr<nsICommandManager> commandManager = do_GetInterface(docShellTreeItem);
    if (commandManager) {
      commandManager->AddCommandObserver(this, "obs_documentCreated");
    }
  }

  nsCOMPtr<nsIDocShellTreeItem> rootTreeItem;
  docShellTreeItem->GetRootTreeItem(getter_AddRefs(rootTreeItem));
  if (rootTreeItem) {
    nsRefPtr<nsRootAccessible> rootAccessible = GetRootAccessible();
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

  nsRefPtr<nsRootAccessible> rootAccessible(GetRootAccessible());
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
      new AccStateChangeEvent(this, nsIAccessibleStates::EXT_STATE_EDITABLE,
                              PR_TRUE, PR_TRUE);
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
  
  
  
  
}

void
nsDocAccessible::AttributeChanged(nsIDocument *aDocument,
                                  dom::Element* aElement,
                                  PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                                  PRInt32 aModType)
{
  AttributeChangedImpl(aElement, aNameSpaceID, aAttribute);

  
  if (aElement == gLastFocusedNode) {
    nsAccessible *focusedAccessible = GetAccService()->GetAccessible(aElement);
    if (focusedAccessible)
      gLastFocusedAccessiblesState = nsAccUtils::State(focusedAccessible);
  }
}


void
nsDocAccessible::AttributeChangedImpl(nsIContent* aContent, PRInt32 aNameSpaceID, nsIAtom* aAttribute)
{
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  nsCOMPtr<nsISupports> container = mDocument->GetContainer();
  nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(container);
  if (!docShell) {
    return;
  }

  if (!IsContentLoaded())
    return; 

  nsCOMPtr<nsIPresShell> shell = GetPresShell();
  if (!shell) {
    return; 
  }

  NS_ASSERTION(aContent, "No node for attr modified");

  
  
  if (aAttribute == nsAccessibilityAtoms::disabled ||
      aAttribute == nsAccessibilityAtoms::aria_disabled) {

    
    
    

    
    

    nsRefPtr<AccEvent> enabledChangeEvent =
      new AccStateChangeEvent(aContent,
                              nsIAccessibleStates::EXT_STATE_ENABLED,
                              PR_TRUE);

    FireDelayedAccessibleEvent(enabledChangeEvent);

    nsRefPtr<AccEvent> sensitiveChangeEvent =
      new AccStateChangeEvent(aContent,
                              nsIAccessibleStates::EXT_STATE_SENSITIVE,
                              PR_TRUE);

    FireDelayedAccessibleEvent(sensitiveChangeEvent);
    return;
  }

  
  if (aNameSpaceID == kNameSpaceID_None) {
    
    if (StringBeginsWith(nsDependentAtomString(aAttribute),
                         NS_LITERAL_STRING("aria-"))) {
      ARIAAttributeChanged(aContent, aAttribute);
    }
  }

  if (aAttribute == nsAccessibilityAtoms::role) {
    if (mContent == aContent) {
      
      
      SetRoleMapEntry(nsAccUtils::GetRoleMapEntry(aContent));
    }
    else {
      
      
      
      RecreateAccessible(aContent);
    }
  }

  if (aAttribute == nsAccessibilityAtoms::href ||
      aAttribute == nsAccessibilityAtoms::onclick) {
    
    
    
    RecreateAccessible(aContent);
    return;
  }
  
  if (aAttribute == nsAccessibilityAtoms::alt ||
      aAttribute == nsAccessibilityAtoms::title ||
      aAttribute == nsAccessibilityAtoms::aria_label ||
      aAttribute == nsAccessibilityAtoms::aria_labelledby) {
    FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_NAME_CHANGE,
                               aContent);
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::selected ||
      aAttribute == nsAccessibilityAtoms::aria_selected) {
    

    nsAccessible *multiSelect =
      nsAccUtils::GetMultiSelectableContainer(aContent);
    
    
    
    
    
    if (multiSelect) {
      
      
      FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_SELECTION_WITHIN,
                                 multiSelect->GetNode(),
                                 AccEvent::eAllowDupes);

      static nsIContent::AttrValuesArray strings[] =
        {&nsAccessibilityAtoms::_empty, &nsAccessibilityAtoms::_false, nsnull};
      if (aContent->FindAttrValueIn(kNameSpaceID_None, aAttribute,
                                    strings, eCaseMatters) >= 0) {
        FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_SELECTION_REMOVE,
                                   aContent);
        return;
      }

      FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_SELECTION_ADD,
                                 aContent);
    }
  }

  if (aAttribute == nsAccessibilityAtoms::contenteditable) {
    nsRefPtr<AccEvent> editableChangeEvent =
      new AccStateChangeEvent(aContent,
                              nsIAccessibleStates::EXT_STATE_EDITABLE,
                              PR_TRUE);
    FireDelayedAccessibleEvent(editableChangeEvent);
    return;
  }
}


void
nsDocAccessible::ARIAAttributeChanged(nsIContent* aContent, nsIAtom* aAttribute)
{
  
  

  if (aAttribute == nsAccessibilityAtoms::aria_required) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(aContent, nsIAccessibleStates::STATE_REQUIRED,
                              PR_FALSE);
    FireDelayedAccessibleEvent(event);
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::aria_invalid) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(aContent, nsIAccessibleStates::STATE_INVALID,
                              PR_FALSE);
    FireDelayedAccessibleEvent(event);
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::aria_activedescendant) {
    
    
    nsCOMPtr<nsINode> focusedNode = GetCurrentFocus();
    if (nsCoreUtils::GetRoleContent(focusedNode) == aContent) {
      nsRefPtr<nsRootAccessible> rootAcc = GetRootAccessible();
      if (rootAcc) {
        rootAcc->FireAccessibleFocusEvent(nsnull, focusedNode, nsnull, PR_TRUE);
      }
    }
    return;
  }

  
  
  if (aAttribute == nsAccessibilityAtoms::aria_grabbed ||
      aAttribute == nsAccessibilityAtoms::aria_dropeffect) {
    FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_OBJECT_ATTRIBUTE_CHANGED,
                               aContent);
  }

  
  if (aAttribute == nsAccessibilityAtoms::aria_expanded) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(aContent, nsIAccessibleStates::STATE_EXPANDED,
                              PR_FALSE);
    FireDelayedAccessibleEvent(event);
    return;
  }

  if (!aContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::role)) {
    
    
    
    
    return;
  }

  
  if (aAttribute == nsAccessibilityAtoms::aria_checked ||
      aAttribute == nsAccessibilityAtoms::aria_pressed) {
    const PRUint32 kState = (aAttribute == nsAccessibilityAtoms::aria_checked) ?
                            nsIAccessibleStates::STATE_CHECKED : 
                            nsIAccessibleStates::STATE_PRESSED;
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(aContent, kState, PR_FALSE);
    FireDelayedAccessibleEvent(event);
    if (aContent == gLastFocusedNode) {
      
      
      
      
      nsAccessible *accessible = event->GetAccessible();
      if (accessible) {
        PRBool wasMixed = (gLastFocusedAccessiblesState & nsIAccessibleStates::STATE_MIXED) != 0;
        PRBool isMixed  =
          (nsAccUtils::State(accessible) & nsIAccessibleStates::STATE_MIXED) != 0;
        if (wasMixed != isMixed) {
          nsRefPtr<AccEvent> event =
            new AccStateChangeEvent(aContent, nsIAccessibleStates::STATE_MIXED,
                                    PR_FALSE, isMixed);
          FireDelayedAccessibleEvent(event);
        }
      }
    }
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::aria_readonly) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(aContent, nsIAccessibleStates::STATE_READONLY,
                              PR_FALSE);
    FireDelayedAccessibleEvent(event);
    return;
  }

  
  
  if (aAttribute == nsAccessibilityAtoms::aria_valuetext ||      
      (aAttribute == nsAccessibilityAtoms::aria_valuenow &&
       (!aContent->HasAttr(kNameSpaceID_None,
           nsAccessibilityAtoms::aria_valuetext) ||
        aContent->AttrValueIs(kNameSpaceID_None,
            nsAccessibilityAtoms::aria_valuetext, nsAccessibilityAtoms::_empty,
            eCaseMatters)))) {
    FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_VALUE_CHANGE,
                               aContent);
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::aria_multiselectable &&
      aContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::role)) {
    
    
    
    RecreateAccessible(aContent);
    return;
  }
}

void nsDocAccessible::ContentAppended(nsIDocument *aDocument,
                                      nsIContent* aContainer,
                                      nsIContent* aFirstNewContent,
                                      PRInt32 )
{
}

void nsDocAccessible::ContentStatesChanged(nsIDocument* aDocument,
                                           nsIContent* aContent1,
                                           nsIContent* aContent2,
                                           nsEventStates aStateMask)
{
  if (aStateMask.HasState(NS_EVENT_STATE_CHECKED)) {
    nsHTMLSelectOptionAccessible::SelectionChangedIfOption(aContent1);
    nsHTMLSelectOptionAccessible::SelectionChangedIfOption(aContent2);
  }

  if (aStateMask.HasState(NS_EVENT_STATE_INVALID)) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(aContent1, nsIAccessibleStates::STATE_INVALID,
                              PR_FALSE, PR_TRUE);
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
  FireTextChangeEventForText(aContent, aInfo, PR_FALSE);
}

void nsDocAccessible::CharacterDataChanged(nsIDocument *aDocument,
                                           nsIContent* aContent,
                                           CharacterDataChangeInfo* aInfo)
{
  FireTextChangeEventForText(aContent, aInfo, PR_TRUE);
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
nsDocAccessible::GetCachedAccessibleByUniqueIDInSubtree(void* aUniqueID)
{
  nsAccessible* child = GetCachedAccessibleByUniqueID(aUniqueID);
  if (child)
    return child;

  PRUint32 childDocCount = mChildDocuments.Length();
  for (PRUint32 childDocIdx= 0; childDocIdx < childDocCount; childDocIdx++) {
    nsDocAccessible* childDocument = mChildDocuments.ElementAt(childDocIdx);
    child = childDocument->GetCachedAccessibleByUniqueIDInSubtree(aUniqueID);
    if (child)
      return child;
  }

  return nsnull;
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
  return true;
}

void
nsDocAccessible::UnbindFromDocument(nsAccessible* aAccessible)
{
  
  if (aAccessible->IsPrimaryForNode() &&
      mNodeToAccessibleMap.Get(aAccessible->GetNode()) == aAccessible)
    mNodeToAccessibleMap.Remove(aAccessible->GetNode());

#ifdef DEBUG
  NS_ASSERTION(mAccessibleCache.GetWeak(aAccessible->UniqueID()),
               "Unbinding the unbound accessible!");
#endif

  void* uniqueID = aAccessible->UniqueID();
  aAccessible->Shutdown();
  mAccessibleCache.Remove(uniqueID);
}

void
nsDocAccessible::UpdateTree(nsIContent* aContainerNode,
                            nsIContent* aStartNode,
                            nsIContent* aEndNode,
                            PRBool aIsInsert)
{
  
  
  
  
  

  
  

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  nsIEventStateManager* esm = presShell->GetPresContext()->EventStateManager();
  PRBool fireAllEvents = PR_TRUE;

  
  
  
  
  nsAccessible* container = nsnull;
  if (aIsInsert) {
    container = aContainerNode ?
      GetAccService()->GetAccessibleOrContainer(aContainerNode, mWeakShell) :
      this;

    
    if (container == this) {
      nsIContent* rootContent = nsCoreUtils::GetRoleContent(mDocument);

      
      
      if (!rootContent)
        return;

      
      if (rootContent != mContent)
        mContent = rootContent;
    }

    
    
    
    
    
    container->InvalidateChildren();

  } else {
    
    container = aContainerNode ?
      GetAccService()->GetCachedAccessibleOrContainer(aContainerNode) :
      this;
  }

  EIsFromUserInput fromUserInput = esm->IsHandlingUserInputExternal() ?
    eFromUserInput : eNoUserInput;

  
  
  PRUint32 updateFlags =
    UpdateTreeInternal(container, aStartNode, aEndNode,
                       aIsInsert, fireAllEvents, fromUserInput);

  
  if (updateFlags == eNoAccessible)
    return;

  
  
  if (aIsInsert && !(updateFlags & eAlertAccessible)) {
    
    
    nsAccessible* ancestor = container;
    while (ancestor) {
      const nsRoleMapEntry* roleMapEntry = ancestor->GetRoleMapEntry();
      if (roleMapEntry && roleMapEntry->role == nsIAccessibleRole::ROLE_ALERT) {
        FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_ALERT,
                                   ancestor->GetNode(), AccEvent::eRemoveDupes,
                                   fromUserInput);
        break;
      }

      
      if (ancestor == this)
        break;

      ancestor = ancestor->GetParent();
    }
  }

  
  
  
  if (!fireAllEvents)
    return;

  
  if (container->Role() == nsIAccessibleRole::ROLE_ENTRY) {
    nsRefPtr<AccEvent> valueChangeEvent =
      new AccEvent(nsIAccessibleEvent::EVENT_VALUE_CHANGE, container,
                   fromUserInput, AccEvent::eRemoveDupes);
    FireDelayedAccessibleEvent(valueChangeEvent);
  }

  
  
  nsRefPtr<AccEvent> reorderEvent =
    new AccEvent(nsIAccessibleEvent::EVENT_REORDER, container->GetNode(),
                 fromUserInput, AccEvent::eCoalesceFromSameSubtree);
  if (reorderEvent)
    FireDelayedAccessibleEvent(reorderEvent);
}

void
nsDocAccessible::RecreateAccessible(nsINode* aNode)
{
  
  
  
  

  nsAccessible* parent = nsnull;

  
  nsAccessible* oldAccessible =
    GetAccService()->GetAccessibleInWeakShell(aNode, mWeakShell);
  if (oldAccessible) {
    parent = oldAccessible->GetParent();

    nsRefPtr<AccEvent> hideEvent = new AccHideEvent(oldAccessible, aNode,
                                                    eAutoDetect);
    if (hideEvent)
      FireDelayedAccessibleEvent(hideEvent);

    
    parent->RemoveChild(oldAccessible);

    if (oldAccessible->IsPrimaryForNode() &&
        mNodeToAccessibleMap.Get(oldAccessible->GetNode()) == oldAccessible)
      mNodeToAccessibleMap.Remove(oldAccessible->GetNode());

  } else {
    parent = GetAccService()->GetContainerAccessible(aNode, mWeakShell);
  }

  
  parent->InvalidateChildren();

  nsAccessible* newAccessible =
    GetAccService()->GetAccessibleInWeakShell(aNode, mWeakShell);
  if (newAccessible) {
    nsRefPtr<AccEvent> showEvent = new AccShowEvent(newAccessible, aNode,
                                                    eAutoDetect);
    if (showEvent)
      FireDelayedAccessibleEvent(showEvent);
  }

  
  if (oldAccessible || newAccessible) {
    nsRefPtr<AccEvent> reorderEvent =
      new AccEvent(nsIAccessibleEvent::EVENT_REORDER, parent->GetNode(),
                   eAutoDetect, AccEvent::eCoalesceFromSameSubtree);

    if (reorderEvent)
      FireDelayedAccessibleEvent(reorderEvent);
  }
}




void
nsDocAccessible::FireValueChangeForTextFields(nsAccessible *aAccessible)
{
  if (aAccessible->Role() != nsIAccessibleRole::ROLE_ENTRY)
    return;

  
  nsRefPtr<AccEvent> valueChangeEvent =
    new AccEvent(nsIAccessibleEvent::EVENT_VALUE_CHANGE, aAccessible,
                 eAutoDetect, AccEvent::eRemoveDupes);
  FireDelayedAccessibleEvent(valueChangeEvent);
}

void
nsDocAccessible::FireTextChangeEventForText(nsIContent *aContent,
                                            CharacterDataChangeInfo* aInfo,
                                            PRBool aIsInserted)
{
  if (!IsContentLoaded())
    return;

  PRInt32 contentOffset = aInfo->mChangeStart;
  PRUint32 contentLength = aIsInserted ?
    aInfo->mReplaceLength: 
    aInfo->mChangeEnd - contentOffset; 

  if (contentLength == 0)
    return;

  nsAccessible *accessible = GetAccService()->GetAccessible(aContent);
  if (!accessible)
    return;

  nsRefPtr<nsHyperTextAccessible> textAccessible =
    do_QueryObject(accessible->GetParent());
  if (!textAccessible)
    return;

  
  
  PRInt32 offset = textAccessible->GetChildOffset(accessible, PR_TRUE);

  
  nsIFrame* frame = aContent->GetPrimaryFrame();
  if (!frame)
    return;

  PRUint32 textOffset = 0;
  nsresult rv = textAccessible->ContentToRenderedOffset(frame, contentOffset,
                                                        &textOffset);
  if (NS_FAILED(rv))
    return;

  nsAutoString text;
  rv = accessible->AppendTextTo(text, textOffset, contentLength);
  if (NS_FAILED(rv))
    return;

  if (text.IsEmpty())
    return;

  
  
  
  nsRefPtr<AccEvent> event =
    new AccTextChangeEvent(textAccessible, offset + textOffset, text,
                          aIsInserted);
  FireDelayedAccessibleEvent(event);

  FireValueChangeForTextFields(textAccessible);
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

  if (mEventQueue)
    mEventQueue->Push(aEvent);

  return NS_OK;
}


void
nsDocAccessible::ProcessPendingEvent(AccEvent* aEvent)
{
  nsAccessible* accessible = aEvent->GetAccessible();
  if (!accessible)
    return;

  PRUint32 eventType = aEvent->GetEventType();

  if (eventType == nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED) {
    nsCOMPtr<nsIAccessibleText> accessibleText = do_QueryObject(accessible);
    PRInt32 caretOffset;
    if (accessibleText &&
        NS_SUCCEEDED(accessibleText->GetCaretOffset(&caretOffset))) {
#ifdef DEBUG_A11Y
      PRUnichar chAtOffset;
      accessibleText->GetCharacterAtOffset(caretOffset, &chAtOffset);
      printf("\nCaret moved to %d with char %c", caretOffset, chAtOffset);
#endif
#ifdef DEBUG_CARET
      
      
      nsAccessible* focusedAcc =
        GetAccService()->GetAccessible(gLastFocusedNode);
      nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_ALERT, focusedAcc);
#endif
      nsRefPtr<AccEvent> caretMoveEvent =
          new AccCaretMoveEvent(accessible, caretOffset);
      if (!caretMoveEvent)
        return;

      nsEventShell::FireEvent(caretMoveEvent);

      PRInt32 selectionCount;
      accessibleText->GetSelectionCount(&selectionCount);
      if (selectionCount) {  
        nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_TEXT_SELECTION_CHANGED,
                                accessible);
      }
    }
  }
  else {
    nsEventShell::FireEvent(aEvent);

    
    if (eventType == nsIAccessibleEvent::EVENT_HIDE)
      ShutdownChildrenInSubtree(accessible);
  }
}

PRUint32
nsDocAccessible::UpdateTreeInternal(nsAccessible* aContainer,
                                    nsIContent* aStartNode,
                                    nsIContent* aEndNode,
                                    PRBool aIsInsert,
                                    PRBool aFireAllEvents,
                                    EIsFromUserInput aFromUserInput)
{
  PRUint32 updateFlags = eNoAccessible;
  for (nsIContent* node = aStartNode; node != aEndNode;
       node = node->GetNextSibling()) {

    
    
    
    if (aIsInsert && !node->GetPrimaryFrame())
      continue;

    nsAccessible* accessible = aIsInsert ?
      GetAccService()->GetAccessibleInWeakShell(node, mWeakShell) :
      GetCachedAccessible(node);

    if (!accessible) {
      updateFlags |= UpdateTreeInternal(aContainer, node->GetFirstChild(),
                                        nsnull, aIsInsert, aFireAllEvents,
                                        aFromUserInput);
      continue;
    }

    updateFlags |= eAccessible;

    
    if (aFireAllEvents) {
      nsRefPtr<AccEvent> event;
      if (aIsInsert)
        event = new AccShowEvent(accessible, node, aFromUserInput);
      else
        event = new AccHideEvent(accessible, node, aFromUserInput);

      if (event)
        FireDelayedAccessibleEvent(event);
    }

    if (aIsInsert) {
      const nsRoleMapEntry* roleMapEntry = accessible->GetRoleMapEntry();
      if (roleMapEntry) {
        if (roleMapEntry->role == nsIAccessibleRole::ROLE_MENUPOPUP) {
          
          FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_MENUPOPUP_START,
                                     node, AccEvent::eRemoveDupes, aFromUserInput);

        } else if (roleMapEntry->role == nsIAccessibleRole::ROLE_ALERT) {
          
          updateFlags = eAlertAccessible;
          FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_ALERT, node,
                                     AccEvent::eRemoveDupes, aFromUserInput);
        }
      }

      
      
      
      
      if (node == gLastFocusedNode) {
        FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_FOCUS,
                                   node, AccEvent::eCoalesceFromSameDocument,
                                   aFromUserInput);
      }
    } else {
      
      aContainer->RemoveChild(accessible);
      UncacheChildrenInSubtree(accessible);
    }
  }

  return updateFlags;
}

void
nsDocAccessible::UncacheChildrenInSubtree(nsAccessible* aRoot)
{
  PRUint32 count = aRoot->GetCachedChildCount();
  for (PRUint32 idx = 0; idx < count; idx++)
    UncacheChildrenInSubtree(aRoot->GetCachedChildAt(idx));

  if (aRoot->IsPrimaryForNode() &&
      mNodeToAccessibleMap.Get(aRoot->GetNode()) == aRoot)
    mNodeToAccessibleMap.Remove(aRoot->GetNode());
}

void
nsDocAccessible::ShutdownChildrenInSubtree(nsAccessible* aAccessible)
{
  
  
  
  
  PRUint32 count = aAccessible->GetCachedChildCount();
  for (PRUint32 idx = 0, jdx = 0; idx < count; idx++) {
    nsAccessible* child = aAccessible->GetCachedChildAt(jdx);
    if (!child->IsBoundToParent()) {
      NS_ERROR("Parent refers to a child, child doesn't refer to parent!");
      jdx++;
    }

    ShutdownChildrenInSubtree(child);
  }

  UnbindFromDocument(aAccessible);
}

