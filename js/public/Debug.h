







#ifndef js_Debug_h
#define js_Debug_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Move.h"

#include "jspubtd.h"

#include "js/RootingAPI.h"
#include "js/TypeDecls.h"

namespace js {
class Debugger;
}

namespace JS {
namespace dbg {



























































































class BuilderOrigin;

class Builder {
    
    
    PersistentRootedObject debuggerObject;

    
    js::Debugger *debugger;

    
    
    
#if DEBUG
    void assertBuilt(JSObject *obj);
#else
    void assertBuilt(JSObject *obj) { }
#endif

  protected:
    
    
    template<typename T>
    class BuiltThing {
        friend class BuilderOrigin;

        void nonNull() {}

      protected:
        
        Builder &owner;

        
        PersistentRooted<T> value;

        BuiltThing(JSContext *cx, Builder &owner_, T value_ = js::GCMethods<T>::initial())
          : owner(owner_), value(cx, value_)
        {
            owner.assertBuilt(value_);
        }

        
        js::Debugger *debugger() const { return owner.debugger; }
        JSObject *debuggerObject() const { return owner.debuggerObject; }

      public:
        BuiltThing(const BuiltThing &rhs) : owner(rhs.owner), value(rhs.value) { }
        BuiltThing &operator=(const BuiltThing &rhs) {
            MOZ_ASSERT(&owner == &rhs.owner);
            owner.assertBuilt(rhs.value);
            value = rhs.value;
            return *this;
        }

        typedef void (BuiltThing::* ConvertibleToBool)();
        operator ConvertibleToBool() const {
            
            return value ? &BuiltThing::nonNull : 0;
        }

      private:
        BuiltThing() MOZ_DELETE;
    };

  public:
    
    
    
    class Object: private BuiltThing<JSObject *> {
        friend class Builder;           
        friend class BuilderOrigin;     

        typedef BuiltThing<JSObject *> Base;

        
        
        Object(JSContext *cx, Builder &owner_, HandleObject obj) : Base(cx, owner_, obj.get()) { }

        bool definePropertyToTrusted(JSContext *cx, const char *name,
                                     JS::MutableHandleValue value);

      public:
        Object(JSContext *cx, Builder &owner_) : Base(cx, owner_, nullptr) { }
        Object(const Object &rhs) : Base(rhs) { }

        
        

        
        
        
        
        
        
        
        
        
        
        
        
        
        bool defineProperty(JSContext *cx, const char *name, JS::HandleValue value);
        bool defineProperty(JSContext *cx, const char *name, JS::HandleObject value);
        bool defineProperty(JSContext *cx, const char *name, Object &value);

        using Base::ConvertibleToBool;
        using Base::operator ConvertibleToBool;
    };

    
    
    Object newObject(JSContext *cx);

  protected:
    Builder(JSContext *cx, js::Debugger *debugger);
};



class BuilderOrigin : public Builder {
    template<typename T>
    T unwrapAny(const BuiltThing<T> &thing) {
        MOZ_ASSERT(&thing.owner == this);
        return thing.value.get();
    }

  public:
    BuilderOrigin(JSContext *cx, js::Debugger *debugger_)
      : Builder(cx, debugger_)
    { }

    JSObject *unwrap(Object &object) { return unwrapAny(object); }
};

} 
} 


#endif 
