







#include "jsarray.h"
#include "jsanalyze.h"
#include "jscompartment.h"
#include "jsinfer.h"
#include "jsprf.h"

#include "gc/Root.h"
#include "vm/GlobalObject.h"
#ifdef JS_ION
#include "ion/IonFrames.h"
#endif

#include "vm/Stack-inl.h"

#ifndef jsinferinlines_h___
#define jsinferinlines_h___

inline bool
js::TaggedProto::isObject() const
{
    
    return uintptr_t(proto) > uintptr_t(Proxy::LazyProto);
}

inline bool
js::TaggedProto::isLazy() const
{
    return proto == Proxy::LazyProto;
}

inline JSObject *
js::TaggedProto::toObject() const
{
    JS_ASSERT(isObject());
    return proto;
}

inline JSObject *
js::TaggedProto::toObjectOrNull() const
{
    JS_ASSERT(!proto || isObject());
    return proto;
}

template<class Outer>
inline bool
js::TaggedProtoOperations<Outer>::isLazy() const
{
    return value()->isLazy();
}

template<class Outer>
inline bool
js::TaggedProtoOperations<Outer>::isObject() const
{
    return value()->isObject();
}

template<class Outer>
inline JSObject *
js::TaggedProtoOperations<Outer>::toObject() const
{
    return value()->toObject();
}

template<class Outer>
inline JSObject *
js::TaggedProtoOperations<Outer>::toObjectOrNull() const
{
    return value()->toObjectOrNull();
}

namespace js {
namespace types {





inline
CompilerOutput::CompilerOutput()
  : script(NULL),
    kindInt(MethodJIT),
    constructing(false),
    barriers(false),
    chunkIndex(false)
{
}

inline mjit::JITScript *
CompilerOutput::mjit() const
{
#ifdef JS_METHODJIT
    JS_ASSERT(kind() == MethodJIT && isValid());
    return script->getJIT(constructing, barriers);
#else
    return NULL;
#endif
}

inline ion::IonScript *
CompilerOutput::ion() const
{
#ifdef JS_ION
    JS_ASSERT(kind() != MethodJIT && isValid());
    switch (kind()) {
      case MethodJIT: break;
      case Ion: return script->ionScript();
      case ParallelIon: return script->parallelIonScript();
    }
#endif
    JS_NOT_REACHED("Invalid kind of CompilerOutput");
    return NULL;
}

inline bool
CompilerOutput::isValid() const
{
    if (!script)
        return false;

#if defined(DEBUG) && (defined(JS_METHODJIT) || defined(JS_ION))
    TypeCompartment &types = script->compartment()->types;
#endif

    switch (kind()) {
      case MethodJIT: {
#ifdef JS_METHODJIT
        mjit::JITScript *jit = script->getJIT(constructing, barriers);
        if (!jit)
            return false;
        mjit::JITChunk *chunk = jit->chunkDescriptor(chunkIndex).chunk;
        if (!chunk)
            return false;
        JS_ASSERT(this == chunk->recompileInfo.compilerOutput(types));
        return true;
#endif
      }

      case Ion:
#ifdef JS_ION
        if (script->hasIonScript()) {
            JS_ASSERT(this == script->ion->recompileInfo().compilerOutput(types));
            return true;
        }
        if (script->isIonCompilingOffThread())
            return true;
#endif
        return false;

      case ParallelIon:
#ifdef JS_ION
        if (script->hasParallelIonScript()) {
            JS_ASSERT(this == script->parallelIonScript()->recompileInfo().compilerOutput(types));
            return true;
        }
        if (script->isParallelIonCompilingOffThread())
            return true;
#endif
        return false;
    }
    return false;
}

inline CompilerOutput*
RecompileInfo::compilerOutput(TypeCompartment &types) const
{
    return &(*types.constrainedOutputs)[outputIndex];
}

inline CompilerOutput*
RecompileInfo::compilerOutput(JSContext *cx) const
{
    return compilerOutput(cx->compartment->types);
}





 inline Type
Type::ObjectType(RawObject obj)
{
    if (obj->hasSingletonType())
        return Type(uintptr_t(obj) | 1);
    return Type(uintptr_t(obj->type()));
}

 inline Type
Type::ObjectType(TypeObject *obj)
{
    if (obj->singleton)
        return Type(uintptr_t(obj->singleton.get()) | 1);
    return Type(uintptr_t(obj));
}

 inline Type
Type::ObjectType(TypeObjectKey *obj)
{
    return Type(uintptr_t(obj));
}

inline Type
GetValueType(JSContext *cx, const Value &val)
{
    JS_ASSERT(cx->typeInferenceEnabled());
    if (val.isDouble())
        return Type::DoubleType();
    if (val.isObject())
        return Type::ObjectType(&val.toObject());
    return Type::PrimitiveType(val.extractNonDoubleType());
}

inline TypeFlags
PrimitiveTypeFlag(JSValueType type)
{
    switch (type) {
      case JSVAL_TYPE_UNDEFINED:
        return TYPE_FLAG_UNDEFINED;
      case JSVAL_TYPE_NULL:
        return TYPE_FLAG_NULL;
      case JSVAL_TYPE_BOOLEAN:
        return TYPE_FLAG_BOOLEAN;
      case JSVAL_TYPE_INT32:
        return TYPE_FLAG_INT32;
      case JSVAL_TYPE_DOUBLE:
        return TYPE_FLAG_DOUBLE;
      case JSVAL_TYPE_STRING:
        return TYPE_FLAG_STRING;
      case JSVAL_TYPE_MAGIC:
        return TYPE_FLAG_LAZYARGS;
      default:
        JS_NOT_REACHED("Bad type");
        return 0;
    }
}

inline JSValueType
TypeFlagPrimitive(TypeFlags flags)
{
    switch (flags) {
      case TYPE_FLAG_UNDEFINED:
        return JSVAL_TYPE_UNDEFINED;
      case TYPE_FLAG_NULL:
        return JSVAL_TYPE_NULL;
      case TYPE_FLAG_BOOLEAN:
        return JSVAL_TYPE_BOOLEAN;
      case TYPE_FLAG_INT32:
        return JSVAL_TYPE_INT32;
      case TYPE_FLAG_DOUBLE:
        return JSVAL_TYPE_DOUBLE;
      case TYPE_FLAG_STRING:
        return JSVAL_TYPE_STRING;
      case TYPE_FLAG_LAZYARGS:
        return JSVAL_TYPE_MAGIC;
      default:
        JS_NOT_REACHED("Bad type");
        return (JSValueType) 0;
    }
}






inline RawId
IdToTypeId(RawId id)
{
    AutoAssertNoGC nogc;
    JS_ASSERT(!JSID_IS_EMPTY(id));

    



    if (JSID_IS_INT(id))
        return JSID_VOID;

    



    if (JSID_IS_STRING(id)) {
        JSFlatString *str = JSID_TO_FLAT_STRING(id);
        TwoByteChars cp = str->range();
        if (JS7_ISDEC(cp[0]) || cp[0] == '-') {
            for (size_t i = 1; i < cp.length(); ++i) {
                if (!JS7_ISDEC(cp[i]))
                    return id;
            }
            return JSID_VOID;
        }
        return id;
    }

    return JSID_VOID;
}

const char * TypeIdStringImpl(jsid id);


static inline const char *
TypeIdString(RawId id)
{
#ifdef DEBUG
    return TypeIdStringImpl(id);
#else
    return "(missing)";
#endif
}


inline bool
IsInlinableCall(jsbytecode *pc)
{
    JSOp op = JSOp(*pc);

    
    
    
    
    return op == JSOP_CALL || op == JSOP_FUNCALL || op == JSOP_FUNAPPLY || op == JSOP_EVAL ||
#ifdef JS_ION
           op == JSOP_NEW ||
#endif
           op == JSOP_GETPROP || op == JSOP_CALLPROP || op == JSOP_LENGTH ||
           op == JSOP_SETPROP || op == JSOP_SETGNAME || op == JSOP_SETNAME;
}











struct AutoEnterAnalysis
{
    
    gc::AutoSuppressGC suppressGC;

    FreeOp *freeOp;
    JSCompartment *compartment;
    bool oldActiveAnalysis;

    AutoEnterAnalysis(JSContext *cx)
      : suppressGC(cx)
    {
        init(cx->runtime->defaultFreeOp(), cx->compartment);
    }

    AutoEnterAnalysis(FreeOp *fop, JSCompartment *comp)
      : suppressGC(comp)
    {
        init(fop, comp);
    }

    ~AutoEnterAnalysis()
    {
        compartment->activeAnalysis = oldActiveAnalysis;

        





        if (!compartment->activeAnalysis) {
            TypeCompartment *types = &compartment->types;
            if (types->pendingNukeTypes)
                types->nukeTypes(freeOp);
            else if (types->pendingRecompiles)
                types->processPendingRecompiles(freeOp);
        }
    }

  private:
    void init(FreeOp *fop, JSCompartment *comp) {
        freeOp = fop;
        compartment = comp;
        oldActiveAnalysis = compartment->activeAnalysis;
        compartment->activeAnalysis = true;
    }
};





struct AutoEnterCompilation
{
    JSContext *cx;
    RecompileInfo &info;
    CompilerOutput::Kind kind;

    AutoEnterCompilation(JSContext *cx, CompilerOutput::Kind kind)
      : cx(cx),
        info(cx->compartment->types.compiledInfo),
        kind(kind)
    {
        JS_ASSERT(cx->compartment->activeAnalysis);
        JS_ASSERT(info.outputIndex == RecompileInfo::NoCompilerRunning);
    }

    bool init(RawScript script, bool constructing, unsigned chunkIndex)
    {
        CompilerOutput co;
        co.script = script;
        co.setKind(kind);
        co.constructing = constructing;
        co.barriers = cx->zone()->compileBarriers();
        co.chunkIndex = chunkIndex;

        
        
        
        
        
        co.pendingRecompilation = true;

        JS_ASSERT(!co.isValid());
        TypeCompartment &types = cx->compartment->types;
        if (!types.constrainedOutputs) {
            types.constrainedOutputs = cx->new_< Vector<CompilerOutput> >(cx);
            if (!types.constrainedOutputs) {
                types.setPendingNukeTypes(cx);
                return false;
            }
        }

        info.outputIndex = types.constrainedOutputs->length();
        
        if (info.outputIndex >= RecompileInfo::NoCompilerRunning)
            return false;

        if (!types.constrainedOutputs->append(co)) {
            info.outputIndex = RecompileInfo::NoCompilerRunning;
            return false;
        }
        return true;
    }

    void initExisting(RecompileInfo oldInfo)
    {
        
        
        info = oldInfo;
    }

    ~AutoEnterCompilation()
    {
        
        if (info.outputIndex >= RecompileInfo::NoCompilerRunning)
            return;

        JS_ASSERT(info.outputIndex < cx->compartment->types.constrainedOutputs->length());
        CompilerOutput *co = info.compilerOutput(cx);
        co->pendingRecompilation = false;
        if (!co->isValid())
            co->invalidate();

        info.outputIndex = RecompileInfo::NoCompilerRunning;
    }
};












inline Class *
GetClassForProtoKey(JSProtoKey key)
{
    switch (key) {
      case JSProto_Object:
        return &ObjectClass;
      case JSProto_Array:
        return &ArrayClass;

      case JSProto_Number:
        return &NumberClass;
      case JSProto_Boolean:
        return &BooleanClass;
      case JSProto_String:
        return &StringClass;
      case JSProto_RegExp:
        return &RegExpClass;

      case JSProto_Int8Array:
      case JSProto_Uint8Array:
      case JSProto_Int16Array:
      case JSProto_Uint16Array:
      case JSProto_Int32Array:
      case JSProto_Uint32Array:
      case JSProto_Float32Array:
      case JSProto_Float64Array:
      case JSProto_Uint8ClampedArray:
        return &TypedArray::classes[key - JSProto_Int8Array];

      case JSProto_ArrayBuffer:
        return &ArrayBufferClass;

      case JSProto_DataView:
        return &DataViewClass;

      default:
        JS_NOT_REACHED("Bad proto key");
        return NULL;
    }
}





inline TypeObject *
GetTypeNewObject(JSContext *cx, JSProtoKey key)
{
    js::RootedObject proto(cx);
    if (!js_GetClassPrototype(cx, key, &proto))
        return NULL;
    return proto->getNewType(cx, GetClassForProtoKey(key));
}


inline TypeObject *
GetTypeCallerInitObject(JSContext *cx, JSProtoKey key)
{
    if (cx->typeInferenceEnabled()) {
        jsbytecode *pc;
        RootedScript script(cx, cx->stack.currentScript(&pc));
        if (script)
            return TypeScript::InitObject(cx, script, pc, key);
    }
    return GetTypeNewObject(cx, key);
}

void MarkIteratorUnknownSlow(JSContext *cx);





inline void
MarkIteratorUnknown(JSContext *cx)
{
    if (cx->typeInferenceEnabled())
        MarkIteratorUnknownSlow(cx);
}

void TypeMonitorCallSlow(JSContext *cx, JSObject *callee, const CallArgs &args,
                         bool constructing);





inline bool
TypeMonitorCall(JSContext *cx, const js::CallArgs &args, bool constructing)
{
    if (args.callee().isFunction()) {
        JSFunction *fun = args.callee().toFunction();
        if (fun->isInterpreted()) {
            if (!fun->nonLazyScript()->ensureRanAnalysis(cx))
                return false;
            if (cx->typeInferenceEnabled())
                TypeMonitorCallSlow(cx, &args.callee(), args, constructing);
        }
    }

    return true;
}

inline bool
TrackPropertyTypes(JSContext *cx, RawObject obj, RawId id)
{
    AutoAssertNoGC nogc;

    if (!cx->typeInferenceEnabled() || obj->hasLazyType() || obj->type()->unknownProperties())
        return false;

    if (obj->hasSingletonType() && !obj->type()->maybeGetProperty(id, cx))
        return false;

    return true;
}


inline void
AddTypePropertyId(JSContext *cx, JSObject *obj, jsid id, Type type)
{
    AssertCanGC();
    if (cx->typeInferenceEnabled())
        id = IdToTypeId(id);
    if (TrackPropertyTypes(cx, obj, id))
        obj->type()->addPropertyType(cx, id, type);
}

inline void
AddTypePropertyId(JSContext *cx, JSObject *obj, jsid id, const Value &value)
{
    AssertCanGC();
    if (cx->typeInferenceEnabled())
        id = IdToTypeId(id);
    if (TrackPropertyTypes(cx, obj, id))
        obj->type()->addPropertyType(cx, id, value);
}

inline void
AddTypeProperty(JSContext *cx, TypeObject *obj, const char *name, Type type)
{
    AssertCanGC();
    if (cx->typeInferenceEnabled() && !obj->unknownProperties())
        obj->addPropertyType(cx, name, type);
}

inline void
AddTypeProperty(JSContext *cx, TypeObject *obj, const char *name, const Value &value)
{
    AssertCanGC();
    if (cx->typeInferenceEnabled() && !obj->unknownProperties())
        obj->addPropertyType(cx, name, value);
}


inline void
MarkTypeObjectFlags(JSContext *cx, RawObject obj, TypeObjectFlags flags)
{
    if (cx->typeInferenceEnabled() && !obj->hasLazyType() && !obj->type()->hasAllFlags(flags))
        obj->type()->setFlags(cx, flags);
}







inline void
MarkTypeObjectUnknownProperties(JSContext *cx, TypeObject *obj,
                                bool markSetsUnknown = false)
{
    if (cx->typeInferenceEnabled()) {
        if (!obj->unknownProperties())
            obj->markUnknown(cx);
        if (markSetsUnknown && !(obj->flags & OBJECT_FLAG_SETS_MARKED_UNKNOWN))
            cx->compartment->types.markSetsUnknown(cx, obj);
    }
}





inline void
MarkTypePropertyConfigured(JSContext *cx, HandleObject obj, RawId id)
{
    if (cx->typeInferenceEnabled())
        id = IdToTypeId(id);
    if (TrackPropertyTypes(cx, obj, id))
        obj->type()->markPropertyConfigured(cx, id);
}


inline void
MarkObjectStateChange(JSContext *cx, RawObject obj)
{
    AutoAssertNoGC nogc;
    if (cx->typeInferenceEnabled() && !obj->hasLazyType() && !obj->type()->unknownProperties())
        obj->type()->markStateChange(cx);
}






inline void
FixArrayType(JSContext *cx, HandleObject obj)
{
    if (cx->typeInferenceEnabled())
        cx->compartment->types.fixArrayType(cx, obj);
}

inline void
FixObjectType(JSContext *cx, HandleObject obj)
{
    if (cx->typeInferenceEnabled())
        cx->compartment->types.fixObjectType(cx, obj);
}


extern void TypeMonitorResult(JSContext *cx, JSScript *script, jsbytecode *pc,
                              const js::Value &rval);
extern void TypeDynamicResult(JSContext *cx, JSScript *script, jsbytecode *pc,
                              js::types::Type type);

inline bool
UseNewTypeAtEntry(JSContext *cx, StackFrame *fp)
{
    if (!fp->isConstructing() || !cx->typeInferenceEnabled() || !fp->prev())
        return false;

    JSScript *prevScript = fp->prev()->script();
    return UseNewType(cx, prevScript, fp->prevpc());
}

inline bool
UseNewTypeForClone(JSFunction *fun)
{
    AutoAssertNoGC nogc;

    if (!fun->isInterpreted())
        return false;

    if (fun->nonLazyScript()->shouldCloneAtCallsite)
        return true;

    if (fun->hasSingletonType())
        return false;

    























    RawScript script = fun->nonLazyScript();

    if (script->length >= 50)
        return false;

    if (script->hasConsts() || script->hasObjects() || script->hasRegexps() || fun->isHeavyweight())
        return false;

    bool hasArguments = false;
    bool hasApply = false;

    for (jsbytecode *pc = script->code;
         pc != script->code + script->length;
         pc += GetBytecodeLength(pc))
    {
        if (*pc == JSOP_ARGUMENTS)
            hasArguments = true;
        if (*pc == JSOP_FUNAPPLY)
            hasApply = true;
    }

    return hasArguments && hasApply;
}





 inline unsigned
TypeScript::NumTypeSets(RawScript script)
{
    return script->nTypeSets + analyze::TotalSlots(script);
}

 inline HeapTypeSet *
TypeScript::ReturnTypes(RawScript script)
{
    AutoAssertNoGC nogc;
    TypeSet *types = script->types->typeArray() + script->nTypeSets + js::analyze::CalleeSlot();
    return types->toHeapTypeSet();
}

 inline StackTypeSet *
TypeScript::ThisTypes(RawScript script)
{
    AutoAssertNoGC nogc;
    TypeSet *types = script->types->typeArray() + script->nTypeSets + js::analyze::ThisSlot();
    return types->toStackTypeSet();
}







 inline StackTypeSet *
TypeScript::ArgTypes(RawScript script, unsigned i)
{
    AutoAssertNoGC nogc;
    JS_ASSERT(i < script->function()->nargs);
    TypeSet *types = script->types->typeArray() + script->nTypeSets + js::analyze::ArgSlot(i);
    return types->toStackTypeSet();
}

 inline StackTypeSet *
TypeScript::LocalTypes(RawScript script, unsigned i)
{
    AutoAssertNoGC nogc;
    JS_ASSERT(i < script->nfixed);
    TypeSet *types = script->types->typeArray() + script->nTypeSets + js::analyze::LocalSlot(script, i);
    return types->toStackTypeSet();
}

 inline StackTypeSet *
TypeScript::SlotTypes(RawScript script, unsigned slot)
{
    AutoAssertNoGC nogc;
    JS_ASSERT(slot < js::analyze::TotalSlots(script));
    TypeSet *types = script->types->typeArray() + script->nTypeSets + slot;
    return types->toStackTypeSet();
}

 inline TypeObject *
TypeScript::StandardType(JSContext *cx, JSProtoKey key)
{
    js::RootedObject proto(cx);
    if (!js_GetClassPrototype(cx, key, &proto, NULL))
        return NULL;
    return proto->getNewType(cx, GetClassForProtoKey(key));
}

struct AllocationSiteKey {
    JSScript *script;

    uint32_t offset : 24;
    JSProtoKey kind : 8;

    static const uint32_t OFFSET_LIMIT = (1 << 23);

    AllocationSiteKey() { PodZero(this); }

    typedef AllocationSiteKey Lookup;

    static inline uint32_t hash(AllocationSiteKey key) {
        return uint32_t(size_t(key.script->code + key.offset)) ^ key.kind;
    }

    static inline bool match(const AllocationSiteKey &a, const AllocationSiteKey &b) {
        return a.script == b.script && a.offset == b.offset && a.kind == b.kind;
    }
};


js::NewObjectKind
UseNewTypeForInitializer(JSContext *cx, JSScript *script, jsbytecode *pc, JSProtoKey key);

js::NewObjectKind
UseNewTypeForInitializer(JSContext *cx, JSScript *script, jsbytecode *pc, Class *clasp);

 inline TypeObject *
TypeScript::InitObject(JSContext *cx, JSScript *script, jsbytecode *pc, JSProtoKey kind)
{
    JS_ASSERT(!UseNewTypeForInitializer(cx, script, pc, kind));

    
    uint32_t offset = pc - script->code;

    if (!cx->typeInferenceEnabled() || !script->compileAndGo || offset >= AllocationSiteKey::OFFSET_LIMIT)
        return GetTypeNewObject(cx, kind);

    AllocationSiteKey key;
    key.script = script;
    key.offset = offset;
    key.kind = kind;

    if (!cx->compartment->types.allocationSiteTable)
        return cx->compartment->types.addAllocationSiteTypeObject(cx, key);

    AllocationSiteTable::Ptr p = cx->compartment->types.allocationSiteTable->lookup(key);

    if (p)
        return p->value;
    return cx->compartment->types.addAllocationSiteTypeObject(cx, key);
}


static inline bool
SetInitializerObjectType(JSContext *cx, HandleScript script, jsbytecode *pc, HandleObject obj, NewObjectKind kind)
{
    if (!cx->typeInferenceEnabled())
        return true;

    JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(obj->getClass());
    JS_ASSERT(key != JSProto_Null);
    JS_ASSERT(kind == UseNewTypeForInitializer(cx, script, pc, key));

    if (kind == SingletonObject) {
        JS_ASSERT(obj->hasSingletonType());

        




        TypeScript::Monitor(cx, script, pc, ObjectValue(*obj));
    } else {
        types::TypeObject *type = TypeScript::InitObject(cx, script, pc, key);
        if (!type)
            return false;
        obj->setType(type);
    }

    return true;
}

 inline void
TypeScript::Monitor(JSContext *cx, JSScript *script, jsbytecode *pc, const js::Value &rval)
{
    if (cx->typeInferenceEnabled())
        TypeMonitorResult(cx, script, pc, rval);
}

 inline void
TypeScript::MonitorOverflow(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    if (cx->typeInferenceEnabled())
        TypeDynamicResult(cx, script, pc, Type::DoubleType());
}

 inline void
TypeScript::MonitorString(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    if (cx->typeInferenceEnabled())
        TypeDynamicResult(cx, script, pc, Type::StringType());
}

 inline void
TypeScript::MonitorUnknown(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    if (cx->typeInferenceEnabled())
        TypeDynamicResult(cx, script, pc, Type::UnknownType());
}

 inline void
TypeScript::GetPcScript(JSContext *cx, JSScript **script, jsbytecode **pc)
{
    AutoAssertNoGC nogc;
#ifdef JS_ION
    if (cx->fp()->beginsIonActivation()) {
        ion::GetPcScript(cx, script, pc);
        return;
    }
#endif
    *script = cx->fp()->script();
    *pc = cx->regs().pc;
}

 inline void
TypeScript::MonitorOverflow(JSContext *cx)
{
    RootedScript script(cx);
    jsbytecode *pc;
    GetPcScript(cx, script.address(), &pc);
    MonitorOverflow(cx, script, pc);
}

 inline void
TypeScript::MonitorString(JSContext *cx)
{
    RootedScript script(cx);
    jsbytecode *pc;
    GetPcScript(cx, script.address(), &pc);
    MonitorString(cx, script, pc);
}

 inline void
TypeScript::MonitorUnknown(JSContext *cx)
{
    RootedScript script(cx);
    jsbytecode *pc;
    GetPcScript(cx, script.address(), &pc);
    MonitorUnknown(cx, script, pc);
}

 inline void
TypeScript::Monitor(JSContext *cx, const js::Value &rval)
{
    RootedScript script(cx);
    jsbytecode *pc;
    GetPcScript(cx, script.address(), &pc);
    Monitor(cx, script, pc, rval);
}

 inline void
TypeScript::MonitorAssign(JSContext *cx, HandleObject obj, jsid id)
{
    if (cx->typeInferenceEnabled() && !obj->hasSingletonType()) {
        







        uint32_t i;
        if (js_IdIsIndex(id, &i))
            return;
        MarkTypeObjectUnknownProperties(cx, obj->type());
    }
}

 inline void
TypeScript::SetThis(JSContext *cx, JSScript *script, Type type)
{
    if (!cx->typeInferenceEnabled())
        return;
    JS_ASSERT(script->types);

    
    bool analyze = cx->hasOption(JSOPTION_METHODJIT_ALWAYS);

    if (!ThisTypes(script)->hasType(type) || analyze) {
        AutoEnterAnalysis enter(cx);

        InferSpew(ISpewOps, "externalType: setThis #%u: %s",
                  script->id(), TypeString(type));
        ThisTypes(script)->addType(cx, type);

        if (analyze)
            script->ensureRanInference(cx);
    }
}

 inline void
TypeScript::SetThis(JSContext *cx, JSScript *script, const js::Value &value)
{
    if (cx->typeInferenceEnabled())
        SetThis(cx, script, GetValueType(cx, value));
}

 inline void
TypeScript::SetLocal(JSContext *cx, JSScript *script, unsigned local, Type type)
{
    if (!cx->typeInferenceEnabled())
        return;
    JS_ASSERT(script->types);

    if (!LocalTypes(script, local)->hasType(type)) {
        AutoEnterAnalysis enter(cx);

        InferSpew(ISpewOps, "externalType: setLocal #%u %u: %s",
                  script->id(), local, TypeString(type));
        LocalTypes(script, local)->addType(cx, type);
    }
}

 inline void
TypeScript::SetLocal(JSContext *cx, JSScript *script, unsigned local, const js::Value &value)
{
    if (cx->typeInferenceEnabled()) {
        Type type = GetValueType(cx, value);
        SetLocal(cx, script, local, type);
    }
}

 inline void
TypeScript::SetArgument(JSContext *cx, JSScript *script, unsigned arg, Type type)
{
    if (!cx->typeInferenceEnabled())
        return;
    JS_ASSERT(script->types);

    if (!ArgTypes(script, arg)->hasType(type)) {
        AutoEnterAnalysis enter(cx);

        InferSpew(ISpewOps, "externalType: setArg #%u %u: %s",
                  script->id(), arg, TypeString(type));
        ArgTypes(script, arg)->addType(cx, type);
    }
}

 inline void
TypeScript::SetArgument(JSContext *cx, JSScript *script, unsigned arg, const js::Value &value)
{
    if (cx->typeInferenceEnabled()) {
        Type type = GetValueType(cx, value);
        SetArgument(cx, script, arg, type);
    }
}





inline JSCompartment *
TypeCompartment::compartment()
{
    return (JSCompartment *)((char *)this - offsetof(JSCompartment, types));
}

inline void
TypeCompartment::addPending(JSContext *cx, TypeConstraint *constraint, TypeSet *source, Type type)
{
    JS_ASSERT(this == &cx->compartment->types);
    JS_ASSERT(!cx->runtime->isHeapBusy());

    InferSpew(ISpewOps, "pending: %sC%p%s %s",
              InferSpewColor(constraint), constraint, InferSpewColorReset(),
              TypeString(type));

    if ((pendingCount == pendingCapacity) && !growPendingArray(cx))
        return;

    PendingWork &pending = pendingArray[pendingCount++];
    pending.constraint = constraint;
    pending.source = source;
    pending.type = type;
}

inline void
TypeCompartment::resolvePending(JSContext *cx)
{
    JS_ASSERT(this == &cx->compartment->types);

    if (resolving) {
        
        return;
    }

    resolving = true;

    
    while (pendingCount) {
        const PendingWork &pending = pendingArray[--pendingCount];
        InferSpew(ISpewOps, "resolve: %sC%p%s %s",
                  InferSpewColor(pending.constraint), pending.constraint,
                  InferSpewColorReset(), TypeString(pending.type));
        pending.constraint->newType(cx, pending.source, pending.type);
    }

    resolving = false;
}













const unsigned SET_ARRAY_SIZE = 8;


static inline unsigned
HashSetCapacity(unsigned count)
{
    JS_ASSERT(count >= 2);

    if (count <= SET_ARRAY_SIZE)
        return SET_ARRAY_SIZE;

    unsigned log2;
    JS_FLOOR_LOG2(log2, count);
    return 1 << (log2 + 2);
}


template <class T, class KEY>
static inline uint32_t
HashKey(T v)
{
    uint32_t nv = KEY::keyBits(v);

    uint32_t hash = 84696351 ^ (nv & 0xff);
    hash = (hash * 16777619) ^ ((nv >> 8) & 0xff);
    hash = (hash * 16777619) ^ ((nv >> 16) & 0xff);
    return (hash * 16777619) ^ ((nv >> 24) & 0xff);
}





template <class T, class U, class KEY>
static U **
HashSetInsertTry(LifoAlloc &alloc, U **&values, unsigned &count, T key)
{
    unsigned capacity = HashSetCapacity(count);
    unsigned insertpos = HashKey<T,KEY>(key) & (capacity - 1);

    
    bool converting = (count == SET_ARRAY_SIZE);

    if (!converting) {
        while (values[insertpos] != NULL) {
            if (KEY::getKey(values[insertpos]) == key)
                return &values[insertpos];
            insertpos = (insertpos + 1) & (capacity - 1);
        }
    }

    count++;
    unsigned newCapacity = HashSetCapacity(count);

    if (newCapacity == capacity) {
        JS_ASSERT(!converting);
        return &values[insertpos];
    }

    U **newValues = alloc.newArray<U*>(newCapacity);
    if (!newValues)
        return NULL;
    PodZero(newValues, newCapacity);

    for (unsigned i = 0; i < capacity; i++) {
        if (values[i]) {
            unsigned pos = HashKey<T,KEY>(KEY::getKey(values[i])) & (newCapacity - 1);
            while (newValues[pos] != NULL)
                pos = (pos + 1) & (newCapacity - 1);
            newValues[pos] = values[i];
        }
    }

    values = newValues;

    insertpos = HashKey<T,KEY>(key) & (newCapacity - 1);
    while (values[insertpos] != NULL)
        insertpos = (insertpos + 1) & (newCapacity - 1);
    return &values[insertpos];
}





template <class T, class U, class KEY>
static inline U **
HashSetInsert(LifoAlloc &alloc, U **&values, unsigned &count, T key)
{
    if (count == 0) {
        JS_ASSERT(values == NULL);
        count++;
        return (U **) &values;
    }

    if (count == 1) {
        U *oldData = (U*) values;
        if (KEY::getKey(oldData) == key)
            return (U **) &values;

        values = alloc.newArray<U*>(SET_ARRAY_SIZE);
        if (!values) {
            values = (U **) oldData;
            return NULL;
        }
        PodZero(values, SET_ARRAY_SIZE);
        count++;

        values[0] = oldData;
        return &values[1];
    }

    if (count <= SET_ARRAY_SIZE) {
        for (unsigned i = 0; i < count; i++) {
            if (KEY::getKey(values[i]) == key)
                return &values[i];
        }

        if (count < SET_ARRAY_SIZE) {
            count++;
            return &values[count - 1];
        }
    }

    return HashSetInsertTry<T,U,KEY>(alloc, values, count, key);
}


template <class T, class U, class KEY>
static inline U *
HashSetLookup(U **values, unsigned count, T key)
{
    if (count == 0)
        return NULL;

    if (count == 1)
        return (KEY::getKey((U *) values) == key) ? (U *) values : NULL;

    if (count <= SET_ARRAY_SIZE) {
        for (unsigned i = 0; i < count; i++) {
            if (KEY::getKey(values[i]) == key)
                return values[i];
        }
        return NULL;
    }

    unsigned capacity = HashSetCapacity(count);
    unsigned pos = HashKey<T,KEY>(key) & (capacity - 1);

    while (values[pos] != NULL) {
        if (KEY::getKey(values[pos]) == key)
            return values[pos];
        pos = (pos + 1) & (capacity - 1);
    }

    return NULL;
}

inline TypeObjectKey *
Type::objectKey() const
{
    JS_ASSERT(isObject());
    if (isTypeObject())
        TypeObject::readBarrier((TypeObject *) data);
    else
        JSObject::readBarrier((JSObject *) (data ^ 1));
    return (TypeObjectKey *) data;
}

inline RawObject
Type::singleObject() const
{
    JS_ASSERT(isSingleObject());
    JSObject::readBarrier((JSObject *) (data ^ 1));
    return (JSObject *) (data ^ 1);
}

inline TypeObject *
Type::typeObject() const
{
    JS_ASSERT(isTypeObject());
    TypeObject::readBarrier((TypeObject *) data);
    return (TypeObject *) data;
}

inline bool
TypeSet::hasType(Type type) const
{
    if (unknown())
        return true;

    if (type.isUnknown()) {
        return false;
    } else if (type.isPrimitive()) {
        return !!(flags & PrimitiveTypeFlag(type.primitive()));
    } else if (type.isAnyObject()) {
        return !!(flags & TYPE_FLAG_ANYOBJECT);
    } else {
        return !!(flags & TYPE_FLAG_ANYOBJECT) ||
            HashSetLookup<TypeObjectKey*,TypeObjectKey,TypeObjectKey>
            (objectSet, baseObjectCount(), type.objectKey()) != NULL;
    }
}

inline void
TypeSet::setBaseObjectCount(uint32_t count)
{
    JS_ASSERT(count <= TYPE_FLAG_OBJECT_COUNT_LIMIT);
    flags = (flags & ~TYPE_FLAG_OBJECT_COUNT_MASK)
          | (count << TYPE_FLAG_OBJECT_COUNT_SHIFT);
}

inline void
TypeSet::clearObjects()
{
    setBaseObjectCount(0);
    objectSet = NULL;
}

inline void
TypeSet::addType(JSContext *cx, Type type)
{
    JS_ASSERT(cx->compartment->activeAnalysis);

    if (unknown())
        return;

    if (type.isUnknown()) {
        flags |= TYPE_FLAG_BASE_MASK;
        clearObjects();
        JS_ASSERT(unknown());
    } else if (type.isPrimitive()) {
        TypeFlags flag = PrimitiveTypeFlag(type.primitive());
        if (flags & flag)
            return;

        
        if (flag == TYPE_FLAG_DOUBLE)
            flag |= TYPE_FLAG_INT32;

        flags |= flag;
    } else {
        if (flags & TYPE_FLAG_ANYOBJECT)
            return;
        if (type.isAnyObject())
            goto unknownObject;

        LifoAlloc &alloc =
            purged() ? cx->compartment->analysisLifoAlloc : cx->compartment->typeLifoAlloc;

        uint32_t objectCount = baseObjectCount();
        TypeObjectKey *object = type.objectKey();
        TypeObjectKey **pentry = HashSetInsert<TypeObjectKey *,TypeObjectKey,TypeObjectKey>
                                     (alloc, objectSet, objectCount, object);
        if (!pentry) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
        if (*pentry)
            return;
        *pentry = object;

        setBaseObjectCount(objectCount);

        if (objectCount == TYPE_FLAG_OBJECT_COUNT_LIMIT)
            goto unknownObject;

        if (type.isTypeObject()) {
            TypeObject *nobject = type.typeObject();
            JS_ASSERT(!nobject->singleton);
            if (nobject->unknownProperties())
                goto unknownObject;
            if (objectCount > 1) {
                nobject->contribution += (objectCount - 1) * (objectCount - 1);
                if (nobject->contribution >= TypeObject::CONTRIBUTION_LIMIT) {
                    InferSpew(ISpewOps, "limitUnknown: %sT%p%s",
                              InferSpewColor(this), this, InferSpewColorReset());
                    goto unknownObject;
                }
            }
        }
    }

    if (false) {
    unknownObject:
        type = Type::AnyObjectType();
        flags |= TYPE_FLAG_ANYOBJECT;
        clearObjects();
    }

    InferSpew(ISpewOps, "addType: %sT%p%s %s",
              InferSpewColor(this), this, InferSpewColorReset(),
              TypeString(type));

    
    TypeConstraint *constraint = constraintList;
    while (constraint) {
        cx->compartment->types.addPending(cx, constraint, this, type);
        constraint = constraint->next;
    }

    cx->compartment->types.resolvePending(cx);
}

inline void
TypeSet::setOwnProperty(JSContext *cx, bool configured)
{
    TypeFlags nflags = TYPE_FLAG_OWN_PROPERTY | (configured ? TYPE_FLAG_CONFIGURED_PROPERTY : 0);

    if ((flags & nflags) == nflags)
        return;

    flags |= nflags;

    
    TypeConstraint *constraint = constraintList;
    while (constraint) {
        constraint->newPropertyState(cx, this);
        constraint = constraint->next;
    }
}

inline unsigned
TypeSet::getObjectCount() const
{
    JS_ASSERT(!unknownObject());
    uint32_t count = baseObjectCount();
    if (count > SET_ARRAY_SIZE)
        return HashSetCapacity(count);
    return count;
}

inline TypeObjectKey *
TypeSet::getObject(unsigned i) const
{
    JS_ASSERT(i < getObjectCount());
    if (baseObjectCount() == 1) {
        JS_ASSERT(i == 0);
        return (TypeObjectKey *) objectSet;
    }
    return objectSet[i];
}

inline RawObject
TypeSet::getSingleObject(unsigned i) const
{
    TypeObjectKey *key = getObject(i);
    return (uintptr_t(key) & 1) ? (JSObject *)(uintptr_t(key) ^ 1) : NULL;
}

inline TypeObject *
TypeSet::getTypeObject(unsigned i) const
{
    TypeObjectKey *key = getObject(i);
    return (key && !(uintptr_t(key) & 1)) ? (TypeObject *) key : NULL;
}





inline
TypeCallsite::TypeCallsite(JSContext *cx, RawScript script, jsbytecode *pc,
                           bool isNew, unsigned argumentCount)
    : script(script), pc(pc), isNew(isNew), argumentCount(argumentCount),
      thisTypes(NULL), returnTypes(NULL)
{
    
    argumentTypes = cx->analysisLifoAlloc().newArray<StackTypeSet*>(argumentCount);
}





inline TypeObject::TypeObject(Class *clasp, TaggedProto proto, bool function, bool unknown)
{
    PodZero(this);

    
    JS_ASSERT_IF(proto.isObject(), !proto.toObject()->getClass()->ext.outerObject);

    this->clasp = clasp;
    this->proto = proto.raw();

    if (function)
        flags |= OBJECT_FLAG_FUNCTION;
    if (unknown)
        flags |= OBJECT_FLAG_UNKNOWN_MASK;

    InferSpew(ISpewOps, "newObject: %s", TypeObjectString(this));
}

inline uint32_t
TypeObject::basePropertyCount() const
{
    return (flags & OBJECT_FLAG_PROPERTY_COUNT_MASK) >> OBJECT_FLAG_PROPERTY_COUNT_SHIFT;
}

inline void
TypeObject::setBasePropertyCount(uint32_t count)
{
    JS_ASSERT(count <= OBJECT_FLAG_PROPERTY_COUNT_LIMIT);
    flags = (flags & ~OBJECT_FLAG_PROPERTY_COUNT_MASK)
          | (count << OBJECT_FLAG_PROPERTY_COUNT_SHIFT);
}

inline HeapTypeSet *
TypeObject::getProperty(JSContext *cx, RawId id, bool own)
{
    JS_ASSERT(cx->compartment->activeAnalysis);
    AssertCanGC();

    JS_ASSERT(JSID_IS_VOID(id) || JSID_IS_EMPTY(id) || JSID_IS_STRING(id));
    JS_ASSERT_IF(!JSID_IS_EMPTY(id), id == IdToTypeId(id));
    JS_ASSERT(!unknownProperties());

    uint32_t propertyCount = basePropertyCount();
    Property **pprop = HashSetInsert<jsid,Property,Property>
                           (cx->compartment->typeLifoAlloc, propertySet, propertyCount, id);
    if (!pprop) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return NULL;
    }

    if (!*pprop) {
        setBasePropertyCount(propertyCount);
        if (!addProperty(cx, id, pprop)) {
            setBasePropertyCount(0);
            propertySet = NULL;
            return NULL;
        }
        AutoAssertNoGC nogc;
        if (propertyCount == OBJECT_FLAG_PROPERTY_COUNT_LIMIT) {
            markUnknown(cx);

            



            unsigned count = getPropertyCount();
            for (unsigned i = 0; i < count; i++) {
                if (Property *prop = getProperty(i))
                    return &prop->types;
            }

            JS_NOT_REACHED("Missing property");
            return NULL;
        }
    }

    HeapTypeSet *types = &(*pprop)->types;
    if (own)
        types->setOwnProperty(cx, false);

    return types;
}

inline HeapTypeSet *
TypeObject::maybeGetProperty(RawId id, JSContext *cx)
{
    AutoAssertNoGC nogc;
    JS_ASSERT(JSID_IS_VOID(id) || JSID_IS_EMPTY(id) || JSID_IS_STRING(id));
    JS_ASSERT_IF(!JSID_IS_EMPTY(id), id == IdToTypeId(id));
    JS_ASSERT(!unknownProperties());

    Property *prop = HashSetLookup<RawId,Property,Property>
        (propertySet, basePropertyCount(), id);

    return prop ? &prop->types : NULL;
}

inline unsigned
TypeObject::getPropertyCount()
{
    uint32_t count = basePropertyCount();
    if (count > SET_ARRAY_SIZE)
        return HashSetCapacity(count);
    return count;
}

inline Property *
TypeObject::getProperty(unsigned i)
{
    JS_ASSERT(i < getPropertyCount());
    if (basePropertyCount() == 1) {
        JS_ASSERT(i == 0);
        return (Property *) propertySet;
    }
    return propertySet[i];
}

inline void
TypeObject::writeBarrierPre(TypeObject *type)
{
#ifdef JSGC_INCREMENTAL
    if (!type)
        return;

    JS::Zone *zone = type->zone();
    if (zone->needsBarrier()) {
        TypeObject *tmp = type;
        MarkTypeObjectUnbarriered(zone->barrierTracer(), &tmp, "write barrier");
        JS_ASSERT(tmp == type);
    }
#endif
}

inline void
TypeObject::writeBarrierPost(TypeObject *type, void *addr)
{
}

inline void
TypeObject::readBarrier(TypeObject *type)
{
#ifdef JSGC_INCREMENTAL
    JS::Zone *zone = type->zone();
    if (zone->needsBarrier()) {
        TypeObject *tmp = type;
        MarkTypeObjectUnbarriered(zone->barrierTracer(), &tmp, "read barrier");
        JS_ASSERT(tmp == type);
    }
#endif
}

inline void
TypeNewScript::writeBarrierPre(TypeNewScript *newScript)
{
#ifdef JSGC_INCREMENTAL
    if (!newScript)
        return;

    JS::Zone *zone = newScript->fun->zone();
    if (zone->needsBarrier()) {
        MarkObject(zone->barrierTracer(), &newScript->fun, "write barrier");
        MarkShape(zone->barrierTracer(), &newScript->shape, "write barrier");
    }
#endif
}

inline void
TypeNewScript::writeBarrierPost(TypeNewScript *newScript, void *addr)
{
}

inline
Property::Property(jsid id)
  : id(id)
{
}

inline
Property::Property(const Property &o)
  : id(o.id.get()), types(o.types)
{
}

} } 

inline bool
JSScript::ensureHasTypes(JSContext *cx)
{
    return types || makeTypes(cx);
}

inline bool
JSScript::ensureRanAnalysis(JSContext *cx)
{
    js::types::AutoEnterAnalysis aea(cx);

    if (!ensureHasTypes(cx))
        return false;
    if (!hasAnalysis() && !makeAnalysis(cx))
        return false;
    JS_ASSERT(analysis()->ranBytecode());
    return true;
}

inline bool
JSScript::ensureRanInference(JSContext *cx)
{
    if (!ensureRanAnalysis(cx))
        return false;
    if (!analysis()->ranInference()) {
        js::types::AutoEnterAnalysis enter(cx);
        analysis()->analyzeTypes(cx);
    }
    return !analysis()->OOM() &&
        !cx->compartment->types.pendingNukeTypes;
}

inline bool
JSScript::hasAnalysis()
{
    return types && types->analysis;
}

inline js::analyze::ScriptAnalysis *
JSScript::analysis()
{
    JS_ASSERT(hasAnalysis());
    return types->analysis;
}

inline void
JSScript::clearAnalysis()
{
    if (types)
        types->analysis = NULL;
}

inline void
JSScript::clearPropertyReadTypes()
{
    if (types && types->propertyReadTypes)
        types->propertyReadTypes = NULL;
}

inline void
js::analyze::ScriptAnalysis::addPushedType(JSContext *cx, uint32_t offset, uint32_t which,
                                           js::types::Type type)
{
    js::types::TypeSet *pushed = pushedTypes(offset, which);
    pushed->addType(cx, type);
}

namespace js {

template <>
struct RootMethods<const types::Type>
{
    static types::Type initial() { return types::Type::UnknownType(); }
    static ThingRootKind kind() { return THING_ROOT_TYPE; }
    static bool poisoned(const types::Type &v) {
        return (v.isTypeObject() && IsPoisonedPtr(v.typeObject()))
            || (v.isSingleObject() && IsPoisonedPtr(v.singleObject()));
    }
};

template <>
struct RootMethods<types::Type>
{
    static types::Type initial() { return types::Type::UnknownType(); }
    static ThingRootKind kind() { return THING_ROOT_TYPE; }
    static bool poisoned(const types::Type &v) {
        return (v.isTypeObject() && IsPoisonedPtr(v.typeObject()))
            || (v.isSingleObject() && IsPoisonedPtr(v.singleObject()));
    }
};

} 

namespace JS {
template<> class AnchorPermitted<js::types::TypeObject *> { };
}  

#endif 
