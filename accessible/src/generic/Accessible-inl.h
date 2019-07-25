





#ifndef mozilla_a11y_Accessible_inl_h_
#define mozilla_a11y_Accessible_inl_h_

#include "nsAccessible.h"
#include "nsARIAMap.h"

inline mozilla::a11y::role
nsAccessible::Role()
{
  if (!mRoleMapEntry || mRoleMapEntry->roleRule != kUseMapRole)
    return ARIATransformRole(NativeRole());

  return ARIATransformRole(mRoleMapEntry->role);
}

inline mozilla::a11y::role
nsAccessible::ARIARole()
{
  if (!mRoleMapEntry || mRoleMapEntry->roleRule != kUseMapRole)
    return mozilla::a11y::roles::NOTHING;

  return ARIATransformRole(mRoleMapEntry->role);
}

#endif
