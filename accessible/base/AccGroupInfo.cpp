



#include "AccGroupInfo.h"
#include "nsAccUtils.h"

#include "Role.h"
#include "States.h"

using namespace mozilla::a11y;

AccGroupInfo::AccGroupInfo(Accessible* aItem, role aRole) :
  mPosInSet(0), mSetSize(0), mParent(nullptr), mItem(aItem), mRole(aRole)
{
  MOZ_COUNT_CTOR(AccGroupInfo);
  Update();
}

void
AccGroupInfo::Update()
{
  Accessible* parent = mItem->Parent();
  if (!parent)
    return;

  int32_t indexInParent = mItem->IndexInParent();
  uint32_t siblingCount = parent->ChildCount();
  if (indexInParent == -1 ||
      indexInParent >= static_cast<int32_t>(siblingCount)) {
    NS_ERROR("Wrong index in parent! Tree invalidation problem.");
    return;
  }

  int32_t level = nsAccUtils::GetARIAOrDefaultLevel(mItem);

  
  mPosInSet = 1;
  for (int32_t idx = indexInParent - 1; idx >= 0 ; idx--) {
    Accessible* sibling = parent->GetChildAt(idx);
    roles::Role siblingRole = sibling->Role();

    
    if (siblingRole == roles::SEPARATOR)
      break;

    
    if (BaseRole(siblingRole) != mRole || sibling->State() & states::INVISIBLE)
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

    
    if (BaseRole(siblingRole) != mRole || sibling->State() & states::INVISIBLE)
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
  if (ShouldReportRelations(mRole, parentRole))
    mParent = parent;

  
  if (parentRole != roles::GROUPING)
    return;

  
  
  
  
  if (mRole == roles::OUTLINEITEM) {
    Accessible* parentPrevSibling = parent->PrevSibling();
    if (parentPrevSibling && parentPrevSibling->Role() == mRole) {
      mParent = parentPrevSibling;
      return;
    }
  }

  
  
  
  if (mRole == roles::LISTITEM || mRole == roles::OUTLINEITEM) {
    Accessible* grandParent = parent->Parent();
    if (grandParent && grandParent->Role() == mRole)
      mParent = grandParent;
  }
}

Accessible*
AccGroupInfo::FirstItemOf(Accessible* aContainer)
{
  
  
  a11y::role containerRole = aContainer->Role();
  Accessible* item = aContainer->NextSibling();
  if (item) {
    if (containerRole == roles::OUTLINEITEM && item->Role() == roles::GROUPING)
      item = item->FirstChild();

    if (item) {
      AccGroupInfo* itemGroupInfo = item->GetGroupInfo();
      if (itemGroupInfo && itemGroupInfo->ConceptualParent() == aContainer)
        return item;
    }
  }

  
  
  item = aContainer->LastChild();
  if (!item)
    return nullptr;

  if (item->Role() == roles::GROUPING &&
      (containerRole == roles::LISTITEM || containerRole == roles::OUTLINEITEM)) {
    item = item->FirstChild();
    if (item) {
      AccGroupInfo* itemGroupInfo = item->GetGroupInfo();
      if (itemGroupInfo && itemGroupInfo->ConceptualParent() == aContainer)
        return item;
    }
  }

  
  item = aContainer->FirstChild();
  if (ShouldReportRelations(item->Role(), containerRole))
    return item;

  return nullptr;
}

Accessible*
AccGroupInfo::NextItemTo(Accessible* aItem)
{
  AccGroupInfo* groupInfo = aItem->GetGroupInfo();
  if (!groupInfo)
    return nullptr;

  
  if (groupInfo->PosInSet() >= groupInfo->SetSize())
    return nullptr;

  Accessible* parent = aItem->Parent();
  uint32_t childCount = parent->ChildCount();
  for (uint32_t idx = aItem->IndexInParent() + 1; idx < childCount; idx++) {
    Accessible* nextItem = parent->GetChildAt(idx);
    AccGroupInfo* nextGroupInfo = nextItem->GetGroupInfo();
    if (nextGroupInfo &&
        nextGroupInfo->ConceptualParent() == groupInfo->ConceptualParent()) {
      return nextItem;
    }
  }

  NS_NOTREACHED("Item in the middle of the group but there's no next item!");
  return nullptr;
}

bool
AccGroupInfo::ShouldReportRelations(role aRole, role aParentRole)
{
  
  
  if (aParentRole == roles::OUTLINE && aRole == roles::OUTLINEITEM)
    return true;
  if (aParentRole == roles::TREE_TABLE && aRole == roles::ROW)
    return true;
  if (aParentRole == roles::LIST && aRole == roles::LISTITEM)
    return true;

  return false;
}
