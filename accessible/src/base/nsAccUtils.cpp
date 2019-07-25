





































#include "nsCoreUtils.h"
#include "nsAccUtils.h"

#include "nsIAccessibleStates.h"
#include "nsIAccessibleTypes.h"

#include "nsAccessibilityService.h"
#include "nsAccessibilityAtoms.h"
#include "nsAccessible.h"
#include "nsAccTreeWalker.h"
#include "nsARIAMap.h"
#include "nsDocAccessible.h"
#include "nsHyperTextAccessible.h"
#include "nsHTMLTableAccessible.h"
#include "nsXULTreeGridAccessible.h"

#include "nsIDOMXULContainerElement.h"
#include "nsIDOMXULSelectCntrlEl.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsWhitespaceTokenizer.h"
#include "nsComponentManagerUtils.h"

void
nsAccUtils::GetAccAttr(nsIPersistentProperties *aAttributes,
                       nsIAtom *aAttrName, nsAString& aAttrValue)
{
  aAttrValue.Truncate();

  aAttributes->GetStringProperty(nsAtomCString(aAttrName), aAttrValue);
}

void
nsAccUtils::SetAccAttr(nsIPersistentProperties *aAttributes,
                       nsIAtom *aAttrName, const nsAString& aAttrValue)
{
  nsAutoString oldValue;
  nsCAutoString attrName;

  aAttributes->SetStringProperty(nsAtomCString(aAttrName), aAttrValue, oldValue);
}

void
nsAccUtils::SetAccGroupAttrs(nsIPersistentProperties *aAttributes,
                             PRInt32 aLevel, PRInt32 aSetSize,
                             PRInt32 aPosInSet)
{
  nsAutoString value;

  if (aLevel) {
    value.AppendInt(aLevel);
    SetAccAttr(aAttributes, nsAccessibilityAtoms::level, value);
  }

  if (aSetSize && aPosInSet) {
    value.Truncate();
    value.AppendInt(aPosInSet);
    SetAccAttr(aAttributes, nsAccessibilityAtoms::posinset, value);

    value.Truncate();
    value.AppendInt(aSetSize);
    SetAccAttr(aAttributes, nsAccessibilityAtoms::setsize, value);
  }
}

PRInt32
nsAccUtils::GetDefaultLevel(nsAccessible *aAccessible)
{
  PRUint32 role = nsAccUtils::Role(aAccessible);

  if (role == nsIAccessibleRole::ROLE_OUTLINEITEM)
    return 1;

  if (role == nsIAccessibleRole::ROLE_ROW) {
    nsAccessible *parent = aAccessible->GetParent();
    if (Role(parent) == nsIAccessibleRole::ROLE_TREE_TABLE) {
      
      
      return 1;
    }
  }

  return 0;
}

PRInt32
nsAccUtils::GetARIAOrDefaultLevel(nsAccessible *aAccessible)
{
  PRInt32 level = 0;
  nsCoreUtils::GetUIntAttr(aAccessible->GetContent(),
                           nsAccessibilityAtoms::aria_level, &level);

  if (level != 0)
    return level;

  return GetDefaultLevel(aAccessible);
}

void
nsAccUtils::GetPositionAndSizeForXULSelectControlItem(nsIContent *aContent,
                                                      PRInt32 *aPosInSet,
                                                      PRInt32 *aSetSize)
{
  nsCOMPtr<nsIDOMXULSelectControlItemElement> item(do_QueryInterface(aContent));
  if (!item)
    return;

  nsCOMPtr<nsIDOMXULSelectControlElement> control;
  item->GetControl(getter_AddRefs(control));
  if (!control)
    return;

  PRUint32 itemsCount = 0;
  control->GetItemCount(&itemsCount);

  PRInt32 indexOf = 0;
  control->GetIndexOfItem(item, &indexOf);

  *aSetSize = itemsCount;
  *aPosInSet = indexOf;

  for (PRUint32 index = 0; index < itemsCount; index++) {
    nsCOMPtr<nsIDOMXULSelectControlItemElement> currItem;
    control->GetItemAtIndex(index, getter_AddRefs(currItem));
    nsCOMPtr<nsIDOMNode> currNode(do_QueryInterface(currItem));

    nsCOMPtr<nsIAccessible> itemAcc;
    GetAccService()->GetAccessibleFor(currNode, getter_AddRefs(itemAcc));
    if (!itemAcc ||
        State(itemAcc) & nsIAccessibleStates::STATE_INVISIBLE) {
      (*aSetSize)--;
      if (index < static_cast<PRUint32>(indexOf))
        (*aPosInSet)--;
    }
  }

  (*aPosInSet)++; 
}

void
nsAccUtils::GetPositionAndSizeForXULContainerItem(nsIContent *aContent,
                                                  PRInt32 *aPosInSet,
                                                  PRInt32 *aSetSize)
{
  nsCOMPtr<nsIDOMXULContainerItemElement> item(do_QueryInterface(aContent));
  if (!item)
    return;

  nsCOMPtr<nsIDOMXULContainerElement> container;
  item->GetParentContainer(getter_AddRefs(container));
  if (!container)
    return;

  
  PRUint32 itemsCount = 0;
  container->GetItemCount(&itemsCount);

  
  PRInt32 indexOf = 0;
  container->GetIndexOfItem(item, &indexOf);

  
  *aSetSize = 0, *aPosInSet = 0;
  for (PRInt32 index = indexOf; index >= 0; index--) {
    nsCOMPtr<nsIDOMXULElement> item;
    container->GetItemAtIndex(index, getter_AddRefs(item));

    nsCOMPtr<nsIAccessible> itemAcc;
    GetAccService()->GetAccessibleFor(item, getter_AddRefs(itemAcc));

    if (itemAcc) {
      PRUint32 itemRole = Role(itemAcc);
      if (itemRole == nsIAccessibleRole::ROLE_SEPARATOR)
        break; 

      PRUint32 itemState = State(itemAcc);
      if (!(itemState & nsIAccessibleStates::STATE_INVISIBLE)) {
        (*aSetSize)++;
        (*aPosInSet)++;
      }
    }
  }

  for (PRInt32 index = indexOf + 1; index < static_cast<PRInt32>(itemsCount);
       index++) {
    nsCOMPtr<nsIDOMXULElement> item;
    container->GetItemAtIndex(index, getter_AddRefs(item));
    
    nsCOMPtr<nsIAccessible> itemAcc;
    GetAccService()->GetAccessibleFor(item, getter_AddRefs(itemAcc));

    if (itemAcc) {
      PRUint32 itemRole = Role(itemAcc);
      if (itemRole == nsIAccessibleRole::ROLE_SEPARATOR)
        break; 

      PRUint32 itemState = State(itemAcc);
      if (!(itemState & nsIAccessibleStates::STATE_INVISIBLE))
        (*aSetSize)++;
    }
  }
}

PRInt32
nsAccUtils::GetLevelForXULContainerItem(nsIContent *aContent)
{
  nsCOMPtr<nsIDOMXULContainerItemElement> item(do_QueryInterface(aContent));
  if (!item)
    return 0;

  nsCOMPtr<nsIDOMXULContainerElement> container;
  item->GetParentContainer(getter_AddRefs(container));
  if (!container)
    return 0;

  
  PRInt32 level = -1;
  while (container) {
    level++;

    nsCOMPtr<nsIDOMXULContainerElement> parentContainer;
    container->GetParentContainer(getter_AddRefs(parentContainer));
    parentContainer.swap(container);
  }

  return level;
}

void
nsAccUtils::SetLiveContainerAttributes(nsIPersistentProperties *aAttributes,
                                       nsIContent *aStartContent,
                                       nsIContent *aTopContent)
{
  nsAutoString atomic, live, relevant, busy;
  nsIContent *ancestor = aStartContent;
  while (ancestor) {

    
    if (relevant.IsEmpty() &&
        nsAccUtils::HasDefinedARIAToken(ancestor, nsAccessibilityAtoms::aria_relevant) &&
        ancestor->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_relevant, relevant))
      SetAccAttr(aAttributes, nsAccessibilityAtoms::containerRelevant, relevant);

    
    if (live.IsEmpty()) {
      nsRoleMapEntry *role = GetRoleMapEntry(ancestor);
      if (nsAccUtils::HasDefinedARIAToken(ancestor,
                                          nsAccessibilityAtoms::aria_live)) {
        ancestor->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_live,
                          live);
      } else if (role) {
        GetLiveAttrValue(role->liveAttRule, live);
      }
      if (!live.IsEmpty()) {
        SetAccAttr(aAttributes, nsAccessibilityAtoms::containerLive, live);
        if (role) {
          nsAccUtils::SetAccAttr(aAttributes,
                                 nsAccessibilityAtoms::containerLiveRole,
                                 NS_ConvertASCIItoUTF16(role->roleString));
        }
      }
    }

    
    if (atomic.IsEmpty() &&
        nsAccUtils::HasDefinedARIAToken(ancestor, nsAccessibilityAtoms::aria_atomic) &&
        ancestor->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_atomic, atomic))
      SetAccAttr(aAttributes, nsAccessibilityAtoms::containerAtomic, atomic);

    
    if (busy.IsEmpty() &&
        nsAccUtils::HasDefinedARIAToken(ancestor, nsAccessibilityAtoms::aria_busy) &&
        ancestor->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_busy, busy))
      SetAccAttr(aAttributes, nsAccessibilityAtoms::containerBusy, busy);

    if (ancestor == aTopContent)
      break;

    ancestor = ancestor->GetParent();
    if (!ancestor)
      ancestor = aTopContent; 
  }
}

PRBool
nsAccUtils::HasDefinedARIAToken(nsIContent *aContent, nsIAtom *aAtom)
{
  NS_ASSERTION(aContent, "aContent is null in call to HasDefinedARIAToken!");

  if (!aContent->HasAttr(kNameSpaceID_None, aAtom) ||
      aContent->AttrValueIs(kNameSpaceID_None, aAtom,
                            nsAccessibilityAtoms::_empty, eCaseMatters) ||
      aContent->AttrValueIs(kNameSpaceID_None, aAtom,
                            nsAccessibilityAtoms::_undefined, eCaseMatters)) {
        return PR_FALSE;
  }
  return PR_TRUE;
}

PRBool
nsAccUtils::HasAccessibleChildren(nsINode *aNode)
{
  if (!aNode)
    return PR_FALSE;

  nsIPresShell *presShell = nsCoreUtils::GetPresShellFor(aNode);
  if (!presShell)
    return PR_FALSE;

  nsIContent *content = nsCoreUtils::GetRoleContent(aNode);
  nsCOMPtr<nsIWeakReference> weakShell(do_GetWeakReference(presShell));
  nsAccTreeWalker walker(weakShell, content, PR_FALSE);
  nsRefPtr<nsAccessible> accessible = walker.GetNextChild();
  return accessible ? PR_TRUE : PR_FALSE;
}

nsAccessible *
nsAccUtils::GetAncestorWithRole(nsAccessible *aDescendant, PRUint32 aRole)
{
  nsAccessible *document = aDescendant->GetDocAccessible();
  nsAccessible *parent = aDescendant;
  while ((parent = parent->GetParent())) {
    PRUint32 testRole = nsAccUtils::Role(parent);
    if (testRole == aRole)
      return parent;

    if (parent == document)
      break;
  }
  return nsnull;
}

void
nsAccUtils::GetARIATreeItemParent(nsIAccessible *aStartTreeItem,
                                  nsIContent *aStartContent,
                                  nsIAccessible **aTreeItemParentResult)
{
  *aTreeItemParentResult = nsnull;

  nsCOMPtr<nsIAccessible> parentAccessible;
  aStartTreeItem->GetParent(getter_AddRefs(parentAccessible));
  if (!parentAccessible)
    return;

  PRUint32 startTreeItemRole = nsAccUtils::Role(aStartTreeItem);

  
  if (startTreeItemRole == nsIAccessibleRole::ROLE_ROW) {
    PRUint32 role = nsAccUtils::Role(parentAccessible);
    if (role != nsIAccessibleRole::ROLE_TREE_TABLE)
      return;
  }

  
  
  
  nsAutoString levelStr;
  if (nsAccUtils::HasDefinedARIAToken(aStartContent, nsAccessibilityAtoms::aria_level) &&
      aStartContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_level, levelStr)) {

    PRInt32 success;
    PRInt32 level = levelStr.ToInteger(&success);
    if (level > 1 && NS_SUCCEEDED(success)) {
      nsCOMPtr<nsIAccessible> currentAccessible = aStartTreeItem, prevAccessible;
      while (PR_TRUE) {
        currentAccessible->GetPreviousSibling(getter_AddRefs(prevAccessible));
        currentAccessible.swap(prevAccessible);
        nsCOMPtr<nsIAccessNode> accessNode = do_QueryInterface(currentAccessible);
        if (!accessNode) {
          break; 
        }
        PRUint32 role = nsAccUtils::Role(currentAccessible);
        if (role != startTreeItemRole)
          continue;

        nsCOMPtr<nsIDOMNode> treeItemNode;
        accessNode->GetDOMNode(getter_AddRefs(treeItemNode));
        nsCOMPtr<nsIContent> treeItemContent = do_QueryInterface(treeItemNode);
        if (treeItemContent &&
            nsAccUtils::HasDefinedARIAToken(treeItemContent,
                                     nsAccessibilityAtoms::aria_level) &&
            treeItemContent->GetAttr(kNameSpaceID_None,
                                     nsAccessibilityAtoms::aria_level, levelStr)) {
          if (levelStr.ToInteger(&success) < level && NS_SUCCEEDED(success)) {
            NS_ADDREF(*aTreeItemParentResult = currentAccessible);
            return;
          }
        }
      }
    }
  }

  
  

  if (startTreeItemRole == nsIAccessibleRole::ROLE_ROW) {
    NS_ADDREF(*aTreeItemParentResult = parentAccessible);
    return; 
  }

  
  
  
  

  PRUint32 role = nsAccUtils::Role(parentAccessible);
  if (role != nsIAccessibleRole::ROLE_GROUPING) {
    NS_ADDREF(*aTreeItemParentResult = parentAccessible);
    return; 
  }

  nsCOMPtr<nsIAccessible> prevAccessible;
  parentAccessible->GetPreviousSibling(getter_AddRefs(prevAccessible));
  if (!prevAccessible)
    return;
  role = nsAccUtils::Role(prevAccessible);
  if (role == nsIAccessibleRole::ROLE_TEXT_LEAF) {
    
    
    
    nsCOMPtr<nsIAccessible> tempAccessible = prevAccessible;
    tempAccessible->GetPreviousSibling(getter_AddRefs(prevAccessible));
    if (!prevAccessible)
      return;
    role = nsAccUtils::Role(prevAccessible);
  }
  if (role == nsIAccessibleRole::ROLE_OUTLINEITEM) {
    
    NS_ADDREF(*aTreeItemParentResult = prevAccessible);
  }
}

nsAccessible *
nsAccUtils::GetSelectableContainer(nsAccessible *aAccessible, PRUint32 aState)
{
  if (!aAccessible)
    return nsnull;

  if (!(aState & nsIAccessibleStates::STATE_SELECTABLE))
    return nsnull;

  nsCOMPtr<nsIAccessibleSelectable> container;
  nsAccessible *parent = aAccessible;
  while (!container) {
    parent = parent->GetParent();
    if (!parent || Role(parent) == nsIAccessibleRole::ROLE_PANE)
      return nsnull;

    container = do_QueryObject(parent);
  }

  return parent;
}

nsAccessible *
nsAccUtils::GetMultiSelectableContainer(nsINode *aNode)
{
  nsAccessible *accessible = GetAccService()->GetAccessible(aNode);
  nsAccessible *container = GetSelectableContainer(accessible,
                                                   State(accessible));

  if (State(container) & nsIAccessibleStates::STATE_MULTISELECTABLE)
    return container;
  return nsnull;
}

PRBool
nsAccUtils::IsARIASelected(nsAccessible *aAccessible)
{
  return aAccessible->GetContent()->
    AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::aria_selected,
                nsAccessibilityAtoms::_true, eCaseMatters);
}

already_AddRefed<nsHyperTextAccessible>
nsAccUtils::GetTextAccessibleFromSelection(nsISelection *aSelection,
                                           nsINode **aNode)
{
  
  

  nsCOMPtr<nsIDOMNode> focusDOMNode;
  aSelection->GetFocusNode(getter_AddRefs(focusDOMNode));
  if (!focusDOMNode)
    return nsnull;

  PRInt32 focusOffset = 0;
  aSelection->GetFocusOffset(&focusOffset);

  nsCOMPtr<nsINode> focusNode(do_QueryInterface(focusDOMNode));
  nsCOMPtr<nsINode> resultNode =
    nsCoreUtils::GetDOMNodeFromDOMPoint(focusNode, focusOffset);

  
  while (resultNode) {
    
    
    resultNode = GetAccService()->GetRelevantContentNodeFor(resultNode);
    if (!resultNode->IsNodeOfType(nsINode::eTEXT)) {
      nsAccessible *accessible = GetAccService()->GetAccessible(resultNode);
      if (accessible) {
        nsHyperTextAccessible *textAcc = nsnull;
        CallQueryInterface(accessible, &textAcc);
        if (textAcc) {
          if (aNode)
            NS_ADDREF(*aNode = resultNode);

          return textAcc;
        }
      }
    }

    resultNode = resultNode->GetNodeParent();
  }

  NS_NOTREACHED("No nsIAccessibleText for selection change event!");

  return nsnull;
}

nsresult
nsAccUtils::ConvertToScreenCoords(PRInt32 aX, PRInt32 aY,
                                  PRUint32 aCoordinateType,
                                  nsAccessNode *aAccessNode,
                                  nsIntPoint *aCoords)
{
  NS_ENSURE_ARG_POINTER(aCoords);

  aCoords->MoveTo(aX, aY);

  switch (aCoordinateType) {
    case nsIAccessibleCoordinateType::COORDTYPE_SCREEN_RELATIVE:
      break;

    case nsIAccessibleCoordinateType::COORDTYPE_WINDOW_RELATIVE:
    {
      NS_ENSURE_ARG(aAccessNode);
      *aCoords += GetScreenCoordsForWindow(aAccessNode);
      break;
    }

    case nsIAccessibleCoordinateType::COORDTYPE_PARENT_RELATIVE:
    {
      NS_ENSURE_ARG(aAccessNode);
      *aCoords += GetScreenCoordsForParent(aAccessNode);
      break;
    }

    default:
      return NS_ERROR_INVALID_ARG;
  }

  return NS_OK;
}

nsresult
nsAccUtils::ConvertScreenCoordsTo(PRInt32 *aX, PRInt32 *aY,
                                  PRUint32 aCoordinateType,
                                  nsAccessNode *aAccessNode)
{
  switch (aCoordinateType) {
    case nsIAccessibleCoordinateType::COORDTYPE_SCREEN_RELATIVE:
      break;

    case nsIAccessibleCoordinateType::COORDTYPE_WINDOW_RELATIVE:
    {
      NS_ENSURE_ARG(aAccessNode);
      nsIntPoint coords = nsAccUtils::GetScreenCoordsForWindow(aAccessNode);
      *aX -= coords.x;
      *aY -= coords.y;
      break;
    }

    case nsIAccessibleCoordinateType::COORDTYPE_PARENT_RELATIVE:
    {
      NS_ENSURE_ARG(aAccessNode);
      nsIntPoint coords = nsAccUtils::GetScreenCoordsForParent(aAccessNode);
      *aX -= coords.x;
      *aY -= coords.y;
      break;
    }

    default:
      return NS_ERROR_INVALID_ARG;
  }

  return NS_OK;
}

nsIntPoint
nsAccUtils::GetScreenCoordsForWindow(nsAccessNode *aAccessNode)
{
  return nsCoreUtils::GetScreenCoordsForWindow(aAccessNode->GetNode());
}

nsIntPoint
nsAccUtils::GetScreenCoordsForParent(nsAccessNode *aAccessNode)
{
  nsAccessible *parent =
    GetAccService()->GetContainerAccessible(aAccessNode->GetNode(), PR_TRUE);
  if (!parent)
    return nsIntPoint(0, 0);

  nsIFrame *parentFrame = parent->GetFrame();
  if (!parentFrame)
    return nsIntPoint(0, 0);

  nsIntRect parentRect = parentFrame->GetScreenRectExternal();
  return nsIntPoint(parentRect.x, parentRect.y);
}

nsRoleMapEntry*
nsAccUtils::GetRoleMapEntry(nsINode *aNode)
{
  nsIContent *content = nsCoreUtils::GetRoleContent(aNode);
  nsAutoString roleString;
  if (!content ||
      !content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::role, roleString) ||
      roleString.IsEmpty()) {
    
    return nsnull;
  }

  nsWhitespaceTokenizer tokenizer(roleString);
  while (tokenizer.hasMoreTokens()) {
    
    NS_LossyConvertUTF16toASCII role(tokenizer.nextToken());
    PRUint32 low = 0;
    PRUint32 high = nsARIAMap::gWAIRoleMapLength;
    while (low < high) {
      PRUint32 index = (low + high) / 2;
      PRInt32 compare = PL_strcmp(role.get(), nsARIAMap::gWAIRoleMap[index].roleString);
      if (compare == 0) {
        
        return &nsARIAMap::gWAIRoleMap[index];
      }
      if (compare < 0) {
        high = index;
      }
      else {
        low = index + 1;
      }
    }
  }

  
  
  return &nsARIAMap::gLandmarkRoleMap;
}

PRUint32
nsAccUtils::RoleInternal(nsIAccessible *aAcc)
{
  PRUint32 role = nsIAccessibleRole::ROLE_NOTHING;
  if (aAcc) {
    nsAccessible* accessible = nsnull;
    CallQueryInterface(aAcc, &accessible);

    if (accessible) {
      accessible->GetRoleInternal(&role);
      NS_RELEASE(accessible);
    }
  }

  return role;
}

PRUint8
nsAccUtils::GetAttributeCharacteristics(nsIAtom* aAtom)
{
    for (PRUint32 i = 0; i < nsARIAMap::gWAIUnivAttrMapLength; i++)
      if (*nsARIAMap::gWAIUnivAttrMap[i].attributeName == aAtom)
        return nsARIAMap::gWAIUnivAttrMap[i].characteristics;

    return 0;
}

PRBool
nsAccUtils::GetLiveAttrValue(PRUint32 aRule, nsAString& aValue)
{
  switch (aRule) {
    case eOffLiveAttr:
      aValue = NS_LITERAL_STRING("off");
      return PR_TRUE;
    case ePoliteLiveAttr:
      aValue = NS_LITERAL_STRING("polite");
      return PR_TRUE;
  }

  return PR_FALSE;
}

#ifdef DEBUG_A11Y

PRBool
nsAccUtils::IsTextInterfaceSupportCorrect(nsAccessible *aAccessible)
{
  PRBool foundText = PR_FALSE;
  
  nsCOMPtr<nsIAccessibleDocument> accDoc = do_QueryObject(aAccessible);
  if (accDoc) {
    
    
    return PR_TRUE;
  }

  PRInt32 childCount = aAccessible->GetChildCount();
  for (PRint32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsAccessible *child = GetChildAt(childIdx);
    if (IsText(child)) {
      foundText = PR_TRUE;
      break;
    }
  }

  if (foundText) {
    
    nsCOMPtr<nsIAccessibleText> text = do_QueryObject(aAccessible);
    if (!text)
      return PR_FALSE;
  }

  return PR_TRUE; 
}
#endif

PRUint32
nsAccUtils::TextLength(nsAccessible *aAccessible)
{
  if (!IsText(aAccessible))
    return 1;

  nsIFrame *frame = aAccessible->GetFrame();
  if (frame && frame->GetType() == nsAccessibilityAtoms::textFrame) {
    
    
    nsIContent *content = frame->GetContent();
    if (content) {
      PRUint32 length;
      nsresult rv = nsHyperTextAccessible::
        ContentToRenderedOffset(frame, content->TextLength(), &length);
      if (NS_FAILED(rv)) {
        NS_NOTREACHED("Failed to get rendered offset!");
        return 0;
      }

      return length;
    }
  }

  
  
  
  
  nsAutoString text;
  aAccessible->AppendTextTo(text, 0, PR_UINT32_MAX); 
  return text.Length();
}

PRBool
nsAccUtils::MustPrune(nsIAccessible *aAccessible)
{ 
  PRUint32 role = nsAccUtils::Role(aAccessible);

  
  
  
  return role == nsIAccessibleRole::ROLE_MENUITEM || 
    role == nsIAccessibleRole::ROLE_COMBOBOX_OPTION ||
    role == nsIAccessibleRole::ROLE_OPTION ||
    role == nsIAccessibleRole::ROLE_ENTRY ||
    role == nsIAccessibleRole::ROLE_FLAT_EQUATION ||
    role == nsIAccessibleRole::ROLE_PASSWORD_TEXT ||
    role == nsIAccessibleRole::ROLE_TOGGLE_BUTTON ||
    role == nsIAccessibleRole::ROLE_GRAPHIC ||
    role == nsIAccessibleRole::ROLE_SLIDER ||
    role == nsIAccessibleRole::ROLE_PROGRESSBAR ||
    role == nsIAccessibleRole::ROLE_SEPARATOR;
}

PRBool
nsAccUtils::IsNodeRelevant(nsINode *aNode)
{
  return aNode == GetAccService()->GetRelevantContentNodeFor(aNode);
}

nsresult
nsAccUtils::GetHeaderCellsFor(nsIAccessibleTable *aTable,
                              nsIAccessibleTableCell *aCell,
                              PRInt32 aRowOrColHeaderCells, nsIArray **aCells)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIMutableArray> cells = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 rowIdx = -1;
  rv = aCell->GetRowIndex(&rowIdx);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 colIdx = -1;
  rv = aCell->GetColumnIndex(&colIdx);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool moveToLeft = aRowOrColHeaderCells == eRowHeaderCells;

  
  PRInt32 index = (moveToLeft ? colIdx : rowIdx) - 1;
  for (; index >= 0; index--) {
    PRInt32 curRowIdx = moveToLeft ? rowIdx : index;
    PRInt32 curColIdx = moveToLeft ? index : colIdx;

    nsCOMPtr<nsIAccessible> cell;
    rv = aTable->GetCellAt(curRowIdx, curColIdx, getter_AddRefs(cell));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIAccessibleTableCell> tableCellAcc =
      do_QueryInterface(cell);

    PRInt32 origIdx = 1;
    if (moveToLeft)
      rv = tableCellAcc->GetColumnIndex(&origIdx);
    else
      rv = tableCellAcc->GetRowIndex(&origIdx);
    NS_ENSURE_SUCCESS(rv, rv);

    if (origIdx == index) {
      
      PRUint32 role = Role(cell);
      PRBool isHeader = moveToLeft ?
        role == nsIAccessibleRole::ROLE_ROWHEADER :
        role == nsIAccessibleRole::ROLE_COLUMNHEADER;

      if (isHeader)
        cells->AppendElement(cell, PR_FALSE);
    }
  }

  NS_ADDREF(*aCells = cells);
  return NS_OK;
}
