





#include "BluetoothUtils.h"
#include "jsapi.h"
#include "nsTArray.h"
#include "nsString.h"
#include "mozilla/Scoped.h"

nsresult
mozilla::dom::bluetooth::StringArrayToJSArray(JSContext* aCx, JSObject* aGlobal,
                                              const nsTArray<nsString>& aSourceArray,
                                              JSObject** aResultArray)
{
  NS_ASSERTION(aCx, "Null context!");
  NS_ASSERTION(aGlobal, "Null global!");

  JSAutoRequest ar(aCx);
  JSAutoEnterCompartment ac;
  if (!ac.enter(aCx, aGlobal)) {
    NS_WARNING("Failed to enter compartment!");
    return NS_ERROR_FAILURE;
  }

  JSObject* arrayObj;

  if (aSourceArray.IsEmpty()) {
    arrayObj = JS_NewArrayObject(aCx, 0, nullptr);
  } else {
    uint32_t valLength = aSourceArray.Length();
    mozilla::ScopedDeleteArray<jsval> valArray(new jsval[valLength]);
    JS::AutoArrayRooter tvr(aCx, valLength, valArray);
    for (PRUint32 index = 0; index < valLength; index++) {
      JSString* s = JS_NewUCStringCopyN(aCx, aSourceArray[index].BeginReading(),
                                        aSourceArray[index].Length());
      if(!s) {
        NS_WARNING("Memory allocation error!");
        return NS_ERROR_OUT_OF_MEMORY;
      }
      valArray[index] = STRING_TO_JSVAL(s);
    }
    arrayObj = JS_NewArrayObject(aCx, valLength, valArray);
  }

  if (!arrayObj) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  if (!JS_FreezeObject(aCx, arrayObj)) {
    return NS_ERROR_FAILURE;
  }

  *aResultArray = arrayObj;
  return NS_OK;
}

