







































#ifndef jsion_vm_functions_h__
#define jsion_vm_functions_h__

#include "jspubtd.h"

namespace js {
namespace ion {

enum DataType {
    Type_Void,
    Type_Bool,
    Type_Object,
    Type_Value
};











struct VMFunction
{
    
    void *wrapped;

    
    
    uint32 explicitArgs;

    
    
    
    DataType outParam;

    
    
    
    
    
    
    
    DataType returnType;

    uint32 argc() const {
        
        return 1 + explicitArgs + ((outParam == Type_Void) ? 0 : 1);
    }

    DataType failType() const {
        return returnType;
    }

    VMFunction(void *wrapped, uint32 explicitArgs, DataType outParam, DataType returnType)
      : wrapped(wrapped), explicitArgs(explicitArgs), outParam(outParam), returnType(returnType)
    {
        
        JS_ASSERT_IF(outParam != Type_Void, returnType == Type_Bool);
        JS_ASSERT(returnType == Type_Bool || returnType == Type_Object);
    }
};

template <class> struct TypeToDataType {  };
template <> struct TypeToDataType<bool> { static const DataType result = Type_Bool; };
template <> struct TypeToDataType<JSObject *> { static const DataType result = Type_Object; };

template <class> struct OutParamToDataType { static const DataType result = Type_Void; };
template <> struct OutParamToDataType<Value *> { static const DataType result = Type_Value; };

#define FUNCTION_INFO_STRUCT_BODY(NbArgs)                                               \
    static inline DataType returnType() {                                               \
        return TypeToDataType<R>::result;                                               \
    }                                                                                   \
    static inline DataType outParam() {                                                 \
        return OutParamToDataType<A ## NbArgs>::result;                                 \
    }                                                                                   \
    static inline size_t explicitArgs() {                                               \
        return NbArgs - (outParam() != Type_Void ? 1 : 0);                              \
    }                                                                                   \
    FunctionInfo(pf fun)                                                                \
      : VMFunction(JS_FUNC_TO_DATA_PTR(void *, fun), explicitArgs(), outParam(),        \
                   returnType())                                                        \
    { }


template <typename Fun>
struct FunctionInfo {
};



template <class R, class A1, class A2>
struct FunctionInfo<R (*)(JSContext *, A1, A2)> : public VMFunction {
    typedef R (*pf)(JSContext *, A1, A2);
    FUNCTION_INFO_STRUCT_BODY(2);
};

template <class R, class A1, class A2, class A3>
struct FunctionInfo<R (*)(JSContext *, A1, A2, A3)> : public VMFunction {
    typedef R (*pf)(JSContext *, A1, A2, A3);
    FUNCTION_INFO_STRUCT_BODY(3);
};

template <class R, class A1, class A2, class A3, class A4>
struct FunctionInfo<R (*)(JSContext *, A1, A2, A3, A4)> : public VMFunction {
    typedef R (*pf)(JSContext *, A1, A2, A3, A4);
    FUNCTION_INFO_STRUCT_BODY(4);
};

bool InvokeFunction(JSContext *cx, JSFunction *fun, uint32 argc, Value *argv, Value *rval);

} 
} 

#endif 

