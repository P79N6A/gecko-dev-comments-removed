





#include "mozilla/DebugOnly.h"

#include "jsapi.h"
#include "jsautooplen.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsdate.h"
#include "jsexn.h"
#include "jsfriendapi.h"
#include "jsgc.h"
#include "jsinfer.h"
#include "jsmath.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsscript.h"
#include "jscntxt.h"
#include "jsstr.h"
#include "jsiter.h"
#include "jsworkers.h"

#ifdef JS_ION
#include "ion/Ion.h"
#include "ion/IonCompartment.h"
#endif
#include "frontend/TokenStream.h"
#include "gc/Marking.h"
#include "js/MemoryMetrics.h"
#include "methodjit/MethodJIT.h"
#include "methodjit/Retcon.h"
#include "vm/Shape.h"
#ifdef JS_METHODJIT
# include "assembler/assembler/MacroAssembler.h"
#endif

#include "jsatominlines.h"
#include "jsgcinlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "vm/Stack-inl.h"

#ifdef __SUNPRO_CC
#include <alloca.h>
#endif

using namespace js;
using namespace js::types;
using namespace js::analyze;

using mozilla::DebugOnly;

static inline jsid
id_prototype(JSContext *cx) {
    return NameToId(cx->names().classPrototype);
}

static inline jsid
id_length(JSContext *cx) {
    return NameToId(cx->names().length);
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
types::TypeIdStringImpl(RawId id)
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

#ifdef DEBUG

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
    
    static const char *colors[] = { "\x1b[31m", "\x1b[32m", "\x1b[33m",
                                    "\x1b[34m", "\x1b[35m", "\x1b[36m",
                                    "\x1b[37m" };
    if (!InferSpewColorable())
        return "";
    return colors[DefaultHasher<TypeConstraint *>::hash(constraint) % 7];
}

const char *
types::InferSpewColor(TypeSet *types)
{
    
    static const char *colors[] = { "\x1b[1;31m", "\x1b[1;32m", "\x1b[1;33m",
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
            JS_NOT_REACHED("Bad type");
            return "";
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
    fprintf(stdout, "[infer] ");
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    va_end(ap);
}

bool
types::TypeHasProperty(JSContext *cx, TypeObject *obj, jsid id, const Value &value)
{
    



    if (cx->typeInferenceEnabled() && !obj->unknownProperties() && !value.isUndefined()) {
        id = IdToTypeId(id);

        
        if (id == id___proto__(cx) || id == id_constructor(cx) || id == id_caller(cx))
            return true;

        




        if (cx->compartment->types.pendingCount)
            return true;

        Type type = GetValueType(cx, value);

        AutoEnterAnalysis enter(cx);

        




        TypeSet *types = obj->maybeGetProperty(id, cx);
        if (!types)
            return true;

        




        if (!types->hasPropagatedProperty())
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

    
    cx->compartment->types.print(cx, true);

    MOZ_ReportAssertionFailure(msgbuf, __FILE__, __LINE__);
    MOZ_CRASH();
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
TypeSet::isSubsetIgnorePrimitives(TypeSet *other)
{
    TypeFlags otherFlags = other->baseFlags() | TYPE_FLAG_PRIMITIVE;
    if ((baseFlags() & otherFlags) != baseFlags())
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

inline void
TypeSet::addTypesToConstraint(JSContext *cx, TypeConstraint *constraint)
{
    



    Vector<Type> types(cx);

    
    if (flags & TYPE_FLAG_UNKNOWN) {
        if (!types.append(Type::UnknownType()))
            cx->compartment->types.setPendingNukeTypes(cx);
    } else {
        
        for (TypeFlags flag = 1; flag < TYPE_FLAG_ANYOBJECT; flag <<= 1) {
            if (flags & flag) {
                Type type = Type::PrimitiveType(TypeFlagPrimitive(flag));
                if (!types.append(type))
                    cx->compartment->types.setPendingNukeTypes(cx);
            }
        }

        
        if (flags & TYPE_FLAG_ANYOBJECT) {
            if (!types.append(Type::AnyObjectType()))
                cx->compartment->types.setPendingNukeTypes(cx);
        } else {
            
            unsigned count = getObjectCount();
            for (unsigned i = 0; i < count; i++) {
                TypeObjectKey *object = getObject(i);
                if (object) {
                    if (!types.append(Type::ObjectType(object)))
                        cx->compartment->types.setPendingNukeTypes(cx);
                }
            }
        }
    }

    for (unsigned i = 0; i < types.length(); i++)
        constraint->newType(cx, this, types[i]);
}

inline void
TypeSet::add(JSContext *cx, TypeConstraint *constraint, bool callExisting)
{
    if (!constraint) {
        
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    JS_ASSERT(cx->compartment->activeAnalysis);

    InferSpew(ISpewOps, "addConstraint: %sT%p%s %sC%p%s %s",
              InferSpewColor(this), this, InferSpewColorReset(),
              InferSpewColor(constraint), constraint, InferSpewColorReset(),
              constraint->kind());

    JS_ASSERT(constraint->next == NULL);
    constraint->next = constraintList;
    constraintList = constraint;

    if (callExisting)
        addTypesToConstraint(cx, constraint);
}

void
TypeSet::print()
{
    if (flags & TYPE_FLAG_OWN_PROPERTY)
        printf(" [own]");
    if (flags & TYPE_FLAG_CONFIGURED_PROPERTY)
        printf(" [configured]");

    if (definiteProperty())
        printf(" [definite:%d]", definiteSlot());

    if (baseFlags() == 0 && !baseObjectCount()) {
        printf(" missing");
        return;
    }

    if (flags & TYPE_FLAG_UNKNOWN)
        printf(" unknown");
    if (flags & TYPE_FLAG_ANYOBJECT)
        printf(" object");

    if (flags & TYPE_FLAG_UNDEFINED)
        printf(" void");
    if (flags & TYPE_FLAG_NULL)
        printf(" null");
    if (flags & TYPE_FLAG_BOOLEAN)
        printf(" bool");
    if (flags & TYPE_FLAG_INT32)
        printf(" int");
    if (flags & TYPE_FLAG_DOUBLE)
        printf(" float");
    if (flags & TYPE_FLAG_STRING)
        printf(" string");
    if (flags & TYPE_FLAG_LAZYARGS)
        printf(" lazyargs");

    uint32_t objectCount = baseObjectCount();
    if (objectCount) {
        printf(" object[%u]", objectCount);

        unsigned count = getObjectCount();
        for (unsigned i = 0; i < count; i++) {
            TypeObjectKey *object = getObject(i);
            if (object)
                printf(" %s", TypeString(Type::ObjectType(object)));
        }
    }
}

StackTypeSet *
StackTypeSet::make(JSContext *cx, const char *name)
{
    JS_ASSERT(cx->compartment->activeAnalysis);

    StackTypeSet *res = cx->analysisLifoAlloc().new_<StackTypeSet>();
    if (!res) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return NULL;
    }

    InferSpew(ISpewOps, "typeSet: %sT%p%s intermediate %s",
              InferSpewColor(res), res, InferSpewColorReset(),
              name);
    res->setPurged();

    return res;
}

const StackTypeSet *
TypeSet::clone(LifoAlloc *alloc) const
{
    unsigned objectCount = baseObjectCount();
    unsigned capacity = (objectCount >= 2) ? HashSetCapacity(objectCount) : 0;

    StackTypeSet *res = alloc->new_<StackTypeSet>();
    if (!res)
        return NULL;

    TypeObjectKey **newSet;
    if (capacity) {
        newSet = alloc->newArray<TypeObjectKey*>(capacity);
        if (!newSet)
            return NULL;
        PodCopy(newSet, objectSet, capacity);
    }

    res->flags = this->flags;
    res->objectSet = capacity ? newSet : this->objectSet;

    return res;
}






class TypeConstraintSubset : public TypeConstraint
{
  public:
    TypeSet *target;

    TypeConstraintSubset(TypeSet *target)
        : target(target)
    {
        JS_ASSERT(target);
    }

    const char *kind() { return "subset"; }

    void newType(JSContext *cx, TypeSet *source, Type type)
    {
        
        target->addType(cx, type);
    }
};

void
StackTypeSet::addSubset(JSContext *cx, TypeSet *target)
{
    add(cx, cx->analysisLifoAlloc().new_<TypeConstraintSubset>(target));
}

void
HeapTypeSet::addSubset(JSContext *cx, TypeSet *target)
{
    JS_ASSERT(!target->purged());
    add(cx, cx->typeLifoAlloc().new_<TypeConstraintSubset>(target));
}

enum PropertyAccessKind {
    PROPERTY_WRITE,
    PROPERTY_READ,
    PROPERTY_READ_EXISTING
};


template <PropertyAccessKind access>
class TypeConstraintProp : public TypeConstraint
{
    RawScript script_;

  public:
    jsbytecode *pc;

    



    StackTypeSet *target;

    
    RawId id;

    TypeConstraintProp(RawScript script, jsbytecode *pc, StackTypeSet *target, RawId id)
        : script_(script), pc(pc), target(target), id(id)
    {
        JS_ASSERT(script && pc && target);
    }

    const char *kind() { return "prop"; }

    void newType(JSContext *cx, TypeSet *source, Type type);
};

typedef TypeConstraintProp<PROPERTY_WRITE> TypeConstraintSetProperty;
typedef TypeConstraintProp<PROPERTY_READ>  TypeConstraintGetProperty;
typedef TypeConstraintProp<PROPERTY_READ_EXISTING> TypeConstraintGetPropertyExisting;

void
StackTypeSet::addGetProperty(JSContext *cx, JSScript *script, jsbytecode *pc,
                             StackTypeSet *target, RawId id)
{
    



    JS_ASSERT(js_CodeSpec[*pc].format & JOF_INVOKE);

    add(cx, cx->analysisLifoAlloc().new_<TypeConstraintGetProperty>(script, pc, target, id));
}

void
StackTypeSet::addSetProperty(JSContext *cx, JSScript *script, jsbytecode *pc,
                             StackTypeSet *target, RawId id)
{
    add(cx, cx->analysisLifoAlloc().new_<TypeConstraintSetProperty>(script, pc, target, id));
}

void
HeapTypeSet::addGetProperty(JSContext *cx, JSScript *script, jsbytecode *pc,
                            StackTypeSet *target, RawId id)
{
    JS_ASSERT(!target->purged());
    add(cx, cx->typeLifoAlloc().new_<TypeConstraintGetProperty>(script, pc, target, id));
}








template <PropertyAccessKind access>
class TypeConstraintCallProp : public TypeConstraint
{
    RawScript script_;

  public:
    jsbytecode *callpc;

    
    jsid id;

    TypeConstraintCallProp(RawScript script, jsbytecode *callpc, jsid id)
        : script_(script), callpc(callpc), id(id)
    {
        JS_ASSERT(script && callpc);
    }

    const char *kind() { return "callprop"; }

    void newType(JSContext *cx, TypeSet *source, Type type);
};

typedef TypeConstraintCallProp<PROPERTY_READ> TypeConstraintCallProperty;
typedef TypeConstraintCallProp<PROPERTY_READ_EXISTING> TypeConstraintCallPropertyExisting;

void
HeapTypeSet::addCallProperty(JSContext *cx, JSScript *script, jsbytecode *pc, jsid id)
{
    




    jsbytecode *callpc = script->analysis()->getCallPC(pc);
    if (JSOp(*callpc) == JSOP_NEW)
        return;

    add(cx, cx->typeLifoAlloc().new_<TypeConstraintCallProperty>(script, callpc, id));
}







class TypeConstraintSetElement : public TypeConstraint
{
    RawScript script_;

  public:
    jsbytecode *pc;

    StackTypeSet *objectTypes;
    StackTypeSet *valueTypes;

    TypeConstraintSetElement(RawScript script, jsbytecode *pc,
                             StackTypeSet *objectTypes, StackTypeSet *valueTypes)
        : script_(script), pc(pc),
          objectTypes(objectTypes), valueTypes(valueTypes)
    {
        JS_ASSERT(script && pc);
    }

    const char *kind() { return "setelement"; }

    void newType(JSContext *cx, TypeSet *source, Type type);
};

void
StackTypeSet::addSetElement(JSContext *cx, JSScript *script, jsbytecode *pc,
                            StackTypeSet *objectTypes, StackTypeSet *valueTypes)
{
    add(cx, cx->analysisLifoAlloc().new_<TypeConstraintSetElement>(script, pc, objectTypes,
                                                                   valueTypes));
}






class TypeConstraintCall : public TypeConstraint
{
  public:
    
    TypeCallsite *callsite;

    TypeConstraintCall(TypeCallsite *callsite)
        : callsite(callsite)
    {}

    const char *kind() { return "call"; }

    void newType(JSContext *cx, TypeSet *source, Type type);
    bool newCallee(JSContext *cx, HandleFunction callee, HandleScript script);
};

void
StackTypeSet::addCall(JSContext *cx, TypeCallsite *site)
{
    add(cx, cx->analysisLifoAlloc().new_<TypeConstraintCall>(site));
}


class TypeConstraintArith : public TypeConstraint
{
    RawScript script_;

  public:
    jsbytecode *pc;

    
    TypeSet *target;

    
    TypeSet *other;

    TypeConstraintArith(RawScript script, jsbytecode *pc, TypeSet *target, TypeSet *other)
        : script_(script), pc(pc), target(target), other(other)
    {
        JS_ASSERT(target);
    }

    const char *kind() { return "arith"; }

    void newType(JSContext *cx, TypeSet *source, Type type);
};

void
StackTypeSet::addArith(JSContext *cx, JSScript *script, jsbytecode *pc, TypeSet *target,
                       TypeSet *other)
{
    add(cx, cx->analysisLifoAlloc().new_<TypeConstraintArith>(script, pc, target, other));
}


class TypeConstraintTransformThis : public TypeConstraint
{
    RawScript script_;

  public:
    TypeSet *target;

    TypeConstraintTransformThis(RawScript script, TypeSet *target)
        : script_(script), target(target)
    {}

    const char *kind() { return "transformthis"; }

    void newType(JSContext *cx, TypeSet *source, Type type);
};

void
StackTypeSet::addTransformThis(JSContext *cx, JSScript *script, TypeSet *target)
{
    add(cx, cx->analysisLifoAlloc().new_<TypeConstraintTransformThis>(script, target));
}





class TypeConstraintPropagateThis : public TypeConstraint
{
    RawScript script_;

  public:
    jsbytecode *callpc;
    Type type;
    StackTypeSet *types;

    TypeConstraintPropagateThis(RawScript script, jsbytecode *callpc, Type type, StackTypeSet *types)
        : script_(script), callpc(callpc), type(type), types(types)
    {}

    const char *kind() { return "propagatethis"; }

    void newType(JSContext *cx, TypeSet *source, Type type);
    bool newCallee(JSContext *cx, HandleFunction callee);
};

void
StackTypeSet::addPropagateThis(JSContext *cx, JSScript *script, jsbytecode *pc,
                               Type type, StackTypeSet *types)
{
    add(cx, cx->analysisLifoAlloc().new_<TypeConstraintPropagateThis>(script, pc, type, types));
}


class TypeConstraintFilterPrimitive : public TypeConstraint
{
  public:
    TypeSet *target;

    TypeConstraintFilterPrimitive(TypeSet *target)
        : target(target)
    {}

    const char *kind() { return "filter"; }

    void newType(JSContext *cx, TypeSet *source, Type type)
    {
        if (type.isPrimitive())
            return;

        target->addType(cx, type);
    }
};

void
HeapTypeSet::addFilterPrimitives(JSContext *cx, TypeSet *target)
{
    add(cx, cx->typeLifoAlloc().new_<TypeConstraintFilterPrimitive>(target));
}


static inline RawShape
GetSingletonShape(JSContext *cx, RawObject obj, RawId idArg)
{
    if (!obj->isNative())
        return NULL;
    RootedId id(cx, idArg);
    RawShape shape = obj->nativeLookup(cx, id);
    if (shape && shape->hasDefaultGetter() && shape->hasSlot())
        return shape;
    return NULL;
}

void
ScriptAnalysis::pruneTypeBarriers(JSContext *cx, uint32_t offset)
{
    TypeBarrier **pbarrier = &getCode(offset).typeBarriers;
    while (*pbarrier) {
        TypeBarrier *barrier = *pbarrier;
        if (barrier->target->hasType(barrier->type)) {
            
            *pbarrier = barrier->next;
            continue;
        }
        if (barrier->singleton) {
            JS_ASSERT(barrier->type.isPrimitive(JSVAL_TYPE_UNDEFINED));
            RawShape shape = GetSingletonShape(cx, barrier->singleton, barrier->singletonId);
            if (shape && !barrier->singleton->nativeGetSlot(shape->slot()).isUndefined()) {
                








                *pbarrier = barrier->next;
                continue;
            }
        }
        pbarrier = &barrier->next;
    }
}






static const uint32_t BARRIER_OBJECT_LIMIT = 10;

void ScriptAnalysis::breakTypeBarriers(JSContext *cx, uint32_t offset, bool all)
{
    pruneTypeBarriers(cx, offset);

    bool resetResolving = !cx->compartment->types.resolving;
    if (resetResolving)
        cx->compartment->types.resolving = true;

    TypeBarrier **pbarrier = &getCode(offset).typeBarriers;
    while (*pbarrier) {
        TypeBarrier *barrier = *pbarrier;
        if (barrier->target->hasType(barrier->type) ) {
            




            *pbarrier = barrier->next;
        } else if (all) {
            
            barrier->target->addType(cx, barrier->type);
            *pbarrier = barrier->next;
        } else if (!barrier->type.isUnknown() &&
                   !barrier->type.isAnyObject() &&
                   barrier->type.isObject() &&
                   barrier->target->getObjectCount() >= BARRIER_OBJECT_LIMIT) {
            
            barrier->target->addType(cx, barrier->type);
            *pbarrier = barrier->next;
        } else {
            pbarrier = &barrier->next;
        }
    }

    if (resetResolving) {
        cx->compartment->types.resolving = false;
        cx->compartment->types.resolvePending(cx);
    }
}

void ScriptAnalysis::breakTypeBarriersSSA(JSContext *cx, const SSAValue &v)
{
    if (v.kind() != SSAValue::PUSHED)
        return;

    uint32_t offset = v.pushedOffset();
    if (JSOp(script_->code[offset]) == JSOP_GETPROP)
        breakTypeBarriersSSA(cx, poppedValue(offset, 0));

    breakTypeBarriers(cx, offset, true);
}





class TypeConstraintSubsetBarrier : public TypeConstraint
{
  public:
    RawScript script;
    jsbytecode *pc;
    TypeSet *target;

    TypeConstraintSubsetBarrier(RawScript script, jsbytecode *pc, TypeSet *target)
        : script(script), pc(pc), target(target)
    {}

    const char *kind() { return "subsetBarrier"; }

    void newType(JSContext *cx, TypeSet *source, Type type)
    {
        if (!target->hasType(type)) {
            if (!script->ensureRanAnalysis(cx))
                return;
            script->analysis()->addTypeBarrier(cx, pc, target, type);
        }
    }
};

void
StackTypeSet::addSubsetBarrier(JSContext *cx, JSScript *script, jsbytecode *pc, TypeSet *target)
{
    add(cx, cx->analysisLifoAlloc().new_<TypeConstraintSubsetBarrier>(script, pc, target));
}

void
HeapTypeSet::addSubsetBarrier(JSContext *cx, JSScript *script, jsbytecode *pc, TypeSet *target)
{
    JS_ASSERT(!target->purged());
    add(cx, cx->typeLifoAlloc().new_<TypeConstraintSubsetBarrier>(script, pc, target));
}






static inline TypeObject *
GetPropertyObject(JSContext *cx, HandleScript script, Type type)
{
    if (type.isTypeObject())
        return type.typeObject();

    
    if (type.isSingleObject())
        return type.singleObject()->getType(cx);

    



    TypeObject *object = NULL;
    switch (type.primitive()) {

      case JSVAL_TYPE_INT32:
      case JSVAL_TYPE_DOUBLE:
        object = TypeScript::StandardType(cx, JSProto_Number);
        break;

      case JSVAL_TYPE_BOOLEAN:
        object = TypeScript::StandardType(cx, JSProto_Boolean);
        break;

      case JSVAL_TYPE_STRING:
        object = TypeScript::StandardType(cx, JSProto_String);
        break;

      default:
        
        return NULL;
    }

    if (!object)
        cx->compartment->types.setPendingNukeTypes(cx);
    return object;
}

static inline bool
UsePropertyTypeBarrier(jsbytecode *pc)
{
    



    uint32_t format = js_CodeSpec[*pc].format;
    return (format & JOF_TYPESET) && !(format & JOF_INVOKE);
}

static inline void
MarkPropertyAccessUnknown(JSContext *cx, JSScript *script, jsbytecode *pc, TypeSet *target)
{
    if (UsePropertyTypeBarrier(pc))
        script->analysis()->addTypeBarrier(cx, pc, target, Type::UnknownType());
    else
        target->addType(cx, Type::UnknownType());
}






static inline Type
GetSingletonPropertyType(JSContext *cx, RawObject rawObjArg, HandleId id)
{
    RootedObject obj(cx, rawObjArg);    

    JS_ASSERT(id == IdToTypeId(id));

    if (JSID_IS_VOID(id))
        return Type::UnknownType();

    if (obj->isTypedArray()) {
        if (id == id_length(cx))
            return Type::Int32Type();
        obj = obj->getProto();
    }

    while (obj) {
        if (!obj->isNative())
            return Type::UnknownType();

        RootedValue v(cx);
        if (HasDataProperty(cx, obj, id, v.address())) {
            if (v.isUndefined())
                return Type::UnknownType();
            return GetValueType(cx, v);
        }

        obj = obj->getProto();
    }

    return Type::UnknownType();
}





template <PropertyAccessKind access>
static inline void
PropertyAccess(JSContext *cx, JSScript *script, jsbytecode *pc, TypeObject *object,
               StackTypeSet *target, RawId idArg)
{
    RootedId id(cx, idArg);

    
    if (object->unknownProperties()) {
        if (access != PROPERTY_WRITE)
            MarkPropertyAccessUnknown(cx, script, pc, target);
        return;
    }

    






    if (object->singleton && object->singleton->isTypedArray() && JSID_IS_VOID(id)) {
        if (access != PROPERTY_WRITE) {
            int arrayKind = object->proto->getClass() - TypedArray::protoClasses;
            JS_ASSERT(arrayKind >= 0 && arrayKind < TypedArray::TYPE_MAX);

            bool maybeDouble = (arrayKind == TypedArray::TYPE_FLOAT32 ||
                                arrayKind == TypedArray::TYPE_FLOAT64);
            target->addType(cx, maybeDouble ? Type::DoubleType() : Type::Int32Type());
        }
        return;
    }

    





    bool markOwn = access == PROPERTY_WRITE && JSID_IS_VOID(id);
    HeapTypeSet *types = object->getProperty(cx, id, markOwn);
    if (!types)
        return;

    







    if (access != PROPERTY_WRITE) {
        RootedObject singleton(cx, object->singleton);

        



        if (!singleton && !types->ownProperty(false))
            singleton = object->proto;

        if (singleton) {
            Type type = GetSingletonPropertyType(cx, singleton, id);
            if (!type.isUnknown())
                target->addType(cx, type);
        }
    }

    
    if (access == PROPERTY_WRITE) {
        target->addSubset(cx, types);
    } else {
        JS_ASSERT_IF(script->hasAnalysis(),
                     target == script->analysis()->bytecodeTypes(pc));
        if (!types->hasPropagatedProperty())
            object->getFromPrototypes(cx, id, types);
        if (UsePropertyTypeBarrier(pc)) {
            if (access == PROPERTY_READ) {
                types->addSubsetBarrier(cx, script, pc, target);
            } else {
                TypeConstraintSubsetBarrier constraint(script, pc, target);
                types->addTypesToConstraint(cx, &constraint);
            }
            if (object->singleton && !JSID_IS_VOID(id)) {
                





                RootedObject singleton(cx, object->singleton);
                RootedShape shape(cx, GetSingletonShape(cx, singleton, id));
                if (shape && singleton->nativeGetSlot(shape->slot()).isUndefined())
                    script->analysis()->addSingletonTypeBarrier(cx, pc, target, singleton, id);
            }
        } else {
            JS_ASSERT(access == PROPERTY_READ);
            types->addSubset(cx, target);
        }
    }
}


static inline bool
UnknownPropertyAccess(HandleScript script, Type type)
{
    return type.isUnknown()
        || type.isAnyObject()
        || (!type.isObject() && !script->compileAndGo);
}

template <PropertyAccessKind access>
void
TypeConstraintProp<access>::newType(JSContext *cx, TypeSet *source, Type type)
{
    RootedScript script(cx, script_);

    if (UnknownPropertyAccess(script, type)) {
        



        if (access == PROPERTY_WRITE)
            cx->compartment->types.monitorBytecode(cx, script, pc - script->code);
        else
            MarkPropertyAccessUnknown(cx, script, pc, target);
        return;
    }

    if (type.isPrimitive(JSVAL_TYPE_MAGIC)) {
        
        if (access == PROPERTY_WRITE || (id != JSID_VOID && id != id_length(cx)))
            return;

        if (id == JSID_VOID)
            MarkPropertyAccessUnknown(cx, script, pc, target);
        else
            target->addType(cx, Type::Int32Type());
        return;
    }

    TypeObject *object = GetPropertyObject(cx, script, type);
    if (object)
        PropertyAccess<access>(cx, script, pc, object, target, id);
}

template <PropertyAccessKind access>
void
TypeConstraintCallProp<access>::newType(JSContext *cx, TypeSet *source, Type type)
{
    RootedScript script(cx, script_);

    






    if (UnknownPropertyAccess(script, type)) {
        cx->compartment->types.monitorBytecode(cx, script, callpc - script->code);
        return;
    }

    TypeObject *object = GetPropertyObject(cx, script, type);
    if (object) {
        if (object->unknownProperties()) {
            cx->compartment->types.monitorBytecode(cx, script, callpc - script->code);
        } else {
            TypeSet *types = object->getProperty(cx, id, false);
            if (!types)
                return;
            if (!types->hasPropagatedProperty())
                object->getFromPrototypes(cx, id, types);
            if (access == PROPERTY_READ) {
                types->add(cx, cx->typeLifoAlloc().new_<TypeConstraintPropagateThis>(
                                script_, callpc, type, (StackTypeSet *) NULL));
            } else {
                TypeConstraintPropagateThis constraint(script, callpc, type, NULL);
                types->addTypesToConstraint(cx, &constraint);
            }
        }
    }
}

void
TypeConstraintSetElement::newType(JSContext *cx, TypeSet *source, Type type)
{
    RootedScript script(cx, script_);
    if (type.isUnknown() ||
        type.isPrimitive(JSVAL_TYPE_INT32) ||
        type.isPrimitive(JSVAL_TYPE_DOUBLE)) {
        objectTypes->addSetProperty(cx, script, pc, valueTypes, JSID_VOID);
    }
}

static inline RawFunction
CloneCallee(JSContext *cx, HandleFunction fun, HandleScript script, jsbytecode *pc)
{
    



    RawFunction callee = CloneFunctionAtCallsite(cx, fun, script, pc);
    if (!callee)
        return NULL;

    InferSpew(ISpewOps, "callsiteCloneType: #%u:%05u: %s",
              script->id(), pc - script->code, TypeString(Type::ObjectType(callee)));

    return callee;
}

void
TypeConstraintCall::newType(JSContext *cx, TypeSet *source, Type type)
{
    RootedScript script(cx, callsite->script);
    jsbytecode *pc = callsite->pc;

    JS_ASSERT_IF(script->hasAnalysis(),
                 callsite->returnTypes == script->analysis()->bytecodeTypes(pc));

    if (type.isUnknown() || type.isAnyObject()) {
        
        cx->compartment->types.monitorBytecode(cx, script, pc - script->code);
        return;
    }

    RootedFunction callee(cx);

    if (type.isSingleObject()) {
        RootedObject obj(cx, type.singleObject());

        if (!obj->isFunction()) {
            
            return;
        }

        if (obj->toFunction()->isNative()) {
            





            cx->compartment->types.monitorBytecode(cx, script, pc - script->code, true);

            






            Native native = obj->toFunction()->native();

            if (native == js::array_push) {
                for (size_t i = 0; i < callsite->argumentCount; i++) {
                    callsite->thisTypes->addSetProperty(cx, script, pc,
                                                        callsite->argumentTypes[i], JSID_VOID);
                }
            }

            if (native == js::array_pop || native == js::array_shift)
                callsite->thisTypes->addGetProperty(cx, script, pc, callsite->returnTypes, JSID_VOID);

            if (native == js_Array) {
                TypeObject *res = TypeScript::InitObject(cx, script, pc, JSProto_Array);
                if (!res)
                    return;

                callsite->returnTypes->addType(cx, Type::ObjectType(res));

                if (callsite->argumentCount >= 2) {
                    for (unsigned i = 0; i < callsite->argumentCount; i++) {
                        PropertyAccess<PROPERTY_WRITE>(cx, script, pc, res,
                                                       callsite->argumentTypes[i], JSID_VOID);
                    }
                }
            }

            if (native == js_String && callsite->isNew) {
                
                
                TypeObject *res = TypeScript::StandardType(cx, JSProto_String);
                if (!res)
                    return;

                callsite->returnTypes->addType(cx, Type::ObjectType(res));
            }

            return;
        }

        callee = obj->toFunction();
    } else if (type.isTypeObject()) {
        callee = type.typeObject()->interpretedFunction;
        if (!callee)
            return;
    } else {
        
        return;
    }

    if (callee->isInterpretedLazy() && !callee->getOrCreateScript(cx))
        return;

    



    if (callee->nonLazyScript()->shouldCloneAtCallsite) {
        RootedFunction clone(cx, CloneCallee(cx, callee, script, pc));
        if (!clone)
            return;
        if (!newCallee(cx, clone, script))
            return;
        if (!newCallee(cx, callee, script))
            return;

        




        for (unsigned i = 0; i < callsite->argumentCount && i < callee->nargs; i++) {
            StackTypeSet *cloneTypes = TypeScript::ArgTypes(clone->nonLazyScript(), i);
            StackTypeSet *originalTypes = TypeScript::ArgTypes(callee->nonLazyScript(), i);
            cloneTypes->addSubset(cx, originalTypes);
        }
    } else {
        newCallee(cx, callee, script);
    }
}

bool
TypeConstraintCall::newCallee(JSContext *cx, HandleFunction callee, HandleScript script)
{
    RootedScript calleeScript(cx, callee->nonLazyScript());
    if (!calleeScript->ensureHasTypes(cx))
        return false;

    unsigned nargs = callee->nargs;

    
    for (unsigned i = 0; i < callsite->argumentCount && i < nargs; i++) {
        StackTypeSet *argTypes = callsite->argumentTypes[i];
        StackTypeSet *types = TypeScript::ArgTypes(calleeScript, i);
        argTypes->addSubsetBarrier(cx, script, callsite->pc, types);
    }

    
    for (unsigned i = callsite->argumentCount; i < nargs; i++) {
        TypeSet *types = TypeScript::ArgTypes(calleeScript, i);
        types->addType(cx, Type::UndefinedType());
    }

    StackTypeSet *thisTypes = TypeScript::ThisTypes(calleeScript);
    HeapTypeSet *returnTypes = TypeScript::ReturnTypes(calleeScript);

    if (callsite->isNew) {
        





        thisTypes->addSubset(cx, returnTypes);
        returnTypes->addFilterPrimitives(cx, callsite->returnTypes);
    } else {
        







        returnTypes->addSubset(cx, callsite->returnTypes);
    }

    return true;
}

void
TypeConstraintPropagateThis::newType(JSContext *cx, TypeSet *source, Type type)
{
    RootedScript script(cx, script_);
    if (type.isUnknown() || type.isAnyObject()) {
        





        cx->compartment->types.monitorBytecode(cx, script, callpc - script->code);
        return;
    }

    
    RootedFunction callee(cx);

    if (type.isSingleObject()) {
        RootedObject object(cx, type.singleObject());
        if (!object->isFunction() || !object->toFunction()->isInterpreted())
            return;
        callee = object->toFunction();
    } else if (type.isTypeObject()) {
        TypeObject *object = type.typeObject();
        if (!object->interpretedFunction)
            return;
        callee = object->interpretedFunction;
    } else {
        
        return;
    }

    if (callee->isInterpretedLazy() && !callee->getOrCreateScript(cx))
        return;

    



    if (callee->nonLazyScript()->shouldCloneAtCallsite) {
        RootedFunction clone(cx, CloneCallee(cx, callee, script, callpc));
        if (!clone)
            return;
        if (!newCallee(cx, clone))
            return;
    }

    newCallee(cx, callee);
}

bool
TypeConstraintPropagateThis::newCallee(JSContext *cx, HandleFunction callee)
{
    if (!callee->nonLazyScript()->ensureHasTypes(cx))
        return false;

    TypeSet *thisTypes = TypeScript::ThisTypes(callee->nonLazyScript());
    if (this->types)
        this->types->addSubset(cx, thisTypes);
    else
        thisTypes->addType(cx, this->type);

    return true;
}

void
TypeConstraintArith::newType(JSContext *cx, TypeSet *source, Type type)
{
    







    RootedScript script(cx, script_);
    if (other) {
        





        if (type.isUnknown() || other->unknown()) {
            target->addType(cx, Type::UnknownType());
        } else if (type.isPrimitive(JSVAL_TYPE_DOUBLE)) {
            if (other->hasAnyFlag(TYPE_FLAG_UNDEFINED | TYPE_FLAG_NULL |
                                  TYPE_FLAG_INT32 | TYPE_FLAG_DOUBLE | TYPE_FLAG_BOOLEAN |
                                  TYPE_FLAG_ANYOBJECT)) {
                target->addType(cx, Type::DoubleType());
            } else if (other->getObjectCount() != 0) {
                TypeDynamicResult(cx, script, pc, Type::DoubleType());
            }
        } else if (type.isPrimitive(JSVAL_TYPE_STRING)) {
            target->addType(cx, Type::StringType());
        } else if (other->hasAnyFlag(TYPE_FLAG_DOUBLE)) {
            target->addType(cx, Type::DoubleType());
        } else if (other->hasAnyFlag(TYPE_FLAG_UNDEFINED | TYPE_FLAG_NULL |
                                     TYPE_FLAG_INT32 | TYPE_FLAG_BOOLEAN |
                                     TYPE_FLAG_ANYOBJECT)) {
            target->addType(cx, Type::Int32Type());
        } else if (other->getObjectCount() != 0) {
            TypeDynamicResult(cx, script, pc, Type::Int32Type());
        }
    } else {
        if (type.isUnknown())
            target->addType(cx, Type::UnknownType());
        else if (type.isPrimitive(JSVAL_TYPE_DOUBLE))
            target->addType(cx, Type::DoubleType());
        else if (!type.isAnyObject() && type.isObject())
            TypeDynamicResult(cx, script, pc, Type::Int32Type());
        else
            target->addType(cx, Type::Int32Type());
    }
}

void
TypeConstraintTransformThis::newType(JSContext *cx, TypeSet *source, Type type)
{
    if (type.isUnknown() || type.isAnyObject() || type.isObject() || script_->strict) {
        target->addType(cx, type);
        return;
    }

    RootedScript script(cx, script_);

    



    if (script->function() && script->function()->isSelfHostedBuiltin()) {
        target->addType(cx, type);
        return;
    }

    



    if (!script->compileAndGo ||
        type.isPrimitive(JSVAL_TYPE_NULL) ||
        type.isPrimitive(JSVAL_TYPE_UNDEFINED)) {
        target->addType(cx, Type::UnknownType());
        return;
    }

    TypeObject *object = NULL;
    switch (type.primitive()) {
      case JSVAL_TYPE_INT32:
      case JSVAL_TYPE_DOUBLE:
        object = TypeScript::StandardType(cx, JSProto_Number);
        break;
      case JSVAL_TYPE_BOOLEAN:
        object = TypeScript::StandardType(cx, JSProto_Boolean);
        break;
      case JSVAL_TYPE_STRING:
        object = TypeScript::StandardType(cx, JSProto_String);
        break;
      default:
        return;
    }

    if (!object) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    target->addType(cx, Type::ObjectType(object));
}






class TypeConstraintFreeze : public TypeConstraint
{
  public:
    RecompileInfo info;

    
    bool typeAdded;

    TypeConstraintFreeze(RecompileInfo info)
        : info(info), typeAdded(false)
    {}

    const char *kind() { return "freeze"; }

    void newType(JSContext *cx, TypeSet *source, Type type)
    {
        if (typeAdded)
            return;

        typeAdded = true;
        cx->compartment->types.addPendingRecompile(cx, info);
    }
};

void
HeapTypeSet::addFreeze(JSContext *cx)
{
    add(cx, cx->typeLifoAlloc().new_<TypeConstraintFreeze>(
                cx->compartment->types.compiledInfo), false);
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
StackTypeSet::getKnownTypeTag()
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
HeapTypeSet::getKnownTypeTag(JSContext *cx)
{
    TypeFlags flags = baseFlags();
    JSValueType type;

    if (baseObjectCount())
        type = flags ? JSVAL_TYPE_UNKNOWN : JSVAL_TYPE_OBJECT;
    else
        type = GetValueTypeFromTypeFlags(flags);

    if (type != JSVAL_TYPE_UNKNOWN)
        addFreeze(cx);

    






    DebugOnly<bool> empty = flags == 0 && baseObjectCount() == 0;
    JS_ASSERT_IF(empty, type == JSVAL_TYPE_UNKNOWN);

    return type;
}


class TypeConstraintFreezeObjectFlags : public TypeConstraint
{
  public:
    RecompileInfo info;

    
    TypeObjectFlags flags;

    
    bool marked;

    TypeConstraintFreezeObjectFlags(RecompileInfo info, TypeObjectFlags flags)
        : info(info), flags(flags),
          marked(false)
    {}

    const char *kind() { return "freezeObjectFlags"; }

    void newType(JSContext *cx, TypeSet *source, Type type) {}

    void newObjectState(JSContext *cx, TypeObject *object, bool force)
    {
        if (!marked && (object->hasAnyFlags(flags) || (!flags && force))) {
            marked = true;
            cx->compartment->types.addPendingRecompile(cx, info);
        }
    }
};

bool
StackTypeSet::hasObjectFlags(JSContext *cx, TypeObjectFlags flags)
{
    if (unknownObject())
        return true;

    



    if (baseObjectCount() == 0)
        return true;

    RootedObject obj(cx);
    unsigned count = getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        TypeObject *object = getTypeObject(i);
        if (!object) {
            if (!(obj = getSingleObject(i)))
                continue;
            if (!(object = obj->getType(cx)))
                return true;
        }
        if (object->hasAnyFlags(flags))
            return true;

        



        TypeSet *types = object->getProperty(cx, JSID_EMPTY, false);
        if (!types)
            return true;
        types->add(cx, cx->typeLifoAlloc().new_<TypeConstraintFreezeObjectFlags>(
                          cx->compartment->types.compiledInfo, flags), false);
    }

    return false;
}

bool
HeapTypeSet::HasObjectFlags(JSContext *cx, TypeObject *object, TypeObjectFlags flags)
{
    if (object->hasAnyFlags(flags))
        return true;

    HeapTypeSet *types = object->getProperty(cx, JSID_EMPTY, false);
    if (!types)
        return true;
    types->add(cx, cx->typeLifoAlloc().new_<TypeConstraintFreezeObjectFlags>(
                      cx->compartment->types.compiledInfo, flags), false);
    return false;
}

static inline void
ObjectStateChange(JSContext *cx, TypeObject *object, bool markingUnknown, bool force)
{
    if (object->unknownProperties())
        return;

    
    TypeSet *types = object->maybeGetProperty(JSID_EMPTY, cx);

    
    if (markingUnknown)
        object->flags |= OBJECT_FLAG_DYNAMIC_MASK | OBJECT_FLAG_UNKNOWN_PROPERTIES;

    if (types) {
        TypeConstraint *constraint = types->constraintList;
        while (constraint) {
            constraint->newObjectState(cx, object, force);
            constraint = constraint->next;
        }
    }
}

void
HeapTypeSet::WatchObjectStateChange(JSContext *cx, TypeObject *obj)
{
    JS_ASSERT(!obj->unknownProperties());
    HeapTypeSet *types = obj->getProperty(cx, JSID_EMPTY, false);
    if (!types)
        return;

    



    types->add(cx, cx->typeLifoAlloc().new_<TypeConstraintFreezeObjectFlags>(
                     cx->compartment->types.compiledInfo,
                     0));
}

class TypeConstraintFreezeOwnProperty : public TypeConstraint
{
  public:
    RecompileInfo info;

    bool updated;
    bool configurable;

    TypeConstraintFreezeOwnProperty(RecompileInfo info, bool configurable)
        : info(info), updated(false), configurable(configurable)
    {}

    const char *kind() { return "freezeOwnProperty"; }

    void newType(JSContext *cx, TypeSet *source, Type type) {}

    void newPropertyState(JSContext *cx, TypeSet *source)
    {
        if (updated)
            return;
        if (source->ownProperty(configurable)) {
            updated = true;
            cx->compartment->types.addPendingRecompile(cx, info);
        }
    }
};

static void
CheckNewScriptProperties(JSContext *cx, HandleTypeObject type, HandleFunction fun);

bool
HeapTypeSet::isOwnProperty(JSContext *cx, TypeObject *object, bool configurable)
{
    





    if (object->flags & OBJECT_FLAG_NEW_SCRIPT_REGENERATE) {
        if (object->newScript) {
            Rooted<TypeObject*> typeObj(cx, object);
            RootedFunction fun(cx, object->newScript->fun);
            CheckNewScriptProperties(cx, typeObj, fun);
        } else {
            JS_ASSERT(object->flags & OBJECT_FLAG_NEW_SCRIPT_CLEARED);
            object->flags &= ~OBJECT_FLAG_NEW_SCRIPT_REGENERATE;
        }
    }

    if (ownProperty(configurable))
        return true;

    add(cx, cx->typeLifoAlloc().new_<TypeConstraintFreezeOwnProperty>(
                                                      cx->compartment->types.compiledInfo,
                                                      configurable), false);
    return false;
}

bool
HeapTypeSet::knownNonEmpty(JSContext *cx)
{
    if (baseFlags() != 0 || baseObjectCount() != 0)
        return true;

    addFreeze(cx);

    return false;
}

bool
StackTypeSet::knownNonStringPrimitive()
{
    TypeFlags flags = baseFlags();

    if (baseObjectCount() > 0)
        return false;

    if (flags >= TYPE_FLAG_STRING)
        return false;

    if (baseFlags() == 0)
        return false;
    return true;
}

bool
StackTypeSet::filtersType(const StackTypeSet *other, Type filteredType) const
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

StackTypeSet::DoubleConversion
StackTypeSet::convertDoubleElements(JSContext *cx)
{
    if (unknownObject() || !getObjectCount())
        return AmbiguousDoubleConversion;

    bool alwaysConvert = true;
    bool maybeConvert = false;
    bool dontConvert = false;

    for (unsigned i = 0; i < getObjectCount(); i++) {
        TypeObject *type = getTypeObject(i);
        if (!type) {
            if (JSObject *obj = getSingleObject(i)) {
                type = obj->getType(cx);
                if (!type)
                    return AmbiguousDoubleConversion;
            } else {
                continue;
            }
        }

        if (type->unknownProperties()) {
            alwaysConvert = false;
            continue;
        }

        HeapTypeSet *types = type->getProperty(cx, JSID_VOID, false);
        if (!types)
            return AmbiguousDoubleConversion;

        types->addFreeze(cx);

        
        
        
        
        if (!types->hasType(Type::DoubleType()) || type->clasp != &ArrayClass) {
            dontConvert = true;
            alwaysConvert = false;
            continue;
        }

        
        
        
        if (types->getKnownTypeTag(cx) == JSVAL_TYPE_DOUBLE &&
            !HeapTypeSet::HasObjectFlags(cx, type, OBJECT_FLAG_NON_PACKED))
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

bool
HeapTypeSet::knownSubset(JSContext *cx, TypeSet *other)
{
    JS_ASSERT(!other->constraintsPurged());

    if (!isSubset(other))
        return false;

    addFreeze(cx);

    return true;
}

Class *
StackTypeSet::getKnownClass()
{
    if (unknownObject())
        return NULL;

    Class *clasp = NULL;
    unsigned count = getObjectCount();

    for (unsigned i = 0; i < count; i++) {
        Class *nclasp;
        if (RawObject object = getSingleObject(i))
            nclasp = object->getClass();
        else if (TypeObject *object = getTypeObject(i))
            nclasp = object->clasp;
        else
            continue;

        if (clasp && clasp != nclasp)
            return NULL;
        clasp = nclasp;
    }

    return clasp;
}

int
StackTypeSet::getTypedArrayType()
{
    Class *clasp = getKnownClass();

    if (clasp && IsTypedArrayClass(clasp))
        return clasp - &TypedArray::classes[0];
    return TypedArray::TYPE_MAX;
}

bool
StackTypeSet::isDOMClass()
{
    if (unknownObject())
        return false;

    unsigned count = getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        Class *clasp;
        if (RawObject object = getSingleObject(i))
            clasp = object->getClass();
        else if (TypeObject *object = getTypeObject(i))
            clasp = object->clasp;
        else
            continue;

        if (!(clasp->flags & JSCLASS_IS_DOMJSCLASS))
            return false;
    }

    return true;
}

JSObject *
StackTypeSet::getCommonPrototype()
{
    if (unknownObject())
        return NULL;

    JSObject *proto = NULL;
    unsigned count = getObjectCount();

    for (unsigned i = 0; i < count; i++) {
        TaggedProto nproto;
        if (RawObject object = getSingleObject(i))
            nproto = object->getProto();
        else if (TypeObject *object = getTypeObject(i))
            nproto = object->proto.get();
        else
            continue;

        if (proto) {
            if (nproto != proto)
                return NULL;
        } else {
            if (!nproto.isObject())
                return NULL;
            proto = nproto.toObject();
        }
    }

    return proto;
}

JSObject *
StackTypeSet::getSingleton()
{
    if (baseFlags() != 0 || baseObjectCount() != 1)
        return NULL;

    return getSingleObject(0);
}

JSObject *
HeapTypeSet::getSingleton(JSContext *cx)
{
    if (baseFlags() != 0 || baseObjectCount() != 1)
        return NULL;

    RootedObject obj(cx, getSingleObject(0));

    if (obj)
        addFreeze(cx);

    return obj;
}

bool
HeapTypeSet::needsBarrier(JSContext *cx)
{
    bool result = unknownObject()
               || getObjectCount() > 0
               || hasAnyFlag(TYPE_FLAG_STRING);
    if (!result)
        addFreeze(cx);
    return result;
}

bool
StackTypeSet::propertyNeedsBarrier(JSContext *cx, RawId id)
{
    RootedId typeId(cx, IdToTypeId(id));

    if (unknownObject())
        return true;

    for (unsigned i = 0; i < getObjectCount(); i++) {
        if (getSingleObject(i))
            return true;

        if (types::TypeObject *otype = getTypeObject(i)) {
            if (otype->unknownProperties())
                return true;

            if (types::HeapTypeSet *propTypes = otype->maybeGetProperty(typeId, cx)) {
                if (propTypes->needsBarrier(cx))
                    return true;
            }
        }
    }

    return false;
}

enum RecompileKind {
    RECOMPILE_CHECK_MONITORED,
    RECOMPILE_CHECK_BARRIERS,
    RECOMPILE_NONE
};








static inline bool
JITCodeHasCheck(RawScript script, jsbytecode *pc, RecompileKind kind)
{
    if (kind == RECOMPILE_NONE)
        return false;

#ifdef JS_METHODJIT
    for (int constructing = 0; constructing <= 1; constructing++) {
        for (int barriers = 0; barriers <= 1; barriers++) {
            mjit::JITScript *jit = script->getJIT((bool) constructing, (bool) barriers);
            if (!jit)
                continue;
            mjit::JITChunk *chunk = jit->chunk(pc);
            if (!chunk)
                continue;
            bool found = false;
            uint32_t count = (kind == RECOMPILE_CHECK_MONITORED)
                             ? chunk->nMonitoredBytecodes
                             : chunk->nTypeBarrierBytecodes;
            uint32_t *bytecodes = (kind == RECOMPILE_CHECK_MONITORED)
                                  ? chunk->monitoredBytecodes()
                                  : chunk->typeBarrierBytecodes();
            for (size_t i = 0; i < count; i++) {
                if (bytecodes[i] == uint32_t(pc - script->code))
                    found = true;
            }
            if (!found)
                return false;
        }
    }
#endif

    if (script->hasAnyIonScript() || script->isIonCompilingOffThread())
        return false;

    return true;
}





static inline void
AddPendingRecompile(JSContext *cx, RawScript script, jsbytecode *pc,
                    RecompileKind kind = RECOMPILE_NONE)
{
    



    if (!JITCodeHasCheck(script, pc, kind))
        cx->compartment->types.addPendingRecompile(cx, script, pc);

    




    RecompileInfo& info = cx->compartment->types.compiledInfo;
    if (info.outputIndex != RecompileInfo::NoCompilerRunning) {
        CompilerOutput *co = info.compilerOutput(cx);
        switch (co->kind()) {
          case CompilerOutput::MethodJIT:
            break;
          case CompilerOutput::Ion:
          case CompilerOutput::ParallelIon:
            if (co->script == script)
                co->invalidate();
            break;
        }
    }

    




    if (script->function() && !script->function()->hasLazyType())
        ObjectStateChange(cx, script->function()->type(), false, true);
}






class TypeConstraintFreezeStack : public TypeConstraint
{
    RawScript script_;

  public:
    TypeConstraintFreezeStack(RawScript script)
        : script_(script)
    {}

    const char *kind() { return "freezeStack"; }

    void newType(JSContext *cx, TypeSet *source, Type type)
    {
        



        RootedScript script(cx, script_);
        AddPendingRecompile(cx, script, NULL);
    }
};





static inline bool
TypeInferenceSupported()
{
#ifdef JS_METHODJIT
    
    
    
    JSC::MacroAssembler masm;
    if (!masm.supportsFloatingPoint())
        return false;
#endif

#if WTF_ARM_ARCH_VERSION == 6
    
    
    return false;
#endif

    return true;
}

void
TypeCompartment::init(JSContext *cx)
{
    PodZero(this);

    compiledInfo.outputIndex = RecompileInfo::NoCompilerRunning;

    if (!cx ||
        !cx->hasOption(JSOPTION_TYPE_INFERENCE) ||
        !TypeInferenceSupported())
    {
        return;
    }

    inferenceEnabled = true;
}

TypeObject *
TypeCompartment::newTypeObject(JSContext *cx, Class *clasp, Handle<TaggedProto> proto, bool unknown)
{
    JS_ASSERT_IF(proto.isObject(), cx->compartment == proto.toObject()->compartment());

    TypeObject *object = gc::NewGCThing<TypeObject, CanGC>(cx, gc::FINALIZE_TYPE_OBJECT,
                                                           sizeof(TypeObject), gc::TenuredHeap);
    if (!object)
        return NULL;
    new(object) TypeObject(clasp, proto, clasp == &FunctionClass, unknown);

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
        return NULL;

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
        return NULL;

    








    if (*initpc != JSOP_NEWARRAY)
        return NULL;

    jsbytecode *last = PreviousOpcode(script, initpc);
    if (!last || *last != JSOP_INITELEM_ARRAY)
        return NULL;

    last = PreviousOpcode(script, last);
    if (!last || *last != JSOP_ENDINIT)
        return NULL;

    




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
        return NULL;

    return previnit;
}

TypeObject *
TypeCompartment::addAllocationSiteTypeObject(JSContext *cx, AllocationSiteKey key)
{
    AutoEnterAnalysis enter(cx);

    if (!allocationSiteTable) {
        allocationSiteTable = cx->new_<AllocationSiteTable>();
        if (!allocationSiteTable || !allocationSiteTable->init()) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return NULL;
        }
    }

    AllocationSiteTable::AddPtr p = allocationSiteTable->lookupForAdd(key);
    JS_ASSERT(!p);

    TypeObject *res = NULL;

    




    jsbytecode *pc = key.script->code + key.offset;
    RootedScript keyScript(cx, key.script);
    jsbytecode *prev = FindPreviousInnerInitializer(keyScript, pc);
    if (prev) {
        AllocationSiteKey nkey;
        nkey.script = key.script;
        nkey.offset = prev - key.script->code;
        nkey.kind = JSProto_Array;

        AllocationSiteTable::Ptr p = cx->compartment->types.allocationSiteTable->lookup(nkey);
        if (p)
            res = p->value;
    }

    if (!res) {
        RootedObject proto(cx);
        if (!js_GetClassPrototype(cx, key.kind, &proto, NULL))
            return NULL;

        Rooted<TaggedProto> tagged(cx, TaggedProto(proto));
        res = newTypeObject(cx, GetClassForProtoKey(key.kind), tagged);
        if (!res) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return NULL;
        }
        key.script = keyScript;
    }

    if (JSOp(*pc) == JSOP_NEWOBJECT) {
        




        RootedObject baseobj(cx, key.script->getObject(GET_UINT32_INDEX(pc)));

        if (!res->addDefiniteProperties(cx, baseobj))
            return NULL;
    }

    if (!allocationSiteTable->add(p, key, res)) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return NULL;
    }

    return res;
}

static inline RawId
GetAtomId(JSContext *cx, RawScript script, const jsbytecode *pc, unsigned offset)
{
    PropertyName *name = script->getName(GET_UINT32_INDEX(pc + offset));
    return IdToTypeId(NameToId(name));
}

bool
types::UseNewType(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    JS_ASSERT(cx->typeInferenceEnabled());

    
















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
types::UseNewTypeForInitializer(JSContext *cx, JSScript *script, jsbytecode *pc, JSProtoKey key)
{
    





    if (!cx->typeInferenceEnabled() || (script->function() && !script->treatAsRunOnce))
        return GenericObject;

    if (key != JSProto_Object && !(key >= JSProto_Int8Array && key <= JSProto_Uint8ClampedArray))
        return GenericObject;

    AutoEnterAnalysis enter(cx);

    if (!script->ensureRanAnalysis(cx))
        return GenericObject;

    if (script->analysis()->getCode(pc).inLoop)
        return GenericObject;

    return SingletonObject;
}

NewObjectKind
types::UseNewTypeForInitializer(JSContext *cx, JSScript *script, jsbytecode *pc, Class *clasp)
{
    return UseNewTypeForInitializer(cx, script, pc, JSCLASS_CACHED_PROTO_KEY(clasp));
}

static inline bool
ClassCanHaveExtraProperties(Class *clasp)
{
    JS_ASSERT(clasp->resolve);
    return clasp->resolve != JS_ResolveStub || clasp->ops.lookupGeneric || clasp->ops.getGeneric;
}

static inline bool
PrototypeHasIndexedProperty(JSContext *cx, JSObject *obj)
{
    do {
        TypeObject *type = obj->getType(cx);
        if (!type)
            return true;
        if (ClassCanHaveExtraProperties(type->clasp))
            return true;
        if (type->unknownProperties())
            return true;
        HeapTypeSet *indexTypes = type->getProperty(cx, JSID_VOID, false);
        if (!indexTypes || indexTypes->isOwnProperty(cx, type, true) || indexTypes->knownNonEmpty(cx))
            return true;
        obj = obj->getProto();
    } while (obj);

    return false;
}

bool
types::ArrayPrototypeHasIndexedProperty(JSContext *cx, HandleScript script)
{
    if (!cx->typeInferenceEnabled() || !script->compileAndGo)
        return true;

    JSObject *proto = script->global().getOrCreateArrayPrototype(cx);
    if (!proto)
        return true;

    return PrototypeHasIndexedProperty(cx, proto);
}

bool
types::TypeCanHaveExtraIndexedProperties(JSContext *cx, StackTypeSet *types)
{
    Class *clasp = types->getKnownClass();

    if (!clasp || ClassCanHaveExtraProperties(clasp))
        return true;

    if (types->hasObjectFlags(cx, types::OBJECT_FLAG_SPARSE_INDEXES))
        return true;

    JSObject *proto = types->getCommonPrototype();
    if (!proto)
        return true;

    return PrototypeHasIndexedProperty(cx, proto);
}

bool
TypeCompartment::growPendingArray(JSContext *cx)
{
    unsigned newCapacity = js::Max(unsigned(100), pendingCapacity * 2);
    PendingWork *newArray = js_pod_calloc<PendingWork>(newCapacity);
    if (!newArray) {
        cx->compartment->types.setPendingNukeTypes(cx);
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
    
    Vector<RecompileInfo> *pending = pendingRecompiles;
    pendingRecompiles = NULL;

    JS_ASSERT(!pending->empty());

#ifdef JS_METHODJIT

    mjit::ExpandInlineFrames(compartment());

    for (unsigned i = 0; i < pending->length(); i++) {
        CompilerOutput &co = *(*pending)[i].compilerOutput(*this);
        switch (co.kind()) {
          case CompilerOutput::MethodJIT:
            JS_ASSERT(co.isValid());
            mjit::Recompiler::clearStackReferences(fop, co.script);
            co.mjit()->destroyChunk(fop, co.chunkIndex);
            JS_ASSERT(co.script == NULL);
            break;
          case CompilerOutput::Ion:
          case CompilerOutput::ParallelIon:
            break;
        }
    }

# ifdef JS_ION
    ion::Invalidate(*this, fop, *pending);
# endif
#endif 

    fop->delete_(pending);
}

void
TypeCompartment::setPendingNukeTypes(JSContext *cx)
{
    if (!pendingNukeTypes) {
        if (cx->compartment)
            js_ReportOutOfMemory(cx);
        pendingNukeTypes = true;
    }
}

void
TypeCompartment::setPendingNukeTypesNoReport()
{
    JS_ASSERT(compartment()->activeAnalysis);
    if (!pendingNukeTypes)
        pendingNukeTypes = true;
}

void
TypeCompartment::nukeTypes(FreeOp *fop)
{
    










    JS_ASSERT(pendingNukeTypes);
    if (pendingRecompiles) {
        fop->free_(pendingRecompiles);
        pendingRecompiles = NULL;
    }

    inferenceEnabled = false;

    
    for (ContextIter acx(fop->runtime()); !acx.done(); acx.next())
        acx->setCompartment(acx->compartment);

#ifdef JS_METHODJIT
    JSCompartment *compartment = this->compartment();
    mjit::ExpandInlineFrames(compartment);
    mjit::ClearAllFrames(compartment);
# ifdef JS_ION
    ion::InvalidateAll(fop, compartment);
# endif

    

    for (gc::CellIter i(compartment, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        RawScript script = i.get<JSScript>();
        mjit::ReleaseScriptCode(fop, script);
# ifdef JS_ION
        ion::FinishInvalidation(fop, script);
# endif
    }
#endif 

    pendingNukeTypes = false;
}

void
TypeCompartment::addPendingRecompile(JSContext *cx, const RecompileInfo &info)
{
    CompilerOutput *co = info.compilerOutput(cx);

    if (co->pendingRecompilation)
        return;

    if (co->isValid())
        CancelOffThreadIonCompile(cx->compartment, co->script);

    if (!co->isValid()) {
        JS_ASSERT(co->script == NULL);
        return;
    }

#ifdef JS_METHODJIT
    mjit::JITScript *jit = co->script->getJIT(co->constructing, co->barriers);
    bool hasJITCode = jit && jit->chunkDescriptor(co->chunkIndex).chunk;

# if defined(JS_ION)
    hasJITCode |= !!co->script->hasAnyIonScript();
# endif

    if (!hasJITCode) {
        
        return;
    }
#endif

    if (!pendingRecompiles) {
        pendingRecompiles = cx->new_< Vector<RecompileInfo> >(cx);
        if (!pendingRecompiles) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
    }

#if DEBUG
    for (size_t i = 0; i < pendingRecompiles->length(); i++) {
        RecompileInfo pr = (*pendingRecompiles)[i];
        JS_ASSERT(info.outputIndex != pr.outputIndex);
    }
#endif

    if (!pendingRecompiles->append(info)) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    co->setPendingRecompilation();
}

void
TypeCompartment::addPendingRecompile(JSContext *cx, RawScript script, jsbytecode *pc)
{
    JS_ASSERT(script);
    if (!constrainedOutputs)
        return;

#ifdef JS_METHODJIT
    for (int constructing = 0; constructing <= 1; constructing++) {
        for (int barriers = 0; barriers <= 1; barriers++) {
            mjit::JITScript *jit = script->getJIT((bool) constructing, (bool) barriers);
            if (!jit)
                continue;

            if (pc) {
                unsigned int chunkIndex = jit->chunkIndex(pc);
                mjit::JITChunk *chunk = jit->chunkDescriptor(chunkIndex).chunk;
                if (chunk)
                    addPendingRecompile(cx, chunk->recompileInfo);
            } else {
                for (size_t chunkIndex = 0; chunkIndex < jit->nchunks; chunkIndex++) {
                    mjit::JITChunk *chunk = jit->chunkDescriptor(chunkIndex).chunk;
                    if (chunk)
                        addPendingRecompile(cx, chunk->recompileInfo);
                }
            }
        }
    }

# ifdef JS_ION
    CancelOffThreadIonCompile(cx->compartment, script);

    if (script->hasIonScript())
        addPendingRecompile(cx, script->ionScript()->recompileInfo());

    if (script->hasParallelIonScript())
        addPendingRecompile(cx, script->parallelIonScript()->recompileInfo());
# endif
#endif
}

void
TypeCompartment::monitorBytecode(JSContext *cx, JSScript *script, uint32_t offset,
                                 bool returnOnly)
{
    if (!script->ensureRanInference(cx))
        return;

    ScriptAnalysis *analysis = script->analysis();
    jsbytecode *pc = script->code + offset;

    JS_ASSERT_IF(returnOnly, js_CodeSpec[*pc].format & JOF_INVOKE);

    Bytecode &code = analysis->getCode(pc);

    if (returnOnly ? code.monitoredTypesReturn : code.monitoredTypes)
        return;

    InferSpew(ISpewOps, "addMonitorNeeded:%s #%u:%05u",
              returnOnly ? " returnOnly" : "", script->id(), offset);

    
    if (js_CodeSpec[*pc].format & JOF_INVOKE)
        code.monitoredTypesReturn = true;

    if (returnOnly)
        return;

    code.monitoredTypes = true;

    AddPendingRecompile(cx, script, pc, RECOMPILE_CHECK_MONITORED);
}

void
TypeCompartment::markSetsUnknown(JSContext *cx, TypeObject *target)
{
    JS_ASSERT(this == &cx->compartment->types);
    JS_ASSERT(!(target->flags & OBJECT_FLAG_SETS_MARKED_UNKNOWN));
    JS_ASSERT(!target->singleton);
    JS_ASSERT(target->unknownProperties());
    target->flags |= OBJECT_FLAG_SETS_MARKED_UNKNOWN;

    AutoEnterAnalysis enter(cx);

    









    Vector<TypeSet *> pending(cx);
    for (gc::CellIter i(cx->compartment, gc::FINALIZE_TYPE_OBJECT); !i.done(); i.next()) {
        TypeObject *object = i.get<TypeObject>();

        unsigned count = object->getPropertyCount();
        for (unsigned i = 0; i < count; i++) {
            Property *prop = object->getProperty(i);
            if (prop && prop->types.hasType(Type::ObjectType(target))) {
                if (!pending.append(&prop->types))
                    cx->compartment->types.setPendingNukeTypes(cx);
            }
        }
    }

    for (unsigned i = 0; i < pending.length(); i++)
        pending[i]->addType(cx, Type::AnyObjectType());

    for (gc::CellIter i(cx->compartment, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        RootedScript script(cx, i.get<JSScript>());
        if (script->types) {
            unsigned count = TypeScript::NumTypeSets(script);
            TypeSet *typeArray = script->types->typeArray();
            for (unsigned i = 0; i < count; i++) {
                if (typeArray[i].hasType(Type::ObjectType(target)))
                    typeArray[i].addType(cx, Type::AnyObjectType());
            }
        }
        if (script->hasAnalysis() && script->analysis()->ranInference()) {
            for (unsigned i = 0; i < script->length; i++) {
                if (!script->analysis()->maybeCode(i))
                    continue;
                jsbytecode *pc = script->code + i;
                if (js_CodeSpec[*pc].format & JOF_DECOMPOSE)
                    continue;
                unsigned defCount = GetDefCount(script, i);
                if (ExtendedDef(pc))
                    defCount++;
                for (unsigned j = 0; j < defCount; j++) {
                    TypeSet *types = script->analysis()->pushedTypes(pc, j);
                    if (types->hasType(Type::ObjectType(target)))
                        types->addType(cx, Type::AnyObjectType());
                }
            }
        }
    }
}

void
ScriptAnalysis::addTypeBarrier(JSContext *cx, const jsbytecode *pc, TypeSet *target, Type type)
{
    Bytecode &code = getCode(pc);

    if (!type.isUnknown() && !type.isAnyObject() &&
        type.isObject() && target->getObjectCount() >= BARRIER_OBJECT_LIMIT) {
        
        target->addType(cx, type);
        return;
    }

    if (!code.typeBarriers) {
        





        RootedScript script(cx, script_);
        AddPendingRecompile(cx, script, const_cast<jsbytecode*>(pc), RECOMPILE_CHECK_BARRIERS);
    }

    
    size_t barrierCount = 0;
    TypeBarrier *barrier = code.typeBarriers;
    while (barrier) {
        if (barrier->target == target && !barrier->singleton) {
            if (barrier->type == type)
                return;
            if (barrier->type.isAnyObject() && !type.isUnknown() &&
                
                type.isObject())
            {
                return;
            }
        }
        barrier = barrier->next;
        barrierCount++;
    }

    




    if (barrierCount >= BARRIER_OBJECT_LIMIT &&
        !type.isUnknown() && !type.isAnyObject() && type.isObject())
    {
        type = Type::AnyObjectType();
    }

    InferSpew(ISpewOps, "typeBarrier: #%u:%05u: %sT%p%s %s",
              script_->id(), pc - script_->code,
              InferSpewColor(target), target, InferSpewColorReset(),
              TypeString(type));

    barrier = cx->analysisLifoAlloc().new_<TypeBarrier>(target, type, (JSObject *) NULL, JSID_VOID);

    if (!barrier) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    barrier->next = code.typeBarriers;
    code.typeBarriers = barrier;
}

void
ScriptAnalysis::addSingletonTypeBarrier(JSContext *cx, const jsbytecode *pc, TypeSet *target,
                                        HandleObject singleton, HandleId singletonId)
{
    JS_ASSERT(singletonId == IdToTypeId(singletonId) && !JSID_IS_VOID(singletonId));

    Bytecode &code = getCode(pc);

    if (!code.typeBarriers) {
        
        RootedScript script(cx, script_);
        AddPendingRecompile(cx, script, const_cast<jsbytecode*>(pc), RECOMPILE_CHECK_BARRIERS);
    }

    InferSpew(ISpewOps, "singletonTypeBarrier: #%u:%05u: %sT%p%s %p %s",
              script_->id(), pc - script_->code,
              InferSpewColor(target), target, InferSpewColorReset(),
              (void *) singleton.get(), TypeIdString(singletonId));

    TypeBarrier *barrier = cx->analysisLifoAlloc().new_<TypeBarrier>(target, Type::UndefinedType(),
                              singleton, singletonId);

    if (!barrier) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    barrier->next = code.typeBarriers;
    code.typeBarriers = barrier;
}

void
TypeCompartment::print(JSContext *cx, bool force)
{
    gc::AutoSuppressGC suppressGC(cx);

    JSCompartment *compartment = this->compartment();
    AutoEnterAnalysis enter(NULL, compartment);

    if (!force && !InferSpewActive(ISpewResult))
        return;

    for (gc::CellIter i(compartment, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        RootedScript script(cx, i.get<JSScript>());
        if (script->hasAnalysis() && script->analysis()->ranInference())
            script->analysis()->printTypes(cx);
    }

#ifdef DEBUG
    for (gc::CellIter i(compartment, gc::FINALIZE_TYPE_OBJECT); !i.done(); i.next()) {
        TypeObject *object = i.get<TypeObject>();
        object->print();
    }
#endif

    printf("Counts: ");
    for (unsigned count = 0; count < TYPE_COUNT_LIMIT; count++) {
        if (count)
            printf("/");
        printf("%u", typeCounts[count]);
    }
    printf(" (%u over)\n", typeCountOver);

    printf("Recompilations: %u\n", recompilations);
}

















static inline bool
NumberTypes(Type a, Type b)
{
    return (a.isPrimitive(JSVAL_TYPE_INT32) || a.isPrimitive(JSVAL_TYPE_DOUBLE))
        && (b.isPrimitive(JSVAL_TYPE_INT32) || b.isPrimitive(JSVAL_TYPE_DOUBLE));
}






static inline Type
GetValueTypeForTable(JSContext *cx, const Value &v)
{
    Type type = GetValueType(cx, v);
    JS_ASSERT(!type.isSingleObject());
    return type;
}

struct types::ArrayTableKey
{
    Type type;
    JSObject *proto;

    ArrayTableKey()
        : type(Type::UndefinedType()), proto(NULL)
    {}

    typedef ArrayTableKey Lookup;

    static inline uint32_t hash(const ArrayTableKey &v) {
        return (uint32_t) (v.type.raw() ^ ((uint32_t)(size_t)v.proto >> 2));
    }

    static inline bool match(const ArrayTableKey &v1, const ArrayTableKey &v2) {
        return v1.type == v2.type && v1.proto == v2.proto;
    }
};

void
TypeCompartment::fixArrayType(JSContext *cx, JSObject *obj)
{
    AutoEnterAnalysis enter(cx);

    if (!arrayTypeTable) {
        arrayTypeTable = cx->new_<ArrayTypeTable>();
        if (!arrayTypeTable || !arrayTypeTable->init()) {
            arrayTypeTable = NULL;
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
    }

    





    JS_ASSERT(obj->isArray());

    unsigned len = obj->getDenseInitializedLength();
    if (len == 0)
        return;

    Type type = GetValueTypeForTable(cx, obj->getDenseElement(0));

    for (unsigned i = 1; i < len; i++) {
        Type ntype = GetValueTypeForTable(cx, obj->getDenseElement(i));
        if (ntype != type) {
            if (NumberTypes(type, ntype))
                type = Type::DoubleType();
            else
                return;
        }
    }

    ArrayTableKey key;
    key.type = type;
    key.proto = obj->getProto();
    ArrayTypeTable::AddPtr p = arrayTypeTable->lookupForAdd(key);

    if (p) {
        obj->setType(p->value);
    } else {
        Rooted<Type> origType(cx, type);
        
        RootedObject objProto(cx, obj->getProto());
        Rooted<TypeObject*> objType(cx, newTypeObject(cx, &ArrayClass, objProto));
        if (!objType) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
        obj->setType(objType);

        if (!objType->unknownProperties())
            objType->addPropertyType(cx, JSID_VOID, type);

        
        
        
        
        if (type != origType || key.proto != obj->getProto()) {
            key.type = origType;
            key.proto = obj->getProto();
            p = arrayTypeTable->lookupForAdd(key);
        }

        if (!arrayTypeTable->relookupOrAdd(p, key, objType)) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
    }
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
UpdateObjectTableEntryTypes(JSContext *cx, ObjectTableEntry &entry,
                            IdValuePair *properties, size_t nproperties)
{
    for (size_t i = 0; i < nproperties; i++) {
        Type type = entry.types[i];
        Type ntype = GetValueTypeForTable(cx, properties[i].value);
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
TypeCompartment::fixObjectType(JSContext *cx, JSObject *obj)
{
    AutoEnterAnalysis enter(cx);

    if (!objectTypeTable) {
        objectTypeTable = cx->new_<ObjectTypeTable>();
        if (!objectTypeTable || !objectTypeTable->init()) {
            objectTypeTable = NULL;
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
    }

    



    JS_ASSERT(obj->isObject());

    if (obj->slotSpan() == 0 || obj->inDictionaryMode() || !obj->hasEmptyElements())
        return;

    Vector<IdValuePair> properties(cx);
    if (!properties.resize(obj->slotSpan())) {
        cx->compartment->types.setPendingNukeTypes(cx);
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
    TypeObject *objType = newTypeObject(cx, &ObjectClass, objProto);
    if (!objType || !objType->addDefiniteProperties(cx, obj)) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    if (obj->isIndexed())
        objType->setFlags(cx, OBJECT_FLAG_SPARSE_INDEXES);

    jsid *ids = cx->pod_calloc<jsid>(properties.length());
    if (!ids) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    Type *types = cx->pod_calloc<Type>(properties.length());
    if (!types) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    for (size_t i = 0; i < properties.length(); i++) {
        ids[i] = properties[i].id;
        types[i] = GetValueTypeForTable(cx, obj->getSlot(i));
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
        cx->compartment->types.setPendingNukeTypes(cx);
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
            objectTypeTable = NULL;
            cx->compartment->types.setPendingNukeTypes(cx);
            return NULL;
        }
    }

    





    







    if (!nproperties || nproperties >= PropertyTree::MAX_HEIGHT)
        return NULL;

    gc::AllocKind allocKind = gc::GetGCObjectKind(nproperties);
    size_t nfixed = gc::GetGCKindSlots(allocKind, &ObjectClass);

    ObjectTableKey::Lookup lookup(properties, nproperties, nfixed);
    ObjectTypeTable::AddPtr p = objectTypeTable->lookupForAdd(lookup);

    if (!p)
        return NULL;

    RootedObject obj(cx, NewBuiltinClassInstance(cx, &ObjectClass, allocKind));
    if (!obj) {
        cx->clearPendingException();
        return NULL;
    }
    JS_ASSERT(obj->getProto() == p->value.object->proto);

    RootedShape shape(cx, p->value.shape);
    if (!JSObject::setLastProperty(cx, obj, shape)) {
        cx->clearPendingException();
        return NULL;
    }

    UpdateObjectTableEntryTypes(cx, p->value, properties, nproperties);

    for (size_t i = 0; i < nproperties; i++)
        obj->setSlot(i, properties[i].value);

    obj->setType(p->value.object);
    return obj;
}





void
TypeObject::getFromPrototypes(JSContext *cx, jsid id, TypeSet *types, bool force)
{
    if (!force && types->hasPropagatedProperty())
        return;

    types->setPropagatedProperty();

    if (!proto)
        return;

    if (proto == Proxy::LazyProto) {
        JS_ASSERT(unknownProperties());
        return;
    }

    types::TypeObject *protoType = proto->getType(cx);
    if (!protoType || protoType->unknownProperties()) {
        types->addType(cx, Type::UnknownType());
        return;
    }

    HeapTypeSet *protoTypes = protoType->getProperty(cx, id, false);
    if (!protoTypes)
        return;

    protoTypes->addSubset(cx, types);

    protoType->getFromPrototypes(cx, id, protoTypes);
}

static inline void
UpdatePropertyType(JSContext *cx, TypeSet *types, RawObject obj, RawShape shape,
                   bool force)
{
    types->setOwnProperty(cx, false);
    if (!shape->writable())
        types->setOwnProperty(cx, true);

    if (shape->hasGetterValue() || shape->hasSetterValue()) {
        types->setOwnProperty(cx, true);
        types->addType(cx, Type::UnknownType());
    } else if (shape->hasDefaultGetter() && shape->hasSlot()) {
        const Value &value = obj->nativeGetSlot(shape->slot());

        



        if (force || !value.isUndefined()) {
            Type type = GetValueType(cx, value);
            types->addType(cx, type);
        }
    }
}

bool
TypeObject::addProperty(JSContext *cx, RawId id, Property **pprop)
{
    JS_ASSERT(!*pprop);
    Property *base = cx->typeLifoAlloc().new_<Property>(id);
    if (!base) {
        cx->compartment->types.setPendingNukeTypes(cx);
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
                    Type type = GetValueType(cx, value);
                    base->types.setOwnProperty(cx, false);
                    base->types.addType(cx, type);
                }
            }
        } else if (!JSID_IS_EMPTY(id)) {
            RootedId rootedId(cx, id);
            RawShape shape = singleton->nativeLookup(cx, rootedId);
            if (shape)
                UpdatePropertyType(cx, &base->types, rSingleton, shape, false);
        }

        if (singleton->watched()) {
            



            base->types.setOwnProperty(cx, true);
        }
    }

    *pprop = base;

    InferSpew(ISpewOps, "typeSet: %sT%p%s property %s %s",
              InferSpewColor(&base->types), &base->types, InferSpewColorReset(),
              TypeObjectString(this), TypeIdString(id));

    return true;
}

bool
TypeObject::addDefiniteProperties(JSContext *cx, JSObject *obj)
{
    if (unknownProperties())
        return true;

    
    AutoEnterAnalysis enter(cx);

    RootedShape shape(cx, obj->lastProperty());
    while (!shape->isEmptyShape()) {
        RawId id = IdToTypeId(shape->propid());
        if (!JSID_IS_VOID(id) && obj->isFixedSlot(shape->slot()) &&
            shape->slot() <= (TYPE_FLAG_DEFINITE_MASK >> TYPE_FLAG_DEFINITE_SHIFT)) {
            TypeSet *types = getProperty(cx, id, true);
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
            RawShape shape = obj->lastProperty();
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

inline void
InlineAddTypeProperty(JSContext *cx, TypeObject *obj, jsid id, Type type)
{
    JS_ASSERT(id == IdToTypeId(id));

    AutoEnterAnalysis enter(cx);

    TypeSet *types = obj->getProperty(cx, id, true);
    if (!types || types->hasType(type))
        return;

    InferSpew(ISpewOps, "externalType: property %s %s: %s",
              TypeObjectString(obj), TypeIdString(id), TypeString(type));
    types->addType(cx, type);
}

void
TypeObject::addPropertyType(JSContext *cx, jsid id, Type type)
{
    InlineAddTypeProperty(cx, this, id, type);
}

void
TypeObject::addPropertyType(JSContext *cx, jsid id, const Value &value)
{
    InlineAddTypeProperty(cx, this, id, GetValueType(cx, value));
}

void
TypeObject::addPropertyType(JSContext *cx, const char *name, Type type)
{
    RawId id = JSID_VOID;
    if (name) {
        JSAtom *atom = Atomize(cx, name, strlen(name));
        if (!atom) {
            AutoEnterAnalysis enter(cx);
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
        id = AtomToId(atom);
    }
    InlineAddTypeProperty(cx, this, id, type);
}

void
TypeObject::addPropertyType(JSContext *cx, const char *name, const Value &value)
{
    addPropertyType(cx, name, GetValueType(cx, value));
}

void
TypeObject::markPropertyConfigured(JSContext *cx, jsid id)
{
    AutoEnterAnalysis enter(cx);

    id = IdToTypeId(id);

    TypeSet *types = getProperty(cx, id, true);
    if (types)
        types->setOwnProperty(cx, true);
}

void
TypeObject::markStateChange(JSContext *cx)
{
    if (unknownProperties())
        return;

    AutoEnterAnalysis enter(cx);
    TypeSet *types = maybeGetProperty(JSID_EMPTY, cx);
    if (types) {
        TypeConstraint *constraint = types->constraintList;
        while (constraint) {
            constraint->newObjectState(cx, this, true);
            constraint = constraint->next;
        }
    }
}

void
TypeObject::setFlags(JSContext *cx, TypeObjectFlags flags)
{
    if ((this->flags & flags) == flags)
        return;

    AutoEnterAnalysis enter(cx);

    if (singleton) {
        
        JS_ASSERT_IF(flags & OBJECT_FLAG_UNINLINEABLE,
                     interpretedFunction->nonLazyScript()->uninlineable);
        JS_ASSERT_IF(flags & OBJECT_FLAG_ITERATED,
                     singleton->lastProperty()->hasObjectFlag(BaseShape::ITERATED_SINGLETON));
    }

    this->flags |= flags;

    InferSpew(ISpewOps, "%s: setFlags 0x%x", TypeObjectString(this), flags);

    ObjectStateChange(cx, this, false, false);
}

void
TypeObject::markUnknown(JSContext *cx)
{
    AutoEnterAnalysis enter(cx);

    JS_ASSERT(cx->compartment->activeAnalysis);
    JS_ASSERT(!unknownProperties());

    if (!(flags & OBJECT_FLAG_NEW_SCRIPT_CLEARED))
        clearNewScript(cx);

    InferSpew(ISpewOps, "UnknownProperties: %s", TypeObjectString(this));

    ObjectStateChange(cx, this, true, true);

    








    unsigned count = getPropertyCount();
    for (unsigned i = 0; i < count; i++) {
        Property *prop = getProperty(i);
        if (prop) {
            prop->types.addType(cx, Type::UnknownType());
            prop->types.setOwnProperty(cx, true);
        }
    }
}

void
TypeObject::clearNewScript(JSContext *cx)
{
    JS_ASSERT(!(flags & OBJECT_FLAG_NEW_SCRIPT_CLEARED));
    flags |= OBJECT_FLAG_NEW_SCRIPT_CLEARED;

    







    if (!newScript)
        return;

    AutoEnterAnalysis enter(cx);

    








    for (unsigned i = 0; i < getPropertyCount(); i++) {
        Property *prop = getProperty(i);
        if (!prop)
            continue;
        if (prop->types.definiteProperty())
            prop->types.setOwnProperty(cx, true);
    }

    







    Vector<uint32_t, 32> pcOffsets(cx);
    for (ScriptFrameIter iter(cx); !iter.done(); ++iter) {
        pcOffsets.append(uint32_t(iter.pc() - iter.script()->code));
        if (iter.isConstructing() &&
            iter.callee() == newScript->fun &&
            iter.thisv().isObject() &&
            !iter.thisv().toObject().hasLazyType() &&
            iter.thisv().toObject().type() == this)
        {
            RootedObject obj(cx, &iter.thisv().toObject());

            
            bool finished = false;

            
            uint32_t numProperties = 0;

            



            size_t depth = 0;
            size_t callDepth = pcOffsets.length() - 1;
            uint32_t offset = pcOffsets[callDepth];

            for (TypeNewScript::Initializer *init = newScript->initializerList;; init++) {
                if (init->kind == TypeNewScript::Initializer::SETPROP) {
                    if (!depth && init->offset > offset) {
                        
                        break;
                    }
                    numProperties++;
                } else if (init->kind == TypeNewScript::Initializer::FRAME_PUSH) {
                    if (depth) {
                        depth++;
                    } else if (init->offset > offset) {
                        
                        break;
                    } else if (init->offset == offset) {
                        if (!callDepth)
                            break;
                        offset = pcOffsets[--callDepth];
                    } else {
                        
                        depth = 1;
                    }
                } else if (init->kind == TypeNewScript::Initializer::FRAME_POP) {
                    if (depth) {
                        depth--;
                    } else {
                        
                        break;
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
    }

    
    TypeNewScript *savedNewScript = newScript;
    newScript = NULL;
    js_free(savedNewScript);

    markStateChange(cx);
}

void
TypeObject::print()
{
    TaggedProto tagged(proto);
    printf("%s : %s",
           TypeObjectString(this),
           tagged.isObject() ? TypeString(Type::ObjectType(proto))
                            : (tagged.isLazy() ? "(lazy)" : "(null)"));

    if (unknownProperties()) {
        printf(" unknown");
    } else {
        if (!hasAnyFlags(OBJECT_FLAG_SPARSE_INDEXES))
            printf(" dense");
        if (!hasAnyFlags(OBJECT_FLAG_NON_PACKED))
            printf(" packed");
        if (!hasAnyFlags(OBJECT_FLAG_LENGTH_OVERFLOW))
            printf(" noLengthOverflow");
        if (hasAnyFlags(OBJECT_FLAG_UNINLINEABLE))
            printf(" uninlineable");
        if (hasAnyFlags(OBJECT_FLAG_EMULATES_UNDEFINED))
            printf(" emulatesUndefined");
        if (hasAnyFlags(OBJECT_FLAG_ITERATED))
            printf(" iterated");
    }

    unsigned count = getPropertyCount();

    if (count == 0) {
        printf(" {}\n");
        return;
    }

    printf(" {");

    for (unsigned i = 0; i < count; i++) {
        Property *prop = getProperty(i);
        if (prop) {
            printf("\n    %s:", TypeIdString(prop->id));
            prop->types.print();
        }
    }

    printf("\n}\n");
}





static inline TypeObject *
GetInitializerType(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    if (!script->compileAndGo)
        return NULL;

    JSOp op = JSOp(*pc);
    JS_ASSERT(op == JSOP_NEWARRAY || op == JSOP_NEWOBJECT || op == JSOP_NEWINIT);

    bool isArray = (op == JSOP_NEWARRAY || (op == JSOP_NEWINIT && GET_UINT8(pc) == JSProto_Array));
    JSProtoKey key = isArray ? JSProto_Array : JSProto_Object;

    if (UseNewTypeForInitializer(cx, script, pc, key))
        return NULL;

    return TypeScript::InitObject(cx, script, pc, key);
}


bool
ScriptAnalysis::analyzeTypesBytecode(JSContext *cx, unsigned offset, TypeInferenceState &state)
{
    JSScript *script = this->script_;

    jsbytecode *pc = script->code + offset;
    JSOp op = (JSOp)*pc;

    Bytecode &code = getCode(offset);
    JS_ASSERT(!code.pushedTypes);

    InferSpew(ISpewOps, "analyze: #%u:%05u", script->id(), offset);

    unsigned defCount = GetDefCount(script, offset);
    if (ExtendedDef(pc))
        defCount++;

    StackTypeSet *pushed = cx->analysisLifoAlloc().newArrayUninitialized<StackTypeSet>(defCount);
    if (!pushed)
        return false;
    PodZero(pushed, defCount);
    code.pushedTypes = pushed;

    





    if (code.newValues) {
        SlotValue *newv = code.newValues;
        while (newv->slot) {
            if (newv->value.kind() != SSAValue::PHI || newv->value.phiOffset() != offset) {
                newv++;
                continue;
            }

            



            if (!state.phiNodes.append(newv->value.phiNode()))
                return false;
            TypeSet &types = newv->value.phiNode()->types;
            InferSpew(ISpewOps, "typeSet: %sT%p%s phi #%u:%05u:%u",
                      InferSpewColor(&types), &types, InferSpewColorReset(),
                      script->id(), offset, newv->slot);
            types.setPurged();

            newv++;
        }
    }

    



    if (js_CodeSpec[*pc].format & JOF_DECOMPOSE)
        return true;

    for (unsigned i = 0; i < defCount; i++) {
        InferSpew(ISpewOps, "typeSet: %sT%p%s pushed%u #%u:%05u",
                  InferSpewColor(&pushed[i]), &pushed[i], InferSpewColorReset(),
                  i, script->id(), offset);
        pushed[i].setPurged();
    }

    
    switch (op) {

        
      case JSOP_POP:
      case JSOP_NOP:
      case JSOP_NOTEARG:
      case JSOP_LOOPHEAD:
      case JSOP_LOOPENTRY:
      case JSOP_GOTO:
      case JSOP_IFEQ:
      case JSOP_IFNE:
      case JSOP_LINENO:
      case JSOP_DEFCONST:
      case JSOP_LEAVEWITH:
      case JSOP_LEAVEBLOCK:
      case JSOP_RETRVAL:
      case JSOP_ENDITER:
      case JSOP_THROWING:
      case JSOP_GOSUB:
      case JSOP_RETSUB:
      case JSOP_CONDSWITCH:
      case JSOP_DEFAULT:
      case JSOP_POPN:
      case JSOP_POPV:
      case JSOP_DEBUGGER:
      case JSOP_SETCALL:
      case JSOP_TABLESWITCH:
      case JSOP_TRY:
      case JSOP_LABEL:
        break;

        
      case JSOP_VOID:
      case JSOP_UNDEFINED:
        pushed[0].addType(cx, Type::UndefinedType());
        break;
      case JSOP_ZERO:
      case JSOP_ONE:
      case JSOP_INT8:
      case JSOP_INT32:
      case JSOP_UINT16:
      case JSOP_UINT24:
      case JSOP_BITAND:
      case JSOP_BITOR:
      case JSOP_BITXOR:
      case JSOP_BITNOT:
      case JSOP_RSH:
      case JSOP_LSH:
      case JSOP_URSH:
        pushed[0].addType(cx, Type::Int32Type());
        break;
      case JSOP_FALSE:
      case JSOP_TRUE:
      case JSOP_EQ:
      case JSOP_NE:
      case JSOP_LT:
      case JSOP_LE:
      case JSOP_GT:
      case JSOP_GE:
      case JSOP_NOT:
      case JSOP_STRICTEQ:
      case JSOP_STRICTNE:
      case JSOP_IN:
      case JSOP_INSTANCEOF:
        pushed[0].addType(cx, Type::BooleanType());
        break;
      case JSOP_DOUBLE:
        pushed[0].addType(cx, Type::DoubleType());
        break;
      case JSOP_STRING:
      case JSOP_TYPEOF:
      case JSOP_TYPEOFEXPR:
        pushed[0].addType(cx, Type::StringType());
        break;
      case JSOP_NULL:
        pushed[0].addType(cx, Type::NullType());
        break;

      case JSOP_REGEXP:
        if (script->compileAndGo) {
            TypeObject *object = TypeScript::StandardType(cx, JSProto_RegExp);
            if (!object)
                return false;
            pushed[0].addType(cx, Type::ObjectType(object));
        } else {
            pushed[0].addType(cx, Type::UnknownType());
        }
        break;

      case JSOP_OBJECT:
        pushed[0].addType(cx, Type::ObjectType(script->getObject(GET_UINT32_INDEX(pc))));
        break;

      case JSOP_STOP:
        
          if (script->function())
            TypeScript::ReturnTypes(script)->addType(cx, Type::UndefinedType());
        break;

      case JSOP_OR:
      case JSOP_AND:
        
        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        break;

      case JSOP_DUP:
        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        poppedTypes(pc, 0)->addSubset(cx, &pushed[1]);
        break;

      case JSOP_DUP2:
        poppedTypes(pc, 1)->addSubset(cx, &pushed[0]);
        poppedTypes(pc, 0)->addSubset(cx, &pushed[1]);
        poppedTypes(pc, 1)->addSubset(cx, &pushed[2]);
        poppedTypes(pc, 0)->addSubset(cx, &pushed[3]);
        break;

      case JSOP_SWAP:
      case JSOP_PICK: {
        unsigned pickedDepth = (op == JSOP_SWAP ? 1 : GET_UINT8(pc));
        
        poppedTypes(pc, pickedDepth)->addSubset(cx, &pushed[pickedDepth]);
        for (unsigned i = 0; i < pickedDepth; i++)
            poppedTypes(pc, i)->addSubset(cx, &pushed[pickedDepth - 1 - i]);
        break;
      }

      case JSOP_GETGNAME:
      case JSOP_CALLGNAME: {
        RawId id = GetAtomId(cx, script, pc, 0);

        StackTypeSet *seen = bytecodeTypes(pc);
        seen->addSubset(cx, &pushed[0]);

        





        if (id == NameToId(cx->names().undefined))
            seen->addType(cx, Type::UndefinedType());
        if (id == NameToId(cx->names().NaN))
            seen->addType(cx, Type::DoubleType());
        if (id == NameToId(cx->names().Infinity))
            seen->addType(cx, Type::DoubleType());

        TypeObject *global = script->global().getType(cx);
        if (!global)
            return false;

        
        if (state.hasPropertyReadTypes)
            PropertyAccess<PROPERTY_READ_EXISTING>(cx, script, pc, global, seen, id);
        else
            PropertyAccess<PROPERTY_READ>(cx, script, pc, global, seen, id);
        break;
      }

      case JSOP_NAME:
      case JSOP_GETINTRINSIC:
      case JSOP_CALLNAME:
      case JSOP_CALLINTRINSIC: {
        StackTypeSet *seen = bytecodeTypes(pc);
        addTypeBarrier(cx, pc, seen, Type::UnknownType());
        seen->addSubset(cx, &pushed[0]);
        break;
      }

      case JSOP_BINDGNAME:
      case JSOP_BINDNAME:
      case JSOP_BINDINTRINSIC:
        break;

      case JSOP_SETGNAME: {
        jsid id = GetAtomId(cx, script, pc, 0);
        TypeObject *global = script->global().getType(cx);
        if (!global)
            return false;
        PropertyAccess<PROPERTY_WRITE>(cx, script, pc, global, poppedTypes(pc, 0), id);
        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        break;
      }

      case JSOP_SETNAME:
      case JSOP_SETINTRINSIC:
      case JSOP_SETCONST:
        cx->compartment->types.monitorBytecode(cx, script, offset);
        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        break;

      case JSOP_GETXPROP: {
        StackTypeSet *seen = bytecodeTypes(pc);
        addTypeBarrier(cx, pc, seen, Type::UnknownType());
        seen->addSubset(cx, &pushed[0]);
        break;
      }

      case JSOP_GETARG:
      case JSOP_CALLARG:
      case JSOP_GETLOCAL:
      case JSOP_CALLLOCAL: {
        uint32_t slot = GetBytecodeSlot(script, pc);
        if (trackSlot(slot)) {
            




            poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        } else if (slot < TotalSlots(script)) {
            StackTypeSet *types = TypeScript::SlotTypes(script, slot);
            types->addSubset(cx, &pushed[0]);
        } else {
            
            pushed[0].addType(cx, Type::UnknownType());
        }
        break;
      }

      case JSOP_SETARG:
      case JSOP_SETLOCAL: {
        uint32_t slot = GetBytecodeSlot(script, pc);
        if (!trackSlot(slot) && slot < TotalSlots(script)) {
            TypeSet *types = TypeScript::SlotTypes(script, slot);
            poppedTypes(pc, 0)->addSubset(cx, types);
        }

        




        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        break;
      }

      case JSOP_GETALIASEDVAR:
      case JSOP_CALLALIASEDVAR:
        






        bytecodeTypes(pc)->addSubset(cx, &pushed[0]);
        break;

      case JSOP_SETALIASEDVAR:
        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        break;

      case JSOP_ARGUMENTS:
        
        if (script->needsArgsObj())
            pushed[0].addType(cx, Type::UnknownType());
        else
            pushed[0].addType(cx, Type::MagicArgType());
        break;

      case JSOP_REST: {
        StackTypeSet *types = script->analysis()->bytecodeTypes(pc);
        if (script->compileAndGo) {
            TypeObject *rest = TypeScript::InitObject(cx, script, pc, JSProto_Array);
            if (!rest)
                return false;

            
            if (!rest->unknownProperties()) {
                HeapTypeSet *propTypes = rest->getProperty(cx, JSID_VOID, true);
                if (!propTypes)
                    return false;
                propTypes->addType(cx, Type::UnknownType());
            }

            types->addType(cx, Type::ObjectType(rest));
        } else {
            types->addType(cx, Type::UnknownType());
        }
        types->addSubset(cx, &pushed[0]);
        break;
      }


      case JSOP_SETPROP: {
        jsid id = GetAtomId(cx, script, pc, 0);
        poppedTypes(pc, 1)->addSetProperty(cx, script, pc, poppedTypes(pc, 0), id);
        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        break;
      }

      case JSOP_LENGTH:
      case JSOP_GETPROP:
      case JSOP_CALLPROP: {
        jsid id = GetAtomId(cx, script, pc, 0);
        StackTypeSet *seen = script->analysis()->bytecodeTypes(pc);

        HeapTypeSet *input = &script->types->propertyReadTypes[state.propertyReadIndex++];
        poppedTypes(pc, 0)->addSubset(cx, input);

        if (state.hasPropertyReadTypes) {
            TypeConstraintGetPropertyExisting getProp(script, pc, seen, id);
            input->addTypesToConstraint(cx, &getProp);
            if (op == JSOP_CALLPROP) {
                TypeConstraintCallPropertyExisting callProp(script, pc, id);
                input->addTypesToConstraint(cx, &callProp);
            }
        } else {
            input->addGetProperty(cx, script, pc, seen, id);
            if (op == JSOP_CALLPROP)
                input->addCallProperty(cx, script, pc, id);
        }

        seen->addSubset(cx, &pushed[0]);
        break;
      }

      




      case JSOP_GETELEM:
      case JSOP_CALLELEM: {
        StackTypeSet *seen = script->analysis()->bytecodeTypes(pc);

        
        if (op == JSOP_CALLELEM)
            seen->addType(cx, Type::AnyObjectType());

        HeapTypeSet *input = &script->types->propertyReadTypes[state.propertyReadIndex++];
        poppedTypes(pc, 1)->addSubset(cx, input);

        if (state.hasPropertyReadTypes) {
            TypeConstraintGetPropertyExisting getProp(script, pc, seen, JSID_VOID);
            input->addTypesToConstraint(cx, &getProp);
        } else {
            input->addGetProperty(cx, script, pc, seen, JSID_VOID);
        }

        seen->addSubset(cx, &pushed[0]);
        break;
      }

      case JSOP_SETELEM:
        poppedTypes(pc, 1)->addSetElement(cx, script, pc, poppedTypes(pc, 2), poppedTypes(pc, 0));
        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        break;

      case JSOP_TOID:
        



        pushed[0].addType(cx, Type::Int32Type());
        break;

      case JSOP_THIS:
        TypeScript::ThisTypes(script)->addTransformThis(cx, script, &pushed[0]);
        break;

      case JSOP_RETURN:
      case JSOP_SETRVAL:
          if (script->function())
            poppedTypes(pc, 0)->addSubset(cx, TypeScript::ReturnTypes(script));
        break;

      case JSOP_ADD:
        poppedTypes(pc, 0)->addArith(cx, script, pc, &pushed[0], poppedTypes(pc, 1));
        poppedTypes(pc, 1)->addArith(cx, script, pc, &pushed[0], poppedTypes(pc, 0));
        break;

      case JSOP_SUB:
      case JSOP_MUL:
      case JSOP_MOD:
      case JSOP_DIV:
        poppedTypes(pc, 0)->addArith(cx, script, pc, &pushed[0]);
        poppedTypes(pc, 1)->addArith(cx, script, pc, &pushed[0]);
        break;

      case JSOP_NEG:
      case JSOP_POS:
        poppedTypes(pc, 0)->addArith(cx, script, pc, &pushed[0]);
        break;

      case JSOP_LAMBDA: {
        RootedObject obj(cx, script->getObject(GET_UINT32_INDEX(pc)));
        TypeSet *res = &pushed[0];

        
        
        
        
        if (script->compileAndGo && !script->treatAsRunOnce && !UseNewTypeForClone(obj->toFunction()))
            res->addType(cx, Type::ObjectType(obj));
        else
            res->addType(cx, Type::AnyObjectType());
        break;
      }

      case JSOP_DEFFUN:
        cx->compartment->types.monitorBytecode(cx, script, offset);
        break;

      case JSOP_DEFVAR:
        break;

      case JSOP_CALL:
      case JSOP_EVAL:
      case JSOP_FUNCALL:
      case JSOP_FUNAPPLY:
      case JSOP_NEW: {
        StackTypeSet *seen = script->analysis()->bytecodeTypes(pc);
        seen->addSubset(cx, &pushed[0]);

        
        unsigned argCount = GetUseCount(script, offset) - 2;
        TypeCallsite *callsite = cx->analysisLifoAlloc().new_<TypeCallsite>(
                                                        cx, script, pc, op == JSOP_NEW, argCount);
        if (!callsite || (argCount && !callsite->argumentTypes)) {
            cx->compartment->types.setPendingNukeTypes(cx);
            break;
        }
        callsite->thisTypes = poppedTypes(pc, argCount);
        callsite->returnTypes = seen;

        for (unsigned i = 0; i < argCount; i++)
            callsite->argumentTypes[i] = poppedTypes(pc, argCount - 1 - i);

        




        if (op == JSOP_FUNCALL || op == JSOP_FUNAPPLY)
            cx->compartment->types.monitorBytecode(cx, script, pc - script->code);

        StackTypeSet *calleeTypes = poppedTypes(pc, argCount + 1);

        




        SSAValue calleeValue = poppedValue(pc, argCount + 1);
        if (*pc != JSOP_NEW &&
            (calleeValue.kind() != SSAValue::PUSHED ||
             script->code[calleeValue.pushedOffset()] != JSOP_CALLPROP))
        {
            calleeTypes->add(cx, cx->analysisLifoAlloc().new_<TypeConstraintPropagateThis>
                                   (script, pc, Type::UndefinedType(), callsite->thisTypes));
        }

        calleeTypes->addCall(cx, callsite);
        break;
      }

      case JSOP_NEWINIT:
      case JSOP_NEWARRAY:
      case JSOP_NEWOBJECT: {
        StackTypeSet *types = script->analysis()->bytecodeTypes(pc);
        types->addSubset(cx, &pushed[0]);

        bool isArray = (op == JSOP_NEWARRAY || (op == JSOP_NEWINIT && GET_UINT8(pc) == JSProto_Array));
        JSProtoKey key = isArray ? JSProto_Array : JSProto_Object;

        if (UseNewTypeForInitializer(cx, script, pc, key)) {
            
            break;
        }

        TypeObject *initializer = GetInitializerType(cx, script, pc);
        if (script->compileAndGo) {
            if (!initializer)
                return false;
            types->addType(cx, Type::ObjectType(initializer));
        } else {
            JS_ASSERT(!initializer);
            types->addType(cx, Type::UnknownType());
        }
        break;
      }

      case JSOP_ENDINIT:
        break;

      case JSOP_INITELEM:
      case JSOP_INITELEM_INC:
      case JSOP_INITELEM_ARRAY:
      case JSOP_SPREAD: {
        const SSAValue &objv = poppedValue(pc, (op == JSOP_INITELEM_ARRAY) ? 1 : 2);
        jsbytecode *initpc = script->code + objv.pushedOffset();
        TypeObject *initializer = GetInitializerType(cx, script, initpc);

        if (initializer) {
            pushed[0].addType(cx, Type::ObjectType(initializer));
            if (!initializer->unknownProperties()) {
                




                TypeSet *types = initializer->getProperty(cx, JSID_VOID, true);
                if (!types)
                    return false;
                if (state.hasGetSet) {
                    JS_ASSERT(op != JSOP_INITELEM_ARRAY);
                    types->addType(cx, Type::UnknownType());
                } else if (state.hasHole) {
                    if (!initializer->unknownProperties())
                        initializer->setFlags(cx, OBJECT_FLAG_NON_PACKED);
                } else if (op == JSOP_SPREAD) {
                    
                    types->addType(cx, Type::UnknownType());
                } else {
                    poppedTypes(pc, 0)->addSubset(cx, types);
                }
            }
        } else {
            pushed[0].addType(cx, Type::UnknownType());
        }
        switch (op) {
          case JSOP_SPREAD:
          case JSOP_INITELEM_INC:
            poppedTypes(pc, 1)->addSubset(cx, &pushed[1]);
            break;
          default:
            break;
        }
        state.hasGetSet = false;
        state.hasHole = false;
        break;
      }

      case JSOP_GETTER:
      case JSOP_SETTER:
        state.hasGetSet = true;
        break;

      case JSOP_HOLE:
        state.hasHole = true;
        break;

      case JSOP_INITPROP: {
        const SSAValue &objv = poppedValue(pc, 1);
        jsbytecode *initpc = script->code + objv.pushedOffset();
        TypeObject *initializer = GetInitializerType(cx, script, initpc);

        if (initializer) {
            pushed[0].addType(cx, Type::ObjectType(initializer));
            if (!initializer->unknownProperties()) {
                jsid id = GetAtomId(cx, script, pc, 0);
                TypeSet *types = initializer->getProperty(cx, id, true);
                if (!types)
                    return false;
                if (id == id___proto__(cx) || id == id_prototype(cx))
                    cx->compartment->types.monitorBytecode(cx, script, offset);
                else if (state.hasGetSet)
                    types->addType(cx, Type::UnknownType());
                else
                    poppedTypes(pc, 0)->addSubset(cx, types);
            }
        } else {
            pushed[0].addType(cx, Type::UnknownType());
        }
        state.hasGetSet = false;
        JS_ASSERT(!state.hasHole);
        break;
      }

      case JSOP_ENTERWITH:
      case JSOP_ENTERBLOCK:
      case JSOP_ENTERLET0:
        




        break;

      case JSOP_ENTERLET1:
        





        poppedTypes(pc, 0)->addSubset(cx, &pushed[defCount - 1]);
        break;

      case JSOP_ITER: {
        







        if (!state.forTypes) {
          state.forTypes = StackTypeSet::make(cx, "forTypes");
          if (!state.forTypes)
              return false;
        }

        if (GET_UINT8(pc) == JSITER_ENUMERATE)
            state.forTypes->addType(cx, Type::StringType());
        else
            state.forTypes->addType(cx, Type::UnknownType());
        break;
      }

      case JSOP_ITERNEXT:
        state.forTypes->addSubset(cx, &pushed[0]);
        break;

      case JSOP_MOREITER:
        pushed[1].addType(cx, Type::BooleanType());
        break;

      case JSOP_ENUMELEM:
      case JSOP_ENUMCONSTELEM:
      case JSOP_ARRAYPUSH:
        cx->compartment->types.monitorBytecode(cx, script, offset);
        break;

      case JSOP_THROW:
        
        break;

      case JSOP_FINALLY:
        
        break;

      case JSOP_IMPLICITTHIS:
      case JSOP_EXCEPTION:
        pushed[0].addType(cx, Type::UnknownType());
        break;

      case JSOP_DELPROP:
      case JSOP_DELELEM:
      case JSOP_DELNAME:
        pushed[0].addType(cx, Type::BooleanType());
        break;

      case JSOP_LEAVEBLOCKEXPR:
        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        break;

      case JSOP_LEAVEFORLETIN:
        break;

      case JSOP_CASE:
        poppedTypes(pc, 1)->addSubset(cx, &pushed[0]);
        break;

      case JSOP_GENERATOR:
        TypeScript::ReturnTypes(script)->addType(cx, Type::UnknownType());
        break;

      case JSOP_YIELD:
        pushed[0].addType(cx, Type::UnknownType());
        break;

      case JSOP_CALLEE:
        pushed[0].addType(cx, Type::AnyObjectType());
        break;

      default:
        
        fprintf(stderr, "Unknown bytecode %02x at #%u:%05u\n", op, script->id(), offset);
        TypeFailure(cx, "Unknown bytecode %02x", op);
    }

    return true;
}

void
ScriptAnalysis::analyzeTypes(JSContext *cx)
{
    JS_ASSERT(!ranInference());

    if (OOM()) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    if (!ranSSA()) {
        analyzeSSA(cx);
        if (failed())
            return;
    }

    



    ranInference_ = true;

    
    for (unsigned i = 0; i < script_->nfixed; i++)
        TypeScript::LocalTypes(script_, i)->addType(cx, Type::UndefinedType());

    TypeInferenceState state(cx);

    






    if (script_->types->propertyReadTypes) {
        state.hasPropertyReadTypes = true;
    } else {
        HeapTypeSet *typeArray =
            (HeapTypeSet*) cx->typeLifoAlloc().alloc(sizeof(HeapTypeSet) * numPropertyReads());
        if (!typeArray) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
        script_->types->propertyReadTypes = typeArray;
        PodZero(typeArray, numPropertyReads());

#ifdef DEBUG
        for (unsigned i = 0; i < numPropertyReads(); i++) {
            InferSpew(ISpewOps, "typeSet: %sT%p%s propertyRead%u #%u",
                      InferSpewColor(&typeArray[i]), &typeArray[i], InferSpewColorReset(),
                      i, script_->id());
        }
#endif
    }

    unsigned offset = 0;
    while (offset < script_->length) {
        Bytecode *code = maybeCode(offset);

        jsbytecode *pc = script_->code + offset;

        if (code && !analyzeTypesBytecode(cx, offset, state)) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }

        offset += GetBytecodeLength(pc);
    }

    JS_ASSERT(state.propertyReadIndex == numPropertyReads());

    for (unsigned i = 0; i < state.phiNodes.length(); i++) {
        SSAPhiNode *node = state.phiNodes[i];
        for (unsigned j = 0; j < node->length; j++) {
            const SSAValue &v = node->options[j];
            getValueTypes(v)->addSubset(cx, &node->types);
        }
    }

    




    TypeResult *result = script_->types->dynamicList;
    while (result) {
        if (result->offset != UINT32_MAX) {
            pushedTypes(result->offset)->addType(cx, result->type);
        } else {
            
            state.forTypes->addType(cx, Type::UnknownType());
        }
        result = result->next;
    }

    if (!script_->hasFreezeConstraints) {
        RootedScript script(cx, script_);
        TypeScript::AddFreezeConstraints(cx, script);
        script_->hasFreezeConstraints = true;
    }
}

bool
ScriptAnalysis::integerOperation(jsbytecode *pc)
{
    JS_ASSERT(uint32_t(pc - script_->code) < script_->length);

    switch (JSOp(*pc)) {
      case JSOP_ADD:
      case JSOP_SUB:
      case JSOP_MUL:
      case JSOP_DIV:
        if (pushedTypes(pc, 0)->getKnownTypeTag() != JSVAL_TYPE_INT32)
            return false;
        if (poppedTypes(pc, 0)->getKnownTypeTag() != JSVAL_TYPE_INT32)
            return false;
        if (poppedTypes(pc, 1)->getKnownTypeTag() != JSVAL_TYPE_INT32)
            return false;
        return true;

      default:
        return true;
    }
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
        if (!object->newScript)
            return;
        





        if (!(object->flags & OBJECT_FLAG_NEW_SCRIPT_CLEARED) && source->ownProperty(true))
            object->clearNewScript(cx);
    }

    void newType(JSContext *cx, TypeSet *source, Type type) {}
};

static bool
AddClearDefiniteGetterSetterForPrototypeChain(JSContext *cx, TypeObject *type, jsid id)
{
    




    RootedObject parent(cx, type->proto);
    while (parent) {
        TypeObject *parentObject = parent->getType(cx);
        if (!parentObject || parentObject->unknownProperties())
            return false;
        HeapTypeSet *parentTypes = parentObject->getProperty(cx, id, false);
        if (!parentTypes || parentTypes->ownProperty(true))
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
        if (object->flags & OBJECT_FLAG_NEW_SCRIPT_CLEARED)
            return;

        if (source->baseFlags() || source->getObjectCount() > 1)
            object->clearNewScript(cx);
    }
};

struct NewScriptPropertiesState
{
    RootedFunction thisFunction;
    RootedObject baseobj;
    Vector<TypeNewScript::Initializer> initializerList;
    Vector<jsid> accessedProperties;

    NewScriptPropertiesState(JSContext *cx)
      : thisFunction(cx), baseobj(cx), initializerList(cx), accessedProperties(cx)
    {}
};

static bool
AnalyzePoppedThis(JSContext *cx, Vector<SSAUseChain *> *pendingPoppedThis,
                  TypeObject *type, JSFunction *fun, NewScriptPropertiesState &state);

static bool
AnalyzeNewScriptProperties(JSContext *cx, TypeObject *type, HandleFunction fun,
                           NewScriptPropertiesState &state)
{
    









    if (state.initializerList.length() > 50) {
        



        return false;
    }

    RootedScript script(cx, fun->nonLazyScript());
    if (!script->ensureRanAnalysis(cx) || !script->ensureRanInference(cx)) {
        state.baseobj = NULL;
        cx->compartment->types.setPendingNukeTypes(cx);
        return false;
    }

    ScriptAnalysis *analysis = script->analysis();

    









    Vector<SSAUseChain *> pendingPoppedThis(cx);

    



    uint32_t lastThisPopped = 0;

    bool entirelyAnalyzed = true;
    unsigned nextOffset = 0;
    while (nextOffset < script->length) {
        unsigned offset = nextOffset;
        jsbytecode *pc = script->code + offset;

        JSOp op = JSOp(*pc);

        nextOffset += GetBytecodeLength(pc);

        Bytecode *code = analysis->maybeCode(pc);
        if (!code)
            continue;

        



        if (op == JSOP_RETURN || op == JSOP_STOP || op == JSOP_RETRVAL) {
            if (offset < lastThisPopped) {
                state.baseobj = NULL;
                entirelyAnalyzed = false;
                break;
            }

            entirelyAnalyzed = code->unconditional;
            break;
        }

        
        if (op == JSOP_EVAL) {
            if (offset < lastThisPopped)
                state.baseobj = NULL;
            entirelyAnalyzed = false;
            break;
        }

        



        if (op != JSOP_THIS)
            continue;

        SSAValue thisv = SSAValue::PushedValue(offset, 0);
        SSAUseChain *uses = analysis->useChain(thisv);

        JS_ASSERT(uses);
        if (uses->next || !uses->popped) {
            
            entirelyAnalyzed = false;
            break;
        }

        
        Bytecode *poppedCode = analysis->maybeCode(uses->offset);
        if (!poppedCode || !poppedCode->unconditional) {
            entirelyAnalyzed = false;
            break;
        }

        




        if (!pendingPoppedThis.empty() &&
            offset >= pendingPoppedThis.back()->offset) {
            lastThisPopped = pendingPoppedThis[0]->offset;
            if (!AnalyzePoppedThis(cx, &pendingPoppedThis, type, fun, state))
                return false;
        }

        if (!pendingPoppedThis.append(uses)) {
            entirelyAnalyzed = false;
            break;
        }
    }

    













    if (entirelyAnalyzed &&
        !pendingPoppedThis.empty() &&
        !AnalyzePoppedThis(cx, &pendingPoppedThis, type, fun, state))
    {
        return false;
    }

    
    return entirelyAnalyzed;
}

static bool
AnalyzePoppedThis(JSContext *cx, Vector<SSAUseChain *> *pendingPoppedThis,
                  TypeObject *type, JSFunction *fun, NewScriptPropertiesState &state)
{
    RootedScript script(cx, fun->nonLazyScript());
    ScriptAnalysis *analysis = script->analysis();

    while (!pendingPoppedThis->empty()) {
        SSAUseChain *uses = pendingPoppedThis->back();
        pendingPoppedThis->popBack();

        jsbytecode *pc = script->code + uses->offset;
        JSOp op = JSOp(*pc);

        if (op == JSOP_SETPROP && uses->u.which == 1) {
            




            RootedId id(cx, NameToId(script->getName(GET_UINT32_INDEX(pc))));
            if (IdToTypeId(id) != id)
                return false;
            if (id_prototype(cx) == id || id___proto__(cx) == id || id_constructor(cx) == id)
                return false;

            



            for (size_t i = 0; i < state.accessedProperties.length(); i++) {
                if (state.accessedProperties[i] == id)
                    return false;
            }

            if (!AddClearDefiniteGetterSetterForPrototypeChain(cx, type, id))
                return false;

            unsigned slotSpan = state.baseobj->slotSpan();
            RootedValue value(cx, UndefinedValue());
            if (!DefineNativeProperty(cx, state.baseobj, id, value, NULL, NULL,
                                      JSPROP_ENUMERATE, 0, 0, DNP_SKIP_TYPE)) {
                cx->compartment->types.setPendingNukeTypes(cx);
                state.baseobj = NULL;
                return false;
            }

            if (state.baseobj->inDictionaryMode()) {
                state.baseobj = NULL;
                return false;
            }

            if (state.baseobj->slotSpan() == slotSpan) {
                
                return false;
            }

            TypeNewScript::Initializer setprop(TypeNewScript::Initializer::SETPROP, uses->offset);
            if (!state.initializerList.append(setprop)) {
                cx->compartment->types.setPendingNukeTypes(cx);
                state.baseobj = NULL;
                return false;
            }

            if (state.baseobj->slotSpan() >= (TYPE_FLAG_DEFINITE_MASK >> TYPE_FLAG_DEFINITE_SHIFT)) {
                
                return false;
            }
        } else if (op == JSOP_GETPROP && uses->u.which == 0) {
            










            RootedId id(cx, NameToId(script->getName(GET_UINT32_INDEX(pc))));
            if (IdToTypeId(id) != id)
                return false;
            if (!state.baseobj->nativeLookup(cx, id) && !state.accessedProperties.append(id.get())) {
                cx->compartment->types.setPendingNukeTypes(cx);
                state.baseobj = NULL;
                return false;
            }

            if (!AddClearDefiniteGetterSetterForPrototypeChain(cx, type, id))
                return false;

            




            Shape *shape = type->proto ? type->proto->nativeLookup(cx, id) : NULL;
            if (shape && shape->hasSlot()) {
                Value protov = type->proto->getSlot(shape->slot());
                TypeSet *types = script->analysis()->bytecodeTypes(pc);
                types->addType(cx, GetValueType(cx, protov));
            }
        } else if ((op == JSOP_FUNCALL || op == JSOP_FUNAPPLY) && uses->u.which == GET_ARGC(pc) - 1) {
            











            
            SSAValue calleev = analysis->poppedValue(pc, GET_ARGC(pc) + 1);
            if (calleev.kind() != SSAValue::PUSHED)
                return false;
            jsbytecode *calleepc = script->code + calleev.pushedOffset();
            if (JSOp(*calleepc) != JSOP_CALLPROP)
                return false;

            



            analysis->breakTypeBarriersSSA(cx, analysis->poppedValue(calleepc, 0));
            analysis->breakTypeBarriers(cx, calleepc - script->code, true);

            StackTypeSet *funcallTypes = analysis->poppedTypes(pc, GET_ARGC(pc) + 1);
            StackTypeSet *scriptTypes = analysis->poppedTypes(pc, GET_ARGC(pc));

            
            RootedFunction function(cx);
            {
                RawObject funcallObj = funcallTypes->getSingleton();
                RawObject scriptObj = scriptTypes->getSingleton();
                if (!funcallObj || !funcallObj->isFunction() ||
                    funcallObj->toFunction()->isInterpreted() ||
                    !scriptObj || !scriptObj->isFunction() ||
                    !scriptObj->toFunction()->isInterpreted()) {
                    return false;
                }
                Native native = funcallObj->toFunction()->native();
                if (native != js_fun_call && native != js_fun_apply)
                    return false;
                function = scriptObj->toFunction();
            }

            



            funcallTypes->add(cx,
                cx->analysisLifoAlloc().new_<TypeConstraintClearDefiniteSingle>(type));
            scriptTypes->add(cx,
                cx->analysisLifoAlloc().new_<TypeConstraintClearDefiniteSingle>(type));

            TypeNewScript::Initializer pushframe(TypeNewScript::Initializer::FRAME_PUSH, uses->offset);
            if (!state.initializerList.append(pushframe)) {
                cx->compartment->types.setPendingNukeTypes(cx);
                state.baseobj = NULL;
                return false;
            }

            if (!AnalyzeNewScriptProperties(cx, type, function, state))
                return false;

            TypeNewScript::Initializer popframe(TypeNewScript::Initializer::FRAME_POP, 0);
            if (!state.initializerList.append(popframe)) {
                cx->compartment->types.setPendingNukeTypes(cx);
                state.baseobj = NULL;
                return false;
            }

            



        } else {
            
            return false;
        }
    }

    return true;
}






static void
CheckNewScriptProperties(JSContext *cx, HandleTypeObject type, HandleFunction fun)
{
    if (type->unknownProperties())
        return;

    NewScriptPropertiesState state(cx);
    state.thisFunction = fun;

    
    state.baseobj = NewBuiltinClassInstance(cx, &ObjectClass, gc::FINALIZE_OBJECT16);
    if (!state.baseobj) {
        if (type->newScript)
            type->clearNewScript(cx);
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    AnalyzeNewScriptProperties(cx, type, fun, state);
    if (!state.baseobj ||
        state.baseobj->slotSpan() == 0 ||
        !!(type->flags & OBJECT_FLAG_NEW_SCRIPT_CLEARED))
    {
        if (type->newScript)
            type->clearNewScript(cx);
        return;
    }

    




    if (type->newScript) {
        if (!type->matchDefiniteProperties(state.baseobj))
            type->clearNewScript(cx);
        return;
    }

    gc::AllocKind kind = gc::GetGCObjectKind(state.baseobj->slotSpan());

    
    JS_ASSERT(gc::GetGCKindSlots(kind) >= state.baseobj->slotSpan());

    TypeNewScript::Initializer done(TypeNewScript::Initializer::DONE, 0);

    




    RootedShape shape(cx, state.baseobj->lastProperty());
    state.baseobj = NewReshapedObject(cx, type, state.baseobj->getParent(), kind, shape);
    if (!state.baseobj ||
        !type->addDefiniteProperties(cx, state.baseobj) ||
        !state.initializerList.append(done)) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    size_t numBytes = sizeof(TypeNewScript)
                    + (state.initializerList.length() * sizeof(TypeNewScript::Initializer));
#ifdef JSGC_ROOT_ANALYSIS
    
    void *p;
    do {
        p = cx->calloc_(numBytes);
    } while (IsPoisonedPtr(p));
    type->newScript = (TypeNewScript *) p;
#else
    type->newScript = (TypeNewScript *) cx->calloc_(numBytes);
#endif

    if (!type->newScript) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    type->newScript->fun = fun;
    type->newScript->allocKind = kind;
    type->newScript->shape = state.baseobj->lastProperty();

    type->newScript->initializerList = (TypeNewScript::Initializer *)
        ((char *) type->newScript.get() + sizeof(TypeNewScript));
    PodCopy(type->newScript->initializerList,
            state.initializerList.begin(),
            state.initializerList.length());
}





void
ScriptAnalysis::printTypes(JSContext *cx)
{
    AutoEnterAnalysis enter(NULL, script_->compartment());
    TypeCompartment *compartment = &script_->compartment()->types;

    



    for (unsigned offset = 0; offset < script_->length; offset++) {
        if (!maybeCode(offset))
            continue;

        jsbytecode *pc = script_->code + offset;

        if (js_CodeSpec[*pc].format & JOF_DECOMPOSE)
            continue;

        unsigned defCount = GetDefCount(script_, offset);
        if (!defCount)
            continue;

        for (unsigned i = 0; i < defCount; i++) {
            TypeSet *types = pushedTypes(offset, i);

            if (types->unknown()) {
                compartment->typeCountOver++;
                continue;
            }

            unsigned typeCount = 0;

            if (types->hasAnyFlag(TYPE_FLAG_ANYOBJECT) || types->getObjectCount() != 0)
                typeCount++;
            for (TypeFlags flag = 1; flag < TYPE_FLAG_ANYOBJECT; flag <<= 1) {
                if (types->hasAnyFlag(flag))
                    typeCount++;
            }

            




            if (types->hasAnyFlag(TYPE_FLAG_DOUBLE)) {
                JS_ASSERT(types->hasAnyFlag(TYPE_FLAG_INT32));
                typeCount--;
            }

            if (typeCount > TypeCompartment::TYPE_COUNT_LIMIT) {
                compartment->typeCountOver++;
            } else if (typeCount == 0) {
                
            } else {
                compartment->typeCounts[typeCount-1]++;
            }
        }
    }

#ifdef DEBUG

    if (script_->function())
        printf("Function");
    else if (script_->isCachedEval)
        printf("Eval");
    else
        printf("Main");
    printf(" #%u %s (line %d):\n", script_->id(), script_->filename(), script_->lineno);

    printf("locals:");
    printf("\n    return:");
    TypeScript::ReturnTypes(script_)->print();
    printf("\n    this:");
    TypeScript::ThisTypes(script_)->print();

    for (unsigned i = 0; script_->function() && i < script_->function()->nargs; i++) {
        printf("\n    arg%u:", i);
        TypeScript::ArgTypes(script_, i)->print();
    }
    for (unsigned i = 0; i < script_->nfixed; i++) {
        if (!trackSlot(LocalSlot(script_, i))) {
            printf("\n    local%u:", i);
            TypeScript::LocalTypes(script_, i)->print();
        }
    }
    printf("\n");

    RootedScript script(cx, script_);
    for (unsigned offset = 0; offset < script_->length; offset++) {
        if (!maybeCode(offset))
            continue;

        jsbytecode *pc = script_->code + offset;

        PrintBytecode(cx, script, pc);

        if (js_CodeSpec[*pc].format & JOF_DECOMPOSE)
            continue;

        if (js_CodeSpec[*pc].format & JOF_TYPESET) {
            TypeSet *types = script_->analysis()->bytecodeTypes(pc);
            printf("  typeset %d:", (int) (types - script_->types->typeArray()));
            types->print();
            printf("\n");
        }

        unsigned defCount = GetDefCount(script_, offset);
        for (unsigned i = 0; i < defCount; i++) {
            printf("  type %d:", i);
            pushedTypes(offset, i)->print();
            printf("\n");
        }

        if (getCode(offset).monitoredTypes)
            printf("  monitored\n");

        TypeBarrier *barrier = getCode(offset).typeBarriers;
        if (barrier != NULL) {
            printf("  barrier:");
            while (barrier) {
                printf(" %s", TypeString(barrier->type));
                barrier = barrier->next;
            }
            printf("\n");
        }
    }

    printf("\n");

#endif 

}





void
types::MarkIteratorUnknownSlow(JSContext *cx)
{
    

    jsbytecode *pc;
    RootedScript script(cx, cx->stack.currentScript(&pc));
    if (!script || !pc)
        return;

    if (JSOp(*pc) != JSOP_ITER)
        return;

    AutoEnterAnalysis enter(cx);

    








    TypeResult *result = script->types->dynamicList;
    while (result) {
        if (result->offset == UINT32_MAX) {
            
            JS_ASSERT(result->type.isUnknown());
            return;
        }
        result = result->next;
    }

    InferSpew(ISpewOps, "externalType: customIterator #%u", script->id());

    result = cx->new_<TypeResult>(UINT32_MAX, Type::UnknownType());
    if (!result) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }
    result->next = script->types->dynamicList;
    script->types->dynamicList = result;

    AddPendingRecompile(cx, script, NULL);

    if (!script->hasAnalysis() || !script->analysis()->ranInference())
        return;

    ScriptAnalysis *analysis = script->analysis();

    for (unsigned i = 0; i < script->length; i++) {
        jsbytecode *pc = script->code + i;
        if (!analysis->maybeCode(pc))
            continue;
        if (JSOp(*pc) == JSOP_ITERNEXT)
            analysis->pushedTypes(pc, 0)->addType(cx, Type::UnknownType());
    }
}

void
types::TypeMonitorCallSlow(JSContext *cx, JSObject *callee, const CallArgs &args,
                           bool constructing)
{
    unsigned nargs = callee->toFunction()->nargs;
    JSScript *script = callee->toFunction()->nonLazyScript();

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
types::TypeDynamicResult(JSContext *cx, JSScript *script, jsbytecode *pc, Type type)
{
    JS_ASSERT(cx->typeInferenceEnabled());
    AutoEnterAnalysis enter(cx);

    
    if (js_CodeSpec[*pc].format & JOF_TYPESET) {
        if (!script->ensureRanAnalysis(cx)) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
        TypeSet *types = script->analysis()->bytecodeTypes(pc);
        if (!types->hasType(type)) {
            InferSpew(ISpewOps, "externalType: monitorResult #%u:%05u: %s",
                      script->id(), pc - script->code, TypeString(type));
            types->addType(cx, type);
        }
        return;
    }

    if (script->hasAnalysis() && script->analysis()->ranInference()) {
        




        TypeSet *pushed = script->analysis()->pushedTypes(pc, 0);
        if (pushed->hasType(type))
            return;
    } else {
        
        TypeResult *result, **pstart = &script->types->dynamicList, **presult = pstart;
        while (*presult) {
            result = *presult;
            if (result->offset == unsigned(pc - script->code) && result->type == type) {
                if (presult != pstart) {
                    
                    *presult = result->next;
                    result->next = *pstart;
                    *pstart = result;
                }
                return;
            }
            presult = &result->next;
        }
    }

    InferSpew(ISpewOps, "externalType: monitorResult #%u:%05u: %s",
              script->id(), pc - script->code, TypeString(type));

    TypeResult *result = cx->new_<TypeResult>(pc - script->code, type);
    if (!result) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }
    result->next = script->types->dynamicList;
    script->types->dynamicList = result;

    














    jsbytecode *ignorePC = pc + GetBytecodeLength(pc);
    if (*ignorePC == JSOP_POP) {
        
    } else if (*ignorePC == JSOP_INT8 && GET_INT8(ignorePC) == -1) {
        ignorePC += JSOP_INT8_LENGTH;
        if (*ignorePC != JSOP_BITAND)
            ignorePC = NULL;
    } else if (*ignorePC == JSOP_ZERO) {
        ignorePC += JSOP_ZERO_LENGTH;
        if (*ignorePC != JSOP_BITOR)
            ignorePC = NULL;
    } else {
        ignorePC = NULL;
    }

    if (ignorePC) {
        AddPendingRecompile(cx, script, pc);
        AddPendingRecompile(cx, script, ignorePC);
    } else {
        AddPendingRecompile(cx, script, NULL);
    }

    if (script->hasAnalysis() && script->analysis()->ranInference()) {
        TypeSet *pushed = script->analysis()->pushedTypes(pc, 0);
        pushed->addType(cx, type);
    }
}

void
types::TypeMonitorResult(JSContext *cx, JSScript *script, jsbytecode *pc, const js::Value &rval)
{
    
    if (!(js_CodeSpec[*pc].format & JOF_TYPESET))
        return;

    AutoEnterAnalysis enter(cx);

    if (!script->ensureRanAnalysis(cx)) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    Type type = GetValueType(cx, rval);
    TypeSet *types = script->analysis()->bytecodeTypes(pc);
    if (types->hasType(type))
        return;

    InferSpew(ISpewOps, "bytecodeType: #%u:%05u: %s",
              script->id(), pc - script->code, TypeString(type));
    types->addType(cx, type);
}









static inline bool
IgnorePushed(const jsbytecode *pc, unsigned index)
{
    switch (JSOp(*pc)) {
      
      case JSOP_BINDNAME:
      case JSOP_BINDGNAME:
      case JSOP_BINDINTRINSIC:
        return true;

      
      case JSOP_IN:
      case JSOP_EQ:
      case JSOP_NE:
      case JSOP_LT:
      case JSOP_LE:
      case JSOP_GT:
      case JSOP_GE:
        return (index == 0);

      
      case JSOP_OR:
      case JSOP_AND:
        return (index == 0);

      
      case JSOP_HOLE:
        return (index == 0);

      
      case JSOP_ENTERWITH:
      case JSOP_ENTERBLOCK:
      case JSOP_ENTERLET0:
      case JSOP_ENTERLET1:
        return true;

      
      case JSOP_ITER:
      case JSOP_ITERNEXT:
      case JSOP_MOREITER:
      case JSOP_ENDITER:
        return true;

      
      case JSOP_DUP:
      case JSOP_DUP2:
      case JSOP_SWAP:
      case JSOP_PICK:
        return true;

      
      case JSOP_FINALLY:
        return true;

      





      case JSOP_GETLOCAL:
        return JSOp(pc[JSOP_GETLOCAL_LENGTH]) == JSOP_POP;

      default:
        return false;
    }
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
        return true;
    }

    AutoEnterAnalysis enter(cx);

    unsigned count = TypeScript::NumTypeSets(this);

    types = (TypeScript *) cx->calloc_(sizeof(TypeScript) + (sizeof(TypeSet) * count));
    if (!types) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return false;
    }

    new(types) TypeScript();

    TypeSet *typeArray = types->typeArray();
    TypeSet *returnTypes = TypeScript::ReturnTypes(this);

    for (unsigned i = 0; i < count; i++) {
        TypeSet *types = &typeArray[i];
        if (types != returnTypes)
            types->setConstraintsPurged();
    }

#ifdef DEBUG
    for (unsigned i = 0; i < nTypeSets; i++)
        InferSpew(ISpewOps, "typeSet: %sT%p%s bytecode%u #%u",
                  InferSpewColor(&typeArray[i]), &typeArray[i], InferSpewColorReset(),
                  i, id());
    InferSpew(ISpewOps, "typeSet: %sT%p%s return #%u",
              InferSpewColor(returnTypes), returnTypes, InferSpewColorReset(),
              id());
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
    for (unsigned i = 0; i < nfixed; i++) {
        TypeSet *types = TypeScript::LocalTypes(this, i);
        InferSpew(ISpewOps, "typeSet: %sT%p%s local%u #%u",
                  InferSpewColor(types), types, InferSpewColorReset(),
                  i, id());
    }
#endif

    return true;
}

bool
JSScript::makeAnalysis(JSContext *cx)
{
    JS_ASSERT(types && !types->analysis);

    AutoEnterAnalysis enter(cx);

    types->analysis = cx->analysisLifoAlloc().new_<ScriptAnalysis>(this);

    if (!types->analysis)
        return false;

    RootedScript self(cx, this);

    self->types->analysis->analyzeBytecode(cx);

    if (self->types->analysis->OOM()) {
        self->types->analysis = NULL;
        return false;
    }

    return true;
}

 bool
JSFunction::setTypeForScriptedFunction(JSContext *cx, HandleFunction fun, bool singleton )
{
    JS_ASSERT(fun->nonLazyScript());
    JS_ASSERT(fun->nonLazyScript()->function() == fun);

    if (!cx->typeInferenceEnabled())
        return true;

    if (singleton) {
        if (!setSingletonType(cx, fun))
            return false;
    } else if (UseNewTypeForClone(fun)) {
        



    } else {
        RootedObject funProto(cx, fun->getProto());
        TypeObject *type = cx->compartment->types.newTypeObject(cx, &FunctionClass, funProto);
        if (!type)
            return false;

        fun->setType(type);
        type->interpretedFunction = fun;
    }

    return true;
}

#ifdef DEBUG

 void
TypeScript::CheckBytecode(JSContext *cx, HandleScript script, jsbytecode *pc, const js::Value *sp)
{
    AutoEnterAnalysis enter(cx);

    if (js_CodeSpec[*pc].format & JOF_DECOMPOSE)
        return;

    if (!script->hasAnalysis() || !script->analysis()->ranInference())
        return;
    ScriptAnalysis *analysis = script->analysis();

    int defCount = GetDefCount(script, pc - script->code);

    for (int i = 0; i < defCount; i++) {
        const js::Value &val = sp[-defCount + i];
        TypeSet *types = analysis->pushedTypes(pc, i);
        if (IgnorePushed(pc, i))
            continue;

        



        if (val.isUndefined())
            continue;

        Type type = GetValueType(cx, val);

        if (!types->hasType(type)) {
            
            fprintf(stderr, "Missing type at #%u:%05u pushed %u: %s\n",
                    script->id(), unsigned(pc - script->code), i, TypeString(type));
            TypeFailure(cx, "Missing type pushed %u: %s", i, TypeString(type));
        }
    }
}

#endif





bool
JSObject::shouldSplicePrototype(JSContext *cx)
{
    







    if (getProto() != NULL)
        return false;
    return !cx->typeInferenceEnabled() || hasSingletonType();
}

bool
JSObject::splicePrototype(JSContext *cx, Class *clasp, Handle<TaggedProto> proto)
{
    JS_ASSERT(cx->compartment == compartment());

    RootedObject self(cx, this);

    





    JS_ASSERT_IF(cx->typeInferenceEnabled(), self->hasSingletonType());

    
    JS_ASSERT_IF(proto.isObject(), !proto.toObject()->getClass()->ext.outerObject);

    



    Rooted<TypeObject*> type(cx, self->getType(cx));
    if (!type)
        return false;
    Rooted<TypeObject*> protoType(cx, NULL);
    if (proto.isObject()) {
        protoType = proto.toObject()->getType(cx);
        if (!protoType)
            return false;
    }

    if (!cx->typeInferenceEnabled()) {
        TypeObject *type = cx->compartment->getNewType(cx, clasp, proto);
        if (!type)
            return false;
        self->type_ = type;
        return true;
    }

    type->clasp = clasp;
    type->proto = proto.raw();

    AutoEnterAnalysis enter(cx);

    if (protoType && protoType->unknownProperties() && !type->unknownProperties()) {
        type->markUnknown(cx);
        return true;
    }

    if (!type->unknownProperties()) {
        
        unsigned count = type->getPropertyCount();
        for (unsigned i = 0; i < count; i++) {
            Property *prop = type->getProperty(i);
            if (prop && prop->types.hasPropagatedProperty())
                type->getFromPrototypes(cx, prop->id, &prop->types, true);
        }
    }

    return true;
}

 TypeObject *
JSObject::makeLazyType(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->hasLazyType());
    JS_ASSERT(cx->compartment == obj->compartment());

    
    if (obj->isFunction() && obj->toFunction()->isInterpretedLazy()) {
        RootedFunction fun(cx, obj->toFunction());
        if (!fun->getOrCreateScript(cx))
            return NULL;
    }
    Rooted<TaggedProto> proto(cx, obj->getTaggedProto());
    TypeObject *type = cx->compartment->types.newTypeObject(cx, obj->getClass(), proto);
    if (!type) {
        if (cx->typeInferenceEnabled())
            cx->compartment->types.setPendingNukeTypes(cx);
        return obj->type_;
    }

    if (!cx->typeInferenceEnabled()) {
        
        obj->type_ = type;
        return type;
    }

    AutoEnterAnalysis enter(cx);

    

    type->singleton = obj;

    if (obj->isFunction() && obj->toFunction()->isInterpreted()) {
        type->interpretedFunction = obj->toFunction();
        if (type->interpretedFunction->nonLazyScript()->uninlineable)
            type->flags |= OBJECT_FLAG_UNINLINEABLE;
    }

    if (obj->lastProperty()->hasObjectFlag(BaseShape::ITERATED_SINGLETON))
        type->flags |= OBJECT_FLAG_ITERATED;

    if (obj->getClass()->emulatesUndefined())
        type->flags |= OBJECT_FLAG_EMULATES_UNDEFINED;

    




    
    type->flags |= OBJECT_FLAG_NON_PACKED;

    if (obj->isIndexed())
        type->flags |= OBJECT_FLAG_SPARSE_INDEXES;

    if (obj->isArray() && obj->getArrayLength() > INT32_MAX)
        type->flags |= OBJECT_FLAG_LENGTH_OVERFLOW;

    obj->type_ = type;

    return type;
}

 inline HashNumber
TypeObjectEntry::hash(const Lookup &lookup)
{
    return PointerHasher<JSObject *, 3>::hash(lookup.proto.raw()) ^
           PointerHasher<Class *, 3>::hash(lookup.clasp);
}

 inline bool
TypeObjectEntry::match(TypeObject *key, const Lookup &lookup)
{
    return key->proto == lookup.proto.raw() && key->clasp == lookup.clasp;
}

#ifdef DEBUG
bool
JSObject::hasNewType(Class *clasp, TypeObject *type)
{
    TypeObjectSet &table = compartment()->newTypeObjects;

    if (!table.initialized())
        return false;

    TypeObjectSet::Ptr p = table.lookup(TypeObjectSet::Lookup(clasp, this));
    return p && *p == type;
}
#endif 

 bool
JSObject::setNewTypeUnknown(JSContext *cx, Class *clasp, HandleObject obj)
{
    if (!obj->setFlag(cx, js::BaseShape::NEW_TYPE_UNKNOWN))
        return false;

    




    TypeObjectSet &table = cx->compartment->newTypeObjects;
    if (table.initialized()) {
        if (TypeObjectSet::Ptr p = table.lookup(TypeObjectSet::Lookup(clasp, obj.get())))
            MarkTypeObjectUnknownProperties(cx, *p);
    }

    return true;
}

TypeObject *
JSCompartment::getNewType(JSContext *cx, Class *clasp, TaggedProto proto_, JSFunction *fun_)
{
    JS_ASSERT_IF(fun_, proto_.isObject());
    JS_ASSERT_IF(proto_.isObject(), cx->compartment == proto_.toObject()->compartment());

    if (!newTypeObjects.initialized() && !newTypeObjects.init())
        return NULL;

    TypeObjectSet::AddPtr p = newTypeObjects.lookupForAdd(TypeObjectSet::Lookup(clasp, proto_));
    if (p) {
        TypeObject *type = *p;

        










        if (type->newScript && type->newScript->fun != fun_)
            type->clearNewScript(cx);

        return type;
    }

    Rooted<TaggedProto> proto(cx, proto_);
    RootedFunction fun(cx, fun_);

    if (proto.isObject() && !proto.toObject()->setDelegate(cx))
        return NULL;

    bool markUnknown =
        proto.isObject()
        ? proto.toObject()->lastProperty()->hasObjectFlag(BaseShape::NEW_TYPE_UNKNOWN)
        : true;

    RootedTypeObject type(cx, types.newTypeObject(cx, clasp, proto, markUnknown));
    if (!type)
        return NULL;

    if (!newTypeObjects.relookupOrAdd(p, TypeObjectSet::Lookup(clasp, proto), type.get()))
        return NULL;

    if (!cx->typeInferenceEnabled())
        return type;

    AutoEnterAnalysis enter(cx);

    




    if (proto.isObject()) {
        RootedObject obj(cx, proto.toObject());

        if (fun)
            CheckNewScriptProperties(cx, type, fun);

        if (obj->isRegExp()) {
            AddTypeProperty(cx, type, "source", types::Type::StringType());
            AddTypeProperty(cx, type, "global", types::Type::BooleanType());
            AddTypeProperty(cx, type, "ignoreCase", types::Type::BooleanType());
            AddTypeProperty(cx, type, "multiline", types::Type::BooleanType());
            AddTypeProperty(cx, type, "sticky", types::Type::BooleanType());
            AddTypeProperty(cx, type, "lastIndex", types::Type::Int32Type());
        }

        if (obj->isString())
            AddTypeProperty(cx, type, "length", Type::Int32Type());
    }

    







    if (type->unknownProperties())
        type->flags |= OBJECT_FLAG_SETS_MARKED_UNKNOWN;

    return type;
}

TypeObject *
JSObject::getNewType(JSContext *cx, Class *clasp, JSFunction *fun)
{
    return cx->compartment->getNewType(cx, clasp, this, fun);
}

TypeObject *
JSCompartment::getLazyType(JSContext *cx, Class *clasp, TaggedProto proto)
{
    JS_ASSERT(cx->compartment == this);
    JS_ASSERT_IF(proto.isObject(), cx->compartment == proto.toObject()->compartment());

    AutoEnterAnalysis enter(cx);

    TypeObjectSet &table = cx->compartment->lazyTypeObjects;

    if (!table.initialized() && !table.init())
        return NULL;

    TypeObjectSet::AddPtr p = table.lookupForAdd(TypeObjectSet::Lookup(clasp, proto));
    if (p) {
        TypeObject *type = *p;
        JS_ASSERT(type->lazy());

        return type;
    }

    Rooted<TaggedProto> protoRoot(cx, proto);
    TypeObject *type = cx->compartment->types.newTypeObject(cx, clasp, protoRoot, false);
    if (!type)
        return NULL;

    if (!table.relookupOrAdd(p, TypeObjectSet::Lookup(clasp, protoRoot), type))
        return NULL;

    type->singleton = (JSObject *) TypeObject::LAZY_SINGLETON;

    return type;
}





void
TypeSet::sweep(JSCompartment *compartment)
{
    JS_ASSERT(!purged());
    JS_ASSERT(compartment->zone()->isGCSweeping());

    





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
                        (compartment->typeLifoAlloc, objectSet, objectCount, object);
                if (pentry)
                    *pentry = object;
                else
                    compartment->types.setPendingNukeTypesNoReport();
            }
        }
        setBaseObjectCount(objectCount);
    } else if (objectCount == 1) {
        TypeObjectKey *object = (TypeObjectKey *) objectSet;
        if (IsAboutToBeFinalized(object)) {
            objectSet = NULL;
            setBaseObjectCount(0);
        }
    }

    



    constraintList = NULL;
    flags &= ~TYPE_FLAG_PROPAGATED_PROPERTY;
}

inline void
TypeObject::clearProperties()
{
    setBasePropertyCount(0);
    propertySet = NULL;
}








inline void
TypeObject::sweep(FreeOp *fop)
{
    



    contribution = 0;

    if (singleton) {
        JS_ASSERT(!newScript);

        



        clearProperties();

        return;
    }

    JSCompartment *compartment = this->compartment();
    JS_ASSERT(compartment->zone()->isGCSweeping());

    if (!isMarked()) {
        if (newScript)
            fop->free_(newScript);
        return;
    }

    





    unsigned propertyCount = basePropertyCount();
    if (propertyCount >= 2) {
        unsigned oldCapacity = HashSetCapacity(propertyCount);
        Property **oldArray = propertySet;

        clearProperties();
        propertyCount = 0;
        for (unsigned i = 0; i < oldCapacity; i++) {
            Property *prop = oldArray[i];
            if (prop && prop->types.ownProperty(false)) {
                Property *newProp = compartment->typeLifoAlloc.new_<Property>(*prop);
                if (newProp) {
                    Property **pentry =
                        HashSetInsert<jsid,Property,Property>
                            (compartment->typeLifoAlloc, propertySet, propertyCount, prop->id);
                    if (pentry) {
                        *pentry = newProp;
                        newProp->types.sweep(compartment);
                    } else {
                        compartment->types.setPendingNukeTypesNoReport();
                    }
                } else {
                    compartment->types.setPendingNukeTypesNoReport();
                }
            }
        }
        setBasePropertyCount(propertyCount);
    } else if (propertyCount == 1) {
        Property *prop = (Property *) propertySet;
        if (prop->types.ownProperty(false)) {
            Property *newProp = compartment->typeLifoAlloc.new_<Property>(*prop);
            if (newProp) {
                propertySet = (Property **) newProp;
                newProp->types.sweep(compartment);
            } else {
                compartment->types.setPendingNukeTypesNoReport();
            }
        } else {
            propertySet = NULL;
            setBasePropertyCount(0);
        }
    }

    if (basePropertyCount() <= SET_ARRAY_SIZE) {
        for (unsigned i = 0; i < basePropertyCount(); i++)
            JS_ASSERT(propertySet[i]);
    }

    




    if (newScript)
        flags |= OBJECT_FLAG_NEW_SCRIPT_REGENERATE;
}

void
SweepTypeObjects(FreeOp *fop, JSCompartment *compartment)
{
    JS_ASSERT(compartment->zone()->isGCSweeping());
    for (gc::CellIterUnderGC iter(compartment, gc::FINALIZE_TYPE_OBJECT); !iter.done(); iter.next()) {
        TypeObject *object = iter.get<TypeObject>();
        object->sweep(fop);
    }
}

void
TypeCompartment::sweep(FreeOp *fop)
{
    JSCompartment *compartment = this->compartment();
    JS_ASSERT(compartment->zone()->isGCSweeping());

    SweepTypeObjects(fop, compartment);

    




    if (arrayTypeTable) {
        for (ArrayTypeTable::Enum e(*arrayTypeTable); !e.empty(); e.popFront()) {
            const ArrayTableKey &key = e.front().key;
            JS_ASSERT(e.front().value->proto == key.proto);
            JS_ASSERT(!key.type.isSingleObject());

            bool remove = false;
            TypeObject *typeObject = NULL;
            if (key.type.isTypeObject()) {
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
            for (unsigned i = 0; !remove && i < key.nproperties; i++) {
                if (JSID_IS_STRING(key.properties[i])) {
                    JSString *str = JSID_TO_STRING(key.properties[i]);
                    if (IsStringAboutToBeFinalized(&str))
                        remove = true;
                    JS_ASSERT(AtomToId((JSAtom *)str) == key.properties[i]);
                }
                JS_ASSERT(!entry.types[i].isSingleObject());
                TypeObject *typeObject = NULL;
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

    pendingArray = NULL;
    pendingCapacity = 0;

    sweepCompilerOutputs(fop, true);
}

void
TypeCompartment::sweepCompilerOutputs(FreeOp *fop, bool discardConstraints)
{
    if (constrainedOutputs) {
        if (discardConstraints) {
            JS_ASSERT(compiledInfo.outputIndex == RecompileInfo::NoCompilerRunning);
#if DEBUG
            for (unsigned i = 0; i < constrainedOutputs->length(); i++) {
                CompilerOutput &co = (*constrainedOutputs)[i];
                JS_ASSERT(!co.isValid());
            }
#endif

            fop->delete_(constrainedOutputs);
            constrainedOutputs = NULL;
        } else {
            
            
            
            size_t len = constrainedOutputs->length();
            for (unsigned i = 0; i < len; i++) {
                if (i != compiledInfo.outputIndex) {
                    CompilerOutput &co = (*constrainedOutputs)[i];
                    JS_ASSERT(!co.isValid());
                    co.invalidate();
                }
            }
        }
    }

    if (pendingRecompiles) {
        fop->delete_(pendingRecompiles);
        pendingRecompiles = NULL;
    }
}

void
JSCompartment::sweepNewTypeObjectTable(TypeObjectSet &table)
{
    gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_TABLES_TYPE_OBJECT);

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
    if (pendingArray)
        js_free(pendingArray);

    if (arrayTypeTable)
        js_delete(arrayTypeTable);

    if (objectTypeTable)
        js_delete(objectTypeTable);

    if (allocationSiteTable)
        js_delete(allocationSiteTable);
}

 void
TypeScript::Sweep(FreeOp *fop, RawScript script)
{
    JSCompartment *compartment = script->compartment();
    JS_ASSERT(compartment->zone()->isGCSweeping());
    JS_ASSERT(compartment->types.inferenceEnabled);

    unsigned num = NumTypeSets(script);
    TypeSet *typeArray = script->types->typeArray();

    
    for (unsigned i = 0; i < num; i++)
        typeArray[i].sweep(compartment);

    TypeResult **presult = &script->types->dynamicList;
    while (*presult) {
        TypeResult *result = *presult;
        Type type = result->type;

        if (!type.isUnknown() && !type.isAnyObject() && type.isObject() &&
            IsAboutToBeFinalized(type.objectKey()))
        {
            *presult = result->next;
            fop->delete_(result);
        } else {
            presult = &result->next;
        }
    }

    



    script->hasFreezeConstraints = false;
}

void
TypeScript::destroy()
{
    while (dynamicList) {
        TypeResult *next = dynamicList->next;
        js_delete(dynamicList);
        dynamicList = next;
    }

    js_free(this);
}

 void
TypeScript::AddFreezeConstraints(JSContext *cx, HandleScript script)
{
    
















    size_t count = TypeScript::NumTypeSets(script);
    TypeSet *returnTypes = TypeScript::ReturnTypes(script);

    TypeSet *array = script->types->typeArray();
    for (size_t i = 0; i < count; i++) {
        TypeSet *types = &array[i];
        if (types == returnTypes)
            continue;
        JS_ASSERT(types->constraintsPurged());
        types->add(cx, cx->analysisLifoAlloc().new_<TypeConstraintFreezeStack>(script), false);
    }
}

 void
TypeScript::Purge(JSContext *cx, HandleScript script)
{
    if (!script->types)
        return;

    unsigned num = NumTypeSets(script);
    TypeSet *typeArray = script->types->typeArray();
    TypeSet *returnTypes = ReturnTypes(script);

    bool ranInference = script->hasAnalysis() && script->analysis()->ranInference();

    script->clearAnalysis();

    if (!ranInference && !script->hasFreezeConstraints) {
        



        ThisTypes(script)->constraintList = NULL;
#ifdef DEBUG
        for (size_t i = 0; i < num; i++) {
            TypeSet *types = &typeArray[i];
            JS_ASSERT_IF(types != returnTypes, !types->constraintList);
        }
#endif
        return;
    }

    for (size_t i = 0; i < num; i++) {
        TypeSet *types = &typeArray[i];
        if (types != returnTypes)
            types->constraintList = NULL;
    }

    if (script->hasFreezeConstraints)
        TypeScript::AddFreezeConstraints(cx, script);
}

void
TypeCompartment::maybePurgeAnalysis(JSContext *cx, bool force)
{
    
    return;

    JS_ASSERT(this == &cx->compartment->types);
    JS_ASSERT(!cx->compartment->activeAnalysis);

    if (!cx->typeInferenceEnabled())
        return;

    size_t triggerBytes = cx->runtime->analysisPurgeTriggerBytes;
    size_t beforeUsed = cx->compartment->analysisLifoAlloc.used();

    if (!force) {
        if (!triggerBytes || triggerBytes >= beforeUsed)
            return;
    }

    AutoEnterAnalysis enter(cx);

    
    cx->compartment->analysisLifoAlloc.releaseAll();

    uint64_t start = PRMJ_Now();

    for (gc::CellIter i(cx->compartment, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        RootedScript script(cx, i.get<JSScript>());
        TypeScript::Purge(cx, script);
    }

    uint64_t done = PRMJ_Now();

    if (cx->runtime->analysisPurgeCallback) {
        size_t afterUsed = cx->compartment->analysisLifoAlloc.used();
        size_t typeUsed = cx->compartment->typeLifoAlloc.used();

        char buf[1000];
        JS_snprintf(buf, sizeof(buf),
                    "Total Time %.2f ms, %d bytes before, %d bytes after\n",
                    (done - start) / double(PRMJ_USEC_PER_MSEC),
                    (int) (beforeUsed + typeUsed),
                    (int) (afterUsed + typeUsed));

        JSString *desc = JS_NewStringCopyZ(cx, buf);
        if (!desc) {
            cx->clearPendingException();
            return;
        }

        cx->runtime->analysisPurgeCallback(cx->runtime, &desc->asFlat());
    }
}

static void
SizeOfScriptTypeInferenceData(RawScript script, TypeInferenceSizes *sizes,
                              JSMallocSizeOfFun mallocSizeOf)
{
    TypeScript *typeScript = script->types;
    if (!typeScript)
        return;

    
    if (!script->compartment()->types.inferenceEnabled) {
        sizes->typeScripts += mallocSizeOf(typeScript);
        return;
    }

    sizes->typeScripts += mallocSizeOf(typeScript);

    TypeResult *result = typeScript->dynamicList;
    while (result) {
        sizes->typeResults += mallocSizeOf(result);
        result = result->next;
    }
}

void
JSCompartment::sizeOfTypeInferenceData(TypeInferenceSizes *sizes, JSMallocSizeOfFun mallocSizeOf)
{
    sizes->analysisPool += analysisLifoAlloc.sizeOfExcludingThis(mallocSizeOf);
    sizes->typePool += typeLifoAlloc.sizeOfExcludingThis(mallocSizeOf);

    
    sizes->pendingArrays += mallocSizeOf(types.pendingArray);

    
    JS_ASSERT(!types.pendingRecompiles);

    for (gc::CellIter i(this, gc::FINALIZE_SCRIPT); !i.done(); i.next())
        SizeOfScriptTypeInferenceData(i.get<JSScript>(), sizes, mallocSizeOf);

    if (types.allocationSiteTable)
        sizes->allocationSiteTables += types.allocationSiteTable->sizeOfIncludingThis(mallocSizeOf);

    if (types.arrayTypeTable)
        sizes->arrayTypeTables += types.arrayTypeTable->sizeOfIncludingThis(mallocSizeOf);

    if (types.objectTypeTable) {
        sizes->objectTypeTables += types.objectTypeTable->sizeOfIncludingThis(mallocSizeOf);

        for (ObjectTypeTable::Enum e(*types.objectTypeTable);
             !e.empty();
             e.popFront())
        {
            const ObjectTableKey &key = e.front().key;
            const ObjectTableEntry &value = e.front().value;

            
            sizes->objectTypeTables += mallocSizeOf(key.properties) + mallocSizeOf(value.types);
        }
    }
}

size_t
TypeObject::sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf)
{
    if (singleton) {
        




        JS_ASSERT(!newScript);
        return 0;
    }

    return mallocSizeOf(newScript);
}
