






#ifndef _ACCESSIBLE_HYPERLINK_H
#define _ACCESSIBLE_HYPERLINK_H

#include "nsISupports.h"

#include "ia2AccessibleAction.h"
#include "AccessibleHyperlink.h"

class ia2AccessibleHyperlink : public ia2AccessibleAction,
                               public IAccessibleHyperlink
{
public:

  
  STDMETHODIMP QueryInterface(REFIID, void**);

  
  FORWARD_IACCESSIBLEACTION(ia2AccessibleAction)

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

