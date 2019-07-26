





#ifndef vm_ErrorObject_h_
#define vm_ErrorObject_h_

#include "jsobj.h"

#include "vm/Shape.h"

struct JSExnPrivate;




extern JSObject *
js_InitExceptionClasses(JSContext *cx, JS::HandleObject obj);

namespace js {

class ErrorObject : public JSObject
{
    static ErrorObject *
    createProto(JSContext *cx, JS::Handle<GlobalObject*> global, JSExnType type,
                JS::HandleObject proto);

    
    friend JSObject *
    ::js_InitExceptionClasses(JSContext *cx, JS::HandleObject global);

    
    friend bool
    EmptyShape::ensureInitialCustomShape<ErrorObject>(ExclusiveContext *cx,
                                                      Handle<ErrorObject*> obj);

    




    static Shape *
    assignInitialShape(ExclusiveContext *cx, Handle<ErrorObject*> obj);

    static bool
    init(JSContext *cx, Handle<ErrorObject*> obj, JSExnType type,
         ScopedJSFreePtr<JSErrorReport> *errorReport, HandleString fileName, HandleString stack,
         uint32_t lineNumber, uint32_t columnNumber, HandleString message);

  protected:
    static const uint32_t EXNTYPE_SLOT      = 0;
    static const uint32_t ERROR_REPORT_SLOT = EXNTYPE_SLOT + 1;
    static const uint32_t FILENAME_SLOT     = ERROR_REPORT_SLOT + 1;
    static const uint32_t LINENUMBER_SLOT   = FILENAME_SLOT + 1;
    static const uint32_t COLUMNNUMBER_SLOT = LINENUMBER_SLOT + 1;
    static const uint32_t STACK_SLOT        = COLUMNNUMBER_SLOT + 1;
    static const uint32_t MESSAGE_SLOT      = STACK_SLOT + 1;

    static const uint32_t RESERVED_SLOTS = MESSAGE_SLOT + 1;

  public:
    static const Class class_;

    
    
    
    
    static ErrorObject *
    create(JSContext *cx, JSExnType type, HandleString stack, HandleString fileName,
           uint32_t lineNumber, uint32_t columnNumber, ScopedJSFreePtr<JSErrorReport> *report,
           HandleString message);

    JSExnType type() const {
        return JSExnType(getReservedSlot(EXNTYPE_SLOT).toInt32());
    }

    JSErrorReport * getErrorReport() const {
        const Value &slot = getReservedSlot(ERROR_REPORT_SLOT);
        if (slot.isUndefined())
            return nullptr;
        return static_cast<JSErrorReport*>(slot.toPrivate());
    }

    JSErrorReport * getOrCreateErrorReport(JSContext *cx);

    inline JSString * fileName(JSContext *cx) const;
    inline uint32_t lineNumber() const;
    inline uint32_t columnNumber() const;
    inline JSString * stack(JSContext *cx) const;

    JSString * getMessage() const {
        const HeapSlot &slot = getReservedSlotRef(MESSAGE_SLOT);
        return slot.isString() ? slot.toString() : nullptr;
    }
};

} 

#endif 
