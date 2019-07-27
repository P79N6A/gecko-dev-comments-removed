





#ifndef vm_WrapperObject_h
#define vm_WrapperObject_h

#include "jsobj.h"
#include "jswrapper.h"

#include "vm/ProxyObject.h"

namespace js {



extern const char sWrapperFamily;

class WrapperObject : public ProxyObject
{
};

class CrossCompartmentWrapperObject : public WrapperObject
{
};

} 

template<>
inline bool
JSObject::is<js::WrapperObject>() const
{
    return js::IsWrapper(const_cast<JSObject*>(this));
}

template<>
inline bool
JSObject::is<js::CrossCompartmentWrapperObject>() const
{
    return js::IsCrossCompartmentWrapper(const_cast<JSObject*>(this));
}

#endif 
