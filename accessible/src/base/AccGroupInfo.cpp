



#include "AccGroupInfo.h"

#include "Role.h"
#include "States.h"

using namespace mozilla::a11y;

AccGroupInfo::AccGroupInfo(Accessible* aItem, role aRole) :
  mPosInSet(0), mSetSize(0), mParent(nullptr)
{
  MOZ_COUNT_CTOR(AccGroupInfo);
  Accessible* parent = aItem->Parent();
  if (!parent)
    return;

  int32_t indexInParent = aItem->IndexInParent();
  uint32_t siblingCount = parent->ChildCount();
  if (indexInParent == -1 ||
      indexInParent >= static_cast<int32_t>(siblingCount)) {
    NS_ERROR("Wrong index in parent! Tree invalidation problem.");
    return;
  }

  int32_t level = nsAccUtils::GetARIAOrDefaultLevel(aItem);

  
  mPosInSet = 1;
  for (int32_t idx = indexInParent - 1; idx >= 0 ; idx--) {
    Accessible* sibling = parent->GetChildAt(idx);
    roles::Role siblingRole = sibling->Role();

    
    if (siblingRole == roles::SEPARATOR)
      break;

    
    if (BaseRole(siblingRole) != aRole || sibling->State() & states::INVISIBLE)
      continue;

    
    
    
    
    int32_t siblingLevel = nsAccUtils::GetARIAOrDefaultLevel(sibling);
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

  for (uint32_t idx = indexInParent + 1; idx < siblingCount; idx++) {
    Accessible* sibling = parent->GetChildAt(idx);

    roles::Role siblingRole = sibling->Role();

    
    if (siblingRole == roles::SEPARATOR)
      break;

    
    if (BaseRole(siblingRole) != aRole || sibling->State() & states::INVISIBLE)
      continue;

    
    int32_t siblingLevel = nsAccUtils::GetARIAOrDefaultLevel(sibling);
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

  roles::Role parentRole = parent->Role();
  if (IsConceptualParent(aRole, parentRole))
    mParent = parent;

  
  
  
  
  if (parentRole != roles::GROUPING || aRole != roles::OUTLINEITEM)
    return;

  Accessible* parentPrevSibling = parent->PrevSibling();
  if (!parentPrevSibling)
    return;

  roles::Role parentPrevSiblingRole = parentPrevSibling->Role();
  if (parentPrevSiblingRole == roles::TEXT_LEAF) {
    
    
    
    
    parentPrevSibling = parentPrevSibling->PrevSibling();
    if (parentPrevSibling)
      parentPrevSiblingRole = parentPrevSibling->Role();
  }

  
  
  if (parentPrevSiblingRole == roles::OUTLINEITEM)
    mParent = parentPrevSibling;
}

bool
AccGroupInfo::IsConceptualParent(role aRole, role aParentRole)
{
  if (aParentRole == roles::OUTLINE && aRole == roles::OUTLINEITEM)
    return true;
  if ((aParentRole == roles::TABLE || aParentRole == roles::TREE_TABLE) &&
      aRole == roles::ROW)
    return true;
  if (aParentRole == roles::ROW &&
      (aRole == roles::CELL || aRole == roles::GRID_CELL))
    return true;
  if (aParentRole == roles::LIST && aRole == roles::LISTITEM)
    return true;
  if (aParentRole == roles::COMBOBOX_LIST && aRole == roles::COMBOBOX_OPTION)
    return true;
  if (aParentRole == roles::LISTBOX && aRole == roles::OPTION)
    return true;
  if (aParentRole == roles::PAGETABLIST && aRole == roles::PAGETAB)
    return true;
  if ((aParentRole == roles::POPUP_MENU || aParentRole == roles::MENUPOPUP) &&
      aRole == roles::MENUITEM)
    return true;

  return false;
}
