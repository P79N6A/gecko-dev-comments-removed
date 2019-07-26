




#ifndef MessageUtils_h
#define MessageUtils_h












static nsresult
convertTimeToInt(JSContext* aCx, const JS::Value& aTime, uint64_t& aReturn)
{
  if (aTime.isObject()) {
    JS::Rooted<JSObject*> timestampObj(aCx, &aTime.toObject());
    if (!JS_ObjectIsDate(aCx, timestampObj)) {
      return NS_ERROR_INVALID_ARG;
    }
    aReturn = js_DateGetMsecSinceEpoch(timestampObj);
  } else {
    if (!aTime.isNumber()) {
      return NS_ERROR_INVALID_ARG;
    }
    double number = aTime.toNumber();
    if (static_cast<uint64_t>(number) != number) {
      return NS_ERROR_INVALID_ARG;
    }
    aReturn = static_cast<uint64_t>(number);
  }
  return NS_OK;
}

#endif
