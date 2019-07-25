







































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

    DataType wrapperReturnType() const {
        if (outParam != Type_Void)
            return outParam;
        return returnType;
    }

    DataType failType() const {
        JS_ASSERT(returnType == Type_Object || returnType == Type_Bool);
        JS_ASSERT_IF(outParam != Type_Void, returnType == Type_Bool);
        return returnType;
    }

    VMFunction(void *wrapped, uint32 explicitArgs, DataType outParam,
               DataType returnType)
      : wrapped(wrapped), explicitArgs(explicitArgs), outParam(outParam),
        returnType(returnType)
    { }
};

template <class> struct TypeToDataType { };
template <> struct TypeToDataType<bool> { static const DataType result = Type_Bool; };
template <> struct TypeToDataType<JSObject *> { static const DataType result = Type_Object; };
template <class> struct OutParamToDataType { static const DataType result = Type_Void; };
template <> struct OutParamToDataType<Value *> { static const DataType result = Type_Value; };


template <class R, class A1, class A2, R pf(JSContext *, A1, A2)>
struct FunctionInfo3 : public VMFunction {
    static inline DataType returnType() {
        return TypeToDataType<R>::result;
    }
    static inline DataType outParam() {
        return OutParamToDataType<A2>::result;
    }
    static inline size_t explicitArgs() {
        return 1 + (outParam() == Type_Void ? 1 : 0);
    }
    FunctionInfo3()
      : VMFunction(JS_FUNC_TO_DATA_PTR(void *, pf), explicitArgs(), outParam(),
                   returnType())
    { }
};


template <class R, class A1, class A2, class A3, class A4, R pf(JSContext *, A1, A2, A3, A4)>
struct FunctionInfo5 : public VMFunction {
    static inline DataType returnType() {
        return TypeToDataType<R>::result;
    }
    static inline DataType outParam() {
        return OutParamToDataType<A4>::result;
    }
    static inline size_t explicitArgs() {
        return 3 + (outParam() == Type_Void ? 1 : 0);
    }
    FunctionInfo5()
      : VMFunction(JS_FUNC_TO_DATA_PTR(void *, pf), explicitArgs(), outParam(),
                   returnType())
    { }
};

bool InvokeFunction(JSContext *cx, JSFunction *fun, uint32 argc, Value *argv, Value *rval);

} 
} 

#endif 

