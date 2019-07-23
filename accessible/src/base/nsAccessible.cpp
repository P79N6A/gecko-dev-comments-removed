






































#include "nsAccessible.h"
#include "nsAccessibleRelation.h"
#include "nsHyperTextAccessibleWrap.h"

#include "nsIAccessibleDocument.h"
#include "nsIAccessibleHyperText.h"
#include "nsAccessibleTreeWalker.h"

#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentXBL.h"
#include "nsIDOMDocumentTraversal.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMNodeFilter.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsIDOMTreeWalker.h"
#include "nsIDOMXULButtonElement.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMXULLabelElement.h"
#include "nsIDOMXULSelectCntrlEl.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsPIDOMWindow.h"

#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsIForm.h"
#include "nsIFormControl.h"

#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIFrame.h"
#include "nsIViewManager.h"
#include "nsIDocShellTreeItem.h"
#include "nsIScrollableFrame.h"

#include "nsXPIDLString.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "prdtoa.h"
#include "nsIAtom.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIURI.h"
#include "nsITimer.h"
#include "nsIMutableArray.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsWhitespaceTokenizer.h"
#include "nsAttrName.h"
#include "nsNetUtil.h"

#ifdef NS_DEBUG
#include "nsIFrameDebug.h"
#include "nsIDOMCharacterData.h"
#endif




nsAccessibleDOMStringList::nsAccessibleDOMStringList()
{
}

nsAccessibleDOMStringList::~nsAccessibleDOMStringList()
{
}

NS_IMPL_ISUPPORTS1(nsAccessibleDOMStringList, nsIDOMDOMStringList)

NS_IMETHODIMP
nsAccessibleDOMStringList::Item(PRUint32 aIndex, nsAString& aResult)
{
  if (aIndex >= (PRUint32)mNames.Count()) {
    SetDOMStringToNull(aResult);
  } else {
    mNames.StringAt(aIndex, aResult);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsAccessibleDOMStringList::GetLength(PRUint32 *aLength)
{
  *aLength = (PRUint32)mNames.Count();

  return NS_OK;
}

NS_IMETHODIMP
nsAccessibleDOMStringList::Contains(const nsAString& aString, PRBool *aResult)
{
  *aResult = mNames.IndexOf(aString) > -1;

  return NS_OK;
}








NS_IMPL_CYCLE_COLLECTION_CLASS(nsAccessible)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsAccessible, nsAccessNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mParent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFirstChild)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mNextSibling)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsAccessible, nsAccessNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mParent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFirstChild)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mNextSibling)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(nsAccessible, nsAccessNode)
NS_IMPL_RELEASE_INHERITED(nsAccessible, nsAccessNode)

#ifdef DEBUG_A11Y





PRBool nsAccessible::IsTextInterfaceSupportCorrect(nsIAccessible *aAccessible)
{
  PRBool foundText = PR_FALSE;

  nsCOMPtr<nsIAccessibleDocument> accDoc = do_QueryInterface(aAccessible);
  if (accDoc) {
    
    
    return PR_TRUE;
  }
  nsCOMPtr<nsIAccessible> child, nextSibling;
  aAccessible->GetFirstChild(getter_AddRefs(child));
  while (child) {
    if (IsText(child)) {
      foundText = PR_TRUE;
      break;
    }
    child->GetNextSibling(getter_AddRefs(nextSibling));
    child.swap(nextSibling);
  }
  if (foundText) {
    
    nsCOMPtr<nsIAccessibleText> text = do_QueryInterface(aAccessible);
    if (!text) {
      return PR_FALSE;
    }
  }
  return PR_TRUE; 
}
#endif

nsresult nsAccessible::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  
  
  *aInstancePtr = nsnull;

  if (aIID.Equals(NS_GET_IID(nsXPCOMCycleCollectionParticipant))) {
    *aInstancePtr = &NS_CYCLE_COLLECTION_NAME(nsAccessible);
    return NS_OK;
  }

  if (aIID.Equals(NS_GET_IID(nsIAccessible))) {
    *aInstancePtr = static_cast<nsIAccessible*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if(aIID.Equals(NS_GET_IID(nsPIAccessible))) {
    *aInstancePtr = static_cast<nsPIAccessible*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if (aIID.Equals(NS_GET_IID(nsAccessible))) {
    *aInstancePtr = static_cast<nsAccessible*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if (aIID.Equals(NS_GET_IID(nsIAccessibleSelectable))) {
    nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
    if (!content) {
      return NS_ERROR_FAILURE; 
    }
    if (content->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::role)) {
      
      
      
      
      
      nsAutoString multiselectable;
      if (content->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::aria_multiselectable,
                               nsAccessibilityAtoms::_true, eCaseMatters)) {
        *aInstancePtr = static_cast<nsIAccessibleSelectable*>(this);
        NS_ADDREF_THIS();
        return NS_OK;
      }
    }
  }

  if (aIID.Equals(NS_GET_IID(nsIAccessibleValue))) {
    if (mRoleMapEntry && mRoleMapEntry->valueRule != eNoValue) {
      *aInstancePtr = static_cast<nsIAccessibleValue*>(this);
      NS_ADDREF_THIS();
      return NS_OK;
    }
  }                       

  if (aIID.Equals(NS_GET_IID(nsIAccessibleHyperLink))) {
    nsCOMPtr<nsIAccessible> parent(GetParent());
    nsCOMPtr<nsIAccessibleHyperText> hyperTextParent(do_QueryInterface(parent));
    if (hyperTextParent) {
      *aInstancePtr = static_cast<nsIAccessibleHyperLink*>(this);
      NS_ADDREF_THIS();
      return NS_OK;
    }
    return NS_ERROR_NO_INTERFACE;
  }

  return nsAccessNodeWrap::QueryInterface(aIID, aInstancePtr);
}

nsAccessible::nsAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell): nsAccessNodeWrap(aNode, aShell), 
  mParent(nsnull), mFirstChild(nsnull), mNextSibling(nsnull), mRoleMapEntry(nsnull),
  mAccChildCount(eChildCountUninitialized)
{
#ifdef NS_DEBUG_X
   {
     nsCOMPtr<nsIPresShell> shell(do_QueryReferent(aShell));
     printf(">>> %p Created Acc - DOM: %p  PS: %p", 
            (void*)static_cast<nsIAccessible*>(this), (void*)aNode,
            (void*)shell.get());
    nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
    if (content) {
      nsAutoString buf;
      if (content->NodeInfo())
        content->NodeInfo()->GetQualifiedName(buf);
      printf(" Con: %s@%p", NS_ConvertUTF16toUTF8(buf).get(), (void *)content.get());
      if (NS_SUCCEEDED(GetName(buf))) {
        printf(" Name:[%s]", NS_ConvertUTF16toUTF8(buf).get());
       }
     }
     printf("\n");
   }
#endif
}




nsAccessible::~nsAccessible()
{
}

NS_IMETHODIMP nsAccessible::SetRoleMapEntry(nsRoleMapEntry* aRoleMapEntry)
{
  mRoleMapEntry = aRoleMapEntry;
  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GetName(nsAString& aName)
{
  aName.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  GetARIAName(aName);
  if (!aName.IsEmpty())
    return NS_OK;

  return GetNameInternal(aName);
}

NS_IMETHODIMP nsAccessible::GetDescription(nsAString& aDescription)
{
  
  
  
  
  
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (!content) {
    return NS_ERROR_FAILURE;  
  }
  if (!content->IsNodeOfType(nsINode::eTEXT)) {
    nsAutoString description;
    nsresult rv = GetTextFromRelationID(nsAccessibilityAtoms::aria_describedby, description);
    if (NS_FAILED(rv)) {
      PRBool isXUL = content->IsNodeOfType(nsINode::eXUL);
      if (isXUL) {
        
        nsIContent *descriptionContent =
          nsAccUtils::FindNeighbourPointingToNode(content, nsAccessibilityAtoms::control,
                                                  nsAccessibilityAtoms::description);

        if (descriptionContent) {
          
          AppendFlatStringFromSubtree(descriptionContent, &description);
        }
      }
      if (description.IsEmpty()) {
        nsIAtom *descAtom = isXUL ? nsAccessibilityAtoms::tooltiptext :
                                    nsAccessibilityAtoms::title;
        if (content->GetAttr(kNameSpaceID_None, descAtom, description)) {
          nsAutoString name;
          GetName(name);
          if (name.IsEmpty() || description == name) {
            
            
            description.Truncate();
          }
        }
      }
    }
    description.CompressWhitespace();
    aDescription = description;
  }

  return NS_OK;
}


#define NS_MODIFIER_SHIFT    1
#define NS_MODIFIER_CONTROL  2
#define NS_MODIFIER_ALT      4
#define NS_MODIFIER_META     8



static PRInt32
GetAccessModifierMask(nsIContent* aContent)
{
  nsCOMPtr<nsIPrefBranch> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (!prefBranch)
    return 0;

  
  PRInt32 accessKey;
  nsresult rv = prefBranch->GetIntPref("ui.key.generalAccessKey", &accessKey);
  if (NS_SUCCEEDED(rv) && accessKey != -1) {
    switch (accessKey) {
      case nsIDOMKeyEvent::DOM_VK_SHIFT:   return NS_MODIFIER_SHIFT;
      case nsIDOMKeyEvent::DOM_VK_CONTROL: return NS_MODIFIER_CONTROL;
      case nsIDOMKeyEvent::DOM_VK_ALT:     return NS_MODIFIER_ALT;
      case nsIDOMKeyEvent::DOM_VK_META:    return NS_MODIFIER_META;
      default:                             return 0;
    }
  }

  
  nsCOMPtr<nsIDocument> document = aContent->GetCurrentDoc();
  if (!document)
    return 0;
  nsCOMPtr<nsISupports> container = document->GetContainer();
  if (!container)
    return 0;
  nsCOMPtr<nsIDocShellTreeItem> treeItem(do_QueryInterface(container));
  if (!treeItem)
    return 0;

  
  PRInt32 itemType, accessModifierMask = 0;
  treeItem->GetItemType(&itemType);
  switch (itemType) {

  case nsIDocShellTreeItem::typeChrome:
    rv = prefBranch->GetIntPref("ui.key.chromeAccess", &accessModifierMask);
    break;

  case nsIDocShellTreeItem::typeContent:
    rv = prefBranch->GetIntPref("ui.key.contentAccess", &accessModifierMask);
    break;
  }

  return NS_SUCCEEDED(rv) ? accessModifierMask : 0;
}

NS_IMETHODIMP
nsAccessible::GetKeyboardShortcut(nsAString& aAccessKey)
{
  aAccessKey.Truncate();

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (!content)
    return NS_ERROR_FAILURE;

  PRUint32 key = nsAccUtils::GetAccessKeyFor(content);
  if (!key && content->IsNodeOfType(nsIContent::eELEMENT)) {
    
    
    nsCOMPtr<nsIContent> labelContent(GetLabelContent(content));
    nsCOMPtr<nsIDOMNode> labelNode = do_QueryInterface(labelContent);
    if (labelNode && !nsAccUtils::IsAncestorOf(labelNode, mDOMNode))
      key = nsAccUtils::GetAccessKeyFor(labelContent);
  }

  if (!key)
    return NS_OK;

  nsAutoString accesskey(key);

  
  nsAutoString propertyKey;
  PRInt32 modifierMask = GetAccessModifierMask(content);
  if (modifierMask & NS_MODIFIER_META) {
    propertyKey.AssignLiteral("VK_META");
    nsAccessible::GetFullKeyName(propertyKey, accesskey, accesskey);
  }
  if (modifierMask & NS_MODIFIER_SHIFT) {
    propertyKey.AssignLiteral("VK_SHIFT");
    nsAccessible::GetFullKeyName(propertyKey, accesskey, accesskey);
  }
  if (modifierMask & NS_MODIFIER_ALT) {
    propertyKey.AssignLiteral("VK_ALT");
    nsAccessible::GetFullKeyName(propertyKey, accesskey, accesskey);
  }
  if (modifierMask & NS_MODIFIER_CONTROL) {
    propertyKey.AssignLiteral("VK_CONTROL");
    nsAccessible::GetFullKeyName(propertyKey, accesskey, accesskey);
  }

  aAccessKey = accesskey;
  return NS_OK;
}

NS_IMETHODIMP nsAccessible::SetParent(nsIAccessible *aParent)
{
  if (mParent != aParent) {
    
    
    
    
    nsCOMPtr<nsPIAccessible> privOldParent = do_QueryInterface(mParent);
    if (privOldParent) {
      privOldParent->InvalidateChildren();
    }
  }

  mParent = aParent;
  return NS_OK;
}

NS_IMETHODIMP nsAccessible::SetFirstChild(nsIAccessible *aFirstChild)
{
  mFirstChild = aFirstChild;

  return NS_OK;
}

NS_IMETHODIMP nsAccessible::SetNextSibling(nsIAccessible *aNextSibling)
{
  mNextSibling = aNextSibling;
  return NS_OK;
}

nsIContent *nsAccessible::GetRoleContent(nsIDOMNode *aDOMNode)
{
  
  
  
  
  
  nsCOMPtr<nsIContent> content(do_QueryInterface(aDOMNode));
  if (!content) {
    nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(aDOMNode));
    if (domDoc) {
      nsCOMPtr<nsIDOMHTMLDocument> htmlDoc(do_QueryInterface(aDOMNode));
      if (htmlDoc) {
        nsCOMPtr<nsIDOMHTMLElement> bodyElement;
        htmlDoc->GetBody(getter_AddRefs(bodyElement));
        content = do_QueryInterface(bodyElement);
      }
      else {
        nsCOMPtr<nsIDOMElement> docElement;
        domDoc->GetDocumentElement(getter_AddRefs(docElement));
        content = do_QueryInterface(docElement);
      }
    }
  }
  return content;
}

NS_IMETHODIMP nsAccessible::Shutdown()
{
  mNextSibling = nsnull;

  
  
  InvalidateChildren();
  if (mParent) {
    nsCOMPtr<nsPIAccessible> privateParent(do_QueryInterface(mParent));
    privateParent->InvalidateChildren();
    mParent = nsnull;
  }

  return nsAccessNodeWrap::Shutdown();
}

NS_IMETHODIMP nsAccessible::InvalidateChildren()
{
  

  
  
  
  
  nsAccessible* child = static_cast<nsAccessible*>(mFirstChild.get());
  while (child) {
    child->mParent = nsnull;

    nsCOMPtr<nsIAccessible> next = child->mNextSibling;
    child->mNextSibling = nsnull;
    child = static_cast<nsAccessible*>(next.get());
  }

  mAccChildCount = eChildCountUninitialized;
  mFirstChild = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsAccessible::GetParent(nsIAccessible **  aParent)
{
  nsresult rv = GetCachedParent(aParent);
  if (NS_FAILED(rv) || *aParent) {
    return rv;
  }

  nsCOMPtr<nsIAccessibleDocument> docAccessible(GetDocAccessible());
  NS_ENSURE_TRUE(docAccessible, NS_ERROR_FAILURE);

  return docAccessible->GetAccessibleInParentChain(mDOMNode, PR_TRUE, aParent);
}

NS_IMETHODIMP nsAccessible::GetCachedParent(nsIAccessible **  aParent)
{
  *aParent = nsnull;
  if (!mWeakShell) {
    
    return NS_ERROR_FAILURE;
  }
  NS_IF_ADDREF(*aParent = mParent);
  return NS_OK;
}

NS_IMETHODIMP nsAccessible::GetCachedFirstChild(nsIAccessible **  aFirstChild)
{
  *aFirstChild = nsnull;
  if (!mWeakShell) {
    
    return NS_ERROR_FAILURE;
  }
  NS_IF_ADDREF(*aFirstChild = mFirstChild);
  return NS_OK;
}

  
NS_IMETHODIMP nsAccessible::GetNextSibling(nsIAccessible * *aNextSibling) 
{ 
  *aNextSibling = nsnull; 
  if (!mWeakShell) {
    
    return NS_ERROR_FAILURE;
  }
  if (!mParent) {
    nsCOMPtr<nsIAccessible> parent(GetParent());
    if (parent) {
      PRInt32 numChildren;
      parent->GetChildCount(&numChildren);  
    }
  }

  if (mNextSibling || !mParent) {
    
    
    NS_IF_ADDREF(*aNextSibling = mNextSibling);

    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

  
NS_IMETHODIMP nsAccessible::GetPreviousSibling(nsIAccessible * *aPreviousSibling) 
{
  *aPreviousSibling = nsnull;

  if (!mWeakShell) {
    
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIAccessible> parent;
  if (NS_FAILED(GetParent(getter_AddRefs(parent))) || !parent) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIAccessible> testAccessible, prevSibling;
  parent->GetFirstChild(getter_AddRefs(testAccessible));
  while (testAccessible && this != testAccessible) {
    prevSibling = testAccessible;
    prevSibling->GetNextSibling(getter_AddRefs(testAccessible));
  }

  if (!prevSibling) {
    return NS_ERROR_FAILURE;
  }

  NS_ADDREF(*aPreviousSibling = prevSibling);
  return NS_OK;
}

  
NS_IMETHODIMP nsAccessible::GetFirstChild(nsIAccessible * *aFirstChild) 
{  
  if (gIsCacheDisabled) {
    InvalidateChildren();
  }
  PRInt32 numChildren;
  GetChildCount(&numChildren);  

#ifdef DEBUG
  nsCOMPtr<nsPIAccessible> firstChild(do_QueryInterface(mFirstChild));
  if (firstChild) {
    nsCOMPtr<nsIAccessible> realParent;
    firstChild->GetCachedParent(getter_AddRefs(realParent));
    NS_ASSERTION(!realParent || realParent == this,
                 "Two accessibles have the same first child accessible.");
  }
#endif

  NS_IF_ADDREF(*aFirstChild = mFirstChild);

  return NS_OK;  
}

  
NS_IMETHODIMP nsAccessible::GetLastChild(nsIAccessible * *aLastChild)
{  
  GetChildAt(-1, aLastChild);
  return NS_OK;
}

NS_IMETHODIMP nsAccessible::GetChildAt(PRInt32 aChildNum, nsIAccessible **aChild)
{
  

  PRInt32 numChildren;
  GetChildCount(&numChildren);

  
  if (aChildNum >= numChildren || numChildren == 0 || !mWeakShell) {
    *aChild = nsnull;
    return NS_ERROR_FAILURE;
  
  } else if (aChildNum < 0) {
    aChildNum = numChildren - 1;
  }

  nsCOMPtr<nsIAccessible> current(mFirstChild), nextSibling;
  PRInt32 index = 0;

  while (current) {
    nextSibling = current;
    if (++index > aChildNum) {
      break;
    }
    nextSibling->GetNextSibling(getter_AddRefs(current));
  }

  NS_IF_ADDREF(*aChild = nextSibling);

  return NS_OK;
}


NS_IMETHODIMP nsAccessible::GetChildren(nsIArray **aOutChildren)
{
  *aOutChildren = nsnull;
  nsCOMPtr<nsIMutableArray> children = do_CreateInstance(NS_ARRAY_CONTRACTID);
  if (!children)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIAccessible> curChild;
  while (NextChild(curChild)) {
    children->AppendElement(curChild, PR_FALSE);
  }
  
  NS_ADDREF(*aOutChildren = children);
  return NS_OK;
}

nsIAccessible *nsAccessible::NextChild(nsCOMPtr<nsIAccessible>& aAccessible)
{
  nsCOMPtr<nsIAccessible> nextChild;
  if (!aAccessible) {
    GetFirstChild(getter_AddRefs(nextChild));
  }
  else {
    aAccessible->GetNextSibling(getter_AddRefs(nextChild));
  }
  return (aAccessible = nextChild);
}

void nsAccessible::CacheChildren()
{
  if (!mWeakShell) {
    
    mAccChildCount = eChildCountUninitialized;
    return;
  }

  if (mAccChildCount == eChildCountUninitialized) {
    mAccChildCount = 0;
    PRBool allowsAnonChildren = PR_FALSE;
    GetAllowsAnonChildAccessibles(&allowsAnonChildren);
    nsAccessibleTreeWalker walker(mWeakShell, mDOMNode, allowsAnonChildren);
    
    
    
    walker.mState.frame = GetFrame();

    nsCOMPtr<nsPIAccessible> privatePrevAccessible;
    PRInt32 childCount = 0;
    walker.GetFirstChild();
    SetFirstChild(walker.mState.accessible);

    while (walker.mState.accessible) {
      ++ childCount;
      privatePrevAccessible = do_QueryInterface(walker.mState.accessible);
      privatePrevAccessible->SetParent(this);
      walker.GetNextSibling();
      privatePrevAccessible->SetNextSibling(walker.mState.accessible);
    }
    mAccChildCount = childCount;
  }
}

NS_IMETHODIMP nsAccessible::GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildren)
{
  *aAllowsAnonChildren = PR_TRUE;
  return NS_OK;
}


NS_IMETHODIMP nsAccessible::GetChildCount(PRInt32 *aAccChildCount) 
{
  CacheChildren();
  *aAccChildCount = mAccChildCount;
  return NS_OK;  
}


NS_IMETHODIMP nsAccessible::GetIndexInParent(PRInt32 *aIndexInParent)
{
  *aIndexInParent = -1;
  if (!mWeakShell) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIAccessible> parent;
  GetParent(getter_AddRefs(parent));
  if (!parent) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIAccessible> sibling;
  parent->GetFirstChild(getter_AddRefs(sibling));
  if (!sibling) {
    return NS_ERROR_FAILURE;
  }

  *aIndexInParent = 0;
  while (sibling != this) {
    NS_ASSERTION(sibling, "Never ran into the same child that we started from");

    if (!sibling)
      return NS_ERROR_FAILURE;

    ++*aIndexInParent;
    nsCOMPtr<nsIAccessible> tempAccessible;
    sibling->GetNextSibling(getter_AddRefs(tempAccessible));
    sibling = tempAccessible;
  }

  return NS_OK;
}

NS_IMETHODIMP nsAccessible::TestChildCache(nsIAccessible *aCachedChild)
{
#ifndef DEBUG_A11Y
  return NS_OK;
#else
  
  
  
  
  if (mAccChildCount <= 0) {
    return NS_OK;
  }
  nsCOMPtr<nsIAccessible> sibling = mFirstChild;

  while (sibling != aCachedChild) {
    NS_ASSERTION(sibling, "[TestChildCache] Never ran into the same child that we started from");
    if (!sibling)
      return NS_ERROR_FAILURE;

    nsCOMPtr<nsIAccessible> tempAccessible;
    sibling->GetNextSibling(getter_AddRefs(tempAccessible));
    sibling = tempAccessible;
  }
  return NS_OK;
#endif
}

nsresult nsAccessible::GetTranslatedString(const nsAString& aKey, nsAString& aStringOut)
{
  nsXPIDLString xsValue;

  if (!gStringBundle || 
    NS_FAILED(gStringBundle->GetStringFromName(PromiseFlatString(aKey).get(), getter_Copies(xsValue)))) 
    return NS_ERROR_FAILURE;

  aStringOut.Assign(xsValue);
  return NS_OK;
}

nsresult nsAccessible::GetFullKeyName(const nsAString& aModifierName, const nsAString& aKeyName, nsAString& aStringOut)
{
  nsXPIDLString modifierName, separator;

  if (!gKeyStringBundle ||
      NS_FAILED(gKeyStringBundle->GetStringFromName(PromiseFlatString(aModifierName).get(), 
                                                    getter_Copies(modifierName))) ||
      NS_FAILED(gKeyStringBundle->GetStringFromName(PromiseFlatString(NS_LITERAL_STRING("MODIFIER_SEPARATOR")).get(), 
                                                    getter_Copies(separator)))) {
    return NS_ERROR_FAILURE;
  }

  aStringOut = modifierName + separator + aKeyName; 
  return NS_OK;
}

PRBool nsAccessible::IsVisible(PRBool *aIsOffscreen) 
{
  
  
  
  
  *aIsOffscreen = PR_TRUE;
  if (!mDOMNode) {
    return PR_FALSE; 
  }

  const PRUint16 kMinPixels  = 12;
   
  nsCOMPtr<nsIPresShell> shell(GetPresShell());
  if (!shell) 
    return PR_FALSE;

  nsIViewManager* viewManager = shell->GetViewManager();
  if (!viewManager)
    return PR_FALSE;

  nsIFrame *frame = GetFrame();
  if (!frame) {
    return PR_FALSE;
  }

  
  if (!frame->GetStyleVisibility()->IsVisible())
  {
      return PR_FALSE;
  }

  nsPresContext *presContext = shell->GetPresContext();
  if (!presContext)
    return PR_FALSE;

  
  
  
  

  nsRect relFrameRect = frame->GetRect();
  nsIView *containingView = frame->GetViewExternal();
  if (containingView) {
    
    relFrameRect.x = relFrameRect.y = 0;
  }
  else {
    nsPoint frameOffset;
    frame->GetOffsetFromView(frameOffset, &containingView);
    if (!containingView)
      return PR_FALSE;  
    relFrameRect.x = frameOffset.x;
    relFrameRect.y = frameOffset.y;
  }

  nsRectVisibility rectVisibility;
  viewManager->GetRectVisibility(containingView, relFrameRect,
                                 nsPresContext::CSSPixelsToAppUnits(kMinPixels),
                                 &rectVisibility);

  if (rectVisibility == nsRectVisibility_kZeroAreaRect) {
    nsIAtom *frameType = frame->GetType();
    if (frameType == nsAccessibilityAtoms::textFrame) {
      
      
      nsAutoString renderedText;
      frame->GetRenderedText (&renderedText, nsnull, nsnull, 0, 1);
      if (!renderedText.IsEmpty()) {
        rectVisibility = nsRectVisibility_kVisible;
      }
    }
    else if (frameType == nsAccessibilityAtoms::inlineFrame) {
      
      
      
      PRInt32 x, y, width, height;
      GetBounds(&x, &y, &width, &height);
      if (width > 0 && height > 0) {
        rectVisibility = nsRectVisibility_kVisible;    
      }
    }
  }

  if (rectVisibility == nsRectVisibility_kZeroAreaRect && frame && 
      0 == (frame->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
    
    
    return PR_FALSE;
  }
  
  
  
  
  
  
  
  
  nsCOMPtr<nsIDOMDocument> domDoc;
  mDOMNode->GetOwnerDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc(do_QueryInterface(domDoc));
  if (!doc)  {
    return PR_FALSE;
  }

  PRBool isVisible = CheckVisibilityInParentChain(doc, containingView);
  if (isVisible && rectVisibility == nsRectVisibility_kVisible) {
    *aIsOffscreen = PR_FALSE;
  }
  return isVisible;
}

nsresult
nsAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  *aState = 0;

  if (!mDOMNode) {
    if (aExtraState) {
      *aExtraState = nsIAccessibleStates::EXT_STATE_DEFUNCT;
    }
    return NS_OK; 
  }

  if (aExtraState)
    *aExtraState = 0;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (!content) {
    return NS_OK;  
  }

  
  
  
  
  PRBool isDisabled;
  if (content->IsNodeOfType(nsINode::eHTML)) {
    
    
    isDisabled = content->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::disabled);
  }
  else {
    isDisabled = content->AttrValueIs(kNameSpaceID_None,
                                      nsAccessibilityAtoms::disabled,
                                      nsAccessibilityAtoms::_true,
                                      eCaseMatters);
  }
  if (isDisabled) {
    *aState |= nsIAccessibleStates::STATE_UNAVAILABLE;
  }
  else if (content->IsNodeOfType(nsINode::eELEMENT)) {
    nsIFrame *frame = GetFrame();
    if (frame && frame->IsFocusable()) {
      *aState |= nsIAccessibleStates::STATE_FOCUSABLE;
    }

    if (gLastFocusedNode == mDOMNode) {
      *aState |= nsIAccessibleStates::STATE_FOCUSED;
    }
  }

  
  
  PRBool isOffscreen;
  if (!IsVisible(&isOffscreen)) {
    *aState |= nsIAccessibleStates::STATE_INVISIBLE;
  }
  if (isOffscreen) {
    *aState |= nsIAccessibleStates::STATE_OFFSCREEN;
  }

  nsIFrame *frame = GetFrame();
  if (frame && (frame->GetStateBits() & NS_FRAME_OUT_OF_FLOW))
    *aState |= nsIAccessibleStates::STATE_FLOATING;

  
  if (nsAccUtils::IsXLink(content))
    *aState |= nsIAccessibleStates::STATE_LINKED;

  return NS_OK;
}

  
NS_IMETHODIMP nsAccessible::GetFocusedChild(nsIAccessible **aFocusedChild) 
{ 
  nsCOMPtr<nsIAccessible> focusedChild;
  if (gLastFocusedNode == mDOMNode) {
    focusedChild = this;
  }
  else if (gLastFocusedNode) {
    nsCOMPtr<nsIAccessibilityService> accService =
      do_GetService("@mozilla.org/accessibilityService;1");
    NS_ENSURE_TRUE(accService, NS_ERROR_FAILURE);

    accService->GetAccessibleFor(gLastFocusedNode,
                                 getter_AddRefs(focusedChild));
    if (focusedChild) {
      nsCOMPtr<nsIAccessible> focusedParentAccessible;
      focusedChild->GetParent(getter_AddRefs(focusedParentAccessible));
      if (focusedParentAccessible != this) {
        focusedChild = nsnull;
      }
    }
  }

  NS_IF_ADDREF(*aFocusedChild = focusedChild);
  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetDeepestChildAtPoint(PRInt32 aX, PRInt32 aY,
                                     nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;

  if (!mDOMNode) {
    return NS_ERROR_FAILURE;  
  }

  
  
  nsCOMPtr<nsIAccessible> fallbackAnswer;
  PRInt32 x, y, width, height;
  GetBounds(&x, &y, &width, &height);
  if (aX >= x && aX < x + width &&
      aY >= y && aY < y + height) {
    fallbackAnswer = this;
  }
  if (MustPrune(this)) {  
    NS_IF_ADDREF(*aAccessible = fallbackAnswer);
    return NS_OK;
  }

  
  
  
  
  
  
  nsCOMPtr<nsIAccessibleDocument> accDocument;
  nsresult rv = GetAccessibleDocument(getter_AddRefs(accDocument));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(accDocument, NS_ERROR_FAILURE);

  nsCOMPtr<nsPIAccessNode> accessNodeDocument(do_QueryInterface(accDocument));
  NS_ASSERTION(accessNodeDocument,
               "nsIAccessibleDocument doesn't implement nsPIAccessNode");

  nsIFrame *frame = accessNodeDocument->GetFrame();
  NS_ENSURE_STATE(frame);

  nsPresContext *presContext = frame->PresContext();

  nsIntRect screenRect = frame->GetScreenRectExternal();
  nsPoint offset(presContext->DevPixelsToAppUnits(aX - screenRect.x),
                 presContext->DevPixelsToAppUnits(aY - screenRect.y));

  nsCOMPtr<nsIPresShell> presShell = presContext->PresShell();
  nsIFrame *foundFrame = presShell->GetFrameForPoint(frame, offset);
  nsCOMPtr<nsIContent> content;
  if (!foundFrame || !(content = foundFrame->GetContent())) {
    NS_IF_ADDREF(*aAccessible = fallbackAnswer);
    return NS_OK;
  }

  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(content));
  nsCOMPtr<nsIAccessibilityService> accService = GetAccService();

  nsCOMPtr<nsIDOMNode> relevantNode;
  accService->GetRelevantContentNodeFor(node, getter_AddRefs(relevantNode));
  if (!relevantNode) {
    NS_IF_ADDREF(*aAccessible = fallbackAnswer);
    return NS_OK;
  }

  nsCOMPtr<nsIAccessible> accessible;
  accService->GetAccessibleFor(relevantNode, getter_AddRefs(accessible));
  if (!accessible) {
    
    
    accDocument->GetAccessibleInParentChain(relevantNode, PR_TRUE,
                                            getter_AddRefs(accessible));
    if (!accessible) {
      NS_IF_ADDREF(*aAccessible = fallbackAnswer);
      return NS_OK;
    }
  }

  if (accessible == this) {
    
    
    
    
    nsCOMPtr<nsIAccessible> child;
    while (NextChild(child)) {
      PRInt32 childX, childY, childWidth, childHeight;
      child->GetBounds(&childX, &childY, &childWidth, &childHeight);
      if (aX >= childX && aX < childX + childWidth &&
          aY >= childY && aY < childY + childHeight &&
          (State(child) & nsIAccessibleStates::STATE_INVISIBLE) == 0) {
        
        NS_IF_ADDREF(*aAccessible = child);
        return NS_OK;
      }
    }
    
    
  }

  NS_IF_ADDREF(*aAccessible = accessible);
  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                              nsIAccessible **aAccessible)
{
  nsresult rv = GetDeepestChildAtPoint(aX, aY, aAccessible);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!*aAccessible)
    return NS_OK;

  nsCOMPtr<nsIAccessible> parent, accessible(*aAccessible);
  while (PR_TRUE) {
    accessible->GetParent(getter_AddRefs(parent));
    if (!parent) {
      NS_ASSERTION(PR_FALSE,
                   "Obtained accessible isn't a child of this accessible.");
      
      

      
      
      PRInt32 x, y, width, height;
      GetBounds(&x, &y, &width, &height);
      if (aX >= x && aX < x + width && aY >= y && aY < y + height)
        NS_ADDREF(*aAccessible = this);

      return NS_OK;
    }

    if (parent == this) {
      
      NS_ADDREF(*aAccessible = accessible);
      return NS_OK;
    }
    accessible.swap(parent);
  }

  return NS_OK;
}

void nsAccessible::GetBoundsRect(nsRect& aTotalBounds, nsIFrame** aBoundingFrame)
{











  
  *aBoundingFrame = nsnull;
  nsIFrame *firstFrame = GetBoundsFrame();
  if (!firstFrame)
    return;

  
  
  
  nsIFrame *ancestorFrame = firstFrame;

  while (ancestorFrame) {  
    *aBoundingFrame = ancestorFrame;
    
    
    if (!IsCorrectFrameType(ancestorFrame, nsAccessibilityAtoms::inlineFrame) &&
        !IsCorrectFrameType(ancestorFrame, nsAccessibilityAtoms::textFrame))
      break;
    ancestorFrame = ancestorFrame->GetParent();
  }

  nsIFrame *iterFrame = firstFrame;
  nsCOMPtr<nsIContent> firstContent(do_QueryInterface(mDOMNode));
  nsIContent* iterContent = firstContent;
  PRInt32 depth = 0;

  
  while (iterContent == firstContent || depth > 0) {
    
    nsRect currFrameBounds = iterFrame->GetRect();
    
    
    currFrameBounds +=
      iterFrame->GetParent()->GetOffsetToExternal(*aBoundingFrame);

    
    aTotalBounds.UnionRect(aTotalBounds, currFrameBounds);

    nsIFrame *iterNextFrame = nsnull;

    if (IsCorrectFrameType(iterFrame, nsAccessibilityAtoms::inlineFrame)) {
      
      
      iterNextFrame = iterFrame->GetFirstChild(nsnull);
    }

    if (iterNextFrame) 
      ++depth;  
    else {  
      
      
      while (iterFrame) {
        iterNextFrame = iterFrame->GetNextContinuation();
        if (!iterNextFrame)
          iterNextFrame = iterFrame->GetNextSibling();
        if (iterNextFrame || --depth < 0) 
          break;
        iterFrame = iterFrame->GetParent();
      }
    }

    
    iterFrame = iterNextFrame;
    if (iterFrame == nsnull)
      break;
    iterContent = nsnull;
    if (depth == 0)
      iterContent = iterFrame->GetContent();
  }
}



NS_IMETHODIMP nsAccessible::GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height)
{
  
  
  
  
  

  nsPresContext *presContext = GetPresContext();
  if (!presContext)
  {
    *x = *y = *width = *height = 0;
    return NS_ERROR_FAILURE;
  }

  nsRect unionRectTwips;
  nsIFrame* aBoundingFrame = nsnull;
  GetBoundsRect(unionRectTwips, &aBoundingFrame);   
  if (!aBoundingFrame) {
    *x = *y = *width = *height = 0;
    return NS_ERROR_FAILURE;
  }

  *x      = presContext->AppUnitsToDevPixels(unionRectTwips.x); 
  *y      = presContext->AppUnitsToDevPixels(unionRectTwips.y);
  *width  = presContext->AppUnitsToDevPixels(unionRectTwips.width);
  *height = presContext->AppUnitsToDevPixels(unionRectTwips.height);

  

  nsRect orgRectPixels = aBoundingFrame->GetScreenRectExternal();
  *x += orgRectPixels.x;
  *y += orgRectPixels.y;

  return NS_OK;
}








PRBool nsAccessible::IsCorrectFrameType( nsIFrame* aFrame, nsIAtom* aAtom ) 
{
  NS_ASSERTION(aFrame != nsnull, "aFrame is null in call to IsCorrectFrameType!");
  NS_ASSERTION(aAtom != nsnull, "aAtom is null in call to IsCorrectFrameType!");

  return aFrame->GetType() == aAtom;
}


nsIFrame* nsAccessible::GetBoundsFrame()
{
  return GetFrame();
}

already_AddRefed<nsIAccessible>
nsAccessible::GetMultiSelectFor(nsIDOMNode *aNode)
{
  NS_ENSURE_TRUE(aNode, nsnull);
  nsCOMPtr<nsIAccessibilityService> accService =
    do_GetService("@mozilla.org/accessibilityService;1");
  NS_ENSURE_TRUE(accService, nsnull);
  nsCOMPtr<nsIAccessible> accessible;
  accService->GetAccessibleFor(aNode, getter_AddRefs(accessible));
  if (!accessible) {
    return nsnull;
  }

  PRUint32 state = State(accessible);
  if (0 == (state & nsIAccessibleStates::STATE_SELECTABLE)) {
    return nsnull;
  }

  PRUint32 containerRole;
  while (0 == (state & nsIAccessibleStates::STATE_MULTISELECTABLE)) {
    nsIAccessible *current = accessible;
    current->GetParent(getter_AddRefs(accessible));
    if (!accessible || (NS_SUCCEEDED(accessible->GetFinalRole(&containerRole)) &&
                        containerRole == nsIAccessibleRole::ROLE_PANE)) {
      return nsnull;
    }
    state = State(accessible);
  }
  nsIAccessible *returnAccessible = nsnull;
  accessible.swap(returnAccessible);
  return returnAccessible;
}


NS_IMETHODIMP nsAccessible::SetSelected(PRBool aSelect)
{
  
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;
  }

  PRUint32 state = State(this);
  if (state & nsIAccessibleStates::STATE_SELECTABLE) {
    nsCOMPtr<nsIAccessible> multiSelect = GetMultiSelectFor(mDOMNode);
    if (!multiSelect) {
      return aSelect ? TakeFocus() : NS_ERROR_FAILURE;
    }
    nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
    NS_ASSERTION(content, "Called for dead accessible");

    if (mRoleMapEntry) {
      if (aSelect) {
        return content->SetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_selected,
                                NS_LITERAL_STRING("true"), PR_TRUE);
      }
      return content->UnsetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_selected, PR_TRUE);
    }
  }

  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsAccessible::TakeSelection()
{
  
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;
  }

  PRUint32 state = State(this);
  if (state & nsIAccessibleStates::STATE_SELECTABLE) {
    nsCOMPtr<nsIAccessible> multiSelect = GetMultiSelectFor(mDOMNode);
    if (multiSelect) {
      nsCOMPtr<nsIAccessibleSelectable> selectable = do_QueryInterface(multiSelect);
      selectable->ClearSelection();
    }
    return SetSelected(PR_TRUE);
  }

  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsAccessible::TakeFocus()
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));

  nsIFrame *frame = GetFrame();
  NS_ENSURE_STATE(frame);

  
  
  
  
  if (!frame->IsFocusable()) {
    nsAutoString id;
    if (content && nsAccUtils::GetID(content, id)) {

      nsCOMPtr<nsIContent> ancestorContent = content;
      while ((ancestorContent = ancestorContent->GetParent()) &&
             !ancestorContent->HasAttr(kNameSpaceID_None,
                                       nsAccessibilityAtoms::aria_activedescendant));

      if (ancestorContent) {
        nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));
        if (presShell) {
          nsIFrame *frame = presShell->GetPrimaryFrameFor(ancestorContent);
          if (frame && frame->IsFocusable()) {

            content = ancestorContent;            
            content->SetAttr(kNameSpaceID_None,
                             nsAccessibilityAtoms::aria_activedescendant,
                             id, PR_TRUE);
          }
        }
      }
    }
  }

  nsCOMPtr<nsIDOMNSHTMLElement> htmlElement(do_QueryInterface(content));
  if (htmlElement) {
    
    
    return htmlElement->Focus();
  }

  content->SetFocus(GetPresContext());
  return NS_OK;
}

nsresult nsAccessible::AppendStringWithSpaces(nsAString *aFlatString, const nsAString& textEquivalent)
{
  
  if (!textEquivalent.IsEmpty()) {
    if (!aFlatString->IsEmpty())
      aFlatString->Append(PRUnichar(' '));
    aFlatString->Append(textEquivalent);
    aFlatString->Append(PRUnichar(' '));
  }
  return NS_OK;
}

nsresult nsAccessible::AppendNameFromAccessibleFor(nsIContent *aContent,
                                                   nsAString *aFlatString,
                                                   PRBool aFromValue)
{
  nsAutoString textEquivalent, value;

  nsCOMPtr<nsIDOMNode> domNode(do_QueryInterface(aContent));
  nsCOMPtr<nsIAccessible> accessible;
  if (domNode == mDOMNode) {
    accessible = this;
    if (!aFromValue) {
      
      return NS_OK;
    }
  }
  else {
    nsCOMPtr<nsIAccessibilityService> accService =
      do_GetService("@mozilla.org/accessibilityService;1");
    NS_ENSURE_TRUE(accService, NS_ERROR_FAILURE);
    accService->GetAccessibleInWeakShell(domNode, mWeakShell, getter_AddRefs(accessible));
  }
  if (accessible) {
    if (aFromValue) {
      accessible->GetValue(textEquivalent);
    }
    else {
      accessible->GetName(textEquivalent);
    }
  }

  textEquivalent.CompressWhitespace();
  return AppendStringWithSpaces(aFlatString, textEquivalent);
}










nsresult nsAccessible::AppendFlatStringFromContentNode(nsIContent *aContent, nsAString *aFlatString)
{
  if (aContent->IsNodeOfType(nsINode::eTEXT)) {
    
    PRBool isHTMLBlock = PR_FALSE;
    nsCOMPtr<nsIPresShell> shell = GetPresShell();
    if (!shell) {
      return NS_ERROR_FAILURE;  
    }

    nsIContent *parentContent = aContent->GetParent();
    nsCOMPtr<nsIContent> appendedSubtreeStart(do_QueryInterface(mDOMNode));
    if (parentContent && parentContent != appendedSubtreeStart) {
      nsIFrame *frame = shell->GetPrimaryFrameFor(parentContent);
      if (frame) {
        
        
        
        const nsStyleDisplay* display = frame->GetStyleDisplay();
        if (display->IsBlockOutside() ||
          display->mDisplay == NS_STYLE_DISPLAY_TABLE_CELL) {
          isHTMLBlock = PR_TRUE;
          if (!aFlatString->IsEmpty()) {
            aFlatString->Append(PRUnichar(' '));
          }
        }
      }
    }
    if (aContent->TextLength() > 0) {
      nsIFrame *frame = shell->GetPrimaryFrameFor(aContent);
      if (frame) {
        nsresult rv = frame->GetRenderedText(aFlatString);
        NS_ENSURE_SUCCESS(rv, rv);
      } else {
        
        aContent->AppendTextTo(*aFlatString);
      }
      if (isHTMLBlock && !aFlatString->IsEmpty()) {
        aFlatString->Append(PRUnichar(' '));
      }
    }
    return NS_OK;
  }

  nsAutoString textEquivalent;
  if (!aContent->IsNodeOfType(nsINode::eHTML)) {
    if (aContent->IsNodeOfType(nsINode::eXUL)) {
      nsCOMPtr<nsIDOMXULLabeledControlElement> labeledEl(do_QueryInterface(aContent));
      if (labeledEl) {
        labeledEl->GetLabel(textEquivalent);
      }
      else {
        if (aContent->NodeInfo()->Equals(nsAccessibilityAtoms::label, kNameSpaceID_XUL)) {
          aContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::value, textEquivalent);
        }
        if (textEquivalent.IsEmpty()) {
          aContent->GetAttr(kNameSpaceID_None,
                            nsAccessibilityAtoms::tooltiptext, textEquivalent);
        }
      }
      AppendNameFromAccessibleFor(aContent, &textEquivalent, PR_TRUE );

      return AppendStringWithSpaces(aFlatString, textEquivalent);
    }
    return NS_OK; 
  }

  nsCOMPtr<nsIAtom> tag = aContent->Tag();
  if (tag == nsAccessibilityAtoms::img) {
    return AppendNameFromAccessibleFor(aContent, aFlatString);
  }

  if (tag == nsAccessibilityAtoms::input) {
    static nsIContent::AttrValuesArray strings[] =
      {&nsAccessibilityAtoms::button, &nsAccessibilityAtoms::submit,
       &nsAccessibilityAtoms::reset, &nsAccessibilityAtoms::image, nsnull};
    if (aContent->FindAttrValueIn(kNameSpaceID_None, nsAccessibilityAtoms::type,
                                  strings, eIgnoreCase) >= 0) {
      return AppendNameFromAccessibleFor(aContent, aFlatString);
    }
  }

  if (tag == nsAccessibilityAtoms::object && !aContent->GetChildCount()) {
    
    aContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::title, textEquivalent);
  }
  else if (tag == nsAccessibilityAtoms::br) {
    
    aFlatString->AppendLiteral("\r\n");
    return NS_OK;
  }
  else if (tag != nsAccessibilityAtoms::a && tag != nsAccessibilityAtoms::area) { 
    AppendNameFromAccessibleFor(aContent, aFlatString, PR_TRUE );
  }

  textEquivalent.CompressWhitespace();
  return AppendStringWithSpaces(aFlatString, textEquivalent);
}


nsresult nsAccessible::AppendFlatStringFromSubtree(nsIContent *aContent, nsAString *aFlatString)
{
  static PRBool isAlreadyHere; 
  if (isAlreadyHere) {
    return NS_OK;
  }

  isAlreadyHere = PR_TRUE;

  nsCOMPtr<nsIPresShell> shell = GetPresShell();
  NS_ENSURE_TRUE(shell, NS_ERROR_FAILURE);

  nsIFrame *frame = shell->GetPrimaryFrameFor(aContent);
  PRBool isHidden = (!frame || !frame->GetStyleVisibility()->IsVisible());
  nsresult rv = AppendFlatStringFromSubtreeRecurse(aContent, aFlatString,
                                                   isHidden);

  isAlreadyHere = PR_FALSE;

  if (NS_SUCCEEDED(rv) && !aFlatString->IsEmpty()) {
    nsAString::const_iterator start, end;
    aFlatString->BeginReading(start);
    aFlatString->EndReading(end);

    PRInt32 spacesToTruncate = 0;
    while (-- end != start && *end == ' ')
      ++ spacesToTruncate;

    if (spacesToTruncate > 0)
      aFlatString->Truncate(aFlatString->Length() - spacesToTruncate);
  }

  return rv;
}

nsresult
nsAccessible::AppendFlatStringFromSubtreeRecurse(nsIContent *aContent,
                                                 nsAString *aFlatString,
                                                 PRBool aIsRootHidden)
{
  
  
  PRUint32 numChildren = 0;
  nsCOMPtr<nsIDOMXULSelectControlElement> selectControlEl(do_QueryInterface(aContent));
  
  if (!selectControlEl && aContent->Tag() != nsAccessibilityAtoms::textarea) {
    
    
    
    numChildren = aContent->GetChildCount();
  }

  if (numChildren == 0) {
    
    AppendFlatStringFromContentNode(aContent, aFlatString);
    return NS_OK;
  }

  
  nsCOMPtr<nsIPresShell> shell = GetPresShell();
  NS_ENSURE_TRUE(shell, NS_ERROR_FAILURE);

  PRUint32 index;
  for (index = 0; index < numChildren; index++) {
    nsCOMPtr<nsIContent> childContent = aContent->GetChildAt(index);

    
    
    if (!aIsRootHidden) {
      nsIFrame *childFrame = shell->GetPrimaryFrameFor(childContent);
      if (!childFrame || !childFrame->GetStyleVisibility()->IsVisible())
        continue;
    }

    AppendFlatStringFromSubtreeRecurse(childContent, aFlatString,
                                       aIsRootHidden);
  }

  return NS_OK;
}

nsIContent *nsAccessible::GetLabelContent(nsIContent *aForNode)
{
  if (aForNode->IsNodeOfType(nsINode::eXUL))
    return nsAccUtils::FindNeighbourPointingToNode(aForNode, nsAccessibilityAtoms::control,
                                                   nsAccessibilityAtoms::label);

  return GetHTMLLabelContent(aForNode);
}

nsIContent* nsAccessible::GetHTMLLabelContent(nsIContent *aForNode)
{
  
  
  nsIContent *walkUpContent = aForNode;

  
  while ((walkUpContent = walkUpContent->GetParent()) != nsnull) {
    nsIAtom *tag = walkUpContent->Tag();
    if (tag == nsAccessibilityAtoms::label) {
      return walkUpContent;  
    }
    if (tag == nsAccessibilityAtoms::form ||
        tag == nsAccessibilityAtoms::body) {
      
      
      
      
      nsAutoString forId;
      if (!nsAccUtils::GetID(aForNode, forId)) {
        break;
      }
      
      return nsAccUtils::FindDescendantPointingToID(&forId, walkUpContent,
                                                    nsAccessibilityAtoms::_for);
    }
  }

  return nsnull;
}

nsresult nsAccessible::GetTextFromRelationID(nsIAtom *aIDProperty, nsString &aName)
{
  
  aName.Truncate();
  NS_ASSERTION(mDOMNode, "Called from shutdown accessible");
  nsCOMPtr<nsIContent> content = GetRoleContent(mDOMNode);
  if (!content)
    return NS_OK;

  nsAutoString ids;
  if (!content->GetAttr(kNameSpaceID_None, aIDProperty, ids))
    return NS_OK;

  ids.CompressWhitespace(PR_TRUE, PR_TRUE);

  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(content->GetOwnerDoc());
  NS_ENSURE_TRUE(domDoc, NS_ERROR_FAILURE);

  
  while (!ids.IsEmpty()) {
    nsAutoString id;
    PRInt32 idLength = ids.FindChar(' ');
    NS_ASSERTION(idLength != 0, "Should not be 0 because of CompressWhitespace() call above");
    if (idLength == kNotFound) {
      id = ids;
      ids.Truncate();
    } else {
      id = Substring(ids, 0, idLength);
      ids.Cut(0, idLength + 1);
    }

    if (!aName.IsEmpty()) {
      aName += ' '; 
    }
    nsCOMPtr<nsIDOMElement> labelElement;
    domDoc->GetElementById(id, getter_AddRefs(labelElement));
    content = do_QueryInterface(labelElement);
    if (!content) {
      return NS_OK;
    }
    
    nsresult rv = AppendFlatStringFromSubtree(content, &aName);
    if (NS_SUCCEEDED(rv)) {
      aName.CompressWhitespace();
    }
  }
  
  return NS_OK;
}






nsresult nsAccessible::GetHTMLName(nsAString& aLabel, PRBool aCanAggregateSubtree)
{
  nsCOMPtr<nsIContent> content = GetRoleContent(mDOMNode);
  if (!content)
    return NS_OK;

  nsIContent *labelContent = GetHTMLLabelContent(content);
  if (labelContent) {
    nsAutoString label;
    AppendFlatStringFromSubtree(labelContent, &label);
    label.CompressWhitespace();
    if (!label.IsEmpty()) {
      aLabel = label;
      return NS_OK;
    }
  }

  if (aCanAggregateSubtree) {
    
    nsresult rv = AppendFlatStringFromSubtree(content, &aLabel);
    if (NS_SUCCEEDED(rv) && !aLabel.IsEmpty()) {
      return NS_OK;
    }
  }

  
  if (!content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::title,
                        aLabel)) {
    aLabel.SetIsVoid(PR_TRUE);
  }
  return NS_OK;
}













nsresult nsAccessible::GetXULName(nsAString& aLabel, PRBool aCanAggregateSubtree)
{
  
  nsresult rv = NS_OK;

  nsAutoString label;
  nsCOMPtr<nsIDOMXULLabeledControlElement> labeledEl(do_QueryInterface(mDOMNode));
  if (labeledEl) {
    rv = labeledEl->GetLabel(label);
  }
  else {
    nsCOMPtr<nsIDOMXULSelectControlItemElement> itemEl(do_QueryInterface(mDOMNode));
    if (itemEl) {
      rv = itemEl->GetLabel(label);
    }
    else {
      nsCOMPtr<nsIDOMXULSelectControlElement> select(do_QueryInterface(mDOMNode));
      
      
      if (!select) {
        nsCOMPtr<nsIDOMXULElement> xulEl(do_QueryInterface(mDOMNode));
        if (xulEl) {
          rv = xulEl->GetAttribute(NS_LITERAL_STRING("label"), label);
        }
      }
    }
  }

  
  nsCOMPtr<nsIContent> content = GetRoleContent(mDOMNode);
  if (!content)
    return NS_OK;

  if (NS_FAILED(rv) || label.IsEmpty()) {
    label.Truncate();
    nsIContent *labelContent =
      nsAccUtils::FindNeighbourPointingToNode(content, nsAccessibilityAtoms::control,
                                              nsAccessibilityAtoms::label);

    nsCOMPtr<nsIDOMXULLabelElement> xulLabel(do_QueryInterface(labelContent));
    
    if (xulLabel && NS_SUCCEEDED(xulLabel->GetValue(label)) && label.IsEmpty()) {
      
      
      AppendFlatStringFromSubtree(labelContent, &label);
    }
  }

  
  label.CompressWhitespace();
  if (!label.IsEmpty()) {
    aLabel = label;
    return NS_OK;
  }

  content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::tooltiptext, label);
  label.CompressWhitespace();
  if (!label.IsEmpty()) {
    aLabel = label;
    return NS_OK;
  }

  
  nsIContent *bindingParent = content->GetBindingParent();
  nsIContent *parent = bindingParent? bindingParent->GetParent() :
                                      content->GetParent();
  while (parent) {
    if (parent->Tag() == nsAccessibilityAtoms::toolbaritem &&
        parent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::title, label)) {
      label.CompressWhitespace();
      aLabel = label;
      return NS_OK;
    }
    parent = parent->GetParent();
  }

  
  return aCanAggregateSubtree? AppendFlatStringFromSubtree(content, &aLabel) : NS_OK;
}

PRBool nsAccessible::IsNodeRelevant(nsIDOMNode *aNode)
{
  
  
  nsCOMPtr<nsIAccessibilityService> accService =
    do_GetService("@mozilla.org/accessibilityService;1");
  NS_ENSURE_TRUE(accService, PR_FALSE);
  nsCOMPtr<nsIDOMNode> relevantNode;
  accService->GetRelevantContentNodeFor(aNode, getter_AddRefs(relevantNode));
  return aNode == relevantNode;
}

NS_IMETHODIMP
nsAccessible::FireToolkitEvent(PRUint32 aEvent, nsIAccessible *aTarget)
{
  
  if (!mWeakShell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIAccessibleEvent> accEvent =
    new nsAccEvent(aEvent, aTarget);
  NS_ENSURE_TRUE(accEvent, NS_ERROR_OUT_OF_MEMORY);

  return FireAccessibleEvent(accEvent);
}

NS_IMETHODIMP
nsAccessible::FireAccessibleEvent(nsIAccessibleEvent *aEvent)
{
  NS_ENSURE_ARG_POINTER(aEvent);
  nsCOMPtr<nsIDOMNode> eventNode;
  aEvent->GetDOMNode(getter_AddRefs(eventNode));
  NS_ENSURE_TRUE(IsNodeRelevant(eventNode), NS_ERROR_FAILURE);

  nsCOMPtr<nsIObserverService> obsService =
    do_GetService("@mozilla.org/observer-service;1");
  NS_ENSURE_TRUE(obsService, NS_ERROR_FAILURE);

  return obsService->NotifyObservers(aEvent, NS_ACCESSIBLE_EVENT_TOPIC, nsnull);
}

NS_IMETHODIMP nsAccessible::GetFinalRole(PRUint32 *aRole)
{
  NS_ENSURE_ARG_POINTER(aRole);
  *aRole = nsIAccessibleRole::ROLE_NOTHING;

  if (mRoleMapEntry) {
    *aRole = mRoleMapEntry->role;

    
    
    if (*aRole == nsIAccessibleRole::ROLE_PUSHBUTTON) {
      nsCOMPtr<nsIContent> content = do_QueryInterface(mDOMNode);
      if (content) {
        if (content->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_pressed)) {
          
          
          *aRole = nsIAccessibleRole::ROLE_TOGGLE_BUTTON;
        }
        else if (content->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::aria_haspopup,
                                      nsAccessibilityAtoms::_true, eCaseMatters)) {
          
          *aRole = nsIAccessibleRole::ROLE_BUTTONMENU;
        }
      }
    }
    else if (*aRole == nsIAccessibleRole::ROLE_LISTBOX) {
      
      nsCOMPtr<nsIAccessible> possibleCombo;
      GetParent(getter_AddRefs(possibleCombo));
      if (possibleCombo && Role(possibleCombo) == nsIAccessibleRole::ROLE_COMBOBOX) {
        *aRole = nsIAccessibleRole::ROLE_COMBOBOX_LIST;
      }
      else {   
        GetAccessibleRelated(nsIAccessibleRelation::RELATION_NODE_CHILD_OF, getter_AddRefs(possibleCombo));
        if (possibleCombo && Role(possibleCombo) == nsIAccessibleRole::ROLE_COMBOBOX) {
          *aRole = nsIAccessibleRole::ROLE_COMBOBOX_LIST;
        }
      }
    }
    else if (*aRole == nsIAccessibleRole::ROLE_OPTION) {
      nsCOMPtr<nsIAccessible> parent;
      GetParent(getter_AddRefs(parent));
      if (parent && Role(parent) == nsIAccessibleRole::ROLE_COMBOBOX_LIST) {
        *aRole = nsIAccessibleRole::ROLE_COMBOBOX_OPTION;
      }
    }

    
    
    if (mRoleMapEntry != &nsARIAMap::gLandmarkRoleMap) {
      
      
      
      
      return NS_OK;
    }
  }
  return mDOMNode ? GetRole(aRole) : NS_ERROR_FAILURE;  
}

NS_IMETHODIMP
nsAccessible::GetAttributes(nsIPersistentProperties **aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);  
  
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> content = GetRoleContent(mDOMNode);
  if (!content) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIPersistentProperties> attributes = *aAttributes;
  if (!attributes) {
    
    attributes = do_CreateInstance(NS_PERSISTENTPROPERTIES_CONTRACTID);
    NS_ENSURE_TRUE(attributes, NS_ERROR_OUT_OF_MEMORY);
    NS_ADDREF(*aAttributes = attributes);
  }
 
  nsresult rv = GetAttributesInternal(attributes);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString id;
  nsAutoString oldValueUnused;
  if (nsAccUtils::GetID(content, id)) {
    
    
    attributes->SetStringProperty(NS_LITERAL_CSTRING("id"), id, oldValueUnused);
  }
  
  nsAutoString xmlRoles;
  if (content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::role, xmlRoles)) {
    attributes->SetStringProperty(NS_LITERAL_CSTRING("xml-roles"),  xmlRoles, oldValueUnused);          
  }

  nsCOMPtr<nsIAccessibleValue> supportsValue = do_QueryInterface(static_cast<nsIAccessible*>(this));
  if (supportsValue) {
    
    
    
    nsAutoString valuetext;
    GetValue(valuetext);
    attributes->SetStringProperty(NS_LITERAL_CSTRING("valuetext"), valuetext, oldValueUnused);
  }


  PRUint32 role = Role(this);
  if (role == nsIAccessibleRole::ROLE_CHECKBUTTON ||
      role == nsIAccessibleRole::ROLE_PUSHBUTTON ||
      role == nsIAccessibleRole::ROLE_MENUITEM ||
      role == nsIAccessibleRole::ROLE_LISTITEM ||
      role == nsIAccessibleRole::ROLE_OPTION ||
      role == nsIAccessibleRole::ROLE_RADIOBUTTON ||
      role == nsIAccessibleRole::ROLE_RICH_OPTION ||
      role == nsIAccessibleRole::ROLE_OUTLINEITEM ||
      content->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_checked)) {
    
    PRUint32 state = 0;
    GetFinalState(&state, nsnull);
    if (state & nsIAccessibleStates::STATE_CHECKABLE) {
      
      attributes->SetStringProperty(NS_LITERAL_CSTRING("checkable"), NS_LITERAL_STRING("true"),
                                    oldValueUnused);
    }
  }

  
  if (!nsAccUtils::HasAccGroupAttrs(attributes)) {
    
    
    

    
    
    if ((role == nsIAccessibleRole::ROLE_LISTITEM ||
         role == nsIAccessibleRole::ROLE_MENUITEM ||
         role == nsIAccessibleRole::ROLE_CHECK_MENU_ITEM ||
         role == nsIAccessibleRole::ROLE_RADIO_MENU_ITEM ||
         role == nsIAccessibleRole::ROLE_RADIOBUTTON ||
         role == nsIAccessibleRole::ROLE_PAGETAB ||
         role == nsIAccessibleRole::ROLE_OPTION ||
         role == nsIAccessibleRole::ROLE_RADIOBUTTON ||
         role == nsIAccessibleRole::ROLE_OUTLINEITEM) &&
        0 == (State(this) & nsIAccessibleStates::STATE_INVISIBLE)) {

      PRUint32 baseRole = role;
      if (role == nsIAccessibleRole::ROLE_CHECK_MENU_ITEM ||
          role == nsIAccessibleRole::ROLE_RADIO_MENU_ITEM)
        baseRole = nsIAccessibleRole::ROLE_MENUITEM;

      nsCOMPtr<nsIAccessible> parent = GetParent();
      NS_ENSURE_TRUE(parent, NS_ERROR_FAILURE);

      PRInt32 positionInGroup = 0;
      PRInt32 setSize = 0;

      nsCOMPtr<nsIAccessible> sibling, nextSibling;
      parent->GetFirstChild(getter_AddRefs(sibling));
      NS_ENSURE_TRUE(sibling, NS_ERROR_FAILURE);

      PRBool foundCurrent = PR_FALSE;
      PRUint32 siblingRole, siblingBaseRole;
      while (sibling) {
        sibling->GetFinalRole(&siblingRole);

        siblingBaseRole = siblingRole;
        if (siblingRole == nsIAccessibleRole::ROLE_CHECK_MENU_ITEM ||
            siblingRole == nsIAccessibleRole::ROLE_RADIO_MENU_ITEM)
          siblingBaseRole = nsIAccessibleRole::ROLE_MENUITEM;

        
        if (siblingBaseRole == baseRole &&
            !(State(sibling) & nsIAccessibleStates::STATE_INVISIBLE)) {
          ++ setSize;
          if (!foundCurrent) {
            ++ positionInGroup;
            if (sibling == this)
              foundCurrent = PR_TRUE;
          }
        }

        
        if (siblingRole == nsIAccessibleRole::ROLE_SEPARATOR) {
          if (foundCurrent) 
            break;

          
          positionInGroup = 0;
          setSize = 0;
        }

        sibling->GetNextSibling(getter_AddRefs(nextSibling));
        sibling = nextSibling;
      }

      PRInt32 groupLevel = 0;
      if (role == nsIAccessibleRole::ROLE_OUTLINEITEM) {
        groupLevel = 1;
        nsCOMPtr<nsIAccessible> nextParent;
        while (parent) {
          parent->GetFinalRole(&role);

          if (role == nsIAccessibleRole::ROLE_OUTLINE)
            break;
          if (role == nsIAccessibleRole::ROLE_GROUPING)
            ++ groupLevel;

          parent->GetParent(getter_AddRefs(nextParent));
          parent.swap(nextParent);
        }
      }

      nsAccUtils::SetAccGroupAttrs(attributes, groupLevel, positionInGroup,
                                   setSize);
    }
  }

  
  PRUint32 numAttrs = content->GetAttrCount();
  for (PRUint32 count = 0; count < numAttrs; count ++) {
    const nsAttrName *attr = content->GetAttrNameAt(count);
    if (attr && attr->NamespaceEquals(kNameSpaceID_None)) {
      nsIAtom *attrAtom = attr->Atom();
      const char *attrStr;
      attrAtom->GetUTF8String(&attrStr);
      if (PL_strncmp(attrStr, "aria-", 5)) 
        continue; 
      if (!nsAccUtils::IsARIAPropForObjectAttr(attrAtom))
        continue; 
      nsAutoString value;
      if (content->GetAttr(kNameSpaceID_None, attrAtom, value)) {
        attributes->SetStringProperty(nsDependentCString(attrStr + 5), value, oldValueUnused);
      }
    }
  }

  return NS_OK;
}

nsresult
nsAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  
  
  nsIContent *content = GetRoleContent(mDOMNode);
  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(content));
  NS_ENSURE_TRUE(element, NS_ERROR_UNEXPECTED);

  nsAutoString tagName;
  element->GetTagName(tagName);
  if (!tagName.IsEmpty()) {
    nsAutoString oldValueUnused;
    aAttributes->SetStringProperty(NS_LITERAL_CSTRING("tag"), tagName,
                                   oldValueUnused);
  }

  nsAccEvent::GetLastEventAttributes(mDOMNode, aAttributes);
 
  
  
  nsAutoString _class;
  if (content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::_class, _class))
    nsAccUtils::SetAccAttr(aAttributes, nsAccessibilityAtoms::_class, _class);

  
  
  
  
  
  
  nsCOMPtr<nsIDOMNode> startNode = mDOMNode;
  nsIContent *startContent = content;
  while (PR_TRUE) {
    NS_ENSURE_STATE(startContent);
    nsIDocument *doc = startContent->GetDocument();
    nsCOMPtr<nsIDOMNode> docNode = do_QueryInterface(doc);
    NS_ENSURE_STATE(docNode);
    nsIContent *topContent = GetRoleContent(docNode);
    NS_ENSURE_STATE(topContent);
    nsAccUtils::GetLiveContainerAttributes(aAttributes, startContent, topContent);
    
    nsCOMPtr<nsISupports> container = doc->GetContainer();
    nsIDocShellTreeItem *docShellTreeItem = nsnull;
    if (container)
      CallQueryInterface(container, &docShellTreeItem);
    if (!docShellTreeItem)
      break;
    nsIDocShellTreeItem *sameTypeParent = nsnull;
    docShellTreeItem->GetSameTypeParent(&sameTypeParent);
    if (!sameTypeParent || sameTypeParent == docShellTreeItem)
      break;
    nsIDocument *parentDoc = doc->GetParentDocument();
    if (!parentDoc)
      break;
    startContent = parentDoc->FindContentForSubDocument(doc);      
  }

  
  nsAutoString displayValue;
  nsresult rv = GetComputedStyleValue(EmptyString(),
                                      NS_LITERAL_STRING("display"),
                                      displayValue);
  if (NS_SUCCEEDED(rv))
    nsAccUtils::SetAccAttr(aAttributes, nsAccessibilityAtoms::display,
                           displayValue);
  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GroupPosition(PRInt32 *aGroupLevel,
                            PRInt32 *aSimilarItemsInGroup,
                            PRInt32 *aPositionInGroup)
{
  
  
  
  
  
  

  NS_ENSURE_ARG_POINTER(aGroupLevel);
  NS_ENSURE_ARG_POINTER(aSimilarItemsInGroup);
  NS_ENSURE_ARG_POINTER(aPositionInGroup);

  *aGroupLevel = 0;
  *aSimilarItemsInGroup = 0;
  *aPositionInGroup = 0;

  nsCOMPtr<nsIPersistentProperties> attributes;
  nsresult rv = GetAttributes(getter_AddRefs(attributes));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!attributes) {
    return NS_ERROR_FAILURE;
  }
  PRInt32 level, posInSet, setSize;
  nsAccUtils::GetAccGroupAttrs(attributes, &level, &posInSet, &setSize);

  if (!posInSet && !setSize)
    return NS_OK;

  *aGroupLevel = level;

  *aPositionInGroup = posInSet;
  *aSimilarItemsInGroup = setSize;

  return NS_OK;
}

PRBool nsAccessible::MappedAttrState(nsIContent *aContent, PRUint32 *aStateInOut,
                                     nsStateMapEntry *aStateMapEntry)
{
  
  if (!aStateMapEntry->attributeName) {
    return PR_FALSE;  
  }

  nsAutoString attribValue;
  if (aContent->GetAttr(kNameSpaceID_None, *aStateMapEntry->attributeName, attribValue)) {
    if (aStateMapEntry->attributeValue == kBoolState) {
      
      if (attribValue.EqualsLiteral("false")) {
        *aStateInOut &= ~aStateMapEntry->state;
      }
      else {
        *aStateInOut |= aStateMapEntry->state;
      }
    }
    else if (NS_ConvertUTF16toUTF8(attribValue).Equals(aStateMapEntry->attributeValue)) {
      *aStateInOut |= aStateMapEntry->state;
    }
  }

  return PR_TRUE;
}

NS_IMETHODIMP
nsAccessible::GetFinalState(PRUint32 *aState, PRUint32 *aExtraState)
{
  NS_ENSURE_ARG_POINTER(aState);

  nsresult rv = GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);

  
  GetARIAState(aState);

  if (mRoleMapEntry && mRoleMapEntry->role == nsIAccessibleRole::ROLE_PAGETAB) {
    if (*aState & nsIAccessibleStates::STATE_FOCUSED) {
      *aState |= nsIAccessibleStates::STATE_SELECTED;
    } else {
      
      
      nsCOMPtr<nsIAccessible> tabPanel;
      rv = GetAccessibleRelated(nsIAccessibleRelation::RELATION_LABEL_FOR,
                                getter_AddRefs(tabPanel));
      NS_ENSURE_SUCCESS(rv, rv);

      if (tabPanel && Role(tabPanel) == nsIAccessibleRole::ROLE_PROPERTYPAGE) {
        nsCOMPtr<nsIAccessNode> tabPanelAccessNode(do_QueryInterface(tabPanel));
        nsCOMPtr<nsIDOMNode> tabPanelNode;
        tabPanelAccessNode->GetDOMNode(getter_AddRefs(tabPanelNode));
        NS_ENSURE_STATE(tabPanelNode);

        if (nsAccUtils::IsAncestorOf(tabPanelNode, gLastFocusedNode))
          *aState |= nsIAccessibleStates::STATE_SELECTED;
      }
    }
  }

  const PRUint32 kExpandCollapseStates =
    nsIAccessibleStates::STATE_COLLAPSED | nsIAccessibleStates::STATE_EXPANDED;
  if ((*aState & kExpandCollapseStates) == kExpandCollapseStates) {
    
    
    
    
    
    *aState &= ~nsIAccessibleStates::STATE_COLLAPSED;
  }

  
  if (!aExtraState)
    return NS_OK;

  if (!(*aState & nsIAccessibleStates::STATE_UNAVAILABLE)) {
    *aExtraState |= nsIAccessibleStates::EXT_STATE_ENABLED |
                    nsIAccessibleStates::EXT_STATE_SENSITIVE;
  }

  if ((*aState & nsIAccessibleStates::STATE_COLLAPSED) ||
      (*aState & nsIAccessibleStates::STATE_EXPANDED))
    *aExtraState |= nsIAccessibleStates::EXT_STATE_EXPANDABLE;

  if (mRoleMapEntry) {
    
    
    
    
    nsCOMPtr<nsIContent> content = do_QueryInterface(mDOMNode);
    nsAutoString id;
    if (content && nsAccUtils::GetID(content, id)) {
      nsIContent *ancestorContent = content;
      nsAutoString activeID;
      while ((ancestorContent = ancestorContent->GetParent()) != nsnull) {
        if (ancestorContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_activedescendant, activeID)) {
          if (id == activeID) {
            *aExtraState |= nsIAccessibleStates::EXT_STATE_ACTIVE;
          }
          break;
        }
      }
    }
  }

  PRUint32 role;
  rv = GetFinalRole(&role);
  NS_ENSURE_SUCCESS(rv, rv);

  if (role == nsIAccessibleRole::ROLE_ENTRY ||
      role == nsIAccessibleRole::ROLE_COMBOBOX) {

    nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
    NS_ENSURE_STATE(content);

    nsAutoString autocomplete;
    if (content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_autocomplete, autocomplete) &&
        (autocomplete.EqualsIgnoreCase("inline") ||
         autocomplete.EqualsIgnoreCase("list") ||
         autocomplete.EqualsIgnoreCase("both"))) {
      *aExtraState |= nsIAccessibleStates::EXT_STATE_SUPPORTS_AUTOCOMPLETION;
    }

    
    if (mRoleMapEntry && mRoleMapEntry->role == nsIAccessibleRole::ROLE_ENTRY) {
      PRBool isMultiLine = content->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::aria_multiline,
                                                nsAccessibilityAtoms::_true, eCaseMatters);
      *aExtraState |= isMultiLine ? nsIAccessibleStates::EXT_STATE_MULTI_LINE : nsIAccessibleStates::EXT_STATE_SINGLE_LINE;
      if (0 == (*aState & nsIAccessibleStates::STATE_READONLY))
        *aExtraState |= nsIAccessibleStates::EXT_STATE_EDITABLE; 
      else  
        *aExtraState &= ~nsIAccessibleStates::EXT_STATE_EDITABLE;
    }
  }

  
  
  nsIFrame *frame = GetFrame();
  if (!frame)
    return NS_OK;

  const nsStyleDisplay* display = frame->GetStyleDisplay();
  if (display && display->mOpacity == 1.0f &&
      !(*aState & nsIAccessibleStates::STATE_INVISIBLE)) {
    *aExtraState |= nsIAccessibleStates::EXT_STATE_OPAQUE;
  }

  const nsStyleXUL *xulStyle = frame->GetStyleXUL();
  if (xulStyle) {
    
    if (xulStyle->mBoxOrient == NS_STYLE_BOX_ORIENT_VERTICAL) {
      *aExtraState |= nsIAccessibleStates::EXT_STATE_VERTICAL;
    }
    else {
      *aExtraState |= nsIAccessibleStates::EXT_STATE_HORIZONTAL;
    }
  }

  return NS_OK;
}

nsresult
nsAccessible::GetARIAState(PRUint32 *aState)
{
  
  nsIContent *content = GetRoleContent(mDOMNode);
  if (!content) {
    return NS_OK;
  }

  PRUint32 index = 0;
  while (MappedAttrState(content, aState, &nsARIAMap::gWAIUnivStateMap[index])) {
    ++ index;
  }

  if (mRoleMapEntry) {
    
    *aState &= ~nsIAccessibleStates::STATE_READONLY;

    if (content->HasAttr(kNameSpaceID_None, content->GetIDAttributeName())) {
      
      nsIContent *ancestorContent = content;
      while ((ancestorContent = ancestorContent->GetParent()) != nsnull) {
        if (ancestorContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_activedescendant)) {
            
          *aState |= nsIAccessibleStates::STATE_FOCUSABLE;
          break;
        }
      }
    }
  }

  if (*aState & nsIAccessibleStates::STATE_FOCUSABLE) {
    
    nsIContent *ancestorContent = content;
    while ((ancestorContent = ancestorContent->GetParent()) != nsnull) {
      if (ancestorContent->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::aria_disabled,
                                       nsAccessibilityAtoms::_true, eCaseMatters)) {
          
        *aState |= nsIAccessibleStates::STATE_UNAVAILABLE;
        break;
      }
    }    
  }

  if (!mRoleMapEntry)
    return NS_OK;

  *aState |= mRoleMapEntry->state;
  if (MappedAttrState(content, aState, &mRoleMapEntry->attributeMap1) &&
      MappedAttrState(content, aState, &mRoleMapEntry->attributeMap2) &&
      MappedAttrState(content, aState, &mRoleMapEntry->attributeMap3) &&
      MappedAttrState(content, aState, &mRoleMapEntry->attributeMap4) &&
      MappedAttrState(content, aState, &mRoleMapEntry->attributeMap5) &&
      MappedAttrState(content, aState, &mRoleMapEntry->attributeMap6) &&
      MappedAttrState(content, aState, &mRoleMapEntry->attributeMap7)) {
    MappedAttrState(content, aState, &mRoleMapEntry->attributeMap8);
  }

  return NS_OK;
}




NS_IMETHODIMP
nsAccessible::GetValue(nsAString& aValue)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (!content)
    return NS_OK;

  if (mRoleMapEntry) {
    if (mRoleMapEntry->valueRule == eNoValue) {
      return NS_OK;
    }

    
    
    if (!content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_valuetext, aValue)) {
      content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_valuenow, aValue);
    }
  }

  if (!aValue.IsEmpty())
    return NS_OK;

  
  if (nsAccUtils::IsXLink(content)) {
    nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));
    if (presShell)
      return presShell->GetLinkLocation(mDOMNode, aValue);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetMaximumValue(double *aMaximumValue)
{
  return GetAttrValue(nsAccessibilityAtoms::aria_valuemax, aMaximumValue);
}

NS_IMETHODIMP
nsAccessible::GetMinimumValue(double *aMinimumValue)
{
  return GetAttrValue(nsAccessibilityAtoms::aria_valuemin, aMinimumValue);
}

NS_IMETHODIMP
nsAccessible::GetMinimumIncrement(double *aMinIncrement)
{
  NS_ENSURE_ARG_POINTER(aMinIncrement);
  *aMinIncrement = 0;

  
  return NS_OK_NO_ARIA_VALUE;
}

NS_IMETHODIMP
nsAccessible::GetCurrentValue(double *aValue)
{
  return GetAttrValue(nsAccessibilityAtoms::aria_valuenow, aValue);
}

NS_IMETHODIMP
nsAccessible::SetCurrentValue(double aValue)
{
  if (!mDOMNode)
    return NS_ERROR_FAILURE;  

  if (!mRoleMapEntry || mRoleMapEntry->valueRule == eNoValue)
    return NS_OK_NO_ARIA_VALUE;

  const PRUint32 kValueCannotChange = nsIAccessibleStates::STATE_READONLY |
                                      nsIAccessibleStates::STATE_UNAVAILABLE;

  if (State(this) & kValueCannotChange)
    return NS_ERROR_FAILURE;

  double minValue = 0;
  if (NS_SUCCEEDED(GetMinimumValue(&minValue)) && aValue < minValue)
    return NS_ERROR_INVALID_ARG;

  double maxValue = 0;
  if (NS_SUCCEEDED(GetMaximumValue(&maxValue)) && aValue > maxValue)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  NS_ENSURE_STATE(content);

  nsAutoString newValue;
  newValue.AppendFloat(aValue);
  return content->SetAttr(kNameSpaceID_None,
                          nsAccessibilityAtoms::aria_valuenow, newValue, PR_TRUE);
}


NS_IMETHODIMP nsAccessible::SetName(const nsAString& name)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsAccessible::GetDefaultKeyBinding(nsAString& aKeyBinding)
{
  aKeyBinding.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GetKeyBindings(PRUint8 aActionIndex,
                             nsIDOMDOMStringList **aKeyBindings)
{
  
  NS_ENSURE_TRUE(aActionIndex == 0, NS_ERROR_INVALID_ARG);

  nsAccessibleDOMStringList *keyBindings = new nsAccessibleDOMStringList();
  NS_ENSURE_TRUE(keyBindings, NS_ERROR_OUT_OF_MEMORY);

  nsAutoString defaultKey;
  nsresult rv = GetDefaultKeyBinding(defaultKey);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!defaultKey.IsEmpty())
    keyBindings->Add(defaultKey);

  NS_ADDREF(*aKeyBindings = keyBindings);
  return NS_OK;
}


NS_IMETHODIMP nsAccessible::GetRole(PRUint32 *aRole)
{
  NS_ENSURE_ARG_POINTER(aRole);
  *aRole = nsIAccessibleRole::ROLE_NOTHING;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (nsAccUtils::IsXLink(content))
    *aRole = nsIAccessibleRole::ROLE_LINK;

  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetNumActions(PRUint8 *aNumActions)
{
  NS_ENSURE_ARG_POINTER(aNumActions);
  *aNumActions = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRUint32 actionRule = GetActionRule(State(this));
  if (actionRule == eNoAction)
    return NS_OK;

  *aNumActions = 1;
  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  aName.Truncate();

  if (aIndex != 0)
    return NS_ERROR_INVALID_ARG;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRUint32 states = State(this);
  PRUint32 actionRule = GetActionRule(states);

 switch (actionRule) {
   case eActivateAction:
     aName.AssignLiteral("activate");
     return NS_OK;

   case eClickAction:
     aName.AssignLiteral("click");
     return NS_OK;

   case eCheckUncheckAction:
     if (states & nsIAccessibleStates::STATE_CHECKED)
       aName.AssignLiteral("uncheck");
     else
       aName.AssignLiteral("check");
     return NS_OK;

   case eJumpAction:
     aName.AssignLiteral("jump");
     return NS_OK;

   case eOpenCloseAction:
     if (states & nsIAccessibleStates::STATE_COLLAPSED)
       aName.AssignLiteral("open");
     else
       aName.AssignLiteral("close");
     return NS_OK;

   case eSelectAction:
     aName.AssignLiteral("select");
     return NS_OK;

   case eSwitchAction:
     aName.AssignLiteral("switch");
     return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}


NS_IMETHODIMP
nsAccessible::GetActionDescription(PRUint8 aIndex, nsAString& aDescription)
{
  
  nsAutoString name;
  nsresult rv = GetActionName(aIndex, name);
  NS_ENSURE_SUCCESS(rv, rv);

  return GetTranslatedString(name, aDescription);
}


NS_IMETHODIMP
nsAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != 0)
    return NS_ERROR_INVALID_ARG;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (GetActionRule(State(this)) != eNoAction) {
    nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
    return DoCommand(content);
  }

  return NS_ERROR_INVALID_ARG;
}


NS_IMETHODIMP nsAccessible::GetHelp(nsAString& _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsAccessible::GetAccessibleToRight(nsIAccessible **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsAccessible::GetAccessibleToLeft(nsIAccessible **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsAccessible::GetAccessibleAbove(nsIAccessible **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsAccessible::GetAccessibleBelow(nsIAccessible **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsIDOMNode* nsAccessible::GetAtomicRegion()
{
  nsCOMPtr<nsIContent> content = GetRoleContent(mDOMNode);
  nsIContent *loopContent = content;
  nsAutoString atomic;
  while (loopContent && !loopContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_atomic, atomic)) {
    loopContent = loopContent->GetParent();
  }

  nsCOMPtr<nsIDOMNode> atomicRegion;
  if (atomic.EqualsLiteral("true")) {
    atomicRegion = do_QueryInterface(loopContent);
  }
  return atomicRegion;
}


NS_IMETHODIMP nsAccessible::GetAccessibleRelated(PRUint32 aRelationType, nsIAccessible **aRelated)
{
  
  
  *aRelated = nsnull;

  
  
  nsIContent *content = GetRoleContent(mDOMNode);
  if (!content) {
    return NS_ERROR_FAILURE;  
  }

  nsCOMPtr<nsIDOMNode> relatedNode;
  nsAutoString relatedID;

  
  switch (aRelationType)
  {
  case nsIAccessibleRelation::RELATION_LABEL_FOR:
    {
      if (content->Tag() == nsAccessibilityAtoms::label) {
        nsIAtom *relatedIDAttr = content->IsNodeOfType(nsINode::eHTML) ?
          nsAccessibilityAtoms::_for : nsAccessibilityAtoms::control;
        content->GetAttr(kNameSpaceID_None, relatedIDAttr, relatedID);
      }
      if (relatedID.IsEmpty()) {
        relatedNode =
          do_QueryInterface(nsAccUtils::FindNeighbourPointingToNode(content, nsAccessibilityAtoms::aria_labelledby));
      }
      break;
    }
  case nsIAccessibleRelation::RELATION_LABELLED_BY:
    {
      if (!content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_labelledby, relatedID)) {
        relatedNode = do_QueryInterface(GetLabelContent(content));
      }
      break;
    }
  case nsIAccessibleRelation::RELATION_DESCRIBED_BY:
    {
      if (!content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_describedby, relatedID)) {
        relatedNode = do_QueryInterface(
          nsAccUtils::FindNeighbourPointingToNode(content, nsAccessibilityAtoms::control, nsAccessibilityAtoms::description));
      }
      break;
    }
  case nsIAccessibleRelation::RELATION_DESCRIPTION_FOR:
    {
      relatedNode =
        do_QueryInterface(nsAccUtils::FindNeighbourPointingToNode(content, nsAccessibilityAtoms::aria_describedby));

      if (!relatedNode && content->Tag() == nsAccessibilityAtoms::description &&
          content->IsNodeOfType(nsINode::eXUL)) {
        
        
        
        content->GetAttr(kNameSpaceID_None,
                         nsAccessibilityAtoms::control, relatedID);
      }
      break;
    }
  case nsIAccessibleRelation::RELATION_NODE_CHILD_OF:
    {
      relatedNode =
        do_QueryInterface(nsAccUtils::FindNeighbourPointingToNode(content, nsAccessibilityAtoms::aria_owns));
      if (!relatedNode && mRoleMapEntry && mRoleMapEntry->role == nsIAccessibleRole::ROLE_OUTLINEITEM) {
        
        nsAccUtils::GetARIATreeItemParent(this, content, aRelated);
        return NS_OK;
      }
      
      
      
      
      nsIFrame *frame = GetFrame();
      if (frame) {
        nsIView *view = frame->GetViewExternal();
        if (view) {
          nsIScrollableFrame *scrollFrame = nsnull;
          CallQueryInterface(frame, &scrollFrame);
          if (scrollFrame || view->GetWidget()) {
            return GetParent(aRelated);
          }
        }
      }
      break;
    }
  case nsIAccessibleRelation::RELATION_CONTROLLED_BY:
    {
      relatedNode =
        do_QueryInterface(nsAccUtils::FindNeighbourPointingToNode(content, nsAccessibilityAtoms::aria_controls));
      break;
    }
  case nsIAccessibleRelation::RELATION_CONTROLLER_FOR:
    {
      content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_controls, relatedID);
      break;
    }
  case nsIAccessibleRelation::RELATION_FLOWS_TO:
    {
      content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_flowto, relatedID);
      break;
    }
  case nsIAccessibleRelation::RELATION_FLOWS_FROM:
    {
      relatedNode =
        do_QueryInterface(nsAccUtils::FindNeighbourPointingToNode(content, nsAccessibilityAtoms::aria_flowto));
      break;
    }
  case nsIAccessibleRelation::RELATION_DEFAULT_BUTTON:
    {
      if (content->IsNodeOfType(nsINode::eHTML)) {
        
        nsCOMPtr<nsIFormControl> control(do_QueryInterface(content));
        if (control) {
          nsCOMPtr<nsIDOMHTMLFormElement> htmlform;
          control->GetForm(getter_AddRefs(htmlform));
          nsCOMPtr<nsIForm> form(do_QueryInterface(htmlform));
          if (form)
            relatedNode = do_QueryInterface(form->GetDefaultSubmitElement());
        }
      }
      else {
        
        nsCOMPtr<nsIDOMXULDocument> xulDoc = do_QueryInterface(content->GetDocument());
        nsCOMPtr<nsIDOMXULButtonElement> buttonEl;
        if (xulDoc) {
          nsCOMPtr<nsIDOMNodeList> possibleDefaultButtons;
          xulDoc->GetElementsByAttribute(NS_LITERAL_STRING("default"),
                                         NS_LITERAL_STRING("true"),
                                         getter_AddRefs(possibleDefaultButtons));
          if (possibleDefaultButtons) {
            PRUint32 length;
            possibleDefaultButtons->GetLength(&length);
            nsCOMPtr<nsIDOMNode> possibleButton;
            
            for (PRUint32 count = 0; count < length && !buttonEl; count ++) {
              possibleDefaultButtons->Item(count, getter_AddRefs(possibleButton));
              buttonEl = do_QueryInterface(possibleButton);
            }
          }
          if (!buttonEl) { 
            nsCOMPtr<nsIDOMDocumentXBL> xblDoc(do_QueryInterface(xulDoc));
            if (xblDoc) {
              nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(xulDoc);
              NS_ASSERTION(domDoc, "No DOM document");
              nsCOMPtr<nsIDOMElement> rootEl;
              domDoc->GetDocumentElement(getter_AddRefs(rootEl));
              if (rootEl) {
                nsCOMPtr<nsIDOMElement> possibleButtonEl;
                xblDoc->GetAnonymousElementByAttribute(rootEl,
                                                      NS_LITERAL_STRING("default"),
                                                      NS_LITERAL_STRING("true"),
                                                      getter_AddRefs(possibleButtonEl));
                buttonEl = do_QueryInterface(possibleButtonEl);
              }
            }
          }
          relatedNode = do_QueryInterface(buttonEl);
        }
      }
      break;
    }
  case nsIAccessibleRelation::RELATION_MEMBER_OF:
    {
      relatedNode = GetAtomicRegion();
      break;
    }
  default:
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  if (!relatedID.IsEmpty()) {
    
    nsCOMPtr<nsIDOMDocument> domDoc;
    mDOMNode->GetOwnerDocument(getter_AddRefs(domDoc));
    NS_ENSURE_TRUE(domDoc, NS_ERROR_FAILURE);
    nsCOMPtr<nsIDOMElement> relatedEl;
    domDoc->GetElementById(relatedID, getter_AddRefs(relatedEl));
    relatedNode = do_QueryInterface(relatedEl);
  }

  
  if (relatedNode) {
    nsCOMPtr<nsIAccessibilityService> accService = GetAccService();
    NS_ENSURE_TRUE(accService, NS_ERROR_FAILURE);
    accService->GetAccessibleInWeakShell(relatedNode, mWeakShell, aRelated);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GetRelationsCount(PRUint32 *aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);
  *aCount = 0;

  nsCOMPtr<nsIArray> relations;
  nsresult rv = GetRelations(getter_AddRefs(relations));
  NS_ENSURE_SUCCESS(rv, rv);

  return relations->GetLength(aCount);
}

NS_IMETHODIMP
nsAccessible::GetRelation(PRUint32 aIndex, nsIAccessibleRelation **aRelation)
{
  NS_ENSURE_ARG_POINTER(aRelation);
  *aRelation = nsnull;

  nsCOMPtr<nsIArray> relations;
  nsresult rv = GetRelations(getter_AddRefs(relations));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIAccessibleRelation> relation;
  rv = relations->QueryElementAt(aIndex, NS_GET_IID(nsIAccessibleRelation),
                                 getter_AddRefs(relation));

  
  if (rv == NS_ERROR_ILLEGAL_VALUE)
    return NS_ERROR_INVALID_ARG;

  NS_ENSURE_SUCCESS(rv, rv);

  NS_IF_ADDREF(*aRelation = relation);
  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GetRelations(nsIArray **aRelations)
{
  NS_ENSURE_ARG_POINTER(aRelations);

  nsCOMPtr<nsIMutableArray> relations = do_CreateInstance(NS_ARRAY_CONTRACTID);
  NS_ENSURE_TRUE(relations, NS_ERROR_OUT_OF_MEMORY);

  for (PRUint32 relType = nsIAccessibleRelation::RELATION_FIRST;
       relType < nsIAccessibleRelation::RELATION_LAST;
       ++relType) {
    nsCOMPtr<nsIAccessible> accessible;
    GetAccessibleRelated(relType, getter_AddRefs(accessible));

    if (accessible) {
      nsCOMPtr<nsIAccessibleRelation> relation =
        new nsAccessibleRelationWrap(relType, accessible);
      NS_ENSURE_TRUE(relation, NS_ERROR_OUT_OF_MEMORY);

      relations->AppendElement(relation, PR_FALSE);
    }
  }

  NS_ADDREF(*aRelations = relations);
  return NS_OK;
}


NS_IMETHODIMP nsAccessible::ExtendSelection()
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsAccessible::GetNativeInterface(void **aOutAccessible)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

void nsAccessible::DoCommandCallback(nsITimer *aTimer, void *aClosure)
{
  NS_ASSERTION(gDoCommandTimer,
               "How did we get here if there was no gDoCommandTimer?");
  NS_RELEASE(gDoCommandTimer);

  nsCOMPtr<nsIContent> content =
    reinterpret_cast<nsIContent*>(aClosure);

  nsIDocument *doc = content->GetDocument();
  if (!doc)
    return;

  nsCOMPtr<nsIPresShell> presShell = doc->GetPrimaryShell();

  
  presShell->ScrollContentIntoView(content, NS_PRESSHELL_SCROLL_ANYWHERE,
                                   NS_PRESSHELL_SCROLL_ANYWHERE);

  
  PRBool res = nsAccUtils::DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, presShell,
                                              content);
  if (!res)
    return;

  nsAccUtils::DispatchMouseEvent(NS_MOUSE_BUTTON_UP, presShell, content);
}









nsresult nsAccessible::DoCommand(nsIContent *aContent)
{
  nsCOMPtr<nsIContent> content = aContent;
  if (!content) {
    content = do_QueryInterface(mDOMNode);
  }
  if (gDoCommandTimer) {
    
    NS_WARNING("Doubling up on do command timers doesn't work. This wasn't expected.");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsITimer> timer = do_CreateInstance("@mozilla.org/timer;1");
  if (!timer) {
    return NS_ERROR_OUT_OF_MEMORY;
  } 

  NS_ADDREF(gDoCommandTimer = timer);
  return gDoCommandTimer->InitWithFuncCallback(DoCommandCallback,
                                               (void*)content, 0,
                                               nsITimer::TYPE_ONE_SHOT);
}

already_AddRefed<nsIAccessible>
nsAccessible::GetNextWithState(nsIAccessible *aStart, PRUint32 matchState)
{
  
  
  NS_ASSERTION(matchState, "GetNextWithState() not called with a state to match");
  NS_ASSERTION(aStart, "GetNextWithState() not called with an accessible to start with");
  nsCOMPtr<nsIAccessible> look, current = aStart;
  PRUint32 state = 0;
  while (0 == (state & matchState)) {
    current->GetFirstChild(getter_AddRefs(look));
    while (!look) {
      if (current == this) {
        return nsnull; 
      }
      current->GetNextSibling(getter_AddRefs(look));
      if (!look) {
        current->GetParent(getter_AddRefs(look));
        current = look;
        look = nsnull;
        continue;
      }
    }
    current.swap(look);
    state = State(current);
  }

  nsIAccessible *returnAccessible = nsnull;
  current.swap(returnAccessible);

  return returnAccessible;
}


NS_IMETHODIMP nsAccessible::GetSelectedChildren(nsIArray **aSelectedAccessibles)
{
  *aSelectedAccessibles = nsnull;

  nsCOMPtr<nsIMutableArray> selectedAccessibles =
    do_CreateInstance(NS_ARRAY_CONTRACTID);
  NS_ENSURE_STATE(selectedAccessibles);

  nsCOMPtr<nsIAccessible> selected = this;
  while ((selected = GetNextWithState(selected, nsIAccessibleStates::STATE_SELECTED)) != nsnull) {
    selectedAccessibles->AppendElement(selected, PR_FALSE);
  }

  PRUint32 length = 0;
  selectedAccessibles->GetLength(&length); 
  if (length) { 
    *aSelectedAccessibles = selectedAccessibles;
    NS_ADDREF(*aSelectedAccessibles);
  }

  return NS_OK;
}


NS_IMETHODIMP nsAccessible::RefSelection(PRInt32 aIndex, nsIAccessible **aSelected)
{
  *aSelected = nsnull;
  if (aIndex < 0) {
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsIAccessible> selected = this;
  PRInt32 count = 0;
  while (count ++ <= aIndex) {
    selected = GetNextWithState(selected, nsIAccessibleStates::STATE_SELECTED);
    if (!selected) {
      return NS_ERROR_FAILURE; 
    }
  }
  NS_IF_ADDREF(*aSelected = selected);
  return NS_OK;
}

NS_IMETHODIMP nsAccessible::GetSelectionCount(PRInt32 *aSelectionCount)
{
  *aSelectionCount = 0;
  nsCOMPtr<nsIAccessible> selected = this;
  while ((selected = GetNextWithState(selected, nsIAccessibleStates::STATE_SELECTED)) != nsnull) {
    ++ *aSelectionCount;
  }

  return NS_OK;
}

NS_IMETHODIMP nsAccessible::AddChildToSelection(PRInt32 aIndex)
{
  
  
  

  NS_ENSURE_TRUE(aIndex >= 0, NS_ERROR_FAILURE);

  nsCOMPtr<nsIAccessible> child;
  GetChildAt(aIndex, getter_AddRefs(child));

  PRUint32 state = State(child);
  if (!(state & nsIAccessibleStates::STATE_SELECTABLE)) {
    return NS_OK;
  }

  return child->SetSelected(PR_TRUE);
}

NS_IMETHODIMP nsAccessible::RemoveChildFromSelection(PRInt32 aIndex)
{
  
  
  

  NS_ENSURE_TRUE(aIndex >= 0, NS_ERROR_FAILURE);

  nsCOMPtr<nsIAccessible> child;
  GetChildAt(aIndex, getter_AddRefs(child));

  PRUint32 state = State(child);
  if (!(state & nsIAccessibleStates::STATE_SELECTED)) {
    return NS_OK;
  }

  return child->SetSelected(PR_FALSE);
}

NS_IMETHODIMP nsAccessible::IsChildSelected(PRInt32 aIndex, PRBool *aIsSelected)
{
  
  
  

  *aIsSelected = PR_FALSE;
  NS_ENSURE_TRUE(aIndex >= 0, NS_ERROR_FAILURE);

  nsCOMPtr<nsIAccessible> child;
  GetChildAt(aIndex, getter_AddRefs(child));

  PRUint32 state = State(child);
  if (state & nsIAccessibleStates::STATE_SELECTED) {
    *aIsSelected = PR_TRUE;
  }
  return NS_OK;
}

NS_IMETHODIMP nsAccessible::ClearSelection()
{
  nsCOMPtr<nsIAccessible> selected = this;
  while ((selected = GetNextWithState(selected, nsIAccessibleStates::STATE_SELECTED)) != nsnull) {
    selected->SetSelected(PR_FALSE);
  }
  return NS_OK;
}

NS_IMETHODIMP nsAccessible::SelectAllSelection(PRBool *_retval)
{
  nsCOMPtr<nsIAccessible> selectable = this;
  while ((selectable = GetNextWithState(selectable, nsIAccessibleStates::STATE_SELECTED)) != nsnull) {
    selectable->SetSelected(PR_TRUE);
  }
  return NS_OK;
}







NS_IMETHODIMP
nsAccessible::GetAnchorCount(PRInt32 *aAnchorCount)
{
  NS_ENSURE_ARG_POINTER(aAnchorCount);
  *aAnchorCount = 1;
  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetStartIndex(PRInt32 *aStartIndex)
{
  NS_ENSURE_ARG_POINTER(aStartIndex);
  *aStartIndex = 0;
  PRInt32 endIndex;
  return GetLinkOffset(aStartIndex, &endIndex);
}


NS_IMETHODIMP
nsAccessible::GetEndIndex(PRInt32 *aEndIndex)
{
  NS_ENSURE_ARG_POINTER(aEndIndex);
  *aEndIndex = 0;
  PRInt32 startIndex;
  return GetLinkOffset(&startIndex, aEndIndex);
}

NS_IMETHODIMP
nsAccessible::GetURI(PRInt32 aIndex, nsIURI **aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  *aURI = nsnull;

  if (aIndex != 0)
    return NS_ERROR_INVALID_ARG;

  
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (nsAccUtils::IsXLink(content)) {
    nsAutoString href;
    content->GetAttr(kNameSpaceID_XLink, nsAccessibilityAtoms::href, href);

    nsCOMPtr<nsIURI> baseURI = content->GetBaseURI();
    nsCOMPtr<nsIDocument> document = content->GetOwnerDoc();
    return NS_NewURI(aURI, href,
                     document ? document->GetDocumentCharacterSet().get() : nsnull,
                     baseURI);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetAnchor(PRInt32 aIndex,
                        nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;

  if (aIndex != 0)
    return NS_ERROR_INVALID_ARG;

  *aAccessible = this;
  NS_ADDREF_THIS();
  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetValid(PRBool *aValid)
{
  NS_ENSURE_ARG_POINTER(aValid);
  PRUint32 state = State(this);
  *aValid = (0 == (state & nsIAccessibleStates::STATE_INVALID));
  
  
  
  
  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetSelected(PRBool *aSelected)
{
  NS_ENSURE_ARG_POINTER(aSelected);
  *aSelected = (gLastFocusedNode == mDOMNode);
  return NS_OK;
}

nsresult nsAccessible::GetLinkOffset(PRInt32* aStartOffset, PRInt32* aEndOffset)
{
  *aStartOffset = *aEndOffset = 0;
  nsCOMPtr<nsIAccessible> parent(GetParent());
  if (!parent) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIAccessible> accessible, nextSibling;
  PRInt32 characterCount = 0;
  parent->GetFirstChild(getter_AddRefs(accessible));

  while (accessible) {
    if (IsText(accessible)) {
      characterCount += TextLength(accessible);
    }
    else if (accessible == this) {
      *aStartOffset = characterCount;
      *aEndOffset = characterCount + 1;
      return NS_OK;
    }
    else {
      ++ characterCount;
    }
    accessible->GetNextSibling(getter_AddRefs(nextSibling));
    accessible.swap(nextSibling);
  }

  return NS_ERROR_FAILURE;
}

PRInt32 nsAccessible::TextLength(nsIAccessible *aAccessible)
{
  if (!IsText(aAccessible))
    return 1;

  nsCOMPtr<nsPIAccessNode> pAccNode(do_QueryInterface(aAccessible));
  NS_ASSERTION(pAccNode, "QI to nsPIAccessNode failed");

  nsIFrame *frame = pAccNode->GetFrame();
  if (frame && frame->GetType() == nsAccessibilityAtoms::textFrame) {
    
    nsIContent *content = frame->GetContent();
    if (content) {
      PRUint32 length;
      nsresult rv = nsHyperTextAccessible::ContentToRenderedOffset(frame, content->TextLength(), &length);
      return NS_SUCCEEDED(rv) ? static_cast<PRInt32>(length) : -1;
    }
  }

  
  
  
  
  nsCOMPtr<nsPIAccessible> pAcc(do_QueryInterface(aAccessible));
  NS_ASSERTION(pAcc, "QI to nsPIAccessible failed");

  nsAutoString text;
  pAcc->AppendTextTo(text, 0, PR_UINT32_MAX); 
  return text.Length();
}

NS_IMETHODIMP
nsAccessible::AppendTextTo(nsAString& aText, PRUint32 aStartOffset, PRUint32 aLength)
{
  return NS_OK;
}




nsresult
nsAccessible::GetARIAName(nsAString& aName)
{
  nsCOMPtr<nsIContent> content = GetRoleContent(mDOMNode);
  if (!content)
    return NS_OK;

  
  nsAutoString label;
  if (content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_label, label)) {
    aName = label;
    return NS_OK;
  }
  
  
  nsresult rv = GetTextFromRelationID(nsAccessibilityAtoms::aria_labelledby, label);
  if (NS_SUCCEEDED(rv))
    aName = label;

  return rv;
}

nsresult
nsAccessible::GetNameInternal(nsAString& aName)
{
  PRBool canAggregateName = mRoleMapEntry &&
    mRoleMapEntry->nameRule == eNameOkFromChildren;

  nsCOMPtr<nsIContent> content = GetRoleContent(mDOMNode);
  if (!content)
    return NS_OK;

  if (content->IsNodeOfType(nsINode::eHTML))
    return GetHTMLName(aName, canAggregateName);

  if (content->IsNodeOfType(nsINode::eXUL))
    return GetXULName(aName, canAggregateName);

  return NS_OK;
}




already_AddRefed<nsIAccessible>
nsAccessible::GetFirstAvailableAccessible(nsIDOMNode *aStartNode, PRBool aRequireLeaf)
{
  nsIAccessibilityService *accService = GetAccService();
  nsCOMPtr<nsIAccessible> accessible;
  nsCOMPtr<nsIDOMTreeWalker> walker; 
  nsCOMPtr<nsIDOMNode> currentNode(aStartNode);

  while (currentNode) {
    accService->GetAccessibleInWeakShell(currentNode, mWeakShell, getter_AddRefs(accessible)); 
    if (accessible && (!aRequireLeaf || IsLeaf(accessible))) {
      nsIAccessible *retAccessible = accessible;
      NS_ADDREF(retAccessible);
      return retAccessible;
    }
    if (!walker) {
      
      
      nsCOMPtr<nsIDOMDocument> document;
      currentNode->GetOwnerDocument(getter_AddRefs(document));
      nsCOMPtr<nsIDOMDocumentTraversal> trav = do_QueryInterface(document);
      NS_ASSERTION(trav, "No DOM document traversal for document");
      NS_ENSURE_TRUE(trav, nsnull);
      trav->CreateTreeWalker(mDOMNode, nsIDOMNodeFilter::SHOW_ELEMENT | nsIDOMNodeFilter::SHOW_TEXT,
                            nsnull, PR_FALSE, getter_AddRefs(walker));
      NS_ENSURE_TRUE(walker, nsnull);
      walker->SetCurrentNode(currentNode);
    }

    walker->NextNode(getter_AddRefs(currentNode));
  }

  return nsnull;
}

PRBool nsAccessible::CheckVisibilityInParentChain(nsIDocument* aDocument, nsIView* aView)
{
  nsIDocument* document = aDocument;
  nsIView* view = aView;
  
  while (document != nsnull) {
    while (view != nsnull) {
      if (view->GetVisibility() == nsViewVisibility_kHide) {
        return PR_FALSE;
      }
      view = view->GetParent();
    }

    nsIDocument* parentDoc = document->GetParentDocument();
    if (parentDoc != nsnull) {
      nsIContent* content = parentDoc->FindContentForSubDocument(document);
      if (content != nsnull) {
        nsIPresShell* shell = parentDoc->GetPrimaryShell();
        if (!shell) {
          return PR_FALSE;
        }
        nsIFrame* frame = shell->GetPrimaryFrameFor(content);
        while (frame != nsnull && !frame->HasView()) {
          frame = frame->GetParent();
        }

        if (frame != nsnull) {
          view = frame->GetViewExternal();
        }
      }
    }

    document = parentDoc;
  }

  return PR_TRUE;
}

nsresult
nsAccessible::GetAttrValue(nsIAtom *aProperty, double *aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);
  *aValue = 0;

  if (!mDOMNode)
    return NS_ERROR_FAILURE;  

 if (!mRoleMapEntry || mRoleMapEntry->valueRule == eNoValue)
    return NS_OK_NO_ARIA_VALUE;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  NS_ENSURE_STATE(content);

  PRInt32 result = NS_OK;
  nsAutoString value;
  if (content->GetAttr(kNameSpaceID_None, aProperty, value))
    *aValue = value.ToFloat(&result);

  return result;
}

PRUint32
nsAccessible::GetActionRule(PRUint32 aStates)
{
  if (aStates & nsIAccessibleStates::STATE_UNAVAILABLE)
    return eNoAction;

  
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (nsAccUtils::IsXLink(content))
    return eJumpAction;

  
  PRBool isOnclick = nsAccUtils::HasListener(content,
                                             NS_LITERAL_STRING("click"));

  if (isOnclick)
    return eClickAction;

  
  if (mRoleMapEntry)
    return mRoleMapEntry->actionRule;

  return eNoAction;
}

PRBool nsAccessible::MustPrune(nsIAccessible *aAccessible)
{ 
  PRUint32 role = Role(aAccessible);
  return role == nsIAccessibleRole::ROLE_MENUITEM || 
         role == nsIAccessibleRole::ROLE_COMBOBOX_OPTION ||
         role == nsIAccessibleRole::ROLE_OPTION ||
         role == nsIAccessibleRole::ROLE_ENTRY ||
         role == nsIAccessibleRole::ROLE_FLAT_EQUATION ||
         role == nsIAccessibleRole::ROLE_PASSWORD_TEXT ||
         role == nsIAccessibleRole::ROLE_PUSHBUTTON ||
         role == nsIAccessibleRole::ROLE_TOGGLE_BUTTON ||
         role == nsIAccessibleRole::ROLE_GRAPHIC ||
         role == nsIAccessibleRole::ROLE_SLIDER ||
         role == nsIAccessibleRole::ROLE_PROGRESSBAR ||
         role == nsIAccessibleRole::ROLE_SEPARATOR;
}
