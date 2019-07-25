





































#include "nsCoreUtils.h"
#include "nsAccUtils.h"

#include "nsIAccessibleTypes.h"
#include "Role.h"
#include "States.h"

#include "nsAccessibilityService.h"
#include "nsARIAMap.h"
#include "nsDocAccessible.h"
#include "nsHyperTextAccessible.h"
#include "nsTextAccessible.h"

#include "nsIDOMXULContainerElement.h"
#include "nsIDOMXULSelectCntrlEl.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsWhitespaceTokenizer.h"
#include "nsComponentManagerUtils.h"

namespace dom = mozilla::dom;
using namespace mozilla::a11y;

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
    SetAccAttr(aAttributes, nsGkAtoms::level, value);
  }

  if (aSetSize && aPosInSet) {
    value.Truncate();
    value.AppendInt(aPosInSet);
    SetAccAttr(aAttributes, nsGkAtoms::posinset, value);

    value.Truncate();
    value.AppendInt(aSetSize);
    SetAccAttr(aAttributes, nsGkAtoms::setsize, value);
  }
}

PRInt32
nsAccUtils::GetDefaultLevel(nsAccessible *aAccessible)
{
  roles::Role role = aAccessible->Role();

  if (role == roles::OUTLINEITEM)
    return 1;

  if (role == roles::ROW) {
    nsAccessible* parent = aAccessible->Parent();
    
    
    if (parent && parent->Role() == roles::TREE_TABLE) 
      return 1;
  }

  return 0;
}

PRInt32
nsAccUtils::GetARIAOrDefaultLevel(nsAccessible *aAccessible)
{
  PRInt32 level = 0;
  nsCoreUtils::GetUIntAttr(aAccessible->GetContent(),
                           nsGkAtoms::aria_level, &level);

  if (level != 0)
    return level;

  return GetDefaultLevel(aAccessible);
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
        nsAccUtils::HasDefinedARIAToken(ancestor, nsGkAtoms::aria_relevant) &&
        ancestor->GetAttr(kNameSpaceID_None, nsGkAtoms::aria_relevant, relevant))
      SetAccAttr(aAttributes, nsGkAtoms::containerRelevant, relevant);

    
    if (live.IsEmpty()) {
      nsRoleMapEntry *role = GetRoleMapEntry(ancestor);
      if (nsAccUtils::HasDefinedARIAToken(ancestor,
                                          nsGkAtoms::aria_live)) {
        ancestor->GetAttr(kNameSpaceID_None, nsGkAtoms::aria_live,
                          live);
      } else if (role) {
        GetLiveAttrValue(role->liveAttRule, live);
      }
      if (!live.IsEmpty()) {
        SetAccAttr(aAttributes, nsGkAtoms::containerLive, live);
        if (role) {
          nsAccUtils::SetAccAttr(aAttributes,
                                 nsGkAtoms::containerLiveRole,
                                 NS_ConvertASCIItoUTF16(role->roleString));
        }
      }
    }

    
    if (atomic.IsEmpty() &&
        nsAccUtils::HasDefinedARIAToken(ancestor, nsGkAtoms::aria_atomic) &&
        ancestor->GetAttr(kNameSpaceID_None, nsGkAtoms::aria_atomic, atomic))
      SetAccAttr(aAttributes, nsGkAtoms::containerAtomic, atomic);

    
    if (busy.IsEmpty() &&
        nsAccUtils::HasDefinedARIAToken(ancestor, nsGkAtoms::aria_busy) &&
        ancestor->GetAttr(kNameSpaceID_None, nsGkAtoms::aria_busy, busy))
      SetAccAttr(aAttributes, nsGkAtoms::containerBusy, busy);

    if (ancestor == aTopContent)
      break;

    ancestor = ancestor->GetParent();
    if (!ancestor)
      ancestor = aTopContent; 
  }
}

bool
nsAccUtils::HasDefinedARIAToken(nsIContent *aContent, nsIAtom *aAtom)
{
  NS_ASSERTION(aContent, "aContent is null in call to HasDefinedARIAToken!");

  if (!aContent->HasAttr(kNameSpaceID_None, aAtom) ||
      aContent->AttrValueIs(kNameSpaceID_None, aAtom,
                            nsGkAtoms::_empty, eCaseMatters) ||
      aContent->AttrValueIs(kNameSpaceID_None, aAtom,
                            nsGkAtoms::_undefined, eCaseMatters)) {
        return false;
  }
  return true;
}

nsIAtom*
nsAccUtils::GetARIAToken(dom::Element* aElement, nsIAtom* aAttr)
{
  if (!nsAccUtils::HasDefinedARIAToken(aElement, aAttr))
    return nsGkAtoms::_empty;

  static nsIContent::AttrValuesArray tokens[] =
    { &nsGkAtoms::_false, &nsGkAtoms::_true,
      &nsGkAtoms::mixed, nsnull};

  PRInt32 idx = aElement->FindAttrValueIn(kNameSpaceID_None,
                                          aAttr, tokens, eCaseMatters);
  if (idx >= 0)
    return *(tokens[idx]);

  return nsnull;
}

nsAccessible*
nsAccUtils::GetAncestorWithRole(nsAccessible *aDescendant, PRUint32 aRole)
{
  nsAccessible* document = aDescendant->Document();
  nsAccessible* parent = aDescendant;
  while ((parent = parent->Parent())) {
    PRUint32 testRole = parent->Role();
    if (testRole == aRole)
      return parent;

    if (parent == document)
      break;
  }
  return nsnull;
}

nsAccessible*
nsAccUtils::GetSelectableContainer(nsAccessible* aAccessible, PRUint64 aState)
{
  if (!aAccessible)
    return nsnull;

  if (!(aState & states::SELECTABLE))
    return nsnull;

  nsAccessible* parent = aAccessible;
  while ((parent = parent->Parent()) && !parent->IsSelect()) {
    if (Role(parent) == nsIAccessibleRole::ROLE_PANE)
      return nsnull;
  }
  return parent;
}

bool
nsAccUtils::IsARIASelected(nsAccessible *aAccessible)
{
  return aAccessible->GetContent()->
    AttrValueIs(kNameSpaceID_None, nsGkAtoms::aria_selected,
                nsGkAtoms::_true, eCaseMatters);
}

nsHyperTextAccessible*
nsAccUtils::GetTextAccessibleFromSelection(nsISelection* aSelection)
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

  
  nsDocAccessible* doc = 
    GetAccService()->GetDocAccessible(resultNode->OwnerDoc());
  nsAccessible* accessible = doc ? 
    doc->GetAccessibleOrContainer(resultNode) : nsnull;
  if (!accessible) {
    NS_NOTREACHED("No nsIAccessibleText for selection change event!");
    return nsnull;
  }

  do {
    nsHyperTextAccessible* textAcc = accessible->AsHyperText();
    if (textAcc)
      return textAcc;

    accessible = accessible->Parent();
  } while (accessible);

  NS_NOTREACHED("We must reach document accessible implementing nsIAccessibleText!");
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
  nsDocAccessible* document = aAccessNode->Document();
  nsAccessible* parent = document->GetContainerAccessible(aAccessNode->GetNode());
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
      !content->GetAttr(kNameSpaceID_None, nsGkAtoms::role, roleString) ||
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

PRUint8
nsAccUtils::GetAttributeCharacteristics(nsIAtom* aAtom)
{
    for (PRUint32 i = 0; i < nsARIAMap::gWAIUnivAttrMapLength; i++)
      if (*nsARIAMap::gWAIUnivAttrMap[i].attributeName == aAtom)
        return nsARIAMap::gWAIUnivAttrMap[i].characteristics;

    return 0;
}

bool
nsAccUtils::GetLiveAttrValue(PRUint32 aRule, nsAString& aValue)
{
  switch (aRule) {
    case eOffLiveAttr:
      aValue = NS_LITERAL_STRING("off");
      return true;
    case ePoliteLiveAttr:
      aValue = NS_LITERAL_STRING("polite");
      return true;
  }

  return false;
}

#ifdef DEBUG_A11Y

bool
nsAccUtils::IsTextInterfaceSupportCorrect(nsAccessible *aAccessible)
{
  
  
  if (aAccessible->IsDoc())
    return true;

  bool foundText = false;
  PRInt32 childCount = aAccessible->GetChildCount();
  for (PRint32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsAccessible *child = GetChildAt(childIdx);
    if (IsText(child)) {
      foundText = true;
      break;
    }
  }

  if (foundText) {
    
    nsCOMPtr<nsIAccessibleText> text = do_QueryObject(aAccessible);
    if (!text)
      return false;
  }

  return true; 
}
#endif

PRUint32
nsAccUtils::TextLength(nsAccessible *aAccessible)
{
  if (!IsText(aAccessible))
    return 1;

  nsTextAccessible* textLeaf = aAccessible->AsTextLeaf();
  if (textLeaf)
    return textLeaf->Text().Length();

  
  
  
  
  nsAutoString text;
  aAccessible->AppendTextTo(text); 
  return text.Length();
}

bool
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

  bool moveToLeft = aRowOrColHeaderCells == eRowHeaderCells;

  
  PRInt32 index = (moveToLeft ? colIdx : rowIdx) - 1;
  for (; index >= 0; index--) {
    PRInt32 curRowIdx = moveToLeft ? rowIdx : index;
    PRInt32 curColIdx = moveToLeft ? index : colIdx;

    nsCOMPtr<nsIAccessible> cell;
    rv = aTable->GetCellAt(curRowIdx, curColIdx, getter_AddRefs(cell));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIAccessibleTableCell> tableCellAcc =
      do_QueryInterface(cell);

    
    NS_ENSURE_STATE(tableCellAcc);

    PRInt32 origIdx = 1;
    if (moveToLeft)
      rv = tableCellAcc->GetColumnIndex(&origIdx);
    else
      rv = tableCellAcc->GetRowIndex(&origIdx);
    NS_ENSURE_SUCCESS(rv, rv);

    if (origIdx == index) {
      
      PRUint32 role = Role(cell);
      bool isHeader = moveToLeft ?
        role == nsIAccessibleRole::ROLE_ROWHEADER :
        role == nsIAccessibleRole::ROLE_COLUMNHEADER;

      if (isHeader)
        cells->AppendElement(cell, false);
    }
  }

  NS_ADDREF(*aCells = cells);
  return NS_OK;
}
