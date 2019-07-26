





#ifndef mozilla_a11y_sdnAccessible_inl_h_
#define mozilla_a11y_sdnAccessible_inl_h_

#include "sdnAccessible.h"

#include "DocAccessible.h"
#include "nsAccessibilityService.h"

namespace mozilla {
namespace a11y {

inline DocAccessible*
sdnAccessible::GetDocument() const
{
  return GetExistingDocAccessible(mNode->OwnerDoc());
}

inline Accessible*
sdnAccessible::GetAccessible() const
{
  DocAccessible* document = GetDocument();
  return document ? document->GetAccessible(mNode) : nullptr;
}

} 
} 

#endif 
