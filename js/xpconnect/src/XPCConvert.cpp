







#include "mozilla/ArrayUtils.h"
#include "mozilla/Range.h"

#include "xpcprivate.h"
#include "nsIAtom.h"
#include "nsWrapperCache.h"
#include "nsJSUtils.h"
#include "WrapperFactory.h"

#include "nsWrapperCacheInlines.h"

#include "jsapi.h"
#include "jsfriendapi.h"
#include "js/CharacterEncoding.h"
#include "jsprf.h"

#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/DOMException.h"
#include "mozilla/dom/PrimitiveConversions.h"
#include "mozilla/jsipc/CrossProcessObjectWrappers.h"

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
        if (mainObj && mozilla::jsipc::IsWrappedCPOW(mainObj))
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
XPCConvert::NativeData2JS(MutableHandleValue d, const void* s,
                          const nsXPTType& type, const nsID* iid, nsresult* pErr)
{
    NS_PRECONDITION(s, "bad param");

    AutoJSContext cx;
    if (pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    switch (type.TagPart()) {
    case nsXPTType::T_I8    :
        d.setInt32(*static_cast<const int8_t*>(s));
        return true;
    case nsXPTType::T_I16   :
        d.setInt32(*static_cast<const int16_t*>(s));
        return true;
    case nsXPTType::T_I32   :
        d.setInt32(*static_cast<const int32_t*>(s));
        return true;
    case nsXPTType::T_I64   :
        d.setNumber(static_cast<double>(*static_cast<const int64_t*>(s)));
        return true;
    case nsXPTType::T_U8    :
        d.setInt32(*static_cast<const uint8_t*>(s));
        return true;
    case nsXPTType::T_U16   :
        d.setInt32(*static_cast<const uint16_t*>(s));
        return true;
    case nsXPTType::T_U32   :
        d.setNumber(*static_cast<const uint32_t*>(s));
        return true;
    case nsXPTType::T_U64   :
        d.setNumber(static_cast<double>(*static_cast<const uint64_t*>(s)));
        return true;
    case nsXPTType::T_FLOAT :
        d.setNumber(*static_cast<const float*>(s));
        return true;
    case nsXPTType::T_DOUBLE:
        d.setNumber(*static_cast<const double*>(s));
        return true;
    case nsXPTType::T_BOOL  :
        d.setBoolean(*static_cast<const bool*>(s));
        return true;
    case nsXPTType::T_CHAR  :
    {
        char p = *static_cast<const char*>(s);

#ifdef STRICT_CHECK_OF_UNICODE
        MOZ_ASSERT(! ILLEGAL_CHAR_RANGE(p) , "passing non ASCII data");
#endif 

        JSString* str = JS_NewStringCopyN(cx, &p, 1);
        if (!str)
            return false;

        d.setString(str);
        return true;
    }
    case nsXPTType::T_WCHAR :
    {
        char16_t p = *static_cast<const char16_t*>(s);

        JSString* str = JS_NewUCStringCopyN(cx, &p, 1);
        if (!str)
            return false;

        d.setString(str);
        return true;
    }

    case nsXPTType::T_JSVAL :
    {
        d.set(*static_cast<const Value*>(s));
        return JS_WrapValue(cx, d);
    }

    case nsXPTType::T_VOID:
        XPC_LOG_ERROR(("XPCConvert::NativeData2JS : void* params not supported"));
        return false;

    case nsXPTType::T_IID:
    {
        nsID* iid2 = *static_cast<nsID* const*>(s);
        if (!iid2) {
            d.setNull();
            return true;
        }

        RootedObject scope(cx, JS::CurrentGlobalOrNull(cx));
        JSObject* obj = xpc_NewIDObject(cx, scope, *iid2);
        if (!obj)
            return false;

        d.setObject(*obj);
        return true;
    }

    case nsXPTType::T_ASTRING:
        

    case nsXPTType::T_DOMSTRING:
    {
        const nsAString* p = *static_cast<const nsAString* const*>(s);
        if (!p || p->IsVoid()) {
            d.setNull();
            return true;
        }

        nsStringBuffer* buf;
        if (!XPCStringConvert::ReadableToJSVal(cx, *p, &buf, d))
            return false;
        if (buf)
            buf->AddRef();
        return true;
    }

    case nsXPTType::T_CHAR_STR:
    {
        const char* p = *static_cast<const char* const*>(s);
        if (!p) {
            d.setNull();
            return true;
        }

#ifdef STRICT_CHECK_OF_UNICODE
        bool isAscii = true;
        for (char* t = p; *t && isAscii; t++) {
          if (ILLEGAL_CHAR_RANGE(*t))
              isAscii = false;
        }
        MOZ_ASSERT(isAscii, "passing non ASCII data");
#endif 

        JSString* str = JS_NewStringCopyZ(cx, p);
        if (!str)
            return false;

        d.setString(str);
        return true;
    }

    case nsXPTType::T_WCHAR_STR:
    {
        const char16_t* p = *static_cast<const char16_t* const*>(s);
        if (!p) {
            d.setNull();
            return true;
        }

        JSString* str = JS_NewUCStringCopyZ(cx, p);
        if (!str)
            return false;

        d.setString(str);
        return true;
    }
    case nsXPTType::T_UTF8STRING:
    {
        const nsACString* utf8String = *static_cast<const nsACString* const*>(s);

        if (!utf8String || utf8String->IsVoid()) {
            d.setNull();
            return true;
        }

        if (utf8String->IsEmpty()) {
            d.set(JS_GetEmptyStringValue(cx));
            return true;
        }

        const uint32_t len = CalcUTF8ToUnicodeLength(*utf8String);
        
        
        if (!len)
            return false;

        const size_t buffer_size = (len + 1) * sizeof(char16_t);
        char16_t* buffer =
            static_cast<char16_t*>(JS_malloc(cx, buffer_size));
        if (!buffer)
            return false;

        uint32_t copied;
        if (!UTF8ToUnicodeBuffer(*utf8String, buffer, &copied) ||
            len != copied) {
            
            
            JS_free(cx, buffer);
            return false;
        }

        
        
        
        JSString* str = JS_NewUCString(cx, buffer, len);
        if (!str) {
            JS_free(cx, buffer);
            return false;
        }

        d.setString(str);
        return true;
    }
    case nsXPTType::T_CSTRING:
    {
        const nsACString* cString = *static_cast<const nsACString* const*>(s);

        if (!cString || cString->IsVoid()) {
            d.setNull();
            return true;
        }

        
        
        
        JSString* str = JS_NewStringCopyN(cx, cString->Data(),
                                          cString->Length());
        if (!str)
            return false;

        d.setString(str);
        return true;
    }

    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
    {
        nsISupports* iface = *static_cast<nsISupports* const*>(s);
        if (!iface) {
            d.setNull();
            return true;
        }

        if (iid->Equals(NS_GET_IID(nsIVariant))) {
            nsCOMPtr<nsIVariant> variant = do_QueryInterface(iface);
            if (!variant)
                return false;

            return XPCVariant::VariantDataToJS(variant,
                                               pErr, d);
        }

        xpcObjectHelper helper(iface);
        return NativeInterface2JSObject(d, nullptr, helper, iid, nullptr, true, pErr);
    }

    default:
        NS_ERROR("bad type");
        return false;
    }
    return true;
}



#ifdef DEBUG
static bool
CheckChar16InCharRange(char16_t c)
{
    if (ILLEGAL_RANGE(c)) {
        
        static const size_t MSG_BUF_SIZE = 64;
        char msg[MSG_BUF_SIZE];
        JS_snprintf(msg, MSG_BUF_SIZE, "char16_t out of char range; high bits of data lost: 0x%x", c);
        NS_WARNING(msg);
        return false;
    }

    return true;
}

template<typename CharT>
static void
CheckCharsInCharRange(const CharT* chars, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (!CheckChar16InCharRange(chars[i]))
            break;
    }
}
#endif

template<typename T>
bool ConvertToPrimitive(JSContext* cx, HandleValue v, T* retval)
{
    return ValueToPrimitive<T, eDefault>(cx, v, retval);
}


bool
XPCConvert::JSData2Native(void* d, HandleValue s,
                          const nsXPTType& type,
                          const nsID* iid,
                          nsresult* pErr)
{
    NS_PRECONDITION(d, "bad param");

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
        JSString* str = ToString(cx, s);
        if (!str) {
            return false;
        }

        char16_t ch;
        if (JS_GetStringLength(str) == 0) {
            ch = 0;
        } else {
            if (!JS_GetStringCharAt(cx, str, 0, &ch))
                return false;
        }
#ifdef DEBUG
        CheckChar16InCharRange(ch);
#endif
        *((char*)d) = char(ch);
        break;
    }
    case nsXPTType::T_WCHAR  :
    {
        JSString* str;
        if (!(str = ToString(cx, s))) {
            return false;
        }
        size_t length = JS_GetStringLength(str);
        if (length == 0) {
            *((uint16_t*)d) = 0;
            break;
        }

        char16_t ch;
        if (!JS_GetStringCharAt(cx, str, 0, &ch))
            return false;

        *((uint16_t*)d) = uint16_t(ch);
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
        if (s.isUndefined()) {
            (**((nsAString**)d)).SetIsVoid(true);
            return true;
        }
        
    }
    case nsXPTType::T_DOMSTRING:
    {
        if (s.isNull()) {
            (**((nsAString**)d)).SetIsVoid(true);
            return true;
        }
        size_t length = 0;
        JSString* str = nullptr;
        if (!s.isUndefined()) {
            str = ToString(cx, s);
            if (!str)
                return false;

            length = JS_GetStringLength(str);
            if (!length) {
                (**((nsAString**)d)).Truncate();
                return true;
            }
        }

        nsAString* ws = *((nsAString**)d);

        if (!str) {
            ws->AssignLiteral(MOZ_UTF16("undefined"));
        } else if (XPCStringConvert::IsDOMString(str)) {
            
            
            const char16_t* chars = JS_GetTwoByteExternalStringChars(str);
            nsStringBuffer::FromData((void*)chars)->ToString(length, *ws);
        } else if (XPCStringConvert::IsLiteral(str)) {
            
            
            const char16_t* chars = JS_GetTwoByteExternalStringChars(str);
            ws->AssignLiteral(chars, length);
        } else {
            if (!AssignJSString(cx, *ws, str))
                return false;
        }
        return true;
    }

    case nsXPTType::T_CHAR_STR:
    {
        if (s.isUndefined() || s.isNull()) {
            *((char**)d) = nullptr;
            return true;
        }

        JSString* str = ToString(cx, s);
        if (!str) {
            return false;
        }
#ifdef DEBUG
        if (JS_StringHasLatin1Chars(str)) {
            size_t len;
            AutoCheckCannotGC nogc;
            const Latin1Char* chars = JS_GetLatin1StringCharsAndLength(cx, nogc, str, &len);
            if (chars)
                CheckCharsInCharRange(chars, len);
        } else {
            size_t len;
            AutoCheckCannotGC nogc;
            const char16_t* chars = JS_GetTwoByteStringCharsAndLength(cx, nogc, str, &len);
            if (chars)
                CheckCharsInCharRange(chars, len);
        }
#endif 
        size_t length = JS_GetStringEncodingLength(cx, str);
        if (length == size_t(-1)) {
            return false;
        }
        char* buffer = static_cast<char*>(moz_xmalloc(length + 1));
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
        JSString* str;

        if (s.isUndefined() || s.isNull()) {
            *((char16_t**)d) = nullptr;
            return true;
        }

        if (!(str = ToString(cx, s))) {
            return false;
        }
        int len = JS_GetStringLength(str);
        int byte_len = (len+1)*sizeof(char16_t);
        if (!(*((void**)d) = moz_xmalloc(byte_len))) {
            
            return false;
        }
        mozilla::Range<char16_t> destChars(*((char16_t**)d), len + 1);
        if (!JS_CopyStringChars(cx, destChars, str))
            return false;
        destChars[len] = 0;

        return true;
    }

    case nsXPTType::T_UTF8STRING:
    {
        if (s.isNull() || s.isUndefined()) {
            nsCString* rs = *((nsCString**)d);
            rs->SetIsVoid(true);
            return true;
        }

        
        JSString* str = ToString(cx, s);
        if (!str)
            return false;

        size_t length = JS_GetStringLength(str);
        if (!length) {
            nsCString* rs = *((nsCString**)d);
            rs->Truncate();
            return true;
        }

        JSFlatString* flat = JS_FlattenString(cx, str);
        if (!flat)
            return false;

        size_t utf8Length = JS::GetDeflatedUTF8StringLength(flat);
        nsACString* rs = *((nsACString**)d);
        rs->SetLength(utf8Length);

        JS::DeflateStringToUTF8Buffer(flat, mozilla::RangedPtr<char>(rs->BeginWriting(), utf8Length));

        return true;
    }

    case nsXPTType::T_CSTRING:
    {
        if (s.isNull() || s.isUndefined()) {
            nsACString* rs = *((nsACString**)d);
            rs->Truncate();
            rs->SetIsVoid(true);
            return true;
        }

        
        JSString* str = ToString(cx, s);
        if (!str) {
            return false;
        }

        size_t length = JS_GetStringEncodingLength(cx, str);
        if (length == size_t(-1)) {
            return false;
        }

        if (!length) {
            nsCString* rs = *((nsCString**)d);
            rs->Truncate();
            return true;
        }

        nsACString* rs = *((nsACString**)d);
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
            nsCOMPtr<nsIVariant> variant = XPCVariant::newVariant(cx, s);
            if (!variant)
                return false;

            variant.forget(static_cast<nsISupports**>(d));
            return true;
        } else if (iid->Equals(NS_GET_IID(nsIAtom)) && s.isString()) {
            
            JSString* str = s.toString();
            nsAutoJSString autoStr;
            if (!autoStr.init(cx, str)) {
                if (pErr)
                    *pErr = NS_ERROR_XPC_BAD_CONVERT_JS_NULL_REF;
                return false;
            }
            nsCOMPtr<nsIAtom> atom = NS_NewAtom(autoStr);
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
CreateHolderIfNeeded(HandleObject obj, MutableHandleValue d,
                     nsIXPConnectJSObjectHolder** dest)
{
    if (dest) {
        if (!obj)
            return false;
        nsRefPtr<XPCJSObjectHolder> objHolder = new XPCJSObjectHolder(obj);
        objHolder.forget(dest);
    }

    d.setObjectOrNull(obj);

    return true;
}



bool
XPCConvert::NativeInterface2JSObject(MutableHandleValue d,
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

    d.setNull();
    if (dest)
        *dest = nullptr;
    if (!aHelper.Object())
        return true;
    if (pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    
    
    
    
    
    
    
    AutoJSContext cx;
    XPCWrappedNativeScope* xpcscope = ObjectScope(JS::CurrentGlobalOrNull(cx));
    if (!xpcscope)
        return false;

    
    
    
    
    
    
    nsWrapperCache* cache = aHelper.GetWrapperCache();

    RootedObject flat(cx, cache ? cache->GetWrapper() : nullptr);
    if (!flat && cache && cache->IsDOMBinding()) {
        RootedObject global(cx, xpcscope->GetGlobalJSObject());
        js::AssertSameCompartment(cx, global);
        flat = cache->WrapObject(cx, JS::NullPtr());
        if (!flat)
            return false;
    }
    if (flat) {
        if (allowNativeWrapper && !JS_WrapObject(cx, &flat))
            return false;
        return CreateHolderIfNeeded(flat, d, dest);
    }

    
    
    
    
    RootedObject cpow(cx, UnwrapNativeCPOW(aHelper.Object()));
    if (cpow) {
        if (!JS_WrapObject(cx, &cpow))
            return false;
        d.setObject(*cpow);
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

    nsRefPtr<XPCWrappedNative> wrapper;
    nsresult rv = XPCWrappedNative::GetNewOrUsed(aHelper, xpcscope, iface,
                                                 getter_AddRefs(wrapper));
    if (NS_FAILED(rv) && pErr)
        *pErr = rv;

    
    if (NS_FAILED(rv) || !wrapper)
        return false;

    
    
    flat = wrapper->GetFlatJSObject();
    if (!allowNativeWrapper) {
        d.setObjectOrNull(flat);
        if (dest)
            wrapper.forget(dest);
        if (pErr)
            *pErr = NS_OK;
        return true;
    }

    
    
    RootedObject original(cx, flat);
    if (!JS_WrapObject(cx, &flat))
        return false;

    d.setObjectOrNull(flat);

    if (dest) {
        
        if (flat == original) {
            wrapper.forget(dest);
        } else {
            if (!flat)
                return false;
            nsRefPtr<XPCJSObjectHolder> objHolder = new XPCJSObjectHolder(flat);
            objHolder.forget(dest);
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
    nsresult rv = nsXPCWrappedJS::GetNewOrUsed(src, *iid, &wrapper);
    if (pErr)
        *pErr = rv;
    if (NS_SUCCEEDED(rv) && wrapper) {
        
        
        
        if (aOuter)
            wrapper->SetAggregatedNativeObject(aOuter);

        
        
        
        
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

    nsCString msgStr(msg);
    if (ifaceName && methodName)
        msgStr.AppendPrintf(format, msg, ifaceName, methodName);

    nsRefPtr<Exception> e = new Exception(msgStr, rv, EmptyCString(), nullptr, data);

    if (cx && jsExceptionPtr) {
        e->StowJSVal(*jsExceptionPtr);
    }

    e.forget(exceptn);
    return NS_OK;
}



class MOZ_STACK_CLASS AutoExceptionRestorer
{
public:
    AutoExceptionRestorer(JSContext* cx, Value v)
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

    if (!s.isPrimitive()) {
        
        RootedObject obj(cx, s.toObjectOrNull());

        if (!obj) {
            NS_ERROR("when is an object not an object?");
            return NS_ERROR_FAILURE;
        }

        
        JSObject* unwrapped = js::CheckedUnwrap(obj,  false);
        if (!unwrapped)
            return NS_ERROR_XPC_SECURITY_MANAGER_VETO;
        XPCWrappedNative* wrapper = IS_WN_REFLECTOR(unwrapped) ? XPCWrappedNative::Get(unwrapped)
                                                               : nullptr;
        if (wrapper) {
            nsISupports* supports = wrapper->GetIdentityObject();
            nsCOMPtr<nsIException> iface = do_QueryInterface(supports);
            if (iface) {
                
                nsCOMPtr<nsIException> temp = iface;
                temp.forget(exceptn);
                return NS_OK;
            } else {
                
                return ConstructException(NS_ERROR_XPC_JS_THREW_NATIVE_OBJECT,
                                          nullptr, ifaceName, methodName, supports,
                                          exceptn, nullptr, nullptr);
            }
        } else {
            

            
            
            const JSErrorReport* report;
            if (nullptr != (report = JS_ErrorFromException(cx, obj))) {
                JSAutoByteString message;
                JSString* str;
                if (nullptr != (str = ToString(cx, s)))
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
                    nsXPCWrappedJS::GetNewOrUsed(obj, NS_GET_IID(nsIException), &jswrapper);
                if (NS_FAILED(rv))
                    return rv;

                *exceptn = static_cast<nsIException*>(jswrapper->GetXPTCStub());
                return NS_OK;
            }


            
            
            
            

            

            JSString* str = ToString(cx, s);
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

    if (s.isUndefined() || s.isNull()) {
        return ConstructException(NS_ERROR_XPC_JS_THREW_NULL,
                                  nullptr, ifaceName, methodName, nullptr,
                                  exceptn, cx, s.address());
    }

    if (s.isNumber()) {
        
        nsresult rv;
        double number;
        bool isResult = false;

        if (s.isInt32()) {
            rv = (nsresult) s.toInt32();
            if (NS_FAILED(rv))
                isResult = true;
            else
                number = (double) s.toInt32();
        } else {
            number = s.toDouble();
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

    
    

    JSString* str = ToString(cx, s);
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
            bestMessage = static_cast<const char16_t*>(report->ucmessage);
        } else if (message) {
            CopyASCIItoUTF16(message, bestMessage);
        } else {
            bestMessage.AssignLiteral("JavaScript Error");
        }

        const char16_t* uclinebuf =
            static_cast<const char16_t*>(report->uclinebuf);

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
XPCConvert::NativeArray2JS(MutableHandleValue d, const void** s,
                           const nsXPTType& type, const nsID* iid,
                           uint32_t count, nsresult* pErr)
{
    NS_PRECONDITION(s, "bad param");

    AutoJSContext cx;

    

    

    RootedObject array(cx, JS_NewArrayObject(cx, count));
    if (!array)
        return false;

    if (pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    uint32_t i;
    RootedValue current(cx, JSVAL_NULL);

#define POPULATE(_t)                                                                    \
    PR_BEGIN_MACRO                                                                      \
        for (i = 0; i < count; i++) {                                                   \
            if (!NativeData2JS(&current, ((_t*)*s)+i, type, iid, pErr) ||               \
                !JS_DefineElement(cx, array, i, current, JSPROP_ENUMERATE))             \
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
    case nsXPTType::T_WCHAR         : POPULATE(char16_t);       break;
    case nsXPTType::T_VOID          : NS_ERROR("bad type");     goto failure;
    case nsXPTType::T_IID           : POPULATE(nsID*);          break;
    case nsXPTType::T_DOMSTRING     : NS_ERROR("bad type");     goto failure;
    case nsXPTType::T_CHAR_STR      : POPULATE(char*);          break;
    case nsXPTType::T_WCHAR_STR     : POPULATE(char16_t*);      break;
    case nsXPTType::T_INTERFACE     : POPULATE(nsISupports*);   break;
    case nsXPTType::T_INTERFACE_IS  : POPULATE(nsISupports*);   break;
    case nsXPTType::T_UTF8STRING    : NS_ERROR("bad type");     goto failure;
    case nsXPTType::T_CSTRING       : NS_ERROR("bad type");     goto failure;
    case nsXPTType::T_ASTRING       : NS_ERROR("bad type");     goto failure;
    default                         : NS_ERROR("bad type");     goto failure;
    }

    if (pErr)
        *pErr = NS_OK;
    d.setObject(*array);
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
    if (count > max || !(*output = moz_xmalloc(byteSize))) {
        if (pErr)
            *pErr = NS_ERROR_OUT_OF_MEMORY;

        return false;
    }

    JS::AutoCheckCannotGC nogc;
    memcpy(*output, JS_GetArrayBufferViewData(tArr, nogc), byteSize);
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
    case js::Scalar::Int8:
        if (!CheckTargetAndPopulate(nsXPTType::T_I8, type,
                                    sizeof(int8_t), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::Scalar::Uint8:
    case js::Scalar::Uint8Clamped:
        if (!CheckTargetAndPopulate(nsXPTType::T_U8, type,
                                    sizeof(uint8_t), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::Scalar::Int16:
        if (!CheckTargetAndPopulate(nsXPTType::T_I16, type,
                                    sizeof(int16_t), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::Scalar::Uint16:
        if (!CheckTargetAndPopulate(nsXPTType::T_U16, type,
                                    sizeof(uint16_t), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::Scalar::Int32:
        if (!CheckTargetAndPopulate(nsXPTType::T_I32, type,
                                    sizeof(int32_t), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::Scalar::Uint32:
        if (!CheckTargetAndPopulate(nsXPTType::T_U32, type,
                                    sizeof(uint32_t), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::Scalar::Float32:
        if (!CheckTargetAndPopulate(nsXPTType::T_FLOAT, type,
                                    sizeof(float), count,
                                    jsArray, &output, pErr)) {
            return false;
        }
        break;

    case js::Scalar::Float64:
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
            nullptr == (array = moz_xmalloc(count * sizeof(_t)))) {            \
            if (pErr)                                                          \
                *pErr = NS_ERROR_OUT_OF_MEMORY;                                \
            goto failure;                                                      \
        }                                                                      \
        for (initedCount = 0; initedCount < count; initedCount++) {            \
            if (!JS_GetElement(cx, jsarray, initedCount, &current) ||          \
                !JSData2Native(((_t*)array)+initedCount, current, type,        \
                               iid, pErr))                                     \
                goto failure;                                                  \
        }                                                                      \
    PR_END_MACRO

    
    enum CleanupMode {na, fr, re};

    CleanupMode cleanupMode;

    void* array = nullptr;
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
    case nsXPTType::T_WCHAR         : POPULATE(na, char16_t);       break;
    case nsXPTType::T_VOID          : NS_ERROR("bad type");         goto failure;
    case nsXPTType::T_IID           : POPULATE(fr, nsID*);          break;
    case nsXPTType::T_DOMSTRING     : NS_ERROR("bad type");         goto failure;
    case nsXPTType::T_CHAR_STR      : POPULATE(fr, char*);          break;
    case nsXPTType::T_WCHAR_STR     : POPULATE(fr, char16_t*);      break;
    case nsXPTType::T_INTERFACE     : POPULATE(re, nsISupports*);   break;
    case nsXPTType::T_INTERFACE_IS  : POPULATE(re, nsISupports*);   break;
    case nsXPTType::T_UTF8STRING    : NS_ERROR("bad type");         goto failure;
    case nsXPTType::T_CSTRING       : NS_ERROR("bad type");         goto failure;
    case nsXPTType::T_ASTRING       : NS_ERROR("bad type");         goto failure;
    default                         : NS_ERROR("bad type");         goto failure;
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
                if (p) free(p);
            }
        }
        free(array);
    }

    return false;

#undef POPULATE
}


bool
XPCConvert::NativeStringWithSize2JS(MutableHandleValue d, const void* s,
                                    const nsXPTType& type,
                                    uint32_t count,
                                    nsresult* pErr)
{
    NS_PRECONDITION(s, "bad param");

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
            d.setString(str);
            break;
        }
        case nsXPTType::T_PWSTRING_SIZE_IS:
        {
            char16_t* p = *((char16_t**)s);
            if (!p)
                break;
            JSString* str;
            if (!(str = JS_NewUCStringCopyN(cx, p, count)))
                return false;
            d.setString(str);
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
    NS_PRECONDITION(!s.isNull(), "bad param");
    NS_PRECONDITION(d, "bad param");

    AutoJSContext cx;
    uint32_t len;

    if (pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    switch (type.TagPart()) {
        case nsXPTType::T_PSTRING_SIZE_IS:
        {
            if (s.isUndefined() || s.isNull()) {
                if (0 != count) {
                    if (pErr)
                        *pErr = NS_ERROR_XPC_NOT_ENOUGH_CHARS_IN_STRING;
                    return false;
                }
                if (0 != count) {
                    len = (count + 1) * sizeof(char);
                    if (!(*((void**)d) = moz_xmalloc(len)))
                        return false;
                    return true;
                }
                

                *((char**)d) = nullptr;
                return true;
            }

            JSString* str = ToString(cx, s);
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
            char* buffer = static_cast<char*>(moz_xmalloc(alloc_len));
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
            JSString* str;

            if (s.isUndefined() || s.isNull()) {
                if (0 != count) {
                    if (pErr)
                        *pErr = NS_ERROR_XPC_NOT_ENOUGH_CHARS_IN_STRING;
                    return false;
                }

                if (0 != count) {
                    len = (count + 1) * sizeof(char16_t);
                    if (!(*((void**)d) = moz_xmalloc(len)))
                        return false;
                    return true;
                }

                
                *((const char16_t**)d) = nullptr;
                return true;
            }

            if (!(str = ToString(cx, s))) {
                return false;
            }

            len = JS_GetStringLength(str);
            if (len > count) {
                if (pErr)
                    *pErr = NS_ERROR_XPC_NOT_ENOUGH_CHARS_IN_STRING;
                return false;
            }

            len = count;

            uint32_t alloc_len = (len + 1) * sizeof(char16_t);
            if (!(*((void**)d) = moz_xmalloc(alloc_len))) {
                
                return false;
            }
            mozilla::Range<char16_t> destChars(*((char16_t**)d), len + 1);
            if (!JS_CopyStringChars(cx, destChars, str))
                return false;
            destChars[count] = 0;

            return true;
        }
        default:
            XPC_LOG_ERROR(("XPCConvert::JSStringWithSize2Native : unsupported type"));
            return false;
    }
}
