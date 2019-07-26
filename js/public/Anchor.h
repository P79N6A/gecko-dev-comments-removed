








#ifndef js_Anchor_h___
#define js_Anchor_h___

#include "mozilla/Attributes.h"

class JSFunction;
class JSObject;
class JSScript;
class JSString;

namespace JS { class Value; }

namespace JS {





























































template<typename T> class AnchorPermitted;
template<> class AnchorPermitted<JSObject *> { };
template<> class AnchorPermitted<const JSObject *> { };
template<> class AnchorPermitted<JSFunction *> { };
template<> class AnchorPermitted<const JSFunction *> { };
template<> class AnchorPermitted<JSString *> { };
template<> class AnchorPermitted<const JSString *> { };
template<> class AnchorPermitted<Value> { };
template<> class AnchorPermitted<const JSScript *> { };
template<> class AnchorPermitted<JSScript *> { };

template<typename T>
class Anchor : AnchorPermitted<T>
{
  public:
    Anchor() { }
    explicit Anchor(T t) { hold = t; }
    inline ~Anchor();
    T &get() { return hold; }
    const T &get() const { return hold; }
    void set(const T &t) { hold = t; }
    void operator=(const T &t) { hold = t; }
    void clear() { hold = 0; }

  private:
    T hold;
    Anchor(const Anchor &other) MOZ_DELETE;
    void operator=(const Anchor &other) MOZ_DELETE;
};

template<typename T>
inline Anchor<T>::~Anchor()
{
#ifdef __GNUC__
    










    asm volatile("":: "g" (hold) : "memory");
#else
    
























    volatile T sink;
    sink = hold;
#endif  
}

} 

#endif 
