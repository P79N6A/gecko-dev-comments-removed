




































#include "filters.h"

#include "nsAccessible.h"
#include "nsAccUtils.h"

bool
filters::GetSelected(nsAccessible* aAccessible)
{
  return nsAccUtils::State(aAccessible) & nsIAccessibleStates::STATE_SELECTED;
}

bool
filters::GetSelectable(nsAccessible* aAccessible)
{
  return nsAccUtils::State(aAccessible) & nsIAccessibleStates::STATE_SELECTABLE;
}

bool
filters::GetRow(nsAccessible* aAccessible)
{
  return nsAccUtils::Role(aAccessible) == nsIAccessibleRole::ROLE_ROW;
}

bool
filters::GetCell(nsAccessible* aAccessible)
{
  PRUint32 role = nsAccUtils::Role(aAccessible);
  return role == nsIAccessibleRole::ROLE_GRID_CELL ||
      role == nsIAccessibleRole::ROLE_ROWHEADER ||
      role == nsIAccessibleRole::ROLE_COLUMNHEADER;
}

bool
filters::GetEmbeddedObject(nsAccessible* aAccessible)
{
  return nsAccUtils::IsEmbeddedObject(aAccessible);
}
