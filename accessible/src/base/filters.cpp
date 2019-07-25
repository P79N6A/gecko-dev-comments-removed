



#include "filters.h"

#include "Accessible-inl.h"
#include "nsAccUtils.h"
#include "Role.h"
#include "States.h"

using namespace mozilla::a11y;

bool
filters::GetSelected(Accessible* aAccessible)
{
  return aAccessible->State() & states::SELECTED;
}

bool
filters::GetSelectable(Accessible* aAccessible)
{
  return aAccessible->State() & states::SELECTABLE;
}

bool
filters::GetRow(Accessible* aAccessible)
{
  return aAccessible->Role() == roles::ROW;
}

bool
filters::GetCell(Accessible* aAccessible)
{
  roles::Role role = aAccessible->Role();
  return role == roles::GRID_CELL || role == roles::ROWHEADER ||
      role == roles::COLUMNHEADER;
}

bool
filters::GetEmbeddedObject(Accessible* aAccessible)
{
  return nsAccUtils::IsEmbeddedObject(aAccessible);
}
