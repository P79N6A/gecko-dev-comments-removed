





#include "jsinferinlines.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"

#include "jsapi.h"
#include "jscntxt.h"
#include "jsgc.h"
#include "jshashutil.h"
#include "jsobj.h"
#include "jsprf.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jsworkers.h"
#include "prmjtime.h"

#include "gc/Marking.h"
#ifdef JS_ION
#include "jit/BaselineJIT.h"
#include "jit/Ion.h"
#include "jit/IonAnalysis.h"
#include "jit/JitCompartment.h"
#endif
#include "js/MemoryMetrics.h"
#include "vm/Opcodes.h"
#include "vm/Shape.h"

#include "jsatominlines.h"
#include "jsgcinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "jit/ExecutionMode-inl.h"

using namespace js;
using namespace js::gc;
using namespace js::types;
using namespace js::analyze;

using mozilla::DebugOnly;
using mozilla::Maybe;
using mozilla::PodArrayZero;
using mozilla::PodCopy;
using mozilla::PodZero;

static inline jsid
id_prototype(JSContext *cx) {
    return NameToId(cx->names().prototype);
}

static inline jsid
id___proto__(JSContext *cx) {
    return NameToId(cx->names().proto);
}

static inline jsid
id_constructor(JSContext *cx) {
    return NameToId(cx->names().constructor);
}

static inline jsid
id_caller(JSContext *cx) {
    return NameToId(cx->names().caller);
}

#ifdef DEBUG
const char *
types::TypeIdStringImpl(jsid id)
{
    if (JSID_IS_VOID(id))
        return "(index)";
    if (JSID_IS_EMPTY(id))
        return "(new)";
    static char bufs[4][100];
    static unsigned which = 0;
    which = (which + 1) & 3;
    PutEscapedString(bufs[which], 100, JSID_TO_FLAT_STRING(id), 0);
    return bufs[which];
}
#endif





#ifdef DEBUG

static bool InferSpewActive(SpewChannel channel)
{
    static bool active[SPEW_COUNT];
    static bool checked = false;
    if (!checked) {
        checked = true;
        PodArrayZero(active);
        const char *env = getenv("INFERFLAGS");
        if (!env)
            return false;
        if (strstr(env, "ops"))
            active[ISpewOps] = true;
        if (strstr(env, "result"))
            active[ISpewResult] = true;
        if (strstr(env, "full")) {
            for (unsigned i = 0; i < SPEW_COUNT; i++)
                active[i] = true;
        }
    }
    return active[channel];
}

static bool InferSpewColorable()
{
    
    static bool colorable = false;
    static bool checked = false;
    if (!checked) {
        checked = true;
        const char *env = getenv("TERM");
        if (!env)
            return false;
        if (strcmp(env, "xterm-color") == 0 || strcmp(env, "xterm-256color") == 0)
            colorable = true;
    }
    return colorable;
}

const char *
types::InferSpewColorReset()
{
    if (!InferSpewColorable())
        return "";
    return "\x1b[0m";
}

const char *
types::InferSpewColor(TypeConstraint *constraint)
{
    
    static const char * const colors[] = { "\x1b[31m", "\x1b[32m", "\x1b[33m",
                                           "\x1b[34m", "\x1b[35m", "\x1b[36m",
                                           "\x1b[37m" };
    if (!InferSpewColorable())
        return "";
    return colors[DefaultHasher<TypeConstraint *>::hash(constraint) % 7];
}

const char *
types::InferSpewColor(TypeSet *types)
{
    
    static const char * const colors[] = { "\x1b[1;31m", "\x1b[1;32m", "\x1b[1;33m",
                                           "\x1b[1;34m", "\x1b[1;35m", "\x1b[1;36m",
                                           "\x1b[1;37m" };
    if (!InferSpewColorable())
        return "";
    return colors[DefaultHasher<TypeSet *>::hash(types) % 7];
}

const char *
types::TypeString(Type type)
{
    if (type.isPrimitive()) {
        switch (type.primitive()) {
          case JSVAL_TYPE_UNDEFINED:
            return "void";
          case JSVAL_TYPE_NULL:
            return "null";
          case JSVAL_TYPE_BOOLEAN:
            return "bool";
          case JSVAL_TYPE_INT32:
            return "int";
          case JSVAL_TYPE_DOUBLE:
            return "float";
          case JSVAL_TYPE_STRING:
            return "string";
          case JSVAL_TYPE_MAGIC:
            return "lazyargs";
          default:
            MOZ_ASSUME_UNREACHABLE("Bad type");
        }
    }
    if (type.isUnknown())
        return "unknown";
    if (type.isAnyObject())
        return " object";

    static char bufs[4][40];
    static unsigned which = 0;
    which = (which + 1) & 3;

    if (type.isSingleObject())
        JS_snprintf(bufs[which], 40, "<0x%p>", (void *) type.singleObject());
    else
        JS_snprintf(bufs[which], 40, "[0x%p]", (void *) type.typeObject());

    return bufs[which];
}

const char *
types::TypeObjectString(TypeObject *type)
{
    return TypeString(Type::ObjectType(type));
}

unsigned JSScript::id() {
    if (!id_) {
        id_ = ++compartment()->types.scriptCount;
        InferSpew(ISpewOps, "script #%u: %p %s:%d",
                  id_, this, filename() ? filename() : "<null>", lineno());
    }
    return id_;
}

void
types::InferSpew(SpewChannel channel, const char *fmt, ...)
{
    if (!InferSpewActive(channel))
        return;

    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[infer] ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

bool
types::TypeHasProperty(JSContext *cx, TypeObject *obj, jsid id, const Value &value)
{
    



    if (!obj->unknownProperties() && !value.isUndefined()) {
        id = IdToTypeId(id);

        
        if (id == id___proto__(cx) || id == id_constructor(cx) || id == id_caller(cx))
            return true;

        Type type = GetValueType(value);

        AutoEnterAnalysis enter(cx);

        




        TypeSet *types = obj->maybeGetProperty(id);
        if (!types)
            return true;

        if (!types->hasType(type)) {
            TypeFailure(cx, "Missing type in object %s %s: %s",
                        TypeObjectString(obj), TypeIdString(id), TypeString(type));
        }
    }
    return true;
}

#endif

void
types::TypeFailure(JSContext *cx, const char *fmt, ...)
{
    char msgbuf[1024]; 
    char errbuf[1024];

    va_list ap;
    va_start(ap, fmt);
    JS_vsnprintf(errbuf, sizeof(errbuf), fmt, ap);
    va_end(ap);

    JS_snprintf(msgbuf, sizeof(msgbuf), "[infer failure] %s", errbuf);

    
    cx->compartment()->types.print(cx, true);

    MOZ_ReportAssertionFailure(msgbuf, __FILE__, __LINE__);
    MOZ_CRASH();
}





TemporaryTypeSet::TemporaryTypeSet(Type type)
{
    if (type.isUnknown()) {
        flags |= TYPE_FLAG_BASE_MASK;
    } else if (type.isPrimitive()) {
        flags = PrimitiveTypeFlag(type.primitive());
        if (flags == TYPE_FLAG_DOUBLE)
            flags |= TYPE_FLAG_INT32;
    } else if (type.isAnyObject()) {
        flags |= TYPE_FLAG_ANYOBJECT;
    } else  if (type.isTypeObject() && type.typeObject()->unknownProperties()) {
        flags |= TYPE_FLAG_ANYOBJECT;
    } else {
        setBaseObjectCount(1);
        objectSet = reinterpret_cast<TypeObjectKey**>(type.objectKey());
    }
}

bool
TypeSet::mightBeMIRType(jit::MIRType type)
{
    if (unknown())
        return true;

    if (type == jit::MIRType_Object)
        return unknownObject() || baseObjectCount() != 0;

    switch (type) {
      case jit::MIRType_Undefined:
        return baseFlags() & TYPE_FLAG_UNDEFINED;
      case jit::MIRType_Null:
        return baseFlags() & TYPE_FLAG_NULL;
      case jit::MIRType_Boolean:
        return baseFlags() & TYPE_FLAG_BOOLEAN;
      case jit::MIRType_Int32:
        return baseFlags() & TYPE_FLAG_INT32;
      case jit::MIRType_Float32: 
      case jit::MIRType_Double:
        return baseFlags() & TYPE_FLAG_DOUBLE;
      case jit::MIRType_String:
        return baseFlags() & TYPE_FLAG_STRING;
      case jit::MIRType_MagicOptimizedArguments:
        return baseFlags() & TYPE_FLAG_LAZYARGS;
      case jit::MIRType_MagicHole:
      case jit::MIRType_MagicIsConstructing:
        
        
        
        
        
        
        
        
        
        return false;
      default:
        MOZ_ASSUME_UNREACHABLE("Bad MIR type");
    }
}

bool
TypeSet::isSubset(TypeSet *other)
{
    if ((baseFlags() & other->baseFlags()) != baseFlags())
        return false;

    if (unknownObject()) {
        JS_ASSERT(other->unknownObject());
    } else {
        for (unsigned i = 0; i < getObjectCount(); i++) {
            TypeObjectKey *obj = getObject(i);
            if (!obj)
                continue;
            if (!other->hasType(Type::ObjectType(obj)))
                return false;
        }
    }

    return true;
}

bool
TypeSet::enumerateTypes(TypeList *list)
{
    
    if (flags & TYPE_FLAG_UNKNOWN)
        return list->append(Type::UnknownType());

    
    for (TypeFlags flag = 1; flag < TYPE_FLAG_ANYOBJECT; flag <<= 1) {
        if (flags & flag) {
            Type type = Type::PrimitiveType(TypeFlagPrimitive(flag));
            if (!list->append(type))
                return false;
        }
    }

    
    if (flags & TYPE_FLAG_ANYOBJECT)
        return list->append(Type::AnyObjectType());

    
    unsigned count = getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        TypeObjectKey *object = getObject(i);
        if (object) {
            if (!list->append(Type::ObjectType(object)))
                return false;
        }
    }

    return true;
}

inline bool
TypeSet::addTypesToConstraint(JSContext *cx, TypeConstraint *constraint)
{
    



    TypeList types;
    if (!enumerateTypes(&types))
        return false;

    for (unsigned i = 0; i < types.length(); i++)
        constraint->newType(cx, this, types[i]);

    return true;
}

bool
ConstraintTypeSet::addConstraint(JSContext *cx, TypeConstraint *constraint, bool callExisting)
{
    if (!constraint) {
        
        return false;
    }

    JS_ASSERT(cx->compartment()->activeAnalysis);

    InferSpew(ISpewOps, "addConstraint: %sT%p%s %sC%p%s %s",
              InferSpewColor(this), this, InferSpewColorReset(),
              InferSpewColor(constraint), constraint, InferSpewColorReset(),
              constraint->kind());

    JS_ASSERT(constraint->next == nullptr);
    constraint->next = constraintList;
    constraintList = constraint;

    if (callExisting)
        return addTypesToConstraint(cx, constraint);
    return true;
}

void
TypeSet::print()
{
    if (flags & TYPE_FLAG_NON_DATA_PROPERTY)
        fprintf(stderr, " [non-data]");

    if (flags & TYPE_FLAG_NON_WRITABLE_PROPERTY)
        fprintf(stderr, " [non-writable]");

    if (definiteProperty())
        fprintf(stderr, " [definite:%d]", definiteSlot());

    if (baseFlags() == 0 && !baseObjectCount()) {
        fprintf(stderr, " missing");
        return;
    }

    if (flags & TYPE_FLAG_UNKNOWN)
        fprintf(stderr, " unknown");
    if (flags & TYPE_FLAG_ANYOBJECT)
        fprintf(stderr, " object");

    if (flags & TYPE_FLAG_UNDEFINED)
        fprintf(stderr, " void");
    if (flags & TYPE_FLAG_NULL)
        fprintf(stderr, " null");
    if (flags & TYPE_FLAG_BOOLEAN)
        fprintf(stderr, " bool");
    if (flags & TYPE_FLAG_INT32)
        fprintf(stderr, " int");
    if (flags & TYPE_FLAG_DOUBLE)
        fprintf(stderr, " float");
    if (flags & TYPE_FLAG_STRING)
        fprintf(stderr, " string");
    if (flags & TYPE_FLAG_LAZYARGS)
        fprintf(stderr, " lazyargs");

    uint32_t objectCount = baseObjectCount();
    if (objectCount) {
        fprintf(stderr, " object[%u]", objectCount);

        unsigned count = getObjectCount();
        for (unsigned i = 0; i < count; i++) {
            TypeObjectKey *object = getObject(i);
            if (object)
                fprintf(stderr, " %s", TypeString(Type::ObjectType(object)));
        }
    }
}

bool
TypeSet::clone(LifoAlloc *alloc, TemporaryTypeSet *result) const
{
    JS_ASSERT(result->empty());

    unsigned objectCount = baseObjectCount();
    unsigned capacity = (objectCount >= 2) ? HashSetCapacity(objectCount) : 0;

    TypeObjectKey **newSet;
    if (capacity) {
        newSet = alloc->newArray<TypeObjectKey*>(capacity);
        if (!newSet)
            return false;
        PodCopy(newSet, objectSet, capacity);
    }

    new(result) TemporaryTypeSet(flags, capacity ? newSet : objectSet);
    return true;
}

TemporaryTypeSet *
TypeSet::clone(LifoAlloc *alloc) const
{
    TemporaryTypeSet *res = alloc->new_<TemporaryTypeSet>();
    if (!res || !clone(alloc, res))
        return nullptr;
    return res;
}

TemporaryTypeSet *
TypeSet::filter(LifoAlloc *alloc, bool filterUndefined, bool filterNull) const
{
    TemporaryTypeSet *res = clone(alloc);
    if (!res)
        return nullptr;

    if (filterUndefined)
        res->flags = res->flags & ~TYPE_FLAG_UNDEFINED;

    if (filterNull)
        res->flags = res->flags & ~TYPE_FLAG_NULL;

    return res;
}

 TemporaryTypeSet *
TypeSet::unionSets(TypeSet *a, TypeSet *b, LifoAlloc *alloc)
{
    TemporaryTypeSet *res = alloc->new_<TemporaryTypeSet>(a->baseFlags() | b->baseFlags(),
                                                          static_cast<TypeObjectKey**>(nullptr));
    if (!res)
        return nullptr;

    if (!res->unknownObject()) {
        for (size_t i = 0; i < a->getObjectCount() && !res->unknownObject(); i++) {
            if (TypeObjectKey *key = a->getObject(i))
                res->addType(Type::ObjectType(key), alloc);
        }
        for (size_t i = 0; i < b->getObjectCount() && !res->unknownObject(); i++) {
            if (TypeObjectKey *key = b->getObject(i))
                res->addType(Type::ObjectType(key), alloc);
        }
    }

    return res;
}

























class CompilerConstraint
{
  public:
    
    HeapTypeSetKey property;

    
    
    
    TemporaryTypeSet *expected;

    CompilerConstraint(LifoAlloc *alloc, const HeapTypeSetKey &property)
      : property(property),
        expected(property.maybeTypes() ? property.maybeTypes()->clone(alloc) : nullptr)
    {}

    
    
    virtual bool generateTypeConstraint(JSContext *cx, RecompileInfo recompileInfo) = 0;
};

class types::CompilerConstraintList
{
  public:
    struct FrozenScript
    {
        JSScript *script;
        TemporaryTypeSet *thisTypes;
        TemporaryTypeSet *argTypes;
        TemporaryTypeSet *bytecodeTypes;
    };

  private:

    
    bool failed_;

#ifdef JS_ION
    
    LifoAlloc *alloc_;

    
    Vector<CompilerConstraint *, 0, jit::IonAllocPolicy> constraints;

    
    Vector<FrozenScript, 1, jit::IonAllocPolicy> frozenScripts;
#endif

  public:
    CompilerConstraintList(jit::TempAllocator &alloc)
      : failed_(false)
#ifdef JS_ION
      , alloc_(alloc.lifoAlloc())
      , constraints(alloc)
      , frozenScripts(alloc)
#endif
    {}

    void add(CompilerConstraint *constraint) {
#ifdef JS_ION
        if (!constraint || !constraints.append(constraint))
            setFailed();
#else
        MOZ_CRASH();
#endif
    }

    void freezeScript(JSScript *script,
                      TemporaryTypeSet *thisTypes,
                      TemporaryTypeSet *argTypes,
                      TemporaryTypeSet *bytecodeTypes)
    {
#ifdef JS_ION
        FrozenScript entry;
        entry.script = script;
        entry.thisTypes = thisTypes;
        entry.argTypes = argTypes;
        entry.bytecodeTypes = bytecodeTypes;
        if (!frozenScripts.append(entry))
            setFailed();
#else
        MOZ_CRASH();
#endif
    }

    size_t length() {
#ifdef JS_ION
        return constraints.length();
#else
        MOZ_CRASH();
#endif
    }

    CompilerConstraint *get(size_t i) {
#ifdef JS_ION
        return constraints[i];
#else
        MOZ_CRASH();
#endif
    }

    size_t numFrozenScripts() {
#ifdef JS_ION
        return frozenScripts.length();
#else
        MOZ_CRASH();
#endif
    }

    const FrozenScript &frozenScript(size_t i) {
#ifdef JS_ION
        return frozenScripts[i];
#else
        MOZ_CRASH();
#endif
    }

    bool failed() {
        return failed_;
    }
    void setFailed() {
        failed_ = true;
    }
    LifoAlloc *alloc() const {
#ifdef JS_ION
        return alloc_;
#else
        MOZ_CRASH();
#endif
    }
};

CompilerConstraintList *
types::NewCompilerConstraintList(jit::TempAllocator &alloc)
{
#ifdef JS_ION
    return alloc.lifoAlloc()->new_<CompilerConstraintList>(alloc);
#else
    MOZ_CRASH();
#endif
}

 bool
TypeScript::FreezeTypeSets(CompilerConstraintList *constraints, JSScript *script,
                           TemporaryTypeSet **pThisTypes,
                           TemporaryTypeSet **pArgTypes,
                           TemporaryTypeSet **pBytecodeTypes)
{
    LifoAlloc *alloc = constraints->alloc();
    StackTypeSet *existing = script->types->typeArray();

    size_t count = NumTypeSets(script);
    TemporaryTypeSet *types = alloc->newArrayUninitialized<TemporaryTypeSet>(count);
    if (!types)
        return false;
    PodZero(types, count);

    for (size_t i = 0; i < count; i++) {
        if (!existing[i].clone(alloc, &types[i]))
            return false;
    }

    *pThisTypes = types + (ThisTypes(script) - existing);
    *pArgTypes = (script->functionNonDelazifying() && script->functionNonDelazifying()->nargs())
                 ? (types + (ArgTypes(script, 0) - existing))
                 : nullptr;
    *pBytecodeTypes = types;

    constraints->freezeScript(script, *pThisTypes, *pArgTypes, *pBytecodeTypes);
    return true;
}

namespace {

template <typename T>
class CompilerConstraintInstance : public CompilerConstraint
{
    T data;

  public:
    CompilerConstraintInstance<T>(LifoAlloc *alloc, const HeapTypeSetKey &property, const T &data)
      : CompilerConstraint(alloc, property), data(data)
    {}

    bool generateTypeConstraint(JSContext *cx, RecompileInfo recompileInfo);
};


template <typename T>
class TypeCompilerConstraint : public TypeConstraint
{
    
    RecompileInfo compilation;

    T data;

  public:
    TypeCompilerConstraint<T>(RecompileInfo compilation, const T &data)
      : compilation(compilation), data(data)
    {}

    const char *kind() { return data.kind(); }

    void newType(JSContext *cx, TypeSet *source, Type type) {
        if (data.invalidateOnNewType(type))
            cx->zone()->types.addPendingRecompile(cx, compilation);
    }

    void newPropertyState(JSContext *cx, TypeSet *source) {
        if (data.invalidateOnNewPropertyState(source))
            cx->zone()->types.addPendingRecompile(cx, compilation);
    }

    void newObjectState(JSContext *cx, TypeObject *object) {
        
        
        
        if (object->unknownProperties() || data.invalidateOnNewObjectState(object))
            cx->zone()->types.addPendingRecompile(cx, compilation);
    }

    bool sweep(TypeZone &zone, TypeConstraint **res) {
        if (data.shouldSweep() || compilation.shouldSweep(zone))
            return false;
        *res = zone.typeLifoAlloc.new_<TypeCompilerConstraint<T> >(compilation, data);
        return true;
    }
};

template <typename T>
bool
CompilerConstraintInstance<T>::generateTypeConstraint(JSContext *cx, RecompileInfo recompileInfo)
{
    if (property.object()->unknownProperties())
        return false;

    if (!property.instantiate(cx))
        return false;

    if (!data.constraintHolds(cx, property, expected))
        return false;

    return property.maybeTypes()->addConstraint(cx, cx->typeLifoAlloc().new_<TypeCompilerConstraint<T> >(recompileInfo, data),
                                                 false);
}

} 

const Class *
TypeObjectKey::clasp()
{
    return isTypeObject() ? asTypeObject()->clasp() : asSingleObject()->getClass();
}

TaggedProto
TypeObjectKey::proto()
{
    JS_ASSERT(hasTenuredProto());
    return isTypeObject() ? asTypeObject()->proto() : asSingleObject()->getTaggedProto();
}

bool
ObjectImpl::hasTenuredProto() const
{
    return type_->hasTenuredProto();
}

bool
TypeObjectKey::hasTenuredProto()
{
    return isTypeObject() ? asTypeObject()->hasTenuredProto() : asSingleObject()->hasTenuredProto();
}

JSObject *
TypeObjectKey::singleton()
{
    return isTypeObject() ? asTypeObject()->singleton() : asSingleObject();
}

TypeNewScript *
TypeObjectKey::newScript()
{
    if (isTypeObject() && asTypeObject()->hasNewScript())
        return asTypeObject()->newScript();
    return nullptr;
}

TypeObject *
TypeObjectKey::maybeType()
{
    if (isTypeObject())
        return asTypeObject();
    if (asSingleObject()->hasLazyType())
        return nullptr;
    return asSingleObject()->type();
}

bool
TypeObjectKey::unknownProperties()
{
    if (TypeObject *type = maybeType())
        return type->unknownProperties();
    return false;
}

HeapTypeSetKey
TypeObjectKey::property(jsid id)
{
    JS_ASSERT(!unknownProperties());

    HeapTypeSetKey property;
    property.object_ = this;
    property.id_ = id;
    if (TypeObject *type = maybeType())
        property.maybeTypes_ = type->maybeGetProperty(id);

    return property;
}

void
TypeObjectKey::ensureTrackedProperty(JSContext *cx, jsid id)
{
#ifdef JS_ION
    
    
    
    if (!JSID_IS_VOID(id) && !JSID_IS_EMPTY(id)) {
        JS_ASSERT(CurrentThreadCanAccessRuntime(cx->runtime()));
        if (JSObject *obj = singleton()) {
            if (obj->isNative() && obj->nativeLookupPure(id))
                EnsureTrackPropertyTypes(cx, obj, id);
        }
    }
#endif 
}

bool
HeapTypeSetKey::instantiate(JSContext *cx)
{
    if (maybeTypes())
        return true;
    if (object()->isSingleObject() && !object()->asSingleObject()->getType(cx)) {
        cx->clearPendingException();
        return false;
    }
    maybeTypes_ = object()->maybeType()->getProperty(cx, id());
    return maybeTypes_ != nullptr;
}

static bool
CheckFrozenTypeSet(JSContext *cx, TemporaryTypeSet *frozen, StackTypeSet *actual)
{
    
    
    
    
    

    if (!actual->isSubset(frozen))
        return false;

    if (!frozen->isSubset(actual)) {
        TypeSet::TypeList list;
        frozen->enumerateTypes(&list);

        for (size_t i = 0; i < list.length(); i++)
            actual->addType(cx, list[i]);
    }

    return true;
}

namespace {






class TypeConstraintFreezeStack : public TypeConstraint
{
    JSScript *script_;

  public:
    TypeConstraintFreezeStack(JSScript *script)
        : script_(script)
    {}

    const char *kind() { return "freezeStack"; }

    void newType(JSContext *cx, TypeSet *source, Type type) {
        



        cx->zone()->types.addPendingRecompile(cx, script_);
    }

    bool sweep(TypeZone &zone, TypeConstraint **res) {
        if (IsScriptAboutToBeFinalized(&script_))
            return false;
        *res = zone.typeLifoAlloc.new_<TypeConstraintFreezeStack>(script_);
        return true;
    }
};

} 

bool
types::FinishCompilation(JSContext *cx, HandleScript script, ExecutionMode executionMode,
                         CompilerConstraintList *constraints, RecompileInfo *precompileInfo)
{
    if (constraints->failed())
        return false;

    CompilerOutput co(script, executionMode);

    TypeZone &types = cx->zone()->types;
    if (!types.compilerOutputs) {
        types.compilerOutputs = cx->new_< Vector<CompilerOutput> >(cx);
        if (!types.compilerOutputs)
            return false;
    }

#ifdef DEBUG
    for (size_t i = 0; i < types.compilerOutputs->length(); i++) {
        const CompilerOutput &co = (*types.compilerOutputs)[i];
        JS_ASSERT_IF(co.isValid(), co.script() != script || co.mode() != executionMode);
    }
#endif

    uint32_t index = types.compilerOutputs->length();
    if (!types.compilerOutputs->append(co))
        return false;

    *precompileInfo = RecompileInfo(index);

    bool succeeded = true;

    for (size_t i = 0; i < constraints->length(); i++) {
        CompilerConstraint *constraint = constraints->get(i);
        if (!constraint->generateTypeConstraint(cx, *precompileInfo))
            succeeded = false;
    }

    for (size_t i = 0; i < constraints->numFrozenScripts(); i++) {
        const CompilerConstraintList::FrozenScript &entry = constraints->frozenScript(i);
        JS_ASSERT(entry.script->types);

        if (!CheckFrozenTypeSet(cx, entry.thisTypes, types::TypeScript::ThisTypes(entry.script)))
            succeeded = false;
        unsigned nargs = entry.script->functionNonDelazifying()
                         ? entry.script->functionNonDelazifying()->nargs()
                         : 0;
        for (size_t i = 0; i < nargs; i++) {
            if (!CheckFrozenTypeSet(cx, &entry.argTypes[i], types::TypeScript::ArgTypes(entry.script, i)))
                succeeded = false;
        }
        for (size_t i = 0; i < entry.script->nTypeSets(); i++) {
            if (!CheckFrozenTypeSet(cx, &entry.bytecodeTypes[i], &entry.script->types->typeArray()[i]))
                succeeded = false;
        }

        
        
        if (entry.script->hasFreezeConstraints())
            continue;
        entry.script->setHasFreezeConstraints();

        size_t count = TypeScript::NumTypeSets(entry.script);

        StackTypeSet *array = entry.script->types->typeArray();
        for (size_t i = 0; i < count; i++) {
            if (!array[i].addConstraint(cx, cx->typeLifoAlloc().new_<TypeConstraintFreezeStack>(entry.script), false))
                succeeded = false;
        }
    }

    if (!succeeded || types.compilerOutputs->back().pendingInvalidation()) {
        types.compilerOutputs->back().invalidate();
        script->resetUseCount();
        return false;
    }

    return true;
}

static void
CheckDefinitePropertiesTypeSet(JSContext *cx, TemporaryTypeSet *frozen, StackTypeSet *actual)
{
    
    
    
    
    
    if (!frozen->isSubset(actual)) {
        TypeSet::TypeList list;
        frozen->enumerateTypes(&list);

        for (size_t i = 0; i < list.length(); i++)
            actual->addType(cx, list[i]);
    }
}

void
types::FinishDefinitePropertiesAnalysis(JSContext *cx, CompilerConstraintList *constraints)
{
#ifdef DEBUG
    
    
    
    
    for (size_t i = 0; i < constraints->numFrozenScripts(); i++) {
        const CompilerConstraintList::FrozenScript &entry = constraints->frozenScript(i);
        JSScript *script = entry.script;
        JS_ASSERT(script->types);

        JS_ASSERT(TypeScript::ThisTypes(script)->isSubset(entry.thisTypes));

        unsigned nargs = entry.script->functionNonDelazifying()
                         ? entry.script->functionNonDelazifying()->nargs()
                         : 0;
        for (size_t j = 0; j < nargs; j++)
            JS_ASSERT(TypeScript::ArgTypes(script, j)->isSubset(&entry.argTypes[j]));

        for (size_t j = 0; j < script->nTypeSets(); j++)
            JS_ASSERT(script->types->typeArray()[j].isSubset(&entry.bytecodeTypes[j]));
    }
#endif

    for (size_t i = 0; i < constraints->numFrozenScripts(); i++) {
        const CompilerConstraintList::FrozenScript &entry = constraints->frozenScript(i);
        JSScript *script = entry.script;
        JS_ASSERT(script->types);

        CheckDefinitePropertiesTypeSet(cx, entry.thisTypes, TypeScript::ThisTypes(script));

        unsigned nargs = script->functionNonDelazifying()
                         ? script->functionNonDelazifying()->nargs()
                         : 0;
        for (size_t j = 0; j < nargs; j++)
            CheckDefinitePropertiesTypeSet(cx, &entry.argTypes[j], TypeScript::ArgTypes(script, j));

        for (size_t j = 0; j < script->nTypeSets(); j++)
            CheckDefinitePropertiesTypeSet(cx, &entry.bytecodeTypes[j], &script->types->typeArray()[j]);
    }
}

namespace {


class ConstraintDataFreeze
{
  public:
    ConstraintDataFreeze() {}

    const char *kind() { return "freeze"; }

    bool invalidateOnNewType(Type type) { return true; }
    bool invalidateOnNewPropertyState(TypeSet *property) { return true; }
    bool invalidateOnNewObjectState(TypeObject *object) { return false; }

    bool constraintHolds(JSContext *cx,
                         const HeapTypeSetKey &property, TemporaryTypeSet *expected)
    {
        return expected
               ? property.maybeTypes()->isSubset(expected)
               : property.maybeTypes()->empty();
    }

    bool shouldSweep() { return false; }
};

} 

void
HeapTypeSetKey::freeze(CompilerConstraintList *constraints)
{
    LifoAlloc *alloc = constraints->alloc();

    typedef CompilerConstraintInstance<ConstraintDataFreeze> T;
    constraints->add(alloc->new_<T>(alloc, *this, ConstraintDataFreeze()));
}

static inline jit::MIRType
GetMIRTypeFromTypeFlags(TypeFlags flags)
{
    switch (flags) {
      case TYPE_FLAG_UNDEFINED:
        return jit::MIRType_Undefined;
      case TYPE_FLAG_NULL:
        return jit::MIRType_Null;
      case TYPE_FLAG_BOOLEAN:
        return jit::MIRType_Boolean;
      case TYPE_FLAG_INT32:
        return jit::MIRType_Int32;
      case (TYPE_FLAG_INT32 | TYPE_FLAG_DOUBLE):
        return jit::MIRType_Double;
      case TYPE_FLAG_STRING:
        return jit::MIRType_String;
      case TYPE_FLAG_LAZYARGS:
        return jit::MIRType_MagicOptimizedArguments;
      case TYPE_FLAG_ANYOBJECT:
        return jit::MIRType_Object;
      default:
        return jit::MIRType_Value;
    }
}

jit::MIRType
TemporaryTypeSet::getKnownMIRType()
{
    TypeFlags flags = baseFlags();
    jit::MIRType type;

    if (baseObjectCount())
        type = flags ? jit::MIRType_Value : jit::MIRType_Object;
    else
        type = GetMIRTypeFromTypeFlags(flags);

    






    DebugOnly<bool> empty = flags == 0 && baseObjectCount() == 0;
    JS_ASSERT_IF(empty, type == jit::MIRType_Value);

    return type;
}

jit::MIRType
HeapTypeSetKey::knownMIRType(CompilerConstraintList *constraints)
{
    TypeSet *types = maybeTypes();

    if (!types || types->unknown())
        return jit::MIRType_Value;

    TypeFlags flags = types->baseFlags() & ~TYPE_FLAG_ANYOBJECT;
    jit::MIRType type;

    if (types->unknownObject() || types->getObjectCount())
        type = flags ? jit::MIRType_Value : jit::MIRType_Object;
    else
        type = GetMIRTypeFromTypeFlags(flags);

    if (type != jit::MIRType_Value)
        freeze(constraints);

    






    JS_ASSERT_IF(types->empty(), type == jit::MIRType_Value);

    return type;
}

bool
HeapTypeSetKey::isOwnProperty(CompilerConstraintList *constraints)
{
    if (maybeTypes() && (!maybeTypes()->empty() || maybeTypes()->nonDataProperty()))
        return true;
    if (JSObject *obj = object()->singleton()) {
        if (CanHaveEmptyPropertyTypesForOwnProperty(obj))
            return true;
    }
    freeze(constraints);
    return false;
}

bool
HeapTypeSetKey::knownSubset(CompilerConstraintList *constraints, const HeapTypeSetKey &other)
{
    if (!maybeTypes() || maybeTypes()->empty()) {
        freeze(constraints);
        return true;
    }
    if (!other.maybeTypes() || !maybeTypes()->isSubset(other.maybeTypes()))
        return false;
    freeze(constraints);
    return true;
}

JSObject *
TemporaryTypeSet::getSingleton()
{
    if (baseFlags() != 0 || baseObjectCount() != 1)
        return nullptr;

    return getSingleObject(0);
}

JSObject *
HeapTypeSetKey::singleton(CompilerConstraintList *constraints)
{
    HeapTypeSet *types = maybeTypes();

    if (!types || types->nonDataProperty() || types->baseFlags() != 0 || types->getObjectCount() != 1)
        return nullptr;

    JSObject *obj = types->getSingleObject(0);

    if (obj)
        freeze(constraints);

    return obj;
}

bool
HeapTypeSetKey::needsBarrier(CompilerConstraintList *constraints)
{
    TypeSet *types = maybeTypes();
    if (!types)
        return false;
    bool result = types->unknownObject()
               || types->getObjectCount() > 0
               || types->hasAnyFlag(TYPE_FLAG_STRING);
    if (!result)
        freeze(constraints);
    return result;
}

namespace {


class ConstraintDataFreezeObjectFlags
{
  public:
    
    TypeObjectFlags flags;

    ConstraintDataFreezeObjectFlags(TypeObjectFlags flags)
      : flags(flags)
    {
        JS_ASSERT(flags);
    }

    const char *kind() { return "freezeObjectFlags"; }

    bool invalidateOnNewType(Type type) { return false; }
    bool invalidateOnNewPropertyState(TypeSet *property) { return false; }
    bool invalidateOnNewObjectState(TypeObject *object) {
        return object->hasAnyFlags(flags);
    }

    bool constraintHolds(JSContext *cx,
                         const HeapTypeSetKey &property, TemporaryTypeSet *expected)
    {
        return !invalidateOnNewObjectState(property.object()->maybeType());
    }

    bool shouldSweep() { return false; }
};

} 

bool
TypeObjectKey::hasFlags(CompilerConstraintList *constraints, TypeObjectFlags flags)
{
    JS_ASSERT(flags);

    if (TypeObject *type = maybeType()) {
        if (type->hasAnyFlags(flags))
            return true;
    }

    HeapTypeSetKey objectProperty = property(JSID_EMPTY);
    LifoAlloc *alloc = constraints->alloc();

    typedef CompilerConstraintInstance<ConstraintDataFreezeObjectFlags> T;
    constraints->add(alloc->new_<T>(alloc, objectProperty, ConstraintDataFreezeObjectFlags(flags)));
    return false;
}

bool
TemporaryTypeSet::hasObjectFlags(CompilerConstraintList *constraints, TypeObjectFlags flags)
{
    if (unknownObject())
        return true;

    



    if (baseObjectCount() == 0)
        return true;

    unsigned count = getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        TypeObjectKey *object = getObject(i);
        if (object && object->hasFlags(constraints, flags))
            return true;
    }

    return false;
}

gc::InitialHeap
TypeObject::initialHeap(CompilerConstraintList *constraints)
{
    
    
    

    if (shouldPreTenure())
        return gc::TenuredHeap;

    if (!canPreTenure())
        return gc::DefaultHeap;

    HeapTypeSetKey objectProperty = TypeObjectKey::get(this)->property(JSID_EMPTY);
    LifoAlloc *alloc = constraints->alloc();

    typedef CompilerConstraintInstance<ConstraintDataFreezeObjectFlags> T;
    constraints->add(alloc->new_<T>(alloc, objectProperty, ConstraintDataFreezeObjectFlags(OBJECT_FLAG_PRE_TENURE)));

    return gc::DefaultHeap;
}

namespace {





class ConstraintDataFreezeObjectForInlinedCall
{
  public:
    ConstraintDataFreezeObjectForInlinedCall()
    {}

    const char *kind() { return "freezeObjectForInlinedCall"; }

    bool invalidateOnNewType(Type type) { return false; }
    bool invalidateOnNewPropertyState(TypeSet *property) { return false; }
    bool invalidateOnNewObjectState(TypeObject *object) {
        
        
        return true;
    }

    bool constraintHolds(JSContext *cx,
                         const HeapTypeSetKey &property, TemporaryTypeSet *expected)
    {
        return true;
    }

    bool shouldSweep() { return false; }
};



class ConstraintDataFreezeObjectForNewScriptTemplate
{
    JSObject *templateObject;

  public:
    ConstraintDataFreezeObjectForNewScriptTemplate(JSObject *templateObject)
      : templateObject(templateObject)
    {}

    const char *kind() { return "freezeObjectForNewScriptTemplate"; }

    bool invalidateOnNewType(Type type) { return false; }
    bool invalidateOnNewPropertyState(TypeSet *property) { return false; }
    bool invalidateOnNewObjectState(TypeObject *object) {
        return !object->hasNewScript() || object->newScript()->templateObject != templateObject;
    }

    bool constraintHolds(JSContext *cx,
                         const HeapTypeSetKey &property, TemporaryTypeSet *expected)
    {
        return !invalidateOnNewObjectState(property.object()->maybeType());
    }

    bool shouldSweep() {
        
        return false;
    }
};



class ConstraintDataFreezeObjectForTypedArrayBuffer
{
    void *viewData;

  public:
    ConstraintDataFreezeObjectForTypedArrayBuffer(void *viewData)
      : viewData(viewData)
    {}

    const char *kind() { return "freezeObjectForTypedArrayBuffer"; }

    bool invalidateOnNewType(Type type) { return false; }
    bool invalidateOnNewPropertyState(TypeSet *property) { return false; }
    bool invalidateOnNewObjectState(TypeObject *object) {
        return object->singleton()->as<TypedArrayObject>().viewData() != viewData;
    }

    bool constraintHolds(JSContext *cx,
                         const HeapTypeSetKey &property, TemporaryTypeSet *expected)
    {
        return !invalidateOnNewObjectState(property.object()->maybeType());
    }

    bool shouldSweep() {
        
        return false;
    }
};

} 

void
TypeObjectKey::watchStateChangeForInlinedCall(CompilerConstraintList *constraints)
{
    HeapTypeSetKey objectProperty = property(JSID_EMPTY);
    LifoAlloc *alloc = constraints->alloc();

    typedef CompilerConstraintInstance<ConstraintDataFreezeObjectForInlinedCall> T;
    constraints->add(alloc->new_<T>(alloc, objectProperty, ConstraintDataFreezeObjectForInlinedCall()));
}

void
TypeObjectKey::watchStateChangeForNewScriptTemplate(CompilerConstraintList *constraints)
{
    JSObject *templateObject = asTypeObject()->newScript()->templateObject;
    HeapTypeSetKey objectProperty = property(JSID_EMPTY);
    LifoAlloc *alloc = constraints->alloc();

    typedef CompilerConstraintInstance<ConstraintDataFreezeObjectForNewScriptTemplate> T;
    constraints->add(alloc->new_<T>(alloc, objectProperty,
                                    ConstraintDataFreezeObjectForNewScriptTemplate(templateObject)));
}

void
TypeObjectKey::watchStateChangeForTypedArrayBuffer(CompilerConstraintList *constraints)
{
    void *viewData = asSingleObject()->as<TypedArrayObject>().viewData();
    HeapTypeSetKey objectProperty = property(JSID_EMPTY);
    LifoAlloc *alloc = constraints->alloc();

    typedef CompilerConstraintInstance<ConstraintDataFreezeObjectForTypedArrayBuffer> T;
    constraints->add(alloc->new_<T>(alloc, objectProperty,
                                    ConstraintDataFreezeObjectForTypedArrayBuffer(viewData)));
}

static void
ObjectStateChange(ExclusiveContext *cxArg, TypeObject *object, bool markingUnknown)
{
    if (object->unknownProperties())
        return;

    
    HeapTypeSet *types = object->maybeGetProperty(JSID_EMPTY);

    
    if (markingUnknown)
        object->addFlags(OBJECT_FLAG_DYNAMIC_MASK | OBJECT_FLAG_UNKNOWN_PROPERTIES);

    if (types) {
        if (JSContext *cx = cxArg->maybeJSContext()) {
            TypeConstraint *constraint = types->constraintList;
            while (constraint) {
                constraint->newObjectState(cx, object);
                constraint = constraint->next;
            }
        } else {
            JS_ASSERT(!types->constraintList);
        }
    }
}

static void
CheckNewScriptProperties(JSContext *cx, TypeObject *type, JSFunction *fun);

namespace {

class ConstraintDataFreezePropertyState
{
  public:
    enum Which {
        NON_DATA,
        NON_WRITABLE
    } which;

    ConstraintDataFreezePropertyState(Which which)
      : which(which)
    {}

    const char *kind() { return (which == NON_DATA) ? "freezeNonDataProperty" : "freezeNonWritableProperty"; }

    bool invalidateOnNewType(Type type) { return false; }
    bool invalidateOnNewPropertyState(TypeSet *property) {
        return (which == NON_DATA)
               ? property->nonDataProperty()
               : property->nonWritableProperty();
    }
    bool invalidateOnNewObjectState(TypeObject *object) { return false; }

    bool constraintHolds(JSContext *cx,
                         const HeapTypeSetKey &property, TemporaryTypeSet *expected)
    {
        return !invalidateOnNewPropertyState(property.maybeTypes());
    }

    bool shouldSweep() { return false; }
};

} 

bool
HeapTypeSetKey::nonData(CompilerConstraintList *constraints)
{
    if (maybeTypes() && maybeTypes()->nonDataProperty())
        return true;

    LifoAlloc *alloc = constraints->alloc();

    typedef CompilerConstraintInstance<ConstraintDataFreezePropertyState> T;
    constraints->add(alloc->new_<T>(alloc, *this,
                                    ConstraintDataFreezePropertyState(ConstraintDataFreezePropertyState::NON_DATA)));
    return false;
}

bool
HeapTypeSetKey::nonWritable(CompilerConstraintList *constraints)
{
    if (maybeTypes() && maybeTypes()->nonWritableProperty())
        return true;

    LifoAlloc *alloc = constraints->alloc();

    typedef CompilerConstraintInstance<ConstraintDataFreezePropertyState> T;
    constraints->add(alloc->new_<T>(alloc, *this,
                                    ConstraintDataFreezePropertyState(ConstraintDataFreezePropertyState::NON_WRITABLE)));
    return false;
}

bool
TemporaryTypeSet::filtersType(const TemporaryTypeSet *other, Type filteredType) const
{
    if (other->unknown())
        return unknown();

    for (TypeFlags flag = 1; flag < TYPE_FLAG_ANYOBJECT; flag <<= 1) {
        Type type = Type::PrimitiveType(TypeFlagPrimitive(flag));
        if (type != filteredType && other->hasType(type) && !hasType(type))
            return false;
    }

    if (other->unknownObject())
        return unknownObject();

    for (size_t i = 0; i < other->getObjectCount(); i++) {
        TypeObjectKey *key = other->getObject(i);
        if (key) {
            Type type = Type::ObjectType(key);
            if (type != filteredType && !hasType(type))
                return false;
        }
    }

    return true;
}

TemporaryTypeSet::DoubleConversion
TemporaryTypeSet::convertDoubleElements(CompilerConstraintList *constraints)
{
    if (unknownObject() || !getObjectCount())
        return AmbiguousDoubleConversion;

    bool alwaysConvert = true;
    bool maybeConvert = false;
    bool dontConvert = false;

    for (unsigned i = 0; i < getObjectCount(); i++) {
        TypeObjectKey *type = getObject(i);
        if (!type)
            continue;

        if (type->unknownProperties()) {
            alwaysConvert = false;
            continue;
        }

        HeapTypeSetKey property = type->property(JSID_VOID);
        property.freeze(constraints);

        
        
        
        
        if (!property.maybeTypes() ||
            !property.maybeTypes()->hasType(Type::DoubleType()) ||
            type->clasp() != &ArrayObject::class_)
        {
            dontConvert = true;
            alwaysConvert = false;
            continue;
        }

        
        
        
        if (property.knownMIRType(constraints) == jit::MIRType_Double &&
            !type->hasFlags(constraints, OBJECT_FLAG_NON_PACKED))
        {
            maybeConvert = true;
        } else {
            alwaysConvert = false;
        }
    }

    JS_ASSERT_IF(alwaysConvert, maybeConvert);

    if (maybeConvert && dontConvert)
        return AmbiguousDoubleConversion;
    if (alwaysConvert)
        return AlwaysConvertToDoubles;
    if (maybeConvert)
        return MaybeConvertToDoubles;
    return DontConvertToDoubles;
}

const Class *
TemporaryTypeSet::getKnownClass()
{
    if (unknownObject())
        return nullptr;

    const Class *clasp = nullptr;
    unsigned count = getObjectCount();

    for (unsigned i = 0; i < count; i++) {
        const Class *nclasp = getObjectClass(i);
        if (!nclasp)
            continue;

        if (clasp && clasp != nclasp)
            return nullptr;
        clasp = nclasp;
    }

    return clasp;
}

TemporaryTypeSet::ForAllResult
TemporaryTypeSet::forAllClasses(bool (*func)(const Class* clasp))
{
    if (unknownObject())
        return ForAllResult::MIXED;

    unsigned count = getObjectCount();
    if (count == 0)
        return ForAllResult::EMPTY;

    bool true_results = false;
    bool false_results = false;
    for (unsigned i = 0; i < count; i++) {
        const Class *clasp = getObjectClass(i);
        if (!clasp)
            return ForAllResult::MIXED;
        if (func(clasp)) {
            true_results = true;
            if (false_results) return ForAllResult::MIXED;
        }
        else {
            false_results = true;
            if (true_results) return ForAllResult::MIXED;
        }
    }

    JS_ASSERT(true_results != false_results);

    return true_results ? ForAllResult::ALL_TRUE : ForAllResult::ALL_FALSE;
}

int
TemporaryTypeSet::getTypedArrayType()
{
    const Class *clasp = getKnownClass();

    if (clasp && IsTypedArrayClass(clasp))
        return clasp - &TypedArrayObject::classes[0];
    return ScalarTypeDescr::TYPE_MAX;
}

bool
TemporaryTypeSet::isDOMClass()
{
    if (unknownObject())
        return false;

    unsigned count = getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        const Class *clasp = getObjectClass(i);
        if (clasp && !clasp->isDOMClass())
            return false;
    }

    return count > 0;
}

bool
TemporaryTypeSet::maybeCallable()
{
    if (!maybeObject())
        return false;

    if (unknownObject())
        return true;

    unsigned count = getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        const Class *clasp = getObjectClass(i);
        if (clasp && clasp->isCallable())
            return true;
    }

    return false;
}

bool
TemporaryTypeSet::maybeEmulatesUndefined()
{
    if (!maybeObject())
        return false;

    if (unknownObject())
        return true;

    unsigned count = getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        
        
        
        const Class *clasp = getObjectClass(i);
        if (clasp && (clasp->emulatesUndefined() || clasp->isProxy()))
            return true;
    }

    return false;
}

JSObject *
TemporaryTypeSet::getCommonPrototype()
{
    if (unknownObject())
        return nullptr;

    JSObject *proto = nullptr;
    unsigned count = getObjectCount();

    for (unsigned i = 0; i < count; i++) {
        TypeObjectKey *object = getObject(i);
        if (!object)
            continue;

        if (!object->hasTenuredProto())
            return nullptr;

        TaggedProto nproto = object->proto();
        if (proto) {
            if (nproto != proto)
                return nullptr;
        } else {
            if (!nproto.isObject())
                return nullptr;
            proto = nproto.toObject();
        }
    }

    return proto;
}

bool
TemporaryTypeSet::propertyNeedsBarrier(CompilerConstraintList *constraints, jsid id)
{
    if (unknownObject())
        return true;

    for (unsigned i = 0; i < getObjectCount(); i++) {
        TypeObjectKey *type = getObject(i);
        if (!type)
            continue;

        if (type->unknownProperties())
            return true;

        HeapTypeSetKey property = type->property(id);
        if (property.needsBarrier(constraints))
            return true;
    }

    return false;
}





TypeCompartment::TypeCompartment()
{
    PodZero(this);
}

TypeObject *
TypeCompartment::newTypeObject(ExclusiveContext *cx, const Class *clasp, Handle<TaggedProto> proto,
                               TypeObjectFlags initialFlags)
{
    JS_ASSERT_IF(proto.isObject(), cx->isInsideCurrentCompartment(proto.toObject()));

    if (cx->isJSContext()) {
        if (proto.isObject() && IsInsideNursery(cx->asJSContext()->runtime(), proto.toObject()))
            initialFlags |= OBJECT_FLAG_NURSERY_PROTO;
    }

    TypeObject *object = js::NewTypeObject(cx);
    if (!object)
        return nullptr;
    new(object) TypeObject(clasp, proto, initialFlags);

    return object;
}

static inline jsbytecode *
PreviousOpcode(HandleScript script, jsbytecode *pc)
{
    ScriptAnalysis *analysis = script->analysis();
    JS_ASSERT(analysis->isReachable(pc));

    if (pc == script->code())
        return nullptr;

    for (pc--;; pc--) {
        if (analysis->isReachable(pc))
            break;
    }

    return pc;
}





static inline jsbytecode *
FindPreviousInnerInitializer(HandleScript script, jsbytecode *initpc)
{
    if (!script->hasAnalysis())
        return nullptr;

    if (!script->analysis()->isReachable(initpc))
        return nullptr;

    








    if (*initpc != JSOP_NEWARRAY)
        return nullptr;

    jsbytecode *last = PreviousOpcode(script, initpc);
    if (!last || *last != JSOP_INITELEM_ARRAY)
        return nullptr;

    last = PreviousOpcode(script, last);
    if (!last || *last != JSOP_ENDINIT)
        return nullptr;

    




    size_t initDepth = 0;
    jsbytecode *previnit;
    for (previnit = last; previnit; previnit = PreviousOpcode(script, previnit)) {
        if (*previnit == JSOP_ENDINIT)
            initDepth++;
        if (*previnit == JSOP_NEWINIT ||
            *previnit == JSOP_NEWARRAY ||
            *previnit == JSOP_NEWOBJECT)
        {
            if (--initDepth == 0)
                break;
        }
    }

    if (!previnit || *previnit != JSOP_NEWARRAY)
        return nullptr;

    return previnit;
}

TypeObject *
TypeCompartment::addAllocationSiteTypeObject(JSContext *cx, AllocationSiteKey key)
{
    AutoEnterAnalysis enter(cx);

    if (!allocationSiteTable) {
        allocationSiteTable = cx->new_<AllocationSiteTable>();
        if (!allocationSiteTable || !allocationSiteTable->init()) {
            js_delete(allocationSiteTable);
            return nullptr;
        }
    }

    AllocationSiteTable::AddPtr p = allocationSiteTable->lookupForAdd(key);
    JS_ASSERT(!p);

    TypeObject *res = nullptr;

    




    jsbytecode *pc = key.script->offsetToPC(key.offset);
    RootedScript keyScript(cx, key.script);
    jsbytecode *prev = FindPreviousInnerInitializer(keyScript, pc);
    if (prev) {
        AllocationSiteKey nkey;
        nkey.script = key.script;
        nkey.offset = key.script->pcToOffset(prev);
        nkey.kind = JSProto_Array;

        AllocationSiteTable::Ptr p = cx->compartment()->types.allocationSiteTable->lookup(nkey);
        if (p)
            res = p->value();
    }

    if (!res) {
        RootedObject proto(cx);
        if (!GetBuiltinPrototype(cx, key.kind, &proto))
            return nullptr;

        Rooted<TaggedProto> tagged(cx, TaggedProto(proto));
        res = newTypeObject(cx, GetClassForProtoKey(key.kind), tagged, OBJECT_FLAG_FROM_ALLOCATION_SITE);
        if (!res)
            return nullptr;
        key.script = keyScript;
    }

    if (JSOp(*pc) == JSOP_NEWOBJECT) {
        




        RootedObject baseobj(cx, key.script->getObject(GET_UINT32_INDEX(pc)));

        if (!res->addDefiniteProperties(cx, baseobj))
            return nullptr;
    }

    if (!allocationSiteTable->add(p, key, res))
        return nullptr;

    return res;
}

static inline jsid
GetAtomId(JSContext *cx, JSScript *script, const jsbytecode *pc, unsigned offset)
{
    PropertyName *name = script->getName(GET_UINT32_INDEX(pc + offset));
    return IdToTypeId(NameToId(name));
}

bool
types::UseNewType(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    
















    if (JSOp(*pc) == JSOP_NEW)
        pc += JSOP_NEW_LENGTH;
    else if (JSOp(*pc) == JSOP_SPREADNEW)
        pc += JSOP_SPREADNEW_LENGTH;
    else
        return false;
    if (JSOp(*pc) == JSOP_SETPROP) {
        jsid id = GetAtomId(cx, script, pc, 0);
        if (id == id_prototype(cx))
            return true;
    }

    return false;
}

NewObjectKind
types::UseNewTypeForInitializer(JSScript *script, jsbytecode *pc, JSProtoKey key)
{
    





    if (script->functionNonDelazifying() && !script->treatAsRunOnce())
        return GenericObject;

    if (key != JSProto_Object && !(key >= JSProto_Int8Array && key <= JSProto_Uint8ClampedArray))
        return GenericObject;

    




    if (!script->hasTrynotes())
        return SingletonObject;

    unsigned offset = script->pcToOffset(pc);

    JSTryNote *tn = script->trynotes()->vector;
    JSTryNote *tnlimit = tn + script->trynotes()->length;
    for (; tn < tnlimit; tn++) {
        if (tn->kind != JSTRY_ITER && tn->kind != JSTRY_LOOP)
            continue;

        unsigned startOffset = script->mainOffset() + tn->start;
        unsigned endOffset = startOffset + tn->length;

        if (offset >= startOffset && offset < endOffset)
            return GenericObject;
    }

    return SingletonObject;
}

NewObjectKind
types::UseNewTypeForInitializer(JSScript *script, jsbytecode *pc, const Class *clasp)
{
    return UseNewTypeForInitializer(script, pc, JSCLASS_CACHED_PROTO_KEY(clasp));
}

static inline bool
ClassCanHaveExtraProperties(const Class *clasp)
{
    JS_ASSERT(clasp->resolve);
    return clasp->resolve != JS_ResolveStub
        || clasp->ops.lookupGeneric
        || clasp->ops.getGeneric
        || IsTypedArrayClass(clasp);
}

static inline bool
PrototypeHasIndexedProperty(CompilerConstraintList *constraints, JSObject *obj)
{
    do {
        TypeObjectKey *type = TypeObjectKey::get(obj);
        if (ClassCanHaveExtraProperties(type->clasp()))
            return true;
        if (type->unknownProperties())
            return true;
        HeapTypeSetKey index = type->property(JSID_VOID);
        if (index.nonData(constraints) || index.isOwnProperty(constraints))
            return true;
        if (!obj->hasTenuredProto())
            return true;
        obj = obj->getProto();
    } while (obj);

    return false;
}

bool
types::ArrayPrototypeHasIndexedProperty(CompilerConstraintList *constraints, JSScript *script)
{
    if (JSObject *proto = script->global().maybeGetArrayPrototype())
        return PrototypeHasIndexedProperty(constraints, proto);
    return true;
}

bool
types::TypeCanHaveExtraIndexedProperties(CompilerConstraintList *constraints,
                                         TemporaryTypeSet *types)
{
    const Class *clasp = types->getKnownClass();

    
    
    
    if (!clasp || (ClassCanHaveExtraProperties(clasp) && !IsTypedArrayClass(clasp)))
        return true;

    if (types->hasObjectFlags(constraints, types::OBJECT_FLAG_SPARSE_INDEXES))
        return true;

    JSObject *proto = types->getCommonPrototype();
    if (!proto)
        return true;

    return PrototypeHasIndexedProperty(constraints, proto);
}

void
TypeZone::processPendingRecompiles(FreeOp *fop)
{
    if (!pendingRecompiles)
        return;

    
    Vector<RecompileInfo> *pending = pendingRecompiles;
    pendingRecompiles = nullptr;

    JS_ASSERT(!pending->empty());

#ifdef JS_ION
    jit::Invalidate(*this, fop, *pending);
#endif

    fop->delete_(pending);
}

void
TypeZone::addPendingRecompile(JSContext *cx, const RecompileInfo &info)
{
    CompilerOutput *co = info.compilerOutput(cx);
    if (!co || !co->isValid() || co->pendingInvalidation())
        return;

    InferSpew(ISpewOps, "addPendingRecompile: %p:%s:%d",
              co->script(), co->script()->filename(), co->script()->lineno());

    co->setPendingInvalidation();

    if (!pendingRecompiles) {
        pendingRecompiles = cx->new_< Vector<RecompileInfo> >(cx);
        if (!pendingRecompiles)
            CrashAtUnhandlableOOM("Could not update pendingRecompiles");
    }

    if (!pendingRecompiles->append(info))
        CrashAtUnhandlableOOM("Could not update pendingRecompiles");
}

void
TypeZone::addPendingRecompile(JSContext *cx, JSScript *script)
{
    JS_ASSERT(script);

#ifdef JS_ION
    CancelOffThreadIonCompile(cx->compartment(), script);

    
    if (jit::IsBaselineEnabled(cx))
        script->resetUseCount();

    if (script->hasIonScript())
        addPendingRecompile(cx, script->ionScript()->recompileInfo());

    if (script->hasParallelIonScript())
        addPendingRecompile(cx, script->parallelIonScript()->recompileInfo());
#endif

    
    
    
    if (script->functionNonDelazifying() && !script->functionNonDelazifying()->hasLazyType())
        ObjectStateChange(cx, script->functionNonDelazifying()->type(), false);
}

void
TypeCompartment::markSetsUnknown(JSContext *cx, TypeObject *target)
{
    JS_ASSERT(this == &cx->compartment()->types);
    JS_ASSERT(!(target->flags() & OBJECT_FLAG_SETS_MARKED_UNKNOWN));
    JS_ASSERT(!target->singleton());
    JS_ASSERT(target->unknownProperties());

    AutoEnterAnalysis enter(cx);

    

    for (gc::CellIter i(cx->zone(), gc::FINALIZE_TYPE_OBJECT); !i.done(); i.next()) {
        TypeObject *object = i.get<TypeObject>();
        unsigned count = object->getPropertyCount();
        for (unsigned i = 0; i < count; i++) {
            Property *prop = object->getProperty(i);
            if (prop && prop->types.hasType(Type::ObjectType(target)))
                prop->types.addType(cx, Type::AnyObjectType());
        }
    }

    for (gc::CellIter i(cx->zone(), gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        RootedScript script(cx, i.get<JSScript>());
        if (script->types) {
            unsigned count = TypeScript::NumTypeSets(script);
            StackTypeSet *typeArray = script->types->typeArray();
            for (unsigned i = 0; i < count; i++) {
                if (typeArray[i].hasType(Type::ObjectType(target)))
                    typeArray[i].addType(cx, Type::AnyObjectType());
            }
        }
    }

    target->addFlags(OBJECT_FLAG_SETS_MARKED_UNKNOWN);
}

void
TypeCompartment::print(JSContext *cx, bool force)
{
#ifdef DEBUG
    gc::AutoSuppressGC suppressGC(cx);

    JSCompartment *compartment = this->compartment();
    AutoEnterAnalysis enter(nullptr, compartment);

    if (!force && !InferSpewActive(ISpewResult))
        return;

    for (gc::CellIter i(compartment->zone(), gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        
        
        RootedScript script(cx->runtime(), i.get<JSScript>());
        if (script->types)
            script->types->printTypes(cx, script);
    }

    for (gc::CellIter i(compartment->zone(), gc::FINALIZE_TYPE_OBJECT); !i.done(); i.next()) {
        TypeObject *object = i.get<TypeObject>();
        object->print();
    }
#endif
}

















static inline bool
NumberTypes(Type a, Type b)
{
    return (a.isPrimitive(JSVAL_TYPE_INT32) || a.isPrimitive(JSVAL_TYPE_DOUBLE))
        && (b.isPrimitive(JSVAL_TYPE_INT32) || b.isPrimitive(JSVAL_TYPE_DOUBLE));
}






static inline Type
GetValueTypeForTable(const Value &v)
{
    Type type = GetValueType(v);
    JS_ASSERT(!type.isSingleObject());
    return type;
}

struct types::ArrayTableKey : public DefaultHasher<types::ArrayTableKey>
{
    Type type;
    JSObject *proto;

    ArrayTableKey()
      : type(Type::UndefinedType()), proto(nullptr)
    {}

    ArrayTableKey(Type type, JSObject *proto)
      : type(type), proto(proto)
    {}

    static inline uint32_t hash(const ArrayTableKey &v) {
        return (uint32_t) (v.type.raw() ^ ((uint32_t)(size_t)v.proto >> 2));
    }

    static inline bool match(const ArrayTableKey &v1, const ArrayTableKey &v2) {
        return v1.type == v2.type && v1.proto == v2.proto;
    }
};

void
TypeCompartment::setTypeToHomogenousArray(ExclusiveContext *cx,
                                          JSObject *obj, Type elementType)
{
    JS_ASSERT(cx->compartment()->activeAnalysis);

    if (!arrayTypeTable) {
        arrayTypeTable = cx->new_<ArrayTypeTable>();
        if (!arrayTypeTable || !arrayTypeTable->init()) {
            arrayTypeTable = nullptr;
            return;
        }
    }

    ArrayTableKey key(elementType, obj->getProto());
    DependentAddPtr<ArrayTypeTable> p(cx, *arrayTypeTable, key);
    if (p) {
        obj->setType(p->value());
    } else {
        
        RootedObject objProto(cx, obj->getProto());
        TypeObject *objType = newTypeObject(cx, &ArrayObject::class_, objProto);
        if (!objType)
            return;
        obj->setType(objType);

        if (!objType->unknownProperties())
            objType->addPropertyType(cx, JSID_VOID, elementType);

        key.proto = objProto;
        (void) p.add(cx, *arrayTypeTable, key, objType);
    }
}

void
TypeCompartment::fixArrayType(ExclusiveContext *cx, JSObject *obj)
{
    AutoEnterAnalysis enter(cx);

    





    JS_ASSERT(obj->is<ArrayObject>());

    unsigned len = obj->getDenseInitializedLength();
    if (len == 0)
        return;

    Type type = GetValueTypeForTable(obj->getDenseElement(0));

    for (unsigned i = 1; i < len; i++) {
        Type ntype = GetValueTypeForTable(obj->getDenseElement(i));
        if (ntype != type) {
            if (NumberTypes(type, ntype))
                type = Type::DoubleType();
            else
                return;
        }
    }

    setTypeToHomogenousArray(cx, obj, type);
}

void
types::FixRestArgumentsType(ExclusiveContext *cx, JSObject *obj)
{
    cx->compartment()->types.fixRestArgumentsType(cx, obj);
}

void
TypeCompartment::fixRestArgumentsType(ExclusiveContext *cx, JSObject *obj)
{
    AutoEnterAnalysis enter(cx);

    



    JS_ASSERT(obj->is<ArrayObject>());

    setTypeToHomogenousArray(cx, obj, Type::UnknownType());
}







struct types::ObjectTableKey
{
    jsid *properties;
    uint32_t nproperties;
    uint32_t nfixed;

    struct Lookup {
        IdValuePair *properties;
        uint32_t nproperties;
        uint32_t nfixed;

        Lookup(IdValuePair *properties, uint32_t nproperties, uint32_t nfixed)
          : properties(properties), nproperties(nproperties), nfixed(nfixed)
        {}
    };

    static inline HashNumber hash(const Lookup &lookup) {
        return (HashNumber) (JSID_BITS(lookup.properties[lookup.nproperties - 1].id) ^
                             lookup.nproperties ^
                             lookup.nfixed);
    }

    static inline bool match(const ObjectTableKey &v, const Lookup &lookup) {
        if (lookup.nproperties != v.nproperties || lookup.nfixed != v.nfixed)
            return false;
        for (size_t i = 0; i < lookup.nproperties; i++) {
            if (lookup.properties[i].id != v.properties[i])
                return false;
        }
        return true;
    }
};

struct types::ObjectTableEntry
{
    ReadBarriered<TypeObject> object;
    ReadBarriered<Shape> shape;
    Type *types;
};

static inline void
UpdateObjectTableEntryTypes(ExclusiveContext *cx, ObjectTableEntry &entry,
                            IdValuePair *properties, size_t nproperties)
{
    if (entry.object->unknownProperties())
        return;
    for (size_t i = 0; i < nproperties; i++) {
        Type type = entry.types[i];
        Type ntype = GetValueTypeForTable(properties[i].value);
        if (ntype == type)
            continue;
        if (ntype.isPrimitive(JSVAL_TYPE_INT32) &&
            type.isPrimitive(JSVAL_TYPE_DOUBLE))
        {
            
        } else {
            if (ntype.isPrimitive(JSVAL_TYPE_DOUBLE) &&
                type.isPrimitive(JSVAL_TYPE_INT32))
            {
                
                entry.types[i] = Type::DoubleType();
            }
            entry.object->addPropertyType(cx, IdToTypeId(properties[i].id), ntype);
        }
    }
}

void
TypeCompartment::fixObjectType(ExclusiveContext *cx, JSObject *obj)
{
    AutoEnterAnalysis enter(cx);

    if (!objectTypeTable) {
        objectTypeTable = cx->new_<ObjectTypeTable>();
        if (!objectTypeTable || !objectTypeTable->init()) {
            js_delete(objectTypeTable);
            objectTypeTable = nullptr;
            return;
        }
    }

    



    JS_ASSERT(obj->is<JSObject>());

    




    if (obj->slotSpan() == 0 || obj->inDictionaryMode() || !obj->hasEmptyElements() || obj->getMetadata())
        return;

    Vector<IdValuePair> properties(cx);
    if (!properties.resize(obj->slotSpan()))
        return;

    Shape *shape = obj->lastProperty();
    while (!shape->isEmptyShape()) {
        IdValuePair &entry = properties[shape->slot()];
        entry.id = shape->propid();
        entry.value = obj->getSlot(shape->slot());
        shape = shape->previous();
    }

    ObjectTableKey::Lookup lookup(properties.begin(), properties.length(), obj->numFixedSlots());
    ObjectTypeTable::AddPtr p = objectTypeTable->lookupForAdd(lookup);

    if (p) {
        JS_ASSERT(obj->getProto() == p->value().object->proto().toObject());
        JS_ASSERT(obj->lastProperty() == p->value().shape);

        UpdateObjectTableEntryTypes(cx, p->value(), properties.begin(), properties.length());
        obj->setType(p->value().object);
        return;
    }

    
    Rooted<TaggedProto> objProto(cx, obj->getTaggedProto());
    TypeObject *objType = newTypeObject(cx, &JSObject::class_, objProto);
    if (!objType || !objType->addDefiniteProperties(cx, obj))
        return;

    if (obj->isIndexed())
        objType->setFlags(cx, OBJECT_FLAG_SPARSE_INDEXES);

    ScopedJSFreePtr<jsid> ids(cx->pod_calloc<jsid>(properties.length()));
    if (!ids)
        return;

    ScopedJSFreePtr<Type> types(cx->pod_calloc<Type>(properties.length()));
    if (!types)
        return;

    for (size_t i = 0; i < properties.length(); i++) {
        ids[i] = properties[i].id;
        types[i] = GetValueTypeForTable(obj->getSlot(i));
        if (!objType->unknownProperties())
            objType->addPropertyType(cx, IdToTypeId(ids[i]), types[i]);
    }

    ObjectTableKey key;
    key.properties = ids;
    key.nproperties = properties.length();
    key.nfixed = obj->numFixedSlots();
    JS_ASSERT(ObjectTableKey::match(key, lookup));

    ObjectTableEntry entry;
    entry.object = objType;
    entry.shape = obj->lastProperty();
    entry.types = types;

    obj->setType(objType);

    p = objectTypeTable->lookupForAdd(lookup);
    if (objectTypeTable->add(p, key, entry)) {
        ids.forget();
        types.forget();
    }
}

JSObject *
TypeCompartment::newTypedObject(JSContext *cx, IdValuePair *properties, size_t nproperties)
{
    AutoEnterAnalysis enter(cx);

    if (!objectTypeTable) {
        objectTypeTable = cx->new_<ObjectTypeTable>();
        if (!objectTypeTable || !objectTypeTable->init()) {
            js_delete(objectTypeTable);
            objectTypeTable = nullptr;
            return nullptr;
        }
    }

    





    







    if (!nproperties || nproperties >= PropertyTree::MAX_HEIGHT)
        return nullptr;

    gc::AllocKind allocKind = gc::GetGCObjectKind(nproperties);
    size_t nfixed = gc::GetGCKindSlots(allocKind, &JSObject::class_);

    ObjectTableKey::Lookup lookup(properties, nproperties, nfixed);
    ObjectTypeTable::AddPtr p = objectTypeTable->lookupForAdd(lookup);

    if (!p)
        return nullptr;

    RootedObject obj(cx, NewBuiltinClassInstance(cx, &JSObject::class_, allocKind));
    if (!obj) {
        cx->clearPendingException();
        return nullptr;
    }
    JS_ASSERT(obj->getProto() == p->value().object->proto().toObject());

    RootedShape shape(cx, p->value().shape);
    if (!JSObject::setLastProperty(cx, obj, shape)) {
        cx->clearPendingException();
        return nullptr;
    }

    UpdateObjectTableEntryTypes(cx, p->value(), properties, nproperties);

    for (size_t i = 0; i < nproperties; i++)
        obj->setSlot(i, properties[i].value);

    obj->setType(p->value().object);
    return obj;
}





void
TypeObject::setProto(JSContext *cx, TaggedProto proto)
{
    JS_ASSERT(singleton());

    if (proto.isObject() && IsInsideNursery(cx->runtime(), proto.toObject()))
        addFlags(OBJECT_FLAG_NURSERY_PROTO);

    setProtoUnchecked(proto);
}

static inline void
UpdatePropertyType(ExclusiveContext *cx, HeapTypeSet *types, JSObject *obj, Shape *shape,
                   bool indexed)
{
    if (!shape->writable())
        types->setNonWritableProperty(cx);

    if (shape->hasGetterValue() || shape->hasSetterValue()) {
        types->setNonDataProperty(cx);
        types->TypeSet::addType(Type::UnknownType(), &cx->typeLifoAlloc());
    } else if (shape->hasDefaultGetter() && shape->hasSlot()) {
        if (!indexed && types->canSetDefinite(shape->slot()))
            types->setDefinite(shape->slot());

        const Value &value = obj->nativeGetSlot(shape->slot());

        




        if (indexed || !value.isUndefined() || !CanHaveEmptyPropertyTypesForOwnProperty(obj)) {
            Type type = GetValueType(value);
            types->TypeSet::addType(type, &cx->typeLifoAlloc());
        }
    }
}

void
TypeObject::updateNewPropertyTypes(ExclusiveContext *cx, jsid id, HeapTypeSet *types)
{
    InferSpew(ISpewOps, "typeSet: %sT%p%s property %s %s",
              InferSpewColor(types), types, InferSpewColorReset(),
              TypeObjectString(this), TypeIdString(id));

    if (!singleton() || !singleton()->isNative())
        return;

    






    if (JSID_IS_VOID(id)) {
        
        RootedShape shape(cx, singleton()->lastProperty());
        while (!shape->isEmptyShape()) {
            if (JSID_IS_VOID(IdToTypeId(shape->propid())))
                UpdatePropertyType(cx, types, singleton(), shape, true);
            shape = shape->previous();
        }

        
        for (size_t i = 0; i < singleton()->getDenseInitializedLength(); i++) {
            const Value &value = singleton()->getDenseElement(i);
            if (!value.isMagic(JS_ELEMENTS_HOLE)) {
                Type type = GetValueType(value);
                types->TypeSet::addType(type, &cx->typeLifoAlloc());
            }
        }
    } else if (!JSID_IS_EMPTY(id)) {
        RootedId rootedId(cx, id);
        Shape *shape = singleton()->nativeLookup(cx, rootedId);
        if (shape)
            UpdatePropertyType(cx, types, singleton(), shape, false);
    }

    if (singleton()->watched()) {
        



        types->setNonDataProperty(cx);
    }
}

bool
TypeObject::addDefiniteProperties(ExclusiveContext *cx, JSObject *obj)
{
    if (unknownProperties())
        return true;

    
    AutoEnterAnalysis enter(cx);

    RootedShape shape(cx, obj->lastProperty());
    while (!shape->isEmptyShape()) {
        jsid id = IdToTypeId(shape->propid());
        if (!JSID_IS_VOID(id) && obj->isFixedSlot(shape->slot())) {
            TypeSet *types = getProperty(cx, id);
            if (!types)
                return false;
            types->setDefinite(shape->slot());
        }
        shape = shape->previous();
    }

    return true;
}

bool
TypeObject::matchDefiniteProperties(HandleObject obj)
{
    unsigned count = getPropertyCount();
    for (unsigned i = 0; i < count; i++) {
        Property *prop = getProperty(i);
        if (!prop)
            continue;
        if (prop->types.definiteProperty()) {
            unsigned slot = prop->types.definiteSlot();

            bool found = false;
            Shape *shape = obj->lastProperty();
            while (!shape->isEmptyShape()) {
                if (shape->slot() == slot && shape->propid() == prop->id) {
                    found = true;
                    break;
                }
                shape = shape->previous();
            }
            if (!found)
                return false;
        }
    }

    return true;
}

static inline void
InlineAddTypeProperty(ExclusiveContext *cx, TypeObject *obj, jsid id, Type type)
{
    JS_ASSERT(id == IdToTypeId(id));

    AutoEnterAnalysis enter(cx);

    HeapTypeSet *types = obj->getProperty(cx, id);
    if (!types || types->hasType(type))
        return;

    InferSpew(ISpewOps, "externalType: property %s %s: %s",
              TypeObjectString(obj), TypeIdString(id), TypeString(type));
    types->addType(cx, type);
}

void
TypeObject::addPropertyType(ExclusiveContext *cx, jsid id, Type type)
{
    InlineAddTypeProperty(cx, this, id, type);
}

void
TypeObject::addPropertyType(ExclusiveContext *cx, jsid id, const Value &value)
{
    InlineAddTypeProperty(cx, this, id, GetValueType(value));
}

void
TypeObject::markPropertyNonData(ExclusiveContext *cx, jsid id)
{
    AutoEnterAnalysis enter(cx);

    id = IdToTypeId(id);

    HeapTypeSet *types = getProperty(cx, id);
    if (types)
        types->setNonDataProperty(cx);
}

void
TypeObject::markPropertyNonWritable(ExclusiveContext *cx, jsid id)
{
    AutoEnterAnalysis enter(cx);

    id = IdToTypeId(id);

    HeapTypeSet *types = getProperty(cx, id);
    if (types)
        types->setNonWritableProperty(cx);
}

bool
TypeObject::isPropertyNonData(jsid id)
{
    TypeSet *types = maybeGetProperty(id);
    if (types)
        return types->nonDataProperty();
    return false;
}

bool
TypeObject::isPropertyNonWritable(jsid id)
{
    TypeSet *types = maybeGetProperty(id);
    if (types)
        return types->nonWritableProperty();
    return false;
}

void
TypeObject::markStateChange(ExclusiveContext *cxArg)
{
    if (unknownProperties())
        return;

    AutoEnterAnalysis enter(cxArg);
    HeapTypeSet *types = maybeGetProperty(JSID_EMPTY);
    if (types) {
        if (JSContext *cx = cxArg->maybeJSContext()) {
            TypeConstraint *constraint = types->constraintList;
            while (constraint) {
                constraint->newObjectState(cx, this);
                constraint = constraint->next;
            }
        } else {
            JS_ASSERT(!types->constraintList);
        }
    }
}

void
TypeObject::setFlags(ExclusiveContext *cx, TypeObjectFlags flags)
{
    if (hasAllFlags(flags))
        return;

    AutoEnterAnalysis enter(cx);

    if (singleton()) {
        
        JS_ASSERT_IF(flags & OBJECT_FLAG_ITERATED,
                     singleton()->lastProperty()->hasObjectFlag(BaseShape::ITERATED_SINGLETON));
    }

    addFlags(flags);

    InferSpew(ISpewOps, "%s: setFlags 0x%x", TypeObjectString(this), flags);

    ObjectStateChange(cx, this, false);
}

void
TypeObject::markUnknown(ExclusiveContext *cx)
{
    AutoEnterAnalysis enter(cx);

    JS_ASSERT(cx->compartment()->activeAnalysis);
    JS_ASSERT(!unknownProperties());

    if (!(flags() & OBJECT_FLAG_ADDENDUM_CLEARED))
        clearAddendum(cx);

    InferSpew(ISpewOps, "UnknownProperties: %s", TypeObjectString(this));

    ObjectStateChange(cx, this, true);

    








    unsigned count = getPropertyCount();
    for (unsigned i = 0; i < count; i++) {
        Property *prop = getProperty(i);
        if (prop) {
            prop->types.addType(cx, Type::UnknownType());
            prop->types.setNonDataProperty(cx);
        }
    }
}

void
TypeObject::clearAddendum(ExclusiveContext *cx)
{
    JS_ASSERT(!(flags() & OBJECT_FLAG_ADDENDUM_CLEARED));

    addFlags(OBJECT_FLAG_ADDENDUM_CLEARED);

    









    if (!addendum)
        return;

    switch (addendum->kind) {
      case TypeObjectAddendum::NewScript:
        clearNewScriptAddendum(cx);
        break;

      case TypeObjectAddendum::TypedObject:
        clearTypedObjectAddendum(cx);
        break;
    }

    
    TypeObjectAddendum *savedAddendum = addendum;
    addendum = nullptr;
    js_free(savedAddendum);

    markStateChange(cx);
}

void
TypeObject::clearNewScriptAddendum(ExclusiveContext *cx)
{
    AutoEnterAnalysis enter(cx);

    








    for (unsigned i = 0; i < getPropertyCount(); i++) {
        Property *prop = getProperty(i);
        if (!prop)
            continue;
        if (prop->types.definiteProperty())
            prop->types.setNonDataProperty(cx);
    }

    







    if (cx->isJSContext()) {
        Vector<uint32_t, 32> pcOffsets(cx);
        for (ScriptFrameIter iter(cx->asJSContext()); !iter.done(); ++iter) {
            pcOffsets.append(iter.script()->pcToOffset(iter.pc()));
            if (!iter.isConstructing() ||
                iter.callee() != newScript()->fun ||
                !iter.thisv().isObject() ||
                iter.thisv().toObject().hasLazyType() ||
                iter.thisv().toObject().type() != this)
            {
                continue;
            }

            
            RootedObject obj(cx, &iter.thisv().toObject());

            
            bool finished = false;

            
            uint32_t numProperties = 0;

            
            
            bool pastProperty = false;

            
            int callDepth = pcOffsets.length() - 1;

            
            int setpropDepth = callDepth;

            for (TypeNewScript::Initializer *init = newScript()->initializerList;; init++) {
                if (init->kind == TypeNewScript::Initializer::SETPROP) {
                    if (!pastProperty && pcOffsets[setpropDepth] < init->offset) {
                        
                        break;
                    }
                    
                    numProperties++;
                    pastProperty = false;
                    setpropDepth = callDepth;
                } else if (init->kind == TypeNewScript::Initializer::SETPROP_FRAME) {
                    if (!pastProperty) {
                        if (pcOffsets[setpropDepth] < init->offset) {
                            
                            break;
                        } else if (pcOffsets[setpropDepth] > init->offset) {
                            
                            pastProperty = true;
                        } else if (setpropDepth == 0) {
                            
                            break;
                        } else {
                            
                            setpropDepth--;
                        }
                    }
                } else {
                    JS_ASSERT(init->kind == TypeNewScript::Initializer::DONE);
                    finished = true;
                    break;
                }
            }

            if (!finished)
                (void) JSObject::rollbackProperties(cx, obj, numProperties);
        }
    } else {
        
        JS_ASSERT(!cx->perThreadData->activation());
    }
}

void
TypeObject::maybeClearNewScriptAddendumOnOOM()
{
    if (!isMarked())
        return;

    if (!addendum || addendum->kind != TypeObjectAddendum::NewScript)
        return;

    for (unsigned i = 0; i < getPropertyCount(); i++) {
        Property *prop = getProperty(i);
        if (!prop)
            continue;
        if (prop->types.definiteProperty())
            prop->types.setNonDataPropertyIgnoringConstraints();
    }

    
    
    js_free(addendum);
    addendum.unsafeSet(nullptr);
}

void
TypeObject::clearTypedObjectAddendum(ExclusiveContext *cx)
{
}

void
TypeObject::print()
{
    TaggedProto tagged(proto());
    fprintf(stderr, "%s : %s",
            TypeObjectString(this),
            tagged.isObject() ? TypeString(Type::ObjectType(tagged.toObject()))
                              : (tagged.isLazy() ? "(lazy)" : "(null)"));

    if (unknownProperties()) {
        fprintf(stderr, " unknown");
    } else {
        if (!hasAnyFlags(OBJECT_FLAG_SPARSE_INDEXES))
            fprintf(stderr, " dense");
        if (!hasAnyFlags(OBJECT_FLAG_NON_PACKED))
            fprintf(stderr, " packed");
        if (!hasAnyFlags(OBJECT_FLAG_LENGTH_OVERFLOW))
            fprintf(stderr, " noLengthOverflow");
        if (hasAnyFlags(OBJECT_FLAG_ITERATED))
            fprintf(stderr, " iterated");
        if (interpretedFunction)
            fprintf(stderr, " ifun");
    }

    unsigned count = getPropertyCount();

    if (count == 0) {
        fprintf(stderr, " {}\n");
        return;
    }

    fprintf(stderr, " {");

    for (unsigned i = 0; i < count; i++) {
        Property *prop = getProperty(i);
        if (prop) {
            fprintf(stderr, "\n    %s:", TypeIdString(prop->id));
            prop->types.print();
        }
    }

    fprintf(stderr, "\n}\n");
}









class TypeConstraintClearDefiniteGetterSetter : public TypeConstraint
{
  public:
    TypeObject *object;

    TypeConstraintClearDefiniteGetterSetter(TypeObject *object)
        : object(object)
    {}

    const char *kind() { return "clearDefiniteGetterSetter"; }

    void newPropertyState(JSContext *cx, TypeSet *source)
    {
        if (!object->hasNewScript())
            return;
        




        if (!(object->flags() & OBJECT_FLAG_ADDENDUM_CLEARED) &&
            (source->nonDataProperty() || source->nonWritableProperty()))
        {
            object->clearAddendum(cx);
        }
    }

    void newType(JSContext *cx, TypeSet *source, Type type) {}

    bool sweep(TypeZone &zone, TypeConstraint **res) {
        if (IsTypeObjectAboutToBeFinalized(&object))
            return false;
        *res = zone.typeLifoAlloc.new_<TypeConstraintClearDefiniteGetterSetter>(object);
        return true;
    }
};

bool
types::AddClearDefiniteGetterSetterForPrototypeChain(JSContext *cx, TypeObject *type, HandleId id)
{
    




    RootedObject parent(cx, type->proto().toObjectOrNull());
    while (parent) {
        TypeObject *parentObject = parent->getType(cx);
        if (!parentObject || parentObject->unknownProperties())
            return false;
        HeapTypeSet *parentTypes = parentObject->getProperty(cx, id);
        if (!parentTypes || parentTypes->nonDataProperty() || parentTypes->nonWritableProperty())
            return false;
        if (!parentTypes->addConstraint(cx, cx->typeLifoAlloc().new_<TypeConstraintClearDefiniteGetterSetter>(type)))
            return false;
        parent = parent->getProto();
    }
    return true;
}





class TypeConstraintClearDefiniteSingle : public TypeConstraint
{
  public:
    TypeObject *object;

    TypeConstraintClearDefiniteSingle(TypeObject *object)
        : object(object)
    {}

    const char *kind() { return "clearDefiniteSingle"; }

    void newType(JSContext *cx, TypeSet *source, Type type) {
        if (object->flags() & OBJECT_FLAG_ADDENDUM_CLEARED)
            return;

        if (source->baseFlags() || source->getObjectCount() > 1)
            object->clearAddendum(cx);
    }

    bool sweep(TypeZone &zone, TypeConstraint **res) {
        if (IsTypeObjectAboutToBeFinalized(&object))
            return false;
        *res = zone.typeLifoAlloc.new_<TypeConstraintClearDefiniteSingle>(object);
        return true;
    }
};

bool
types::AddClearDefiniteFunctionUsesInScript(JSContext *cx, TypeObject *type,
                                            JSScript *script, JSScript *calleeScript)
{
    
    
    
    
    

    TypeObjectKey *calleeKey = Type::ObjectType(calleeScript->functionNonDelazifying()).objectKey();

    unsigned count = TypeScript::NumTypeSets(script);
    StackTypeSet *typeArray = script->types->typeArray();

    for (unsigned i = 0; i < count; i++) {
        StackTypeSet *types = &typeArray[i];
        if (!types->unknownObject() && types->getObjectCount() == 1) {
            if (calleeKey != types->getObject(0)) {
                
                
                
                JSObject *singleton = types->getSingleObject(0);
                if (!singleton || !singleton->is<JSFunction>())
                    continue;
                JSFunction *fun = &singleton->as<JSFunction>();
                if (!fun->isNative())
                    continue;
                if (fun->native() != js_fun_call && fun->native() != js_fun_apply)
                    continue;
            }
            
            
            if (!types->addConstraint(cx, cx->typeLifoAlloc().new_<TypeConstraintClearDefiniteSingle>(type)))
                return false;
        }
    }

    return true;
}






static void
CheckNewScriptProperties(JSContext *cx, TypeObject *type, JSFunction *fun)
{
    JS_ASSERT(cx->compartment()->activeAnalysis);

#ifdef JS_ION
    if (type->unknownProperties())
        return;

    
    RootedObject baseobj(cx, NewBuiltinClassInstance(cx, &JSObject::class_, gc::FINALIZE_OBJECT16));
    if (!baseobj)
        return;

    Vector<TypeNewScript::Initializer> initializerList(cx);

    if (!jit::AnalyzeNewScriptProperties(cx, fun, type, baseobj, &initializerList) ||
        baseobj->slotSpan() == 0 ||
        !!(type->flags() & OBJECT_FLAG_ADDENDUM_CLEARED))
    {
        if (type->hasNewScript())
            type->clearAddendum(cx);
        return;
    }

    




    if (type->hasNewScript()) {
        if (!type->matchDefiniteProperties(baseobj))
            type->clearAddendum(cx);
        return;
    }
    JS_ASSERT(!type->hasNewScript());
    JS_ASSERT(!(type->flags() & OBJECT_FLAG_ADDENDUM_CLEARED));

    gc::AllocKind kind = gc::GetGCObjectKind(baseobj->slotSpan());

    
    JS_ASSERT(gc::GetGCKindSlots(kind) >= baseobj->slotSpan());

    TypeNewScript::Initializer done(TypeNewScript::Initializer::DONE, 0);

    




    Rooted<TypeObject *> rootedType(cx, type);
    RootedShape shape(cx, baseobj->lastProperty());
    baseobj = NewReshapedObject(cx, rootedType, baseobj->getParent(), kind, shape, MaybeSingletonObject);
    if (!baseobj ||
        !type->addDefiniteProperties(cx, baseobj) ||
        !initializerList.append(done))
    {
        return;
    }

    size_t numBytes = sizeof(TypeNewScript)
                    + (initializerList.length() * sizeof(TypeNewScript::Initializer));
    TypeNewScript *newScript = (TypeNewScript *) cx->calloc_(numBytes);
    if (!newScript)
        return;

    new (newScript) TypeNewScript();

    type->setAddendum(newScript);

    newScript->fun = fun;
    newScript->templateObject = baseobj;

    newScript->initializerList = (TypeNewScript::Initializer *)
        ((char *) newScript + sizeof(TypeNewScript));
    PodCopy(newScript->initializerList,
            initializerList.begin(),
            initializerList.length());
#endif 
}





void
types::TypeMonitorCallSlow(JSContext *cx, JSObject *callee, const CallArgs &args,
                           bool constructing)
{
    unsigned nargs = callee->as<JSFunction>().nargs();
    JSScript *script = callee->as<JSFunction>().nonLazyScript();

    if (!constructing)
        TypeScript::SetThis(cx, script, args.thisv());

    




    unsigned arg = 0;
    for (; arg < args.length() && arg < nargs; arg++)
        TypeScript::SetArgument(cx, script, arg, args[arg]);

    
    for (; arg < nargs; arg++)
        TypeScript::SetArgument(cx, script, arg, UndefinedValue());
}

static inline bool
IsAboutToBeFinalized(TypeObjectKey *key)
{
    
    gc::Cell *tmp = reinterpret_cast<gc::Cell *>(uintptr_t(key) & ~1);
    bool isAboutToBeFinalized = IsCellAboutToBeFinalized(&tmp);
    JS_ASSERT(tmp == reinterpret_cast<gc::Cell *>(uintptr_t(key) & ~1));
    return isAboutToBeFinalized;
}

void
types::TypeMonitorResult(JSContext *cx, JSScript *script, jsbytecode *pc, const js::Value &rval)
{
    
    if (!(js_CodeSpec[*pc].format & JOF_TYPESET))
        return;

    if (!script->hasBaselineScript())
        return;

    AutoEnterAnalysis enter(cx);

    Type type = GetValueType(rval);
    StackTypeSet *types = TypeScript::BytecodeTypes(script, pc);
    if (types->hasType(type))
        return;

    InferSpew(ISpewOps, "bytecodeType: #%u:%05u: %s",
              script->id(), script->pcToOffset(pc), TypeString(type));
    types->addType(cx, type);
}

bool
types::UseNewTypeForClone(JSFunction *fun)
{
    if (!fun->isInterpreted())
        return false;

    if (fun->hasScript() && fun->nonLazyScript()->shouldCloneAtCallsite())
        return true;

    if (fun->isArrow())
        return false;

    if (fun->hasSingletonType())
        return false;

    























    uint32_t begin, end;
    if (fun->hasScript()) {
        if (!fun->nonLazyScript()->usesArgumentsAndApply())
            return false;
        begin = fun->nonLazyScript()->sourceStart();
        end = fun->nonLazyScript()->sourceEnd();
    } else {
        if (!fun->lazyScript()->usesArgumentsAndApply())
            return false;
        begin = fun->lazyScript()->begin();
        end = fun->lazyScript()->end();
    }

    return end - begin <= 100;
}




bool
JSScript::makeTypes(JSContext *cx)
{
    JS_ASSERT(!types);

    AutoEnterAnalysis enter(cx);

    unsigned count = TypeScript::NumTypeSets(this);

    TypeScript *typeScript = (TypeScript *) cx->calloc_(sizeof(TypeScript) + (sizeof(StackTypeSet) * count));
    if (!typeScript)
        return false;

    new(typeScript) TypeScript();

    TypeSet *typeArray = typeScript->typeArray();

    for (unsigned i = 0; i < count; i++)
        new (&typeArray[i]) StackTypeSet();

    types = typeScript;

#ifdef DEBUG
    for (unsigned i = 0; i < nTypeSets(); i++) {
        InferSpew(ISpewOps, "typeSet: %sT%p%s bytecode%u #%u",
                  InferSpewColor(&typeArray[i]), &typeArray[i], InferSpewColorReset(),
                  i, id());
    }
    TypeSet *thisTypes = TypeScript::ThisTypes(this);
    InferSpew(ISpewOps, "typeSet: %sT%p%s this #%u",
              InferSpewColor(thisTypes), thisTypes, InferSpewColorReset(),
              id());
    unsigned nargs = functionNonDelazifying() ? functionNonDelazifying()->nargs() : 0;
    for (unsigned i = 0; i < nargs; i++) {
        TypeSet *types = TypeScript::ArgTypes(this, i);
        InferSpew(ISpewOps, "typeSet: %sT%p%s arg%u #%u",
                  InferSpewColor(types), types, InferSpewColorReset(),
                  i, id());
    }
#endif

    return analyzedArgsUsage() || ensureRanAnalysis(cx);
}

bool
JSScript::makeAnalysis(JSContext *cx)
{
    JS_ASSERT(types && !types->analysis);

    AutoEnterAnalysis enter(cx);

    types->analysis = cx->typeLifoAlloc().new_<ScriptAnalysis>(this);

    if (!types->analysis)
        return false;

    RootedScript self(cx, this);

    if (!self->types->analysis->analyzeBytecode(cx)) {
        self->types->analysis = nullptr;
        return false;
    }

    return true;
}

 bool
JSFunction::setTypeForScriptedFunction(ExclusiveContext *cx, HandleFunction fun,
                                       bool singleton )
{
    if (singleton) {
        if (!setSingletonType(cx, fun))
            return false;
    } else {
        RootedObject funProto(cx, fun->getProto());
        TypeObject *type =
            cx->compartment()->types.newTypeObject(cx, &JSFunction::class_, funProto);
        if (!type)
            return false;

        fun->setType(type);
        type->interpretedFunction = fun;
    }

    return true;
}





bool
JSObject::shouldSplicePrototype(JSContext *cx)
{
    





    if (getProto() != nullptr)
        return false;
    return hasSingletonType();
}

bool
JSObject::splicePrototype(JSContext *cx, const Class *clasp, Handle<TaggedProto> proto)
{
    JS_ASSERT(cx->compartment() == compartment());

    RootedObject self(cx, this);

    




    JS_ASSERT(self->hasSingletonType());

    
    JS_ASSERT_IF(proto.isObject(), !proto.toObject()->getClass()->ext.outerObject);

    



    Rooted<TypeObject*> type(cx, self->getType(cx));
    if (!type)
        return false;
    Rooted<TypeObject*> protoType(cx, nullptr);
    if (proto.isObject()) {
        protoType = proto.toObject()->getType(cx);
        if (!protoType)
            return false;
    }

    type->setClasp(clasp);
    type->setProto(cx, proto);
    return true;
}

 TypeObject *
JSObject::makeLazyType(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->hasLazyType());
    JS_ASSERT(cx->compartment() == obj->compartment());

    
    if (obj->is<JSFunction>() && obj->as<JSFunction>().isInterpretedLazy()) {
        RootedFunction fun(cx, &obj->as<JSFunction>());
        if (!fun->getOrCreateScript(cx))
            return nullptr;
    }

    
    
    TypeObjectFlags initialFlags = OBJECT_FLAG_NON_PACKED;

    if (obj->lastProperty()->hasObjectFlag(BaseShape::ITERATED_SINGLETON))
        initialFlags |= OBJECT_FLAG_ITERATED;

    if (obj->isIndexed())
        initialFlags |= OBJECT_FLAG_SPARSE_INDEXES;

    if (obj->is<ArrayObject>() && obj->as<ArrayObject>().length() > INT32_MAX)
        initialFlags |= OBJECT_FLAG_LENGTH_OVERFLOW;

    Rooted<TaggedProto> proto(cx, obj->getTaggedProto());
    TypeObject *type = cx->compartment()->types.newTypeObject(cx, obj->getClass(), proto, initialFlags);
    if (!type)
        return nullptr;

    AutoEnterAnalysis enter(cx);

    

    type->initSingleton(obj);

    if (obj->is<JSFunction>() && obj->as<JSFunction>().isInterpreted())
        type->interpretedFunction = &obj->as<JSFunction>();

    obj->type_ = type;

    return type;
}

 inline HashNumber
TypeObjectWithNewScriptEntry::hash(const Lookup &lookup)
{
    return PointerHasher<JSObject *, 3>::hash(lookup.hashProto.raw()) ^
           PointerHasher<const Class *, 3>::hash(lookup.clasp) ^
           PointerHasher<JSFunction *, 3>::hash(lookup.newFunction);
}

 inline bool
TypeObjectWithNewScriptEntry::match(const TypeObjectWithNewScriptEntry &key, const Lookup &lookup)
{
    return key.object->proto() == lookup.matchProto &&
           key.object->clasp() == lookup.clasp &&
           key.newFunction == lookup.newFunction;
}

#ifdef DEBUG
bool
JSObject::hasNewType(const Class *clasp, TypeObject *type)
{
    TypeObjectWithNewScriptSet &table = compartment()->newTypeObjects;

    if (!table.initialized())
        return false;

    TypeObjectWithNewScriptSet::Ptr p = table.lookup(TypeObjectWithNewScriptSet::Lookup(clasp, this, nullptr));
    return p && p->object == type;
}
#endif 

 bool
JSObject::setNewTypeUnknown(JSContext *cx, const Class *clasp, HandleObject obj)
{
    if (!obj->setFlag(cx, js::BaseShape::NEW_TYPE_UNKNOWN))
        return false;

    




    TypeObjectWithNewScriptSet &table = cx->compartment()->newTypeObjects;
    if (table.initialized()) {
        if (TypeObjectWithNewScriptSet::Ptr p = table.lookup(TypeObjectWithNewScriptSet::Lookup(clasp, obj.get(), nullptr)))
            MarkTypeObjectUnknownProperties(cx, p->object);
    }

    return true;
}

#ifdef JSGC_GENERATIONAL





class NewTypeObjectsSetRef : public BufferableRef
{
    TypeObjectWithNewScriptSet *set;
    const Class *clasp;
    JSObject *proto;
    JSFunction *newFunction;

  public:
    NewTypeObjectsSetRef(TypeObjectWithNewScriptSet *s, const Class *clasp, JSObject *proto, JSFunction *newFunction)
        : set(s), clasp(clasp), proto(proto), newFunction(newFunction)
    {}

    void mark(JSTracer *trc) {
        JSObject *prior = proto;
        trc->setTracingLocation(&*prior);
        Mark(trc, &proto, "newTypeObjects set prototype");
        if (prior == proto)
            return;

        TypeObjectWithNewScriptSet::Ptr p = set->lookup(TypeObjectWithNewScriptSet::Lookup(clasp, prior, proto, newFunction));
        JS_ASSERT(p);  

        set->rekeyAs(TypeObjectWithNewScriptSet::Lookup(clasp, prior, proto, newFunction),
                     TypeObjectWithNewScriptSet::Lookup(clasp, proto, newFunction), *p);
    }
};
#endif

TypeObject *
ExclusiveContext::getNewType(const Class *clasp, TaggedProto proto, JSFunction *fun)
{
    JS_ASSERT_IF(fun, proto.isObject());
    JS_ASSERT_IF(proto.isObject(), isInsideCurrentCompartment(proto.toObject()));

    TypeObjectWithNewScriptSet &newTypeObjects = compartment()->newTypeObjects;

    if (!newTypeObjects.initialized() && !newTypeObjects.init())
        return nullptr;

    
    if (fun) {
        if (fun->hasScript())
            fun = fun->nonLazyScript()->functionNonDelazifying();
        else if (fun->isInterpretedLazy() && !fun->isSelfHostedBuiltin())
            fun = fun->lazyScript()->functionNonDelazifying();
        else
            fun = nullptr;
    }

    TypeObjectWithNewScriptSet::AddPtr p =
        newTypeObjects.lookupForAdd(TypeObjectWithNewScriptSet::Lookup(clasp, proto, fun));
    if (p) {
        TypeObject *type = p->object;
        JS_ASSERT(type->clasp() == clasp);
        JS_ASSERT(type->proto() == proto);
        JS_ASSERT_IF(type->hasNewScript(), type->newScript()->fun == fun);
        return type;
    }

    AutoEnterAnalysis enter(this);

    if (proto.isObject() && !proto.toObject()->setDelegate(this))
        return nullptr;

    TypeObjectFlags initialFlags = 0;
    if (!proto.isObject() || proto.toObject()->lastProperty()->hasObjectFlag(BaseShape::NEW_TYPE_UNKNOWN)) {
        
        
        
        
        
        
        initialFlags = OBJECT_FLAG_UNKNOWN_MASK | OBJECT_FLAG_SETS_MARKED_UNKNOWN;
    }

    Rooted<TaggedProto> protoRoot(this, proto);
    TypeObject *type = compartment()->types.newTypeObject(this, clasp, protoRoot, initialFlags);
    if (!type)
        return nullptr;

    if (!newTypeObjects.add(p, TypeObjectWithNewScriptEntry(type, fun)))
        return nullptr;

#ifdef JSGC_GENERATIONAL
    if (proto.isObject() && hasNursery() && nursery().isInside(proto.toObject())) {
        asJSContext()->runtime()->gcStoreBuffer.putGeneric(
            NewTypeObjectsSetRef(&newTypeObjects, clasp, proto.toObject(), fun));
    }
#endif

    if (proto.isObject()) {
        RootedObject obj(this, proto.toObject());

        if (fun)
            CheckNewScriptProperties(asJSContext(), type, fun);

        






        if (obj->is<RegExpObject>()) {
            AddTypePropertyId(this, type, NameToId(names().source), Type::StringType());
            AddTypePropertyId(this, type, NameToId(names().global), Type::BooleanType());
            AddTypePropertyId(this, type, NameToId(names().ignoreCase), Type::BooleanType());
            AddTypePropertyId(this, type, NameToId(names().multiline), Type::BooleanType());
            AddTypePropertyId(this, type, NameToId(names().sticky), Type::BooleanType());
            AddTypePropertyId(this, type, NameToId(names().lastIndex), Type::Int32Type());
        }

        if (obj->is<StringObject>())
            AddTypePropertyId(this, type, NameToId(names().length), Type::Int32Type());

        if (obj->is<ErrorObject>()) {
            AddTypePropertyId(this, type, NameToId(names().fileName), Type::StringType());
            AddTypePropertyId(this, type, NameToId(names().lineNumber), Type::Int32Type());
            AddTypePropertyId(this, type, NameToId(names().columnNumber), Type::Int32Type());
            AddTypePropertyId(this, type, NameToId(names().stack), Type::StringType());
        }
    }

    return type;
}

#if defined(JSGC_GENERATIONAL) && defined(JS_GC_ZEAL)
void
JSCompartment::checkNewTypeObjectTableAfterMovingGC()
{
    




    JS::shadow::Runtime *rt = JS::shadow::Runtime::asShadowRuntime(runtimeFromMainThread());
    for (TypeObjectWithNewScriptSet::Enum e(newTypeObjects); !e.empty(); e.popFront()) {
        TypeObjectWithNewScriptEntry entry = e.front();
        JS_ASSERT(!IsInsideNursery(rt, entry.newFunction));
        TaggedProto proto = entry.object->proto();
        JS_ASSERT_IF(proto.isObject(), !IsInsideNursery(rt, proto.toObject()));
        TypeObjectWithNewScriptEntry::Lookup
            lookup(entry.object->clasp(), proto, entry.newFunction);
        TypeObjectWithNewScriptSet::Ptr ptr = newTypeObjects.lookup(lookup);
        JS_ASSERT(ptr.found() && &*ptr == &e.front());
    }
}
#endif

TypeObject *
ExclusiveContext::getSingletonType(const Class *clasp, TaggedProto proto)
{
    JS_ASSERT_IF(proto.isObject(), compartment() == proto.toObject()->compartment());

    AutoEnterAnalysis enter(this);

    TypeObjectWithNewScriptSet &table = compartment()->lazyTypeObjects;

    if (!table.initialized() && !table.init())
        return nullptr;

    TypeObjectWithNewScriptSet::AddPtr p = table.lookupForAdd(TypeObjectWithNewScriptSet::Lookup(clasp, proto, nullptr));
    if (p) {
        TypeObject *type = p->object;
        JS_ASSERT(type->lazy());

        return type;
    }

    Rooted<TaggedProto> protoRoot(this, proto);
    TypeObject *type = compartment()->types.newTypeObject(this, clasp, protoRoot);
    if (!type)
        return nullptr;

    if (!table.add(p, TypeObjectWithNewScriptEntry(type, nullptr)))
        return nullptr;

    type->initSingleton((JSObject *) TypeObject::LAZY_SINGLETON);
    MOZ_ASSERT(type->singleton(), "created type must be a proper singleton");

    return type;
}





void
ConstraintTypeSet::sweep(Zone *zone, bool *oom)
{
    





    unsigned objectCount = baseObjectCount();
    if (objectCount >= 2) {
        unsigned oldCapacity = HashSetCapacity(objectCount);
        TypeObjectKey **oldArray = objectSet;

        clearObjects();
        objectCount = 0;
        for (unsigned i = 0; i < oldCapacity; i++) {
            TypeObjectKey *object = oldArray[i];
            if (object && !IsAboutToBeFinalized(object)) {
                TypeObjectKey **pentry =
                    HashSetInsert<TypeObjectKey *,TypeObjectKey,TypeObjectKey>
                        (zone->types.typeLifoAlloc, objectSet, objectCount, object);
                if (pentry) {
                    *pentry = object;
                } else {
                    *oom = true;
                    flags |= TYPE_FLAG_ANYOBJECT;
                    clearObjects();
                    objectCount = 0;
                    break;
                }
            }
        }
        setBaseObjectCount(objectCount);
    } else if (objectCount == 1) {
        TypeObjectKey *object = (TypeObjectKey *) objectSet;
        if (IsAboutToBeFinalized(object)) {
            objectSet = nullptr;
            setBaseObjectCount(0);
        }
    }

    



    TypeConstraint *constraint = constraintList;
    constraintList = nullptr;
    while (constraint) {
        TypeConstraint *copy;
        if (constraint->sweep(zone->types, &copy)) {
            if (copy) {
                copy->next = constraintList;
                constraintList = copy;
            } else {
                *oom = true;
            }
        }
        constraint = constraint->next;
    }
}

inline void
TypeObject::clearProperties()
{
    setBasePropertyCount(0);
    propertySet = nullptr;
}








inline void
TypeObject::sweep(FreeOp *fop, bool *oom)
{
    if (!isMarked()) {
        if (addendum)
            fop->free_(addendum);
        return;
    }

    LifoAlloc &typeLifoAlloc = zone()->types.typeLifoAlloc;

    



    unsigned propertyCount = basePropertyCount();
    if (propertyCount >= 2) {
        unsigned oldCapacity = HashSetCapacity(propertyCount);
        Property **oldArray = propertySet;

        clearProperties();
        propertyCount = 0;
        for (unsigned i = 0; i < oldCapacity; i++) {
            Property *prop = oldArray[i];
            if (prop) {
                if (singleton() && !prop->types.constraintList && !zone()->isPreservingCode()) {
                    





                    continue;
                }

                Property *newProp = typeLifoAlloc.new_<Property>(*prop);
                if (newProp) {
                    Property **pentry =
                        HashSetInsert<jsid,Property,Property>
                            (typeLifoAlloc, propertySet, propertyCount, prop->id);
                    if (pentry) {
                        *pentry = newProp;
                        newProp->types.sweep(zone(), oom);
                        continue;
                    }
                }

                *oom = true;
                addFlags(OBJECT_FLAG_DYNAMIC_MASK | OBJECT_FLAG_UNKNOWN_PROPERTIES);
                clearProperties();
                return;
            }
        }
        setBasePropertyCount(propertyCount);
    } else if (propertyCount == 1) {
        Property *prop = (Property *) propertySet;
        if (singleton() && !prop->types.constraintList && !zone()->isPreservingCode()) {
            
            clearProperties();
        } else {
            Property *newProp = typeLifoAlloc.new_<Property>(*prop);
            if (newProp) {
                propertySet = (Property **) newProp;
                newProp->types.sweep(zone(), oom);
            } else {
                *oom = true;
                addFlags(OBJECT_FLAG_DYNAMIC_MASK | OBJECT_FLAG_UNKNOWN_PROPERTIES);
                clearProperties();
                return;
            }
        }
    }
}

void
TypeCompartment::sweep(FreeOp *fop)
{
    




    if (arrayTypeTable) {
        for (ArrayTypeTable::Enum e(*arrayTypeTable); !e.empty(); e.popFront()) {
            const ArrayTableKey &key = e.front().key();
            JS_ASSERT(key.type.isUnknown() || !key.type.isSingleObject());

            bool remove = false;
            TypeObject *typeObject = nullptr;
            if (!key.type.isUnknown() && key.type.isTypeObject()) {
                typeObject = key.type.typeObject();
                if (IsTypeObjectAboutToBeFinalized(&typeObject))
                    remove = true;
            }
            if (IsTypeObjectAboutToBeFinalized(e.front().value().unsafeGet()))
                remove = true;

            if (remove) {
                e.removeFront();
            } else if (typeObject && typeObject != key.type.typeObject()) {
                ArrayTableKey newKey;
                newKey.type = Type::ObjectType(typeObject);
                newKey.proto = key.proto;
                e.rekeyFront(newKey);
            }
        }
    }

    if (objectTypeTable) {
        for (ObjectTypeTable::Enum e(*objectTypeTable); !e.empty(); e.popFront()) {
            const ObjectTableKey &key = e.front().key();
            ObjectTableEntry &entry = e.front().value();

            bool remove = false;
            if (IsTypeObjectAboutToBeFinalized(entry.object.unsafeGet()))
                remove = true;
            if (IsShapeAboutToBeFinalized(entry.shape.unsafeGet()))
                remove = true;
            for (unsigned i = 0; !remove && i < key.nproperties; i++) {
                if (JSID_IS_STRING(key.properties[i])) {
                    JSString *str = JSID_TO_STRING(key.properties[i]);
                    if (IsStringAboutToBeFinalized(&str))
                        remove = true;
                    JS_ASSERT(AtomToId((JSAtom *)str) == key.properties[i]);
                }
                JS_ASSERT(!entry.types[i].isSingleObject());
                TypeObject *typeObject = nullptr;
                if (entry.types[i].isTypeObject()) {
                    typeObject = entry.types[i].typeObject();
                    if (IsTypeObjectAboutToBeFinalized(&typeObject))
                        remove = true;
                    else if (typeObject != entry.types[i].typeObject())
                        entry.types[i] = Type::ObjectType(typeObject);
                }
            }

            if (remove) {
                js_free(key.properties);
                js_free(entry.types);
                e.removeFront();
            }
        }
    }

    if (allocationSiteTable) {
        for (AllocationSiteTable::Enum e(*allocationSiteTable); !e.empty(); e.popFront()) {
            AllocationSiteKey key = e.front().key();
            bool keyDying = IsScriptAboutToBeFinalized(&key.script);
            bool valDying = IsTypeObjectAboutToBeFinalized(e.front().value().unsafeGet());
            if (keyDying || valDying)
                e.removeFront();
            else if (key.script != e.front().key().script)
                e.rekeyFront(key);
        }
    }
}

void
JSCompartment::sweepNewTypeObjectTable(TypeObjectWithNewScriptSet &table)
{
    gcstats::AutoPhase ap(runtimeFromMainThread()->gcStats,
                          gcstats::PHASE_SWEEP_TABLES_TYPE_OBJECT);

    JS_ASSERT(zone()->isGCSweeping());
    if (table.initialized()) {
        for (TypeObjectWithNewScriptSet::Enum e(table); !e.empty(); e.popFront()) {
            TypeObjectWithNewScriptEntry entry = e.front();
            if (IsTypeObjectAboutToBeFinalized(entry.object.unsafeGet())) {
                e.removeFront();
            } else if (entry.newFunction && IsObjectAboutToBeFinalized(&entry.newFunction)) {
                e.removeFront();
            } else if (entry.object != e.front().object) {
                TypeObjectWithNewScriptSet::Lookup lookup(entry.object->clasp(),
                                                          entry.object->proto(),
                                                          entry.newFunction);
                e.rekeyFront(lookup, entry);
            }
        }
    }
}

TypeCompartment::~TypeCompartment()
{
    js_delete(arrayTypeTable);
    js_delete(objectTypeTable);
    js_delete(allocationSiteTable);
}

 void
TypeScript::Sweep(FreeOp *fop, JSScript *script, bool *oom)
{
    JSCompartment *compartment = script->compartment();
    JS_ASSERT(compartment->zone()->isGCSweeping());

    unsigned num = NumTypeSets(script);
    StackTypeSet *typeArray = script->types->typeArray();

    
    for (unsigned i = 0; i < num; i++)
        typeArray[i].sweep(compartment->zone(), oom);
}

void
TypeScript::destroy()
{
    js_free(this);
}

void
Zone::addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf,
                             size_t *typePool,
                             size_t *baselineStubsOptimized)
{
    *typePool += types.typeLifoAlloc.sizeOfExcludingThis(mallocSizeOf);
#ifdef JS_ION
    if (jitZone()) {
        *baselineStubsOptimized +=
            jitZone()->optimizedStubSpace()->sizeOfExcludingThis(mallocSizeOf);
    }
#endif
}

void
TypeCompartment::addSizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf,
                                        size_t *allocationSiteTables,
                                        size_t *arrayTypeTables,
                                        size_t *objectTypeTables)
{
    if (allocationSiteTable)
        *allocationSiteTables += allocationSiteTable->sizeOfIncludingThis(mallocSizeOf);

    if (arrayTypeTable)
        *arrayTypeTables += arrayTypeTable->sizeOfIncludingThis(mallocSizeOf);

    if (objectTypeTable) {
        *objectTypeTables += objectTypeTable->sizeOfIncludingThis(mallocSizeOf);

        for (ObjectTypeTable::Enum e(*objectTypeTable);
             !e.empty();
             e.popFront())
        {
            const ObjectTableKey &key = e.front().key();
            const ObjectTableEntry &value = e.front().value();

            
            *objectTypeTables += mallocSizeOf(key.properties) + mallocSizeOf(value.types);
        }
    }
}

size_t
TypeObject::sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
    return mallocSizeOf(addendum);
}

TypeZone::TypeZone(Zone *zone)
  : zone_(zone),
    typeLifoAlloc(TYPE_LIFO_ALLOC_PRIMARY_CHUNK_SIZE),
    compilerOutputs(nullptr),
    pendingRecompiles(nullptr)
{
}

TypeZone::~TypeZone()
{
    js_delete(compilerOutputs);
    js_delete(pendingRecompiles);
}

void
TypeZone::sweep(FreeOp *fop, bool releaseTypes, bool *oom)
{
    JS_ASSERT(zone()->isGCSweeping());

    JSRuntime *rt = fop->runtime();

    



    LifoAlloc oldAlloc(typeLifoAlloc.defaultChunkSize());
    oldAlloc.steal(&typeLifoAlloc);

    
    size_t newCompilerOutputCount = 0;
    if (compilerOutputs) {
        for (size_t i = 0; i < compilerOutputs->length(); i++) {
            CompilerOutput &output = (*compilerOutputs)[i];
            if (output.isValid()) {
                JSScript *script = output.script();
                if (IsScriptAboutToBeFinalized(&script)) {
                    jit::GetIonScript(script, output.mode())->recompileInfoRef() = uint32_t(-1);
                    output.invalidate();
                } else {
                    output.setSweepIndex(newCompilerOutputCount++);
                }
            }
        }
    }

    {
        gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_DISCARD_TI);

        for (CellIterUnderGC i(zone(), FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            if (script->types) {
                types::TypeScript::Sweep(fop, script, oom);

                if (releaseTypes) {
                    if (script->hasParallelIonScript()) {
                        
                        
                        
                        MOZ_ASSERT(jit::ShouldPreserveParallelJITCode(rt, script));
                        script->parallelIonScript()->recompileInfoRef().shouldSweep(*this);
                    } else {
                        script->types->destroy();
                        script->types = nullptr;

                        



                        script->clearHasFreezeConstraints();
                    }

                    JS_ASSERT(!script->hasIonScript());
                } else {
                    
                    if (script->hasIonScript())
                        script->ionScript()->recompileInfoRef().shouldSweep(*this);
                    if (script->hasParallelIonScript())
                        script->parallelIonScript()->recompileInfoRef().shouldSweep(*this);
                }
            }
        }
    }

    {
        gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_SWEEP_TYPES);

        for (gc::CellIterUnderGC iter(zone(), gc::FINALIZE_TYPE_OBJECT);
             !iter.done(); iter.next())
        {
            TypeObject *object = iter.get<TypeObject>();
            object->sweep(fop, oom);
        }

        for (CompartmentsInZoneIter comp(zone()); !comp.done(); comp.next())
            comp->types.sweep(fop);
    }

    if (compilerOutputs) {
        size_t sweepIndex = 0;
        for (size_t i = 0; i < compilerOutputs->length(); i++) {
            CompilerOutput output = (*compilerOutputs)[i];
            if (output.isValid()) {
                JS_ASSERT(sweepIndex == output.sweepIndex());
                output.invalidateSweepIndex();
                (*compilerOutputs)[sweepIndex++] = output;
            }
        }
        JS_ASSERT(sweepIndex == newCompilerOutputCount);
        JS_ALWAYS_TRUE(compilerOutputs->resize(newCompilerOutputCount));
    }

    {
        gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_CLEAR_SCRIPT_ANALYSIS);
        for (CellIterUnderGC i(zone(), FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            script->clearAnalysis();
        }
    }

    {
        gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_FREE_TI_ARENA);
        rt->freeLifoAlloc.transferFrom(&oldAlloc);
    }
}

void
TypeZone::clearAllNewScriptAddendumsOnOOM()
{
    for (gc::CellIterUnderGC iter(zone(), gc::FINALIZE_TYPE_OBJECT);
         !iter.done(); iter.next())
    {
        TypeObject *object = iter.get<TypeObject>();
        object->maybeClearNewScriptAddendumOnOOM();
    }
}

#ifdef DEBUG
void
TypeScript::printTypes(JSContext *cx, HandleScript script) const
{
    JS_ASSERT(script->types == this);

    if (!script->hasBaselineScript())
        return;

    AutoEnterAnalysis enter(nullptr, script->compartment());

    if (script->functionNonDelazifying())
        fprintf(stderr, "Function");
    else if (script->isForEval())
        fprintf(stderr, "Eval");
    else
        fprintf(stderr, "Main");
    fprintf(stderr, " #%u %s:%d ", script->id(), script->filename(), (int) script->lineno());

    if (script->functionNonDelazifying()) {
        if (js::PropertyName *name = script->functionNonDelazifying()->name()) {
            const jschar *chars = name->getChars(nullptr);
            JSString::dumpChars(chars, name->length());
        }
    }

    fprintf(stderr, "\n    this:");
    TypeScript::ThisTypes(script)->print();

    for (unsigned i = 0;
         script->functionNonDelazifying() && i < script->functionNonDelazifying()->nargs();
         i++)
    {
        fprintf(stderr, "\n    arg%u:", i);
        TypeScript::ArgTypes(script, i)->print();
    }
    fprintf(stderr, "\n");

    for (jsbytecode *pc = script->code(); pc < script->codeEnd(); pc += GetBytecodeLength(pc)) {
        PrintBytecode(cx, script, pc);

        if (js_CodeSpec[*pc].format & JOF_TYPESET) {
            StackTypeSet *types = TypeScript::BytecodeTypes(script, pc);
            fprintf(stderr, "  typeset %u:", unsigned(types - typeArray()));
            types->print();
            fprintf(stderr, "\n");
        }
    }

    fprintf(stderr, "\n");
}
#endif 





void
TypeObject::setAddendum(TypeObjectAddendum *addendum)
{
    this->addendum = addendum;
}

bool
TypeObject::addTypedObjectAddendum(JSContext *cx, Handle<TypeDescr*> descr)
{
    
    
    
    JS_ASSERT(!IsInsideNursery(cx->runtime(), descr));
    JS_ASSERT(descr);

    if (flags() & OBJECT_FLAG_ADDENDUM_CLEARED)
        return true;

    JS_ASSERT(!unknownProperties());

    if (addendum) {
        JS_ASSERT(hasTypedObject());
        JS_ASSERT(&typedObject()->descr() == descr);
        return true;
    }

    TypeTypedObject *typedObject = js_new<TypeTypedObject>(descr);
    if (!typedObject)
        return false;
    addendum = typedObject;
    return true;
}





TypeObjectAddendum::TypeObjectAddendum(Kind kind)
  : kind(kind)
{}

TypeNewScript::TypeNewScript()
  : TypeObjectAddendum(NewScript)
{}

TypeTypedObject::TypeTypedObject(Handle<TypeDescr*> descr)
  : TypeObjectAddendum(TypedObject),
    descr_(descr)
{
}

TypeDescr &
js::types::TypeTypedObject::descr() {
    return descr_->as<TypeDescr>();
}

