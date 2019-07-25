





































BEGIN_WORKERS_NAMESPACE

inline
void
SetJSPrivateSafeish(JSObject* aObj, PrivatizableBase* aBase)
{
  JS_SetPrivate(aObj, aBase);
}

template <class Derived>
inline
Derived*
GetJSPrivateSafeish(JSObject* aObj)
{
  return static_cast<Derived*>(
    static_cast<PrivatizableBase*>(JS_GetPrivate(aObj)));
}

END_WORKERS_NAMESPACE
