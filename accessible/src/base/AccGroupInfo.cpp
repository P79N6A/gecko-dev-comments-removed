




































#include "AccGroupInfo.h"

#include "States.h"

using namespace mozilla::a11y;

AccGroupInfo::AccGroupInfo(nsAccessible* aItem, PRUint32 aRole) :
  mPosInSet(0), mSetSize(0), mParent(nsnull)
{
  MOZ_COUNT_CTOR(AccGroupInfo);
  nsAccessible* parent = aItem->Parent();
  if (!parent)
    return;

  PRInt32 indexInParent = aItem->IndexInParent();
  PRInt32 siblingCount = parent->GetChildCount();
  if (siblingCount < indexInParent) {
    NS_ERROR("Wrong index in parent! Tree invalidation problem.");
    return;
  }

  PRInt32 level = nsAccUtils::GetARIAOrDefaultLevel(aItem);

  
  mPosInSet = 1;
  for (PRInt32 idx = indexInParent - 1; idx >=0 ; idx--) {
    nsAccessible* sibling = parent->GetChildAt(idx);
    PRUint32 siblingRole = sibling->Role();

    
    if (siblingRole == nsIAccessibleRole::ROLE_SEPARATOR)
      break;

    
    if (BaseRole(siblingRole) != aRole || sibling->State() & states::INVISIBLE)
      continue;

    
    
    
    
    PRInt32 siblingLevel = nsAccUtils::GetARIAOrDefaultLevel(sibling);
    if (siblingLevel < level) {
      mParent = sibling;
      break;
    }

    
    if (siblingLevel > level)
      continue;

    
    
    if (sibling->mGroupInfo) {
      mPosInSet += sibling->mGroupInfo->mPosInSet;
      mParent = sibling->mGroupInfo->mParent;
      mSetSize = sibling->mGroupInfo->mSetSize;
      return;
    }

    mPosInSet++;
  }

  
  mSetSize = mPosInSet;

  for (PRInt32 idx = indexInParent + 1; idx < siblingCount; idx++) {
    nsAccessible* sibling = parent->GetChildAt(idx);

    PRUint32 siblingRole = sibling->Role();

    
    if (siblingRole == nsIAccessibleRole::ROLE_SEPARATOR)
      break;

    
    if (BaseRole(siblingRole) != aRole || sibling->State() & states::INVISIBLE)
      continue;

    
    PRInt32 siblingLevel = nsAccUtils::GetARIAOrDefaultLevel(sibling);
    if (siblingLevel < level)
      break;

    
    if (siblingLevel > level)
      continue;

    
    
    if (sibling->mGroupInfo) {
      mParent = sibling->mGroupInfo->mParent;
      mSetSize = sibling->mGroupInfo->mSetSize;
      return;
    }

    mSetSize++;
  }

  if (mParent)
    return;

  PRUint32 parentRole = parent->Role();
  if (IsConceptualParent(aRole, parentRole))
    mParent = parent;

  
  
  
  
  if (parentRole != nsIAccessibleRole::ROLE_GROUPING ||
      aRole != nsIAccessibleRole::ROLE_OUTLINEITEM)
    return;

  nsAccessible* parentPrevSibling = parent->PrevSibling();
  if (!parentPrevSibling)
    return;

  PRUint32 parentPrevSiblingRole = parentPrevSibling->Role();
  if (parentPrevSiblingRole == nsIAccessibleRole::ROLE_TEXT_LEAF) {
    
    
    
    
    parentPrevSibling = parentPrevSibling->PrevSibling();
    if (parentPrevSibling)
      parentPrevSiblingRole = parentPrevSibling->Role();
  }

  
  
  if (parentPrevSiblingRole == nsIAccessibleRole::ROLE_OUTLINEITEM)
    mParent = parentPrevSibling;
}

bool
AccGroupInfo::IsConceptualParent(PRUint32 aRole, PRUint32 aParentRole)
{
  if (aParentRole == nsIAccessibleRole::ROLE_OUTLINE &&
      aRole == nsIAccessibleRole::ROLE_OUTLINEITEM)
    return true;
  if ((aParentRole == nsIAccessibleRole::ROLE_TABLE ||
       aParentRole == nsIAccessibleRole::ROLE_TREE_TABLE) &&
      aRole == nsIAccessibleRole::ROLE_ROW)
    return true;
  if (aParentRole == nsIAccessibleRole::ROLE_ROW &&
      (aRole == nsIAccessibleRole::ROLE_CELL ||
       aRole == nsIAccessibleRole::ROLE_GRID_CELL))
    return true;
  if (aParentRole == nsIAccessibleRole::ROLE_LIST &&
      aRole == nsIAccessibleRole::ROLE_LISTITEM)
    return true;
  if (aParentRole == nsIAccessibleRole::ROLE_COMBOBOX_LIST &&
      aRole == nsIAccessibleRole::ROLE_COMBOBOX_OPTION)
    return true;
  if (aParentRole == nsIAccessibleRole::ROLE_LISTBOX &&
      aRole == nsIAccessibleRole::ROLE_OPTION)
    return true;
  if (aParentRole == nsIAccessibleRole::ROLE_PAGETABLIST &&
      aRole == nsIAccessibleRole::ROLE_PAGETAB)
    return true;
  if ((aParentRole == nsIAccessibleRole::ROLE_POPUP_MENU ||
       aParentRole == nsIAccessibleRole::ROLE_MENUPOPUP) &&
      aRole == nsIAccessibleRole::ROLE_MENUITEM)
    return true;

  return false;
}
