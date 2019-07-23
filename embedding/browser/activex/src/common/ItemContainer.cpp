





































#include "StdAfx.h"

#include "ItemContainer.h"

CItemContainer::CItemContainer()
{
}

CItemContainer::~CItemContainer()
{
}





HRESULT STDMETHODCALLTYPE CItemContainer::ParseDisplayName( IBindCtx __RPC_FAR *pbc,  LPOLESTR pszDisplayName,  ULONG __RPC_FAR *pchEaten,  IMoniker __RPC_FAR *__RPC_FAR *ppmkOut)
{
    
    return E_NOTIMPL;
}






HRESULT STDMETHODCALLTYPE CItemContainer::EnumObjects( DWORD grfFlags,  IEnumUnknown __RPC_FAR *__RPC_FAR *ppenum)
{
    HRESULT hr = E_NOTIMPL;

























    return hr;
}
        

HRESULT STDMETHODCALLTYPE CItemContainer::LockContainer( BOOL fLock)
{
    
    return S_OK;
}






HRESULT STDMETHODCALLTYPE CItemContainer::GetObject( LPOLESTR pszItem,  DWORD dwSpeedNeeded,  IBindCtx __RPC_FAR *pbc,  REFIID riid,  void __RPC_FAR *__RPC_FAR *ppvObject)
{
    if (pszItem == NULL)
    {
        return E_INVALIDARG;
    }
    if (ppvObject == NULL)
    {
        return E_INVALIDARG;
    }

    *ppvObject = NULL;
    
    return MK_E_NOOBJECT;
}


HRESULT STDMETHODCALLTYPE CItemContainer::GetObjectStorage( LPOLESTR pszItem,  IBindCtx __RPC_FAR *pbc,  REFIID riid,  void __RPC_FAR *__RPC_FAR *ppvStorage)
{
    
    return MK_E_NOOBJECT;
}


HRESULT STDMETHODCALLTYPE CItemContainer::IsRunning( LPOLESTR pszItem)
{
    
    return MK_E_NOOBJECT;
}
