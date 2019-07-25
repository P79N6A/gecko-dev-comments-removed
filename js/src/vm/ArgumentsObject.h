







































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












































class ArgumentsObject : public ::JSObject
{
    static const uint32 INITIAL_LENGTH_SLOT = 0;
    static const uint32 DATA_SLOT = 1;

  public:
    static const uint32 RESERVED_SLOTS = 2;

  private:
    
    static const uint32 LENGTH_OVERRIDDEN_BIT = 0x1;
    static const uint32 PACKED_BITS_COUNT = 1;

    



#ifdef JS_POLYIC
    friend class ::GetPropCompiler;
    friend struct mjit::ic::GetElementIC;
#endif

    void initInitialLength(uint32 length);

    void initData(ArgumentsData *data);

  public:
    
    static ArgumentsObject *create(JSContext *cx, uint32 argc, JSObject &callee);

    



    inline uint32 initialLength() const;

    
    inline bool hasOverriddenLength() const;
    inline void markLengthOverridden();

    







    inline bool getElement(uint32 i, js::Value *vp);

    








    inline bool getElements(uint32 start, uint32 count, js::Value *vp);

    inline js::ArgumentsData *data() const;

    inline const js::Value &element(uint32 i) const;
    inline const js::Value *elements() const;
    inline void setElement(uint32 i, const js::Value &v);

    
    inline js::StackFrame *maybeStackFrame() const;
    inline void setStackFrame(js::StackFrame *frame);
};





class NormalArgumentsObject : public ArgumentsObject
{
    friend bool JSObject::isNormalArguments() const;
    friend struct EmptyShape; 
    friend ArgumentsObject *
    ArgumentsObject::create(JSContext *cx, uint32 argc, JSObject &callee);

  public:
    



    inline const js::Value &callee() const;

    
    inline void clearCallee();
};







class StrictArgumentsObject : public ArgumentsObject
{
    friend bool JSObject::isStrictArguments() const;
    friend ArgumentsObject *
    ArgumentsObject::create(JSContext *cx, uint32 argc, JSObject &callee);
};

} 

js::NormalArgumentsObject *
JSObject::asNormalArguments()
{
    JS_ASSERT(isNormalArguments());
    return reinterpret_cast<js::NormalArgumentsObject *>(this);
}

js::StrictArgumentsObject *
JSObject::asStrictArguments()
{
    JS_ASSERT(isStrictArguments());
    return reinterpret_cast<js::StrictArgumentsObject *>(this);
}

js::ArgumentsObject *
JSObject::asArguments()
{
    JS_ASSERT(isArguments());
    return reinterpret_cast<js::ArgumentsObject *>(this);
}

#endif 
