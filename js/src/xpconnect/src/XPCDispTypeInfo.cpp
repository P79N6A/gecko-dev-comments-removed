
















































#include "xpcprivate.h"







inline
void FillOutElemDesc(VARTYPE vt, PRUint16 paramFlags, ELEMDESC & elemDesc)
{
    elemDesc.tdesc.vt = vt;
    elemDesc.paramdesc.wParamFlags = paramFlags;
    elemDesc.paramdesc.pparamdescex = 0;
    elemDesc.tdesc.lptdesc = 0;
    elemDesc.tdesc.lpadesc = 0;
    elemDesc.tdesc.hreftype = 0;
}

XPCDispTypeInfo::~XPCDispTypeInfo()
{
    delete mIDArray;
}

void XPCDispJSPropertyInfo::GetReturnType(XPCCallContext& ccx, ELEMDESC & elemDesc)
{
    VARTYPE vt;
    if(IsSetter())      
    {
        vt = VT_EMPTY;
    }
    else if(IsProperty())   
    {
        vt = XPCDispConvert::JSTypeToCOMType(ccx, mProperty);
    }
    else                    
    {
        vt = VT_VARIANT;
    }
    FillOutElemDesc(vt, PARAMFLAG_FRETVAL, elemDesc);
}

ELEMDESC* XPCDispJSPropertyInfo::GetParamInfo()
{
    PRUint32 paramCount = GetParamCount();
    ELEMDESC* elemDesc;
    if(paramCount != 0)
    {
        elemDesc = new ELEMDESC[paramCount];
        if(elemDesc)
        {
            for(PRUint32 index = 0; index < paramCount; ++index)
            {
                FillOutElemDesc(VT_VARIANT, PARAMFLAG_FIN, elemDesc[index]);
            }
        }
    }
    else
        elemDesc = 0;
    
    return elemDesc;
}

XPCDispJSPropertyInfo::XPCDispJSPropertyInfo(JSContext* cx, PRUint32 memid, 
                                             JSObject* obj, jsval val) : 
    mPropertyType(INVALID), mMemID(memid)
{
    PRUint32 len;
    jschar * chars = xpc_JSString2String(cx, val, &len);
    if(!chars)
        return;

    mName = nsString(NS_REINTERPRET_CAST(const PRUnichar *, chars), len);
    JSBool found;
    uintN attr;
    
    if(!JS_GetUCPropertyAttributes(cx, obj, chars, len, &attr, &found) || 
        !found || (attr & JSPROP_ENUMERATE) == 0)
        return;

    
    if(!chars || !JS_GetUCProperty(cx, obj, chars, len, &mProperty) ||
        JSVAL_IS_NULL(mProperty))
        return;

    
    if(JSVAL_IS_OBJECT(mProperty) && 
        JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(mProperty)))
    {
        mPropertyType = FUNCTION;
        JSObject * funcObj = JSVAL_TO_OBJECT(mProperty);
        JSIdArray * funcObjArray = JS_Enumerate(cx, funcObj);
        if(funcObjArray)
        {
            mParamCount = funcObjArray->length;
        }
    }
    else 
    {
        mParamCount = 0;
        if((attr & JSPROP_READONLY) != 0)
        {
            mPropertyType = READONLY_PROPERTY;
        }
        else
        {
            mPropertyType = PROPERTY;
        }
    }
}

PRBool XPCDispTypeInfo::FuncDescArray::BuildFuncDesc(XPCCallContext& ccx, JSObject* obj, 
                                     XPCDispJSPropertyInfo & propInfo)
{
    
    FUNCDESC* funcDesc = new FUNCDESC;
    if(!funcDesc)
        return PR_FALSE;
    mArray.AppendElement(funcDesc);
    
    funcDesc->memid = propInfo.GetMemID();
    funcDesc->lprgscode = 0; 
    funcDesc->funckind = FUNC_DISPATCH;
    funcDesc->invkind = propInfo.GetInvokeKind();
    funcDesc->callconv = CC_STDCALL;
    funcDesc->cParams = propInfo.GetParamCount();
    funcDesc->lprgelemdescParam = propInfo.GetParamInfo();
    
    funcDesc->cParamsOpt = 0; 
    
    
    funcDesc->oVft = 0;         
    funcDesc->cScodes = 0;      
    funcDesc->wFuncFlags = 0;   
    propInfo.GetReturnType(ccx, funcDesc->elemdescFunc);
    return PR_TRUE;
}

XPCDispTypeInfo::FuncDescArray::FuncDescArray(XPCCallContext& ccx, JSObject* obj, const XPCDispIDArray& array, XPCDispNameArray & names)
{
    PRUint32 size = array.Length();
    names.SetSize(size);
    PRUint32 memid = 0;
    JSContext* cx = ccx;
    
    for(PRUint32 index = 0; index < size; ++index)
    {
        XPCDispJSPropertyInfo propInfo(cx, ++memid, obj, array.Item(cx, index));
        names.SetName(index + 1, propInfo.GetName());
        if(!BuildFuncDesc(ccx, obj, propInfo))
            return;
        
        if(propInfo.IsProperty() && !propInfo.IsReadOnly())
        {
            propInfo.SetSetter();
            if(!BuildFuncDesc(ccx, obj, propInfo))
                return;
        }
    }
}

XPCDispTypeInfo::FuncDescArray::~FuncDescArray()
{
    PRUint32 size = mArray.Count();
    for(PRUint32 index = 0; index < size; ++index)
    {
        FUNCDESC* funcDesc = NS_REINTERPRET_CAST(FUNCDESC*,mArray.ElementAt(index));
        delete [] funcDesc->lprgelemdescParam;
        delete funcDesc;
    }
}

XPCDispTypeInfo::XPCDispTypeInfo(XPCCallContext& ccx, JSObject* obj, 
                               XPCDispIDArray* array) :
    mRefCnt(0), mJSObject(obj), mIDArray(array), 
    mFuncDescArray(ccx, obj, *array, mNameArray)
{
}

XPCDispTypeInfo * XPCDispTypeInfo::New(XPCCallContext& ccx, JSObject* obj)
{
    XPCDispTypeInfo * pTypeInfo = 0;
    JSIdArray * jsArray = JS_Enumerate(ccx, obj);
    if(!jsArray)
        return nsnull;
    XPCDispIDArray* array = new XPCDispIDArray(ccx, jsArray);
    if(!array)
        return nsnull;
    pTypeInfo = new XPCDispTypeInfo(ccx, obj, array);
    if(!pTypeInfo)
    {
        delete array;
        return nsnull;
    }
    NS_ADDREF(pTypeInfo);
    return pTypeInfo;
}

NS_COM_MAP_BEGIN(XPCDispTypeInfo)
    NS_COM_MAP_ENTRY(ITypeInfo)
    NS_COM_MAP_ENTRY(IUnknown)
NS_COM_MAP_END

NS_IMPL_THREADSAFE_ADDREF(XPCDispTypeInfo)
NS_IMPL_THREADSAFE_RELEASE(XPCDispTypeInfo)


STDMETHODIMP XPCDispTypeInfo::GetTypeAttr( 
     TYPEATTR __RPC_FAR *__RPC_FAR *ppTypeAttr)
{
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::GetTypeComp(
     ITypeComp __RPC_FAR *__RPC_FAR *ppTComp)
{
    return E_NOTIMPL;
}

STDMETHODIMP XPCDispTypeInfo::GetFuncDesc(
         UINT index,
         FUNCDESC __RPC_FAR *__RPC_FAR *ppFuncDesc)
{
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::GetVarDesc(
         UINT index,
         VARDESC __RPC_FAR *__RPC_FAR *ppVarDesc)
{
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::GetNames(
         MEMBERID memid,
         BSTR __RPC_FAR *rgBstrNames,
         UINT cMaxNames,
         UINT __RPC_FAR *pcNames)
{
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::GetRefTypeOfImplType(
         UINT index,
         HREFTYPE __RPC_FAR *pRefType)
{
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::GetImplTypeFlags(
         UINT index,
         INT __RPC_FAR *pImplTypeFlags)
{
    return E_NOTIMPL;
}

STDMETHODIMP XPCDispTypeInfo::GetIDsOfNames(
         LPOLESTR __RPC_FAR *rgszNames,
         UINT cNames,
         MEMBERID __RPC_FAR *pMemId)
{
    
    for(UINT index = 0; index < cNames; ++index)
    {
        nsDependentString buffer(NS_STATIC_CAST(const PRUnichar *,
                                 rgszNames[index]),
                                 wcslen(rgszNames[index]));
        pMemId[index] = mNameArray.Find(buffer);

    }
    return S_OK;
}

STDMETHODIMP XPCDispTypeInfo::Invoke(
         PVOID pvInstance,
         MEMBERID memid,
         WORD wFlags,
         DISPPARAMS __RPC_FAR *pDispParams,
         VARIANT __RPC_FAR *pVarResult,
         EXCEPINFO __RPC_FAR *pExcepInfo,
         UINT __RPC_FAR *puArgErr)
{
    CComQIPtr<IDispatch> pDisp(NS_REINTERPRET_CAST(IUnknown*,pvInstance));
    if(pDisp)
    {
        return pDisp->Invoke(memid, IID_NULL, LOCALE_SYSTEM_DEFAULT, wFlags, 
                               pDispParams, pVarResult, pExcepInfo, puArgErr);
    }
    return E_NOINTERFACE;
}
    
STDMETHODIMP XPCDispTypeInfo::GetDocumentation(
         MEMBERID memid,
         BSTR __RPC_FAR *pBstrName,
         BSTR __RPC_FAR *pBstrDocString,
         DWORD __RPC_FAR *pdwHelpContext,
         BSTR __RPC_FAR *pBstrHelpFile)
{
    PRUint32 index = memid;
    if(index < mIDArray->Length())
        return E_FAIL;

    XPCCallContext ccx(NATIVE_CALLER);
    PRUnichar * chars = xpc_JSString2PRUnichar(ccx, mIDArray->Item(ccx, index));
    if(!chars)
    {
        return E_FAIL;
    }
    CComBSTR name(chars);
    *pBstrName = name.Detach();
    pBstrDocString = 0;
    pdwHelpContext = 0;
    pBstrHelpFile = 0;
    return S_OK;
}
    
STDMETHODIMP XPCDispTypeInfo::GetDllEntry(
         MEMBERID memid,
         INVOKEKIND invKind,
         BSTR __RPC_FAR *pBstrDllName,
         BSTR __RPC_FAR *pBstrName,
         WORD __RPC_FAR *pwOrdinal)
{
    
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::GetRefTypeInfo(
         HREFTYPE hRefType,
         ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::AddressOfMember(
         MEMBERID memid,
         INVOKEKIND invKind,
         PVOID __RPC_FAR *ppv)
{
    
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::CreateInstance(
         IUnknown __RPC_FAR *pUnkOuter,
         REFIID riid,
         PVOID __RPC_FAR *ppvObj)
{
    
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::GetMops(
         MEMBERID memid,
         BSTR __RPC_FAR *pBstrMops)
{
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::GetContainingTypeLib(
         ITypeLib __RPC_FAR *__RPC_FAR *ppTLib,
         UINT __RPC_FAR *pIndex)
{
    
    return E_NOTIMPL;
}
    
void STDMETHODCALLTYPE XPCDispTypeInfo::ReleaseTypeAttr( 
     TYPEATTR __RPC_FAR *pTypeAttr)
{
    
}

void STDMETHODCALLTYPE XPCDispTypeInfo::ReleaseFuncDesc( 
     FUNCDESC __RPC_FAR *pFuncDesc)
{
    
}

void STDMETHODCALLTYPE XPCDispTypeInfo::ReleaseVarDesc( 
     VARDESC __RPC_FAR *pVarDesc)
{
    
}

XPCDispIDArray::XPCDispIDArray(XPCCallContext& ccx, JSIdArray* array) : 
    mMarked(JS_FALSE), mIDArray(array->length)
{
    
    for(jsint index = 0; index < array->length; ++index)
    {
        mIDArray.ReplaceElementAt(NS_REINTERPRET_CAST(void*,
                                                      array->vector[index]), 
                                  index);
    }   
}

void XPCDispIDArray::TraceJS(JSTracer* trc)
{
    
    if(JS_IsGCMarkingTracer(trc))
    {
        if (IsMarked())
            return;
        mMarked = JS_TRUE;
    }

    PRInt32 count = Length();
    jsval val;

    
    for(PRInt32 index = 0; index < count; ++index)
    {
        if(JS_IdToValue(trc->context,
                        NS_REINTERPRET_CAST(jsid, mIDArray.ElementAt(index)),
                        &val))
        {
            JS_CALL_VALUE_TRACER(trc, val, "disp_id_array_element");
        }
    }
}

