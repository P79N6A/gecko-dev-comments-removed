






#if !defined(jsion_ion_gc_h__) && defined(JS_ION)
#define jsion_ion_gc_h__

#include "jscntxt.h"
#include "gc/Root.h"

namespace js {
namespace ion {




template <typename T>
class CompilerRoot : public CompilerRootNode
{
  public:
    CompilerRoot()
      : CompilerRootNode(NULL)
    { }

    CompilerRoot(T ptr)
      : CompilerRootNode(NULL)
    {
        if (ptr)
            setRoot(ptr);
    }

  public:
    
    void setRoot(T root) {
        JS::CompilerRootNode *&rootList = GetIonContext()->temp->rootList();

        JS_ASSERT(!ptr);
        ptr = root;
        next = rootList;
        rootList = this;
    }

  public:
    operator T () const { return static_cast<T>(ptr); }
    T operator ->() const { return static_cast<T>(ptr); }
};

typedef CompilerRoot<JSObject*>   CompilerRootObject;
typedef CompilerRoot<JSFunction*> CompilerRootFunction;
typedef CompilerRoot<PropertyName*> CompilerRootPropertyName;
typedef CompilerRoot<Value> CompilerRootValue;

} 
} 

#endif 

