







































#ifndef __NS_APPLICATION_ACCESSIBLE_WRAP_H__
#define __NS_APPLICATION_ACCESSIBLE_WRAP_H__

#include "nsApplicationAccessible.h"

#include "AccessibleApplication.h"

class nsApplicationAccessibleWrap: public nsApplicationAccessible,
                                   public IAccessibleApplication
{
public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  STDMETHODIMP QueryInterface(REFIID, void**);

  
  virtual  HRESULT STDMETHODCALLTYPE get_appName(
             BSTR *name);

  virtual  HRESULT STDMETHODCALLTYPE get_appVersion(
       BSTR *version);

  virtual  HRESULT STDMETHODCALLTYPE get_toolkitName(
       BSTR *name);

  virtual  HRESULT STDMETHODCALLTYPE get_toolkitVersion(
           BSTR *version);

public:
  static void PreCreate();
};

#endif

