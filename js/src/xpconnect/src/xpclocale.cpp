







































#include "prinit.h"
#include "plstr.h"

#include "jsapi.h"

#include "nsCollationCID.h"
#include "nsDOMClassInfo.h"
#include "nsJSUtils.h"
#include "nsICharsetConverterManager.h"
#include "nsIPlatformCharset.h"
#include "nsILocaleService.h"
#include "nsICollation.h"
#include "nsIServiceManager.h"
#include "nsUnicharUtils.h"















struct XPCLocaleCallbacks : public JSLocaleCallbacks
{
  








  static XPCLocaleCallbacks*
  MaybeThis(JSContext* cx)
  {
    JSLocaleCallbacks* lc = JS_GetLocaleCallbacks(cx);
    return (lc &&
            lc->localeToUpperCase == LocaleToUpperCase &&
            lc->localeToLowerCase == LocaleToLowerCase &&
            lc->localeCompare == LocaleCompare &&
            lc->localeToUnicode == LocaleToUnicode) ? This(cx) : nsnull;
  }

  static JSBool
  ChangeCase(JSContext* cx, JSString* src, jsval* rval,
             void(*changeCaseFnc)(const nsAString&, nsAString&))
  {
    nsDependentJSString depStr;
    if (!depStr.init(cx, src)) {
      return JS_FALSE;
    }

    nsAutoString result;
    changeCaseFnc(depStr, result);

    JSString *ucstr =
      JS_NewUCStringCopyN(cx, (jschar*)result.get(), result.Length());
    if (!ucstr) {
      return JS_FALSE;
    }

    *rval = STRING_TO_JSVAL(ucstr);

    return JS_TRUE;
  }

  static JSBool
  LocaleToUpperCase(JSContext *cx, JSString *src, jsval *rval)
  {
    return ChangeCase(cx, src, rval, ToUpperCase);
  }

  static JSBool
  LocaleToLowerCase(JSContext *cx, JSString *src, jsval *rval)
  {
    return ChangeCase(cx, src, rval, ToLowerCase);
  }

  








  static XPCLocaleCallbacks*
  This(JSContext* cx)
  {
    XPCLocaleCallbacks* ths =
      static_cast<XPCLocaleCallbacks*>(JS_GetLocaleCallbacks(cx));
    ths->AssertThreadSafety();
    return ths;
  }

  static JSBool
  LocaleToUnicode(JSContext* cx, const char* src, jsval* rval)
  {
    return This(cx)->ToUnicode(cx, src, rval);
  }

  static JSBool
  LocaleCompare(JSContext *cx, JSString *src1, JSString *src2, jsval *rval)
  {
    return This(cx)->Compare(cx, src1, src2, rval);
  }

  XPCLocaleCallbacks()
#ifdef DEBUG
    : mThread(nsnull)
#endif
  {
    MOZ_COUNT_CTOR(XPCLocaleCallbacks);

    localeToUpperCase = LocaleToUpperCase;
    localeToLowerCase = LocaleToLowerCase;
    localeCompare = LocaleCompare;
    localeToUnicode = LocaleToUnicode;
    localeGetErrorMessage = nsnull;
  }

  ~XPCLocaleCallbacks()
  {
    MOZ_COUNT_DTOR(XPCLocaleCallbacks);
    AssertThreadSafety();
  }

  JSBool
  ToUnicode(JSContext* cx, const char* src, jsval* rval)
  {
    nsresult rv;

    if (!mDecoder) {
      
      nsCOMPtr<nsILocaleService> localeService =
        do_GetService(NS_LOCALESERVICE_CONTRACTID, &rv);
      if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsILocale> appLocale;
        rv = localeService->GetApplicationLocale(getter_AddRefs(appLocale));
        if (NS_SUCCEEDED(rv)) {
          nsAutoString localeStr;
          rv = appLocale->
               GetCategory(NS_LITERAL_STRING(NSILOCALE_TIME), localeStr);
          NS_ASSERTION(NS_SUCCEEDED(rv), "failed to get app locale info");

          nsCOMPtr<nsIPlatformCharset> platformCharset =
            do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);

          if (NS_SUCCEEDED(rv)) {
            nsCAutoString charset;
            rv = platformCharset->GetDefaultCharsetForLocale(localeStr, charset);
            if (NS_SUCCEEDED(rv)) {
              
              nsCOMPtr<nsICharsetConverterManager> ccm =
                do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
              if (NS_SUCCEEDED(rv))
                ccm->GetUnicodeDecoder(charset.get(), getter_AddRefs(mDecoder));
            }
          }
        }
      }
    }

    JSString *str = nsnull;
    PRInt32 srcLength = PL_strlen(src);

    if (mDecoder) {
      PRInt32 unicharLength = srcLength;
      PRUnichar *unichars =
        (PRUnichar *)JS_malloc(cx, (srcLength + 1) * sizeof(PRUnichar));
      if (unichars) {
        rv = mDecoder->Convert(src, &srcLength, unichars, &unicharLength);
        if (NS_SUCCEEDED(rv)) {
          
          unichars[unicharLength] = 0;

          
          if (unicharLength + 1 < srcLength + 1) {
            PRUnichar *shrunkUnichars =
              (PRUnichar *)JS_realloc(cx, unichars,
                                      (unicharLength + 1) * sizeof(PRUnichar));
            if (shrunkUnichars)
              unichars = shrunkUnichars;
          }
          str = JS_NewUCString(cx,
                               reinterpret_cast<jschar*>(unichars),
                               unicharLength);
        }
        if (!str)
          JS_free(cx, unichars);
      }
    }

    if (!str) {
      nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_OUT_OF_MEMORY);
      return JS_FALSE;
    }

    *rval = STRING_TO_JSVAL(str);
    return JS_TRUE;
  }

  JSBool
  Compare(JSContext *cx, JSString *src1, JSString *src2, jsval *rval)
  {
    nsresult rv;

    if (!mCollation) {
      nsCOMPtr<nsILocaleService> localeService =
        do_GetService(NS_LOCALESERVICE_CONTRACTID, &rv);

      if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsILocale> locale;
        rv = localeService->GetApplicationLocale(getter_AddRefs(locale));

        if (NS_SUCCEEDED(rv)) {
          nsCOMPtr<nsICollationFactory> colFactory =
            do_CreateInstance(NS_COLLATIONFACTORY_CONTRACTID, &rv);

          if (NS_SUCCEEDED(rv)) {
            rv = colFactory->CreateCollation(locale, getter_AddRefs(mCollation));
          }
        }
      }

      if (NS_FAILED(rv)) {
        nsDOMClassInfo::ThrowJSException(cx, rv);

        return JS_FALSE;
      }
    }

    nsDependentJSString depStr1, depStr2;
    if (!depStr1.init(cx, src1) || !depStr2.init(cx, src2)) {
      return JS_FALSE;
    }

    PRInt32 result;
    rv = mCollation->CompareString(nsICollation::kCollationStrengthDefault,
                                   depStr1, depStr2, &result);

    if (NS_FAILED(rv)) {
      nsDOMClassInfo::ThrowJSException(cx, rv);

      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(result);

    return JS_TRUE;
  }

  nsCOMPtr<nsICollation> mCollation;
  nsCOMPtr<nsIUnicodeDecoder> mDecoder;

#ifdef DEBUG
  PRThread* mThread;

  
  
  
  void AssertThreadSafety()
  {
    NS_ABORT_IF_FALSE(!mThread || mThread == PR_GetCurrentThread(),
                      "XPCLocaleCallbacks used unsafely!");
    if (!mThread) {
      mThread = PR_GetCurrentThread();
    }
  }
#else
    void AssertThreadSafety() { }
#endif  
};












static PRCallOnceType sHookRuntime;
static JSContextCallback sOldContextCallback;
#ifdef DEBUG
static JSRuntime* sHookedRuntime;
#endif  

static JSBool
DelocalizeContextCallback(JSContext *cx, uintN contextOp)
{
  NS_ABORT_IF_FALSE(JS_GetRuntime(cx) == sHookedRuntime, "unknown runtime!");

  JSBool ok = JS_TRUE;
  if (sOldContextCallback && !sOldContextCallback(cx, contextOp)) {
    ok = JS_FALSE;
    
    
  }

  if (contextOp == JSCONTEXT_DESTROY) {
    if (XPCLocaleCallbacks* lc = XPCLocaleCallbacks::MaybeThis(cx)) {
      
      JS_SetLocaleCallbacks(cx, nsnull);
      delete lc;
    }
  }

  return ok;
}

static PRStatus
HookRuntime(void* arg)
{
  JSRuntime* rt = static_cast<JSRuntime*>(arg);

  NS_ABORT_IF_FALSE(!sHookedRuntime && !sOldContextCallback,
                    "PRCallOnce called twice?");

  
  
  
  sOldContextCallback = JS_SetContextCallback(rt, DelocalizeContextCallback);
#ifdef DEBUG
  sHookedRuntime = rt;
#endif

  return PR_SUCCESS;
}

NS_EXPORT_(void)
xpc_LocalizeContext(JSContext *cx)
{
  JSRuntime* rt = JS_GetRuntime(cx);
  PR_CallOnceWithArg(&sHookRuntime, HookRuntime, rt);

  NS_ABORT_IF_FALSE(sHookedRuntime == rt, "created multiple JSRuntimes?");

  JS_SetLocaleCallbacks(cx, new XPCLocaleCallbacks());
}
