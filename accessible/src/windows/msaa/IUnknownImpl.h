






#ifndef mozilla_a11y_IUnknownImpl_h_
#define mozilla_a11y_IUnknownImpl_h_

#include <windows.h>
#include "nsError.h"





#ifdef _MSC_VER
#pragma warning( disable : 4509 )
#endif

namespace mozilla {
namespace a11y {

class AutoRefCnt
{
public:
  AutoRefCnt() : mValue(0) {}

  ULONG operator++() { return ++mValue; }
  ULONG operator--() { return --mValue; }
  ULONG operator++(int) { return ++mValue; }
  ULONG operator--(int) { return --mValue; }

  operator ULONG() const { return mValue; }

private:
  ULONG mValue;
};

} 
} 

#define DECL_IUNKNOWN                                                          \
public:                                                                        \
  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void**);            \
  virtual ULONG STDMETHODCALLTYPE AddRef() MOZ_FINAL                           \
  {                                                                            \
    MOZ_ASSERT(int32_t(mRefCnt) >= 0, "illegal refcnt");                       \
    ++mRefCnt;                                                                 \
    return mRefCnt;                                                            \
  }                                                                            \
  virtual ULONG STDMETHODCALLTYPE Release() MOZ_FINAL                          \
  {                                                                            \
     MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");                          \
     --mRefCnt;                                                                \
     if (mRefCnt)                                                              \
       return mRefCnt;                                                         \
                                                                               \
     delete this;                                                              \
     return 0;                                                                 \
  }                                                                            \
private:                                                                       \
  mozilla::a11y::AutoRefCnt mRefCnt;                                           \
public:

#define DECL_IUNKNOWN_INHERITED                                                \
public:                                                                        \
virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void**);              \

#define IMPL_IUNKNOWN_QUERY_HEAD(Class)                                        \
STDMETHODIMP                                                                   \
Class::QueryInterface(REFIID aIID, void** aInstancePtr)                        \
{                                                                              \
  A11Y_TRYBLOCK_BEGIN                                                          \
  if (!aInstancePtr)                                                           \
    return E_INVALIDARG;                                                       \
  *aInstancePtr = nullptr;                                                     \
                                                                               \
  HRESULT hr = E_NOINTERFACE;

#define IMPL_IUNKNOWN_QUERY_TAIL                                               \
  return hr;                                                                   \
  A11Y_TRYBLOCK_END                                                            \
}

#define IMPL_IUNKNOWN_QUERY_IFACE(Iface)                                       \
  if (aIID == IID_##Iface) {                                                   \
    *aInstancePtr = static_cast<Iface*>(this);                                 \
    AddRef();                                                                  \
    return S_OK;                                                               \
  }

#define IMPL_IUNKNOWN_QUERY_IFACE_AMBIGIOUS(Iface, aResolveIface)              \
  if (aIID == IID_##Iface) {                                                   \
    *aInstancePtr = static_cast<Iface*>(static_cast<aResolveIface*>(this));    \
    AddRef();                                                                  \
    return S_OK;                                                               \
  }

#define IMPL_IUNKNOWN_QUERY_CLASS(Class)                                       \
  hr = Class::QueryInterface(aIID, aInstancePtr);                              \
  if (SUCCEEDED(hr))                                                           \
    return hr;

#define IMPL_IUNKNOWN_QUERY_CLASS_COND(Class, Cond)                            \
  if (Cond) {                                                                  \
    hr = Class::QueryInterface(aIID, aInstancePtr);                            \
    if (SUCCEEDED(hr))                                                         \
      return hr;                                                               \
  }

#define IMPL_IUNKNOWN_QUERY_AGGR_COND(Member, Cond)                            \
  if (Cond) {                                                                  \
    hr = Member->QueryInterface(aIID, aInstancePtr);                           \
    if (SUCCEEDED(hr))                                                         \
      return hr;                                                               \
  }

#define IMPL_IUNKNOWN1(Class, I1)                                              \
  IMPL_IUNKNOWN_QUERY_HEAD(Class)                                              \
  IMPL_IUNKNOWN_QUERY_IFACE(I1);                                               \
  IMPL_IUNKNOWN_QUERY_IFACE(IUnknown);                                         \
  IMPL_IUNKNOWN_QUERY_TAIL                                                     \

#define IMPL_IUNKNOWN2(Class, I1, I2)                                          \
  IMPL_IUNKNOWN_QUERY_HEAD(Class)                                              \
  IMPL_IUNKNOWN_QUERY_IFACE(I1);                                               \
  IMPL_IUNKNOWN_QUERY_IFACE(I2);                                               \
  IMPL_IUNKNOWN_QUERY_IFACE_AMBIGIOUS(IUnknown, I1);                           \
  IMPL_IUNKNOWN_QUERY_TAIL                                                     \

#define IMPL_IUNKNOWN_INHERITED1(Class, Super0, Super1)                        \
  IMPL_IUNKNOWN_QUERY_HEAD(Class)                                              \
  IMPL_IUNKNOWN_QUERY_CLASS(Super1);                                           \
  IMPL_IUNKNOWN_QUERY_CLASS(Super0)                                            \
  IMPL_IUNKNOWN_QUERY_TAIL                                                     \

#define IMPL_IUNKNOWN_INHERITED2(Class, Super0, Super1, Super2)                \
  IMPL_IUNKNOWN_QUERY_HEAD(Class)                                              \
  IMPL_IUNKNOWN_QUERY_CLASS(Super1);                                           \
  IMPL_IUNKNOWN_QUERY_CLASS(Super2);                                           \
  IMPL_IUNKNOWN_QUERY_CLASS(Super0)                                            \
  IMPL_IUNKNOWN_QUERY_TAIL






#define A11Y_TRYBLOCK_BEGIN                                                    \
  MOZ_SEH_TRY {

#define A11Y_TRYBLOCK_END                                                      \
  } MOZ_SEH_EXCEPT(mozilla::a11y::FilterExceptions(::GetExceptionCode(),       \
                                                   GetExceptionInformation())) \
  { }                                                                          \
  return E_FAIL;


namespace mozilla {
namespace a11y {




HRESULT GetHRESULT(nsresult aResult);




int FilterExceptions(unsigned int aCode, EXCEPTION_POINTERS* aExceptionInfo);

} 
} 

#endif
