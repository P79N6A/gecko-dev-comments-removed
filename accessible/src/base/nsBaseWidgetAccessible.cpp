





































#include "nsBaseWidgetAccessible.h"
#include "nsAccessibilityAtoms.h"
#include "nsIAccessibilityService.h"
#include "nsIAccessibleDocument.h"
#include "nsAccessibleWrap.h"
#include "nsAccessibilityUtils.h"
#include "nsGUIEvent.h"
#include "nsHyperTextAccessibleWrap.h"
#include "nsILink.h"
#include "nsIFrame.h"
#include "nsINameSpaceManager.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"
#include "nsIURI.h"





nsLeafAccessible::nsLeafAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsAccessibleWrap(aNode, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsLeafAccessible, nsAccessible)


NS_IMETHODIMP nsLeafAccessible::GetFirstChild(nsIAccessible **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}


NS_IMETHODIMP nsLeafAccessible::GetLastChild(nsIAccessible **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}


NS_IMETHODIMP nsLeafAccessible::GetChildCount(PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
}


NS_IMETHODIMP
nsLeafAccessible::GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildren)
{
  *aAllowsAnonChildren = PR_FALSE;
  return NS_OK;
}





nsLinkableAccessible::nsLinkableAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell) :
  nsHyperTextAccessibleWrap(aNode, aShell),
  mActionContent(nsnull),
  mIsLink(PR_FALSE),
  mIsOnclick(PR_FALSE)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsLinkableAccessible, nsHyperTextAccessibleWrap)

NS_IMETHODIMP nsLinkableAccessible::TakeFocus()
{ 
  if (mActionContent && mActionContent->IsFocusable()) {
    mActionContent->SetFocus(nsCOMPtr<nsPresContext>(GetPresContext()));
  }
  
  return NS_OK;
}


NS_IMETHODIMP
nsLinkableAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mIsLink) {
    *aState |= nsIAccessibleStates::STATE_LINKED;
    nsCOMPtr<nsILink> link = do_QueryInterface(mActionContent);
    if (link) {
      nsLinkState linkState;
      link->GetLinkState(linkState);
      if (linkState == eLinkState_Visited) {
        *aState |= nsIAccessibleStates::STATE_TRAVERSED;
      }
    }
    
    PRUint32 role;
    GetRole(&role);
    if (role != nsIAccessibleRole::ROLE_LINK) {
      nsCOMPtr<nsIAccessible> parentAccessible(GetParent());
      if (parentAccessible) {
        PRUint32 orState = State(parentAccessible);
        *aState |= orState;
      }
    }
  }
  if (mActionContent && !mActionContent->IsFocusable()) {
    
    *aState &= ~nsIAccessibleStates::STATE_FOCUSABLE;
  }

  
  
  nsCOMPtr<nsIAccessible> docAccessible =
    do_QueryInterface(nsCOMPtr<nsIAccessibleDocument>(GetDocAccessible()));
  if (docAccessible) {
    PRUint32 docState = 0, docExtraState = 0;
    rv = docAccessible->GetFinalState(&docState, &docExtraState);
    if (NS_SUCCEEDED(rv) &&
        (docExtraState & nsIAccessibleStates::EXT_STATE_EDITABLE)) {
      
      *aState &= ~(nsIAccessibleStates::STATE_FOCUSED |
                   nsIAccessibleStates::STATE_FOCUSABLE);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsLinkableAccessible::GetValue(nsAString& _retval)
{
  if (mIsLink) {
    nsCOMPtr<nsIDOMNode> linkNode(do_QueryInterface(mActionContent));
    nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));
    if (linkNode && presShell)
      return presShell->GetLinkLocation(linkNode, _retval);
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP nsLinkableAccessible::GetNumActions(PRUint8 *aNumActions)
{
  *aNumActions = mActionContent ? 1 : 0;
  return NS_OK;
}


NS_IMETHODIMP nsLinkableAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  
  aName.Truncate();
  if (aIndex == eAction_Jump) {   
    if (mIsLink) {
      aName.AssignLiteral("jump");
      return NS_OK;
    }
    else if (mIsOnclick) {
      aName.AssignLiteral("click");
      return NS_OK;
    }
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  return NS_ERROR_INVALID_ARG;
}


NS_IMETHODIMP nsLinkableAccessible::DoAction(PRUint8 index)
{
  
  if (index == eAction_Jump) {
    if (mActionContent) {
      return DoCommand(mActionContent);
    }
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP nsLinkableAccessible::GetKeyboardShortcut(nsAString& aKeyboardShortcut)
{
  if (mActionContent) {
    nsCOMPtr<nsIDOMNode> actionNode(do_QueryInterface(mActionContent));
    if (actionNode && mDOMNode != actionNode) {
      nsCOMPtr<nsIAccessible> accessible;
      nsCOMPtr<nsIAccessibilityService> accService = 
        do_GetService("@mozilla.org/accessibilityService;1");
      accService->GetAccessibleInWeakShell(actionNode, mWeakShell,
                                           getter_AddRefs(accessible));
      if (accessible) {
        accessible->GetKeyboardShortcut(aKeyboardShortcut);
      }
      return NS_OK;
    }
  }
  return nsAccessible::GetKeyboardShortcut(aKeyboardShortcut);
}

void nsLinkableAccessible::CacheActionContent()
{
  for (nsCOMPtr<nsIContent> walkUpContent(do_QueryInterface(mDOMNode));
       walkUpContent;
       walkUpContent = walkUpContent->GetParent()) {
    nsIAtom *tag = walkUpContent->Tag();
    if ((tag == nsAccessibilityAtoms::a || tag == nsAccessibilityAtoms::area) &&
        walkUpContent->IsNodeOfType(nsINode::eHTML)) {
      nsCOMPtr<nsILink> link = do_QueryInterface(walkUpContent);
      if (link) {
        
        
        
        nsCOMPtr<nsIURI> uri;
        link->GetHrefURI(getter_AddRefs(uri));
        if (uri) {
          mActionContent = walkUpContent;
          mIsLink = PR_TRUE;
          break;
        }
      }
    }
    if (nsAccessibilityUtils::HasListener(walkUpContent, NS_LITERAL_STRING("click"))) {
      mActionContent = walkUpContent;
      mIsOnclick = PR_TRUE;
      break;
    }
  }
}


NS_IMETHODIMP nsLinkableAccessible::GetURI(PRInt32 aIndex, nsIURI **aURI)
{
  
  *aURI = nsnull;
  if (aIndex != 0 || !mIsLink || !SameCOMIdentity(mDOMNode, mActionContent)) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsILink> link(do_QueryInterface(mActionContent));
  if (link) {
    return link->GetHrefURI(aURI);
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsLinkableAccessible::Init()
{
  CacheActionContent();
  return nsHyperTextAccessibleWrap::Init();
}

NS_IMETHODIMP nsLinkableAccessible::Shutdown()
{
  mActionContent = nsnull;
  return nsHyperTextAccessibleWrap::Shutdown();
}





nsEnumRoleAccessible::nsEnumRoleAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell, PRUint32 aRole) :
  nsAccessibleWrap(aNode, aShell),
  mRole(aRole)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsEnumRoleAccessible, nsAccessible)
