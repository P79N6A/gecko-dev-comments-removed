





#ifndef mozilla_a11y_Accessible_inl_h_
#define mozilla_a11y_Accessible_inl_h_

#include "Accessible.h"
#include "nsARIAMap.h"

namespace mozilla {
namespace a11y {

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

inline void
Accessible::SetRoleMapEntry(nsRoleMapEntry* aRoleMapEntry)
{
  mRoleMapEntry = aRoleMapEntry;
  if (mRoleMapEntry)
    mFlags |= mRoleMapEntry->accTypes;
}

inline bool
Accessible::HasNumericValue() const
{
  if (mStateFlags & eHasNumericValue)
    return true;

  return mRoleMapEntry && mRoleMapEntry->valueRule != eNoValue;
}

} 
} 

#endif
