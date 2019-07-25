






































#include "AccEvent.h"

#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsApplicationAccessibleWrap.h"
#include "nsDocAccessible.h"
#include "nsIAccessibleText.h"
#ifdef MOZ_XUL
#include "nsXULTreeAccessible.h"
#endif
#include "nsAccEvent.h"

#include "nsIDOMDocument.h"
#include "nsIEventStateManager.h"
#include "nsIServiceManager.h"
#ifdef MOZ_XUL
#include "nsIDOMXULMultSelectCntrlEl.h"
#endif








AccEvent::AccEvent(PRUint32 aEventType, nsAccessible* aAccessible,
                   PRBool aIsAsync, EIsFromUserInput aIsFromUserInput,
                   EEventRule aEventRule) :
  mEventType(aEventType), mEventRule(aEventRule), mIsAsync(aIsAsync),
  mAccessible(aAccessible)
{
  CaptureIsFromUserInput(aIsFromUserInput);
}

AccEvent::AccEvent(PRUint32 aEventType, nsINode* aNode,
                   PRBool aIsAsync, EIsFromUserInput aIsFromUserInput,
                   EEventRule aEventRule) :
  mEventType(aEventType), mEventRule(aEventRule), mIsAsync(aIsAsync),
  mNode(aNode)
{
  CaptureIsFromUserInput(aIsFromUserInput);
}




nsAccessible *
AccEvent::GetAccessible()
{
  if (!mAccessible)
    mAccessible = GetAccessibleForNode();

  return mAccessible;
}

nsINode*
AccEvent::GetNode()
{
  if (!mNode && mAccessible)
    mNode = mAccessible->GetNode();

  return mNode;
}

nsDocAccessible*
AccEvent::GetDocAccessible()
{
  nsINode *node = GetNode();
  if (node)
    return GetAccService()->GetDocAccessible(node->GetOwnerDoc());

  return nsnull;
}

already_AddRefed<nsAccEvent>
AccEvent::CreateXPCOMObject()
{
  nsAccEvent* event = new nsAccEvent(this);
  NS_IF_ADDREF(event);
  return event;
}




NS_IMPL_CYCLE_COLLECTION_CLASS(AccEvent)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_NATIVE(AccEvent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mAccessible)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_BEGIN(AccEvent)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mAccessible");
  cb.NoteXPCOMChild(static_cast<nsIAccessible*>(tmp->mAccessible));
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(AccEvent, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(AccEvent, Release)




nsAccessible *
AccEvent::GetAccessibleForNode() const
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
AccEvent::CaptureIsFromUserInput(EIsFromUserInput aIsFromUserInput)
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






AccReorderEvent::
  AccReorderEvent(nsAccessible* aAccTarget, PRBool aIsAsynch,
                  PRBool aIsUnconditional, nsINode* aReasonNode) :
  AccEvent(::nsIAccessibleEvent::EVENT_REORDER, aAccTarget,
           aIsAsynch, eAutoDetect, AccEvent::eCoalesceFromSameSubtree),
  mUnconditionalEvent(aIsUnconditional), mReasonNode(aReasonNode)
{
}

PRBool
AccReorderEvent::IsUnconditionalEvent()
{
  return mUnconditionalEvent;
}

PRBool
AccReorderEvent::HasAccessibleInReasonSubtree()
{
  if (!mReasonNode)
    return PR_FALSE;

  nsAccessible *accessible = GetAccService()->GetAccessible(mReasonNode);
  return accessible || nsAccUtils::HasAccessibleChildren(mReasonNode);
}









AccStateChangeEvent::
  AccStateChangeEvent(nsAccessible* aAccessible,
                      PRUint32 aState, PRBool aIsExtraState,
                      PRBool aIsEnabled, PRBool aIsAsynch,
                      EIsFromUserInput aIsFromUserInput):
  AccEvent(nsIAccessibleEvent::EVENT_STATE_CHANGE, aAccessible, aIsAsynch,
           aIsFromUserInput, eAllowDupes),
  mState(aState), mIsExtraState(aIsExtraState), mIsEnabled(aIsEnabled)
{
}

AccStateChangeEvent::
  AccStateChangeEvent(nsINode* aNode, PRUint32 aState, PRBool aIsExtraState,
                      PRBool aIsEnabled):
  AccEvent(::nsIAccessibleEvent::EVENT_STATE_CHANGE, aNode),
  mState(aState), mIsExtraState(aIsExtraState), mIsEnabled(aIsEnabled)
{
}

AccStateChangeEvent::
  AccStateChangeEvent(nsINode* aNode, PRUint32 aState, PRBool aIsExtraState) :
  AccEvent(::nsIAccessibleEvent::EVENT_STATE_CHANGE, aNode),
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

already_AddRefed<nsAccEvent>
AccStateChangeEvent::CreateXPCOMObject()
{
  nsAccEvent* event = new nsAccStateChangeEvent(this);
  NS_IF_ADDREF(event);
  return event;
}














AccTextChangeEvent::
  AccTextChangeEvent(nsAccessible* aAccessible, PRInt32 aStart,
                     nsAString& aModifiedText, PRBool aIsInserted,
                     PRBool aIsAsynch, EIsFromUserInput aIsFromUserInput)
  : AccEvent(aIsInserted ?
             static_cast<PRUint32>(nsIAccessibleEvent::EVENT_TEXT_INSERTED) :
             static_cast<PRUint32>(nsIAccessibleEvent::EVENT_TEXT_REMOVED),
             aAccessible, aIsAsynch, aIsFromUserInput, eAllowDupes)
  , mStart(aStart)
  , mIsInserted(aIsInserted)
  , mModifiedText(aModifiedText)
{
}

already_AddRefed<nsAccEvent>
AccTextChangeEvent::CreateXPCOMObject()
{
  nsAccEvent* event = new nsAccTextChangeEvent(this);
  NS_IF_ADDREF(event);
  return event;
}






AccHideEvent::
  AccHideEvent(nsAccessible* aTarget, nsINode* aTargetNode,
               PRBool aIsAsynch, EIsFromUserInput aIsFromUserInput) :
  AccEvent(nsIAccessibleEvent::EVENT_HIDE, aTarget, aIsAsynch,
           aIsFromUserInput, eCoalesceFromSameSubtree)
{
  mNode = aTargetNode;
  mParent = mAccessible->GetCachedParent();
  mNextSibling = mAccessible->GetCachedNextSibling();
  mPrevSibling = mAccessible->GetCachedPrevSibling();
}






AccCaretMoveEvent::
  AccCaretMoveEvent(nsAccessible* aAccessible, PRInt32 aCaretOffset) :
  AccEvent(::nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED, aAccessible, PR_TRUE), 
  mCaretOffset(aCaretOffset)
{
}

AccCaretMoveEvent::
  AccCaretMoveEvent(nsINode* aNode) :
  AccEvent(::nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED, aNode, PR_TRUE), 
  mCaretOffset(-1)
{
}

already_AddRefed<nsAccEvent>
AccCaretMoveEvent::CreateXPCOMObject()
{
  nsAccEvent* event = new nsAccCaretMoveEvent(this);
  NS_IF_ADDREF(event);
  return event;
}






AccTableChangeEvent::
  AccTableChangeEvent(nsAccessible* aAccessible, PRUint32 aEventType,
                      PRInt32 aRowOrColIndex, PRInt32 aNumRowsOrCols,
                      PRBool aIsAsynch) :
  AccEvent(aEventType, aAccessible, aIsAsynch),
  mRowOrColIndex(aRowOrColIndex), mNumRowsOrCols(aNumRowsOrCols)
{
}

already_AddRefed<nsAccEvent>
AccTableChangeEvent::CreateXPCOMObject()
{
  nsAccEvent* event = new nsAccTableChangeEvent(this);
  NS_IF_ADDREF(event);
  return event;
}

