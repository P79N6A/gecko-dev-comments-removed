






































#include "nsNativeTypes.h"
#include "nsNativeMethod.h"
#include "nsServiceManagerUtils.h"
#include "nsAutoPtr.h"
#include "nsILocalFile.h"
#include "prlink.h"
#include "jsapi.h"

static inline nsresult
jsvalToUint16(JSContext* aContext, jsval aVal, PRUint16& aResult)
{
  if (JSVAL_IS_INT(aVal)) {
    PRUint32 i = JSVAL_TO_INT(aVal);
    if (i <= PR_UINT16_MAX) {
      aResult = i;
      return NS_OK;
    }
  }

  JS_ReportError(aContext, "Parameter must be an integer");
  return NS_ERROR_INVALID_ARG;
}

static inline nsresult
jsvalToCString(JSContext* aContext, jsval aVal, const char*& aResult)
{
  if (JSVAL_IS_STRING(aVal)) {
    aResult = JS_GetStringBytes(JSVAL_TO_STRING(aVal));
    return NS_OK;
  }

  JS_ReportError(aContext, "Parameter must be a string");
  return NS_ERROR_INVALID_ARG;
}

NS_IMPL_ISUPPORTS1(nsNativeTypes, nsINativeTypes)

nsNativeTypes::nsNativeTypes()
  : mLibrary(nsnull)
{
}

nsNativeTypes::~nsNativeTypes()
{
  Close();
}

NS_IMETHODIMP
nsNativeTypes::Open(nsILocalFile* aFile)
{
  NS_ENSURE_ARG(aFile);
  NS_ENSURE_TRUE(!mLibrary, NS_ERROR_ALREADY_INITIALIZED);

  return aFile->Load(&mLibrary);
}

NS_IMETHODIMP
nsNativeTypes::Close()
{
  if (mLibrary) {
    PR_UnloadLibrary(mLibrary);
    mLibrary = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsNativeTypes::Declare(nsISupports** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  NS_ENSURE_TRUE(mLibrary, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;

  nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID());

  nsAXPCNativeCallContext* ncc;
  rv = xpc->GetCurrentNativeCallContext(&ncc);
  NS_ENSURE_SUCCESS(rv, rv);

  JSContext *ctx;
  rv = ncc->GetJSContext(&ctx);
  NS_ENSURE_SUCCESS(rv, rv);

  JSAutoRequest ar(ctx);

  PRUint32 argc;
  jsval *argv;
  ncc->GetArgc(&argc);
  ncc->GetArgvPtr(&argv);

  
  if (argc < 3) {
    JS_ReportError(ctx, "Insufficient number of arguments");
    return NS_ERROR_INVALID_ARG;
  }

  const char* name;
  rv = jsvalToCString(ctx, argv[0], name);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint16 callType;
  rv = jsvalToUint16(ctx, argv[1], callType);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoTArray<jsval, 16> argTypes;
  for (PRUint32 i = 3; i < argc; ++i) {
    argTypes.AppendElement(argv[i]);
  }

  PRFuncPtr func = PR_FindFunctionSymbol(mLibrary, name);
  if (!func) {
    JS_ReportError(ctx, "Couldn't find function symbol in library");
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<nsNativeMethod> call = new nsNativeMethod;
  rv = call->Init(ctx, this, func, callType, argv[2], argTypes);
  NS_ENSURE_SUCCESS(rv, rv);

  call.forget(aResult);
  return rv;
}

