




































#include "AccGroupInfo.h"

#include "States.h"

AccGroupInfo::AccGroupInfo(nsAccessible* aItem, PRUint32 aRole) :
  mPosInSet(0), mSetSize(0), mParent(nsnull)
{
  MOZ_COUNT_CTOR(AccGroupInfo);
  nsAccessible* parent = aItem->GetParent();
  if (!parent)
    return;

  PRInt32 indexInParent = aItem->IndexInParent();
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

  PRInt32 siblingCount = parent->GetChildCount();
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

  
  
  if (aRole == nsIAccessibleRole::ROLE_ROW &&
      parentRole == nsIAccessibleRole::ROLE_TREE_TABLE) {
    mParent = parent;
    return;
  }

  
  
  
  
  

  if (parentRole != nsIAccessibleRole::ROLE_GROUPING) {
    mParent = parent;
    return;
  }

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
