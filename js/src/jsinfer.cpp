





#include "jsinferinlines.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"

#include "jsapi.h"
#include "jsautooplen.h"
#include "jscntxt.h"
#include "jsgc.h"
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
#include "jit/IonCompartment.h"
#endif
#include "js/MemoryMetrics.h"
#include "vm/Shape.h"

#include "jsatominlines.h"
#include "jsgcinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

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
                  id_, this, filename() ? filename() : "<null>", lineno);
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
    



    if (cx->typeInferenceEnabled() && !obj->unknownProperties() && !value.isUndefined()) {
        id = IdToTypeId(id);

        
        if (id == id___proto__(cx) || id == id_constructor(cx) || id == id_caller(cx))
            return true;

        




        if (cx->compartment()->types.pendingCount)
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
TypeSet::mightBeType(JSValueType type)
{
    if (unknown())
        return true;

    if (type == JSVAL_TYPE_OBJECT)
        return unknownObject() || baseObjectCount() != 0;

    return baseFlags() & PrimitiveTypeFlag(type);
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

inline void
TypeSet::addTypesToConstraint(JSContext *cx, TypeConstraint *constraint)
{
    



    TypeList types;
    if (!enumerateTypes(&types))
        cx->compartment()->types.setPendingNukeTypes(cx);

    for (unsigned i = 0; i < types.length(); i++)
        constraint->newType(cx, this, types[i]);
}

void
TypeSet::add(JSContext *cx, TypeConstraint *constraint, bool callExisting)
{
    JS_ASSERT(isStackSet() || isHeapSet());

    if (!constraint) {
        
        cx->compartment()->types.setPendingNukeTypes(cx);
        return;
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
        addTypesToConstraint(cx, constraint);
}

void
TypeSet::print()
{
    if (flags & TYPE_FLAG_CONFIGURED_PROPERTY)
        fprintf(stderr, " [configured]");

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

    uint32_t newFlags = flags & ~(TYPE_FLAG_STACK_SET | TYPE_FLAG_HEAP_SET);

    new(result) TemporaryTypeSet(newFlags, capacity ? newSet : objectSet);
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
TypeSet::unionSets(TypeSet *a, TypeSet *b, LifoAlloc *alloc)
{
    TemporaryTypeSet *res = alloc->new_<TemporaryTypeSet>(a->baseFlags() | b->baseFlags(),
                                                          static_cast<TypeObjectKey**>(nullptr));
    if (!res)
        return nullptr;

    if (!res->unknownObject()) {
        for (size_t i = 0; i < a->getObjectCount() && !res->unknownObject(); i++) {
            TypeObjectKey *key = a->getObject(i);
            if (key && !res->addType(Type::ObjectType(key), alloc))
                return nullptr;
        }
        for (size_t i = 0; i < b->getObjectCount() && !res->unknownObject(); i++) {
            TypeObjectKey *key = b->getObject(i);
            if (key && !res->addType(Type::ObjectType(key), alloc))
                return nullptr;
        }
    }

    return res;
}























static LifoAlloc *IonAlloc() {
#ifdef JS_ION
    return jit::GetIonContext()->temp->lifoAlloc();
#else
    MOZ_CRASH();
#endif
}

namespace {



class CompilerConstraint
{
  public:
    
    HeapTypeSetKey property;

    
    
    
    TemporaryTypeSet *expected;

    CompilerConstraint(const HeapTypeSetKey &property)
      : property(property),
        expected(property.maybeTypes() ? property.maybeTypes()->clone(IonAlloc()) : NULL)
    {}

    
    
    virtual bool generateTypeConstraint(JSContext *cx, RecompileInfo recompileInfo) = 0;
};

} 

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

#ifdef JS_ION
    
    Vector<CompilerConstraint *, 0, jit::IonAllocPolicy> constraints;

    
    Vector<FrozenScript, 1, jit::IonAllocPolicy> frozenScripts;
#endif

    
    bool failed_;

  public:
    CompilerConstraintList()
      : failed_(false)
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
};

CompilerConstraintList *
types::NewCompilerConstraintList()
{
    return IonAlloc()->new_<CompilerConstraintList>();
}

 bool
TypeScript::FreezeTypeSets(CompilerConstraintList *constraints, JSScript *script,
                           TemporaryTypeSet **pThisTypes,
                           TemporaryTypeSet **pArgTypes,
                           TemporaryTypeSet **pBytecodeTypes)
{
    LifoAlloc *alloc = IonAlloc();
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
    *pArgTypes = (script->function() && script->function()->nargs)
                 ? (types + (ArgTypes(script, 0) - existing))
                 : NULL;
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
    CompilerConstraintInstance<T>(const HeapTypeSetKey &property, const T &data)
      : CompilerConstraint(property), data(data)
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
            cx->compartment()->types.addPendingRecompile(cx, compilation);
    }

    void newPropertyState(JSContext *cx, TypeSet *source) {
        if (data.invalidateOnNewPropertyState(source))
            cx->compartment()->types.addPendingRecompile(cx, compilation);
    }

    void newObjectState(JSContext *cx, TypeObject *object) {
        
        
        
        if (object->unknownProperties() || data.invalidateOnNewObjectState(object))
            cx->compartment()->types.addPendingRecompile(cx, compilation);
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

    property.maybeTypes()->add(cx, cx->typeLifoAlloc().new_<TypeCompilerConstraint<T> >(recompileInfo, data),
                                false);
    return true;
}

} 

const Class *
TypeObjectKey::clasp()
{
    return isTypeObject() ? asTypeObject()->clasp : asSingleObject()->getClass();
}

TaggedProto
TypeObjectKey::proto()
{
    return isTypeObject() ? TaggedProto(asTypeObject()->proto) : asSingleObject()->getTaggedProto();
}

JSObject *
TypeObjectKey::singleton()
{
    return isTypeObject() ? asTypeObject()->singleton : asSingleObject();
}

TypeNewScript *
TypeObjectKey::newScript()
{
    if (isTypeObject()) {
        TypeObjectAddendum *addendum = asTypeObject()->addendum;
        if (addendum && addendum->isNewScript())
            return addendum->asNewScript();
    }
    return nullptr;
}

TypeObject *
TypeObjectKey::maybeType()
{
    if (isTypeObject())
        return asTypeObject();
    if (asSingleObject()->hasLazyType())
        return NULL;
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
TypeObjectKey::property(jsid id, JSContext *maybecx )
{
    JS_ASSERT(!unknownProperties());

    HeapTypeSetKey property;
    property.object_ = this;
    property.id_ = id;
    if (TypeObject *type = maybeType())
        property.maybeTypes_ = type->maybeGetProperty(id);

#ifdef JS_ION
    
    
    
    if (maybecx && !property.maybeTypes() && !JSID_IS_VOID(id) && !JSID_IS_EMPTY(id)) {
        JS_ASSERT(CurrentThreadCanAccessRuntime(maybecx->runtime()));
        JSObject *singleton = isSingleObject() ? asSingleObject() : asTypeObject()->singleton;
        if (singleton && singleton->isNative() && singleton->nativeLookupPure(id)) {
            EnsureTrackPropertyTypes(maybecx, singleton, id);
            if (TypeObject *type = maybeType())
                property.maybeTypes_ = type->maybeGetProperty(id);
        }
    }
#endif 

    return property;
}

bool
HeapTypeSetKey::instantiate(JSContext *cx)
{
    if (maybeTypes())
        return true;
    if (object()->isSingleObject() && !object()->asSingleObject()->getType(cx))
        return false;
    maybeTypes_ = object()->maybeType()->getProperty(cx, id());
    return maybeTypes_ != NULL;
}

static bool
CheckFrozenTypeSet(JSContext *cx, TemporaryTypeSet *frozen, StackTypeSet *actual)
{
    
    
    
    
    

    if (!actual->isSubset(frozen))
        return false;

    if (!frozen->isSubset(actual)) {
        TypeSet::TypeList list;
        frozen->enumerateTypes(&list);

        for (size_t i = 0; i < list.length(); i++) {
            
            actual->addType(cx, list[i]);
        }
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

    void newType(JSContext *cx, TypeSet *source, Type type)
    {
        



        cx->compartment()->types.addPendingRecompile(cx, script_);
    }
};

} 

bool
types::FinishCompilation(JSContext *cx, JSScript *script, ExecutionMode executionMode,
                         CompilerConstraintList *constraints, RecompileInfo *precompileInfo)
{
    if (constraints->failed())
        return false;

    CompilerOutput co(script, executionMode);

    TypeCompartment &types = cx->compartment()->types;
    if (!types.constrainedOutputs) {
        types.constrainedOutputs = cx->new_< Vector<CompilerOutput> >(cx);
        if (!types.constrainedOutputs)
            return false;
    }

    uint32_t index = types.constrainedOutputs->length();
    if (!types.constrainedOutputs->append(co))
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
        unsigned nargs = entry.script->function() ? entry.script->function()->nargs : 0;
        for (size_t i = 0; i < nargs; i++) {
            if (!CheckFrozenTypeSet(cx, &entry.argTypes[i], types::TypeScript::ArgTypes(entry.script, i)))
                succeeded = false;
        }
        for (size_t i = 0; i < entry.script->nTypeSets; i++) {
            if (!CheckFrozenTypeSet(cx, &entry.bytecodeTypes[i], &entry.script->types->typeArray()[i]))
                succeeded = false;
        }

        
        
        if (entry.script->hasFreezeConstraints)
            continue;
        entry.script->hasFreezeConstraints = true;

        size_t count = TypeScript::NumTypeSets(entry.script);

        StackTypeSet *array = entry.script->types->typeArray();
        for (size_t i = 0; i < count; i++)
            array[i].add(cx, cx->typeLifoAlloc().new_<TypeConstraintFreezeStack>(entry.script), false);
    }

    if (!succeeded || types.constrainedOutputs->back().pendingInvalidation()) {
        types.constrainedOutputs->back().invalidate();
        return false;
    }

    return true;
}

namespace {


class ConstraintDataFreeze
{
  public:
    ConstraintDataFreeze() {}

    const char *kind() { return "freeze"; }

    bool invalidateOnNewType(Type type) { return true; }
    bool invalidateOnNewPropertyState(TypeSet *property) { return false; }
    bool invalidateOnNewObjectState(TypeObject *object) { return false; }

    bool constraintHolds(JSContext *cx,
                         const HeapTypeSetKey &property, TemporaryTypeSet *expected)
    {
        return expected
               ? property.maybeTypes()->isSubset(expected)
               : property.maybeTypes()->empty();
    }
};

} 

void
HeapTypeSetKey::freeze(CompilerConstraintList *constraints)
{
    constraints->add(IonAlloc()->new_<CompilerConstraintInstance<ConstraintDataFreeze> >(*this, ConstraintDataFreeze()));
}

static inline JSValueType
GetValueTypeFromTypeFlags(TypeFlags flags)
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
      case (TYPE_FLAG_INT32 | TYPE_FLAG_DOUBLE):
        return JSVAL_TYPE_DOUBLE;
      case TYPE_FLAG_STRING:
        return JSVAL_TYPE_STRING;
      case TYPE_FLAG_LAZYARGS:
        return JSVAL_TYPE_MAGIC;
      case TYPE_FLAG_ANYOBJECT:
        return JSVAL_TYPE_OBJECT;
      default:
        return JSVAL_TYPE_UNKNOWN;
    }
}

JSValueType
TemporaryTypeSet::getKnownTypeTag()
{
    TypeFlags flags = baseFlags();
    JSValueType type;

    if (baseObjectCount())
        type = flags ? JSVAL_TYPE_UNKNOWN : JSVAL_TYPE_OBJECT;
    else
        type = GetValueTypeFromTypeFlags(flags);

    






    DebugOnly<bool> empty = flags == 0 && baseObjectCount() == 0;
    JS_ASSERT_IF(empty, type == JSVAL_TYPE_UNKNOWN);

    return type;
}

JSValueType
HeapTypeSetKey::knownTypeTag(CompilerConstraintList *constraints)
{
    TypeSet *types = maybeTypes();

    if (!types || types->unknown())
        return JSVAL_TYPE_UNKNOWN;

    TypeFlags flags = types->baseFlags() & ~TYPE_FLAG_ANYOBJECT;
    JSValueType type;

    if (types->unknownObject() || types->getObjectCount())
        type = flags ? JSVAL_TYPE_UNKNOWN : JSVAL_TYPE_OBJECT;
    else
        type = GetValueTypeFromTypeFlags(flags);

    if (type != JSVAL_TYPE_UNKNOWN)
        freeze(constraints);

    






    JS_ASSERT_IF(types->empty(), type == JSVAL_TYPE_UNKNOWN);

    return type;
}

bool
HeapTypeSetKey::notEmpty(CompilerConstraintList *constraints)
{
    if (maybeTypes() && !maybeTypes()->empty())
        return true;
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
    TypeSet *types = maybeTypes();

    if (!types || types->baseFlags() != 0 || types->getObjectCount() != 1)
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
    constraints->add(IonAlloc()->new_<CompilerConstraintInstance<ConstraintDataFreezeObjectFlags> >(objectProperty, ConstraintDataFreezeObjectFlags(flags)));
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
        return object->singleton->as<TypedArrayObject>().viewData() != viewData;
    }

    bool constraintHolds(JSContext *cx,
                         const HeapTypeSetKey &property, TemporaryTypeSet *expected)
    {
        return !invalidateOnNewObjectState(property.object()->maybeType());
    }
};

} 

void
TypeObjectKey::watchStateChangeForInlinedCall(CompilerConstraintList *constraints)
{
    HeapTypeSetKey objectProperty = property(JSID_EMPTY);
    constraints->add(IonAlloc()->new_<CompilerConstraintInstance<ConstraintDataFreezeObjectForInlinedCall> >(objectProperty, ConstraintDataFreezeObjectForInlinedCall()));
}

void
TypeObjectKey::watchStateChangeForNewScriptTemplate(CompilerConstraintList *constraints)
{
    JSObject *templateObject = asTypeObject()->newScript()->templateObject;
    HeapTypeSetKey objectProperty = property(JSID_EMPTY);
    constraints->add(IonAlloc()->new_<CompilerConstraintInstance<ConstraintDataFreezeObjectForNewScriptTemplate> >(objectProperty, ConstraintDataFreezeObjectForNewScriptTemplate(templateObject)));
}

void
TypeObjectKey::watchStateChangeForTypedArrayBuffer(CompilerConstraintList *constraints)
{
    void *viewData = asSingleObject()->as<TypedArrayObject>().viewData();
    HeapTypeSetKey objectProperty = property(JSID_EMPTY);
    constraints->add(IonAlloc()->new_<CompilerConstraintInstance<ConstraintDataFreezeObjectForTypedArrayBuffer> >(objectProperty, ConstraintDataFreezeObjectForTypedArrayBuffer(viewData)));
}

static void
ObjectStateChange(ExclusiveContext *cxArg, TypeObject *object, bool markingUnknown)
{
    if (object->unknownProperties())
        return;

    
    TypeSet *types = object->maybeGetProperty(JSID_EMPTY);

    
    if (markingUnknown)
        object->flags |= OBJECT_FLAG_DYNAMIC_MASK | OBJECT_FLAG_UNKNOWN_PROPERTIES;

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

class ConstraintDataFreezeConfiguredProperty
{
  public:
    TypeObjectKey *object;

    ConstraintDataFreezeConfiguredProperty(TypeObjectKey *object)
      : object(object)
    {}

    const char *kind() { return "freezeConfiguredProperty"; }

    bool invalidateOnNewType(Type type) { return false; }
    bool invalidateOnNewPropertyState(TypeSet *property) {
        return property->configuredProperty();
    }
    bool invalidateOnNewObjectState(TypeObject *object) { return false; }

    bool constraintHolds(JSContext *cx,
                         const HeapTypeSetKey &property, TemporaryTypeSet *expected)
    {
        
        
        
        
        TypeObject *type = object->isSingleObject()
                           ? object->asSingleObject()->type()
                           : object->asTypeObject();
        if (type->flags & OBJECT_FLAG_NEW_SCRIPT_REGENERATE) {
            type->flags &= ~OBJECT_FLAG_NEW_SCRIPT_REGENERATE;
            if (type->hasNewScript()) {
                CheckNewScriptProperties(cx, type, type->newScript()->fun);
            } else {
                JS_ASSERT(type->flags & OBJECT_FLAG_ADDENDUM_CLEARED);
                type->flags &= ~OBJECT_FLAG_NEW_SCRIPT_REGENERATE;
            }
        }

        return !property.maybeTypes()->configuredProperty();
    }
};

} 

bool
HeapTypeSetKey::configured(CompilerConstraintList *constraints, TypeObjectKey *type)
{
    if (maybeTypes() && maybeTypes()->configuredProperty())
        return true;

    constraints->add(IonAlloc()->new_<CompilerConstraintInstance<ConstraintDataFreezeConfiguredProperty> >(*this, ConstraintDataFreezeConfiguredProperty(type)));
    return false;
}

bool
TypeObject::incrementTenureCount()
{
    uint32_t count = tenureCount();
    JS_ASSERT(count <= OBJECT_FLAG_TENURE_COUNT_LIMIT);

    if (count >= OBJECT_FLAG_TENURE_COUNT_LIMIT)
        return false;

    flags = (flags & ~OBJECT_FLAG_TENURE_COUNT_MASK)
          | ((count + 1) << OBJECT_FLAG_TENURE_COUNT_SHIFT);

    return count >= MaxJITAllocTenures;
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

        
        
        
        if (property.knownTypeTag(constraints) == JSVAL_TYPE_DOUBLE &&
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

int
TemporaryTypeSet::getTypedArrayType()
{
    const Class *clasp = getKnownClass();

    if (clasp && IsTypedArrayClass(clasp))
        return clasp - &TypedArrayObject::classes[0];
    return ScalarTypeRepresentation::TYPE_MAX;
}

bool
TemporaryTypeSet::isDOMClass()
{
    if (unknownObject())
        return false;

    unsigned count = getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        const Class *clasp = getObjectClass(i);
        if (clasp && !(clasp->flags & JSCLASS_IS_DOMJSCLASS))
            return false;
    }

    return true;
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
        if (clasp && (clasp->emulatesUndefined() || IsProxyClass(clasp)))
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
        TaggedProto nproto;
        if (JSObject *object = getSingleObject(i))
            nproto = object->getProto();
        else if (TypeObject *object = getTypeObject(i))
            nproto = object->proto.get();
        else
            continue;

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

void
TypeZone::init(JSContext *cx)
{
    if (!cx ||
        !cx->options().typeInference() ||
        !cx->runtime()->jitSupportsFloatingPoint)
    {
        return;
    }

    inferenceEnabled = true;
}

TypeObject *
TypeCompartment::newTypeObject(ExclusiveContext *cx, const Class *clasp, Handle<TaggedProto> proto, bool unknown)
{
    JS_ASSERT_IF(proto.isObject(), cx->isInsideCurrentCompartment(proto.toObject()));

    TypeObject *object = gc::NewGCThing<TypeObject, CanGC>(cx, gc::FINALIZE_TYPE_OBJECT,
                                                           sizeof(TypeObject), gc::TenuredHeap);
    if (!object)
        return nullptr;
    new(object) TypeObject(clasp, proto, unknown);

    if (!cx->typeInferenceEnabled())
        object->flags |= OBJECT_FLAG_UNKNOWN_MASK;

    return object;
}

static inline jsbytecode *
PreviousOpcode(HandleScript script, jsbytecode *pc)
{
    ScriptAnalysis *analysis = script->analysis();
    JS_ASSERT(analysis->maybeCode(pc));

    if (pc == script->code)
        return nullptr;

    for (pc--;; pc--) {
        if (analysis->maybeCode(pc))
            break;
    }

    return pc;
}





static inline jsbytecode *
FindPreviousInnerInitializer(HandleScript script, jsbytecode *initpc)
{
    if (!script->hasAnalysis())
        return nullptr;

    if (!script->analysis()->maybeCode(initpc))
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
            cx->compartment()->types.setPendingNukeTypes(cx);
            return nullptr;
        }
    }

    AllocationSiteTable::AddPtr p = allocationSiteTable->lookupForAdd(key);
    JS_ASSERT(!p);

    TypeObject *res = nullptr;

    




    jsbytecode *pc = key.script->code + key.offset;
    RootedScript keyScript(cx, key.script);
    jsbytecode *prev = FindPreviousInnerInitializer(keyScript, pc);
    if (prev) {
        AllocationSiteKey nkey;
        nkey.script = key.script;
        nkey.offset = prev - key.script->code;
        nkey.kind = JSProto_Array;

        AllocationSiteTable::Ptr p = cx->compartment()->types.allocationSiteTable->lookup(nkey);
        if (p)
            res = p->value;
    }

    if (!res) {
        RootedObject proto(cx);
        if (!js_GetClassPrototype(cx, key.kind, &proto, nullptr))
            return nullptr;

        Rooted<TaggedProto> tagged(cx, TaggedProto(proto));
        res = newTypeObject(cx, GetClassForProtoKey(key.kind), tagged);
        if (!res) {
            cx->compartment()->types.setPendingNukeTypes(cx);
            return nullptr;
        }
        key.script = keyScript;
    }

    if (JSOp(*pc) == JSOP_NEWOBJECT) {
        




        RootedObject baseobj(cx, key.script->getObject(GET_UINT32_INDEX(pc)));

        if (!res->addDefiniteProperties(cx, baseobj))
            return nullptr;
    }

    if (!allocationSiteTable->add(p, key, res)) {
        cx->compartment()->types.setPendingNukeTypes(cx);
        return nullptr;
    }

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
    JS_ASSERT(cx->typeInferenceEnabled());

    
















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
    





    if (script->function() && !script->treatAsRunOnce)
        return GenericObject;

    if (key != JSProto_Object && !(key >= JSProto_Int8Array && key <= JSProto_Uint8ClampedArray))
        return GenericObject;

    




    if (!script->hasTrynotes())
        return SingletonObject;

    unsigned offset = pc - script->code;

    JSTryNote *tn = script->trynotes()->vector;
    JSTryNote *tnlimit = tn + script->trynotes()->length;
    for (; tn < tnlimit; tn++) {
        if (tn->kind != JSTRY_ITER && tn->kind != JSTRY_LOOP)
            continue;

        unsigned startOffset = script->mainOffset + tn->start;
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
    return clasp->resolve != JS_ResolveStub || clasp->ops.lookupGeneric || clasp->ops.getGeneric;
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
        if (index.configured(constraints, type) || index.notEmpty(constraints))
            return true;
        obj = obj->getProto();
    } while (obj);

    return false;
}

bool
types::ArrayPrototypeHasIndexedProperty(CompilerConstraintList *constraints,
                                        HandleScript script)
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

bool
TypeCompartment::growPendingArray(JSContext *cx)
{
    unsigned newCapacity = js::Max(unsigned(100), pendingCapacity * 2);
    PendingWork *newArray = js_pod_calloc<PendingWork>(newCapacity);
    if (!newArray) {
        cx->compartment()->types.setPendingNukeTypes(cx);
        return false;
    }

    PodCopy(newArray, pendingArray, pendingCount);
    js_free(pendingArray);

    pendingArray = newArray;
    pendingCapacity = newCapacity;

    return true;
}

void
TypeCompartment::processPendingRecompiles(FreeOp *fop)
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
TypeCompartment::setPendingNukeTypes(ExclusiveContext *cx)
{
    TypeZone *zone = &compartment()->zone()->types;
    if (!zone->pendingNukeTypes) {
        if (cx->compartment())
            js_ReportOutOfMemory(cx);
        zone->pendingNukeTypes = true;
    }
}

void
TypeZone::setPendingNukeTypes()
{
    pendingNukeTypes = true;
}

void
TypeZone::nukeTypes(FreeOp *fop)
{
    










    JS_ASSERT(pendingNukeTypes);

    for (CompartmentsInZoneIter comp(zone()); !comp.done(); comp.next()) {
        if (comp->types.pendingRecompiles) {
            fop->free_(comp->types.pendingRecompiles);
            comp->types.pendingRecompiles = nullptr;
        }
    }

    inferenceEnabled = false;

#ifdef JS_ION
    jit::InvalidateAll(fop, zone());

    

    for (gc::CellIter i(zone(), gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        jit::FinishInvalidation(fop, script);
    }
#endif 

    pendingNukeTypes = false;
}

void
TypeCompartment::addPendingRecompile(JSContext *cx, const RecompileInfo &info)
{
    CompilerOutput *co = info.compilerOutput(cx);
    if (!co || !co->isValid() || co->pendingInvalidation())
        return;

    if (!pendingRecompiles) {
        pendingRecompiles = cx->new_< Vector<RecompileInfo> >(cx);
        if (!pendingRecompiles) {
            cx->compartment()->types.setPendingNukeTypes(cx);
            return;
        }
    }

    if (!pendingRecompiles->append(info)) {
        cx->compartment()->types.setPendingNukeTypes(cx);
        return;
    }

    InferSpew(ISpewOps, "addPendingRecompile: %p:%s:%d",
              co->script(), co->script()->filename(), co->script()->lineno);

    co->setPendingInvalidation();
}

void
TypeCompartment::addPendingRecompile(JSContext *cx, JSScript *script)
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

    
    
    
    if (script->function() && !script->function()->hasLazyType())
        ObjectStateChange(cx, script->function()->type(), false);
}

void
TypeCompartment::markSetsUnknown(JSContext *cx, TypeObject *target)
{
    JS_ASSERT(this == &cx->compartment()->types);
    JS_ASSERT(!(target->flags & OBJECT_FLAG_SETS_MARKED_UNKNOWN));
    JS_ASSERT(!target->singleton);
    JS_ASSERT(target->unknownProperties());
    target->flags |= OBJECT_FLAG_SETS_MARKED_UNKNOWN;

    AutoEnterAnalysis enter(cx);

    









    Vector<TypeSet *> pending(cx);
    for (gc::CellIter i(cx->zone(), gc::FINALIZE_TYPE_OBJECT); !i.done(); i.next()) {
        TypeObject *object = i.get<TypeObject>();
        unsigned count = object->getPropertyCount();
        for (unsigned i = 0; i < count; i++) {
            Property *prop = object->getProperty(i);
            if (prop && prop->types.hasType(Type::ObjectType(target))) {
                if (!pending.append(&prop->types))
                    cx->compartment()->types.setPendingNukeTypes(cx);
            }
        }
    }

    for (unsigned i = 0; i < pending.length(); i++)
        pending[i]->addType(cx, Type::AnyObjectType());

    for (gc::CellIter i(cx->zone(), gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        RootedScript script(cx, i.get<JSScript>());
        if (script->types) {
            unsigned count = TypeScript::NumTypeSets(script);
            TypeSet *typeArray = script->types->typeArray();
            for (unsigned i = 0; i < count; i++) {
                if (typeArray[i].hasType(Type::ObjectType(target)))
                    typeArray[i].addType(cx, Type::AnyObjectType());
            }
        }
    }
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
    if (!arrayTypeTable) {
        arrayTypeTable = cx->new_<ArrayTypeTable>();
        if (!arrayTypeTable || !arrayTypeTable->init()) {
            arrayTypeTable = nullptr;
            cx->compartment()->types.setPendingNukeTypes(cx);
            return;
        }
    }

    ArrayTableKey key;
    key.type = elementType;
    key.proto = obj->getProto();
    ArrayTypeTable::AddPtr p = arrayTypeTable->lookupForAdd(key);

    if (p) {
        obj->setType(p->value);
    } else {
        
        RootedObject objProto(cx, obj->getProto());
        TypeObject *objType = newTypeObject(cx, &ArrayObject::class_, objProto);
        if (!objType) {
            cx->compartment()->types.setPendingNukeTypes(cx);
            return;
        }
        obj->setType(objType);

        if (!objType->unknownProperties())
            objType->addPropertyType(cx, JSID_VOID, elementType);

        if (!arrayTypeTable->relookupOrAdd(p, key, objType)) {
            cx->compartment()->types.setPendingNukeTypes(cx);
            return;
        }
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
    if (cx->typeInferenceEnabled())
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
            objectTypeTable = nullptr;
            cx->compartment()->types.setPendingNukeTypes(cx);
            return;
        }
    }

    



    JS_ASSERT(obj->is<JSObject>());

    if (obj->slotSpan() == 0 || obj->inDictionaryMode() || !obj->hasEmptyElements())
        return;

    Vector<IdValuePair> properties(cx);
    if (!properties.resize(obj->slotSpan())) {
        cx->compartment()->types.setPendingNukeTypes(cx);
        return;
    }

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
        JS_ASSERT(obj->getProto() == p->value.object->proto);
        JS_ASSERT(obj->lastProperty() == p->value.shape);

        UpdateObjectTableEntryTypes(cx, p->value, properties.begin(), properties.length());
        obj->setType(p->value.object);
        return;
    }

    
    Rooted<TaggedProto> objProto(cx, obj->getTaggedProto());
    TypeObject *objType = newTypeObject(cx, &JSObject::class_, objProto);
    if (!objType || !objType->addDefiniteProperties(cx, obj)) {
        cx->compartment()->types.setPendingNukeTypes(cx);
        return;
    }

    if (obj->isIndexed())
        objType->setFlags(cx, OBJECT_FLAG_SPARSE_INDEXES);

    jsid *ids = cx->pod_calloc<jsid>(properties.length());
    if (!ids) {
        cx->compartment()->types.setPendingNukeTypes(cx);
        return;
    }

    Type *types = cx->pod_calloc<Type>(properties.length());
    if (!types) {
        cx->compartment()->types.setPendingNukeTypes(cx);
        return;
    }

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

    p = objectTypeTable->lookupForAdd(lookup);
    if (!objectTypeTable->add(p, key, entry)) {
        cx->compartment()->types.setPendingNukeTypes(cx);
        return;
    }

    obj->setType(objType);
}

JSObject *
TypeCompartment::newTypedObject(JSContext *cx, IdValuePair *properties, size_t nproperties)
{
    AutoEnterAnalysis enter(cx);

    if (!objectTypeTable) {
        objectTypeTable = cx->new_<ObjectTypeTable>();
        if (!objectTypeTable || !objectTypeTable->init()) {
            objectTypeTable = nullptr;
            cx->compartment()->types.setPendingNukeTypes(cx);
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
    JS_ASSERT(obj->getProto() == p->value.object->proto);

    RootedShape shape(cx, p->value.shape);
    if (!JSObject::setLastProperty(cx, obj, shape)) {
        cx->clearPendingException();
        return nullptr;
    }

    UpdateObjectTableEntryTypes(cx, p->value, properties, nproperties);

    for (size_t i = 0; i < nproperties; i++)
        obj->setSlot(i, properties[i].value);

    obj->setType(p->value.object);
    return obj;
}





static inline void
UpdatePropertyType(ExclusiveContext *cx, TypeSet *types, JSObject *obj, Shape *shape,
                   bool force)
{
    if (!shape->writable())
        types->setConfiguredProperty(cx);

    if (shape->hasGetterValue() || shape->hasSetterValue()) {
        types->setConfiguredProperty(cx);
        types->addType(cx, Type::UnknownType());
    } else if (shape->hasDefaultGetter() && shape->hasSlot()) {
        const Value &value = obj->nativeGetSlot(shape->slot());

        



        if (force || !value.isUndefined()) {
            Type type = GetValueType(value);
            types->addType(cx, type);
        }
    }
}

bool
TypeObject::addProperty(ExclusiveContext *cx, jsid id, Property **pprop)
{
    JS_ASSERT(!*pprop);
    Property *base = cx->typeLifoAlloc().new_<Property>(id);
    if (!base) {
        cx->compartment()->types.setPendingNukeTypes(cx);
        return false;
    }

    if (singleton && singleton->isNative()) {
        






        RootedObject rSingleton(cx, singleton);
        if (JSID_IS_VOID(id)) {
            
            RootedShape shape(cx, singleton->lastProperty());
            while (!shape->isEmptyShape()) {
                if (JSID_IS_VOID(IdToTypeId(shape->propid())))
                    UpdatePropertyType(cx, &base->types, rSingleton, shape, true);
                shape = shape->previous();
            }

            
            for (size_t i = 0; i < singleton->getDenseInitializedLength(); i++) {
                const Value &value = singleton->getDenseElement(i);
                if (!value.isMagic(JS_ELEMENTS_HOLE)) {
                    Type type = GetValueType(value);
                    base->types.addType(cx, type);
                }
            }
        } else if (!JSID_IS_EMPTY(id)) {
            RootedId rootedId(cx, id);
            Shape *shape = singleton->nativeLookup(cx, rootedId);
            if (shape)
                UpdatePropertyType(cx, &base->types, rSingleton, shape, false);
        }

        if (singleton->watched()) {
            



            base->types.setConfiguredProperty(cx);
        }
    }

    *pprop = base;

    InferSpew(ISpewOps, "typeSet: %sT%p%s property %s %s",
              InferSpewColor(&base->types), &base->types, InferSpewColorReset(),
              TypeObjectString(this), TypeIdString(id));

    return true;
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
        if (!JSID_IS_VOID(id) && obj->isFixedSlot(shape->slot()) &&
            shape->slot() <= (TYPE_FLAG_DEFINITE_MASK >> TYPE_FLAG_DEFINITE_SHIFT))
        {
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

    TypeSet *types = obj->getProperty(cx, id);
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
TypeObject::addPropertyType(ExclusiveContext *cx, const char *name, Type type)
{
    jsid id = JSID_VOID;
    if (name) {
        JSAtom *atom = Atomize(cx, name, strlen(name));
        if (!atom) {
            AutoEnterAnalysis enter(cx);
            cx->compartment()->types.setPendingNukeTypes(cx);
            return;
        }
        id = AtomToId(atom);
    }
    InlineAddTypeProperty(cx, this, id, type);
}

void
TypeObject::addPropertyType(ExclusiveContext *cx, const char *name, const Value &value)
{
    addPropertyType(cx, name, GetValueType(value));
}

void
TypeObject::markPropertyConfigured(ExclusiveContext *cx, jsid id)
{
    AutoEnterAnalysis enter(cx);

    id = IdToTypeId(id);

    TypeSet *types = getProperty(cx, id);
    if (types)
        types->setConfiguredProperty(cx);
}

bool
TypeObject::isPropertyConfigured(jsid id)
{
    TypeSet *types = maybeGetProperty(id);
    if (types)
        return types->configuredProperty();
    return false;
}

void
TypeObject::markStateChange(ExclusiveContext *cxArg)
{
    if (unknownProperties())
        return;

    AutoEnterAnalysis enter(cxArg);
    TypeSet *types = maybeGetProperty(JSID_EMPTY);
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
    if ((this->flags & flags) == flags)
        return;

    AutoEnterAnalysis enter(cx);

    if (singleton) {
        
        JS_ASSERT_IF(flags & OBJECT_FLAG_ITERATED,
                     singleton->lastProperty()->hasObjectFlag(BaseShape::ITERATED_SINGLETON));
    }

    this->flags |= flags;

    InferSpew(ISpewOps, "%s: setFlags 0x%x", TypeObjectString(this), flags);

    ObjectStateChange(cx, this, false);
}

void
TypeObject::markUnknown(ExclusiveContext *cx)
{
    AutoEnterAnalysis enter(cx);

    JS_ASSERT(cx->compartment()->activeAnalysis);
    JS_ASSERT(!unknownProperties());

    if (!(flags & OBJECT_FLAG_ADDENDUM_CLEARED))
        clearAddendum(cx);

    InferSpew(ISpewOps, "UnknownProperties: %s", TypeObjectString(this));

    ObjectStateChange(cx, this, true);

    








    unsigned count = getPropertyCount();
    for (unsigned i = 0; i < count; i++) {
        Property *prop = getProperty(i);
        if (prop) {
            prop->types.addType(cx, Type::UnknownType());
            prop->types.setConfiguredProperty(cx);
        }
    }
}

void
TypeObject::clearAddendum(ExclusiveContext *cx)
{
    JS_ASSERT(!(flags & OBJECT_FLAG_ADDENDUM_CLEARED));
    flags |= OBJECT_FLAG_ADDENDUM_CLEARED;

    









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
            prop->types.setConfiguredProperty(cx);
    }

    







    if (cx->isJSContext()) {
        Vector<uint32_t, 32> pcOffsets(cx);
        for (ScriptFrameIter iter(cx->asJSContext()); !iter.done(); ++iter) {
            pcOffsets.append(uint32_t(iter.pc() - iter.script()->code));
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
                obj->rollbackProperties(cx, numProperties);
        }
    } else {
        
        JS_ASSERT(!cx->perThreadData->activation());
    }
}

void
TypeObject::clearTypedObjectAddendum(ExclusiveContext *cx)
{
}

void
TypeObject::print()
{
    TaggedProto tagged(proto);
    fprintf(stderr, "%s : %s",
           TypeObjectString(this),
           tagged.isObject() ? TypeString(Type::ObjectType(proto))
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
        





        if (!(object->flags & OBJECT_FLAG_ADDENDUM_CLEARED) && source->configuredProperty())
            object->clearAddendum(cx);
    }

    void newType(JSContext *cx, TypeSet *source, Type type) {}
};

bool
types::AddClearDefiniteGetterSetterForPrototypeChain(JSContext *cx, TypeObject *type, jsid id)
{
    




    RootedObject parent(cx, type->proto);
    while (parent) {
        TypeObject *parentObject = parent->getType(cx);
        if (!parentObject || parentObject->unknownProperties())
            return false;
        HeapTypeSet *parentTypes = parentObject->getProperty(cx, id);
        if (!parentTypes || parentTypes->configuredProperty())
            return false;
        parentTypes->add(cx, cx->typeLifoAlloc().new_<TypeConstraintClearDefiniteGetterSetter>(type));
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
        if (object->flags & OBJECT_FLAG_ADDENDUM_CLEARED)
            return;

        if (source->baseFlags() || source->getObjectCount() > 1)
            object->clearAddendum(cx);
    }
};

void
types::AddClearDefiniteFunctionUsesInScript(JSContext *cx, TypeObject *type,
                                            JSScript *script, JSScript *calleeScript)
{
    
    
    
    
    

    TypeObjectKey *calleeKey = Type::ObjectType(calleeScript->function()).objectKey();

    unsigned count = TypeScript::NumTypeSets(script);
    TypeSet *typeArray = script->types->typeArray();

    for (unsigned i = 0; i < count; i++) {
        TypeSet *types = &typeArray[i];
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
            
            
            types->add(cx,
                cx->typeLifoAlloc().new_<TypeConstraintClearDefiniteSingle>(type));
        }
    }
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

    if (!jit::AnalyzeNewScriptProperties(cx, fun, type, baseobj, &initializerList)) {
        cx->compartment()->types.setPendingNukeTypes(cx);
        return;
    }

    if (baseobj->slotSpan() == 0 ||
        !!(type->flags & OBJECT_FLAG_ADDENDUM_CLEARED))
    {
        if (type->addendum)
            type->clearAddendum(cx);
        return;
    }

    




    if (type->hasNewScript()) {
        if (!type->matchDefiniteProperties(baseobj))
            type->clearAddendum(cx);
        return;
    }
    JS_ASSERT(!type->addendum);
    JS_ASSERT(!(type->flags & OBJECT_FLAG_ADDENDUM_CLEARED));

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
        cx->compartment()->types.setPendingNukeTypes(cx);
        return;
    }

    size_t numBytes = sizeof(TypeNewScript)
                    + (initializerList.length() * sizeof(TypeNewScript::Initializer));
    TypeNewScript *newScript;
#ifdef JSGC_ROOT_ANALYSIS
    
    void *p;
    do {
        p = cx->calloc_(numBytes);
    } while (IsPoisonedPtr(p));
    newScript = (TypeNewScript *) p;
#else
    newScript = (TypeNewScript *) cx->calloc_(numBytes);
#endif
    new (newScript) TypeNewScript();
    type->addendum = newScript;

    if (!newScript) {
        cx->compartment()->types.setPendingNukeTypes(cx);
        return;
    }

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
    unsigned nargs = callee->as<JSFunction>().nargs;
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

    if (!script->types)
        return;

    AutoEnterAnalysis enter(cx);

    if (!script->ensureHasBytecodeTypeMap(cx)) {
        cx->compartment()->types.setPendingNukeTypes(cx);
        return;
    }

    Type type = GetValueType(rval);
    TypeSet *types = TypeScript::BytecodeTypes(script, pc);
    if (types->hasType(type))
        return;

    InferSpew(ISpewOps, "bytecodeType: #%u:%05u: %s",
              script->id(), pc - script->code, TypeString(type));
    types->addType(cx, type);
}

bool
types::UseNewTypeForClone(JSFunction *fun)
{
    if (!fun->isInterpreted())
        return false;

    if (fun->hasScript() && fun->nonLazyScript()->shouldCloneAtCallsite)
        return true;

    if (fun->isArrow())
        return false;

    if (fun->hasSingletonType())
        return false;

    























    uint32_t begin, end;
    if (fun->hasScript()) {
        if (!fun->nonLazyScript()->usesArgumentsAndApply)
            return false;
        begin = fun->nonLazyScript()->sourceStart;
        end = fun->nonLazyScript()->sourceEnd;
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

    if (!cx->typeInferenceEnabled()) {
        types = cx->pod_calloc<TypeScript>();
        if (!types) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        new(types) TypeScript();
        return analyzedArgsUsage() || ensureRanAnalysis(cx);
    }

    AutoEnterAnalysis enter(cx);

    unsigned count = TypeScript::NumTypeSets(this);

    types = (TypeScript *) cx->calloc_(sizeof(TypeScript) + (sizeof(TypeSet) * count));
    if (!types) {
        cx->compartment()->types.setPendingNukeTypes(cx);
        return false;
    }

    new(types) TypeScript();

    TypeSet *typeArray = types->typeArray();

    for (unsigned i = 0; i < count; i++)
        new (&typeArray[i]) StackTypeSet();

#ifdef DEBUG
    for (unsigned i = 0; i < nTypeSets; i++)
        InferSpew(ISpewOps, "typeSet: %sT%p%s bytecode%u #%u",
                  InferSpewColor(&typeArray[i]), &typeArray[i], InferSpewColorReset(),
                  i, id());
    TypeSet *thisTypes = TypeScript::ThisTypes(this);
    InferSpew(ISpewOps, "typeSet: %sT%p%s this #%u",
              InferSpewColor(thisTypes), thisTypes, InferSpewColorReset(),
              id());
    unsigned nargs = function() ? function()->nargs : 0;
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
JSScript::makeBytecodeTypeMap(JSContext *cx)
{
    JS_ASSERT(cx->typeInferenceEnabled());
    JS_ASSERT(types && !types->bytecodeMap);

    types->bytecodeMap = cx->typeLifoAlloc().newArrayUninitialized<uint32_t>(nTypeSets + 1);

    if (!types->bytecodeMap)
        return false;

    uint32_t added = 0;
    for (jsbytecode *pc = code; pc < code + length; pc += GetBytecodeLength(pc)) {
        JSOp op = JSOp(*pc);
        if (js_CodeSpec[op].format & JOF_TYPESET) {
            types->bytecodeMap[added++] = pc - code;
            if (added == nTypeSets)
                break;
        }
    }

    JS_ASSERT(added == nTypeSets);

    
    
    types->bytecodeMap[nTypeSets] = 0;

    return true;
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

    self->types->analysis->analyzeBytecode(cx);

    if (self->types->analysis->OOM()) {
        self->types->analysis = nullptr;
        return false;
    }

    return true;
}

 bool
JSFunction::setTypeForScriptedFunction(ExclusiveContext *cx, HandleFunction fun,
                                       bool singleton )
{
    if (!cx->typeInferenceEnabled())
        return true;

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
    return !cx->typeInferenceEnabled() || hasSingletonType();
}

bool
JSObject::splicePrototype(JSContext *cx, const Class *clasp, Handle<TaggedProto> proto)
{
    JS_ASSERT(cx->compartment() == compartment());

    RootedObject self(cx, this);

    





    JS_ASSERT_IF(cx->typeInferenceEnabled(), self->hasSingletonType());

    
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

    if (!cx->typeInferenceEnabled()) {
        TypeObject *type = cx->getNewType(clasp, proto);
        if (!type)
            return false;
        self->type_ = type;
        return true;
    }

    type->clasp = clasp;
    type->proto = proto.raw();

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
    Rooted<TaggedProto> proto(cx, obj->getTaggedProto());
    TypeObject *type = cx->compartment()->types.newTypeObject(cx, obj->getClass(), proto);
    if (!type) {
        if (cx->typeInferenceEnabled())
            cx->compartment()->types.setPendingNukeTypes(cx);
        return nullptr;
    }

    if (!cx->typeInferenceEnabled()) {
        
        obj->type_ = type;
        return type;
    }

    AutoEnterAnalysis enter(cx);

    

    type->singleton = obj;

    if (obj->is<JSFunction>() && obj->as<JSFunction>().isInterpreted())
        type->interpretedFunction = &obj->as<JSFunction>();

    if (obj->lastProperty()->hasObjectFlag(BaseShape::ITERATED_SINGLETON))
        type->flags |= OBJECT_FLAG_ITERATED;

    




    
    type->flags |= OBJECT_FLAG_NON_PACKED;

    if (obj->isIndexed())
        type->flags |= OBJECT_FLAG_SPARSE_INDEXES;

    if (obj->is<ArrayObject>() && obj->as<ArrayObject>().length() > INT32_MAX)
        type->flags |= OBJECT_FLAG_LENGTH_OVERFLOW;

    obj->type_ = type;

    return type;
}

 inline HashNumber
TypeObjectEntry::hash(const Lookup &lookup)
{
    return PointerHasher<JSObject *, 3>::hash(lookup.hashProto.raw()) ^
           PointerHasher<const Class *, 3>::hash(lookup.clasp);
}

 inline bool
TypeObjectEntry::match(TypeObject *key, const Lookup &lookup)
{
    return key->proto == lookup.matchProto.raw() && key->clasp == lookup.clasp;
}

#ifdef DEBUG
bool
JSObject::hasNewType(const Class *clasp, TypeObject *type)
{
    TypeObjectSet &table = compartment()->newTypeObjects;

    if (!table.initialized())
        return false;

    TypeObjectSet::Ptr p = table.lookup(TypeObjectSet::Lookup(clasp, this));
    return p && *p == type;
}
#endif 

 bool
JSObject::setNewTypeUnknown(JSContext *cx, const Class *clasp, HandleObject obj)
{
    if (!obj->setFlag(cx, js::BaseShape::NEW_TYPE_UNKNOWN))
        return false;

    




    TypeObjectSet &table = cx->compartment()->newTypeObjects;
    if (table.initialized()) {
        if (TypeObjectSet::Ptr p = table.lookup(TypeObjectSet::Lookup(clasp, obj.get())))
            MarkTypeObjectUnknownProperties(cx, *p);
    }

    return true;
}

#ifdef JSGC_GENERATIONAL





class NewTypeObjectsSetRef : public BufferableRef
{
    TypeObjectSet *set;
    TypeObject    *typeObject;
    JSObject      *proto;

  public:
    NewTypeObjectsSetRef(TypeObjectSet *s, TypeObject *t, JSObject *p)
      : set(s), typeObject(t), proto(p) {}

    void mark(JSTracer *trc) {
        const Class *clasp = typeObject->clasp;
        JSObject *prior = proto;
        JS_SET_TRACING_LOCATION(trc, (void*)&*prior);
        Mark(trc, &proto, "newTypeObjects set prototype");
        if (prior == proto)
            return;

        TypeObjectSet::Ptr p = set->lookup(TypeObjectSet::Lookup(clasp, prior, proto));
        JS_ASSERT(p);  

        set->rekeyAs(TypeObjectSet::Lookup(clasp, prior, proto),
                     TypeObjectSet::Lookup(clasp, proto), typeObject);
    }
};
#endif

TypeObject *
ExclusiveContext::getNewType(const Class *clasp, TaggedProto proto_, JSFunction *fun_)
{
    JS_ASSERT_IF(fun_, proto_.isObject());
    JS_ASSERT_IF(proto_.isObject(), isInsideCurrentCompartment(proto_.toObject()));

    TypeObjectSet &newTypeObjects = compartment_->newTypeObjects;

    if (!newTypeObjects.initialized() && !newTypeObjects.init())
        return nullptr;

    TypeObjectSet::AddPtr p = newTypeObjects.lookupForAdd(TypeObjectSet::Lookup(clasp, proto_));
    SkipRoot skipHash(this, &p); 
    uint64_t originalGcNumber = gcNumber();
    if (p) {
        TypeObject *type = *p;
        JS_ASSERT(type->clasp == clasp);
        JS_ASSERT(type->proto.get() == proto_.raw());

        










        if (type->hasNewScript() && type->newScript()->fun != fun_)
            type->clearAddendum(this);

        return type;
    }

    Rooted<TaggedProto> proto(this, proto_);
    RootedFunction fun(this, fun_);

    if (proto.isObject() && !proto.toObject()->setDelegate(this))
        return nullptr;

    bool markUnknown =
        proto.isObject()
        ? proto.toObject()->lastProperty()->hasObjectFlag(BaseShape::NEW_TYPE_UNKNOWN)
        : true;

    RootedTypeObject type(this, compartment_->types.newTypeObject(this, clasp, proto, markUnknown));
    if (!type)
        return nullptr;

    



    bool gcHappened = gcNumber() != originalGcNumber;
    bool added =
        gcHappened ? newTypeObjects.putNew(TypeObjectSet::Lookup(clasp, proto), type.get())
                   : newTypeObjects.relookupOrAdd(p, TypeObjectSet::Lookup(clasp, proto), type.get());
    if (!added)
        return nullptr;

#ifdef JSGC_GENERATIONAL
    if (proto.isObject() && hasNursery() && nursery().isInside(proto.toObject())) {
        asJSContext()->runtime()->gcStoreBuffer.putGeneric(
            NewTypeObjectsSetRef(&newTypeObjects, type.get(), proto.toObject()));
    }
#endif

    if (!typeInferenceEnabled())
        return type;

    AutoEnterAnalysis enter(this);

    




    if (proto.isObject()) {
        RootedObject obj(this, proto.toObject());

        if (fun)
            CheckNewScriptProperties(asJSContext(), type, fun);

        if (obj->is<RegExpObject>()) {
            AddTypeProperty(this, type, "source", types::Type::StringType());
            AddTypeProperty(this, type, "global", types::Type::BooleanType());
            AddTypeProperty(this, type, "ignoreCase", types::Type::BooleanType());
            AddTypeProperty(this, type, "multiline", types::Type::BooleanType());
            AddTypeProperty(this, type, "sticky", types::Type::BooleanType());
            AddTypeProperty(this, type, "lastIndex", types::Type::Int32Type());
        }

        if (obj->is<StringObject>())
            AddTypeProperty(this, type, "length", Type::Int32Type());
    }

    







    if (type->unknownProperties())
        type->flags |= OBJECT_FLAG_SETS_MARKED_UNKNOWN;

    return type;
}

TypeObject *
ExclusiveContext::getLazyType(const Class *clasp, TaggedProto proto)
{
    JS_ASSERT_IF(proto.isObject(), compartment() == proto.toObject()->compartment());

    AutoEnterAnalysis enter(this);

    TypeObjectSet &table = compartment()->lazyTypeObjects;

    if (!table.initialized() && !table.init())
        return nullptr;

    TypeObjectSet::AddPtr p = table.lookupForAdd(TypeObjectSet::Lookup(clasp, proto));
    if (p) {
        TypeObject *type = *p;
        JS_ASSERT(type->lazy());

        return type;
    }

    Rooted<TaggedProto> protoRoot(this, proto);
    TypeObject *type = compartment()->types.newTypeObject(this, clasp, protoRoot, false);
    if (!type)
        return nullptr;

    if (!table.relookupOrAdd(p, TypeObjectSet::Lookup(clasp, protoRoot), type))
        return nullptr;

    type->singleton = (JSObject *) TypeObject::LAZY_SINGLETON;

    return type;
}





void
TypeSet::sweep(Zone *zone)
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
                if (pentry)
                    *pentry = object;
                else
                    zone->types.setPendingNukeTypes();
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

    
    constraintList = nullptr;
}

inline void
TypeObject::clearProperties()
{
    setBasePropertyCount(0);
    propertySet = nullptr;
}








inline void
TypeObject::sweep(FreeOp *fop)
{
    if (singleton) {
        JS_ASSERT(!hasNewScript());

        



        clearProperties();

        return;
    }

    if (!isMarked()) {
        if (addendum)
            fop->free_(addendum);
        return;
    }

    js::LifoAlloc &typeLifoAlloc = zone()->types.typeLifoAlloc;

    



    unsigned propertyCount = basePropertyCount();
    if (propertyCount >= 2) {
        unsigned oldCapacity = HashSetCapacity(propertyCount);
        Property **oldArray = propertySet;

        clearProperties();
        propertyCount = 0;
        for (unsigned i = 0; i < oldCapacity; i++) {
            Property *prop = oldArray[i];
            if (prop) {
                Property *newProp = typeLifoAlloc.new_<Property>(*prop);
                if (newProp) {
                    Property **pentry =
                        HashSetInsert<jsid,Property,Property>
                            (typeLifoAlloc, propertySet, propertyCount, prop->id);
                    if (pentry) {
                        *pentry = newProp;
                        newProp->types.sweep(zone());
                    } else {
                        zone()->types.setPendingNukeTypes();
                    }
                } else {
                    zone()->types.setPendingNukeTypes();
                }
            }
        }
        setBasePropertyCount(propertyCount);
    } else if (propertyCount == 1) {
        Property *prop = (Property *) propertySet;
        Property *newProp = typeLifoAlloc.new_<Property>(*prop);
        if (newProp) {
            propertySet = (Property **) newProp;
            newProp->types.sweep(zone());
        } else {
            zone()->types.setPendingNukeTypes();
        }
    }

    if (basePropertyCount() <= SET_ARRAY_SIZE) {
        for (unsigned i = 0; i < basePropertyCount(); i++)
            JS_ASSERT(propertySet[i]);
    }

    




    if (hasNewScript())
        flags |= OBJECT_FLAG_NEW_SCRIPT_REGENERATE;
}

void
TypeCompartment::sweep(FreeOp *fop)
{
    




    if (arrayTypeTable) {
        for (ArrayTypeTable::Enum e(*arrayTypeTable); !e.empty(); e.popFront()) {
            const ArrayTableKey &key = e.front().key;
            JS_ASSERT(key.type.isUnknown() || !key.type.isSingleObject());

            bool remove = false;
            TypeObject *typeObject = nullptr;
            if (!key.type.isUnknown() && key.type.isTypeObject()) {
                typeObject = key.type.typeObject();
                if (IsTypeObjectAboutToBeFinalized(&typeObject))
                    remove = true;
            }
            if (IsTypeObjectAboutToBeFinalized(e.front().value.unsafeGet()))
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
            const ObjectTableKey &key = e.front().key;
            ObjectTableEntry &entry = e.front().value;

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
            AllocationSiteKey key = e.front().key;
            bool keyDying = IsScriptAboutToBeFinalized(&key.script);
            bool valDying = IsTypeObjectAboutToBeFinalized(e.front().value.unsafeGet());
            if (keyDying || valDying)
                e.removeFront();
            else if (key.script != e.front().key.script)
                e.rekeyFront(key);
        }
    }

    



    if (pendingArray)
        fop->free_(pendingArray);

    pendingArray = nullptr;
    pendingCapacity = 0;
}

void
TypeCompartment::sweepShapes(FreeOp *fop)
{
    



    if (objectTypeTable) {
        for (ObjectTypeTable::Enum e(*objectTypeTable); !e.empty(); e.popFront()) {
            const ObjectTableKey &key = e.front().key;
            ObjectTableEntry &entry = e.front().value;

            if (IsShapeAboutToBeFinalized(entry.shape.unsafeGet())) {
                fop->free_(key.properties);
                fop->free_(entry.types);
                e.removeFront();
            }
        }
    }
}

void
TypeCompartment::clearCompilerOutputs(FreeOp *fop)
{
    if (constrainedOutputs) {
        fop->delete_(constrainedOutputs);
        constrainedOutputs = nullptr;
    }

    if (pendingRecompiles) {
        JS_ASSERT(pendingRecompiles->length() == 0);
        fop->delete_(pendingRecompiles);
        pendingRecompiles = nullptr;
    }
}

void
JSCompartment::sweepNewTypeObjectTable(TypeObjectSet &table)
{
    gcstats::AutoPhase ap(runtimeFromMainThread()->gcStats,
                          gcstats::PHASE_SWEEP_TABLES_TYPE_OBJECT);

    JS_ASSERT(zone()->isGCSweeping());
    if (table.initialized()) {
        for (TypeObjectSet::Enum e(table); !e.empty(); e.popFront()) {
            TypeObject *type = e.front();
            if (IsTypeObjectAboutToBeFinalized(&type))
                e.removeFront();
            else if (type != e.front())
                e.rekeyFront(TypeObjectSet::Lookup(type->clasp, type->proto.get()), type);
        }
    }
}

TypeCompartment::~TypeCompartment()
{
    js_free(pendingArray);
    js_delete(arrayTypeTable);
    js_delete(objectTypeTable);
    js_delete(allocationSiteTable);
}

 void
TypeScript::Sweep(FreeOp *fop, JSScript *script)
{
    JSCompartment *compartment = script->compartment();
    JS_ASSERT(compartment->zone()->isGCSweeping());
    JS_ASSERT(compartment->zone()->types.inferenceEnabled);

    unsigned num = NumTypeSets(script);
    TypeSet *typeArray = script->types->typeArray();

    
    for (unsigned i = 0; i < num; i++)
        typeArray[i].sweep(compartment->zone());

    



    script->hasFreezeConstraints = false;
}

void
TypeScript::destroy()
{
    js_free(this);
}

void
Zone::addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf, size_t *typePool)
{
    *typePool += types.typeLifoAlloc.sizeOfExcludingThis(mallocSizeOf);
}

void
TypeCompartment::addSizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf,
                                        size_t *pendingArrays,
                                        size_t *allocationSiteTables,
                                        size_t *arrayTypeTables,
                                        size_t *objectTypeTables)
{
    
    *pendingArrays += mallocSizeOf(pendingArray);

    



    JS_ASSERT(!pendingRecompiles);

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
            const ObjectTableKey &key = e.front().key;
            const ObjectTableEntry &value = e.front().value;

            
            *objectTypeTables += mallocSizeOf(key.properties) + mallocSizeOf(value.types);
        }
    }
}

size_t
TypeObject::sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
    if (singleton) {
        




        JS_ASSERT(!hasNewScript());
        return 0;
    }

    return mallocSizeOf(addendum);
}

TypeZone::TypeZone(Zone *zone)
  : zone_(zone),
    typeLifoAlloc(TYPE_LIFO_ALLOC_PRIMARY_CHUNK_SIZE),
    pendingNukeTypes(false),
    inferenceEnabled(false)
{
}

TypeZone::~TypeZone()
{
}

void
TypeZone::sweep(FreeOp *fop, bool releaseTypes)
{
    JS_ASSERT(zone()->isGCSweeping());

    JSRuntime *rt = fop->runtime();

    



    LifoAlloc oldAlloc(typeLifoAlloc.defaultChunkSize());
    oldAlloc.steal(&typeLifoAlloc);

    




    if (inferenceEnabled) {
        gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_DISCARD_TI);

        for (CellIterUnderGC i(zone(), FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            if (script->types) {
                types::TypeScript::Sweep(fop, script);

                if (releaseTypes) {
                    script->types->destroy();
                    script->types = nullptr;
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
            object->sweep(fop);
        }

        for (CompartmentsInZoneIter comp(zone()); !comp.done(); comp.next())
            comp->types.sweep(fop);
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

#ifdef DEBUG
void
TypeScript::printTypes(JSContext *cx, HandleScript script) const
{
    JS_ASSERT(script->types == this);

    if (!bytecodeMap)
        return;

    AutoEnterAnalysis enter(nullptr, script->compartment());

    if (script->function())
        fprintf(stderr, "Function");
    else if (script->isForEval())
        fprintf(stderr, "Eval");
    else
        fprintf(stderr, "Main");
    fprintf(stderr, " #%u %s:%d ", script->id(), script->filename(), script->lineno);

    if (script->function()) {
        if (js::PropertyName *name = script->function()->name()) {
            const jschar *chars = name->getChars(nullptr);
            JSString::dumpChars(chars, name->length());
        }
    }

    fprintf(stderr, "\n    this:");
    TypeScript::ThisTypes(script)->print();

    for (unsigned i = 0; script->function() && i < script->function()->nargs; i++) {
        fprintf(stderr, "\n    arg%u:", i);
        TypeScript::ArgTypes(script, i)->print();
    }
    fprintf(stderr, "\n");

    for (jsbytecode *pc = script->code;
         pc < script->code + script->length;
         pc += GetBytecodeLength(pc))
    {
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





bool
TypeObject::addTypedObjectAddendum(JSContext *cx, TypeRepresentation *repr)
{
    if (!cx->typeInferenceEnabled())
        return true;

    JS_ASSERT(repr);

    if (flags & OBJECT_FLAG_ADDENDUM_CLEARED)
        return true;

    JS_ASSERT(!unknownProperties());

    if (addendum) {
        JS_ASSERT(hasTypedObject());
        JS_ASSERT(typedObject()->typeRepr == repr);
        return true;
    }

    TypeTypedObject *typedObject = js_new<TypeTypedObject>(repr);
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

TypeTypedObject::TypeTypedObject(TypeRepresentation *repr)
  : TypeObjectAddendum(TypedObject),
    typeRepr(repr)
{
}
