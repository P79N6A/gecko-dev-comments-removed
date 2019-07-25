








































#ifndef ArgumentsObject_h___
#define ArgumentsObject_h___

#include "jsfun.h"

namespace js {








struct ArgumentsData
{
    



    HeapValue   callee;

    



    size_t      *deletedBits;

    



    HeapValue   slots[1];
};





















































class ArgumentsObject : public JSObject
{
    static const uint32_t INITIAL_LENGTH_SLOT = 0;
    static const uint32_t DATA_SLOT = 1;
    static const uint32_t STACK_FRAME_SLOT = 2;

    
    static const uint32_t LENGTH_OVERRIDDEN_BIT = 0x1;
    static const uint32_t PACKED_BITS_COUNT = 1;

    void initInitialLength(uint32_t length);
    void initData(ArgumentsData *data);
    static ArgumentsObject *create(JSContext *cx, uint32_t argc, JSObject &callee);

  public:
    static const uint32_t RESERVED_SLOTS = 3;
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT4;

    
    static bool create(JSContext *cx, StackFrame *fp);

    





    static ArgumentsObject *createUnexpected(JSContext *cx, StackFrame *fp);

    



    static ArgumentsObject *createPoison(JSContext *cx, uint32_t argc, JSObject &callee);

    



    inline uint32_t initialLength() const;

    
    inline bool hasOverriddenLength() const;
    inline void markLengthOverridden();

    







    inline bool getElement(uint32_t i, js::Value *vp);

    








    inline bool getElements(uint32_t start, uint32_t count, js::Value *vp);

    inline js::ArgumentsData *data() const;

    













    inline bool isElementDeleted(uint32_t i) const;
    inline bool isAnyElementDeleted() const;
    inline void markElementDeleted(uint32_t i);

    inline const js::Value &element(uint32_t i) const;
    inline void setElement(uint32_t i, const js::Value &v);

    
    inline js::StackFrame *maybeStackFrame() const;
    inline void setStackFrame(js::StackFrame *frame);

    



    inline size_t sizeOfMisc(JSMallocSizeOfFun mallocSizeOf) const;
};

class NormalArgumentsObject : public ArgumentsObject
{
  public:
    



    inline const js::Value &callee() const;

    
    inline void clearCallee();

    




    static bool optimizedGetElem(JSContext *cx, StackFrame *fp, const Value &elem, Value *vp);
};

class StrictArgumentsObject : public ArgumentsObject
{};

} 

js::NormalArgumentsObject &
JSObject::asNormalArguments()
{
    JS_ASSERT(isNormalArguments());
    return *static_cast<js::NormalArgumentsObject *>(this);
}

js::StrictArgumentsObject &
JSObject::asStrictArguments()
{
    JS_ASSERT(isStrictArguments());
    return *static_cast<js::StrictArgumentsObject *>(this);
}

js::ArgumentsObject &
JSObject::asArguments()
{
    JS_ASSERT(isArguments());
    return *static_cast<js::ArgumentsObject *>(this);
}

const js::ArgumentsObject &
JSObject::asArguments() const
{
    JS_ASSERT(isArguments());
    return *static_cast<const js::ArgumentsObject *>(this);
}

#endif 
