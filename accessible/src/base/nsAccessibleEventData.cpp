





































#include "nsAccessibleEventData.h"
#include "nsAccessibilityAtoms.h"
#include "nsIAccessibilityService.h"
#include "nsIAccessNode.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIEventStateManager.h"
#include "nsIPersistentProperties2.h"
#include "nsIServiceManager.h"
#ifdef MOZ_XUL
#include "nsIDOMXULMultSelectCntrlEl.h"
#include "nsXULTreeAccessible.h"
#endif

PRBool nsAccEvent::gLastEventFromUserInput = PR_FALSE;
nsIDOMNode* nsAccEvent::gLastEventNodeWeak = 0;

NS_IMPL_ISUPPORTS1(nsAccEvent, nsIAccessibleEvent)

nsAccEvent::nsAccEvent(PRUint32 aEventType, nsIAccessible *aAccessible,
                       void *aEventData, PRBool aIsAsynch):
  mEventType(aEventType), mAccessible(aAccessible), mEventData(aEventData)
{
  CaptureIsFromUserInput(aIsAsynch);
}

nsAccEvent::nsAccEvent(PRUint32 aEventType, nsIDOMNode *aDOMNode,
                       void *aEventData, PRBool aIsAsynch):
  mEventType(aEventType), mDOMNode(aDOMNode), mEventData(aEventData)
{
  CaptureIsFromUserInput(aIsAsynch);
}

void nsAccEvent::GetLastEventAttributes(nsIDOMNode *aNode,
                                        nsIPersistentProperties *aAttributes)
{
  if (aNode != gLastEventNodeWeak) {
    return; 
  }
  nsAutoString oldValueUnused;
  aAttributes->SetStringProperty(NS_LITERAL_CSTRING("event-from-input"),
                                 gLastEventFromUserInput ? NS_LITERAL_STRING("true") :
                                                           NS_LITERAL_STRING("false"),
                                 oldValueUnused);

  nsCOMPtr<nsIContent> lastEventContent = do_QueryInterface(aNode);
  nsIContent *loopContent = lastEventContent;

  nsAutoString atomic, live, relevant, channel, busy;

  while (loopContent) {
    if (relevant.IsEmpty()) {
      loopContent->GetAttr(kNameSpaceID_WAIProperties, nsAccessibilityAtoms::relevant, relevant);
    }
    if (live.IsEmpty()) {
      loopContent->GetAttr(kNameSpaceID_WAIProperties, nsAccessibilityAtoms::live, live);
    }
    if (channel.IsEmpty()) {
      loopContent->GetAttr(kNameSpaceID_WAIProperties, nsAccessibilityAtoms::channel, channel);
    }
    if (atomic.IsEmpty()) {
      loopContent->GetAttr(kNameSpaceID_WAIProperties, nsAccessibilityAtoms::atomic, atomic);
    }
    if (busy.IsEmpty()) {
      loopContent->GetAttr(kNameSpaceID_WAIProperties, nsAccessibilityAtoms::busy, busy);
    }
    loopContent = loopContent->GetParent();
  }

  if (!relevant.IsEmpty()) {
    aAttributes->SetStringProperty(NS_LITERAL_CSTRING("container-relevant"), relevant, oldValueUnused);
  }
  if (!live.IsEmpty()) {
    aAttributes->SetStringProperty(NS_LITERAL_CSTRING("container-live"), live, oldValueUnused);
  }
  if (!channel.IsEmpty()) {
    aAttributes->SetStringProperty(NS_LITERAL_CSTRING("container-channel"), channel, oldValueUnused);
  }
  if (!atomic.IsEmpty()) {
    aAttributes->SetStringProperty(NS_LITERAL_CSTRING("container-atomic"), atomic, oldValueUnused);
  }
  if (!busy.IsEmpty()) {
    aAttributes->SetStringProperty(NS_LITERAL_CSTRING("container-busy"), busy, oldValueUnused);
  }
}

nsIDOMNode* nsAccEvent::GetLastEventAtomicRegion(nsIDOMNode *aNode)
{
  if (aNode != gLastEventNodeWeak) {
    return nsnull; 
  }
  nsCOMPtr<nsIContent> lastEventContent = do_QueryInterface(aNode);
  nsIContent *loopContent = lastEventContent;
  nsAutoString atomic;

  while (loopContent) {
    loopContent->GetAttr(kNameSpaceID_WAIProperties, nsAccessibilityAtoms::atomic, atomic);
    if (!atomic.IsEmpty()) {
      break;
    }
    loopContent = loopContent->GetParent();
  }

  nsCOMPtr<nsIDOMNode> atomicRegion;
  if (atomic.EqualsLiteral("true")) {
    atomicRegion = do_QueryInterface(loopContent);
  }
  return atomicRegion;
}

void nsAccEvent::CaptureIsFromUserInput(PRBool aIsAsynch)
{
  nsCOMPtr<nsIDOMNode> eventNode;
  GetDOMNode(getter_AddRefs(eventNode));
  if (!eventNode) {
    NS_NOTREACHED("There should always be a DOM node for an event");
    return;
  }

  if (aIsAsynch) {
    
    gLastEventNodeWeak = eventNode;
  }
  else {
    PrepareForEvent(eventNode);
  }
  
  mIsFromUserInput = gLastEventFromUserInput;
}

NS_IMETHODIMP
nsAccEvent::GetIsFromUserInput(PRBool *aIsFromUserInput)
{
  *aIsFromUserInput = mIsFromUserInput;
  return NS_OK;
}

void nsAccEvent::PrepareForEvent(nsIAccessibleEvent *aEvent)
{
  nsCOMPtr<nsIDOMNode> eventNode;
  aEvent->GetDOMNode(getter_AddRefs(eventNode));
  PRBool isFromUserInput;
  aEvent->GetIsFromUserInput(&isFromUserInput);
  PrepareForEvent(eventNode, isFromUserInput);
}

void nsAccEvent::PrepareForEvent(nsIDOMNode *aEventNode,
                                 PRBool aForceIsFromUserInput)
{
  gLastEventNodeWeak = aEventNode;
  if (aForceIsFromUserInput) {
    gLastEventFromUserInput = PR_TRUE;
    return;
  }

  nsCOMPtr<nsIDOMDocument> domDoc;
  aEventNode->GetOwnerDocument(getter_AddRefs(domDoc));
  if (!domDoc) {  
    domDoc = do_QueryInterface(aEventNode);
  }
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  if (!doc) {
    NS_NOTREACHED("There should always be a document for an event");
    return;
  }

  nsCOMPtr<nsIPresShell> presShell = doc->GetPrimaryShell();
  if (!presShell) {
    NS_NOTREACHED("Threre should always be an pres shell for an event");
    return;
  }

  nsIEventStateManager *esm = presShell->GetPresContext()->EventStateManager();
  if (!esm) {
    NS_NOTREACHED("Threre should always be an ESM for an event");
    return;
  }

  gLastEventFromUserInput = esm->IsHandlingUserInputExternal();
}

NS_IMETHODIMP
nsAccEvent::GetEventType(PRUint32 *aEventType)
{
  *aEventType = mEventType;
  return NS_OK;
}

NS_IMETHODIMP
nsAccEvent::GetAccessible(nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;

  if (!mAccessible)
    mAccessible = GetAccessibleByNode();

  NS_IF_ADDREF(*aAccessible = mAccessible);
  return NS_OK;
}

NS_IMETHODIMP
nsAccEvent::GetDOMNode(nsIDOMNode **aDOMNode)
{
  NS_ENSURE_ARG_POINTER(aDOMNode);
  *aDOMNode = nsnull;

  if (!mDOMNode) {
    nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(mAccessible));
    NS_ENSURE_TRUE(accessNode, NS_ERROR_FAILURE);
    accessNode->GetDOMNode(getter_AddRefs(mDOMNode));
  }

  NS_IF_ADDREF(*aDOMNode = mDOMNode);
  return NS_OK;
}

NS_IMETHODIMP
nsAccEvent::GetAccessibleDocument(nsIAccessibleDocument **aDocAccessible)
{
  NS_ENSURE_ARG_POINTER(aDocAccessible);
  *aDocAccessible = nsnull;

  if (!mDocAccessible) {
    if (!mAccessible) {
      nsCOMPtr<nsIAccessible> accessible;
      GetAccessible(getter_AddRefs(accessible));
    }

    nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(mAccessible));
    NS_ENSURE_TRUE(accessNode, NS_ERROR_FAILURE);
    accessNode->GetAccessibleDocument(getter_AddRefs(mDocAccessible));
  }

  NS_IF_ADDREF(*aDocAccessible = mDocAccessible);
  return NS_OK;
}

already_AddRefed<nsIAccessible>
nsAccEvent::GetAccessibleByNode()
{
  if (!mDOMNode)
    return nsnull;

  nsCOMPtr<nsIAccessibilityService> accService = 
    do_GetService("@mozilla.org/accessibilityService;1");
  if (!accService)
    return nsnull;

  nsIAccessible *accessible = nsnull;
  accService->GetAccessibleFor(mDOMNode, &accessible);
#ifdef MOZ_XUL
  
  
  
  
  nsAutoString localName;
  mDOMNode->GetLocalName(localName);
  if (localName.EqualsLiteral("tree")) {
    nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelect =
      do_QueryInterface(mDOMNode);
    if (multiSelect) {
      PRInt32 treeIndex = -1;
      multiSelect->GetCurrentIndex(&treeIndex);
      if (treeIndex >= 0) {
        nsCOMPtr<nsIAccessibleTreeCache> treeCache(do_QueryInterface(accessible));
        NS_IF_RELEASE(accessible);
        nsCOMPtr<nsIAccessible> treeItemAccessible;
        if (!treeCache ||
            NS_FAILED(treeCache->GetCachedTreeitemAccessible(
                      treeIndex,
                      nsnull,
                      getter_AddRefs(treeItemAccessible))) ||
                      !treeItemAccessible) {
          return nsnull;
        }
        NS_IF_ADDREF(accessible = treeItemAccessible);
      }
    }
  }
#endif

  return accessible;
}



NS_IMPL_ISUPPORTS_INHERITED1(nsAccStateChangeEvent, nsAccEvent,
                             nsIAccessibleStateChangeEvent)

nsAccStateChangeEvent::
  nsAccStateChangeEvent(nsIAccessible *aAccessible,
                        PRUint32 aState, PRBool aIsExtraState,
                        PRBool aIsEnabled):
  nsAccEvent(::nsIAccessibleEvent::EVENT_STATE_CHANGE, aAccessible, nsnull),
  mState(aState), mIsExtraState(aIsExtraState), mIsEnabled(aIsEnabled)
{
}

nsAccStateChangeEvent::
  nsAccStateChangeEvent(nsIDOMNode *aNode,
                        PRUint32 aState, PRBool aIsExtraState,
                        PRBool aIsEnabled):
  nsAccEvent(::nsIAccessibleEvent::EVENT_STATE_CHANGE, aNode, nsnull),
  mState(aState), mIsExtraState(aIsExtraState), mIsEnabled(aIsEnabled)
{
}

nsAccStateChangeEvent::
  nsAccStateChangeEvent(nsIDOMNode *aNode,
                        PRUint32 aState, PRBool aIsExtraState):
  nsAccEvent(::nsIAccessibleEvent::EVENT_STATE_CHANGE, aNode, nsnull),
  mState(aState), mIsExtraState(aIsExtraState)
{
  
  
  
  nsCOMPtr<nsIAccessible> accessible(GetAccessibleByNode());
  if (accessible) {
    PRUint32 state = 0, extraState = 0;
    accessible->GetFinalState(&state, &extraState);
    mIsEnabled = ((mIsExtraState ? extraState : state) & mState) != 0;
  } else {
    mIsEnabled = PR_FALSE;
  }
}

NS_IMETHODIMP
nsAccStateChangeEvent::GetState(PRUint32 *aState)
{
  *aState = mState;
  return NS_OK;
}

NS_IMETHODIMP
nsAccStateChangeEvent::IsExtraState(PRBool *aIsExtraState)
{
  *aIsExtraState = mIsExtraState;
  return NS_OK;
}

NS_IMETHODIMP
nsAccStateChangeEvent::IsEnabled(PRBool *aIsEnabled)
{
  *aIsEnabled = mIsEnabled;
  return NS_OK;
}



NS_IMPL_ISUPPORTS_INHERITED1(nsAccTextChangeEvent, nsAccEvent,
                             nsIAccessibleTextChangeEvent)

nsAccTextChangeEvent::
  nsAccTextChangeEvent(nsIAccessible *aAccessible,
                       PRInt32 aStart, PRUint32 aLength, PRBool aIsInserted):
  nsAccEvent(aIsInserted ? nsIAccessibleEvent::EVENT_TEXT_INSERTED : nsIAccessibleEvent::EVENT_TEXT_REMOVED,
             aAccessible, nsnull),
  mStart(aStart), mLength(aLength), mIsInserted(aIsInserted)
{
}

NS_IMETHODIMP
nsAccTextChangeEvent::GetStart(PRInt32 *aStart)
{
  *aStart = mStart;
  return NS_OK;
}

NS_IMETHODIMP
nsAccTextChangeEvent::GetLength(PRUint32 *aLength)
{
  *aLength = mLength;
  return NS_OK;
}

NS_IMETHODIMP
nsAccTextChangeEvent::IsInserted(PRBool *aIsInserted)
{
  *aIsInserted = mIsInserted;
  return NS_OK;
}


NS_IMPL_ISUPPORTS_INHERITED1(nsAccCaretMoveEvent, nsAccEvent,
                             nsIAccessibleCaretMoveEvent)

nsAccCaretMoveEvent::
  nsAccCaretMoveEvent(nsIAccessible *aAccessible, PRInt32 aCaretOffset) :
  nsAccEvent(::nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED, aAccessible, nsnull, PR_TRUE), 
  mCaretOffset(aCaretOffset)
{
}

nsAccCaretMoveEvent::
  nsAccCaretMoveEvent(nsIDOMNode *aNode) :
  nsAccEvent(::nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED, aNode, nsnull, PR_TRUE), 
  mCaretOffset(-1)
{
}

NS_IMETHODIMP
nsAccCaretMoveEvent::GetCaretOffset(PRInt32* aCaretOffset)
{
  NS_ENSURE_ARG_POINTER(aCaretOffset);

  *aCaretOffset = mCaretOffset;
  return NS_OK;
}

