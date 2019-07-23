





































#include "nsRootAccessible.h"
#include "nsAccessibilityAtoms.h"
#include "nsAccessibleEventData.h"
#include "nsIAccessibilityService.h"
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
#include "nsIScrollableView.h"
#include "nsIViewManager.h"
#include "nsIView.h"
#include "nsUnicharUtils.h"
#include "nsIURI.h"
#include "nsIWebNavigation.h"
#include "nsIFocusController.h"
#ifdef MOZ_XUL
#include "nsIXULDocument.h"
#endif








nsDocAccessible::nsDocAccessible(nsIDOMNode *aDOMNode, nsIWeakReference* aShell):
  nsHyperTextAccessible(aDOMNode, aShell), mWnd(nsnull),
  mScrollPositionChangedTicks(0), mIsContentLoaded(PR_FALSE)
{
  
  if (!mDOMNode)
    return;

  
  
  
  

  nsCOMPtr<nsIPresShell> shell(do_QueryReferent(mWeakShell));
  if (shell) {
    mDocument = shell->GetDocument();
    nsIViewManager* vm = shell->GetViewManager();
    if (vm) {
      nsCOMPtr<nsIWidget> widget;
      vm->GetWidget(getter_AddRefs(widget));
      if (widget) {
        mWnd = widget->GetNativeData(NS_NATIVE_WINDOW);
      }
    }
  }

  
  mAccessNodeCache.Init(kDefaultCacheSize);

  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =                              
    GetDocShellTreeItemFor(mDOMNode);
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

NS_INTERFACE_MAP_BEGIN(nsDocAccessible)
  NS_INTERFACE_MAP_ENTRY(nsIAccessibleDocument)
  NS_INTERFACE_MAP_ENTRY(nsPIAccessibleDocument)
  NS_INTERFACE_MAP_ENTRY(nsIDocumentObserver)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY(nsIScrollPositionListener)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIAccessibleDocument)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
NS_INTERFACE_MAP_END_INHERITING(nsHyperTextAccessible)

NS_IMPL_ADDREF_INHERITED(nsDocAccessible, nsHyperTextAccessible)
NS_IMPL_RELEASE_INHERITED(nsDocAccessible, nsHyperTextAccessible)

NS_IMETHODIMP nsDocAccessible::GetName(nsAString& aName)
{
  nsresult rv = NS_OK;
  aName.Truncate();
  if (mRoleMapEntry) {
    rv = nsAccessible::GetName(aName);
  }
  if (aName.IsEmpty()) {
    rv = GetTitle(aName);
  }
  if (aName.IsEmpty() && mParent) {
    rv = mParent->GetName(aName);
  }

  return rv;
}

NS_IMETHODIMP nsDocAccessible::GetRole(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_PANE; 

  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
    GetDocShellTreeItemFor(mDOMNode);
  if (docShellTreeItem) {
    nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
    docShellTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));
    if (sameTypeRoot == docShellTreeItem) {
      
      PRInt32 itemType;
      docShellTreeItem->GetItemType(&itemType);
      if (itemType == nsIDocShellTreeItem::typeChrome) {
        *aRole = nsIAccessibleRole::ROLE_CHROME_WINDOW;
      }
      else if (itemType == nsIDocShellTreeItem::typeContent) {
#ifdef MOZ_XUL
        nsCOMPtr<nsIXULDocument> xulDoc(do_QueryInterface(mDocument));
        *aRole = xulDoc ? nsIAccessibleRole::ROLE_APPLICATION :
                          nsIAccessibleRole::ROLE_DOCUMENT;
#else
        *aRole = nsIAccessibleRole::ROLE_DOCUMENT;
#endif
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsDocAccessible::GetValue(nsAString& aValue)
{
  return GetURL(aValue);
}

NS_IMETHODIMP
nsDocAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  
  nsAccessible::GetState(aState, aExtraState);

  nsCOMPtr<nsIXULDocument> xulDoc(do_QueryInterface(mDocument));
  if (!xulDoc) {
    
    
    
    *aState |= nsIAccessibleStates::STATE_FOCUSABLE;
  }

  if (!mIsContentLoaded) {
    *aState |= nsIAccessibleStates::STATE_BUSY;
  }
 
  nsIFrame* frame = GetFrame();
  while (frame != nsnull && !frame->HasView()) {
    frame = frame->GetParent();
  }
 
  if (frame != nsnull) {
    if (!CheckVisibilityInParentChain(mDocument, frame->GetViewExternal())) {
      *aState |= nsIAccessibleStates::STATE_INVISIBLE;
    }
  }

  nsCOMPtr<nsIEditor> editor = GetEditor();
  if (!editor) {
    *aState |= nsIAccessibleStates::STATE_READONLY;
  }
  else if (aExtraState) {
    *aExtraState |= nsIAccessibleStates::EXT_STATE_EDITABLE;
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
  nsCOMPtr<nsIDOMWindow> domWin;
  GetWindow(getter_AddRefs(domWin));
  nsCOMPtr<nsPIDOMWindow> privateDOMWindow(do_QueryInterface(domWin));
  NS_ENSURE_TRUE(privateDOMWindow, NS_ERROR_FAILURE);
  nsIFocusController *focusController =
    privateDOMWindow->GetRootFocusController();
  if (focusController) {
    nsCOMPtr<nsIDOMElement> ele(do_QueryInterface(mDOMNode));
    focusController->SetFocusedElement(ele);
    return NS_OK;
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
  if (mDocument) {
    aTitle = mDocument->GetDocumentTitle();
    return NS_OK;
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

NS_IMETHODIMP nsDocAccessible::GetCaretAccessible(nsIAccessible **aCaretAccessible)
{
  
  *aCaretAccessible = nsnull;
  return NS_OK;
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

void nsDocAccessible::SetEditor(nsIEditor* aEditor)
{
  mEditor = aEditor;
  if (mEditor)
    mEditor->AddEditActionListener(this);
}

void nsDocAccessible::CheckForEditor()
{
  if (mEditor) {
    return;  
  }
  if (!mDocument) {
    return;  
  }

  nsCOMPtr<nsISupports> container = mDocument->GetContainer();
  nsCOMPtr<nsIEditingSession> editingSession(do_GetInterface(container));
  if (!editingSession)
    return; 

  nsCOMPtr<nsIEditor> editor;
  editingSession->GetEditorForWindow(mDocument->GetWindow(),
                                     getter_AddRefs(editor));
  SetEditor(editor);
  if (!editor)
    return;

  
  nsCOMPtr<nsIAccessibleStateChangeEvent> event =
    new nsAccStateChangeEvent(this, nsIAccessibleStates::EXT_STATE_EDITABLE,
                              PR_TRUE, PR_TRUE);
  FireAccessibleEvent(event);
}

NS_IMETHODIMP nsDocAccessible::GetCachedAccessNode(void *aUniqueID, nsIAccessNode **aAccessNode)
{
  GetCacheEntry(mAccessNodeCache, aUniqueID, aAccessNode); 
#ifdef DEBUG_A11Y
  
  
  
  
  nsCOMPtr<nsIAccessible> accessible = do_QueryInterface(*aAccessNode);
  nsCOMPtr<nsPIAccessible> privateAccessible = do_QueryInterface(accessible);
  if (privateAccessible) {
    nsCOMPtr<nsIAccessible> parent;
    privateAccessible->GetCachedParent(getter_AddRefs(parent));
    nsCOMPtr<nsPIAccessible> privateParent(do_QueryInterface(parent));
    if (privateParent) {
      privateParent->TestChildCache(accessible);
    }
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP nsDocAccessible::CacheAccessNode(void *aUniqueID, nsIAccessNode *aAccessNode)
{
  PutCacheEntry(mAccessNodeCache, aUniqueID, aAccessNode);
  return NS_OK;
}

NS_IMETHODIMP nsDocAccessible::GetParent(nsIAccessible **aParent)
{
  
  *aParent = nsnull;
  NS_ENSURE_TRUE(mDocument, NS_ERROR_FAILURE);
  if (!mParent) {
    nsIDocument *parentDoc = mDocument->GetParentDocument();
    NS_ENSURE_TRUE(parentDoc, NS_ERROR_FAILURE);
    nsIContent *ownerContent = parentDoc->FindContentForSubDocument(mDocument);
    nsCOMPtr<nsIDOMNode> ownerNode(do_QueryInterface(ownerContent));
    if (ownerNode) {
      nsCOMPtr<nsIAccessibilityService> accService =
        do_GetService("@mozilla.org/accessibilityService;1");
      if (accService) {
        
        
        
        
        
        accService->GetAccessibleFor(ownerNode, getter_AddRefs(mParent));
      }
    }
  }
  return mParent ? nsAccessible::GetParent(aParent) : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocAccessible::GetAttributes(nsIPersistentProperties **aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);
  *aAttributes = nsnull;

  return mDOMNode ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsDocAccessible::Init()
{
  PutCacheEntry(gGlobalDocAccessibleCache, mWeakShell, this);

  AddEventListeners();

  nsresult rv = nsHyperTextAccessible::Init();

  if (mRoleMapEntry && mRoleMapEntry->role != nsIAccessibleRole::ROLE_DIALOG &&
      mRoleMapEntry->role != nsIAccessibleRole::ROLE_APPLICATION &&
      mRoleMapEntry->role != nsIAccessibleRole::ROLE_ALERT &&
      mRoleMapEntry->role != nsIAccessibleRole::ROLE_DOCUMENT) {
    
    
    mRoleMapEntry = nsnull; 
  }

  return rv;
}


NS_IMETHODIMP nsDocAccessible::Destroy()
{
  gGlobalDocAccessibleCache.Remove(NS_STATIC_CAST(void*, mWeakShell));
  return Shutdown();
}

NS_IMETHODIMP nsDocAccessible::Shutdown()
{
  if (!mWeakShell) {
    return NS_OK;  
  }

  nsCOMPtr<nsIDocShellTreeItem> treeItem = GetDocShellTreeItemFor(mDOMNode);
  ShutdownChildDocuments(treeItem);

  if (mEditor) {
    mEditor->RemoveEditActionListener(this);
    mEditor = nsnull;
  }

  if (mDocLoadTimer) {
    mDocLoadTimer->Cancel();
    mDocLoadTimer = nsnull;
  }

  RemoveEventListeners();

  mWeakShell = nsnull;  

  if (mFireEventTimer) {
    mFireEventTimer->Cancel();
    mFireEventTimer = nsnull;
  }
  mEventsToFire.Clear();

  ClearCache(mAccessNodeCache);

  mDocument = nsnull;

  return nsHyperTextAccessible::Shutdown();
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
      nsCOMPtr<nsPIAccessNode> accessNode = do_QueryInterface(docAccessible);
      if (accessNode) {
        accessNode->Shutdown();
      }
    }
  }
}

nsIFrame* nsDocAccessible::GetFrame()
{
  nsCOMPtr<nsIPresShell> shell(do_QueryReferent(mWeakShell));

  nsIFrame* root = nsnull;
  if (shell)
    root = shell->GetRootFrame();

  return root;
}

void nsDocAccessible::GetBoundsRect(nsRect& aBounds, nsIFrame** aRelativeFrame)
{
  *aRelativeFrame = GetFrame();

  nsIDocument *document = mDocument;
  nsIDocument *parentDoc = nsnull;

  while (document) {
    nsIPresShell *presShell = document->GetShellAt(0);
    if (!presShell) {
      return;
    }
    nsIViewManager* vm = presShell->GetViewManager();
    if (!vm) {
      return;
    }

    nsIScrollableView* scrollableView = nsnull;
    vm->GetRootScrollableView(&scrollableView);

    nsRect viewBounds(0, 0, 0, 0);
    if (scrollableView) {
      viewBounds = scrollableView->View()->GetBounds();
    }
    else {
      nsIView *view;
      vm->GetRootView(view);
      if (view) {
        viewBounds = view->GetBounds();
      }
    }

    if (parentDoc) {  
      aBounds.IntersectRect(viewBounds, aBounds);
    }
    else {  
      aBounds = viewBounds;
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
    CheckForEditor();

    if (!mEditor) {
      
      nsCOMPtr<nsICommandManager> commandManager = do_GetInterface(docShellTreeItem);
      if (commandManager) {
        commandManager->AddCommandObserver(this, "obs_documentCreated");
      }
    }
  }

  
  mDocument->AddObserver(this);
  return NS_OK;
}

nsresult nsDocAccessible::RemoveEventListeners()
{
  
  
  RemoveScrollListener();

  
  mDocument->RemoveObserver(this);

  if (mScrollWatchTimer) {
    mScrollWatchTimer->Cancel();
    mScrollWatchTimer = nsnull;
  }

  nsCOMPtr<nsISupports> container = mDocument->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem(do_QueryInterface(container));
  NS_ENSURE_TRUE(docShellTreeItem, NS_ERROR_FAILURE);

  PRInt32 itemType;
  docShellTreeItem->GetItemType(&itemType);
  if (itemType == nsIDocShellTreeItem::typeContent) {
    nsCOMPtr<nsICommandManager> commandManager = do_GetInterface(docShellTreeItem);
    if (commandManager) {
      commandManager->RemoveCommandObserver(this, "obs_documentCreated");
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsDocAccessible::FireAnchorJumpEvent()
{
  if (!mIsContentLoaded || !mDocument) {
    return NS_OK;
  }
  nsCOMPtr<nsISupports> container = mDocument->GetContainer();
  nsCOMPtr<nsIWebNavigation> webNav(do_GetInterface(container));
  nsCAutoString theURL;
  if (webNav) {
    nsCOMPtr<nsIURI> pURI;
    webNav->GetCurrentURI(getter_AddRefs(pURI));
    if (pURI) {
      pURI->GetSpec(theURL);
    }
  }
  static nsCAutoString lastAnchor;
  const char kHash = '#';
  nsCAutoString currentAnchor;
  PRInt32 hasPosition = theURL.FindChar(kHash);
  if (hasPosition > 0 && hasPosition < (PRInt32)theURL.Length() - 1) {
    mIsAnchor = PR_TRUE;
    currentAnchor.Assign(Substring(theURL,
                                   hasPosition+1, 
                                   (PRInt32)theURL.Length()-hasPosition-1));
  }

  if (currentAnchor.Equals(lastAnchor)) {
    mIsAnchorJumped = PR_FALSE;
  } else {
    mIsAnchorJumped = PR_TRUE;
    lastAnchor.Assign(currentAnchor);
  }

  if (mIsAnchorJumped) {
    FireToolkitEvent(nsIAccessibleEvent::EVENT_DOCUMENT_ATTRIBUTES_CHANGED,
                     this, nsnull);
  }

  return NS_OK;
}

NS_IMETHODIMP nsDocAccessible::FireDocLoadEvents(PRUint32 aEventType)
{
  if (!mDocument || !mWeakShell) {
    return NS_OK;  
  }

  PRBool isFinished = 
             (aEventType == nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_COMPLETE ||
              aEventType == nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_STOPPED);

  if (mIsContentLoaded == isFinished) {
    return NS_OK;
  }
  mIsContentLoaded = isFinished;

  if (isFinished) {
    
    AddScrollListener();
    nsCOMPtr<nsIAccessible> parent(nsAccessible::GetParent());
    nsCOMPtr<nsPIAccessible> privateAccessible(do_QueryInterface(parent));
    if (privateAccessible) {
      
      privateAccessible->InvalidateChildren();
    }
    
    
    if (!mDocLoadTimer) {
      mDocLoadTimer = do_CreateInstance("@mozilla.org/timer;1");
    }
    if (mDocLoadTimer) {
      mDocLoadTimer->InitWithFuncCallback(DocLoadCallback, this, 0,
                                          nsITimer::TYPE_ONE_SHOT);
    }
    
    nsCOMPtr<nsIAccessibleStateChangeEvent> accEvent =
      new nsAccStateChangeEvent(this, nsIAccessibleStates::STATE_BUSY,
                                PR_FALSE, PR_FALSE);
    FireAccessibleEvent(accEvent);
  } else {
    nsCOMPtr<nsIDocShellTreeItem> treeItem = GetDocShellTreeItemFor(mDOMNode);
    if (!treeItem) {
      return NS_OK;
    }

    nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
    treeItem->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));
    if (sameTypeRoot != treeItem) {
      return NS_OK; 
    }

    
    nsCOMPtr<nsIAccessibleStateChangeEvent> accEvent =
      new nsAccStateChangeEvent(this, nsIAccessibleStates::STATE_BUSY,
                                PR_FALSE, PR_TRUE);
    FireAccessibleEvent(accEvent);
  }

  FireToolkitEvent(aEventType, this, nsnull);
  return NS_OK;
}

void nsDocAccessible::ScrollTimerCallback(nsITimer *aTimer, void *aClosure)
{
  nsDocAccessible *docAcc = NS_REINTERPRET_CAST(nsDocAccessible*, aClosure);

  if (docAcc && docAcc->mScrollPositionChangedTicks &&
      ++docAcc->mScrollPositionChangedTicks > 2) {
    
    
    
    
    docAcc->FireToolkitEvent(nsIAccessibleEvent::EVENT_SCROLLING_END, docAcc,
                             nsnull);
    docAcc->mScrollPositionChangedTicks = 0;
    if (docAcc->mScrollWatchTimer) {
      docAcc->mScrollWatchTimer->Cancel();
      docAcc->mScrollWatchTimer = nsnull;
    }
  }
}

void nsDocAccessible::AddScrollListener()
{
  nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));

  nsIViewManager* vm = nsnull;
  if (presShell)
    vm = presShell->GetViewManager();

  nsIScrollableView* scrollableView = nsnull;
  if (vm)
    vm->GetRootScrollableView(&scrollableView);

  if (scrollableView)
    scrollableView->AddScrollPositionListener(this);
}

void nsDocAccessible::RemoveScrollListener()
{
  nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));

  nsIViewManager* vm = nsnull;
  if (presShell)
    vm = presShell->GetViewManager();

  nsIScrollableView* scrollableView = nsnull;
  if (vm)
    vm->GetRootScrollableView(&scrollableView);

  if (scrollableView)
    scrollableView->RemoveScrollPositionListener(this);
}

NS_IMETHODIMP nsDocAccessible::ScrollPositionWillChange(nsIScrollableView *aView, nscoord aX, nscoord aY)
{
  return NS_OK;
}

NS_IMETHODIMP nsDocAccessible::ScrollPositionDidChange(nsIScrollableView *aScrollableView, nscoord aX, nscoord aY)
{
  
  
  const PRUint32 kScrollPosCheckWait = 50;
  if (mScrollWatchTimer) {
    mScrollWatchTimer->SetDelay(kScrollPosCheckWait);  
  }
  else {
    mScrollWatchTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (mScrollWatchTimer) {
      mScrollWatchTimer->InitWithFuncCallback(ScrollTimerCallback, this,
                                              kScrollPosCheckWait,
                                              nsITimer::TYPE_REPEATING_SLACK);
    }
  }
  mScrollPositionChangedTicks = 1;
  return NS_OK;
}

NS_IMETHODIMP nsDocAccessible::Observe(nsISupports *aSubject, const char *aTopic,
                                       const PRUnichar *aData)
{
  if (!nsCRT::strcmp(aTopic,"obs_documentCreated")) {
    CheckForEditor();
    NS_ASSERTION(mEditor, "Should have editor if we see obs_documentCreated");
  }

  return NS_OK;
}




NS_IMPL_NSIDOCUMENTOBSERVER_CORE_STUB(nsDocAccessible)
NS_IMPL_NSIDOCUMENTOBSERVER_LOAD_STUB(nsDocAccessible)
NS_IMPL_NSIDOCUMENTOBSERVER_STYLE_STUB(nsDocAccessible)

void
nsDocAccessible::AttributeChanged(nsIDocument *aDocument, nsIContent* aContent,
                                  PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                                  PRInt32 aModType)
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

  if (aNameSpaceID == kNameSpaceID_WAIProperties) {
    ARIAAttributeChanged(aContent, aAttribute);
    return;
  }

  nsCOMPtr<nsIDOMNode> targetNode(do_QueryInterface(aContent));
  NS_ASSERTION(targetNode, "No node for attr modified");
  if (!targetNode) {
    return;
  }

  if (aNameSpaceID == kNameSpaceID_XHTML2_Unofficial ||
      aNameSpaceID == kNameSpaceID_XHTML) {
    if (aAttribute == nsAccessibilityAtoms::role)
      InvalidateCacheSubtree(aContent, nsIAccessibleEvent::EVENT_REORDER);
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::href ||
      aAttribute == nsAccessibilityAtoms::onclick ||
      aAttribute == nsAccessibilityAtoms::droppable) {
    InvalidateCacheSubtree(aContent, nsIAccessibleEvent::EVENT_REORDER);
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::selected) {
    
    nsCOMPtr<nsIAccessible> multiSelect = GetMultiSelectFor(targetNode);
    
    
    
    
    
    if (multiSelect) {
      
      
      nsCOMPtr<nsIAccessNode> multiSelectAccessNode =
        do_QueryInterface(multiSelect);
      nsCOMPtr<nsIDOMNode> multiSelectDOMNode;
      multiSelectAccessNode->GetDOMNode(getter_AddRefs(multiSelectDOMNode));
      NS_ASSERTION(multiSelectDOMNode, "A new accessible without a DOM node!");
      FireDelayedToolkitEvent(nsIAccessibleEvent::EVENT_SELECTION_WITHIN,
                              multiSelectDOMNode, nsnull, PR_TRUE);

      static nsIContent::AttrValuesArray strings[] =
        {&nsAccessibilityAtoms::_empty, &nsAccessibilityAtoms::_false, nsnull};
      if (aContent->FindAttrValueIn(kNameSpaceID_None,
                                    nsAccessibilityAtoms::selected,
                                    strings, eCaseMatters) !=
          nsIContent::ATTR_VALUE_NO_MATCH) {

        FireDelayedToolkitEvent(nsIAccessibleEvent::EVENT_SELECTION_REMOVE,
                                targetNode, nsnull);
        return;
      }

      FireDelayedToolkitEvent(nsIAccessibleEvent::EVENT_SELECTION_ADD,
                                                  targetNode, nsnull);
    }
  }
}

void
nsDocAccessible::ARIAAttributeChanged(nsIContent* aContent, nsIAtom* aAttribute)
{
  nsCOMPtr<nsIDOMNode> targetNode(do_QueryInterface(aContent));
  if (!targetNode)
    return;

  
  if (aAttribute == nsAccessibilityAtoms::disabled) {
    nsCOMPtr<nsIAccessibleStateChangeEvent> event =
      new nsAccStateChangeEvent(targetNode,
                                nsIAccessibleStates::EXT_STATE_ENABLED,
                                PR_TRUE);
    FireDelayedAccessibleEvent(event);
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::required) {
    nsCOMPtr<nsIAccessibleStateChangeEvent> event =
      new nsAccStateChangeEvent(targetNode,
                                nsIAccessibleStates::STATE_REQUIRED,
                                PR_FALSE);
    FireDelayedAccessibleEvent(event);
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::invalid) {
    nsCOMPtr<nsIAccessibleStateChangeEvent> event =
      new nsAccStateChangeEvent(targetNode,
                                nsIAccessibleStates::STATE_INVALID,
                                PR_FALSE);
    FireDelayedAccessibleEvent(event);
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::activedescendant) {
    
    
    nsCOMPtr<nsIDOMNode> currentFocus = GetCurrentFocus();
    if (SameCOMIdentity(currentFocus, aContent)) {
      nsRefPtr<nsRootAccessible> rootAcc = GetRootAccessible();
      if (rootAcc)
        rootAcc->FireAccessibleFocusEvent(nsnull, currentFocus, nsnull, PR_TRUE);
    }
    return;
  }

  if (!HasRoleAttribute(aContent)) {
    
    
    
    
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::checked) {
    nsCOMPtr<nsIAccessibleStateChangeEvent> event =
      new nsAccStateChangeEvent(targetNode,
                                nsIAccessibleStates::STATE_CHECKED,
                                PR_FALSE);
    FireDelayedAccessibleEvent(event);
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::expanded) {
    nsCOMPtr<nsIAccessibleStateChangeEvent> event =
      new nsAccStateChangeEvent(targetNode,
                                nsIAccessibleStates::STATE_EXPANDED,
                                PR_FALSE);
    FireDelayedAccessibleEvent(event);
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::readonly) {
    nsCOMPtr<nsIAccessibleStateChangeEvent> event =
      new nsAccStateChangeEvent(targetNode,
                                nsIAccessibleStates::STATE_READONLY,
                                PR_FALSE);
    FireDelayedAccessibleEvent(event);
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::valuenow) {
    FireDelayedToolkitEvent(nsIAccessibleEvent::EVENT_VALUE_CHANGE,
                            targetNode, nsnull);
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::multiselectable) {
    
    
    
    if (HasRoleAttribute(aContent)) {
      
      
      InvalidateCacheSubtree(aContent, nsIAccessibleEvent::EVENT_REORDER);
    }
  }
}

void nsDocAccessible::ContentAppended(nsIDocument *aDocument,
                                      nsIContent* aContainer,
                                      PRInt32 aNewIndexInContainer)
{
  
  
  
  
  PRUint32 childCount = aContainer->GetChildCount();
  for (PRUint32 index = aNewIndexInContainer; index < childCount; index ++) {
    InvalidateCacheSubtree(aContainer->GetChildAt(index),
                           nsIAccessibleEvent::EVENT_SHOW);
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

void nsDocAccessible::CharacterDataChanged(nsIDocument *aDocument,
                                           nsIContent* aContent,
                                           CharacterDataChangeInfo* aInfo)
{
  InvalidateCacheSubtree(aContent, nsIAccessibleEvent::EVENT_REORDER);
}

void
nsDocAccessible::ContentInserted(nsIDocument *aDocument, nsIContent* aContainer,
                                 nsIContent* aChild, PRInt32 aIndexInContainer)
{
  
  
  
  
  InvalidateCacheSubtree(aChild, nsIAccessibleEvent::EVENT_SHOW);
}

void
nsDocAccessible::ContentRemoved(nsIDocument *aDocument, nsIContent* aContainer,
                                nsIContent* aChild, PRInt32 aIndexInContainer)
{
  InvalidateCacheSubtree(aChild, nsIAccessibleEvent::EVENT_HIDE);
}

void
nsDocAccessible::ParentChainChanged(nsIContent *aContent)
{
}

nsresult nsDocAccessible::FireDelayedToolkitEvent(PRUint32 aEvent,
                                                  nsIDOMNode *aDOMNode,
                                                  void *aData,
                                                  PRBool aAllowDupes)
{
  nsCOMPtr<nsIAccessibleEvent> event = new nsAccEvent(aEvent, aDOMNode, aData);
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  return FireDelayedAccessibleEvent(event);
}

nsresult
nsDocAccessible::FireDelayedAccessibleEvent(nsIAccessibleEvent *aEvent,
                                           PRBool aAllowDupes)
{
  PRBool isTimerStarted = PR_TRUE;
  PRInt32 numQueuedEvents = mEventsToFire.Count();
  if (!mFireEventTimer) {
    
    mFireEventTimer = do_CreateInstance("@mozilla.org/timer;1");
    NS_ENSURE_TRUE(mFireEventTimer, NS_ERROR_OUT_OF_MEMORY);
  }

  PRUint32 newEventType;
  aEvent->GetEventType(&newEventType);

  nsCOMPtr<nsIDOMNode> newEventDOMNode;
  aEvent->GetDOMNode(getter_AddRefs(newEventDOMNode));

  if (numQueuedEvents == 0) {
    isTimerStarted = PR_FALSE;
  } else if (!aAllowDupes) {
    
    
    
    for (PRInt32 index = 0; index < numQueuedEvents; index ++) {
      nsIAccessibleEvent *accessibleEvent = mEventsToFire[index];
      NS_ASSERTION(accessibleEvent, "Array item is not an accessible event");
      if (!accessibleEvent) {
        continue;
      }
      PRUint32 eventType;
      accessibleEvent->GetEventType(&eventType);
      if (eventType == newEventType) {
        nsCOMPtr<nsIDOMNode> domNode;
        accessibleEvent->GetDOMNode(getter_AddRefs(domNode));
        if (domNode == newEventDOMNode) {
          mEventsToFire.RemoveObjectAt(index);
          -- index;
          -- numQueuedEvents;
        }
      }
    }
  }

  mEventsToFire.AppendObject(aEvent);
  if (!isTimerStarted) {
    
    
    mFireEventTimer->InitWithFuncCallback(FlushEventsCallback,
                                          NS_STATIC_CAST(nsPIAccessibleDocument*, this),
                                          0, nsITimer::TYPE_ONE_SHOT);
  }

  return NS_OK;
}

NS_IMETHODIMP nsDocAccessible::FlushPendingEvents()
{
  PRUint32 length = mEventsToFire.Count();
  NS_ASSERTION(length, "How did we get here without events to fire?");
  PRUint32 index;
  for (index = 0; index < length; index ++) {
    nsCOMPtr<nsIAccessibleEvent> accessibleEvent(
      do_QueryInterface(mEventsToFire[index]));
    NS_ASSERTION(accessibleEvent, "Array item is not an accessible event");

    nsCOMPtr<nsIAccessible> accessible;
    accessibleEvent->GetAccessible(getter_AddRefs(accessible));
    if (accessible) {
      PRUint32 eventType;
      accessibleEvent->GetEventType(&eventType);
      if (eventType == nsIAccessibleEvent::EVENT_INTERNAL_LOAD) {
        nsCOMPtr<nsPIAccessibleDocument> docAccessible =
          do_QueryInterface(accessible);
        NS_ASSERTION(docAccessible, "No doc accessible for doc load event");
        if (docAccessible) {
          docAccessible->FireDocLoadEvents(nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_COMPLETE);
        }
      }
      else if (eventType == nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED) {
        nsCOMPtr<nsIAccessibleText> accessibleText = do_QueryInterface(accessible);
        PRInt32 caretOffset;
        if (accessibleText && NS_SUCCEEDED(accessibleText->GetCaretOffset(&caretOffset))) {
          FireToolkitEvent(nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED,
                           accessible, &caretOffset);
          PRInt32 selectionCount;
          accessibleText->GetSelectionCount(&selectionCount);
          if (selectionCount) {  
           FireToolkitEvent(nsIAccessibleEvent::EVENT_TEXT_SELECTION_CHANGED,
                                                accessible, nsnull);
          }
        }
      }
      else {
        FireAccessibleEvent(accessibleEvent);
      }
    }
  }
  mEventsToFire.Clear(); 
  return NS_OK;
}

void nsDocAccessible::FlushEventsCallback(nsITimer *aTimer, void *aClosure)
{
  nsPIAccessibleDocument *accessibleDoc = NS_STATIC_CAST(nsPIAccessibleDocument*, aClosure);
  NS_ASSERTION(accessibleDoc, "How did we get here without an accessible document?");
  accessibleDoc->FlushPendingEvents();
}

void nsDocAccessible::RefreshNodes(nsIDOMNode *aStartNode, PRUint32 aChangeEvent)
{
  nsCOMPtr<nsIDOMNode> iterNode(aStartNode), nextNode;
  nsCOMPtr<nsIAccessNode> accessNode;

  do {
    GetCachedAccessNode(iterNode, getter_AddRefs(accessNode));
    if (accessNode) {
      
      
      

      
      if (accessNode != NS_STATIC_CAST(nsIAccessNode*, this)) {
        if (aChangeEvent != nsIAccessibleEvent::EVENT_SHOW) {
          nsCOMPtr<nsIAccessible> accessible(do_QueryInterface(accessNode));
          if (accessible) {
            
            PRUint32 role, event = 0;
            accessible->GetFinalRole(&role);
            if (role == nsIAccessibleRole::ROLE_MENUPOPUP) {
              nsCOMPtr<nsIDOMNode> domNode;
              accessNode->GetDOMNode(getter_AddRefs(domNode));
              nsCOMPtr<nsIDOMXULPopupElement> popup(do_QueryInterface(domNode));
              if (!popup) {
                
                
                event = nsIAccessibleEvent::EVENT_MENUPOPUP_END;
              }
            }
            else if (role == nsIAccessibleRole::ROLE_PROGRESSBAR &&
                     iterNode != aStartNode) {
              
              event = nsIAccessibleEvent::EVENT_HIDE;
            }
            if (event) {
              FireToolkitEvent(event, accessible, nsnull);
            }
          }
        }
        void *uniqueID;
        accessNode->GetUniqueID(&uniqueID);
        nsCOMPtr<nsPIAccessNode> privateAccessNode(do_QueryInterface(accessNode));
        privateAccessNode->Shutdown();
        
        mAccessNodeCache.Remove(uniqueID);
      }
    }

    iterNode->GetFirstChild(getter_AddRefs(nextNode));
    if (nextNode) {
      iterNode = nextNode;
      continue;
    }

    if (iterNode == aStartNode)
      break;
    iterNode->GetNextSibling(getter_AddRefs(nextNode));
    if (nextNode) {
      iterNode = nextNode;
      continue;
    }

    do {
      iterNode->GetParentNode(getter_AddRefs(nextNode));
      if (!nextNode || nextNode == aStartNode) {
        return;
      }
      nextNode->GetNextSibling(getter_AddRefs(iterNode));
      if (iterNode)
        break;
      iterNode = nextNode;
    } while (PR_TRUE);
  }
  while (iterNode && iterNode != aStartNode);
}

NS_IMETHODIMP nsDocAccessible::InvalidateCacheSubtree(nsIContent *aChild,
                                                      PRUint32 aChangeEventType)
{
  NS_ASSERTION(aChangeEventType == nsIAccessibleEvent::EVENT_REORDER ||
               aChangeEventType == nsIAccessibleEvent::EVENT_SHOW ||
               aChangeEventType == nsIAccessibleEvent::EVENT_HIDE,
               "Incorrect aChangeEventType passed in");

  
  
  
  

  nsCOMPtr<nsIDOMNode> childNode = aChild ? do_QueryInterface(aChild) : mDOMNode;
  if (!mIsContentLoaded && mAccessNodeCache.Count() <= 1) {
    return NS_OK; 
  }

  nsCOMPtr<nsIAccessNode> childAccessNode;
  GetCachedAccessNode(childNode, getter_AddRefs(childAccessNode));
  nsCOMPtr<nsIAccessible> childAccessible = do_QueryInterface(childAccessNode);
  if (!childAccessible && mIsContentLoaded && aChangeEventType != nsIAccessibleEvent::EVENT_HIDE) {
    
    
    GetAccService()->GetAccessibleFor(childNode, getter_AddRefs(childAccessible));
  }
  nsCOMPtr<nsPIAccessible> privateChildAccessible =
    do_QueryInterface(childAccessible);
#ifdef DEBUG_A11Y
  nsAutoString localName;
  childNode->GetLocalName(localName);
  const char *hasAccessible = childAccessible ? " (acc)" : "";
  if (aChangeEventType == nsIAccessibleEvent::EVENT_HIDE) {
    printf("[Hide %s %s]\n", NS_ConvertUTF16toUTF8(localName).get(), hasAccessible);
  }
  else if (aChangeEventType == nsIAccessibleEvent::EVENT_SHOW) {
    printf("[Show %s %s]\n", NS_ConvertUTF16toUTF8(localName).get(), hasAccessible);
  }
  else if (aChangeEventType == nsIAccessibleEvent::EVENT_REORDER) {
    printf("[Reorder %s %s]\n", NS_ConvertUTF16toUTF8(localName).get(), hasAccessible);
  }
#endif

  if (aChangeEventType == nsIAccessibleEvent::EVENT_HIDE) {
    
    
    if (privateChildAccessible) {
      privateChildAccessible->FireToolkitEvent(nsIAccessibleEvent::EVENT_HIDE,
                                               childAccessible, nsnull);
    }
  }

  
  if (aChangeEventType != nsIAccessibleEvent::EVENT_SHOW) {
    RefreshNodes(childNode, aChangeEventType);
  }

  
  
  
  
  
  

  nsCOMPtr<nsIAccessible> containerAccessible;
  if (childNode == mDOMNode) {
    
    
    nsCOMPtr<nsISupports> container = mDocument->GetContainer();
    nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem(do_QueryInterface(container));
    NS_ENSURE_TRUE(docShellTreeItem, NS_ERROR_FAILURE);
    nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
    docShellTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));
    if (sameTypeRoot == docShellTreeItem) {
      containerAccessible = this;  
    }
  }
  if (!containerAccessible && privateChildAccessible) {
    GetAccessibleInParentChain(childNode, getter_AddRefs(containerAccessible));
  }
  nsCOMPtr<nsPIAccessible> privateContainerAccessible =
    do_QueryInterface(containerAccessible);
  if (privateContainerAccessible) {
    privateContainerAccessible->InvalidateChildren();
  }

  if (!mIsContentLoaded) {
    return NS_OK;
  }

  
  
  
  
  nsCOMPtr<nsIAccessNode> containerAccessNode =
    do_QueryInterface(containerAccessible);
  if (containerAccessNode) {
    nsCOMPtr<nsIDOMNode> containerNode;
    containerAccessNode->GetDOMNode(getter_AddRefs(containerNode));
    if (containerNode) {
      FireDelayedToolkitEvent(nsIAccessibleEvent::EVENT_REORDER,
                              containerNode, nsnull);
    }
  }

  if (aChangeEventType == nsIAccessibleEvent::EVENT_SHOW && aChild) {
    
    
    
    
    
    FireDelayedToolkitEvent(nsIAccessibleEvent::EVENT_SHOW, childNode, nsnull);
    nsAutoString role;
    if (GetRoleAttribute(aChild, role) &&
        StringEndsWith(role, NS_LITERAL_STRING(":menu"), nsCaseInsensitiveStringComparator())) {
      FireDelayedToolkitEvent(nsIAccessibleEvent::EVENT_MENUPOPUP_START,
                              childNode, nsnull);
    }
  }

  
  if (aChangeEventType != nsIAccessibleEvent::EVENT_HIDE) {
    nsIContent *ancestor = aChild;
    nsAutoString role;
    while (ancestor) {
      if (GetRoleAttribute(ancestor, role) &&
          StringEndsWith(role, NS_LITERAL_STRING(":alert"), nsCaseInsensitiveStringComparator())) {
        nsCOMPtr<nsIDOMNode> alertNode(do_QueryInterface(ancestor));
        FireDelayedToolkitEvent(nsIAccessibleEvent::EVENT_ALERT, alertNode, nsnull);
        break;
      }
      ancestor = ancestor->GetParent();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocAccessible::GetAccessibleInParentChain(nsIDOMNode *aNode,
                                            nsIAccessible **aAccessible)
{
  
  *aAccessible = nsnull;
  nsCOMPtr<nsIDOMNode> currentNode(aNode), parentNode;
  nsCOMPtr<nsIAccessNode> accessNode;

  nsIAccessibilityService *accService = GetAccService();
  NS_ENSURE_TRUE(accService, NS_ERROR_FAILURE);

  do {
    currentNode->GetParentNode(getter_AddRefs(parentNode));
    currentNode = parentNode;
    if (!currentNode) {
      NS_ADDREF_THIS();
      *aAccessible = this;
      break;
    }

    nsCOMPtr<nsIDOMNode> relevantNode;
    if (NS_SUCCEEDED(accService->GetRelevantContentNodeFor(currentNode, getter_AddRefs(relevantNode))) && relevantNode) {
      currentNode = relevantNode;
    }

    accService->GetAccessibleInWeakShell(currentNode, mWeakShell, aAccessible);
  } while (!*aAccessible);

  return NS_OK;
}

NS_IMETHODIMP
nsDocAccessible::FireToolkitEvent(PRUint32 aEvent, nsIAccessible *aTarget,
                                  void * aData)
{
  
  if (!mWeakShell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIAccessibleEvent> accEvent =
    new nsAccEvent(aEvent, aTarget, aData);
  NS_ENSURE_TRUE(accEvent, NS_ERROR_OUT_OF_MEMORY);

  return FireAccessibleEvent(accEvent);
}

void nsDocAccessible::DocLoadCallback(nsITimer *aTimer, void *aClosure)
{
  
  
  
  
  

  nsDocAccessible *docAcc =
    NS_REINTERPRET_CAST(nsDocAccessible*, aClosure);
  if (!docAcc) {
    return;
  }

  
  nsCOMPtr<nsIDOMNode> docDomNode;
  docAcc->GetDOMNode(getter_AddRefs(docDomNode));
  nsCOMPtr<nsIDocument> doc(do_QueryInterface(docDomNode));
  if (doc) {
    nsCOMPtr<nsISupports> container = doc->GetContainer();
    nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem = do_QueryInterface(container);
    if (!docShellTreeItem) {
      return;
    }
    nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
    docShellTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));
    if (sameTypeRoot != docShellTreeItem) {
      
      docAcc->InvalidateCacheSubtree(nsnull, nsIAccessibleEvent::EVENT_REORDER);
      return;
    }

    
    if (gLastFocusedNode) {
      nsCOMPtr<nsIDocShellTreeItem> focusedTreeItem =
        GetDocShellTreeItemFor(gLastFocusedNode);
      if (focusedTreeItem) {
        nsCOMPtr<nsIDocShellTreeItem> sameTypeRootOfFocus;
        focusedTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRootOfFocus));
        if (sameTypeRoot == sameTypeRootOfFocus) {
          docAcc->FireToolkitEvent(nsIAccessibleEvent::EVENT_STATE_CHANGE,
                                  docAcc, nsnull);
          docAcc->FireAnchorJumpEvent();
        }
      }
    }
  }
}

