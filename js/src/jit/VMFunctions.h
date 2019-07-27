





#ifndef jit_VMFunctions_h
#define jit_VMFunctions_h

#include "mozilla/Attributes.h"

#include "jspubtd.h"

#include "jit/CompileInfo.h"
#include "jit/JitFrames.h"

namespace js {

class DeclEnvObject;
class StaticWithObject;
class InlineTypedObject;
class GeneratorObject;

namespace jit {

enum DataType {
    Type_Void,
    Type_Bool,
    Type_Int32,
    Type_Double,
    Type_Pointer,
    Type_Object,
    Type_Value,
    Type_Handle
};

struct PopValues
{
    uint32_t numValues;

    explicit PopValues(uint32_t numValues)
      : numValues(numValues)
    { }
};

enum MaybeTailCall {
    TailCall,
    NonTailCall
};












struct VMFunction
{
    
    static VMFunction* functions;
    VMFunction* next;

    
    void* wrapped;

    
    
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

    
    
    
    uint32_t extraValuesToPop;

    
    
    
    MaybeTailCall expectTailCall;

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

    size_t doubleByRefArgs() const {
        size_t count = 0;

        
        uint32_t n =
            ((1 << (explicitArgs * 2)) - 1) 
            & argumentProperties;

        
        
        n = (n & 0x55555555) & (n >> 1);

        
        
        while (n) {
            count++;
            n &= n - 1;
        }
        return count;
    }

    VMFunction()
      : wrapped(nullptr),
        explicitArgs(0),
        argumentProperties(0),
        argumentPassedInFloatRegs(0),
        outParam(Type_Void),
        returnType(Type_Void),
        outParamRootType(RootNone),
        extraValuesToPop(0)
    {
    }


    VMFunction(void* wrapped, uint32_t explicitArgs, uint32_t argumentProperties,
               uint32_t argumentPassedInFloatRegs, uint64_t argRootTypes,
               DataType outParam, RootType outParamRootType, DataType returnType,
               uint32_t extraValuesToPop = 0, MaybeTailCall expectTailCall = NonTailCall)
      : wrapped(wrapped),
        explicitArgs(explicitArgs),
        argumentProperties(argumentProperties),
        argumentPassedInFloatRegs(argumentPassedInFloatRegs),
        outParam(outParam),
        returnType(returnType),
        argumentRootTypes(argRootTypes),
        outParamRootType(outParamRootType),
        extraValuesToPop(extraValuesToPop),
        expectTailCall(expectTailCall)
    {
        
        MOZ_ASSERT_IF(outParam != Type_Void, returnType == Type_Bool);
        MOZ_ASSERT(returnType == Type_Bool ||
                   returnType == Type_Object);
    }

    VMFunction(const VMFunction& o) {
        *this = o;
        addToFunctions();
    }

  private:
    
    void addToFunctions();
};

template <class> struct TypeToDataType {  };
template <> struct TypeToDataType<bool> { static const DataType result = Type_Bool; };
template <> struct TypeToDataType<JSObject*> { static const DataType result = Type_Object; };
template <> struct TypeToDataType<NativeObject*> { static const DataType result = Type_Object; };
template <> struct TypeToDataType<PlainObject*> { static const DataType result = Type_Object; };
template <> struct TypeToDataType<InlineTypedObject*> { static const DataType result = Type_Object; };
template <> struct TypeToDataType<DeclEnvObject*> { static const DataType result = Type_Object; };
template <> struct TypeToDataType<ArrayObject*> { static const DataType result = Type_Object; };
template <> struct TypeToDataType<JSString*> { static const DataType result = Type_Object; };
template <> struct TypeToDataType<JSFlatString*> { static const DataType result = Type_Object; };
template <> struct TypeToDataType<HandleObject> { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<HandleString> { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<HandlePropertyName> { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<HandleFunction> { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<Handle<NativeObject*> > { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<Handle<InlineTypedObject*> > { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<Handle<ArrayObject*> > { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<Handle<GeneratorObject*> > { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<Handle<PlainObject*> > { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<Handle<StaticWithObject*> > { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<Handle<StaticBlockObject*> > { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<HandleScript> { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<HandleValue> { static const DataType result = Type_Handle; };
template <> struct TypeToDataType<MutableHandleValue> { static const DataType result = Type_Handle; };


template <class T> struct TypeToArgProperties {
    static const uint32_t result =
        (sizeof(T) <= sizeof(void*) ? VMFunction::Word : VMFunction::Double);
};
template <> struct TypeToArgProperties<const Value&> {
    static const uint32_t result = TypeToArgProperties<Value>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<HandleObject> {
    static const uint32_t result = TypeToArgProperties<JSObject*>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<HandleString> {
    static const uint32_t result = TypeToArgProperties<JSString*>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<HandlePropertyName> {
    static const uint32_t result = TypeToArgProperties<PropertyName*>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<HandleFunction> {
    static const uint32_t result = TypeToArgProperties<JSFunction*>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<Handle<NativeObject*> > {
    static const uint32_t result = TypeToArgProperties<NativeObject*>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<Handle<InlineTypedObject*> > {
    static const uint32_t result = TypeToArgProperties<InlineTypedObject*>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<Handle<ArrayObject*> > {
    static const uint32_t result = TypeToArgProperties<ArrayObject*>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<Handle<GeneratorObject*> > {
    static const uint32_t result = TypeToArgProperties<GeneratorObject*>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<Handle<PlainObject*> > {
    static const uint32_t result = TypeToArgProperties<PlainObject*>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<Handle<StaticWithObject*> > {
    static const uint32_t result = TypeToArgProperties<StaticWithObject*>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<Handle<StaticBlockObject*> > {
    static const uint32_t result = TypeToArgProperties<StaticBlockObject*>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<HandleScript> {
    static const uint32_t result = TypeToArgProperties<JSScript*>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<HandleValue> {
    static const uint32_t result = TypeToArgProperties<Value>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<MutableHandleValue> {
    static const uint32_t result = TypeToArgProperties<Value>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<HandleShape> {
    static const uint32_t result = TypeToArgProperties<Shape*>::result | VMFunction::ByRef;
};
template <> struct TypeToArgProperties<HandleObjectGroup> {
    static const uint32_t result = TypeToArgProperties<ObjectGroup*>::result | VMFunction::ByRef;
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
template <> struct TypeToRootType<HandleObjectGroup> {
    static const uint32_t result = VMFunction::RootCell;
};
template <> struct TypeToRootType<HandleScript> {
    static const uint32_t result = VMFunction::RootCell;
};
template <> struct TypeToRootType<Handle<NativeObject*> > {
    static const uint32_t result = VMFunction::RootObject;
};
template <> struct TypeToRootType<Handle<InlineTypedObject*> > {
    static const uint32_t result = VMFunction::RootObject;
};
template <> struct TypeToRootType<Handle<ArrayObject*> > {
    static const uint32_t result = VMFunction::RootObject;
};
template <> struct TypeToRootType<Handle<GeneratorObject*> > {
    static const uint32_t result = VMFunction::RootObject;
};
template <> struct TypeToRootType<Handle<PlainObject*> > {
    static const uint32_t result = VMFunction::RootObject;
};
template <> struct TypeToRootType<Handle<StaticBlockObject*> > {
    static const uint32_t result = VMFunction::RootObject;
};
template <> struct TypeToRootType<Handle<StaticWithObject*> > {
    static const uint32_t result = VMFunction::RootCell;
};
template <class T> struct TypeToRootType<Handle<T> > {
    
};

template <class> struct OutParamToDataType { static const DataType result = Type_Void; };
template <> struct OutParamToDataType<Value*> { static const DataType result = Type_Value; };
template <> struct OutParamToDataType<int*> { static const DataType result = Type_Int32; };
template <> struct OutParamToDataType<uint32_t*> { static const DataType result = Type_Int32; };
template <> struct OutParamToDataType<uint8_t**> { static const DataType result = Type_Pointer; };
template <> struct OutParamToDataType<bool*> { static const DataType result = Type_Bool; };
template <> struct OutParamToDataType<double*> { static const DataType result = Type_Double; };
template <> struct OutParamToDataType<MutableHandleValue> { static const DataType result = Type_Handle; };
template <> struct OutParamToDataType<MutableHandleObject> { static const DataType result = Type_Handle; };
template <> struct OutParamToDataType<MutableHandleString> { static const DataType result = Type_Handle; };

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
template <> struct MatchContext<JSContext*> {
    static const bool valid = true;
};
template <> struct MatchContext<ExclusiveContext*> {
    static const bool valid = true;
};


template <typename... ArgTypes>
struct LastArg;

template <>
struct LastArg<>
{
    typedef void Type;
    static MOZ_CONSTEXPR_VAR size_t nbArgs = 0;
};

template <typename HeadType>
struct LastArg<HeadType>
{
    typedef HeadType Type;
    static MOZ_CONSTEXPR_VAR size_t nbArgs = 1;
};

template <typename HeadType, typename... TailTypes>
struct LastArg<HeadType, TailTypes...>
{
    typedef typename LastArg<TailTypes...>::Type Type;
    static MOZ_CONSTEXPR_VAR size_t nbArgs = LastArg<TailTypes...>::nbArgs + 1;
};






template <template<typename> class Each, typename ResultType, size_t Shift,
          typename... Args>
struct BitMask;

template <template<typename> class Each, typename ResultType, size_t Shift>
struct BitMask<Each, ResultType, Shift>
{
    static MOZ_CONSTEXPR_VAR ResultType result = ResultType();
};

template <template<typename> class Each, typename ResultType, size_t Shift,
          typename HeadType, typename... TailTypes>
struct BitMask<Each, ResultType, Shift, HeadType, TailTypes...>
{
    static_assert(ResultType(Each<HeadType>::result) < (1 << Shift),
                  "not enough bits reserved by the shift for individual results");
    static_assert(LastArg<TailTypes...>::nbArgs < (8 * sizeof(ResultType) / Shift),
                  "not enough bits in the result type to store all bit masks");

    static MOZ_CONSTEXPR_VAR ResultType result =
        ResultType(Each<HeadType>::result) |
        (BitMask<Each, ResultType, Shift, TailTypes...>::result << Shift);
};




template <typename... Args>
struct FunctionInfo;

template <class R, class Context, typename... Args>
struct FunctionInfo<R (*)(Context, Args...)> : public VMFunction
{
    typedef R (*pf)(Context, Args...);

    static inline DataType returnType() {
        return TypeToDataType<R>::result;
    }
    static inline DataType outParam() {
        return OutParamToDataType<typename LastArg<Args...>::Type>::result;
    }
    static inline RootType outParamRootType() {
        return OutParamToRootType<typename LastArg<Args...>::Type>::result;
    }
    static inline size_t NbArgs() {
        return LastArg<Args...>::nbArgs;
    }
    static inline size_t explicitArgs() {
        return NbArgs() - (outParam() != Type_Void ? 1 : 0);
    }
    static inline uint32_t argumentProperties() {
        return BitMask<TypeToArgProperties, uint32_t, 2, Args...>::result;
    }
    static inline uint32_t argumentPassedInFloatRegs() {
        return BitMask<TypeToPassInFloatReg, uint32_t, 2, Args...>::result;
    }
    static inline uint64_t argumentRootTypes() {
        return BitMask<TypeToRootType, uint64_t, 3, Args...>::result;
    }
    explicit FunctionInfo(pf fun, PopValues extraValuesToPop = PopValues(0))
        : VMFunction(JS_FUNC_TO_DATA_PTR(void*, fun), explicitArgs(),
                     argumentProperties(), argumentPassedInFloatRegs(),
                     argumentRootTypes(), outParam(), outParamRootType(),
                     returnType(), extraValuesToPop.numValues, NonTailCall)
    {
        static_assert(MatchContext<Context>::valid, "Invalid cx type in VMFunction");
    }
    explicit FunctionInfo(pf fun, MaybeTailCall expectTailCall,
                          PopValues extraValuesToPop = PopValues(0))
        : VMFunction(JS_FUNC_TO_DATA_PTR(void*, fun), explicitArgs(),
                     argumentProperties(), argumentPassedInFloatRegs(),
                     argumentRootTypes(), outParam(), outParamRootType(),
                     returnType(), extraValuesToPop.numValues, expectTailCall)
    {
        static_assert(MatchContext<Context>::valid, "Invalid cx type in VMFunction");
    }
};

class AutoDetectInvalidation
{
    JSContext* cx_;
    IonScript* ionScript_;
    MutableHandleValue rval_;
    bool disabled_;

    void setReturnOverride();

  public:
    AutoDetectInvalidation(JSContext* cx, MutableHandleValue rval, IonScript* ionScript)
      : cx_(cx), ionScript_(ionScript), rval_(rval), disabled_(false)
    {
        MOZ_ASSERT(ionScript);
    }

    AutoDetectInvalidation(JSContext* cx, MutableHandleValue rval);

    void disable() {
        MOZ_ASSERT(!disabled_);
        disabled_ = true;
    }

    ~AutoDetectInvalidation() {
        if (!disabled_ && ionScript_->invalidated())
            setReturnOverride();
    }
};

bool InvokeFunction(JSContext* cx, HandleObject obj0, bool constructing, uint32_t argc,
                    Value* argv, MutableHandleValue rval);
bool InvokeFunctionShuffleNewTarget(JSContext* cx, HandleObject obj, uint32_t numActualArgs,
                                    uint32_t numFormalArgs, Value* argv, MutableHandleValue rval);

bool CheckOverRecursed(JSContext* cx);
bool CheckOverRecursedWithExtra(JSContext* cx, BaselineFrame* frame,
                                uint32_t extra, uint32_t earlyCheck);

bool DefVarOrConst(JSContext* cx, HandlePropertyName dn, unsigned attrs, HandleObject scopeChain);
bool SetConst(JSContext* cx, HandlePropertyName name, HandleObject scopeChain, HandleValue rval);
bool MutatePrototype(JSContext* cx, HandlePlainObject obj, HandleValue value);
bool InitProp(JSContext* cx, HandleObject obj, HandlePropertyName name, HandleValue value,
              jsbytecode* pc);

template<bool Equal>
bool LooselyEqual(JSContext* cx, MutableHandleValue lhs, MutableHandleValue rhs, bool* res);

template<bool Equal>
bool StrictlyEqual(JSContext* cx, MutableHandleValue lhs, MutableHandleValue rhs, bool* res);

bool LessThan(JSContext* cx, MutableHandleValue lhs, MutableHandleValue rhs, bool* res);
bool LessThanOrEqual(JSContext* cx, MutableHandleValue lhs, MutableHandleValue rhs, bool* res);
bool GreaterThan(JSContext* cx, MutableHandleValue lhs, MutableHandleValue rhs, bool* res);
bool GreaterThanOrEqual(JSContext* cx, MutableHandleValue lhs, MutableHandleValue rhs, bool* res);

template<bool Equal>
bool StringsEqual(JSContext* cx, HandleString left, HandleString right, bool* res);

bool ArrayPopDense(JSContext* cx, HandleObject obj, MutableHandleValue rval);
bool ArrayPushDense(JSContext* cx, HandleObject obj, HandleValue v, uint32_t* length);
bool ArrayShiftDense(JSContext* cx, HandleObject obj, MutableHandleValue rval);
JSObject* ArrayConcatDense(JSContext* cx, HandleObject obj1, HandleObject obj2, HandleObject res);
JSString* ArrayJoin(JSContext* cx, HandleObject array, HandleString sep);

bool CharCodeAt(JSContext* cx, HandleString str, int32_t index, uint32_t* code);
JSFlatString* StringFromCharCode(JSContext* cx, int32_t code);

bool SetProperty(JSContext* cx, HandleObject obj, HandlePropertyName name, HandleValue value,
                 bool strict, jsbytecode* pc);

bool InterruptCheck(JSContext* cx);

void* MallocWrapper(JSRuntime* rt, size_t nbytes);
JSObject* NewCallObject(JSContext* cx, HandleShape shape, HandleObjectGroup group,
                        uint32_t lexicalBegin);
JSObject* NewSingletonCallObject(JSContext* cx, HandleShape shape, uint32_t lexicalBegin);
JSObject* NewStringObject(JSContext* cx, HandleString str);

bool OperatorIn(JSContext* cx, HandleValue key, HandleObject obj, bool* out);
bool OperatorInI(JSContext* cx, uint32_t index, HandleObject obj, bool* out);

bool GetIntrinsicValue(JSContext* cx, HandlePropertyName name, MutableHandleValue rval);

bool CreateThis(JSContext* cx, HandleObject callee, MutableHandleValue rval);

void GetDynamicName(JSContext* cx, JSObject* scopeChain, JSString* str, Value* vp);

bool FilterArgumentsOrEval(JSContext* cx, JSString* str);

void PostWriteBarrier(JSRuntime* rt, JSObject* obj);
void PostGlobalWriteBarrier(JSRuntime* rt, JSObject* obj);

uint32_t GetIndexFromString(JSString* str);

bool DebugPrologue(JSContext* cx, BaselineFrame* frame, jsbytecode* pc, bool* mustReturn);
bool DebugEpilogue(JSContext* cx, BaselineFrame* frame, jsbytecode* pc, bool ok);
bool DebugEpilogueOnBaselineReturn(JSContext* cx, BaselineFrame* frame, jsbytecode* pc);
void FrameIsDebuggeeCheck(BaselineFrame* frame);

JSObject* CreateGenerator(JSContext* cx, BaselineFrame* frame);
bool NormalSuspend(JSContext* cx, HandleObject obj, BaselineFrame* frame, jsbytecode* pc,
                   uint32_t stackDepth);
bool FinalSuspend(JSContext* cx, HandleObject obj, BaselineFrame* frame, jsbytecode* pc);
bool InterpretResume(JSContext* cx, HandleObject obj, HandleValue val, HandlePropertyName kind,
                     MutableHandleValue rval);
bool DebugAfterYield(JSContext* cx, BaselineFrame* frame);
bool GeneratorThrowOrClose(JSContext* cx, BaselineFrame* frame, Handle<GeneratorObject*> genObj,
                           HandleValue arg, uint32_t resumeKind);

bool StrictEvalPrologue(JSContext* cx, BaselineFrame* frame);
bool HeavyweightFunPrologue(JSContext* cx, BaselineFrame* frame);

bool NewArgumentsObject(JSContext* cx, BaselineFrame* frame, MutableHandleValue res);

JSObject* InitRestParameter(JSContext* cx, uint32_t length, Value* rest, HandleObject templateObj,
                            HandleObject res);

bool HandleDebugTrap(JSContext* cx, BaselineFrame* frame, uint8_t* retAddr, bool* mustReturn);
bool OnDebuggerStatement(JSContext* cx, BaselineFrame* frame, jsbytecode* pc, bool* mustReturn);
bool GlobalHasLiveOnDebuggerStatement(JSContext* cx);

bool EnterWith(JSContext* cx, BaselineFrame* frame, HandleValue val,
               Handle<StaticWithObject*> templ);
bool LeaveWith(JSContext* cx, BaselineFrame* frame);

bool PushBlockScope(JSContext* cx, BaselineFrame* frame, Handle<StaticBlockObject*> block);
bool PopBlockScope(JSContext* cx, BaselineFrame* frame);
bool DebugLeaveThenPopBlockScope(JSContext* cx, BaselineFrame* frame, jsbytecode* pc);
bool FreshenBlockScope(JSContext* cx, BaselineFrame* frame);
bool DebugLeaveThenFreshenBlockScope(JSContext* cx, BaselineFrame* frame, jsbytecode* pc);
bool DebugLeaveBlock(JSContext* cx, BaselineFrame* frame, jsbytecode* pc);

bool InitBaselineFrameForOsr(BaselineFrame* frame, InterpreterFrame* interpFrame,
                             uint32_t numStackValues);

JSObject* CreateDerivedTypedObj(JSContext* cx, HandleObject descr,
                                HandleObject owner, int32_t offset);

bool ArraySpliceDense(JSContext* cx, HandleObject obj, uint32_t start, uint32_t deleteCount);

bool Recompile(JSContext* cx);
bool ForcedRecompile(JSContext* cx);
JSString* RegExpReplace(JSContext* cx, HandleString string, HandleObject regexp,
                        HandleString repl);
JSString* StringReplace(JSContext* cx, HandleString string, HandleString pattern,
                        HandleString repl);

bool SetDenseOrUnboxedArrayElement(JSContext* cx, HandleObject obj, int32_t index,
                                   HandleValue value, bool strict);

void AssertValidObjectPtr(JSContext* cx, JSObject* obj);
void AssertValidObjectOrNullPtr(JSContext* cx, JSObject* obj);
void AssertValidStringPtr(JSContext* cx, JSString* str);
void AssertValidSymbolPtr(JSContext* cx, JS::Symbol* sym);
void AssertValidValue(JSContext* cx, Value* v);

void MarkValueFromIon(JSRuntime* rt, Value* vp);
void MarkStringFromIon(JSRuntime* rt, JSString** stringp);
void MarkObjectFromIon(JSRuntime* rt, JSObject** objp);
void MarkShapeFromIon(JSRuntime* rt, Shape** shapep);
void MarkObjectGroupFromIon(JSRuntime* rt, ObjectGroup** groupp);


inline void*
IonMarkFunction(MIRType type)
{
    switch (type) {
      case MIRType_Value:
        return JS_FUNC_TO_DATA_PTR(void*, MarkValueFromIon);
      case MIRType_String:
        return JS_FUNC_TO_DATA_PTR(void*, MarkStringFromIon);
      case MIRType_Object:
        return JS_FUNC_TO_DATA_PTR(void*, MarkObjectFromIon);
      case MIRType_Shape:
        return JS_FUNC_TO_DATA_PTR(void*, MarkShapeFromIon);
      case MIRType_ObjectGroup:
        return JS_FUNC_TO_DATA_PTR(void*, MarkObjectGroupFromIon);
      default: MOZ_CRASH();
    }
}

bool ObjectIsCallable(JSObject* obj);

bool ThrowUninitializedLexical(JSContext* cx);

} 
} 

#endif 
