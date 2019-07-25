












































#include "mozilla/Util.h"

#include "xpcprivate.h"
#include "nsString.h"
#include "nsIAtom.h"
#include "XPCWrapper.h"
#include "nsJSPrincipals.h"
#include "nsWrapperCache.h"
#include "WrapperFactory.h"
#include "AccessCheck.h"
#include "nsJSUtils.h"

#include "dombindings.h"
#include "nsWrapperCacheInlines.h"

#include "jstypedarray.h"

using namespace mozilla;


#ifdef STRICT_CHECK_OF_UNICODE
#define ILLEGAL_RANGE(c) (0!=((c) & 0xFF80))
#else 
#define ILLEGAL_RANGE(c) (0!=((c) & 0xFF00))
#endif 

#define ILLEGAL_CHAR_RANGE(c) (0!=((c) & 0x80))

static intN sXPCOMUCStringFinalizerIndex = -1;




JSBool
XPCConvert::IsMethodReflectable(const XPTMethodDescriptor& info)
{
    if (XPT_MD_IS_NOTXPCOM(info.flags) || XPT_MD_IS_HIDDEN(info.flags))
        return false;

    for (int i = info.num_args-1; i >= 0; i--) {
        const nsXPTParamInfo& param = info.params[i];
        const nsXPTType& type = param.GetType();

        
        
        if (type.TagPart() == nsXPTType::T_VOID)
            return false;
    }
    return true;
}




JSBool
XPCConvert::GetISupportsFromJSObject(JSObject* obj, nsISupports** iface)
{
    JSClass* jsclass = js::GetObjectJSClass(obj);
    NS_ASSERTION(jsclass, "obj has no class");
    if (jsclass &&
        (jsclass->flags & JSCLASS_HAS_PRIVATE) &&
        (jsclass->flags & JSCLASS_PRIVATE_IS_NSISUPPORTS)) {
        *iface = (nsISupports*) xpc_GetJSPrivate(obj);
        return true;
    }
    return false;
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

    if (sXPCOMUCStringFinalizerIndex == -1) {
        return false;
    }

    return true;
}


void
XPCConvert::RemoveXPCOMUCStringFinalizer()
{
    JS_RemoveExternalStringFinalizer(FinalizeXPCOMUCString);
    sXPCOMUCStringFinalizerIndex = -1;
}


#define FIT_U32(i)     ((i) <= JSVAL_INT_MAX                                  \
                        ? INT_TO_JSVAL(i)                                     \
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
                      cx->compartment == js::GetObjectCompartment(lccx.GetScopeForNewJSObjects()),
                      "bad scope for new JSObjects");

    if (pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    switch (type.TagPart()) {
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
    case nsXPTType::T_BOOL  :
        {
            bool b = *((bool*)s);

            NS_WARN_IF_FALSE(b == 1 || b == 0,
                             "Passing a malformed bool through XPConnect");
            *d = BOOLEAN_TO_JSVAL(!!b);
            break;
        }
    case nsXPTType::T_CHAR  :
        {
            char* p = (char*)s;
            if (!p)
                return false;

#ifdef STRICT_CHECK_OF_UNICODE
            NS_ASSERTION(! ILLEGAL_CHAR_RANGE(p) , "passing non ASCII data");
#endif 

            JSString* str;
            if (!(str = JS_NewStringCopyN(cx, p, 1)))
                return false;
            *d = STRING_TO_JSVAL(str);
            break;
        }
    case nsXPTType::T_WCHAR :
        {
            jschar* p = (jschar*)s;
            if (!p)
                return false;
            JSString* str;
            if (!(str = JS_NewUCStringCopyN(cx, p, 1)))
                return false;
            *d = STRING_TO_JSVAL(str);
            break;
        }

    case nsXPTType::T_JSVAL :
        {
            *d = *((jsval*)s);
            if (!JS_WrapValue(cx, d))
                return false;
            break;
        }

    default:

        
        *d = JSVAL_NULL;

        switch (type.TagPart()) {
        case nsXPTType::T_VOID:
            XPC_LOG_ERROR(("XPCConvert::NativeData2JS : void* params not supported"));
            return false;

        case nsXPTType::T_IID:
            {
                nsID* iid2 = *((nsID**)s);
                if (!iid2)
                    break;
                JSObject* obj;
                if (!(obj = xpc_NewIDObject(cx, lccx.GetScopeForNewJSObjects(), *iid2)))
                    return false;
                *d = OBJECT_TO_JSVAL(obj);
                break;
            }

        case nsXPTType::T_ASTRING:
            

        case nsXPTType::T_DOMSTRING:
            {
                const nsAString* p = *((const nsAString**)s);
                if (!p)
                    break;

                if (!p->IsVoid()) {
                    nsStringBuffer* buf;
                    jsval str = XPCStringConvert::ReadableToJSVal(cx, *p, &buf);
                    if (JSVAL_IS_NULL(str))
                        return false;
                    if (buf)
                        buf->AddRef();

                    *d = str;
                }

                
                

                break;
            }

        case nsXPTType::T_CHAR_STR:
            {
                char* p = *((char**)s);
                if (!p)
                    break;

#ifdef STRICT_CHECK_OF_UNICODE
                bool isAscii = true;
                char* t;
                for (t=p; *t && isAscii ; t++) {
                  if (ILLEGAL_CHAR_RANGE(*t))
                      isAscii = false;
                }
                NS_ASSERTION(isAscii, "passing non ASCII data");
#endif 
                JSString* str;
                if (!(str = JS_NewStringCopyZ(cx, p)))
                    return false;
                *d = STRING_TO_JSVAL(str);
                break;
            }

        case nsXPTType::T_WCHAR_STR:
            {
                jschar* p = *((jschar**)s);
                if (!p)
                    break;
                JSString* str;
                if (!(str = JS_NewUCStringCopyZ(cx, p)))
                    return false;
                *d = STRING_TO_JSVAL(str);
                break;
            }
        case nsXPTType::T_UTF8STRING:
            {
                const nsACString* cString = *((const nsACString**)s);

                if (!cString)
                    break;

                if (!cString->IsVoid()) {
                    PRUint32 len;
                    jschar *p = (jschar *)UTF8ToNewUnicode(*cString, &len);

                    if (!p)
                        return false;

                    if (sXPCOMUCStringFinalizerIndex == -1 &&
                        !AddXPCOMUCStringFinalizer())
                        return false;

                    JSString* jsString =
                        JS_NewExternalString(cx, p, len,
                                             sXPCOMUCStringFinalizerIndex);

                    if (!jsString) {
                        nsMemory::Free(p);
                        return false;
                    }

                    *d = STRING_TO_JSVAL(jsString);
                }

                break;

            }
        case nsXPTType::T_CSTRING:
            {
                const nsACString* cString = *((const nsACString**)s);

                if (!cString)
                    break;

                if (!cString->IsVoid()) {
                    PRUnichar* unicodeString = ToNewUnicode(*cString);
                    if (!unicodeString)
                        return false;

                    if (sXPCOMUCStringFinalizerIndex == -1 &&
                        !AddXPCOMUCStringFinalizer())
                        return false;

                    JSString* jsString = JS_NewExternalString(cx,
                                                              (jschar*)unicodeString,
                                                              cString->Length(),
                                                              sXPCOMUCStringFinalizerIndex);

                    if (!jsString) {
                        nsMemory::Free(unicodeString);
                        return false;
                    }

                    *d = STRING_TO_JSVAL(jsString);
                }

                break;
            }

        case nsXPTType::T_INTERFACE:
        case nsXPTType::T_INTERFACE_IS:
            {
                nsISupports* iface = *((nsISupports**)s);
                if (iface) {
                    if (iid->Equals(NS_GET_IID(nsIVariant))) {
                        nsCOMPtr<nsIVariant> variant = do_QueryInterface(iface);
                        if (!variant)
                            return false;

                        return XPCVariant::VariantDataToJS(lccx, variant,
                                                           pErr, d);
                    }
                    

                    
                    
                    
                    
                    
                    xpcObjectHelper helper(iface);
                    if (!NativeInterface2JSObject(lccx, d, nsnull, helper, iid,
                                                  nsnull, true,
                                                  OBJ_IS_NOT_GLOBAL, pErr))
                        return false;

#ifdef DEBUG
                    JSObject* jsobj = JSVAL_TO_OBJECT(*d);
                    if (jsobj && !js::GetObjectParent(jsobj))
                        NS_ASSERTION(js::GetObjectClass(jsobj)->flags & JSCLASS_IS_GLOBAL,
                                     "Why did we recreate this wrapper?");
#endif
                }
                break;
            }

        default:
            NS_ERROR("bad type");
            return false;
        }
    }
    return true;
}



#ifdef DEBUG
static bool
CheckJSCharInCharRange(jschar c)
{
    if (ILLEGAL_RANGE(c)) {
        
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
    JSBool   tb;
    JSBool isDOMString = true;

    if (pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS;

    switch (type.TagPart()) {
    case nsXPTType::T_I8     :
        if (!JS_ValueToECMAInt32(cx, s, &ti))
            return false;
        *((int8*)d)  = (int8) ti;
        break;
    case nsXPTType::T_I16    :
        if (!JS_ValueToECMAInt32(cx, s, &ti))
            return false;
        *((int16*)d)  = (int16) ti;
        break;
    case nsXPTType::T_I32    :
        if (!JS_ValueToECMAInt32(cx, s, (int32*)d))
            return false;
        break;
    case nsXPTType::T_I64    :
        if (JSVAL_IS_INT(s)) {
            if (!JS_ValueToECMAInt32(cx, s, &ti))
                return false;
            LL_I2L(*((int64*)d),ti);

        } else {
            if (!JS_ValueToNumber(cx, s, &td))
                return false;
            LL_D2L(*((int64*)d),td);
        }
        break;
    case nsXPTType::T_U8     :
        if (!JS_ValueToECMAUint32(cx, s, &tu))
            return false;
        *((uint8*)d)  = (uint8) tu;
        break;
    case nsXPTType::T_U16    :
        if (!JS_ValueToECMAUint32(cx, s, &tu))
            return false;
        *((uint16*)d)  = (uint16) tu;
        break;
    case nsXPTType::T_U32    :
        if (!JS_ValueToECMAUint32(cx, s, (uint32*)d))
            return false;
        break;
    case nsXPTType::T_U64    :
        if (JSVAL_IS_INT(s)) {
            if (!JS_ValueToECMAUint32(cx, s, &tu))
                return false;
            LL_UI2L(*((int64*)d),tu);
        } else {
            if (!JS_ValueToNumber(cx, s, &td))
                return false;
#ifdef XP_WIN
            
            *((uint64*)d) = (uint64)((int64) td);
#else
            LL_D2L(*((uint64*)d),td);
#endif
        }
        break;
    case nsXPTType::T_FLOAT  :
        if (!JS_ValueToNumber(cx, s, &td))
            return false;
        *((float*)d) = (float) td;
        break;
    case nsXPTType::T_DOUBLE :
        if (!JS_ValueToNumber(cx, s, (double*)d))
            return false;
        break;
    case nsXPTType::T_BOOL   :
        JS_ValueToBoolean(cx, s, &tb);
        *((bool*)d) = tb;
        break;
    case nsXPTType::T_CHAR   :
        {
            JSString* str = JS_ValueToString(cx, s);
            if (!str) {
                return false;
            }
            size_t length;
            const jschar* chars = JS_GetStringCharsAndLength(cx, str, &length);
            if (!chars) {
                return false;
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
            if (!(str = JS_ValueToString(cx, s))) {
                return false;
            }
            size_t length;
            const jschar* chars = JS_GetStringCharsAndLength(cx, str, &length);
            if (!chars) {
                return false;
            }
            if (length == 0) {
                *((uint16*)d) = 0;
                break;
            }
            *((uint16*)d) = (uint16) chars[0];
            break;
        }
    case nsXPTType::T_JSVAL :
        *((jsval*)d) = s;
        break;
    default:

        switch (type.TagPart()) {
        case nsXPTType::T_VOID:
            XPC_LOG_ERROR(("XPCConvert::JSData2Native : void* params not supported"));
            NS_ERROR("void* params not supported");
            return false;
        case nsXPTType::T_IID:
        {
            JSObject* obj;
            const nsID* pid=nsnull;

            
            if (JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s)) {
                if (pErr)
                  *pErr = NS_ERROR_XPC_BAD_CONVERT_JS;
                return false;
            }

            if (!JSVAL_IS_OBJECT(s) ||
                (!(obj = JSVAL_TO_OBJECT(s))) ||
                (!(pid = xpc_JSObjectToID(cx, obj))) ||
                (!(pid = (const nsID*) nsMemory::Clone(pid, sizeof(nsID))))) {
                return false;
            }
            *((const nsID**)d) = pid;
            return true;
        }

        case nsXPTType::T_ASTRING:
        {
            isDOMString = false;
            
        }
        case nsXPTType::T_DOMSTRING:
        {
            static const PRUnichar EMPTY_STRING[] = { '\0' };
            static const PRUnichar VOID_STRING[] = { 'u', 'n', 'd', 'e', 'f', 'i', 'n', 'e', 'd', '\0' };

            const PRUnichar* chars = nsnull;
            JSString* str = nsnull;
            JSBool isNewString = false;
            PRUint32 length = 0;

            if (JSVAL_IS_VOID(s)) {
                if (isDOMString) {
                    chars  = VOID_STRING;
                    length = ArrayLength(VOID_STRING) - 1;
                } else {
                    chars = EMPTY_STRING;
                    length = 0;
                }
            } else if (!JSVAL_IS_NULL(s)) {
                str = JS_ValueToString(cx, s);
                if (!str)
                    return false;

                length = (PRUint32) JS_GetStringLength(str);
                if (length) {
                    chars = JS_GetStringCharsZ(cx, str);
                    if (!chars)
                        return false;
                    if (STRING_TO_JSVAL(str) != s)
                        isNewString = true;
                } else {
                    str = nsnull;
                    chars = EMPTY_STRING;
                }
            }

            if (useAllocator) {
                
                if (str && !isNewString) {
                    size_t strLength;
                    const jschar *strChars = JS_GetStringCharsZAndLength(cx, str, &strLength);
                    if (!strChars)
                        return false;

                    XPCReadableJSStringWrapper *wrapper =
                        ccx.NewStringWrapper(strChars, strLength);
                    if (!wrapper)
                        return false;

                    *((const nsAString**)d) = wrapper;
                } else if (JSVAL_IS_NULL(s)) {
                    XPCReadableJSStringWrapper *wrapper =
                        new XPCReadableJSStringWrapper();
                    if (!wrapper)
                        return false;

                    *((const nsAString**)d) = wrapper;
                } else {
                    
                    const nsAString *rs = new nsString(chars, length);
                    if (!rs)
                        return false;
                    *((const nsAString**)d) = rs;
                }
            } else {
                nsAString* ws = *((nsAString**)d);

                if (JSVAL_IS_NULL(s) || (!isDOMString && JSVAL_IS_VOID(s))) {
                    ws->Truncate();
                    ws->SetIsVoid(true);
                } else
                    ws->Assign(chars, length);
            }
            return true;
        }

        case nsXPTType::T_CHAR_STR:
        {
            if (JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s)) {
                *((char**)d) = nsnull;
                return true;
            }

            JSString* str = JS_ValueToString(cx, s);
            if (!str) {
                return false;
            }
#ifdef DEBUG
            const jschar* chars=nsnull;
            if (nsnull != (chars = JS_GetStringCharsZ(cx, str))) {
                bool legalRange = true;
                int len = JS_GetStringLength(str);
                const jschar* t;
                PRInt32 i=0;
                for (t=chars; (i< len) && legalRange ; i++,t++) {
                    if (!CheckJSCharInCharRange(*t))
                        break;
                }
            }
#endif 
            size_t length = JS_GetStringEncodingLength(cx, str);
            if (length == size_t(-1)) {
                return false;
            }
            char *buffer = static_cast<char *>(nsMemory::Alloc(length + 1));
            if (!buffer) {
                return false;
            }
            JS_EncodeStringToBuffer(str, buffer, length);
            buffer[length] = '\0';
            *((void**)d) = buffer;
            return true;
        }

        case nsXPTType::T_WCHAR_STR:
        {
            const jschar* chars=nsnull;
            JSString* str;

            if (JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s)) {
                *((jschar**)d) = nsnull;
                return true;
            }

            if (!(str = JS_ValueToString(cx, s))) {
                return false;
            }
            if (!(chars = JS_GetStringCharsZ(cx, str))) {
                return false;
            }
            int len = JS_GetStringLength(str);
            int byte_len = (len+1)*sizeof(jschar);
            if (!(*((void**)d) = nsMemory::Alloc(byte_len))) {
                
                return false;
            }
            jschar* destchars = *((jschar**)d);
            memcpy(destchars, chars, byte_len);
            destchars[len] = 0;

            return true;
        }

        case nsXPTType::T_UTF8STRING:
        {
            const jschar* chars;
            PRUint32 length;
            JSString* str;

            if (JSVAL_IS_NULL(s) || JSVAL_IS_VOID(s)) {
                if (useAllocator) {
                    nsACString *rs = new nsCString();
                    if (!rs)
                        return false;

                    rs->SetIsVoid(true);
                    *((nsACString**)d) = rs;
                } else {
                    nsCString* rs = *((nsCString**)d);
                    rs->Truncate();
                    rs->SetIsVoid(true);
                }
                return true;
            }

            

            if (!(str = JS_ValueToString(cx, s))||
                !(chars = JS_GetStringCharsZ(cx, str))) {
                return false;
            }

            length = JS_GetStringLength(str);

            nsCString *rs;
            if (useAllocator) {
                
                rs = new nsCString();
                if (!rs)
                    return false;

                *((const nsCString**)d) = rs;
            } else {
                rs = *((nsCString**)d);
            }
            const PRUnichar* start = (const PRUnichar*)chars;
            const PRUnichar* end = start + length;
            CopyUTF16toUTF8(nsDependentSubstring(start, end), *rs);
            return true;
        }

        case nsXPTType::T_CSTRING:
        {
            if (JSVAL_IS_NULL(s) || JSVAL_IS_VOID(s)) {
                if (useAllocator) {
                    nsACString *rs = new nsCString();
                    if (!rs)
                        return false;

                    rs->SetIsVoid(true);
                    *((nsACString**)d) = rs;
                } else {
                    nsACString* rs = *((nsACString**)d);
                    rs->Truncate();
                    rs->SetIsVoid(true);
                }
                return true;
            }

            
            JSString* str = JS_ValueToString(cx, s);
            if (!str) {
                return false;
            }

            size_t length = JS_GetStringEncodingLength(cx, str);
            if (length == size_t(-1)) {
                return false;
            }

            nsACString *rs;
            if (useAllocator) {
                rs = new nsCString();
                if (!rs)
                    return false;
                *((const nsACString**)d) = rs;
            } else {
                rs = *((nsACString**)d);
            }

            rs->SetLength(PRUint32(length));
            if (rs->Length() != PRUint32(length)) {
                return false;
            }
            JS_EncodeStringToBuffer(str, rs->BeginWriting(), length);

            return true;
        }

        case nsXPTType::T_INTERFACE:
        case nsXPTType::T_INTERFACE_IS:
        {
            JSObject* obj;
            NS_ASSERTION(iid,"can't do interface conversions without iid");

            if (iid->Equals(NS_GET_IID(nsIVariant))) {
                XPCVariant* variant = XPCVariant::newVariant(ccx, s);
                if (!variant)
                    return false;
                *((nsISupports**)d) = static_cast<nsIVariant*>(variant);
                return true;
            } else if (iid->Equals(NS_GET_IID(nsIAtom)) &&
                       JSVAL_IS_STRING(s)) {
                
                JSString* str = JSVAL_TO_STRING(s);
                const PRUnichar* chars = JS_GetStringCharsZ(cx, str);
                if (!chars) {
                    if (pErr)
                        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS_NULL_REF;
                    return false;
                }
                PRUint32 length = JS_GetStringLength(str);
                nsIAtom* atom = NS_NewAtom(nsDependentSubstring(chars,
                                                                chars + length));
                if (!atom && pErr)
                    *pErr = NS_ERROR_OUT_OF_MEMORY;
                *((nsISupports**)d) = atom;
                return atom != nsnull;
            }
            

            if (JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s)) {
                *((nsISupports**)d) = nsnull;
                return true;
            }

            
            if (!JSVAL_IS_OBJECT(s) || !(obj = JSVAL_TO_OBJECT(s))) {
                if (pErr && JSVAL_IS_INT(s) && 0 == JSVAL_TO_INT(s))
                    *pErr = NS_ERROR_XPC_BAD_CONVERT_JS_ZERO_ISNOT_NULL;
                return false;
            }

            return JSObject2NativeInterface(ccx, (void**)d, obj, iid,
                                            nsnull, pErr);
        }
        default:
            NS_ERROR("bad type");
            return false;
        }
    }
    return true;
}

inline JSBool
CreateHolderIfNeeded(XPCCallContext& ccx, JSObject* obj, jsval* d,
                     nsIXPConnectJSObjectHolder** dest)
{
    if (dest) {
        XPCJSObjectHolder* objHolder = XPCJSObjectHolder::newHolder(ccx, obj);
        if (!objHolder)
            return false;

        NS_ADDREF(*dest = objHolder);
    }

    *d = OBJECT_TO_JSVAL(obj);

    return true;
}



JSBool
XPCConvert::NativeInterface2JSObject(XPCLazyCallContext& lccx,
                                     jsval* d,
                                     nsIXPConnectJSObjectHolder** dest,
                                     xpcObjectHelper& aHelper,
                                     const nsID* iid,
                                     XPCNativeInterface** Interface,
                                     bool allowNativeWrapper,
                                     bool isGlobal,
                                     nsresult* pErr)
{
    NS_ASSERTION(!Interface || iid,
                 "Need the iid if you pass in an XPCNativeInterface cache.");

    *d = JSVAL_NULL;
    if (dest)
        *dest = nsnull;
    nsISupports *src = aHelper.Object();
    if (!src)
        return true;
    if (pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    
    
    
    
    
    
    
    JSContext* cx = lccx.GetJSContext();
    NS_ABORT_IF_FALSE(js::GetObjectCompartment(lccx.GetScopeForNewJSObjects()) == cx->compartment,
                      "bad scope for new JSObjects");

    JSObject *jsscope = lccx.GetScopeForNewJSObjects();
    XPCWrappedNativeScope* xpcscope =
        XPCWrappedNativeScope::FindInJSObjectScope(cx, jsscope);
    if (!xpcscope)
        return false;

    
    
    
    
    
    
    nsWrapperCache *cache = aHelper.GetWrapperCache();

    bool tryConstructSlimWrapper = false;
    JSObject *flat;
    if (cache) {
        flat = cache->GetWrapper();
        if (cache->IsProxy()) {
            XPCCallContext &ccx = lccx.GetXPCCallContext();
            if (!ccx.IsValid())
                return false;

            if (!flat) {
                bool triedToWrap;
                flat = cache->WrapObject(lccx.GetJSContext(), xpcscope,
                                         &triedToWrap);
                if (!flat && triedToWrap)
                    return false;
            }

            if (flat) {
                if (!JS_WrapObject(ccx, &flat))
                    return false;

                return CreateHolderIfNeeded(ccx, flat, d, dest);
            }
        }

        if (!dest) {
            if (!flat) {
                tryConstructSlimWrapper = true;
            } else if (IS_SLIM_WRAPPER_OBJECT(flat)) {
                if (js::GetObjectCompartment(flat) == cx->compartment) {
                    *d = OBJECT_TO_JSVAL(flat);
                    return true;
                }
            }
        }
    } else {
        flat = nsnull;
    }

    
    
    if (tryConstructSlimWrapper) {
        XPCCallContext &ccx = lccx.GetXPCCallContext();
        if (!ccx.IsValid())
            return false;

        jsval slim;
        if (ConstructSlimWrapper(ccx, aHelper, xpcscope, &slim)) {
            *d = slim;
            return true;
        }

        if (JS_IsExceptionPending(cx))
            return false;

        
        
        
        
        flat = cache->GetWrapper();
    }

    
    
    
    
    
    AutoMarkingNativeInterfacePtr iface;
    if (iid) {
        XPCCallContext &ccx = lccx.GetXPCCallContext();
        if (!ccx.IsValid())
            return false;

        iface.Init(ccx);

        if (Interface)
            iface = *Interface;

        if (!iface) {
            iface = XPCNativeInterface::GetNewOrUsed(ccx, iid);
            if (!iface)
                return false;

            if (Interface)
                *Interface = iface;
        }
    }

    NS_ASSERTION(!flat || IS_WRAPPER_CLASS(js::GetObjectClass(flat)),
                 "What kind of wrapper is this?");

    nsresult rv;
    XPCWrappedNative* wrapper;
    nsRefPtr<XPCWrappedNative> strongWrapper;
    if (!flat) {
        XPCCallContext &ccx = lccx.GetXPCCallContext();
        if (!ccx.IsValid())
            return false;

        rv = XPCWrappedNative::GetNewOrUsed(ccx, aHelper, xpcscope, iface,
                                            isGlobal,
                                            getter_AddRefs(strongWrapper));

        wrapper = strongWrapper;
    } else if (IS_WN_WRAPPER_OBJECT(flat)) {
        wrapper = static_cast<XPCWrappedNative*>(xpc_GetJSPrivate(flat));

        
        
        
        if (dest)
            strongWrapper = wrapper;
        
        
        
        if (iface)
            wrapper->FindTearOff(lccx.GetXPCCallContext(), iface, false,
                                 &rv);
        else
            rv = NS_OK;
    } else {
        NS_ASSERTION(IS_SLIM_WRAPPER(flat),
                     "What kind of wrapper is this?");

        XPCCallContext &ccx = lccx.GetXPCCallContext();
        if (!ccx.IsValid())
            return false;

        SLIM_LOG(("***** morphing from XPCConvert::NativeInterface2JSObject"
                  "(%p)\n",
                  static_cast<nsISupports*>(xpc_GetJSPrivate(flat))));

        rv = XPCWrappedNative::Morph(ccx, flat, iface, cache,
                                     getter_AddRefs(strongWrapper));
        wrapper = strongWrapper;
    }

    if (NS_FAILED(rv) && pErr)
        *pErr = rv;

    
    if (NS_FAILED(rv) || !wrapper)
        return false;

    
    
    flat = wrapper->GetFlatJSObject();
    jsval v = OBJECT_TO_JSVAL(flat);
    if (!XPCPerThreadData::IsMainThread(lccx.GetJSContext()) ||
        !allowNativeWrapper) {
        *d = v;
        if (dest)
            *dest = strongWrapper.forget().get();
        if (pErr)
            *pErr = NS_OK;
        return true;
    }

    XPCCallContext &ccx = lccx.GetXPCCallContext();
    if (!ccx.IsValid())
        return false;

    JSObject *original = flat;
    if (!JS_WrapObject(ccx, &flat))
        return false;

    
    
    
    if (original == flat) {
        if (xpc::WrapperFactory::IsLocationObject(flat)) {
            JSObject *locationWrapper = wrapper->GetWrapper();
            if (!locationWrapper) {
                locationWrapper = xpc::WrapperFactory::WrapLocationObject(cx, flat);
                if (!locationWrapper)
                    return false;

                
                
                wrapper->SetWrapper(locationWrapper);
            }

            flat = locationWrapper;
        } else if (wrapper->NeedsSOW() &&
                   !xpc::AccessCheck::isChrome(cx->compartment)) {
            JSObject *sowWrapper = wrapper->GetWrapper();
            if (!sowWrapper) {
                sowWrapper = xpc::WrapperFactory::WrapSOWObject(cx, flat);
                if (!sowWrapper)
                    return false;

                
                
                wrapper->SetWrapper(sowWrapper);
            }

            flat = sowWrapper;
        } else {
            flat = JS_ObjectToOuterObject(cx, flat);
            NS_ASSERTION(flat, "bad outer object hook!");
            NS_ASSERTION(js::GetObjectCompartment(flat) == cx->compartment,
                         "bad compartment");
        }
    }

    *d = OBJECT_TO_JSVAL(flat);

    if (dest) {
        
        if (flat == original) {
            *dest = strongWrapper.forget().get();
        } else {
            nsRefPtr<XPCJSObjectHolder> objHolder =
                XPCJSObjectHolder::newHolder(ccx, flat);
            if (!objHolder)
                return false;

            *dest = objHolder.forget().get();
        }
    }

    if (pErr)
        *pErr = NS_OK;

    return true;
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

    if (!ac.enter(cx, src)) {
       if (pErr)
           *pErr = NS_ERROR_UNEXPECTED;
       return false;
    }

    *dest = nsnull;
     if (pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS;

    nsISupports* iface;

    if (!aOuter) {
        
        
        
        

        
        
        
        JSObject* inner = nsnull;
        if (XPCWrapper::IsSecurityWrapper(src)) {
            inner = XPCWrapper::Unwrap(cx, src);
            if (!inner) {
                if (pErr)
                    *pErr = NS_ERROR_XPC_SECURITY_MANAGER_VETO;
                return false;
            }
        }

        
        XPCWrappedNative* wrappedNative =
                    XPCWrappedNative::GetWrappedNativeOfJSObject(cx,
                                                                 inner
                                                                 ? inner
                                                                 : src);
        if (wrappedNative) {
            iface = wrappedNative->GetIdentityObject();
            return NS_SUCCEEDED(iface->QueryInterface(*iid, dest));
        }
        

        
        
        
        if (JS_TypeOfValue(cx, OBJECT_TO_JSVAL(src)) == JSTYPE_XML)
            return false;

        
        if (GetISupportsFromJSObject(src, &iface)) {
            if (iface)
                return NS_SUCCEEDED(iface->QueryInterface(*iid, dest));

            return false;
        }
    }

    

    nsXPCWrappedJS* wrapper;
    nsresult rv = nsXPCWrappedJS::GetNewOrUsed(ccx, src, *iid, aOuter, &wrapper);
    if (pErr)
        *pErr = rv;
    if (NS_SUCCEEDED(rv) && wrapper) {
        
        
        
        
        rv = aOuter ? wrapper->AggregatedQueryInterface(*iid, dest) :
                      wrapper->QueryInterface(*iid, dest);
        if (pErr)
            *pErr = rv;
        NS_RELEASE(wrapper);
        return NS_SUCCEEDED(rv);
    }

    
    return false;
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
    if (errorObject) {
        if (NS_SUCCEEDED(errorObject->GetMessageMoz(getter_Copies(xmsg)))) {
            CopyUTF16toUTF8(xmsg, sxmsg);
            msg = sxmsg.get();
        }
    }
    if (!msg)
        if (!nsXPCException::NameAndFormatForNSResult(rv, nsnull, &msg) || ! msg)
            msg = "<error>";
    if (ifaceName && methodName)
        msg = sz = JS_smprintf(format, msg, ifaceName, methodName);

    nsresult res = nsXPCException::NewException(msg, rv, nsnull, data, exceptn);

    if (NS_SUCCEEDED(res) && cx && jsExceptionPtr && *exceptn) {
        nsCOMPtr<nsIXPCException> xpcEx = do_QueryInterface(*exceptn);
        if (xpcEx)
            xpcEx->StowJSVal(cx, *jsExceptionPtr);
    }

    if (sz)
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

    if (!JSVAL_IS_PRIMITIVE(s)) {
        
        JSObject* obj = JSVAL_TO_OBJECT(s);

        if (!obj) {
            NS_ERROR("when is an object not an object?");
            return NS_ERROR_FAILURE;
        }

        
        XPCWrappedNative* wrapper;
        if (nsnull != (wrapper =
                       XPCWrappedNative::GetWrappedNativeOfJSObject(cx,obj))) {
            nsISupports* supports = wrapper->GetIdentityObject();
            nsCOMPtr<nsIException> iface = do_QueryInterface(supports);
            if (iface) {
                
                nsIException* temp = iface;
                NS_ADDREF(temp);
                *exceptn = temp;
                return NS_OK;
            } else {
                
                return ConstructException(NS_ERROR_XPC_JS_THREW_NATIVE_OBJECT,
                                          nsnull, ifaceName, methodName, supports,
                                          exceptn, nsnull, nsnull);
            }
        } else {
            

            
            
            const JSErrorReport* report;
            if (nsnull != (report = JS_ErrorFromException(cx, s))) {
                JSAutoByteString message;
                JSString* str;
                if (nsnull != (str = JS_ValueToString(cx, s)))
                    message.encode(cx, str);
                return JSErrorToXPCException(ccx, message.ptr(), ifaceName,
                                             methodName, report, exceptn);
            }


            uintN ignored;
            JSBool found;

            
            if (!JS_GetPropertyAttributes(cx, obj, "message", &ignored, &found))
               return NS_ERROR_FAILURE;

            if (found && !JS_GetPropertyAttributes(cx, obj, "result", &ignored, &found))
                return NS_ERROR_FAILURE;

            if (found) {
                
                nsXPCWrappedJS* jswrapper;
                nsresult rv =
                    nsXPCWrappedJS::GetNewOrUsed(ccx, obj,
                                                 NS_GET_IID(nsIException),
                                                 nsnull, &jswrapper);
                if (NS_FAILED(rv))
                    return rv;

                *exceptn = static_cast<nsIException *>(jswrapper->GetXPTCStub());
                return NS_OK;
            }


            
            
            
            

            

            JSString* str = JS_ValueToString(cx, s);
            if (!str)
                return NS_ERROR_FAILURE;

            JSAutoByteString strBytes(cx, str);
            if (!strBytes)
                return NS_ERROR_FAILURE;

            return ConstructException(NS_ERROR_XPC_JS_THREW_JS_OBJECT,
                                      strBytes.ptr(), ifaceName, methodName,
                                      nsnull, exceptn, cx, &s);
        }
    }

    if (JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s)) {
        return ConstructException(NS_ERROR_XPC_JS_THREW_NULL,
                                  nsnull, ifaceName, methodName, nsnull,
                                  exceptn, cx, &s);
    }

    if (JSVAL_IS_NUMBER(s)) {
        
        nsresult rv;
        double number;
        JSBool isResult = false;

        if (JSVAL_IS_INT(s)) {
            rv = (nsresult) JSVAL_TO_INT(s);
            if (NS_FAILED(rv))
                isResult = true;
            else
                number = (double) JSVAL_TO_INT(s);
        } else {
            number = JSVAL_TO_DOUBLE(s);
            if (number > 0.0 &&
                number < (double)0xffffffff &&
                0.0 == fmod(number,1)) {
                rv = (nsresult) number;
                if (NS_FAILED(rv))
                    isResult = true;
            }
        }

        if (isResult)
            return ConstructException(rv, nsnull, ifaceName, methodName,
                                      nsnull, exceptn, cx, &s);
        else {
            
            
            nsISupportsDouble* data;
            nsCOMPtr<nsIComponentManager> cm;
            if (NS_FAILED(NS_GetComponentManager(getter_AddRefs(cm))) || !cm ||
                NS_FAILED(cm->CreateInstanceByContractID(NS_SUPPORTS_DOUBLE_CONTRACTID,
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
    if (str) {
        JSAutoByteString strBytes(cx, str);
        if (!!strBytes) {
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
    if (report) {
        nsAutoString bestMessage;
        if (report && report->ucmessage) {
            bestMessage = (const PRUnichar *)report->ucmessage;
        } else if (message) {
            bestMessage.AssignWithConversion(message);
        } else {
            bestMessage.AssignLiteral("JavaScript Error");
        }

        data = new nsScriptError();
        if (!data)
            return NS_ERROR_OUT_OF_MEMORY;


        data->InitWithWindowID(bestMessage.get(),
                               NS_ConvertASCIItoUTF16(report->filename).get(),
                               (const PRUnichar *)report->uclinebuf, report->lineno,
                               report->uctokenptr - report->uclinebuf, report->flags,
                               "XPConnect JavaScript",
                               nsJSUtils::GetCurrentlyRunningCodeInnerWindowID(ccx.GetJSContext()));
    }

    if (data) {
        nsCAutoString formattedMsg;
        data->ToString(formattedMsg);

        rv = ConstructException(NS_ERROR_XPC_JAVASCRIPT_ERROR_WITH_DETAILS,
                                formattedMsg.get(), ifaceName, methodName,
                                static_cast<nsIScriptError*>(data.get()),
                                exceptn, nsnull, nsnull);
    } else {
        rv = ConstructException(NS_ERROR_XPC_JAVASCRIPT_ERROR,
                                nsnull, ifaceName, methodName, nsnull,
                                exceptn, nsnull, nsnull);
    }
    return rv;
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
    if (!ccx.IsValid())
        return false;

    JSContext* cx = ccx.GetJSContext();
    NS_ABORT_IF_FALSE(js::GetObjectCompartment(lccx.GetScopeForNewJSObjects()) == cx->compartment,
                      "bad scope for new JSObjects");

    

    

    JSObject *array = JS_NewArrayObject(cx, count, nsnull);

    if (!array)
        return false;

    
    *d = OBJECT_TO_JSVAL(array);
    AUTO_MARK_JSVAL(ccx, d);

    if (pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    JSUint32 i;
    jsval current = JSVAL_NULL;
    AUTO_MARK_JSVAL(ccx, &current);

#define POPULATE(_t)                                                          \
    PR_BEGIN_MACRO                                                            \
        for (i = 0; i < count; i++) {                                         \
            if (!NativeData2JS(ccx, &current, ((_t*)*s)+i, type, iid, pErr) ||\
                !JS_SetElement(cx, array, i, &current))                       \
                goto failure;                                                 \
        }                                                                     \
    PR_END_MACRO

    

    switch (type.TagPart()) {
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
    case nsXPTType::T_BOOL          : POPULATE(bool);         break;
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

    if (pErr)
        *pErr = NS_OK;
    return true;

failure:
    return false;

#undef POPULATE
}







static JSBool
CheckTargetAndPopulate(const nsXPTType& type,
                       PRUint8 requiredType,
                       size_t typeSize,
                       JSUint32 count,
                       JSObject* tArr,
                       void** output,
                       nsresult* pErr)
{
    
    
    
    if (type.TagPart() != requiredType) {
        if (pErr)
            *pErr = NS_ERROR_XPC_BAD_CONVERT_JS;

        return false;
    }

    
    
    size_t max = PR_UINT32_MAX / typeSize;

    
    size_t byteSize = count * typeSize;
    if (count > max || !(*output = nsMemory::Alloc(byteSize))) {
        if (pErr)
            *pErr = NS_ERROR_OUT_OF_MEMORY;

        return false;
    }

    memcpy(*output, JS_GetTypedArrayData(tArr), byteSize);
    return true;
}










JSBool
XPCConvert::JSTypedArray2Native(XPCCallContext& ccx,
                                void** d,
                                JSObject* jsArray,
                                JSUint32 count,
                                const nsXPTType& type,
                                nsresult* pErr)
{
    NS_ABORT_IF_FALSE(jsArray, "bad param");
    NS_ABORT_IF_FALSE(d, "bad param");
    NS_ABORT_IF_FALSE(js_IsTypedArray(jsArray), "not a typed array");

    
    
    JSUint32 len = JS_GetTypedArrayLength(jsArray);
    if (len < count) {
        if (pErr)
            *pErr = NS_ERROR_XPC_NOT_ENOUGH_ELEMENTS_IN_ARRAY;

        return false;
    }

    void* output = nsnull;

    switch (JS_GetTypedArrayType(jsArray)) {
    case js::TypedArray::TYPE_INT8:
        if (!CheckTargetAndPopulate(nsXPTType::T_I8, type,
                                    sizeof(int8), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::TypedArray::TYPE_UINT8:
    case js::TypedArray::TYPE_UINT8_CLAMPED:
        if (!CheckTargetAndPopulate(nsXPTType::T_U8, type,
                                    sizeof(uint8), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::TypedArray::TYPE_INT16:
        if (!CheckTargetAndPopulate(nsXPTType::T_I16, type,
                                    sizeof(int16), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::TypedArray::TYPE_UINT16:
        if (!CheckTargetAndPopulate(nsXPTType::T_U16, type,
                                    sizeof(uint16), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::TypedArray::TYPE_INT32:
        if (!CheckTargetAndPopulate(nsXPTType::T_I32, type,
                                    sizeof(int32), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::TypedArray::TYPE_UINT32:
        if (!CheckTargetAndPopulate(nsXPTType::T_U32, type,
                                    sizeof(uint32), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::TypedArray::TYPE_FLOAT32:
        if (!CheckTargetAndPopulate(nsXPTType::T_FLOAT, type,
                                    sizeof(float), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::TypedArray::TYPE_FLOAT64:
        if (!CheckTargetAndPopulate(nsXPTType::T_DOUBLE, type,
                                    sizeof(double), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    
    default:
        if (pErr)
            *pErr = NS_ERROR_XPC_BAD_CONVERT_JS;

        return false;
    }

    *d = output;
    if (pErr)
        *pErr = NS_OK;

    return true;
}


JSBool
XPCConvert::JSArray2Native(XPCCallContext& ccx, void** d, jsval s,
                           JSUint32 count, const nsXPTType& type,
                           const nsID* iid, nsresult* pErr)
{
    NS_ABORT_IF_FALSE(d, "bad param");

    JSContext* cx = ccx.GetJSContext();

    
    enum CleanupMode {na, fr, re};

    CleanupMode cleanupMode;

    JSObject* jsarray = nsnull;
    void* array = nsnull;
    JSUint32 initedCount;
    jsval current;

    

    

    if (JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s)) {
        if (0 != count) {
            if (pErr)
                *pErr = NS_ERROR_XPC_NOT_ENOUGH_ELEMENTS_IN_ARRAY;
            return false;
        }

        *d = nsnull;
        return true;
    }

    if (!JSVAL_IS_OBJECT(s)) {
        if (pErr)
            *pErr = NS_ERROR_XPC_CANT_CONVERT_PRIMITIVE_TO_ARRAY;
        return false;
    }

    jsarray = JSVAL_TO_OBJECT(s);

    
    if (js_IsTypedArray(jsarray)) {
        return JSTypedArray2Native(ccx, d, jsarray, count, type, pErr);
    }

    if (!JS_IsArrayObject(cx, jsarray)) {
        if (pErr)
            *pErr = NS_ERROR_XPC_CANT_CONVERT_OBJECT_TO_ARRAY;
        return false;
    }

    JSUint32 len;
    if (!JS_GetArrayLength(cx, jsarray, &len) || len < count) {
        if (pErr)
            *pErr = NS_ERROR_XPC_NOT_ENOUGH_ELEMENTS_IN_ARRAY;
        return false;
    }

    if (pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS;

#define POPULATE(_mode, _t)                                                   \
    PR_BEGIN_MACRO                                                            \
        cleanupMode = _mode;                                                  \
        size_t max = PR_UINT32_MAX / sizeof(_t);                              \
        if (count > max ||                                                    \
            nsnull == (array = nsMemory::Alloc(count * sizeof(_t)))) {        \
            if (pErr)                                                         \
                *pErr = NS_ERROR_OUT_OF_MEMORY;                               \
            goto failure;                                                     \
        }                                                                     \
        for (initedCount = 0; initedCount < count; initedCount++) {           \
            if (!JS_GetElement(cx, jsarray, initedCount, &current) ||         \
                !JSData2Native(ccx, ((_t*)array)+initedCount, current, type,  \
                               true, iid, pErr))                              \
                goto failure;                                                 \
        }                                                                     \
    PR_END_MACRO


    

    

    switch (type.TagPart()) {
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
    case nsXPTType::T_BOOL          : POPULATE(na, bool);           break;
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
    if (pErr)
        *pErr = NS_OK;
    return true;

failure:
    
    if (array) {
        if (cleanupMode == re) {
            nsISupports** a = (nsISupports**) array;
            for (PRUint32 i = 0; i < initedCount; i++) {
                nsISupports* p = a[i];
                NS_IF_RELEASE(p);
            }
        } else if (cleanupMode == fr) {
            void** a = (void**) array;
            for (PRUint32 i = 0; i < initedCount; i++) {
                void* p = a[i];
                if (p) nsMemory::Free(p);
            }
        }
        nsMemory::Free(array);
    }

    return false;

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

    if (pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    switch (type.TagPart()) {
        case nsXPTType::T_PSTRING_SIZE_IS:
        {
            char* p = *((char**)s);
            if (!p)
                break;
            JSString* str;
            if (!(str = JS_NewStringCopyN(cx, p, count)))
                return false;
            *d = STRING_TO_JSVAL(str);
            break;
        }
        case nsXPTType::T_PWSTRING_SIZE_IS:
        {
            jschar* p = *((jschar**)s);
            if (!p)
                break;
            JSString* str;
            if (!(str = JS_NewUCStringCopyN(cx, p, count)))
                return false;
            *d = STRING_TO_JSVAL(str);
            break;
        }
        default:
            XPC_LOG_ERROR(("XPCConvert::NativeStringWithSize2JS : unsupported type"));
            return false;
    }
    return true;
}


JSBool
XPCConvert::JSStringWithSize2Native(XPCCallContext& ccx, void* d, jsval s,
                                    JSUint32 count, const nsXPTType& type,
                                    nsresult* pErr)
{
    NS_PRECONDITION(!JSVAL_IS_NULL(s), "bad param");
    NS_PRECONDITION(d, "bad param");

    JSContext* cx = ccx.GetJSContext();

    JSUint32 len;

    if (pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    switch (type.TagPart()) {
        case nsXPTType::T_PSTRING_SIZE_IS:
        {
            if (JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s)) {
                if (0 != count) {
                    if (pErr)
                        *pErr = NS_ERROR_XPC_NOT_ENOUGH_CHARS_IN_STRING;
                    return false;
                }
                if (0 != count) {
                    len = (count + 1) * sizeof(char);
                    if (!(*((void**)d) = nsMemory::Alloc(len)))
                        return false;
                    return true;
                }
                

                *((char**)d) = nsnull;
                return true;
            }

            JSString* str = JS_ValueToString(cx, s);
            if (!str) {
                return false;
            }

            size_t length = JS_GetStringEncodingLength(cx, str);
            if (length == size_t(-1)) {
                return false;
            }
            if (length > count) {
                if (pErr)
                    *pErr = NS_ERROR_XPC_NOT_ENOUGH_CHARS_IN_STRING;
                return false;
            }
            len = PRUint32(length);

            if (len < count)
                len = count;

            JSUint32 alloc_len = (len + 1) * sizeof(char);
            char *buffer = static_cast<char *>(nsMemory::Alloc(alloc_len));
            if (!buffer) {
                return false;
            }
            JS_EncodeStringToBuffer(str, buffer, len);
            buffer[len] = '\0';
            *((char**)d) = buffer;

            return true;
        }

        case nsXPTType::T_PWSTRING_SIZE_IS:
        {
            const jschar* chars=nsnull;
            JSString* str;

            if (JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s)) {
                if (0 != count) {
                    if (pErr)
                        *pErr = NS_ERROR_XPC_NOT_ENOUGH_CHARS_IN_STRING;
                    return false;
                }

                if (0 != count) {
                    len = (count + 1) * sizeof(jschar);
                    if (!(*((void**)d) = nsMemory::Alloc(len)))
                        return false;
                    return true;
                }

                
                *((const jschar**)d) = nsnull;
                return true;
            }

            if (!(str = JS_ValueToString(cx, s))) {
                return false;
            }

            len = JS_GetStringLength(str);
            if (len > count) {
                if (pErr)
                    *pErr = NS_ERROR_XPC_NOT_ENOUGH_CHARS_IN_STRING;
                return false;
            }
            if (len < count)
                len = count;

            if (!(chars = JS_GetStringCharsZ(cx, str))) {
                return false;
            }
            JSUint32 alloc_len = (len + 1) * sizeof(jschar);
            if (!(*((void**)d) = nsMemory::Alloc(alloc_len))) {
                
                return false;
            }
            memcpy(*((jschar**)d), chars, alloc_len);
            (*((jschar**)d))[count] = 0;

            return true;
        }
        default:
            XPC_LOG_ERROR(("XPCConvert::JSStringWithSize2Native : unsupported type"));
            return false;
    }
}

