





































#include "nsRootAccessible.h"
#include "nsAccessibilityAtoms.h"
#include "nsAccessibilityService.h"
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
#ifdef MOZ_XUL
#include "nsIXULDocument.h"
#endif




PRUint32 nsDocAccessible::gLastFocusedAccessiblesState = 0;
nsIAtom *nsDocAccessible::gLastFocusedFrameType = nsnull;





nsDocAccessible::nsDocAccessible(nsIDOMNode *aDOMNode, nsIWeakReference* aShell):
  nsHyperTextAccessibleWrap(aDOMNode, aShell), mWnd(nsnull),
  mScrollPositionChangedTicks(0), mIsContentLoaded(PR_FALSE),
  mIsLoadCompleteFired(PR_FALSE)
{
  
  mAccessNodeCache.Init(kDefaultCacheSize);

  
  if (!mDOMNode)
    return;

  
  
  
  

  nsCOMPtr<nsIPresShell> shell(do_QueryReferent(mWeakShell));
  if (shell) {
    
    mDocument = shell->GetDocument();
    
    
    nsIViewManager* vm = shell->GetViewManager();
    if (vm) {
      nsCOMPtr<nsIWidget> widget;
      vm->GetRootWidget(getter_AddRefs(widget));
      if (widget) {
        mWnd = widget->GetNativeData(NS_NATIVE_WINDOW);
      }
    }
  }

  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
    nsCoreUtils::GetDocShellTreeItemFor(mDOMNode);
  nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(docShellTreeItem);
  if (docShell) {
    PRUint32 busyFlags;
    docShell->GetBusyFlags(&busyFlags);
    if (busyFlags == nsIDocShell::BUSY_FLAGS_NONE) {
      mIsContentLoaded = PR_TRUE;                                               
    }
  }
}

nsDocAccessible::~nsDocAccessible()
{
}





NS_IMPL_CYCLE_COLLECTION_CLASS(nsDocAccessible)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDocAccessible, nsAccessible)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mEventQueue");
  cb.NoteXPCOMChild(tmp->mEventQueue.get());

  CycleCollectorTraverseCache(tmp->mAccessNodeCache, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDocAccessible, nsAccessible)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mEventQueue)
  ClearCache(tmp->mAccessNodeCache);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDocAccessible)
  NS_INTERFACE_MAP_STATIC_AMBIGUOUS(nsDocAccessible)
  NS_INTERFACE_MAP_ENTRY(nsIAccessibleDocument)
  NS_INTERFACE_MAP_ENTRY(nsIDocumentObserver)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIAccessibleDocument)
NS_INTERFACE_MAP_END_INHERITING(nsHyperTextAccessible)

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


nsresult
nsDocAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_PANE; 

  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
    nsCoreUtils::GetDocShellTreeItemFor(mDOMNode);
  if (docShellTreeItem) {
    nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
    docShellTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));
    PRInt32 itemType;
    docShellTreeItem->GetItemType(&itemType);
    if (sameTypeRoot == docShellTreeItem) {
      
      if (itemType == nsIDocShellTreeItem::typeChrome) {
        *aRole = nsIAccessibleRole::ROLE_CHROME_WINDOW;
      }
      else if (itemType == nsIDocShellTreeItem::typeContent) {
#ifdef MOZ_XUL
        nsCOMPtr<nsIXULDocument> xulDoc(do_QueryInterface(mDocument));
        if (xulDoc) {
          *aRole = nsIAccessibleRole::ROLE_APPLICATION;
        } else {
          *aRole = nsIAccessibleRole::ROLE_DOCUMENT;
        }
#else
        *aRole = nsIAccessibleRole::ROLE_DOCUMENT;
#endif
      }
    }
    else if (itemType == nsIDocShellTreeItem::typeContent) {
      *aRole = nsIAccessibleRole::ROLE_DOCUMENT;
    }
  }

  return NS_OK;
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
  nsCOMPtr<nsIDOMNode> ownerNode(do_QueryInterface(ownerContent));
  if (ownerNode) {
    nsRoleMapEntry *roleMapEntry = nsAccUtils::GetRoleMapEntry(ownerNode);
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
  nsresult rv = nsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

#ifdef MOZ_XUL
  nsCOMPtr<nsIXULDocument> xulDoc(do_QueryInterface(mDocument));
  if (!xulDoc)
#endif
  {
    
    
    
    *aState |= nsIAccessibleStates::STATE_FOCUSABLE;
    if (gLastFocusedNode == mDOMNode) {
      *aState |= nsIAccessibleStates::STATE_FOCUSED;
    }
  }

  if (!mIsContentLoaded) {
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

  
  
  nsCOMPtr<nsIAccessibilityService> accService =
    do_GetService("@mozilla.org/accessibilityService;1");
  return accService->GetAccessibleFor(gLastFocusedNode, aFocusedChild);
}

NS_IMETHODIMP nsDocAccessible::TakeFocus()
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_FAILURE);
  PRUint32 state;
  GetStateInternal(&state, nsnull);
  if (0 == (state & nsIAccessibleStates::STATE_FOCUSABLE)) {
    return NS_ERROR_FAILURE; 
  }

  nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
  if (fm) {
    nsCOMPtr<nsIDOMDocument> domDocument(do_QueryInterface(mDOMNode));
    nsCOMPtr<nsIDocument> document(do_QueryInterface(domDocument));
    if (document) {
      
      nsCOMPtr<nsIDOMElement> newFocus;
      return fm->MoveFocus(document->GetWindow(), nsnull,
                           nsIFocusManager::MOVEFOCUS_ROOT, 0,
                           getter_AddRefs(newFocus));
    }
  }
  return NS_ERROR_FAILURE;
}





NS_IMETHODIMP nsDocAccessible::GetURL(nsAString& aURL)
{
  if (!mDocument) {
    return NS_ERROR_FAILURE; 
  }
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
  *aWindow = mWnd;
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

NS_IMETHODIMP nsDocAccessible::GetDocument(nsIDOMDocument **aDOMDoc)
{
  nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(mDocument));
  *aDOMDoc = domDoc;

  if (domDoc) {
    NS_ADDREF(*aDOMDoc);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsDocAccessible::GetAssociatedEditor(nsIEditor **aEditor)
{
  NS_ENSURE_ARG_POINTER(aEditor);
  *aEditor = nsnull;

  if (!mDocument)
    return NS_ERROR_FAILURE;

  
  
  if (!mDocument->HasFlag(NODE_IS_EDITABLE)) {
    nsCOMPtr<nsIDOMNode> DOMDocument(do_QueryInterface(mDocument));
    nsCOMPtr<nsIDOMElement> DOMElement =
      nsCoreUtils::GetDOMElementFor(DOMDocument);
    nsCOMPtr<nsIContent> content(do_QueryInterface(DOMElement));

    if (!content || !content->HasFlag(NODE_IS_EDITABLE))
      return NS_OK;
  }

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

nsAccessNode*
nsDocAccessible::GetCachedAccessNode(void *aUniqueID)
{
  nsAccessNode* accessNode = mAccessNodeCache.GetWeak(aUniqueID);

  
  
  if (!accessNode) {
    void* thisUniqueID = nsnull;
    GetUniqueID(&thisUniqueID);
    if (thisUniqueID == aUniqueID)
      accessNode = this;
  }

#ifdef DEBUG
  
  
  
  
  nsRefPtr<nsAccessible> acc =
    nsAccUtils::QueryObject<nsAccessible>(accessNode);

  if (acc) {
    nsAccessible* parent(acc->GetCachedParent());
    if (parent)
      parent->TestChildCache(acc);
  }
#endif

  return accessNode;
}


PRBool
nsDocAccessible::CacheAccessNode(void *aUniqueID, nsAccessNode *aAccessNode)
{
  
  
  nsAccessNode* accessNode = mAccessNodeCache.GetWeak(aUniqueID);
  if (accessNode)
    accessNode->Shutdown();

  return mAccessNodeCache.Put(aUniqueID, aAccessNode);
}


void
nsDocAccessible::RemoveAccessNodeFromCache(nsIAccessNode *aAccessNode)
{
  if (!aAccessNode)
    return;

  void *uniqueID = nsnull;
  aAccessNode->GetUniqueID(&uniqueID);
  mAccessNodeCache.Remove(uniqueID);
}




nsresult
nsDocAccessible::Init()
{
  
  if (!gGlobalDocAccessibleCache.Put(static_cast<void*>(mDocument), this))
    return NS_ERROR_OUT_OF_MEMORY;

  
  mEventQueue = new nsAccEventQueue(this);
  if (!mEventQueue)
    return NS_ERROR_OUT_OF_MEMORY;

  AddEventListeners();

  
  GetParent();

  
  
  nsRefPtr<nsAccEvent> reorderEvent =
    new nsAccReorderEvent(mParent, PR_FALSE, PR_TRUE, mDOMNode);
  if (!reorderEvent)
    return NS_ERROR_OUT_OF_MEMORY;

  FireDelayedAccessibleEvent(reorderEvent);
  return NS_OK;
}

nsresult
nsDocAccessible::Shutdown()
{
  if (!mWeakShell) {
    return NS_OK;  
  }

  if (mEventQueue) {
    mEventQueue->Shutdown();
    mEventQueue = nsnull;
  }

  nsCOMPtr<nsIDocShellTreeItem> treeItem =
    nsCoreUtils::GetDocShellTreeItemFor(mDOMNode);
  ShutdownChildDocuments(treeItem);

  RemoveEventListeners();

  mWeakShell = nsnull;  

  ClearCache(mAccessNodeCache);

  nsCOMPtr<nsIDocument> kungFuDeathGripDoc = mDocument;
  mDocument = nsnull;

  nsHyperTextAccessibleWrap::Shutdown();

  
  
  
  if (!nsAccessibilityService::gIsShutdown)
    gGlobalDocAccessibleCache.Remove(static_cast<void*>(kungFuDeathGripDoc));

  return NS_OK;
}


void nsDocAccessible::ShutdownChildDocuments(nsIDocShellTreeItem *aStart)
{
  nsCOMPtr<nsIDocShellTreeNode> treeNode(do_QueryInterface(aStart));
  if (treeNode) {
    PRInt32 subDocuments;
    treeNode->GetChildCount(&subDocuments);
    for (PRInt32 count = 0; count < subDocuments; count ++) {
      nsCOMPtr<nsIDocShellTreeItem> treeItemChild;
      treeNode->GetChildAt(count, getter_AddRefs(treeItemChild));
      NS_ASSERTION(treeItemChild, "No tree item when there should be");
      if (!treeItemChild) {
        continue;
      }
      nsCOMPtr<nsIAccessibleDocument> docAccessible =
        GetDocAccessibleFor(treeItemChild);
      if (docAccessible) {
        nsRefPtr<nsAccessNode> docAccNode =
          nsAccUtils::QueryAccessNode(docAccessible);
        docAccNode->Shutdown();
      }
    }
  }
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
  if (nsHyperTextAccessibleWrap::IsDefunct())
    return PR_TRUE;

  return !mDocument;
}


void nsDocAccessible::GetBoundsRect(nsRect& aBounds, nsIFrame** aRelativeFrame)
{
  *aRelativeFrame = GetFrame();

  nsIDocument *document = mDocument;
  nsIDocument *parentDoc = nsnull;

  while (document) {
    nsIPresShell *presShell = document->GetPrimaryShell();
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
    nsCOMPtr<nsIAccessibleDocument> rootAccDoc =
      GetDocAccessibleFor(rootTreeItem, PR_TRUE); 
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


void
nsDocAccessible::FireDocLoadEvents(PRUint32 aEventType)
{
  if (IsDefunct())
    return;

  PRBool isFinished = 
             (aEventType == nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_COMPLETE ||
              aEventType == nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_STOPPED);

  mIsContentLoaded = isFinished;
  if (isFinished) {
    if (mIsLoadCompleteFired)
      return;

    mIsLoadCompleteFired = PR_TRUE;
  }

  nsCOMPtr<nsIDocShellTreeItem> treeItem =
    nsCoreUtils::GetDocShellTreeItemFor(mDOMNode);
  if (!treeItem)
    return;

  nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
  treeItem->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));

  if (isFinished) {
    
    AddScrollListener();
    nsRefPtr<nsAccessible> acc(GetParent());
    if (acc) {
      
      acc->InvalidateChildren();
    }

    if (sameTypeRoot != treeItem) {
      
      
      InvalidateCacheSubtree(nsnull,
                             nsIAccessibilityService::NODE_SIGNIFICANT_CHANGE);
    }
    
    if (gLastFocusedNode) {
      nsCOMPtr<nsIDocShellTreeItem> focusedTreeItem =
        nsCoreUtils::GetDocShellTreeItemFor(gLastFocusedNode);
      if (focusedTreeItem) {
        nsCOMPtr<nsIDocShellTreeItem> sameTypeRootOfFocus;
        focusedTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRootOfFocus));
        if (sameTypeRoot == sameTypeRootOfFocus) {
          nsRefPtr<nsAccEvent> accEvent =
            new nsAccStateChangeEvent(this, nsIAccessibleStates::STATE_BUSY, PR_FALSE, PR_FALSE);
          nsEventShell::FireEvent(accEvent);
        }
      }
    }
  }

  if (sameTypeRoot == treeItem) {
    
    if (!isFinished) {
      
      
      nsRefPtr<nsAccEvent> accEvent =
        new nsAccStateChangeEvent(this, nsIAccessibleStates::STATE_BUSY,
                                  PR_FALSE, PR_TRUE);
      nsEventShell::FireEvent(accEvent);
    }

    nsEventShell::FireEvent(aEventType, this);
  }
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
    
    nsRefPtr<nsAccEvent> event =
      new nsAccStateChangeEvent(this, nsIAccessibleStates::EXT_STATE_EDITABLE,
                                PR_TRUE, PR_TRUE);
    nsEventShell::FireEvent(event);
  }

  return NS_OK;
}




NS_IMPL_NSIDOCUMENTOBSERVER_CORE_STUB(nsDocAccessible)
NS_IMPL_NSIDOCUMENTOBSERVER_LOAD_STUB(nsDocAccessible)
NS_IMPL_NSIDOCUMENTOBSERVER_STYLE_STUB(nsDocAccessible)

void
nsDocAccessible::AttributeWillChange(nsIDocument *aDocument,
                                     nsIContent* aContent, PRInt32 aNameSpaceID,
                                     nsIAtom* aAttribute, PRInt32 aModType)
{
  
  
  
  
}

void
nsDocAccessible::AttributeChanged(nsIDocument *aDocument, nsIContent* aContent,
                                  PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                                  PRInt32 aModType)
{
  AttributeChangedImpl(aContent, aNameSpaceID, aAttribute);

  
  nsCOMPtr<nsIDOMNode> targetNode = do_QueryInterface(aContent);
  if (targetNode == gLastFocusedNode) {
    nsCOMPtr<nsIAccessible> focusedAccessible;
    GetAccService()->GetAccessibleFor(targetNode, getter_AddRefs(focusedAccessible));
    if (focusedAccessible) {
      gLastFocusedAccessiblesState = nsAccUtils::State(focusedAccessible);
    }
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

  PRUint32 busyFlags;
  docShell->GetBusyFlags(&busyFlags);
  if (busyFlags) {
    return; 
  }

  nsCOMPtr<nsIPresShell> shell = GetPresShell();
  if (!shell) {
    return; 
  }

  nsCOMPtr<nsIDOMNode> targetNode(do_QueryInterface(aContent));
  NS_ASSERTION(targetNode, "No node for attr modified");
  if (!targetNode || !nsAccUtils::IsNodeRelevant(targetNode))
    return;

  
  
  if (aAttribute == nsAccessibilityAtoms::disabled ||
      aAttribute == nsAccessibilityAtoms::aria_disabled) {

    
    
    

    
    

    nsRefPtr<nsAccEvent> enabledChangeEvent =
      new nsAccStateChangeEvent(targetNode,
                                nsIAccessibleStates::EXT_STATE_ENABLED,
                                PR_TRUE);

    FireDelayedAccessibleEvent(enabledChangeEvent);

    nsRefPtr<nsAccEvent> sensitiveChangeEvent =
      new nsAccStateChangeEvent(targetNode,
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

  if (aAttribute == nsAccessibilityAtoms::role ||
      aAttribute == nsAccessibilityAtoms::href ||
      aAttribute == nsAccessibilityAtoms::onclick) {
    
    
    
    InvalidateCacheSubtree(aContent,
                           nsIAccessibilityService::NODE_SIGNIFICANT_CHANGE);
    return;
  }
  
  if (aAttribute == nsAccessibilityAtoms::alt ||
      aAttribute == nsAccessibilityAtoms::title ||
      aAttribute == nsAccessibilityAtoms::aria_label ||
      aAttribute == nsAccessibilityAtoms::aria_labelledby) {
    FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_NAME_CHANGE,
                               targetNode);
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::selected ||
      aAttribute == nsAccessibilityAtoms::aria_selected) {
    
    nsCOMPtr<nsIAccessible> multiSelect =
      nsAccUtils::GetMultiSelectableContainer(targetNode);
    
    
    
    
    
    if (multiSelect) {
      
      
      nsCOMPtr<nsIAccessNode> multiSelectAccessNode =
        do_QueryInterface(multiSelect);
      nsCOMPtr<nsIDOMNode> multiSelectDOMNode;
      multiSelectAccessNode->GetDOMNode(getter_AddRefs(multiSelectDOMNode));
      NS_ASSERTION(multiSelectDOMNode, "A new accessible without a DOM node!");
      FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_SELECTION_WITHIN,
                                 multiSelectDOMNode,
                                 nsAccEvent::eAllowDupes);

      static nsIContent::AttrValuesArray strings[] =
        {&nsAccessibilityAtoms::_empty, &nsAccessibilityAtoms::_false, nsnull};
      if (aContent->FindAttrValueIn(kNameSpaceID_None, aAttribute,
                                    strings, eCaseMatters) >= 0) {
        FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_SELECTION_REMOVE,
                                   targetNode);
        return;
      }

      FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_SELECTION_ADD,
                                 targetNode);
    }
  }

  if (aAttribute == nsAccessibilityAtoms::contenteditable) {
    nsRefPtr<nsAccEvent> editableChangeEvent =
      new nsAccStateChangeEvent(targetNode,
                                nsIAccessibleStates::EXT_STATE_EDITABLE,
                                PR_TRUE);
    FireDelayedAccessibleEvent(editableChangeEvent);
    return;
  }
}


void
nsDocAccessible::ARIAAttributeChanged(nsIContent* aContent, nsIAtom* aAttribute)
{
  nsCOMPtr<nsIDOMNode> targetNode(do_QueryInterface(aContent));
  if (!targetNode)
    return;

  
  

  if (aAttribute == nsAccessibilityAtoms::aria_required) {
    nsRefPtr<nsAccEvent> event =
      new nsAccStateChangeEvent(targetNode,
                                nsIAccessibleStates::STATE_REQUIRED,
                                PR_FALSE);
    FireDelayedAccessibleEvent(event);
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::aria_invalid) {
    nsRefPtr<nsAccEvent> event =
      new nsAccStateChangeEvent(targetNode,
                                nsIAccessibleStates::STATE_INVALID,
                                PR_FALSE);
    FireDelayedAccessibleEvent(event);
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::aria_activedescendant) {
    
    
    nsCOMPtr<nsIDOMNode> currentFocus = GetCurrentFocus();
    if (SameCOMIdentity(nsCoreUtils::GetRoleContent(currentFocus), targetNode)) {
      nsRefPtr<nsRootAccessible> rootAcc = GetRootAccessible();
      if (rootAcc)
        rootAcc->FireAccessibleFocusEvent(nsnull, currentFocus, nsnull, PR_TRUE);
    }
    return;
  }

  
  
  if (aAttribute == nsAccessibilityAtoms::aria_grabbed ||
      aAttribute == nsAccessibilityAtoms::aria_dropeffect) {
    FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_OBJECT_ATTRIBUTE_CHANGED,
                               targetNode);
  }

  
  if (aAttribute == nsAccessibilityAtoms::aria_expanded) {
    nsRefPtr<nsAccEvent> event =
      new nsAccStateChangeEvent(targetNode,
                                nsIAccessibleStates::STATE_EXPANDED,
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
    nsRefPtr<nsAccEvent> event =
      new nsAccStateChangeEvent(targetNode, kState, PR_FALSE);
    FireDelayedAccessibleEvent(event);
    if (targetNode == gLastFocusedNode) {
      
      
      
      
      nsCOMPtr<nsIAccessible> accessible;
      event->GetAccessible(getter_AddRefs(accessible));
      if (accessible) {
        PRBool wasMixed = (gLastFocusedAccessiblesState & nsIAccessibleStates::STATE_MIXED) != 0;
        PRBool isMixed  =
          (nsAccUtils::State(accessible) & nsIAccessibleStates::STATE_MIXED) != 0;
        if (wasMixed != isMixed) {
          nsRefPtr<nsAccEvent> event =
            new nsAccStateChangeEvent(targetNode,
                                      nsIAccessibleStates::STATE_MIXED,
                                      PR_FALSE, isMixed);
          FireDelayedAccessibleEvent(event);
        }
      }
    }
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::aria_readonly) {
    nsRefPtr<nsAccEvent> event =
      new nsAccStateChangeEvent(targetNode,
                                nsIAccessibleStates::STATE_READONLY,
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
                               targetNode);
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::aria_multiselectable &&
      aContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::role)) {
    
    
    
    InvalidateCacheSubtree(aContent,
                           nsIAccessibilityService::NODE_SIGNIFICANT_CHANGE);
    return;
  }
}

void nsDocAccessible::ContentAppended(nsIDocument *aDocument,
                                      nsIContent* aContainer,
                                      PRInt32 aNewIndexInContainer)
{
  if ((!mIsContentLoaded || !mDocument) && mAccessNodeCache.Count() <= 1) {
    
    InvalidateChildren();
    return;
  }

  PRUint32 childCount = aContainer->GetChildCount();
  for (PRUint32 index = aNewIndexInContainer; index < childCount; index ++) {
    nsCOMPtr<nsIContent> child(aContainer->GetChildAt(index));
    
    
    
    
    InvalidateCacheSubtree(child, nsIAccessibilityService::NODE_APPEND);
  }
}


void nsDocAccessible::ContentStatesChanged(nsIDocument* aDocument,
                                           nsIContent* aContent1,
                                           nsIContent* aContent2,
                                           PRInt32 aStateMask)
{
  if (0 == (aStateMask & NS_EVENT_STATE_CHECKED)) {
    return;
  }

  nsHTMLSelectOptionAccessible::SelectionChangedIfOption(aContent1);
  nsHTMLSelectOptionAccessible::SelectionChangedIfOption(aContent2);
}

void nsDocAccessible::DocumentStatesChanged(nsIDocument* aDocument,
                                            PRInt32 aStateMask)
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
                                 nsIContent* aChild, PRInt32 aIndexInContainer)
{
  
  
  
  
  InvalidateCacheSubtree(aChild, nsIAccessibilityService::NODE_APPEND);
}

void
nsDocAccessible::ContentRemoved(nsIDocument *aDocument, nsIContent* aContainer,
                                nsIContent* aChild, PRInt32 aIndexInContainer)
{
  
  
  
  
  
  
}

void
nsDocAccessible::ParentChainChanged(nsIContent *aContent)
{
}





nsAccessible*
nsDocAccessible::GetParent()
{
  if (IsDefunct())
    return nsnull;

  if (mParent)
    return mParent;

  nsIDocument* parentDoc = mDocument->GetParentDocument();
  if (!parentDoc)
    return nsnull;

  nsIContent* ownerContent = parentDoc->FindContentForSubDocument(mDocument);
  nsCOMPtr<nsIDOMNode> ownerNode(do_QueryInterface(ownerContent));
  if (ownerNode) {
    nsCOMPtr<nsIAccessibilityService> accService = GetAccService();
    if (accService) {
      
      
      
      
      
      nsCOMPtr<nsIAccessible> parent;
      accService->GetAccessibleFor(ownerNode, getter_AddRefs(parent));
      mParent = nsAccUtils::QueryObject<nsAccessible>(parent);
    }
  }

  NS_ASSERTION(mParent, "No parent for not root document accessible!");
  return mParent;
}





void
nsDocAccessible::FireValueChangeForTextFields(nsIAccessible *aPossibleTextFieldAccessible)
{
  if (nsAccUtils::Role(aPossibleTextFieldAccessible) != nsIAccessibleRole::ROLE_ENTRY)
    return;

  
  nsRefPtr<nsAccEvent> valueChangeEvent =
    new nsAccEvent(nsIAccessibleEvent::EVENT_VALUE_CHANGE, aPossibleTextFieldAccessible,
                   PR_FALSE, eAutoDetect, nsAccEvent::eRemoveDupes);
  FireDelayedAccessibleEvent(valueChangeEvent);
}

void
nsDocAccessible::FireTextChangeEventForText(nsIContent *aContent,
                                            CharacterDataChangeInfo* aInfo,
                                            PRBool aIsInserted)
{
  if (!mIsContentLoaded || !mDocument) {
    return;
  }

  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(aContent));
  if (!node)
    return;

  nsCOMPtr<nsIAccessible> accessible;
  nsresult rv = GetAccessibleInParentChain(node, PR_TRUE, getter_AddRefs(accessible));
  if (NS_FAILED(rv) || !accessible)
    return;

  nsRefPtr<nsHyperTextAccessible> textAccessible;
  rv = accessible->QueryInterface(NS_GET_IID(nsHyperTextAccessible),
                                  getter_AddRefs(textAccessible));
  if (NS_FAILED(rv) || !textAccessible)
    return;

  PRInt32 start = aInfo->mChangeStart;

  PRInt32 offset = 0;
  rv = textAccessible->DOMPointToHypertextOffset(node, start, &offset);
  if (NS_FAILED(rv))
    return;

  PRInt32 length = aIsInserted ?
    aInfo->mReplaceLength: 
    aInfo->mChangeEnd - start; 

  if (length > 0) {
    PRUint32 renderedStartOffset, renderedEndOffset;
    nsIFrame* frame = aContent->GetPrimaryFrame();
    if (!frame)
      return;

    rv = textAccessible->ContentToRenderedOffset(frame, start,
                                                 &renderedStartOffset);
    if (NS_FAILED(rv))
      return;

    rv = textAccessible->ContentToRenderedOffset(frame, start + length,
                                                 &renderedEndOffset);
    if (NS_FAILED(rv))
      return;

    nsRefPtr<nsAccEvent> event =
      new nsAccTextChangeEvent(accessible, offset,
                               renderedEndOffset - renderedStartOffset,
                               aIsInserted, PR_FALSE);
    nsEventShell::FireEvent(event);

    FireValueChangeForTextFields(accessible);
  }
}

already_AddRefed<nsAccEvent>
nsDocAccessible::CreateTextChangeEventForNode(nsIAccessible *aContainerAccessible,
                                              nsIDOMNode *aChangeNode,
                                              nsIAccessible *aAccessibleForChangeNode,
                                              PRBool aIsInserting,
                                              PRBool aIsAsynch,
                                              EIsFromUserInput aIsFromUserInput)
{
  nsRefPtr<nsHyperTextAccessible> textAccessible;
  aContainerAccessible->QueryInterface(NS_GET_IID(nsHyperTextAccessible),
                                       getter_AddRefs(textAccessible));
  if (!textAccessible) {
    return nsnull;
  }

  PRInt32 offset;
  PRInt32 length = 0;
  nsCOMPtr<nsIAccessible> changeAccessible;
  nsresult rv = textAccessible->DOMPointToHypertextOffset(aChangeNode, -1, &offset,
                                                          getter_AddRefs(changeAccessible));
  NS_ENSURE_SUCCESS(rv, nsnull);

  if (!aAccessibleForChangeNode) {
    
    
    
    
    
    if (!changeAccessible) {
      return nsnull; 
    }

    nsCOMPtr<nsINode> changeNode(do_QueryInterface(aChangeNode));
    nsCOMPtr<nsIAccessible> child = changeAccessible;
    while (PR_TRUE) {
      nsCOMPtr<nsIAccessNode> childAccessNode =
        do_QueryInterface(changeAccessible);

      nsCOMPtr<nsIDOMNode> childDOMNode;
      childAccessNode->GetDOMNode(getter_AddRefs(childDOMNode));

      nsCOMPtr<nsINode> childNode(do_QueryInterface(childDOMNode));
      if (!nsCoreUtils::IsAncestorOf(changeNode, childNode)) {
        break;  
      }
      length += nsAccUtils::TextLength(child);
      child->GetNextSibling(getter_AddRefs(changeAccessible));
      if (!changeAccessible) {
        break;
      }
      child.swap(changeAccessible);
    }
  }
  else {
    NS_ASSERTION(!changeAccessible || changeAccessible == aAccessibleForChangeNode,
                 "Hypertext is reporting a different accessible for this node");

    length = nsAccUtils::TextLength(aAccessibleForChangeNode);
    if (nsAccUtils::Role(aAccessibleForChangeNode) == nsIAccessibleRole::ROLE_WHITESPACE) {  
      
      nsCOMPtr<nsIEditor> editor;
      textAccessible->GetAssociatedEditor(getter_AddRefs(editor));
      if (editor) {
        PRBool isEmpty = PR_FALSE;
        editor->GetDocumentIsEmpty(&isEmpty);
        if (isEmpty) {
          return nsnull;
        }
      }
    }
  }

  if (length <= 0) {
    return nsnull;
  }

  nsAccEvent *event =
    new nsAccTextChangeEvent(aContainerAccessible, offset, length, aIsInserting,
                             aIsAsynch, aIsFromUserInput);
  NS_IF_ADDREF(event);

  return event;
}


nsresult
nsDocAccessible::FireDelayedAccessibleEvent(PRUint32 aEventType,
                                            nsIDOMNode *aDOMNode,
                                            nsAccEvent::EEventRule aAllowDupes,
                                            PRBool aIsAsynch,
                                            EIsFromUserInput aIsFromUserInput)
{
  nsRefPtr<nsAccEvent> event =
    new nsAccEvent(aEventType, aDOMNode, aIsAsynch, aIsFromUserInput, aAllowDupes);
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  return FireDelayedAccessibleEvent(event);
}


nsresult
nsDocAccessible::FireDelayedAccessibleEvent(nsAccEvent *aEvent)
{
  NS_ENSURE_ARG(aEvent);

  if (mEventQueue)
    mEventQueue->Push(aEvent);

  return NS_OK;
}

void
nsDocAccessible::ProcessPendingEvent(nsAccEvent *aEvent)
{  
  nsCOMPtr<nsIAccessible> accessible;
  aEvent->GetAccessible(getter_AddRefs(accessible));

  nsCOMPtr<nsIDOMNode> domNode;
  aEvent->GetDOMNode(getter_AddRefs(domNode));

  PRUint32 eventType = aEvent->GetEventType();
  EIsFromUserInput isFromUserInput =
    aEvent->IsFromUserInput() ? eFromUserInput : eNoUserInput;

  PRBool isAsync = aEvent->IsAsync();

  if (domNode == gLastFocusedNode && isAsync &&
      (eventType == nsIAccessibleEvent::EVENT_SHOW ||
       eventType == nsIAccessibleEvent::EVENT_HIDE)) {
    
    
    
    
    nsCOMPtr<nsIContent> focusContent(do_QueryInterface(domNode));
    if (focusContent) {
      nsIFrame *focusFrame = focusContent->GetPrimaryFrame();
      nsIAtom *newFrameType =
        (focusFrame && focusFrame->GetStyleVisibility()->IsVisible()) ?
        focusFrame->GetType() : nsnull;

      if (newFrameType == gLastFocusedFrameType) {
        
        
        FireShowHideEvents(domNode, PR_TRUE, eventType, eNormalEvent,
                           isAsync, isFromUserInput); 
        return;
      }
      gLastFocusedFrameType = newFrameType;
    }
  }

  if (eventType == nsIAccessibleEvent::EVENT_SHOW) {

    nsCOMPtr<nsIAccessible> containerAccessible;
    if (accessible)
      accessible->GetParent(getter_AddRefs(containerAccessible));

    if (!containerAccessible) {
      GetAccessibleInParentChain(domNode, PR_TRUE,
                                 getter_AddRefs(containerAccessible));
      if (!containerAccessible)
        containerAccessible = this;
    }

    if (isAsync) {
      
      nsRefPtr<nsAccessible> containerAcc =
        nsAccUtils::QueryAccessible(containerAccessible);
      if (containerAcc)
        containerAcc->InvalidateChildren();

      
      
      
      InvalidateChildrenInSubtree(domNode);
    }

    
    
    
    
    
    if (domNode && domNode != mDOMNode) {
      nsRefPtr<nsAccEvent> textChangeEvent =
        CreateTextChangeEventForNode(containerAccessible, domNode, accessible,
                                     PR_TRUE, PR_TRUE, isFromUserInput);
      if (textChangeEvent) {
        
        
        
        nsEventShell::FireEvent(textChangeEvent);
      }
    }

    
    FireShowHideEvents(domNode, PR_FALSE, eventType, eNormalEvent, isAsync,
                       isFromUserInput); 
    return;
  }

  if (accessible) {
    if (eventType == nsIAccessibleEvent::EVENT_INTERNAL_LOAD) {
      nsRefPtr<nsDocAccessible> docAcc =
        nsAccUtils::QueryAccessibleDocument(accessible);
      NS_ASSERTION(docAcc, "No doc accessible for doc load event");

      if (docAcc)
        docAcc->FireDocLoadEvents(nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_COMPLETE);
    }
    else if (eventType == nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED) {
      nsCOMPtr<nsIAccessibleText> accessibleText = do_QueryInterface(accessible);
      PRInt32 caretOffset;
      if (accessibleText && NS_SUCCEEDED(accessibleText->GetCaretOffset(&caretOffset))) {
#ifdef DEBUG_A11Y
        PRUnichar chAtOffset;
        accessibleText->GetCharacterAtOffset(caretOffset, &chAtOffset);
        printf("\nCaret moved to %d with char %c", caretOffset, chAtOffset);
#endif
#ifdef DEBUG_CARET
        
        
        nsCOMPtr<nsIAccessible> accForFocus;
        GetAccService()->GetAccessibleFor(gLastFocusedNode, getter_AddRefs(accForFocus));
        nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_ALERT, accForFocus);
#endif
        nsRefPtr<nsAccEvent> caretMoveEvent =
          new nsAccCaretMoveEvent(accessible, caretOffset);
        if (!caretMoveEvent)
          return;

        nsEventShell::FireEvent(caretMoveEvent);

        PRInt32 selectionCount;
        accessibleText->GetSelectionCount(&selectionCount);
        if (selectionCount) {  
          nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_TEXT_SELECTION_CHANGED,
                                  accessible, PR_TRUE);
        }
      } 
    }
    else if (eventType == nsIAccessibleEvent::EVENT_REORDER) {
      
      
      
      nsCOMPtr<nsAccReorderEvent> reorderEvent = do_QueryInterface(aEvent);
      if (reorderEvent->IsUnconditionalEvent() ||
          reorderEvent->HasAccessibleInReasonSubtree()) {
        nsEventShell::FireEvent(aEvent);
      }
    }
    else {
      nsEventShell::FireEvent(aEvent);

      
      if (eventType == nsIAccessibleEvent::EVENT_HIDE) {
        
        
        nsCOMPtr<nsIDOMNode> hidingNode;
        aEvent->GetDOMNode(getter_AddRefs(hidingNode));
        if (hidingNode) {
          RefreshNodes(hidingNode); 
        }
      }
    }
  }
}

void nsDocAccessible::InvalidateChildrenInSubtree(nsIDOMNode *aStartNode)
{
  nsRefPtr<nsAccessible> acc =
    nsAccUtils::QueryObject<nsAccessible>(GetCachedAccessNode(aStartNode));
  if (acc)
    acc->InvalidateChildren();

  
  nsCOMPtr<nsINode> node = do_QueryInterface(aStartNode);
  PRInt32 index, numChildren = node->GetChildCount();
  for (index = 0; index < numChildren; index ++) {
    nsCOMPtr<nsIDOMNode> childNode = do_QueryInterface(node->GetChildAt(index));
    if (childNode)
      InvalidateChildrenInSubtree(childNode);
  }
}

void nsDocAccessible::RefreshNodes(nsIDOMNode *aStartNode)
{
  if (mAccessNodeCache.Count() <= 1) {
    return; 
  }

  nsRefPtr<nsAccessNode> accessNode = GetCachedAccessNode(aStartNode);

  
  
  nsRefPtr<nsAccessible> accessible =
    nsAccUtils::QueryObject<nsAccessible>(accessNode);
  if (accessible) {
    
    PRUint32 role = nsAccUtils::Role(accessible);
    if (role == nsIAccessibleRole::ROLE_MENUPOPUP) {
      nsCOMPtr<nsIDOMNode> domNode;
      accessNode->GetDOMNode(getter_AddRefs(domNode));
      nsCOMPtr<nsIDOMXULPopupElement> popup(do_QueryInterface(domNode));
      if (!popup) {
        
        
        nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_MENUPOPUP_END,
                                accessible);
      }
    }

    
    
    if (accessible->GetCachedFirstChild()) {
      nsCOMPtr<nsIArray> children;
      
      
      
      accessible->GetChildren(getter_AddRefs(children));
      PRUint32 childCount =0;
      if (children)
        children->GetLength(&childCount);
      nsCOMPtr<nsIDOMNode> possibleAnonNode;
      for (PRUint32 index = 0; index < childCount; index++) {
        nsCOMPtr<nsIAccessNode> childAccessNode;
        children->QueryElementAt(index, NS_GET_IID(nsIAccessNode),
                                 getter_AddRefs(childAccessNode));
        childAccessNode->GetDOMNode(getter_AddRefs(possibleAnonNode));
        nsCOMPtr<nsIContent> iterContent = do_QueryInterface(possibleAnonNode);
        if (iterContent && iterContent->IsInAnonymousSubtree()) {
          
          
          
          RefreshNodes(possibleAnonNode);
        }
      }
    }
  }

  
  
  nsCOMPtr<nsIDOMNode> nextNode, iterNode;
  aStartNode->GetFirstChild(getter_AddRefs(nextNode));
  while (nextNode) {
    nextNode.swap(iterNode);
    RefreshNodes(iterNode);
    iterNode->GetNextSibling(getter_AddRefs(nextNode));
  }

  if (!accessNode)
    return;

  if (accessNode == this) {
    
    
    
    
    InvalidateChildren();
    return;
  }

  
  void *uniqueID;
  accessNode->GetUniqueID(&uniqueID);
  accessNode->Shutdown();

  
  mAccessNodeCache.Remove(uniqueID);
}


void
nsDocAccessible::InvalidateCacheSubtree(nsIContent *aChild,
                                        PRUint32 aChangeType)
{
  PRBool isHiding =
    aChangeType == nsIAccessibilityService::FRAME_HIDE ||
    aChangeType == nsIAccessibilityService::NODE_REMOVE;

  PRBool isShowing =
    aChangeType == nsIAccessibilityService::FRAME_SHOW ||
    aChangeType == nsIAccessibilityService::NODE_APPEND;

  PRBool isChanging =
    aChangeType == nsIAccessibilityService::NODE_SIGNIFICANT_CHANGE ||
    aChangeType == nsIAccessibilityService::FRAME_SIGNIFICANT_CHANGE;

  NS_ASSERTION(isChanging || isHiding || isShowing,
               "Incorrect aChangeEventType passed in");

  PRBool isAsynch =
    aChangeType == nsIAccessibilityService::FRAME_HIDE ||
    aChangeType == nsIAccessibilityService::FRAME_SHOW ||
    aChangeType == nsIAccessibilityService::FRAME_SIGNIFICANT_CHANGE;

  
  
  
  

  NS_ENSURE_TRUE(mDOMNode,);

  nsCOMPtr<nsIDOMNode> childNode = aChild ? do_QueryInterface(aChild) : mDOMNode;

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  NS_ENSURE_TRUE(presShell,);
  
  if (!mIsContentLoaded) {
    
    if (mAccessNodeCache.Count() <= 1) {
      
      
      
      
      
      
      
      InvalidateChildren();
      return;
    }

    nsIEventStateManager *esm = presShell->GetPresContext()->EventStateManager();
    NS_ENSURE_TRUE(esm,);

    if (!esm->IsHandlingUserInputExternal()) {
      
      
      
      nsCOMPtr<nsIAccessible> containerAccessible;
      GetAccessibleInParentChain(childNode, PR_FALSE, getter_AddRefs(containerAccessible));
      if (!containerAccessible) {
        containerAccessible = this;
      }

      nsRefPtr<nsAccessible> containerAcc =
        nsAccUtils::QueryAccessible(containerAccessible);
      containerAcc->InvalidateChildren();
      return;
    }     
    
    
    
  }

  
  nsCOMPtr<nsIAccessNode> childAccessNode = GetCachedAccessNode(childNode);
  nsCOMPtr<nsIAccessible> childAccessible = do_QueryInterface(childAccessNode);

#ifdef DEBUG_A11Y
  nsAutoString localName;
  childNode->GetLocalName(localName);
  const char *hasAccessible = childAccessible ? " (acc)" : "";
  if (aChangeType == nsIAccessibilityService::FRAME_HIDE)
    printf("[Hide %s %s]\n", NS_ConvertUTF16toUTF8(localName).get(), hasAccessible);
  else if (aChangeType == nsIAccessibilityService::FRAME_SHOW)
    printf("[Show %s %s]\n", NS_ConvertUTF16toUTF8(localName).get(), hasAccessible);
  else if (aChangeType == nsIAccessibilityService::FRAME_SIGNIFICANT_CHANGE)
    printf("[Layout change %s %s]\n", NS_ConvertUTF16toUTF8(localName).get(), hasAccessible);
  else if (aChangeType == nsIAccessibilityService::NODE_APPEND)
    printf("[Create %s %s]\n", NS_ConvertUTF16toUTF8(localName).get(), hasAccessible);
  else if (aChangeType == nsIAccessibilityService::NODE_REMOVE)
    printf("[Destroy  %s %s]\n", NS_ConvertUTF16toUTF8(localName).get(), hasAccessible);
  else if (aChangeType == nsIAccessibilityService::NODE_SIGNIFICANT_CHANGE)
    printf("[Type change %s %s]\n", NS_ConvertUTF16toUTF8(localName).get(), hasAccessible);
#endif

  nsCOMPtr<nsIAccessible> containerAccessible;
  GetAccessibleInParentChain(childNode, PR_TRUE, getter_AddRefs(containerAccessible));
  if (!containerAccessible) {
    containerAccessible = this;
  }

  if (!isShowing) {
    
    if (isHiding) {
      nsCOMPtr<nsIContent> content(do_QueryInterface(childNode));
      if (content) {
        nsIFrame *frame = content->GetPrimaryFrame();
        if (frame) {
          nsIFrame *frameParent = frame->GetParent();
          if (!frameParent || !frameParent->GetStyleVisibility()->IsVisible()) {
            
            
            
            
            
            return;
          }
        }
      }
    }

    
    
    
    nsresult rv = FireShowHideEvents(childNode, PR_FALSE,
                                     nsIAccessibleEvent::EVENT_HIDE,
                                     eDelayedEvent, isAsynch);
    if (NS_FAILED(rv))
      return;

    if (childNode != mDOMNode) { 
      
      
      
      
      
      nsRefPtr<nsAccEvent> textChangeEvent =
        CreateTextChangeEventForNode(containerAccessible, childNode, childAccessible,
                                     PR_FALSE, isAsynch);
      if (textChangeEvent) {
        nsEventShell::FireEvent(textChangeEvent);
      }
    }
  }

  
  
  
  
  
  

  if (aChild && !isHiding) {
    if (!isAsynch) {
      
      
      nsRefPtr<nsAccessible> containerAcc =
        nsAccUtils::QueryAccessible(containerAccessible);
      if (containerAcc)
        containerAcc->InvalidateChildren();

    }

    

    
    
    
    

    FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_SHOW, childNode,
                               nsAccEvent::eCoalesceFromSameSubtree,
                               isAsynch);

    
    
    nsRoleMapEntry *roleMapEntry = nsAccUtils::GetRoleMapEntry(childNode);
    if (roleMapEntry && roleMapEntry->role == nsIAccessibleRole::ROLE_MENUPOPUP) {
      FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_MENUPOPUP_START,
                                 childNode, nsAccEvent::eRemoveDupes,
                                 isAsynch);
    }

    
    nsIContent *ancestor = aChild;
    while (PR_TRUE) {
      if (roleMapEntry && roleMapEntry->role == nsIAccessibleRole::ROLE_ALERT) {
        nsCOMPtr<nsIDOMNode> alertNode(do_QueryInterface(ancestor));
        FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_ALERT, alertNode,
                                   nsAccEvent::eRemoveDupes, isAsynch);
        break;
      }
      ancestor = ancestor->GetParent();
      nsCOMPtr<nsIDOMNode> ancestorNode = do_QueryInterface(ancestor);
      if (!ancestorNode) {
        break;
      }
      roleMapEntry = nsAccUtils::GetRoleMapEntry(ancestorNode);
    }
  }

  FireValueChangeForTextFields(containerAccessible);

  
  

  
  
  
  
  
  
  
  

  PRBool isUnconditionalEvent = childAccessible ||
    aChild && nsAccUtils::HasAccessibleChildren(childNode);

  nsRefPtr<nsAccEvent> reorderEvent =
    new nsAccReorderEvent(containerAccessible, isAsynch,
                          isUnconditionalEvent,
                          aChild ? childNode.get() : nsnull);
  NS_ENSURE_TRUE(reorderEvent,);

  FireDelayedAccessibleEvent(reorderEvent);
}


NS_IMETHODIMP
nsDocAccessible::GetAccessibleInParentChain(nsIDOMNode *aNode,
                                            PRBool aCanCreate,
                                            nsIAccessible **aAccessible)
{
  
  *aAccessible = nsnull;
  nsCOMPtr<nsIDOMNode> currentNode(aNode), parentNode;
  nsCOMPtr<nsIAccessNode> accessNode;

  do {
    currentNode->GetParentNode(getter_AddRefs(parentNode));
    currentNode = parentNode;
    if (!currentNode) {
      NS_ADDREF_THIS();
      *aAccessible = this;
      break;
    }

    nsCOMPtr<nsIDOMNode> relevantNode;
    if (NS_SUCCEEDED(GetAccService()->GetRelevantContentNodeFor(currentNode, getter_AddRefs(relevantNode))) && relevantNode) {
      currentNode = relevantNode;
    }
    if (aCanCreate) {
      nsRefPtr<nsAccessNode> acc =
        GetAccService()->GetAccessibleInWeakShell(currentNode, mWeakShell);
      if (acc)
        CallQueryInterface(acc, aAccessible);
    }
    else { 
      nsAccessNode* accessNode = GetCachedAccessNode(currentNode);
      if (accessNode)
        CallQueryInterface(accessNode, aAccessible);
    }
  } while (!*aAccessible);

  return NS_OK;
}

nsresult
nsDocAccessible::FireShowHideEvents(nsIDOMNode *aDOMNode,
                                    PRBool aAvoidOnThisNode,
                                    PRUint32 aEventType,
                                    EEventFiringType aDelayedOrNormal,
                                    PRBool aIsAsyncChange,
                                    EIsFromUserInput aIsFromUserInput)
{
  NS_ENSURE_ARG(aDOMNode);

  nsCOMPtr<nsIAccessible> accessible;
  if (!aAvoidOnThisNode) {
    if (aEventType == nsIAccessibleEvent::EVENT_HIDE) {
      
      accessible = do_QueryInterface(GetCachedAccessNode(aDOMNode));
    } else {
      
      GetAccService()->GetAttachedAccessibleFor(aDOMNode,
                                                getter_AddRefs(accessible));
    }
  }

  if (accessible) {
    
    
    nsRefPtr<nsAccEvent> event =
      new nsAccEvent(aEventType, accessible, aIsAsyncChange, aIsFromUserInput,
                     nsAccEvent::eCoalesceFromSameSubtree);
    NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

    if (aDelayedOrNormal == eDelayedEvent)
      return FireDelayedAccessibleEvent(event);

    nsEventShell::FireEvent(event);
    return NS_OK;
  }

  
  
  nsCOMPtr<nsINode> node(do_QueryInterface(aDOMNode));
  PRUint32 count = node->GetChildCount();
  for (PRUint32 index = 0; index < count; index++) {
    nsCOMPtr<nsIDOMNode> childNode = do_QueryInterface(node->GetChildAt(index));
    nsresult rv = FireShowHideEvents(childNode, PR_FALSE, aEventType,
                                     aDelayedOrNormal, aIsAsyncChange,
                                     aIsFromUserInput);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}
