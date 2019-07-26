





#ifndef vm_ProxyObject_h
#define vm_ProxyObject_h

#include "jsobj.h"
#include "jsproxy.h"

namespace js {



class ProxyObject : public JSObject
{
};

class FunctionProxyObject : public ProxyObject
{
  public:
    static Class class_;
};

class ObjectProxyObject : public ProxyObject
{
  public:
    static Class class_;
};

class OuterWindowProxyObject : public ObjectProxyObject
{
  public:
    static Class class_;
};

} 







template<>
inline bool
JSObject::is<js::ProxyObject>() const
{
    return js::IsProxy(const_cast<JSObject*>(this));
}

template<>
inline bool
JSObject::is<js::FunctionProxyObject>() const
{
    return js::IsFunctionProxy(const_cast<JSObject*>(this));
}




template<>
inline bool
JSObject::is<js::ObjectProxyObject>() const
{
    return js::IsObjectProxy(const_cast<JSObject*>(this));
}

#endif 
