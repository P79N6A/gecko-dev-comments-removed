







































#ifndef _ACCESSIBLE_VALUE_H
#define _ACCESSIBLE_VALUE_H

#include "nsISupports.h"

#include "AccessibleValue.h"

class CAccessibleValue: public nsISupports,
                        public IAccessibleValue
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

