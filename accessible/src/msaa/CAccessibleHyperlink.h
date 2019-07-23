







































#ifndef _ACCESSIBLE_HYPERLINK_H
#define _ACCESSIBLE_HYPERLINK_H

#include "nsISupports.h"

#include "CAccessibleAction.h"
#include "AccessibleHyperlink.h"

class CAccessibleHyperlink: public CAccessibleAction,
                            public IAccessibleHyperlink
{
public:

  
  STDMETHODIMP QueryInterface(REFIID, void**);

  
  FORWARD_IACCESSIBLEACTION(CAccessibleAction)

  virtual  HRESULT STDMETHODCALLTYPE get_anchor(
       long index,
       VARIANT *anchor);

  virtual  HRESULT STDMETHODCALLTYPE get_anchorTarget(
       long index,
       VARIANT *anchorTarget);

  virtual  HRESULT STDMETHODCALLTYPE get_startIndex(
       long *index);

  virtual  HRESULT STDMETHODCALLTYPE get_endIndex(
       long *index);

  virtual  HRESULT STDMETHODCALLTYPE get_valid(
       boolean *valid);

};

#endif

