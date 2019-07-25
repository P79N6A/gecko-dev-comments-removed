





#ifndef mozilla_a11y_DocAccessible_inl_h_
#define mozilla_a11y_DocAccessible_inl_h_

#include "DocAccessible.h"
#include "nsAccessibilityService.h"

inline DocAccessible*
DocAccessible::ParentDocument() const
{
  return GetAccService()->GetDocAccessible(mDocument->GetParentDocument());
}

#endif
