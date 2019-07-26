







#ifndef js_Anchor_h
#define js_Anchor_h

#include "mozilla/Attributes.h"

#include "js/TypeDecls.h"

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

  private:
    T hold;

    


















    void operator=(const T &t) MOZ_DELETE;

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
