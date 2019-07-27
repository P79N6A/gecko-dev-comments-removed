





#ifndef mozilla_a11y_Accessible_inl_h_
#define mozilla_a11y_Accessible_inl_h_

#include "DocAccessible.h"
#include "ARIAMap.h"
#include "nsCoreUtils.h"

namespace mozilla {
namespace a11y {

inline mozilla::a11y::role
Accessible::Role()
{
  if (!mRoleMapEntry || mRoleMapEntry->roleRule != kUseMapRole)
    return ARIATransformRole(NativeRole());

  return ARIATransformRole(mRoleMapEntry->role);
}

inline bool
Accessible::IsARIARole(nsIAtom* aARIARole) const
{
  return mRoleMapEntry && mRoleMapEntry->Is(aARIARole);
}

inline bool
Accessible::HasStrongARIARole() const
{
  return mRoleMapEntry && mRoleMapEntry->roleRule == kUseMapRole;
}

inline mozilla::a11y::role
Accessible::ARIARole()
{
  if (!mRoleMapEntry || mRoleMapEntry->roleRule != kUseMapRole)
    return mozilla::a11y::roles::NOTHING;

  return ARIATransformRole(mRoleMapEntry->role);
}

inline bool
Accessible::HasGenericType(AccGenericType aType) const
{
  return (mGenericTypes & aType) ||
    (mRoleMapEntry && mRoleMapEntry->IsOfType(aType));
}

inline bool
Accessible::HasNumericValue() const
{
  if (mStateFlags & eHasNumericValue)
    return true;

  return mRoleMapEntry && mRoleMapEntry->valueRule != eNoValue;
}

inline void
Accessible::ScrollTo(uint32_t aHow) const
{
  if (mContent)
    nsCoreUtils::ScrollTo(mDoc->PresShell(), mContent, aHow);
}

} 
} 

#endif
