





































BEGIN_WORKERS_NAMESPACE

inline
JSBool
SetJSPrivateSafeish(JSContext* aCx, JSObject* aObj, PrivatizableBase* aBase)
{
  return JS_SetPrivate(aCx, aObj, aBase);
}

template <class Derived>
inline
Derived*
GetJSPrivateSafeish(JSContext* aCx, JSObject* aObj)
{
  return static_cast<Derived*>(
    static_cast<PrivatizableBase*>(JS_GetPrivate(aCx, aObj)));
}

END_WORKERS_NAMESPACE
