





#ifndef ion_VMFunctions_h
#define ion_VMFunctions_h

#include "jspubtd.h"

namespace js {

class DeclEnvObject;
class ForkJoinSlice;

namespace ion {

enum DataType {
    Type_Void,
    Type_Bool,
    Type_Int32,
    Type_Object,
    Type_Value,
    Type_Handle,
    Type_ParallelResult
};

struct PopValues
{
    uint32_t numValues;

    explicit PopValues(uint32_t numValues)
      : numValues(numValues)
    { }
};











struct VMFunction
{
    
    static VMFunction *functions;
    VMFunction *next;

    
    void *wrapped;

    
    
    uint32_t explicitArgs;

    enum ArgProperties {
        WordByValue = 0,
        DoubleByValue = 1,
        WordByRef = 2,
        DoubleByRef = 3,
        
        Word = 0,
        Double = 1,
        ByRef = 2
    };

    
    uint32_t argumentProperties;

    
    
    uint32_t argumentPassedInFloatRegs;

    
    
    
    DataType outParam;

    
    
    
    
    
    
    
    DataType returnType;

    
    enum RootType {
        RootNone = 0,
        RootObject,
        RootString,
        RootPropertyName,
        RootFunction,
        RootValue,
        RootCell
    };

    
    
    uint64_t argumentRootTypes;

    
    RootType outParamRootType;

    
    ExecutionMode executionMode;

    
    
    
    uint32_t extraValuesToPop;

    uint32_t argc() const {
        
        return 1 + explicitArgc() + ((outParam == Type_Void) ? 0 : 1);
    }

    DataType failType() const {
        return returnType;
    }

    ArgProperties argProperties(uint32_t explicitArg) const {
        return ArgProperties((argumentProperties >> (2 * explicitArg)) & 3);
    }

    RootType argRootType(uint32_t explicitArg) const {
        return RootType((argumentRootTypes >> (3 * explicitArg)) & 7);
    }

    bool argPassedInFloatReg(uint32_t explicitArg) const {
        return ((argumentPassedInFloatRegs >> explicitArg) & 1) == 1;
    }

    
    size_t explicitStackSlots() const {
        size_t stackSlots = explicitArgs;

        
        uint32_t n =
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

        
        uint32_t n =
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
        argumentPassedInFloatRegs(0),
        outParam(Type_Void),
        returnType(Type_Void),
        outParamRootType(RootNone),
        executionMode(SequentialExecution),
        extraValuesToPop(0)
    {
    }


    VMFunction(void *wrapped, uint32_t explicitArgs, uint32_t argumentProperties,
               uint32_t argumentPassedInFloatRegs, uint64_t argRootTypes,
               DataType outParam, RootType outParamRootType, DataType returnType,
               ExecutionMode executionMode, uint32_t extraValuesToPop = 0)
      : wrapped(wrapped),
        explicitArgs(explicitArgs),
        argumentProperties(argumentProperties),
        argumentPassedInFloatRegs(argumentPassedInFloatRegs),
        outParam(outParam),
        returnType(returnType),
        argumentRootTypes(argRootTypes),
        outParamRootType(outParamRootType),
        executionMode(executionMode),
        extraValuesToPop(extraValuesToPop)
    {
        
        JS_ASSERT_IF(outParam != Type_Void && executionMode == SequentialExecution,
                     returnType == Type_Bool);
        JS_ASSERT_IF(executionMode == ParallelExecution, returnType == Type_ParallelResult);
        JS_ASSERT(returnType == Type_Bool ||
                  returnType == Type_Object ||
                  returnType == Type_ParallelResult);
    }

    VMFunction(const VMFunction &o)
    {
        *this = o;
        addToFunctions();
    }

  private:
    
    void addToFunctions();
};

template <class> struct TypeToDataType {  };
template <> struct TypeToDataType<bool> { static const DataType result = Type_Bool; };
template <> struct TypeToDataType<JSObject *> { static const DataType result = Type_Object; };
template <> struct TypeToDataType<DeclEnvObject *> { static const DataType result = Type_Object; };
template <> struct TypeToDataType<JSString *> { static const DataType result = Type_Object; };
template <> struct TypeToDataType<JSFlatString *> { static const DataType result = Type_Object; };
template <> struct TypeToDataType<HandleObject> { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<HandleString> { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<HandlePropertyName> { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<HandleFunction> { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<Handle<StaticBlockObject *> > { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<HandleScript> { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<HandleValue> { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<MutableHandleValue> { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<ParallelResult> { static const DataType result = Type_ParallelResult; };


template <class T> struct TypeToArgProperties {
    static const uint32_t result =
        (sizeof(T) <= sizeof(void *) ? VMFunction::Word : VMFunction::Double);
};
template <> struct TypeToArgProperties<const Value &> {
    static const uint32_t result = TypeToArgProperties<Value>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<HandleObject> {
    static const uint32_t result = TypeToArgProperties<JSObject *>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<HandleString> {
    static const uint32_t result = TypeToArgProperties<JSString *>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<HandlePropertyName> {
    static const uint32_t result = TypeToArgProperties<PropertyName *>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<HandleFunction> {
    static const uint32_t result = TypeToArgProperties<JSFunction *>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<Handle<StaticBlockObject *> > {
    static const uint32_t result = TypeToArgProperties<StaticBlockObject *>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<HandleScript> {
    static const uint32_t result = TypeToArgProperties<JSScript *>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<HandleValue> {
    static const uint32_t result = TypeToArgProperties<Value>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<MutableHandleValue> {
    static const uint32_t result = TypeToArgProperties<Value>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<HandleShape> {
    static const uint32_t result = TypeToArgProperties<Shape *>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<HandleTypeObject> {
    static const uint32_t result = TypeToArgProperties<types::TypeObject *>::result | VMFunction::ByRef;
};



template <class T> struct TypeToPassInFloatReg {
    static const uint32_t result = 0;
};
template <> struct TypeToPassInFloatReg<double> {
    static const uint32_t result = 1;
};


template <class T> struct TypeToRootType {
    static const uint32_t result = VMFunction::RootNone;
};
template <> struct TypeToRootType<HandleObject> {
    static const uint32_t result = VMFunction::RootObject;
};
template <> struct TypeToRootType<HandleString> {
    static const uint32_t result = VMFunction::RootString;
};
template <> struct TypeToRootType<HandlePropertyName> {
    static const uint32_t result = VMFunction::RootPropertyName;
};
template <> struct TypeToRootType<HandleFunction> {
    static const uint32_t result = VMFunction::RootFunction;
};
template <> struct TypeToRootType<HandleValue> {
    static const uint32_t result = VMFunction::RootValue;
};
template <> struct TypeToRootType<MutableHandleValue> {
    static const uint32_t result = VMFunction::RootValue;
};
template <> struct TypeToRootType<HandleShape> {
    static const uint32_t result = VMFunction::RootCell;
};
template <> struct TypeToRootType<HandleTypeObject> {
    static const uint32_t result = VMFunction::RootCell;
};

template <class> struct OutParamToDataType { static const DataType result = Type_Void; };
template <> struct OutParamToDataType<Value *> { static const DataType result = Type_Value; };
template <> struct OutParamToDataType<int *> { static const DataType result = Type_Int32; };
template <> struct OutParamToDataType<uint32_t *> { static const DataType result = Type_Int32; };
template <> struct OutParamToDataType<MutableHandleValue> { static const DataType result = Type_Handle; };
template <> struct OutParamToDataType<MutableHandleObject> { static const DataType result = Type_Handle; };

template <class> struct OutParamToRootType {
    static const VMFunction::RootType result = VMFunction::RootNone;
};
template <> struct OutParamToRootType<MutableHandleValue> {
    static const VMFunction::RootType result = VMFunction::RootValue;
};
template <> struct OutParamToRootType<MutableHandleObject> {
    static const VMFunction::RootType result = VMFunction::RootObject;
};
template <> struct OutParamToRootType<MutableHandleString> {
    static const VMFunction::RootType result = VMFunction::RootString;
};

template <class> struct MatchContext { };
template <> struct MatchContext<JSContext *> {
    static const ExecutionMode execMode = SequentialExecution;
};
template <> struct MatchContext<ForkJoinSlice *> {
    static const ExecutionMode execMode = ParallelExecution;
};

#define FOR_EACH_ARGS_1(Macro, Sep, Last) Macro(1) Last(1)
#define FOR_EACH_ARGS_2(Macro, Sep, Last) FOR_EACH_ARGS_1(Macro, Sep, Sep) Macro(2) Last(2)
#define FOR_EACH_ARGS_3(Macro, Sep, Last) FOR_EACH_ARGS_2(Macro, Sep, Sep) Macro(3) Last(3)
#define FOR_EACH_ARGS_4(Macro, Sep, Last) FOR_EACH_ARGS_3(Macro, Sep, Sep) Macro(4) Last(4)
#define FOR_EACH_ARGS_5(Macro, Sep, Last) FOR_EACH_ARGS_4(Macro, Sep, Sep) Macro(5) Last(5)
#define FOR_EACH_ARGS_6(Macro, Sep, Last) FOR_EACH_ARGS_5(Macro, Sep, Sep) Macro(6) Last(6)

#define COMPUTE_INDEX(NbArg) NbArg
#define COMPUTE_OUTPARAM_RESULT(NbArg) OutParamToDataType<A ## NbArg>::result
#define COMPUTE_OUTPARAM_ROOT(NbArg) OutParamToRootType<A ## NbArg>::result
#define COMPUTE_ARG_PROP(NbArg) (TypeToArgProperties<A ## NbArg>::result << (2 * (NbArg - 1)))
#define COMPUTE_ARG_ROOT(NbArg) (uint64_t(TypeToRootType<A ## NbArg>::result) << (3 * (NbArg - 1)))
#define COMPUTE_ARG_FLOAT(NbArg) (TypeToPassInFloatReg<A ## NbArg>::result) << (NbArg - 1)
#define SEP_OR(_) |
#define NOTHING(_)

#define FUNCTION_INFO_STRUCT_BODY(ForEachNb)                                            \
    static inline ExecutionMode executionMode() {                                       \
        return MatchContext<Context>::execMode;                                         \
    }                                                                                   \
    static inline DataType returnType() {                                               \
        return TypeToDataType<R>::result;                                               \
    }                                                                                   \
    static inline DataType outParam() {                                                 \
        return ForEachNb(NOTHING, NOTHING, COMPUTE_OUTPARAM_RESULT);                    \
    }                                                                                   \
    static inline RootType outParamRootType() {                                         \
        return ForEachNb(NOTHING, NOTHING, COMPUTE_OUTPARAM_ROOT);                      \
    }                                                                                   \
    static inline size_t NbArgs() {                                                     \
        return ForEachNb(NOTHING, NOTHING, COMPUTE_INDEX);                              \
    }                                                                                   \
    static inline size_t explicitArgs() {                                               \
        return NbArgs() - (outParam() != Type_Void ? 1 : 0);                            \
    }                                                                                   \
    static inline uint32_t argumentProperties() {                                       \
        return ForEachNb(COMPUTE_ARG_PROP, SEP_OR, NOTHING);                            \
    }                                                                                   \
    static inline uint32_t argumentPassedInFloatRegs() {                                \
        return ForEachNb(COMPUTE_ARG_FLOAT, SEP_OR, NOTHING);                           \
    }                                                                                   \
    static inline uint64_t argumentRootTypes() {                                        \
        return ForEachNb(COMPUTE_ARG_ROOT, SEP_OR, NOTHING);                            \
    }                                                                                   \
    FunctionInfo(pf fun, PopValues extraValuesToPop = PopValues(0))                     \
        : VMFunction(JS_FUNC_TO_DATA_PTR(void *, fun), explicitArgs(),                  \
                     argumentProperties(), argumentPassedInFloatRegs(),                 \
                     argumentRootTypes(), outParam(), outParamRootType(),               \
                     returnType(), executionMode(),                                     \
                     extraValuesToPop.numValues)                                        \
    { }

template <typename Fun>
struct FunctionInfo {
};


template <class R, class Context>
struct FunctionInfo<R (*)(Context)> : public VMFunction {
    typedef R (*pf)(Context);

    static inline ExecutionMode executionMode() {
        return MatchContext<Context>::execMode;
    }
    static inline DataType returnType() {
        return TypeToDataType<R>::result;
    }
    static inline DataType outParam() {
        return Type_Void;
    }
    static inline RootType outParamRootType() {
        return RootNone;
    }
    static inline size_t explicitArgs() {
        return 0;
    }
    static inline uint32_t argumentProperties() {
        return 0;
    }
    static inline uint32_t argumentPassedInFloatRegs() {
        return 0;
    }
    static inline uint64_t argumentRootTypes() {
        return 0;
    }
    FunctionInfo(pf fun)
      : VMFunction(JS_FUNC_TO_DATA_PTR(void *, fun), explicitArgs(),
                   argumentProperties(), argumentPassedInFloatRegs(),
                   argumentRootTypes(), outParam(), outParamRootType(),
                   returnType(), executionMode())
    { }
};



template <class R, class Context, class A1>
struct FunctionInfo<R (*)(Context, A1)> : public VMFunction {
    typedef R (*pf)(Context, A1);
    FUNCTION_INFO_STRUCT_BODY(FOR_EACH_ARGS_1)
};

template <class R, class Context, class A1, class A2>
struct FunctionInfo<R (*)(Context, A1, A2)> : public VMFunction {
    typedef R (*pf)(Context, A1, A2);
    FUNCTION_INFO_STRUCT_BODY(FOR_EACH_ARGS_2)
};

template <class R, class Context, class A1, class A2, class A3>
struct FunctionInfo<R (*)(Context, A1, A2, A3)> : public VMFunction {
    typedef R (*pf)(Context, A1, A2, A3);
    FUNCTION_INFO_STRUCT_BODY(FOR_EACH_ARGS_3)
};

template <class R, class Context, class A1, class A2, class A3, class A4>
struct FunctionInfo<R (*)(Context, A1, A2, A3, A4)> : public VMFunction {
    typedef R (*pf)(Context, A1, A2, A3, A4);
    FUNCTION_INFO_STRUCT_BODY(FOR_EACH_ARGS_4)
};

template <class R, class Context, class A1, class A2, class A3, class A4, class A5>
    struct FunctionInfo<R (*)(Context, A1, A2, A3, A4, A5)> : public VMFunction {
    typedef R (*pf)(Context, A1, A2, A3, A4, A5);
    FUNCTION_INFO_STRUCT_BODY(FOR_EACH_ARGS_5)
};

template <class R, class Context, class A1, class A2, class A3, class A4, class A5, class A6>
    struct FunctionInfo<R (*)(Context, A1, A2, A3, A4, A5, A6)> : public VMFunction {
    typedef R (*pf)(Context, A1, A2, A3, A4, A5, A6);
    FUNCTION_INFO_STRUCT_BODY(FOR_EACH_ARGS_6)
};

#undef FUNCTION_INFO_STRUCT_BODY

#undef FOR_EACH_ARGS_6
#undef FOR_EACH_ARGS_5
#undef FOR_EACH_ARGS_4
#undef FOR_EACH_ARGS_3
#undef FOR_EACH_ARGS_2
#undef FOR_EACH_ARGS_1

#undef COMPUTE_INDEX
#undef COMPUTE_OUTPARAM_RESULT
#undef COMPUTE_OUTPARAM_ROOT
#undef COMPUTE_ARG_PROP
#undef COMPUTE_ARG_FLOAT
#undef SEP_OR
#undef NOTHING

class AutoDetectInvalidation
{
    JSContext *cx_;
    IonScript *ionScript_;
    Value *rval_;
    bool disabled_;

  public:
    AutoDetectInvalidation(JSContext *cx, Value *rval, IonScript *ionScript = NULL)
      : cx_(cx),
        ionScript_(ionScript ? ionScript : GetTopIonJSScript(cx)->ionScript()),
        rval_(rval),
        disabled_(false)
    { }

    void disable() {
        JS_ASSERT(!disabled_);
        disabled_ = true;
    }

    ~AutoDetectInvalidation() {
        if (!disabled_ && ionScript_->invalidated())
            cx_->runtime()->setIonReturnOverride(*rval_);
    }
};

bool InvokeFunction(JSContext *cx, HandleFunction fun0, uint32_t argc, Value *argv, Value *rval);
JSObject *NewGCThing(JSContext *cx, gc::AllocKind allocKind, size_t thingSize);

bool CheckOverRecursed(JSContext *cx);

bool DefVarOrConst(JSContext *cx, HandlePropertyName dn, unsigned attrs, HandleObject scopeChain);
bool SetConst(JSContext *cx, HandlePropertyName name, HandleObject scopeChain, HandleValue rval);
bool InitProp(JSContext *cx, HandleObject obj, HandlePropertyName name, HandleValue value);

template<bool Equal>
bool LooselyEqual(JSContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, JSBool *res);

template<bool Equal>
bool StrictlyEqual(JSContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, JSBool *res);

bool LessThan(JSContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, JSBool *res);
bool LessThanOrEqual(JSContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, JSBool *res);
bool GreaterThan(JSContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, JSBool *res);
bool GreaterThanOrEqual(JSContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, JSBool *res);

template<bool Equal>
bool StringsEqual(JSContext *cx, HandleString left, HandleString right, JSBool *res);

JSBool ObjectEmulatesUndefined(JSObject *obj);

bool IteratorMore(JSContext *cx, HandleObject obj, JSBool *res);


JSObject *NewInitParallelArray(JSContext *cx, HandleObject templateObj);
JSObject *NewInitArray(JSContext *cx, uint32_t count, types::TypeObject *type);
JSObject *NewInitObject(JSContext *cx, HandleObject templateObject);
JSObject *NewInitObjectWithClassPrototype(JSContext *cx, HandleObject templateObject);

bool ArrayPopDense(JSContext *cx, HandleObject obj, MutableHandleValue rval);
bool ArrayPushDense(JSContext *cx, HandleObject obj, HandleValue v, uint32_t *length);
bool ArrayShiftDense(JSContext *cx, HandleObject obj, MutableHandleValue rval);
JSObject *ArrayConcatDense(JSContext *cx, HandleObject obj1, HandleObject obj2, HandleObject res);

bool CharCodeAt(JSContext *cx, HandleString str, int32_t index, uint32_t *code);
JSFlatString *StringFromCharCode(JSContext *cx, int32_t code);

bool SetProperty(JSContext *cx, HandleObject obj, HandlePropertyName name, HandleValue value,
                 bool strict, int jsop);

bool InterruptCheck(JSContext *cx);

HeapSlot *NewSlots(JSRuntime *rt, unsigned nslots);
JSObject *NewCallObject(JSContext *cx, HandleScript script,
                        HandleShape shape, HandleTypeObject type, HeapSlot *slots);
JSObject *NewStringObject(JSContext *cx, HandleString str);

bool SPSEnter(JSContext *cx, HandleScript script);
bool SPSExit(JSContext *cx, HandleScript script);

bool OperatorIn(JSContext *cx, HandleValue key, HandleObject obj, JSBool *out);
bool OperatorInI(JSContext *cx, uint32_t index, HandleObject obj, JSBool *out);

bool GetIntrinsicValue(JSContext *cx, HandlePropertyName name, MutableHandleValue rval);

bool CreateThis(JSContext *cx, HandleObject callee, MutableHandleValue rval);

void GetDynamicName(JSContext *cx, JSObject *scopeChain, JSString *str, Value *vp);

JSBool FilterArguments(JSContext *cx, JSString *str);

#ifdef JSGC_GENERATIONAL
void PostWriteBarrier(JSRuntime *rt, JSObject *obj);
#endif

uint32_t GetIndexFromString(JSString *str);

bool DebugPrologue(JSContext *cx, BaselineFrame *frame, JSBool *mustReturn);
bool DebugEpilogue(JSContext *cx, BaselineFrame *frame, JSBool ok);

bool StrictEvalPrologue(JSContext *cx, BaselineFrame *frame);
bool HeavyweightFunPrologue(JSContext *cx, BaselineFrame *frame);

bool NewArgumentsObject(JSContext *cx, BaselineFrame *frame, MutableHandleValue res);

JSObject *InitRestParameter(JSContext *cx, uint32_t length, Value *rest, HandleObject templateObj,
                            HandleObject res);

bool HandleDebugTrap(JSContext *cx, BaselineFrame *frame, uint8_t *retAddr, JSBool *mustReturn);
bool OnDebuggerStatement(JSContext *cx, BaselineFrame *frame, jsbytecode *pc, JSBool *mustReturn);

bool EnterBlock(JSContext *cx, BaselineFrame *frame, Handle<StaticBlockObject *> block);
bool LeaveBlock(JSContext *cx, BaselineFrame *frame);

bool InitBaselineFrameForOsr(BaselineFrame *frame, StackFrame *interpFrame,
                             uint32_t numStackValues);

} 
} 

#endif
