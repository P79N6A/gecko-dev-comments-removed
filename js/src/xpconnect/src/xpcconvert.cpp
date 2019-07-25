












































#include "xpcprivate.h"
#include "nsString.h"
#include "nsIAtom.h"
#include "XPCWrapper.h"
#include "nsJSPrincipals.h"
#include "nsWrapperCache.h"
#include "WrapperFactory.h"
#include "AccessCheck.h"
#include "nsJSUtils.h"


#ifdef STRICT_CHECK_OF_UNICODE
#define ILLEGAL_RANGE(c) (0!=((c) & 0xFF80))
#else 
#define ILLEGAL_RANGE(c) (0!=((c) & 0xFF00))
#endif 

#define ILLEGAL_CHAR_RANGE(c) (0!=((c) & 0x80))














#define XPC_MK_BIT(p,o) (1 << (((p)?1:0)+((o)?2:0)))
#define XPC_IS_REFLECTABLE(f, p, o) ((f) & XPC_MK_BIT((p),(o)))
#define XPC_MK_FLAG(np_no,p_no,np_o,p_o) \
        ((uint8)((np_no) | ((p_no) << 1) | ((np_o) << 2) | ((p_o) << 3)))





#define XPC_FLAG_COUNT (1 << 5)


static uint8 xpc_reflectable_flags[XPC_FLAG_COUNT] = {
    
    
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), 
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), 
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), 
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), 
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), 
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), 
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), 
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), 
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), 
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), 
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), 
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), 
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), 
    XPC_MK_FLAG(  0  ,  0  ,   0 ,  0 ), 
    XPC_MK_FLAG(  0  ,  1  ,   0 ,  1 ), 
    XPC_MK_FLAG(  0  ,  1  ,   0 ,  0 ), 
    XPC_MK_FLAG(  0  ,  1  ,   0 ,  1 ), 
    XPC_MK_FLAG(  0  ,  1  ,   0 ,  1 ), 
    XPC_MK_FLAG(  0  ,  1  ,   0 ,  1 ), 
    XPC_MK_FLAG(  0  ,  1  ,   0 ,  1 ), 
    XPC_MK_FLAG(  0  ,  1  ,   0 ,  1 ), 
    XPC_MK_FLAG(  0  ,  1  ,   0 ,  1 ), 
    XPC_MK_FLAG(  0  ,  1  ,   0 ,  1 ), 
    XPC_MK_FLAG(  0  ,  1  ,   0 ,  0 ), 
    XPC_MK_FLAG(  0  ,  1  ,   0 ,  0 ), 
    XPC_MK_FLAG(  0  ,  1  ,   0 ,  0 ), 
    XPC_MK_FLAG(  1  ,  0  ,   1 ,  0 ), 
    XPC_MK_FLAG(  0  ,  0  ,   0 ,  0 ), 
    XPC_MK_FLAG(  0  ,  0  ,   0 ,  0 ), 
    XPC_MK_FLAG(  0  ,  0  ,   0 ,  0 ), 
    XPC_MK_FLAG(  0  ,  0  ,   0 ,  0 ), 
    XPC_MK_FLAG(  0  ,  0  ,   0 ,  0 )  
    };

static intN sXPCOMUCStringFinalizerIndex = -1;




JSBool
XPCConvert::IsMethodReflectable(const XPTMethodDescriptor& info)
{
    if(XPT_MD_IS_NOTXPCOM(info.flags) || XPT_MD_IS_HIDDEN(info.flags))
        return JS_FALSE;

    for(int i = info.num_args-1; i >= 0; i--)
    {
        const nsXPTParamInfo& param = info.params[i];
        const nsXPTType& type = param.GetType();

        uint8 base_type = type.TagPart();
        NS_ASSERTION(base_type < XPC_FLAG_COUNT, "BAD TYPE");

        if(!XPC_IS_REFLECTABLE(xpc_reflectable_flags[base_type],
                               type.IsPointer(), param.IsOut()))
            return JS_FALSE;
    }
    return JS_TRUE;
}




JSBool
XPCConvert::GetISupportsFromJSObject(JSObject* obj, nsISupports** iface)
{
    JSClass* jsclass = obj->getJSClass();
    NS_ASSERTION(jsclass, "obj has no class");
    if(jsclass &&
       (jsclass->flags & JSCLASS_HAS_PRIVATE) &&
       (jsclass->flags & JSCLASS_PRIVATE_IS_NSISUPPORTS))
    {
        *iface = (nsISupports*) xpc_GetJSPrivate(obj);
        return JS_TRUE;
    }
    return JS_FALSE;
}



static void
FinalizeXPCOMUCString(JSContext *cx, JSString *str)
{
    NS_ASSERTION(sXPCOMUCStringFinalizerIndex != -1,
                 "XPCConvert: XPCOM Unicode string finalizer called uninitialized!");

    jschar* buffer = const_cast<jschar *>(JS_GetStringCharsZ(cx, str));
    NS_ASSERTION(buffer, "How could this OOM if we allocated the memory?");
    nsMemory::Free(buffer);
}


static JSBool
AddXPCOMUCStringFinalizer()
{

    sXPCOMUCStringFinalizerIndex =
        JS_AddExternalStringFinalizer(FinalizeXPCOMUCString);

    if(sXPCOMUCStringFinalizerIndex == -1)
    {        
        return JS_FALSE;
    }

    return JS_TRUE;
}


void
XPCConvert::RemoveXPCOMUCStringFinalizer()
{
    JS_RemoveExternalStringFinalizer(FinalizeXPCOMUCString);
    sXPCOMUCStringFinalizerIndex = -1;
}


#define FIT_U32(i)     ((i) <= JSVAL_INT_MAX      \
                        ? INT_TO_JSVAL(i)         \
                        : DOUBLE_TO_JSVAL(i))






#ifdef HAVE_LONG_LONG

#define INT64_TO_DOUBLE(i)      ((jsdouble) (i))

#define UINT64_TO_DOUBLE(u)     ((jsdouble) (int64) (u))

#else

inline jsdouble
INT64_TO_DOUBLE(const int64 &v)
{
    jsdouble d;
    LL_L2D(d, v);
    return d;
}


#define UINT64_TO_DOUBLE INT64_TO_DOUBLE

#endif


JSBool
XPCConvert::NativeData2JS(XPCLazyCallContext& lccx, jsval* d, const void* s,
                          const nsXPTType& type, const nsID* iid, nsresult* pErr)
{
    NS_PRECONDITION(s, "bad param");
    NS_PRECONDITION(d, "bad param");

   JSContext* cx = lccx.GetJSContext();

    
    
    NS_ABORT_IF_FALSE(type.IsArithmetic() ||
                      cx->compartment == lccx.GetScopeForNewJSObjects()->compartment(),
                      "bad scope for new JSObjects");

    if(pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    switch(type.TagPart())
    {
    case nsXPTType::T_I8    : *d = INT_TO_JSVAL((int32)*((int8*)s));                 break;
    case nsXPTType::T_I16   : *d = INT_TO_JSVAL((int32)*((int16*)s));                break;
    case nsXPTType::T_I32   : *d = INT_TO_JSVAL(*((int32*)s));                       break;
    case nsXPTType::T_I64   : *d = DOUBLE_TO_JSVAL(INT64_TO_DOUBLE(*((int64*)s)));   break;
    case nsXPTType::T_U8    : *d = INT_TO_JSVAL((int32)*((uint8*)s));                break;
    case nsXPTType::T_U16   : *d = INT_TO_JSVAL((int32)*((uint16*)s));               break;
    case nsXPTType::T_U32   : *d = FIT_U32(*((uint32*)s));                           break;
    case nsXPTType::T_U64   : *d = DOUBLE_TO_JSVAL(UINT64_TO_DOUBLE(*((uint64*)s))); break;
    case nsXPTType::T_FLOAT : *d = DOUBLE_TO_JSVAL(*((float*)s));                    break;
    case nsXPTType::T_DOUBLE: *d = DOUBLE_TO_JSVAL(*((double*)s));                   break;
    case nsXPTType::T_BOOL  : *d = BOOLEAN_TO_JSVAL(*((PRBool*)s));                  break;
    case nsXPTType::T_CHAR  :
        {
            char* p = (char*)s;
            if(!p)
                return JS_FALSE;

#ifdef STRICT_CHECK_OF_UNICODE
            NS_ASSERTION(! ILLEGAL_CHAR_RANGE(p) , "passing non ASCII data");
#endif 

            JSString* str;
            if(!(str = JS_NewStringCopyN(cx, p, 1)))
                return JS_FALSE;
            *d = STRING_TO_JSVAL(str);
            break;
        }
    case nsXPTType::T_WCHAR :
        {
            jschar* p = (jschar*)s;
            if(!p)
                return JS_FALSE;
            JSString* str;
            if(!(str = JS_NewUCStringCopyN(cx, p, 1)))
                return JS_FALSE;
            *d = STRING_TO_JSVAL(str);
            break;
        }

    case nsXPTType::T_JSVAL :
        {
            *d = *((jsval*)s);
            if(!JS_WrapValue(cx, d))
                return JS_FALSE;
            break;
        }

    default:
        if(!type.IsPointer())
        {
            XPC_LOG_ERROR(("XPCConvert::NativeData2JS : unsupported type"));
            return JS_FALSE;
        }

        
        *d = JSVAL_NULL;

        switch(type.TagPart())
        {
        case nsXPTType::T_VOID:
            XPC_LOG_ERROR(("XPCConvert::NativeData2JS : void* params not supported"));
            return JS_FALSE;

        case nsXPTType::T_IID:
            {
                nsID* iid2 = *((nsID**)s);
                if(!iid2)
                    break;
                JSObject* obj;
                if(!(obj = xpc_NewIDObject(cx, lccx.GetScopeForNewJSObjects(), *iid2)))
                    return JS_FALSE;
                *d = OBJECT_TO_JSVAL(obj);
                break;
            }

        case nsXPTType::T_ASTRING:
            

        case nsXPTType::T_DOMSTRING:
            {
                const nsAString* p = *((const nsAString**)s);
                if(!p)
                    break;

                if(!p->IsVoid()) {
                    nsStringBuffer* buf;
                    jsval str = XPCStringConvert::ReadableToJSVal(cx, *p, &buf);
                    if(JSVAL_IS_NULL(str))
                        return JS_FALSE;
                    if(buf)
                        buf->AddRef();

                    *d = str;
                }

                
                

                break;
            }

        case nsXPTType::T_CHAR_STR:
            {
                char* p = *((char**)s);
                if(!p)
                    break;

#ifdef STRICT_CHECK_OF_UNICODE
                PRBool isAscii = PR_TRUE;
                char* t;
                for(t=p; *t && isAscii ; t++) {
                  if(ILLEGAL_CHAR_RANGE(*t))
                      isAscii = PR_FALSE;
                }
                NS_ASSERTION(isAscii, "passing non ASCII data");
#endif 
                JSString* str;
                if(!(str = JS_NewStringCopyZ(cx, p)))
                    return JS_FALSE;
                *d = STRING_TO_JSVAL(str);
                break;
            }

        case nsXPTType::T_WCHAR_STR:
            {
                jschar* p = *((jschar**)s);
                if(!p)
                    break;
                JSString* str;
                if(!(str = JS_NewUCStringCopyZ(cx, p)))
                    return JS_FALSE;
                *d = STRING_TO_JSVAL(str);
                break;
            }
        case nsXPTType::T_UTF8STRING:
            {                          
                const nsACString* cString = *((const nsACString**)s);

                if(!cString)
                    break;
                
                if(!cString->IsVoid()) 
                {
                    PRUint32 len;
                    jschar *p = (jschar *)UTF8ToNewUnicode(*cString, &len);

                    if(!p)
                        return JS_FALSE;

                    if(sXPCOMUCStringFinalizerIndex == -1 && 
                       !AddXPCOMUCStringFinalizer())
                        return JS_FALSE;

                    JSString* jsString =
                        JS_NewExternalString(cx, p, len,
                                             sXPCOMUCStringFinalizerIndex);

                    if(!jsString) {
                        nsMemory::Free(p); 
                        return JS_FALSE; 
                    }

                    *d = STRING_TO_JSVAL(jsString);
                }

                break;

            }
        case nsXPTType::T_CSTRING:
            {                          
                const nsACString* cString = *((const nsACString**)s);

                if(!cString)
                    break;
                
                if(!cString->IsVoid()) 
                {
                    PRUnichar* unicodeString = ToNewUnicode(*cString);
                    if(!unicodeString)
                        return JS_FALSE;

                    if(sXPCOMUCStringFinalizerIndex == -1 && 
                       !AddXPCOMUCStringFinalizer())
                        return JS_FALSE;

                    JSString* jsString = JS_NewExternalString(cx,
                                             (jschar*)unicodeString,
                                             cString->Length(),
                                             sXPCOMUCStringFinalizerIndex);

                    if(!jsString)
                    {
                        nsMemory::Free(unicodeString);
                        return JS_FALSE;
                    }

                    *d = STRING_TO_JSVAL(jsString);
                }

                break;
            }

        case nsXPTType::T_INTERFACE:
        case nsXPTType::T_INTERFACE_IS:
            {
                nsISupports* iface = *((nsISupports**)s);
                if(iface)
                {
                    if(iid->Equals(NS_GET_IID(nsIVariant)))
                    {
                        nsCOMPtr<nsIVariant> variant = do_QueryInterface(iface);
                        if(!variant)
                            return JS_FALSE;

                        return XPCVariant::VariantDataToJS(lccx, variant, 
                                                           pErr, d);
                    }
                    
                    
                    
                    
                    
                    
                    
                    xpcObjectHelper helper(iface);
                    if(!NativeInterface2JSObject(lccx, d, nsnull, helper, iid,
                                                 nsnull, PR_TRUE,
                                                 OBJ_IS_NOT_GLOBAL, pErr))
                        return JS_FALSE;

#ifdef DEBUG
                    JSObject* jsobj = JSVAL_TO_OBJECT(*d);
                    if(jsobj && !jsobj->getParent())
                        NS_ASSERTION(jsobj->getClass()->flags & JSCLASS_IS_GLOBAL,
                                     "Why did we recreate this wrapper?");
#endif
                }
                break;
            }

        default:
            NS_ERROR("bad type");
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}



#ifdef DEBUG
static bool
CheckJSCharInCharRange(jschar c)
{
    if(ILLEGAL_RANGE(c))
    {
        
        static const size_t MSG_BUF_SIZE = 64;
        char msg[MSG_BUF_SIZE];
        JS_snprintf(msg, MSG_BUF_SIZE, "jschar out of char range; high bits of data lost: 0x%x", c);
        NS_WARNING(msg);
        return false;
    }

    return true;
}
#endif


JSBool
XPCConvert::JSData2Native(XPCCallContext& ccx, void* d, jsval s,
                          const nsXPTType& type,
                          JSBool useAllocator, const nsID* iid,
                          nsresult* pErr)
{
    NS_PRECONDITION(d, "bad param");

    JSContext* cx = ccx.GetJSContext();

    int32    ti;
    uint32   tu;
    jsdouble td;
    JSBool isDOMString = JS_TRUE;

    if(pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS;

    switch(type.TagPart())
    {
    case nsXPTType::T_I8     :
        if(!JS_ValueToECMAInt32(cx, s, &ti))
            return JS_FALSE;
        *((int8*)d)  = (int8) ti;
        break;
    case nsXPTType::T_I16    :
        if(!JS_ValueToECMAInt32(cx, s, &ti))
            return JS_FALSE;
        *((int16*)d)  = (int16) ti;
        break;
    case nsXPTType::T_I32    :
        if(!JS_ValueToECMAInt32(cx, s, (int32*)d))
            return JS_FALSE;
        break;
    case nsXPTType::T_I64    :
        if(JSVAL_IS_INT(s))
        {
            if(!JS_ValueToECMAInt32(cx, s, &ti))
                return JS_FALSE;
            LL_I2L(*((int64*)d),ti);

        }
        else
        {
            if(!JS_ValueToNumber(cx, s, &td))
                return JS_FALSE;
            LL_D2L(*((int64*)d),td);
        }
        break;
    case nsXPTType::T_U8     :
        if(!JS_ValueToECMAUint32(cx, s, &tu))
            return JS_FALSE;
        *((uint8*)d)  = (uint8) tu;
        break;
    case nsXPTType::T_U16    :
        if(!JS_ValueToECMAUint32(cx, s, &tu))
            return JS_FALSE;
        *((uint16*)d)  = (uint16) tu;
        break;
    case nsXPTType::T_U32    :
        if(!JS_ValueToECMAUint32(cx, s, (uint32*)d))
            return JS_FALSE;
        break;
    case nsXPTType::T_U64    :
        if(JSVAL_IS_INT(s))
        {
            if(!JS_ValueToECMAUint32(cx, s, &tu))
                return JS_FALSE;
            LL_UI2L(*((int64*)d),tu);
        }
        else
        {
            if(!JS_ValueToNumber(cx, s, &td))
                return JS_FALSE;
#ifdef XP_WIN
            
            *((uint64*)d) = (uint64)((int64) td);
#else
            LL_D2L(*((uint64*)d),td);
#endif
        }
        break;
    case nsXPTType::T_FLOAT  :
        if(!JS_ValueToNumber(cx, s, &td))
            return JS_FALSE;
        *((float*)d) = (float) td;
        break;
    case nsXPTType::T_DOUBLE :
        if(!JS_ValueToNumber(cx, s, (double*)d))
            return JS_FALSE;
        break;
    case nsXPTType::T_BOOL   :
        JS_ValueToBoolean(cx, s, (JSBool*)d);
        break;
    case nsXPTType::T_CHAR   :
        {
            JSString* str = JS_ValueToString(cx, s);
            if(!str)
            {
                return JS_FALSE;
            }
            size_t length;
            const jschar* chars = JS_GetStringCharsAndLength(cx, str, &length);
            if (!chars)
            {
                return JS_FALSE;
            }
            jschar ch = length ? chars[0] : 0;
#ifdef DEBUG
            CheckJSCharInCharRange(ch);
#endif
            *((char*)d) = char(ch);
            break;
        }
    case nsXPTType::T_WCHAR  :
        {
            JSString* str;
            if(!(str = JS_ValueToString(cx, s)))
            {
                return JS_FALSE;
            }
            size_t length;
            const jschar* chars = JS_GetStringCharsAndLength(cx, str, &length);
            if (!chars)
            {
                return JS_FALSE;
            }
            if(length == 0)
            {
                *((uint16*)d) = 0;
                break;
            }
            *((uint16*)d) = (uint16) chars[0];
            break;
        }
    case nsXPTType::T_JSVAL :
        {
            if (useAllocator) {
                
                jsval *buf = new jsval(s);
                if(!buf)
                    return JS_FALSE;
                *((jsval**)d) = buf;
            } else {
                *((jsval*)d) = s;
            }
            break;
        }
    default:
        if(!type.IsPointer())
        {
            NS_ERROR("unsupported type");
            return JS_FALSE;
        }

        switch(type.TagPart())
        {
        case nsXPTType::T_VOID:
            XPC_LOG_ERROR(("XPCConvert::JSData2Native : void* params not supported"));
            NS_ERROR("void* params not supported");
            return JS_FALSE;
        case nsXPTType::T_IID:
        {
            NS_ASSERTION(useAllocator,"trying to convert a JSID to nsID without allocator : this would leak");

            JSObject* obj;
            const nsID* pid=nsnull;

            if(JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s))
            {
                if(type.IsReference())
                {
                    if(pErr)
                        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS_NULL_REF;
                    return JS_FALSE;
                }
                
                *((const nsID**)d) = nsnull;
                return JS_TRUE;
            }

            if(!JSVAL_IS_OBJECT(s) ||
               (!(obj = JSVAL_TO_OBJECT(s))) ||
               (!(pid = xpc_JSObjectToID(cx, obj))) ||
               (!(pid = (const nsID*) nsMemory::Clone(pid, sizeof(nsID)))))
            {
                return JS_FALSE;
            }
            *((const nsID**)d) = pid;
            return JS_TRUE;
        }

        case nsXPTType::T_ASTRING:        
        {            
            isDOMString = JS_FALSE;
            
        }
        case nsXPTType::T_DOMSTRING:
        {
            static const PRUnichar EMPTY_STRING[] = { '\0' };
            static const PRUnichar VOID_STRING[] = { 'u', 'n', 'd', 'e', 'f', 'i', 'n', 'e', 'd', '\0' };

            const PRUnichar* chars;
            JSString* str = nsnull;
            JSBool isNewString = JS_FALSE;
            PRUint32 length;

            if(JSVAL_IS_VOID(s))
            {
                if(isDOMString) 
                {
                    chars  = VOID_STRING;
                    length = NS_ARRAY_LENGTH(VOID_STRING) - 1;
                }
                else
                {
                    chars = EMPTY_STRING;
                    length = 0;
                }
            }
            else if(!JSVAL_IS_NULL(s))
            {
                str = JS_ValueToString(cx, s);
                if(!str)
                    return JS_FALSE;

                length = (PRUint32) JS_GetStringLength(str);
                if(length)
                {
                    chars = JS_GetStringCharsZ(cx, str);
                    if(!chars)
                        return JS_FALSE;
                    if(STRING_TO_JSVAL(str) != s)
                        isNewString = JS_TRUE;
                }
                else
                {
                    str = nsnull;
                    chars = EMPTY_STRING;
                }
            }

            if(useAllocator)
            {
                
                if(str && !isNewString)
                {
                    size_t strLength;
                    const jschar *strChars = JS_GetStringCharsZAndLength(cx, str, &strLength);
                    if (!strChars)
                        return JS_FALSE;

                    XPCReadableJSStringWrapper *wrapper =
                        ccx.NewStringWrapper(strChars, strLength);
                    if(!wrapper)
                        return JS_FALSE;

                    *((const nsAString**)d) = wrapper;
                }
                else if(JSVAL_IS_NULL(s))
                {
                    XPCReadableJSStringWrapper *wrapper =
                        new XPCReadableJSStringWrapper();
                    if(!wrapper)
                        return JS_FALSE;

                    *((const nsAString**)d) = wrapper;
                }
                else
                {
                    
                    const nsAString *rs = new nsString(chars, length);
                    if(!rs)
                        return JS_FALSE;
                    *((const nsAString**)d) = rs;
                }
            }
            else
            {
                nsAString* ws = *((nsAString**)d);

                if(JSVAL_IS_NULL(s) || (!isDOMString && JSVAL_IS_VOID(s)))
                {
                    ws->Truncate();
                    ws->SetIsVoid(PR_TRUE);
                }
                else
                    ws->Assign(chars, length);
            }
            return JS_TRUE;
        }

        case nsXPTType::T_CHAR_STR:
        {
            NS_ASSERTION(useAllocator,"cannot convert a JSString to char * without allocator");
            if(!useAllocator)
            {
                NS_ERROR("bad type");
                return JS_FALSE;
            }
            
            if(JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s))
            {
                if(type.IsReference())
                {
                    if(pErr)
                        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS_NULL_REF;
                    return JS_FALSE;
                }
                
                *((char**)d) = nsnull;
                return JS_TRUE;
            }

            JSString* str = JS_ValueToString(cx, s);
            if(!str)
            {
                return JS_FALSE;
            }
#ifdef DEBUG
            const jschar* chars=nsnull;
            if(nsnull != (chars = JS_GetStringCharsZ(cx, str)))
            {
                PRBool legalRange = PR_TRUE;
                int len = JS_GetStringLength(str);
                const jschar* t;
                PRInt32 i=0;
                for(t=chars; (i< len) && legalRange ; i++,t++) {
                    if(!CheckJSCharInCharRange(*t))
                        break;
                }
            }
#endif 
            size_t length = JS_GetStringEncodingLength(cx, str);
            if(length == size_t(-1))
            {
                return JS_FALSE;
            }
            char *buffer = static_cast<char *>(nsMemory::Alloc(length + 1));
            if(!buffer)
            {
                return JS_FALSE;
            }
            JS_EncodeStringToBuffer(str, buffer, length);
            buffer[length] = '\0';
            *((void**)d) = buffer;
            return JS_TRUE;
        }

        case nsXPTType::T_WCHAR_STR:
        {
            const jschar* chars=nsnull;
            JSString* str;

            if(JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s))
            {
                if(type.IsReference())
                {
                    if(pErr)
                        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS_NULL_REF;
                    return JS_FALSE;
                }
                
                *((jschar**)d) = nsnull;
                return JS_TRUE;
            }

            if(!(str = JS_ValueToString(cx, s)))
            {
                return JS_FALSE;
            }
            if(useAllocator)
            {
                if(!(chars = JS_GetStringCharsZ(cx, str)))
                {
                    return JS_FALSE;
                }
                int len = JS_GetStringLength(str);
                int byte_len = (len+1)*sizeof(jschar);
                if(!(*((void**)d) = nsMemory::Alloc(byte_len)))
                {
                    
                    return JS_FALSE;
                }
                jschar* destchars = *((jschar**)d);
                memcpy(destchars, chars, byte_len);
                destchars[len] = 0;
            }
            else
            {
                if(!(chars = JS_GetStringCharsZ(cx, str)))
                {
                    return JS_FALSE;
                }
                *((const jschar**)d) = chars;
            }

            return JS_TRUE;
        }

        case nsXPTType::T_UTF8STRING:            
        {
            const jschar* chars;
            PRUint32 length;
            JSString* str;

            if(JSVAL_IS_NULL(s) || JSVAL_IS_VOID(s))
            {
                if(useAllocator) 
                {
                    nsACString *rs = new nsCString();
                    if(!rs) 
                        return JS_FALSE;

                    rs->SetIsVoid(PR_TRUE);
                    *((nsACString**)d) = rs;
                }
                else
                {
                    nsCString* rs = *((nsCString**)d);
                    rs->Truncate();
                    rs->SetIsVoid(PR_TRUE);
                }
                return JS_TRUE;
            }

            

            if(!(str = JS_ValueToString(cx, s))||
               !(chars = JS_GetStringCharsZ(cx, str)))
            {
                return JS_FALSE;
            }

            length = JS_GetStringLength(str);

            nsCString *rs;
            if(useAllocator)
            {                
                
                rs = new nsCString();
                if(!rs)
                    return JS_FALSE;

                *((const nsCString**)d) = rs;
            }
            else
            {
                rs = *((nsCString**)d);
            }
            const PRUnichar* start = (const PRUnichar*)chars;
            const PRUnichar* end = start + length;
            CopyUTF16toUTF8(nsDependentSubstring(start, end), *rs);
            return JS_TRUE;
        }

        case nsXPTType::T_CSTRING:
        {
            if(JSVAL_IS_NULL(s) || JSVAL_IS_VOID(s))
            {
                if(useAllocator)
                {
                    nsACString *rs = new nsCString();
                    if(!rs) 
                        return JS_FALSE;

                    rs->SetIsVoid(PR_TRUE);
                    *((nsACString**)d) = rs;
                }
                else
                {
                    nsACString* rs = *((nsACString**)d);
                    rs->Truncate();
                    rs->SetIsVoid(PR_TRUE);
                }
                return JS_TRUE;
            }

            
            JSString* str = JS_ValueToString(cx, s);
            if(!str)
            {
                return JS_FALSE;
            }

            size_t length = JS_GetStringEncodingLength(cx, str);
            if(length == size_t(-1))
            {
                return JS_FALSE;
            }

            nsACString *rs;
            if(useAllocator)
            {
                rs = new nsCString();
                if(!rs)
                    return JS_FALSE;
                *((const nsACString**)d) = rs;
            }
            else
            {
                rs = *((nsACString**)d);
            }

            rs->SetLength(PRUint32(length));
            if(rs->Length() != PRUint32(length))
            {
                return JS_FALSE;
            }
            JS_EncodeStringToBuffer(str, rs->BeginWriting(), length);

            return JS_TRUE;
        }

        case nsXPTType::T_INTERFACE:
        case nsXPTType::T_INTERFACE_IS:
        {
            JSObject* obj;
            NS_ASSERTION(iid,"can't do interface conversions without iid");

            if(iid->Equals(NS_GET_IID(nsIVariant)))
            {
                XPCVariant* variant = XPCVariant::newVariant(ccx, s);
                if(!variant)
                    return JS_FALSE;
                *((nsISupports**)d) = static_cast<nsIVariant*>(variant);
                return JS_TRUE;
            }
            else if(iid->Equals(NS_GET_IID(nsIAtom)) &&
                    JSVAL_IS_STRING(s))
            {
                
                JSString* str = JSVAL_TO_STRING(s);
                const PRUnichar* chars = JS_GetStringCharsZ(cx, str);
                if (!chars) {
                    if (pErr)
                        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS_NULL_REF;
                    return JS_FALSE;
                }
                PRUint32 length = JS_GetStringLength(str);
                nsIAtom* atom = NS_NewAtom(nsDependentSubstring(chars,
                                             chars + length));
                if (!atom && pErr)
                    *pErr = NS_ERROR_OUT_OF_MEMORY;
                *((nsISupports**)d) = atom;
                return atom != nsnull;                
            }
            

            if(JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s))
            {
                if(type.IsReference())
                {
                    if(pErr)
                        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS_NULL_REF;
                    return JS_FALSE;
                }
                
                *((nsISupports**)d) = nsnull;
                return JS_TRUE;
            }

            
            if(!JSVAL_IS_OBJECT(s) || !(obj = JSVAL_TO_OBJECT(s)))
            {
                if(pErr && JSVAL_IS_INT(s) && 0 == JSVAL_TO_INT(s))
                    *pErr = NS_ERROR_XPC_BAD_CONVERT_JS_ZERO_ISNOT_NULL;
                return JS_FALSE;
            }

            return JSObject2NativeInterface(ccx, (void**)d, obj, iid,
                                            nsnull, pErr);
        }
        default:
            NS_ERROR("bad type");
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

inline JSBool
CreateHolderIfNeeded(XPCCallContext& ccx, JSObject* obj, jsval* d,
                     nsIXPConnectJSObjectHolder** dest)
{
    if(dest)
    {
        XPCJSObjectHolder* objHolder = XPCJSObjectHolder::newHolder(ccx, obj);
        if(!objHolder)
            return JS_FALSE;

        NS_ADDREF(*dest = objHolder);
    }

    *d = OBJECT_TO_JSVAL(obj);

    return JS_TRUE;
}



JSBool
XPCConvert::NativeInterface2JSObject(XPCLazyCallContext& lccx,
                                     jsval* d,
                                     nsIXPConnectJSObjectHolder** dest,
                                     xpcObjectHelper& aHelper,
                                     const nsID* iid,
                                     XPCNativeInterface** Interface,
                                     PRBool allowNativeWrapper,
                                     PRBool isGlobal,
                                     nsresult* pErr)
{
    NS_ASSERTION(!Interface || iid,
                 "Need the iid if you pass in an XPCNativeInterface cache.");

    *d = JSVAL_NULL;
    if(dest)
        *dest = nsnull;
    nsISupports *src = aHelper.Object();
    if(!src)
        return JS_TRUE;
    if(pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    
    
    
    
    
    
    
    JSContext* cx = lccx.GetJSContext();
    NS_ABORT_IF_FALSE(lccx.GetScopeForNewJSObjects()->compartment() == cx->compartment,
                      "bad scope for new JSObjects");

    JSObject *jsscope = lccx.GetScopeForNewJSObjects();
    XPCWrappedNativeScope* xpcscope =
        XPCWrappedNativeScope::FindInJSObjectScope(cx, jsscope);
    if(!xpcscope)
        return JS_FALSE;

    
    
    
    
    
    
    nsWrapperCache *cache = aHelper.GetWrapperCache();

    PRBool tryConstructSlimWrapper = PR_FALSE;
    JSObject *flat;
    if(cache)
    {
        flat = cache->GetWrapper();
        if(cache->IsProxy())
        {
            XPCCallContext &ccx = lccx.GetXPCCallContext();
            if(!ccx.IsValid())
                return JS_FALSE;

            if(!flat)
                flat = ConstructProxyObject(ccx, aHelper, xpcscope);

            if(!JS_WrapObject(ccx, &flat))
                return JS_FALSE;

            return CreateHolderIfNeeded(ccx, flat, d, dest);
        }

        if(!dest)
        {
            if(!flat)
            {
                tryConstructSlimWrapper = PR_TRUE;
            }
            else if(IS_SLIM_WRAPPER_OBJECT(flat))
            {
                if(flat->compartment() == cx->compartment)
                {
                    *d = OBJECT_TO_JSVAL(flat);
                    return JS_TRUE;
                }
            }
        }
    }
    else
    {
        flat = nsnull;
    }

    
    
    if(tryConstructSlimWrapper)
    {
        XPCCallContext &ccx = lccx.GetXPCCallContext();
        if(!ccx.IsValid())
            return JS_FALSE;

        jsval slim;
        if(ConstructSlimWrapper(ccx, aHelper, xpcscope, &slim))
        {
            *d = slim;
            return JS_TRUE;
        }

        
        
        
        
        flat = cache->GetWrapper();
    }

    
    
    
    
    
    AutoMarkingNativeInterfacePtr iface;
    if(iid)
    {
        XPCCallContext &ccx = lccx.GetXPCCallContext();
        if(!ccx.IsValid())
            return JS_FALSE;

        iface.Init(ccx);

        if(Interface)
            iface = *Interface;

        if(!iface)
        {
            iface = XPCNativeInterface::GetNewOrUsed(ccx, iid);
            if(!iface)
                return JS_FALSE;

            if(Interface)
                *Interface = iface;
        }
    }

    NS_ASSERTION(!flat || IS_WRAPPER_CLASS(flat->getClass()),
                 "What kind of wrapper is this?");

    nsresult rv;
    XPCWrappedNative* wrapper;
    nsRefPtr<XPCWrappedNative> strongWrapper;
    if(!flat)
    {
        XPCCallContext &ccx = lccx.GetXPCCallContext();
        if(!ccx.IsValid())
            return JS_FALSE;

        rv = XPCWrappedNative::GetNewOrUsed(ccx, aHelper, xpcscope, iface,
                                            isGlobal,
                                            getter_AddRefs(strongWrapper));

        wrapper = strongWrapper;
    }
    else if(IS_WN_WRAPPER_OBJECT(flat))
    {
        wrapper = static_cast<XPCWrappedNative*>(xpc_GetJSPrivate(flat));

        
        
        
        if(dest)
            strongWrapper = wrapper;
        
        
        
        if(iface)
            wrapper->FindTearOff(lccx.GetXPCCallContext(), iface, JS_FALSE,
                                 &rv);
        else
            rv = NS_OK;
    }
    else
    {
        NS_ASSERTION(IS_SLIM_WRAPPER(flat),
                     "What kind of wrapper is this?");

        XPCCallContext &ccx = lccx.GetXPCCallContext();
        if(!ccx.IsValid())
            return JS_FALSE;

        SLIM_LOG(("***** morphing from XPCConvert::NativeInterface2JSObject"
                  "(%p)\n",
                  static_cast<nsISupports*>(xpc_GetJSPrivate(flat))));

        rv = XPCWrappedNative::Morph(ccx, flat, iface, cache,
                                     getter_AddRefs(strongWrapper));
        wrapper = strongWrapper;
    }

    if(pErr)
        *pErr = rv;

    
    if(NS_FAILED(rv) || !wrapper)
        return JS_FALSE;

    
    
    flat = wrapper->GetFlatJSObject();
    jsval v = OBJECT_TO_JSVAL(flat);
    if(!XPCPerThreadData::IsMainThread(lccx.GetJSContext()) ||
       !allowNativeWrapper)
    {
        *d = v;
        if(dest)
            *dest = strongWrapper.forget().get();
        return JS_TRUE;
    }

    XPCCallContext &ccx = lccx.GetXPCCallContext();
    if(!ccx.IsValid())
        return JS_FALSE;

    JSObject *original = flat;
    if(!JS_WrapObject(ccx, &flat))
        return JS_FALSE;

    
    
    
    if(original == flat)
    {
        if(xpc::WrapperFactory::IsLocationObject(flat))
        {
            JSObject *locationWrapper = wrapper->GetWrapper();
            if(!locationWrapper)
            {
                locationWrapper = xpc::WrapperFactory::WrapLocationObject(cx, flat);
                if(!locationWrapper)
                    return JS_FALSE;

                
                
                wrapper->SetWrapper(locationWrapper);
            }

            flat = locationWrapper;
        }
        else if(wrapper->NeedsSOW() &&
                !xpc::AccessCheck::isChrome(cx->compartment))
        {
            JSObject *sowWrapper = wrapper->GetWrapper();
            if(!sowWrapper)
            {
                sowWrapper = xpc::WrapperFactory::WrapSOWObject(cx, flat);
                if(!sowWrapper)
                    return JS_FALSE;

                
                
                wrapper->SetWrapper(sowWrapper);
            }

            flat = sowWrapper;
        }
        else
        {
            OBJ_TO_OUTER_OBJECT(cx, flat);
            NS_ASSERTION(flat, "bad outer object hook!");
            NS_ASSERTION(flat->compartment() == cx->compartment,
                         "bad compartment");
        }
    }

    *d = OBJECT_TO_JSVAL(flat);

    if(dest)
    {
        
        if(flat == original)
        {
            *dest = strongWrapper.forget().get();
        }
        else
        {
            nsRefPtr<XPCJSObjectHolder> objHolder =
                XPCJSObjectHolder::newHolder(ccx, flat);
            if(!objHolder)
                return JS_FALSE;

            *dest = objHolder.forget().get();
        }
    }

    return JS_TRUE;
}




JSBool
XPCConvert::JSObject2NativeInterface(XPCCallContext& ccx,
                                     void** dest, JSObject* src,
                                     const nsID* iid,
                                     nsISupports* aOuter,
                                     nsresult* pErr)
{
    NS_ASSERTION(dest, "bad param");
    NS_ASSERTION(src, "bad param");
    NS_ASSERTION(iid, "bad param");

    JSContext* cx = ccx.GetJSContext();

    JSAutoEnterCompartment ac;

    if(!ac.enter(cx, src))
    {
       if(pErr)
           *pErr = NS_ERROR_UNEXPECTED;
       return PR_FALSE;
    }

    *dest = nsnull;
     if(pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS;

    nsISupports* iface;

    if(!aOuter)
    {
        
        
        
        

        
        
        
        JSObject* inner = nsnull;
        if(XPCWrapper::IsSecurityWrapper(src))
        {
            inner = XPCWrapper::Unwrap(cx, src);
            if(!inner)
            {
                if(pErr)
                    *pErr = NS_ERROR_XPC_SECURITY_MANAGER_VETO;
                return JS_FALSE;
            }
        }

        
        XPCWrappedNative* wrappedNative =
                    XPCWrappedNative::GetWrappedNativeOfJSObject(cx,
                                                                 inner
                                                                 ? inner
                                                                 : src);
        if(wrappedNative)
        {
            iface = wrappedNative->GetIdentityObject();
            return NS_SUCCEEDED(iface->QueryInterface(*iid, dest));
        }
        

        
        
        
        if(JS_TypeOfValue(cx, OBJECT_TO_JSVAL(src)) == JSTYPE_XML)
            return JS_FALSE;

        
        if(GetISupportsFromJSObject(src, &iface))
        {
            if(iface)
                return NS_SUCCEEDED(iface->QueryInterface(*iid, dest));

            return JS_FALSE;
        }
    }

    

    nsXPCWrappedJS* wrapper;
    nsresult rv = nsXPCWrappedJS::GetNewOrUsed(ccx, src, *iid, aOuter, &wrapper);
    if(pErr)
        *pErr = rv;
    if(NS_SUCCEEDED(rv) && wrapper)
    {
        
        
        
        
        rv = aOuter ? wrapper->AggregatedQueryInterface(*iid, dest) :
                      wrapper->QueryInterface(*iid, dest);
        if(pErr)
            *pErr = rv;
        NS_RELEASE(wrapper);
        return NS_SUCCEEDED(rv);        
    }

    
    return JS_FALSE;
}





nsresult
XPCConvert::ConstructException(nsresult rv, const char* message,
                               const char* ifaceName, const char* methodName,
                               nsISupports* data,
                               nsIException** exceptn,
                               JSContext* cx,
                               jsval* jsExceptionPtr)
{
    NS_ASSERTION(!cx == !jsExceptionPtr, "Expected cx and jsExceptionPtr to cooccur.");

    static const char format[] = "\'%s\' when calling method: [%s::%s]";
    const char * msg = message;
    char* sz = nsnull;
    nsXPIDLString xmsg;
    nsCAutoString sxmsg;

    nsCOMPtr<nsIScriptError> errorObject = do_QueryInterface(data);
    if(errorObject) {
        if (NS_SUCCEEDED(errorObject->GetMessageMoz(getter_Copies(xmsg)))) {
            CopyUTF16toUTF8(xmsg, sxmsg);
            msg = sxmsg.get();
        }
    }
    if(!msg)
        if(!nsXPCException::NameAndFormatForNSResult(rv, nsnull, &msg) || ! msg)
            msg = "<error>";
    if(ifaceName && methodName)
        msg = sz = JS_smprintf(format, msg, ifaceName, methodName);

    nsresult res = nsXPCException::NewException(msg, rv, nsnull, data, exceptn);

    if(NS_SUCCEEDED(res) && cx && jsExceptionPtr && *exceptn)
    {
        nsCOMPtr<nsIXPCException> xpcEx = do_QueryInterface(*exceptn);
        if(xpcEx)
            xpcEx->StowJSVal(cx, *jsExceptionPtr);
    }

    if(sz)
        JS_smprintf_free(sz);
    return res;
}



class AutoExceptionRestorer
{
public:
    AutoExceptionRestorer(JSContext *cx, jsval v)
        : mContext(cx), tvr(cx, v)
    {
        JS_ClearPendingException(mContext);
    }

    ~AutoExceptionRestorer()
    {
        JS_SetPendingException(mContext, tvr.jsval_value());
    }

private:
    JSContext * const mContext;
    js::AutoValueRooter tvr;
};


nsresult
XPCConvert::JSValToXPCException(XPCCallContext& ccx,
                                jsval s,
                                const char* ifaceName,
                                const char* methodName,
                                nsIException** exceptn)
{
    JSContext* cx = ccx.GetJSContext();
    AutoExceptionRestorer aer(cx, s);

    if(!JSVAL_IS_PRIMITIVE(s))
    {
        
        JSObject* obj = JSVAL_TO_OBJECT(s);

        if(!obj)
        {
            NS_ERROR("when is an object not an object?");
            return NS_ERROR_FAILURE;
        }

        
        XPCWrappedNative* wrapper;
        if(nsnull != (wrapper =
           XPCWrappedNative::GetWrappedNativeOfJSObject(cx,obj)))
        {
            nsISupports* supports = wrapper->GetIdentityObject();
            nsCOMPtr<nsIException> iface = do_QueryInterface(supports);
            if(iface)
            {
                
                nsIException* temp = iface;
                NS_ADDREF(temp);
                *exceptn = temp;
                return NS_OK;
            }
            else
            {
                
                return ConstructException(NS_ERROR_XPC_JS_THREW_NATIVE_OBJECT,
                                          nsnull, ifaceName, methodName, supports,
                                          exceptn, nsnull, nsnull);
            }
        }
        else
        {
            

            
            
            const JSErrorReport* report;
            if(nsnull != (report = JS_ErrorFromException(cx, s)))
            {
                JSAutoByteString message;
                JSString* str;
                if(nsnull != (str = JS_ValueToString(cx, s)))
                    message.encode(cx, str);
                return JSErrorToXPCException(ccx, message.ptr(), ifaceName,
                                             methodName, report, exceptn);
            }


            uintN ignored;
            JSBool found;

            
            if(JS_GetPropertyAttributes(cx, obj, "message", &ignored, &found) &&
               found &&
               JS_GetPropertyAttributes(cx, obj, "result", &ignored, &found) &&
               found)
            {
                
                nsXPCWrappedJS* jswrapper;
                nsresult rv =
                    nsXPCWrappedJS::GetNewOrUsed(ccx, obj,
                                                 NS_GET_IID(nsIException),
                                                 nsnull, &jswrapper);
                if(NS_FAILED(rv))
                    return rv;
                *exceptn = reinterpret_cast<nsIException*>
                           (jswrapper);
                return NS_OK;
            }


            
            
            
            

            

            JSString* str = JS_ValueToString(cx, s);
            if(!str)
                return NS_ERROR_FAILURE;

            JSAutoByteString strBytes(cx, str);
            if (!strBytes)
                return NS_ERROR_FAILURE;

            return ConstructException(NS_ERROR_XPC_JS_THREW_JS_OBJECT,
                                      strBytes.ptr(), ifaceName, methodName,
                                      nsnull, exceptn, cx, &s);
        }
    }

    if(JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s))
    {
        return ConstructException(NS_ERROR_XPC_JS_THREW_NULL,
                                  nsnull, ifaceName, methodName, nsnull,
                                  exceptn, cx, &s);
    }

    if(JSVAL_IS_NUMBER(s))
    {
        
        nsresult rv;
        double number;
        JSBool isResult = JS_FALSE;

        if(JSVAL_IS_INT(s))
        {
            rv = (nsresult) JSVAL_TO_INT(s);
            if(NS_FAILED(rv))
                isResult = JS_TRUE;
            else
                number = (double) JSVAL_TO_INT(s);
        }
        else
        {
            number = JSVAL_TO_DOUBLE(s);
            if(number > 0.0 &&
               number < (double)0xffffffff &&
               0.0 == fmod(number,1))
            {
                rv = (nsresult) number;
                if(NS_FAILED(rv))
                    isResult = JS_TRUE;
            }
        }

        if(isResult)
            return ConstructException(rv, nsnull, ifaceName, methodName,
                                      nsnull, exceptn, cx, &s);
        else
        {
            
            
            nsISupportsDouble* data;
            nsCOMPtr<nsIComponentManager> cm;
            if(NS_FAILED(NS_GetComponentManager(getter_AddRefs(cm))) || !cm ||
               NS_FAILED(cm->CreateInstanceByContractID(
                                NS_SUPPORTS_DOUBLE_CONTRACTID,
                                nsnull,
                                NS_GET_IID(nsISupportsDouble),
                                (void**)&data)))
                return NS_ERROR_FAILURE;
            data->SetData(number);
            rv = ConstructException(NS_ERROR_XPC_JS_THREW_NUMBER, nsnull,
                                    ifaceName, methodName, data, exceptn, cx, &s);
            NS_RELEASE(data);
            return rv;
        }
    }

    
    

    JSString* str = JS_ValueToString(cx, s);
    if(str)
    {
        JSAutoByteString strBytes(cx, str);
        if(!!strBytes)
        {
            return ConstructException(NS_ERROR_XPC_JS_THREW_STRING,
                                      strBytes.ptr(), ifaceName, methodName,
                                      nsnull, exceptn, cx, &s);
        }
    }
    return NS_ERROR_FAILURE;
}




nsresult
XPCConvert::JSErrorToXPCException(XPCCallContext& ccx,
                                  const char* message,
                                  const char* ifaceName,
                                  const char* methodName,
                                  const JSErrorReport* report,
                                  nsIException** exceptn)
{
    nsresult rv = NS_ERROR_FAILURE;
    nsRefPtr<nsScriptError> data;
    if(report)
    {
        nsAutoString bestMessage;
        if(report && report->ucmessage)
        {
            bestMessage = (const PRUnichar *)report->ucmessage;
        }
        else if(message)
        {
            bestMessage.AssignWithConversion(message);
        }
        else
        {
            bestMessage.AssignLiteral("JavaScript Error");
        }

        data = new nsScriptError();
        if(!data)
            return NS_ERROR_OUT_OF_MEMORY;


        data->InitWithWindowID(bestMessage.get(),
                               NS_ConvertASCIItoUTF16(report->filename).get(),
                               (const PRUnichar *)report->uclinebuf, report->lineno,
                               report->uctokenptr - report->uclinebuf, report->flags,
                               "XPConnect JavaScript",
                               nsJSUtils::GetCurrentlyRunningCodeWindowID(ccx.GetJSContext()));
    }

    if(data)
    {
        nsCAutoString formattedMsg;
        data->ToString(formattedMsg);

        rv = ConstructException(NS_ERROR_XPC_JAVASCRIPT_ERROR_WITH_DETAILS,
                                formattedMsg.get(), ifaceName, methodName,
                                static_cast<nsIScriptError*>(data.get()),
                                exceptn, nsnull, nsnull);
    }
    else
    {
        rv = ConstructException(NS_ERROR_XPC_JAVASCRIPT_ERROR,
                                nsnull, ifaceName, methodName, nsnull,
                                exceptn, nsnull, nsnull);
    }
    return rv;
}








#ifdef HAVE_VA_COPY
#define VARARGS_ASSIGN(foo, bar)	VA_COPY(foo,bar)
#elif defined(HAVE_VA_LIST_AS_ARRAY)
#define VARARGS_ASSIGN(foo, bar)	foo[0] = bar[0]
#else
#define VARARGS_ASSIGN(foo, bar)	(foo) = (bar)
#endif


const char* XPC_ARG_FORMATTER_FORMAT_STRINGS[] = {"%ip", "%iv", "%is", nsnull};

JSBool
XPC_JSArgumentFormatter(JSContext *cx, const char *format,
                        JSBool fromJS, jsval **vpp, va_list *app)
{
    XPCCallContext ccx(NATIVE_CALLER, cx);
    if(!ccx.IsValid())
        return JS_FALSE;

    jsval *vp;
    va_list ap;

    vp = *vpp;
    VARARGS_ASSIGN(ap, *app);

    nsXPTType type;
    const nsIID* iid;
    void* p;

    NS_ASSERTION(format[0] == '%' && format[1] == 'i', "bad format!");
    char which = format[2];

    if(fromJS)
    {
        switch(which)
        {
            case 'p':
                type = nsXPTType((uint8)(TD_INTERFACE_TYPE | XPT_TDP_POINTER));                
                iid = &NS_GET_IID(nsISupports);
                break;
            case 'v':
                type = nsXPTType((uint8)(TD_INTERFACE_TYPE | XPT_TDP_POINTER));                
                iid = &NS_GET_IID(nsIVariant);
                break;
            case 's':
                type = nsXPTType((uint8)(TD_DOMSTRING | XPT_TDP_POINTER));                
                iid = nsnull;
                p = va_arg(ap, void *);
                break;
            default:
                NS_ERROR("bad format!");
                return JS_FALSE;
        }

        if(!XPCConvert::JSData2Native(ccx, &p, vp[0], type, JS_FALSE,
                                      iid, nsnull))
            return JS_FALSE;
        
        if(which != 's')
            *va_arg(ap, void **) = p;
    }
    else
    {
        switch(which)
        {
            case 'p':
                type = nsXPTType((uint8)(TD_INTERFACE_TYPE | XPT_TDP_POINTER));                
                iid  = va_arg(ap, const nsIID*);
                break;
            case 'v':
                type = nsXPTType((uint8)(TD_INTERFACE_TYPE | XPT_TDP_POINTER));                
                iid = &NS_GET_IID(nsIVariant);
                break;
            case 's':
                type = nsXPTType((uint8)(TD_DOMSTRING | XPT_TDP_POINTER));                
                iid = nsnull;
                break;
            default:
                NS_ERROR("bad format!");
                return JS_FALSE;
        }

        
        p = va_arg(ap, void *);

        ccx.SetScopeForNewJSObjects(JS_GetGlobalForScopeChain(cx));
        if(!XPCConvert::NativeData2JS(ccx, &vp[0], &p, type, iid, nsnull))
            return JS_FALSE;
    }
    *vpp = vp + 1;
    VARARGS_ASSIGN(*app, ap);
    return JS_TRUE;
}





#ifdef POPULATE
#undef POPULATE
#endif


JSBool
XPCConvert::NativeArray2JS(XPCLazyCallContext& lccx,
                           jsval* d, const void** s,
                           const nsXPTType& type, const nsID* iid,
                           JSUint32 count, nsresult* pErr)
{
    NS_PRECONDITION(s, "bad param");
    NS_PRECONDITION(d, "bad param");

    XPCCallContext& ccx = lccx.GetXPCCallContext();
    if(!ccx.IsValid())
        return JS_FALSE;

    JSContext* cx = ccx.GetJSContext();
    NS_ABORT_IF_FALSE(lccx.GetScopeForNewJSObjects()->compartment() == cx->compartment,
                      "bad scope for new JSObjects");

    

    

    JSObject *array = JS_NewArrayObject(cx, count, nsnull);

    if(!array)
        return JS_FALSE;

    
    *d = OBJECT_TO_JSVAL(array);
    AUTO_MARK_JSVAL(ccx, d);

    if(pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    JSUint32 i;
    jsval current = JSVAL_NULL;
    AUTO_MARK_JSVAL(ccx, &current);

#define POPULATE(_t)                                                         \
    PR_BEGIN_MACRO                                                           \
        for(i = 0; i < count; i++)                                           \
        {                                                                    \
            if(!NativeData2JS(ccx, &current, ((_t*)*s)+i, type, iid, pErr) ||\
               !JS_SetElement(cx, array, i, &current))                       \
                goto failure;                                                \
        }                                                                    \
    PR_END_MACRO

    

    switch(type.TagPart())
    {
    case nsXPTType::T_I8            : POPULATE(int8);           break;
    case nsXPTType::T_I16           : POPULATE(int16);          break;
    case nsXPTType::T_I32           : POPULATE(int32);          break;
    case nsXPTType::T_I64           : POPULATE(int64);          break;
    case nsXPTType::T_U8            : POPULATE(uint8);          break;
    case nsXPTType::T_U16           : POPULATE(uint16);         break;
    case nsXPTType::T_U32           : POPULATE(uint32);         break;
    case nsXPTType::T_U64           : POPULATE(uint64);         break;
    case nsXPTType::T_FLOAT         : POPULATE(float);          break;
    case nsXPTType::T_DOUBLE        : POPULATE(double);         break;
    case nsXPTType::T_BOOL          : POPULATE(PRBool);         break;
    case nsXPTType::T_CHAR          : POPULATE(char);           break;
    case nsXPTType::T_WCHAR         : POPULATE(jschar);         break;
    case nsXPTType::T_VOID          : NS_ERROR("bad type"); goto failure;
    case nsXPTType::T_IID           : POPULATE(nsID*);          break;
    case nsXPTType::T_DOMSTRING     : NS_ERROR("bad type"); goto failure;
    case nsXPTType::T_CHAR_STR      : POPULATE(char*);          break;
    case nsXPTType::T_WCHAR_STR     : POPULATE(jschar*);        break;
    case nsXPTType::T_INTERFACE     : POPULATE(nsISupports*);   break;
    case nsXPTType::T_INTERFACE_IS  : POPULATE(nsISupports*);   break;
    case nsXPTType::T_UTF8STRING    : NS_ERROR("bad type"); goto failure;
    case nsXPTType::T_CSTRING       : NS_ERROR("bad type"); goto failure;
    case nsXPTType::T_ASTRING       : NS_ERROR("bad type"); goto failure;
    default                         : NS_ERROR("bad type"); goto failure;
    }

    if(pErr)
        *pErr = NS_OK;
    return JS_TRUE;

failure:
    return JS_FALSE;

#undef POPULATE
}


JSBool
XPCConvert::JSArray2Native(XPCCallContext& ccx, void** d, jsval s,
                           JSUint32 count, JSUint32 capacity,
                           const nsXPTType& type,
                           JSBool useAllocator, const nsID* iid,
                           uintN* pErr)
{
    NS_PRECONDITION(d, "bad param");

    JSContext* cx = ccx.GetJSContext();

    
    enum CleanupMode {na, fr, re};

    CleanupMode cleanupMode;

    JSObject* jsarray = nsnull;
    void* array = nsnull;
    JSUint32 initedCount;
    jsval current;

    

    

    if(JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s))
    {
        if(0 != count)
        {
            if(pErr)
                *pErr = NS_ERROR_XPC_NOT_ENOUGH_ELEMENTS_IN_ARRAY;
            return JS_FALSE;
        }

        
        
        if(0 != capacity)
            goto fill_array;

        *d = nsnull;
        return JS_TRUE;
    }

    if(!JSVAL_IS_OBJECT(s))
    {
        if(pErr)
            *pErr = NS_ERROR_XPC_CANT_CONVERT_PRIMITIVE_TO_ARRAY;
        return JS_FALSE;
    }

    jsarray = JSVAL_TO_OBJECT(s);
    if(!JS_IsArrayObject(cx, jsarray))
    {
        if(pErr)
            *pErr = NS_ERROR_XPC_CANT_CONVERT_OBJECT_TO_ARRAY;
        return JS_FALSE;
    }

    jsuint len;
    if(!JS_GetArrayLength(cx, jsarray, &len) || len < count || capacity < count)
    {
        if(pErr)
            *pErr = NS_ERROR_XPC_NOT_ENOUGH_ELEMENTS_IN_ARRAY;
        return JS_FALSE;
    }

    if(pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS;

#define POPULATE(_mode, _t)                                                  \
    PR_BEGIN_MACRO                                                           \
        cleanupMode = _mode;                                                 \
        if (capacity > PR_UINT32_MAX / sizeof(_t) ||                         \
            nsnull == (array = nsMemory::Alloc(capacity * sizeof(_t))))      \
        {                                                                    \
            if(pErr)                                                         \
                *pErr = NS_ERROR_OUT_OF_MEMORY;                              \
            goto failure;                                                    \
        }                                                                    \
        for(initedCount = 0; initedCount < count; initedCount++)             \
        {                                                                    \
            if(!JS_GetElement(cx, jsarray, initedCount, &current) ||         \
               !JSData2Native(ccx, ((_t*)array)+initedCount, current, type,  \
                              useAllocator, iid, pErr))                      \
                goto failure;                                                \
        }                                                                    \
    PR_END_MACRO


    

    

fill_array:
    switch(type.TagPart())
    {
    case nsXPTType::T_I8            : POPULATE(na, int8);           break;
    case nsXPTType::T_I16           : POPULATE(na, int16);          break;
    case nsXPTType::T_I32           : POPULATE(na, int32);          break;
    case nsXPTType::T_I64           : POPULATE(na, int64);          break;
    case nsXPTType::T_U8            : POPULATE(na, uint8);          break;
    case nsXPTType::T_U16           : POPULATE(na, uint16);         break;
    case nsXPTType::T_U32           : POPULATE(na, uint32);         break;
    case nsXPTType::T_U64           : POPULATE(na, uint64);         break;
    case nsXPTType::T_FLOAT         : POPULATE(na, float);          break;
    case nsXPTType::T_DOUBLE        : POPULATE(na, double);         break;
    case nsXPTType::T_BOOL          : POPULATE(na, PRBool);         break;
    case nsXPTType::T_CHAR          : POPULATE(na, char);           break;
    case nsXPTType::T_WCHAR         : POPULATE(na, jschar);         break;
    case nsXPTType::T_VOID          : NS_ERROR("bad type"); goto failure;
    case nsXPTType::T_IID           : POPULATE(fr, nsID*);          break;
    case nsXPTType::T_DOMSTRING     : NS_ERROR("bad type"); goto failure;
    case nsXPTType::T_CHAR_STR      : POPULATE(fr, char*);          break;
    case nsXPTType::T_WCHAR_STR     : POPULATE(fr, jschar*);        break;
    case nsXPTType::T_INTERFACE     : POPULATE(re, nsISupports*);   break;
    case nsXPTType::T_INTERFACE_IS  : POPULATE(re, nsISupports*);   break;
    case nsXPTType::T_UTF8STRING    : NS_ERROR("bad type"); goto failure;
    case nsXPTType::T_CSTRING       : NS_ERROR("bad type"); goto failure;
    case nsXPTType::T_ASTRING       : NS_ERROR("bad type"); goto failure;
    default                         : NS_ERROR("bad type"); goto failure;
    }

    *d = array;
    if(pErr)
        *pErr = NS_OK;
    return JS_TRUE;

failure:
    
    if(array)
    {
        if(cleanupMode == re)
        {
            nsISupports** a = (nsISupports**) array;
            for(PRUint32 i = 0; i < initedCount; i++)
            {
                nsISupports* p = a[i];
                NS_IF_RELEASE(p);
            }
        }
        else if(cleanupMode == fr && useAllocator)
        {
            void** a = (void**) array;
            for(PRUint32 i = 0; i < initedCount; i++)
            {
                void* p = a[i];
                if(p) nsMemory::Free(p);
            }
        }
        nsMemory::Free(array);
    }

    return JS_FALSE;

#undef POPULATE
}


JSBool
XPCConvert::NativeStringWithSize2JS(JSContext* cx,
                                    jsval* d, const void* s,
                                    const nsXPTType& type,
                                    JSUint32 count,
                                    nsresult* pErr)
{
    NS_PRECONDITION(s, "bad param");
    NS_PRECONDITION(d, "bad param");

    if(pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    if(!type.IsPointer())
    {
        XPC_LOG_ERROR(("XPCConvert::NativeStringWithSize2JS : unsupported type"));
        return JS_FALSE;
    }
    switch(type.TagPart())
    {
        case nsXPTType::T_PSTRING_SIZE_IS:
        {
            char* p = *((char**)s);
            if(!p)
                break;
            JSString* str;
            if(!(str = JS_NewStringCopyN(cx, p, count)))
                return JS_FALSE;
            *d = STRING_TO_JSVAL(str);
            break;
        }
        case nsXPTType::T_PWSTRING_SIZE_IS:
        {
            jschar* p = *((jschar**)s);
            if(!p)
                break;
            JSString* str;
            if(!(str = JS_NewUCStringCopyN(cx, p, count)))
                return JS_FALSE;
            *d = STRING_TO_JSVAL(str);
            break;
        }
        default:
            XPC_LOG_ERROR(("XPCConvert::NativeStringWithSize2JS : unsupported type"));
            return JS_FALSE;
    }
    return JS_TRUE;
}


JSBool
XPCConvert::JSStringWithSize2Native(XPCCallContext& ccx, void* d, jsval s,
                                    JSUint32 count, JSUint32 capacity,
                                    const nsXPTType& type,
                                    JSBool useAllocator,
                                    uintN* pErr)
{
    NS_PRECONDITION(!JSVAL_IS_NULL(s), "bad param");
    NS_PRECONDITION(d, "bad param");

    JSContext* cx = ccx.GetJSContext();

    JSUint32 len;

    if(pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    if(capacity < count)
    {
        if(pErr)
            *pErr = NS_ERROR_XPC_NOT_ENOUGH_CHARS_IN_STRING;
        return JS_FALSE;
    }

    if(!type.IsPointer())
    {
        XPC_LOG_ERROR(("XPCConvert::JSStringWithSize2Native : unsupported type"));
        return JS_FALSE;
    }
    switch(type.TagPart())
    {
        case nsXPTType::T_PSTRING_SIZE_IS:
        {
            NS_ASSERTION(useAllocator,"cannot convert a JSString to char * without allocator");
            if(!useAllocator)
            {
                XPC_LOG_ERROR(("XPCConvert::JSStringWithSize2Native : unsupported type"));
                return JS_FALSE;
            }

            if(JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s))
            {
                if(0 != count)
                {
                    if(pErr)
                        *pErr = NS_ERROR_XPC_NOT_ENOUGH_CHARS_IN_STRING;
                    return JS_FALSE;
                }
                if(type.IsReference())
                {
                    if(pErr)
                        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS_NULL_REF;
                    return JS_FALSE;
                }

                if(0 != capacity)
                {
                    len = (capacity + 1) * sizeof(char);
                    if(!(*((void**)d) = nsMemory::Alloc(len)))
                        return JS_FALSE;
                    return JS_TRUE;
                }
                

                *((char**)d) = nsnull;
                return JS_TRUE;
            }

            JSString* str = JS_ValueToString(cx, s);
            if(!str)
            {
                return JS_FALSE;
            }

            size_t length = JS_GetStringEncodingLength(cx, str);
            if (length == size_t(-1))
            {
                return JS_FALSE;
            }
            if(length > count)
            {
                if(pErr)
                    *pErr = NS_ERROR_XPC_NOT_ENOUGH_CHARS_IN_STRING;
                return JS_FALSE;
            }
            len = PRUint32(length);

            if(len < capacity)
                len = capacity;

            JSUint32 alloc_len = (len + 1) * sizeof(char);
            char *buffer = static_cast<char *>(nsMemory::Alloc(alloc_len));
            if(!buffer)
            {
                return JS_FALSE;
            }
            JS_EncodeStringToBuffer(str, buffer, len);
            buffer[len] = '\0';
            *((char**)d) = buffer;

            return JS_TRUE;
        }

        case nsXPTType::T_PWSTRING_SIZE_IS:
        {
            const jschar* chars=nsnull;
            JSString* str;

            if(JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s))
            {
                if(0 != count)
                {
                    if(pErr)
                        *pErr = NS_ERROR_XPC_NOT_ENOUGH_CHARS_IN_STRING;
                    return JS_FALSE;
                }
                if(type.IsReference())
                {
                    if(pErr)
                        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS_NULL_REF;
                    return JS_FALSE;
                }

                if(useAllocator && 0 != capacity)
                {
                    len = (capacity + 1) * sizeof(jschar);
                    if(!(*((void**)d) = nsMemory::Alloc(len)))
                        return JS_FALSE;
                    return JS_TRUE;
                }

                
                *((const jschar**)d) = nsnull;
                return JS_TRUE;
            }

            if(!(str = JS_ValueToString(cx, s)))
            {
                return JS_FALSE;
            }

            len = JS_GetStringLength(str);
            if(len > count)
            {
                if(pErr)
                    *pErr = NS_ERROR_XPC_NOT_ENOUGH_CHARS_IN_STRING;
                return JS_FALSE;
            }
            if(len < capacity)
                len = capacity;

            if(useAllocator)
            {
                if(!(chars = JS_GetStringCharsZ(cx, str)))
                {
                    return JS_FALSE;
                }
                JSUint32 alloc_len = (len + 1) * sizeof(jschar);
                if(!(*((void**)d) = nsMemory::Alloc(alloc_len)))
                {
                    
                    return JS_FALSE;
                }
                memcpy(*((jschar**)d), chars, alloc_len);
                (*((jschar**)d))[count] = 0;
            }
            else
            {
                if(!(chars = JS_GetStringCharsZ(cx, str)))
                {
                    return JS_FALSE;
                }
                *((const jschar**)d) = chars;
            }

            return JS_TRUE;
        }
        default:
            XPC_LOG_ERROR(("XPCConvert::JSStringWithSize2Native : unsupported type"));
            return JS_FALSE;
    }
}

