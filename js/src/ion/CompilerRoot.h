







































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
        JSRuntime *rt = root->compartment()->rt;

        JS_ASSERT(!ptr);
        ptr = root;
        next = rt->ionCompilerRootList;
        rt->ionCompilerRootList = this;
    }

  public:
    operator T () const { return static_cast<T>(ptr); }
    T operator ->() const { return static_cast<T>(ptr); }
};

typedef CompilerRoot<JSObject*>   CompilerRootObject;
typedef CompilerRoot<JSFunction*> CompilerRootFunction;


class AutoCompilerRoots
{
    JSRuntime *rt_;

  public:
    AutoCompilerRoots(JSRuntime *rt)
      : rt_(rt)
    {
        JS_ASSERT(rt_->ionCompilerRootList == NULL);
    }

    ~AutoCompilerRoots()
    {
        rt_->ionCompilerRootList = NULL;
    }
};

} 
} 

#endif 

