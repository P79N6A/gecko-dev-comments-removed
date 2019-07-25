







































#ifndef jsion_vm_functions_h__
#define jsion_vm_functions_h__

#include "jspubtd.h"

namespace js {
namespace ion {

enum DataType {
    Type_Void,
    Type_Bool,
    Type_Int32,
    Type_Object,
    Type_Value
};











struct VMFunction
{
    
    void *wrapped;

    
    
    uint32 explicitArgs;

    enum ArgProperties {
        WordByValue = 0,
        DoubleByValue = 1,
        WordByRef = 2,
        DoubleByRef = 3,
        
        Word = 0,
        Double = 1,
        ByRef = 2
    };

    
    uint32 argumentProperties;

    
    
    
    DataType outParam;

    
    
    
    
    
    
    
    DataType returnType;

    uint32 argc() const {
        
        return 1 + explicitArgc() + ((outParam == Type_Void) ? 0 : 1);
    }

    DataType failType() const {
        return returnType;
    }

    ArgProperties argProperties(uint32 explicitArg) const {
        return ArgProperties((argumentProperties >> (2 * explicitArg)) & 3);
    }

    
    size_t explicitStackSlots() const {
        size_t stackSlots = explicitArgs;

        
        uint32 n =
            ((1 << (explicitArgs * 2)) - 1) 
            & 0x55555555                    
            & argumentProperties;

        
        
        while (n) {
            stackSlots++;
            n &= n - 1;
        }
        return stackSlots;
    }

    
    
    
    
    
    size_t explicitArgc() const {
        size_t stackSlots = explicitArgs;

        
        uint32 n =
            ((1 << (explicitArgs * 2)) - 1) 
            & argumentProperties;

        
        
        n = (n & 0x55555555) & ~(n >> 1);

        
        
        while (n) {
            stackSlots++;
            n &= n - 1;
        }
        return stackSlots;
    }

    VMFunction()
      : wrapped(NULL),
        explicitArgs(0),
        argumentProperties(0),
        outParam(Type_Void),
        returnType(Type_Void)
    {
    }

    VMFunction(void *wrapped, uint32 explicitArgs, uint32 argumentProperties, DataType outParam, DataType returnType)
      : wrapped(wrapped),
        explicitArgs(explicitArgs),
        argumentProperties(argumentProperties),
        outParam(outParam),
        returnType(returnType)
    {
        
        JS_ASSERT_IF(outParam != Type_Void, returnType == Type_Bool);
        JS_ASSERT(returnType == Type_Bool || returnType == Type_Object);
    }
};

template <class> struct TypeToDataType {  };
template <> struct TypeToDataType<bool> { static const DataType result = Type_Bool; };
template <> struct TypeToDataType<JSObject *> { static const DataType result = Type_Object; };
template <> struct TypeToDataType<JSString *> { static const DataType result = Type_Object; };
template <> struct TypeToDataType<JSFixedString *> { static const DataType result = Type_Object; };


template <class T> struct TypeToArgProperties {
    static const uint32 result =
        (sizeof(T) <= sizeof(void *) ? VMFunction::Word : VMFunction::Double);
};
template <> struct TypeToArgProperties<const Value &> {
    static const uint32 result = TypeToArgProperties<Value>::result | VMFunction::ByRef;
};

template <class> struct OutParamToDataType { static const DataType result = Type_Void; };
template <> struct OutParamToDataType<Value *> { static const DataType result = Type_Value; };
template <> struct OutParamToDataType<int *> { static const DataType result = Type_Int32; };

#define FOR_EACH_ARGS_1(Macro, Sep, Last) Macro(1) Last(1)
#define FOR_EACH_ARGS_2(Macro, Sep, Last) FOR_EACH_ARGS_1(Macro, Sep, Sep) Macro(2) Last(2)
#define FOR_EACH_ARGS_3(Macro, Sep, Last) FOR_EACH_ARGS_2(Macro, Sep, Sep) Macro(3) Last(3)
#define FOR_EACH_ARGS_4(Macro, Sep, Last) FOR_EACH_ARGS_3(Macro, Sep, Sep) Macro(4) Last(4)

#define COMPUTE_INDEX(NbArg) NbArg
#define COMPUTE_OUTPARAM_RESULT(NbArg) OutParamToDataType<A ## NbArg>::result
#define COMPUTE_ARG_PROP(NbArg) (TypeToArgProperties<A ## NbArg>::result << (2 * (NbArg - 1)))
#define SEP_OR(_) |
#define NOTHING(_)

#define FUNCTION_INFO_STRUCT_BODY(ForEachNb)                                            \
    static inline DataType returnType() {                                               \
        return TypeToDataType<R>::result;                                               \
    }                                                                                   \
    static inline DataType outParam() {                                                 \
        return ForEachNb(NOTHING, NOTHING, COMPUTE_OUTPARAM_RESULT);                    \
    }                                                                                   \
    static inline size_t NbArgs() {                                                     \
        return ForEachNb(NOTHING, NOTHING, COMPUTE_INDEX);                              \
    }                                                                                   \
    static inline size_t explicitArgs() {                                               \
        return NbArgs() - (outParam() != Type_Void ? 1 : 0);                            \
    }                                                                                   \
    static inline uint32 argumentProperties() {                                         \
        return ForEachNb(COMPUTE_ARG_PROP, SEP_OR, NOTHING);                            \
    }                                                                                   \
    FunctionInfo(pf fun)                                                                \
        : VMFunction(JS_FUNC_TO_DATA_PTR(void *, fun), explicitArgs(),                  \
                     argumentProperties(), outParam(), returnType())                    \
    { }

template <typename Fun>
struct FunctionInfo {
};


template <class R>
struct FunctionInfo<R (*)(JSContext *)> : public VMFunction {
    typedef R (*pf)(JSContext *);

    static inline DataType returnType() {
        return TypeToDataType<R>::result;
    }
    static inline DataType outParam() {
        return Type_Void;
    }
    static inline size_t explicitArgs() {
        return 0;
    }
    static inline uint32 argumentProperties() {
        return 0;
    }
    FunctionInfo(pf fun)
      : VMFunction(JS_FUNC_TO_DATA_PTR(void *, fun), explicitArgs(),
                   argumentProperties(), outParam(), returnType())
    { }
};



template <class R, class A1>
struct FunctionInfo<R (*)(JSContext *, A1)> : public VMFunction {
    typedef R (*pf)(JSContext *, A1);
    FUNCTION_INFO_STRUCT_BODY(FOR_EACH_ARGS_1);
};

template <class R, class A1, class A2>
struct FunctionInfo<R (*)(JSContext *, A1, A2)> : public VMFunction {
    typedef R (*pf)(JSContext *, A1, A2);
    FUNCTION_INFO_STRUCT_BODY(FOR_EACH_ARGS_2);
};

template <class R, class A1, class A2, class A3>
struct FunctionInfo<R (*)(JSContext *, A1, A2, A3)> : public VMFunction {
    typedef R (*pf)(JSContext *, A1, A2, A3);
    FUNCTION_INFO_STRUCT_BODY(FOR_EACH_ARGS_3);
};

template <class R, class A1, class A2, class A3, class A4>
struct FunctionInfo<R (*)(JSContext *, A1, A2, A3, A4)> : public VMFunction {
    typedef R (*pf)(JSContext *, A1, A2, A3, A4);
    FUNCTION_INFO_STRUCT_BODY(FOR_EACH_ARGS_4);
};

#undef FUNCTION_INFO_STRUCT_BODY

#undef FOR_EACH_ARGS_4
#undef FOR_EACH_ARGS_3
#undef FOR_EACH_ARGS_2
#undef FOR_EACH_ARGS_1

#undef COMPUTE_INDEX
#undef COMPUTE_OUTPARAM_RESULT
#undef COMPUTE_ARG_PROP
#undef SEP_OR
#undef NOTHING

bool InvokeFunction(JSContext *cx, JSFunction *fun, uint32 argc, Value *argv, Value *rval);
bool InvokeConstructorFunction(JSContext *cx, JSFunction *fun, uint32 argc, Value *argv, Value *rval);
bool ReportOverRecursed(JSContext *cx);

bool DefVarOrConst(JSContext *cx, PropertyName *dn, unsigned attrs, JSObject *scopeChain);

template<bool Equal>
bool LooselyEqual(JSContext *cx, const Value &lhs, const Value &rhs, JSBool *res);

template<bool Equal>
bool StrictlyEqual(JSContext *cx, const Value &lhs, const Value &rhs, JSBool *res);

bool LessThan(JSContext *cx, const Value &lhs, const Value &rhs, JSBool *res);
bool LessThanOrEqual(JSContext *cx, const Value &lhs, const Value &rhs, JSBool *res);
bool GreaterThan(JSContext *cx, const Value &lhs, const Value &rhs, JSBool *res);
bool GreaterThanOrEqual(JSContext *cx, const Value &lhs, const Value &rhs, JSBool *res);

bool ValueToBooleanComplement(JSContext *cx, const Value &input, JSBool *output);

bool IteratorMore(JSContext *cx, JSObject *obj, JSBool *res);

bool CloseIteratorFromIon(JSContext *cx, JSObject *obj);

} 
} 

#endif 

