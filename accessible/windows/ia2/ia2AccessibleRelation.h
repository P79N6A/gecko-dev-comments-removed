






#ifndef _NS_ACCESSIBLE_RELATION_WRAP_H
#define _NS_ACCESSIBLE_RELATION_WRAP_H

#include "Accessible.h"
#include "IUnknownImpl.h"
#include "nsIAccessibleRelation.h"

#include <utility>
#include "nsTArray.h"

#include "AccessibleRelation.h"

namespace mozilla {
namespace a11y {

class ia2AccessibleRelation MOZ_FINAL : public IAccessibleRelation
{
public:
  ia2AccessibleRelation(RelationType aType, Relation* aRel);

  
  DECL_IUNKNOWN

  
  virtual  HRESULT STDMETHODCALLTYPE get_relationType(
       BSTR *relationType);

  virtual  HRESULT STDMETHODCALLTYPE get_localizedRelationType(
       BSTR *localizedRelationType);

  virtual  HRESULT STDMETHODCALLTYPE get_nTargets(
       long *nTargets);

  virtual  HRESULT STDMETHODCALLTYPE get_target(
       long targetIndex,
       IUnknown **target);

  virtual  HRESULT STDMETHODCALLTYPE get_targets(
       long maxTargets,
       IUnknown **target,
       long *nTargets);

  inline bool HasTargets() const
    { return mTargets.Length(); }

private:
  ia2AccessibleRelation();
  ia2AccessibleRelation(const ia2AccessibleRelation&);
  ia2AccessibleRelation& operator = (const ia2AccessibleRelation&);

  RelationType mType;
  nsTArray<nsRefPtr<Accessible> > mTargets;
};






const WCHAR *const IA2_RELATION_NULL = L"";

#define RELATIONTYPE(geckoType, name, atkType, msaaType, ia2Type) \
  std::pair<RelationType, const WCHAR *const>(RelationType::geckoType, ia2Type),

static const std::pair<RelationType, const WCHAR *const> sRelationTypePairs[] = {
#include "RelationTypeMap.h"
};

#undef RELATIONTYPE

} 
} 

#endif

