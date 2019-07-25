





































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
#ifdef MOZ_XUL
#include "nsIXULDocument.h"
#endif




PRUint32 nsDocAccessible::gLastFocusedAccessiblesState = 0;
nsIAtom *nsDocAccessible::gLastFocusedFrameType = nsnull;





nsDocAccessible::
  nsDocAccessible(nsIDocument *aDocument, nsIContent *aRootContent,
                  nsIWeakReference *aShell) :
  nsHyperTextAccessibleWrap(aRootContent, aShell), mWnd(nsnull),
  mDocument(aDocument), mScrollPositionChangedTicks(0), mIsLoaded(PR_FALSE)
{
  
  mAccessibleCache.Init(kDefaultCacheSize);

  
  if (!mDocument)
    return;

  
  nsCOMPtr<nsIPresShell> shell(do_QueryReferent(mWeakShell));
  nsIViewManager* vm = shell->GetViewManager();
  if (vm) {
    nsCOMPtr<nsIWidget> widget;
    vm->GetRootWidget(getter_AddRefs(widget));
    if (widget) {
      mWnd = widget->GetNativeData(NS_NATIVE_WINDOW);
    }
  }

  
  
  AddScrollListener();
}

nsDocAccessible::~nsDocAccessible()
{
}





NS_IMPL_CYCLE_COLLECTION_CLASS(nsDocAccessible)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDocAccessible, nsAccessible)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mEventQueue");
  cb.NoteXPCOMChild(tmp->mEventQueue.get());

  CycleCollectorTraverseCache(tmp->mAccessibleCache, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDocAccessible, nsAccessible)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mEventQueue)
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


nsresult
nsDocAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_PANE; 

  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
    nsCoreUtils::GetDocShellTreeItemFor(mDocument);
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

  if (aExtraState)
    *aExtraState = 0;

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

NS_IMETHODIMP
nsDocAccessible::GetDOMDocument(nsIDOMDocument **aDOMDocument)
{
  NS_ENSURE_ARG_POINTER(aDOMDocument);
  *aDOMDocument = nsnull;

  if (mDocument)
    CallQueryInterface(mDocument, aDOMDocument);

  return NS_OK;
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
nsDocAccessible::GetCachedAccessible(void *aUniqueID)
{
  nsAccessible* accessible = mAccessibleCache.GetWeak(aUniqueID);

  
  
  if (!accessible) {
    void* thisUniqueID = nsnull;
    GetUniqueID(&thisUniqueID);
    if (thisUniqueID != aUniqueID)
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
nsDocAccessible::CacheAccessible(void *aUniqueID, nsAccessible *aAccessible)
{
  
  
  nsAccessible *accessible = mAccessibleCache.GetWeak(aUniqueID);
  NS_ASSERTION(!accessible,
               "Caching new accessible for the DOM node while the old one is alive");

  if (accessible)
    accessible->Shutdown();

  return mAccessibleCache.Put(aUniqueID, aAccessible);
}


void
nsDocAccessible::RemoveAccessNodeFromCache(nsAccessible *aAccessible)
{
  if (!aAccessible)
    return;

  void *uniqueID = nsnull;
  aAccessible->GetUniqueID(&uniqueID);
  mAccessibleCache.Remove(uniqueID);
}




PRBool
nsDocAccessible::Init()
{
  NS_LOG_ACCDOCCREATE_FOR("document initialize", mDocument, this)

  
  mEventQueue = new nsAccEventQueue(this);
  if (!mEventQueue)
    return PR_FALSE;

  AddEventListeners();

  
  
  nsRefPtr<nsAccEvent> reorderEvent =
    new nsAccReorderEvent(mParent, PR_FALSE, PR_TRUE, mDocument);
  if (!reorderEvent)
    return PR_FALSE;

  FireDelayedAccessibleEvent(reorderEvent);
  return PR_TRUE;
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

  if (mParent)
    mParent->RemoveChild(this);

  mWeakShell = nsnull;  

  ClearCache(mAccessibleCache);

  nsCOMPtr<nsIDocument> kungFuDeathGripDoc = mDocument;
  mDocument = nsnull;

  nsHyperTextAccessibleWrap::Shutdown();
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
    
    
    
    
    nsRefPtr<nsAccEvent> event =
      new nsAccStateChangeEvent(this, nsIAccessibleStates::EXT_STATE_EDITABLE,
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

  
  if (aContent == gLastFocusedNode) {
    nsAccessible *focusedAccessible = GetAccService()->GetAccessible(aContent);
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

    
    
    

    
    

    nsRefPtr<nsAccEvent> enabledChangeEvent =
      new nsAccStateChangeEvent(aContent,
                                nsIAccessibleStates::EXT_STATE_ENABLED,
                                PR_TRUE);

    FireDelayedAccessibleEvent(enabledChangeEvent);

    nsRefPtr<nsAccEvent> sensitiveChangeEvent =
      new nsAccStateChangeEvent(aContent,
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
                                 nsAccEvent::eAllowDupes);

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
    nsRefPtr<nsAccEvent> editableChangeEvent =
      new nsAccStateChangeEvent(aContent,
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
    nsRefPtr<nsAccEvent> event =
      new nsAccStateChangeEvent(aContent,
                                nsIAccessibleStates::STATE_REQUIRED,
                                PR_FALSE);
    FireDelayedAccessibleEvent(event);
    return;
  }

  if (aAttribute == nsAccessibilityAtoms::aria_invalid) {
    nsRefPtr<nsAccEvent> event =
      new nsAccStateChangeEvent(aContent,
                                nsIAccessibleStates::STATE_INVALID,
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
    nsRefPtr<nsAccEvent> event =
      new nsAccStateChangeEvent(aContent,
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
      new nsAccStateChangeEvent(aContent, kState, PR_FALSE);
    FireDelayedAccessibleEvent(event);
    if (aContent == gLastFocusedNode) {
      
      
      
      
      nsAccessible *accessible = event->GetAccessible();
      if (accessible) {
        PRBool wasMixed = (gLastFocusedAccessiblesState & nsIAccessibleStates::STATE_MIXED) != 0;
        PRBool isMixed  =
          (nsAccUtils::State(accessible) & nsIAccessibleStates::STATE_MIXED) != 0;
        if (wasMixed != isMixed) {
          nsRefPtr<nsAccEvent> event =
            new nsAccStateChangeEvent(aContent,
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
      new nsAccStateChangeEvent(aContent,
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
                               aContent);
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
                                      nsIContent* aFirstNewContent,
                                      PRInt32 )
{
  if (!IsContentLoaded() && mAccessibleCache.Count() <= 1) {
    
    InvalidateChildren();
    return;
  }

  
  for (nsIContent* cur = aFirstNewContent; cur; cur = cur->GetNextSibling()) {
    
    
    
    
    InvalidateCacheSubtree(cur, nsIAccessibilityService::NODE_APPEND);
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
                                 nsIContent* aChild, PRInt32 )
{
  
  
  
  
  InvalidateCacheSubtree(aChild, nsIAccessibilityService::NODE_APPEND);
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
nsDocAccessible::HandleAccEvent(nsAccEvent *aAccEvent)
{
  NS_LOG_ACCDOCLOAD_HANDLEEVENT(aAccEvent)

  return nsHyperTextAccessible::HandleAccEvent(aAccEvent);

}
#endif








void
nsDocAccessible::FireValueChangeForTextFields(nsAccessible *aAccessible)
{
  if (nsAccUtils::Role(aAccessible) != nsIAccessibleRole::ROLE_ENTRY)
    return;

  
  nsRefPtr<nsAccEvent> valueChangeEvent =
    new nsAccEvent(nsIAccessibleEvent::EVENT_VALUE_CHANGE, aAccessible,
                   PR_FALSE, eAutoDetect, nsAccEvent::eRemoveDupes);
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

  
  
  
  nsRefPtr<nsAccEvent> event =
    new nsAccTextChangeEvent(textAccessible, offset + textOffset, text,
                             aIsInserted, PR_FALSE);
  FireDelayedAccessibleEvent(event);

  FireValueChangeForTextFields(textAccessible);
}

already_AddRefed<nsAccEvent>
nsDocAccessible::CreateTextChangeEventForNode(nsAccessible *aContainerAccessible,
                                              nsIContent *aChangeNode,
                                              nsAccessible *aChangeChild,
                                              PRBool aIsInserting,
                                              PRBool aIsAsynch,
                                              EIsFromUserInput aIsFromUserInput)
{
  nsRefPtr<nsHyperTextAccessible> textAccessible =
    do_QueryObject(aContainerAccessible);
  if (!textAccessible) {
    return nsnull;
  }

  nsAutoString text;
  PRInt32 offset = 0;
  if (aChangeChild) {
    
    if (nsAccUtils::Role(aChangeChild) == nsIAccessibleRole::ROLE_WHITESPACE) {
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

    offset = textAccessible->GetChildOffset(aChangeChild);
    aChangeChild->AppendTextTo(text, 0, PR_UINT32_MAX);

  } else {
    
    
    
    
    nsAccTreeWalker walker(mWeakShell, aChangeNode,
                           GetAllowsAnonChildAccessibles());
    nsRefPtr<nsAccessible> child = walker.GetNextChild();

    
    if (!child)
      return nsnull;

    offset = textAccessible->GetChildOffset(child);
    child->AppendTextTo(text, 0, PR_UINT32_MAX);

    nsINode* containerNode = textAccessible->GetNode();
    PRInt32 childCount = textAccessible->GetChildCount();
    PRInt32 childIdx = child->GetIndexInParent();

    for (PRInt32 idx = childIdx + 1; idx < childCount; idx++) {
      nsAccessible* nextChild = textAccessible->GetChildAt(idx);
      
      if (!nsCoreUtils::IsAncestorOf(aChangeNode, nextChild->GetNode(),
                                     containerNode))
        break;

      nextChild->AppendTextTo(text, 0, PR_UINT32_MAX);
    }
  }

  if (text.IsEmpty())
    return nsnull;

  nsAccEvent *event =
    new nsAccTextChangeEvent(aContainerAccessible, offset, text,
                             aIsInserting, aIsAsynch, aIsFromUserInput);
  NS_IF_ADDREF(event);

  return event;
}


nsresult
nsDocAccessible::FireDelayedAccessibleEvent(PRUint32 aEventType, nsINode *aNode,
                                            nsAccEvent::EEventRule aAllowDupes,
                                            PRBool aIsAsynch,
                                            EIsFromUserInput aIsFromUserInput)
{
  nsRefPtr<nsAccEvent> event =
    new nsAccEvent(aEventType, aNode, aIsAsynch, aIsFromUserInput, aAllowDupes);
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  return FireDelayedAccessibleEvent(event);
}


nsresult
nsDocAccessible::FireDelayedAccessibleEvent(nsAccEvent *aEvent)
{
  NS_ENSURE_ARG(aEvent);
  NS_LOG_ACCDOCLOAD_FIREEVENT(aEvent)

  if (mEventQueue)
    mEventQueue->Push(aEvent);

  return NS_OK;
}

void
nsDocAccessible::ProcessPendingEvent(nsAccEvent *aEvent)
{  
  nsAccessible *accessible = aEvent->GetAccessible();
  nsINode *node = aEvent->GetNode();

  PRUint32 eventType = aEvent->GetEventType();
  EIsFromUserInput isFromUserInput =
    aEvent->IsFromUserInput() ? eFromUserInput : eNoUserInput;

  PRBool isAsync = aEvent->IsAsync();

  if (node == gLastFocusedNode && isAsync &&
      (eventType == nsIAccessibleEvent::EVENT_SHOW ||
       eventType == nsIAccessibleEvent::EVENT_HIDE)) {
    
    
    
    
    nsCOMPtr<nsIContent> focusContent(do_QueryInterface(node));
    if (focusContent) {
      nsIFrame *focusFrame = focusContent->GetPrimaryFrame();
      nsIAtom *newFrameType =
        (focusFrame && focusFrame->GetStyleVisibility()->IsVisible()) ?
        focusFrame->GetType() : nsnull;

      if (newFrameType == gLastFocusedFrameType) {
        
        
        FireShowHideEvents(node, PR_TRUE, eventType, eNormalEvent,
                           isAsync, isFromUserInput);
        return;
      }
      gLastFocusedFrameType = newFrameType;
    }
  }

  if (eventType == nsIAccessibleEvent::EVENT_SHOW) {

    nsAccessible* containerAccessible = nsnull;
    if (accessible) {
      containerAccessible = accessible->GetParent();
    } else {
      nsCOMPtr<nsIWeakReference> weakShell(nsCoreUtils::GetWeakShellFor(node));
      containerAccessible = GetAccService()->GetContainerAccessible(node,
                                                                    weakShell);
    }

    if (!containerAccessible)
      containerAccessible = this;

    if (isAsync) {
      
      containerAccessible->InvalidateChildren();

      
      
      
      InvalidateChildrenInSubtree(node);
    }

    
    
    
    
    
    if (node && node != mDocument) {
      nsCOMPtr<nsIContent> content(do_QueryInterface(node));
      nsRefPtr<nsAccEvent> textChangeEvent =
        CreateTextChangeEventForNode(containerAccessible, content, accessible,
                                     PR_TRUE, PR_TRUE, isFromUserInput);
      if (textChangeEvent) {
        
        
        
        nsEventShell::FireEvent(textChangeEvent);
      }
    }

    
    FireShowHideEvents(node, PR_FALSE, eventType, eNormalEvent, isAsync,
                       isFromUserInput); 
    return;
  }

  if (accessible) {
    if (eventType == nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED) {
      nsCOMPtr<nsIAccessibleText> accessibleText = do_QueryObject(accessible);
      PRInt32 caretOffset;
      if (accessibleText && NS_SUCCEEDED(accessibleText->GetCaretOffset(&caretOffset))) {
#ifdef DEBUG_A11Y
        PRUnichar chAtOffset;
        accessibleText->GetCharacterAtOffset(caretOffset, &chAtOffset);
        printf("\nCaret moved to %d with char %c", caretOffset, chAtOffset);
#endif
#ifdef DEBUG_CARET
        
        
        nsAccessible *focusedAcc =
          GetAccService()->GetAccessible(gLastFocusedNode);
        nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_ALERT, focusedAcc);
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
      
      
      
      nsAccReorderEvent *reorderEvent = downcast_accEvent(aEvent);
      if (reorderEvent->IsUnconditionalEvent() ||
          reorderEvent->HasAccessibleInReasonSubtree()) {
        nsEventShell::FireEvent(aEvent);
      }
    }
    else {
      nsEventShell::FireEvent(aEvent);

      
      if (eventType == nsIAccessibleEvent::EVENT_HIDE && node) {
        
        
        
        RefreshNodes(node);
      }
    }
  }
}

void
nsDocAccessible::InvalidateChildrenInSubtree(nsINode *aStartNode)
{
  nsAccessible *accessible = GetCachedAccessible(aStartNode);
  if (accessible)
    accessible->InvalidateChildren();

  
  PRInt32 index, numChildren = aStartNode->GetChildCount();
  for (index = 0; index < numChildren; index ++) {
    nsINode *childNode = aStartNode->GetChildAt(index);
    InvalidateChildrenInSubtree(childNode);
  }
}

void
nsDocAccessible::RefreshNodes(nsINode *aStartNode)
{
  if (mAccessibleCache.Count() <= 1) {
    return; 
  }

  
  
  nsAccessible *accessible = GetCachedAccessible(aStartNode);
  if (accessible) {
    
    PRUint32 role = nsAccUtils::Role(accessible);
    if (role == nsIAccessibleRole::ROLE_MENUPOPUP) {
      nsCOMPtr<nsIDOMXULPopupElement> popup(do_QueryInterface(aStartNode));
      if (!popup) {
        
        
        nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_MENUPOPUP_END,
                                accessible);
      }
    }

    
    
    if (accessible->GetCachedChildCount() > 0) {
      nsCOMPtr<nsIArray> children;
      
      
      accessible->GetChildren(getter_AddRefs(children));
      PRUint32 childCount =0;
      if (children)
        children->GetLength(&childCount);
      nsINode *possibleAnonNode = nsnull;
      for (PRUint32 index = 0; index < childCount; index++) {
        nsRefPtr<nsAccessNode> childAccessNode;
        children->QueryElementAt(index, NS_GET_IID(nsAccessNode),
                                 getter_AddRefs(childAccessNode));
        possibleAnonNode = childAccessNode->GetNode();
        nsCOMPtr<nsIContent> iterContent = do_QueryInterface(possibleAnonNode);
        if (iterContent && iterContent->IsInAnonymousSubtree()) {
          
          
          
          RefreshNodes(possibleAnonNode);
        }
      }
    }
  }

  
  
  PRUint32 childCount = aStartNode->GetChildCount();
  for (PRUint32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsIContent *childContent = aStartNode->GetChildAt(childIdx);
    RefreshNodes(childContent);
  }

  if (!accessible)
    return;

  if (accessible == this) {
    
    
    
    
    InvalidateChildren();
    return;
  }

  
  void *uniqueID;
  accessible->GetUniqueID(&uniqueID);
  accessible->Shutdown();

  
  mAccessibleCache.Remove(uniqueID);
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

  
  
  
  

  NS_ENSURE_TRUE(mDocument,);

  nsINode *childNode = aChild;
  if (!childNode)
    childNode = mDocument;

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  NS_ENSURE_TRUE(presShell,);
  
  if (!IsContentLoaded()) {
    
    if (mAccessibleCache.Count() <= 1) {
      
      
      
      
      
      
      
      InvalidateChildren();
      return;
    }

    nsIEventStateManager *esm = presShell->GetPresContext()->EventStateManager();
    NS_ENSURE_TRUE(esm,);

    if (!esm->IsHandlingUserInputExternal()) {
      
      
      
      nsAccessible *containerAccessible =
        GetAccService()->GetCachedContainerAccessible(childNode);
      if (!containerAccessible) {
        containerAccessible = this;
      }

      containerAccessible->InvalidateChildren();
      return;
    }     
    
    
    
  }

  
  nsAccessible *childAccessible = GetCachedAccessible(childNode);

#ifdef DEBUG_A11Y
  nsAutoString localName;
  if (aChild)
    aChild->NodeInfo()->GetName(localName);
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

  nsAccessible *containerAccessible =
    GetAccService()->GetCachedContainerAccessible(childNode);
  if (!containerAccessible) {
    containerAccessible = this;
  }

  if (!isShowing) {
    
    if (isHiding) {
      if (aChild) {
        nsIFrame *frame = aChild->GetPrimaryFrame();
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
  }

  
  
  
  
  
  

  if (aChild && !isHiding) {
    if (!isAsynch) {
      
      
      containerAccessible->InvalidateChildren();
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
        FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_ALERT, ancestor,
                                   nsAccEvent::eRemoveDupes, isAsynch);
        break;
      }
      ancestor = ancestor->GetParent();
      if (!ancestor)
        break;

      roleMapEntry = nsAccUtils::GetRoleMapEntry(ancestor);
    }
  }

  FireValueChangeForTextFields(containerAccessible);

  
  

  
  
  
  
  
  
  
  

  PRBool isUnconditionalEvent = childAccessible ||
    aChild && nsAccUtils::HasAccessibleChildren(childNode);

  nsRefPtr<nsAccEvent> reorderEvent =
    new nsAccReorderEvent(containerAccessible, isAsynch,
                          isUnconditionalEvent,
                          aChild ? aChild : nsnull);
  NS_ENSURE_TRUE(reorderEvent,);

  FireDelayedAccessibleEvent(reorderEvent);
}

nsresult
nsDocAccessible::FireShowHideEvents(nsINode *aNode,
                                    PRBool aAvoidOnThisNode,
                                    PRUint32 aEventType,
                                    EEventFiringType aDelayedOrNormal,
                                    PRBool aIsAsyncChange,
                                    EIsFromUserInput aIsFromUserInput)
{
  NS_ENSURE_ARG(aNode);

  nsAccessible *accessible = nsnull;
  if (!aAvoidOnThisNode) {
    if (aEventType == nsIAccessibleEvent::EVENT_HIDE) {
      
      accessible = GetCachedAccessible(aNode);
    } else {
      
      accessible = GetAccService()->GetAccessible(aNode);
    }
  }

  if (accessible) {
    
    
    nsRefPtr<nsAccEvent> event;
    if (aDelayedOrNormal == eDelayedEvent &&
        aEventType == nsIAccessibleEvent::EVENT_HIDE) {
      
      
      event = new AccHideEvent(accessible, accessible->GetNode(),
                               aIsAsyncChange, aIsFromUserInput);

    } else {
      event = new nsAccEvent(aEventType, accessible, aIsAsyncChange,
                             aIsFromUserInput,
                             nsAccEvent::eCoalesceFromSameSubtree);
    }
    NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

    if (aDelayedOrNormal == eDelayedEvent)
      return FireDelayedAccessibleEvent(event);

    nsEventShell::FireEvent(event);
    return NS_OK;
  }

  
  
  PRUint32 count = aNode->GetChildCount();
  for (PRUint32 index = 0; index < count; index++) {
    nsINode *childNode = aNode->GetChildAt(index);
    nsresult rv = FireShowHideEvents(childNode, PR_FALSE, aEventType,
                                     aDelayedOrNormal, aIsAsyncChange,
                                     aIsFromUserInput);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}
