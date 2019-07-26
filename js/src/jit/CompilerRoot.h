





#ifndef jit_CompilerRoot_h
#define jit_CompilerRoot_h

#ifdef JS_ION

#include "jscntxt.h"

#include "jit/Ion.h"
#include "jit/IonAllocPolicy.h"
#include "js/RootingAPI.h"

namespace js {
namespace jit {




template <typename T>
class CompilerRoot : public CompilerRootNode
{
  public:
    explicit CompilerRoot(T ptr)
      : CompilerRootNode(nullptr)
    {
        if (ptr) {
            JS_ASSERT(!GetIonContext()->runtime->isInsideNursery(ptr));
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
