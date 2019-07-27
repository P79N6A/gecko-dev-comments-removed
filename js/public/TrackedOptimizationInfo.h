





#ifndef js_TrackedOptimizationInfo_h
#define js_TrackedOptimizationInfo_h

namespace JS {

#define TRACKED_STRATEGY_LIST(_)                        \
    _(GetProp_ArgumentsLength,                          \
      "getprop arguments.length")                       \
    _(GetProp_ArgumentsCallee,                          \
      "getprop arguments.callee")                       \
    _(GetProp_InferredConstant,                         \
      "getprop inferred constant")                      \
    _(GetProp_Constant,                                 \
      "getprop constant")                               \
    _(GetProp_TypedObject,                              \
      "getprop TypedObject")                            \
    _(GetProp_DefiniteSlot,                             \
      "getprop definite slot")                          \
    _(GetProp_Unboxed,                                  \
      "getprop unboxed object")                         \
    _(GetProp_CommonGetter,                             \
      "getprop common getter")                          \
    _(GetProp_InlineAccess,                             \
      "getprop inline access")                          \
    _(GetProp_Innerize,                                 \
      "getprop innerize (access on global window)")     \
    _(GetProp_InlineCache,                              \
      "getprop IC")                                     \
                                                        \
    _(SetProp_CommonSetter,                             \
      "setprop common setter")                          \
    _(SetProp_TypedObject,                              \
      "setprop TypedObject")                            \
    _(SetProp_DefiniteSlot,                             \
      "setprop definite slot")                          \
    _(SetProp_Unboxed,                                  \
      "setprop unboxed object")                         \
    _(SetProp_InlineAccess,                             \
      "setprop inline access")                          \
                                                        \
    _(GetElem_TypedObject,                              \
      "getprop TypedObject")                            \
    _(GetElem_Dense,                                    \
      "getelem dense")                                  \
    _(GetElem_TypedStatic,                              \
      "getelem TypedArray static")                      \
    _(GetElem_TypedArray,                               \
      "getelem TypedArray")                             \
    _(GetElem_String,                                   \
      "getelem string")                                 \
    _(GetElem_Arguments,                                \
      "getelem arguments")                              \
    _(GetElem_ArgumentsInlined,                         \
      "getelem arguments inlined")                      \
    _(GetElem_InlineCache,                              \
      "getelem IC")                                     \
                                                        \
    _(SetElem_TypedObject,                              \
      "setelem TypedObject")                            \
    _(SetElem_TypedStatic,                              \
      "setelem TypedArray static")                      \
    _(SetElem_TypedArray,                               \
      "setelem TypedArray")                             \
    _(SetElem_Dense,                                    \
      "setelem dense")                                  \
    _(SetElem_Arguments,                                \
      "setelem arguments")                              \
    _(SetElem_InlineCache,                              \
      "setelem IC")                                     \
                                                        \
    _(Call_Inline,                                      \
      "call inline")





#define TRACKED_OUTCOME_LIST(_)                                         \
    _(GenericFailure,                                                   \
      "failure")                                                        \
    _(Disabled,                                                         \
      "disabled")                                                       \
    _(NoTypeInfo,                                                       \
      "no type info")                                                   \
    _(NoAnalysisInfo,                                                   \
      "no newscript analysis")                                          \
    _(NoShapeInfo,                                                      \
      "cannot determine shape")                                         \
    _(UnknownObject,                                                    \
      "unknown object")                                                 \
    _(UnknownProperties,                                                \
      "unknown properties")                                             \
    _(Singleton,                                                        \
      "is singleton")                                                   \
    _(NotSingleton,                                                     \
      "is not singleton")                                               \
    _(NotFixedSlot,                                                     \
      "property not in fixed slot")                                     \
    _(InconsistentFixedSlot,                                            \
      "property not in a consistent fixed slot")                        \
    _(NotObject,                                                        \
      "not definitely an object")                                       \
    _(NotStruct,                                                        \
      "not definitely a TypedObject struct")                            \
    _(NotUnboxed,                                                       \
      "not definitely an unboxed object")                               \
    _(UnboxedConvertedToNative,                                         \
      "unboxed object may have been converted")                         \
    _(StructNoField,                                                    \
      "struct doesn't definitely have field")                           \
    _(InconsistentFieldType,                                            \
      "unboxed property does not have consistent type")                 \
    _(InconsistentFieldOffset,                                          \
      "unboxed property does not have consistent offset")               \
    _(NeedsTypeBarrier,                                                 \
      "needs type barrier")                                             \
    _(InDictionaryMode,                                                 \
      "object in dictionary mode")                                      \
    _(NoProtoFound,                                                     \
      "no proto found")                                                 \
    _(MultiProtoPaths,                                                  \
      "not all paths to property go through same proto")                \
    _(NonWritableProperty,                                              \
      "non-writable property")                                          \
    _(ProtoIndexedProps,                                                \
      "prototype has indexed properties")                               \
    _(ArrayBadFlags,                                                    \
      "array observed to be sparse, overflowed .length, or has been iterated") \
    _(ArrayDoubleConversion,                                            \
      "array has ambiguous double conversion")                          \
    _(ArrayRange,                                                       \
      "array range issue (.length problems)")                           \
    _(ArraySeenNegativeIndex,                                           \
      "has seen array access with negative index")                      \
    _(TypedObjectNeutered,                                              \
      "TypedObject might have been neutered")                           \
    _(TypedObjectArrayRange,                                            \
      "TypedObject array of unknown length")                            \
    _(AccessNotDense,                                                   \
      "access not on dense native (check receiver, index, and result types)") \
    _(AccessNotTypedObject,                                             \
      "access not on typed array (check receiver and index types)")     \
    _(AccessNotTypedArray,                                              \
      "access not on typed array (check receiver, index, and result types)") \
    _(AccessNotString,                                                  \
      "getelem not on string (check receiver and index types)")         \
    _(StaticTypedArrayUint32,                                           \
      "static uint32 arrays currently cannot be optimized")             \
    _(StaticTypedArrayCantComputeMask,                                  \
      "can't compute mask for static typed array access (index isn't constant or not int32)") \
    _(OutOfBounds,                                                      \
      "observed out of bounds access")                                  \
    _(GetElemStringNotCached,                                           \
      "getelem on strings is not inline cached")                        \
    _(NonNativeReceiver,                                                \
      "observed non-native receiver")                                   \
    _(IndexType,                                                        \
      "index type must be int32, string, or symbol")                    \
    _(SetElemNonDenseNonTANotCached,                                    \
      "setelem on non-dense non-TAs are not inline cached")             \
                                                                        \
    _(CantInlineGeneric,                                                \
      "can't inline")                                                   \
    _(CantInlineNoTarget,                                               \
      "can't inline: no target")                                        \
    _(CantInlineNotInterpreted,                                         \
      "can't inline: not interpreted")                                  \
    _(CantInlineNoBaseline,                                             \
      "can't inline: no baseline code")                                 \
    _(CantInlineLazy,                                                   \
      "can't inline: lazy script")                                      \
    _(CantInlineNotConstructor,                                         \
      "can't inline: calling non-constructor with 'new'")               \
    _(CantInlineDisabledIon,                                            \
      "can't inline: ion disabled for callee")                          \
    _(CantInlineTooManyArgs,                                            \
      "can't inline: too many arguments")                               \
    _(CantInlineRecursive,                                              \
      "can't inline: recursive")                                        \
    _(CantInlineHeavyweight,                                            \
      "can't inline: heavyweight")                                      \
    _(CantInlineNeedsArgsObj,                                           \
      "can't inline: needs arguments object")                           \
    _(CantInlineDebuggee,                                               \
      "can't inline: debuggee")                                         \
    _(CantInlineUnknownProps,                                           \
      "can't inline: type has unknown properties")                      \
    _(CantInlineExceededDepth,                                          \
      "can't inline: exceeded inlining depth")                          \
    _(CantInlineBigLoop,                                                \
      "can't inline: big function with a loop")                         \
    _(CantInlineBigCaller,                                              \
      "can't inline: big caller")                                       \
    _(CantInlineBigCallee,                                              \
      "can't inline: big callee")                                       \
    _(CantInlineNotHot,                                                 \
      "can't inline: not hot enough")                                   \
    _(CantInlineNotInDispatch,                                          \
      "can't inline: not in dispatch table")                            \
    _(CantInlineNativeBadForm,                                          \
      "can't inline native: bad form (arity mismatch/constructing)")    \
    _(CantInlineNativeBadType,                                          \
      "can't inline native: bad argument or return type observed")      \
    _(CantInlineNativeNoTemplateObj,                                    \
      "can't inline native: no template object")                        \
    _(CantInlineBound,                                                  \
      "can't inline bound function invocation")                         \
                                                                        \
    _(GenericSuccess,                                                   \
      "success")                                                        \
    _(Inlined,                                                          \
      "inlined")                                                        \
    _(DOM,                                                              \
      "DOM")                                                            \
    _(Monomorphic,                                                      \
      "monomorphic")                                                    \
    _(Polymorphic,                                                      \
      "polymorphic")

#define TRACKED_TYPESITE_LIST(_)                \
    _(Receiver,                                 \
      "receiver object")                        \
    _(Index,                                    \
      "index")                                  \
    _(Value,                                    \
      "value")                                  \
    _(Call_Target,                              \
      "call target")                            \
    _(Call_This,                                \
      "call 'this'")                            \
    _(Call_Arg,                                 \
      "call argument")                          \
    _(Call_Return,                              \
      "call return")

enum class TrackedStrategy : uint32_t {
#define STRATEGY_OP(name, msg) name,
    TRACKED_STRATEGY_LIST(STRATEGY_OP)
#undef STRATEGY_OPT

    Count
};

enum class TrackedOutcome : uint32_t {
#define OUTCOME_OP(name, msg) name,
    TRACKED_OUTCOME_LIST(OUTCOME_OP)
#undef OUTCOME_OP

    Count
};

enum class TrackedTypeSite : uint32_t {
#define TYPESITE_OP(name, msg) name,
    TRACKED_TYPESITE_LIST(TYPESITE_OP)
#undef TYPESITE_OP

    Count
};

extern JS_PUBLIC_API(const char *)
TrackedStrategyString(TrackedStrategy strategy);

extern JS_PUBLIC_API(const char *)
TrackedOutcomeString(TrackedOutcome outcome);

extern JS_PUBLIC_API(const char *)
TrackedTypeSiteString(TrackedTypeSite site);

struct ForEachTrackedOptimizationAttemptOp
{
    virtual void operator()(TrackedStrategy strategy, TrackedOutcome outcome) = 0;
};

JS_PUBLIC_API(void)
ForEachTrackedOptimizationAttempt(JSRuntime *rt, void *addr,
                                  ForEachTrackedOptimizationAttemptOp &op);

struct ForEachTrackedOptimizationTypeInfoOp
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual void readType(const char *keyedBy, const char *name,
                          const char *location, unsigned lineno) = 0;

    
    virtual void operator()(TrackedTypeSite site, const char *mirType) = 0;
};

extern JS_PUBLIC_API(void)
ForEachTrackedOptimizationTypeInfo(JSRuntime *rt, void *addr,
                                   ForEachTrackedOptimizationTypeInfoOp &op);

} 

#endif
