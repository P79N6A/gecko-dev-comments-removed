





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
#include "prmjtime.h"

#include "gc/Marking.h"
#include "jit/BaselineJIT.h"
#include "jit/CompileInfo.h"
#include "jit/Ion.h"
#include "jit/IonAnalysis.h"
#include "jit/JitCompartment.h"
#include "js/MemoryMetrics.h"
#include "vm/HelperThreads.h"
#include "vm/Opcodes.h"
#include "vm/Shape.h"
#include "vm/UnboxedObject.h"

#include "jsatominlines.h"
#include "jsgcinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "vm/NativeObject-inl.h"

using namespace js;
using namespace js::gc;
using namespace js::types;

using mozilla::DebugOnly;
using mozilla::Maybe;
using mozilla::PodArrayZero;
using mozilla::PodCopy;
using mozilla::PodZero;

static inline jsid
id_prototype(JSContext *cx)
{
    return NameToId(cx->names().prototype);
}

#ifdef DEBUG

static inline jsid
id___proto__(JSContext *cx)
{
    return NameToId(cx->names().proto);
}

static inline jsid
id_constructor(JSContext *cx)
{
    return NameToId(cx->names().constructor);
}

static inline jsid
id_caller(JSContext *cx)
{
    return NameToId(cx->names().caller);
}

const char *
types::TypeIdStringImpl(jsid id)
{
    if (JSID_IS_VOID(id))
        return "(index)";
    if (JSID_IS_EMPTY(id))
        return "(new)";
    if (JSID_IS_SYMBOL(id))
        return "(symbol)";
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
          case JSVAL_TYPE_SYMBOL:
            return "symbol";
          case JSVAL_TYPE_MAGIC:
            return "lazyargs";
          default:
            MOZ_CRASH("Bad type");
        }
    }
    if (type.isUnknown())
        return "unknown";
    if (type.isAnyObject())
        return " object";

    static char bufs[4][40];
    static unsigned which = 0;
    which = (which + 1) & 3;

    if (type.isSingleton())
        JS_snprintf(bufs[which], 40, "<0x%p>", (void *) type.singleton());
    else
        JS_snprintf(bufs[which], 40, "[0x%p]", (void *) type.group());

    return bufs[which];
}

const char *
types::ObjectGroupString(ObjectGroup *group)
{
    return TypeString(Type::ObjectType(group));
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
types::TypeHasProperty(JSContext *cx, ObjectGroup *group, jsid id, const Value &value)
{
    



    if (!group->unknownProperties() && !value.isUndefined()) {
        id = IdToTypeId(id);

        
        if (id == id___proto__(cx) || id == id_constructor(cx) || id == id_caller(cx))
            return true;

        Type type = GetValueType(value);

        
        
        if (value.isObject() &&
            !value.toObject().hasLazyGroup() &&
            value.toObject().group()->flags() & OBJECT_FLAG_UNKNOWN_PROPERTIES)
        {
            return true;
        }

        AutoEnterAnalysis enter(cx);

        




        TypeSet *types = group->maybeGetProperty(id);
        if (!types)
            return true;

        if (!types->hasType(type)) {
            TypeFailure(cx, "Missing type in object %s %s: %s",
                        ObjectGroupString(group), TypeIdString(id), TypeString(type));
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





TemporaryTypeSet::TemporaryTypeSet(LifoAlloc *alloc, Type type)
{
    if (type.isUnknown()) {
        flags |= TYPE_FLAG_BASE_MASK;
    } else if (type.isPrimitive()) {
        flags = PrimitiveTypeFlag(type.primitive());
        if (flags == TYPE_FLAG_DOUBLE)
            flags |= TYPE_FLAG_INT32;
    } else if (type.isAnyObject()) {
        flags |= TYPE_FLAG_ANYOBJECT;
    } else  if (type.isGroup() && type.group()->unknownProperties()) {
        flags |= TYPE_FLAG_ANYOBJECT;
    } else {
        setBaseObjectCount(1);
        objectSet = reinterpret_cast<TypeSetObjectKey**>(type.objectKey());

        if (type.isGroup()) {
            ObjectGroup *ngroup = type.group();
            if (ngroup->newScript() && ngroup->newScript()->initializedGroup())
                addType(Type::ObjectType(ngroup->newScript()->initializedGroup()), alloc);
        }
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
      case jit::MIRType_Symbol:
        return baseFlags() & TYPE_FLAG_SYMBOL;
      case jit::MIRType_MagicOptimizedArguments:
        return baseFlags() & TYPE_FLAG_LAZYARGS;
      case jit::MIRType_MagicHole:
      case jit::MIRType_MagicIsConstructing:
        
        
        
        
        
        
        
        
        
        return false;
      default:
        MOZ_CRASH("Bad MIR type");
    }
}

bool
TypeSet::objectsAreSubset(TypeSet *other)
{
    if (other->unknownObject())
        return true;

    if (unknownObject())
        return false;

    for (unsigned i = 0; i < getObjectCount(); i++) {
        TypeSetObjectKey *key = getObject(i);
        if (!key)
            continue;
        if (!other->hasType(Type::ObjectType(key)))
            return false;
    }

    return true;
}

bool
TypeSet::isSubset(const TypeSet *other) const
{
    if ((baseFlags() & other->baseFlags()) != baseFlags())
        return false;

    if (unknownObject()) {
        MOZ_ASSERT(other->unknownObject());
    } else {
        for (unsigned i = 0; i < getObjectCount(); i++) {
            TypeSetObjectKey *key = getObject(i);
            if (!key)
                continue;
            if (!other->hasType(Type::ObjectType(key)))
                return false;
        }
    }

    return true;
}

bool
TypeSet::enumerateTypes(TypeList *list) const
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
        TypeSetObjectKey *key = getObject(i);
        if (key) {
            if (!list->append(Type::ObjectType(key)))
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

    MOZ_ASSERT(cx->zone()->types.activeAnalysis);

    InferSpew(ISpewOps, "addConstraint: %sT%p%s %sC%p%s %s",
              InferSpewColor(this), this, InferSpewColorReset(),
              InferSpewColor(constraint), constraint, InferSpewColorReset(),
              constraint->kind());

    MOZ_ASSERT(constraint->next == nullptr);
    constraint->next = constraintList;
    constraintList = constraint;

    if (callExisting)
        return addTypesToConstraint(cx, constraint);
    return true;
}

void
TypeSet::clearObjects()
{
    setBaseObjectCount(0);
    objectSet = nullptr;
}

void
TypeSet::addType(Type type, LifoAlloc *alloc)
{
    if (unknown())
        return;

    if (type.isUnknown()) {
        flags |= TYPE_FLAG_BASE_MASK;
        clearObjects();
        MOZ_ASSERT(unknown());
        return;
    }

    if (type.isPrimitive()) {
        TypeFlags flag = PrimitiveTypeFlag(type.primitive());
        if (flags & flag)
            return;

        
        if (flag == TYPE_FLAG_DOUBLE)
            flag |= TYPE_FLAG_INT32;

        flags |= flag;
        return;
    }

    if (flags & TYPE_FLAG_ANYOBJECT)
        return;
    if (type.isAnyObject())
        goto unknownObject;

    {
        uint32_t objectCount = baseObjectCount();
        TypeSetObjectKey *key = type.objectKey();
        TypeSetObjectKey **pentry = HashSetInsert<TypeSetObjectKey *,TypeSetObjectKey,TypeSetObjectKey>
                                     (*alloc, objectSet, objectCount, key);
        if (!pentry)
            goto unknownObject;
        if (*pentry)
            return;
        *pentry = key;

        setBaseObjectCount(objectCount);

        
        
        
        
        if (objectCount >= TYPE_FLAG_OBJECT_COUNT_LIMIT) {
            JS_STATIC_ASSERT(TYPE_FLAG_DOMOBJECT_COUNT_LIMIT >= TYPE_FLAG_OBJECT_COUNT_LIMIT);
            
            
            if (objectCount == TYPE_FLAG_OBJECT_COUNT_LIMIT) {
                for (unsigned i = 0; i < objectCount; i++) {
                    const Class *clasp = getObjectClass(i);
                    if (clasp && !clasp->isDOMClass())
                        goto unknownObject;
                }
            }

            
            if (!key->clasp()->isDOMClass())
                goto unknownObject;

            
            if (objectCount == TYPE_FLAG_DOMOBJECT_COUNT_LIMIT)
                goto unknownObject;
        }
    }

    if (type.isGroup()) {
        ObjectGroup *ngroup = type.group();
        MOZ_ASSERT(!ngroup->singleton());
        if (ngroup->unknownProperties())
            goto unknownObject;

        
        
        
        if (ngroup->newScript() && ngroup->newScript()->initializedGroup())
            addType(Type::ObjectType(ngroup->newScript()->initializedGroup()), alloc);
    }

    if (false) {
    unknownObject:
        flags |= TYPE_FLAG_ANYOBJECT;
        clearObjects();
    }
}

void
ConstraintTypeSet::addType(ExclusiveContext *cxArg, Type type)
{
    MOZ_ASSERT(cxArg->zone()->types.activeAnalysis);

    if (hasType(type))
        return;

    TypeSet::addType(type, &cxArg->typeLifoAlloc());

    if (type.isObjectUnchecked() && unknownObject())
        type = Type::AnyObjectType();

    InferSpew(ISpewOps, "addType: %sT%p%s %s",
              InferSpewColor(this), this, InferSpewColorReset(),
              TypeString(type));

    
    if (JSContext *cx = cxArg->maybeJSContext()) {
        TypeConstraint *constraint = constraintList;
        while (constraint) {
            constraint->newType(cx, this, type);
            constraint = constraint->next;
        }
    } else {
        MOZ_ASSERT(!constraintList);
    }
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
    if (flags & TYPE_FLAG_SYMBOL)
        fprintf(stderr, " symbol");
    if (flags & TYPE_FLAG_LAZYARGS)
        fprintf(stderr, " lazyargs");

    uint32_t objectCount = baseObjectCount();
    if (objectCount) {
        fprintf(stderr, " object[%u]", objectCount);

        unsigned count = getObjectCount();
        for (unsigned i = 0; i < count; i++) {
            TypeSetObjectKey *key = getObject(i);
            if (key)
                fprintf(stderr, " %s", TypeString(Type::ObjectType(key)));
        }
    }
}

 void
TypeSet::readBarrier(const TypeSet *types)
{
    if (types->unknownObject())
        return;

    for (unsigned i = 0; i < types->getObjectCount(); i++) {
        if (TypeSetObjectKey *key = types->getObject(i)) {
            if (key->isSingleton())
                (void) key->singleton();
            else
                (void) key->group();
        }
    }
}

bool
TypeSet::clone(LifoAlloc *alloc, TemporaryTypeSet *result) const
{
    MOZ_ASSERT(result->empty());

    unsigned objectCount = baseObjectCount();
    unsigned capacity = (objectCount >= 2) ? HashSetCapacity(objectCount) : 0;

    TypeSetObjectKey **newSet;
    if (capacity) {
        newSet = alloc->newArray<TypeSetObjectKey*>(capacity);
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
TypeSet::cloneObjectsOnly(LifoAlloc *alloc)
{
    TemporaryTypeSet *res = clone(alloc);
    if (!res)
        return nullptr;

    res->flags &= ~TYPE_FLAG_BASE_MASK | TYPE_FLAG_ANYOBJECT;

    return res;
}

TemporaryTypeSet *
TypeSet::cloneWithoutObjects(LifoAlloc *alloc)
{
    TemporaryTypeSet *res = alloc->new_<TemporaryTypeSet>();
    if (!res)
        return nullptr;

    res->flags = flags & ~TYPE_FLAG_ANYOBJECT;
    res->setBaseObjectCount(0);
    return res;
}

 TemporaryTypeSet *
TypeSet::unionSets(TypeSet *a, TypeSet *b, LifoAlloc *alloc)
{
    TemporaryTypeSet *res = alloc->new_<TemporaryTypeSet>(a->baseFlags() | b->baseFlags(),
                                                          static_cast<TypeSetObjectKey**>(nullptr));
    if (!res)
        return nullptr;

    if (!res->unknownObject()) {
        for (size_t i = 0; i < a->getObjectCount() && !res->unknownObject(); i++) {
            if (TypeSetObjectKey *key = a->getObject(i))
                res->addType(Type::ObjectType(key), alloc);
        }
        for (size_t i = 0; i < b->getObjectCount() && !res->unknownObject(); i++) {
            if (TypeSetObjectKey *key = b->getObject(i))
                res->addType(Type::ObjectType(key), alloc);
        }
    }

    return res;
}

 TemporaryTypeSet *
TypeSet::intersectSets(TemporaryTypeSet *a, TemporaryTypeSet *b, LifoAlloc *alloc)
{
    TemporaryTypeSet *res;
    res = alloc->new_<TemporaryTypeSet>(a->baseFlags() & b->baseFlags(),
                static_cast<TypeSetObjectKey**>(nullptr));
    if (!res)
        return nullptr;

    res->setBaseObjectCount(0);
    if (res->unknownObject())
        return res;

    MOZ_ASSERT(!a->unknownObject() || !b->unknownObject());

    if (a->unknownObject()) {
        for (size_t i = 0; i < b->getObjectCount(); i++) {
            if (b->getObject(i))
                res->addType(Type::ObjectType(b->getObject(i)), alloc);
        }
        return res;
    }

    if (b->unknownObject()) {
        for (size_t i = 0; i < a->getObjectCount(); i++) {
            if (b->getObject(i))
                res->addType(Type::ObjectType(a->getObject(i)), alloc);
        }
        return res;
    }

    MOZ_ASSERT(!a->unknownObject() && !b->unknownObject());

    for (size_t i = 0; i < a->getObjectCount(); i++) {
        for (size_t j = 0; j < b->getObjectCount(); j++) {
            if (b->getObject(j) != a->getObject(i))
                continue;
            if (!b->getObject(j))
                continue;
            res->addType(Type::ObjectType(b->getObject(j)), alloc);
            break;
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

    
    LifoAlloc *alloc_;

    
    Vector<CompilerConstraint *, 0, jit::JitAllocPolicy> constraints;

    
    Vector<FrozenScript, 1, jit::JitAllocPolicy> frozenScripts;

  public:
    explicit CompilerConstraintList(jit::TempAllocator &alloc)
      : failed_(false),
        alloc_(alloc.lifoAlloc()),
        constraints(alloc),
        frozenScripts(alloc)
    {}

    void add(CompilerConstraint *constraint) {
        if (!constraint || !constraints.append(constraint))
            setFailed();
    }

    void freezeScript(JSScript *script,
                      TemporaryTypeSet *thisTypes,
                      TemporaryTypeSet *argTypes,
                      TemporaryTypeSet *bytecodeTypes)
    {
        FrozenScript entry;
        entry.script = script;
        entry.thisTypes = thisTypes;
        entry.argTypes = argTypes;
        entry.bytecodeTypes = bytecodeTypes;
        if (!frozenScripts.append(entry))
            setFailed();
    }

    size_t length() {
        return constraints.length();
    }

    CompilerConstraint *get(size_t i) {
        return constraints[i];
    }

    size_t numFrozenScripts() {
        return frozenScripts.length();
    }

    const FrozenScript &frozenScript(size_t i) {
        return frozenScripts[i];
    }

    bool failed() {
        return failed_;
    }
    void setFailed() {
        failed_ = true;
    }
    LifoAlloc *alloc() const {
        return alloc_;
    }
};

CompilerConstraintList *
types::NewCompilerConstraintList(jit::TempAllocator &alloc)
{
    return alloc.lifoAlloc()->new_<CompilerConstraintList>(alloc);
}

 bool
TypeScript::FreezeTypeSets(CompilerConstraintList *constraints, JSScript *script,
                           TemporaryTypeSet **pThisTypes,
                           TemporaryTypeSet **pArgTypes,
                           TemporaryTypeSet **pBytecodeTypes)
{
    LifoAlloc *alloc = constraints->alloc();
    StackTypeSet *existing = script->types()->typeArray();

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

    void newObjectState(JSContext *cx, ObjectGroup *group) {
        
        
        
        if (group->unknownProperties() || data.invalidateOnNewObjectState(group))
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
TypeSetObjectKey::clasp()
{
    return isGroup() ? group()->clasp() : singleton()->getClass();
}

TaggedProto
TypeSetObjectKey::proto()
{
    MOZ_ASSERT(hasTenuredProto());
    return isGroup() ? group()->proto() : singleton()->getTaggedProto();
}

TaggedProto
TypeSetObjectKey::protoMaybeInNursery()
{
    return isGroup() ? group()->proto() : singleton()->getTaggedProto();
}

bool
JSObject::hasTenuredProto() const
{
    return group_->hasTenuredProto();
}

bool
TypeSetObjectKey::hasTenuredProto()
{
    return isGroup() ? group()->hasTenuredProto() : singleton()->hasTenuredProto();
}

TypeNewScript *
TypeSetObjectKey::newScript()
{
    if (isGroup() && group()->newScript())
        return group()->newScript();
    return nullptr;
}

ObjectGroup *
TypeSetObjectKey::maybeGroup()
{
    if (isGroup())
        return group();
    if (!singleton()->hasLazyGroup())
        return singleton()->group();
    return nullptr;
}

bool
TypeSetObjectKey::unknownProperties()
{
    if (ObjectGroup *group = maybeGroup())
        return group->unknownProperties();
    return false;
}

HeapTypeSetKey
TypeSetObjectKey::property(jsid id)
{
    MOZ_ASSERT(!unknownProperties());

    HeapTypeSetKey property;
    property.object_ = this;
    property.id_ = id;
    if (ObjectGroup *group = maybeGroup())
        property.maybeTypes_ = group->maybeGetProperty(id);

    return property;
}

void
TypeSetObjectKey::ensureTrackedProperty(JSContext *cx, jsid id)
{
    
    
    
    if (!JSID_IS_VOID(id) && !JSID_IS_EMPTY(id)) {
        MOZ_ASSERT(CurrentThreadCanAccessRuntime(cx->runtime()));
        if (isSingleton()) {
            JSObject *obj = singleton();
            if (obj->isNative() && obj->as<NativeObject>().containsPure(id))
                EnsureTrackPropertyTypes(cx, obj, id);
        }
    }
}

bool
HeapTypeSetKey::instantiate(JSContext *cx)
{
    if (maybeTypes())
        return true;
    if (object()->isSingleton() && !object()->singleton()->getGroup(cx)) {
        cx->clearPendingException();
        return false;
    }
    maybeTypes_ = object()->maybeGroup()->getProperty(cx, id());
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
    explicit TypeConstraintFreezeStack(JSScript *script)
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
types::FinishCompilation(JSContext *cx, HandleScript script, CompilerConstraintList *constraints,
                         RecompileInfo *precompileInfo)
{
    if (constraints->failed())
        return false;

    CompilerOutput co(script);

    TypeZone &types = cx->zone()->types;
    if (!types.compilerOutputs) {
        types.compilerOutputs = cx->new_<TypeZone::CompilerOutputVector>();
        if (!types.compilerOutputs)
            return false;
    }

#ifdef DEBUG
    for (size_t i = 0; i < types.compilerOutputs->length(); i++) {
        const CompilerOutput &co = (*types.compilerOutputs)[i];
        MOZ_ASSERT_IF(co.isValid(), co.script() != script);
    }
#endif

    uint32_t index = types.compilerOutputs->length();
    if (!types.compilerOutputs->append(co)) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    *precompileInfo = RecompileInfo(index, types.generation);

    bool succeeded = true;

    for (size_t i = 0; i < constraints->length(); i++) {
        CompilerConstraint *constraint = constraints->get(i);
        if (!constraint->generateTypeConstraint(cx, *precompileInfo))
            succeeded = false;
    }

    for (size_t i = 0; i < constraints->numFrozenScripts(); i++) {
        const CompilerConstraintList::FrozenScript &entry = constraints->frozenScript(i);
        if (!entry.script->types()) {
            succeeded = false;
            break;
        }

        
        
        
        if (entry.script->isDebuggee()) {
            succeeded = false;
            break;
        }

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
            if (!CheckFrozenTypeSet(cx, &entry.bytecodeTypes[i], &entry.script->types()->typeArray()[i]))
                succeeded = false;
        }

        
        
        if (entry.script->hasFreezeConstraints())
            continue;
        entry.script->setHasFreezeConstraints();

        size_t count = TypeScript::NumTypeSets(entry.script);

        StackTypeSet *array = entry.script->types()->typeArray();
        for (size_t i = 0; i < count; i++) {
            if (!array[i].addConstraint(cx, cx->typeLifoAlloc().new_<TypeConstraintFreezeStack>(entry.script), false))
                succeeded = false;
        }
    }

    if (!succeeded || types.compilerOutputs->back().pendingInvalidation()) {
        types.compilerOutputs->back().invalidate();
        script->resetWarmUpCounter();
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
        MOZ_ASSERT(script->types());

        MOZ_ASSERT(TypeScript::ThisTypes(script)->isSubset(entry.thisTypes));

        unsigned nargs = entry.script->functionNonDelazifying()
                         ? entry.script->functionNonDelazifying()->nargs()
                         : 0;
        for (size_t j = 0; j < nargs; j++)
            MOZ_ASSERT(TypeScript::ArgTypes(script, j)->isSubset(&entry.argTypes[j]));

        for (size_t j = 0; j < script->nTypeSets(); j++)
            MOZ_ASSERT(script->types()->typeArray()[j].isSubset(&entry.bytecodeTypes[j]));
    }
#endif

    for (size_t i = 0; i < constraints->numFrozenScripts(); i++) {
        const CompilerConstraintList::FrozenScript &entry = constraints->frozenScript(i);
        JSScript *script = entry.script;
        if (!script->types())
            MOZ_CRASH();

        CheckDefinitePropertiesTypeSet(cx, entry.thisTypes, TypeScript::ThisTypes(script));

        unsigned nargs = script->functionNonDelazifying()
                         ? script->functionNonDelazifying()->nargs()
                         : 0;
        for (size_t j = 0; j < nargs; j++)
            CheckDefinitePropertiesTypeSet(cx, &entry.argTypes[j], TypeScript::ArgTypes(script, j));

        for (size_t j = 0; j < script->nTypeSets(); j++)
            CheckDefinitePropertiesTypeSet(cx, &entry.bytecodeTypes[j], &script->types()->typeArray()[j]);
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
    bool invalidateOnNewObjectState(ObjectGroup *group) { return false; }

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
      case TYPE_FLAG_SYMBOL:
        return jit::MIRType_Symbol;
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
    MOZ_ASSERT_IF(empty, type == jit::MIRType_Value);

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

    






    MOZ_ASSERT_IF(types->empty(), type == jit::MIRType_Value);

    return type;
}

bool
HeapTypeSetKey::isOwnProperty(CompilerConstraintList *constraints,
                              bool allowEmptyTypesForGlobal)
{
    if (maybeTypes() && (!maybeTypes()->empty() || maybeTypes()->nonDataProperty()))
        return true;
    if (object()->isSingleton()) {
        JSObject *obj = object()->singleton();
        MOZ_ASSERT(CanHaveEmptyPropertyTypesForOwnProperty(obj) == obj->is<GlobalObject>());
        if (!allowEmptyTypesForGlobal) {
            if (CanHaveEmptyPropertyTypesForOwnProperty(obj))
                return true;
        }
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
TemporaryTypeSet::maybeSingleton()
{
    if (baseFlags() != 0 || baseObjectCount() != 1)
        return nullptr;

    return getSingleton(0);
}

JSObject *
HeapTypeSetKey::singleton(CompilerConstraintList *constraints)
{
    HeapTypeSet *types = maybeTypes();

    if (!types || types->nonDataProperty() || types->baseFlags() != 0 || types->getObjectCount() != 1)
        return nullptr;

    JSObject *obj = types->getSingleton(0);

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
               || types->hasAnyFlag(TYPE_FLAG_STRING | TYPE_FLAG_SYMBOL);
    if (!result)
        freeze(constraints);
    return result;
}

namespace {


class ConstraintDataFreezeObjectFlags
{
  public:
    
    ObjectGroupFlags flags;

    explicit ConstraintDataFreezeObjectFlags(ObjectGroupFlags flags)
      : flags(flags)
    {
        MOZ_ASSERT(flags);
    }

    const char *kind() { return "freezeObjectFlags"; }

    bool invalidateOnNewType(Type type) { return false; }
    bool invalidateOnNewPropertyState(TypeSet *property) { return false; }
    bool invalidateOnNewObjectState(ObjectGroup *group) {
        return group->hasAnyFlags(flags);
    }

    bool constraintHolds(JSContext *cx,
                         const HeapTypeSetKey &property, TemporaryTypeSet *expected)
    {
        return !invalidateOnNewObjectState(property.object()->maybeGroup());
    }

    bool shouldSweep() { return false; }
};

} 

bool
TypeSetObjectKey::hasFlags(CompilerConstraintList *constraints, ObjectGroupFlags flags)
{
    MOZ_ASSERT(flags);

    if (ObjectGroup *group = maybeGroup()) {
        if (group->hasAnyFlags(flags))
            return true;
    }

    HeapTypeSetKey objectProperty = property(JSID_EMPTY);
    LifoAlloc *alloc = constraints->alloc();

    typedef CompilerConstraintInstance<ConstraintDataFreezeObjectFlags> T;
    constraints->add(alloc->new_<T>(alloc, objectProperty, ConstraintDataFreezeObjectFlags(flags)));
    return false;
}

bool
TypeSetObjectKey::hasStableClassAndProto(CompilerConstraintList *constraints)
{
    return !hasFlags(constraints, OBJECT_FLAG_UNKNOWN_PROPERTIES);
}

bool
TemporaryTypeSet::hasObjectFlags(CompilerConstraintList *constraints, ObjectGroupFlags flags)
{
    if (unknownObject())
        return true;

    



    if (baseObjectCount() == 0)
        return true;

    unsigned count = getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        TypeSetObjectKey *key = getObject(i);
        if (key && key->hasFlags(constraints, flags))
            return true;
    }

    return false;
}

gc::InitialHeap
ObjectGroup::initialHeap(CompilerConstraintList *constraints)
{
    
    
    

    if (shouldPreTenure())
        return gc::TenuredHeap;

    if (!canPreTenure())
        return gc::DefaultHeap;

    HeapTypeSetKey objectProperty = TypeSetObjectKey::get(this)->property(JSID_EMPTY);
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
    bool invalidateOnNewObjectState(ObjectGroup *group) {
        
        
        return true;
    }

    bool constraintHolds(JSContext *cx,
                         const HeapTypeSetKey &property, TemporaryTypeSet *expected)
    {
        return true;
    }

    bool shouldSweep() { return false; }
};



class ConstraintDataFreezeObjectForTypedArrayData
{
    void *viewData;
    uint32_t length;

  public:
    explicit ConstraintDataFreezeObjectForTypedArrayData(TypedArrayObject &tarray)
      : viewData(tarray.viewData()),
        length(tarray.length())
    {}

    const char *kind() { return "freezeObjectForTypedArrayData"; }

    bool invalidateOnNewType(Type type) { return false; }
    bool invalidateOnNewPropertyState(TypeSet *property) { return false; }
    bool invalidateOnNewObjectState(ObjectGroup *group) {
        TypedArrayObject &tarray = group->singleton()->as<TypedArrayObject>();
        return tarray.viewData() != viewData || tarray.length() != length;
    }

    bool constraintHolds(JSContext *cx,
                         const HeapTypeSetKey &property, TemporaryTypeSet *expected)
    {
        return !invalidateOnNewObjectState(property.object()->maybeGroup());
    }

    bool shouldSweep() {
        
        return false;
    }
};

} 

void
TypeSetObjectKey::watchStateChangeForInlinedCall(CompilerConstraintList *constraints)
{
    HeapTypeSetKey objectProperty = property(JSID_EMPTY);
    LifoAlloc *alloc = constraints->alloc();

    typedef CompilerConstraintInstance<ConstraintDataFreezeObjectForInlinedCall> T;
    constraints->add(alloc->new_<T>(alloc, objectProperty, ConstraintDataFreezeObjectForInlinedCall()));
}

void
TypeSetObjectKey::watchStateChangeForTypedArrayData(CompilerConstraintList *constraints)
{
    TypedArrayObject &tarray = singleton()->as<TypedArrayObject>();
    HeapTypeSetKey objectProperty = property(JSID_EMPTY);
    LifoAlloc *alloc = constraints->alloc();

    typedef CompilerConstraintInstance<ConstraintDataFreezeObjectForTypedArrayData> T;
    constraints->add(alloc->new_<T>(alloc, objectProperty,
                                    ConstraintDataFreezeObjectForTypedArrayData(tarray)));
}

static void
ObjectStateChange(ExclusiveContext *cxArg, ObjectGroup *group, bool markingUnknown)
{
    if (group->unknownProperties())
        return;

    
    HeapTypeSet *types = group->maybeGetProperty(JSID_EMPTY);

    
    if (markingUnknown)
        group->addFlags(OBJECT_FLAG_DYNAMIC_MASK | OBJECT_FLAG_UNKNOWN_PROPERTIES);

    if (types) {
        if (JSContext *cx = cxArg->maybeJSContext()) {
            TypeConstraint *constraint = types->constraintList;
            while (constraint) {
                constraint->newObjectState(cx, group);
                constraint = constraint->next;
            }
        } else {
            MOZ_ASSERT(!types->constraintList);
        }
    }
}

namespace {

class ConstraintDataFreezePropertyState
{
  public:
    enum Which {
        NON_DATA,
        NON_WRITABLE
    } which;

    explicit ConstraintDataFreezePropertyState(Which which)
      : which(which)
    {}

    const char *kind() { return (which == NON_DATA) ? "freezeNonDataProperty" : "freezeNonWritableProperty"; }

    bool invalidateOnNewType(Type type) { return false; }
    bool invalidateOnNewPropertyState(TypeSet *property) {
        return (which == NON_DATA)
               ? property->nonDataProperty()
               : property->nonWritableProperty();
    }
    bool invalidateOnNewObjectState(ObjectGroup *group) { return false; }

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

namespace {

class ConstraintDataConstantProperty
{
  public:
    explicit ConstraintDataConstantProperty() {}

    const char *kind() { return "constantProperty"; }

    bool invalidateOnNewType(Type type) { return false; }
    bool invalidateOnNewPropertyState(TypeSet *property) {
        return property->nonConstantProperty();
    }
    bool invalidateOnNewObjectState(ObjectGroup *group) { return false; }

    bool constraintHolds(JSContext *cx,
                         const HeapTypeSetKey &property, TemporaryTypeSet *expected)
    {
        return !invalidateOnNewPropertyState(property.maybeTypes());
    }

    bool shouldSweep() { return false; }
};

} 

bool
HeapTypeSetKey::constant(CompilerConstraintList *constraints, Value *valOut)
{
    if (nonData(constraints))
        return false;

    
    JSObject *obj = object()->singleton();
    if (!obj || !obj->isNative())
        return false;

    if (maybeTypes() && maybeTypes()->nonConstantProperty())
        return false;

    
    Shape *shape = obj->as<NativeObject>().lookupPure(id());
    if (!shape || !shape->hasDefaultGetter() || !shape->hasSlot() || shape->hadOverwrite())
        return false;

    Value val = obj->as<NativeObject>().getSlot(shape->slot());

    
    if (val.isGCThing() && IsInsideNursery(val.toGCThing()))
        return false;

    
    if (val.isString() && !val.toString()->isAtom())
        return false;

    *valOut = val;

    LifoAlloc *alloc = constraints->alloc();
    typedef CompilerConstraintInstance<ConstraintDataConstantProperty> T;
    constraints->add(alloc->new_<T>(alloc, *this, ConstraintDataConstantProperty()));
    return true;
}


class ConstraintDataInert
{
  public:
    explicit ConstraintDataInert() {}

    const char *kind() { return "inert"; }

    bool invalidateOnNewType(Type type) { return false; }
    bool invalidateOnNewPropertyState(TypeSet *property) { return false; }
    bool invalidateOnNewObjectState(ObjectGroup *group) { return false; }

    bool constraintHolds(JSContext *cx,
                         const HeapTypeSetKey &property, TemporaryTypeSet *expected)
    {
        return true;
    }

    bool shouldSweep() { return false; }
};

bool
HeapTypeSetKey::couldBeConstant(CompilerConstraintList *constraints)
{
    
    if (!object()->isSingleton())
        return false;

    if (!maybeTypes() || !maybeTypes()->nonConstantProperty())
        return true;

    
    
    
    
    

    LifoAlloc *alloc = constraints->alloc();
    typedef CompilerConstraintInstance<ConstraintDataInert> T;
    constraints->add(alloc->new_<T>(alloc, *this, ConstraintDataInert()));

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
        TypeSetObjectKey *key = other->getObject(i);
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
        TypeSetObjectKey *key = getObject(i);
        if (!key)
            continue;

        if (key->unknownProperties()) {
            alwaysConvert = false;
            continue;
        }

        HeapTypeSetKey property = key->property(JSID_VOID);
        property.freeze(constraints);

        
        
        
        
        if (!property.maybeTypes() ||
            !property.maybeTypes()->hasType(Type::DoubleType()) ||
            key->clasp() != &ArrayObject::class_)
        {
            dontConvert = true;
            alwaysConvert = false;
            continue;
        }

        
        
        
        if (property.knownMIRType(constraints) == jit::MIRType_Double &&
            !key->hasFlags(constraints, OBJECT_FLAG_NON_PACKED))
        {
            maybeConvert = true;
        } else {
            alwaysConvert = false;
        }
    }

    MOZ_ASSERT_IF(alwaysConvert, maybeConvert);

    if (maybeConvert && dontConvert)
        return AmbiguousDoubleConversion;
    if (alwaysConvert)
        return AlwaysConvertToDoubles;
    if (maybeConvert)
        return MaybeConvertToDoubles;
    return DontConvertToDoubles;
}

const Class *
TemporaryTypeSet::getKnownClass(CompilerConstraintList *constraints)
{
    if (unknownObject())
        return nullptr;

    const Class *clasp = nullptr;
    unsigned count = getObjectCount();

    for (unsigned i = 0; i < count; i++) {
        const Class *nclasp = getObjectClass(i);
        if (!nclasp)
            continue;

        if (getObject(i)->unknownProperties())
            return nullptr;

        if (clasp && clasp != nclasp)
            return nullptr;
        clasp = nclasp;
    }

    if (clasp) {
        for (unsigned i = 0; i < count; i++) {
            TypeSetObjectKey *key = getObject(i);
            if (key && !key->hasStableClassAndProto(constraints))
                return nullptr;
        }
    }

    return clasp;
}

TemporaryTypeSet::ForAllResult
TemporaryTypeSet::forAllClasses(CompilerConstraintList *constraints,
                                bool (*func)(const Class* clasp))
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
            continue;
        if (!getObject(i)->hasStableClassAndProto(constraints))
            return ForAllResult::MIXED;
        if (func(clasp)) {
            true_results = true;
            if (false_results)
                return ForAllResult::MIXED;
        }
        else {
            false_results = true;
            if (true_results)
                return ForAllResult::MIXED;
        }
    }

    MOZ_ASSERT(true_results != false_results);

    return true_results ? ForAllResult::ALL_TRUE : ForAllResult::ALL_FALSE;
}

Scalar::Type
TemporaryTypeSet::getTypedArrayType(CompilerConstraintList *constraints)
{
    const Class *clasp = getKnownClass(constraints);

    if (clasp && IsTypedArrayClass(clasp))
        return (Scalar::Type) (clasp - &TypedArrayObject::classes[0]);
    return Scalar::MaxTypedArrayViewType;
}

Scalar::Type
TemporaryTypeSet::getSharedTypedArrayType(CompilerConstraintList *constraints)
{
    const Class *clasp = getKnownClass(constraints);

    if (clasp && IsSharedTypedArrayClass(clasp))
        return (Scalar::Type) (clasp - &SharedTypedArrayObject::classes[0]);
    return Scalar::MaxTypedArrayViewType;
}

bool
TemporaryTypeSet::isDOMClass(CompilerConstraintList *constraints)
{
    if (unknownObject())
        return false;

    unsigned count = getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        const Class *clasp = getObjectClass(i);
        if (!clasp)
            continue;
        if (!clasp->isDOMClass() || !getObject(i)->hasStableClassAndProto(constraints))
            return false;
    }

    return count > 0;
}

bool
TemporaryTypeSet::maybeCallable(CompilerConstraintList *constraints)
{
    if (!maybeObject())
        return false;

    if (unknownObject())
        return true;

    unsigned count = getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        const Class *clasp = getObjectClass(i);
        if (!clasp)
            continue;
        if (clasp->isProxy() || clasp->nonProxyCallable())
            return true;
        if (!getObject(i)->hasStableClassAndProto(constraints))
            return true;
    }

    return false;
}

bool
TemporaryTypeSet::maybeEmulatesUndefined(CompilerConstraintList *constraints)
{
    if (!maybeObject())
        return false;

    if (unknownObject())
        return true;

    unsigned count = getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        
        
        
        const Class *clasp = getObjectClass(i);
        if (!clasp)
            continue;
        if (clasp->emulatesUndefined() || clasp->isProxy())
            return true;
        if (!getObject(i)->hasStableClassAndProto(constraints))
            return true;
    }

    return false;
}

JSObject *
TemporaryTypeSet::getCommonPrototype(CompilerConstraintList *constraints)
{
    if (unknownObject())
        return nullptr;

    JSObject *proto = nullptr;
    unsigned count = getObjectCount();

    for (unsigned i = 0; i < count; i++) {
        TypeSetObjectKey *key = getObject(i);
        if (!key)
            continue;

        if (key->unknownProperties() || !key->hasTenuredProto())
            return nullptr;

        TaggedProto nproto = key->proto();
        if (proto) {
            if (nproto != TaggedProto(proto))
                return nullptr;
        } else {
            if (!nproto.isObject())
                return nullptr;
            proto = nproto.toObject();
        }
    }

    
    for (unsigned i = 0; i < count; i++) {
        TypeSetObjectKey *key = getObject(i);
        if (key)
            JS_ALWAYS_TRUE(key->hasStableClassAndProto(constraints));
    }

    return proto;
}

bool
TemporaryTypeSet::propertyNeedsBarrier(CompilerConstraintList *constraints, jsid id)
{
    if (unknownObject())
        return true;

    for (unsigned i = 0; i < getObjectCount(); i++) {
        TypeSetObjectKey *key = getObject(i);
        if (!key)
            continue;

        if (key->unknownProperties())
            return true;

        HeapTypeSetKey property = key->property(id);
        if (property.needsBarrier(constraints))
            return true;
    }

    return false;
}





TypeCompartment::TypeCompartment()
{
    PodZero(this);
}

ObjectGroup *
TypeCompartment::newObjectGroup(ExclusiveContext *cx, const Class *clasp, Handle<TaggedProto> proto,
                                ObjectGroupFlags initialFlags)
{
    MOZ_ASSERT_IF(proto.isObject(), cx->isInsideCurrentCompartment(proto.toObject()));

    if (cx->isJSContext()) {
        if (proto.isObject() && IsInsideNursery(proto.toObject()))
            initialFlags |= OBJECT_FLAG_NURSERY_PROTO;
    }

    ObjectGroup *group = js::NewObjectGroup(cx);
    if (!group)
        return nullptr;
    new(group) ObjectGroup(clasp, proto, initialFlags);

    return group;
}

ObjectGroup *
TypeCompartment::addAllocationSiteObjectGroup(JSContext *cx, AllocationSiteKey key)
{
    AutoEnterAnalysis enter(cx);

    if (!allocationSiteTable) {
        allocationSiteTable = cx->new_<AllocationSiteTable>();
        if (!allocationSiteTable || !allocationSiteTable->init()) {
            js_delete(allocationSiteTable);
            allocationSiteTable = nullptr;
            return nullptr;
        }
    }

    AllocationSiteTable::AddPtr p = allocationSiteTable->lookupForAdd(key);
    MOZ_ASSERT(!p);

    ObjectGroup *res = nullptr;

    jsbytecode *pc = key.script->offsetToPC(key.offset);
    RootedScript keyScript(cx, key.script);

    if (!res) {
        RootedObject proto(cx);
        if (key.kind != JSProto_Null && !GetBuiltinPrototype(cx, key.kind, &proto))
            return nullptr;

        Rooted<TaggedProto> tagged(cx, TaggedProto(proto));
        res = newObjectGroup(cx, GetClassForProtoKey(key.kind), tagged, OBJECT_FLAG_FROM_ALLOCATION_SITE);
        if (!res)
            return nullptr;
        key.script = keyScript;
    }

    if (JSOp(*pc) == JSOP_NEWOBJECT) {
        




        RootedObject baseobj(cx, key.script->getObject(GET_UINT32_INDEX(pc)));

        if (!res->addDefiniteProperties(cx, baseobj->lastProperty()))
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
types::UseSingletonForNewObject(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    
















    if (script->isGenerator())
        return false;
    if (JSOp(*pc) != JSOP_NEW)
        return false;
    pc += JSOP_NEW_LENGTH;
    if (JSOp(*pc) == JSOP_SETPROP) {
        jsid id = GetAtomId(cx, script, pc, 0);
        if (id == id_prototype(cx))
            return true;
    }

    return false;
}

NewObjectKind
types::UseSingletonForInitializer(JSScript *script, jsbytecode *pc, JSProtoKey key)
{
    
    
    JS_STATIC_ASSERT(GenericObject == 0);

    





    if (script->functionNonDelazifying() && !script->treatAsRunOnce())
        return GenericObject;

    if (key != JSProto_Object &&
        !(key >= JSProto_Int8Array && key <= JSProto_Uint8ClampedArray) &&
        !(key >= JSProto_SharedInt8Array && key <= JSProto_SharedUint8ClampedArray))
    {
        return GenericObject;
    }

    




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
types::UseSingletonForInitializer(JSScript *script, jsbytecode *pc, const Class *clasp)
{
    return UseSingletonForInitializer(script, pc, JSCLASS_CACHED_PROTO_KEY(clasp));
}

static inline bool
ClassCanHaveExtraProperties(const Class *clasp)
{
    return clasp->resolve
        || clasp->ops.lookupProperty
        || clasp->ops.getProperty
        || IsAnyTypedArrayClass(clasp);
}

static inline bool
PrototypeHasIndexedProperty(CompilerConstraintList *constraints, JSObject *obj)
{
    do {
        TypeSetObjectKey *key = TypeSetObjectKey::get(obj);
        if (ClassCanHaveExtraProperties(key->clasp()))
            return true;
        if (key->unknownProperties())
            return true;
        HeapTypeSetKey index = key->property(JSID_VOID);
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
    const Class *clasp = types->getKnownClass(constraints);

    
    
    
    if (!clasp || (ClassCanHaveExtraProperties(clasp) && !IsAnyTypedArrayClass(clasp)))
        return true;

    if (types->hasObjectFlags(constraints, types::OBJECT_FLAG_SPARSE_INDEXES))
        return true;

    JSObject *proto = types->getCommonPrototype(constraints);
    if (!proto)
        return true;

    return PrototypeHasIndexedProperty(constraints, proto);
}

void
TypeZone::processPendingRecompiles(FreeOp *fop, RecompileInfoVector &recompiles)
{
    MOZ_ASSERT(!recompiles.empty());

    



    RecompileInfoVector pending;
    for (size_t i = 0; i < recompiles.length(); i++) {
        if (!pending.append(recompiles[i]))
            CrashAtUnhandlableOOM("processPendingRecompiles");
    }
    recompiles.clear();

    jit::Invalidate(*this, fop, pending);

    MOZ_ASSERT(recompiles.empty());
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

    if (!cx->zone()->types.activeAnalysis->pendingRecompiles.append(info))
        CrashAtUnhandlableOOM("Could not update pendingRecompiles");
}

void
TypeZone::addPendingRecompile(JSContext *cx, JSScript *script)
{
    MOZ_ASSERT(script);

    CancelOffThreadIonCompile(cx->compartment(), script);

    
    if (jit::IsBaselineEnabled(cx))
        script->resetWarmUpCounter();

    if (script->hasIonScript())
        addPendingRecompile(cx, script->ionScript()->recompileInfo());

    
    
    
    if (script->functionNonDelazifying() && !script->functionNonDelazifying()->hasLazyGroup())
        ObjectStateChange(cx, script->functionNonDelazifying()->group(), false);
}

void
TypeCompartment::print(JSContext *cx, bool force)
{
#ifdef DEBUG
    gc::AutoSuppressGC suppressGC(cx);
    JSAutoRequest request(cx);

    Zone *zone = compartment()->zone();
    AutoEnterAnalysis enter(nullptr, zone);

    if (!force && !InferSpewActive(ISpewResult))
        return;

    for (gc::ZoneCellIter i(zone, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        RootedScript script(cx, i.get<JSScript>());
        if (script->types())
            script->types()->printTypes(cx, script);
    }

    for (gc::ZoneCellIter i(zone, gc::FINALIZE_OBJECT_GROUP); !i.done(); i.next()) {
        ObjectGroup *group = i.get<ObjectGroup>();
        group->print();
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
    MOZ_ASSERT(!type.isSingleton());
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

    bool operator==(const ArrayTableKey& other) {
        return type == other.type && proto == other.proto;
    }

    bool operator!=(const ArrayTableKey& other) {
        return !(*this == other);
    }
};

void
TypeCompartment::setTypeToHomogenousArray(ExclusiveContext *cx,
                                          JSObject *obj, Type elementType)
{
    MOZ_ASSERT(cx->zone()->types.activeAnalysis);

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
        obj->setGroup(p->value());
    } else {
        
        RootedObject objProto(cx, obj->getProto());
        Rooted<TaggedProto> taggedProto(cx, TaggedProto(objProto));
        ObjectGroup *group = newObjectGroup(cx, &ArrayObject::class_, taggedProto);
        if (!group)
            return;
        obj->setGroup(group);

        AddTypePropertyId(cx, group, JSID_VOID, elementType);

        key.proto = objProto;
        (void) p.add(cx, *arrayTypeTable, key, group);
    }
}

void
TypeCompartment::fixArrayGroup(ExclusiveContext *cx, ArrayObject *obj)
{
    AutoEnterAnalysis enter(cx);

    






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
types::FixRestArgumentsType(ExclusiveContext *cx, ArrayObject *obj)
{
    cx->compartment()->types.fixRestArgumentsType(cx, obj);
}

void
TypeCompartment::fixRestArgumentsType(ExclusiveContext *cx, ArrayObject *obj)
{
    AutoEnterAnalysis enter(cx);

    



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
    ReadBarrieredObjectGroup group;
    ReadBarrieredShape shape;
    Type *types;
};

static inline void
UpdateObjectTableEntryTypes(ExclusiveContext *cx, ObjectTableEntry &entry,
                            IdValuePair *properties, size_t nproperties)
{
    if (entry.group->unknownProperties())
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
            AddTypePropertyId(cx, entry.group, IdToTypeId(properties[i].id), ntype);
        }
    }
}

void
TypeCompartment::fixObjectGroup(ExclusiveContext *cx, PlainObject *obj)
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
        MOZ_ASSERT(obj->getProto() == p->value().group->proto().toObject());
        MOZ_ASSERT(obj->lastProperty() == p->value().shape);

        UpdateObjectTableEntryTypes(cx, p->value(), properties.begin(), properties.length());
        obj->setGroup(p->value().group);
        return;
    }

    
    Rooted<TaggedProto> objProto(cx, obj->getTaggedProto());
    ObjectGroup *group = newObjectGroup(cx, &PlainObject::class_, objProto);
    if (!group || !group->addDefiniteProperties(cx, obj->lastProperty()))
        return;

    if (obj->isIndexed())
        group->setFlags(cx, OBJECT_FLAG_SPARSE_INDEXES);

    ScopedJSFreePtr<jsid> ids(group->zone()->pod_calloc<jsid>(properties.length()));
    if (!ids)
        return;

    ScopedJSFreePtr<Type> types(group->zone()->pod_calloc<Type>(properties.length()));
    if (!types)
        return;

    for (size_t i = 0; i < properties.length(); i++) {
        ids[i] = properties[i].id;
        types[i] = GetValueTypeForTable(obj->getSlot(i));
        AddTypePropertyId(cx, group, IdToTypeId(ids[i]), types[i]);
    }

    ObjectTableKey key;
    key.properties = ids;
    key.nproperties = properties.length();
    key.nfixed = obj->numFixedSlots();
    MOZ_ASSERT(ObjectTableKey::match(key, lookup));

    ObjectTableEntry entry;
    entry.group.set(group);
    entry.shape.set(obj->lastProperty());
    entry.types = types;

    obj->setGroup(group);

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
    size_t nfixed = gc::GetGCKindSlots(allocKind, &PlainObject::class_);

    ObjectTableKey::Lookup lookup(properties, nproperties, nfixed);
    ObjectTypeTable::AddPtr p = objectTypeTable->lookupForAdd(lookup);

    if (!p)
        return nullptr;

    RootedPlainObject obj(cx, NewBuiltinClassInstance<PlainObject>(cx, allocKind));
    if (!obj) {
        cx->clearPendingException();
        return nullptr;
    }
    MOZ_ASSERT(obj->getProto() == p->value().group->proto().toObject());

    RootedShape shape(cx, p->value().shape);
    if (!NativeObject::setLastProperty(cx, obj, shape)) {
        cx->clearPendingException();
        return nullptr;
    }

    UpdateObjectTableEntryTypes(cx, p->value(), properties, nproperties);

    for (size_t i = 0; i < nproperties; i++)
        obj->setSlot(i, properties[i].value);

    obj->setGroup(p->value().group);
    return obj;
}





void
ObjectGroup::setProto(JSContext *cx, TaggedProto proto)
{
    MOZ_ASSERT(singleton());

    if (proto.isObject() && IsInsideNursery(proto.toObject()))
        addFlags(OBJECT_FLAG_NURSERY_PROTO);

    setProtoUnchecked(proto);
}

static inline void
UpdatePropertyType(ExclusiveContext *cx, HeapTypeSet *types, NativeObject *obj, Shape *shape,
                   bool indexed)
{
    MOZ_ASSERT(obj->isSingleton() && !obj->hasLazyGroup());

    if (!shape->writable())
        types->setNonWritableProperty(cx);

    if (shape->hasGetterValue() || shape->hasSetterValue()) {
        types->setNonDataProperty(cx);
        types->TypeSet::addType(Type::UnknownType(), &cx->typeLifoAlloc());
    } else if (shape->hasDefaultGetter() && shape->hasSlot()) {
        if (!indexed && types->canSetDefinite(shape->slot()))
            types->setDefinite(shape->slot());

        const Value &value = obj->getSlot(shape->slot());

        







        MOZ_ASSERT_IF(IsUntrackedValue(value), obj->is<CallObject>());
        if ((indexed || !value.isUndefined() || !CanHaveEmptyPropertyTypesForOwnProperty(obj)) &&
            !IsUntrackedValue(value))
        {
            Type type = GetValueType(value);
            types->TypeSet::addType(type, &cx->typeLifoAlloc());
        }

        if (indexed || shape->hadOverwrite()) {
            types->setNonConstantProperty(cx);
        } else {
            InferSpew(ISpewOps, "typeSet: %sT%p%s property %s %s - setConstant",
                      InferSpewColor(types), types, InferSpewColorReset(),
                      ObjectGroupString(obj->group()), TypeIdString(shape->propid()));
        }
    }
}

void
ObjectGroup::updateNewPropertyTypes(ExclusiveContext *cx, jsid id, HeapTypeSet *types)
{
    InferSpew(ISpewOps, "typeSet: %sT%p%s property %s %s",
              InferSpewColor(types), types, InferSpewColorReset(),
              ObjectGroupString(this), TypeIdString(id));

    if (!singleton() || !singleton()->isNative()) {
        types->setNonConstantProperty(cx);
        return;
    }

    NativeObject *obj = &singleton()->as<NativeObject>();

    






    if (JSID_IS_VOID(id)) {
        
        RootedShape shape(cx, obj->lastProperty());
        while (!shape->isEmptyShape()) {
            if (JSID_IS_VOID(IdToTypeId(shape->propid())))
                UpdatePropertyType(cx, types, obj, shape, true);
            shape = shape->previous();
        }

        
        for (size_t i = 0; i < obj->getDenseInitializedLength(); i++) {
            const Value &value = obj->getDenseElement(i);
            if (!value.isMagic(JS_ELEMENTS_HOLE)) {
                Type type = GetValueType(value);
                types->TypeSet::addType(type, &cx->typeLifoAlloc());
            }
        }
    } else if (!JSID_IS_EMPTY(id)) {
        RootedId rootedId(cx, id);
        Shape *shape = obj->lookup(cx, rootedId);
        if (shape)
            UpdatePropertyType(cx, types, obj, shape, false);
    }

    if (obj->watched()) {
        



        types->setNonDataProperty(cx);
    }
}

bool
ObjectGroup::addDefiniteProperties(ExclusiveContext *cx, Shape *shape)
{
    if (unknownProperties())
        return true;

    
    AutoEnterAnalysis enter(cx);

    while (!shape->isEmptyShape()) {
        jsid id = IdToTypeId(shape->propid());
        if (!JSID_IS_VOID(id)) {
            MOZ_ASSERT_IF(shape->slot() >= shape->numFixedSlots(),
                          shape->numFixedSlots() == NativeObject::MAX_FIXED_SLOTS);
            TypeSet *types = getProperty(cx, id);
            if (!types)
                return false;
            if (types->canSetDefinite(shape->slot()))
                types->setDefinite(shape->slot());
        }

        shape = shape->previous();
    }

    return true;
}

bool
ObjectGroup::matchDefiniteProperties(HandleObject obj)
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

void
types::AddTypePropertyId(ExclusiveContext *cx, ObjectGroup *group, jsid id, Type type)
{
    MOZ_ASSERT(id == IdToTypeId(id));

    if (group->unknownProperties())
        return;

    AutoEnterAnalysis enter(cx);

    HeapTypeSet *types = group->getProperty(cx, id);
    if (!types)
        return;

    
    if (!types->empty() && !types->nonConstantProperty()) {
        InferSpew(ISpewOps, "constantMutated: %sT%p%s %s",
                  InferSpewColor(types), types, InferSpewColorReset(), TypeString(type));
        types->setNonConstantProperty(cx);
    }

    if (types->hasType(type))
        return;

    InferSpew(ISpewOps, "externalType: property %s %s: %s",
              ObjectGroupString(group), TypeIdString(id), TypeString(type));
    types->addType(cx, type);

    
    
    
    
    
    if (group->newScript() && group->newScript()->initializedGroup()) {
        if (type.isObjectUnchecked() && types->unknownObject())
            type = Type::AnyObjectType();
        AddTypePropertyId(cx, group->newScript()->initializedGroup(), id, type);
    }
}

void
types::AddTypePropertyId(ExclusiveContext *cx, ObjectGroup *group, jsid id, const Value &value)
{
    AddTypePropertyId(cx, group, id, GetValueType(value));
}

void
ObjectGroup::markPropertyNonData(ExclusiveContext *cx, jsid id)
{
    AutoEnterAnalysis enter(cx);

    id = IdToTypeId(id);

    HeapTypeSet *types = getProperty(cx, id);
    if (types)
        types->setNonDataProperty(cx);
}

void
ObjectGroup::markPropertyNonWritable(ExclusiveContext *cx, jsid id)
{
    AutoEnterAnalysis enter(cx);

    id = IdToTypeId(id);

    HeapTypeSet *types = getProperty(cx, id);
    if (types)
        types->setNonWritableProperty(cx);
}

bool
ObjectGroup::isPropertyNonData(jsid id)
{
    TypeSet *types = maybeGetProperty(id);
    if (types)
        return types->nonDataProperty();
    return false;
}

bool
ObjectGroup::isPropertyNonWritable(jsid id)
{
    TypeSet *types = maybeGetProperty(id);
    if (types)
        return types->nonWritableProperty();
    return false;
}

void
ObjectGroup::markStateChange(ExclusiveContext *cxArg)
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
            MOZ_ASSERT(!types->constraintList);
        }
    }
}

void
ObjectGroup::setFlags(ExclusiveContext *cx, ObjectGroupFlags flags)
{
    if (hasAllFlags(flags))
        return;

    AutoEnterAnalysis enter(cx);

    if (singleton()) {
        
        MOZ_ASSERT_IF(flags & OBJECT_FLAG_ITERATED,
                      singleton()->lastProperty()->hasObjectFlag(BaseShape::ITERATED_SINGLETON));
    }

    addFlags(flags);

    InferSpew(ISpewOps, "%s: setFlags 0x%x", ObjectGroupString(this), flags);

    ObjectStateChange(cx, this, false);

    
    
    if (newScript() && newScript()->initializedGroup())
        newScript()->initializedGroup()->setFlags(cx, flags);
}

void
ObjectGroup::markUnknown(ExclusiveContext *cx)
{
    AutoEnterAnalysis enter(cx);

    MOZ_ASSERT(cx->zone()->types.activeAnalysis);
    MOZ_ASSERT(!unknownProperties());

    InferSpew(ISpewOps, "UnknownProperties: %s", ObjectGroupString(this));

    clearNewScript(cx);
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

TypeNewScript *
ObjectGroup::anyNewScript()
{
    if (newScript())
        return newScript();
    if (maybeUnboxedLayout())
        return unboxedLayout().newScript();
    return nullptr;
}

void
ObjectGroup::detachNewScript(bool writeBarrier)
{
    
    
    
    
    TypeNewScript *newScript = anyNewScript();
    MOZ_ASSERT(newScript);

    if (newScript->analyzed()) {
        NewObjectGroupTable &newObjectGroups = newScript->function()->compartment()->newObjectGroups;
        NewObjectGroupTable::Ptr p =
            newObjectGroups.lookup(NewObjectGroupTable::Lookup(nullptr, proto(), newScript->function()));
        MOZ_ASSERT(p->group == this);

        newObjectGroups.remove(p);
    }

    if (this->newScript())
        setAddendum(Addendum_None, nullptr, writeBarrier);
    else
        unboxedLayout().setNewScript(nullptr, writeBarrier);
}

void
ObjectGroup::maybeClearNewScriptOnOOM()
{
    MOZ_ASSERT(zone()->isGCSweepingOrCompacting());

    if (!isMarked())
        return;

    TypeNewScript *newScript = anyNewScript();
    if (!newScript)
        return;

    addFlags(OBJECT_FLAG_NEW_SCRIPT_CLEARED);

    
    detachNewScript( false);

    js_delete(newScript);
}

void
ObjectGroup::clearNewScript(ExclusiveContext *cx)
{
    TypeNewScript *newScript = anyNewScript();
    if (!newScript)
        return;

    AutoEnterAnalysis enter(cx);

    
    setFlags(cx, OBJECT_FLAG_NEW_SCRIPT_CLEARED);

    
    
    if (!newScript->function()->setNewScriptCleared(cx))
        cx->recoverFromOutOfMemory();

    detachNewScript( true);

    if (cx->isJSContext()) {
        bool found = newScript->rollbackPartiallyInitializedObjects(cx->asJSContext(), this);

        
        
        
        
        
        
        if (found) {
            for (unsigned i = 0; i < getPropertyCount(); i++) {
                Property *prop = getProperty(i);
                if (!prop)
                    continue;
                if (prop->types.definiteProperty())
                    prop->types.setNonDataProperty(cx);
            }
        }
    } else {
        
        MOZ_ASSERT(!cx->perThreadData->runtimeIfOnOwnerThread() ||
                   !cx->perThreadData->runtimeIfOnOwnerThread()->activation());
    }

    js_delete(newScript);
    markStateChange(cx);
}

void
ObjectGroup::print()
{
    TaggedProto tagged(proto());
    fprintf(stderr, "%s : %s",
            ObjectGroupString(this),
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
        if (maybeInterpretedFunction())
            fprintf(stderr, " ifun");
    }

    unsigned count = getPropertyCount();

    if (count == 0) {
        fprintf(stderr, " {}\n");
        return;
    }

    fprintf(stderr, " {");

    if (newScript()) {
        if (newScript()->analyzed()) {
            fprintf(stderr, "\n    newScript %d properties",
                    (int) newScript()->templateObject()->slotSpan());
            if (newScript()->initializedGroup()) {
                fprintf(stderr, " initializedGroup %p with %d properties",
                        newScript()->initializedGroup(), (int) newScript()->initializedShape()->slotSpan());
            }
        } else {
            fprintf(stderr, "\n    newScript unanalyzed");
        }
    }

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
    ObjectGroup *group;

    explicit TypeConstraintClearDefiniteGetterSetter(ObjectGroup *group)
      : group(group)
    {}

    const char *kind() { return "clearDefiniteGetterSetter"; }

    void newPropertyState(JSContext *cx, TypeSet *source) {
        




        if (source->nonDataProperty() || source->nonWritableProperty())
            group->clearNewScript(cx);
    }

    void newType(JSContext *cx, TypeSet *source, Type type) {}

    bool sweep(TypeZone &zone, TypeConstraint **res) {
        if (IsObjectGroupAboutToBeFinalized(&group))
            return false;
        *res = zone.typeLifoAlloc.new_<TypeConstraintClearDefiniteGetterSetter>(group);
        return true;
    }
};

bool
types::AddClearDefiniteGetterSetterForPrototypeChain(JSContext *cx, ObjectGroup *group, HandleId id)
{
    




    RootedObject proto(cx, group->proto().toObjectOrNull());
    while (proto) {
        ObjectGroup *protoGroup = proto->getGroup(cx);
        if (!protoGroup || protoGroup->unknownProperties())
            return false;
        HeapTypeSet *protoTypes = protoGroup->getProperty(cx, id);
        if (!protoTypes || protoTypes->nonDataProperty() || protoTypes->nonWritableProperty())
            return false;
        if (!protoTypes->addConstraint(cx, cx->typeLifoAlloc().new_<TypeConstraintClearDefiniteGetterSetter>(group)))
            return false;
        proto = proto->getProto();
    }
    return true;
}





class TypeConstraintClearDefiniteSingle : public TypeConstraint
{
  public:
    ObjectGroup *group;

    explicit TypeConstraintClearDefiniteSingle(ObjectGroup *group)
      : group(group)
    {}

    const char *kind() { return "clearDefiniteSingle"; }

    void newType(JSContext *cx, TypeSet *source, Type type) {
        if (source->baseFlags() || source->getObjectCount() > 1)
            group->clearNewScript(cx);
    }

    bool sweep(TypeZone &zone, TypeConstraint **res) {
        if (IsObjectGroupAboutToBeFinalized(&group))
            return false;
        *res = zone.typeLifoAlloc.new_<TypeConstraintClearDefiniteSingle>(group);
        return true;
    }
};

bool
types::AddClearDefiniteFunctionUsesInScript(JSContext *cx, ObjectGroup *group,
                                            JSScript *script, JSScript *calleeScript)
{
    
    
    
    
    
    
    

    TypeSetObjectKey *calleeKey = Type::ObjectType(calleeScript->functionNonDelazifying()).objectKey();

    unsigned count = TypeScript::NumTypeSets(script);
    StackTypeSet *typeArray = script->types()->typeArray();

    for (unsigned i = 0; i < count; i++) {
        StackTypeSet *types = &typeArray[i];
        if (!types->unknownObject() && types->getObjectCount() == 1) {
            if (calleeKey != types->getObject(0)) {
                
                
                
                JSObject *singleton = types->getSingleton(0);
                if (!singleton || !singleton->is<JSFunction>())
                    continue;
                JSFunction *fun = &singleton->as<JSFunction>();
                if (!fun->isNative())
                    continue;
                if (fun->native() != js_fun_call && fun->native() != js_fun_apply)
                    continue;
            }
            
            
            if (!types->addConstraint(cx, cx->typeLifoAlloc().new_<TypeConstraintClearDefiniteSingle>(group)))
                return false;
        }
    }

    return true;
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
IsAboutToBeFinalized(TypeSetObjectKey **keyp)
{
    
    uintptr_t flagBit = uintptr_t(*keyp) & 1;
    gc::Cell *tmp = reinterpret_cast<gc::Cell *>(uintptr_t(*keyp) & ~1);
    bool isAboutToBeFinalized = IsCellAboutToBeFinalized(&tmp);
    *keyp = reinterpret_cast<TypeSetObjectKey *>(uintptr_t(tmp) | flagBit);
    return isAboutToBeFinalized;
}

void
types::FillBytecodeTypeMap(JSScript *script, uint32_t *bytecodeMap)
{
    uint32_t added = 0;
    for (jsbytecode *pc = script->code(); pc < script->codeEnd(); pc += GetBytecodeLength(pc)) {
        JSOp op = JSOp(*pc);
        if (js_CodeSpec[op].format & JOF_TYPESET) {
            bytecodeMap[added++] = script->pcToOffset(pc);
            if (added == script->nTypeSets())
                break;
        }
    }
    MOZ_ASSERT(added == script->nTypeSets());
}

ArrayObject *
types::GetOrFixupCopyOnWriteObject(JSContext *cx, HandleScript script, jsbytecode *pc)
{
    
    
    RootedArrayObject obj(cx, &script->getObject(GET_UINT32_INDEX(pc))->as<ArrayObject>());
    MOZ_ASSERT(obj->denseElementsAreCopyOnWrite());

    if (obj->group()->fromAllocationSite()) {
        MOZ_ASSERT(obj->group()->hasAnyFlags(OBJECT_FLAG_COPY_ON_WRITE));
        return obj;
    }

    RootedObjectGroup group(cx, TypeScript::InitGroup(cx, script, pc, JSProto_Array));
    if (!group)
        return nullptr;

    group->addFlags(OBJECT_FLAG_COPY_ON_WRITE);

    
    MOZ_ASSERT(obj->slotSpan() == 0);
    for (size_t i = 0; i < obj->getDenseInitializedLength(); i++) {
        const Value &v = obj->getDenseElement(i);
        AddTypePropertyId(cx, group, JSID_VOID, v);
    }

    obj->setGroup(group);
    return obj;
}

ArrayObject *
types::GetCopyOnWriteObject(JSScript *script, jsbytecode *pc)
{
    
    
    
    
    
    ArrayObject *obj = &script->getObject(GET_UINT32_INDEX(pc))->as<ArrayObject>();
    MOZ_ASSERT(obj->denseElementsAreCopyOnWrite());

    return obj;
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
types::UseSingletonForClone(JSFunction *fun)
{
    if (!fun->isInterpreted())
        return false;

    if (fun->hasScript() && fun->nonLazyScript()->shouldCloneAtCallsite())
        return true;

    if (fun->isArrow())
        return false;

    if (fun->isSingleton())
        return false;

    























    uint32_t begin, end;
    if (fun->hasScript()) {
        if (!fun->nonLazyScript()->usesArgumentsApplyAndThis())
            return false;
        begin = fun->nonLazyScript()->sourceStart();
        end = fun->nonLazyScript()->sourceEnd();
    } else {
        if (!fun->lazyScript()->usesArgumentsApplyAndThis())
            return false;
        begin = fun->lazyScript()->begin();
        end = fun->lazyScript()->end();
    }

    return end - begin <= 100;
}





bool
JSScript::makeTypes(JSContext *cx)
{
    MOZ_ASSERT(!types_);

    AutoEnterAnalysis enter(cx);

    unsigned count = TypeScript::NumTypeSets(this);

    TypeScript *typeScript = (TypeScript *)
        zone()->pod_calloc<uint8_t>(TypeScript::SizeIncludingTypeArray(count));
    if (!typeScript)
        return false;

    types_ = typeScript;
    setTypesGeneration(cx->zone()->types.generation);

#ifdef DEBUG
    StackTypeSet *typeArray = typeScript->typeArray();
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

    return true;
}

 bool
JSFunction::setTypeForScriptedFunction(ExclusiveContext *cx, HandleFunction fun,
                                       bool singleton )
{
    if (singleton) {
        if (!setSingleton(cx, fun))
            return false;
    } else {
        RootedObject funProto(cx, fun->getProto());
        Rooted<TaggedProto> taggedProto(cx, TaggedProto(funProto));
        ObjectGroup *group =
            cx->compartment()->types.newObjectGroup(cx, &JSFunction::class_, taggedProto);
        if (!group)
            return false;

        fun->setGroup(group);
        group->setInterpretedFunction(fun);
    }

    return true;
}





void
PreliminaryObjectArray::registerNewObject(JSObject *res)
{
    
    
    
    MOZ_ASSERT(!IsInsideNursery(res));

    for (size_t i = 0; i < COUNT; i++) {
        if (!objects[i]) {
            objects[i] = res;
            return;
        }
    }

    MOZ_CRASH("There should be room for registering the new object");
}

void
PreliminaryObjectArray::unregisterNewObject(JSObject *res)
{
    for (size_t i = 0; i < COUNT; i++) {
        if (objects[i] == res) {
            objects[i] = nullptr;
            return;
        }
    }

    MOZ_CRASH("The object should be one of the preliminary objects");
}

bool
PreliminaryObjectArray::full() const
{
    for (size_t i = 0; i < COUNT; i++) {
        if (!objects[i])
            return false;
    }
    return true;
}

void
PreliminaryObjectArray::sweep()
{
    
    
    for (size_t i = 0; i < COUNT; i++) {
        JSObject **ptr = &objects[i];
        if (*ptr && IsObjectAboutToBeFinalized(ptr))
            *ptr = nullptr;
    }
}







 void
TypeNewScript::make(JSContext *cx, ObjectGroup *group, JSFunction *fun)
{
    MOZ_ASSERT(cx->zone()->types.activeAnalysis);
    MOZ_ASSERT(!group->newScript());
    MOZ_ASSERT(!group->maybeUnboxedLayout());

    if (group->unknownProperties())
        return;

    ScopedJSDeletePtr<TypeNewScript> newScript(cx->new_<TypeNewScript>());
    if (!newScript)
        return;

    newScript->function_ = fun;

    newScript->preliminaryObjects = group->zone()->new_<PreliminaryObjectArray>();
    if (!newScript->preliminaryObjects)
        return;

    group->setNewScript(newScript.forget());

    gc::TraceTypeNewScript(group);
}

size_t
TypeNewScript::sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
    size_t n = mallocSizeOf(this);
    n += mallocSizeOf(preliminaryObjects);
    n += mallocSizeOf(initializerList);
    return n;
}

void
TypeNewScript::registerNewObject(PlainObject *res)
{
    MOZ_ASSERT(!analyzed());

    
    
    
    MOZ_ASSERT(res->numFixedSlots() == NativeObject::MAX_FIXED_SLOTS);

    preliminaryObjects->registerNewObject(res);
}

void
TypeNewScript::unregisterNewObject(PlainObject *res)
{
    MOZ_ASSERT(!analyzed());
    preliminaryObjects->unregisterNewObject(res);
}


static bool
OnlyHasDataProperties(Shape *shape)
{
    MOZ_ASSERT(!shape->inDictionary());

    while (!shape->isEmptyShape()) {
        if (!shape->isDataDescriptor() ||
            !shape->configurable() ||
            !shape->enumerable() ||
            !shape->writable() ||
            !shape->hasSlot())
        {
            return false;
        }
        shape = shape->previous();
    }

    return true;
}



static Shape *
CommonPrefix(Shape *first, Shape *second)
{
    MOZ_ASSERT(OnlyHasDataProperties(first));
    MOZ_ASSERT(OnlyHasDataProperties(second));

    while (first->slotSpan() > second->slotSpan())
        first = first->previous();
    while (second->slotSpan() > first->slotSpan())
        second = second->previous();

    while (first != second && !first->isEmptyShape()) {
        first = first->previous();
        second = second->previous();
    }

    return first;
}

static bool
ChangeObjectFixedSlotCount(JSContext *cx, PlainObject *obj, gc::AllocKind allocKind)
{
    MOZ_ASSERT(OnlyHasDataProperties(obj->lastProperty()));

    
    RootedShape oldShape(cx, obj->lastProperty());
    RootedObjectGroup group(cx, obj->group());
    JSObject *clone = NewReshapedObject(cx, group, obj->getParent(), allocKind, oldShape);
    if (!clone)
        return false;

    obj->setLastPropertyShrinkFixedSlots(clone->lastProperty());
    return true;
}

namespace {

struct DestroyTypeNewScript
{
    JSContext *cx;
    ObjectGroup *group;

    DestroyTypeNewScript(JSContext *cx, ObjectGroup *group)
      : cx(cx), group(group)
    {}

    ~DestroyTypeNewScript() {
        if (group)
            group->clearNewScript(cx);
    }
};

} 

bool
TypeNewScript::maybeAnalyze(JSContext *cx, ObjectGroup *group, bool *regenerate, bool force)
{
    
    
    MOZ_ASSERT(this == group->newScript());

    
    
    group->maybeSweep(nullptr);
    if (!group->newScript())
        return true;

    if (regenerate)
        *regenerate = false;

    if (analyzed()) {
        
        return true;
    }

    
    
    if (!force && !preliminaryObjects->full())
        return true;

    AutoEnterAnalysis enter(cx);

    
    DestroyTypeNewScript destroyNewScript(cx, group);

    
    
    Shape *prefixShape = nullptr;
    size_t maxSlotSpan = 0;
    for (size_t i = 0; i < PreliminaryObjectArray::COUNT; i++) {
        JSObject *objBase = preliminaryObjects->get(i);
        if (!objBase)
            continue;
        PlainObject *obj = &objBase->as<PlainObject>();

        
        
        Shape *shape = obj->lastProperty();
        if (shape->inDictionary() ||
            !OnlyHasDataProperties(shape) ||
            shape->getObjectFlags() != 0 ||
            shape->getObjectMetadata() != nullptr)
        {
            return true;
        }

        maxSlotSpan = Max<size_t>(maxSlotSpan, obj->slotSpan());

        if (prefixShape) {
            MOZ_ASSERT(shape->numFixedSlots() == prefixShape->numFixedSlots());
            prefixShape = CommonPrefix(prefixShape, shape);
        } else {
            prefixShape = shape;
        }
        if (prefixShape->isEmptyShape()) {
            
            return true;
        }
    }
    if (!prefixShape)
        return true;

    gc::AllocKind kind = gc::GetGCObjectKind(maxSlotSpan);

    if (kind != gc::GetGCObjectKind(NativeObject::MAX_FIXED_SLOTS)) {
        
        
        
        
        
        
        
        
        Shape *newPrefixShape = nullptr;
        for (size_t i = 0; i < PreliminaryObjectArray::COUNT; i++) {
            JSObject *objBase = preliminaryObjects->get(i);
            if (!objBase)
                continue;
            PlainObject *obj = &objBase->as<PlainObject>();
            if (!ChangeObjectFixedSlotCount(cx, obj, kind))
                return false;
            if (newPrefixShape) {
                MOZ_ASSERT(CommonPrefix(obj->lastProperty(), newPrefixShape) == newPrefixShape);
            } else {
                newPrefixShape = obj->lastProperty();
                while (newPrefixShape->slotSpan() > prefixShape->slotSpan())
                    newPrefixShape = newPrefixShape->previous();
            }
        }
        prefixShape = newPrefixShape;
    }

    RootedObjectGroup groupRoot(cx, group);
    templateObject_ = NewObjectWithGroup<PlainObject>(cx, groupRoot, cx->global(), kind,
                                                      MaybeSingletonObject);
    if (!templateObject_)
        return false;

    Vector<Initializer> initializerVector(cx);

    RootedPlainObject templateRoot(cx, templateObject());
    if (!jit::AnalyzeNewScriptDefiniteProperties(cx, function(), group, templateRoot, &initializerVector))
        return false;

    if (!group->newScript())
        return true;

    MOZ_ASSERT(OnlyHasDataProperties(templateObject()->lastProperty()));

    if (templateObject()->slotSpan() != 0) {
        
        
        
        
        
        
        if (templateObject()->slotSpan() > prefixShape->slotSpan())
            return true;
        {
            Shape *shape = prefixShape;
            while (shape->slotSpan() != templateObject()->slotSpan())
                shape = shape->previous();
            Shape *templateShape = templateObject()->lastProperty();
            while (!shape->isEmptyShape()) {
                if (shape->slot() != templateShape->slot())
                    return true;
                if (shape->propid() != templateShape->propid())
                    return true;
                shape = shape->previous();
                templateShape = templateShape->previous();
            }
            if (!templateShape->isEmptyShape())
                return true;
        }

        Initializer done(Initializer::DONE, 0);

        if (!initializerVector.append(done))
            return false;

        initializerList = group->zone()->pod_calloc<Initializer>(initializerVector.length());
        if (!initializerList)
            return false;
        PodCopy(initializerList, initializerVector.begin(), initializerVector.length());
    }

    
    if (!TryConvertToUnboxedLayout(cx, templateObject()->lastProperty(), group, preliminaryObjects))
        return false;

    js_delete(preliminaryObjects);
    preliminaryObjects = nullptr;

    if (group->maybeUnboxedLayout()) {
        
        
        MOZ_ASSERT(group->unboxedLayout().newScript() == this);
        destroyNewScript.group = nullptr;

        
        
        
        
        templateObject_ = nullptr;

        return true;
    }

    if (prefixShape->slotSpan() == templateObject()->slotSpan()) {
        
        
        
        if (!group->addDefiniteProperties(cx, templateObject()->lastProperty()))
            return false;

        destroyNewScript.group = nullptr;
        return true;
    }

    
    
    
    
    
    MOZ_ASSERT(prefixShape->slotSpan() > templateObject()->slotSpan());

    ObjectGroupFlags initialFlags = group->flags() & OBJECT_FLAG_DYNAMIC_MASK;

    Rooted<TaggedProto> protoRoot(cx, group->proto());
    ObjectGroup *initialGroup =
        cx->compartment()->types.newObjectGroup(cx, group->clasp(), protoRoot, initialFlags);
    if (!initialGroup)
        return false;

    if (!initialGroup->addDefiniteProperties(cx, templateObject()->lastProperty()))
        return false;
    if (!group->addDefiniteProperties(cx, prefixShape))
        return false;

    NewObjectGroupTable &table = cx->compartment()->newObjectGroups;
    NewObjectGroupTable::Lookup lookup(nullptr, group->proto(), function());

    MOZ_ASSERT(table.lookup(lookup)->group == group);
    table.remove(lookup);
    table.putNew(lookup, NewObjectGroupEntry(initialGroup, function()));

    templateObject()->setGroup(initialGroup);

    
    
    group->setNewScript(nullptr);
    initialGroup->setNewScript(this);

    initializedShape_ = prefixShape;
    initializedGroup_ = group;

    destroyNewScript.group = nullptr;

    if (regenerate)
        *regenerate = true;
    return true;
}

bool
TypeNewScript::rollbackPartiallyInitializedObjects(JSContext *cx, ObjectGroup *group)
{
    
    
    
    
    
    
    

    if (!initializerList)
        return false;

    bool found = false;

    RootedFunction function(cx, this->function());
    Vector<uint32_t, 32> pcOffsets(cx);
    for (ScriptFrameIter iter(cx); !iter.done(); ++iter) {
        pcOffsets.append(iter.script()->pcToOffset(iter.pc()));

        if (!iter.isConstructing() || !iter.matchCallee(cx, function))
            continue;

        Value thisv = iter.thisv(cx);
        if (!thisv.isObject() ||
            thisv.toObject().hasLazyGroup() ||
            thisv.toObject().group() != group)
        {
            continue;
        }

        if (thisv.toObject().is<UnboxedPlainObject>() &&
            !thisv.toObject().as<UnboxedPlainObject>().convertToNative(cx))
        {
            CrashAtUnhandlableOOM("rollbackPartiallyInitializedObjects");
        }

        
        RootedPlainObject obj(cx, &thisv.toObject().as<PlainObject>());

        
        bool finished = false;

        
        uint32_t numProperties = 0;

        
        
        bool pastProperty = false;

        
        int callDepth = pcOffsets.length() - 1;

        
        int setpropDepth = callDepth;

        for (Initializer *init = initializerList;; init++) {
            if (init->kind == Initializer::SETPROP) {
                if (!pastProperty && pcOffsets[setpropDepth] < init->offset) {
                    
                    break;
                }
                
                numProperties++;
                pastProperty = false;
                setpropDepth = callDepth;
            } else if (init->kind == Initializer::SETPROP_FRAME) {
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
                MOZ_ASSERT(init->kind == Initializer::DONE);
                finished = true;
                break;
            }
        }

        if (!finished) {
            (void) NativeObject::rollbackProperties(cx, obj, numProperties);
            found = true;
        }
    }

    return found;
}

void
TypeNewScript::trace(JSTracer *trc)
{
    MarkObject(trc, &function_, "TypeNewScript_function");

    if (templateObject_)
        MarkObject(trc, &templateObject_, "TypeNewScript_templateObject");

    if (initializedShape_)
        MarkShape(trc, &initializedShape_, "TypeNewScript_initializedShape");

    if (initializedGroup_)
        MarkObjectGroup(trc, &initializedGroup_, "TypeNewScript_initializedGroup");
}

void
TypeNewScript::sweep()
{
    if (preliminaryObjects)
        preliminaryObjects->sweep();
}





bool
JSObject::shouldSplicePrototype(JSContext *cx)
{
    





    if (getProto() != nullptr)
        return false;
    return isSingleton();
}

bool
JSObject::splicePrototype(JSContext *cx, const Class *clasp, Handle<TaggedProto> proto)
{
    MOZ_ASSERT(cx->compartment() == compartment());

    RootedObject self(cx, this);

    




    MOZ_ASSERT(self->isSingleton());

    
    MOZ_ASSERT_IF(proto.isObject(), !proto.toObject()->getClass()->ext.outerObject);

    if (proto.isObject() && !proto.toObject()->setDelegate(cx))
        return false;

    
    RootedObjectGroup group(cx, self->getGroup(cx));
    if (!group)
        return false;
    RootedObjectGroup protoGroup(cx, nullptr);
    if (proto.isObject()) {
        protoGroup = proto.toObject()->getGroup(cx);
        if (!protoGroup)
            return false;
    }

    group->setClasp(clasp);
    group->setProto(cx, proto);
    return true;
}

 ObjectGroup *
JSObject::makeLazyGroup(JSContext *cx, HandleObject obj)
{
    MOZ_ASSERT(obj->hasLazyGroup());
    MOZ_ASSERT(cx->compartment() == obj->compartment());

    
    if (obj->is<JSFunction>() && obj->as<JSFunction>().isInterpretedLazy()) {
        RootedFunction fun(cx, &obj->as<JSFunction>());
        if (!fun->getOrCreateScript(cx))
            return nullptr;
    }

    
    
    ObjectGroupFlags initialFlags = OBJECT_FLAG_NON_PACKED;

    if (obj->lastProperty()->hasObjectFlag(BaseShape::ITERATED_SINGLETON))
        initialFlags |= OBJECT_FLAG_ITERATED;

    if (obj->isIndexed())
        initialFlags |= OBJECT_FLAG_SPARSE_INDEXES;

    if (obj->is<ArrayObject>() && obj->as<ArrayObject>().length() > INT32_MAX)
        initialFlags |= OBJECT_FLAG_LENGTH_OVERFLOW;

    Rooted<TaggedProto> proto(cx, obj->getTaggedProto());
    ObjectGroup *group = cx->compartment()->types.newObjectGroup(cx, obj->getClass(), proto,
                                                                 initialFlags);
    if (!group)
        return nullptr;

    AutoEnterAnalysis enter(cx);

    

    group->initSingleton(obj);

    if (obj->is<JSFunction>() && obj->as<JSFunction>().isInterpreted())
        group->setInterpretedFunction(&obj->as<JSFunction>());

    obj->group_ = group;

    return group;
}

 inline HashNumber
NewObjectGroupEntry::hash(const Lookup &lookup)
{
    return PointerHasher<JSObject *, 3>::hash(lookup.hashProto.raw()) ^
           PointerHasher<const Class *, 3>::hash(lookup.clasp) ^
           PointerHasher<JSObject *, 3>::hash(lookup.associated);
}

 inline bool
NewObjectGroupEntry::match(const NewObjectGroupEntry &key, const Lookup &lookup)
{
    return key.group->proto() == lookup.matchProto &&
           (!lookup.clasp || key.group->clasp() == lookup.clasp) &&
           key.associated == lookup.associated;
}

#ifdef DEBUG
bool
JSObject::hasNewGroup(const Class *clasp, ObjectGroup *group)
{
    NewObjectGroupTable &table = compartment()->newObjectGroups;

    if (!table.initialized())
        return false;

    NewObjectGroupTable::Ptr p =
        table.lookup(NewObjectGroupTable::Lookup(clasp, TaggedProto(this), nullptr));
    return p && p->group == group;
}
#endif 

 bool
JSObject::setNewGroupUnknown(JSContext *cx, const Class *clasp, HandleObject obj)
{
    if (!obj->setFlag(cx, js::BaseShape::NEW_GROUP_UNKNOWN))
        return false;

    
    NewObjectGroupTable &table = cx->compartment()->newObjectGroups;
    if (table.initialized()) {
        Rooted<TaggedProto> taggedProto(cx, TaggedProto(obj));
        NewObjectGroupTable::Ptr p =
            table.lookup(NewObjectGroupTable::Lookup(clasp, taggedProto, nullptr));
        if (p)
            MarkObjectGroupUnknownProperties(cx, p->group);
    }

    return true;
}






class NewObjectGroupsSetRef : public BufferableRef
{
    NewObjectGroupTable *set;
    const Class *clasp;
    JSObject *proto;
    JSObject *associated;

  public:
    NewObjectGroupsSetRef(NewObjectGroupTable *s, const Class *clasp, JSObject *proto, JSObject *associated)
        : set(s), clasp(clasp), proto(proto), associated(associated)
    {}

    void mark(JSTracer *trc) {
        JSObject *prior = proto;
        trc->setTracingLocation(&*prior);
        Mark(trc, &proto, "newObjectGroups set prototype");
        if (prior == proto)
            return;

        NewObjectGroupTable::Ptr p =
            set->lookup(NewObjectGroupTable::Lookup(clasp, TaggedProto(prior), TaggedProto(proto),
                                                    associated));
        if (!p)
            return;

        set->rekeyAs(NewObjectGroupTable::Lookup(clasp, TaggedProto(prior), TaggedProto(proto), associated),
                     NewObjectGroupTable::Lookup(clasp, TaggedProto(proto), associated), *p);
    }
};

static void
ObjectGroupTablePostBarrier(ExclusiveContext *cx, NewObjectGroupTable *table,
                           const Class *clasp, TaggedProto proto, JSObject *associated)
{
    MOZ_ASSERT_IF(associated, !IsInsideNursery(associated));

    if (!proto.isObject())
        return;

    if (!cx->isJSContext()) {
        MOZ_ASSERT(!IsInsideNursery(proto.toObject()));
        return;
    }

    if (IsInsideNursery(proto.toObject())) {
        StoreBuffer &sb = cx->asJSContext()->runtime()->gc.storeBuffer;
        sb.putGeneric(NewObjectGroupsSetRef(table, clasp, proto.toObject(), associated));
    }
}

ObjectGroup *
ExclusiveContext::getNewGroup(const Class *clasp, TaggedProto proto, JSObject *associated)
{
    MOZ_ASSERT_IF(associated, proto.isObject());
    MOZ_ASSERT_IF(associated, associated->is<JSFunction>() || associated->is<TypeDescr>());
    MOZ_ASSERT_IF(proto.isObject(), isInsideCurrentCompartment(proto.toObject()));

    
    
    
    MOZ_ASSERT(!clasp == (associated && associated->is<JSFunction>()));

    NewObjectGroupTable &newObjectGroups = compartment()->newObjectGroups;

    if (!newObjectGroups.initialized() && !newObjectGroups.init())
        return nullptr;

    if (associated && associated->is<JSFunction>()) {
        MOZ_ASSERT(!clasp);

        
        JSFunction *fun = &associated->as<JSFunction>();
        if (fun->hasScript())
            associated = fun->nonLazyScript()->functionNonDelazifying();
        else if (fun->isInterpretedLazy() && !fun->isSelfHostedBuiltin())
            associated = fun->lazyScript()->functionNonDelazifying();
        else
            associated = nullptr;

        
        
        if (associated && associated->wasNewScriptCleared())
            associated = nullptr;

        if (!associated)
            clasp = &PlainObject::class_;
    }

    NewObjectGroupTable::AddPtr p =
        newObjectGroups.lookupForAdd(NewObjectGroupTable::Lookup(clasp, proto, associated));
    if (p) {
        ObjectGroup *group = p->group;
        MOZ_ASSERT_IF(clasp, group->clasp() == clasp);
        MOZ_ASSERT_IF(!clasp, group->clasp() == &PlainObject::class_ ||
                              group->clasp() == &UnboxedPlainObject::class_);
        MOZ_ASSERT(group->proto() == proto);
        return group;
    }

    AutoEnterAnalysis enter(this);

    if (proto.isObject() && !proto.toObject()->setDelegate(this))
        return nullptr;

    ObjectGroupFlags initialFlags = 0;
    if (!proto.isObject() || proto.toObject()->isNewGroupUnknown())
        initialFlags = OBJECT_FLAG_DYNAMIC_MASK;

    Rooted<TaggedProto> protoRoot(this, proto);
    ObjectGroup *group = compartment()->types.newObjectGroup(this,
                                                             clasp ? clasp : &PlainObject::class_,
                                                             protoRoot, initialFlags);
    if (!group)
        return nullptr;

    if (!newObjectGroups.add(p, NewObjectGroupEntry(group, associated)))
        return nullptr;

    ObjectGroupTablePostBarrier(this, &newObjectGroups, clasp, proto, associated);

    if (proto.isObject()) {
        RootedObject obj(this, proto.toObject());

        if (associated) {
            if (associated->is<JSFunction>())
                TypeNewScript::make(asJSContext(), group, &associated->as<JSFunction>());
            else
                group->setTypeDescr(&associated->as<TypeDescr>());
        }

        






        if (obj->is<RegExpObject>()) {
            AddTypePropertyId(this, group, NameToId(names().source), Type::StringType());
            AddTypePropertyId(this, group, NameToId(names().global), Type::BooleanType());
            AddTypePropertyId(this, group, NameToId(names().ignoreCase), Type::BooleanType());
            AddTypePropertyId(this, group, NameToId(names().multiline), Type::BooleanType());
            AddTypePropertyId(this, group, NameToId(names().sticky), Type::BooleanType());
            AddTypePropertyId(this, group, NameToId(names().lastIndex), Type::Int32Type());
        }

        if (obj->is<StringObject>())
            AddTypePropertyId(this, group, NameToId(names().length), Type::Int32Type());

        if (obj->is<ErrorObject>()) {
            AddTypePropertyId(this, group, NameToId(names().fileName), Type::StringType());
            AddTypePropertyId(this, group, NameToId(names().lineNumber), Type::Int32Type());
            AddTypePropertyId(this, group, NameToId(names().columnNumber), Type::Int32Type());
            AddTypePropertyId(this, group, NameToId(names().stack), Type::StringType());
        }
    }

    return group;
}

ObjectGroup *
ExclusiveContext::getLazySingletonGroup(const Class *clasp, TaggedProto proto)
{
    MOZ_ASSERT_IF(proto.isObject(), compartment() == proto.toObject()->compartment());

    AutoEnterAnalysis enter(this);

    NewObjectGroupTable &table = compartment()->lazyObjectGroups;

    if (!table.initialized() && !table.init())
        return nullptr;

    NewObjectGroupTable::AddPtr p = table.lookupForAdd(NewObjectGroupTable::Lookup(clasp, proto, nullptr));
    if (p) {
        ObjectGroup *group = p->group;
        MOZ_ASSERT(group->lazy());

        return group;
    }

    Rooted<TaggedProto> protoRoot(this, proto);
    ObjectGroup *group = compartment()->types.newObjectGroup(this, clasp, protoRoot);
    if (!group)
        return nullptr;

    if (!table.add(p, NewObjectGroupEntry(group, nullptr)))
        return nullptr;

    ObjectGroupTablePostBarrier(this, &table, clasp, proto, nullptr);

    group->initSingleton((JSObject *) ObjectGroup::LAZY_SINGLETON);
    MOZ_ASSERT(group->singleton(), "created group must be a proper singleton");

    return group;
}





void
ConstraintTypeSet::sweep(Zone *zone, AutoClearTypeInferenceStateOnOOM &oom)
{
    MOZ_ASSERT(zone->isGCSweepingOrCompacting());

    
    
    MOZ_ASSERT(!zone->runtimeFromMainThread()->isHeapMinorCollecting());

    





    unsigned objectCount = baseObjectCount();
    if (objectCount >= 2) {
        unsigned oldCapacity = HashSetCapacity(objectCount);
        TypeSetObjectKey **oldArray = objectSet;

        clearObjects();
        objectCount = 0;
        for (unsigned i = 0; i < oldCapacity; i++) {
            TypeSetObjectKey *key = oldArray[i];
            if (!key)
                continue;
            if (!IsAboutToBeFinalized(&key)) {
                TypeSetObjectKey **pentry =
                    HashSetInsert<TypeSetObjectKey *,TypeSetObjectKey,TypeSetObjectKey>
                        (zone->types.typeLifoAlloc, objectSet, objectCount, key);
                if (pentry) {
                    *pentry = key;
                } else {
                    oom.setOOM();
                    flags |= TYPE_FLAG_ANYOBJECT;
                    clearObjects();
                    objectCount = 0;
                    break;
                }
            } else if (key->isGroup() && key->group()->unknownProperties()) {
                
                
                
                flags |= TYPE_FLAG_ANYOBJECT;
                clearObjects();
                objectCount = 0;
                break;
            }
        }
        setBaseObjectCount(objectCount);
    } else if (objectCount == 1) {
        TypeSetObjectKey *key = (TypeSetObjectKey *) objectSet;
        if (!IsAboutToBeFinalized(&key)) {
            objectSet = reinterpret_cast<TypeSetObjectKey **>(key);
        } else {
            
            
            if (key->isGroup() && key->group()->unknownProperties())
                flags |= TYPE_FLAG_ANYOBJECT;
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
                oom.setOOM();
            }
        }
        constraint = constraint->next;
    }
}

inline void
ObjectGroup::clearProperties()
{
    setBasePropertyCount(0);
    propertySet = nullptr;
}

#ifdef DEBUG
bool
ObjectGroup::needsSweep()
{
    
    
    return generation() != zoneFromAnyThread()->types.generation;
}
#endif

static void
EnsureHasAutoClearTypeInferenceStateOnOOM(AutoClearTypeInferenceStateOnOOM *&oom, Zone *zone,
                                          Maybe<AutoClearTypeInferenceStateOnOOM> &fallback)
{
    if (!oom) {
        if (zone->types.activeAnalysis) {
            oom = &zone->types.activeAnalysis->oom;
        } else {
            fallback.emplace(zone);
            oom = &fallback.ref();
        }
    }
}








void
ObjectGroup::maybeSweep(AutoClearTypeInferenceStateOnOOM *oom)
{
    if (generation() == zoneFromAnyThread()->types.generation) {
        
        return;
    }

    setGeneration(zone()->types.generation);

    MOZ_ASSERT(zone()->isGCSweepingOrCompacting());
    MOZ_ASSERT(!zone()->runtimeFromMainThread()->isHeapMinorCollecting());

    Maybe<AutoClearTypeInferenceStateOnOOM> fallbackOOM;
    EnsureHasAutoClearTypeInferenceStateOnOOM(oom, zone(), fallbackOOM);

    if (maybeUnboxedLayout() && unboxedLayout().newScript())
        unboxedLayout().newScript()->sweep();

    if (newScript())
        newScript()->sweep();

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
                        newProp->types.sweep(zone(), *oom);
                        continue;
                    }
                }

                oom->setOOM();
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
                newProp->types.sweep(zone(), *oom);
            } else {
                oom->setOOM();
                addFlags(OBJECT_FLAG_DYNAMIC_MASK | OBJECT_FLAG_UNKNOWN_PROPERTIES);
                clearProperties();
                return;
            }
        }
    }
}

void
TypeCompartment::clearTables()
{
    if (allocationSiteTable && allocationSiteTable->initialized())
        allocationSiteTable->clear();
    if (arrayTypeTable && arrayTypeTable->initialized())
        arrayTypeTable->clear();
    if (objectTypeTable && objectTypeTable->initialized())
        objectTypeTable->clear();
}

void
TypeCompartment::sweep(FreeOp *fop)
{
    




    if (arrayTypeTable) {
        for (ArrayTypeTable::Enum e(*arrayTypeTable); !e.empty(); e.popFront()) {
            ArrayTableKey key = e.front().key();
            MOZ_ASSERT(key.type.isUnknown() || !key.type.isSingleton());

            bool remove = false;
            if (!key.type.isUnknown() && key.type.isGroup()) {
                ObjectGroup *group = key.type.groupNoBarrier();
                if (IsObjectGroupAboutToBeFinalized(&group))
                    remove = true;
                else
                    key.type = Type::ObjectType(group);
            }
            if (key.proto && key.proto != TaggedProto::LazyProto &&
                IsObjectAboutToBeFinalized(&key.proto))
            {
                remove = true;
            }
            if (IsObjectGroupAboutToBeFinalized(e.front().value().unsafeGet()))
                remove = true;

            if (remove)
                e.removeFront();
            else if (key != e.front().key())
                e.rekeyFront(key);
        }
    }

    if (objectTypeTable) {
        for (ObjectTypeTable::Enum e(*objectTypeTable); !e.empty(); e.popFront()) {
            const ObjectTableKey &key = e.front().key();
            ObjectTableEntry &entry = e.front().value();

            bool remove = false;
            if (IsObjectGroupAboutToBeFinalized(entry.group.unsafeGet()))
                remove = true;
            if (IsShapeAboutToBeFinalized(entry.shape.unsafeGet()))
                remove = true;
            for (unsigned i = 0; !remove && i < key.nproperties; i++) {
                if (JSID_IS_STRING(key.properties[i])) {
                    JSString *str = JSID_TO_STRING(key.properties[i]);
                    if (IsStringAboutToBeFinalized(&str))
                        remove = true;
                    MOZ_ASSERT(AtomToId((JSAtom *)str) == key.properties[i]);
                } else if (JSID_IS_SYMBOL(key.properties[i])) {
                    JS::Symbol *sym = JSID_TO_SYMBOL(key.properties[i]);
                    if (IsSymbolAboutToBeFinalized(&sym))
                        remove = true;
                }

                MOZ_ASSERT(!entry.types[i].isSingleton());
                ObjectGroup *group = nullptr;
                if (entry.types[i].isGroup()) {
                    group = entry.types[i].groupNoBarrier();
                    if (IsObjectGroupAboutToBeFinalized(&group))
                        remove = true;
                    else if (group != entry.types[i].groupNoBarrier())
                        entry.types[i] = Type::ObjectType(group);
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
            bool valDying = IsObjectGroupAboutToBeFinalized(e.front().value().unsafeGet());
            if (keyDying || valDying)
                e.removeFront();
            else if (key.script != e.front().key().script)
                e.rekeyFront(key);
        }
    }
}

void
JSCompartment::sweepNewObjectGroupTable(NewObjectGroupTable &table)
{
    MOZ_ASSERT(zone()->runtimeFromAnyThread()->isHeapCollecting());
    if (table.initialized()) {
        for (NewObjectGroupTable::Enum e(table); !e.empty(); e.popFront()) {
            NewObjectGroupEntry entry = e.front();
            if (IsObjectGroupAboutToBeFinalizedFromAnyThread(entry.group.unsafeGet()) ||
                (entry.associated && IsObjectAboutToBeFinalizedFromAnyThread(&entry.associated)))
            {
                e.removeFront();
            } else {
                
                MOZ_ASSERT(entry.group.unbarrieredGet() == e.front().group.unbarrieredGet());
                MOZ_ASSERT(entry.associated == e.front().associated);
            }
        }
    }
}

void
JSCompartment::fixupNewObjectGroupTable(NewObjectGroupTable &table)
{
    



    MOZ_ASSERT(zone()->isCollecting());
    if (table.initialized()) {
        for (NewObjectGroupTable::Enum e(table); !e.empty(); e.popFront()) {
            NewObjectGroupEntry entry = e.front();
            bool needRekey = false;
            if (IsForwarded(entry.group.get())) {
                entry.group.set(Forwarded(entry.group.get()));
                needRekey = true;
            }
            TaggedProto proto = entry.group->proto();
            if (proto.isObject() && IsForwarded(proto.toObject())) {
                proto = TaggedProto(Forwarded(proto.toObject()));
                needRekey = true;
            }
            if (entry.associated && IsForwarded(entry.associated)) {
                entry.associated = Forwarded(entry.associated);
                needRekey = true;
            }
            if (needRekey) {
                const Class *clasp = entry.group->clasp();
                if (entry.associated && entry.associated->is<JSFunction>())
                    clasp = nullptr;
                NewObjectGroupTable::Lookup lookup(clasp, proto, entry.associated);
                e.rekeyFront(lookup, entry);
            }
        }
    }
}

#ifdef JSGC_HASH_TABLE_CHECKS

void
JSCompartment::checkObjectGroupTablesAfterMovingGC()
{
    checkObjectGroupTableAfterMovingGC(newObjectGroups);
    checkObjectGroupTableAfterMovingGC(lazyObjectGroups);
}

void
JSCompartment::checkObjectGroupTableAfterMovingGC(NewObjectGroupTable &table)
{
    



    if (!table.initialized())
        return;

    for (NewObjectGroupTable::Enum e(table); !e.empty(); e.popFront()) {
        NewObjectGroupEntry entry = e.front();
        CheckGCThingAfterMovingGC(entry.group.get());
        TaggedProto proto = entry.group->proto();
        if (proto.isObject())
            CheckGCThingAfterMovingGC(proto.toObject());
        CheckGCThingAfterMovingGC(entry.associated);

        const Class *clasp = entry.group->clasp();
        if (entry.associated && entry.associated->is<JSFunction>())
            clasp = nullptr;

        NewObjectGroupTable::Lookup lookup(clasp, proto, entry.associated);
        NewObjectGroupTable::Ptr ptr = table.lookup(lookup);
        MOZ_ASSERT(ptr.found() && &*ptr == &e.front());
    }
}

#endif 

TypeCompartment::~TypeCompartment()
{
    js_delete(arrayTypeTable);
    js_delete(objectTypeTable);
    js_delete(allocationSiteTable);
}

 void
JSScript::maybeSweepTypes(AutoClearTypeInferenceStateOnOOM *oom)
{
    if (!types_ || typesGeneration() == zone()->types.generation)
        return;

    setTypesGeneration(zone()->types.generation);

    MOZ_ASSERT(zone()->isGCSweepingOrCompacting());
    MOZ_ASSERT(!zone()->runtimeFromMainThread()->isHeapMinorCollecting());

    Maybe<AutoClearTypeInferenceStateOnOOM> fallbackOOM;
    EnsureHasAutoClearTypeInferenceStateOnOOM(oom, zone(), fallbackOOM);

    TypeZone &types = zone()->types;

    
    
    
    if (types.sweepReleaseTypes &&
        !hasBaselineScript() &&
        !hasIonScript())
    {
        types_->destroy();
        types_ = nullptr;

        
        
        hasFreezeConstraints_ = false;

        return;
    }

    unsigned num = TypeScript::NumTypeSets(this);
    StackTypeSet *typeArray = types_->typeArray();

    
    for (unsigned i = 0; i < num; i++)
        typeArray[i].sweep(zone(), *oom);

    
    if (hasIonScript())
        ionScript()->recompileInfoRef().shouldSweep(types);
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
    if (jitZone()) {
        *baselineStubsOptimized +=
            jitZone()->optimizedStubSpace()->sizeOfExcludingThis(mallocSizeOf);
    }
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
ObjectGroup::sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
    size_t n = 0;
    if (TypeNewScript *newScript = newScriptDontCheckGeneration())
        n += newScript->sizeOfIncludingThis(mallocSizeOf);
    if (UnboxedLayout *layout = maybeUnboxedLayoutDontCheckGeneration())
        n += layout->sizeOfIncludingThis(mallocSizeOf);
    return n;
}

TypeZone::TypeZone(Zone *zone)
  : zone_(zone),
    typeLifoAlloc(TYPE_LIFO_ALLOC_PRIMARY_CHUNK_SIZE),
    generation(0),
    compilerOutputs(nullptr),
    sweepTypeLifoAlloc(TYPE_LIFO_ALLOC_PRIMARY_CHUNK_SIZE),
    sweepCompilerOutputs(nullptr),
    sweepReleaseTypes(false),
    activeAnalysis(nullptr)
{
}

TypeZone::~TypeZone()
{
    js_delete(compilerOutputs);
    js_delete(sweepCompilerOutputs);
}

void
TypeZone::beginSweep(FreeOp *fop, bool releaseTypes, AutoClearTypeInferenceStateOnOOM &oom)
{
    MOZ_ASSERT(zone()->isGCSweepingOrCompacting());
    MOZ_ASSERT(!sweepCompilerOutputs);
    MOZ_ASSERT(!sweepReleaseTypes);

    sweepReleaseTypes = releaseTypes;

    
    
    sweepTypeLifoAlloc.steal(&typeLifoAlloc);

    
    
    if (compilerOutputs) {
        CompilerOutputVector *newCompilerOutputs = nullptr;
        for (size_t i = 0; i < compilerOutputs->length(); i++) {
            CompilerOutput &output = (*compilerOutputs)[i];
            if (output.isValid()) {
                JSScript *script = output.script();
                if (IsScriptAboutToBeFinalized(&script)) {
                    script->ionScript()->recompileInfoRef() = RecompileInfo();
                    output.invalidate();
                } else {
                    CompilerOutput newOutput(script);

                    if (!newCompilerOutputs)
                        newCompilerOutputs = js_new<CompilerOutputVector>();
                    if (newCompilerOutputs && newCompilerOutputs->append(newOutput)) {
                        output.setSweepIndex(newCompilerOutputs->length() - 1);
                    } else {
                        oom.setOOM();
                        script->ionScript()->recompileInfoRef() = RecompileInfo();
                        output.invalidate();
                    }
                }
            }
        }
        sweepCompilerOutputs = compilerOutputs;
        compilerOutputs = newCompilerOutputs;
    }

    
    
    
    generation++;

    for (CompartmentsInZoneIter comp(zone()); !comp.done(); comp.next())
        comp->types.sweep(fop);
}

void
TypeZone::endSweep(JSRuntime *rt)
{
    js_delete(sweepCompilerOutputs);
    sweepCompilerOutputs = nullptr;

    sweepReleaseTypes = false;

    rt->gc.freeAllLifoBlocksAfterSweeping(&sweepTypeLifoAlloc);
}

void
TypeZone::clearAllNewScriptsOnOOM()
{
    for (gc::ZoneCellIterUnderGC iter(zone(), gc::FINALIZE_OBJECT_GROUP);
         !iter.done(); iter.next())
    {
        ObjectGroup *group = iter.get<ObjectGroup>();
        if (!IsObjectGroupAboutToBeFinalized(&group))
            group->maybeClearNewScriptOnOOM();
    }
}

AutoClearTypeInferenceStateOnOOM::~AutoClearTypeInferenceStateOnOOM()
{
    if (oom) {
        zone->setPreservingCode(false);
        zone->discardJitCode(zone->runtimeFromMainThread()->defaultFreeOp());
        zone->types.clearAllNewScriptsOnOOM();
    }
}

#ifdef DEBUG
void
TypeScript::printTypes(JSContext *cx, HandleScript script) const
{
    MOZ_ASSERT(script->types() == this);

    if (!script->hasBaselineScript())
        return;

    AutoEnterAnalysis enter(nullptr, script->zone());

    if (script->functionNonDelazifying())
        fprintf(stderr, "Function");
    else if (script->isForEval())
        fprintf(stderr, "Eval");
    else
        fprintf(stderr, "Main");
    fprintf(stderr, " #%u %s:%d ", script->id(), script->filename(), (int) script->lineno());

    if (script->functionNonDelazifying()) {
        if (js::PropertyName *name = script->functionNonDelazifying()->name())
            name->dumpCharsNoNewline();
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
        {
            fprintf(stderr, "#%u:", script->id());
            Sprinter sprinter(cx);
            if (!sprinter.init())
                return;
            js_Disassemble1(cx, script, pc, script->pcToOffset(pc), true, &sprinter);
            fprintf(stderr, "%s", sprinter.string());
        }

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
ObjectGroup::setAddendum(AddendumKind kind, void *addendum, bool writeBarrier )
{
    MOZ_ASSERT(!needsSweep());
    MOZ_ASSERT(kind <= (OBJECT_FLAG_ADDENDUM_MASK >> OBJECT_FLAG_ADDENDUM_SHIFT));

    if (writeBarrier) {
        
        
        if (newScript())
            TypeNewScript::writeBarrierPre(newScript());
        else
            MOZ_ASSERT(addendumKind() == Addendum_None || addendumKind() == kind);
    }

    flags_ &= ~OBJECT_FLAG_ADDENDUM_MASK;
    flags_ |= kind << OBJECT_FLAG_ADDENDUM_SHIFT;
    addendum_ = addendum;
}
