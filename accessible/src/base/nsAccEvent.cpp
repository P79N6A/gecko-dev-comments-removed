





































#include "nsAccEvent.h"

#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsApplicationAccessibleWrap.h"
#include "nsDocAccessible.h"
#include "nsIAccessibleText.h"
#ifdef MOZ_XUL
#include "nsXULTreeAccessible.h"
#endif

#include "nsIDOMDocument.h"
#include "nsIEventStateManager.h"
#include "nsIServiceManager.h"
#ifdef MOZ_XUL
#include "nsIDOMXULMultSelectCntrlEl.h"
#endif








NS_IMPL_CYCLE_COLLECTION_CLASS(nsAccEvent)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsAccEvent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mAccessible)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsAccEvent)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mAccessible");
  cb.NoteXPCOMChild(static_cast<nsIAccessible*>(tmp->mAccessible));
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsAccEvent)
  NS_INTERFACE_MAP_ENTRY(nsIAccessibleEvent)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsAccEvent)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsAccEvent)




nsAccEvent::nsAccEvent(PRUint32 aEventType, nsAccessible *aAccessible,
                       PRBool aIsAsync, EIsFromUserInput aIsFromUserInput,
                       EEventRule aEventRule) :
  mEventType(aEventType), mEventRule(aEventRule), mIsAsync(aIsAsync),
  mAccessible(aAccessible)
{
  CaptureIsFromUserInput(aIsFromUserInput);
}

nsAccEvent::nsAccEvent(PRUint32 aEventType, nsINode *aNode,
                       PRBool aIsAsync, EIsFromUserInput aIsFromUserInput,
                       EEventRule aEventRule) :
  mEventType(aEventType), mEventRule(aEventRule), mIsAsync(aIsAsync),
  mNode(aNode)
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

  NS_IF_ADDREF(*aAccessible = GetAccessible());
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




nsAccessible *
nsAccEvent::GetAccessible()
{
  if (!mAccessible)
    mAccessible = GetAccessibleForNode();

  return mAccessible;
}

nsINode*
nsAccEvent::GetNode()
{
  if (!mNode && mAccessible)
    mNode = mAccessible->GetNode();

  return mNode;
}

nsDocAccessible*
nsAccEvent::GetDocAccessible()
{
  nsINode *node = GetNode();
  if (node)
    return GetAccService()->GetDocAccessible(node->GetOwnerDoc());

  return nsnull;
}




nsAccessible *
nsAccEvent::GetAccessibleForNode() const
{
  if (!mNode)
    return nsnull;

  nsAccessible *accessible = GetAccService()->GetAccessible(mNode);

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
        nsRefPtr<nsXULTreeAccessible> treeAcc = do_QueryObject(accessible);
        if (treeAcc)
          return treeAcc->GetTreeItemAccessible(treeIndex);
      }
    }
  }
#endif

  return accessible;
}

void
nsAccEvent::CaptureIsFromUserInput(EIsFromUserInput aIsFromUserInput)
{
  nsINode *targetNode = GetNode();

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

  nsIPresShell *presShell = nsCoreUtils::GetPresShellFor(targetNode);
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






NS_IMPL_ISUPPORTS_INHERITED0(nsAccReorderEvent, nsAccEvent)

nsAccReorderEvent::nsAccReorderEvent(nsAccessible *aAccTarget,
                                     PRBool aIsAsynch,
                                     PRBool aIsUnconditional,
                                     nsINode *aReasonNode) :
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

  nsAccessible *accessible = GetAccService()->GetAccessible(mReasonNode);
  return accessible || nsAccUtils::HasAccessibleChildren(mReasonNode);
}






NS_IMPL_ISUPPORTS_INHERITED1(nsAccStateChangeEvent, nsAccEvent,
                             nsIAccessibleStateChangeEvent)




nsAccStateChangeEvent::
  nsAccStateChangeEvent(nsAccessible *aAccessible,
                        PRUint32 aState, PRBool aIsExtraState,
                        PRBool aIsEnabled, PRBool aIsAsynch,
                        EIsFromUserInput aIsFromUserInput):
  nsAccEvent(nsIAccessibleEvent::EVENT_STATE_CHANGE, aAccessible, aIsAsynch,
             aIsFromUserInput, eAllowDupes),
  mState(aState), mIsExtraState(aIsExtraState), mIsEnabled(aIsEnabled)
{
}

nsAccStateChangeEvent::
  nsAccStateChangeEvent(nsINode *aNode, PRUint32 aState, PRBool aIsExtraState,
                        PRBool aIsEnabled):
  nsAccEvent(::nsIAccessibleEvent::EVENT_STATE_CHANGE, aNode),
  mState(aState), mIsExtraState(aIsExtraState), mIsEnabled(aIsEnabled)
{
}

nsAccStateChangeEvent::
  nsAccStateChangeEvent(nsINode *aNode, PRUint32 aState, PRBool aIsExtraState) :
  nsAccEvent(::nsIAccessibleEvent::EVENT_STATE_CHANGE, aNode),
  mState(aState), mIsExtraState(aIsExtraState)
{
  
  
  
  nsAccessible *accessible = GetAccessibleForNode();
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
  NS_ENSURE_ARG_POINTER(aState);
  *aState = mState;
  return NS_OK;
}

NS_IMETHODIMP
nsAccStateChangeEvent::IsExtraState(PRBool *aIsExtraState)
{
  NS_ENSURE_ARG_POINTER(aIsExtraState);
  *aIsExtraState = mIsExtraState;
  return NS_OK;
}

NS_IMETHODIMP
nsAccStateChangeEvent::IsEnabled(PRBool *aIsEnabled)
{
  NS_ENSURE_ARG_POINTER(aIsEnabled);
  *aIsEnabled = mIsEnabled;
  return NS_OK;
}





NS_IMPL_ISUPPORTS_INHERITED1(nsAccTextChangeEvent, nsAccEvent,
                             nsIAccessibleTextChangeEvent)









nsAccTextChangeEvent::
  nsAccTextChangeEvent(nsAccessible *aAccessible, PRInt32 aStart,
                       nsAString& aModifiedText, PRBool aIsInserted,
                       PRBool aIsAsynch, EIsFromUserInput aIsFromUserInput)
  : nsAccEvent(aIsInserted ?
               static_cast<PRUint32>(nsIAccessibleEvent::EVENT_TEXT_INSERTED) :
               static_cast<PRUint32>(nsIAccessibleEvent::EVENT_TEXT_REMOVED),
               aAccessible, aIsAsynch, aIsFromUserInput, eAllowDupes)
  , mStart(aStart)
  , mIsInserted(aIsInserted)
  , mModifiedText(aModifiedText)
{
}

NS_IMETHODIMP
nsAccTextChangeEvent::GetStart(PRInt32 *aStart)
{
  NS_ENSURE_ARG_POINTER(aStart);
  *aStart = mStart;
  return NS_OK;
}

NS_IMETHODIMP
nsAccTextChangeEvent::GetLength(PRUint32 *aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);
  *aLength = GetLength();
  return NS_OK;
}

NS_IMETHODIMP
nsAccTextChangeEvent::IsInserted(PRBool *aIsInserted)
{
  NS_ENSURE_ARG_POINTER(aIsInserted);
  *aIsInserted = mIsInserted;
  return NS_OK;
}

NS_IMETHODIMP
nsAccTextChangeEvent::GetModifiedText(nsAString& aModifiedText)
{
  aModifiedText = mModifiedText;
  return NS_OK;
}






AccHideEvent::
  AccHideEvent(nsAccessible* aTarget, nsINode* aTargetNode,
               PRBool aIsAsynch, EIsFromUserInput aIsFromUserInput) :
  nsAccEvent(nsIAccessibleEvent::EVENT_HIDE, aTarget, aIsAsynch,
             aIsFromUserInput, eCoalesceFromSameSubtree)
{
  mNode = aTargetNode;
  mParent = mAccessible->GetCachedParent();
  mNextSibling = mAccessible->GetCachedNextSibling();
  mPrevSibling = mAccessible->GetCachedPrevSibling();
}






NS_IMPL_ISUPPORTS_INHERITED1(nsAccCaretMoveEvent, nsAccEvent,
                             nsIAccessibleCaretMoveEvent)

nsAccCaretMoveEvent::
  nsAccCaretMoveEvent(nsAccessible *aAccessible, PRInt32 aCaretOffset) :
  nsAccEvent(::nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED, aAccessible, PR_TRUE), 
  mCaretOffset(aCaretOffset)
{
}

nsAccCaretMoveEvent::
  nsAccCaretMoveEvent(nsINode *aNode) :
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
  nsAccTableChangeEvent(nsAccessible *aAccessible, PRUint32 aEventType,
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

