




































#ifndef PROPERTYBAG_H
#define PROPERTYBAG_H

#include "PropertyList.h"




class CPropertyBag :    public CComObjectRootEx<CComSingleThreadModel>,
                        public IPropertyBag
{
    
    PropertyList m_PropertyList;

public:
    
    CPropertyBag();
    
    virtual ~CPropertyBag();

BEGIN_COM_MAP(CPropertyBag)
    COM_INTERFACE_ENTRY(IPropertyBag)
END_COM_MAP()


    virtual  HRESULT STDMETHODCALLTYPE Read( LPCOLESTR pszPropName,  VARIANT __RPC_FAR *pVar,  IErrorLog __RPC_FAR *pErrorLog);
    virtual HRESULT STDMETHODCALLTYPE Write( LPCOLESTR pszPropName,  VARIANT __RPC_FAR *pVar);
};

typedef CComObject<CPropertyBag> CPropertyBagInstance;

#endif