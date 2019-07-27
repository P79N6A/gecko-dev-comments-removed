







#ifndef js_Debug_h
#define js_Debug_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/UniquePtr.h"

#include "jspubtd.h"

#include "js/GCAPI.h"
#include "js/RootingAPI.h"
#include "js/TypeDecls.h"

namespace js {
class Debugger;
}

namespace JS {

using mozilla::UniquePtr;

namespace dbg {



























































































class BuilderOrigin;

class Builder {
    
    
    PersistentRootedObject debuggerObject;

    
    js::Debugger* debugger;

    
    
    
#if DEBUG
    void assertBuilt(JSObject* obj);
#else
    void assertBuilt(JSObject* obj) { }
#endif

  protected:
    
    
    template<typename T>
    class BuiltThing {
        friend class BuilderOrigin;

      protected:
        
        Builder& owner;

        
        PersistentRooted<T> value;

        BuiltThing(JSContext* cx, Builder& owner_, T value_ = js::GCMethods<T>::initial())
          : owner(owner_), value(cx, value_)
        {
            owner.assertBuilt(value_);
        }

        
        js::Debugger* debugger() const { return owner.debugger; }
        JSObject* debuggerObject() const { return owner.debuggerObject; }

      public:
        BuiltThing(const BuiltThing& rhs) : owner(rhs.owner), value(rhs.value) { }
        BuiltThing& operator=(const BuiltThing& rhs) {
            MOZ_ASSERT(&owner == &rhs.owner);
            owner.assertBuilt(rhs.value);
            value = rhs.value;
            return *this;
        }

        explicit operator bool() const {
            
            return value;
        }

      private:
        BuiltThing() = delete;
    };

  public:
    
    
    
    class Object: private BuiltThing<JSObject*> {
        friend class Builder;           
        friend class BuilderOrigin;     

        typedef BuiltThing<JSObject*> Base;

        
        
        Object(JSContext* cx, Builder& owner_, HandleObject obj) : Base(cx, owner_, obj.get()) { }

        bool definePropertyToTrusted(JSContext* cx, const char* name,
                                     JS::MutableHandleValue value);

      public:
        Object(JSContext* cx, Builder& owner_) : Base(cx, owner_, nullptr) { }
        Object(const Object& rhs) : Base(rhs) { }

        
        

        
        
        
        
        
        
        
        
        
        
        
        
        
        bool defineProperty(JSContext* cx, const char* name, JS::HandleValue value);
        bool defineProperty(JSContext* cx, const char* name, JS::HandleObject value);
        bool defineProperty(JSContext* cx, const char* name, Object& value);

        using Base::operator bool;
    };

    
    
    Object newObject(JSContext* cx);

  protected:
    Builder(JSContext* cx, js::Debugger* debugger);
};



class BuilderOrigin : public Builder {
    template<typename T>
    T unwrapAny(const BuiltThing<T>& thing) {
        MOZ_ASSERT(&thing.owner == this);
        return thing.value.get();
    }

  public:
    BuilderOrigin(JSContext* cx, js::Debugger* debugger_)
      : Builder(cx, debugger_)
    { }

    JSObject* unwrap(Object& object) { return unwrapAny(object); }
};













JS_PUBLIC_API(void)
SetDebuggerMallocSizeOf(JSRuntime* runtime, mozilla::MallocSizeOf mallocSizeOf);



JS_PUBLIC_API(mozilla::MallocSizeOf)
GetDebuggerMallocSizeOf(JSRuntime* runtime);
















JS_PUBLIC_API(bool)
FireOnGarbageCollectionHook(JSContext* cx, GarbageCollectionEvent::Ptr&& data);
















JS_PUBLIC_API(void)
onNewPromise(JSContext* cx, HandleObject promise);










JS_PUBLIC_API(void)
onPromiseSettled(JSContext* cx, HandleObject promise);




JS_PUBLIC_API(bool)
IsDebugger(const JSObject &obj);



JS_PUBLIC_API(bool)
GetDebuggeeGlobals(JSContext *cx, const JSObject &dbgObj, AutoObjectVector &vector);

} 
} 


#endif 
