




































#include "filters.h"

#include "nsAccessible.h"
#include "nsAccUtils.h"
#include "Role.h"
#include "States.h"

using namespace mozilla::a11y;

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
  return aAccessible->Role() == roles::ROW;
}

bool
filters::GetCell(nsAccessible* aAccessible)
{
  roles::Role role = aAccessible->Role();
  return role == roles::GRID_CELL || role == roles::ROWHEADER ||
      role == roles::COLUMNHEADER;
}

bool
filters::GetEmbeddedObject(nsAccessible* aAccessible)
{
  return nsAccUtils::IsEmbeddedObject(aAccessible);
}
