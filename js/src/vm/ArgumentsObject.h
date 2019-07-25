







































#ifndef ArgumentsObject_h___
#define ArgumentsObject_h___

#include "jsfun.h"

#ifdef JS_POLYIC
class GetPropCompiler;
#endif

#define JS_ARGUMENTS_OBJECT_ON_TRACE ((void *)0xa126)

#ifdef JS_TRACER
namespace nanojit {
class ValidateWriter;
}
#endif

namespace js {

#ifdef JS_POLYIC
struct VMFrame;
namespace mjit {
namespace ic {
struct PICInfo;


#ifdef GetProp
#undef GetProp
#endif
void JS_FASTCALL GetProp(VMFrame &f, ic::PICInfo *pic);
}
}
#endif

#ifdef JS_TRACER
namespace tjit {
class Writer;
}
#endif

struct EmptyShape;








struct ArgumentsData
{
    



    js::Value   callee;

    



    js::Value   slots[1];
};












































class ArgumentsObject : public ::JSObject
{
    static const uint32 INITIAL_LENGTH_SLOT = 0;
    static const uint32 DATA_SLOT = 1;

  protected:
    static const uint32 RESERVED_SLOTS = 2;

  private:
    
    static const uint32 LENGTH_OVERRIDDEN_BIT = 0x1;
    static const uint32 PACKED_BITS_COUNT = 1;

#ifdef JS_TRACER
    



    friend class tjit::Writer;

    



    friend class ::nanojit::ValidateWriter;
#endif

    



#ifdef JS_TRACER
    friend class TraceRecorder;
#endif
#ifdef JS_POLYIC
    friend class ::GetPropCompiler;
#endif

    void setInitialLength(uint32 length);

    void setCalleeAndData(JSObject &callee, ArgumentsData *data);

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
};








class NormalArgumentsObject : public ArgumentsObject
{
    static js::Class jsClass;

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
    static js::Class jsClass;

    friend bool JSObject::isStrictArguments() const;
    friend ArgumentsObject *
    ArgumentsObject::create(JSContext *cx, uint32 argc, JSObject &callee);
};

} 

inline bool
JSObject::isNormalArguments() const
{
    return getClass() == &js::NormalArgumentsObject::jsClass;
}

js::NormalArgumentsObject *
JSObject::asNormalArguments()
{
    JS_ASSERT(isNormalArguments());
    return reinterpret_cast<js::NormalArgumentsObject *>(this);
}

inline bool
JSObject::isStrictArguments() const
{
    return getClass() == &js::StrictArgumentsObject::jsClass;
}

js::StrictArgumentsObject *
JSObject::asStrictArguments()
{
    JS_ASSERT(isStrictArguments());
    return reinterpret_cast<js::StrictArgumentsObject *>(this);
}

inline bool
JSObject::isArguments() const
{
    return isNormalArguments() || isStrictArguments();
}

js::ArgumentsObject *
JSObject::asArguments()
{
    JS_ASSERT(isArguments());
    return reinterpret_cast<js::ArgumentsObject *>(this);
}

#endif 
