







































#ifndef MOZILLA_A11Y_APPLICATION_ACCESSIBLE_WRAP_H__
#define MOZILLA_A11Y_APPLICATION_ACCESSIBLE_WRAP_H__

#include "ApplicationAccessible.h"

#include "AccessibleApplication.h"

class ApplicationAccessibleWrap: public ApplicationAccessible,
                                 public IAccessibleApplication
{
public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetAttributes(nsIPersistentProperties** aAttributes);

  
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
  static void Unload();
};

#endif

