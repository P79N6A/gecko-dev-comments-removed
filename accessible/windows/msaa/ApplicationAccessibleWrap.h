






#ifndef mozilla_a11y_ApplicationAccessibleWrap_h__
#define mozilla_a11y_ApplicationAccessibleWrap_h__

#include "ApplicationAccessible.h"

#include "AccessibleApplication.h"

namespace mozilla {
namespace a11y {
 
class ApplicationAccessibleWrap: public ApplicationAccessible,
                                 public IAccessibleApplication
{
public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual already_AddRefed<nsIPersistentProperties> NativeAttributes() MOZ_OVERRIDE;

  
  STDMETHODIMP QueryInterface(REFIID, void**);

  
  virtual  HRESULT STDMETHODCALLTYPE get_appName(
             BSTR *name);

  virtual  HRESULT STDMETHODCALLTYPE get_appVersion(
       BSTR *version);

  virtual  HRESULT STDMETHODCALLTYPE get_toolkitName(
       BSTR *name);

  virtual  HRESULT STDMETHODCALLTYPE get_toolkitVersion(
           BSTR *version);

};

} 
} 

#endif

