






#ifndef IA2_ACCESSIBLE_COMPONENT_H_
#define IA2_ACCESSIBLE_COMPONENT_H_

#include "AccessibleComponent.h"

class ia2AccessibleComponent : public IAccessibleComponent
{
public:

  
  STDMETHODIMP QueryInterface(REFIID, void**);

  
  virtual  HRESULT STDMETHODCALLTYPE get_locationInParent(
       long *x,
       long *y);

  virtual  HRESULT STDMETHODCALLTYPE get_foreground(
       IA2Color *foreground);

  virtual  HRESULT STDMETHODCALLTYPE get_background(
       IA2Color *background);
};

#endif

