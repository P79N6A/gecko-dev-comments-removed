



#ifndef __NSTARRAYHELPERS_H__
#define __NSTARRAYHELPERS_H__

template <class T>
inline nsresult
nsTArrayToJSArray(JSContext* aCx, const nsTArray<T>& aSourceArray,
                  JSObject** aResultArray)
{
  MOZ_ASSERT(aCx);
  JSAutoRequest ar(aCx);

  JSObject* arrayObj = JS_NewArrayObject(aCx, aSourceArray.Length(), nullptr);
  if (!arrayObj) {
    NS_WARNING("JS_NewArrayObject failed!");
    return NS_ERROR_OUT_OF_MEMORY;
  }

  JSObject* global = JS_GetGlobalForScopeChain(aCx);
  MOZ_ASSERT(global);

  for (uint32_t index = 0; index < aSourceArray.Length(); index++) {
    nsCOMPtr<nsISupports> obj;
    nsresult rv = CallQueryInterface(aSourceArray[index], getter_AddRefs(obj));
    NS_ENSURE_SUCCESS(rv, rv);

    jsval wrappedVal;
    rv = nsContentUtils::WrapNative(aCx, global, obj, &wrappedVal, nullptr, true);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!JS_SetElement(aCx, arrayObj, index, &wrappedVal)) {
      NS_WARNING("JS_SetElement failed!");
      return NS_ERROR_FAILURE;
    }
  }

  if (!JS_FreezeObject(aCx, arrayObj)) {
    NS_WARNING("JS_FreezeObject failed!");
    return NS_ERROR_FAILURE;
  }

  *aResultArray = arrayObj;
  return NS_OK;
}

#endif 
