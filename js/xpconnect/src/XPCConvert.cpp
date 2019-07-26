








#include "mozilla/Util.h"

#include "xpcprivate.h"
#include "nsIAtom.h"
#include "nsWrapperCache.h"
#include "nsJSUtils.h"
#include "WrapperFactory.h"

#include "nsWrapperCacheInlines.h"

#include "jsapi.h"
#include "jsfriendapi.h"
#include "jsprf.h"
#include "JavaScriptParent.h"

#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/DOMException.h"
#include "mozilla/dom/PrimitiveConversions.h"

using namespace xpc;
using namespace mozilla;
using namespace mozilla::dom;
using namespace JS;


#ifdef STRICT_CHECK_OF_UNICODE
#define ILLEGAL_RANGE(c) (0!=((c) & 0xFF80))
#else 
#define ILLEGAL_RANGE(c) (0!=((c) & 0xFF00))
#endif 

#define ILLEGAL_CHAR_RANGE(c) (0!=((c) & 0x80))




bool
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

static JSObject*
UnwrapNativeCPOW(nsISupports* wrapper)
{
    nsCOMPtr<nsIXPConnectWrappedJS> underware = do_QueryInterface(wrapper);
    if (underware) {
        JSObject* mainObj = underware->GetJSObject();
        if (mainObj && mozilla::jsipc::JavaScriptParent::IsCPOW(mainObj))
            return mainObj;
    }
    return nullptr;
}




bool
XPCConvert::GetISupportsFromJSObject(JSObject* obj, nsISupports** iface)
{
    const JSClass* jsclass = js::GetObjectJSClass(obj);
    MOZ_ASSERT(jsclass, "obj has no class");
    if (jsclass &&
        (jsclass->flags & JSCLASS_HAS_PRIVATE) &&
        (jsclass->flags & JSCLASS_PRIVATE_IS_NSISUPPORTS)) {
        *iface = (nsISupports*) xpc_GetJSPrivate(obj);
        return true;
    }
    *iface = UnwrapDOMObjectToISupports(obj);
    return !!*iface;
}




bool
XPCConvert::NativeData2JS(jsval* d, const void* s,
                          const nsXPTType& type, const nsID* iid, nsresult* pErr)
{
    NS_PRECONDITION(s, "bad param");
    NS_PRECONDITION(d, "bad param");

    AutoJSContext cx;
    if (pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    switch (type.TagPart()) {
    case nsXPTType::T_I8    : *d = INT_TO_JSVAL(int32_t(*((int8_t*)s)));             break;
    case nsXPTType::T_I16   : *d = INT_TO_JSVAL(int32_t(*((int16_t*)s)));            break;
    case nsXPTType::T_I32   : *d = INT_TO_JSVAL(*((int32_t*)s));                     break;
    case nsXPTType::T_I64   : *d = DOUBLE_TO_JSVAL(double(*((int64_t*)s)));          break;
    case nsXPTType::T_U8    : *d = INT_TO_JSVAL(int32_t(*((uint8_t*)s)));            break;
    case nsXPTType::T_U16   : *d = INT_TO_JSVAL(int32_t(*((uint16_t*)s)));           break;
    case nsXPTType::T_U32   : *d = UINT_TO_JSVAL(*((uint32_t*)s));                   break;
    case nsXPTType::T_U64   : *d = DOUBLE_TO_JSVAL(double(*((uint64_t*)s)));         break;
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
            MOZ_ASSERT(! ILLEGAL_CHAR_RANGE(p) , "passing non ASCII data");
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
                RootedObject scope(cx, JS::CurrentGlobalOrNull(cx));
                JSObject* obj;
                if (!(obj = xpc_NewIDObject(cx, scope, *iid2)))
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
                MOZ_ASSERT(isAscii, "passing non ASCII data");
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
                const nsACString* utf8String = *((const nsACString**)s);

                if (!utf8String || utf8String->IsVoid())
                    break;

                if (utf8String->IsEmpty()) {
                    *d = JS_GetEmptyStringValue(cx);
                    break;
                }

                const uint32_t len = CalcUTF8ToUnicodeLength(*utf8String);
                
                
                if (!len)
                    return false;

                const size_t buffer_size = (len + 1) * sizeof(PRUnichar);
                PRUnichar* buffer =
                    static_cast<PRUnichar*>(JS_malloc(cx, buffer_size));
                if (!buffer)
                    return false;

                uint32_t copied;
                if (!UTF8ToUnicodeBuffer(*utf8String, buffer, &copied) ||
                    len != copied) {
                    
                    
                    JS_free(cx, buffer);
                    return false;
                }

                
                
                
                JSString* str = JS_NewUCString(cx, (jschar*)buffer, len);
                if (!str) {
                    JS_free(cx, buffer);
                    return false;
                }

                *d = STRING_TO_JSVAL(str);
                break;
            }
        case nsXPTType::T_CSTRING:
            {
                const nsACString* cString = *((const nsACString**)s);

                if (!cString || cString->IsVoid())
                    break;

                if (cString->IsEmpty()) {
                    *d = JS_GetEmptyStringValue(cx);
                    break;
                }

                
                
                
                JSString* str = JS_NewStringCopyN(cx, cString->Data(),
                                                  cString->Length());
                if (!str)
                    return false;

                *d = STRING_TO_JSVAL(str);
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

                        return XPCVariant::VariantDataToJS(variant,
                                                           pErr, d);
                    }
                    
                    xpcObjectHelper helper(iface);
                    if (!NativeInterface2JSObject(d, nullptr, helper, iid,
                                                  nullptr, true, pErr))
                        return false;

#ifdef DEBUG
                    JSObject* jsobj = JSVAL_TO_OBJECT(*d);
                    if (jsobj && !js::GetObjectParent(jsobj))
                        MOZ_ASSERT(js::GetObjectClass(jsobj)->flags & JSCLASS_IS_GLOBAL,
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

template<typename T>
bool ConvertToPrimitive(JSContext *cx, HandleValue v, T *retval)
{
    return ValueToPrimitive<T, eDefault>(cx, v, retval);
}


bool
XPCConvert::JSData2Native(void* d, HandleValue s,
                          const nsXPTType& type,
                          bool useAllocator, const nsID* iid,
                          nsresult* pErr)
{
    NS_PRECONDITION(d, "bad param");

    bool isDOMString = true;

    AutoJSContext cx;
    if (pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS;

    switch (type.TagPart()) {
    case nsXPTType::T_I8     :
        return ConvertToPrimitive(cx, s, static_cast<int8_t*>(d));
    case nsXPTType::T_I16    :
        return ConvertToPrimitive(cx, s, static_cast<int16_t*>(d));
    case nsXPTType::T_I32    :
        return ConvertToPrimitive(cx, s, static_cast<int32_t*>(d));
    case nsXPTType::T_I64    :
        return ConvertToPrimitive(cx, s, static_cast<int64_t*>(d));
    case nsXPTType::T_U8     :
        return ConvertToPrimitive(cx, s, static_cast<uint8_t*>(d));
    case nsXPTType::T_U16    :
        return ConvertToPrimitive(cx, s, static_cast<uint16_t*>(d));
    case nsXPTType::T_U32    :
        return ConvertToPrimitive(cx, s, static_cast<uint32_t*>(d));
    case nsXPTType::T_U64    :
        return ConvertToPrimitive(cx, s, static_cast<uint64_t*>(d));
    case nsXPTType::T_FLOAT  :
        return ConvertToPrimitive(cx, s, static_cast<float*>(d));
    case nsXPTType::T_DOUBLE :
        return ConvertToPrimitive(cx, s, static_cast<double*>(d));
    case nsXPTType::T_BOOL   :
        return ConvertToPrimitive(cx, s, static_cast<bool*>(d));
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
            *((uint16_t*)d) = 0;
            break;
        }
        *((uint16_t*)d) = uint16_t(chars[0]);
        break;
    }
    case nsXPTType::T_JSVAL :
        *((jsval*)d) = s;
        break;
    case nsXPTType::T_VOID:
        XPC_LOG_ERROR(("XPCConvert::JSData2Native : void* params not supported"));
        NS_ERROR("void* params not supported");
        return false;
    case nsXPTType::T_IID:
    {
        const nsID* pid = nullptr;

        
        if (s.isNullOrUndefined()) {
            if (pErr)
                *pErr = NS_ERROR_XPC_BAD_CONVERT_JS;
            return false;
        }

        if (!s.isObject() ||
            (!(pid = xpc_JSObjectToID(cx, &s.toObject()))) ||
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

        const PRUnichar* chars = nullptr;
        JSString* str = nullptr;
        bool isNewString = false;
        uint32_t length = 0;

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

            length = (uint32_t) JS_GetStringLength(str);
            if (length) {
                chars = JS_GetStringCharsZ(cx, str);
                if (!chars)
                    return false;
                if (STRING_TO_JSVAL(str) != s)
                    isNewString = true;
            } else {
                str = nullptr;
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
                    nsXPConnect::GetRuntimeInstance()->NewStringWrapper(strChars, strLength);
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
            *((char**)d) = nullptr;
            return true;
        }

        JSString* str = JS_ValueToString(cx, s);
        if (!str) {
            return false;
        }
#ifdef DEBUG
        const jschar* chars=nullptr;
        if (nullptr != (chars = JS_GetStringCharsZ(cx, str))) {
            bool legalRange = true;
            int len = JS_GetStringLength(str);
            const jschar* t;
            int32_t i=0;
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
        JS_EncodeStringToBuffer(cx, str, buffer, length);
        buffer[length] = '\0';
        *((void**)d) = buffer;
        return true;
    }

    case nsXPTType::T_WCHAR_STR:
    {
        const jschar* chars=nullptr;
        JSString* str;

        if (JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s)) {
            *((jschar**)d) = nullptr;
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
        uint32_t length;
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

        rs->SetLength(uint32_t(length));
        if (rs->Length() != uint32_t(length)) {
            return false;
        }
        JS_EncodeStringToBuffer(cx, str, rs->BeginWriting(), length);

        return true;
    }

    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
    {
        MOZ_ASSERT(iid,"can't do interface conversions without iid");

        if (iid->Equals(NS_GET_IID(nsIVariant))) {
            XPCVariant* variant = XPCVariant::newVariant(cx, s);
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
            uint32_t length = JS_GetStringLength(str);
            nsCOMPtr<nsIAtom> atom =
                NS_NewAtom(nsDependentSubstring(chars, chars + length));
            atom.forget((nsISupports**)d);
            return true;
        }
        

        if (s.isNullOrUndefined()) {
            *((nsISupports**)d) = nullptr;
            return true;
        }

        
        if (!s.isObject()) {
            if (pErr && s.isInt32() && 0 == s.toInt32())
                *pErr = NS_ERROR_XPC_BAD_CONVERT_JS_ZERO_ISNOT_NULL;
            return false;
        }

        RootedObject src(cx, &s.toObject());
        return JSObject2NativeInterface((void**)d, src, iid, nullptr, pErr);
    }
    default:
        NS_ERROR("bad type");
        return false;
    }
    return true;
}

static inline bool
CreateHolderIfNeeded(HandleObject obj, jsval* d,
                     nsIXPConnectJSObjectHolder** dest)
{
    if (dest) {
        XPCJSObjectHolder* objHolder = XPCJSObjectHolder::newHolder(obj);
        if (!objHolder)
            return false;

        NS_ADDREF(*dest = objHolder);
    }

    *d = OBJECT_TO_JSVAL(obj);

    return true;
}



bool
XPCConvert::NativeInterface2JSObject(jsval* d,
                                     nsIXPConnectJSObjectHolder** dest,
                                     xpcObjectHelper& aHelper,
                                     const nsID* iid,
                                     XPCNativeInterface** Interface,
                                     bool allowNativeWrapper,
                                     nsresult* pErr)
{
    MOZ_ASSERT_IF(Interface, iid);
    if (!iid)
        iid = &NS_GET_IID(nsISupports);

    *d = JSVAL_NULL;
    if (dest)
        *dest = nullptr;
    if (!aHelper.Object())
        return true;
    if (pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    
    
    
    
    
    
    
    AutoJSContext cx;
    XPCWrappedNativeScope* xpcscope = GetObjectScope(JS::CurrentGlobalOrNull(cx));
    if (!xpcscope)
        return false;

    
    
    
    
    
    
    nsWrapperCache *cache = aHelper.GetWrapperCache();

    RootedObject flat(cx);
    if (cache) {
        flat = cache->GetWrapper();
        if (cache->IsDOMBinding()) {
            if (!flat) {
                JS::Rooted<JSObject*> global(cx, xpcscope->GetGlobalJSObject());
                flat = cache->WrapObject(cx, global);
                if (!flat)
                    return false;
            }

            if (allowNativeWrapper && !JS_WrapObject(cx, &flat))
                return false;

            return CreateHolderIfNeeded(flat, d, dest);
        }
    } else {
        flat = nullptr;
    }

    
    
    
    
    RootedObject cpow(cx, UnwrapNativeCPOW(aHelper.Object()));
    if (cpow) {
        if (!JS_WrapObject(cx, &cpow))
            return false;
        *d = JS::ObjectValue(*cpow);
        return true;
    }

    
    
    
    
    
    AutoMarkingNativeInterfacePtr iface(cx);
    if (iid) {
        if (Interface)
            iface = *Interface;

        if (!iface) {
            iface = XPCNativeInterface::GetNewOrUsed(iid);
            if (!iface)
                return false;

            if (Interface)
                *Interface = iface;
        }
    }

    MOZ_ASSERT(!flat || IS_WN_REFLECTOR(flat), "What kind of wrapper is this?");

    nsresult rv;
    XPCWrappedNative* wrapper;
    nsRefPtr<XPCWrappedNative> strongWrapper;
    if (!flat) {
        rv = XPCWrappedNative::GetNewOrUsed(aHelper, xpcscope, iface,
                                            getter_AddRefs(strongWrapper));

        wrapper = strongWrapper;
    } else {
        MOZ_ASSERT(IS_WN_REFLECTOR(flat));

        wrapper = XPCWrappedNative::Get(flat);

        
        
        
        if (dest)
            strongWrapper = wrapper;
        if (iface)
            wrapper->FindTearOff(iface, false, &rv);
        else
            rv = NS_OK;
    }

    if (NS_FAILED(rv) && pErr)
        *pErr = rv;

    
    if (NS_FAILED(rv) || !wrapper)
        return false;

    
    
    flat = wrapper->GetFlatJSObject();
    jsval v = OBJECT_TO_JSVAL(flat);
    if (!allowNativeWrapper) {
        *d = v;
        if (dest)
            *dest = strongWrapper.forget().get();
        if (pErr)
            *pErr = NS_OK;
        return true;
    }

    
    
    RootedObject original(cx, flat);
    if (!JS_WrapObject(cx, &flat))
        return false;

    *d = OBJECT_TO_JSVAL(flat);

    if (dest) {
        
        if (flat == original) {
            *dest = strongWrapper.forget().get();
        } else {
            nsRefPtr<XPCJSObjectHolder> objHolder =
                XPCJSObjectHolder::newHolder(flat);
            if (!objHolder)
                return false;

            *dest = objHolder.forget().get();
        }
    }

    if (pErr)
        *pErr = NS_OK;

    return true;
}




bool
XPCConvert::JSObject2NativeInterface(void** dest, HandleObject src,
                                     const nsID* iid,
                                     nsISupports* aOuter,
                                     nsresult* pErr)
{
    MOZ_ASSERT(dest, "bad param");
    MOZ_ASSERT(src, "bad param");
    MOZ_ASSERT(iid, "bad param");

    AutoJSContext cx;
    JSAutoCompartment ac(cx, src);

    *dest = nullptr;
     if (pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS;

    nsISupports* iface;

    if (!aOuter) {
        
        
        
        

        
        
        
        JSObject* inner = js::CheckedUnwrap(src,  false);

        
        
        
        
        
        if (!inner && MOZ_UNLIKELY(xpc::WrapperFactory::IsCOW(src)))
            inner = js::UncheckedUnwrap(src);
        if (!inner) {
            if (pErr)
                *pErr = NS_ERROR_XPC_SECURITY_MANAGER_VETO;
            return false;
        }

        
        XPCWrappedNative* wrappedNative = nullptr;
        if (IS_WN_REFLECTOR(inner))
            wrappedNative = XPCWrappedNative::Get(inner);
        if (wrappedNative) {
            iface = wrappedNative->GetIdentityObject();
            return NS_SUCCEEDED(iface->QueryInterface(*iid, dest));
        }
        

        
        if (GetISupportsFromJSObject(inner ? inner : src, &iface)) {
            if (iface)
                return NS_SUCCEEDED(iface->QueryInterface(*iid, dest));

            return false;
        }
    }

    

    nsXPCWrappedJS* wrapper;
    nsresult rv = nsXPCWrappedJS::GetNewOrUsed(src, *iid, aOuter, &wrapper);
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
    MOZ_ASSERT(!cx == !jsExceptionPtr, "Expected cx and jsExceptionPtr to cooccur.");

    static const char format[] = "\'%s\' when calling method: [%s::%s]";
    const char * msg = message;
    char* sz = nullptr;
    nsXPIDLString xmsg;
    nsAutoCString sxmsg;

    nsCOMPtr<nsIScriptError> errorObject = do_QueryInterface(data);
    if (errorObject) {
        if (NS_SUCCEEDED(errorObject->GetMessageMoz(getter_Copies(xmsg)))) {
            CopyUTF16toUTF8(xmsg, sxmsg);
            msg = sxmsg.get();
        }
    }
    if (!msg)
        if (!nsXPCException::NameAndFormatForNSResult(rv, nullptr, &msg) || ! msg)
            msg = "<error>";
    if (ifaceName && methodName)
        msg = sz = JS_smprintf(format, msg, ifaceName, methodName);

    nsRefPtr<Exception> e = new Exception(msg, rv, nullptr, nullptr, data);

    if (cx && jsExceptionPtr) {
        e->StowJSVal(*jsExceptionPtr);
    }

    e.forget(exceptn);

    if (sz)
        JS_smprintf_free(sz);
    return NS_OK;
}



class MOZ_STACK_CLASS AutoExceptionRestorer
{
public:
    AutoExceptionRestorer(JSContext *cx, Value v)
        : mContext(cx), tvr(cx, v)
    {
        JS_ClearPendingException(mContext);
    }

    ~AutoExceptionRestorer()
    {
        JS_SetPendingException(mContext, tvr);
    }

private:
    JSContext * const mContext;
    RootedValue tvr;
};


nsresult
XPCConvert::JSValToXPCException(MutableHandleValue s,
                                const char* ifaceName,
                                const char* methodName,
                                nsIException** exceptn)
{
    AutoJSContext cx;
    AutoExceptionRestorer aer(cx, s);

    if (!JSVAL_IS_PRIMITIVE(s)) {
        
        RootedObject obj(cx, JSVAL_TO_OBJECT(s));

        if (!obj) {
            NS_ERROR("when is an object not an object?");
            return NS_ERROR_FAILURE;
        }

        
        JSObject *unwrapped = js::CheckedUnwrap(obj,  false);
        if (!unwrapped)
            return NS_ERROR_XPC_SECURITY_MANAGER_VETO;
        XPCWrappedNative* wrapper = IS_WN_REFLECTOR(unwrapped) ? XPCWrappedNative::Get(unwrapped)
                                                               : nullptr;
        if (wrapper) {
            nsISupports* supports = wrapper->GetIdentityObject();
            nsCOMPtr<nsIException> iface = do_QueryInterface(supports);
            if (iface) {
                
                nsIException* temp = iface;
                NS_ADDREF(temp);
                *exceptn = temp;
                return NS_OK;
            } else {
                
                return ConstructException(NS_ERROR_XPC_JS_THREW_NATIVE_OBJECT,
                                          nullptr, ifaceName, methodName, supports,
                                          exceptn, nullptr, nullptr);
            }
        } else {
            

            
            
            const JSErrorReport* report;
            if (nullptr != (report = JS_ErrorFromException(cx, s))) {
                JSAutoByteString message;
                JSString* str;
                if (nullptr != (str = JS_ValueToString(cx, s)))
                    message.encodeLatin1(cx, str);
                return JSErrorToXPCException(message.ptr(), ifaceName,
                                             methodName, report, exceptn);
            }


            bool found;

            
            if (!JS_HasProperty(cx, obj, "message", &found))
                return NS_ERROR_FAILURE;

            if (found && !JS_HasProperty(cx, obj, "result", &found))
                return NS_ERROR_FAILURE;

            if (found) {
                
                nsXPCWrappedJS* jswrapper;
                nsresult rv =
                    nsXPCWrappedJS::GetNewOrUsed(obj, NS_GET_IID(nsIException),
                                                 nullptr, &jswrapper);
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
                                      nullptr, exceptn, cx, s.address());
        }
    }

    if (JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s)) {
        return ConstructException(NS_ERROR_XPC_JS_THREW_NULL,
                                  nullptr, ifaceName, methodName, nullptr,
                                  exceptn, cx, s.address());
    }

    if (JSVAL_IS_NUMBER(s)) {
        
        nsresult rv;
        double number;
        bool isResult = false;

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
                
                
                
                rv = (nsresult)(uint32_t) number;
                if (NS_FAILED(rv))
                    isResult = true;
            }
        }

        if (isResult)
            return ConstructException(rv, nullptr, ifaceName, methodName,
                                      nullptr, exceptn, cx, s.address());
        else {
            
            
            nsISupportsDouble* data;
            nsCOMPtr<nsIComponentManager> cm;
            if (NS_FAILED(NS_GetComponentManager(getter_AddRefs(cm))) || !cm ||
                NS_FAILED(cm->CreateInstanceByContractID(NS_SUPPORTS_DOUBLE_CONTRACTID,
                                                         nullptr,
                                                         NS_GET_IID(nsISupportsDouble),
                                                         (void**)&data)))
                return NS_ERROR_FAILURE;
            data->SetData(number);
            rv = ConstructException(NS_ERROR_XPC_JS_THREW_NUMBER, nullptr,
                                    ifaceName, methodName, data, exceptn, cx, s.address());
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
                                      nullptr, exceptn, cx, s.address());
        }
    }
    return NS_ERROR_FAILURE;
}




nsresult
XPCConvert::JSErrorToXPCException(const char* message,
                                  const char* ifaceName,
                                  const char* methodName,
                                  const JSErrorReport* report,
                                  nsIException** exceptn)
{
    AutoJSContext cx;
    nsresult rv = NS_ERROR_FAILURE;
    nsRefPtr<nsScriptError> data;
    if (report) {
        nsAutoString bestMessage;
        if (report && report->ucmessage) {
            bestMessage = static_cast<const PRUnichar*>(report->ucmessage);
        } else if (message) {
            CopyASCIItoUTF16(message, bestMessage);
        } else {
            bestMessage.AssignLiteral("JavaScript Error");
        }

        const PRUnichar* uclinebuf =
            static_cast<const PRUnichar*>(report->uclinebuf);

        data = new nsScriptError();
        data->InitWithWindowID(
            bestMessage,
            NS_ConvertASCIItoUTF16(report->filename),
            uclinebuf ? nsDependentString(uclinebuf) : EmptyString(),
            report->lineno,
            report->uctokenptr - report->uclinebuf, report->flags,
            NS_LITERAL_CSTRING("XPConnect JavaScript"),
            nsJSUtils::GetCurrentlyRunningCodeInnerWindowID(cx));
    }

    if (data) {
        nsAutoCString formattedMsg;
        data->ToString(formattedMsg);

        rv = ConstructException(NS_ERROR_XPC_JAVASCRIPT_ERROR_WITH_DETAILS,
                                formattedMsg.get(), ifaceName, methodName,
                                static_cast<nsIScriptError*>(data.get()),
                                exceptn, nullptr, nullptr);
    } else {
        rv = ConstructException(NS_ERROR_XPC_JAVASCRIPT_ERROR,
                                nullptr, ifaceName, methodName, nullptr,
                                exceptn, nullptr, nullptr);
    }
    return rv;
}





#ifdef POPULATE
#undef POPULATE
#endif


bool
XPCConvert::NativeArray2JS(jsval* d, const void** s,
                           const nsXPTType& type, const nsID* iid,
                           uint32_t count, nsresult* pErr)
{
    NS_PRECONDITION(s, "bad param");
    NS_PRECONDITION(d, "bad param");

    AutoJSContext cx;

    

    

    RootedObject array(cx, JS_NewArrayObject(cx, count, nullptr));
    if (!array)
        return false;

    if (pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    uint32_t i;
    RootedValue current(cx, JSVAL_NULL);

#define POPULATE(_t)                                                                    \
    PR_BEGIN_MACRO                                                                      \
        for (i = 0; i < count; i++) {                                                   \
            if (!NativeData2JS(current.address(), ((_t*)*s)+i, type, iid, pErr) ||      \
                !JS_SetElement(cx, array, i, &current))                                 \
                goto failure;                                                           \
        }                                                                               \
    PR_END_MACRO

    

    switch (type.TagPart()) {
    case nsXPTType::T_I8            : POPULATE(int8_t);         break;
    case nsXPTType::T_I16           : POPULATE(int16_t);        break;
    case nsXPTType::T_I32           : POPULATE(int32_t);        break;
    case nsXPTType::T_I64           : POPULATE(int64_t);        break;
    case nsXPTType::T_U8            : POPULATE(uint8_t);        break;
    case nsXPTType::T_U16           : POPULATE(uint16_t);       break;
    case nsXPTType::T_U32           : POPULATE(uint32_t);       break;
    case nsXPTType::T_U64           : POPULATE(uint64_t);       break;
    case nsXPTType::T_FLOAT         : POPULATE(float);          break;
    case nsXPTType::T_DOUBLE        : POPULATE(double);         break;
    case nsXPTType::T_BOOL          : POPULATE(bool);           break;
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
    *d = OBJECT_TO_JSVAL(array);
    return true;

failure:
    return false;

#undef POPULATE
}







static bool
CheckTargetAndPopulate(const nsXPTType& type,
                       uint8_t requiredType,
                       size_t typeSize,
                       uint32_t count,
                       JSObject* tArr,
                       void** output,
                       nsresult* pErr)
{
    
    
    
    if (type.TagPart() != requiredType) {
        if (pErr)
            *pErr = NS_ERROR_XPC_BAD_CONVERT_JS;

        return false;
    }

    
    
    size_t max = UINT32_MAX / typeSize;

    
    size_t byteSize = count * typeSize;
    if (count > max || !(*output = nsMemory::Alloc(byteSize))) {
        if (pErr)
            *pErr = NS_ERROR_OUT_OF_MEMORY;

        return false;
    }

    memcpy(*output, JS_GetArrayBufferViewData(tArr), byteSize);
    return true;
}











bool
XPCConvert::JSTypedArray2Native(void** d,
                                JSObject* jsArray,
                                uint32_t count,
                                const nsXPTType& type,
                                nsresult* pErr)
{
    MOZ_ASSERT(jsArray, "bad param");
    MOZ_ASSERT(d, "bad param");
    MOZ_ASSERT(JS_IsTypedArrayObject(jsArray), "not a typed array");

    
    
    uint32_t len = JS_GetTypedArrayLength(jsArray);
    if (len < count) {
        if (pErr)
            *pErr = NS_ERROR_XPC_NOT_ENOUGH_ELEMENTS_IN_ARRAY;

        return false;
    }

    void* output = nullptr;

    switch (JS_GetArrayBufferViewType(jsArray)) {
    case js::ArrayBufferView::TYPE_INT8:
        if (!CheckTargetAndPopulate(nsXPTType::T_I8, type,
                                    sizeof(int8_t), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::ArrayBufferView::TYPE_UINT8:
    case js::ArrayBufferView::TYPE_UINT8_CLAMPED:
        if (!CheckTargetAndPopulate(nsXPTType::T_U8, type,
                                    sizeof(uint8_t), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::ArrayBufferView::TYPE_INT16:
        if (!CheckTargetAndPopulate(nsXPTType::T_I16, type,
                                    sizeof(int16_t), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::ArrayBufferView::TYPE_UINT16:
        if (!CheckTargetAndPopulate(nsXPTType::T_U16, type,
                                    sizeof(uint16_t), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::ArrayBufferView::TYPE_INT32:
        if (!CheckTargetAndPopulate(nsXPTType::T_I32, type,
                                    sizeof(int32_t), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::ArrayBufferView::TYPE_UINT32:
        if (!CheckTargetAndPopulate(nsXPTType::T_U32, type,
                                    sizeof(uint32_t), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::ArrayBufferView::TYPE_FLOAT32:
        if (!CheckTargetAndPopulate(nsXPTType::T_FLOAT, type,
                                    sizeof(float), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::ArrayBufferView::TYPE_FLOAT64:
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


bool
XPCConvert::JSArray2Native(void** d, HandleValue s,
                           uint32_t count, const nsXPTType& type,
                           const nsID* iid, nsresult* pErr)
{
    MOZ_ASSERT(d, "bad param");

    AutoJSContext cx;

    

    

    if (s.isNullOrUndefined()) {
        if (0 != count) {
            if (pErr)
                *pErr = NS_ERROR_XPC_NOT_ENOUGH_ELEMENTS_IN_ARRAY;
            return false;
        }

        *d = nullptr;
        return true;
    }

    if (!s.isObject()) {
        if (pErr)
            *pErr = NS_ERROR_XPC_CANT_CONVERT_PRIMITIVE_TO_ARRAY;
        return false;
    }

    RootedObject jsarray(cx, &s.toObject());

    
    if (JS_IsTypedArrayObject(jsarray)) {
        return JSTypedArray2Native(d, jsarray, count, type, pErr);
    }

    if (!JS_IsArrayObject(cx, jsarray)) {
        if (pErr)
            *pErr = NS_ERROR_XPC_CANT_CONVERT_OBJECT_TO_ARRAY;
        return false;
    }

    uint32_t len;
    if (!JS_GetArrayLength(cx, jsarray, &len) || len < count) {
        if (pErr)
            *pErr = NS_ERROR_XPC_NOT_ENOUGH_ELEMENTS_IN_ARRAY;
        return false;
    }

    if (pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS;

#define POPULATE(_mode, _t)                                                    \
    PR_BEGIN_MACRO                                                             \
        cleanupMode = _mode;                                                   \
        size_t max = UINT32_MAX / sizeof(_t);                                  \
        if (count > max ||                                                     \
            nullptr == (array = nsMemory::Alloc(count * sizeof(_t)))) {        \
            if (pErr)                                                          \
                *pErr = NS_ERROR_OUT_OF_MEMORY;                                \
            goto failure;                                                      \
        }                                                                      \
        for (initedCount = 0; initedCount < count; initedCount++) {            \
            if (!JS_GetElement(cx, jsarray, initedCount, &current) ||          \
                !JSData2Native(((_t*)array)+initedCount, current, type,        \
                               true, iid, pErr))                               \
                goto failure;                                                  \
        }                                                                      \
    PR_END_MACRO

    
    enum CleanupMode {na, fr, re};

    CleanupMode cleanupMode;

    void *array = nullptr;
    uint32_t initedCount;
    RootedValue current(cx);

    
    

    switch (type.TagPart()) {
    case nsXPTType::T_I8            : POPULATE(na, int8_t);         break;
    case nsXPTType::T_I16           : POPULATE(na, int16_t);        break;
    case nsXPTType::T_I32           : POPULATE(na, int32_t);        break;
    case nsXPTType::T_I64           : POPULATE(na, int64_t);        break;
    case nsXPTType::T_U8            : POPULATE(na, uint8_t);        break;
    case nsXPTType::T_U16           : POPULATE(na, uint16_t);       break;
    case nsXPTType::T_U32           : POPULATE(na, uint32_t);       break;
    case nsXPTType::T_U64           : POPULATE(na, uint64_t);       break;
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
            for (uint32_t i = 0; i < initedCount; i++) {
                nsISupports* p = a[i];
                NS_IF_RELEASE(p);
            }
        } else if (cleanupMode == fr) {
            void** a = (void**) array;
            for (uint32_t i = 0; i < initedCount; i++) {
                void* p = a[i];
                if (p) nsMemory::Free(p);
            }
        }
        nsMemory::Free(array);
    }

    return false;

#undef POPULATE
}


bool
XPCConvert::NativeStringWithSize2JS(jsval* d, const void* s,
                                    const nsXPTType& type,
                                    uint32_t count,
                                    nsresult* pErr)
{
    NS_PRECONDITION(s, "bad param");
    NS_PRECONDITION(d, "bad param");

    AutoJSContext cx;
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


bool
XPCConvert::JSStringWithSize2Native(void* d, HandleValue s,
                                    uint32_t count, const nsXPTType& type,
                                    nsresult* pErr)
{
    NS_PRECONDITION(!JSVAL_IS_NULL(s), "bad param");
    NS_PRECONDITION(d, "bad param");

    AutoJSContext cx;
    uint32_t len;

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
                

                *((char**)d) = nullptr;
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
            len = uint32_t(length);

            if (len < count)
                len = count;

            uint32_t alloc_len = (len + 1) * sizeof(char);
            char *buffer = static_cast<char *>(nsMemory::Alloc(alloc_len));
            if (!buffer) {
                return false;
            }
            JS_EncodeStringToBuffer(cx, str, buffer, len);
            buffer[len] = '\0';
            *((char**)d) = buffer;

            return true;
        }

        case nsXPTType::T_PWSTRING_SIZE_IS:
        {
            const jschar* chars=nullptr;
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

                
                *((const jschar**)d) = nullptr;
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
            uint32_t alloc_len = (len + 1) * sizeof(jschar);
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

