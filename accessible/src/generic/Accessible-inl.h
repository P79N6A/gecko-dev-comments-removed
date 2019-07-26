





#ifndef mozilla_a11y_Accessible_inl_h_
#define mozilla_a11y_Accessible_inl_h_

#include "Accessible.h"
#include "nsARIAMap.h"

inline mozilla::a11y::role
Accessible::Role()
{
  if (!mRoleMapEntry || mRoleMapEntry->roleRule != kUseMapRole)
    return ARIATransformRole(NativeRole());

  return ARIATransformRole(mRoleMapEntry->role);
}

inline mozilla::a11y::role
Accessible::ARIARole()
{
  if (!mRoleMapEntry || mRoleMapEntry->roleRule != kUseMapRole)
    return mozilla::a11y::roles::NOTHING;

  return ARIATransformRole(mRoleMapEntry->role);
}

inline bool
Accessible::HasNumericValue() const
{
  if (mFlags & eHasNumericValue)
    return true;

  return mRoleMapEntry && mRoleMapEntry->valueRule != eNoValue;
}

#endif
