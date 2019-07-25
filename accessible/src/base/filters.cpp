




































#include "filters.h"

#include "nsAccessible.h"
#include "nsAccUtils.h"
#include "States.h"

bool
filters::GetSelected(nsAccessible* aAccessible)
{
  return aAccessible->State() & states::SELECTED;
}

bool
filters::GetSelectable(nsAccessible* aAccessible)
{
  return aAccessible->State() & states::SELECTABLE;
}

bool
filters::GetRow(nsAccessible* aAccessible)
{
  return aAccessible->Role() == nsIAccessibleRole::ROLE_ROW;
}

bool
filters::GetCell(nsAccessible* aAccessible)
{
  PRUint32 role = aAccessible->Role();
  return role == nsIAccessibleRole::ROLE_GRID_CELL ||
      role == nsIAccessibleRole::ROLE_ROWHEADER ||
      role == nsIAccessibleRole::ROLE_COLUMNHEADER;
}

bool
filters::GetEmbeddedObject(nsAccessible* aAccessible)
{
  return nsAccUtils::IsEmbeddedObject(aAccessible);
}
