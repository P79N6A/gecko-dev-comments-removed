






#include "mozilla/Assertions.h"

#include "plstr.h"

#include "jsapi.h"

#include "nsCollationCID.h"
#include "nsJSUtils.h"
#include "nsICharsetConverterManager.h"
#include "nsIPlatformCharset.h"
#include "nsILocaleService.h"
#include "nsICollation.h"
#include "nsIServiceManager.h"
#include "nsUnicharUtils.h"

#include "xpcpublic.h"

using namespace JS;







struct XPCLocaleCallbacks : public JSLocaleCallbacks
{
  XPCLocaleCallbacks()
#ifdef DEBUG
    : mThread(PR_GetCurrentThread())
#endif
  {
    MOZ_COUNT_CTOR(XPCLocaleCallbacks);

    localeToUpperCase = LocaleToUpperCase;
    localeToLowerCase = LocaleToLowerCase;
    localeCompare = LocaleCompare;
    localeToUnicode = LocaleToUnicode;
    localeGetErrorMessage = nullptr;
  }

  ~XPCLocaleCallbacks()
  {
    AssertThreadSafety();
    MOZ_COUNT_DTOR(XPCLocaleCallbacks);
  }

  



  static XPCLocaleCallbacks*
  This(JSRuntime *rt)
  {
    
    
    JSLocaleCallbacks* lc = JS_GetLocaleCallbacks(rt);
    MOZ_ASSERT(lc);
    MOZ_ASSERT(lc->localeToUpperCase == LocaleToUpperCase);
    MOZ_ASSERT(lc->localeToLowerCase == LocaleToLowerCase);
    MOZ_ASSERT(lc->localeCompare == LocaleCompare);
    MOZ_ASSERT(lc->localeToUnicode == LocaleToUnicode);

    XPCLocaleCallbacks* ths = static_cast<XPCLocaleCallbacks*>(lc);
    ths->AssertThreadSafety();
    return ths;
  }

  static bool
  LocaleToUpperCase(JSContext *cx, HandleString src, MutableHandleValue rval)
  {
    return ChangeCase(cx, src, rval, ToUpperCase);
  }

  static bool
  LocaleToLowerCase(JSContext *cx, HandleString src, MutableHandleValue rval)
  {
    return ChangeCase(cx, src, rval, ToLowerCase);
  }

  static bool
  LocaleToUnicode(JSContext* cx, const char* src, MutableHandleValue rval)
  {
    return This(JS_GetRuntime(cx))->ToUnicode(cx, src, rval);
  }

  static bool
  LocaleCompare(JSContext *cx, HandleString src1, HandleString src2, MutableHandleValue rval)
  {
    return This(JS_GetRuntime(cx))->Compare(cx, src1, src2, rval);
  }

private:
  static bool
  ChangeCase(JSContext* cx, HandleString src, MutableHandleValue rval,
             void(*changeCaseFnc)(const nsAString&, nsAString&))
  {
    nsDependentJSString depStr;
    if (!depStr.init(cx, src)) {
      return false;
    }

    nsAutoString result;
    changeCaseFnc(depStr, result);

    JSString *ucstr =
      JS_NewUCStringCopyN(cx, (jschar*)result.get(), result.Length());
    if (!ucstr) {
      return false;
    }

    rval.set(STRING_TO_JSVAL(ucstr));
    return true;
  }

  bool
  Compare(JSContext *cx, HandleString src1, HandleString src2, MutableHandleValue rval)
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
        xpc::Throw(cx, rv);
        return false;
      }
    }

    nsDependentJSString depStr1, depStr2;
    if (!depStr1.init(cx, src1) || !depStr2.init(cx, src2)) {
      return false;
    }

    int32_t result;
    rv = mCollation->CompareString(nsICollation::kCollationStrengthDefault,
                                   depStr1, depStr2, &result);

    if (NS_FAILED(rv)) {
      xpc::Throw(cx, rv);
      return false;
    }

    rval.set(INT_TO_JSVAL(result));
    return true;
  }

  bool
  ToUnicode(JSContext* cx, const char* src, MutableHandleValue rval)
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
          MOZ_ASSERT(NS_SUCCEEDED(rv), "failed to get app locale info");

          nsCOMPtr<nsIPlatformCharset> platformCharset =
            do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);

          if (NS_SUCCEEDED(rv)) {
            nsAutoCString charset;
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

    int32_t srcLength = strlen(src);

    if (mDecoder) {
      int32_t unicharLength = srcLength;
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
          JSString *str = JS_NewUCString(cx, reinterpret_cast<jschar*>(unichars), unicharLength);
          if (str) {
            rval.setString(str);
            return true;
          }
        }
        JS_free(cx, unichars);
      }
    }

    xpc::Throw(cx, NS_ERROR_OUT_OF_MEMORY);
    return false;
  }

  void AssertThreadSafety()
  {
    MOZ_ASSERT(mThread == PR_GetCurrentThread(),
               "XPCLocaleCallbacks used unsafely!");
  }

  nsCOMPtr<nsICollation> mCollation;
  nsCOMPtr<nsIUnicodeDecoder> mDecoder;
#ifdef DEBUG
  PRThread* mThread;
#endif
};

NS_EXPORT_(bool)
xpc_LocalizeRuntime(JSRuntime *rt)
{
  JS_SetLocaleCallbacks(rt, new XPCLocaleCallbacks());

  
  nsCOMPtr<nsILocaleService> localeService =
    do_GetService(NS_LOCALESERVICE_CONTRACTID);
  if (!localeService)
    return false;

  nsCOMPtr<nsILocale> appLocale;
  nsresult rv = localeService->GetApplicationLocale(getter_AddRefs(appLocale));
  if (NS_FAILED(rv))
    return false;

  nsAutoString localeStr;
  rv = appLocale->GetCategory(NS_LITERAL_STRING(NSILOCALE_TIME), localeStr);
  MOZ_ASSERT(NS_SUCCEEDED(rv), "failed to get app locale info");
  NS_LossyConvertUTF16toASCII locale(localeStr);

  return !!JS_SetDefaultLocale(rt, locale.get());
}

NS_EXPORT_(void)
xpc_DelocalizeRuntime(JSRuntime *rt)
{
  XPCLocaleCallbacks* lc = XPCLocaleCallbacks::This(rt);
  JS_SetLocaleCallbacks(rt, nullptr);
  delete lc;
}
