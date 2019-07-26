






#ifndef _ACCESSIBLE_VALUE_H
#define _ACCESSIBLE_VALUE_H

#include "AccessibleValue.h"

class ia2AccessibleValue: public IAccessibleValue
{
public:

  
  STDMETHODIMP QueryInterface(REFIID, void**);

  
  virtual  HRESULT STDMETHODCALLTYPE get_currentValue(
       VARIANT *currentValue);

  virtual HRESULT STDMETHODCALLTYPE setCurrentValue(
       VARIANT value);

  virtual  HRESULT STDMETHODCALLTYPE get_maximumValue(
       VARIANT *maximumValue);

  virtual  HRESULT STDMETHODCALLTYPE get_minimumValue(
       VARIANT *minimumValue);

};

#endif

