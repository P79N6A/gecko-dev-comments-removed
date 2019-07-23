







































#ifndef _ACCESSIBLE_COMPONENt_H
#define _ACCESSIBLE_COMPONENT_H

#include "nsISupports.h"

#include "AccessibleComponent.h"

class CAccessibleComponent: public nsISupports,
                            public IAccessibleComponent
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

protected:

  


  HRESULT GetARGBValueFromCSSProperty(const nsAString& aPropName,
                                      IA2Color *aColorValue);
};

#endif

