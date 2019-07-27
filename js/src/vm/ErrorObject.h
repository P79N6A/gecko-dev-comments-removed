





#ifndef vm_ErrorObject_h_
#define vm_ErrorObject_h_

#include "mozilla/ArrayUtils.h"

#include "vm/NativeObject.h"
#include "vm/SavedStacks.h"
#include "vm/Shape.h"

namespace js {




extern JSObject*
InitExceptionClasses(JSContext* cx, HandleObject obj);

class ErrorObject : public NativeObject
{
    static JSObject*
    createProto(JSContext* cx, JSProtoKey key);

    static JSObject*
    createConstructor(JSContext* cx, JSProtoKey key);

    
    friend JSObject*
    js::InitExceptionClasses(JSContext* cx, HandleObject global);

    static bool
    init(JSContext* cx, Handle<ErrorObject*> obj, JSExnType type,
         ScopedJSFreePtr<JSErrorReport>* errorReport, HandleString fileName, HandleObject stack,
         uint32_t lineNumber, uint32_t columnNumber, HandleString message);

    static bool checkAndUnwrapThis(JSContext* cx, CallArgs& args, const char* fnName,
                                   MutableHandle<ErrorObject*> error);

  protected:
    static const uint32_t EXNTYPE_SLOT          = 0;
    static const uint32_t STACK_SLOT            = EXNTYPE_SLOT + 1;
    static const uint32_t ERROR_REPORT_SLOT     = STACK_SLOT + 1;
    static const uint32_t FILENAME_SLOT         = ERROR_REPORT_SLOT + 1;
    static const uint32_t LINENUMBER_SLOT       = FILENAME_SLOT + 1;
    static const uint32_t COLUMNNUMBER_SLOT     = LINENUMBER_SLOT + 1;
    static const uint32_t MESSAGE_SLOT          = COLUMNNUMBER_SLOT + 1;

    static const uint32_t RESERVED_SLOTS = MESSAGE_SLOT + 1;

  public:
    static const Class classes[JSEXN_LIMIT];

    static const Class * classForType(JSExnType type) {
        MOZ_ASSERT(type != JSEXN_NONE);
        MOZ_ASSERT(type < JSEXN_LIMIT);
        return &classes[type];
    }

    static bool isErrorClass(const Class* clasp) {
        return &classes[0] <= clasp && clasp < &classes[0] + mozilla::ArrayLength(classes);
    }

    
    
    
    
    static ErrorObject*
    create(JSContext* cx, JSExnType type, HandleObject stack, HandleString fileName,
           uint32_t lineNumber, uint32_t columnNumber, ScopedJSFreePtr<JSErrorReport>* report,
           HandleString message);

    




    static Shape*
    assignInitialShape(ExclusiveContext* cx, Handle<ErrorObject*> obj);

    JSExnType type() const {
        return JSExnType(getReservedSlot(EXNTYPE_SLOT).toInt32());
    }

    JSErrorReport * getErrorReport() const {
        const Value& slot = getReservedSlot(ERROR_REPORT_SLOT);
        if (slot.isUndefined())
            return nullptr;
        return static_cast<JSErrorReport*>(slot.toPrivate());
    }

    JSErrorReport * getOrCreateErrorReport(JSContext* cx);

    inline JSString * fileName(JSContext* cx) const;
    inline uint32_t lineNumber() const;
    inline uint32_t columnNumber() const;
    inline JSObject * stack() const;

    JSString * getMessage() const {
        const HeapSlot& slot = getReservedSlotRef(MESSAGE_SLOT);
        return slot.isString() ? slot.toString() : nullptr;
    }

    
    static bool getStack(JSContext* cx, unsigned argc, Value* vp);
    static bool setStack(JSContext* cx, unsigned argc, Value* vp);
    static bool setStack_impl(JSContext* cx, CallArgs args);
};

} 

template<>
inline bool
JSObject::is<js::ErrorObject>() const
{
    return js::ErrorObject::isErrorClass(getClass());
}

#endif 
