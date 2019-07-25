






































#include "nsAccessible.h"

#include "nsIXBLAccessible.h"

#include "AccGroupInfo.h"
#include "AccIterator.h"
#include "nsAccUtils.h"
#include "nsARIAMap.h"
#include "nsDocAccessible.h"
#include "nsEventShell.h"

#include "nsAccessibilityService.h"
#include "nsAccTreeWalker.h"
#include "nsRelUtils.h"
#include "nsTextEquivUtils.h"

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
#include "nsIView.h"
#include "nsIDocShellTreeItem.h"
#include "nsIScrollableFrame.h"
#include "nsFocusManager.h"

#include "nsXPIDLString.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "prdtoa.h"
#include "nsIAtom.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIURI.h"
#include "nsArrayUtils.h"
#include "nsIMutableArray.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsWhitespaceTokenizer.h"
#include "nsAttrName.h"
#include "nsNetUtil.h"

#ifdef NS_DEBUG
#include "nsIDOMCharacterData.h"
#endif





NS_IMPL_CYCLE_COLLECTION_CLASS(nsAccessible)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsAccessible, nsAccessNode)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mParent");
  cb.NoteXPCOMChild(static_cast<nsIAccessible*>(tmp->mParent.get()));

  PRUint32 i, length = tmp->mChildren.Length();
  for (i = 0; i < length; ++i) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mChildren[i]");
    cb.NoteXPCOMChild(static_cast<nsIAccessible*>(tmp->mChildren[i].get()));
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsAccessible, nsAccessNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mParent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSTARRAY(mChildren)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(nsAccessible, nsAccessNode)
NS_IMPL_RELEASE_INHERITED(nsAccessible, nsAccessNode)

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

  if (aIID.Equals(NS_GET_IID(nsAccessible))) {
    *aInstancePtr = static_cast<nsAccessible*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if (aIID.Equals(NS_GET_IID(nsIAccessibleSelectable))) {
    if (mRoleMapEntry &&
        (mRoleMapEntry->attributeMap1 == eARIAMultiSelectable ||
         mRoleMapEntry->attributeMap2 == eARIAMultiSelectable ||
         mRoleMapEntry->attributeMap3 == eARIAMultiSelectable)) {

      
      
      
      

      *aInstancePtr = static_cast<nsIAccessibleSelectable*>(this);
      NS_ADDREF_THIS();
      return NS_OK;
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
    
    
    nsCOMPtr<nsIAccessibleHyperText> hyperTextParent = do_QueryObject(GetParent());
    if (hyperTextParent && nsAccUtils::IsEmbeddedObject(this)) {
      *aInstancePtr = static_cast<nsIAccessibleHyperLink*>(this);
      NS_ADDREF_THIS();
      return NS_OK;
    }
    return NS_ERROR_NO_INTERFACE;
  }

  return nsAccessNodeWrap::QueryInterface(aIID, aInstancePtr);
}

nsAccessible::nsAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessNodeWrap(aContent, aShell),
  mParent(nsnull), mAreChildrenInitialized(PR_FALSE), mIndexInParent(-1),
  mRoleMapEntry(nsnull)
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

void
nsAccessible::SetRoleMapEntry(nsRoleMapEntry* aRoleMapEntry)
{
  mRoleMapEntry = aRoleMapEntry;
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

  nsCOMPtr<nsIXBLAccessible> xblAccessible(do_QueryInterface(mContent));
  if (xblAccessible) {
    xblAccessible->GetAccessibleName(aName);
    if (!aName.IsEmpty())
      return NS_OK;
  }

  nsresult rv = GetNameInternal(aName);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aName.IsEmpty())
    return NS_OK;

  
  nsIAtom *tooltipAttr = nsnull;

  if (mContent->IsHTML())
    tooltipAttr = nsAccessibilityAtoms::title;
  else if (mContent->IsXUL())
    tooltipAttr = nsAccessibilityAtoms::tooltiptext;
  else
    return NS_OK;

  
  nsAutoString name;
  if (mContent->GetAttr(kNameSpaceID_None, tooltipAttr, name)) {
    name.CompressWhitespace();
    aName = name;
    return NS_OK_NAME_FROM_TOOLTIP;
  }

  if (rv != NS_OK_EMPTY_NAME)
    aName.SetIsVoid(PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP nsAccessible::GetDescription(nsAString& aDescription)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  
  
  
  

  if (!mContent->IsNodeOfType(nsINode::eTEXT)) {
    nsAutoString description;
    nsresult rv = nsTextEquivUtils::
      GetTextEquivFromIDRefs(this, nsAccessibilityAtoms::aria_describedby,
                             description);
    NS_ENSURE_SUCCESS(rv, rv);

    if (description.IsEmpty()) {
      PRBool isXUL = mContent->IsXUL();
      if (isXUL) {
        
        nsIContent *descriptionContent =
          nsCoreUtils::FindNeighbourPointingToNode(mContent,
                                                   nsAccessibilityAtoms::control,
                                                   nsAccessibilityAtoms::description);

        if (descriptionContent) {
          
          nsTextEquivUtils::
            AppendTextEquivFromContent(this, descriptionContent, &description);
        }
      }
      if (description.IsEmpty()) {
        nsIAtom *descAtom = isXUL ? nsAccessibilityAtoms::tooltiptext :
                                    nsAccessibilityAtoms::title;
        if (mContent->GetAttr(kNameSpaceID_None, descAtom, description)) {
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

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRUint32 key = nsCoreUtils::GetAccessKeyFor(mContent);
  if (!key && mContent->IsElement()) {
    
    
    nsCOMPtr<nsIContent> labelContent(nsCoreUtils::GetLabelContent(mContent));
    if (labelContent && !nsCoreUtils::IsAncestorOf(labelContent, mContent))
      key = nsCoreUtils::GetAccessKeyFor(labelContent);
  }

  if (!key)
    return NS_OK;

  nsAutoString accesskey(key);

  
  nsAutoString propertyKey;
  PRInt32 modifierMask = GetAccessModifierMask(mContent);
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

NS_IMETHODIMP
nsAccessible::GetParent(nsIAccessible **aParent)
{
  NS_ENSURE_ARG_POINTER(aParent);

  NS_IF_ADDREF(*aParent = GetParent());
  return *aParent ? NS_OK : NS_ERROR_FAILURE;
}

  
NS_IMETHODIMP
nsAccessible::GetNextSibling(nsIAccessible **aNextSibling) 
{
  NS_ENSURE_ARG_POINTER(aNextSibling);

  nsresult rv = NS_OK;
  NS_IF_ADDREF(*aNextSibling = GetSiblingAtOffset(1, &rv));
  return rv;
}

  
NS_IMETHODIMP
nsAccessible::GetPreviousSibling(nsIAccessible * *aPreviousSibling) 
{
  NS_ENSURE_ARG_POINTER(aPreviousSibling);

  nsresult rv = NS_OK;
  NS_IF_ADDREF(*aPreviousSibling = GetSiblingAtOffset(-1, &rv));
  return rv;
}

  
NS_IMETHODIMP
nsAccessible::GetFirstChild(nsIAccessible **aFirstChild) 
{
  NS_ENSURE_ARG_POINTER(aFirstChild);
  *aFirstChild = nsnull;

  if (gIsCacheDisabled)
    InvalidateChildren();

  PRInt32 childCount = GetChildCount();
  NS_ENSURE_TRUE(childCount != -1, NS_ERROR_FAILURE);

  if (childCount > 0)
    NS_ADDREF(*aFirstChild = GetChildAt(0));

  return NS_OK;
}

  
NS_IMETHODIMP
nsAccessible::GetLastChild(nsIAccessible **aLastChild)
{
  NS_ENSURE_ARG_POINTER(aLastChild);
  *aLastChild = nsnull;

  PRInt32 childCount = GetChildCount();
  NS_ENSURE_TRUE(childCount != -1, NS_ERROR_FAILURE);

  NS_IF_ADDREF(*aLastChild = GetChildAt(childCount - 1));
  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GetChildAt(PRInt32 aChildIndex, nsIAccessible **aChild)
{
  NS_ENSURE_ARG_POINTER(aChild);
  *aChild = nsnull;

  PRInt32 childCount = GetChildCount();
  NS_ENSURE_TRUE(childCount != -1, NS_ERROR_FAILURE);

  
  
  if (aChildIndex < 0)
    aChildIndex = childCount - 1;

  nsAccessible* child = GetChildAt(aChildIndex);
  if (!child)
    return NS_ERROR_INVALID_ARG;

  NS_ADDREF(*aChild = child);
  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetChildren(nsIArray **aOutChildren)
{
  NS_ENSURE_ARG_POINTER(aOutChildren);
  *aOutChildren = nsnull;

  PRInt32 childCount = GetChildCount();
  NS_ENSURE_TRUE(childCount != -1, NS_ERROR_FAILURE);

  nsresult rv = NS_OK;
  nsCOMPtr<nsIMutableArray> children =
    do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsIAccessible* child = GetChildAt(childIdx);
    children->AppendElement(child, PR_FALSE);
  }

  NS_ADDREF(*aOutChildren = children);
  return NS_OK;
}

PRBool
nsAccessible::GetAllowsAnonChildAccessibles()
{
  return PR_TRUE;
}


NS_IMETHODIMP
nsAccessible::GetChildCount(PRInt32 *aChildCount) 
{
  NS_ENSURE_ARG_POINTER(aChildCount);

  *aChildCount = GetChildCount();
  return *aChildCount != -1 ? NS_OK : NS_ERROR_FAILURE;  
}


NS_IMETHODIMP
nsAccessible::GetIndexInParent(PRInt32 *aIndexInParent)
{
  NS_ENSURE_ARG_POINTER(aIndexInParent);

  *aIndexInParent = GetIndexInParent();
  return *aIndexInParent != -1 ? NS_OK : NS_ERROR_FAILURE;
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
  if (IsDefunct())
    return PR_FALSE;

  const PRUint16 kMinPixels  = 12;
   
  nsCOMPtr<nsIPresShell> shell(GetPresShell());
  if (!shell) 
    return PR_FALSE;

  nsIFrame *frame = GetFrame();
  if (!frame) {
    return PR_FALSE;
  }

  
  if (!frame->GetStyleVisibility()->IsVisible())
  {
      return PR_FALSE;
  }

  
  
  
  nsSize frameSize = frame->GetSize();
  nsRectVisibility rectVisibility =
    shell->GetRectVisibility(frame, nsRect(nsPoint(0,0), frameSize),
                             nsPresContext::CSSPixelsToAppUnits(kMinPixels));

  if (frame->GetRect().IsEmpty()) {
    PRBool isEmpty = PR_TRUE;

    nsIAtom *frameType = frame->GetType();
    if (frameType == nsAccessibilityAtoms::textFrame) {
      
      
      nsAutoString renderedText;
      frame->GetRenderedText (&renderedText, nsnull, nsnull, 0, 1);
      isEmpty = renderedText.IsEmpty();
    }
    else if (frameType == nsAccessibilityAtoms::inlineFrame) {
      
      
      
      PRInt32 x, y, width, height;
      GetBounds(&x, &y, &width, &height);
      isEmpty = width == 0 || height == 0;
    }

    if (isEmpty && !(frame->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
      
      
      return PR_FALSE;
    }
  }

  
  nsIDocument* doc = mContent->GetOwnerDoc();
  if (!doc)  {
    return PR_FALSE;
  }

  nsIFrame* frameWithView =
    frame->HasView() ? frame : frame->GetAncestorWithViewExternal();
  nsIView* view = frameWithView->GetViewExternal();
  PRBool isVisible = CheckVisibilityInParentChain(doc, view);
  if (isVisible && rectVisibility == nsRectVisibility_kVisible) {
    *aIsOffscreen = PR_FALSE;
  }
  return isVisible;
}

nsresult
nsAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  *aState = 0;

  if (IsDefunct()) {
    if (aExtraState)
      *aExtraState = nsIAccessibleStates::EXT_STATE_DEFUNCT;

    return NS_OK_DEFUNCT_OBJECT;
  }

  if (aExtraState)
    *aExtraState = 0;

  
  
  
  
  PRBool isDisabled;
  if (mContent->IsHTML()) {
    
    
    isDisabled = mContent->HasAttr(kNameSpaceID_None,
                                   nsAccessibilityAtoms::disabled);
  }
  else {
    isDisabled = mContent->AttrValueIs(kNameSpaceID_None,
                                       nsAccessibilityAtoms::disabled,
                                       nsAccessibilityAtoms::_true,
                                       eCaseMatters);
  }
  if (isDisabled) {
    *aState |= nsIAccessibleStates::STATE_UNAVAILABLE;
  }
  else if (mContent->IsElement()) {
    nsIFrame *frame = GetFrame();
    if (frame && frame->IsFocusable()) {
      *aState |= nsIAccessibleStates::STATE_FOCUSABLE;
    }

    if (gLastFocusedNode == mContent) {
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

  
  if (mContent->IsXUL())
    if (mContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::popup))
      *aState |= nsIAccessibleStates::STATE_HASPOPUP;

  
  if (nsCoreUtils::IsXLink(mContent))
    *aState |= nsIAccessibleStates::STATE_LINKED;

  return NS_OK;
}

  
NS_IMETHODIMP
nsAccessible::GetFocusedChild(nsIAccessible **aFocusedChild) 
{ 
  nsAccessible *focusedChild = nsnull;
  if (gLastFocusedNode == mContent) {
    focusedChild = this;
  }
  else if (gLastFocusedNode) {
    focusedChild = GetAccService()->GetAccessible(gLastFocusedNode);
    if (focusedChild && focusedChild->GetParent() != this)
      focusedChild = nsnull;
  }

  NS_IF_ADDREF(*aFocusedChild = focusedChild);
  return NS_OK;
}


nsresult
nsAccessible::GetChildAtPoint(PRInt32 aX, PRInt32 aY, PRBool aDeepestChild,
                              nsIAccessible **aChild)
{
  
  
  PRInt32 x = 0, y = 0, width = 0, height = 0;
  nsresult rv = GetBounds(&x, &y, &width, &height);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIAccessible> fallbackAnswer;
  if (aX >= x && aX < x + width && aY >= y && aY < y + height)
    fallbackAnswer = this;

  if (nsAccUtils::MustPrune(this)) {  
    NS_IF_ADDREF(*aChild = fallbackAnswer);
    return NS_OK;
  }

  
  
  
  
  
  
  nsDocAccessible *accDocument = GetDocAccessible();
  NS_ENSURE_TRUE(accDocument, NS_ERROR_FAILURE);

  nsIFrame *frame = accDocument->GetFrame();
  NS_ENSURE_STATE(frame);

  nsPresContext *presContext = frame->PresContext();

  nsIntRect screenRect = frame->GetScreenRectExternal();
  nsPoint offset(presContext->DevPixelsToAppUnits(aX - screenRect.x),
                 presContext->DevPixelsToAppUnits(aY - screenRect.y));

  nsCOMPtr<nsIPresShell> presShell = presContext->PresShell();
  nsIFrame *foundFrame = presShell->GetFrameForPoint(frame, offset);

  nsIContent* content = nsnull;
  if (!foundFrame || !(content = foundFrame->GetContent())) {
    NS_IF_ADDREF(*aChild = fallbackAnswer);
    return NS_OK;
  }

  
  
  nsAccessible* accessible =
   GetAccService()->GetAccessibleOrContainer(content, mWeakShell);
  if (!accessible) {
    NS_IF_ADDREF(*aChild = fallbackAnswer);
    return NS_OK;
  }

  if (accessible == this) {
    
    
    
    
    
    PRInt32 childCount = GetChildCount();
    for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
      nsAccessible *child = GetChildAt(childIdx);

      PRInt32 childX, childY, childWidth, childHeight;
      child->GetBounds(&childX, &childY, &childWidth, &childHeight);
      if (aX >= childX && aX < childX + childWidth &&
          aY >= childY && aY < childY + childHeight &&
          (nsAccUtils::State(child) & nsIAccessibleStates::STATE_INVISIBLE) == 0) {

        if (aDeepestChild)
          return child->GetDeepestChildAtPoint(aX, aY, aChild);

        NS_IF_ADDREF(*aChild = child);
        return NS_OK;
      }
    }

    
    
    NS_IF_ADDREF(*aChild = accessible);
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIAccessible> parent, child(accessible);
  while (PR_TRUE) {
    child->GetParent(getter_AddRefs(parent));
    if (!parent) {
      
      
      NS_IF_ADDREF(*aChild = fallbackAnswer);      
      return NS_OK;
    }

    if (parent == this) {
      NS_ADDREF(*aChild = (aDeepestChild ? accessible : child));
      return NS_OK;
    }
    child.swap(parent);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                              nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  return GetChildAtPoint(aX, aY, PR_FALSE, aAccessible);
}


NS_IMETHODIMP
nsAccessible::GetDeepestChildAtPoint(PRInt32 aX, PRInt32 aY,
                                     nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  return GetChildAtPoint(aX, aY, PR_TRUE, aAccessible);
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
    
    
    if (!nsCoreUtils::IsCorrectFrameType(ancestorFrame,
                                         nsAccessibilityAtoms::inlineFrame) &&
        !nsCoreUtils::IsCorrectFrameType(ancestorFrame,
                                         nsAccessibilityAtoms::textFrame))
      break;
    ancestorFrame = ancestorFrame->GetParent();
  }

  nsIFrame *iterFrame = firstFrame;
  nsCOMPtr<nsIContent> firstContent(mContent);
  nsIContent* iterContent = firstContent;
  PRInt32 depth = 0;

  
  while (iterContent == firstContent || depth > 0) {
    
    nsRect currFrameBounds = iterFrame->GetRect();
    
    
    currFrameBounds +=
      iterFrame->GetParent()->GetOffsetToExternal(*aBoundingFrame);

    
    aTotalBounds.UnionRect(aTotalBounds, currFrameBounds);

    nsIFrame *iterNextFrame = nsnull;

    if (nsCoreUtils::IsCorrectFrameType(iterFrame,
                                        nsAccessibilityAtoms::inlineFrame)) {
      
      
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

  

  nsIntRect orgRectPixels = aBoundingFrame->GetScreenRectExternal();
  *x += orgRectPixels.x;
  *y += orgRectPixels.y;

  return NS_OK;
}



nsIFrame* nsAccessible::GetBoundsFrame()
{
  return GetFrame();
}


NS_IMETHODIMP nsAccessible::SetSelected(PRBool aSelect)
{
  
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRUint32 state = nsAccUtils::State(this);
  if (state & nsIAccessibleStates::STATE_SELECTABLE) {
    nsCOMPtr<nsIAccessible> multiSelect =
      nsAccUtils::GetMultiSelectableContainer(mContent);
    if (!multiSelect) {
      return aSelect ? TakeFocus() : NS_ERROR_FAILURE;
    }

    if (mRoleMapEntry) {
      if (aSelect) {
        return mContent->SetAttr(kNameSpaceID_None,
                                 nsAccessibilityAtoms::aria_selected,
                                 NS_LITERAL_STRING("true"), PR_TRUE);
      }
      return mContent->UnsetAttr(kNameSpaceID_None,
                                 nsAccessibilityAtoms::aria_selected, PR_TRUE);
    }
  }

  return NS_OK;
}


NS_IMETHODIMP nsAccessible::TakeSelection()
{
  
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRUint32 state = nsAccUtils::State(this);
  if (state & nsIAccessibleStates::STATE_SELECTABLE) {
    nsCOMPtr<nsIAccessible> multiSelect =
      nsAccUtils::GetMultiSelectableContainer(mContent);
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

  nsIFrame *frame = GetFrame();
  NS_ENSURE_STATE(frame);

  nsIContent* focusContent = mContent;

  
  
  
  
  if (!frame->IsFocusable()) {
    nsAutoString id;
    if (nsCoreUtils::GetID(mContent, id)) {

      nsIContent* ancestorContent = mContent;
      while ((ancestorContent = ancestorContent->GetParent()) &&
             !ancestorContent->HasAttr(kNameSpaceID_None,
                                       nsAccessibilityAtoms::aria_activedescendant));

      if (ancestorContent) {
        nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));
        if (presShell) {
          nsIFrame *frame = ancestorContent->GetPrimaryFrame();
          if (frame && frame->IsFocusable()) {
            focusContent = ancestorContent;
            focusContent->SetAttr(kNameSpaceID_None,
                                  nsAccessibilityAtoms::aria_activedescendant,
                                  id, PR_TRUE);
          }
        }
      }
    }
  }

  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(focusContent));
  nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
  if (fm)
    fm->SetFocus(element, 0);

  return NS_OK;
}

nsresult
nsAccessible::GetHTMLName(nsAString& aLabel)
{
  nsIContent *labelContent = nsCoreUtils::GetHTMLLabelContent(mContent);
  if (labelContent) {
    nsAutoString label;
    nsresult rv =
      nsTextEquivUtils::AppendTextEquivFromContent(this, labelContent, &label);
    NS_ENSURE_SUCCESS(rv, rv);

    label.CompressWhitespace();
    if (!label.IsEmpty()) {
      aLabel = label;
      return NS_OK;
    }
  }

  return nsTextEquivUtils::GetNameFromSubtree(this, aLabel);
}













nsresult
nsAccessible::GetXULName(nsAString& aLabel)
{
  
  nsresult rv = NS_OK;

  nsAutoString label;
  nsCOMPtr<nsIDOMXULLabeledControlElement> labeledEl(do_QueryInterface(mContent));
  if (labeledEl) {
    rv = labeledEl->GetLabel(label);
  }
  else {
    nsCOMPtr<nsIDOMXULSelectControlItemElement> itemEl(do_QueryInterface(mContent));
    if (itemEl) {
      rv = itemEl->GetLabel(label);
    }
    else {
      nsCOMPtr<nsIDOMXULSelectControlElement> select(do_QueryInterface(mContent));
      
      
      if (!select) {
        nsCOMPtr<nsIDOMXULElement> xulEl(do_QueryInterface(mContent));
        if (xulEl) {
          rv = xulEl->GetAttribute(NS_LITERAL_STRING("label"), label);
        }
      }
    }
  }

  
  if (NS_FAILED(rv) || label.IsEmpty()) {
    label.Truncate();
    nsIContent *labelContent =
      nsCoreUtils::FindNeighbourPointingToNode(mContent,
                                               nsAccessibilityAtoms::control,
                                               nsAccessibilityAtoms::label);

    nsCOMPtr<nsIDOMXULLabelElement> xulLabel(do_QueryInterface(labelContent));
    
    if (xulLabel && NS_SUCCEEDED(xulLabel->GetValue(label)) && label.IsEmpty()) {
      
      
      nsTextEquivUtils::AppendTextEquivFromContent(this, labelContent, &label);
    }
  }

  
  label.CompressWhitespace();
  if (!label.IsEmpty()) {
    aLabel = label;
    return NS_OK;
  }

  
  nsIContent *bindingParent = mContent->GetBindingParent();
  nsIContent *parent = bindingParent? bindingParent->GetParent() :
                                      mContent->GetParent();
  while (parent) {
    if (parent->Tag() == nsAccessibilityAtoms::toolbaritem &&
        parent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::title, label)) {
      label.CompressWhitespace();
      aLabel = label;
      return NS_OK;
    }
    parent = parent->GetParent();
  }

  return nsTextEquivUtils::GetNameFromSubtree(this, aLabel);
}

nsresult
nsAccessible::HandleAccEvent(nsAccEvent *aEvent)
{
  NS_ENSURE_ARG_POINTER(aEvent);

  nsCOMPtr<nsIObserverService> obsService =
    mozilla::services::GetObserverService();
  NS_ENSURE_TRUE(obsService, NS_ERROR_FAILURE);

  return obsService->NotifyObservers(aEvent, NS_ACCESSIBLE_EVENT_TOPIC, nsnull);
}

NS_IMETHODIMP
nsAccessible::GetRole(PRUint32 *aRole)
{
  NS_ENSURE_ARG_POINTER(aRole);
  *aRole = nsIAccessibleRole::ROLE_NOTHING;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (mRoleMapEntry) {
    *aRole = mRoleMapEntry->role;

    
    
    if (*aRole == nsIAccessibleRole::ROLE_PUSHBUTTON) {

      if (nsAccUtils::HasDefinedARIAToken(mContent,
                                          nsAccessibilityAtoms::aria_pressed)) {
        
        
        *aRole = nsIAccessibleRole::ROLE_TOGGLE_BUTTON;

      } else if (mContent->AttrValueIs(kNameSpaceID_None,
                                       nsAccessibilityAtoms::aria_haspopup,
                                       nsAccessibilityAtoms::_true,
                                       eCaseMatters)) {
        
        *aRole = nsIAccessibleRole::ROLE_BUTTONMENU;
      }
    }
    else if (*aRole == nsIAccessibleRole::ROLE_LISTBOX) {
      
      nsCOMPtr<nsIAccessible> possibleCombo;
      GetParent(getter_AddRefs(possibleCombo));
      if (nsAccUtils::Role(possibleCombo) == nsIAccessibleRole::ROLE_COMBOBOX) {
        *aRole = nsIAccessibleRole::ROLE_COMBOBOX_LIST;
      }
      else {   
        possibleCombo = nsRelUtils::
          GetRelatedAccessible(this, nsIAccessibleRelation::RELATION_NODE_CHILD_OF);
        if (nsAccUtils::Role(possibleCombo) == nsIAccessibleRole::ROLE_COMBOBOX)
          *aRole = nsIAccessibleRole::ROLE_COMBOBOX_LIST;
      }
    }
    else if (*aRole == nsIAccessibleRole::ROLE_OPTION) {
      if (nsAccUtils::Role(GetParent()) == nsIAccessibleRole::ROLE_COMBOBOX_LIST)
        *aRole = nsIAccessibleRole::ROLE_COMBOBOX_OPTION;
    }

    
    if (mRoleMapEntry->roleRule == kUseMapRole)
      return NS_OK;
  }

  return GetRoleInternal(aRole);
}

NS_IMETHODIMP
nsAccessible::GetAttributes(nsIPersistentProperties **aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);  
  
  if (IsDefunct())
    return NS_ERROR_FAILURE;

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
  if (nsCoreUtils::GetID(mContent, id)) {
    
    
    attributes->SetStringProperty(NS_LITERAL_CSTRING("id"), id, oldValueUnused);
  }
  
  nsAutoString xmlRoles;
  if (mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::role, xmlRoles)) {
    attributes->SetStringProperty(NS_LITERAL_CSTRING("xml-roles"),  xmlRoles, oldValueUnused);          
  }

  nsCOMPtr<nsIAccessibleValue> supportsValue = do_QueryInterface(static_cast<nsIAccessible*>(this));
  if (supportsValue) {
    
    
    
    nsAutoString valuetext;
    GetValue(valuetext);
    attributes->SetStringProperty(NS_LITERAL_CSTRING("valuetext"), valuetext, oldValueUnused);
  }

  
  if (nsAccUtils::State(this) & nsIAccessibleStates::STATE_CHECKABLE)
    nsAccUtils::SetAccAttr(attributes, nsAccessibilityAtoms::checkable, NS_LITERAL_STRING("true"));

  
  PRInt32 level = 0, posInSet = 0, setSize = 0;
  rv = GroupPosition(&level, &setSize, &posInSet);
  if (NS_SUCCEEDED(rv))
    nsAccUtils::SetAccGroupAttrs(attributes, level, setSize, posInSet);

  
  PRUint32 numAttrs = mContent->GetAttrCount();
  for (PRUint32 count = 0; count < numAttrs; count ++) {
    const nsAttrName *attr = mContent->GetAttrNameAt(count);
    if (attr && attr->NamespaceEquals(kNameSpaceID_None)) {
      nsIAtom *attrAtom = attr->Atom();
      nsDependentAtomString attrStr(attrAtom);
      if (!StringBeginsWith(attrStr, NS_LITERAL_STRING("aria-"))) 
        continue; 
      PRUint8 attrFlags = nsAccUtils::GetAttributeCharacteristics(attrAtom);
      if (attrFlags & ATTR_BYPASSOBJ)
        continue; 
      if ((attrFlags & ATTR_VALTOKEN) &&
          !nsAccUtils::HasDefinedARIAToken(mContent, attrAtom))
        continue; 
      nsAutoString value;
      if (mContent->GetAttr(kNameSpaceID_None, attrAtom, value)) {
        attributes->SetStringProperty(NS_ConvertUTF16toUTF8(Substring(attrStr, 5)), value, oldValueUnused);
      }
    }
  }

  
  
  if (mRoleMapEntry) {
    nsAutoString live;
    nsAccUtils::GetAccAttr(attributes, nsAccessibilityAtoms::live, live);
    if (live.IsEmpty()) {
      if (nsAccUtils::GetLiveAttrValue(mRoleMapEntry->liveAttRule, live))
        nsAccUtils::SetAccAttr(attributes, nsAccessibilityAtoms::live, live);
    }
  }

  return NS_OK;
}

nsresult
nsAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  
  
  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(mContent));

  nsAutoString tagName;
  element->GetTagName(tagName);
  if (!tagName.IsEmpty()) {
    nsAutoString oldValueUnused;
    aAttributes->SetStringProperty(NS_LITERAL_CSTRING("tag"), tagName,
                                   oldValueUnused);
  }

  nsEventShell::GetEventAttributes(GetNode(), aAttributes);
 
  
  
  nsAutoString _class;
  if (mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::_class, _class))
    nsAccUtils::SetAccAttr(aAttributes, nsAccessibilityAtoms::_class, _class);

  
  
  
  
  
  
  nsIContent *startContent = mContent;
  while (PR_TRUE) {
    NS_ENSURE_STATE(startContent);
    nsIDocument *doc = startContent->GetDocument();
    nsIContent* rootContent = nsCoreUtils::GetRoleContent(doc);
    NS_ENSURE_STATE(rootContent);
    nsAccUtils::SetLiveContainerAttributes(aAttributes, startContent,
                                           rootContent);

    
    nsCOMPtr<nsISupports> container = doc->GetContainer(); 
    nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
      do_QueryInterface(container);
    if (!docShellTreeItem)
      break;

    nsCOMPtr<nsIDocShellTreeItem> sameTypeParent;
    docShellTreeItem->GetSameTypeParent(getter_AddRefs(sameTypeParent));
    if (!sameTypeParent || sameTypeParent == docShellTreeItem)
      break;

    nsIDocument *parentDoc = doc->GetParentDocument();
    if (!parentDoc)
      break;

    startContent = parentDoc->FindContentForSubDocument(doc);      
  }

  
  nsAutoString value;
  nsresult rv = GetComputedStyleValue(EmptyString(),
                                      NS_LITERAL_STRING("display"),
                                      value);
  if (NS_SUCCEEDED(rv))
    nsAccUtils::SetAccAttr(aAttributes, nsAccessibilityAtoms::display,
                           value);

  
  rv = GetComputedStyleValue(EmptyString(), NS_LITERAL_STRING("text-align"),
                             value);
  if (NS_SUCCEEDED(rv))
    nsAccUtils::SetAccAttr(aAttributes, nsAccessibilityAtoms::textAlign,
                           value);

  
  rv = GetComputedStyleValue(EmptyString(), NS_LITERAL_STRING("text-indent"),
                             value);
  if (NS_SUCCEEDED(rv))
    nsAccUtils::SetAccAttr(aAttributes, nsAccessibilityAtoms::textIndent,
                           value);

  
  nsCOMPtr<nsIDOMNSHTMLElement> htmlElement = do_QueryInterface(mContent);
  if (htmlElement) {
    PRBool draggable = PR_FALSE;
    htmlElement->GetDraggable(&draggable);
    if (draggable) {
      nsAccUtils::SetAccAttr(aAttributes, nsAccessibilityAtoms::draggable,
                             NS_LITERAL_STRING("true"));
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GroupPosition(PRInt32 *aGroupLevel,
                            PRInt32 *aSimilarItemsInGroup,
                            PRInt32 *aPositionInGroup)
{
  NS_ENSURE_ARG_POINTER(aGroupLevel);
  *aGroupLevel = 0;

  NS_ENSURE_ARG_POINTER(aSimilarItemsInGroup);
  *aSimilarItemsInGroup = 0;

  NS_ENSURE_ARG_POINTER(aPositionInGroup);
  *aPositionInGroup = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  nsCoreUtils::GetUIntAttr(mContent, nsAccessibilityAtoms::aria_level,
                           aGroupLevel);
  nsCoreUtils::GetUIntAttr(mContent, nsAccessibilityAtoms::aria_posinset,
                           aPositionInGroup);
  nsCoreUtils::GetUIntAttr(mContent, nsAccessibilityAtoms::aria_setsize,
                           aSimilarItemsInGroup);

  
  
  if (nsAccUtils::State(this) & nsIAccessibleStates::STATE_INVISIBLE)
    return NS_OK;

  
  if (*aGroupLevel == 0) {
    PRInt32 level = GetLevelInternal();
    if (level != 0)
      *aGroupLevel = level;
  }

  
  if (*aSimilarItemsInGroup == 0 || *aPositionInGroup == 0) {
    PRInt32 posInSet = 0, setSize = 0;
    GetPositionAndSizeInternal(&posInSet, &setSize);
    if (posInSet != 0 && setSize != 0) {
      if (*aPositionInGroup == 0)
        *aPositionInGroup = posInSet;

      if (*aSimilarItemsInGroup == 0)
        *aSimilarItemsInGroup = setSize;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  NS_ENSURE_ARG_POINTER(aState);

  if (!IsDefunct()) {
    
    
    
    nsCOMPtr<nsIPresShell> presShell = GetPresShell();
    presShell->FlushPendingNotifications(Flush_Layout);
  }

  nsresult rv = GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  
  GetARIAState(aState, aExtraState);

  if (mRoleMapEntry && mRoleMapEntry->role == nsIAccessibleRole::ROLE_PAGETAB) {
    if (*aState & nsIAccessibleStates::STATE_FOCUSED) {
      *aState |= nsIAccessibleStates::STATE_SELECTED;
    } else {
      
      
      nsCOMPtr<nsIAccessible> tabPanel = nsRelUtils::
        GetRelatedAccessible(this, nsIAccessibleRelation::RELATION_LABEL_FOR);

      if (nsAccUtils::Role(tabPanel) == nsIAccessibleRole::ROLE_PROPERTYPAGE) {
        nsRefPtr<nsAccessible> tabPanelAcc(do_QueryObject(tabPanel));
        nsINode *tabPanelNode = tabPanelAcc->GetNode();
        if (nsCoreUtils::IsAncestorOf(tabPanelNode, gLastFocusedNode))
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
    
    
    
    
    nsAutoString id;
    if (nsCoreUtils::GetID(mContent, id)) {
      nsIContent *ancestorContent = mContent;
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
  rv = GetRole(&role);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
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
  
  
  if (*aExtraState & nsIAccessibleStates::EXT_STATE_EDITABLE)
    *aState &= ~nsIAccessibleStates::STATE_READONLY;
 
  return NS_OK;
}

nsresult
nsAccessible::GetARIAState(PRUint32 *aState, PRUint32 *aExtraState)
{
  
  PRUint32 index = 0;
  while (nsStateMapEntry::MapToStates(mContent, aState, aExtraState,
                                      nsARIAMap::gWAIUnivStateMap[index])) {
    ++ index;
  }

  if (mRoleMapEntry) {

    
    
    
    if (mRoleMapEntry->role != nsIAccessibleRole::ROLE_NOTHING)
      *aState &= ~nsIAccessibleStates::STATE_READONLY;

    if (mContent->HasAttr(kNameSpaceID_None, mContent->GetIDAttributeName())) {
      
      nsIContent *ancestorContent = mContent;
      while ((ancestorContent = ancestorContent->GetParent()) != nsnull) {
        if (ancestorContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_activedescendant)) {
            
          *aState |= nsIAccessibleStates::STATE_FOCUSABLE;
          break;
        }
      }
    }
  }

  if (*aState & nsIAccessibleStates::STATE_FOCUSABLE) {
    
    nsIContent *ancestorContent = mContent;
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
  if (nsStateMapEntry::MapToStates(mContent, aState, aExtraState,
                                   mRoleMapEntry->attributeMap1) &&
      nsStateMapEntry::MapToStates(mContent, aState, aExtraState,
                                   mRoleMapEntry->attributeMap2)) {
    nsStateMapEntry::MapToStates(mContent, aState, aExtraState,
                                 mRoleMapEntry->attributeMap3);
  }

  return NS_OK;
}




NS_IMETHODIMP
nsAccessible::GetValue(nsAString& aValue)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (mRoleMapEntry) {
    if (mRoleMapEntry->valueRule == eNoValue) {
      return NS_OK;
    }

    
    
    if (!mContent->GetAttr(kNameSpaceID_None,
                           nsAccessibilityAtoms::aria_valuetext, aValue)) {
      mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_valuenow,
                        aValue);
    }
  }

  if (!aValue.IsEmpty())
    return NS_OK;

  
  if (nsCoreUtils::IsXLink(mContent)) {
    nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));
    if (presShell) {
      nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
      return presShell->GetLinkLocation(DOMNode, aValue);
    }
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
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (!mRoleMapEntry || mRoleMapEntry->valueRule == eNoValue)
    return NS_OK_NO_ARIA_VALUE;

  const PRUint32 kValueCannotChange = nsIAccessibleStates::STATE_READONLY |
                                      nsIAccessibleStates::STATE_UNAVAILABLE;

  if (nsAccUtils::State(this) & kValueCannotChange)
    return NS_ERROR_FAILURE;

  double minValue = 0;
  if (NS_SUCCEEDED(GetMinimumValue(&minValue)) && aValue < minValue)
    return NS_ERROR_INVALID_ARG;

  double maxValue = 0;
  if (NS_SUCCEEDED(GetMaximumValue(&maxValue)) && aValue > maxValue)
    return NS_ERROR_INVALID_ARG;

  nsAutoString newValue;
  newValue.AppendFloat(aValue);
  return mContent->SetAttr(kNameSpaceID_None,
                           nsAccessibilityAtoms::aria_valuenow, newValue,
                           PR_TRUE);
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


nsresult
nsAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_NOTHING;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (nsCoreUtils::IsXLink(mContent))
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

  PRUint32 actionRule = GetActionRule(nsAccUtils::State(this));
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

  PRUint32 states = nsAccUtils::State(this);
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
     else if (states & nsIAccessibleStates::STATE_MIXED)
       aName.AssignLiteral("cycle");
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
     
   case eSortAction:
     aName.AssignLiteral("sort");
     return NS_OK;
   
   case eExpandAction:
     if (states & nsIAccessibleStates::STATE_COLLAPSED)
       aName.AssignLiteral("expand");
     else
       aName.AssignLiteral("collapse");
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

  if (GetActionRule(nsAccUtils::State(this)) != eNoAction) {
    DoCommand();
    return NS_OK;
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
  nsIContent *loopContent = mContent;
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


NS_IMETHODIMP
nsAccessible::GetRelationByType(PRUint32 aRelationType,
                                nsIAccessibleRelation **aRelation)
{
  NS_ENSURE_ARG_POINTER(aRelation);
  *aRelation = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  
  nsresult rv;
  switch (aRelationType)
  {
  case nsIAccessibleRelation::RELATION_LABEL_FOR:
    {
      if (mContent->Tag() == nsAccessibilityAtoms::label) {
        nsIAtom *IDAttr = mContent->IsHTML() ?
          nsAccessibilityAtoms::_for : nsAccessibilityAtoms::control;
        rv = nsRelUtils::
          AddTargetFromIDRefAttr(aRelationType, aRelation, mContent, IDAttr);
        NS_ENSURE_SUCCESS(rv, rv);

        if (rv != NS_OK_NO_RELATION_TARGET)
          return NS_OK; 
      }

      return nsRelUtils::
        AddTargetFromNeighbour(aRelationType, aRelation, mContent,
                               nsAccessibilityAtoms::aria_labelledby);
    }

  case nsIAccessibleRelation::RELATION_LABELLED_BY:
    {
      rv = nsRelUtils::
        AddTargetFromIDRefsAttr(aRelationType, aRelation, mContent,
                                nsAccessibilityAtoms::aria_labelledby);
      NS_ENSURE_SUCCESS(rv, rv);

      if (rv != NS_OK_NO_RELATION_TARGET)
        return NS_OK; 

      return nsRelUtils::
        AddTargetFromContent(aRelationType, aRelation,
                             nsCoreUtils::GetLabelContent(mContent));
    }

  case nsIAccessibleRelation::RELATION_DESCRIBED_BY:
    {
      rv = nsRelUtils::
        AddTargetFromIDRefsAttr(aRelationType, aRelation, mContent,
                                nsAccessibilityAtoms::aria_describedby);
      NS_ENSURE_SUCCESS(rv, rv);

      if (rv != NS_OK_NO_RELATION_TARGET)
        return NS_OK; 

      return nsRelUtils::
        AddTargetFromNeighbour(aRelationType, aRelation, mContent,
                               nsAccessibilityAtoms::control,
                               nsAccessibilityAtoms::description);
    }

  case nsIAccessibleRelation::RELATION_DESCRIPTION_FOR:
    {
      rv = nsRelUtils::
        AddTargetFromNeighbour(aRelationType, aRelation, mContent,
                               nsAccessibilityAtoms::aria_describedby);
      NS_ENSURE_SUCCESS(rv, rv);

      if (rv != NS_OK_NO_RELATION_TARGET)
        return NS_OK; 

      if (mContent->Tag() == nsAccessibilityAtoms::description &&
          mContent->IsXUL()) {
        
        
        
        return nsRelUtils::
          AddTargetFromIDRefAttr(aRelationType, aRelation, mContent,
                                 nsAccessibilityAtoms::control);
      }

      return NS_OK;
    }

  case nsIAccessibleRelation::RELATION_NODE_CHILD_OF:
    {
      rv = nsRelUtils::
        AddTargetFromNeighbour(aRelationType, aRelation, mContent,
                               nsAccessibilityAtoms::aria_owns);
      NS_ENSURE_SUCCESS(rv, rv);

      if (rv != NS_OK_NO_RELATION_TARGET)
        return NS_OK; 

      
      
      if (mRoleMapEntry &&
          (mRoleMapEntry->role == nsIAccessibleRole::ROLE_OUTLINEITEM ||
           mRoleMapEntry->role == nsIAccessibleRole::ROLE_ROW)) {

        AccGroupInfo* groupInfo = GetGroupInfo();
        if (!groupInfo)
          return NS_OK_NO_RELATION_TARGET;

        return nsRelUtils::AddTarget(aRelationType, aRelation,
                                     groupInfo->GetConceptualParent());
      }

      
      
      
      
      
      
      nsIFrame *frame = GetFrame();
      if (frame) {
        nsIView *view = frame->GetViewExternal();
        if (view) {
          nsIScrollableFrame *scrollFrame = do_QueryFrame(frame);
          if (scrollFrame || view->GetWidget() || !frame->GetParent()) {
            return nsRelUtils::AddTarget(aRelationType, aRelation, GetParent());
          }
        }
      }

      return NS_OK;
    }

  case nsIAccessibleRelation::RELATION_CONTROLLED_BY:
    {
      return nsRelUtils::
        AddTargetFromNeighbour(aRelationType, aRelation, mContent,
                               nsAccessibilityAtoms::aria_controls);
    }

  case nsIAccessibleRelation::RELATION_CONTROLLER_FOR:
    {
      return nsRelUtils::
        AddTargetFromIDRefsAttr(aRelationType, aRelation, mContent,
                                nsAccessibilityAtoms::aria_controls);
    }

  case nsIAccessibleRelation::RELATION_FLOWS_TO:
    {
      return nsRelUtils::
        AddTargetFromIDRefsAttr(aRelationType, aRelation, mContent,
                                nsAccessibilityAtoms::aria_flowto);
    }

  case nsIAccessibleRelation::RELATION_FLOWS_FROM:
    {
      return nsRelUtils::
        AddTargetFromNeighbour(aRelationType, aRelation, mContent,
                               nsAccessibilityAtoms::aria_flowto);
    }

  case nsIAccessibleRelation::RELATION_DEFAULT_BUTTON:
    {
      if (mContent->IsHTML()) {
        
        nsCOMPtr<nsIFormControl> control(do_QueryInterface(mContent));
        if (control) {
          nsCOMPtr<nsIForm> form(do_QueryInterface(control->GetFormElement()));
          if (form) {
            nsCOMPtr<nsIContent> formContent =
              do_QueryInterface(form->GetDefaultSubmitElement());
            return nsRelUtils::AddTargetFromContent(aRelationType, aRelation,
                                                    formContent);
          }
        }
      }
      else {
        
        nsCOMPtr<nsIDOMXULDocument> xulDoc =
          do_QueryInterface(mContent->GetOwnerDoc());
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
          nsCOMPtr<nsIContent> relatedContent(do_QueryInterface(buttonEl));
          return nsRelUtils::AddTargetFromContent(aRelationType, aRelation,
                                                  relatedContent);
        }
      }
      return NS_OK;
    }

  case nsIAccessibleRelation::RELATION_MEMBER_OF:
    {
      nsCOMPtr<nsIContent> regionContent = do_QueryInterface(GetAtomicRegion());
      return nsRelUtils::
        AddTargetFromContent(aRelationType, aRelation, regionContent);
    }

  case nsIAccessibleRelation::RELATION_SUBWINDOW_OF:
  case nsIAccessibleRelation::RELATION_EMBEDS:
  case nsIAccessibleRelation::RELATION_EMBEDDED_BY:
  case nsIAccessibleRelation::RELATION_POPUP_FOR:
  case nsIAccessibleRelation::RELATION_PARENT_WINDOW_OF:
    {
      return NS_OK_NO_RELATION_TARGET;
    }

  default:
    return NS_ERROR_INVALID_ARG;
  }
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

    nsCOMPtr<nsIAccessibleRelation> relation;
    nsresult rv = GetRelationByType(relType, getter_AddRefs(relation));

    if (NS_SUCCEEDED(rv) && relation)
      relations->AppendElement(relation, PR_FALSE);
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

void
nsAccessible::DoCommand(nsIContent *aContent, PRUint32 aActionIndex)
{
  nsIContent* content = aContent ? aContent : mContent.get();
  NS_DISPATCH_RUNNABLEMETHOD_ARG2(DispatchClickEvent, this, content,
                                  aActionIndex);
}

void
nsAccessible::DispatchClickEvent(nsIContent *aContent, PRUint32 aActionIndex)
{
  if (IsDefunct())
    return;

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();

  
  presShell->ScrollContentIntoView(aContent, NS_PRESSHELL_SCROLL_ANYWHERE,
                                   NS_PRESSHELL_SCROLL_ANYWHERE);

  
  PRBool res = nsCoreUtils::DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, presShell,
                                               aContent);
  if (!res)
    return;

  nsCoreUtils::DispatchMouseEvent(NS_MOUSE_BUTTON_UP, presShell, aContent);
}


NS_IMETHODIMP nsAccessible::GetSelectedChildren(nsIArray **aSelectedAccessibles)
{
  *aSelectedAccessibles = nsnull;

  nsCOMPtr<nsIMutableArray> selectedAccessibles =
    do_CreateInstance(NS_ARRAY_CONTRACTID);
  NS_ENSURE_STATE(selectedAccessibles);

  AccIterator iter(this, filters::GetSelected, AccIterator::eTreeNav);
  nsIAccessible *selected = nsnull;
  while ((selected = iter.GetNext()))
    selectedAccessibles->AppendElement(selected, PR_FALSE);

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
  NS_ENSURE_ARG_POINTER(aSelected);
  *aSelected = nsnull;

  if (aIndex < 0) {
    return NS_ERROR_INVALID_ARG;
  }

  AccIterator iter(this, filters::GetSelected, AccIterator::eTreeNav);
  nsAccessible *selected = nsnull;

  PRInt32 count = 0;
  while (count ++ <= aIndex) {
    selected = iter.GetNext();
    if (!selected) {
      
      return NS_ERROR_INVALID_ARG;
    }
  }
  NS_IF_ADDREF(*aSelected = selected);
  return NS_OK;
}

NS_IMETHODIMP nsAccessible::GetSelectionCount(PRInt32 *aSelectionCount)
{
  NS_ENSURE_ARG_POINTER(aSelectionCount);
  *aSelectionCount = 0;

  AccIterator iter(this, filters::GetSelected, AccIterator::eTreeNav);
  nsAccessible *selected = nsnull;
  while ((selected = iter.GetNext()))
    ++(*aSelectionCount);

  return NS_OK;
}

NS_IMETHODIMP nsAccessible::AddChildToSelection(PRInt32 aIndex)
{
  
  
  

  NS_ENSURE_TRUE(aIndex >= 0, NS_ERROR_FAILURE);

  nsAccessible* child = GetChildAt(aIndex);
  PRUint32 state = nsAccUtils::State(child);
  if (!(state & nsIAccessibleStates::STATE_SELECTABLE)) {
    return NS_OK;
  }

  return child->SetSelected(PR_TRUE);
}

NS_IMETHODIMP nsAccessible::RemoveChildFromSelection(PRInt32 aIndex)
{
  
  
  

  NS_ENSURE_TRUE(aIndex >= 0, NS_ERROR_FAILURE);

  nsAccessible* child = GetChildAt(aIndex);
  PRUint32 state = nsAccUtils::State(child);
  if (!(state & nsIAccessibleStates::STATE_SELECTED)) {
    return NS_OK;
  }

  return child->SetSelected(PR_FALSE);
}

NS_IMETHODIMP nsAccessible::IsChildSelected(PRInt32 aIndex, PRBool *aIsSelected)
{
  
  
  

  *aIsSelected = PR_FALSE;
  NS_ENSURE_TRUE(aIndex >= 0, NS_ERROR_FAILURE);

  nsAccessible* child = GetChildAt(aIndex);
  PRUint32 state = nsAccUtils::State(child);
  if (state & nsIAccessibleStates::STATE_SELECTED) {
    *aIsSelected = PR_TRUE;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::ClearSelection()
{
  AccIterator iter(this, filters::GetSelected, AccIterator::eTreeNav);
  nsAccessible *selected = nsnull;
  while ((selected = iter.GetNext()))
    selected->SetSelected(PR_FALSE);

  return NS_OK;
}

NS_IMETHODIMP nsAccessible::SelectAllSelection(PRBool *_retval)
{
  AccIterator iter(this, filters::GetSelectable, AccIterator::eTreeNav);
  nsAccessible *selectable = nsnull;
  while((selectable = iter.GetNext()))
    selectable->SetSelected(PR_TRUE);

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

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRInt32 endIndex;
  return GetLinkOffset(aStartIndex, &endIndex);
}


NS_IMETHODIMP
nsAccessible::GetEndIndex(PRInt32 *aEndIndex)
{
  NS_ENSURE_ARG_POINTER(aEndIndex);
  *aEndIndex = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

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

  
  if (nsCoreUtils::IsXLink(mContent)) {
    nsAutoString href;
    mContent->GetAttr(kNameSpaceID_XLink, nsAccessibilityAtoms::href, href);

    nsCOMPtr<nsIURI> baseURI = mContent->GetBaseURI();
    nsCOMPtr<nsIDocument> document = mContent->GetOwnerDoc();
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
  PRUint32 state = nsAccUtils::State(this);
  *aValid = (0 == (state & nsIAccessibleStates::STATE_INVALID));
  
  
  
  
  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetSelected(PRBool *aSelected)
{
  NS_ENSURE_ARG_POINTER(aSelected);

  *aSelected = (gLastFocusedNode == GetNode());
  return NS_OK;
}

nsresult
nsAccessible::GetLinkOffset(PRInt32 *aStartOffset, PRInt32 *aEndOffset)
{
  nsAccessible *parent = GetParent();
  NS_ENSURE_STATE(parent);

  PRUint32 characterCount = 0;

  PRInt32 childCount = parent->GetChildCount();
  for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsAccessible *sibling = parent->GetChildAt(childIdx);

    if (sibling == this) {
      *aStartOffset = characterCount;
      *aEndOffset = characterCount + 1;
      return NS_OK;
    }

    characterCount += nsAccUtils::TextLength(sibling);
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsAccessible::AppendTextTo(nsAString& aText, PRUint32 aStartOffset, PRUint32 aLength)
{
  
  
  if (aStartOffset != 0)
    return NS_OK;

  nsIFrame *frame = GetFrame();
  NS_ENSURE_STATE(frame);

  if (frame->GetType() == nsAccessibilityAtoms::brFrame) {
    aText += kForcedNewLineChar;
  } else if (nsAccUtils::MustPrune(this)) {
    
    
    aText += kImaginaryEmbeddedObjectChar;
  } else {
    aText += kEmbeddedObjectChar;
  }

  return NS_OK;
}




PRBool
nsAccessible::Init()
{
  if (!nsAccessNodeWrap::Init())
    return PR_FALSE;

  nsDocAccessible *docAcc =
    GetAccService()->GetDocAccessible(mContent->GetOwnerDoc());
  NS_ASSERTION(docAcc, "Cannot cache new nsAccessible!");
  if (!docAcc)
    return PR_FALSE;

  void *uniqueID = nsnull;
  GetUniqueID(&uniqueID);

  return docAcc->CacheAccessible(uniqueID, this);
}

void
nsAccessible::Shutdown()
{
  
  
  InvalidateChildren();
  if (mParent) {
    mParent->InvalidateChildren();
    UnbindFromParent();
  }

  nsAccessNodeWrap::Shutdown();
}




nsresult
nsAccessible::GetARIAName(nsAString& aName)
{
  nsAutoString label;

  
  nsresult rv = nsTextEquivUtils::
    GetTextEquivFromIDRefs(this, nsAccessibilityAtoms::aria_labelledby, label);
  if (NS_SUCCEEDED(rv)) {
    label.CompressWhitespace();
    aName = label;
  }

  if (label.IsEmpty() &&
      mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_label,
                        label)) {
    label.CompressWhitespace();
    aName = label;
  }
  
  return NS_OK;
}

nsresult
nsAccessible::GetNameInternal(nsAString& aName)
{
  if (mContent->IsHTML())
    return GetHTMLName(aName);

  if (mContent->IsXUL())
    return GetXULName(aName);

  return NS_OK;
}


void
nsAccessible::BindToParent(nsAccessible* aParent, PRUint32 aIndexInParent)
{
  NS_PRECONDITION(aParent, "This method isn't used to set null parent!");

  if (mParent && mParent != aParent) {
    
    
    
    
    NS_ASSERTION(PR_FALSE, "Adopting child!");
    if (mParent)
      mParent->InvalidateChildren();
  }

  mParent = aParent;
  mIndexInParent = aIndexInParent;
}

void
nsAccessible::UnbindFromParent()
{
  mParent = nsnull;
  mIndexInParent = -1;
  mGroupInfo = nsnull;
}

void
nsAccessible::InvalidateChildren()
{
  PRInt32 childCount = mChildren.Length();
  for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsAccessible* child = mChildren.ElementAt(childIdx);
    child->UnbindFromParent();
  }

  mChildren.Clear();
  mAreChildrenInitialized = PR_FALSE;
}

PRBool
nsAccessible::AppendChild(nsAccessible* aChild)
{
  if (!mChildren.AppendElement(aChild))
    return PR_FALSE;

  aChild->BindToParent(this, mChildren.Length() - 1);
  return PR_TRUE;
}

PRBool
nsAccessible::InsertChildAt(PRUint32 aIndex, nsAccessible* aChild)
{
  if (!mChildren.InsertElementAt(aIndex, aChild))
    return PR_FALSE;

  for (PRUint32 idx = aIndex + 1; idx < mChildren.Length(); idx++)
    mChildren[idx]->mIndexInParent++;

  aChild->BindToParent(this, aIndex);
  return PR_TRUE;
}

PRBool
nsAccessible::RemoveChild(nsAccessible* aChild)
{
  if (aChild->mParent != this || aChild->mIndexInParent == -1)
    return PR_FALSE;

  for (PRUint32 idx = aChild->mIndexInParent + 1; idx < mChildren.Length(); idx++)
    mChildren[idx]->mIndexInParent--;

  mChildren.RemoveElementAt(aChild->mIndexInParent);
  aChild->UnbindFromParent();
  return PR_TRUE;
}

nsAccessible*
nsAccessible::GetParent()
{
  if (mParent)
    return mParent;

  if (IsDefunct())
    return nsnull;

  
  
  
  
  
  NS_WARNING("Bad accessible tree!");

#ifdef DEBUG
  nsDocAccessible *docAccessible = GetDocAccessible();
  NS_ASSERTION(docAccessible, "No document accessible for valid accessible!");
#endif

  nsAccessible* parent = GetAccService()->GetContainerAccessible(mContent,
                                                                 mWeakShell);
  NS_ASSERTION(parent, "No accessible parent for valid accessible!");
  if (!parent)
    return nsnull;

  
  parent->EnsureChildren();
  NS_ASSERTION(parent == mParent, "Wrong children repair!");
  return parent;
}

nsAccessible*
nsAccessible::GetChildAt(PRUint32 aIndex)
{
  if (EnsureChildren())
    return nsnull;

  nsAccessible *child = mChildren.SafeElementAt(aIndex, nsnull);
  if (!child)
    return nsnull;

#ifdef DEBUG
  nsAccessible* realParent = child->mParent;
  NS_ASSERTION(!realParent || realParent == this,
               "Two accessibles have the same first child accessible!");
#endif

  return child;
}

PRInt32
nsAccessible::GetChildCount()
{
  return EnsureChildren() ? -1 : mChildren.Length();
}

PRInt32
nsAccessible::GetIndexOf(nsAccessible* aChild)
{
  return EnsureChildren() || (aChild->mParent != this) ?
    -1 : aChild->GetIndexInParent();
}

PRInt32
nsAccessible::GetIndexInParent()
{
  
  nsAccessible* parent = GetParent();
  return mIndexInParent;
}

#ifdef DEBUG
PRBool
nsAccessible::IsInCache()
{
  nsDocAccessible *docAccessible =
    GetAccService()->GetDocAccessible(mContent->GetOwnerDoc());
  if (!docAccessible)
    return nsnull;

  void *uniqueID = nsnull;
  GetUniqueID(&uniqueID);

  return docAccessible->GetCachedAccessible(uniqueID) ? PR_TRUE : PR_FALSE;
}
#endif





void
nsAccessible::CacheChildren()
{
  nsAccTreeWalker walker(mWeakShell, mContent, GetAllowsAnonChildAccessibles());

  nsRefPtr<nsAccessible> child;
  while ((child = walker.GetNextChild()) && AppendChild(child));
}

void
nsAccessible::TestChildCache(nsAccessible *aCachedChild)
{
#ifdef DEBUG
  PRInt32 childCount = mChildren.Length();
  if (childCount == 0) {
    NS_ASSERTION(!mAreChildrenInitialized, "No children but initialized!");
    return;
  }

  nsAccessible *child = nsnull;
  for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
    child = mChildren[childIdx];
    if (child == aCachedChild)
      break;
  }

  NS_ASSERTION(child == aCachedChild,
               "[TestChildCache] cached accessible wasn't found. Wrong accessible tree!");  
#endif
}


PRBool
nsAccessible::EnsureChildren()
{
  if (IsDefunct()) {
    mAreChildrenInitialized = PR_FALSE;
    return PR_TRUE;
  }

  if (mAreChildrenInitialized)
    return PR_FALSE;

  mAreChildrenInitialized = PR_TRUE; 
  CacheChildren();

  return PR_FALSE;
}

nsAccessible*
nsAccessible::GetSiblingAtOffset(PRInt32 aOffset, nsresult* aError)
{
  if (IsDefunct()) {
    if (aError)
      *aError = NS_ERROR_FAILURE;

    return nsnull;
  }

  nsAccessible *parent = GetParent();
  if (!parent) {
    if (aError)
      *aError = NS_ERROR_UNEXPECTED;

    return nsnull;
  }

  if (mIndexInParent == -1) {
    if (aError)
      *aError = NS_ERROR_UNEXPECTED;

    return nsnull;
  }

  if (aError) {
    PRInt32 childCount = parent->GetChildCount();
    if (mIndexInParent + aOffset >= childCount) {
      *aError = NS_OK; 
      return nsnull;
    }
  }

  nsAccessible* child = parent->GetChildAt(mIndexInParent + aOffset);
  if (aError && !child)
    *aError = NS_ERROR_UNEXPECTED;

  return child;
}

nsAccessible *
nsAccessible::GetFirstAvailableAccessible(nsINode *aStartNode) const
{
  nsAccessible *accessible =
    GetAccService()->GetAccessibleInWeakShell(aStartNode, mWeakShell);
  if (accessible)
    return accessible;

  nsIContent *content = nsCoreUtils::GetRoleContent(aStartNode);
  nsAccTreeWalker walker(mWeakShell, content, PR_FALSE);
  nsRefPtr<nsAccessible> childAccessible = walker.GetNextChild();
  return childAccessible;
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
        nsIPresShell* shell = parentDoc->GetShell();
        if (!shell) {
          return PR_FALSE;
        }
        nsIFrame* frame = content->GetPrimaryFrame();
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

  if (IsDefunct())
    return NS_ERROR_FAILURE;  

 if (!mRoleMapEntry || mRoleMapEntry->valueRule == eNoValue)
    return NS_OK_NO_ARIA_VALUE;

  nsAutoString attrValue;
  mContent->GetAttr(kNameSpaceID_None, aProperty, attrValue);

  
  if (attrValue.IsEmpty())
    return NS_OK;

  PRInt32 error = NS_OK;
  double value = attrValue.ToFloat(&error);
  if (NS_SUCCEEDED(error))
    *aValue = value;

  return NS_OK;
}

PRUint32
nsAccessible::GetActionRule(PRUint32 aStates)
{
  if (aStates & nsIAccessibleStates::STATE_UNAVAILABLE)
    return eNoAction;
  
  
  if (nsCoreUtils::IsXLink(mContent))
    return eJumpAction;

  
  if (mContent->IsXUL())
    if (mContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::popup))
      return eClickAction;

  
  PRBool isOnclick = nsCoreUtils::HasClickListener(mContent);

  if (isOnclick)
    return eClickAction;
  
  
  if (mRoleMapEntry &&
      mRoleMapEntry->actionRule != eNoAction)
    return mRoleMapEntry->actionRule;

  
  if (nsAccUtils::HasDefinedARIAToken(mContent,
                                      nsAccessibilityAtoms::aria_expanded))
    return eExpandAction;

  return eNoAction;
}

AccGroupInfo*
nsAccessible::GetGroupInfo()
{
  if (mGroupInfo)
    return mGroupInfo;

  mGroupInfo = AccGroupInfo::CreateGroupInfo(this);
  return mGroupInfo;
}

void
nsAccessible::GetPositionAndSizeInternal(PRInt32 *aPosInSet, PRInt32 *aSetSize)
{
  AccGroupInfo* groupInfo = GetGroupInfo();
  if (groupInfo) {
    *aPosInSet = groupInfo->PosInSet();
    *aSetSize = groupInfo->SetSize();
  }
}

PRInt32
nsAccessible::GetLevelInternal()
{
  PRInt32 level = nsAccUtils::GetDefaultLevel(this);

  PRUint32 role = nsAccUtils::Role(this);
  nsAccessible* parent = GetParent();

  if (role == nsIAccessibleRole::ROLE_OUTLINEITEM) {
    
    
    
    level = 1;

    while (parent) {
      PRUint32 parentRole = nsAccUtils::Role(parent);

      if (parentRole == nsIAccessibleRole::ROLE_OUTLINE)
        break;
      if (parentRole == nsIAccessibleRole::ROLE_GROUPING)
        ++ level;

      parent = parent->GetParent();
    }

  } else if (role == nsIAccessibleRole::ROLE_LISTITEM) {
    
    
    
    

    
    level = 0;

    while (parent) {
      PRUint32 parentRole = nsAccUtils::Role(parent);

      if (parentRole == nsIAccessibleRole::ROLE_LISTITEM)
        ++ level;
      else if (parentRole != nsIAccessibleRole::ROLE_LIST)
        break;

      parent = parent->GetParent();
    }

    if (level == 0) {
      
      
      nsAccessible* parent(GetParent());
      PRInt32 siblingCount = parent->GetChildCount();
      for (PRInt32 siblingIdx = 0; siblingIdx < siblingCount; siblingIdx++) {
        nsAccessible* sibling = parent->GetChildAt(siblingIdx);

        nsCOMPtr<nsIAccessible> siblingChild;
        sibling->GetLastChild(getter_AddRefs(siblingChild));
        if (nsAccUtils::Role(siblingChild) == nsIAccessibleRole::ROLE_LIST) {
          level = 1;
          break;
        }
      }
    } else {
      ++ level; 
    }
  }

  return level;
}
