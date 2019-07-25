






































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

const char *
types::TypeString(jstype type)
{
    switch (type) {
      case TYPE_UNDEFINED:
        return "void";
      case TYPE_NULL:
        return "null";
      case TYPE_BOOLEAN:
        return "bool";
      case TYPE_INT32:
        return "int";
      case TYPE_DOUBLE:
        return "float";
      case TYPE_STRING:
        return "string";
      case TYPE_UNKNOWN:
        return "unknown";
      default: {
        JS_ASSERT(TypeIsObject(type));
        TypeObject *object = (TypeObject *) type;
        return object->name();
      }
    }
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


static inline bool
TypeSetMatches(JSContext *cx, TypeSet *types, jstype type)
{
    if (types->hasType(type))
        return true;

    





    if (TypeIsObject(type) && ((TypeObject*)type)->unknownProperties()) {
        unsigned count = types->getObjectCount();
        for (unsigned i = 0; i < count; i++) {
            TypeObject *object = types->getObject(i);
            if (object && object->unknownProperties())
                return true;
        }
    }

    return false;
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

        jstype type = GetValueType(cx, value);

        AutoEnterTypeInference enter(cx);

        TypeSet *types = obj->getProperty(cx, id, false);
        if (types && !TypeSetMatches(cx, types, type)) {
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
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[infer failure] ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    cx->compartment->types.print(cx, cx->compartment);

    fflush(stderr);
    *((int*)NULL) = 0;  
}





void
TypeSet::addTypeSet(JSContext *cx, ClonedTypeSet *types)
{
    if (types->typeFlags & TYPE_FLAG_UNKNOWN) {
        addType(cx, TYPE_UNKNOWN);
        return;
    }

    for (jstype type = TYPE_UNDEFINED; type <= TYPE_STRING; type++) {
        if (types->typeFlags & (1 << type))
            addType(cx, type);
    }

    if (types->objectCount >= 2) {
        for (unsigned i = 0; i < types->objectCount; i++)
            addType(cx, (jstype) types->objectSet[i]);
    } else if (types->objectCount == 1) {
        addType(cx, (jstype) types->objectSet);
    }
}

inline void
TypeSet::add(JSContext *cx, TypeConstraint *constraint, bool callExisting)
{
    if (!constraint) {
        
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    JS_ASSERT_IF(!constraint->condensed() && !constraint->persistentObject(),
                 constraint->script->compartment == cx->compartment);
    JS_ASSERT_IF(!constraint->condensed(), cx->compartment->activeInference);
    JS_ASSERT_IF(intermediate(), !constraint->persistentObject() && !constraint->condensed());

    InferSpew(ISpewOps, "addConstraint: T%p C%p %s",
              this, constraint, constraint->kind());

    JS_ASSERT(constraint->next == NULL);
    constraint->next = constraintList;
    constraintList = constraint;

    if (!callExisting)
        return;

    if (typeFlags & TYPE_FLAG_UNKNOWN) {
        cx->compartment->types.addPending(cx, constraint, this, TYPE_UNKNOWN);
        cx->compartment->types.resolvePending(cx);
        return;
    }

    for (jstype type = TYPE_UNDEFINED; type <= TYPE_STRING; type++) {
        if (typeFlags & (1 << type))
            cx->compartment->types.addPending(cx, constraint, this, type);
    }

    unsigned count = getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        TypeObject *object = getObject(i);
        if (object)
            cx->compartment->types.addPending(cx, constraint, this, (jstype) object);
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

    if (objectCount) {
        printf(" object[%u]", objectCount);

        unsigned count = getObjectCount();
        for (unsigned i = 0; i < count; i++) {
            TypeObject *object = getObject(i);
            if (object)
                printf(" %s", object->name());
        }
    }
}






class TypeConstraintSubset : public TypeConstraint
{
public:
    TypeSet *target;

    TypeConstraintSubset(JSScript *script, TypeSet *target)
        : TypeConstraint("subset", script), target(target)
    {
        JS_ASSERT(target);
    }

    void newType(JSContext *cx, TypeSet *source, jstype type)
    {
        
        target->addType(cx, type);
    }
};

void
TypeSet::addSubset(JSContext *cx, JSScript *script, TypeSet *target)
{
    add(cx, ArenaNew<TypeConstraintSubset>(cx->compartment->pool, script, target));
}


class TypeConstraintBaseSubset : public TypeConstraint
{
public:
    TypeObject *object;
    TypeSet *target;

    TypeConstraintBaseSubset(TypeObject *object, TypeSet *target)
        : TypeConstraint("baseSubset", (JSScript *) 0x1),
          object(object), target(target)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type)
    {
        target->addType(cx, type);
    }

    TypeObject * persistentObject() { return object; }
};

void
TypeSet::addBaseSubset(JSContext *cx, TypeObject *obj, TypeSet *target)
{
    add(cx, cx->new_<TypeConstraintBaseSubset>(obj, target));
}


class TypeConstraintCondensed : public TypeConstraint
{
public:
    TypeConstraintCondensed(JSScript *script)
        : TypeConstraint("condensed", script)
    {}

    void checkAnalysis(JSContext *cx)
    {
        if (script->hasAnalysis() && script->analysis(cx)->ranInference()) {
            









            return;
        }

        script->analysis(cx)->analyzeTypes(cx);
    }

    void newType(JSContext *cx, TypeSet*, jstype) { checkAnalysis(cx); }
    void newPropertyState(JSContext *cx, TypeSet*) { checkAnalysis(cx); }
    void newObjectState(JSContext *cx, TypeObject*, bool) { checkAnalysis(cx); }

    bool condensed() { return true; }
};

bool
TypeSet::addCondensed(JSContext *cx, JSScript *script)
{
    
    TypeConstraintCondensed *constraint = OffTheBooks::new_<TypeConstraintCondensed>(script);

    if (!constraint)
        return false;

    add(cx, constraint, false);
    return true;
}


class TypeConstraintProp : public TypeConstraint
{
public:
    jsbytecode *pc;

    



    bool assign;
    TypeSet *target;

    
    jsid id;

    TypeConstraintProp(JSScript *script, jsbytecode *pc,
                       TypeSet *target, jsid id, bool assign)
        : TypeConstraint("prop", script), pc(pc),
          assign(assign), target(target), id(id)
    {
        JS_ASSERT(script && pc);

        
        JS_ASSERT_IF(!target, assign);
    }

    void newType(JSContext *cx, TypeSet *source, jstype type);
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


class TypeConstraintNewObject : public TypeConstraint
{
    TypeFunction *fun;
    TypeSet *target;

  public:
    TypeConstraintNewObject(JSScript *script, TypeFunction *fun, TypeSet *target)
        : TypeConstraint("newObject", script), fun(fun), target(target)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type);
};

void
TypeSet::addNewObject(JSContext *cx, JSScript *script, TypeFunction *fun, TypeSet *target)
{
    add(cx, ArenaNew<TypeConstraintNewObject>(cx->compartment->pool, script, fun, target));
}






class TypeConstraintCall : public TypeConstraint
{
public:
    
    TypeCallsite *callsite;

    TypeConstraintCall(TypeCallsite *callsite)
        : TypeConstraint("call", callsite->script), callsite(callsite)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type);
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

    TypeConstraintArith(JSScript *script, TypeSet *target, TypeSet *other)
        : TypeConstraint("arith", script), target(target), other(other)
    {
        JS_ASSERT(target);
    }

    void newType(JSContext *cx, TypeSet *source, jstype type);
};

void
TypeSet::addArith(JSContext *cx, JSScript *script, TypeSet *target, TypeSet *other)
{
    add(cx, ArenaNew<TypeConstraintArith>(cx->compartment->pool, script, target, other));
}


class TypeConstraintTransformThis : public TypeConstraint
{
public:
    TypeSet *target;

    TypeConstraintTransformThis(JSScript *script, TypeSet *target)
        : TypeConstraint("transformthis", script), target(target)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type);
};

void
TypeSet::addTransformThis(JSContext *cx, JSScript *script, TypeSet *target)
{
    add(cx, ArenaNew<TypeConstraintTransformThis>(cx->compartment->pool, script, target));
}





class TypeConstraintPropagateThis : public TypeConstraint
{
public:
    jstype type;

    TypeConstraintPropagateThis(JSScript *script, jstype type)
        : TypeConstraint("propagatethis", script), type(type)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type);
};

void
TypeSet::addPropagateThis(JSContext *cx, JSScript *script, jsbytecode *pc, jstype type)
{
    




    SSAValue calleev = SSAValue::PushedValue(pc - script->code, 0);
    SSAUseChain *uses = script->analysis(cx)->useChain(calleev);

    if (uses && !uses->next && uses->popped) {
        jsbytecode *callpc = script->code + uses->offset;
        UntrapOpcode untrap(cx, script, callpc);
        if (JSOp(*callpc) == JSOP_NEW)
            return;
    }

    add(cx, ArenaNew<TypeConstraintPropagateThis>(cx->compartment->pool, script, type));
}


class TypeConstraintFilterPrimitive : public TypeConstraint
{
public:
    TypeSet *target;

    
    bool onlyNullVoid;

    TypeConstraintFilterPrimitive(JSScript *script, TypeSet *target, bool onlyNullVoid)
        : TypeConstraint("filter", script), target(target), onlyNullVoid(onlyNullVoid)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type)
    {
        if (onlyNullVoid) {
            if (type == TYPE_NULL || type == TYPE_UNDEFINED)
                return;
        } else if (type != TYPE_UNKNOWN && TypeIsPrimitive(type)) {
            return;
        }

        target->addType(cx, type);
    }
};

void
TypeSet::addFilterPrimitives(JSContext *cx, JSScript *script, TypeSet *target, bool onlyNullVoid)
{
    add(cx, ArenaNew<TypeConstraintFilterPrimitive>(cx->compartment->pool,
                                                    script, target, onlyNullVoid));
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
        } else if (all || (TypeIsObject(barrier->type) &&
                           barrier->target->getObjectCount() >= BARRIER_OBJECT_LIMIT)) {
            
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
    jsbytecode *pc;
    TypeSet *target;

    TypeConstraintSubsetBarrier(JSScript *script, jsbytecode *pc, TypeSet *target)
        : TypeConstraint("subsetBarrier", script), pc(pc), target(target)
    {
        JS_ASSERT(!target->intermediate());
    }

    void newType(JSContext *cx, TypeSet *source, jstype type)
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





class TypeConstraintGenerator : public TypeConstraint
{
public:
    TypeSet *target;

    TypeConstraintGenerator(JSScript *script, TypeSet *target)
        : TypeConstraint("generator", script), target(target)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type)
    {
        if (type == TYPE_UNKNOWN) {
            target->addType(cx, TYPE_UNKNOWN);
            return;
        }

        if (TypeIsPrimitive(type))
            return;

        



        TypeObject *object = (TypeObject *) type;
        if (object->proto) {
            Class *clasp = object->proto->getClass();
            if (clasp == &js_IteratorClass || clasp == &js_GeneratorClass)
                target->addType(cx, TYPE_UNKNOWN);
        }
    }
};






static inline TypeObject *
GetPropertyObject(JSContext *cx, JSScript *script, jstype type)
{
    if (TypeIsObject(type))
        return (TypeObject*) type;

    



    TypeObject *object = NULL;
    switch (type) {

      case TYPE_INT32:
      case TYPE_DOUBLE:
        object = script->getTypeNewObject(cx, JSProto_Number);
        break;

      case TYPE_BOOLEAN:
        object = script->getTypeNewObject(cx, JSProto_Boolean);
        break;

      case TYPE_STRING:
        object = script->getTypeNewObject(cx, JSProto_String);
        break;

      default:
        
        return NULL;
    }

    if (!object)
        cx->compartment->types.setPendingNukeTypes(cx);
    return object;
}





static inline void
PropertyAccess(JSContext *cx, JSScript *script, jsbytecode *pc, TypeObject *object,
               bool assign, TypeSet *target, jsid id)
{
    JS_ASSERT_IF(!target, assign);

    
    if (assign && id == id_prototype(cx)) {
        cx->compartment->types.monitorBytecode(cx, script, pc - script->code);
        return;
    }

    
    if (id == id___proto__(cx) || id == id_constructor(cx) || id == id_caller(cx)) {
        if (assign)
            cx->compartment->types.monitorBytecode(cx, script, pc - script->code);
        else
            target->addType(cx, TYPE_UNKNOWN);
        return;
    }

    
    if (object->unknownProperties()) {
        if (!assign)
            target->addType(cx, TYPE_UNKNOWN);
        return;
    }

    
    if (target) {
        TypeSet *types = object->getProperty(cx, id, assign);
        if (!types)
            return;
        if (assign)
            target->addSubset(cx, script, types);
        else if (CanHaveReadBarrier(pc))
            types->addSubsetBarrier(cx, script, pc, target);
        else
            types->addSubset(cx, script, target);
    } else {
        TypeSet *readTypes = object->getProperty(cx, id, false);
        TypeSet *writeTypes = object->getProperty(cx, id, true);
        if (!readTypes || !writeTypes)
            return;
        readTypes->addArith(cx, script, writeTypes);
    }
}

void
TypeConstraintProp::newType(JSContext *cx, TypeSet *source, jstype type)
{
    if (type == TYPE_UNKNOWN || (!TypeIsObject(type) && !script->global)) {
        





        if (assign)
            cx->compartment->types.monitorBytecode(cx, script, pc - script->code);
        else
            target->addType(cx, TYPE_UNKNOWN);
        return;
    }

    TypeObject *object = GetPropertyObject(cx, script, type);
    if (object) {
        UntrapOpcode untrap(cx, script, pc);
        PropertyAccess(cx, script, pc, object, assign, target, id);

        if (!object->unknownProperties() &&
            (JSOp(*pc) == JSOP_CALLPROP || JSOp(*pc) == JSOP_CALLELEM)) {
            JS_ASSERT(!assign);
            TypeSet *types = object->getProperty(cx, id, false);
            if (!types)
                return;
            types->addPropagateThis(cx, script, pc, type);
        }
    }
}

void
TypeConstraintNewObject::newType(JSContext *cx, TypeSet *source, jstype type)
{
    if (type == TYPE_UNKNOWN) {
        target->addType(cx, TYPE_UNKNOWN);
        return;
    }

    if (TypeIsObject(type)) {
        TypeObject *object = (TypeObject *) type;
        if (object->unknownProperties()) {
            target->addType(cx, TYPE_UNKNOWN);
        } else {
            TypeSet *newTypes = object->getProperty(cx, JSID_EMPTY, false);
            if (!newTypes)
                return;
            newTypes->addSubset(cx, script, target);
        }
    } else if (!fun->script) {
        




    } else if (!fun->script->global) {
        target->addType(cx, TYPE_UNKNOWN);
    } else {
        TypeObject *object = fun->script->getTypeNewObject(cx, JSProto_Object);
        if (!object) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
        target->addType(cx, (jstype) object);
    }
}

void
TypeConstraintCall::newType(JSContext *cx, TypeSet *source, jstype type)
{
    JSScript *script = callsite->script;
    jsbytecode *pc = callsite->pc;

    if (type == TYPE_UNKNOWN) {
        
        cx->compartment->types.monitorBytecode(cx, script, pc - script->code);
        return;
    }

    if (!TypeIsObject(type))
        return;

    
    TypeObject *object = (TypeObject*) type;
    if (object->unknownProperties()) {
        
        cx->compartment->types.monitorBytecode(cx, script, pc - script->code);
        return;
    }
    if (!object->isFunction) {
        



        return;
    }
    TypeFunction *function = object->asFunction();

    if (!function->script) {
        JS_ASSERT(function->handler && function->singleton);

        





        if (!script->global || script->global != function->singleton->getGlobal()) {
            callsite->returnTypes->addType(cx, TYPE_UNKNOWN);
            return;
        }

        if (function->isGeneric) {
            if (callsite->argumentCount == 0) {
                
                return; 
            }

            





            TypeSet *thisTypes = TypeSet::make(cx, "genericthis");
            if (!thisTypes)
                return;
            callsite->argumentTypes[0]->addTransformThis(cx, script, thisTypes);

            TypeCallsite *newSite = ArenaNew<TypeCallsite>(cx->compartment->pool,
                                                           cx, script, pc, callsite->isNew,
                                                           callsite->argumentCount - 1);
            if (!newSite || (callsite->argumentCount > 1 && !newSite->argumentTypes)) {
                cx->compartment->types.setPendingNukeTypes(cx);
                return;
            }

            newSite->thisTypes = thisTypes;
            newSite->returnTypes = callsite->returnTypes;
            for (unsigned i = 0; i < callsite->argumentCount - 1; i++)
                newSite->argumentTypes[i] = callsite->argumentTypes[i + 1];

            function->handler(cx, (JSTypeFunction*)function, (JSTypeCallsite*)newSite);
        } else {
            
            function->handler(cx, (JSTypeFunction*)function, (JSTypeCallsite*)callsite);
        }

        return;
    }

    JSScript *callee = function->script;
    unsigned nargs = callee->fun->nargs;

    if (!callee->ensureTypeArray(cx))
        return;

    
    if (!callee->analyzed) {
        ScriptAnalysis *calleeAnalysis = callee->analysis(cx);
        if (!calleeAnalysis) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
        calleeAnalysis->analyzeTypes(cx);
    }

    
    for (unsigned i = 0; i < callsite->argumentCount && i < nargs; i++) {
        TypeSet *argTypes = callsite->argumentTypes[i];
        TypeSet *types = callee->argTypes(i);
        argTypes->addSubsetBarrier(cx, script, pc, types);
    }

    
    for (unsigned i = callsite->argumentCount; i < nargs; i++) {
        TypeSet *types = callee->argTypes(i);
        types->addType(cx, TYPE_UNDEFINED);
    }

    if (callsite->isNew) {
        
        callee->typeSetNewCalled(cx);

        



        callee->thisTypes()->addSubset(cx, script, callsite->returnTypes);
        callee->returnTypes()->addFilterPrimitives(cx, script, callsite->returnTypes, false);
    } else {
        







        callee->returnTypes()->addSubset(cx, script, callsite->returnTypes);
    }
}

void
TypeConstraintPropagateThis::newType(JSContext *cx, TypeSet *source, jstype type)
{
    



    if (type == TYPE_UNKNOWN || !TypeIsObject(type))
        return;

    TypeObject *object = (TypeObject*) type;
    if (object->unknownProperties() || !object->isFunction)
        return;
    TypeFunction *function = object->asFunction();

    if (!function->script)
        return;

    JSScript *callee = function->script;

    if (!callee->ensureTypeArray(cx))
        return;

    callee->thisTypes()->addType(cx, this->type);
}

void
TypeConstraintArith::newType(JSContext *cx, TypeSet *source, jstype type)
{
    







    if (other) {
        





        if (other->unknown()) {
            target->addType(cx, TYPE_UNKNOWN);
            return;
        }
        switch (type) {
          case TYPE_DOUBLE:
            if (other->hasAnyFlag(TYPE_FLAG_UNDEFINED | TYPE_FLAG_NULL |
                                  TYPE_FLAG_INT32 | TYPE_FLAG_DOUBLE | TYPE_FLAG_BOOLEAN) ||
                other->getObjectCount() != 0) {
                target->addType(cx, TYPE_DOUBLE);
            }
            break;
          case TYPE_STRING:
            target->addType(cx, TYPE_STRING);
            break;
          case TYPE_UNKNOWN:
            target->addType(cx, TYPE_UNKNOWN);
          default:
            if (other->hasAnyFlag(TYPE_FLAG_UNDEFINED | TYPE_FLAG_NULL |
                                  TYPE_FLAG_INT32 | TYPE_FLAG_BOOLEAN) ||
                other->getObjectCount() != 0) {
                target->addType(cx, TYPE_INT32);
            }
            if (other->hasAnyFlag(TYPE_FLAG_DOUBLE))
                target->addType(cx, TYPE_DOUBLE);
            break;
        }
    } else {
        switch (type) {
          case TYPE_DOUBLE:
            target->addType(cx, TYPE_DOUBLE);
            break;
          case TYPE_UNKNOWN:
            target->addType(cx, TYPE_UNKNOWN);
          default:
            target->addType(cx, TYPE_INT32);
            break;
        }
    }
}

void
TypeConstraintTransformThis::newType(JSContext *cx, TypeSet *source, jstype type)
{
    if (type == TYPE_UNKNOWN || TypeIsObject(type) || script->strictModeCode) {
        target->addType(cx, type);
        return;
    }

    



    if (!script->global || type == TYPE_NULL || type == TYPE_UNDEFINED) {
        target->addType(cx, TYPE_UNKNOWN);
        return;
    }

    TypeObject *object = NULL;
    switch (type) {
      case TYPE_INT32:
      case TYPE_DOUBLE:
        object = script->getTypeNewObject(cx, JSProto_Number);
        break;
      case TYPE_BOOLEAN:
        object = script->getTypeNewObject(cx, JSProto_Boolean);
        break;
      case TYPE_STRING:
        object = script->getTypeNewObject(cx, JSProto_String);
        break;
      default:
        JS_NOT_REACHED("Bad type");
    }

    if (!object) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    target->addType(cx, (jstype) object);
}






class TypeConstraintPushAll : public TypeConstraint
{
public:
    const jsbytecode *pc;

    TypeConstraintPushAll(JSScript *script, const jsbytecode *pc)
        : TypeConstraint("pushAll", script), pc(pc)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type)
    {
        cx->compartment->types.dynamicPush(cx, script, pc - script->code, type);
    }
};

void
TypeSet::pushAllTypes(JSContext *cx, JSScript *script, const jsbytecode *pc)
{
    add(cx, ArenaNew<TypeConstraintPushAll>(cx->compartment->pool, script, pc));
}


class TypeConstraintFreeze : public TypeConstraint
{
public:
    
    bool typeAdded;

    TypeConstraintFreeze(JSScript *script)
        : TypeConstraint("freeze", script), typeAdded(false)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type)
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

void
TypeSet::Clone(JSContext *cx, TypeSet *source, ClonedTypeSet *target)
{
    if (!source) {
        target->typeFlags = TYPE_FLAG_UNKNOWN;
        return;
    }

    if (cx->compartment->types.compiledScript && !source->unknown())
        source->addFreeze(cx);

    target->typeFlags = source->baseFlags();
    target->objectCount = source->objectCount;
    if (source->objectCount >= 2) {
        target->objectSet = (TypeObject **) cx->calloc_(sizeof(TypeObject*) * source->objectCount);
        if (!target->objectSet) {
            cx->compartment->types.setPendingNukeTypes(cx);
            target->objectCount = 0;
            return;
        }
        unsigned objectCapacity = HashSetCapacity(source->objectCount);
        unsigned index = 0;
        for (unsigned i = 0; i < objectCapacity; i++) {
            TypeObject *object = source->objectSet[i];
            if (object)
                target->objectSet[index++] = object;
        }
        JS_ASSERT(index == source->objectCount);
    } else if (source->objectCount == 1) {
        target->objectSet = source->objectSet;
    } else {
        target->objectSet = NULL;
    }
}





class TypeConstraintFreezeTypeTag : public TypeConstraint
{
public:
    



    bool typeUnknown;

    TypeConstraintFreezeTypeTag(JSScript *script)
        : TypeConstraint("freezeTypeTag", script), typeUnknown(false)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type)
    {
        if (typeUnknown)
            return;

        if (type != TYPE_UNKNOWN && TypeIsObject(type)) {
            
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
    
    TypeObjectFlags flags;

    
    bool *pmarked;
    bool localMarked;

    TypeConstraintFreezeObjectFlags(JSScript *script, TypeObjectFlags flags, bool *pmarked)
        : TypeConstraint("freezeObjectFlags", script), flags(flags),
          pmarked(pmarked), localMarked(false)
    {}

    TypeConstraintFreezeObjectFlags(JSScript *script, TypeObjectFlags flags)
        : TypeConstraint("freezeObjectFlags", script), flags(flags),
          pmarked(&localMarked), localMarked(false)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type) {}

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
    TypeObjectFlags flags;
    bool marked;

    TypeConstraintFreezeObjectFlagsSet(JSScript *script, TypeObjectFlags flags)
        : TypeConstraint("freezeObjectKindSet", script), flags(flags), marked(false)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type)
    {
        if (marked) {
            
            return;
        }

        if (type == TYPE_UNKNOWN) {
            
        } else if (TypeIsObject(type)) {
            TypeObject *object = (TypeObject *) type;
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
    if (unknown())
        return true;

    



    if (objectCount == 0)
        return true;

    unsigned count = getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        TypeObject *object = getObject(i);
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

static inline void
ObjectStateChange(JSContext *cx, TypeObject *object, bool markingUnknown, bool force)
{
    if (object->unknownProperties())
        return;

    
    TypeSet *elementTypes = object->getProperty(cx, JSID_VOID, false);
    if (!elementTypes)
        return;
    if (markingUnknown) {
        
        object->flags = OBJECT_FLAG_UNKNOWN_MASK;
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
    JS_ASSERT(obj->isGlobal() && !obj->getType()->unknownProperties());
    TypeSet *types = obj->getType()->getProperty(cx, JSID_VOID, false);
    if (!types)
        return;

    




    types->add(cx, ArenaNew<TypeConstraintFreezeObjectFlags>(cx->compartment->pool,
                                                             cx->compartment->types.compiledScript,
                                                             0));
}

class TypeConstraintFreezeOwnProperty : public TypeConstraint
{
public:
    bool updated;
    bool configurable;

    TypeConstraintFreezeOwnProperty(JSScript *script, bool configurable)
        : TypeConstraint("freezeOwnProperty", script),
          updated(false), configurable(configurable)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type) {}

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

JSObject *
TypeSet::getSingleton(JSContext *cx)
{
    if (baseFlags() != 0 || objectCount != 1)
        return NULL;

    TypeObject *object = (TypeObject *) objectSet;
    if (!object->singleton)
        return NULL;

    add(cx, ArenaNew<TypeConstraintFreeze>(cx->compartment->pool,
                                           cx->compartment->types.compiledScript), false);

    return object->singleton;
}





void
TypeCompartment::init(JSContext *cx)
{
    PodZero(this);

    




#ifdef DEBUG
    typeEmpty.name_ = JSID_VOID;
#endif
    typeEmpty.flags = OBJECT_FLAG_UNKNOWN_MASK;

    if (cx && cx->getRunOptions() & JSOPTION_TYPE_INFERENCE)
        inferenceEnabled = true;
}

TypeObject *
TypeCompartment::newTypeObject(JSContext *cx, JSScript *script,
                               const char *name, const char *postfix,
                               bool isFunction, bool isArray, JSObject *proto)
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
    JSAtom *atom = js_Atomize(cx, name, strlen(name), 0);
    if (!atom)
        return NULL;
    jsid id = ATOM_TO_JSID(atom);
#else
    jsid id = JSID_VOID;
#endif

    TypeObject *object = isFunction
        ? cx->new_<TypeFunction>(id, proto)
        : cx->new_<TypeObject>(id, proto);
    if (!object)
        return NULL;

    TypeObject *&objects = script ? script->typeObjects : this->objects;
    object->next = objects;
    objects = object;

    if (!cx->typeInferenceEnabled())
        object->flags = OBJECT_FLAG_UNKNOWN_MASK;
    else if (!isArray)
        object->flags = OBJECT_FLAG_NON_DENSE_ARRAY | OBJECT_FLAG_NON_PACKED_ARRAY;

    if (proto) {
        
        TypeObject *prototype = proto->getType();
        if (prototype->unknownProperties())
            object->flags = OBJECT_FLAG_UNKNOWN_MASK;
        object->instanceNext = prototype->instanceList;
        prototype->instanceList = object;
    }

    return object;
}

TypeObject *
TypeCompartment::newInitializerTypeObject(JSContext *cx, JSScript *script,
                                          uint32 offset, bool isArray)
{
    char *name = NULL;
#ifdef DEBUG
    name = (char *) alloca(40);
    JS_snprintf(name, 40, "#%lu:%lu:%s", script->id(), offset, isArray ? "Array" : "Object");
#endif

    JSObject *proto;
    JSProtoKey key = isArray ? JSProto_Array : JSProto_Object;
    if (!js_GetClassPrototype(cx, script->getGlobal(), key, &proto, NULL))
        return NULL;

    TypeObject *res = newTypeObject(cx, script, name, "", false, isArray, proto);
    if (!res)
        return NULL;

    if (isArray)
        res->initializerArray = true;
    else
        res->initializerObject = true;
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
TypeCompartment::dynamicCall(JSContext *cx, JSObject *callee,
                             const js::CallArgs &args, bool constructing)
{
    unsigned nargs = callee->getFunctionPrivate()->nargs;
    JSScript *script = callee->getFunctionPrivate()->script();

    if (!script->ensureTypeArray(cx))
        return;

    if (constructing) {
        script->typeSetNewCalled(cx);
    } else {
        jstype type = GetValueType(cx, args.thisv());
        script->typeSetThis(cx, type);
    }

    




    unsigned arg = 0;
    for (; arg < args.argc() && arg < nargs; arg++)
        script->typeSetArgument(cx, arg, args[arg]);

    
    for (; arg < nargs; arg++)
        script->typeSetArgument(cx, arg, UndefinedValue());
}


class TypeIntermediatePushed : public TypeIntermediate
{
    uint32 offset;
    jstype type;

  public:
    TypeIntermediatePushed(uint32 offset, jstype type)
        : offset(offset), type(type)
    {}

    void replay(JSContext *cx, JSScript *script)
    {
        TypeSet *pushed = script->analysis(cx)->pushedTypes(offset);
        pushed->addType(cx, type);
    }

    bool hasDynamicResult(uint32 offset, jstype type) {
        return this->offset == offset && this->type == type;
    }

    bool sweep(JSContext *cx, JSCompartment *compartment)
    {
        if (!TypeIsObject(type))
            return true;

        TypeObject *object = (TypeObject *) type;
        if (object->marked)
            return true;

        if (object->unknownProperties()) {
            type = (jstype) &compartment->types.typeEmpty;
            return true;
        }

        return false;
    }
};

void
TypeCompartment::dynamicPush(JSContext *cx, JSScript *script, uint32 offset, jstype type)
{
    JS_ASSERT(cx->typeInferenceEnabled());
    AutoEnterTypeInference enter(cx);

    jsbytecode *pc = script->code + offset;
    UntrapOpcode untrap(cx, script, pc);

    
    if (CanHaveReadBarrier(pc)) {
        TypeSet *types = script->bytecodeTypes(pc);
        if (!types->hasType(type)) {
            InferSpew(ISpewOps, "externalType: monitorResult #%u:%05u: %s",
                      script->id(), offset, TypeString(type));
            types->addType(cx, type);
        }
        return;
    }

    







    JSOp op = JSOp(*pc);
    const JSCodeSpec *cs = &js_CodeSpec[op];
    if (cs->format & (JOF_INC | JOF_DEC)) {
        switch (op) {
          case JSOP_INCGNAME:
          case JSOP_DECGNAME:
          case JSOP_GNAMEINC:
          case JSOP_GNAMEDEC: {
            jsid id = GetAtomId(cx, script, pc, 0);
            TypeObject *global = script->getGlobalType();
            if (!global->unknownProperties()) {
                TypeSet *types = global->getProperty(cx, id, true);
                if (!types)
                    break;
                types->addType(cx, type);
            }
            break;
          }

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
                TypeSet *types = script->slotTypes(slot);
                types->addType(cx, type);
            }
            break;
          }

          default:;
        }
    }

    if (script->hasAnalysis() && script->analysis(cx)->ranInference()) {
        




        TypeSet *pushed = script->analysis(cx)->pushedTypes(offset, 0);
        if (pushed->hasType(type))
            return;
    } else {
        
        TypeIntermediate *result, **presult = &script->intermediateTypes;
        while (*presult) {
            result = *presult;
            if (result->hasDynamicResult(offset, type)) {
                if (presult != &script->intermediateTypes) {
                    
                    *presult = result->next;
                    result->next = script->intermediateTypes;
                    script->intermediateTypes = result;
                }
                return;
            }
            presult = &result->next;
        }
    }

    InferSpew(ISpewOps, "externalType: monitorResult #%u:%05u: %s",
               script->id(), offset, TypeString(type));

    TypeIntermediatePushed *result = cx->new_<TypeIntermediatePushed>(offset, type);
    if (!result) {
        setPendingNukeTypes(cx);
        return;
    }
    script->addIntermediateType(result);

    if (script->hasAnalysis() && script->analysis(cx)->ranInference()) {
        TypeSet *pushed = script->analysis(cx)->pushedTypes(offset, 0);
        pushed->addType(cx, type);
    } else if (script->analyzed) {
        
        ScriptAnalysis *analysis = script->analysis(cx);
        if (!analysis) {
            setPendingNukeTypes(cx);
            return;
        }
        analysis->analyzeTypes(cx);
    }

    
    if (script->fun)
        ObjectStateChange(cx, script->fun->getType(), false, true);
}

void
TypeCompartment::processPendingRecompiles(JSContext *cx)
{
    
    Vector<JSScript*> *pending = pendingRecompiles;
    pendingRecompiles = NULL;

    JS_ASSERT(!pending->empty());

#ifdef JS_METHODJIT

    mjit::ExpandInlineFrames(cx, true);

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

    mjit::ExpandInlineFrames(cx, true);

    
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
}

void
TypeCompartment::monitorBytecode(JSContext *cx, JSScript *script, uint32 offset)
{
    if (script->analysis(cx)->getCode(offset).monitoredTypes)
        return;

    jsbytecode *pc = script->code + offset;
    UntrapOpcode untrap(cx, script, pc);

    




    JSOp op = JSOp(*pc);
    switch (op) {
      case JSOP_SETNAME:
      case JSOP_SETGNAME:
      case JSOP_SETXMLNAME:
      case JSOP_SETCONST:
      case JSOP_SETELEM:
      case JSOP_SETHOLE:
      case JSOP_SETPROP:
      case JSOP_SETMETHOD:
      case JSOP_INITPROP:
      case JSOP_INITMETHOD:
      case JSOP_FORPROP:
      case JSOP_FORNAME:
      case JSOP_FORGNAME:
      case JSOP_ENUMELEM:
      case JSOP_ENUMCONSTELEM:
      case JSOP_DEFFUN:
      case JSOP_DEFFUN_FC:
      case JSOP_ARRAYPUSH:
        break;
      case JSOP_INCNAME:
      case JSOP_DECNAME:
      case JSOP_NAMEINC:
      case JSOP_NAMEDEC:
      case JSOP_INCGNAME:
      case JSOP_DECGNAME:
      case JSOP_GNAMEINC:
      case JSOP_GNAMEDEC:
      case JSOP_INCELEM:
      case JSOP_DECELEM:
      case JSOP_ELEMINC:
      case JSOP_ELEMDEC:
      case JSOP_INCPROP:
      case JSOP_DECPROP:
      case JSOP_PROPINC:
      case JSOP_PROPDEC:
      case JSOP_CALL:
      case JSOP_EVAL:
      case JSOP_FUNCALL:
      case JSOP_FUNAPPLY:
      case JSOP_NEW:
        script->analysis(cx)->addPushedType(cx, offset, 0, TYPE_UNKNOWN);
        break;
      default:
        TypeFailure(cx, "Monitoring unknown bytecode at #%u:%05u", script->id(), offset);
    }

    InferSpew(ISpewOps, "addMonitorNeeded: #%u:%05u", script->id(), offset);

    script->analysis(cx)->getCode(offset).monitoredTypes = true;

    if (script->hasJITCode())
        cx->compartment->types.addPendingRecompile(cx, script);

    
    if (script->fun)
        ObjectStateChange(cx, script->fun->getType(), false, true);
}

void
ScriptAnalysis::addTypeBarrier(JSContext *cx, const jsbytecode *pc, TypeSet *target, jstype type)
{
    Bytecode &code = getCode(pc);

    if (TypeIsObject(type) && target->getObjectCount() >= BARRIER_OBJECT_LIMIT) {
        
        target->addType(cx, type);
        return;
    }

    if (!code.typeBarriers) {
        





        cx->compartment->types.addPendingRecompile(cx, script);

        
        if (script->fun)
            ObjectStateChange(cx, script->fun->getType(), false, true);
    }

    InferSpew(ISpewOps, "typeBarrier: #%u:%05u: T%p %s",
              script->id(), pc - script->code, target, TypeString(type));

    TypeBarrier *barrier = ArenaNew<TypeBarrier>(cx->compartment->pool);
    barrier->target = target;
    barrier->type = type;

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
        TypeObject *object = script->typeObjects;
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
NumberTypes(jstype a, jstype b)
{
    return (a == TYPE_INT32 || a == TYPE_DOUBLE) && (b == TYPE_INT32 || b == TYPE_DOUBLE);
}

struct types::ArrayTableKey
{
    jstype type;
    JSObject *proto;

    typedef ArrayTableKey Lookup;

    static inline uint32 hash(const ArrayTableKey &v) {
        return (uint32) (v.type ^ ((uint32)(size_t)v.proto >> 2));
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

    jstype type = GetValueType(cx, obj->getDenseArrayElement(0));

    for (unsigned i = 1; i < len; i++) {
        jstype ntype = GetValueType(cx, obj->getDenseArrayElement(i));
        if (ntype != type) {
            if (NumberTypes(type, ntype))
                type = TYPE_DOUBLE;
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

        TypeObject *objType = newTypeObject(cx, NULL, name, "", false, true, obj->getProto());
        if (!objType) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
        obj->setType(objType);

        cx->addTypePropertyId(objType, JSID_VOID, type);

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
    jstype *types;
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
        
        jstype *types = p->value.types;
        for (unsigned i = 0; i < obj->slotSpan(); i++) {
            jstype ntype = GetValueType(cx, obj->getSlot(i));
            if (ntype != types[i]) {
                if (NumberTypes(ntype, types[i])) {
                    if (types[i] == TYPE_INT32) {
                        types[i] = TYPE_DOUBLE;
                        const Shape *shape = baseShape;
                        while (!JSID_IS_EMPTY(shape->propid)) {
                            if (shape->slot == i) {
                                cx->addTypePropertyId(p->value.object, shape->propid, TYPE_DOUBLE);
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

        TypeObject *objType = newTypeObject(cx, NULL, name, "", false, false, obj->getProto());
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

        jstype *types = (jstype *) cx->calloc_(obj->slotSpan() * sizeof(jstype));
        if (!types) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }

        const Shape *shape = baseShape;
        while (!JSID_IS_EMPTY(shape->propid)) {
            ids[shape->slot] = shape->propid;
            types[shape->slot] = GetValueType(cx, obj->getSlot(shape->slot));
            cx->addTypePropertyId(objType, shape->propid, types[shape->slot]);
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
         TypeObject *object = obj->getType();
         Property *p =
             HashSetLookup<jsid,Property,Property>(object->propertySet, object->propertyCount, base->id);
         if (p)
             p->types.addBaseSubset(cx, this, &base->types);
         obj = obj->getProto();
     }
}

void
TypeObject::splicePrototype(JSContext *cx, JSObject *proto)
{
    





    JS_ASSERT(singleton);

    if (this->proto) {
        
        TypeObject **plist = &this->proto->getType()->instanceList;
        while (*plist != this)
            plist = &(*plist)->instanceNext;
        *plist = this->instanceNext;
    }

    this->proto = proto;

    
    this->instanceNext = proto->getType()->instanceList;
    proto->getType()->instanceList = this;

    if (!cx->typeInferenceEnabled())
        return;

    AutoEnterTypeInference enter(cx);

    if (proto->getType()->unknownProperties()) {
        markUnknown(cx);
        return;
    }

    



    unsigned count = getPropertyCount();
    for (unsigned i = 0; i < count; i++) {
        Property *prop = getProperty(i);
        if (prop && !JSID_IS_EMPTY(prop->id))
            getFromPrototypes(cx, prop);
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

    InferSpew(ISpewOps, "typeSet: T%p property %s %s",
              &base->types, name(), TypeIdString(id));

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

void
TypeObject::setFlags(JSContext *cx, TypeObjectFlags flags)
{
    JS_ASSERT(cx->compartment->activeInference);
    JS_ASSERT((this->flags & flags) != flags);

    this->flags |= flags;

    InferSpew(ISpewOps, "%s: setFlags %u", name(), flags);

    ObjectStateChange(cx, this, false, false);
}

void
TypeObject::markUnknown(JSContext *cx)
{
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
            prop->types.addType(cx, TYPE_UNKNOWN);
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

#ifdef JS_METHODJIT
    mjit::ExpandInlineFrames(cx, true);
#endif

    







    for (AllFramesIter iter(cx); !iter.done(); ++iter) {
        StackFrame *fp = iter.fp();
        if (fp->isScriptFrame() && fp->isConstructing() &&
            fp->script() == newScript->script && fp->thisValue().isObject() &&
            fp->thisValue().toObject().type == this) {
            JSObject *obj = &fp->thisValue().toObject();
            JSInlinedSite *inline_;
            jsbytecode *pc = fp->pc(cx, NULL, &inline_);
            JS_ASSERT(!inline_);

            
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
                        if (seg.currentFrame() == fp)
                            break;
                        fp = seg.computeNextFrame(fp);
                        pc = fp->pc(cx, NULL, &inline_);
                        JS_ASSERT(!inline_);
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
                obj->rollbackProperties(numProperties);
        }
    }

    cx->free_(newScript);
    newScript = NULL;
}

void
TypeObject::print(JSContext *cx)
{
    printf("%s : %s", name(), proto ? proto->getType()->name() : "(null)");

    if (unknownProperties()) {
        printf(" unknown");
    } else {
        if (!hasAnyFlags(OBJECT_FLAG_NON_PACKED_ARRAY))
            printf(" packed");
        if (!hasAnyFlags(OBJECT_FLAG_NON_DENSE_ARRAY))
            printf(" dense");
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
    if (!script->global)
        return NULL;

    UntrapOpcode untrap(cx, script, pc);

    JSOp op = JSOp(*pc);
    JS_ASSERT(op == JSOP_NEWARRAY || op == JSOP_NEWOBJECT || op == JSOP_NEWINIT);

    bool isArray = (op == JSOP_NEWARRAY || (op == JSOP_NEWINIT && pc[1] == JSProto_Array));
    return script->getTypeInitObject(cx, pc, isArray);
}

inline void
ScriptAnalysis::setForTypes(JSContext *cx, jsbytecode *pc, TypeSet *types)
{
    
    const SSAValue &iterv = poppedValue(pc, 0);
    jsbytecode *iterpc = script->code + iterv.pushedOffset();
    JS_ASSERT(JSOp(*iterpc) == JSOP_ITER || JSOp(*iterpc) == JSOP_TRAP);

    uintN flags = iterpc[1];
    if (flags & JSITER_FOREACH) {
        types->addType(cx, TYPE_UNKNOWN);
        return;
    }

    




    types->addType(cx, TYPE_STRING);

    pushedTypes(iterpc, 0)->add(cx,
        ArenaNew<TypeConstraintGenerator>(cx->compartment->pool, script, types));
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
            InferSpew(ISpewOps, "typeSet: T%p phi #%u:%05u:%u", &types,
                      script->id(), offset, newv->slot);
            newv++;
        }
    }

    for (unsigned i = 0; i < defCount; i++) {
        pushed[i].setIntermediate();
        InferSpew(ISpewOps, "typeSet: T%p pushed%u #%u:%05u", &pushed[i], i, script->id(), offset);
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
        pushed[0].addType(cx, TYPE_UNDEFINED);
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
        
        pushed[0].addType(cx, TYPE_INT32);
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
        pushed[0].addType(cx, TYPE_BOOLEAN);
        break;
      case JSOP_DOUBLE:
        pushed[0].addType(cx, TYPE_DOUBLE);
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
        pushed[0].addType(cx, TYPE_STRING);
        break;
      case JSOP_NULL:
        pushed[0].addType(cx, TYPE_NULL);
        break;

      case JSOP_REGEXP:
        if (script->global) {
            TypeObject *object = script->getTypeNewObject(cx, JSProto_RegExp);
            if (!object)
                return false;
            pushed[0].addType(cx, (jstype) object);
        } else {
            pushed[0].addType(cx, TYPE_UNKNOWN);
        }
        break;

      case JSOP_OBJECT: {
        JSObject *obj = GetScriptObject(cx, script, pc, 0);
        pushed[0].addType(cx, (jstype) obj->getType());
        break;
      }

      case JSOP_STOP:
        
        if (script->fun)
            script->returnTypes()->addType(cx, TYPE_UNDEFINED);
        break;

      case JSOP_OR:
      case JSOP_ORX:
      case JSOP_AND:
      case JSOP_ANDX:
        
        poppedTypes(pc, 0)->addSubset(cx, script, &pushed[0]);
        break;

      case JSOP_DUP:
        poppedTypes(pc, 0)->addSubset(cx, script, &pushed[0]);
        poppedTypes(pc, 0)->addSubset(cx, script, &pushed[1]);
        break;

      case JSOP_DUP2:
        poppedTypes(pc, 1)->addSubset(cx, script, &pushed[0]);
        poppedTypes(pc, 0)->addSubset(cx, script, &pushed[1]);
        poppedTypes(pc, 1)->addSubset(cx, script, &pushed[2]);
        poppedTypes(pc, 0)->addSubset(cx, script, &pushed[3]);
        break;

      case JSOP_GETGLOBAL:
      case JSOP_CALLGLOBAL:
      case JSOP_GETGNAME:
      case JSOP_CALLGNAME: {
        jsid id;
        if (op == JSOP_GETGLOBAL || op == JSOP_CALLGLOBAL)
            id = GetGlobalId(cx, script, pc);
        else
            id = GetAtomId(cx, script, pc, 0);

        TypeSet *seen = script->bytecodeTypes(pc);
        seen->addSubset(cx, script, &pushed[0]);

        




        if (id == ATOM_TO_JSID(cx->runtime->atomState.typeAtoms[JSTYPE_VOID]))
            seen->addType(cx, TYPE_UNDEFINED);
        if (id == ATOM_TO_JSID(cx->runtime->atomState.NaNAtom))
            seen->addType(cx, TYPE_DOUBLE);
        if (id == ATOM_TO_JSID(cx->runtime->atomState.InfinityAtom))
            seen->addType(cx, TYPE_DOUBLE);

        
        PropertyAccess(cx, script, pc, script->getGlobalType(), false, seen, id);

        if (op == JSOP_CALLGLOBAL || op == JSOP_CALLGNAME) {
            pushed[1].addType(cx, TYPE_UNKNOWN);
            pushed[0].addPropagateThis(cx, script, pc, TYPE_UNKNOWN);
        }

        if (CheckNextTest(pc))
            pushed[0].addType(cx, TYPE_UNDEFINED);

        



        if (cx->compartment->debugMode)
            seen->addType(cx, TYPE_UNKNOWN);
        break;
      }

      case JSOP_INCGNAME:
      case JSOP_DECGNAME:
      case JSOP_GNAMEINC:
      case JSOP_GNAMEDEC: {
        jsid id = GetAtomId(cx, script, pc, 0);
        PropertyAccess(cx, script, pc, script->getGlobalType(), true, NULL, id);
        PropertyAccess(cx, script, pc, script->getGlobalType(), false, &pushed[0], id);

        if (cx->compartment->debugMode)
            pushed[0].addType(cx, TYPE_UNKNOWN);
        break;
      }

      case JSOP_NAME:
      case JSOP_CALLNAME: {
        



        TypeSet *seen = script->bytecodeTypes(pc);
        seen->addSubset(cx, script, &pushed[0]);
        if (op == JSOP_CALLNAME) {
            pushed[1].addType(cx, TYPE_UNKNOWN);
            pushed[0].addPropagateThis(cx, script, pc, TYPE_UNKNOWN);
        }
        break;
      }

      case JSOP_BINDGNAME:
      case JSOP_BINDNAME:
        break;

      case JSOP_SETGNAME: {
        jsid id = GetAtomId(cx, script, pc, 0);
        PropertyAccess(cx, script, pc, script->getGlobalType(),
                       true, poppedTypes(pc, 0), id);
        poppedTypes(pc, 0)->addSubset(cx, script, &pushed[0]);
        break;
      }

      case JSOP_SETNAME:
      case JSOP_SETCONST:
        cx->compartment->types.monitorBytecode(cx, script, offset);
        poppedTypes(pc, 0)->addSubset(cx, script, &pushed[0]);
        break;

      case JSOP_INCNAME:
      case JSOP_DECNAME:
      case JSOP_NAMEINC:
      case JSOP_NAMEDEC:
        cx->compartment->types.monitorBytecode(cx, script, offset);
        break;

      case JSOP_GETXPROP: {
        TypeSet *seen = script->bytecodeTypes(pc);
        seen->addSubset(cx, script, &pushed[0]);
        break;
      }

      case JSOP_GETFCSLOT:
      case JSOP_CALLFCSLOT: {
        unsigned index = GET_UINT16(pc);
        TypeSet *types = script->upvarTypes(index);
        types->addSubset(cx, script, &pushed[0]);
        if (op == JSOP_CALLFCSLOT) {
            pushed[1].addType(cx, TYPE_UNDEFINED);
            pushed[0].addPropagateThis(cx, script, pc, TYPE_UNDEFINED);
        }
        break;
      }

      case JSOP_GETUPVAR_DBG:
      case JSOP_CALLUPVAR_DBG:
        pushed[0].addType(cx, TYPE_UNKNOWN);
        if (op == JSOP_CALLUPVAR_DBG) {
            pushed[1].addType(cx, TYPE_UNDEFINED);
            pushed[0].addPropagateThis(cx, script, pc, TYPE_UNDEFINED);
        }
        break;

      case JSOP_GETARG:
      case JSOP_CALLARG:
      case JSOP_GETLOCAL:
      case JSOP_CALLLOCAL: {
        uint32 slot = GetBytecodeSlot(script, pc);
        if (trackSlot(slot)) {
            




            poppedTypes(pc, 0)->addSubset(cx, script, &pushed[0]);
        } else if (slot < TotalSlots(script)) {
            TypeSet *types = script->slotTypes(slot);
            types->addSubset(cx, script, &pushed[0]);
        } else {
            
            pushed[0].addType(cx, TYPE_UNKNOWN);
        }
        if (op == JSOP_CALLARG || op == JSOP_CALLLOCAL) {
            pushed[1].addType(cx, TYPE_UNDEFINED);
            pushed[0].addPropagateThis(cx, script, pc, TYPE_UNDEFINED);
        }
        break;
      }

      case JSOP_SETARG:
      case JSOP_SETLOCAL:
      case JSOP_SETLOCALPOP: {
        uint32 slot = GetBytecodeSlot(script, pc);
        if (!trackSlot(slot) && slot < TotalSlots(script)) {
            TypeSet *types = script->slotTypes(slot);
            poppedTypes(pc, 0)->addSubset(cx, script, types);
        }

        




        poppedTypes(pc, 0)->addSubset(cx, script, &pushed[0]);
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
            poppedTypes(pc, 0)->addArith(cx, script, &pushed[0]);
        } else if (slot < TotalSlots(script)) {
            TypeSet *types = script->slotTypes(slot);
            types->addArith(cx, script, types);
            types->addSubset(cx, script, &pushed[0]);
        } else {
            pushed[0].addType(cx, TYPE_UNKNOWN);
        }
        break;
      }

      case JSOP_ARGSUB:
        pushed[0].addType(cx, TYPE_UNKNOWN);
        break;

      case JSOP_ARGUMENTS:
      case JSOP_ARGCNT:
        pushed[0].addType(cx, TYPE_UNKNOWN);
        break;

      case JSOP_SETPROP:
      case JSOP_SETMETHOD: {
        jsid id = GetAtomId(cx, script, pc, 0);
        poppedTypes(pc, 1)->addSetProperty(cx, script, pc, poppedTypes(pc, 0), id);
        poppedTypes(pc, 0)->addSubset(cx, script, &pushed[0]);
        break;
      }

      case JSOP_LENGTH:
      case JSOP_GETPROP:
      case JSOP_CALLPROP: {
        jsid id = GetAtomId(cx, script, pc, 0);
        TypeSet *seen = script->bytecodeTypes(pc);

        




        poppedTypes(pc, 0)->addGetProperty(cx, script, pc, seen, id);

        seen->addSubset(cx, script, &pushed[0]);
        if (op == JSOP_CALLPROP)
            poppedTypes(pc, 0)->addFilterPrimitives(cx, script, &pushed[1], true);
        if (CheckNextTest(pc))
            pushed[0].addType(cx, TYPE_UNDEFINED);
        break;
      }

      case JSOP_INCPROP:
      case JSOP_DECPROP:
      case JSOP_PROPINC:
      case JSOP_PROPDEC: {
        jsid id = GetAtomId(cx, script, pc, 0);
        poppedTypes(pc, 0)->addGetProperty(cx, script, pc, &pushed[0], id);
        break;
      }

      




      case JSOP_GETELEM:
      case JSOP_CALLELEM: {
        TypeSet *seen = script->bytecodeTypes(pc);

        
        poppedTypes(pc, 1)->addGetProperty(cx, script, pc, seen, JSID_VOID);

        seen->addSubset(cx, script, &pushed[0]);
        if (op == JSOP_CALLELEM)
            poppedTypes(pc, 1)->addFilterPrimitives(cx, script, &pushed[1], true);
        if (CheckNextTest(pc))
            pushed[0].addType(cx, TYPE_UNDEFINED);
        break;
      }

      case JSOP_INCELEM:
      case JSOP_DECELEM:
      case JSOP_ELEMINC:
      case JSOP_ELEMDEC:
        poppedTypes(pc, 1)->addGetProperty(cx, script, pc, &pushed[0], JSID_VOID);
        break;

      case JSOP_SETELEM:
      case JSOP_SETHOLE:
        poppedTypes(pc, 2)->addSetProperty(cx, script, pc, poppedTypes(pc, 0), JSID_VOID);
        poppedTypes(pc, 0)->addSubset(cx, script, &pushed[0]);
        break;

      case JSOP_THIS:
        script->thisTypes()->addTransformThis(cx, script, &pushed[0]);
        break;

      case JSOP_RETURN:
      case JSOP_SETRVAL:
        if (script->fun)
            poppedTypes(pc, 0)->addSubset(cx, script, script->returnTypes());
        break;

      case JSOP_ADD:
        poppedTypes(pc, 0)->addArith(cx, script, &pushed[0], poppedTypes(pc, 1));
        poppedTypes(pc, 1)->addArith(cx, script, &pushed[0], poppedTypes(pc, 0));
        break;

      case JSOP_SUB:
      case JSOP_MUL:
      case JSOP_MOD:
      case JSOP_DIV:
        poppedTypes(pc, 0)->addArith(cx, script, &pushed[0]);
        poppedTypes(pc, 1)->addArith(cx, script, &pushed[0]);
        break;

      case JSOP_NEG:
      case JSOP_POS:
        poppedTypes(pc, 0)->addArith(cx, script, &pushed[0]);
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
                res = script->slotTypes(slot);
            }
        }

        if (res) {
            if (script->global)
                res->addType(cx, (jstype) obj->getType());
            else
                res->addType(cx, TYPE_UNKNOWN);
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
        
        unsigned argCount = GetUseCount(script, offset) - 2;
        TypeCallsite *callsite = ArenaNew<TypeCallsite>(cx->compartment->pool,
                                                        cx, script, pc, op == JSOP_NEW, argCount);
        if (!callsite || (argCount && !callsite->argumentTypes)) {
            cx->compartment->types.setPendingNukeTypes(cx);
            break;
        }
        callsite->thisTypes = poppedTypes(pc, argCount);
        callsite->returnTypes = &pushed[0];

        for (unsigned i = 0; i < argCount; i++)
            callsite->argumentTypes[i] = poppedTypes(pc, argCount - 1 - i);

        poppedTypes(pc, argCount + 1)->addCall(cx, callsite);
        break;
      }

      case JSOP_NEWINIT:
      case JSOP_NEWARRAY:
      case JSOP_NEWOBJECT: {
        TypeObject *initializer = GetInitializerType(cx, script, pc);
        if (script->global) {
            if (!initializer)
                return false;
            pushed[0].addType(cx, (jstype) initializer);
        } else {
            JS_ASSERT(!initializer);
            pushed[0].addType(cx, TYPE_UNKNOWN);
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
            pushed[0].addType(cx, (jstype) initializer);
            if (!initializer->unknownProperties()) {
                




                TypeSet *types = initializer->getProperty(cx, JSID_VOID, true);
                if (!types)
                    return false;
                if (state.hasGetSet)
                    types->addType(cx, TYPE_UNKNOWN);
                else if (state.hasHole)
                    cx->markTypeObjectFlags(initializer, js::types::OBJECT_FLAG_NON_PACKED_ARRAY);
                else
                    poppedTypes(pc, 0)->addSubset(cx, script, types);
            }
        } else {
            pushed[0].addType(cx, TYPE_UNKNOWN);
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
            pushed[0].addType(cx, (jstype) initializer);
            if (!initializer->unknownProperties()) {
                jsid id = GetAtomId(cx, script, pc, 0);
                TypeSet *types = initializer->getProperty(cx, id, true);
                if (!types)
                    return false;
                if (id == id___proto__(cx) || id == id_prototype(cx))
                    cx->compartment->types.monitorBytecode(cx, script, offset);
                else if (state.hasGetSet)
                    types->addType(cx, TYPE_UNKNOWN);
                else
                    poppedTypes(pc, 0)->addSubset(cx, script, types);
            }
        } else {
            pushed[0].addType(cx, TYPE_UNKNOWN);
        }
        state.hasGetSet = false;
        JS_ASSERT(!state.hasHole);
        break;
      }

      case JSOP_ENTERWITH:
      case JSOP_ENTERBLOCK:
        




        break;

      case JSOP_ITER:
        





        poppedTypes(pc, 0)->addSubset(cx, script, &pushed[0]);
        break;

      case JSOP_MOREITER:
        poppedTypes(pc, 0)->addSubset(cx, script, &pushed[0]);
        pushed[1].addType(cx, TYPE_BOOLEAN);
        break;

      case JSOP_FORGNAME: {
        jsid id = GetAtomId(cx, script, pc, 0);
        TypeObject *global = script->getGlobalType();
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
                setForTypes(cx, pc, script->slotTypes(slot));
        }
        break;
      }

      case JSOP_FORELEM:
        poppedTypes(pc, 0)->addSubset(cx, script, &pushed[0]);
        pushed[1].addType(cx, TYPE_UNKNOWN);
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
        pushed[0].addType(cx, TYPE_UNKNOWN);
        break;

      case JSOP_DELPROP:
      case JSOP_DELELEM:
      case JSOP_DELNAME:
        
        pushed[0].addType(cx, TYPE_BOOLEAN);
        break;

      case JSOP_LEAVEBLOCKEXPR:
        poppedTypes(pc, 0)->addSubset(cx, script, &pushed[0]);
        break;

      case JSOP_CASE:
      case JSOP_CASEX:
        poppedTypes(pc, 1)->addSubset(cx, script, &pushed[0]);
        break;

      case JSOP_UNBRAND:
        poppedTypes(pc, 0)->addSubset(cx, script, &pushed[0]);
        break;

      case JSOP_GENERATOR:
        if (script->fun) {
            if (script->global) {
                TypeObject *object = script->getTypeNewObject(cx, JSProto_Generator);
                if (!object)
                    return false;
                script->returnTypes()->addType(cx, (jstype) object);
            } else {
                script->returnTypes()->addType(cx, TYPE_UNKNOWN);
            }
        }
        break;

      case JSOP_YIELD:
        pushed[0].addType(cx, TYPE_UNKNOWN);
        break;

      case JSOP_CALLXMLNAME:
        pushed[1].addType(cx, TYPE_UNKNOWN);
        
      case JSOP_XMLNAME:
        pushed[0].addType(cx, TYPE_UNKNOWN);
        break;

      case JSOP_SETXMLNAME:
        cx->compartment->types.monitorBytecode(cx, script, offset);
        poppedTypes(pc, 0)->addSubset(cx, script, &pushed[0]);
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
        pushed[0].addType(cx, TYPE_UNKNOWN);
        break;

      case JSOP_FILTER:
        
        poppedTypes(pc, 0)->addSubset(cx, script, &pushed[0]);
        break;

      case JSOP_ENDFILTER:
        poppedTypes(pc, 1)->addSubset(cx, script, &pushed[0]);
        break;

      case JSOP_DEFSHARP:
        break;

      case JSOP_USESHARP:
        pushed[0].addType(cx, TYPE_UNKNOWN);
        break;

      case JSOP_CALLEE:
        if (script->global)
            pushed[0].addType(cx, (jstype) script->fun->getType());
        else
            pushed[0].addType(cx, TYPE_UNKNOWN);
        break;

      default:
        TypeFailure(cx, "Unknown bytecode at #%u:%05u", script->id(), offset);
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

    







    if (script->global && script->global->isCleared())
        return;

    if (!ranSSA()) {
        analyzeSSA(cx);
        if (failed())
            return;
    }

    if (!script->ensureTypeArray(cx)) {
        setOOM(cx);
        return;
    }

    if (script->analyzed) {
        




        cx->compartment->types.addPendingRecompile(cx, script);
    }

    
    script->analyzed = true;

    



    ranInference_ = true;

    if (script->calledWithNew)
        analyzeTypesNew(cx);

    
    for (unsigned i = 0; i < script->nfixed; i++)
        script->localTypes(i)->addType(cx, TYPE_UNDEFINED);

    TypeInferenceState state(cx);

    unsigned offset = 0;
    while (offset < script->length) {
        Bytecode *code = maybeCode(offset);

        jsbytecode *pc = script->code + offset;
        UntrapOpcode untrap(cx, script, pc);

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
            getValueTypes(v)->addSubset(cx, script, &node->types);
        }
    }

    




    TypeIntermediate *result = script->intermediateTypes;
    while (result) {
        result->replay(cx, script);
        result = result->next;
    }
}

void
ScriptAnalysis::analyzeTypesNew(JSContext *cx)
{
    JS_ASSERT(script->calledWithNew && script->fun);

    




    if (script->fun->getType()->unknownProperties() ||
        script->fun->isFunctionPrototype() ||
        !script->global) {
        script->thisTypes()->addType(cx, TYPE_UNKNOWN);
        return;
    }

    TypeFunction *funType = script->fun->getType()->asFunction();
    TypeSet *prototypeTypes = funType->getProperty(cx, id_prototype(cx), false);
    if (!prototypeTypes)
        return;
    prototypeTypes->addNewObject(cx, script, funType, script->thisTypes());
}





class TypeConstraintClearDefiniteSetter : public TypeConstraint
{
public:
    TypeObject *object;

    TypeConstraintClearDefiniteSetter(TypeObject *object)
        : TypeConstraint("baseClearDefinite", (JSScript *) 0x1), object(object)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type) {
        if (!object->newScript)
            return;
        




        if (!object->newScriptCleared && type == TYPE_UNKNOWN)
            object->clearNewScript(cx);
    }

    TypeObject * persistentObject() { return object; }
};





class TypeConstraintClearDefiniteSingle : public TypeConstraint
{
public:
    TypeObject *object;

    TypeConstraintClearDefiniteSingle(JSScript *script, TypeObject *object)
        : TypeConstraint("baseClearDefinite", script), object(object)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type) {
        if (!object->newScriptCleared && !source->getSingleObject())
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
        pushed->add(cx, ArenaNew<TypeConstraintClearDefiniteSingle>(cx->compartment->pool, script, object));
    }

    bool sweep(JSContext *cx, JSCompartment *compartment)
    {
        return object->marked;
    }
};

static bool
AnalyzeNewScriptProperties(JSContext *cx, TypeObject *type, JSScript *script, JSObject **pbaseobj,
                           Vector<TypeNewScript::Initializer> *initializerList)
{
    









    if (initializerList->length() > 50) {
        



        return false;
    }

    ScriptAnalysis *analysis = script->analysis(cx);
    if (analysis && !analysis->ranInference())
        analysis->analyzeTypes(cx);
    if (!analysis || analysis->OOM()) {
        *pbaseobj = NULL;
        cx->compartment->types.setPendingNukeTypes(cx);
        return false;
    }

    







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
            jsid id = GetAtomId(cx, script, pc, 0);
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

            




            TypeSet *parentTypes = type->proto->getType()->getProperty(cx, id, false);
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

            
            TypeObject *funcallObj = funcallTypes->getSingleObject();
            if (!funcallObj || !funcallObj->singleton ||
                !funcallObj->singleton->isFunction() ||
                funcallObj->singleton->getFunctionPrivate()->maybeNative() != js_fun_call) {
                return false;
            }
            TypeObject *scriptObj = scriptTypes->getSingleObject();
            if (!scriptObj || !scriptObj->isFunction || !scriptObj->asFunction()->script)
                return false;

            



            TypeIntermediateClearDefinite *funcallTrap =
                cx->new_<TypeIntermediateClearDefinite>(calleev.pushedOffset(), 0, type);
            TypeIntermediateClearDefinite *calleeTrap =
                cx->new_<TypeIntermediateClearDefinite>(calleev.pushedOffset(), 1, type);
            if (!funcallTrap || !calleeTrap) {
                cx->compartment->types.setPendingNukeTypes(cx);
                *pbaseobj = NULL;
                return false;
            }
            script->addIntermediateType(funcallTrap);
            script->addIntermediateType(calleeTrap);
            funcallTrap->replay(cx, script);
            calleeTrap->replay(cx, script);

            TypeNewScript::Initializer pushframe(TypeNewScript::Initializer::FRAME_PUSH, uses->offset);
            if (!initializerList->append(pushframe)) {
                cx->compartment->types.setPendingNukeTypes(cx);
                *pbaseobj = NULL;
                return false;
            }

            if (!AnalyzeNewScriptProperties(cx, type, scriptObj->asFunction()->script,
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
ScriptAnalysis::printTypes(JSContext *cx)
{
    AutoEnterAnalysis enter(cx);
    TypeCompartment *compartment = &script->compartment->types;

    



    for (unsigned offset = 0; offset < script->length; offset++) {
        if (!maybeCode(offset))
            continue;

        jsbytecode *pc = script->code + offset;
        UntrapOpcode untrap(cx, script, pc);

        unsigned defCount = GetDefCount(script, offset);
        if (!defCount)
            continue;

        for (unsigned i = 0; i < defCount; i++) {
            TypeSet *types = pushedTypes(offset, i);

            if (types->unknown()) {
                compartment->typeCountOver++;
                continue;
            }

            unsigned typeCount = types->getObjectCount() ? 1 : 0;
            for (jstype type = TYPE_UNDEFINED; type <= TYPE_STRING; type++) {
                if (types->hasAnyFlag(1 << type))
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
    script->returnTypes()->print(cx);
    printf("\n    this:");
    script->thisTypes()->print(cx);

    for (unsigned i = 0; script->fun && i < script->fun->nargs; i++) {
        printf("\n    arg%u:", i);
        script->argTypes(i)->print(cx);
    }
    for (unsigned i = 0; i < script->nfixed; i++) {
        if (!trackSlot(LocalSlot(script, i))) {
            printf("\n    local%u:", i);
            script->localTypes(i)->print(cx);
        }
    }
    for (unsigned i = 0; i < script->bindings.countUpvars(); i++) {
        printf("\n    upvar%u:", i);
        script->upvarTypes(i)->print(cx);
    }
    printf("\n");

    for (unsigned offset = 0; offset < script->length; offset++) {
        if (!maybeCode(offset))
            continue;

        jsbytecode *pc = script->code + offset;
        UntrapOpcode untrap(cx, script, pc);

        PrintBytecode(cx, script, pc);

        if (js_CodeSpec[*pc].format & JOF_TYPESET) {
            TypeSet *types = script->bytecodeTypes(pc);
            printf("  typeset %d:", (int) (types - script->typeArray));
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
JSScript::makeTypeArray(JSContext *cx)
{
    JS_ASSERT(!typeArray);

    AutoEnterTypeInference enter(cx);

    unsigned count = nTypeSets + TotalSlots(this) + bindings.countUpvars();
    typeArray = (TypeSet *) cx->calloc_(sizeof(TypeSet) * count);
    if (!typeArray) {
        compartment->types.setPendingNukeTypes(cx);
        return false;
    }

#ifdef DEBUG
    for (unsigned i = 0; i < nTypeSets; i++)
        InferSpew(ISpewOps, "typeSet: T%p bytecode%u #%u", &typeArray[i], i, id());
    InferSpew(ISpewOps, "typeSet: T%p return #%u", returnTypes(), id());
    InferSpew(ISpewOps, "typeSet: T%p this #%u", thisTypes(), id());
    unsigned nargs = fun ? fun->nargs : 0;
    for (unsigned i = 0; i < nargs; i++)
        InferSpew(ISpewOps, "typeSet: T%p arg%u #%u", argTypes(i), i, id());
    for (unsigned i = 0; i < nfixed; i++)
        InferSpew(ISpewOps, "typeSet: T%p local%u #%u", localTypes(i), i, id());
    for (unsigned i = 0; i < bindings.countUpvars(); i++)
        InferSpew(ISpewOps, "typeSet: T%p upvar%u #%u", upvarTypes(i), i, id());
#endif

    return true;
}

bool
JSScript::typeSetFunction(JSContext *cx, JSFunction *fun)
{
    this->fun = fun;

    if (!cx->typeInferenceEnabled())
        return true;

    char *name = NULL;
#ifdef DEBUG
    name = (char *) alloca(10);
    JS_snprintf(name, 10, "#%u", id());
#endif

    TypeObject *type = cx->compartment->types.newTypeObject(cx, this, name, "",
                                                            true, false, fun->getProto());
    if (!type)
        return false;

    if (!fun->setTypeAndUniqueShape(cx, type))
        return false;
    type->asFunction()->script = this;
    this->fun = fun;

    return true;
}

#ifdef DEBUG

void
JSScript::typeCheckBytecode(JSContext *cx, const jsbytecode *pc, const js::Value *sp)
{
    AutoEnterTypeInference enter(cx);

    if (!(analysis_ && analysis_->ranInference()))
        return;

    int defCount = GetDefCount(this, pc - code);

    for (int i = 0; i < defCount; i++) {
        const js::Value &val = sp[-defCount + i];
        TypeSet *types = analysis_->pushedTypes(pc, i);
        if (IgnorePushed(pc, i))
            continue;

        jstype type = GetValueType(cx, val);

        if (!TypeSetMatches(cx, types, type)) {
            TypeFailure(cx, "Missing type at #%u:%05u pushed %u: %s",
                                   id(), pc - code, i, TypeString(type));
        }

        if (TypeIsObject(type)) {
            JS_ASSERT(val.isObject());
            JSObject *obj = &val.toObject();
            TypeObject *object = (TypeObject *) type;

            if (object->unknownProperties())
                continue;

            
            bool dense = !object->hasAnyFlags(OBJECT_FLAG_NON_DENSE_ARRAY);
            bool packed = !object->hasAnyFlags(OBJECT_FLAG_NON_PACKED_ARRAY);
            JS_ASSERT_IF(packed, dense);
            if (dense) {
                if (!obj->isDenseArray() || (packed && !obj->isPackedDenseArray())) {
                    TypeFailure(cx, "Object not %s array at #%u:%05u popped %u: %s",
                        packed ? "packed" : "dense",
                        id(), pc - code, i, object->name());
                }
            }
        }
    }
}

#endif





void
JSObject::makeNewType(JSContext *cx, JSScript *newScript)
{
    JS_ASSERT(!newType);

    TypeObject *type = cx->compartment->types.newTypeObject(cx, NULL, getType()->name(), "new",
                                                            false, false, this);
    if (!type)
        return;

    if (!cx->typeInferenceEnabled()) {
        newType = type;
        setDelegate();
        return;
    }

    AutoEnterTypeInference enter(cx);

    if (!getType()->unknownProperties()) {
        
        TypeSet *types = getType()->getProperty(cx, JSID_EMPTY, true);
        if (types)
            types->addType(cx, (jstype) type);
    }

    if (newScript && !type->unknownProperties()) {
        
        JSObject *baseobj = NewBuiltinClassInstance(cx, &js_ObjectClass, gc::FINALIZE_OBJECT16);
        if (!baseobj)
            return;

        Vector<TypeNewScript::Initializer> initializerList(cx);
        AnalyzeNewScriptProperties(cx, type, newScript, &baseobj, &initializerList);
        if (baseobj && baseobj->slotSpan() > 0 && !type->newScriptCleared) {
            js::gc::FinalizeKind kind = js::gc::GetGCObjectKind(baseobj->slotSpan());

            
            JS_ASSERT(js::gc::GetGCKindSlots(kind) >= baseobj->slotSpan());

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

            type->newScript->script = newScript;
            type->newScript->finalizeKind = unsigned(kind);
            type->newScript->shape = baseobj->lastProperty();

            type->newScript->initializerList = (TypeNewScript::Initializer *)
                ((char *) type->newScript + sizeof(TypeNewScript));
            PodCopy(type->newScript->initializerList, initializerList.begin(), initializerList.length());
        }
    }

    newType = type;
    setDelegate();
}





void
types::TypeObject::trace(JSTracer *trc)
{
    JS_ASSERT(!marked);

    



    if (trc->context->runtime->gcMarkAndSweep)
        marked = true;

#ifdef DEBUG
    gc::MarkId(trc, name_, "type_name");
#endif

    unsigned count = getPropertyCount();
    for (unsigned i = 0; i < count; i++) {
        Property *prop = getProperty(i);
        if (prop)
            gc::MarkId(trc, prop->id, "type_prop");
    }

    if (emptyShapes) {
        int count = gc::FINALIZE_OBJECT_LAST - gc::FINALIZE_OBJECT0 + 1;
        for (int i = 0; i < count; i++) {
            if (emptyShapes[i])
                MarkShape(trc, emptyShapes[i], "empty_shape");
        }
    }

    if (proto)
        gc::MarkObject(trc, *proto, "type_proto");

    if (singleton)
        gc::MarkObject(trc, *singleton, "type_singleton");

    if (newScript) {
        js_TraceScript(trc, newScript->script);
        gc::MarkShape(trc, newScript->shape, "new_shape");
    }
}






bool
TypeSet::CondenseSweepTypeSet(JSContext *cx, JSCompartment *compartment,
                              ScriptSet &condensed, TypeSet *types)
{
    






    JS_ASSERT(!types->intermediate());

    if (types->objectCount >= 2) {
        bool removed = false;
        unsigned objectCapacity = HashSetCapacity(types->objectCount);
        for (unsigned i = 0; i < objectCapacity; i++) {
            TypeObject *object = types->objectSet[i];
            if (object && !object->marked) {
                







                if (object->unknownProperties())
                    types->objectSet[i] = &compartment->types.typeEmpty;
                else
                    types->objectSet[i] = NULL;
                removed = true;
            }
        }
        if (removed) {
            
            TypeObject **oldArray = types->objectSet;
            types->objectSet = NULL;
            types->objectCount = 0;
            for (unsigned i = 0; i < objectCapacity; i++) {
                TypeObject *object = oldArray[i];
                if (object) {
                    TypeObject **pentry = HashSetInsert<TypeObject *,TypeObject,TypeObjectKey>
                        (cx, types->objectSet, types->objectCount, object, false);
                    if (pentry)
                        *pentry = object;
                }
            }
            cx->free_(oldArray);
        }
    } else if (types->objectCount == 1) {
        TypeObject *object = (TypeObject*) types->objectSet;
        if (!object->marked) {
            if (object->unknownProperties()) {
                types->objectSet = (TypeObject**) &compartment->types.typeEmpty;
            } else {
                types->objectSet = NULL;
                types->objectCount = 0;
            }
        }
    }

    TypeConstraint *constraint = types->constraintList;
    types->constraintList = NULL;

    






    while (constraint) {
        TypeConstraint *next = constraint->next;

        TypeObject *object = constraint->persistentObject();
        if (object) {
            




            if (object->marked) {
                constraint->next = types->constraintList;
                types->constraintList = constraint;
            } else {
                cx->delete_(constraint);
            }
            constraint = next;
            continue;
        }

        



        JSScript *script = constraint->script;
        if (script->isCachedEval ||
            (script->u.object && IsAboutToBeFinalized(cx, script->u.object)) ||
            (script->fun && IsAboutToBeFinalized(cx, script->fun))) {
            if (constraint->condensed())
                cx->delete_(constraint);
            constraint = next;
            continue;
        }

        ScriptSet::AddPtr p = condensed.lookupForAdd(script);
        if (!p) {
            if (!condensed.add(p, script) || !types->addCondensed(cx, script)) {
                SwitchToCompartment enterCompartment(cx, compartment);
                AutoEnterTypeInference enter(cx);
                compartment->types.setPendingNukeTypes(cx);
                return false;
            }
        }

        if (constraint->condensed())
            cx->free_(constraint);
        constraint = next;
    }

    condensed.clear();
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
CondenseTypeObjectList(JSContext *cx, JSCompartment *compartment, TypeObject *objects)
{
    TypeSet::ScriptSet condensed;
    if (!condensed.init()) {
        SwitchToCompartment enterCompartment(cx, compartment);
        AutoEnterTypeInference enter(cx);
        compartment->types.setPendingNukeTypes(cx);
        return false;
    }

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
            if (prop && !TypeSet::CondenseSweepTypeSet(cx, compartment, condensed, &prop->types))
                return false;
        }

        object = object->next;
    }

    return true;
}

bool
JSCompartment::condenseTypes(JSContext *cx)
{
    PruneInstanceObjects(&types.typeEmpty);

    return CondenseTypeObjectList(cx, this, types.objects);
}

static void
DestroyProperty(JSContext *cx, Property *prop)
{
    prop->types.destroy(cx);
    cx->delete_(prop);
}

static void
SweepTypeObjectList(JSContext *cx, TypeObject *&objects)
{
    TypeObject **pobject = &objects;
    while (*pobject) {
        TypeObject *object = *pobject;
        if (object->marked) {
            object->marked = false;
            pobject = &object->next;
        } else {
            if (object->emptyShapes)
                cx->free_(object->emptyShapes);
            *pobject = object->next;

            unsigned count = object->getPropertyCount();
            for (unsigned i = 0; i < count; i++) {
                Property *prop = object->getProperty(i);
                if (prop)
                    DestroyProperty(cx, prop);
            }
            if (count >= 2)
                cx->free_(object->propertySet);

            cx->delete_(object);
        }
    }
}

void
TypeCompartment::sweep(JSContext *cx)
{
    if (typeEmpty.marked) {
        typeEmpty.marked = false;
    } else if (typeEmpty.emptyShapes) {
        cx->free_(typeEmpty.emptyShapes);
        typeEmpty.emptyShapes = NULL;
    }

    




    if (arrayTypeTable) {
        for (ArrayTypeTable::Enum e(*arrayTypeTable); !e.empty(); e.popFront()) {
            const ArrayTableKey &key = e.front().key;
            TypeObject *obj = e.front().value;
            JS_ASSERT(obj->proto == key.proto);

            bool remove = false;
            if (TypeIsObject(key.type) && !((TypeObject *)key.type)->marked)
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
                if (TypeIsObject(entry.types[i]) && !((TypeObject *)entry.types[i])->marked)
                    remove = true;
            }

            if (remove) {
                cx->free_(key.ids);
                cx->free_(entry.types);
                e.removeFront();
            }
        }
    }

    SweepTypeObjectList(cx, objects);
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

bool
JSScript::condenseTypes(JSContext *cx)
{
    if (!CondenseTypeObjectList(cx, compartment, typeObjects))
        return false;

    if (typeArray) {
        TypeSet::ScriptSet condensed;
        if (!condensed.init()) {
            SwitchToCompartment enterCompartment(cx, compartment);
            AutoEnterTypeInference enter(cx);
            compartment->types.setPendingNukeTypes(cx);
            return false;
        }

        unsigned num = nTypeSets + TotalSlots(this) + bindings.countUpvars();

        if (isCachedEval ||
            (u.object && IsAboutToBeFinalized(cx, u.object)) ||
            (fun && IsAboutToBeFinalized(cx, fun))) {
            for (unsigned i = 0; i < num; i++)
                typeArray[i].destroy(cx);
            cx->free_(typeArray);
            typeArray = NULL;
        } else {
            for (unsigned i = 0; i < num; i++) {
                if (!TypeSet::CondenseSweepTypeSet(cx, compartment, condensed, &typeArray[i]))
                    return false;
            }
        }
    }

    TypeIntermediate **presult = &intermediateTypes;
    while (*presult) {
        TypeIntermediate *result = *presult;
        if (result->sweep(cx, compartment)) {
            presult = &result->next;
        } else {
            *presult = result->next;
            cx->delete_(result);
        }
    }

    return true;
}

void
JSScript::sweepAnalysis(JSContext *cx)
{
    SweepTypeObjectList(cx, typeObjects);

    if (analysis_ && !compartment->activeAnalysis) {
        



        analysis_ = NULL;
    }
}
