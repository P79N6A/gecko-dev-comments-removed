






#ifndef _NS_ACCESSIBLE_RELATION_WRAP_H
#define _NS_ACCESSIBLE_RELATION_WRAP_H

#include "Accessible.h"

#include "nsTArray.h"

#include "AccessibleRelation.h"

namespace mozilla {
namespace a11y {

class ia2AccessibleRelation : public IAccessibleRelation
{
public:
  ia2AccessibleRelation(uint32_t aType, Relation* aRel);
  virtual ~ia2AccessibleRelation() { }

  
  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID aIID, void** aOutPtr);
  virtual ULONG STDMETHODCALLTYPE AddRef();
  virtual ULONG STDMETHODCALLTYPE Release();

  
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

  uint32_t mType;
  nsTArray<nsRefPtr<Accessible> > mTargets;
  ULONG mReferences;
};

} 
} 

#endif

