





































#include "nsAccEvent.h"

#include "nsApplicationAccessibleWrap.h"
#include "nsDocAccessible.h"

#include "nsIDOMDocument.h"
#include "nsIEventStateManager.h"
#include "nsIPersistentProperties2.h"
#include "nsIServiceManager.h"
#ifdef MOZ_XUL
#include "nsIDOMXULMultSelectCntrlEl.h"
#include "nsXULTreeAccessible.h"
#endif
#include "nsIAccessibleText.h"
#include "nsIContent.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"








NS_IMPL_CYCLE_COLLECTION_1(nsAccEvent, mAccessible)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsAccEvent)
  NS_INTERFACE_MAP_ENTRY(nsIAccessibleEvent)
  NS_INTERFACE_MAP_ENTRY(nsAccEvent)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsAccEvent)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsAccEvent)




nsAccEvent::nsAccEvent(PRUint32 aEventType, nsIAccessible *aAccessible,
                       PRBool aIsAsync, EIsFromUserInput aIsFromUserInput,
                       EEventRule aEventRule) :
  mEventType(aEventType), mEventRule(aEventRule), mIsAsync(aIsAsync),
  mAccessible(aAccessible)
{
  CaptureIsFromUserInput(aIsFromUserInput);
}

nsAccEvent::nsAccEvent(PRUint32 aEventType, nsIDOMNode *aDOMNode,
                       PRBool aIsAsync, EIsFromUserInput aIsFromUserInput,
                       EEventRule aEventRule) :
  mEventType(aEventType), mEventRule(aEventRule), mIsAsync(aIsAsync),
  mNode(do_QueryInterface(aDOMNode))
{
  CaptureIsFromUserInput(aIsFromUserInput);
}




NS_IMETHODIMP
nsAccEvent::GetIsFromUserInput(PRBool *aIsFromUserInput)
{
  *aIsFromUserInput = mIsFromUserInput;
  return NS_OK;
}

NS_IMETHODIMP
nsAccEvent::SetIsFromUserInput(PRBool aIsFromUserInput)
{
  mIsFromUserInput = aIsFromUserInput;
  return NS_OK;
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

  if (!mNode)
    mNode = GetNode();

  if (mNode)
    CallQueryInterface(mNode, aDOMNode);

  return NS_OK;
}

NS_IMETHODIMP
nsAccEvent::GetAccessibleDocument(nsIAccessibleDocument **aDocAccessible)
{
  NS_ENSURE_ARG_POINTER(aDocAccessible);

  NS_IF_ADDREF(*aDocAccessible = GetDocAccessible());
  return NS_OK;
}




nsINode*
nsAccEvent::GetNode()
{
  if (!mNode) {
    nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(mAccessible));
    if (!accessNode)
      return nsnull;

    nsCOMPtr<nsIDOMNode> DOMNode;
    accessNode->GetDOMNode(getter_AddRefs(DOMNode));

    mNode = do_QueryInterface(DOMNode);
  }

  return mNode;
}

nsDocAccessible*
nsAccEvent::GetDocAccessible()
{
  nsINode *node = GetNode();
  if (node)
    return nsAccessNode::GetDocAccessibleFor(node->GetOwnerDoc());

  return nsnull;
}




already_AddRefed<nsIAccessible>
nsAccEvent::GetAccessibleByNode()
{
  if (!mNode)
    return nsnull;

  nsCOMPtr<nsIAccessibilityService> accService = 
    do_GetService("@mozilla.org/accessibilityService;1");
  if (!accService)
    return nsnull;

  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mNode));

  nsCOMPtr<nsIAccessible> accessible;
  accService->GetAccessibleFor(DOMNode, getter_AddRefs(accessible));

#ifdef MOZ_XUL
  
  
  
  
  nsCOMPtr<nsIContent> content(do_QueryInterface(mNode));
  if (content && content->NodeInfo()->Equals(nsAccessibilityAtoms::tree,
                                             kNameSpaceID_XUL)) {

    nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelect =
      do_QueryInterface(mNode);

    if (multiSelect) {
      PRInt32 treeIndex = -1;
      multiSelect->GetCurrentIndex(&treeIndex);
      if (treeIndex >= 0) {
        nsRefPtr<nsXULTreeAccessible> treeAcc =
          nsAccUtils::QueryAccessibleTree(accessible);
        if (treeAcc)
          accessible = treeAcc->GetTreeItemAccessible(treeIndex);
      }
    }
  }
#endif

  return accessible.forget();
}

void
nsAccEvent::CaptureIsFromUserInput(EIsFromUserInput aIsFromUserInput)
{
  nsCOMPtr<nsIDOMNode> targetNode;
  GetDOMNode(getter_AddRefs(targetNode));

#ifdef DEBUG
  if (!targetNode) {
    
    
    
    nsApplicationAccessible *applicationAcc =
      nsAccessNode::GetApplicationAccessible();

    if (mAccessible != static_cast<nsIAccessible*>(applicationAcc))
      NS_ASSERTION(targetNode, "There should always be a DOM node for an event");
  }
#endif

  if (aIsFromUserInput != eAutoDetect) {
    mIsFromUserInput = aIsFromUserInput == eFromUserInput ? PR_TRUE : PR_FALSE;
    return;
  }

  if (!targetNode)
    return;

  nsCOMPtr<nsIPresShell> presShell = nsCoreUtils::GetPresShellFor(targetNode);
  if (!presShell) {
    NS_NOTREACHED("Threre should always be an pres shell for an event");
    return;
  }

  nsIEventStateManager *esm = presShell->GetPresContext()->EventStateManager();
  if (!esm) {
    NS_NOTREACHED("There should always be an ESM for an event");
    return;
  }

  mIsFromUserInput = esm->IsHandlingUserInputExternal();
}






NS_IMPL_ISUPPORTS_INHERITED1(nsAccReorderEvent, nsAccEvent,
                             nsAccReorderEvent)

nsAccReorderEvent::nsAccReorderEvent(nsIAccessible *aAccTarget,
                                     PRBool aIsAsynch,
                                     PRBool aIsUnconditional,
                                     nsIDOMNode *aReasonNode) :
  nsAccEvent(::nsIAccessibleEvent::EVENT_REORDER, aAccTarget,
             aIsAsynch, eAutoDetect, nsAccEvent::eCoalesceFromSameSubtree),
  mUnconditionalEvent(aIsUnconditional), mReasonNode(aReasonNode)
{
}

PRBool
nsAccReorderEvent::IsUnconditionalEvent()
{
  return mUnconditionalEvent;
}

PRBool
nsAccReorderEvent::HasAccessibleInReasonSubtree()
{
  if (!mReasonNode)
    return PR_FALSE;

  nsCOMPtr<nsIAccessible> accessible;
  GetAccService()->GetAccessibleFor(mReasonNode, getter_AddRefs(accessible));

  return accessible || nsAccUtils::HasAccessibleChildren(mReasonNode);
}





NS_IMPL_ISUPPORTS_INHERITED1(nsAccStateChangeEvent, nsAccEvent,
                             nsIAccessibleStateChangeEvent)

nsAccStateChangeEvent::
  nsAccStateChangeEvent(nsIAccessible *aAccessible,
                        PRUint32 aState, PRBool aIsExtraState,
                        PRBool aIsEnabled):
  nsAccEvent(::nsIAccessibleEvent::EVENT_STATE_CHANGE, aAccessible),
  mState(aState), mIsExtraState(aIsExtraState), mIsEnabled(aIsEnabled)
{
}

nsAccStateChangeEvent::
  nsAccStateChangeEvent(nsIDOMNode *aNode,
                        PRUint32 aState, PRBool aIsExtraState,
                        PRBool aIsEnabled):
  nsAccEvent(::nsIAccessibleEvent::EVENT_STATE_CHANGE, aNode),
  mState(aState), mIsExtraState(aIsExtraState), mIsEnabled(aIsEnabled)
{
}

nsAccStateChangeEvent::
  nsAccStateChangeEvent(nsIDOMNode *aNode,
                        PRUint32 aState, PRBool aIsExtraState):
  nsAccEvent(::nsIAccessibleEvent::EVENT_STATE_CHANGE, aNode),
  mState(aState), mIsExtraState(aIsExtraState)
{
  
  
  
  nsCOMPtr<nsIAccessible> accessible(GetAccessibleByNode());
  if (accessible) {
    PRUint32 state = 0, extraState = 0;
    accessible->GetState(&state, mIsExtraState ? &extraState : nsnull);
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
                       PRInt32 aStart, PRUint32 aLength, PRBool aIsInserted,
                       PRBool aIsAsynch, EIsFromUserInput aIsFromUserInput) :
  nsAccEvent(aIsInserted ? nsIAccessibleEvent::EVENT_TEXT_INSERTED : nsIAccessibleEvent::EVENT_TEXT_REMOVED,
             aAccessible, aIsAsynch, aIsFromUserInput),
  mStart(aStart), mLength(aLength), mIsInserted(aIsInserted)
{
#ifdef XP_WIN
  nsCOMPtr<nsIAccessibleText> textAccessible = do_QueryInterface(aAccessible);
  NS_ASSERTION(textAccessible, "Should not be firing test change event for non-text accessible!!!");
  if (textAccessible) {
    textAccessible->GetText(aStart, aStart + aLength, mModifiedText);
  }
#endif
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

NS_IMETHODIMP
nsAccTextChangeEvent::GetModifiedText(nsAString& aModifiedText)
{
  aModifiedText = mModifiedText;
  return NS_OK;
}





NS_IMPL_ISUPPORTS_INHERITED1(nsAccCaretMoveEvent, nsAccEvent,
                             nsIAccessibleCaretMoveEvent)

nsAccCaretMoveEvent::
  nsAccCaretMoveEvent(nsIAccessible *aAccessible, PRInt32 aCaretOffset) :
  nsAccEvent(::nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED, aAccessible, PR_TRUE), 
  mCaretOffset(aCaretOffset)
{
}

nsAccCaretMoveEvent::
  nsAccCaretMoveEvent(nsIDOMNode *aNode) :
  nsAccEvent(::nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED, aNode, PR_TRUE), 
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





NS_IMPL_ISUPPORTS_INHERITED1(nsAccTableChangeEvent, nsAccEvent,
                             nsIAccessibleTableChangeEvent)

nsAccTableChangeEvent::
  nsAccTableChangeEvent(nsIAccessible *aAccessible, PRUint32 aEventType,
                        PRInt32 aRowOrColIndex, PRInt32 aNumRowsOrCols, PRBool aIsAsynch):
  nsAccEvent(aEventType, aAccessible, aIsAsynch), 
  mRowOrColIndex(aRowOrColIndex), mNumRowsOrCols(aNumRowsOrCols)
{
}

NS_IMETHODIMP
nsAccTableChangeEvent::GetRowOrColIndex(PRInt32* aRowOrColIndex)
{
  NS_ENSURE_ARG_POINTER(aRowOrColIndex);

  *aRowOrColIndex = mRowOrColIndex;
  return NS_OK;
}

NS_IMETHODIMP
nsAccTableChangeEvent::GetNumRowsOrCols(PRInt32* aNumRowsOrCols)
{
  NS_ENSURE_ARG_POINTER(aNumRowsOrCols);

  *aNumRowsOrCols = mNumRowsOrCols;
  return NS_OK;
}

