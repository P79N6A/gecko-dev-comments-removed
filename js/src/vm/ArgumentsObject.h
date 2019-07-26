






#ifndef ArgumentsObject_h___
#define ArgumentsObject_h___

#include "jsfun.h"

namespace js {







struct ArgumentsData
{
    



    unsigned    numArgs;

    



    HeapValue   callee;

    
    JSScript    *script;

    



    size_t      *deletedBits;

    








    HeapValue   args[1];

    
    static ptrdiff_t offsetOfArgs() { return offsetof(ArgumentsData, args); }
};






































class ArgumentsObject : public JSObject
{
  protected:
    static const uint32_t INITIAL_LENGTH_SLOT = 0;
    static const uint32_t DATA_SLOT = 1;
    static const uint32_t MAYBE_CALL_SLOT = 2;

    static const uint32_t LENGTH_OVERRIDDEN_BIT = 0x1;
    static const uint32_t PACKED_BITS_COUNT = 1;

    template <typename CopyArgs>
    static ArgumentsObject *create(JSContext *cx, HandleScript script, HandleFunction callee,
                                   unsigned numActuals, CopyArgs &copy);

    inline ArgumentsData *data() const;

  public:
    static const uint32_t RESERVED_SLOTS = 3;
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT4_BACKGROUND;

    
    static ArgumentsObject *createExpected(JSContext *cx, AbstractFramePtr frame);

    





    static ArgumentsObject *createUnexpected(JSContext *cx, StackIter &iter);
    static ArgumentsObject *createUnexpected(JSContext *cx, AbstractFramePtr frame);

    



    inline uint32_t initialLength() const;

    
    JSScript *containingScript() const;

    
    inline bool hasOverriddenLength() const;
    inline void markLengthOverridden();

    













    inline bool isElementDeleted(uint32_t i) const;
    inline bool isAnyElementDeleted() const;
    inline void markElementDeleted(uint32_t i);

    














    inline const Value &element(uint32_t i) const;
    inline void setElement(uint32_t i, const Value &v);
    inline const Value &arg(unsigned i) const;
    inline void setArg(unsigned i, const Value &v);

    








    inline bool maybeGetElement(uint32_t i, MutableHandleValue vp);
    inline bool maybeGetElements(uint32_t start, uint32_t count, js::Value *vp);

    



    inline size_t sizeOfMisc(JSMallocSizeOfFun mallocSizeOf) const;

    static void finalize(FreeOp *fop, RawObject obj);
    static void trace(JSTracer *trc, RawObject obj);

    
    static size_t getDataSlotOffset() {
        return getFixedSlotOffset(DATA_SLOT);
    }

    static void MaybeForwardToCallObject(AbstractFramePtr frame, JSObject *obj, ArgumentsData *data);
};

class NormalArgumentsObject : public ArgumentsObject
{
  public:
    



    inline const js::Value &callee() const;

    
    inline void clearCallee();
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
