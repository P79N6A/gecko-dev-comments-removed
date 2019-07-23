







































#ifndef _NS_ACCESSIBLE_RELATION_WRAP_H
#define _NS_ACCESSIBLE_RELATION_WRAP_H

#include "nsAccessibleRelation.h"
#include "AccessibleRelation.h"

#include "nsIWinAccessNode.h"
#include "nsISupports.h"

class nsAccessibleRelationWrap: public nsAccessibleRelation,
                                public nsIWinAccessNode,
                                public IAccessibleRelation
{
public:
  nsAccessibleRelationWrap(PRUint32 aType, nsIAccessible *aTarget);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIWINACCESSNODE

  
  STDMETHODIMP QueryInterface(REFIID, void**);

  
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

};

#endif

