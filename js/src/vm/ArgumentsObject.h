







































#ifndef ArgumentsObject_h___
#define ArgumentsObject_h___

#include "jsfun.h"

#ifdef JS_POLYIC
class GetPropCompiler;
#endif

namespace js {

#ifdef JS_POLYIC
struct VMFrame;
namespace mjit {
namespace ic {
struct PICInfo;
struct GetElementIC;


#ifdef GetProp
#undef GetProp
#endif
void JS_FASTCALL GetProp(VMFrame &f, ic::PICInfo *pic);
}
}
#endif

struct EmptyShape;








struct ArgumentsData
{
    



    HeapValue   callee;

    



    HeapValue   slots[1];
};





















































class ArgumentsObject : public JSObject
{
    static const uint32_t INITIAL_LENGTH_SLOT = 0;
    static const uint32_t DATA_SLOT = 1;
    static const uint32_t STACK_FRAME_SLOT = 2;

  public:
    static const uint32_t RESERVED_SLOTS = 3;
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT4;

  private:
    
    static const uint32_t LENGTH_OVERRIDDEN_BIT = 0x1;
    static const uint32_t PACKED_BITS_COUNT = 1;

    



#ifdef JS_POLYIC
    friend class ::GetPropCompiler;
    friend struct mjit::ic::GetElementIC;
#endif

    void initInitialLength(uint32_t length);

    void initData(ArgumentsData *data);

  public:
    
    static ArgumentsObject *create(JSContext *cx, uint32_t argc, JSObject &callee);

    



    inline uint32_t initialLength() const;

    
    inline bool hasOverriddenLength() const;
    inline void markLengthOverridden();

    







    inline bool getElement(uint32_t i, js::Value *vp);

    








    inline bool getElements(uint32_t start, uint32_t count, js::Value *vp);

    inline js::ArgumentsData *data() const;

    inline const js::Value &element(uint32_t i) const;
    inline const js::Value *elements() const;
    inline void setElement(uint32_t i, const js::Value &v);

    
    inline js::StackFrame *maybeStackFrame() const;
    inline void setStackFrame(js::StackFrame *frame);

    



    inline size_t sizeOfMisc(JSMallocSizeOfFun mallocSizeOf) const;
};

class NormalArgumentsObject : public ArgumentsObject
{
    friend bool JSObject::isNormalArguments() const;
    friend struct EmptyShape; 
    friend ArgumentsObject *
    ArgumentsObject::create(JSContext *cx, uint32_t argc, JSObject &callee);

  public:
    



    inline const js::Value &callee() const;

    
    inline void clearCallee();
};

class StrictArgumentsObject : public ArgumentsObject
{
    friend bool JSObject::isStrictArguments() const;
    friend ArgumentsObject *
    ArgumentsObject::create(JSContext *cx, uint32_t argc, JSObject &callee);
};

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
