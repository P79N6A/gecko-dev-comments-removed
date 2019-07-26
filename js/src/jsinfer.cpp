





#include "jsapi.h"
#include "jsautooplen.h"
#include "jsbool.h"
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
#include "jsscope.h"
#include "jsstr.h"
#include "jsiter.h"

#include "ion/Ion.h"
#include "ion/IonCompartment.h"
#include "frontend/TokenStream.h"
#include "gc/Marking.h"
#include "js/MemoryMetrics.h"
#include "methodjit/MethodJIT.h"
#include "methodjit/Retcon.h"
#ifdef JS_METHODJIT
# include "assembler/assembler/MacroAssembler.h"
#endif

#include "jsatominlines.h"
#include "jsgcinlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"
#include "vm/Stack-inl.h"

#ifdef JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#ifdef __SUNPRO_CC
#include <alloca.h>
#endif

using namespace js;
using namespace js::types;
using namespace js::analyze;

static inline jsid
id_prototype(JSContext *cx) {
    return NameToId(cx->runtime->atomState.classPrototypeAtom);
}

static inline jsid
id_arguments(JSContext *cx) {
    return NameToId(cx->runtime->atomState.argumentsAtom);
}

static inline jsid
id_length(JSContext *cx) {
    return NameToId(cx->runtime->atomState.lengthAtom);
}

static inline jsid
id___proto__(JSContext *cx) {
    return NameToId(cx->runtime->atomState.protoAtom);
}

static inline jsid
id_constructor(JSContext *cx) {
    return NameToId(cx->runtime->atomState.constructorAtom);
}

static inline jsid
id_caller(JSContext *cx) {
    return NameToId(cx->runtime->atomState.callerAtom);
}

static inline jsid
id_toString(JSContext *cx)
{
    return NameToId(cx->runtime->atomState.toStringAtom);
}

static inline jsid
id_toSource(JSContext *cx)
{
    return NameToId(cx->runtime->atomState.toSourceAtom);
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
                  id_, this, filename ? filename : "<null>", lineno);
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
        id = MakeTypeId(cx, id);

        
        if (id == id___proto__(cx) || id == id_constructor(cx) || id == id_caller(cx))
            return true;

        




        if (cx->compartment->types.pendingCount)
            return true;

        Type type = GetValueType(cx, value);

        AutoEnterTypeInference enter(cx);

        




        TypeSet *types = obj->maybeGetProperty(cx, id);
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





TypeSet *
TypeSet::make(JSContext *cx, const char *name)
{
    JS_ASSERT(cx->compartment->activeInference);

    TypeSet *res = cx->typeLifoAlloc().new_<TypeSet>();
    if (!res) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return NULL;
    }

    InferSpew(ISpewOps, "typeSet: %sT%p%s intermediate %s",
              InferSpewColor(res), res, InferSpewColorReset(),
              name);

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

    InferSpew(ISpewOps, "addConstraint: %sT%p%s %sC%p%s %s",
              InferSpewColor(this), this, InferSpewColorReset(),
              InferSpewColor(constraint), constraint, InferSpewColorReset(),
              constraint->kind());

    JS_ASSERT(constraint->next == NULL);
    constraint->next = constraintList;
    constraintList = constraint;

    if (!callExisting)
        return;

    
    if (flags & TYPE_FLAG_UNKNOWN) {
        cx->compartment->types.addPending(cx, constraint, this, Type::UnknownType());
    } else {
        
        for (TypeFlags flag = 1; flag < TYPE_FLAG_ANYOBJECT; flag <<= 1) {
            if (flags & flag) {
                Type type = Type::PrimitiveType(TypeFlagPrimitive(flag));
                cx->compartment->types.addPending(cx, constraint, this, type);
            }
        }

        
        if (flags & TYPE_FLAG_ANYOBJECT) {
            cx->compartment->types.addPending(cx, constraint, this, Type::AnyObjectType());
        } else {
            
            unsigned count = getObjectCount();
            for (unsigned i = 0; i < count; i++) {
                TypeObjectKey *object = getObject(i);
                if (object)
                    cx->compartment->types.addPending(cx, constraint, this,
                                                      Type::ObjectType(object));
            }
        }
    }

    cx->compartment->types.resolvePending(cx);
}

void
TypeSet::print(JSContext *cx)
{
    if (flags & TYPE_FLAG_OWN_PROPERTY)
        printf(" [own]");
    if (flags & TYPE_FLAG_CONFIGURED_PROPERTY)
        printf(" [configured]");

    if (isDefiniteProperty())
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

bool
TypeSet::propertyNeedsBarrier(JSContext *cx, jsid id)
{
    id = MakeTypeId(cx, id);

    if (unknownObject())
        return true;

    for (unsigned i = 0; i < getObjectCount(); i++) {
        if (getSingleObject(i))
            return true;

        if (types::TypeObject *otype = getTypeObject(i)) {
            if (otype->unknownProperties())
                return true;

            if (types::TypeSet *propTypes = otype->maybeGetProperty(cx, id)) {
                if (propTypes->needsBarrier(cx))
                    return true;
            }
        }
    }

    addFreeze(cx);
    return false;
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
    add(cx, cx->typeLifoAlloc().new_<TypeConstraintSubset>(target));
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
    add(cx, cx->typeLifoAlloc().new_<TypeConstraintProp>(script, pc, target, id, false));
}

void
TypeSet::addSetProperty(JSContext *cx, JSScript *script, jsbytecode *pc,
                        TypeSet *target, jsid id)
{
    add(cx, cx->typeLifoAlloc().new_<TypeConstraintProp>(script, pc, target, id, true));
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
    




    jsbytecode *callpc = script->analysis()->getCallPC(pc);
    if (JSOp(*callpc) == JSOP_NEW)
        return;

    add(cx, cx->typeLifoAlloc().new_<TypeConstraintCallProp>(script, callpc, id));
}







class TypeConstraintSetElement : public TypeConstraint
{
public:
    JSScript *script;
    jsbytecode *pc;

    TypeSet *objectTypes;
    TypeSet *valueTypes;

    TypeConstraintSetElement(JSScript *script, jsbytecode *pc,
                             TypeSet *objectTypes, TypeSet *valueTypes)
        : TypeConstraint("setelement"), script(script), pc(pc),
          objectTypes(objectTypes), valueTypes(valueTypes)
    {
        JS_ASSERT(script && pc);
    }

    void newType(JSContext *cx, TypeSet *source, Type type);
};

void
TypeSet::addSetElement(JSContext *cx, JSScript *script, jsbytecode *pc,
                       TypeSet *objectTypes, TypeSet *valueTypes)
{
    add(cx, cx->typeLifoAlloc().new_<TypeConstraintSetElement>(script, pc, objectTypes,
                                                               valueTypes));
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
    add(cx, cx->typeLifoAlloc().new_<TypeConstraintCall>(site));
}


class TypeConstraintArith : public TypeConstraint
{
public:
    JSScript *script;
    jsbytecode *pc;

    
    TypeSet *target;

    
    TypeSet *other;

    TypeConstraintArith(JSScript *script, jsbytecode *pc, TypeSet *target, TypeSet *other)
        : TypeConstraint("arith"), script(script), pc(pc), target(target), other(other)
    {
        JS_ASSERT(target);
    }

    void newType(JSContext *cx, TypeSet *source, Type type);
};

void
TypeSet::addArith(JSContext *cx, JSScript *script, jsbytecode *pc, TypeSet *target, TypeSet *other)
{
    add(cx, cx->typeLifoAlloc().new_<TypeConstraintArith>(script, pc, target, other));
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
    add(cx, cx->typeLifoAlloc().new_<TypeConstraintTransformThis>(script, target));
}





class TypeConstraintPropagateThis : public TypeConstraint
{
public:
    JSScript *script;
    jsbytecode *callpc;
    Type type;
    TypeSet *types;

    TypeConstraintPropagateThis(JSScript *script, jsbytecode *callpc, Type type, TypeSet *types)
        : TypeConstraint("propagatethis"), script(script), callpc(callpc), type(type), types(types)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type);
};

void
TypeSet::addPropagateThis(JSContext *cx, JSScript *script, jsbytecode *pc, Type type, TypeSet *types)
{
    
    jsbytecode *callpc = script->analysis()->getCallPC(pc);
    if (JSOp(*callpc) == JSOP_NEW)
        return;

    add(cx, cx->typeLifoAlloc().new_<TypeConstraintPropagateThis>(script, callpc, type, types));
}


class TypeConstraintFilterPrimitive : public TypeConstraint
{
public:
    TypeSet *target;
    TypeSet::FilterKind filter;

    TypeConstraintFilterPrimitive(TypeSet *target, TypeSet::FilterKind filter)
        : TypeConstraint("filter"), target(target), filter(filter)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type)
    {
        switch (filter) {
          case TypeSet::FILTER_ALL_PRIMITIVES:
            if (type.isPrimitive())
                return;
            break;

          case TypeSet::FILTER_NULL_VOID:
            if (type.isPrimitive(JSVAL_TYPE_NULL) || type.isPrimitive(JSVAL_TYPE_UNDEFINED))
                return;
            break;

          case TypeSet::FILTER_VOID:
            if (type.isPrimitive(JSVAL_TYPE_UNDEFINED))
                return;
            break;

          default:
            JS_NOT_REACHED("Bad filter");
        }

        target->addType(cx, type);
    }
};

void
TypeSet::addFilterPrimitives(JSContext *cx, TypeSet *target, FilterKind filter)
{
    add(cx, cx->typeLifoAlloc().new_<TypeConstraintFilterPrimitive>(target, filter));
}


static inline Shape *
GetSingletonShape(JSContext *cx, JSObject *obj, jsid id)
{
    if (!obj->isNative())
        return NULL;
    Shape *shape = obj->nativeLookup(cx, id);
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
            Shape *shape = GetSingletonShape(cx, barrier->singleton, barrier->singletonId);
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
    {}

    void newType(JSContext *cx, TypeSet *source, Type type)
    {
        if (!target->hasType(type))
            script->analysis()->addTypeBarrier(cx, pc, target, type);
    }
};

void
TypeSet::addSubsetBarrier(JSContext *cx, JSScript *script, jsbytecode *pc, TypeSet *target)
{
    add(cx, cx->typeLifoAlloc().new_<TypeConstraintSubsetBarrier>(script, pc, target));
}






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
        object = TypeScript::StandardType(cx, script, JSProto_Number);
        break;

      case JSVAL_TYPE_BOOLEAN:
        object = TypeScript::StandardType(cx, script, JSProto_Boolean);
        break;

      case JSVAL_TYPE_STRING:
        object = TypeScript::StandardType(cx, script, JSProto_String);
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





static inline void
PropertyAccess(JSContext *cx, JSScript *script_, jsbytecode *pc, TypeObject *object_,
               bool assign, TypeSet *target, jsid id)
{
    RootedScript script(cx, script_);
    Rooted<TypeObject*> object(cx, object_);

    
    if (object->unknownProperties()) {
        if (!assign)
            MarkPropertyAccessUnknown(cx, script, pc, target);
        return;
    }

    
    TypeSet *types = object->getProperty(cx, id, assign);
    if (!types)
        return;
    if (assign) {
        target->addSubset(cx, types);
    } else {
        if (!types->hasPropagatedProperty())
            object->getFromPrototypes(cx, id, types);
        if (UsePropertyTypeBarrier(pc)) {
            types->addSubsetBarrier(cx, script, pc, target);
            if (object->singleton && !JSID_IS_VOID(id)) {
                





                Shape *shape = GetSingletonShape(cx, object->singleton, id);
                if (shape && object->singleton->nativeGetSlot(shape->slot()).isUndefined())
                    script->analysis()->addSingletonTypeBarrier(cx, pc, target, object->singleton, id);
            }
        } else {
            types->addSubset(cx, target);
        }
    }
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
            
            types->add(cx, cx->typeLifoAlloc().new_<TypeConstraintPropagateThis>(
                            script, callpc, type, (TypeSet *) NULL));
        }
    }
}

void
TypeConstraintSetElement::newType(JSContext *cx, TypeSet *source, Type type)
{
    if (type.isUnknown() ||
        type.isPrimitive(JSVAL_TYPE_INT32) ||
        type.isPrimitive(JSVAL_TYPE_DOUBLE)) {
        objectTypes->addSetProperty(cx, script, pc, valueTypes, JSID_VOID);
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

    JSFunction *callee = NULL;

    if (type.isSingleObject()) {
        JSObject *obj = type.singleObject();

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
                        PropertyAccess(cx, script, pc, res, true,
                                       callsite->argumentTypes[i], JSID_VOID);
                    }
                }
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

    if (!callee->script()->ensureHasTypes(cx))
        return;

    unsigned nargs = callee->nargs;

    
    for (unsigned i = 0; i < callsite->argumentCount && i < nargs; i++) {
        TypeSet *argTypes = callsite->argumentTypes[i];
        TypeSet *types = TypeScript::ArgTypes(callee->script(), i);
        argTypes->addSubsetBarrier(cx, script, pc, types);
    }

    
    for (unsigned i = callsite->argumentCount; i < nargs; i++) {
        TypeSet *types = TypeScript::ArgTypes(callee->script(), i);
        types->addType(cx, Type::UndefinedType());
    }

    TypeSet *thisTypes = TypeScript::ThisTypes(callee->script());
    TypeSet *returnTypes = TypeScript::ReturnTypes(callee->script());

    if (callsite->isNew) {
        





        thisTypes->addSubset(cx, callsite->returnTypes);
        returnTypes->addFilterPrimitives(cx, callsite->returnTypes,
                                         TypeSet::FILTER_ALL_PRIMITIVES);
    } else {
        







        returnTypes->addSubset(cx, callsite->returnTypes);
    }
}

void
TypeConstraintPropagateThis::newType(JSContext *cx, TypeSet *source, Type type)
{
    if (type.isUnknown() || type.isAnyObject()) {
        





        cx->compartment->types.monitorBytecode(cx, script, callpc - script->code);
        return;
    }

    
    JSFunction *callee = NULL;

    if (type.isSingleObject()) {
        JSObject *object = type.singleObject();
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

    if (!callee->script()->ensureHasTypes(cx))
        return;

    TypeSet *thisTypes = TypeScript::ThisTypes(callee->script());
    if (this->types)
        this->types->addSubset(cx, thisTypes);
    else
        thisTypes->addType(cx, this->type);
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
        object = TypeScript::StandardType(cx, script, JSProto_Number);
        break;
      case JSVAL_TYPE_BOOLEAN:
        object = TypeScript::StandardType(cx, script, JSProto_Boolean);
        break;
      case JSVAL_TYPE_STRING:
        object = TypeScript::StandardType(cx, script, JSProto_String);
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
        : TypeConstraint("freeze"), info(info), typeAdded(false)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type)
    {
        if (typeAdded)
            return;

        typeAdded = true;
        cx->compartment->types.addPendingRecompile(cx, info);
    }
};

void
TypeSet::addFreeze(JSContext *cx)
{
    add(cx, cx->typeLifoAlloc().new_<TypeConstraintFreeze>(
                cx->compartment->types.compiledInfo), false);
}





class TypeConstraintFreezeTypeTag : public TypeConstraint
{
public:
    RecompileInfo info;

    



    bool typeUnknown;

    TypeConstraintFreezeTypeTag(RecompileInfo info)
        : TypeConstraint("freezeTypeTag"), info(info), typeUnknown(false)
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
        cx->compartment->types.addPendingRecompile(cx, info);
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
      case TYPE_FLAG_ANYOBJECT:
        return JSVAL_TYPE_OBJECT;
      default:
        return JSVAL_TYPE_UNKNOWN;
    }
}

JSValueType
TypeSet::getKnownTypeTag(JSContext *cx)
{
    TypeFlags flags = baseFlags();
    JSValueType type;

    if (baseObjectCount())
        type = flags ? JSVAL_TYPE_UNKNOWN : JSVAL_TYPE_OBJECT;
    else
        type = GetValueTypeFromTypeFlags(flags);

    






    bool empty = flags == 0 && baseObjectCount() == 0;
    JS_ASSERT_IF(empty, type == JSVAL_TYPE_UNKNOWN);

    if (cx->compartment->types.compiledInfo.compilerOutput(cx)->script &&
        (empty || type != JSVAL_TYPE_UNKNOWN))
    {
        add(cx, cx->typeLifoAlloc().new_<TypeConstraintFreezeTypeTag>(
                  cx->compartment->types.compiledInfo), false);
    }

    return type;
}


class TypeConstraintFreezeObjectFlags : public TypeConstraint
{
public:
    RecompileInfo info;

    
    TypeObjectFlags flags;

    
    bool *pmarked;
    bool localMarked;

    TypeConstraintFreezeObjectFlags(RecompileInfo info, TypeObjectFlags flags, bool *pmarked)
        : TypeConstraint("freezeObjectFlags"), info(info), flags(flags),
          pmarked(pmarked), localMarked(false)
    {}

    TypeConstraintFreezeObjectFlags(RecompileInfo info, TypeObjectFlags flags)
        : TypeConstraint("freezeObjectFlags"), info(info), flags(flags),
          pmarked(&localMarked), localMarked(false)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type) {}

    void newObjectState(JSContext *cx, TypeObject *object, bool force)
    {
        if (object->hasAnyFlags(flags) && !*pmarked) {
            *pmarked = true;
            cx->compartment->types.addPendingRecompile(cx, info);
        } else if (force) {
            cx->compartment->types.addPendingRecompile(cx, info);
        }
    }
};





class TypeConstraintFreezeObjectFlagsSet : public TypeConstraint
{
public:
    RecompileInfo info;

    TypeObjectFlags flags;
    bool marked;

    TypeConstraintFreezeObjectFlagsSet(RecompileInfo info, TypeObjectFlags flags)
        : TypeConstraint("freezeObjectKindSet"), info(info), flags(flags), marked(false)
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
                



                TypeSet *types = object->getProperty(cx, JSID_EMPTY, false);
                if (!types)
                    return;
                types->add(cx, cx->typeLifoAlloc().new_<TypeConstraintFreezeObjectFlags>(
                                  info, flags, &marked), false);
                return;
            }
        } else {
            return;
        }

        marked = true;
        cx->compartment->types.addPendingRecompile(cx, info);
    }
};

bool
TypeSet::hasObjectFlags(JSContext *cx, TypeObjectFlags flags)
{
    if (unknownObject())
        return true;

    



    if (baseObjectCount() == 0)
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

    



    add(cx, cx->typeLifoAlloc().new_<TypeConstraintFreezeObjectFlagsSet>(
                 cx->compartment->types.compiledInfo, flags));

    return false;
}

bool
TypeSet::HasObjectFlags(JSContext *cx, TypeObject *object, TypeObjectFlags flags)
{
    if (object->hasAnyFlags(flags))
        return true;

    TypeSet *types = object->getProperty(cx, JSID_EMPTY, false);
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

    
    TypeSet *types = object->maybeGetProperty(cx, JSID_EMPTY);

    
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
TypeSet::WatchObjectStateChange(JSContext *cx, TypeObject *obj)
{
    JS_ASSERT(!obj->unknownProperties());
    TypeSet *types = obj->getProperty(cx, JSID_EMPTY, false);
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
        : TypeConstraint("freezeOwnProperty"),
          info(info), updated(false), configurable(configurable)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type) {}

    void newPropertyState(JSContext *cx, TypeSet *source)
    {
        if (updated)
            return;
        if (source->isOwnProperty(configurable)) {
            updated = true;
            cx->compartment->types.addPendingRecompile(cx, info);
        }
    }
};

static void
CheckNewScriptProperties(JSContext *cx, HandleTypeObject type, JSFunction *fun);

bool
TypeSet::isOwnProperty(JSContext *cx, TypeObject *object, bool configurable)
{
    





    if (object->flags & OBJECT_FLAG_NEW_SCRIPT_REGENERATE) {
        if (object->newScript) {
            Rooted<TypeObject*> typeObj(cx, object);
            CheckNewScriptProperties(cx, typeObj, object->newScript->fun);
        } else {
            JS_ASSERT(object->flags & OBJECT_FLAG_NEW_SCRIPT_CLEARED);
            object->flags &= ~OBJECT_FLAG_NEW_SCRIPT_REGENERATE;
        }
    }

    if (isOwnProperty(configurable))
        return true;

    add(cx, cx->typeLifoAlloc().new_<TypeConstraintFreezeOwnProperty>(
                                                      cx->compartment->types.compiledInfo,
                                                      configurable), false);
    return false;
}

bool
TypeSet::knownNonEmpty(JSContext *cx)
{
    if (baseFlags() != 0 || baseObjectCount() != 0)
        return true;

    addFreeze(cx);

    return false;
}

bool
TypeSet::knownNonStringPrimitive(JSContext *cx)
{
    TypeFlags flags = baseFlags();

    if (baseObjectCount() > 0)
        return false;

    if (flags >= TYPE_FLAG_STRING)
        return false;

    




    add(cx, cx->typeLifoAlloc().new_<TypeConstraintFreezeTypeTag>(
                cx->compartment->types.compiledInfo), false);

    if (baseFlags() == 0)
        return false;
    return true;
}

bool
TypeSet::knownSubset(JSContext *cx, TypeSet *other)
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

    addFreeze(cx);

    return true;
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

        int objArrayType = proto->getClass() - TypedArray::protoClasses;
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
    if (baseFlags() != 0 || baseObjectCount() != 1)
        return NULL;

    JSObject *obj = getSingleObject(0);
    if (!obj)
        return NULL;

    if (freeze) {
        add(cx, cx->typeLifoAlloc().new_<TypeConstraintFreeze>(
                                               cx->compartment->types.compiledInfo), false);
    }

    return obj;
}

bool
TypeSet::needsBarrier(JSContext *cx)
{
    bool result = unknownObject()
               || getObjectCount() > 0
               || hasAnyFlag(TYPE_FLAG_STRING);
    if (!result)
        addFreeze(cx);
    return result;
}





void
TypeCompartment::init(JSContext *cx)
{
    PodZero(this);

    compiledInfo.outputIndex = RecompileInfo::NoCompilerRunning;

    if (cx && cx->getRunOptions() & JSOPTION_TYPE_INFERENCE) {
#ifdef JS_METHODJIT
        JSC::MacroAssembler masm;
        if (masm.supportsFloatingPoint())
#endif
            inferenceEnabled = true;
    }
}

TypeObject *
TypeCompartment::newTypeObject(JSContext *cx, JSScript *script,
                               JSProtoKey key, JSObject *proto_, bool unknown,
                               bool isDOM)
{
    RootedObject proto(cx, proto_);
    TypeObject *object = gc::NewGCThing<TypeObject>(cx, gc::FINALIZE_TYPE_OBJECT, sizeof(TypeObject));
    if (!object)
        return NULL;
    new(object) TypeObject(proto, key == JSProto_Function, unknown);

    if (!cx->typeInferenceEnabled()) {
        object->flags |= OBJECT_FLAG_UNKNOWN_MASK;
    } else {
        if (isDOM) {
            object->setFlags(cx, OBJECT_FLAG_NON_DENSE_ARRAY
                               | OBJECT_FLAG_NON_TYPED_ARRAY
                               | OBJECT_FLAG_NON_PACKED_ARRAY);
        } else {
            object->setFlagsFromKey(cx, key);
        }
    }

    return object;
}

TypeObject *
TypeCompartment::newAllocationSiteTypeObject(JSContext *cx, AllocationSiteKey key)
{
    AutoEnterTypeInference enter(cx);

    if (!allocationSiteTable) {
        allocationSiteTable = cx->new_<AllocationSiteTable>();
        if (!allocationSiteTable || !allocationSiteTable->init()) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return NULL;
        }
    }

    AllocationSiteTable::AddPtr p = allocationSiteTable->lookupForAdd(key);
    JS_ASSERT(!p);

    RootedObject proto(cx);
    RootedObject global(cx, &key.script->global());
    if (!js_GetClassPrototype(cx, global, key.kind, &proto, NULL))
        return NULL;

    RootedScript keyScript(cx, key.script);
    TypeObject *res = newTypeObject(cx, key.script, key.kind, proto);
    if (!res) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return NULL;
    }
    key.script = keyScript;

    jsbytecode *pc = key.script->code + key.offset;

    if (JSOp(*pc) == JSOP_NEWOBJECT) {
        




        JSObject *baseobj = key.script->getObject(GET_UINT32_INDEX(pc));

        if (!res->addDefiniteProperties(cx, baseobj))
            return NULL;
    }

    if (!allocationSiteTable->add(p, key, res)) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return NULL;
    }

    return res;
}

static inline jsid
GetAtomId(JSContext *cx, JSScript *script, const jsbytecode *pc, unsigned offset)
{
    PropertyName *name = script->getName(GET_UINT32_INDEX(pc + offset));
    return MakeTypeId(cx, NameToId(name));
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

bool
types::UseNewTypeForInitializer(JSContext *cx, JSScript *script, jsbytecode *pc, JSProtoKey key)
{
    





    if (!cx->typeInferenceEnabled() || script->function())
        return false;

    if (key != JSProto_Object && !(key >= JSProto_Int8Array && key <= JSProto_Uint8ClampedArray))
        return false;

    AutoEnterTypeInference enter(cx);

    if (!script->ensureRanAnalysis(cx))
        return false;

    return !script->analysis()->getCode(pc).inLoop;
}

bool
types::ArrayPrototypeHasIndexedProperty(JSContext *cx, JSScript *script)
{
    if (!cx->typeInferenceEnabled() || !script->hasGlobal())
        return true;

    JSObject *proto = script->global().getOrCreateArrayPrototype(cx);
    if (!proto)
        return true;

    do {
        TypeObject *type = proto->getType(cx);
        if (type->unknownProperties())
            return true;
        TypeSet *indexTypes = type->getProperty(cx, JSID_VOID, false);
        if (!indexTypes || indexTypes->isOwnProperty(cx, type, true) || indexTypes->knownNonEmpty(cx))
            return true;
        proto = proto->getProto();
    } while (proto);

    return false;
}

bool
TypeCompartment::growPendingArray(JSContext *cx)
{
    unsigned newCapacity = js::Max(unsigned(100), pendingCapacity * 2);
    PendingWork *newArray = (PendingWork *) OffTheBooks::calloc_(newCapacity * sizeof(PendingWork));
    if (!newArray) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return false;
    }

    PodCopy(newArray, pendingArray, pendingCount);
    cx->free_(pendingArray);

    pendingArray = newArray;
    pendingCapacity = newCapacity;

    return true;
}

void
TypeCompartment::processPendingRecompiles(FreeOp *fop)
{
    
    Vector<CompilerOutput> *pending = pendingRecompiles;
    pendingRecompiles = NULL;

    JS_ASSERT(!pending->empty());

#ifdef JS_METHODJIT

    mjit::ExpandInlineFrames(compartment());

    for (unsigned i = 0; i < pending->length(); i++) {
        const CompilerOutput &info = (*pending)[i];
        JS_ASSERT(info.isValid());
        if (info.isJM()) {
            mjit::JITScript *jit = info.out.mjit;
            if (jit && jit->chunkDescriptor(info.chunkIndex).chunk) {
                mjit::Recompiler::clearStackReferences(fop, info.script);
                jit->destroyChunk(fop, info.chunkIndex);
            }
        }
    }

#endif 

#ifdef JS_ION
    ion::Invalidate(fop, *pending);
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
    JS_ASSERT(compartment()->activeInference);
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
        JSScript *script = i.get<JSScript>();
        mjit::ReleaseScriptCode(fop, script);
# ifdef JS_ION
        ion::FinishInvalidation(fop, script);
# endif
    }
#endif 
}

void
TypeCompartment::addPendingRecompile(JSContext *cx, CompilerOutput &co)
{
    if (!co.isValid())
        return;

#ifdef JS_METHODJIT
    mjit::JITScript *jit = co.script->getJIT(co.constructing, co.barriers);
    bool hasJITCode = jit && jit->chunkDescriptor(co.chunkIndex).chunk;

# if defined(JS_ION)
    hasJITCode |= !!co.script->hasIonScript();
# endif

    if (!hasJITCode) {
        
        return;
    }

    if (!pendingRecompiles) {
        pendingRecompiles = cx->new_< Vector<CompilerOutput> >(cx);
        if (!pendingRecompiles) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
    }

    if (!pendingRecompiles->append(co)) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    co.invalidate();
#endif
}

void
TypeCompartment::addPendingRecompile(JSContext *cx, const RecompileInfo &info)
{
    addPendingRecompile(cx, (*constrainedOutputs)[info.outputIndex]);
}

void
TypeCompartment::addPendingRecompile(JSContext *cx, JSScript *script, jsbytecode *pc)
{
#ifdef JS_METHODJIT
    CompilerOutput info;
    info.script = script;
    info.isIonFlag = false;

    for (int constructing = 0; constructing <= 1; constructing++) {
        for (int barriers = 0; barriers <= 1; barriers++) {
            if (mjit::JITScript *jit = script->getJIT((bool) constructing, (bool) barriers)) {
                info.constructing = constructing;
                info.barriers = barriers;
                info.chunkIndex = jit->chunkIndex(pc);
                info.out.mjit = jit;
                addPendingRecompile(cx, info);
            }
        }
    }
# ifdef JS_ION
    if (script->hasIonScript()) {
        info = CompilerOutput(script);
        addPendingRecompile(cx, info);
    }
# endif
#endif
}

void
TypeCompartment::monitorBytecode(JSContext *cx, JSScript *script, uint32_t offset,
                                 bool returnOnly)
{
    ScriptAnalysis *analysis = script->analysis();
    JS_ASSERT(analysis->ranInference());

    jsbytecode *pc = script->code + offset;

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

    cx->compartment->types.addPendingRecompile(cx, script, pc);

    
    if (script->function() && !script->function()->hasLazyType())
        ObjectStateChange(cx, script->function()->type(), false, true);
}

void
TypeCompartment::markSetsUnknown(JSContext *cx, TypeObject *target)
{
    JS_ASSERT(this == &cx->compartment->types);
    JS_ASSERT(!(target->flags & OBJECT_FLAG_SETS_MARKED_UNKNOWN));
    JS_ASSERT(!target->singleton);
    JS_ASSERT(target->unknownProperties());
    target->flags |= OBJECT_FLAG_SETS_MARKED_UNKNOWN;

    AutoEnterTypeInference enter(cx);

    









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
        JSScript *script = i.get<JSScript>();
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
        





        cx->compartment->types.addPendingRecompile(cx, script, const_cast<jsbytecode*>(pc));

        
        if (script->function() && !script->function()->hasLazyType())
            ObjectStateChange(cx, script->function()->type(), false, true);
    }

    
    TypeBarrier *barrier = code.typeBarriers;
    while (barrier) {
        if (barrier->target == target && barrier->type == type && !barrier->singleton)
            return;
        barrier = barrier->next;
    }

    InferSpew(ISpewOps, "typeBarrier: #%u:%05u: %sT%p%s %s",
              script->id(), pc - script->code,
              InferSpewColor(target), target, InferSpewColorReset(),
              TypeString(type));

    barrier = cx->typeLifoAlloc().new_<TypeBarrier>(target, type, (JSObject *) NULL, JSID_VOID);

    if (!barrier) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    barrier->next = code.typeBarriers;
    code.typeBarriers = barrier;
}

void
ScriptAnalysis::addSingletonTypeBarrier(JSContext *cx, const jsbytecode *pc, TypeSet *target, JSObject *singleton, jsid singletonId)
{
    JS_ASSERT(singletonId == MakeTypeId(cx, singletonId) && !JSID_IS_VOID(singletonId));

    Bytecode &code = getCode(pc);

    if (!code.typeBarriers) {
        
        cx->compartment->types.addPendingRecompile(cx, script, const_cast<jsbytecode*>(pc));
        if (script->function() && !script->function()->hasLazyType())
            ObjectStateChange(cx, script->function()->type(), false, true);
    }

    InferSpew(ISpewOps, "singletonTypeBarrier: #%u:%05u: %sT%p%s %p %s",
              script->id(), pc - script->code,
              InferSpewColor(target), target, InferSpewColorReset(),
              (void *) singleton, TypeIdString(singletonId));

    TypeBarrier *barrier = cx->typeLifoAlloc().new_<TypeBarrier>(target, Type::UndefinedType(),
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
    JSCompartment *compartment = this->compartment();
    AutoEnterAnalysis enter(compartment);

    if (!force && !InferSpewActive(ISpewResult))
        return;

    for (gc::CellIter i(compartment, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (script->hasAnalysis() && script->analysis()->ranInference())
            script->analysis()->printTypes(cx);
    }

#ifdef DEBUG
    for (gc::CellIter i(compartment, gc::FINALIZE_TYPE_OBJECT); !i.done(); i.next()) {
        TypeObject *object = i.get<TypeObject>();
        object->print(cx);
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
TypeCompartment::fixArrayType(JSContext *cx, JSObject *obj_)
{
    RootedObject obj(cx, obj_);
    AutoEnterTypeInference enter(cx);

    if (!arrayTypeTable) {
        arrayTypeTable = cx->new_<ArrayTypeTable>();
        if (!arrayTypeTable || !arrayTypeTable->init()) {
            arrayTypeTable = NULL;
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
    }

    





    JS_ASSERT(obj->isDenseArray());

    unsigned len = obj->getDenseArrayInitializedLength();
    if (len == 0)
        return;

    Type type = GetValueTypeForTable(cx, obj->getDenseArrayElement(0));

    for (unsigned i = 1; i < len; i++) {
        Type ntype = GetValueTypeForTable(cx, obj->getDenseArrayElement(i));
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
        
        Rooted<TypeObject*> objType(cx, newTypeObject(cx, NULL, JSProto_Array, obj->getProto()));
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
    jsid *ids;
    uint32_t nslots;
    uint32_t nfixed;
    JSObject *proto;

    typedef JSObject * Lookup;

    static inline uint32_t hash(JSObject *obj) {
        return (uint32_t) (JSID_BITS(obj->lastProperty()->propid().get()) ^
                         obj->slotSpan() ^ obj->numFixedSlots() ^
                         ((uint32_t)(size_t)obj->getProto() >> 2));
    }

    static inline bool match(const ObjectTableKey &v, JSObject *obj) {
        if (obj->slotSpan() != v.nslots ||
            obj->numFixedSlots() != v.nfixed ||
            obj->getProto() != v.proto) {
            return false;
        }
        Shape *shape = obj->lastProperty();
        while (!shape->isEmptyShape()) {
            if (shape->propid() != v.ids[shape->slot()])
                return false;
            shape = shape->previous();
        }
        return true;
    }
};

struct types::ObjectTableEntry
{
    ReadBarriered<TypeObject> object;
    Type *types;
};

void
TypeCompartment::fixObjectType(JSContext *cx, JSObject *obj_)
{
    RootedObject obj(cx, obj_);
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

    ObjectTypeTable::AddPtr p = objectTypeTable->lookupForAdd(obj.get());
    Shape *baseShape = obj->lastProperty();

    if (p) {
        
        Type *types = p->value.types;
        for (unsigned i = 0; i < obj->slotSpan(); i++) {
            Type ntype = GetValueTypeForTable(cx, obj->getSlot(i));
            if (ntype != types[i]) {
                if (NumberTypes(ntype, types[i])) {
                    if (types[i].isPrimitive(JSVAL_TYPE_INT32)) {
                        types[i] = Type::DoubleType();
                        Shape *shape = baseShape;
                        while (!shape->isEmptyShape()) {
                            if (shape->slot() == i) {
                                Type type = Type::DoubleType();
                                if (!p->value.object->unknownProperties()) {
                                    jsid id = MakeTypeId(cx, shape->propid());
                                    p->value.object->addPropertyType(cx, id, type);
                                }
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

        obj->setType(p->value.object);
    } else {
        
        TypeObject *objType = newTypeObject(cx, NULL, JSProto_Object, obj->getProto());
        if (!objType || !objType->addDefiniteProperties(cx, obj)) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }

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

        Shape *shape = baseShape;
        while (!shape->isEmptyShape()) {
            ids[shape->slot()] = shape->propid();
            types[shape->slot()] = GetValueTypeForTable(cx, obj->getSlot(shape->slot()));
            if (!objType->unknownProperties()) {
                jsid id = MakeTypeId(cx, shape->propid());
                objType->addPropertyType(cx, id, types[shape->slot()]);
            }
            shape = shape->previous();
        }

        ObjectTableKey key;
        key.ids = ids;
        key.nslots = obj->slotSpan();
        key.nfixed = obj->numFixedSlots();
        key.proto = obj->getProto();
        JS_ASSERT(ObjectTableKey::match(key, obj.get()));

        ObjectTableEntry entry;
        entry.object = objType;
        entry.types = types;

        p = objectTypeTable->lookupForAdd(obj.get());
        if (!objectTypeTable->add(p, key, entry)) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }

        obj->setType(objType);
    }
}





void
TypeObject::getFromPrototypes(JSContext *cx, jsid id, TypeSet *types, bool force)
{
    if (!force && types->hasPropagatedProperty())
        return;

    types->setPropagatedProperty();

    if (!proto)
        return;

    RootedTypeObject self(cx, this);
    if (proto->getType(cx)->unknownProperties()) {
        types->addType(cx, Type::UnknownType());
        return;
    }

    TypeSet *protoTypes = self->proto->getType(cx)->getProperty(cx, id, false);
    if (!protoTypes)
        return;

    protoTypes->addSubset(cx, types);

    self->proto->getType(cx)->getFromPrototypes(cx, id, protoTypes);
}

static inline void
UpdatePropertyType(JSContext *cx, TypeSet *types, JSObject *obj, Shape *shape, bool force)
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
TypeObject::addProperty(JSContext *cx, jsid id, Property **pprop)
{
    JS_ASSERT(!*pprop);
    Property *base = cx->typeLifoAlloc().new_<Property>(id);
    if (!base) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return false;
    }

    if (singleton) {
        







        if (JSID_IS_VOID(id)) {
            
            Shape *shape = singleton->lastProperty();
            while (!shape->isEmptyShape()) {
                if (JSID_IS_VOID(MakeTypeId(cx, shape->propid())))
                    UpdatePropertyType(cx, &base->types, singleton, shape, true);
                shape = shape->previous();
            }
        } else if (!JSID_IS_EMPTY(id) && singleton->isNative()) {
            Shape *shape = singleton->nativeLookup(cx, id);
            if (shape)
                UpdatePropertyType(cx, &base->types, singleton, shape, false);
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

    
    AutoEnterTypeInference enter(cx);

    Shape *shape = obj->lastProperty();
    while (!shape->isEmptyShape()) {
        jsid id = MakeTypeId(cx, shape->propid());
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
TypeObject::matchDefiniteProperties(JSObject *obj)
{
    unsigned count = getPropertyCount();
    for (unsigned i = 0; i < count; i++) {
        Property *prop = getProperty(i);
        if (!prop)
            continue;
        if (prop->types.isDefiniteProperty()) {
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

inline void
InlineAddTypeProperty(JSContext *cx, TypeObject *obj, jsid id, Type type)
{
    JS_ASSERT(id == MakeTypeId(cx, id));

    AutoEnterTypeInference enter(cx);

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
    jsid id = JSID_VOID;
    if (name) {
        JSAtom *atom = js_Atomize(cx, name, strlen(name));
        if (!atom) {
            AutoEnterTypeInference enter(cx);
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
    AutoEnterTypeInference enter(cx);

    id = MakeTypeId(cx, id);

    TypeSet *types = getProperty(cx, id, true);
    if (types)
        types->setOwnProperty(cx, true);
}

void
TypeObject::markStateChange(JSContext *cx)
{
    if (unknownProperties())
        return;

    AutoEnterTypeInference enter(cx);
    TypeSet *types = maybeGetProperty(cx, JSID_EMPTY);
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

    if (singleton) {
        
        JS_ASSERT_IF(flags & OBJECT_FLAG_UNINLINEABLE,
                     interpretedFunction->script()->uninlineable);
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
    AutoEnterTypeInference enter(cx);

    JS_ASSERT(cx->compartment->activeInference);
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

    AutoEnterTypeInference enter(cx);

    








    for (unsigned i = 0; i < getPropertyCount(); i++) {
        Property *prop = getProperty(i);
        if (!prop)
            continue;
        if (prop->types.isDefiniteProperty())
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
            JSObject *obj = &iter.thisv().toObject();

            
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
    cx->free_(savedNewScript);

    markStateChange(cx);
}

void
TypeObject::print(JSContext *cx)
{
    printf("%s : %s",
           TypeObjectString(this),
           proto ? TypeString(Type::ObjectType(proto)) : "(null)");

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
      case JSOP_AND:
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

    JSOp op = JSOp(*pc);
    JS_ASSERT(op == JSOP_NEWARRAY || op == JSOP_NEWOBJECT || op == JSOP_NEWINIT);

    bool isArray = (op == JSOP_NEWARRAY || (op == JSOP_NEWINIT && GET_UINT8(pc) == JSProto_Array));
    JSProtoKey key = isArray ? JSProto_Array : JSProto_Object;

    if (UseNewTypeForInitializer(cx, script, pc, key))
        return NULL;

    return TypeScript::InitObject(cx, script, pc, key);
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

    TypeSet *pushed = cx->typeLifoAlloc().newArrayUninitialized<TypeSet>(defCount);
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
            newv++;
        }
    }

    



    if (js_CodeSpec[*pc].format & JOF_DECOMPOSE)
        return true;

    for (unsigned i = 0; i < defCount; i++) {
        InferSpew(ISpewOps, "typeSet: %sT%p%s pushed%u #%u:%05u",
                  InferSpewColor(&pushed[i]), &pushed[i], InferSpewColorReset(),
                  i, script->id(), offset);
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
      case JSOP_STARTXML:
      case JSOP_STARTXMLEXPR:
      case JSOP_DEFXMLNS:
      case JSOP_POPV:
      case JSOP_DEBUGGER:
      case JSOP_SETCALL:
      case JSOP_TABLESWITCH:
      case JSOP_LOOKUPSWITCH:
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
      case JSOP_ACTUALSFILLED:
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
            TypeObject *object = TypeScript::StandardType(cx, script, JSProto_RegExp);
            if (!object)
                return false;
            pushed[0].addType(cx, Type::ObjectType(object));
        } else {
            pushed[0].addType(cx, Type::UnknownType());
        }
        break;

      case JSOP_OBJECT: {
        JSObject *obj = script->getObject(GET_UINT32_INDEX(pc));
        pushed[0].addType(cx, Type::ObjectType(obj));
        break;
      }

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
        jsid id = GetAtomId(cx, script, pc, 0);

        TypeSet *seen = bytecodeTypes(pc);
        seen->addSubset(cx, &pushed[0]);

        




        if (id == NameToId(cx->runtime->atomState.typeAtoms[JSTYPE_VOID]))
            seen->addType(cx, Type::UndefinedType());
        if (id == NameToId(cx->runtime->atomState.NaNAtom))
            seen->addType(cx, Type::DoubleType());
        if (id == NameToId(cx->runtime->atomState.InfinityAtom))
            seen->addType(cx, Type::DoubleType());

        
        PropertyAccess(cx, script, pc, script->global().getType(cx), false, seen, id);

        if (op == JSOP_CALLGNAME)
            pushed[0].addPropagateThis(cx, script, pc, Type::UnknownType());

        if (CheckNextTest(pc))
            pushed[0].addType(cx, Type::UndefinedType());
        break;
      }

      case JSOP_NAME:
      case JSOP_INTRINSICNAME:
      case JSOP_CALLNAME:
      case JSOP_CALLINTRINSIC: {
        TypeSet *seen = bytecodeTypes(pc);
        addTypeBarrier(cx, pc, seen, Type::UnknownType());
        seen->addSubset(cx, &pushed[0]);
        if (op == JSOP_CALLNAME || op == JSOP_CALLINTRINSIC)
            pushed[0].addPropagateThis(cx, script, pc, Type::UnknownType());
        break;
      }

      case JSOP_BINDGNAME:
      case JSOP_BINDNAME:
        break;

      case JSOP_SETGNAME: {
        jsid id = GetAtomId(cx, script, pc, 0);
        PropertyAccess(cx, script, pc, script->global().getType(cx),
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
        TypeSet *seen = bytecodeTypes(pc);
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
            TypeSet *types = TypeScript::SlotTypes(script, slot);
            types->addSubset(cx, &pushed[0]);
        } else {
            
            pushed[0].addType(cx, Type::UnknownType());
        }
        if (op == JSOP_CALLARG || op == JSOP_CALLLOCAL)
            pushed[0].addPropagateThis(cx, script, pc, Type::UndefinedType());
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
        if (op == JSOP_CALLALIASEDVAR)
            pushed[0].addPropagateThis(cx, script, pc, Type::UnknownType());
        break;

      case JSOP_SETALIASEDVAR:
        poppedTypes(pc, 0)->addSubset(cx, &pushed[0]);
        break;

      case JSOP_INCARG:
      case JSOP_DECARG:
      case JSOP_ARGINC:
      case JSOP_ARGDEC:
      case JSOP_INCLOCAL:
      case JSOP_DECLOCAL:
      case JSOP_LOCALINC:
      case JSOP_LOCALDEC: {
        uint32_t slot = GetBytecodeSlot(script, pc);
        if (trackSlot(slot)) {
            poppedTypes(pc, 0)->addArith(cx, script, pc, &pushed[0]);
        } else if (slot < TotalSlots(script)) {
            TypeSet *types = TypeScript::SlotTypes(script, slot);
            types->addArith(cx, script, pc, types);
            types->addSubset(cx, &pushed[0]);
        } else {
            pushed[0].addType(cx, Type::UnknownType());
        }
        break;
      }

      case JSOP_ARGUMENTS:
        
        if (script->needsArgsObj())
            pushed[0].addType(cx, Type::UnknownType());
        else
            pushed[0].addType(cx, Type::MagicArgType());
        break;

      case JSOP_REST: {
        TypeSet *types = script->analysis()->bytecodeTypes(pc);
        if (script->hasGlobal()) {
            TypeObject *rest = TypeScript::InitObject(cx, script, pc, JSProto_Array);
            if (!rest)
                return false;
            types->addType(cx, Type::ObjectType(rest));

            
            TypeSet *propTypes = rest->getProperty(cx, JSID_VOID, true);
            if (!propTypes)
                return false;
            propTypes->addType(cx, Type::UnknownType());
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
        TypeSet *seen = script->analysis()->bytecodeTypes(pc);

        poppedTypes(pc, 0)->addGetProperty(cx, script, pc, seen, id);
        if (op == JSOP_CALLPROP)
            poppedTypes(pc, 0)->addCallProperty(cx, script, pc, id);

        seen->addSubset(cx, &pushed[0]);
        if (CheckNextTest(pc))
            pushed[0].addType(cx, Type::UndefinedType());
        break;
      }

      




      case JSOP_GETELEM:
      case JSOP_CALLELEM: {
        TypeSet *seen = script->analysis()->bytecodeTypes(pc);

        poppedTypes(pc, 1)->addGetProperty(cx, script, pc, seen, JSID_VOID);

        seen->addSubset(cx, &pushed[0]);
        if (op == JSOP_CALLELEM)
            pushed[0].addPropagateThis(cx, script, pc, Type::UndefinedType(), poppedTypes(pc, 1));
        if (CheckNextTest(pc))
            pushed[0].addType(cx, Type::UndefinedType());
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

      case JSOP_LAMBDA:
      case JSOP_DEFFUN: {
        JSObject *obj = script->getObject(GET_UINT32_INDEX(pc));

        TypeSet *res = NULL;
        if (op == JSOP_LAMBDA)
            res = &pushed[0];

        if (res) {
            if (script->hasGlobal() && !UseNewTypeForClone(obj->toFunction()))
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
        TypeSet *seen = script->analysis()->bytecodeTypes(pc);
        seen->addSubset(cx, &pushed[0]);

        
        unsigned argCount = GetUseCount(script, offset) - 2;
        TypeCallsite *callsite = cx->typeLifoAlloc().new_<TypeCallsite>(
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
        TypeSet *types = script->analysis()->bytecodeTypes(pc);
        types->addSubset(cx, &pushed[0]);

        bool isArray = (op == JSOP_NEWARRAY || (op == JSOP_NEWINIT && GET_UINT8(pc) == JSProto_Array));
        JSProtoKey key = isArray ? JSProto_Array : JSProto_Object;

        if (UseNewTypeForInitializer(cx, script, pc, key)) {
            
            break;
        }

        TypeObject *initializer = GetInitializerType(cx, script, pc);
        if (script->hasGlobal()) {
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
      case JSOP_SPREAD: {
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
          state.forTypes = TypeSet::make(cx, "forTypes");
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
          if (script->function()) {
            if (script->hasGlobal()) {
                JSObject *proto = script->global().getOrCreateGeneratorPrototype(cx);
                if (!proto)
                    return false;
                TypeObject *object = proto->getNewType(cx);
                if (!object)
                    return false;
                TypeScript::ReturnTypes(script)->addType(cx, Type::ObjectType(object));
            } else {
                TypeScript::ReturnTypes(script)->addType(cx, Type::UnknownType());
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
      case JSOP_FILTER:
        
      case JSOP_ENDFILTER:
        pushed[0].addType(cx, Type::UnknownType());
        break;

      case JSOP_CALLEE: {
        JSFunction *fun = script->function();
        if (script->hasGlobal() && !UseNewTypeForClone(fun))
            pushed[0].addType(cx, Type::ObjectType(fun));
        else
            pushed[0].addType(cx, Type::UnknownType());
        break;
      }

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

    



    ranInference_ = true;

    
    for (unsigned i = 0; i < script->nfixed; i++)
        TypeScript::LocalTypes(script, i)->addType(cx, Type::UndefinedType());

    TypeInferenceState state(cx);

    unsigned offset = 0;
    while (offset < script->length) {
        Bytecode *code = maybeCode(offset);

        jsbytecode *pc = script->code + offset;

        if (code && !analyzeTypesBytecode(cx, offset, state)) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
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

    




    TypeResult *result = script->types->dynamicList;
    while (result) {
        if (result->offset != UINT32_MAX) {
            pushedTypes(result->offset)->addType(cx, result->type);
        } else {
            
            state.forTypes->addType(cx, Type::UnknownType());
        }
        result = result->next;
    }
}

bool
ScriptAnalysis::integerOperation(JSContext *cx, jsbytecode *pc)
{
    JS_ASSERT(uint32_t(pc - script->code) < script->length);

    switch (JSOp(*pc)) {

      case JSOP_INCARG:
      case JSOP_DECARG:
      case JSOP_ARGINC:
      case JSOP_ARGDEC:
      case JSOP_INCLOCAL:
      case JSOP_DECLOCAL:
      case JSOP_LOCALINC:
      case JSOP_LOCALDEC: {
        if (pushedTypes(pc, 0)->getKnownTypeTag(cx) != JSVAL_TYPE_INT32)
            return false;
        uint32_t slot = GetBytecodeSlot(script, pc);
        if (trackSlot(slot)) {
            if (poppedTypes(pc, 0)->getKnownTypeTag(cx) != JSVAL_TYPE_INT32)
                return false;
        }
        return true;
      }

      case JSOP_ADD:
      case JSOP_SUB:
      case JSOP_MUL:
      case JSOP_DIV:
        if (pushedTypes(pc, 0)->getKnownTypeTag(cx) != JSVAL_TYPE_INT32)
            return false;
        if (poppedTypes(pc, 0)->getKnownTypeTag(cx) != JSVAL_TYPE_INT32)
            return false;
        if (poppedTypes(pc, 1)->getKnownTypeTag(cx) != JSVAL_TYPE_INT32)
            return false;
        return true;

      default:
        return true;
    }
}





class TypeConstraintClearDefiniteSetter : public TypeConstraint
{
public:
    TypeObject *object;

    TypeConstraintClearDefiniteSetter(TypeObject *object)
        : TypeConstraint("clearDefiniteSetter"), object(object)
    {}

    void newPropertyState(JSContext *cx, TypeSet *source)
    {
        if (!object->newScript)
            return;
        





        if (!(object->flags & OBJECT_FLAG_NEW_SCRIPT_CLEARED) && source->isOwnProperty(true))
            object->clearNewScript(cx);
    }

    void newType(JSContext *cx, TypeSet *source, Type type) {}
};





class TypeConstraintClearDefiniteSingle : public TypeConstraint
{
public:
    TypeObject *object;

    TypeConstraintClearDefiniteSingle(TypeObject *object)
        : TypeConstraint("clearDefiniteSingle"), object(object)
    {}

    void newType(JSContext *cx, TypeSet *source, Type type) {
        if (object->flags & OBJECT_FLAG_NEW_SCRIPT_CLEARED)
            return;

        if (source->baseFlags() || source->getObjectCount() > 1)
            object->clearNewScript(cx);
    }
};

static bool
AnalyzePoppedThis(JSContext *cx, Vector<SSAUseChain *> *pendingPoppedThis,
                  TypeObject *type, JSFunction *fun, JSObject **pbaseobj,
                  Vector<TypeNewScript::Initializer> *initializerList);

static bool
AnalyzeNewScriptProperties(JSContext *cx, TypeObject *type, JSFunction *fun, JSObject **pbaseobj,
                           Vector<TypeNewScript::Initializer> *initializerList)
{
    









    if (initializerList->length() > 50) {
        



        return false;
    }

    JSScript *script = fun->script();
    if (!script->ensureRanAnalysis(cx) || !script->ensureRanInference(cx)) {
        *pbaseobj = NULL;
        cx->compartment->types.setPendingNukeTypes(cx);
        return false;
    }

    if (script->hasClearedGlobal())
        return false;

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
                *pbaseobj = NULL;
                entirelyAnalyzed = false;
                break;
            }

            entirelyAnalyzed = code->unconditional;
            break;
        }

        
        if (op == JSOP_EVAL) {
            if (offset < lastThisPopped)
                *pbaseobj = NULL;
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
            if (!AnalyzePoppedThis(cx, &pendingPoppedThis, type, fun, pbaseobj,
                                   initializerList)) {
                return false;
            }
        }

        if (!pendingPoppedThis.append(uses)) {
            entirelyAnalyzed = false;
            break;
        }
    }

    
    if (!pendingPoppedThis.empty() &&
        !AnalyzePoppedThis(cx, &pendingPoppedThis, type, fun, pbaseobj,
                           initializerList)) {
        return false;
    }

    
    return entirelyAnalyzed;
}

static bool
AnalyzePoppedThis(JSContext *cx, Vector<SSAUseChain *> *pendingPoppedThis,
                  TypeObject *type, JSFunction *fun, JSObject **pbaseobj,
                  Vector<TypeNewScript::Initializer> *initializerList)
{
    JSScript *script = fun->script();
    ScriptAnalysis *analysis = script->analysis();

    while (!pendingPoppedThis->empty()) {
        SSAUseChain *uses = pendingPoppedThis->back();
        pendingPoppedThis->popBack();

        jsbytecode *pc = script->code + uses->offset;
        JSOp op = JSOp(*pc);

        RootedObject obj(cx, *pbaseobj);

        if (op == JSOP_SETPROP && uses->u.which == 1) {
            




            RootedId id(cx, NameToId(script->getName(GET_UINT32_INDEX(pc))));
            if (MakeTypeId(cx, id) != id)
                return false;
            if (id_prototype(cx) == id || id___proto__(cx) == id || id_constructor(cx) == id)
                return false;

            




            JSObject *parent = type->proto;
            while (parent) {
                TypeObject *parentObject = parent->getType(cx);
                if (parentObject->unknownProperties())
                    return false;
                TypeSet *parentTypes = parentObject->getProperty(cx, id, false);
                if (!parentTypes || parentTypes->isOwnProperty(true))
                    return false;
                parentTypes->add(cx, cx->typeLifoAlloc().new_<TypeConstraintClearDefiniteSetter>(type));
                parent = parent->getProto();
            }

            unsigned slotSpan = obj->slotSpan();
            RootedValue value(cx, UndefinedValue());
            if (!DefineNativeProperty(cx, obj, id, value, NULL, NULL,
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
        } else if (op == JSOP_FUNCALL && uses->u.which == GET_ARGC(pc) - 1) {
            











            
            SSAValue calleev = analysis->poppedValue(pc, GET_ARGC(pc) + 1);
            if (calleev.kind() != SSAValue::PUSHED)
                return false;
            jsbytecode *calleepc = script->code + calleev.pushedOffset();
            if (JSOp(*calleepc) != JSOP_CALLPROP)
                return false;

            



            analysis->breakTypeBarriersSSA(cx, analysis->poppedValue(calleepc, 0));
            analysis->breakTypeBarriers(cx, calleepc - script->code, true);

            TypeSet *funcallTypes = analysis->poppedTypes(pc, GET_ARGC(pc) + 1);
            TypeSet *scriptTypes = analysis->poppedTypes(pc, GET_ARGC(pc));

            
            JSObject *funcallObj = funcallTypes->getSingleton(cx, false);
            JSObject *scriptObj = scriptTypes->getSingleton(cx, false);
            if (!funcallObj || !scriptObj || !scriptObj->isFunction() ||
                !scriptObj->toFunction()->isInterpreted()) {
                return false;
            }

            JSFunction *function = scriptObj->toFunction();

            



            funcallTypes->add(cx,
                cx->typeLifoAlloc().new_<TypeConstraintClearDefiniteSingle>(type));
            scriptTypes->add(cx,
                cx->typeLifoAlloc().new_<TypeConstraintClearDefiniteSingle>(type));

            TypeNewScript::Initializer pushframe(TypeNewScript::Initializer::FRAME_PUSH, uses->offset);
            if (!initializerList->append(pushframe)) {
                cx->compartment->types.setPendingNukeTypes(cx);
                *pbaseobj = NULL;
                return false;
            }

            if (!AnalyzeNewScriptProperties(cx, type, function,
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






static void
CheckNewScriptProperties(JSContext *cx, HandleTypeObject type, JSFunction *fun)
{
    if (type->unknownProperties())
        return;

    
    RootedObject baseobj(cx, NewBuiltinClassInstance(cx, &ObjectClass, gc::FINALIZE_OBJECT16));
    if (!baseobj) {
        if (type->newScript)
            type->clearNewScript(cx);
        return;
    }

    Vector<TypeNewScript::Initializer> initializerList(cx);
    AnalyzeNewScriptProperties(cx, type, fun, baseobj.address(), &initializerList);
    if (!baseobj || baseobj->slotSpan() == 0 || !!(type->flags & OBJECT_FLAG_NEW_SCRIPT_CLEARED)) {
        if (type->newScript)
            type->clearNewScript(cx);
        return;
    }

    




    if (type->newScript) {
        if (!type->matchDefiniteProperties(baseobj))
            type->clearNewScript(cx);
        return;
    }

    gc::AllocKind kind = gc::GetGCObjectKind(baseobj->slotSpan());

    
    JS_ASSERT(gc::GetGCKindSlots(kind) >= baseobj->slotSpan());

    TypeNewScript::Initializer done(TypeNewScript::Initializer::DONE, 0);

    




    RootedShape shape(cx, baseobj->lastProperty());
    baseobj = NewReshapedObject(cx, type, baseobj->getParent(), kind, shape);
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

    type->newScript->fun = fun;
    type->newScript->allocKind = kind;
    type->newScript->shape = baseobj->lastProperty();

    type->newScript->initializerList = (TypeNewScript::Initializer *)
        ((char *) type->newScript.get() + sizeof(TypeNewScript));
    PodCopy(type->newScript->initializerList, initializerList.begin(), initializerList.length());
}





void
ScriptAnalysis::printTypes(JSContext *cx)
{
    AutoEnterAnalysis enter(script->compartment());
    TypeCompartment *compartment = &script->compartment()->types;

    



    for (unsigned offset = 0; offset < script->length; offset++) {
        if (!maybeCode(offset))
            continue;

        jsbytecode *pc = script->code + offset;

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

    if (script->function())
        printf("Function");
    else if (script->isCachedEval)
        printf("Eval");
    else
        printf("Main");
    printf(" #%u %s (line %d):\n", script->id(), script->filename, script->lineno);

    printf("locals:");
    printf("\n    return:");
    TypeScript::ReturnTypes(script)->print(cx);
    printf("\n    this:");
    TypeScript::ThisTypes(script)->print(cx);

    for (unsigned i = 0; script->function() && i < script->function()->nargs; i++) {
        printf("\n    arg%u:", i);
        TypeScript::ArgTypes(script, i)->print(cx);
    }
    for (unsigned i = 0; i < script->nfixed; i++) {
        if (!trackSlot(LocalSlot(script, i))) {
            printf("\n    local%u:", i);
            TypeScript::LocalTypes(script, i)->print(cx);
        }
    }
    printf("\n");

    for (unsigned offset = 0; offset < script->length; offset++) {
        if (!maybeCode(offset))
            continue;

        jsbytecode *pc = script->code + offset;

        PrintBytecode(cx, script, pc);

        if (js_CodeSpec[*pc].format & JOF_DECOMPOSE)
            continue;

        if (js_CodeSpec[*pc].format & JOF_TYPESET) {
            TypeSet *types = script->analysis()->bytecodeTypes(pc);
            printf("  typeset %d:", (int) (types - script->types->typeArray()));
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

    if (JSOp(*pc) != JSOP_ITER)
        return;

    AutoEnterTypeInference enter(cx);

    








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

    
    if (script->function() && !script->function()->hasLazyType())
        ObjectStateChange(cx, script->function()->type(), false, true);
}

void
TypeMonitorCallSlow(JSContext *cx, JSObject *callee,
                    const CallArgs &args, bool constructing)
{
    unsigned nargs = callee->toFunction()->nargs;
    JSScript *script = callee->toFunction()->script();

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
    bool isMarked = IsCellMarked(&tmp);
    JS_ASSERT(tmp == reinterpret_cast<gc::Cell *>(uintptr_t(key) & ~1));
    return !isMarked;
}

void
TypeDynamicResult(JSContext *cx, JSScript *script, jsbytecode *pc, Type type)
{
    JS_ASSERT(cx->typeInferenceEnabled());
    AutoEnterTypeInference enter(cx);

    
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
            





            uint32_t slot = GetBytecodeSlot(script, pc);
            if (slot < TotalSlots(script)) {
                TypeSet *types = TypeScript::SlotTypes(script, slot);
                types->addType(cx, type);
            }
            break;
          }

          default:;
        }
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

    if (script->hasAnalysis() && script->analysis()->ranInference()) {
        TypeSet *pushed = script->analysis()->pushedTypes(pc, 0);
        pushed->addType(cx, type);
    }

    
    if (script->function() && !script->function()->hasLazyType())
        ObjectStateChange(cx, script->function()->type(), false, true);
}

void
TypeMonitorResult(JSContext *cx, JSScript *script, jsbytecode *pc, const js::Value &rval)
{
    
    if (!(js_CodeSpec[*pc].format & JOF_TYPESET))
        return;

    AutoEnterTypeInference enter(cx);

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

} } 









static inline bool
IgnorePushed(const jsbytecode *pc, unsigned index)
{
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
      case JSOP_AND:
        return (index == 0);

      
      case JSOP_HOLE:
        return (index == 0);
      case JSOP_FILTER:
        return (index == 1);

      
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
        types = (TypeScript *) cx->calloc_(sizeof(TypeScript));
        if (!types) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        new(types) TypeScript();
        return true;
    }

    AutoEnterTypeInference enter(cx);

    unsigned count = TypeScript::NumTypeSets(this);

    types = (TypeScript *) cx->calloc_(sizeof(TypeScript) + (sizeof(TypeSet) * count));
    if (!types) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return false;
    }

    new(types) TypeScript();

#ifdef DEBUG
    TypeSet *typeArray = types->typeArray();
    for (unsigned i = 0; i < nTypeSets; i++)
        InferSpew(ISpewOps, "typeSet: %sT%p%s bytecode%u #%u",
                  InferSpewColor(&typeArray[i]), &typeArray[i], InferSpewColorReset(),
                  i, id());
    TypeSet *returnTypes = TypeScript::ReturnTypes(this);
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

    types->analysis = cx->typeLifoAlloc().new_<ScriptAnalysis>(this);

    if (!types->analysis)
        return false;

    Rooted<JSScript*> self(cx, this);

    types->analysis->analyzeBytecode(cx);

    if (self->types->analysis->OOM()) {
        self->types->analysis = NULL;
        return false;
    }

    return true;
}

bool
JSFunction::setTypeForScriptedFunction(JSContext *cx, bool singleton)
{
    JS_ASSERT(script());
    JS_ASSERT(script()->function() == this);

    if (!cx->typeInferenceEnabled())
        return true;

    if (singleton) {
        if (!setSingletonType(cx))
            return false;
    } else if (UseNewTypeForClone(this)) {
        



    } else {
        RootedFunction self(cx, this);

        TypeObject *type = cx->compartment->types.newTypeObject(cx, script(),
                                                                JSProto_Function, getProto());
        if (!type)
            return false;

        self->setType(type);
        type->interpretedFunction = self;
    }

    return true;
}

#ifdef DEBUG

 void
TypeScript::CheckBytecode(JSContext *cx, JSScript *script, jsbytecode *pc, const js::Value *sp)
{
    AutoEnterTypeInference enter(cx);

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
JSObject::splicePrototype(JSContext *cx, JSObject *proto_)
{
    RootedObject proto(cx, proto_);
    RootedObject self(cx, this);

    





    JS_ASSERT_IF(cx->typeInferenceEnabled(), self->hasSingletonType());

    
    JS_ASSERT_IF(proto, !proto->getClass()->ext.outerObject);

    



    Rooted<TypeObject*> type(cx, self->getType(cx));
    Rooted<TypeObject*> protoType(cx, NULL);
    if (proto) {
        protoType = proto->getType(cx);
        if (!proto->getNewType(cx))
            return false;
    }

    if (!cx->typeInferenceEnabled()) {
        TypeObject *type = proto ? proto->getNewType(cx) : cx->compartment->getEmptyType(cx);
        if (!type)
            return false;
        self->type_ = type;
        return true;
    }

    type->proto = proto;

    AutoEnterTypeInference enter(cx);

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

void
JSObject::makeLazyType(JSContext *cx)
{
    JS_ASSERT(hasLazyType());

    RootedObject self(cx, this);
    JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(getClass());
    TypeObject *type = cx->compartment->types.newTypeObject(cx, NULL, key, getProto());
    AssertRootingUnnecessary safe(cx);
    if (!type) {
        if (cx->typeInferenceEnabled())
            cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    if (!cx->typeInferenceEnabled()) {
        
        self->type_ = type;
        return;
    }

    AutoEnterTypeInference enter(cx);

    

    type->singleton = self;

    if (self->isFunction() && self->toFunction()->isInterpreted()) {
        type->interpretedFunction = self->toFunction();
        JSScript *script = type->interpretedFunction->script();
        if (script->uninlineable)
            type->flags |= OBJECT_FLAG_UNINLINEABLE;
    }

    if (self->lastProperty()->hasObjectFlag(BaseShape::ITERATED_SINGLETON))
        type->flags |= OBJECT_FLAG_ITERATED;

#if JS_HAS_XML_SUPPORT
    



    if (self->isXML() && !type->unknownProperties())
        type->markUnknown(cx);
#endif

    if (self->getClass()->ext.equality)
        type->flags |= OBJECT_FLAG_SPECIAL_EQUALITY;

    




    if (self->isSlowArray())
        type->flags |= OBJECT_FLAG_NON_DENSE_ARRAY | OBJECT_FLAG_NON_PACKED_ARRAY;

    if (IsTypedArrayProto(self))
        type->flags |= OBJECT_FLAG_NON_TYPED_ARRAY;

    self->type_ = type;
}

 inline HashNumber
TypeObjectEntry::hash(JSObject *proto)
{
    return PointerHasher<JSObject *, 3>::hash(proto);
}

 inline bool
TypeObjectEntry::match(TypeObject *key, JSObject *lookup)
{
    return key->proto == lookup;
}

#ifdef DEBUG
bool
JSObject::hasNewType(TypeObject *type)
{
    TypeObjectSet &table = compartment()->newTypeObjects;

    if (!table.initialized())
        return false;

    TypeObjectSet::Ptr p = table.lookup(this);
    return p && *p == type;
}
#endif 

bool
JSObject::setNewTypeUnknown(JSContext *cx)
{
    if (!setFlag(cx, js::BaseShape::NEW_TYPE_UNKNOWN))
        return false;

    




    TypeObjectSet &table = cx->compartment->newTypeObjects;
    if (table.initialized()) {
        if (TypeObjectSet::Ptr p = table.lookup(this))
            MarkTypeObjectUnknownProperties(cx, *p);
    }

    return true;
}

TypeObject *
JSObject::getNewType(JSContext *cx, JSFunction *fun_, bool isDOM)
{
    TypeObjectSet &table = cx->compartment->newTypeObjects;

    if (!table.initialized() && !table.init())
        return NULL;

    TypeObjectSet::AddPtr p = table.lookupForAdd(this);
    if (p) {
        TypeObject *type = *p;

        










        if (type->newScript && type->newScript->fun != fun_)
            type->clearNewScript(cx);

        if (!isDOM && !type->hasAnyFlags(OBJECT_FLAG_NON_DOM))
            type->setFlags(cx, OBJECT_FLAG_NON_DOM);

        return type;
    }

    RootedObject self(cx, this);
    RootedFunction fun(cx, fun_);

    if (!setDelegate(cx))
        return NULL;

    bool markUnknown = self->lastProperty()->hasObjectFlag(BaseShape::NEW_TYPE_UNKNOWN);

    RootedTypeObject type(cx);
    type = cx->compartment->types.newTypeObject(cx, NULL, JSProto_Object, self,
                                                markUnknown, isDOM);
    if (!type)
        return NULL;

    if (!table.relookupOrAdd(p, self, type.get()))
        return NULL;

    if (!cx->typeInferenceEnabled())
        return type;

    AutoEnterTypeInference enter(cx);

    




    if (self->hasSpecialEquality())
        type->flags |= OBJECT_FLAG_SPECIAL_EQUALITY;

    if (fun)
        CheckNewScriptProperties(cx, type, fun);

#if JS_HAS_XML_SUPPORT
    
    if (self->isXML() && !type->unknownProperties())
        type->flags |= OBJECT_FLAG_UNKNOWN_MASK;
#endif

    if (self->getClass()->ext.equality)
        type->flags |= OBJECT_FLAG_SPECIAL_EQUALITY;

    







    if (type->unknownProperties())
        type->flags |= OBJECT_FLAG_SETS_MARKED_UNKNOWN;

    return type;
}

TypeObject *
JSCompartment::getLazyType(JSContext *cx, JSObject *proto_)
{
    RootedObject proto(cx, proto_);
    MaybeCheckStackRoots(cx);

    TypeObjectSet &table = cx->compartment->lazyTypeObjects;

    if (!table.initialized() && !table.init())
        return NULL;

    TypeObjectSet::AddPtr p = table.lookupForAdd(proto);
    if (p) {
        TypeObject *type = *p;
        JS_ASSERT(type->lazy());

        return type;
    }

    TypeObject *type = cx->compartment->types.newTypeObject(cx, NULL,
                                                            JSProto_Object, proto, false);
    if (!type)
        return NULL;

    if (!table.relookupOrAdd(p, proto, type))
        return NULL;

    type->singleton = (JSObject *) TypeObject::LAZY_SINGLETON;

    return type;
}





void
TypeSet::sweep(JSCompartment *compartment)
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
                        (compartment, objectSet, objectCount, object);
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

    if (!isMarked()) {
        if (newScript)
            fop->free_(newScript);
        return;
    }

    JSCompartment *compartment = this->compartment();

    





    unsigned propertyCount = basePropertyCount();
    if (propertyCount >= 2) {
        unsigned oldCapacity = HashSetCapacity(propertyCount);
        Property **oldArray = propertySet;

        clearProperties();
        propertyCount = 0;
        for (unsigned i = 0; i < oldCapacity; i++) {
            Property *prop = oldArray[i];
            if (prop && prop->types.isOwnProperty(false)) {
                Property *newProp = compartment->typeLifoAlloc.new_<Property>(*prop);
                if (newProp) {
                    Property **pentry =
                        HashSetInsert<jsid,Property,Property>
                            (compartment, propertySet, propertyCount, prop->id);
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
        if (prop->types.isOwnProperty(false)) {
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

struct SweepTypeObjectOp
{
    FreeOp *fop;
    SweepTypeObjectOp(FreeOp *fop) : fop(fop) {}
    void operator()(gc::Cell *cell) {
        TypeObject *object = static_cast<TypeObject *>(cell);
        object->sweep(fop);
    }
};

void
SweepTypeObjects(FreeOp *fop, JSCompartment *compartment)
{
    SweepTypeObjectOp op(fop);
    gc::ForEachArenaAndCell(compartment, gc::FINALIZE_TYPE_OBJECT, gc::EmptyArenaOp, op);
}

void
TypeCompartment::sweep(FreeOp *fop)
{
    JSCompartment *compartment = this->compartment();

    SweepTypeObjects(fop, compartment);

    




    if (arrayTypeTable) {
        for (ArrayTypeTable::Enum e(*arrayTypeTable); !e.empty(); e.popFront()) {
            const ArrayTableKey &key = e.front().key;
            TypeObject *obj = e.front().value;
            JS_ASSERT(obj->proto == key.proto);
            JS_ASSERT(!key.type.isSingleObject());

            bool remove = false;
            if (key.type.isTypeObject() && !key.type.typeObject()->isMarked())
                remove = true;
            if (!obj->isMarked())
                remove = true;

            if (remove)
                e.removeFront();
        }
    }

    if (objectTypeTable) {
        for (ObjectTypeTable::Enum e(*objectTypeTable); !e.empty(); e.popFront()) {
            const ObjectTableKey &key = e.front().key;
            ObjectTableEntry &entry = e.front().value;
            JS_ASSERT(entry.object->proto == key.proto);

            bool remove = false;
            if (!IsTypeObjectMarked(entry.object.unsafeGet()))
                remove = true;
            for (unsigned i = 0; !remove && i < key.nslots; i++) {
                if (JSID_IS_STRING(key.ids[i])) {
                    JSString *str = JSID_TO_STRING(key.ids[i]);
                    if (!IsStringMarked(&str))
                        remove = true;
                    JS_ASSERT(AtomToId((JSAtom *)str) == key.ids[i]);
                }
                JS_ASSERT(!entry.types[i].isSingleObject());
                if (entry.types[i].isTypeObject() && !entry.types[i].typeObject()->isMarked())
                    remove = true;
            }

            if (remove) {
                Foreground::free_(key.ids);
                Foreground::free_(entry.types);
                e.removeFront();
            }
        }
    }

    if (allocationSiteTable) {
        for (AllocationSiteTable::Enum e(*allocationSiteTable); !e.empty(); e.popFront()) {
            AllocationSiteKey key = e.front().key;
            bool keyMarked = IsScriptMarked(&key.script);
            bool valMarked = IsTypeObjectMarked(e.front().value.unsafeGet());
            if (!keyMarked || !valMarked)
                e.removeFront();
            else
                e.rekeyFront(key);
        }
    }

    



    if (pendingArray)
        fop->free_(pendingArray);

    pendingArray = NULL;
    pendingCapacity = 0;
}

void
JSCompartment::sweepNewTypeObjectTable(TypeObjectSet &table)
{
    if (table.initialized()) {
        for (TypeObjectSet::Enum e(table); !e.empty(); e.popFront()) {
            TypeObject *type = e.front();
            if (!type->isMarked())
                e.removeFront();
        }
    }
}

TypeCompartment::~TypeCompartment()
{
    if (pendingArray)
        Foreground::free_(pendingArray);

    if (arrayTypeTable)
        Foreground::delete_(arrayTypeTable);

    if (objectTypeTable)
        Foreground::delete_(objectTypeTable);

    if (allocationSiteTable)
        Foreground::delete_(allocationSiteTable);
}

 void
TypeScript::Sweep(FreeOp *fop, JSScript *script)
{
    JSCompartment *compartment = script->compartment();
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
}

void
TypeScript::destroy()
{
    while (dynamicList) {
        TypeResult *next = dynamicList->next;
        Foreground::delete_(dynamicList);
        dynamicList = next;
    }

    Foreground::free_(this);
}

inline size_t
TypeSet::computedSizeOfExcludingThis()
{
    




    uint32_t count = baseObjectCount();
    if (count >= 2)
        return HashSetCapacity(count) * sizeof(TypeObject *);
    return 0;
}

inline size_t
TypeObject::computedSizeOfExcludingThis()
{
    




    size_t bytes = 0;

    uint32_t count = basePropertyCount();
    if (count >= 2)
        bytes += HashSetCapacity(count) * sizeof(TypeObject *);

    count = getPropertyCount();
    for (unsigned i = 0; i < count; i++) {
        Property *prop = getProperty(i);
        if (prop)
            bytes += sizeof(Property) + prop->types.computedSizeOfExcludingThis();
    }

    return bytes;
}

static void
SizeOfScriptTypeInferenceData(JSScript *script, TypeInferenceSizes *sizes,
                              JSMallocSizeOfFun mallocSizeOf)
{
    TypeScript *typeScript = script->types;
    if (!typeScript)
        return;

    
    if (!script->compartment()->types.inferenceEnabled) {
        sizes->scripts += mallocSizeOf(typeScript);
        return;
    }

    unsigned count = TypeScript::NumTypeSets(script);
    sizes->scripts += mallocSizeOf(typeScript);

    TypeResult *result = typeScript->dynamicList;
    while (result) {
        sizes->scripts += mallocSizeOf(result);
        result = result->next;
    }

    



    TypeSet *typeArray = typeScript->typeArray();
    for (unsigned i = 0; i < count; i++) {
        size_t bytes = typeArray[i].computedSizeOfExcludingThis();
        sizes->scripts += bytes;
        sizes->temporary -= bytes;
    }
}

void
JSCompartment::sizeOfTypeInferenceData(TypeInferenceSizes *sizes, JSMallocSizeOfFun mallocSizeOf)
{
    




    sizes->temporary += typeLifoAlloc.sizeOfExcludingThis(mallocSizeOf);

    
    sizes->temporary += mallocSizeOf(types.pendingArray);

    
    JS_ASSERT(!types.pendingRecompiles);

    for (gc::CellIter i(this, gc::FINALIZE_SCRIPT); !i.done(); i.next())
        SizeOfScriptTypeInferenceData(i.get<JSScript>(), sizes, mallocSizeOf);

    if (types.allocationSiteTable)
        sizes->tables += types.allocationSiteTable->sizeOfIncludingThis(mallocSizeOf);

    if (types.arrayTypeTable)
        sizes->tables += types.arrayTypeTable->sizeOfIncludingThis(mallocSizeOf);

    if (types.objectTypeTable) {
        sizes->tables += types.objectTypeTable->sizeOfIncludingThis(mallocSizeOf);

        for (ObjectTypeTable::Enum e(*types.objectTypeTable);
             !e.empty();
             e.popFront())
        {
            const ObjectTableKey &key = e.front().key;
            const ObjectTableEntry &value = e.front().value;

            
            sizes->tables += mallocSizeOf(key.ids) + mallocSizeOf(value.types);
        }
    }
}

void
TypeObject::sizeOfExcludingThis(TypeInferenceSizes *sizes, JSMallocSizeOfFun mallocSizeOf)
{
    if (singleton) {
        




        JS_ASSERT(!newScript);
        return;
    }

    sizes->objects += mallocSizeOf(newScript);

    



    size_t bytes = computedSizeOfExcludingThis();
    sizes->objects += bytes;
    sizes->temporary -= bytes;
}
