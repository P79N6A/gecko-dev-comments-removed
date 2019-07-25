







































#ifndef _ACCESSIBLE_VALUE_H
#define _ACCESSIBLE_VALUE_H

#include "nsISupports.h"

#include "AccessibleValue.h"

class CAccessibleValue: public IAccessibleValue
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

  
  NS_IMETHOD QueryInterface(const nsIID& uuid, void** result) = 0;

};

#endif

