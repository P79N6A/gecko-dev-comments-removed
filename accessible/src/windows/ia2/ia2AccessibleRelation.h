






#ifndef _NS_ACCESSIBLE_RELATION_WRAP_H
#define _NS_ACCESSIBLE_RELATION_WRAP_H

#include "Accessible.h"
#include "IUnknownImpl.h"
#include "nsIAccessibleRelation.h"

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





static const RelationType sRelationTypesForIA2[] = {
  RelationType::LABELLED_BY,
  RelationType::LABEL_FOR,
  RelationType::DESCRIBED_BY,
  RelationType::DESCRIPTION_FOR,
  RelationType::NODE_CHILD_OF,
  RelationType::NODE_PARENT_OF,
  RelationType::CONTROLLED_BY,
  RelationType::CONTROLLER_FOR,
  RelationType::FLOWS_TO,
  RelationType::FLOWS_FROM,
  RelationType::MEMBER_OF,
  RelationType::SUBWINDOW_OF,
  RelationType::EMBEDS,
  RelationType::EMBEDDED_BY,
  RelationType::POPUP_FOR,
  RelationType::PARENT_WINDOW_OF
};

} 
} 

#endif

