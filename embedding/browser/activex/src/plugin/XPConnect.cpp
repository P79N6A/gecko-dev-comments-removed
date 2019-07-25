




































#include "stdafx.h"

#ifdef XPCOM_GLUE
#include "nsXPCOMGlue.h"
#endif

#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"

#include "nsIMozAxPlugin.h"
#include "nsIVariant.h"
#include "nsMemory.h"

#include "nsIDOMDocument.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMWindow.h"

#include "nsIInterfaceRequestorUtils.h"
#include "nsIEventListenerManager.h"

#include "nsIScriptEventManager.h"
#include "jsapi.h"

#include "LegacyPlugin.h"
#include "XPConnect.h"

#ifdef XPC_IDISPATCH_SUPPORT
#include "nsIDOMWindowInternal.h"
#include "nsIDOMLocation.h"
#include "nsNetUtil.h"
#ifdef XPCOM_GLUE
#include "nsEmbedString.h"
#endif
#include "nsIURI.h"
#endif





nsScriptablePeer::nsScriptablePeer() :
    mTearOff(new nsScriptablePeerTearOff(this))
{
    NS_ASSERTION(mTearOff, "can't create tearoff");
    MozAxPlugin::AddRef();
}

nsScriptablePeer::~nsScriptablePeer()
{
    delete mTearOff;
    MozAxPlugin::Release();
}

NS_IMPL_ADDREF(nsScriptablePeer)
NS_IMPL_RELEASE(nsScriptablePeer)


NS_IMETHODIMP
nsScriptablePeer::QueryInterface(const nsIID & aIID, void **aInstancePtr)
{
    NS_ASSERTION(aInstancePtr, "QueryInterface requires a non-NULL destination!");
    if (!aInstancePtr)
        return NS_ERROR_NULL_POINTER;
    *aInstancePtr = nsnull;

    nsISupports* foundInterface = nsnull;
    if (aIID.Equals(NS_GET_IID(nsISupports)))
        foundInterface = static_cast<nsISupports *>(static_cast<nsIClassInfo *>(this));
    else if (aIID.Equals(NS_GET_IID(nsIClassInfo)))
        foundInterface = static_cast<nsIClassInfo*>(this);
    else if (aIID.Equals(NS_GET_IID(nsIMozAxPlugin)))
        foundInterface = static_cast<nsIMozAxPlugin*>(this);
    else if (memcmp(&aIID, &__uuidof(IDispatch), sizeof(nsID)) == 0)
    {
        HRESULT hr = mTearOff->QueryInterface(__uuidof(IDispatch), aInstancePtr);
        if (SUCCEEDED(hr)) return NS_OK;
        return E_FAIL;
    }

    NS_IF_ADDREF(foundInterface);
    *aInstancePtr = foundInterface;
    return (*aInstancePtr) ? NS_OK : NS_NOINTERFACE;
}

HRESULT
nsScriptablePeer::GetIDispatch(IDispatch **pdisp)
{
    if (pdisp == NULL)
    {
        return E_INVALIDARG;
    }
    *pdisp = NULL;

    IUnknownPtr unk;
    HRESULT hr = mPlugin->pControlSite->GetControlUnknown(&unk);
    if (unk.GetInterfacePtr() == NULL)
    {
		return E_FAIL; 
	}

    IDispatchPtr disp = unk;
    if (disp.GetInterfacePtr() == NULL)
    { 
        return E_FAIL; 
    }

    *pdisp = disp.GetInterfacePtr();
    (*pdisp)->AddRef();

    return S_OK;
}

nsresult
nsScriptablePeer::HR2NS(HRESULT hr) const
{
    switch (hr)
    {
    case S_OK:
        return NS_OK;
    case E_NOTIMPL:
        return NS_ERROR_NOT_IMPLEMENTED;
    case E_NOINTERFACE:
        return NS_ERROR_NO_INTERFACE;
    case E_INVALIDARG:
        return NS_ERROR_INVALID_ARG;
    case E_ABORT:
        return NS_ERROR_ABORT;
    case E_UNEXPECTED:
        return NS_ERROR_UNEXPECTED;
    case E_OUTOFMEMORY:
        return NS_ERROR_OUT_OF_MEMORY;
    case E_POINTER:
        return NS_ERROR_INVALID_POINTER;
    case E_FAIL:
    default:
        return NS_ERROR_FAILURE;
    }
}

HRESULT
nsScriptablePeer::ConvertVariants(nsIVariant *aIn, VARIANT *aOut)
{
    if (aIn == NULL || aOut == NULL)
    {
        return NS_ERROR_INVALID_ARG;
    }

    PRBool isWritable = PR_FALSE;
    nsCOMPtr<nsIWritableVariant> writable = do_QueryInterface(aIn);
    if (writable)
    {
        writable->GetWritable(&isWritable);
    }

    PRUint16 type;
    nsresult rv = aIn->GetDataType(&type);
    switch (type)
    {
    case nsIDataType::VTYPE_INT8:
        {
            PRUint8 value = 0;
            rv = aIn->GetAsInt8(&value);
            aOut->vt = VT_I1;
            aOut->cVal = value;
        }
        break;
    case nsIDataType::VTYPE_INT16:
        {
            PRInt16 value = 0;
            rv = aIn->GetAsInt16(&value);
            aOut->vt = VT_I2;
            aOut->iVal = value;
        }
        break;
    case nsIDataType::VTYPE_INT32:
        {
            PRInt32 value = 0;
            rv = aIn->GetAsInt32(&value);
            aOut->vt = VT_I4;
            aOut->lVal = value;
        }
        break;
    case nsIDataType::VTYPE_CHAR:
    case nsIDataType::VTYPE_UINT8:
        {
            PRUint8 value = 0;
            rv = aIn->GetAsInt8(&value);
            aOut->vt = VT_UI1;
            aOut->bVal = value;
        }
        break;
    case nsIDataType::VTYPE_WCHAR:
    case nsIDataType::VTYPE_UINT16:
        {
            PRUint16 value = 0;
            rv = aIn->GetAsUint16(&value);
            aOut->vt = VT_I2;
            aOut->uiVal = value;
        }
        break;
    case nsIDataType::VTYPE_UINT32:
        {
            PRUint32 value = 0;
            rv = aIn->GetAsUint32(&value);
            aOut->vt = VT_I4;
            aOut->ulVal = value;
        }
        break;
    case nsIDataType::VTYPE_FLOAT:
        {
            float value = 0;
            rv = aIn->GetAsFloat(&value);
            aOut->vt = VT_R4;
            aOut->fltVal = value;
        }
        break;
    case nsIDataType::VTYPE_DOUBLE:
        {
            double value = 0;
            rv = aIn->GetAsDouble(&value);
            aOut->vt = VT_R4;
            aOut->dblVal = value;
        }
        break;
    case nsIDataType::VTYPE_BOOL:
        {
            PRBool value = 0;
            rv = aIn->GetAsBool(&value);
            aOut->vt = VT_BOOL;
            aOut->dblVal = value ? VARIANT_TRUE : VARIANT_FALSE;
        }
        break;
    case nsIDataType::VTYPE_EMPTY:
        VariantClear(aOut);
        break;

    case nsIDataType::VTYPE_STRING_SIZE_IS:
    case nsIDataType::VTYPE_CHAR_STR:
        {
            nsCString value;
            aIn->GetAsString(getter_Copies(value));
            nsString valueWide;
            NS_CStringToUTF16(value, NS_CSTRING_ENCODING_ASCII, valueWide);
            aOut->vt = VT_BSTR;
            aOut->bstrVal = SysAllocString(valueWide.get());
        }
        break;
    case nsIDataType::VTYPE_WSTRING_SIZE_IS:
    case nsIDataType::VTYPE_WCHAR_STR:
        {
            nsString value;
            aIn->GetAsWString(getter_Copies(value));
            aOut->vt = VT_BSTR;
            aOut->bstrVal = SysAllocString(value.get());
        }
        break;

    case nsIDataType::VTYPE_ASTRING:
        {
            nsAutoString value;
            aIn->GetAsAString(value);
            aOut->vt = VT_BSTR;
            aOut->bstrVal = SysAllocString(value.get());
        }
        break;

    case nsIDataType::VTYPE_DOMSTRING:
        {
            nsAutoString value;
            aIn->GetAsAString(value);
            aOut->vt = VT_BSTR;
            aOut->bstrVal = SysAllocString(value.get());
        }
        break;

    case nsIDataType::VTYPE_CSTRING:
        {
            nsCAutoString value;
            aIn->GetAsACString(value);
            nsAutoString valueWide;
            NS_CStringToUTF16(value, NS_CSTRING_ENCODING_ASCII, valueWide);
            aOut->vt = VT_BSTR;
            aOut->bstrVal = SysAllocString(valueWide.get());
        }
        break;
    case nsIDataType::VTYPE_UTF8STRING:
        {
            nsCAutoString value;
            aIn->GetAsAUTF8String(value);
            nsAutoString valueWide;
            CopyUTF8toUTF16(value, valueWide);
            aOut->vt = VT_BSTR;
            aOut->bstrVal = SysAllocString(valueWide.get());
        }

    
    default:
    case nsIDataType::VTYPE_INT64:
    case nsIDataType::VTYPE_UINT64:
    case nsIDataType::VTYPE_VOID:
    case nsIDataType::VTYPE_ID:
    case nsIDataType::VTYPE_INTERFACE:
    case nsIDataType::VTYPE_INTERFACE_IS:
    case nsIDataType::VTYPE_ARRAY:
    case nsIDataType::VTYPE_EMPTY_ARRAY:
        return E_INVALIDARG;
    }

    return S_OK;
}


HRESULT
nsScriptablePeer::ConvertVariants(VARIANT *aIn, nsIVariant **aOut)
{
    if (aIn == NULL || aOut == NULL)
    {
        return NS_ERROR_INVALID_ARG;
    }

    *aOut = nsnull;

    nsresult rv;
    nsCOMPtr<nsIWritableVariant> v = do_CreateInstance("@mozilla.org/variant;1", &rv);

    
    
    
    if (NS_FAILED(rv))
    {
        
        
        HMODULE hlib = ::LoadLibraryW(L"xpcom.dll");
        if (hlib)
        {
            nsIComponentManager *pManager = nsnull; 
            typedef nsresult (*Moz1XGetComponentManagerFunc)(nsIComponentManager* *result);
            Moz1XGetComponentManagerFunc compMgr = (Moz1XGetComponentManagerFunc)
              ::GetProcAddress(hlib, "NS_GetComponentManager");
            if (compMgr)
            {
                compMgr(&pManager);
                if (pManager)
                {
                    rv = pManager->CreateInstanceByContractID("@mozilla.org/variant;1",
                        nsnull, NS_GET_IID(nsIWritableVariant),
                        getter_AddRefs(v));
                    pManager->Release();
                }
            }
            ::FreeLibrary(hlib);
        }
    }
    
    NS_ENSURE_SUCCESS(rv, rv);

    switch (aIn->vt)
    {
    case VT_EMPTY:
        v->SetAsEmpty();
        break;
    case VT_BSTR:
        v->SetAsWString(aIn->bstrVal);
        break;
    case VT_I1:
        v->SetAsInt8(aIn->cVal);
        break;
    case VT_I2:
        v->SetAsInt16(aIn->iVal);
        break;
    case VT_I4:
        v->SetAsInt32(aIn->lVal);
        break;
    case VT_UI1:
        v->SetAsUint8(aIn->bVal);
        break;
    case VT_UI2:
        v->SetAsUint16(aIn->uiVal);
        break;
    case VT_UI4:
        v->SetAsUint32(aIn->ulVal);
        break;
    case VT_BOOL:
        v->SetAsBool(aIn->boolVal == VARIANT_TRUE ? PR_TRUE : PR_FALSE);
        break;
    case VT_R4:
        v->SetAsFloat(aIn->fltVal);
        break;
    case VT_R8:
        v->SetAsDouble(aIn->dblVal);
        break;
    }

    *aOut = v;
    NS_ADDREF(*aOut);

    return NS_OK;
} 

NS_IMETHODIMP
nsScriptablePeer::InternalInvoke(const char *aMethod, unsigned int aNumArgs, nsIVariant *aArgs[])
{
    HRESULT hr;
    DISPID dispid;

    IDispatchPtr disp;
    if (FAILED(GetIDispatch(&disp)))
    {
        return NPERR_GENERIC_ERROR; 
    }

    USES_CONVERSION;
    OLECHAR* szMember = A2OLE(aMethod);
    hr = disp->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_USER_DEFAULT, &dispid);
    if (FAILED(hr))
    { 
        return NPERR_GENERIC_ERROR; 
    }
    
    _variant_t *pArgs = NULL;
    if (aNumArgs > 0)
    {
        pArgs = new _variant_t[aNumArgs];
        if (pArgs == NULL)
        {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        for (unsigned int i = 0; i < aNumArgs; i++)
        {
            hr = ConvertVariants(aArgs[i], &pArgs[i]);
            if (FAILED(hr))
            {
                delete []pArgs;
                return NS_ERROR_INVALID_ARG;
            }
        }
    }

    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    _variant_t vResult;

    dispparams.cArgs = aNumArgs;
    dispparams.rgvarg = pArgs;

    hr = disp->Invoke(
        dispid,
        IID_NULL,
        LOCALE_USER_DEFAULT,
        DISPATCH_METHOD,
        &dispparams, &vResult, NULL, NULL);

    if (pArgs)
    {
        delete []pArgs;
    }

    if (FAILED(hr))
    { 
        return NPERR_GENERIC_ERROR; 
    }

    return NS_OK;
}




NS_IMETHODIMP 
nsScriptablePeer::Invoke(const char *aMethod)
{
    return InternalInvoke(aMethod, 0, NULL);
}

NS_IMETHODIMP 
nsScriptablePeer::Invoke1(const char *aMethod, nsIVariant *a)
{
    nsIVariant *args[1];
    args[0] = a;
    return InternalInvoke(aMethod, sizeof(args) / sizeof(args[0]), args);
}

NS_IMETHODIMP 
nsScriptablePeer::Invoke2(const char *aMethod, nsIVariant *a, nsIVariant *b)
{
    nsIVariant *args[2];
    args[0] = a;
    args[1] = b;
    return InternalInvoke(aMethod, sizeof(args) / sizeof(args[0]), args);
}

NS_IMETHODIMP 
nsScriptablePeer::Invoke3(const char *aMethod, nsIVariant *a, nsIVariant *b, nsIVariant *c)
{
    nsIVariant *args[3];
    args[0] = a;
    args[1] = b;
    args[2] = c;
    return InternalInvoke(aMethod, sizeof(args) / sizeof(args[0]), args);
}

NS_IMETHODIMP 
nsScriptablePeer::Invoke4(const char *aMethod, nsIVariant *a, nsIVariant *b, nsIVariant *c, nsIVariant *d)
{
    nsIVariant *args[4];
    args[0] = a;
    args[1] = b;
    args[2] = c;
    args[3] = d;
    return InternalInvoke(aMethod, sizeof(args) / sizeof(args[0]), args);
}

NS_IMETHODIMP 
nsScriptablePeer::GetProperty(const char *propertyName, nsIVariant **_retval)
{
    HRESULT hr;
    DISPID dispid;
    IDispatchPtr disp;
    if (FAILED(GetIDispatch(&disp)))
    {
        return NPERR_GENERIC_ERROR; 
    }
    USES_CONVERSION;
    OLECHAR* szMember = A2OLE(propertyName);
    hr = disp->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_USER_DEFAULT, &dispid);
    if (FAILED(hr))
    { 
        return NPERR_GENERIC_ERROR;
    }

    _variant_t vResult;
    
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    hr = disp->Invoke(
        dispid,
        IID_NULL,
        LOCALE_USER_DEFAULT,
        DISPATCH_PROPERTYGET,
        &dispparamsNoArgs, &vResult, NULL, NULL);
    
    if (FAILED(hr))
    { 
        return NPERR_GENERIC_ERROR;
    }

    nsCOMPtr<nsIVariant> propertyValue;
    ConvertVariants(&vResult, getter_AddRefs(propertyValue));
    *_retval = propertyValue;
    NS_IF_ADDREF(*_retval);

    return NS_OK;
}


NS_IMETHODIMP
nsScriptablePeer::SetProperty(const char *propertyName, nsIVariant *propertyValue)
{
    HRESULT hr;
    DISPID dispid;
    IDispatchPtr disp;
    if (FAILED(GetIDispatch(&disp)))
    {
        return NPERR_GENERIC_ERROR; 
    }
    USES_CONVERSION;
    OLECHAR* szMember = A2OLE(propertyName);
    hr = disp->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_USER_DEFAULT, &dispid);
    if (FAILED(hr))
    { 
        return NPERR_GENERIC_ERROR;
    }

    _variant_t v;
    ConvertVariants(propertyValue, &v);
    
    DISPID dispIdPut = DISPID_PROPERTYPUT;
    DISPPARAMS functionArgs;
    _variant_t vResult;
    
    functionArgs.rgdispidNamedArgs = &dispIdPut;
    functionArgs.rgvarg = &v;
    functionArgs.cArgs = 1;
    functionArgs.cNamedArgs = 1;

    hr = disp->Invoke(
        dispid,
        IID_NULL,
        LOCALE_USER_DEFAULT,
        DISPATCH_PROPERTYPUT,
        &functionArgs, &vResult, NULL, NULL);
    
    if (FAILED(hr))
    { 
        return NPERR_GENERIC_ERROR;
    }

    return NS_OK;
}




#ifdef XPC_IDISPATCH_SUPPORT
HRESULT
nsEventSink::InternalInvoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    FUNCDESC *pFuncDesc = NULL;
    HRESULT hr = S_OK;
    CComBSTR bstrName;

    
    if (m_spEventSinkTypeInfo)
    {
	    HRESULT hr = S_OK;
	    TYPEATTR* pAttr;
	    hr = m_spEventSinkTypeInfo->GetTypeAttr(&pAttr);
        if (pAttr)
        {
	        int i;
	        for (i = 0; i < pAttr->cFuncs;i++)
	        {
		        hr = m_spEventSinkTypeInfo->GetFuncDesc(i, &pFuncDesc);
		        if (FAILED(hr))
			        return hr;
		        if (pFuncDesc->memid == dispIdMember)
                {
                    UINT cNames = 0;
                    m_spEventSinkTypeInfo->GetNames(dispIdMember, &bstrName, 1, &cNames);
			        break;
                }
                
		        m_spEventSinkTypeInfo->ReleaseFuncDesc(pFuncDesc);
		        pFuncDesc = NULL;
	        }
	        m_spEventSinkTypeInfo->ReleaseTypeAttr(pAttr);
        }
    }
    if (!pFuncDesc)
    {
        
        return S_OK;
    }

#ifdef DEBUG
    {
        
        ATLTRACE(_T("Invoke(%d)\n"), (int) dispIdMember);

        ATLTRACE(_T("  "));

        
        switch (pFuncDesc->elemdescFunc.tdesc.vt)
        {
        case VT_HRESULT:     ATLTRACE(_T("HRESULT")); break;
        case VT_VOID:        ATLTRACE(_T("void")); break;
        default:             ATLTRACE(_T("void /* vt = %d */"), pFuncDesc->elemdescFunc.tdesc.vt); break;
        }

        
        ATLTRACE(_T(" %S("), SUCCEEDED(hr) ? bstrName.m_str : L"?unknown?");

        
        for (int i = 0; i < pFuncDesc->cParams; i++)
        {
            USHORT  paramFlags = pFuncDesc->lprgelemdescParam[i].paramdesc.wParamFlags;
            ATLTRACE("[");
            BOOL addComma = FALSE;
            if (paramFlags & PARAMFLAG_FIN)
            {
                ATLTRACE(_T("in")); addComma = TRUE;
            }
            if (paramFlags & PARAMFLAG_FOUT)
            {
                ATLTRACE(addComma ? _T(",out") : _T("out")); addComma = TRUE;
            }
            if (paramFlags & PARAMFLAG_FRETVAL)
            {
                ATLTRACE(addComma ? _T(",retval") : _T("retval")); addComma = TRUE;
            }
            ATLTRACE("] ");

            VARTYPE vt = pFuncDesc->lprgelemdescParam[i].tdesc.vt;
            switch (vt)
            {
            case VT_HRESULT:     ATLTRACE(_T("HRESULT")); break;
            case VT_VARIANT:     ATLTRACE(_T("VARIANT")); break;
            case VT_I2:          ATLTRACE(_T("short")); break;
            case VT_I4:          ATLTRACE(_T("long")); break;
            case VT_R8:          ATLTRACE(_T("double")); break;
            case VT_BOOL:        ATLTRACE(_T("VARIANT_BOOL")); break;
            case VT_BSTR:        ATLTRACE(_T("BSTR")); break;
            case VT_DISPATCH:    ATLTRACE(_T("IDispatch *")); break;
            case VT_UNKNOWN:     ATLTRACE(_T("IUnknown *")); break;
            case VT_USERDEFINED: ATLTRACE(_T("/* Userdefined */")); break;
            case VT_PTR:         ATLTRACE(_T("void *")); break;
            case VT_VOID:        ATLTRACE(_T("void")); break;
            
            default:             ATLTRACE(_T("/* vt = %d */"), vt); break;
            }
            if (i + 1 < pFuncDesc->cParams)
            {
                ATLTRACE(_T(", "));
            }
        }
        ATLTRACE(_T(");\n"));
    }
#endif
    m_spEventSinkTypeInfo->ReleaseFuncDesc(pFuncDesc);
    pFuncDesc = NULL;

    nsCOMPtr<nsIDOMElement> element;
    NPN_GetValue(mPlugin->pPluginInstance, NPNVDOMElement, 
        static_cast<nsIDOMElement **>(getter_AddRefs(element)));
    if (!element)
    {
        NS_ERROR("can't get the object element");
        return S_OK;
    }
    nsAutoString id;
    if (NS_FAILED(element->GetAttribute(NS_LITERAL_STRING("id"), id)) ||
        id.IsEmpty())
    {
        
        return S_OK;
    }

    nsDependentString eventName(bstrName.m_str);

    
    nsCOMPtr<nsIDOMWindow> window;
    NPN_GetValue(mPlugin->pPluginInstance, NPNVDOMWindow, 
        static_cast<nsIDOMWindow **>(getter_AddRefs(window)));
    
    nsCOMPtr<nsIScriptEventManager> eventManager(do_GetInterface(window));
    if (!eventManager) return S_OK;

    nsCOMPtr<nsISupports> handler;

    eventManager->FindEventHandler(id, eventName, pDispParams->cArgs, getter_AddRefs(handler));
    if (!handler)
    {
        return S_OK;
    }

    
    
    
    
    
    
    const int kMaxArgsOnStack = 10;

    PRUint32 argc = pDispParams->cArgs;
    jsval *args = nsnull;
    jsval stackArgs[kMaxArgsOnStack];

    
    
    if (argc > kMaxArgsOnStack)
    {
        args = new jsval[argc];
        if (!args) return S_OK;
    }
    else if (argc)
    {
        
        args = stackArgs;
    }

    if (argc)
    {
        nsCOMPtr<nsIDispatchSupport> disp(do_GetService("@mozilla.org/nsdispatchsupport;1"));
        for (UINT i = 0; i < argc; i++)
        {
            
            disp->COMVariant2JSVal(&pDispParams->rgvarg[argc - 1 - i], &args[i]);
        }
    }

    
    eventManager->InvokeEventHandler(handler, element, args, argc);

    
    if (args != stackArgs)
    {
        delete [] args;
    }

    

    

    

    if (pExcepInfo)
    {
        pExcepInfo->wCode = 0;
    }

    return S_OK;
}
#endif






nsScriptablePeerTearOff::nsScriptablePeerTearOff(nsScriptablePeer *pOwner) :
    mOwner(pOwner)
{
    NS_ASSERTION(mOwner, "no owner");
}

HRESULT STDMETHODCALLTYPE nsScriptablePeerTearOff::QueryInterface(REFIID riid, void **ppvObject)
{
    if (::IsEqualIID(riid, _uuidof(IDispatch)))
    {
        *ppvObject = dynamic_cast<IDispatch *>(this);
        mOwner->AddRef();
        return NS_OK;
    }
    nsID iid;
    memcpy(&iid, &riid, sizeof(nsID));
    return mOwner->QueryInterface(iid, ppvObject);
}

ULONG STDMETHODCALLTYPE nsScriptablePeerTearOff::AddRef()
{
    return mOwner->AddRef();
}

ULONG STDMETHODCALLTYPE nsScriptablePeerTearOff::Release()
{
    return mOwner->Release();
}


HRESULT STDMETHODCALLTYPE nsScriptablePeerTearOff::GetTypeInfoCount(UINT __RPC_FAR *pctinfo)
{
    CComPtr<IDispatch> disp;
    if (FAILED(mOwner->GetIDispatch(&disp)))
    {
        return E_UNEXPECTED;
    }
    return disp->GetTypeInfoCount(pctinfo);
}

HRESULT STDMETHODCALLTYPE nsScriptablePeerTearOff::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{
    CComPtr<IDispatch> disp;
    if (FAILED(mOwner->GetIDispatch(&disp)))
    {
        return E_UNEXPECTED;
    }
    return disp->GetTypeInfo(iTInfo, lcid, ppTInfo);
}

HRESULT STDMETHODCALLTYPE nsScriptablePeerTearOff::GetIDsOfNames(REFIID riid, LPOLESTR __RPC_FAR *rgszNames, UINT cNames, LCID lcid, DISPID __RPC_FAR *rgDispId)
{
    CComPtr<IDispatch> disp;
    if (FAILED(mOwner->GetIDispatch(&disp)))
    {
        return E_UNEXPECTED;
    }
    return disp->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);
}

HRESULT STDMETHODCALLTYPE nsScriptablePeerTearOff::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr)
{
    CComPtr<IDispatch> disp;
    if (FAILED(mOwner->GetIDispatch(&disp)))
    {
        return E_UNEXPECTED;
    }
    return disp->Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}







static PRUint32 gInstances = 0;

void MozAxPlugin::AddRef()
{
    if (gInstances == 0)
    {
#ifdef XPCOM_GLUE
        XPCOMGlueStartup(nsnull);
#endif
        MozAxPlugin::PrefGetHostingFlags(); 
    }
    gInstances++;
}

void MozAxPlugin::Release()
{
    if (--gInstances == 0)
    {
#ifdef XPC_IDISPATCH_SUPPORT
        MozAxPlugin::ReleasePrefObserver();
#endif
#ifdef XPCOM_GLUE
        XPCOMGlueShutdown();
#endif
    }
}

#ifdef XPC_IDISPATCH_SUPPORT
nsresult MozAxPlugin::GetCurrentLocation(NPP instance, nsIURI **aLocation)
{
    NS_ENSURE_ARG_POINTER(aLocation);
    *aLocation = nsnull;
    nsCOMPtr<nsIDOMWindow> domWindow;
    NPN_GetValue(instance, NPNVDOMWindow, (void *) &domWindow);
    if (!domWindow)
    {
        return NS_ERROR_FAILURE;
    }
    nsCOMPtr<nsIDOMWindowInternal> windowInternal = do_QueryInterface(domWindow);
    if (!windowInternal)
    {
        return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIDOMLocation> location;
#ifdef XPCOM_GLUE
    nsEmbedString href;
#else
    nsAutoString href;
#endif
    windowInternal->GetLocation(getter_AddRefs(location));
    if (!location ||
        NS_FAILED(location->GetHref(href)))
    {
        return NS_ERROR_FAILURE;
    }

    return NS_NewURI(aLocation, href);
}
#endif

CLSID MozAxPlugin::GetCLSIDForType(const char *mimeType)
{
    if (mimeType == NULL)
    {
        return CLSID_NULL;
    }

    
    
    USES_CONVERSION;
    CRegKey keyMimeDB;
    if (keyMimeDB.Open(HKEY_CLASSES_ROOT, _T("MIME\\Database\\Content Type"), KEY_READ) == ERROR_SUCCESS)
    {
        CRegKey keyMimeType;
        if (keyMimeType.Open(keyMimeDB, A2CT(mimeType), KEY_READ) == ERROR_SUCCESS)
        {
	        USES_CONVERSION;
	        TCHAR szGUID[64];
	        ULONG nCount = (sizeof(szGUID) / sizeof(szGUID[0])) - 1;

            GUID guidValue = GUID_NULL;
            if (keyMimeType.QueryValue(szGUID, _T("CLSID"), &nCount) == ERROR_SUCCESS &&
                SUCCEEDED(::CLSIDFromString(T2OLE(szGUID), &guidValue)))
            {
                return guidValue;
            }
        }
    }
    return CLSID_NULL;
}


nsScriptablePeer *
MozAxPlugin::GetPeerForCLSID(const CLSID &clsid)
{
    return new nsScriptablePeer();
}

void
MozAxPlugin::GetIIDForCLSID(const CLSID &clsid, nsIID &iid)
{
#ifdef XPC_IDISPATCH_SUPPORT
    memcpy(&iid, &__uuidof(IDispatch), sizeof(iid));
#else
    iid = NS_GET_IID(nsIMozAxPlugin);
#endif
}


NPError
MozAxPlugin::GetValue(NPP instance, NPPVariable variable, void *value)
{
    if (instance == NULL)
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    PluginInstanceData *pData = (PluginInstanceData *) instance->pdata;
    if (!pData ||
        !pData->pControlSite ||
        !pData->pControlSite->IsObjectValid())
    {
        return NPERR_GENERIC_ERROR;
    }

    

#ifdef XPC_IDISPATCH_SUPPORT
    PRUint32 hostingFlags = MozAxPlugin::PrefGetHostingFlags();
    if (hostingFlags & nsIActiveXSecurityPolicy::HOSTING_FLAGS_SCRIPT_SAFE_OBJECTS &&
        !(hostingFlags & nsIActiveXSecurityPolicy::HOSTING_FLAGS_SCRIPT_ALL_OBJECTS))
    {
        
        nsCOMPtr<nsIDispatchSupport> dispSupport = do_GetService(NS_IDISPATCH_SUPPORT_CONTRACTID);
        if (!dispSupport) return NPERR_GENERIC_ERROR;
        
        PRBool isScriptable = PR_FALSE;

        
        nsIID iid;
        memcpy(&iid, &__uuidof(IDispatch), sizeof(iid));
        CComPtr<IUnknown> controlUnk;
        pData->pControlSite->GetControlUnknown(&controlUnk);
        dispSupport->IsObjectSafeForScripting(reinterpret_cast<void *>(controlUnk.p), iid, &isScriptable);

        
        if (!isScriptable)
        {
            PRBool classExists = PR_FALSE;
            nsCID cid;
            memcpy(&cid, &pData->clsid, sizeof(cid));
            dispSupport->IsClassMarkedSafeForScripting(cid, &classExists, &isScriptable);
        }
        if (!isScriptable)
        {
            return NPERR_GENERIC_ERROR;
        }
    }
    else if (hostingFlags & nsIActiveXSecurityPolicy::HOSTING_FLAGS_SCRIPT_ALL_OBJECTS)
    {
        
    }
    else
    {
        return NPERR_GENERIC_ERROR;
    }
#else
    
    if (!pData->pControlSite->IsObjectSafeForScripting(__uuidof(IDispatch)))
    {
        return NPERR_GENERIC_ERROR;
    }
#endif

    
    

    const int kVarScriptableInstance = 10; 
    const int kVarScriptableIID = 11; 

    if (variable == kVarScriptableInstance)
    {
        if (!pData->pScriptingPeer)
        {
            nsScriptablePeer *peer = MozAxPlugin::GetPeerForCLSID(pData->clsid);
            if (peer)
            {
                peer->AddRef();
                pData->pScriptingPeer = (nsIMozAxPlugin *) peer;
                peer->mPlugin = pData;
            }
        }
        if (pData->pScriptingPeer)
        {
            pData->pScriptingPeer->AddRef();
            *((nsISupports **) value)= pData->pScriptingPeer;
            return NPERR_NO_ERROR;
        }
    }
    else if (variable == kVarScriptableIID)
    {
        nsIID *piid = (nsIID *) NPN_MemAlloc(sizeof(nsIID));
        GetIIDForCLSID(pData->clsid, *piid);
        *((nsIID **) value) = piid;
        return NPERR_NO_ERROR;
    }
    return NPERR_GENERIC_ERROR;
}
