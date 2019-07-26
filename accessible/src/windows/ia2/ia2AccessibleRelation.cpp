






#include "ia2AccessibleRelation.h"

#include "Relation.h"
#include "nsIAccessibleRelation.h"
#include "nsID.h"

#include "AccessibleRelation_i.c"

using namespace mozilla::a11y;

ia2AccessibleRelation::ia2AccessibleRelation(RelationType aType, Relation* aRel) :
  mType(aType)
{
  Accessible* target = nullptr;
  while ((target = aRel->Next()))
    mTargets.AppendElement(target);
}



IMPL_IUNKNOWN_QUERY_HEAD(ia2AccessibleRelation)
  IMPL_IUNKNOWN_QUERY_IFACE(IAccessibleRelation)
  IMPL_IUNKNOWN_QUERY_IFACE(IUnknown)
IMPL_IUNKNOWN_QUERY_TAIL



STDMETHODIMP
ia2AccessibleRelation::get_relationType(BSTR *aRelationType)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aRelationType)
    return E_INVALIDARG;

  *aRelationType = nullptr;

  switch (mType) {
    case RelationType::CONTROLLED_BY:
      *aRelationType = ::SysAllocString(IA2_RELATION_CONTROLLED_BY);
      break;
    case RelationType::CONTROLLER_FOR:
      *aRelationType = ::SysAllocString(IA2_RELATION_CONTROLLER_FOR);
      break;
    case RelationType::DESCRIBED_BY:
      *aRelationType = ::SysAllocString(IA2_RELATION_DESCRIBED_BY);
      break;
    case RelationType::DESCRIPTION_FOR:
      *aRelationType = ::SysAllocString(IA2_RELATION_DESCRIPTION_FOR);
      break;
    case RelationType::EMBEDDED_BY:
      *aRelationType = ::SysAllocString(IA2_RELATION_EMBEDDED_BY);
      break;
    case RelationType::EMBEDS:
      *aRelationType = ::SysAllocString(IA2_RELATION_EMBEDS);
      break;
    case RelationType::FLOWS_FROM:
      *aRelationType = ::SysAllocString(IA2_RELATION_FLOWS_FROM);
      break;
    case RelationType::FLOWS_TO:
      *aRelationType = ::SysAllocString(IA2_RELATION_FLOWS_TO);
      break;
    case RelationType::LABEL_FOR:
      *aRelationType = ::SysAllocString(IA2_RELATION_LABEL_FOR);
      break;
    case RelationType::LABELLED_BY:
      *aRelationType = ::SysAllocString(IA2_RELATION_LABELED_BY);
      break;
    case RelationType::MEMBER_OF:
      *aRelationType = ::SysAllocString(IA2_RELATION_MEMBER_OF);
      break;
    case RelationType::NODE_CHILD_OF:
      *aRelationType = ::SysAllocString(IA2_RELATION_NODE_CHILD_OF);
      break;
    case RelationType::NODE_PARENT_OF:
      *aRelationType = ::SysAllocString(IA2_RELATION_NODE_PARENT_OF);
      break;
    case RelationType::PARENT_WINDOW_OF:
      *aRelationType = ::SysAllocString(IA2_RELATION_PARENT_WINDOW_OF);
      break;
    case RelationType::POPUP_FOR:
      *aRelationType = ::SysAllocString(IA2_RELATION_POPUP_FOR);
      break;
    case RelationType::SUBWINDOW_OF:
      *aRelationType = ::SysAllocString(IA2_RELATION_SUBWINDOW_OF);
      break;
  }

  return *aRelationType ? S_OK : E_OUTOFMEMORY;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleRelation::get_localizedRelationType(BSTR *aLocalizedRelationType)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aLocalizedRelationType)
    return E_INVALIDARG;

  *aLocalizedRelationType = nullptr;
  return E_NOTIMPL;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleRelation::get_nTargets(long *aNTargets)
{
  A11Y_TRYBLOCK_BEGIN

 if (!aNTargets)
   return E_INVALIDARG;

 *aNTargets = mTargets.Length();
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleRelation::get_target(long aTargetIndex, IUnknown **aTarget)
{
  A11Y_TRYBLOCK_BEGIN

  if (aTargetIndex < 0 || (uint32_t)aTargetIndex >= mTargets.Length() || !aTarget)
    return E_INVALIDARG;

  AccessibleWrap* target =
    static_cast<AccessibleWrap*>(mTargets[aTargetIndex].get());
  *aTarget = static_cast<IAccessible*>(target);
  (*aTarget)->AddRef();

  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleRelation::get_targets(long aMaxTargets, IUnknown **aTargets,
                                   long *aNTargets)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aNTargets || !aTargets)
    return E_INVALIDARG;

  *aNTargets = 0;
  long maxTargets = mTargets.Length();
  if (maxTargets > aMaxTargets)
    maxTargets = aMaxTargets;

  for (long idx = 0; idx < maxTargets; idx++)
    get_target(idx, aTargets + idx);

  *aNTargets = maxTargets;
  return S_OK;

  A11Y_TRYBLOCK_END
}

