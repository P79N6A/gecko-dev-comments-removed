





#ifndef mozilla_a11y_uiaRawElmProvider_h__
#define mozilla_a11y_uiaRawElmProvider_h__

#include "objbase.h"
#include "AccessibleWrap.h"
#include "uiautomation.h"

namespace mozilla {
namespace a11y {

class AccessibleWrap;




class uiaRawElmProvider MOZ_FINAL : public IAccessibleEx,
                                    public IRawElementProviderSimple
{
public:
  uiaRawElmProvider(AccessibleWrap* aAcc) : mAcc(aAcc), mRefCnt(0) { }

  
  DECL_IUNKNOWN

  
  virtual HRESULT STDMETHODCALLTYPE GetObjectForChild(
     long aIdChild,
     __RPC__deref_out_opt IAccessibleEx** aAccEx);

  virtual HRESULT STDMETHODCALLTYPE GetIAccessiblePair(
     __RPC__deref_out_opt IAccessible** aAcc,
     __RPC__out long* aIdChild);

  virtual HRESULT STDMETHODCALLTYPE GetRuntimeId(
     __RPC__deref_out_opt SAFEARRAY** aRuntimeIds);

  virtual HRESULT STDMETHODCALLTYPE ConvertReturnedElement(
     __RPC__in_opt IRawElementProviderSimple* aRawElmProvider,
     __RPC__deref_out_opt IAccessibleEx** aAccEx);

  
  virtual  HRESULT STDMETHODCALLTYPE get_ProviderOptions(
     __RPC__out enum ProviderOptions* aProviderOptions);

  virtual HRESULT STDMETHODCALLTYPE GetPatternProvider(
     PATTERNID aPatternId,
     __RPC__deref_out_opt IUnknown** aPatternProvider);

  virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(
     PROPERTYID aPropertyId,
     __RPC__out VARIANT* aPropertyValue);

  virtual  HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(
     __RPC__deref_out_opt IRawElementProviderSimple** aRawElmProvider);

private:
  uiaRawElmProvider() MOZ_DELETE;
  uiaRawElmProvider& operator =(const uiaRawElmProvider&) MOZ_DELETE;
  uiaRawElmProvider(const uiaRawElmProvider&) MOZ_DELETE;

protected:
  nsRefPtr<AccessibleWrap> mAcc;
};

} 
} 

#endif
