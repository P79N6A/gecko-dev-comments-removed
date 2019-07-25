






































#include "jsapi.h"
#include "jsautooplen.h"
#include "jsbit.h"
#include "jsbool.h"
#include "jsdate.h"
#include "jsexn.h"
#include "jsgc.h"
#include "jsgcmark.h"
#include "jsinfer.h"
#include "jsmath.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsscript.h"
#include "jscntxt.h"
#include "jsscan.h"
#include "jsscope.h"
#include "jsstr.h"
#include "jstl.h"
#include "jsiter.h"

#include "methodjit/MethodJIT.h"
#include "methodjit/Retcon.h"

#include "jsatominlines.h"
#include "jsgcinlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"
#include "vm/Stack-inl.h"

#ifdef JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

using namespace js;
using namespace js::types;
using namespace js::analyze;

static inline jsid
id_prototype(JSContext *cx) {
    return ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom);
}

static inline jsid
id_arguments(JSContext *cx) {
    return ATOM_TO_JSID(cx->runtime->atomState.argumentsAtom);
}

static inline jsid
id_length(JSContext *cx) {
    return ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);
}

static inline jsid
id___proto__(JSContext *cx) {
    return ATOM_TO_JSID(cx->runtime->atomState.protoAtom);
}

static inline jsid
id_constructor(JSContext *cx) {
    return ATOM_TO_JSID(cx->runtime->atomState.constructorAtom);
}

static inline jsid
id_caller(JSContext *cx) {
    return ATOM_TO_JSID(cx->runtime->atomState.callerAtom);
}

static inline jsid
id_toString(JSContext *cx)
{
    return ATOM_TO_JSID(cx->runtime->atomState.toStringAtom);
}

static inline jsid
id_toSource(JSContext *cx)
{
    return ATOM_TO_JSID(cx->runtime->atomState.toSourceAtom);
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
    
    const char *env = getenv("TERM");
    if (!env)
        return false;
    return strcmp(env, "xterm-color") == 0;
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
    if (type.isSingleObject()) {
        static char bufs[4][40];
        static unsigned which = 0;
        which = (which + 1) & 3;
        JS_snprintf(bufs[which], 40, "<0x%p>", (void *) type.singleObject());
        return bufs[which];
    }
    return type.typeObject()->name();
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
        id = MakeTypeId(cx, id);

        
        if (id == id___proto__(cx) || id == id_constructor(cx) || id == id_caller(cx))
            return true;

        




        if (cx->compartment->types.pendingCount)
            return true;

        Type type = GetValueType(cx, value);

        AutoEnterTypeInference enter(cx);

        




        if (!obj->hasProperty(cx, id))
            return true;

        TypeSet *types = obj->getProperty(cx, id, false);
        if (types && !types->hasType(type)) {
            TypeFailure(cx, "Missing type in object %s %s: %s",
                        obj->name(), TypeIdString(id), TypeString(type));
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

    




    cx->compartment->types.print(cx, cx->compartment);

    
    JS_Assert(msgbuf, __FILE__, __LINE__);
    
    *((int*)NULL) = 0;  
}





TypeSet *
TypeSet::make(JSContext *cx, const char *name)
{
    JS_ASSERT(cx->compartment->activeInference);

    TypeSet *res = ArenaNew<TypeSet>(cx->compartment->pool);
    if (!res) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return NULL;
    }

    InferSpew(ISpewOps, "typeSet: %sT%p%s intermediate %s",
              InferSpewColor(res), res, InferSpewColorReset(),
              name);
    res->setIntermediate();

    return res;
}

inline void
TypeSet::add(JSContext *cx, TypeConstraint *constraint, bool callExisting)
{
    if (!constraint) {
        
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    JS_ASSERT(cx->compartment->activeInference);
    JS_ASSERT_IF(intermediate(), !constraint->persistentObject());

    InferSpew(ISpewOps, "addConstraint: %sT%p%s %sC%p%s %s",
              InferSpewColor(this), this, InferSpewColorReset(),
              InferSpewColor(constraint), constraint, InferSpewColorReset(),
              constraint->kind());

    JS_ASSERT(constraint->next == NULL);
    constraint->next = constraintList;
    constraintList = constraint;

    if (constraint->persistentObject())
        typeFlags |= TYPE_FLAG_HAS_PERSISTENT_CONSTRAINTS;

    if (!callExisting)
        return;

    if (typeFlags & TYPE_FLAG_UNKNOWN) {
        cx->compartment->types.addPending(cx, constraint, this, Type::UnknownType());
        cx->compartment->types.resolvePending(cx);
        return;
    }

    for (TypeFlags flag = 1; flag < TYPE_FLAG_ANYOBJECT; flag <<= 1) {
        if (typeFlags & flag) {
            Type type = Type::PrimitiveType(TypeFlagPrimitive(flag));
            cx->compartment->types.addPending(cx, constraint, this, type);
        }
    }

    if (typeFlags & TYPE_FLAG_ANYOBJECT) {
        cx->compartment->types.addPending(cx, constraint, this, Type::AnyObjectType());
        cx->compartment->types.resolvePending(cx);
        return;
    }

    unsigned count = getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        TypeObjectKey *object = getObject(i);
        if (object)
            cx->compartment->types.addPending(cx, constraint, this, Type::ObjectType(object));
    }

    cx->compartment->types.resolvePending(cx);
}

void
TypeSet::print(JSContext *cx)
{
    if (typeFlags & TYPE_FLAG_OWN_PROPERTY)
        printf(" [own]");
    if (typeFlags & TYPE_FLAG_CONFIGURED_PROPERTY)
        printf(" [configured]");

    if (isDefiniteProperty())
        printf(" [definite:%d]", definiteSlot());

    if (baseFlags() == 0 && !objectCount) {
        printf(" missing");
        return;
    }

    if (typeFlags & TYPE_FLAG_UNKNOWN)
        printf(" unknown");
    if (typeFlags & TYPE_FLAG_ANYOBJECT)
        printf(" object");

    if (typeFlags & TYPE_FLAG_UNDEFINED)
        printf(" void");
    if (typeFlags & TYPE_FLAG_NULL)
        printf(" null");
    if (typeFlags & TYPE_FLAG_BOOLEAN)
        printf(" bool");
    if (typeFlags & TYPE_FLAG_INT32)
        printf(" int");
    if (typeFlags & TYPE_FLAG_DOUBLE)
        printf(" float");
    if (typeFlags & TYPE_FLAG_STRING)
        printf(" string");
    if (typeFlags & TYPE_FLAG_LAZYARGS)
        printf(" lazyargs");

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






class TypeConstraintSubset : public TypeConstraint
{
public:
    TypeSet *target;

    TypeConstraintSubset(TypeSet *target)
        : TypeConstraint("subset"), target(target)
    {
        JS_ASSERT(target);
    }

    void newType(JSContext *cx, TypeSet *source, Type type)
    {
        
        target->addType(cx, type);
    }
};

void
TypeSet::addSubset(JSContext *cx, TypeSet *target)
{
    add(cx, ArenaNew<TypeConstraintSubset>(cx->compartment->pool, target));
}


class TypeConstraintBaseSubset : public TypeConstraint
{
public:
    TypeObject *object;
    TypeSet *target;

    TypeConstraintBaseSubset(TypeObject *object, TypeSet *target)
        : TypeConstraint("baseSubset"), object(object), target(target)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type)
    {
        target->addType(cx, type);
    }

    TypeObject * persistentObject() { return object; }

    size_t allocatedSize() { return sizeof(TypeConstraintBaseSubset); }
};

void
TypeSet::addBaseSubset(JSContext *cx, TypeObject *obj, TypeSet *target)
{
    add(cx, cx->new_<TypeConstraintBaseSubset>(obj, target));
}


class TypeConstraintProp : public TypeConstraint
{
public:
    JSScript *script;
    jsbytecode *pc;

    



    bool assign;
    TypeSet *target;

    
    jsid id;

    TypeConstraintProp(JSScript *script, jsbytecode *pc,
                       TypeSet *target, jsid id, bool assign)
        : TypeConstraint("prop"), script(script), pc(pc),
          assign(assign), target(target), id(id)
    {
        JS_ASSERT(script && pc && target);
    }

    void newType(JSContext *cx, TypeSet *source, Type type);
};

void
TypeSet::addGetProperty(JSContext *cx, JSScript *script, jsbytecode *pc,
                        TypeSet *target, jsid id)
{
    add(cx, ArenaNew<TypeConstraintProp>(cx->compartment->pool, script, pc, target, id, false));
}

void
TypeSet::addSetProperty(JSContext *cx, JSScript *script, jsbytecode *pc,
                        TypeSet *target, jsid id)
{
    add(cx, ArenaNew<TypeConstraintProp>(cx->compartment->pool, script, pc, target, id, true));
}








class TypeConstraintCallProp : public TypeConstraint
{
public:
    JSScript *script;
    jsbytecode *callpc;

    
    jsid id;

    TypeConstraintCallProp(JSScript *script, jsbytecode *callpc, jsid id)
        : TypeConstraint("callprop"), script(script), callpc(callpc), id(id)
    {
        JS_ASSERT(script && callpc);
    }

    void newType(JSContext *cx, TypeSet *source, Type type);
};

void
TypeSet::addCallProperty(JSContext *cx, JSScript *script, jsbytecode *pc, jsid id)
{
    




    jsbytecode *callpc = script->analysis(cx)->getCallPC(pc);
    UntrapOpcode untrap(cx, script, callpc);
    if (JSOp(*callpc) == JSOP_NEW)
        return;

    add(cx, ArenaNew<TypeConstraintCallProp>(cx->compartment->pool, script, callpc, id));
}


class TypeConstraintNewObject : public TypeConstraint
{
    TypeObject *fun;
    TypeSet *target;

  public:
    TypeConstraintNewObject(TypeObject *fun, TypeSet *target)
        : TypeConstraint("newObject"), fun(fun), target(target)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type);
};

void
TypeSet::addNewObject(JSContext *cx, TypeObject *fun, TypeSet *target)
{
    add(cx, ArenaNew<TypeConstraintNewObject>(cx->compartment->pool, fun, target));
}






class TypeConstraintCall : public TypeConstraint
{
public:
    
    TypeCallsite *callsite;

    TypeConstraintCall(TypeCallsite *callsite)
        : TypeConstraint("call"), callsite(callsite)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type);
};

void
TypeSet::addCall(JSContext *cx, TypeCallsite *site)
{
    add(cx, ArenaNew<TypeConstraintCall>(cx->compartment->pool, site));
}


class TypeConstraintArith : public TypeConstraint
{
public:
    
    TypeSet *target;

    
    TypeSet *other;

    TypeConstraintArith(TypeSet *target, TypeSet *other)
        : TypeConstraint("arith"), target(target), other(other)
    {
        JS_ASSERT(target);
    }

    void newType(JSContext *cx, TypeSet *source, Type type);
};

void
TypeSet::addArith(JSContext *cx, TypeSet *target, TypeSet *other)
{
    add(cx, ArenaNew<TypeConstraintArith>(cx->compartment->pool, target, other));
}


class TypeConstraintTransformThis : public TypeConstraint
{
public:
    JSScript *script;
    TypeSet *target;

    TypeConstraintTransformThis(JSScript *script, TypeSet *target)
        : TypeConstraint("transformthis"), script(script), target(target)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type);
};

void
TypeSet::addTransformThis(JSContext *cx, JSScript *script, TypeSet *target)
{
    add(cx, ArenaNew<TypeConstraintTransformThis>(cx->compartment->pool, script, target));
}





class TypeConstraintPropagateThis : public TypeConstraint
{
public:
    JSScript *script;
    jsbytecode *callpc;
    Type type;

    TypeConstraintPropagateThis(JSScript *script, jsbytecode *callpc, Type type)
        : TypeConstraint("propagatethis"), script(script), callpc(callpc), type(type)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type);
};

void
TypeSet::addPropagateThis(JSContext *cx, JSScript *script, jsbytecode *pc, Type type)
{
    
    jsbytecode *callpc = script->analysis(cx)->getCallPC(pc);
    UntrapOpcode untrap(cx, script, callpc);
    if (JSOp(*callpc) == JSOP_NEW)
        return;

    add(cx, ArenaNew<TypeConstraintPropagateThis>(cx->compartment->pool, script, callpc, type));
}


class TypeConstraintFilterPrimitive : public TypeConstraint
{
public:
    TypeSet *target;

    
    bool onlyNullVoid;

    TypeConstraintFilterPrimitive(TypeSet *target, bool onlyNullVoid)
        : TypeConstraint("filter"), target(target), onlyNullVoid(onlyNullVoid)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type)
    {
        if (onlyNullVoid) {
            if (type.isPrimitive(JSVAL_TYPE_NULL) || type.isPrimitive(JSVAL_TYPE_UNDEFINED))
                return;
        } else if (type.isPrimitive()) {
            return;
        }

        target->addType(cx, type);
    }
};

void
TypeSet::addFilterPrimitives(JSContext *cx, TypeSet *target, bool onlyNullVoid)
{
    add(cx, ArenaNew<TypeConstraintFilterPrimitive>(cx->compartment->pool,
                                                    target, onlyNullVoid));
}

void
ScriptAnalysis::pruneTypeBarriers(uint32 offset)
{
    TypeBarrier **pbarrier = &getCode(offset).typeBarriers;
    while (*pbarrier) {
        TypeBarrier *barrier = *pbarrier;
        if (barrier->target->hasType(barrier->type)) {
            
            *pbarrier = barrier->next;
        } else {
            pbarrier = &barrier->next;
        }
    }
}






static const uint32 BARRIER_OBJECT_LIMIT = 10;

void ScriptAnalysis::breakTypeBarriers(JSContext *cx, uint32 offset, bool all)
{
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
}

void ScriptAnalysis::breakTypeBarriersSSA(JSContext *cx, const SSAValue &v)
{
    if (v.kind() != SSAValue::PUSHED)
        return;

    uint32 offset = v.pushedOffset();
    if (JSOp(script->code[offset]) == JSOP_GETPROP)
        breakTypeBarriersSSA(cx, poppedValue(offset, 0));

    breakTypeBarriers(cx, offset, true);
}





class TypeConstraintSubsetBarrier : public TypeConstraint
{
public:
    JSScript *script;
    jsbytecode *pc;
    TypeSet *target;

    TypeConstraintSubsetBarrier(JSScript *script, jsbytecode *pc, TypeSet *target)
        : TypeConstraint("subsetBarrier"), script(script), pc(pc), target(target)
    {
        JS_ASSERT(!target->intermediate());
    }

    void newType(JSContext *cx, TypeSet *source, Type type)
    {
        if (!target->hasType(type)) {
            script->analysis(cx)->addTypeBarrier(cx, pc, target, type);
            return;
        }

        target->addType(cx, type);
    }
};

void
TypeSet::addSubsetBarrier(JSContext *cx, JSScript *script, jsbytecode *pc, TypeSet *target)
{
    add(cx, ArenaNew<TypeConstraintSubsetBarrier>(cx->compartment->pool, script, pc, target));
}





class TypeConstraintLazyArguments : public TypeConstraint
{
public:
    TypeSet *target;

    TypeConstraintLazyArguments(TypeSet *target)
        : TypeConstraint("lazyArgs"), target(target)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type) {}

    void newObjectState(JSContext *cx, TypeObject *object, bool force)
    {
        if (object->hasAnyFlags(OBJECT_FLAG_CREATED_ARGUMENTS))
            target->addType(cx, Type::UnknownType());
    }
};

void
TypeSet::addLazyArguments(JSContext *cx, TypeSet *target)
{
    add(cx, ArenaNew<TypeConstraintLazyArguments>(cx->compartment->pool, target));
}





class TypeConstraintGenerator : public TypeConstraint
{
public:
    TypeSet *target;

    TypeConstraintGenerator(TypeSet *target)
        : TypeConstraint("generator"), target(target)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type)
    {
        if (type.isUnknown() || type.isAnyObject()) {
            target->addType(cx, Type::UnknownType());
            return;
        }

        if (type.isPrimitive())
            return;

        



        JSObject *proto = type.isTypeObject()
            ? type.typeObject()->proto
            : type.singleObject()->getProto();

        if (proto) {
            Class *clasp = proto->getClass();
            if (clasp == &js_IteratorClass || clasp == &js_GeneratorClass)
                target->addType(cx, Type::UnknownType());
        }
    }
};






static inline TypeObject *
GetPropertyObject(JSContext *cx, JSScript *script, Type type)
{
    if (type.isTypeObject())
        return type.typeObject();

    
    if (type.isSingleObject())
        return type.singleObject()->getType(cx);

    



    TypeObject *object = NULL;
    switch (type.primitive()) {

      case JSVAL_TYPE_INT32:
      case JSVAL_TYPE_DOUBLE:
        object = script->types.standardType(cx, JSProto_Number);
        break;

      case JSVAL_TYPE_BOOLEAN:
        object = script->types.standardType(cx, JSProto_Boolean);
        break;

      case JSVAL_TYPE_STRING:
        object = script->types.standardType(cx, JSProto_String);
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
    



    uint32 format = js_CodeSpec[*pc].format;
    return (format & JOF_TYPESET) && !(format & JOF_INVOKE);
}

static inline void
MarkPropertyAccessUnknown(JSContext *cx, JSScript *script, jsbytecode *pc, TypeSet *target)
{
    if (UsePropertyTypeBarrier(pc))
        script->analysis(cx)->addTypeBarrier(cx, pc, target, Type::UnknownType());
    else
        target->addType(cx, Type::UnknownType());
}





static inline void
PropertyAccess(JSContext *cx, JSScript *script, jsbytecode *pc, TypeObject *object,
               bool assign, TypeSet *target, jsid id)
{
    
    if (assign && id == id_prototype(cx)) {
        cx->compartment->types.monitorBytecode(cx, script, pc - script->code);
        return;
    }

    
    if (id == id___proto__(cx) || id == id_constructor(cx) || id == id_caller(cx)) {
        if (assign)
            cx->compartment->types.monitorBytecode(cx, script, pc - script->code);
        else
            target->addType(cx, Type::UnknownType());
        return;
    }

    
    if (object->unknownProperties()) {
        if (!assign)
            MarkPropertyAccessUnknown(cx, script, pc, target);
        return;
    }

    
    TypeSet *types = object->getProperty(cx, id, assign);
    if (!types)
        return;
    if (assign)
        target->addSubset(cx, types);
    else if (UsePropertyTypeBarrier(pc))
        types->addSubsetBarrier(cx, script, pc, target);
    else
        types->addSubset(cx, target);
}


static inline bool
UnknownPropertyAccess(JSScript *script, Type type)
{
    return type.isUnknown()
        || type.isAnyObject()
        || (!type.isObject() && !script->hasGlobal());
}

void
TypeConstraintProp::newType(JSContext *cx, TypeSet *source, Type type)
{
    UntrapOpcode untrap(cx, script, pc);

    if (UnknownPropertyAccess(script, type)) {
        



        if (assign)
            cx->compartment->types.monitorBytecode(cx, script, pc - script->code);
        else
            MarkPropertyAccessUnknown(cx, script, pc, target);
        return;
    }

    if (type.isPrimitive(JSVAL_TYPE_MAGIC)) {
        
        if (assign || (id != JSID_VOID && id != id_length(cx)))
            return;

        if (id == JSID_VOID)
            MarkPropertyAccessUnknown(cx, script, pc, target);
        else
            target->addType(cx, Type::Int32Type());
        return;
    }

    TypeObject *object = GetPropertyObject(cx, script, type);
    if (object)
        PropertyAccess(cx, script, pc, object, assign, target, id);
}

void
TypeConstraintCallProp::newType(JSContext *cx, TypeSet *source, Type type)
{
    UntrapOpcode untrap(cx, script, callpc);

    






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
            
            types->add(cx, ArenaNew<TypeConstraintPropagateThis>(cx->compartment->pool,
                                                                 script, callpc, type));
        }
    }
}

void
TypeConstraintNewObject::newType(JSContext *cx, TypeSet *source, Type type)
{
    if (type.isUnknown() || type.isAnyObject()) {
        target->addType(cx, Type::UnknownType());
        return;
    }

    if (type.isObject()) {
        TypeObject *object = type.isTypeObject()
            ? type.typeObject()
            : type.singleObject()->getType(cx);
        if (object->unknownProperties()) {
            target->addType(cx, Type::UnknownType());
        } else {
            TypeSet *newTypes = object->getProperty(cx, JSID_EMPTY, false);
            if (!newTypes)
                return;
            newTypes->addSubset(cx, target);
        }
    } else if (!fun->functionScript) {
        




    } else if (!fun->functionScript->hasGlobal()) {
        target->addType(cx, Type::UnknownType());
    } else {
        TypeObject *object = fun->functionScript->types.standardType(cx, JSProto_Object);
        if (!object) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
        target->addType(cx, Type::ObjectType(object));
    }
}

void
TypeConstraintCall::newType(JSContext *cx, TypeSet *source, Type type)
{
    JSScript *script = callsite->script;
    jsbytecode *pc = callsite->pc;

    if (type.isUnknown() || type.isAnyObject()) {
        
        cx->compartment->types.monitorBytecode(cx, script, pc - script->code);
        return;
    }

    JSScript *callee = NULL;

    if (type.isSingleObject()) {
        JSObject *obj = type.singleObject();

        if (!obj->isFunction()) {
            
            return;
        }

        if (obj->getFunctionPrivate()->isNative()) {
            





            cx->compartment->types.monitorBytecode(cx, script, pc - script->code, true);

            






            Native native = obj->getFunctionPrivate()->native();

            if (native == js::array_push) {
                for (size_t ind = 0; ind < callsite->argumentCount; ind++) {
                    callsite->thisTypes->addSetProperty(cx, script, pc,
                                                        callsite->argumentTypes[ind], JSID_VOID);
                }
            }

            if (native == js::array_pop)
                callsite->thisTypes->addGetProperty(cx, script, pc, callsite->returnTypes, JSID_VOID);

            return;
        }

        callee = obj->getFunctionPrivate()->script();
    } else if (type.isTypeObject()) {
        callee = type.typeObject()->functionScript;
        if (!callee)
            return;
    } else {
        
        return;
    }

    unsigned nargs = callee->fun->nargs;

    if (!callee->types.ensureTypeArray(cx))
        return;

    
    if (!callee->ensureRanInference(cx)) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    
    for (unsigned i = 0; i < callsite->argumentCount && i < nargs; i++) {
        TypeSet *argTypes = callsite->argumentTypes[i];
        TypeSet *types = callee->types.argTypes(i);
        argTypes->addSubsetBarrier(cx, script, pc, types);
    }

    
    for (unsigned i = callsite->argumentCount; i < nargs; i++) {
        TypeSet *types = callee->types.argTypes(i);
        types->addType(cx, Type::UndefinedType());
    }

    if (callsite->isNew) {
        
        callee->types.setNewCalled(cx);

        



        callee->types.thisTypes()->addSubset(cx, callsite->returnTypes);
        callee->types.returnTypes()->addFilterPrimitives(cx, callsite->returnTypes, false);
    } else {
        







        callee->types.returnTypes()->addSubset(cx, callsite->returnTypes);
    }
}

void
TypeConstraintPropagateThis::newType(JSContext *cx, TypeSet *source, Type type)
{
    if (type.isUnknown() || type.isAnyObject()) {
        





        cx->compartment->types.monitorBytecode(cx, script, callpc - script->code);
        return;
    }

    
    JSScript *callee = NULL;

    if (type.isSingleObject()) {
        JSObject *object = type.singleObject();
        if (!object->isFunction() || !object->getFunctionPrivate()->isInterpreted())
            return;
        callee = object->getFunctionPrivate()->script();
    } else if (type.isTypeObject()) {
        TypeObject *object = type.typeObject();
        if (!object->isFunction || !object->functionScript)
            return;
        callee = object->functionScript;
    } else {
        
        return;
    }

    if (!callee->types.ensureTypeArray(cx))
        return;

    callee->types.thisTypes()->addType(cx, this->type);
}

void
TypeConstraintArith::newType(JSContext *cx, TypeSet *source, Type type)
{
    







    if (other) {
        





        if (type.isUnknown() || other->unknown()) {
            target->addType(cx, Type::UnknownType());
        } else if (type.isPrimitive(JSVAL_TYPE_DOUBLE)) {
            if (other->hasAnyFlag(TYPE_FLAG_UNDEFINED | TYPE_FLAG_NULL |
                                  TYPE_FLAG_INT32 | TYPE_FLAG_DOUBLE | TYPE_FLAG_BOOLEAN |
                                  TYPE_FLAG_ANYOBJECT) ||
                other->getObjectCount() != 0) {
                target->addType(cx, Type::DoubleType());
            }
        } else if (type.isPrimitive(JSVAL_TYPE_STRING)) {
            target->addType(cx, Type::StringType());
        } else {
            if (other->hasAnyFlag(TYPE_FLAG_UNDEFINED | TYPE_FLAG_NULL |
                                  TYPE_FLAG_INT32 | TYPE_FLAG_BOOLEAN |
                                  TYPE_FLAG_ANYOBJECT) ||
                other->getObjectCount() != 0) {
                target->addType(cx, Type::Int32Type());
            }
            if (other->hasAnyFlag(TYPE_FLAG_DOUBLE))
                target->addType(cx, Type::DoubleType());
        }
    } else {
        if (type.isUnknown())
            target->addType(cx, Type::UnknownType());
        else if (type.isPrimitive(JSVAL_TYPE_DOUBLE))
            target->addType(cx, Type::DoubleType());
        else
            target->addType(cx, Type::Int32Type());
    }
}

void
TypeConstraintTransformThis::newType(JSContext *cx, TypeSet *source, Type type)
{
    if (type.isUnknown() || type.isAnyObject() || type.isObject() || script->strictModeCode) {
        target->addType(cx, type);
        return;
    }

    



    if (!script->hasGlobal() ||
        type.isPrimitive(JSVAL_TYPE_NULL) ||
        type.isPrimitive(JSVAL_TYPE_UNDEFINED)) {
        target->addType(cx, Type::UnknownType());
        return;
    }

    TypeObject *object = NULL;
    switch (type.primitive()) {
      case JSVAL_TYPE_INT32:
      case JSVAL_TYPE_DOUBLE:
        object = script->types.standardType(cx, JSProto_Number);
        break;
      case JSVAL_TYPE_BOOLEAN:
        object = script->types.standardType(cx, JSProto_Boolean);
        break;
      case JSVAL_TYPE_STRING:
        object = script->types.standardType(cx, JSProto_String);
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
    JSScript *script;

    
    bool typeAdded;

    TypeConstraintFreeze(JSScript *script)
        : TypeConstraint("freeze"), script(script), typeAdded(false)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type)
    {
        if (typeAdded)
            return;

        typeAdded = true;
        cx->compartment->types.addPendingRecompile(cx, script);
    }
};

void
TypeSet::addFreeze(JSContext *cx)
{
    add(cx, ArenaNew<TypeConstraintFreeze>(cx->compartment->pool,
                                           cx->compartment->types.compiledScript), false);
}





class TypeConstraintFreezeTypeTag : public TypeConstraint
{
public:
    JSScript *script;

    



    bool typeUnknown;

    TypeConstraintFreezeTypeTag(JSScript *script)
        : TypeConstraint("freezeTypeTag"), script(script), typeUnknown(false)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type)
    {
        if (typeUnknown)
            return;

        if (!type.isUnknown() && !type.isAnyObject() && type.isObject()) {
            
            if (source->getObjectCount() >= 2)
                return;
        }

        typeUnknown = true;
        cx->compartment->types.addPendingRecompile(cx, script);
    }
};

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
      default:
        return JSVAL_TYPE_UNKNOWN;
    }
}

JSValueType
TypeSet::getKnownTypeTag(JSContext *cx)
{
    TypeFlags flags = baseFlags();
    JSValueType type;

    if (objectCount)
        type = flags ? JSVAL_TYPE_UNKNOWN : JSVAL_TYPE_OBJECT;
    else
        type = GetValueTypeFromTypeFlags(flags);

    






    bool empty = flags == 0 && objectCount == 0;
    JS_ASSERT_IF(empty, type == JSVAL_TYPE_UNKNOWN);

    if (cx->compartment->types.compiledScript && (empty || type != JSVAL_TYPE_UNKNOWN)) {
        add(cx, ArenaNew<TypeConstraintFreezeTypeTag>(cx->compartment->pool,
                                                      cx->compartment->types.compiledScript), false);
    }

    return type;
}


class TypeConstraintFreezeObjectFlags : public TypeConstraint
{
public:
    JSScript *script;

    
    TypeObjectFlags flags;

    
    bool *pmarked;
    bool localMarked;

    TypeConstraintFreezeObjectFlags(JSScript *script, TypeObjectFlags flags, bool *pmarked)
        : TypeConstraint("freezeObjectFlags"), script(script), flags(flags),
          pmarked(pmarked), localMarked(false)
    {}

    TypeConstraintFreezeObjectFlags(JSScript *script, TypeObjectFlags flags)
        : TypeConstraint("freezeObjectFlags"), script(script), flags(flags),
          pmarked(&localMarked), localMarked(false)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type) {}

    void newObjectState(JSContext *cx, TypeObject *object, bool force)
    {
        if (object->hasAnyFlags(flags) && !*pmarked) {
            *pmarked = true;
            cx->compartment->types.addPendingRecompile(cx, script);
        } else if (force) {
            cx->compartment->types.addPendingRecompile(cx, script);
        }
    }
};





class TypeConstraintFreezeObjectFlagsSet : public TypeConstraint
{
public:
    JSScript *script;

    TypeObjectFlags flags;
    bool marked;

    TypeConstraintFreezeObjectFlagsSet(JSScript *script, TypeObjectFlags flags)
        : TypeConstraint("freezeObjectKindSet"), script(script), flags(flags), marked(false)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type)
    {
        if (marked) {
            
            return;
        }

        if (type.isUnknown() || type.isAnyObject()) {
            
        } else if (type.isObject()) {
            TypeObject *object = type.isSingleObject()
                ? type.singleObject()->getType(cx)
                : type.typeObject();
            if (!object->hasAnyFlags(flags)) {
                



                TypeSet *elementTypes = object->getProperty(cx, JSID_VOID, false);
                if (!elementTypes)
                    return;
                elementTypes->add(cx,
                    ArenaNew<TypeConstraintFreezeObjectFlags>(cx->compartment->pool,
                                                              script, flags, &marked), false);
                return;
            }
        } else {
            return;
        }

        marked = true;
        cx->compartment->types.addPendingRecompile(cx, script);
    }
};

bool
TypeSet::hasObjectFlags(JSContext *cx, TypeObjectFlags flags)
{
    if (unknownObject())
        return true;

    



    if (objectCount == 0)
        return true;

    unsigned count = getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        TypeObject *object = getTypeObject(i);
        if (!object) {
            JSObject *obj = getSingleObject(i);
            if (obj)
                object = obj->getType(cx);
        }
        if (object && object->hasAnyFlags(flags))
            return true;
    }

    



    add(cx, ArenaNew<TypeConstraintFreezeObjectFlagsSet>(cx->compartment->pool,
                                                         cx->compartment->types.compiledScript, flags));

    return false;
}

bool
TypeSet::HasObjectFlags(JSContext *cx, TypeObject *object, TypeObjectFlags flags)
{
    if (object->hasAnyFlags(flags))
        return true;

    TypeSet *elementTypes = object->getProperty(cx, JSID_VOID, false);
    if (!elementTypes)
        return true;
    elementTypes->add(cx,
        ArenaNew<TypeConstraintFreezeObjectFlags>(cx->compartment->pool,
                                                  cx->compartment->types.compiledScript, flags), false);
    return false;
}

void
FixLazyArguments(JSContext *cx, JSScript *script)
{
#ifdef JS_METHODJIT
    mjit::ExpandInlineFrames(cx->compartment, true);
#endif

    ScriptAnalysis *analysis = script->analysis(cx);
    if (analysis && !analysis->ranBytecode())
        analysis->analyzeBytecode(cx);
    if (!analysis || analysis->OOM())
        return;

    for (FrameRegsIter iter(cx); !iter.done(); ++iter) {
        StackFrame *fp = iter.fp();
        if (fp->isScriptFrame() && fp->script() == script) {
            



            Value *sp = fp->base() + analysis->getCode(iter.pc()).stackDepth;
            for (Value *vp = fp->slots(); vp < sp; vp++) {
                if (vp->isMagicCheck(JS_LAZY_ARGUMENTS)) {
                    if (!js_GetArgsValue(cx, fp, vp))
                        vp->setNull();
                }
            }
        }
    }
}

static inline void
ObjectStateChange(JSContext *cx, TypeObject *object, bool markingUnknown, bool force)
{
    if (object->unknownProperties())
        return;

    
    TypeSet *elementTypes = object->getProperty(cx, JSID_VOID, false);
    if (!elementTypes)
        return;
    if (markingUnknown) {
        bool fixArgs = false;
        if (!(object->flags & OBJECT_FLAG_CREATED_ARGUMENTS) && object->isFunction) {
            if (object->functionScript && object->functionScript->usedLazyArgs)
                fixArgs = true;
        }

        
        object->flags = OBJECT_FLAG_UNKNOWN_MASK;

        if (fixArgs)
            FixLazyArguments(cx, object->functionScript);
    }

    TypeConstraint *constraint = elementTypes->constraintList;
    while (constraint) {
        constraint->newObjectState(cx, object, force);
        constraint = constraint->next;
    }
}

void
TypeSet::WatchObjectReallocation(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isGlobal() && !obj->type()->unknownProperties());
    TypeSet *types = obj->type()->getProperty(cx, JSID_VOID, false);
    if (!types)
        return;

    




    types->add(cx, ArenaNew<TypeConstraintFreezeObjectFlags>(cx->compartment->pool,
                                                             cx->compartment->types.compiledScript,
                                                             0));
}

class TypeConstraintFreezeOwnProperty : public TypeConstraint
{
public:
    JSScript *script;

    bool updated;
    bool configurable;

    TypeConstraintFreezeOwnProperty(JSScript *script, bool configurable)
        : TypeConstraint("freezeOwnProperty"),
          script(script), updated(false), configurable(configurable)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type) {}

    void newPropertyState(JSContext *cx, TypeSet *source)
    {
        if (updated)
            return;
        if (source->hasAnyFlag(configurable
                               ? TYPE_FLAG_CONFIGURED_PROPERTY
                               : TYPE_FLAG_OWN_PROPERTY)) {
            updated = true;
            cx->compartment->types.addPendingRecompile(cx, script);
        }
    }
};

bool
TypeSet::isOwnProperty(JSContext *cx, bool configurable)
{
    if (hasAnyFlag(configurable
                   ? TYPE_FLAG_CONFIGURED_PROPERTY
                   : TYPE_FLAG_OWN_PROPERTY)) {
        return true;
    }

    add(cx, ArenaNew<TypeConstraintFreezeOwnProperty>(cx->compartment->pool,
                                                      cx->compartment->types.compiledScript,
                                                      configurable), false);
    return false;
}

bool
TypeSet::knownNonEmpty(JSContext *cx)
{
    if (baseFlags() != 0 || objectCount != 0)
        return true;

    add(cx, ArenaNew<TypeConstraintFreeze>(cx->compartment->pool,
                                           cx->compartment->types.compiledScript), false);

    return false;
}

int
TypeSet::getTypedArrayType(JSContext *cx)
{
    int arrayType = TypedArray::TYPE_MAX;
    unsigned count = getObjectCount();

    for (unsigned i = 0; i < count; i++) {
        JSObject *proto = NULL;
        if (JSObject *object = getSingleObject(i)) {
            proto = object->getProto();
        } else if (TypeObject *object = getTypeObject(i)) {
            JS_ASSERT(!object->hasAnyFlags(OBJECT_FLAG_NON_TYPED_ARRAY));
            proto = object->proto;
        }
        if (!proto)
            continue;

        int objArrayType = proto->getClass() - TypedArray::slowClasses;
        JS_ASSERT(objArrayType >= 0 && objArrayType < TypedArray::TYPE_MAX);

        



        if (arrayType == TypedArray::TYPE_MAX)
            arrayType = objArrayType;
        else if (arrayType != objArrayType)
            return TypedArray::TYPE_MAX;
    }

    




    JS_ASSERT(arrayType != TypedArray::TYPE_MAX);

    
    addFreeze(cx);

    return arrayType;
}

JSObject *
TypeSet::getSingleton(JSContext *cx, bool freeze)
{
    if (baseFlags() != 0 || objectCount != 1)
        return NULL;

    JSObject *obj = getSingleObject(0);
    if (!obj)
        return NULL;

    if (freeze) {
        add(cx, ArenaNew<TypeConstraintFreeze>(cx->compartment->pool,
                                               cx->compartment->types.compiledScript), false);
    }

    return obj;
}





void
TypeCompartment::init(JSContext *cx)
{
    PodZero(this);

    




#ifdef DEBUG
    typeEmpty.name_ = JSID_VOID;
#endif
    typeEmpty.flags = OBJECT_FLAG_UNKNOWN_MASK;
    typeEmpty.setsMarkedUnknown = true;

#ifndef JS_CPU_ARM
    if (cx && cx->getRunOptions() & JSOPTION_TYPE_INFERENCE)
        inferenceEnabled = true;
#endif
}

TypeObject *
TypeCompartment::newTypeObject(JSContext *cx, JSScript *script,
                               const char *name, const char *postfix,
                               JSProtoKey key, JSObject *proto)
{
#ifdef DEBUG
    if (*postfix) {
        unsigned len = strlen(name) + strlen(postfix) + 2;
        char *newName = (char *) alloca(len);
        JS_snprintf(newName, len, "%s:%s", name, postfix);
        name = newName;
    }
#if 0
    
    static unsigned nameCount = 0;
    unsigned len = strlen(name) + 15;
    char *newName = (char *) alloca(len);
    JS_snprintf(newName, len, "%u:%s", ++nameCount, name);
    name = newName;
#endif
    JSAtom *atom = js_Atomize(cx, name, strlen(name));
    if (!atom)
        return NULL;
    jsid id = ATOM_TO_JSID(atom);
#else
    jsid id = JSID_VOID;
#endif

    TypeObject *object = cx->new_<TypeObject>(id, proto, key == JSProto_Function);
    if (!object)
        return NULL;

    TypeObject *&objects = script ? script->types.typeObjects : this->objects;
    object->next = objects;
    objects = object;

    if (!cx->typeInferenceEnabled())
        object->flags = OBJECT_FLAG_UNKNOWN_MASK;
    else
        object->setFlagsFromKey(cx, key);

    if (proto) {
        
        TypeObject *prototype = proto->getType(cx);
        if (prototype->unknownProperties())
            object->flags = OBJECT_FLAG_UNKNOWN_MASK;
        object->instanceNext = prototype->instanceList;
        prototype->instanceList = object;
    }

    return object;
}

TypeObject *
TypeCompartment::newInitializerTypeObject(JSContext *cx, JSScript *script,
                                          uint32 offset, JSProtoKey key)
{
    char *name = NULL;
#ifdef DEBUG
    name = (char *) alloca(40);
    JS_snprintf(name, 40, "#%lu:%lu", script->id(), offset);
#endif

    JSObject *proto;
    if (!js_GetClassPrototype(cx, script->global(), key, &proto, NULL))
        return NULL;

    TypeObject *res = newTypeObject(cx, script, name, "", key, proto);
    if (!res)
        return NULL;

    res->initializerKey = key;
    res->initializerOffset = offset;

    jsbytecode *pc = script->code + offset;
    UntrapOpcode untrap(cx, script, pc);

    if (JSOp(*pc) == JSOP_NEWOBJECT) {
        




        JSObject *baseobj = script->getObject(GET_SLOTNO(pc));

        if (!res->addDefiniteProperties(cx, baseobj))
            return NULL;
    }

    return res;
}

static inline jsid
GetAtomId(JSContext *cx, JSScript *script, const jsbytecode *pc, unsigned offset)
{
    unsigned index = js_GetIndexFromBytecode(cx, script, (jsbytecode*) pc, offset);
    return MakeTypeId(cx, ATOM_TO_JSID(script->getAtom(index)));
}

static inline jsid
GetGlobalId(JSContext *cx, JSScript *script, const jsbytecode *pc)
{
    unsigned index = GET_SLOTNO(pc);
    return MakeTypeId(cx, ATOM_TO_JSID(script->getGlobalAtom(index)));
}

static inline JSObject *
GetScriptObject(JSContext *cx, JSScript *script, const jsbytecode *pc, unsigned offset)
{
    unsigned index = js_GetIndexFromBytecode(cx, script, (jsbytecode*) pc, offset);
    return script->getObject(index);
}

static inline const Value &
GetScriptConst(JSContext *cx, JSScript *script, const jsbytecode *pc)
{
    unsigned index = js_GetIndexFromBytecode(cx, script, (jsbytecode*) pc, 0);
    return script->getConst(index);
}

bool
types::UseNewType(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    JS_ASSERT(cx->typeInferenceEnabled());

    UntrapOpcode untrap(cx, script, pc);

    
















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

void
TypeCompartment::growPendingArray(JSContext *cx)
{
    unsigned newCapacity = js::Max(unsigned(100), pendingCapacity * 2);
    PendingWork *newArray = (PendingWork *) js::OffTheBooks::calloc_(newCapacity * sizeof(PendingWork));
    if (!newArray) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    memcpy(newArray, pendingArray, pendingCount * sizeof(PendingWork));
    cx->free_(pendingArray);

    pendingArray = newArray;
    pendingCapacity = newCapacity;
}

void
TypeCompartment::processPendingRecompiles(JSContext *cx)
{
    
    Vector<JSScript*> *pending = pendingRecompiles;
    pendingRecompiles = NULL;

    JS_ASSERT(!pending->empty());

#ifdef JS_METHODJIT

    mjit::ExpandInlineFrames(cx->compartment, true);

    for (unsigned i = 0; i < pending->length(); i++) {
        JSScript *script = (*pending)[i];
        mjit::Recompiler recompiler(cx, script);
        if (script->hasJITCode())
            recompiler.recompile();
    }

#endif 

    cx->delete_(pending);
}

void
TypeCompartment::setPendingNukeTypes(JSContext *cx)
{
    JS_ASSERT(cx->compartment->activeInference);
    if (!pendingNukeTypes) {
        js_ReportOutOfMemory(cx);
        pendingNukeTypes = true;
    }
}

void
TypeCompartment::nukeTypes(JSContext *cx)
{
    JSCompartment *compartment = cx->compartment;
    JS_ASSERT(this == &compartment->types);

    










    JS_ASSERT(pendingNukeTypes);
    if (pendingRecompiles) {
        cx->free_(pendingRecompiles);
        pendingRecompiles = NULL;
    }

    




#ifdef JS_THREADSAFE
    Maybe<AutoLockGC> maybeLock;
    if (!cx->runtime->gcMarkAndSweep)
        maybeLock.construct(cx->runtime);
#endif

    inferenceEnabled = false;

    
    for (JSCList *cl = cx->runtime->contextList.next;
         cl != &cx->runtime->contextList;
         cl = cl->next) {
        JSContext *cx = js_ContextFromLinkField(cl);
        cx->setCompartment(cx->compartment);
    }

#ifdef JS_METHODJIT

    mjit::ExpandInlineFrames(cx->compartment, true);

    
    for (JSCList *cursor = compartment->scripts.next;
         cursor != &compartment->scripts;
         cursor = cursor->next) {
        JSScript *script = reinterpret_cast<JSScript *>(cursor);
        if (script->hasJITCode()) {
            mjit::Recompiler recompiler(cx, script);
            recompiler.recompile();
        }
    }

#endif 

}

void
TypeCompartment::addPendingRecompile(JSContext *cx, JSScript *script)
{
#ifdef JS_METHODJIT
    if (!script->jitNormal && !script->jitCtor) {
        
        return;
    }

    if (!pendingRecompiles) {
        pendingRecompiles = cx->new_< Vector<JSScript*> >(cx);
        if (!pendingRecompiles) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
    }

    for (unsigned i = 0; i < pendingRecompiles->length(); i++) {
        if (script == (*pendingRecompiles)[i])
            return;
    }

    if (!pendingRecompiles->append(script)) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }
#endif
}

void
TypeCompartment::monitorBytecode(JSContext *cx, JSScript *script, uint32 offset,
                                 bool returnOnly)
{
    ScriptAnalysis *analysis = script->analysis(cx);
    JS_ASSERT(analysis->ranInference());

    jsbytecode *pc = script->code + offset;
    UntrapOpcode untrap(cx, script, pc);

    JS_ASSERT_IF(returnOnly, js_CodeSpec[*pc].format & JOF_INVOKE);

    Bytecode &code = analysis->getCode(pc);

    if (returnOnly ? code.monitoredTypesReturn : code.monitoredTypes)
        return;

    InferSpew(ISpewOps, "addMonitorNeeded:%s #%u:%05u",
              returnOnly ? " returnOnly" : "", script->id(), offset);

    
    if (js_CodeSpec[*pc].format & JOF_INVOKE)
        code.monitoredTypesReturn = true;

    if (!returnOnly)
        code.monitoredTypes = true;

    cx->compartment->types.addPendingRecompile(cx, script);

    
    if (script->fun && !script->fun->hasLazyType())
        ObjectStateChange(cx, script->fun->type(), false, true);
}

static void
MarkTypeObjectListSetsUnknown(JSContext *cx, TypeObject *objects, TypeObject *target)
{
    while (objects) {
        unsigned count = objects->getPropertyCount();
        for (unsigned i = 0; i < count; i++) {
            Property *prop = objects->getProperty(i);
            if (prop && prop->types.hasType(Type::ObjectType(target)))
                prop->types.addType(cx, Type::AnyObjectType());
        }
        objects = objects->next;
    }
}

void
TypeCompartment::markSetsUnknown(JSContext *cx, TypeObject *target)
{
    JS_ASSERT(this == &cx->compartment->types);
    JS_ASSERT(!target->setsMarkedUnknown);
    JS_ASSERT(!target->singleton);
    JS_ASSERT(target->unknownProperties());
    target->setsMarkedUnknown = true;

    AutoEnterTypeInference enter(cx);

    






    MarkTypeObjectListSetsUnknown(cx, objects, target);

    for (JSCList *cursor = cx->compartment->scripts.next;
         cursor != &cx->compartment->scripts;
         cursor = cursor->next) {
        JSScript *script = reinterpret_cast<JSScript *>(cursor);
        if (script->types.typeArray) {
            unsigned count = script->types.numTypeSets();
            for (unsigned i = 0; i < count; i++) {
                if (script->types.typeArray[i].hasType(Type::ObjectType(target)))
                    script->types.typeArray[i].addType(cx, Type::AnyObjectType());
            }
        }
        MarkTypeObjectListSetsUnknown(cx, script->types.typeObjects, target);
        if (script->hasAnalysis() && script->analysis(cx)->ranInference()) {
            for (unsigned i = 0; i < script->length; i++) {
                if (!script->analysis(cx)->maybeCode(i))
                    continue;
                jsbytecode *pc = script->code + i;
                UntrapOpcode untrap(cx, script, pc);
                if (js_CodeSpec[*pc].format & JOF_DECOMPOSE)
                    continue;
                unsigned defCount = GetDefCount(script, i);
                if (ExtendedDef(pc))
                    defCount++;
                for (unsigned j = 0; j < defCount; j++) {
                    TypeSet *types = script->analysis(cx)->pushedTypes(pc, j);
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
        





        cx->compartment->types.addPendingRecompile(cx, script);

        
        if (script->fun && !script->fun->hasLazyType())
            ObjectStateChange(cx, script->fun->type(), false, true);
    }

    
    TypeBarrier *barrier = code.typeBarriers;
    while (barrier) {
        if (barrier->target == target && barrier->type == type)
            return;
        barrier = barrier->next;
    }

    InferSpew(ISpewOps, "typeBarrier: #%u:%05u: %sT%p%s %s",
              script->id(), pc - script->code,
              InferSpewColor(target), target, InferSpewColorReset(),
              TypeString(type));

    barrier = ArenaNew<TypeBarrier>(cx->compartment->pool, target, type);

    barrier->next = code.typeBarriers;
    code.typeBarriers = barrier;
}

void
TypeCompartment::print(JSContext *cx, JSCompartment *compartment)
{
    JS_ASSERT(this == &compartment->types);

    if (!InferSpewActive(ISpewResult) || JS_CLIST_IS_EMPTY(&compartment->scripts))
        return;

    for (JSScript *script = (JSScript *)compartment->scripts.next;
         &script->links != &compartment->scripts;
         script = (JSScript *)script->links.next) {
        if (script->hasAnalysis() && script->analysis(cx)->ranInference())
            script->analysis(cx)->printTypes(cx);
        TypeObject *object = script->types.typeObjects;
        while (object) {
            object->print(cx);
            object = object->next;
        }
    }

#ifdef DEBUG
    TypeObject *object = objects;
    while (object) {
        object->print(cx);
        object = object->next;
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

struct types::ArrayTableKey
{
    Type type;
    JSObject *proto;

    ArrayTableKey()
        : type(Type::UndefinedType()), proto(NULL)
    {}

    typedef ArrayTableKey Lookup;

    static inline uint32 hash(const ArrayTableKey &v) {
        return (uint32) (v.type.raw() ^ ((uint32)(size_t)v.proto >> 2));
    }

    static inline bool match(const ArrayTableKey &v1, const ArrayTableKey &v2) {
        return v1.type == v2.type && v1.proto == v2.proto;
    }
};

void
TypeCompartment::fixArrayType(JSContext *cx, JSObject *obj)
{
    AutoEnterTypeInference enter(cx);

    if (!arrayTypeTable) {
        arrayTypeTable = cx->new_<ArrayTypeTable>();
        if (!arrayTypeTable || !arrayTypeTable->init()) {
            arrayTypeTable = NULL;
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
    }

    





    JS_ASSERT(obj->isPackedDenseArray());

    unsigned len = obj->getDenseArrayInitializedLength();
    if (len == 0)
        return;

    Type type = GetValueType(cx, obj->getDenseArrayElement(0));

    for (unsigned i = 1; i < len; i++) {
        Type ntype = GetValueType(cx, obj->getDenseArrayElement(i));
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
        char *name = NULL;
#ifdef DEBUG
        static unsigned count = 0;
        name = (char *) alloca(20);
        JS_snprintf(name, 20, "TableArray:%u", ++count);
#endif

        TypeObject *objType = newTypeObject(cx, NULL, name, "", JSProto_Array, obj->getProto());
        if (!objType) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
        obj->setType(objType);

        if (!objType->unknownProperties())
            objType->addPropertyType(cx, JSID_VOID, type);

        if (!arrayTypeTable->relookupOrAdd(p, key, objType)) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
    }
}







struct types::ObjectTableKey
{
    jsid *ids;
    uint32 nslots;
    uint32 nfixed;
    JSObject *proto;

    typedef JSObject * Lookup;

    static inline uint32 hash(JSObject *obj) {
        return (uint32) (JSID_BITS(obj->lastProperty()->propid) ^
                         obj->slotSpan() ^ obj->numFixedSlots() ^
                         ((uint32)(size_t)obj->getProto() >> 2));
    }

    static inline bool match(const ObjectTableKey &v, JSObject *obj) {
        if (obj->slotSpan() != v.nslots ||
            obj->numFixedSlots() != v.nfixed ||
            obj->getProto() != v.proto) {
            return false;
        }
        const Shape *shape = obj->lastProperty();
        while (!JSID_IS_EMPTY(shape->propid)) {
            if (shape->propid != v.ids[shape->slot])
                return false;
            shape = shape->previous();
        }
        return true;
    }
};

struct types::ObjectTableEntry
{
    TypeObject *object;
    Shape *newShape;
    Type *types;
};

void
TypeCompartment::fixObjectType(JSContext *cx, JSObject *obj)
{
    AutoEnterTypeInference enter(cx);

    if (!objectTypeTable) {
        objectTypeTable = cx->new_<ObjectTypeTable>();
        if (!objectTypeTable || !objectTypeTable->init()) {
            objectTypeTable = NULL;
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
    }

    





    JS_ASSERT(obj->isObject());

    if (obj->slotSpan() == 0 || obj->inDictionaryMode())
        return;

    ObjectTypeTable::AddPtr p = objectTypeTable->lookupForAdd(obj);
    const Shape *baseShape = obj->lastProperty();

    if (p) {
        
        Type *types = p->value.types;
        for (unsigned i = 0; i < obj->slotSpan(); i++) {
            Type ntype = GetValueType(cx, obj->getSlot(i));
            if (ntype != types[i]) {
                if (NumberTypes(ntype, types[i])) {
                    if (types[i].isPrimitive(JSVAL_TYPE_INT32)) {
                        types[i] = Type::DoubleType();
                        const Shape *shape = baseShape;
                        while (!JSID_IS_EMPTY(shape->propid)) {
                            if (shape->slot == i) {
                                Type type = Type::DoubleType();
                                if (!p->value.object->unknownProperties())
                                    p->value.object->addPropertyType(cx, shape->propid, type);
                                break;
                            }
                            shape = shape->previous();
                        }
                    }
                } else {
                    return;
                }
            }
        }

        obj->setTypeAndShape(p->value.object, p->value.newShape);
    } else {
        




        JSObject *xobj = NewBuiltinClassInstance(cx, &js_ObjectClass,
                                                 (gc::FinalizeKind) obj->finalizeKind());
        if (!xobj) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
        AutoObjectRooter xvr(cx, xobj);

        char *name = NULL;
#ifdef DEBUG
        static unsigned count = 0;
        name = (char *) alloca(20);
        JS_snprintf(name, 20, "TableObject:%u", ++count);
#endif

        TypeObject *objType = newTypeObject(cx, NULL, name, "", JSProto_Object, obj->getProto());
        if (!objType) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
        xobj->setType(objType);

        jsid *ids = (jsid *) cx->calloc_(obj->slotSpan() * sizeof(jsid));
        if (!ids) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }

        Type *types = (Type *) cx->calloc_(obj->slotSpan() * sizeof(Type));
        if (!types) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }

        const Shape *shape = baseShape;
        while (!JSID_IS_EMPTY(shape->propid)) {
            ids[shape->slot] = shape->propid;
            types[shape->slot] = GetValueType(cx, obj->getSlot(shape->slot));
            if (!objType->unknownProperties())
                objType->addPropertyType(cx, shape->propid, types[shape->slot]);
            shape = shape->previous();
        }

        
        for (unsigned i = 0; i < obj->slotSpan(); i++) {
            if (!DefineNativeProperty(cx, xobj, ids[i], UndefinedValue(), NULL, NULL,
                                      JSPROP_ENUMERATE, 0, 0, DNP_SKIP_TYPE)) {
                cx->compartment->types.setPendingNukeTypes(cx);
                return;
            }
        }
        JS_ASSERT(!xobj->inDictionaryMode());
        const Shape *newShape = xobj->lastProperty();

        if (!objType->addDefiniteProperties(cx, xobj))
            return;

        ObjectTableKey key;
        key.ids = ids;
        key.nslots = obj->slotSpan();
        key.nfixed = obj->numFixedSlots();
        key.proto = obj->getProto();
        JS_ASSERT(ObjectTableKey::match(key, obj));

        ObjectTableEntry entry;
        entry.object = objType;
        entry.newShape = (Shape *) newShape;
        entry.types = types;

        p = objectTypeTable->lookupForAdd(obj);
        if (!objectTypeTable->add(p, key, entry)) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }

        obj->setTypeAndShape(objType, newShape);
    }
}





void
TypeObject::storeToInstances(JSContext *cx, Property *base)
{
    TypeObject *object = instanceList;
    while (object) {
        Property *p =
            HashSetLookup<jsid,Property,Property>(object->propertySet, object->propertyCount, base->id);
        if (p)
            base->types.addBaseSubset(cx, object, &p->types);
        if (object->instanceList)
            object->storeToInstances(cx, base);
        object = object->instanceNext;
    }
}

void
TypeObject::getFromPrototypes(JSContext *cx, Property *base)
{
     JSObject *obj = proto;
     while (obj) {
         TypeObject *object = obj->type();
         Property *p =
             HashSetLookup<jsid,Property,Property>(object->propertySet, object->propertyCount, base->id);
         if (p)
             p->types.addBaseSubset(cx, this, &base->types);
         obj = obj->getProto();
     }
}

bool
TypeObject::addProperty(JSContext *cx, jsid id, Property **pprop)
{
    JS_ASSERT(!*pprop);
    Property *base = cx->new_<Property>(id);
    if (!base) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return false;
    }

    *pprop = base;

    InferSpew(ISpewOps, "typeSet: %sT%p%s property %s %s",
              InferSpewColor(&base->types), &base->types, InferSpewColorReset(),
              name(), TypeIdString(id));

    if (!JSID_IS_EMPTY(id)) {
        
        if (instanceList)
            storeToInstances(cx, base);

        
        getFromPrototypes(cx, base);
    }

    return true;
}

bool
TypeObject::addDefiniteProperties(JSContext *cx, JSObject *obj)
{
    if (unknownProperties())
        return true;

    




    AutoEnterTypeInference enter(cx);

    const Shape *shape = obj->lastProperty();
    while (!JSID_IS_EMPTY(shape->propid)) {
        jsid id = MakeTypeId(cx, shape->propid);
        if (!JSID_IS_VOID(id) && obj->isFixedSlot(shape->slot) &&
            shape->slot <= (TYPE_FLAG_DEFINITE_MASK >> TYPE_FLAG_DEFINITE_SHIFT)) {
            TypeSet *types = getProperty(cx, id, true);
            if (!types)
                return false;
            types->setDefinite(shape->slot);
        }
        shape = shape->previous();
    }

    return true;
}

inline void
InlineAddTypeProperty(JSContext *cx, TypeObject *obj, jsid id, Type type)
{
    
    id = MakeTypeId(cx, id);

    AutoEnterTypeInference enter(cx);

    TypeSet *types = obj->getProperty(cx, id, true);
    if (!types || types->hasType(type))
        return;

    InferSpew(ISpewOps, "externalType: property %s %s: %s",
              obj->name(), TypeIdString(id), TypeString(type));
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
    jsid id = JSID_VOID;
    if (name) {
        JSAtom *atom = js_Atomize(cx, name, strlen(name));
        if (!atom) {
            AutoEnterTypeInference enter(cx);
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
        id = ATOM_TO_JSID(atom);
    }
    InlineAddTypeProperty(cx, this, id, type);
}

void
TypeObject::addPropertyType(JSContext *cx, const char *name, const Value &value)
{
    addPropertyType(cx, name, GetValueType(cx, value));
}

void
TypeObject::aliasProperties(JSContext *cx, jsid first, jsid second)
{
    AutoEnterTypeInference enter(cx);

    first = MakeTypeId(cx, first);
    second = MakeTypeId(cx, second);

    TypeSet *firstTypes = getProperty(cx, first, true);
    TypeSet *secondTypes = getProperty(cx, second, true);
    if (!firstTypes || !secondTypes)
        return;

    firstTypes->addBaseSubset(cx, this, secondTypes);
    secondTypes->addBaseSubset(cx, this, firstTypes);
}

void
TypeObject::markPropertyConfigured(JSContext *cx, jsid id)
{
    AutoEnterTypeInference enter(cx);

    id = MakeTypeId(cx, id);

    TypeSet *types = getProperty(cx, id, true);
    if (types)
        types->setOwnProperty(cx, true);
}

void
TypeObject::markSlotReallocation(JSContext *cx)
{
    



    AutoEnterTypeInference enter(cx);
    TypeSet *types = getProperty(cx, JSID_VOID, false);
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

    AutoEnterTypeInference enter(cx);

    bool fixArgs = false;
    if ((flags & ~this->flags & OBJECT_FLAG_CREATED_ARGUMENTS) &&
        functionScript && functionScript->usedLazyArgs) {
        fixArgs = true;
    }

    this->flags |= flags;

    if (fixArgs)
        FixLazyArguments(cx, functionScript);

    InferSpew(ISpewOps, "%s: setFlags %u", name(), flags);

    ObjectStateChange(cx, this, false, false);
}

void
TypeObject::markUnknown(JSContext *cx)
{
    AutoEnterTypeInference enter(cx);

    JS_ASSERT(cx->compartment->activeInference);
    JS_ASSERT(!unknownProperties());

    InferSpew(ISpewOps, "UnknownProperties: %s", name());

    ObjectStateChange(cx, this, true, true);

    

    TypeObject *instance = instanceList;
    while (instance) {
        if (!instance->unknownProperties())
            instance->markUnknown(cx);
        instance = instance->instanceNext;
    }

    








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
    JS_ASSERT(!newScriptCleared);
    newScriptCleared = true;

    







    if (!newScript)
        return;

    AutoEnterTypeInference enter(cx);

    








    for (unsigned i = 0; i < getPropertyCount(); i++) {
        Property *prop = getProperty(i);
        if (!prop)
            continue;
        if (prop->types.isDefiniteProperty())
            prop->types.setOwnProperty(cx, true);
    }

    







    for (FrameRegsIter iter(cx); !iter.done(); ++iter) {
        StackFrame *fp = iter.fp();
        if (fp->isScriptFrame() && fp->isConstructing() &&
            fp->script() == newScript->script && fp->thisValue().isObject() &&
            !fp->thisValue().toObject().hasLazyType() &&
            fp->thisValue().toObject().type() == this) {
            JSObject *obj = &fp->thisValue().toObject();
            jsbytecode *pc = iter.pc();

            
            bool finished = false;

            
            uint32 numProperties = 0;

            



            size_t depth = 0;

            for (TypeNewScript::Initializer *init = newScript->initializerList;; init++) {
                uint32 offset = uint32(pc - fp->script()->code);
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
                        StackSegment &seg = cx->stack.space().containingSegment(fp);
                        if (seg.maybefp() == fp)
                            break;
                        fp = seg.computeNextFrame(fp);
                        pc = fp->pcQuadratic(cx->stack);
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

    cx->free_(newScript);
    newScript = NULL;
}

void
TypeObject::print(JSContext *cx)
{
    printf("%s : %s", name(), proto ? TypeString(Type::ObjectType(proto)) : "(null)");

    if (unknownProperties()) {
        printf(" unknown");
    } else {
        if (!hasAnyFlags(OBJECT_FLAG_NON_PACKED_ARRAY))
            printf(" packed");
        if (!hasAnyFlags(OBJECT_FLAG_NON_DENSE_ARRAY))
            printf(" dense");
        if (!hasAnyFlags(OBJECT_FLAG_NON_TYPED_ARRAY))
            printf(" typed");
        if (hasAnyFlags(OBJECT_FLAG_UNINLINEABLE))
            printf(" uninlineable");
        if (hasAnyFlags(OBJECT_FLAG_SPECIAL_EQUALITY))
            printf(" specialEquality");
        if (hasAnyFlags(OBJECT_FLAG_ITERATED))
            printf(" iterated");
    }

    if (propertyCount == 0) {
        printf(" {}\n");
        return;
    }

    printf(" {");

    unsigned count = getPropertyCount();
    for (unsigned i = 0; i < count; i++) {
        Property *prop = getProperty(i);
        if (prop) {
            printf("\n    %s:", TypeIdString(prop->id));
            prop->types.print(cx);
        }
    }

    printf("\n}\n");
}









static inline bool
CheckNextTest(jsbytecode *pc)
{
    jsbytecode *next = pc + GetBytecodeLength(pc);
    switch ((JSOp)*next) {
      case JSOP_IFEQ:
      case JSOP_IFNE:
      case JSOP_NOT:
      case JSOP_OR:
      case JSOP_ORX:
      case JSOP_AND:
      case JSOP_ANDX:
      case JSOP_TYPEOF:
      case JSOP_TYPEOFEXPR:
        return true;
      default:
        
        return false;
    }
}

static inline TypeObject *
GetInitializerType(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    if (!script->hasGlobal())
        return NULL;

    UntrapOpcode untrap(cx, script, pc);

    JSOp op = JSOp(*pc);
    JS_ASSERT(op == JSOP_NEWARRAY || op == JSOP_NEWOBJECT || op == JSOP_NEWINIT);

    bool isArray = (op == JSOP_NEWARRAY || (op == JSOP_NEWINIT && pc[1] == JSProto_Array));
    return script->types.initObject(cx, pc, isArray ? JSProto_Array : JSProto_Object);
}

inline void
ScriptAnalysis::setForTypes(JSContext *cx, jsbytecode *pc, TypeSet *types)
{
    
    const SSAValue &iterv = poppedValue(pc, 0);
    jsbytecode *iterpc = script->code + iterv.pushedOffset();
    JS_ASSERT(JSOp(*iterpc) == JSOP_ITER || JSOp(*iterpc) == JSOP_TRAP);

    uintN flags = iterpc[1];
    if (flags & JSITER_FOREACH) {
        types->addType(cx, Type::UnknownType());
        return;
    }

    




    types->addType(cx, Type::StringType());

    pushedTypes(iterpc, 0)->add(cx,
        ArenaNew<TypeConstraintGenerator>(cx->compartment->pool, types));
}


bool
ScriptAnalysis::analyzeTypesBytecode(JSContext *cx, unsigned offset,
                                              TypeInferenceState &state)
{
    jsbytecode *pc = script->code + offset;
    JSOp op = (JSOp)*pc;

    Bytecode &code = getCode(offset);
    JS_ASSERT(!code.pushedTypes);

    InferSpew(ISpewOps, "analyze: #%u:%05u", script->id(), offset);

    unsigned defCount = GetDefCount(script, offset);
    if (ExtendedDef(pc))
        defCount++;

    TypeSet *pushed = ArenaArray<TypeSet>(cx->compartment->pool, defCount);
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
            types.setIntermediate();
            InferSpew(ISpewOps, "typeSet: %sT%p%s phi #%u:%05u:%u",
                      InferSpewColor(&types), &types, InferSpewColorReset(),
                      script->id(), offset, newv->slot);
            newv++;
        }
    }

    for (unsigned i = 0; i < defCount; i++) {
        pushed[i].setIntermediate();
        InferSpew(ISpewOps, "typeSet: %sT%p%s pushed%u #%u:%05u",
                  InferSpewColor(&pushed[i]), &pushed[i], InferSpewColorReset(),
                  i, script->id(), offset);
    }

    
    switch (op) {

        
      case JSOP_POP:
      case JSOP_NOP:
      case JSOP_TRACE:
      case JSOP_NOTRACE:
      case JSOP_GOTO:
      case JSOP_GOTOX:
      case JSOP_IFEQ:
      case JSOP_IFEQX:
      case JSOP_IFNE:
      case JSOP_IFNEX:
      case JSOP_LINENO:
      case JSOP_DEFCONST:
      case JSOP_LEAVEWITH:
      case JSOP_LEAVEBLOCK:
      case JSOP_RETRVAL:
      case JSOP_ENDITER:
      case JSOP_THROWING:
      case JSOP_GOSUB:
      case JSOP_GOSUBX:
      case JSOP_RETSUB:
      case JSOP_CONDSWITCH:
      case JSOP_DEFAULT:
      case JSOP_DEFAULTX:
      case JSOP_POPN:
      case JSOP_UNBRANDTHIS:
      case JSOP_STARTXML:
      case JSOP_STARTXMLEXPR:
      case JSOP_DEFXMLNS:
      case JSOP_SHARPINIT:
      case JSOP_INDEXBASE:
      case JSOP_INDEXBASE1:
      case JSOP_INDEXBASE2:
      case JSOP_INDEXBASE3:
      case JSOP_RESETBASE:
      case JSOP_RESETBASE0:
      case JSOP_BLOCKCHAIN:
      case JSOP_NULLBLOCKCHAIN:
      case JSOP_POPV:
      case JSOP_DEBUGGER:
      case JSOP_SETCALL:
      case JSOP_TABLESWITCH:
      case JSOP_TABLESWITCHX:
      case JSOP_LOOKUPSWITCH:
      case JSOP_LOOKUPSWITCHX:
      case JSOP_TRY:
        break;

        
      case JSOP_VOID:
      case JSOP_PUSH:
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
      case JSOP_DELDESC:
        pushed[0].addType(cx, Type::BooleanType());
        break;
      case JSOP_DOUBLE:
        pushed[0].addType(cx, Type::DoubleType());
        break;
      case JSOP_STRING:
      case JSOP_TYPEOF:
      case JSOP_TYPEOFEXPR:
      case JSOP_QNAMEPART:
      case JSOP_XMLTAGEXPR:
      case JSOP_TOATTRVAL:
      case JSOP_ADDATTRNAME:
      case JSOP_ADDATTRVAL:
      case JSOP_XMLELTEXPR:
        pushed[0].addType(cx, Type::StringType());
        break;
      case JSOP_NULL:
        pushed[0].addType(cx, Type::NullType());
        break;

      case JSOP_REGEXP:
        if (script->hasGlobal()) {
            TypeObject *object = script->types.standardType(cx, JSProto_RegExp);
            if (!object)
                return false;
            pushed[0].addType(cx, Type::ObjectType(object));
        } else {
            pushed[0].addType(cx, Type::UnknownType());
        }
        break;

      case JSOP_OBJECT: {
        JSObject *obj = GetScriptObject(cx, script, pc, 0);
        pushed[0].addType(cx, Type::ObjectType(obj));
        break;
      }

      case JSOP_STOP:
        
        if (script->fun)
            script->types.returnTypes()->addType(cx, Type::UndefinedType());
        break;

      case JSOP_OR:
      case JSOP_ORX:
      case JSOP_AND:
      case JSOP_ANDX:
        
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
        unsigned pickedDepth = (op == JSOP_SWAP ? 1 : pc[1]);
        
        poppedTypes(pc, pickedDepth)->addSubset(cx, &pushed[pickedDepth]);
        for (unsigned i = 0; i < pickedDepth; i++)
            poppedTypes(pc, i)->addSubset(cx, &pushed[pickedDepth - 1 - i]);
        break;
      }

      case JSOP_GETGLOBAL:
      case JSOP_CALLGLOBAL:
      case JSOP_GETGNAME:
      case JSOP_CALLGNAME: {
        jsid id;
        if (op == JSOP_GETGLOBAL || op == JSOP_CALLGLOBAL)
            id = GetGlobalId(cx, script, pc);
        else
            id = GetAtomId(cx, script, pc, 0);

        TypeSet *seen = script->types.bytecodeTypes(pc);
        seen->addSubset(cx, &pushed[0]);

        




        if (id == ATOM_TO_JSID(cx->runtime->atomState.typeAtoms[JSTYPE_VOID]))
            seen->addType(cx, Type::UndefinedType());
        if (id == ATOM_TO_JSID(cx->runtime->atomState.NaNAtom))
            seen->addType(cx, Type::DoubleType());
        if (id == ATOM_TO_JSID(cx->runtime->atomState.InfinityAtom))
            seen->addType(cx, Type::DoubleType());

        
        PropertyAccess(cx, script, pc, script->global()->type(), false, seen, id);

        if (op == JSOP_CALLGLOBAL || op == JSOP_CALLGNAME) {
            pushed[1].addType(cx, Type::UnknownType());
            pushed[0].addPropagateThis(cx, script, pc, Type::UnknownType());
        }

        if (CheckNextTest(pc))
            pushed[0].addType(cx, Type::UndefinedType());
        break;
      }

      case JSOP_NAME:
      case JSOP_CALLNAME: {
        



        TypeSet *seen = script->types.bytecodeTypes(pc);
        addTypeBarrier(cx, pc, seen, Type::UnknownType());
        seen->addSubset(cx, &pushed[0]);
        if (op == JSOP_CALLNAME) {
            pushed[1].addType(cx, Type::UnknownType());
            pushed[0].addPropagateThis(cx, script, pc, Type::UnknownType());
        }
        break;
      }

      case JSOP_BINDGNAME:
      case JSOP_BINDNAME:
        break;

      case JSOP_SETGNAME: {
        jsid id = GetAtomId(cx, script, pc, 0);
        PropertyAccess(cx, script, pc, script->global()->type(),
                       true, poppedTypes(pc, 0), id);
        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        break;
      }

      case JSOP_SETNAME:
      case JSOP_SETCONST:
        cx->compartment->types.monitorBytecode(cx, script, offset);
        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        break;

      case JSOP_GETXPROP: {
        TypeSet *seen = script->types.bytecodeTypes(pc);
        addTypeBarrier(cx, pc, seen, Type::UnknownType());
        seen->addSubset(cx, &pushed[0]);
        break;
      }

      case JSOP_GETFCSLOT:
      case JSOP_CALLFCSLOT: {
        unsigned index = GET_UINT16(pc);
        TypeSet *types = script->types.upvarTypes(index);
        types->addSubset(cx, &pushed[0]);
        if (op == JSOP_CALLFCSLOT) {
            pushed[1].addType(cx, Type::UndefinedType());
            pushed[0].addPropagateThis(cx, script, pc, Type::UndefinedType());
        }
        break;
      }

      case JSOP_GETARG:
      case JSOP_CALLARG:
      case JSOP_GETLOCAL:
      case JSOP_CALLLOCAL: {
        uint32 slot = GetBytecodeSlot(script, pc);
        if (trackSlot(slot)) {
            




            poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        } else if (slot < TotalSlots(script)) {
            TypeSet *types = script->types.slotTypes(slot);
            types->addSubset(cx, &pushed[0]);
        } else {
            
            pushed[0].addType(cx, Type::UnknownType());
        }
        if (op == JSOP_CALLARG || op == JSOP_CALLLOCAL) {
            pushed[1].addType(cx, Type::UndefinedType());
            pushed[0].addPropagateThis(cx, script, pc, Type::UndefinedType());
        }
        break;
      }

      case JSOP_SETARG:
      case JSOP_SETLOCAL:
      case JSOP_SETLOCALPOP: {
        uint32 slot = GetBytecodeSlot(script, pc);
        if (!trackSlot(slot) && slot < TotalSlots(script)) {
            TypeSet *types = script->types.slotTypes(slot);
            poppedTypes(pc, 0)->addSubset(cx, types);
        }

        




        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        break;
      }

      case JSOP_INCARG:
      case JSOP_DECARG:
      case JSOP_ARGINC:
      case JSOP_ARGDEC:
      case JSOP_INCLOCAL:
      case JSOP_DECLOCAL:
      case JSOP_LOCALINC:
      case JSOP_LOCALDEC: {
        uint32 slot = GetBytecodeSlot(script, pc);
        if (trackSlot(slot)) {
            poppedTypes(pc, 0)->addArith(cx, &pushed[0]);
        } else if (slot < TotalSlots(script)) {
            TypeSet *types = script->types.slotTypes(slot);
            types->addArith(cx, types);
            types->addSubset(cx, &pushed[0]);
        } else {
            pushed[0].addType(cx, Type::UnknownType());
        }
        break;
      }

      case JSOP_ARGUMENTS: {
        
        TypeObject *funType = script->fun->getType(cx);
        if (funType->unknownProperties() || funType->hasAnyFlags(OBJECT_FLAG_CREATED_ARGUMENTS)) {
            pushed[0].addType(cx, Type::UnknownType());
            break;
        }
        TypeSet *prop = funType->getProperty(cx, JSID_VOID, false);
        if (!prop)
            break;
        prop->addLazyArguments(cx, &pushed[0]);
        pushed[0].addType(cx, Type::LazyArgsType());
        break;
      }

      case JSOP_SETPROP:
      case JSOP_SETMETHOD: {
        jsid id = GetAtomId(cx, script, pc, 0);
        poppedTypes(pc, 1)->addSetProperty(cx, script, pc, poppedTypes(pc, 0), id);
        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        break;
      }

      case JSOP_LENGTH:
      case JSOP_GETPROP:
      case JSOP_CALLPROP: {
        jsid id = GetAtomId(cx, script, pc, 0);
        TypeSet *seen = script->types.bytecodeTypes(pc);

        poppedTypes(pc, 0)->addGetProperty(cx, script, pc, seen, id);
        if (op == JSOP_CALLPROP)
            poppedTypes(pc, 0)->addCallProperty(cx, script, pc, id);

        seen->addSubset(cx, &pushed[0]);
        if (op == JSOP_CALLPROP)
            poppedTypes(pc, 0)->addFilterPrimitives(cx, &pushed[1], true);
        if (CheckNextTest(pc))
            pushed[0].addType(cx, Type::UndefinedType());
        break;
      }

      




      case JSOP_GETELEM:
      case JSOP_CALLELEM: {
        TypeSet *seen = script->types.bytecodeTypes(pc);

        poppedTypes(pc, 1)->addGetProperty(cx, script, pc, seen, JSID_VOID);
        if (op == JSOP_CALLELEM)
            poppedTypes(pc, 1)->addCallProperty(cx, script, pc, JSID_VOID);

        seen->addSubset(cx, &pushed[0]);
        if (op == JSOP_CALLELEM)
            poppedTypes(pc, 1)->addFilterPrimitives(cx, &pushed[1], true);
        if (CheckNextTest(pc))
            pushed[0].addType(cx, Type::UndefinedType());
        break;
      }

      case JSOP_SETELEM:
      case JSOP_SETHOLE:
        poppedTypes(pc, 2)->addSetProperty(cx, script, pc, poppedTypes(pc, 0), JSID_VOID);
        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        break;

      case JSOP_TOID:
        



        pushed[0].addType(cx, Type::Int32Type());
        break;

      case JSOP_THIS:
        script->types.thisTypes()->addTransformThis(cx, script, &pushed[0]);
        break;

      case JSOP_RETURN:
      case JSOP_SETRVAL:
        if (script->fun)
            poppedTypes(pc, 0)->addSubset(cx, script->types.returnTypes());
        break;

      case JSOP_ADD:
        poppedTypes(pc, 0)->addArith(cx, &pushed[0], poppedTypes(pc, 1));
        poppedTypes(pc, 1)->addArith(cx, &pushed[0], poppedTypes(pc, 0));
        break;

      case JSOP_SUB:
      case JSOP_MUL:
      case JSOP_MOD:
      case JSOP_DIV:
        poppedTypes(pc, 0)->addArith(cx, &pushed[0]);
        poppedTypes(pc, 1)->addArith(cx, &pushed[0]);
        break;

      case JSOP_NEG:
      case JSOP_POS:
        poppedTypes(pc, 0)->addArith(cx, &pushed[0]);
        break;

      case JSOP_LAMBDA:
      case JSOP_LAMBDA_FC:
      case JSOP_DEFFUN:
      case JSOP_DEFFUN_FC:
      case JSOP_DEFLOCALFUN:
      case JSOP_DEFLOCALFUN_FC: {
        unsigned off = (op == JSOP_DEFLOCALFUN || op == JSOP_DEFLOCALFUN_FC) ? SLOTNO_LEN : 0;
        JSObject *obj = GetScriptObject(cx, script, pc, off);

        TypeSet *res = NULL;
        if (op == JSOP_LAMBDA || op == JSOP_LAMBDA_FC) {
            res = &pushed[0];
        } else if (op == JSOP_DEFLOCALFUN || op == JSOP_DEFLOCALFUN_FC) {
            uint32 slot = GetBytecodeSlot(script, pc);
            if (trackSlot(slot)) {
                res = &pushed[0];
            } else {
                
                JS_ASSERT(slot < TotalSlots(script));
                res = script->types.slotTypes(slot);
            }
        }

        if (res) {
            if (script->hasGlobal())
                res->addType(cx, Type::ObjectType(obj));
            else
                res->addType(cx, Type::UnknownType());
        } else {
            cx->compartment->types.monitorBytecode(cx, script, offset);
        }
        break;
      }

      case JSOP_DEFVAR:
        break;

      case JSOP_CALL:
      case JSOP_EVAL:
      case JSOP_FUNCALL:
      case JSOP_FUNAPPLY:
      case JSOP_NEW: {
        TypeSet *seen = script->types.bytecodeTypes(pc);
        seen->addSubset(cx, &pushed[0]);

        
        unsigned argCount = GetUseCount(script, offset) - 2;
        TypeCallsite *callsite = ArenaNew<TypeCallsite>(cx->compartment->pool,
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

        poppedTypes(pc, argCount + 1)->addCall(cx, callsite);
        break;
      }

      case JSOP_NEWINIT:
      case JSOP_NEWARRAY:
      case JSOP_NEWOBJECT: {
        TypeObject *initializer = GetInitializerType(cx, script, pc);
        if (script->hasGlobal()) {
            if (!initializer)
                return false;
            pushed[0].addType(cx, Type::ObjectType(initializer));
        } else {
            JS_ASSERT(!initializer);
            pushed[0].addType(cx, Type::UnknownType());
        }
        break;
      }

      case JSOP_ENDINIT:
        break;

      case JSOP_INITELEM: {
        const SSAValue &objv = poppedValue(pc, 2);
        jsbytecode *initpc = script->code + objv.pushedOffset();
        TypeObject *initializer = GetInitializerType(cx, script, initpc);

        if (initializer) {
            pushed[0].addType(cx, Type::ObjectType(initializer));
            if (!initializer->unknownProperties()) {
                




                TypeSet *types = initializer->getProperty(cx, JSID_VOID, true);
                if (!types)
                    return false;
                if (state.hasGetSet) {
                    types->addType(cx, Type::UnknownType());
                } else if (state.hasHole) {
                    if (!initializer->unknownProperties())
                        initializer->setFlags(cx, OBJECT_FLAG_NON_PACKED_ARRAY);
                } else {
                    poppedTypes(pc, 0)->addSubset(cx, types);
                }
            }
        } else {
            pushed[0].addType(cx, Type::UnknownType());
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

      case JSOP_INITPROP:
      case JSOP_INITMETHOD: {
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
        




        break;

      case JSOP_ITER:
        





        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        break;

      case JSOP_MOREITER:
        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        pushed[1].addType(cx, Type::BooleanType());
        break;

      case JSOP_FORGNAME: {
        jsid id = GetAtomId(cx, script, pc, 0);
        TypeObject *global = script->global()->type();
        if (!global->unknownProperties()) {
            TypeSet *types = global->getProperty(cx, id, true);
            if (!types)
                return false;
            setForTypes(cx, pc, types);
        }
        break;
      }

      case JSOP_FORNAME:
        cx->compartment->types.monitorBytecode(cx, script, offset);
        break;

      case JSOP_FORARG:
      case JSOP_FORLOCAL: {
        uint32 slot = GetBytecodeSlot(script, pc);
        if (trackSlot(slot)) {
            setForTypes(cx, pc, &pushed[1]);
        } else {
            if (slot < TotalSlots(script))
                setForTypes(cx, pc, script->types.slotTypes(slot));
        }
        break;
      }

      case JSOP_FORELEM:
        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        pushed[1].addType(cx, Type::UnknownType());
        break;

      case JSOP_FORPROP:
      case JSOP_ENUMELEM:
      case JSOP_ENUMCONSTELEM:
      case JSOP_ARRAYPUSH:
        cx->compartment->types.monitorBytecode(cx, script, offset);
        break;

      case JSOP_THROW:
        
        break;

      case JSOP_FINALLY:
        
        break;

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

      case JSOP_CASE:
      case JSOP_CASEX:
        poppedTypes(pc, 1)->addSubset(cx, &pushed[0]);
        break;

      case JSOP_UNBRAND:
        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        break;

      case JSOP_GENERATOR:
        if (script->fun) {
            if (script->hasGlobal()) {
                TypeObject *object = script->types.standardType(cx, JSProto_Generator);
                if (!object)
                    return false;
                script->types.returnTypes()->addType(cx, Type::ObjectType(object));
            } else {
                script->types.returnTypes()->addType(cx, Type::UnknownType());
            }
        }
        break;

      case JSOP_YIELD:
        pushed[0].addType(cx, Type::UnknownType());
        break;

      case JSOP_CALLXMLNAME:
        pushed[1].addType(cx, Type::UnknownType());
        
      case JSOP_XMLNAME:
        pushed[0].addType(cx, Type::UnknownType());
        break;

      case JSOP_SETXMLNAME:
        cx->compartment->types.monitorBytecode(cx, script, offset);
        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        break;

      case JSOP_BINDXMLNAME:
        break;

      case JSOP_TOXML:
      case JSOP_TOXMLLIST:
      case JSOP_XMLPI:
      case JSOP_XMLCDATA:
      case JSOP_XMLCOMMENT:
      case JSOP_DESCENDANTS:
      case JSOP_TOATTRNAME:
      case JSOP_QNAMECONST:
      case JSOP_QNAME:
      case JSOP_ANYNAME:
      case JSOP_GETFUNNS:
        pushed[0].addType(cx, Type::UnknownType());
        break;

      case JSOP_FILTER:
        
        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        break;

      case JSOP_ENDFILTER:
        poppedTypes(pc, 1)->addSubset(cx, &pushed[0]);
        break;

      case JSOP_DEFSHARP:
        break;

      case JSOP_USESHARP:
        pushed[0].addType(cx, Type::UnknownType());
        break;

      case JSOP_CALLEE:
        if (script->hasGlobal())
            pushed[0].addType(cx, Type::ObjectType(script->fun));
        else
            pushed[0].addType(cx, Type::UnknownType());
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

    







    if (script->hasClearedGlobal())
        return;

    if (!ranSSA()) {
        analyzeSSA(cx);
        if (failed())
            return;
    }

    if (!script->types.ensureTypeArray(cx)) {
        setOOM(cx);
        return;
    }

    



    ranInference_ = true;

    if (script->calledWithNew)
        analyzeTypesNew(cx);

    
    for (unsigned i = 0; i < script->nfixed; i++)
        script->types.localTypes(i)->addType(cx, Type::UndefinedType());

    TypeInferenceState state(cx);

    unsigned offset = 0;
    while (offset < script->length) {
        Bytecode *code = maybeCode(offset);

        jsbytecode *pc = script->code + offset;
        UntrapOpcode untrap(cx, script, pc);

        if (code && !(js_CodeSpec[*pc].format & JOF_DECOMPOSE)) {
            if (!analyzeTypesBytecode(cx, offset, state)) {
                cx->compartment->types.setPendingNukeTypes(cx);
                return;
            }
        }

        offset += GetBytecodeLength(pc);
    }

    for (unsigned i = 0; i < state.phiNodes.length(); i++) {
        SSAPhiNode *node = state.phiNodes[i];
        for (unsigned j = 0; j < node->length; j++) {
            const SSAValue &v = node->options[j];
            getValueTypes(v)->addSubset(cx, &node->types);
        }
    }

    




    TypeIntermediate *result = script->types.intermediateList;
    while (result) {
        result->replay(cx, script);
        result = result->next;
    }

    if (!script->usesArguments)
        return;

    




    if (script->fun->hasLazyType())
        return;

    if (script->fun->type()->hasAnyFlags(OBJECT_FLAG_CREATED_ARGUMENTS))
        return;

    






    if (script->fun->isHeavyweight() || cx->compartment->debugMode) {
        script->fun->type()->setFlags(cx, OBJECT_FLAG_CREATED_ARGUMENTS);
        return;
    }

    offset = 0;
    while (offset < script->length) {
        Bytecode *code = maybeCode(offset);
        jsbytecode *pc = script->code + offset;

        if (code && JSOp(*pc) == JSOP_ARGUMENTS) {
            Vector<SSAValue> seen(cx);
            if (!followEscapingArguments(cx, SSAValue::PushedValue(offset, 0), &seen)) {
                script->fun->type()->setFlags(cx, OBJECT_FLAG_CREATED_ARGUMENTS);
                return;
            }
        }

        offset += GetBytecodeLength(pc);
    }

    





    script->usedLazyArgs = true;
}

bool
ScriptAnalysis::followEscapingArguments(JSContext *cx, const SSAValue &v, Vector<SSAValue> *seen)
{
    



    if (!trackUseChain(v))
        return true;

    for (unsigned i = 0; i < seen->length(); i++) {
        if (v.equals((*seen)[i]))
            return true;
    }
    if (!seen->append(v)) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return false;
    }

    SSAUseChain *use = useChain(v);
    while (use) {
        if (!followEscapingArguments(cx, use, seen))
            return false;
        use = use->next;
    }

    return true;
}

bool
ScriptAnalysis::followEscapingArguments(JSContext *cx, SSAUseChain *use, Vector<SSAValue> *seen)
{
    if (!use->popped)
        return followEscapingArguments(cx, SSAValue::PhiValue(use->offset, use->u.phi), seen);

    jsbytecode *pc = script->code + use->offset;
    uint32 which = use->u.which;

    JSOp op = JSOp(*pc);
    JS_ASSERT(op != JSOP_TRAP);

    if (op == JSOP_POP || op == JSOP_POPN)
        return true;

    

    



    if (op == JSOP_GETELEM && which == 1)
        return true;

    if (op == JSOP_LENGTH)
        return true;

    

    if (op == JSOP_SETLOCAL) {
        uint32 slot = GetBytecodeSlot(script, pc);
        if (!trackSlot(slot))
            return false;
        if (!followEscapingArguments(cx, SSAValue::PushedValue(use->offset, 0), seen))
            return false;
        return followEscapingArguments(cx, SSAValue::WrittenVar(slot, use->offset), seen);
    }

    if (op == JSOP_GETLOCAL)
        return followEscapingArguments(cx, SSAValue::PushedValue(use->offset, 0), seen);

    return false;
}

void
ScriptAnalysis::analyzeTypesNew(JSContext *cx)
{
    JS_ASSERT(script->calledWithNew && script->fun);

    




    if (script->fun->getType(cx)->unknownProperties() ||
        script->fun->isFunctionPrototype() ||
        !script->hasGlobal()) {
        script->types.thisTypes()->addType(cx, Type::UnknownType());
        return;
    }

    TypeObject *funType = script->fun->getType(cx);
    TypeSet *prototypeTypes = funType->getProperty(cx, id_prototype(cx), false);
    if (!prototypeTypes)
        return;
    prototypeTypes->addNewObject(cx, funType, script->types.thisTypes());
}





class TypeConstraintClearDefiniteSetter : public TypeConstraint
{
public:
    TypeObject *object;

    TypeConstraintClearDefiniteSetter(TypeObject *object)
        : TypeConstraint("baseClearDefinite"), object(object)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type) {
        if (!object->newScript)
            return;
        




        if (!object->newScriptCleared && type.isUnknown())
            object->clearNewScript(cx);
    }

    TypeObject * persistentObject() { return object; }
};





class TypeConstraintClearDefiniteSingle : public TypeConstraint
{
public:
    TypeObject *object;

    TypeConstraintClearDefiniteSingle(TypeObject *object)
        : TypeConstraint("baseClearDefinite"), object(object)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type) {
        if (object->newScriptCleared)
            return;

        if (source->baseFlags() || source->getObjectCount() > 1)
            object->clearNewScript(cx);
    }
};





class TypeIntermediateClearDefinite : public TypeIntermediate
{
    uint32 offset;
    uint32 which;
    TypeObject *object;

  public:
    TypeIntermediateClearDefinite(uint32 offset, uint32 which, TypeObject *object)
        : offset(offset), which(which), object(object)
    {}

    void replay(JSContext *cx, JSScript *script)
    {
        TypeSet *pushed = script->analysis(cx)->pushedTypes(offset, which);
        pushed->add(cx, ArenaNew<TypeConstraintClearDefiniteSingle>(cx->compartment->pool, object));
    }

    bool sweep(JSContext *cx, JSCompartment *compartment)
    {
        return object->marked;
    }

    size_t allocatedSize() { return sizeof(TypeIntermediateClearDefinite); }
};

static bool
AnalyzeNewScriptProperties(JSContext *cx, TypeObject *type, JSScript *script, JSObject **pbaseobj,
                           Vector<TypeNewScript::Initializer> *initializerList)
{
    









    if (initializerList->length() > 50) {
        



        return false;
    }

    if (!script->ensureRanInference(cx)) {
        *pbaseobj = NULL;
        cx->compartment->types.setPendingNukeTypes(cx);
        return false;
    }
    ScriptAnalysis *analysis = script->analysis(cx);

    







    uint32 lastThisPopped = 0;

    unsigned nextOffset = 0;
    while (nextOffset < script->length) {
        unsigned offset = nextOffset;
        jsbytecode *pc = script->code + offset;
        UntrapOpcode untrap(cx, script, pc);

        JSOp op = JSOp(*pc);

        nextOffset += GetBytecodeLength(pc);

        Bytecode *code = analysis->maybeCode(pc);
        if (!code)
            continue;

        



        if (op == JSOP_RETURN || op == JSOP_STOP || op == JSOP_RETRVAL) {
            if (offset < lastThisPopped) {
                *pbaseobj = NULL;
                return false;
            }
            return code->unconditional;
        }

        
        if (op == JSOP_EVAL) {
            if (offset < lastThisPopped)
                *pbaseobj = NULL;
            return false;
        }

        



        if (op != JSOP_THIS)
            continue;

        SSAValue thisv = SSAValue::PushedValue(offset, 0);
        SSAUseChain *uses = analysis->useChain(thisv);

        JS_ASSERT(uses);
        if (uses->next || !uses->popped) {
            
            return false;
        }

        
        if (offset < lastThisPopped) {
            *pbaseobj = NULL;
            return false;
        }
        lastThisPopped = uses->offset;

        
        Bytecode *poppedCode = analysis->maybeCode(uses->offset);
        if (!poppedCode || !poppedCode->unconditional)
            return false;

        pc = script->code + uses->offset;
        UntrapOpcode untrapUse(cx, script, pc);

        op = JSOp(*pc);

        JSObject *obj = *pbaseobj;

        if (op == JSOP_SETPROP && uses->u.which == 1) {
            




            unsigned index = js_GetIndexFromBytecode(cx, script, pc, 0);
            jsid id = ATOM_TO_JSID(script->getAtom(index));
            if (MakeTypeId(cx, id) != id)
                return false;
            if (id == id_prototype(cx) || id == id___proto__(cx) || id == id_constructor(cx))
                return false;

            unsigned slotSpan = obj->slotSpan();
            if (!DefineNativeProperty(cx, obj, id, UndefinedValue(), NULL, NULL,
                                      JSPROP_ENUMERATE, 0, 0, DNP_SKIP_TYPE)) {
                cx->compartment->types.setPendingNukeTypes(cx);
                *pbaseobj = NULL;
                return false;
            }

            if (obj->inDictionaryMode()) {
                *pbaseobj = NULL;
                return false;
            }

            if (obj->slotSpan() == slotSpan) {
                
                return false;
            }

            TypeNewScript::Initializer setprop(TypeNewScript::Initializer::SETPROP, uses->offset);
            if (!initializerList->append(setprop)) {
                cx->compartment->types.setPendingNukeTypes(cx);
                *pbaseobj = NULL;
                return false;
            }

            if (obj->slotSpan() >= (TYPE_FLAG_DEFINITE_MASK >> TYPE_FLAG_DEFINITE_SHIFT)) {
                
                return false;
            }

            




            TypeSet *parentTypes = type->proto->getType(cx)->getProperty(cx, id, false);
            if (!parentTypes || parentTypes->unknown())
                return false;
            parentTypes->add(cx, cx->new_<TypeConstraintClearDefiniteSetter>(type));
        } else if (op == JSOP_FUNCALL && uses->u.which == GET_ARGC(pc) - 1) {
            











            
            SSAValue calleev = analysis->poppedValue(pc, GET_ARGC(pc) + 1);
            if (calleev.kind() != SSAValue::PUSHED)
                return false;
            jsbytecode *calleepc = script->code + calleev.pushedOffset();
            UntrapOpcode untrapCallee(cx, script, calleepc);
            if (JSOp(*calleepc) != JSOP_CALLPROP || calleev.pushedIndex() != 0)
                return false;

            



            analysis->breakTypeBarriersSSA(cx, analysis->poppedValue(calleepc, 0));
            analysis->breakTypeBarriers(cx, calleepc - script->code, true);

            TypeSet *funcallTypes = analysis->pushedTypes(calleepc, 0);
            TypeSet *scriptTypes = analysis->pushedTypes(calleepc, 1);

            
            JSObject *funcallObj = funcallTypes->getSingleton(cx, false);
            JSObject *scriptObj = scriptTypes->getSingleton(cx, false);
            if (!funcallObj || !scriptObj || !scriptObj->isFunction() ||
                !scriptObj->getFunctionPrivate()->isInterpreted()) {
                return false;
            }

            JSScript *functionScript = scriptObj->getFunctionPrivate()->script();

            



            TypeIntermediateClearDefinite *funcallTrap =
                cx->new_<TypeIntermediateClearDefinite>(calleev.pushedOffset(), 0, type);
            TypeIntermediateClearDefinite *calleeTrap =
                cx->new_<TypeIntermediateClearDefinite>(calleev.pushedOffset(), 1, type);
            if (!funcallTrap || !calleeTrap) {
                cx->compartment->types.setPendingNukeTypes(cx);
                *pbaseobj = NULL;
                return false;
            }
            script->types.addIntermediate(funcallTrap);
            script->types.addIntermediate(calleeTrap);
            funcallTrap->replay(cx, script);
            calleeTrap->replay(cx, script);

            TypeNewScript::Initializer pushframe(TypeNewScript::Initializer::FRAME_PUSH, uses->offset);
            if (!initializerList->append(pushframe)) {
                cx->compartment->types.setPendingNukeTypes(cx);
                *pbaseobj = NULL;
                return false;
            }

            if (!AnalyzeNewScriptProperties(cx, type, functionScript,
                                            pbaseobj, initializerList)) {
                return false;
            }

            TypeNewScript::Initializer popframe(TypeNewScript::Initializer::FRAME_POP, 0);
            if (!initializerList->append(popframe)) {
                cx->compartment->types.setPendingNukeTypes(cx);
                *pbaseobj = NULL;
                return false;
            }

            



        } else {
            
            return false;
        }
    }

    
    return true;
}

void
types::CheckNewScriptProperties(JSContext *cx, TypeObject *type, JSScript *script)
{
    if (type->unknownProperties())
        return;

    
    JSObject *baseobj = NewBuiltinClassInstance(cx, &js_ObjectClass, gc::FINALIZE_OBJECT16);
    if (!baseobj)
        return;

    Vector<TypeNewScript::Initializer> initializerList(cx);
    AnalyzeNewScriptProperties(cx, type, script, &baseobj, &initializerList);
    if (!baseobj || (baseobj->slotSpan() == 0 && type->newScriptCleared))
        return;

    gc::FinalizeKind kind = gc::GetGCObjectKind(baseobj->slotSpan());

    
    JS_ASSERT(gc::GetGCKindSlots(kind) >= baseobj->slotSpan());

    TypeNewScript::Initializer done(TypeNewScript::Initializer::DONE, 0);

    




    baseobj = NewReshapedObject(cx, type, baseobj->getParent(), kind,
                                baseobj->lastProperty());
    if (!baseobj ||
        !type->addDefiniteProperties(cx, baseobj) ||
        !initializerList.append(done)) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    size_t numBytes = sizeof(TypeNewScript)
                    + (initializerList.length() * sizeof(TypeNewScript::Initializer));
    type->newScript = (TypeNewScript *) cx->calloc_(numBytes);
    if (!type->newScript) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    type->newScript->script = script;
    type->newScript->finalizeKind = unsigned(kind);
    type->newScript->shape = baseobj->lastProperty();

    type->newScript->initializerList = (TypeNewScript::Initializer *)
        ((char *) type->newScript + sizeof(TypeNewScript));
    PodCopy(type->newScript->initializerList, initializerList.begin(), initializerList.length());
}





void
ScriptAnalysis::printTypes(JSContext *cx)
{
    AutoEnterAnalysis enter(cx);
    TypeCompartment *compartment = &script->compartment->types;

    



    for (unsigned offset = 0; offset < script->length; offset++) {
        if (!maybeCode(offset))
            continue;

        jsbytecode *pc = script->code + offset;
        UntrapOpcode untrap(cx, script, pc);

        if (js_CodeSpec[*pc].format & JOF_DECOMPOSE)
            continue;

        unsigned defCount = GetDefCount(script, offset);
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

    if (script->fun)
        printf("Function");
    else if (script->isCachedEval || script->isUncachedEval)
        printf("Eval");
    else
        printf("Main");
    printf(" #%u %s (line %d):\n", script->id(), script->filename, script->lineno);

    printf("locals:");
    printf("\n    return:");
    script->types.returnTypes()->print(cx);
    printf("\n    this:");
    script->types.thisTypes()->print(cx);

    for (unsigned i = 0; script->fun && i < script->fun->nargs; i++) {
        printf("\n    arg%u:", i);
        script->types.argTypes(i)->print(cx);
    }
    for (unsigned i = 0; i < script->nfixed; i++) {
        if (!trackSlot(LocalSlot(script, i))) {
            printf("\n    local%u:", i);
            script->types.localTypes(i)->print(cx);
        }
    }
    for (unsigned i = 0; i < script->bindings.countUpvars(); i++) {
        printf("\n    upvar%u:", i);
        script->types.upvarTypes(i)->print(cx);
    }
    printf("\n");

    for (unsigned offset = 0; offset < script->length; offset++) {
        if (!maybeCode(offset))
            continue;

        jsbytecode *pc = script->code + offset;
        UntrapOpcode untrap(cx, script, pc);

        PrintBytecode(cx, script, pc);

        if (js_CodeSpec[*pc].format & JOF_DECOMPOSE)
            continue;

        if (js_CodeSpec[*pc].format & JOF_TYPESET) {
            TypeSet *types = script->types.bytecodeTypes(pc);
            printf("  typeset %d:", (int) (types - script->types.typeArray));
            types->print(cx);
            printf("\n");
        }

        unsigned defCount = GetDefCount(script, offset);
        for (unsigned i = 0; i < defCount; i++) {
            printf("  type %d:", i);
            pushedTypes(offset, i)->print(cx);
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





namespace js {
namespace types {

void
MarkIteratorUnknownSlow(JSContext *cx)
{
    

    jsbytecode *pc;
    JSScript *script = cx->stack.currentScript(&pc);
    if (!script || !pc)
        return;

    



    if (script->compartment != cx->compartment)
        return;

    js::analyze::UntrapOpcode untrap(cx, script, pc);

    if (JSOp(*pc) == JSOP_ITER)
        TypeDynamicResult(cx, script, pc, Type::UnknownType());
}

void
TypeMonitorCallSlow(JSContext *cx, JSObject *callee,
                    const CallArgs &args, bool constructing)
{
    unsigned nargs = callee->getFunctionPrivate()->nargs;
    JSScript *script = callee->getFunctionPrivate()->script();

    if (!script->types.ensureTypeArray(cx))
        return;

    if (constructing) {
        script->types.setNewCalled(cx);
    } else {
        Type type = GetValueType(cx, args.thisv());
        script->types.setThis(cx, type);
    }

    




    unsigned arg = 0;
    for (; arg < args.argc() && arg < nargs; arg++)
        script->types.setArgument(cx, arg, args[arg]);

    
    for (; arg < nargs; arg++)
        script->types.setArgument(cx, arg, UndefinedValue());
}

static inline bool
IsAboutToBeFinalized(JSContext *cx, TypeObjectKey *key)
{
    Type type = Type::ObjectType(key);
    if (type.isSingleObject())
        return IsAboutToBeFinalized(cx, type.singleObject());
    return !type.typeObject()->marked;
}


class TypeIntermediatePushed : public TypeIntermediate
{
    uint32 offset;
    Type type;

  public:
    TypeIntermediatePushed(uint32 offset, Type type)
        : offset(offset), type(type)
    {}

    void replay(JSContext *cx, JSScript *script)
    {
        TypeSet *pushed = script->analysis(cx)->pushedTypes(offset);
        pushed->addType(cx, type);
    }

    bool hasDynamicResult(uint32 offset, Type type) {
        return this->offset == offset && this->type == type;
    }

    bool sweep(JSContext *cx, JSCompartment *compartment)
    {
        if (type.isUnknown() || type.isAnyObject() || !type.isObject())
            return true;

        TypeObjectKey *object = type.objectKey();
        if (!IsAboutToBeFinalized(cx, object))
            return true;

        return false;
    }

    size_t allocatedSize() { return sizeof(TypeIntermediatePushed); }
};

void
TypeDynamicResult(JSContext *cx, JSScript *script, jsbytecode *pc, Type type)
{
    JS_ASSERT(cx->typeInferenceEnabled());
    AutoEnterTypeInference enter(cx);

    UntrapOpcode untrap(cx, script, pc);

    
    if (js_CodeSpec[*pc].format & JOF_TYPESET) {
        TypeSet *types = script->types.bytecodeTypes(pc);
        if (!types->hasType(type)) {
            InferSpew(ISpewOps, "externalType: monitorResult #%u:%05u: %s",
                      script->id(), pc - script->code, TypeString(type));
            types->addType(cx, type);
        }
        return;
    }

    





    JSOp op = JSOp(*pc);
    const JSCodeSpec *cs = &js_CodeSpec[op];
    if (cs->format & (JOF_INC | JOF_DEC)) {
        switch (op) {
          case JSOP_INCLOCAL:
          case JSOP_DECLOCAL:
          case JSOP_LOCALINC:
          case JSOP_LOCALDEC:
          case JSOP_INCARG:
          case JSOP_DECARG:
          case JSOP_ARGINC:
          case JSOP_ARGDEC: {
            





            uint32 slot = GetBytecodeSlot(script, pc);
            if (slot < TotalSlots(script)) {
                TypeSet *types = script->types.slotTypes(slot);
                types->addType(cx, type);
            }
            break;
          }

          default:;
        }
    }

    if (script->hasAnalysis() && script->analysis(cx)->ranInference()) {
        




        TypeSet *pushed = script->analysis(cx)->pushedTypes(pc, 0);
        if (pushed->hasType(type))
            return;
    } else {
        
        TypeIntermediate *result, **pstart = &script->types.intermediateList, **presult = pstart;
        while (*presult) {
            result = *presult;
            if (result->hasDynamicResult(pc - script->code, type)) {
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

    TypeIntermediatePushed *result = cx->new_<TypeIntermediatePushed>(pc - script->code, type);
    if (!result) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }
    script->types.addIntermediate(result);

    if (script->hasAnalysis() && script->analysis(cx)->ranInference()) {
        TypeSet *pushed = script->analysis(cx)->pushedTypes(pc, 0);
        pushed->addType(cx, type);
    }

    
    if (script->fun && !script->fun->hasLazyType())
        ObjectStateChange(cx, script->fun->type(), false, true);
}

void
TypeMonitorResult(JSContext *cx, JSScript *script, jsbytecode *pc, const js::Value &rval)
{
    UntrapOpcode untrap(cx, script, pc);

    
    if (!(js_CodeSpec[*pc].format & JOF_TYPESET))
        return;

    Type type = GetValueType(cx, rval);
    TypeSet *types = script->types.bytecodeTypes(pc);
    if (types->hasType(type))
        return;

    AutoEnterTypeInference enter(cx);

    InferSpew(ISpewOps, "bytecodeType: #%u:%05u: %s",
              script->id(), pc - script->code, TypeString(type));
    types->addType(cx, type);
}

} } 









static inline bool
IgnorePushed(const jsbytecode *pc, unsigned index)
{
    JS_ASSERT(JSOp(*pc) != JSOP_TRAP);

    switch (JSOp(*pc)) {
      
      case JSOP_BINDNAME:
      case JSOP_BINDGNAME:
      case JSOP_BINDXMLNAME:
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
      case JSOP_ORX:
      case JSOP_AND:
      case JSOP_ANDX:
        return (index == 0);

      
      case JSOP_HOLE:
        return (index == 0);
      case JSOP_FILTER:
        return (index == 1);

      
      case JSOP_ENTERWITH:
      case JSOP_ENTERBLOCK:
        return true;

      
      case JSOP_FORNAME:
      case JSOP_FORGNAME:
      case JSOP_FORLOCAL:
      case JSOP_FORARG:
      case JSOP_FORPROP:
      case JSOP_FORELEM:
      case JSOP_ITER:
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
TypeScript::makeTypeArray(JSContext *cx)
{
    JS_ASSERT(!typeArray);

    AutoEnterTypeInference enter(cx);

    unsigned count = numTypeSets();
    typeArray = (TypeSet *) cx->calloc_(sizeof(TypeSet) * count);
    if (!typeArray) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return false;
    }

#ifdef DEBUG
    unsigned id = script()->id();
    for (unsigned i = 0; i < script()->nTypeSets; i++)
        InferSpew(ISpewOps, "typeSet: %sT%p%s bytecode%u #%u",
                  InferSpewColor(&typeArray[i]), &typeArray[i], InferSpewColorReset(),
                  i, id);
    InferSpew(ISpewOps, "typeSet: %sT%p%s return #%u",
              InferSpewColor(returnTypes()), returnTypes(), InferSpewColorReset(),
              id);
    InferSpew(ISpewOps, "typeSet: %sT%p%s this #%u",
              InferSpewColor(thisTypes()), thisTypes(), InferSpewColorReset(),
              id);
    unsigned nargs = script()->fun ? script()->fun->nargs : 0;
    for (unsigned i = 0; i < nargs; i++)
        InferSpew(ISpewOps, "typeSet: %sT%p%s arg%u #%u",
                  InferSpewColor(argTypes(i)), argTypes(i), InferSpewColorReset(),
                  i, id);
    for (unsigned i = 0; i < script()->nfixed; i++)
        InferSpew(ISpewOps, "typeSet: %sT%p%s local%u #%u",
                  InferSpewColor(localTypes(i)), localTypes(i), InferSpewColorReset(),
                  i, id);
    for (unsigned i = 0; i < script()->bindings.countUpvars(); i++)
        InferSpew(ISpewOps, "typeSet: %sT%p%s upvar%u #%u",
                  InferSpewColor(upvarTypes(i)), upvarTypes(i), InferSpewColorReset(),
                  i, id);
#endif

    return true;
}

bool
JSScript::typeSetFunction(JSContext *cx, JSFunction *fun, bool singleton)
{
    this->fun = fun;

    if (!cx->typeInferenceEnabled())
        return true;

    char *name = NULL;
#ifdef DEBUG
    name = (char *) alloca(10);
    JS_snprintf(name, 10, "#%u", id());
#endif

    if (singleton) {
        if (!fun->setSingletonType(cx))
            return false;
    } else {
        TypeObject *type = cx->compartment->types.newTypeObject(cx, this, name, "",
                                                                JSProto_Function, fun->getProto());
        if (!type)
            return false;
        AutoTypeRooter root(cx, type);

        js::Shape *shape = js::EmptyShape::create(cx, fun->getClass());
        if (!shape)
            return false;

        fun->setType(type);
        fun->setMap(shape);

        type->functionScript = this;
    }

    this->fun = fun;
    return true;
}

#ifdef DEBUG

void
TypeScript::checkBytecode(JSContext *cx, jsbytecode *pc, const js::Value *sp)
{
    AutoEnterTypeInference enter(cx);
    UntrapOpcode untrap(cx, script(), pc);

    if (js_CodeSpec[*pc].format & JOF_DECOMPOSE)
        return;

    if (!script()->hasAnalysis() || !script()->analysis(cx)->ranInference())
        return;
    ScriptAnalysis *analysis = script()->analysis(cx);

    int defCount = GetDefCount(script(), pc - script()->code);

    for (int i = 0; i < defCount; i++) {
        const js::Value &val = sp[-defCount + i];
        TypeSet *types = analysis->pushedTypes(pc, i);
        if (IgnorePushed(pc, i))
            continue;

        Type type = GetValueType(cx, val);

        if (!types->hasType(type)) {
            
            fprintf(stderr, "Missing type at #%u:%05u pushed %u: %s\n", 
                    script()->id(), unsigned(pc - script()->code), i, TypeString(type));
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
JSObject::splicePrototype(JSContext *cx, JSObject *proto)
{
    





    JS_ASSERT_IF(cx->typeInferenceEnabled(), hasSingletonType());

    



    getType(cx);
    if (proto)
        proto->getType(cx);

    if (!cx->typeInferenceEnabled()) {
        TypeObject *type = proto ? proto->getNewType(cx) : GetTypeEmpty(cx);
        if (!type)
            return false;
        type_ = type;
        return true;
    }

    if (type()->proto) {
        
        TypeObject **plist = &type()->proto->type()->instanceList;
        while (*plist != type())
            plist = &(*plist)->instanceNext;
        *plist = type()->instanceNext;
    }

    type()->proto = proto;

    if (proto) {
        
        type()->instanceNext = proto->type()->instanceList;
        proto->type()->instanceList = type();
    } else {
        type()->instanceNext = NULL;
    }

    AutoEnterTypeInference enter(cx);

    if (proto && proto->type()->unknownProperties() && !type()->unknownProperties()) {
        type()->markUnknown(cx);
        return true;
    }

    
    unsigned count = type()->getPropertyCount();
    for (unsigned i = 0; i < count; i++) {
        Property *prop = type()->getProperty(i);
        if (prop && !JSID_IS_EMPTY(prop->id))
            type()->getFromPrototypes(cx, prop);
    }

    return true;
}

void
JSObject::makeLazyType(JSContext *cx)
{
    JS_ASSERT(cx->typeInferenceEnabled() && hasLazyType());
    AutoEnterTypeInference enter(cx);

    char *name = NULL;
#ifdef DEBUG
    name = (char *) alloca(20);
    JS_snprintf(name, 20, "<0x%p>", (void *) this);
#endif

    TypeObject *type = cx->compartment->types.newTypeObject(cx, NULL, name, "",
                                                            JSProto_Object, getProto());
    if (!type) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    

    type->singleton = this;

    if (isFunction() && getFunctionPrivate() && getFunctionPrivate()->isInterpreted())
        type->functionScript = getFunctionPrivate()->script();

#if JS_HAS_XML_SUPPORT
    



    if (isXML() && !type->unknownProperties())
        type->markUnknown(cx);
#endif

    if (type->unknownProperties()) {
        type_ = type;
        flags ^= LAZY_TYPE;
        return;
    }

    
    type->flags |= OBJECT_FLAG_NON_DENSE_ARRAY
                |  OBJECT_FLAG_NON_PACKED_ARRAY
                |  OBJECT_FLAG_NON_TYPED_ARRAY;

    if (hasSpecialEquality())
        type->flags |= OBJECT_FLAG_SPECIAL_EQUALITY;

    const Shape *shape = lastProperty();
    while (!shape->isEmptyShape()) {
        TypeSet *types = type->getProperty(cx, MakeTypeId(cx, shape->propid), true);
        if (!types)
            return;

        if (shape->hasGetterValue() || shape->hasSetterValue()) {
            types->addType(cx, Type::UnknownType());
        } else if (shape->slot != SHAPE_INVALID_SLOT &&
                   (shape->hasDefaultGetter() || shape->isMethod())) {
            types->addType(cx, GetValueType(cx, nativeGetSlot(shape->slot)));
        } else {
            



        }

        shape = shape->previous();
    }

    type_ = type;
    flags ^= LAZY_TYPE;
}

void
JSObject::makeNewType(JSContext *cx, JSScript *newScript, bool markUnknown)
{
    JS_ASSERT(!newType);

    
    getType(cx);

    const char *name = this->type()->name();
    TypeObject *type = cx->compartment->types.newTypeObject(cx, NULL, name, "new",
                                                            JSProto_Object, this);
    if (!type)
        return;

    if (!cx->typeInferenceEnabled()) {
        newType = type;
        setDelegate();
        return;
    }

    AutoEnterTypeInference enter(cx);

    if (!this->type()->unknownProperties()) {
        
        TypeSet *types = this->type()->getProperty(cx, JSID_EMPTY, true);
        if (types)
            types->addType(cx, Type::ObjectType(type));
    }

    




    if (hasSpecialEquality())
        type->flags |= OBJECT_FLAG_SPECIAL_EQUALITY;

    if (newScript)
        CheckNewScriptProperties(cx, type, newScript);

#if JS_HAS_XML_SUPPORT
    
    if (isXML() && !type->unknownProperties())
        type->markUnknown(cx);
#endif

    if (markUnknown && !type->unknownProperties())
        type->markUnknown(cx);

    







    if (type->unknownProperties())
        type->setsMarkedUnknown = true;

    newType = type;
    setDelegate();
}





bool
TypeSet::sweep(JSContext *cx)
{
    JS_ASSERT(!intermediate());

    if (objectCount >= 2) {
        bool removed = false;
        unsigned objectCapacity = HashSetCapacity(objectCount);
        for (unsigned i = 0; i < objectCapacity; i++) {
            TypeObjectKey *object = objectSet[i];
            if (object && IsAboutToBeFinalized(cx, object)) {
                objectSet[i] = NULL;
                removed = true;
            }
        }
        if (removed) {
            
            TypeObjectKey **oldArray = objectSet;
            objectSet = NULL;
            objectCount = 0;
            for (unsigned i = 0; i < objectCapacity; i++) {
                TypeObjectKey *object = oldArray[i];
                if (object) {
                    TypeObjectKey **pentry =
                        HashSetInsert<TypeObjectKey *,TypeObjectKey,TypeObjectKey>
                            (cx, objectSet, objectCount, object, false);
                    if (!pentry)
                        return false;
                    *pentry = object;
                }
            }
            cx->free_(oldArray);
        }
    } else if (objectCount == 1) {
        TypeObjectKey *object = (TypeObjectKey *) objectSet;
        if (IsAboutToBeFinalized(cx, object)) {
            objectSet = NULL;
            objectCount = 0;
        }
    }

    if (typeFlags & TYPE_FLAG_HAS_PERSISTENT_CONSTRAINTS) {
        TypeConstraint *constraint = constraintList;
        constraintList = NULL;

        while (constraint) {
            TypeConstraint *next = constraint->next;

            TypeObject *object = constraint->persistentObject();
            if (object) {
                




                if (object->marked) {
                    constraint->next = constraintList;
                    constraintList = constraint;
                } else {
                    Foreground::delete_(constraint);
                }
            }

            constraint = next;
        }

        if (!constraintList) {
            
            typeFlags ^= TYPE_FLAG_HAS_PERSISTENT_CONSTRAINTS;
        }
    } else {
        
        constraintList = NULL;
    }

    return true;
}


static inline void
PruneInstanceObjects(TypeObject *object)
{
    TypeObject **pinstance = &object->instanceList;
    while (*pinstance) {
        if ((*pinstance)->marked)
            pinstance = &(*pinstance)->instanceNext;
        else
            *pinstance = (*pinstance)->instanceNext;
    }
}

static bool
SweepTypeObjectList(JSContext *cx, TypeObject *objects)
{
    TypeObject *object = objects;
    while (object) {
        if (!object->marked) {
            



            object = object->next;
            continue;
        }

        PruneInstanceObjects(object);

        
        unsigned count = object->getPropertyCount();
        for (unsigned i = 0; i < count; i++) {
            Property *prop = object->getProperty(i);
            if (prop) {
                if (!prop->types.sweep(cx))
                    return false;
            }
        }

        object = object->next;
    }

    return true;
}

void
TypeCompartment::sweep(JSContext *cx)
{
    PruneInstanceObjects(&typeEmpty);

    if (!SweepTypeObjectList(cx, objects))
        return;

    




    if (arrayTypeTable) {
        for (ArrayTypeTable::Enum e(*arrayTypeTable); !e.empty(); e.popFront()) {
            const ArrayTableKey &key = e.front().key;
            TypeObject *obj = e.front().value;
            JS_ASSERT(obj->proto == key.proto);
            JS_ASSERT(!key.type.isSingleObject());

            bool remove = false;
            if (key.type.isTypeObject() && !key.type.typeObject()->marked)
                remove = true;
            if (!obj->marked)
                remove = true;

            if (remove)
                e.removeFront();
        }
    }

    if (objectTypeTable) {
        for (ObjectTypeTable::Enum e(*objectTypeTable); !e.empty(); e.popFront()) {
            const ObjectTableKey &key = e.front().key;
            const ObjectTableEntry &entry = e.front().value;
            JS_ASSERT(entry.object->proto == key.proto);

            bool remove = false;
            if (!entry.object->marked || !entry.newShape->isMarked())
                remove = true;
            for (unsigned i = 0; !remove && i < key.nslots; i++) {
                if (JSID_IS_STRING(key.ids[i])) {
                    JSString *str = JSID_TO_STRING(key.ids[i]);
                    if (!str->isStaticAtom() && !str->isMarked())
                        remove = true;
                }
                JS_ASSERT(!entry.types[i].isSingleObject());
                if (entry.types[i].isTypeObject() && !entry.types[i].typeObject()->marked)
                    remove = true;
            }

            if (remove) {
                Foreground::free_(key.ids);
                Foreground::free_(entry.types);
                e.removeFront();
            }
        }
    }
}

inline void
TypeSet::destroy()
{
    JS_ASSERT(!intermediate());
    clearObjects();
    if (typeFlags & TYPE_FLAG_HAS_PERSISTENT_CONSTRAINTS) {
        while (constraintList) {
            TypeConstraint *next = constraintList->next;
            if (constraintList->persistentObject())
                Foreground::free_(constraintList);
            constraintList = next;
        }
    }
}

static void
DestroyProperty(Property *prop)
{
    prop->types.destroy();
    Foreground::delete_(prop);
}

static void
FinalizeTypeObjectList(TypeObject *&objects)
{
    TypeObject **pobject = &objects;
    while (*pobject) {
        TypeObject *object = *pobject;
        if (object->marked) {
            object->marked = false;
            object->contribution = 0;
            pobject = &object->next;
        } else {
            if (object->emptyShapes)
                Foreground::free_(object->emptyShapes);
            *pobject = object->next;

            unsigned count = object->getPropertyCount();
            for (unsigned i = 0; i < count; i++) {
                Property *prop = object->getProperty(i);
                if (prop)
                    DestroyProperty(prop);
            }
            if (count >= 2)
                Foreground::free_(object->propertySet);

            if (object->newScript)
                Foreground::free_(object->newScript);

            Foreground::delete_(object);
        }
    }
}

void
TypeCompartment::finalizeObjects()
{
    if (typeEmpty.marked) {
        typeEmpty.marked = false;
    } else if (typeEmpty.emptyShapes) {
        Foreground::free_(typeEmpty.emptyShapes);
        typeEmpty.emptyShapes = NULL;
    }

    FinalizeTypeObjectList(objects);
}

TypeCompartment::~TypeCompartment()
{
    if (pendingArray)
        Foreground::free_(pendingArray);

    if (arrayTypeTable)
        Foreground::delete_(arrayTypeTable);

    if (objectTypeTable)
        Foreground::delete_(objectTypeTable);
}

void
TypeScript::sweep(JSContext *cx)
{
    JSCompartment *compartment = script()->compartment;
    JS_ASSERT(compartment->types.inferenceEnabled);

    if (!SweepTypeObjectList(cx, typeObjects))
        return;

    if (typeArray) {
        unsigned num = numTypeSets();

        if (script()->isAboutToBeFinalized(cx)) {
            
            for (unsigned i = 0; i < num; i++)
                typeArray[i].destroy();
            cx->free_(typeArray);
            typeArray = NULL;
        } else {
            
            for (unsigned i = 0; i < num; i++) {
                if (!typeArray[i].sweep(cx))
                    return;
            }
        }
    }

    TypeIntermediate **presult = &intermediateList;
    while (*presult) {
        TypeIntermediate *result = *presult;
        if (result->sweep(cx, compartment)) {
            presult = &result->next;
        } else {
            *presult = result->next;
            cx->delete_(result);
        }
    }

    



#ifdef JS_METHODJIT
    if (script()->jitNormal)
        mjit::ReleaseScriptCode(cx, script(), true);
    if (script()->jitCtor)
        mjit::ReleaseScriptCode(cx, script(), false);
#endif
}

void
TypeScript::trace(JSTracer *trc)
{
    



    types::TypeObject *obj = typeObjects;
    while (obj) {
        if (!obj->marked)
            obj->trace(trc);
        obj = obj->next;
    }
}

void
TypeScript::finalizeObjects()
{
    FinalizeTypeObjectList(typeObjects);

    if (!script()->compartment->activeAnalysis) {
        



        script()->clearAnalysis();
    }
}

void
TypeScript::destroy()
{
    
    while (typeObjects) {
        types::TypeObject *next = typeObjects->next;
        typeObjects->next = script()->compartment->types.objects;
        script()->compartment->types.objects = typeObjects;
        typeObjects = next;
    }

    while (intermediateList) {
        TypeIntermediate *next = intermediateList->next;
        Foreground::delete_(intermediateList);
        intermediateList = next;
    }

    Foreground::free_(typeArray);
}

size_t
TypeSet::dynamicSize()
{
    size_t res = 0;

    if (objectCount >= 2)
        res += HashSetCapacity(objectCount) * sizeof(TypeObject *);

    
    TypeConstraint *constraint = constraintList;
    while (constraint) {
        res += constraint->allocatedSize();
        constraint = constraint->next;
    }

    return res;
}

static void
GetObjectListMemoryStats(TypeObject *object, JSCompartment::TypeInferenceMemoryStats *stats)
{
    while (object) {
        stats->objectMain += sizeof(TypeObject);

        if (object->propertyCount >= 2)
            stats->objectMain += HashSetCapacity(object->propertyCount) * sizeof(Property *);

        unsigned count = object->getPropertyCount();
        for (unsigned i = 0; i < count; i++) {
            Property *prop = object->getProperty(i);
            if (prop) {
                stats->objectMain += sizeof(Property);
                stats->objectSets += prop->types.dynamicSize();
            }
        }

        object = object->next;
    }
}

static void
GetScriptMemoryStats(JSScript *script, JSCompartment::TypeInferenceMemoryStats *stats)
{
    GetObjectListMemoryStats(script->types.typeObjects, stats);

    if (!script->types.typeArray)
        return;

    unsigned count = script->types.numTypeSets();
    stats->scriptMain += count * sizeof(TypeSet);
    for (unsigned i = 0; i < count; i++)
        stats->scriptSets += script->types.typeArray[i].dynamicSize();

    TypeIntermediate *intermediate = script->types.intermediateList;
    while (intermediate) {
        stats->scriptMain += intermediate->allocatedSize();
        intermediate = intermediate->next;
    }
}

void
JSCompartment::getTypeInferenceMemoryStats(TypeInferenceMemoryStats *stats)
{
    GetObjectListMemoryStats(types.objects, stats);

    for (JSCList *cursor = scripts.next; cursor != &scripts; cursor = cursor->next) {
        JSScript *script = reinterpret_cast<JSScript *>(cursor);
        GetScriptMemoryStats(script, stats);
    }

    stats->poolMain += ArenaAllocatedSize(pool);
}
