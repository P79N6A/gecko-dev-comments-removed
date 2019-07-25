





































#include "nsStructuredCloneContainer.h"

#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsIJSContextStack.h"
#include "nsIScriptContext.h"
#include "nsIVariant.h"
#include "nsServiceManagerUtils.h"
#include "nsContentUtils.h"
#include "xpcprivate.h"

NS_IMPL_ADDREF(nsStructuredCloneContainer)
NS_IMPL_RELEASE(nsStructuredCloneContainer)

NS_INTERFACE_MAP_BEGIN(nsStructuredCloneContainer)
  NS_INTERFACE_MAP_ENTRY(nsIStructuredCloneContainer)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

nsStructuredCloneContainer::nsStructuredCloneContainer()
  : mData(nsnull), mSize(0), mVersion(0)
{
}

nsStructuredCloneContainer::~nsStructuredCloneContainer()
{
  free(mData);
}

nsresult
nsStructuredCloneContainer::InitFromVariant(nsIVariant *aData, JSContext *aCx)
{
  NS_ENSURE_STATE(!mData);
  NS_ENSURE_ARG_POINTER(aData);
  NS_ENSURE_ARG_POINTER(aCx);

  
  
  jsval jsData;
  nsresult rv = aData->GetAsJSVal(&jsData);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_UNEXPECTED);

  
  JSAutoRequest ar(aCx);
  JSAutoEnterCompartment ac;
  NS_ENSURE_STATE(ac.enter(aCx, JS_GetGlobalObject(aCx)));

  nsCxPusher cxPusher;
  cxPusher.Push(aCx);

  PRUint64* jsBytes = nsnull;
  PRBool success = JS_WriteStructuredClone(aCx, jsData, &jsBytes, &mSize,
                                           nsnull, nsnull);
  NS_ENSURE_STATE(success);
  NS_ENSURE_STATE(jsBytes);

  
  mData = (PRUint64*) malloc(mSize);
  if (!mData) {
    mSize = 0;
    mVersion = 0;

    
    JS_free(aCx, jsBytes);

    return NS_ERROR_FAILURE;
  }
  else {
    mVersion = JS_STRUCTURED_CLONE_VERSION;
  }

  memcpy(mData, jsBytes, mSize);

  
  JS_free(aCx, jsBytes);
  return NS_OK;
}

nsresult
nsStructuredCloneContainer::InitFromBase64(const nsAString &aData,
                                           PRUint32 aFormatVersion,
                                           JSContext *aCx)
{
  NS_ENSURE_STATE(!mData);

  NS_ConvertUTF16toUTF8 data(aData);

  nsCAutoString binaryData;
  nsresult rv = nsXPConnect::Base64Decode(data, binaryData);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mData = (PRUint64*) malloc(binaryData.Length());
  NS_ENSURE_STATE(mData);
  memcpy(mData, binaryData.get(), binaryData.Length());

  mSize = binaryData.Length();
  mVersion = aFormatVersion;
  return NS_OK;
}


nsresult
nsStructuredCloneContainer::DeserializeToVariant(JSContext *aCx,
                                                 nsIVariant **aData)
{
  NS_ENSURE_STATE(mData);
  NS_ENSURE_ARG_POINTER(aData);
  *aData = nsnull;

  
  jsval jsStateObj;
  PRBool success = JS_ReadStructuredClone(aCx, mData, mSize, mVersion,
                                          &jsStateObj, nsnull, nsnull);
  NS_ENSURE_STATE(success);

  
  nsCOMPtr<nsIVariant> varStateObj;
  nsCOMPtr<nsIXPConnect> xpconnect = do_GetService(nsIXPConnect::GetCID());
  NS_ENSURE_STATE(xpconnect);
  xpconnect->JSValToVariant(aCx, &jsStateObj, getter_AddRefs(varStateObj));
  NS_ENSURE_STATE(varStateObj);

  NS_IF_ADDREF(*aData = varStateObj);
  return NS_OK;
}

nsresult
nsStructuredCloneContainer::GetDataAsBase64(nsAString &aOut)
{
  NS_ENSURE_STATE(mData);
  aOut.Truncate();

  nsCAutoString binaryData(reinterpret_cast<char*>(mData), mSize);
  nsCAutoString base64Data;
  nsresult rv = nsXPConnect::Base64Encode(binaryData, base64Data);
  NS_ENSURE_SUCCESS(rv, rv);

  aOut.Assign(NS_ConvertASCIItoUTF16(base64Data));
  return NS_OK;
}

nsresult
nsStructuredCloneContainer::GetSerializedNBytes(PRUint64 *aSize)
{
  NS_ENSURE_STATE(mData);
  NS_ENSURE_ARG_POINTER(aSize);

  
  
  
  *aSize = mSize;

  return NS_OK;
}

nsresult
nsStructuredCloneContainer::GetFormatVersion(PRUint32 *aFormatVersion)
{
  NS_ENSURE_STATE(mData);
  NS_ENSURE_ARG_POINTER(aFormatVersion);
  *aFormatVersion = mVersion;
  return NS_OK;
}
