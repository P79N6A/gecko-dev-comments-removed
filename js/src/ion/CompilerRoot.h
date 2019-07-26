





#ifndef ion_CompilerRoot_h
#define ion_CompilerRoot_h

#ifdef JS_ION

#include "jscntxt.h"

#include "js/RootingAPI.h"

namespace js {
namespace ion {




template <typename T>
class CompilerRoot : public CompilerRootNode
{
  public:
    CompilerRoot(T ptr)
      : CompilerRootNode(NULL)
    {
        if (ptr) {
            JS_ASSERT(!UninlinedIsInsideNursery(GetIonContext()->compartment->rt, ptr));
            setRoot(ptr);
        }
    }

  public:
    
    void setRoot(T root) {
        CompilerRootNode *&rootList = GetIonContext()->temp->rootList();

        JS_ASSERT(!ptr_);
        ptr_ = root;
        next = rootList;
        rootList = this;
    }

  public:
    operator T () const { return static_cast<T>(ptr_); }
    T operator ->() const { return static_cast<T>(ptr_); }

  private:
    CompilerRoot() MOZ_DELETE;
    CompilerRoot(const CompilerRoot<T> &) MOZ_DELETE;
    CompilerRoot<T> &operator =(const CompilerRoot<T> &) MOZ_DELETE;
};

typedef CompilerRoot<JSObject*> CompilerRootObject;
typedef CompilerRoot<JSFunction*> CompilerRootFunction;
typedef CompilerRoot<JSScript*> CompilerRootScript;
typedef CompilerRoot<PropertyName*> CompilerRootPropertyName;
typedef CompilerRoot<Shape*> CompilerRootShape;
typedef CompilerRoot<Value> CompilerRootValue;

} 
} 

#endif 

#endif 
