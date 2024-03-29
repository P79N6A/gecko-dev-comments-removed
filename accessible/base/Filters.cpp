



#include "Filters.h"

#include "Accessible-inl.h"
#include "nsAccUtils.h"
#include "Role.h"
#include "States.h"

using namespace mozilla::a11y;
using namespace mozilla::a11y::filters;

uint32_t
filters::GetSelected(Accessible* aAccessible)
{
  if (aAccessible->State() & states::SELECTED)
    return eMatch | eSkipSubtree;

  return eSkip;
}

uint32_t
filters::GetSelectable(Accessible* aAccessible)
{
  if (aAccessible->InteractiveState() & states::SELECTABLE)
    return eMatch | eSkipSubtree;

  return eSkip;
}

uint32_t
filters::GetRow(Accessible* aAccessible)
{
  a11y::role role = aAccessible->Role();
  if (role == roles::ROW)
    return eMatch | eSkipSubtree;

  
  if (role == roles::GROUPING)
    return eSkip;

  return eSkipSubtree;
}

uint32_t
filters::GetCell(Accessible* aAccessible)
{
  a11y::role role = aAccessible->Role();
  return role == roles::CELL || role == roles::GRID_CELL ||
         role == roles::ROWHEADER || role == roles::COLUMNHEADER ?
    eMatch : eSkipSubtree;
}

uint32_t
filters::GetEmbeddedObject(Accessible* aAccessible)
{
  return nsAccUtils::IsEmbeddedObject(aAccessible) ?
    eMatch | eSkipSubtree : eSkipSubtree;
}
