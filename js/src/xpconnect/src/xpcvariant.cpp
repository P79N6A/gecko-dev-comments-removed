









































#include "xpcprivate.h"

NS_IMPL_CYCLE_COLLECTION_CLASS(XPCVariant)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(XPCVariant)
  NS_INTERFACE_MAP_ENTRY(XPCVariant)
  NS_INTERFACE_MAP_ENTRY(nsIVariant)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_IMPL_QUERY_CLASSINFO(XPCVariant)
NS_INTERFACE_MAP_END
NS_IMPL_CI_INTERFACE_GETTER2(XPCVariant, XPCVariant, nsIVariant)

NS_IMPL_CYCLE_COLLECTING_ADDREF(XPCVariant)
NS_IMPL_CYCLE_COLLECTING_RELEASE(XPCVariant)

XPCVariant::XPCVariant(XPCCallContext& ccx, jsval aJSVal)
    : mJSVal(aJSVal)
{
    nsVariant::Initialize(&mData);
    if(!JSVAL_IS_PRIMITIVE(mJSVal))
    {
        
        
        

        JSObject* proto;
        XPCWrappedNative* wn =
            XPCWrappedNative::GetWrappedNativeOfJSObject(ccx,
                                                         JSVAL_TO_OBJECT(mJSVal),
                                                         nsnull,
                                                         &proto);
        mReturnRawObject = !wn && !proto;
    }
    else
        mReturnRawObject = JS_FALSE;
}

XPCTraceableVariant::~XPCTraceableVariant()
{
    NS_ASSERTION(JSVAL_IS_GCTHING(mJSVal), "Must be traceable or unlinked");

    
    
    if(!JSVAL_IS_STRING(mJSVal))
        nsVariant::Cleanup(&mData);

    if(!JSVAL_IS_NULL(mJSVal))
        RemoveFromRootSet(nsXPConnect::GetRuntimeInstance()->GetJSRuntime());
}

void XPCTraceableVariant::TraceJS(JSTracer* trc)
{
    NS_ASSERTION(JSVAL_IS_TRACEABLE(mJSVal), "Must be traceable");
    JS_SET_TRACING_DETAILS(trc, PrintTraceName, this, 0);
    JS_CallTracer(trc, JSVAL_TO_TRACEABLE(mJSVal), JSVAL_TRACE_KIND(mJSVal));
}

#ifdef DEBUG

void
XPCTraceableVariant::PrintTraceName(JSTracer* trc, char *buf, size_t bufsize)
{
    JS_snprintf(buf, bufsize, "XPCVariant[0x%p].mJSVal", trc->debugPrintArg);
}
#endif

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(XPCVariant)
    if(JSVAL_IS_OBJECT(tmp->mJSVal))
        cb.NoteScriptChild(nsIProgrammingLanguage::JAVASCRIPT,
                           JSVAL_TO_OBJECT(tmp->mJSVal));

    nsVariant::Traverse(tmp->mData, cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(XPCVariant)
    
    
    if(JSVAL_IS_STRING(tmp->mJSVal))
        tmp->mData.u.wstr.mWStringValue = nsnull;
    nsVariant::Cleanup(&tmp->mData);

    if(JSVAL_IS_TRACEABLE(tmp->mJSVal))
    {
        XPCTraceableVariant *v = static_cast<XPCTraceableVariant*>(tmp);
        v->RemoveFromRootSet(nsXPConnect::GetRuntimeInstance()->GetJSRuntime());
    }
    tmp->mJSVal = JSVAL_NULL;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


XPCVariant* XPCVariant::newVariant(XPCCallContext& ccx, jsval aJSVal)
{
    XPCVariant* variant;

    if(!JSVAL_IS_TRACEABLE(aJSVal))
        variant = new XPCVariant(ccx, aJSVal);
    else
        variant = new XPCTraceableVariant(ccx, aJSVal);

    if(!variant)
        return nsnull;
    NS_ADDREF(variant);

    if(!variant->InitializeData(ccx))
        NS_RELEASE(variant);     

    return variant;
}


class XPCArrayHomogenizer
{
private:
    enum Type
    {
        tNull  = 0 ,  
        tInt       ,  
        tDbl       ,  
        tBool      ,  
        tStr       ,  
        tID        ,  
        tArr       ,  
        tISup      ,  
        tUnk       ,  

        tTypeCount ,  

        tVar       ,  
        tErr          
    };

    
    static const Type StateTable[tTypeCount][tTypeCount-1];

public:
    static JSBool GetTypeForArray(XPCCallContext& ccx, JSObject* array, 
                                  jsuint length, 
                                  nsXPTType* resultType, nsID* resultID);
};






const XPCArrayHomogenizer::Type 
XPCArrayHomogenizer::StateTable[tTypeCount][tTypeCount-1] = {

{tNull,tVar ,tVar ,tVar ,tStr ,tID  ,tVar ,tISup },
{tVar ,tInt ,tDbl ,tVar ,tVar ,tVar ,tVar ,tVar  },
{tVar ,tDbl ,tDbl ,tVar ,tVar ,tVar ,tVar ,tVar  },
{tVar ,tVar ,tVar ,tBool,tVar ,tVar ,tVar ,tVar  },
{tStr ,tVar ,tVar ,tVar ,tStr ,tVar ,tVar ,tVar  },
{tID  ,tVar ,tVar ,tVar ,tVar ,tID  ,tVar ,tVar  },
{tErr ,tErr ,tErr ,tErr ,tErr ,tErr ,tErr ,tErr  },
{tISup,tVar ,tVar ,tVar ,tVar ,tVar ,tVar ,tISup },
{tNull,tInt ,tDbl ,tBool,tStr ,tID  ,tVar ,tISup }};


JSBool
XPCArrayHomogenizer::GetTypeForArray(XPCCallContext& ccx, JSObject* array,
                                     jsuint length, 
                                     nsXPTType* resultType, nsID* resultID)    
{
    Type state = tUnk;
    Type type;
       
    for(jsuint i = 0; i < length; i++)
    {
        jsval val;
        if(!JS_GetElement(ccx, array, i, &val))
            return JS_FALSE;
           
        if(JSVAL_IS_INT(val))
            type = tInt;
        else if(JSVAL_IS_DOUBLE(val))
            type = tDbl;
        else if(JSVAL_IS_BOOLEAN(val))
            type = tBool;
        else if(JSVAL_IS_VOID(val))
        {
            state = tVar;
            break;
        }
        else if(JSVAL_IS_NULL(val))
            type = tNull;
        else if(JSVAL_IS_STRING(val))
            type = tStr;
        else
        {
            NS_ASSERTION(JSVAL_IS_OBJECT(val), "invalid type of jsval!");
            JSObject* jsobj = JSVAL_TO_OBJECT(val);
            if(JS_IsArrayObject(ccx, jsobj))
                type = tArr;
            else if(xpc_JSObjectIsID(ccx, jsobj))
                type = tID;
            else
                type = tISup;
        }

        NS_ASSERTION(state != tErr, "bad state table!");
        NS_ASSERTION(type  != tErr, "bad type!");
        NS_ASSERTION(type  != tVar, "bad type!");
        NS_ASSERTION(type  != tUnk, "bad type!");

        state = StateTable[state][type];
        
        NS_ASSERTION(state != tErr, "bad state table!");
        NS_ASSERTION(state != tUnk, "bad state table!");
        
        if(state == tVar)
            break;
    }

    switch(state)
    {
        case tInt : 
            *resultType = nsXPTType((uint8)TD_INT32);
            break;
        case tDbl : 
            *resultType = nsXPTType((uint8)TD_DOUBLE);
            break;
        case tBool:
            *resultType = nsXPTType((uint8)TD_BOOL);
            break;
        case tStr : 
            *resultType = nsXPTType((uint8)(TD_PWSTRING | XPT_TDP_POINTER));
            break;
        case tID  : 
            *resultType = nsXPTType((uint8)(TD_PNSIID | XPT_TDP_POINTER));
            break;
        case tISup: 
            *resultType = nsXPTType((uint8)(TD_INTERFACE_IS_TYPE | XPT_TDP_POINTER));
            *resultID = NS_GET_IID(nsISupports);
            break;
        case tNull: 
            
        case tVar :
            *resultType = nsXPTType((uint8)(TD_INTERFACE_IS_TYPE | XPT_TDP_POINTER));
            *resultID = NS_GET_IID(nsIVariant);
            break;
        case tArr : 
            
        case tUnk : 
            
        case tErr : 
            
        default:
            NS_ERROR("bad state");
            return JS_FALSE;
    }
    return JS_TRUE;
}

JSBool XPCVariant::InitializeData(XPCCallContext& ccx)
{
    if(JSVAL_IS_INT(mJSVal))
        return NS_SUCCEEDED(nsVariant::SetFromInt32(&mData, 
                                                    JSVAL_TO_INT(mJSVal)));
    if(JSVAL_IS_DOUBLE(mJSVal))
        return NS_SUCCEEDED(nsVariant::SetFromDouble(&mData, 
                                                     *JSVAL_TO_DOUBLE(mJSVal)));
    if(JSVAL_IS_BOOLEAN(mJSVal))
        return NS_SUCCEEDED(nsVariant::SetFromBool(&mData, 
                                                   JSVAL_TO_BOOLEAN(mJSVal)));
    if(JSVAL_IS_VOID(mJSVal))
        return NS_SUCCEEDED(nsVariant::SetToVoid(&mData));
    if(JSVAL_IS_NULL(mJSVal))
        return NS_SUCCEEDED(nsVariant::SetToEmpty(&mData));
    if(JSVAL_IS_STRING(mJSVal))
    {
        
        
        JSString* str = JSVAL_TO_STRING(mJSVal);
        if(!JS_MakeStringImmutable(ccx, str))
            return JS_FALSE;

        
        
        
        NS_ASSERTION(mData.mType == nsIDataType::VTYPE_EMPTY,
                     "Why do we already have data?");

        mData.u.wstr.mWStringValue = 
            reinterpret_cast<PRUnichar*>(JS_GetStringChars(str));
        
        
        mData.u.wstr.mWStringLength = (PRUint32)JS_GetStringLength(str);
        mData.mType = nsIDataType::VTYPE_WSTRING_SIZE_IS;
        
        return JS_TRUE;
    }

    
    NS_ASSERTION(JSVAL_IS_OBJECT(mJSVal), "invalid type of jsval!");
    
    JSObject* jsobj = JSVAL_TO_OBJECT(mJSVal);

    

    const nsID* id = xpc_JSObjectToID(ccx, jsobj);
    if(id)
        return NS_SUCCEEDED(nsVariant::SetFromID(&mData, *id));
    
    

    jsuint len;

    if(JS_IsArrayObject(ccx, jsobj) && JS_GetArrayLength(ccx, jsobj, &len))
    {
        if(!len) 
        {
            
            nsVariant::SetToEmptyArray(&mData);
            return JS_TRUE;
        }
        
        nsXPTType type;
        nsID id;

        if(!XPCArrayHomogenizer::GetTypeForArray(ccx, jsobj, len, &type, &id))
            return JS_FALSE; 

        if(!XPCConvert::JSArray2Native(ccx, &mData.u.array.mArrayValue, 
                                       mJSVal, len, len,
                                       type, type.IsPointer(),
                                       &id, nsnull))
            return JS_FALSE;

        mData.mType = nsIDataType::VTYPE_ARRAY;
        if(type.IsInterfacePointer())
            mData.u.array.mArrayInterfaceID = id;
        mData.u.array.mArrayCount = len;
        mData.u.array.mArrayType = type.TagPart();
        
        return JS_TRUE;
    }    

    

    nsXPConnect*  xpc;
    nsCOMPtr<nsISupports> wrapper;
    const nsIID& iid = NS_GET_IID(nsISupports);

    return nsnull != (xpc = nsXPConnect::GetXPConnect()) &&
           NS_SUCCEEDED(xpc->WrapJS(ccx, jsobj,
                        iid, getter_AddRefs(wrapper))) &&
           NS_SUCCEEDED(nsVariant::SetFromInterface(&mData, iid, wrapper));
}


JSBool 
XPCVariant::VariantDataToJS(XPCLazyCallContext& lccx, 
                            nsIVariant* variant,
                            JSObject* scope, nsresult* pErr,
                            jsval* pJSVal)
{
    
    PRUint16 type;
    if(NS_FAILED(variant->GetDataType(&type)))
        return JS_FALSE;

    nsCOMPtr<XPCVariant> xpcvariant = do_QueryInterface(variant);
    if(xpcvariant)
    {
        jsval realVal = xpcvariant->GetJSVal();
        if(JSVAL_IS_PRIMITIVE(realVal) || 
           type == nsIDataType::VTYPE_ARRAY ||
           type == nsIDataType::VTYPE_EMPTY_ARRAY ||
           type == nsIDataType::VTYPE_ID)
        {
            
            
            
            *pJSVal = realVal;
            return JS_TRUE;
        }

        if(xpcvariant->mReturnRawObject)
        {
            NS_ASSERTION(type == nsIDataType::VTYPE_INTERFACE ||
                         type == nsIDataType::VTYPE_INTERFACE_IS,
                         "Weird variant");
            *pJSVal = realVal;
            return JS_TRUE;
        }

        
        
        
        
    }

    
    
    
    

    nsXPTCVariant xpctvar;
    nsID iid;
    nsAutoString astring;
    nsCAutoString cString;
    nsUTF8String utf8String;
    PRUint32 size;
    xpctvar.flags = 0;
    JSBool success;

    JSContext* cx = lccx.GetJSContext();
    switch(type)
    {
        case nsIDataType::VTYPE_INT8:        
        case nsIDataType::VTYPE_INT16:        
        case nsIDataType::VTYPE_INT32:        
        case nsIDataType::VTYPE_INT64:        
        case nsIDataType::VTYPE_UINT8:        
        case nsIDataType::VTYPE_UINT16:        
        case nsIDataType::VTYPE_UINT32:        
        case nsIDataType::VTYPE_UINT64:        
        case nsIDataType::VTYPE_FLOAT:        
        case nsIDataType::VTYPE_DOUBLE:        
        {
            
            if(NS_FAILED(variant->GetAsDouble(&xpctvar.val.d)))
                return JS_FALSE;
            return JS_NewNumberValue(cx, xpctvar.val.d, pJSVal);
        }
        case nsIDataType::VTYPE_BOOL:        
        {
            
            if(NS_FAILED(variant->GetAsBool(&xpctvar.val.b)))
                return JS_FALSE;
            *pJSVal = BOOLEAN_TO_JSVAL(xpctvar.val.b);
            return JS_TRUE;
        }
        case nsIDataType::VTYPE_CHAR: 
            if(NS_FAILED(variant->GetAsChar(&xpctvar.val.c)))
                return JS_FALSE;
            xpctvar.type = (uint8)TD_CHAR;
            break;
        case nsIDataType::VTYPE_WCHAR:        
            if(NS_FAILED(variant->GetAsWChar(&xpctvar.val.wc)))
                return JS_FALSE;
            xpctvar.type = (uint8)TD_WCHAR;
            break;
        case nsIDataType::VTYPE_ID:        
            if(NS_FAILED(variant->GetAsID(&iid)))
                return JS_FALSE;
            xpctvar.type = (uint8)(TD_PNSIID | XPT_TDP_POINTER);
            xpctvar.val.p = &iid;
            break;
        case nsIDataType::VTYPE_ASTRING:        
            if(NS_FAILED(variant->GetAsAString(astring)))
                return JS_FALSE;
            xpctvar.type = (uint8)(TD_ASTRING | XPT_TDP_POINTER);
            xpctvar.val.p = &astring;
            break;
        case nsIDataType::VTYPE_DOMSTRING:
            if(NS_FAILED(variant->GetAsAString(astring)))
                return JS_FALSE;
            xpctvar.type = (uint8)(TD_DOMSTRING | XPT_TDP_POINTER);
            xpctvar.val.p = &astring;
            break;
        case nsIDataType::VTYPE_CSTRING:            
            if(NS_FAILED(variant->GetAsACString(cString)))
                return JS_FALSE;
            xpctvar.type = (uint8)(TD_CSTRING | XPT_TDP_POINTER);
            xpctvar.val.p = &cString;
            break;
        case nsIDataType::VTYPE_UTF8STRING:            
            if(NS_FAILED(variant->GetAsAUTF8String(utf8String)))
                return JS_FALSE;
            xpctvar.type = (uint8)(TD_UTF8STRING | XPT_TDP_POINTER);
            xpctvar.val.p = &utf8String;
            break;       
        case nsIDataType::VTYPE_CHAR_STR:       
            if(NS_FAILED(variant->GetAsString((char**)&xpctvar.val.p)))
                return JS_FALSE;
            xpctvar.type = (uint8)(TD_PSTRING | XPT_TDP_POINTER);
            xpctvar.SetValIsAllocated();
            break;
        case nsIDataType::VTYPE_STRING_SIZE_IS:
            if(NS_FAILED(variant->GetAsStringWithSize(&size, 
                                                      (char**)&xpctvar.val.p)))
                return JS_FALSE;
            xpctvar.type = (uint8)(TD_PSTRING_SIZE_IS | XPT_TDP_POINTER);
            xpctvar.SetValIsAllocated();
            break;
        case nsIDataType::VTYPE_WCHAR_STR:        
            if(NS_FAILED(variant->GetAsWString((PRUnichar**)&xpctvar.val.p)))
                return JS_FALSE;
            xpctvar.type = (uint8)(TD_PWSTRING | XPT_TDP_POINTER);
            xpctvar.SetValIsAllocated();
            break;
        case nsIDataType::VTYPE_WSTRING_SIZE_IS:        
            if(NS_FAILED(variant->GetAsWStringWithSize(&size, 
                                                      (PRUnichar**)&xpctvar.val.p)))
                return JS_FALSE;
            xpctvar.type = (uint8)(TD_PWSTRING_SIZE_IS | XPT_TDP_POINTER);
            xpctvar.SetValIsAllocated();
            break;
        case nsIDataType::VTYPE_INTERFACE:        
        case nsIDataType::VTYPE_INTERFACE_IS:        
        {
            nsID* piid;
            if(NS_FAILED(variant->GetAsInterface(&piid, &xpctvar.val.p)))
                return JS_FALSE;

            iid = *piid;
            nsMemory::Free((char*)piid);

            xpctvar.type = (uint8)(TD_INTERFACE_IS_TYPE | XPT_TDP_POINTER);
            if(xpctvar.val.p)
                xpctvar.SetValIsInterface();
            break;
        }
        case nsIDataType::VTYPE_ARRAY:
        {
            nsDiscriminatedUnion du;
            nsVariant::Initialize(&du);
            nsresult rv;

            rv = variant->GetAsArray(&du.u.array.mArrayType,
                                     &du.u.array.mArrayInterfaceID,
                                     &du.u.array.mArrayCount,
                                     &du.u.array.mArrayValue);
            if(NS_FAILED(rv))
                return JS_FALSE;
        
            
            du.mType = nsIDataType::VTYPE_ARRAY;
            success = JS_FALSE;

            nsXPTType conversionType;
            PRUint16 elementType = du.u.array.mArrayType;
            const nsID* pid = nsnull;

            switch(elementType)
            {
                case nsIDataType::VTYPE_INT8:        
                case nsIDataType::VTYPE_INT16:        
                case nsIDataType::VTYPE_INT32:        
                case nsIDataType::VTYPE_INT64:        
                case nsIDataType::VTYPE_UINT8:        
                case nsIDataType::VTYPE_UINT16:        
                case nsIDataType::VTYPE_UINT32:        
                case nsIDataType::VTYPE_UINT64:        
                case nsIDataType::VTYPE_FLOAT:        
                case nsIDataType::VTYPE_DOUBLE:        
                case nsIDataType::VTYPE_BOOL:        
                case nsIDataType::VTYPE_CHAR:        
                case nsIDataType::VTYPE_WCHAR:        
                    conversionType = nsXPTType((uint8)elementType);
                    break;

                case nsIDataType::VTYPE_ID:        
                case nsIDataType::VTYPE_CHAR_STR:        
                case nsIDataType::VTYPE_WCHAR_STR:        
                    conversionType = nsXPTType((uint8)elementType | XPT_TDP_POINTER);
                    break;

                case nsIDataType::VTYPE_INTERFACE:        
                    pid = &NS_GET_IID(nsISupports);
                    conversionType = nsXPTType((uint8)elementType | XPT_TDP_POINTER);
                    break;

                case nsIDataType::VTYPE_INTERFACE_IS:        
                    pid = &du.u.array.mArrayInterfaceID;
                    conversionType = nsXPTType((uint8)elementType | XPT_TDP_POINTER);
                    break;

                
                case nsIDataType::VTYPE_VOID:        
                case nsIDataType::VTYPE_ASTRING:        
                case nsIDataType::VTYPE_DOMSTRING:        
                case nsIDataType::VTYPE_CSTRING:        
                case nsIDataType::VTYPE_UTF8STRING:        
                case nsIDataType::VTYPE_WSTRING_SIZE_IS:        
                case nsIDataType::VTYPE_STRING_SIZE_IS:        
                case nsIDataType::VTYPE_ARRAY:
                case nsIDataType::VTYPE_EMPTY_ARRAY:
                case nsIDataType::VTYPE_EMPTY:
                default:
                    NS_ERROR("bad type in array!");
                    goto VARIANT_DONE;
            }

            success = 
                XPCConvert::NativeArray2JS(lccx, pJSVal, 
                                           (const void**)&du.u.array.mArrayValue,
                                           conversionType, pid,
                                           du.u.array.mArrayCount, 
                                           scope, pErr);

VARIANT_DONE:                                
            nsVariant::Cleanup(&du);
            return success;
        }        
        case nsIDataType::VTYPE_EMPTY_ARRAY: 
        {
            JSObject* array = JS_NewArrayObject(cx, 0, nsnull);
            if(!array) 
                return JS_FALSE;
            *pJSVal = OBJECT_TO_JSVAL(array);
            return JS_TRUE;
        }
        case nsIDataType::VTYPE_VOID:        
            *pJSVal = JSVAL_VOID;
            return JS_TRUE;
        case nsIDataType::VTYPE_EMPTY:
            *pJSVal = JSVAL_NULL;
            return JS_TRUE;
        default:
            NS_ERROR("bad type in variant!");
            return JS_FALSE;
    }

    
    
    if(xpctvar.type.TagPart() == TD_PSTRING_SIZE_IS ||
       xpctvar.type.TagPart() == TD_PWSTRING_SIZE_IS)
    {
        success = XPCConvert::NativeStringWithSize2JS(cx, pJSVal,
                                                      (const void*)&xpctvar.val,
                                                      xpctvar.type,
                                                      size, pErr);
    }
    else
    {
        success = XPCConvert::NativeData2JS(lccx, pJSVal,
                                            (const void*)&xpctvar.val,
                                            xpctvar.type,
                                            &iid, scope, pErr);
    }

    if(xpctvar.IsValAllocated())
        nsMemory::Free((char*)xpctvar.val.p);
    else if(xpctvar.IsValInterface())
        ((nsISupports*)xpctvar.val.p)->Release();

    return success;
}








NS_IMETHODIMP XPCVariant::GetDataType(PRUint16 *aDataType)
{
    *aDataType = mData.mType;
    return NS_OK;
}


NS_IMETHODIMP XPCVariant::GetAsInt8(PRUint8 *_retval)
{
    return nsVariant::ConvertToInt8(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsInt16(PRInt16 *_retval)
{
    return nsVariant::ConvertToInt16(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsInt32(PRInt32 *_retval)
{
    return nsVariant::ConvertToInt32(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsInt64(PRInt64 *_retval)
{
    return nsVariant::ConvertToInt64(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsUint8(PRUint8 *_retval)
{
    return nsVariant::ConvertToUint8(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsUint16(PRUint16 *_retval)
{
    return nsVariant::ConvertToUint16(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsUint32(PRUint32 *_retval)
{
    return nsVariant::ConvertToUint32(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsUint64(PRUint64 *_retval)
{
    return nsVariant::ConvertToUint64(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsFloat(float *_retval)
{
    return nsVariant::ConvertToFloat(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsDouble(double *_retval)
{
    return nsVariant::ConvertToDouble(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsBool(PRBool *_retval)
{
    return nsVariant::ConvertToBool(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsChar(char *_retval)
{
    return nsVariant::ConvertToChar(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsWChar(PRUnichar *_retval)
{
    return nsVariant::ConvertToWChar(mData, _retval);
}


NS_IMETHODIMP_(nsresult) XPCVariant::GetAsID(nsID *retval)
{
    return nsVariant::ConvertToID(mData, retval);
}


NS_IMETHODIMP XPCVariant::GetAsAString(nsAString & _retval)
{
    return nsVariant::ConvertToAString(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsDOMString(nsAString & _retval)
{
    
    
    return nsVariant::ConvertToAString(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsACString(nsACString & _retval)
{
    return nsVariant::ConvertToACString(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsAUTF8String(nsAUTF8String & _retval)
{
    return nsVariant::ConvertToAUTF8String(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsString(char **_retval)
{
    return nsVariant::ConvertToString(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsWString(PRUnichar **_retval)
{
    return nsVariant::ConvertToWString(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsISupports(nsISupports **_retval)
{
    return nsVariant::ConvertToISupports(mData, _retval);
}


NS_IMETHODIMP XPCVariant::GetAsInterface(nsIID * *iid, void * *iface)
{
    return nsVariant::ConvertToInterface(mData, iid, iface);
}



NS_IMETHODIMP_(nsresult) XPCVariant::GetAsArray(PRUint16 *type, nsIID *iid, PRUint32 *count, void * *ptr)
{
    return nsVariant::ConvertToArray(mData, type, iid, count, ptr);
}


NS_IMETHODIMP XPCVariant::GetAsStringWithSize(PRUint32 *size, char **str)
{
    return nsVariant::ConvertToStringWithSize(mData, size, str);
}


NS_IMETHODIMP XPCVariant::GetAsWStringWithSize(PRUint32 *size, PRUnichar **str)
{
    return nsVariant::ConvertToWStringWithSize(mData, size, str);
}


