






































#include "jsapi.h"
#include "jsautooplen.h"
#include "jsbit.h"
#include "jsbool.h"
#include "jsdate.h"
#include "jsexn.h"
#include "jsgc.h"
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
#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#ifdef JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

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

namespace js {
namespace types {

static const char *js_CodeNameTwo[] = {
#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format) \
    name,
#include "jsopcode.tbl"
#undef OPDEF
};

const char *
TypeIdStringImpl(jsid id)
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
TypeString(jstype type)
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

void InferSpew(SpewChannel channel, const char *fmt, ...)
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

    





    if (js::types::TypeIsObject(type) && ((js::types::TypeObject*)type)->unknownProperties) {
        if (types->objectCount >= 2) {
            unsigned objectCapacity = HashSetCapacity(types->objectCount);
            for (unsigned i = 0; i < objectCapacity; i++) {
                TypeObject *object = types->objectSet[i];
                if (object && object->unknownProperties)
                    return true;
            }
        } else if (types->objectCount == 1) {
            TypeObject *object = (TypeObject *) types->objectSet;
            if (object->unknownProperties)
                return true;
        }
    }

    return false;
}

bool
TypeHasProperty(JSContext *cx, TypeObject *obj, jsid id, const Value &value)
{
    





    if (cx->typeInferenceEnabled() && !obj->unknownProperties && !value.isUndefined()) {
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

        cx->compartment->types.checkPendingRecompiles(cx);
    }
    return true;
}

#endif

void TypeFailure(JSContext *cx, const char *fmt, ...)
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







struct AnalyzeStateStack {
    TypeSet *types;

    
    bool isForEach;

    
    TypeObject *initializer;
};

struct AnalyzeState {
    JSContext *cx;

    analyze::Script &analysis;
    JSArenaPool &pool;

    AnalyzeStateStack *stack;

    
    unsigned stackDepth;

    
    TypeSet ***joinTypes;

    
    bool hasGetSet;

    
    bool hasHole;

    AnalyzeState(JSContext *cx, analyze::Script &analysis)
        : cx(cx), analysis(analysis), pool(analysis.pool),
          stack(NULL), stackDepth(0), hasGetSet(false), hasHole(false)
    {}

    bool init(JSScript *script)
    {
        unsigned length = (script->nslots * sizeof(AnalyzeStateStack))
                        + (script->length * sizeof(TypeSet**));
        unsigned char *cursor = (unsigned char *) cx->calloc(length);
        if (!cursor)
            return false;

        stack = (AnalyzeStateStack *) cursor;

        cursor += (script->nslots * sizeof(AnalyzeStateStack));
        joinTypes = (TypeSet ***) cursor;
        return true;
    }

    ~AnalyzeState()
    {
        cx->free(stack);
    }

    AnalyzeStateStack &popped(unsigned i) {
        JS_ASSERT(i < stackDepth);
        return stack[stackDepth - 1 - i];
    }

    const AnalyzeStateStack &popped(unsigned i) const {
        JS_ASSERT(i < stackDepth);
        return stack[stackDepth - 1 - i];
    }
};





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
    JS_ASSERT_IF(!constraint->condensed(), cx->compartment->types.inferenceDepth);
    JS_ASSERT_IF(typeFlags & TYPE_FLAG_INTERMEDIATE_SET,
                 !constraint->baseSubset() && !constraint->condensed());

    if (!constraint) {
        
        cx->compartment->types.setPendingNukeTypes(cx);
    }

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

    if (objectCount >= 2) {
        unsigned objectCapacity = HashSetCapacity(objectCount);
        for (unsigned i = 0; i < objectCapacity; i++) {
            TypeObject *object = objectSet[i];
            if (object)
                cx->compartment->types.addPending(cx, constraint, this, (jstype) object);
        }
    } else if (objectCount == 1) {
        TypeObject *object = (TypeObject*) objectSet;
        cx->compartment->types.addPending(cx, constraint, this, (jstype) object);
    }

    cx->compartment->types.resolvePending(cx);
}

void
TypeSet::print(JSContext *cx)
{
    if ((typeFlags & ~TYPE_FLAG_INTERMEDIATE_SET) == 0 && !objectCount) {
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

        if (objectCount >= 2) {
            unsigned objectCapacity = HashSetCapacity(objectCount);
            for (unsigned i = 0; i < objectCapacity; i++) {
                TypeObject *object = objectSet[i];
                if (object)
                    printf(" %s", object->name());
            }
        } else if (objectCount == 1) {
            TypeObject *object = (TypeObject*) objectSet;
            printf(" %s", object->name());
        }
    }
}

class TypeConstraintInput : public TypeConstraint
{
public:
    TypeConstraintInput(JSScript *script)
        : TypeConstraint("input", script)
    {}

    bool input() { return true; }

    void newType(JSContext *cx, TypeSet *source, jstype type);
};


class TypeConstraintSubset : public TypeConstraint
{
public:
    TypeSet *target;

    TypeConstraintSubset(JSScript *script, TypeSet *target)
        : TypeConstraint("subset", script), target(target)
    {
        JS_ASSERT(target);
    }

    void newType(JSContext *cx, TypeSet *source, jstype type);
};

void
TypeSet::addSubset(JSContext *cx, JSScript *script, TypeSet *target)
{
    add(cx, ArenaNew<TypeConstraintSubset>(cx->compartment->types.pool, script, target));
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

    void newType(JSContext *cx, TypeSet *source, jstype type);

    TypeObject * baseSubset() { return object; }
};

void
TypeSet::addBaseSubset(JSContext *cx, TypeObject *obj, TypeSet *target)
{
    TypeConstraintBaseSubset *constraint =
        (TypeConstraintBaseSubset *) ::js_calloc(sizeof(TypeConstraintBaseSubset));
    if (constraint)
        new(constraint) TypeConstraintBaseSubset(obj, target);
    add(cx, constraint);
}


class TypeConstraintCondensed : public TypeConstraint
{
public:
    TypeConstraintCondensed(JSScript *script)
        : TypeConstraint("condensed", script)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type);
    void newObjectState(JSContext *cx);

    bool condensed() { return true; }
};

void
TypeSet::addCondensed(JSContext *cx, JSScript *script)
{
    TypeConstraintCondensed *constraint =
        (TypeConstraintCondensed *) ::js_calloc(sizeof(TypeConstraintCondensed));

    if (!constraint) {
        



        script->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    new(constraint) TypeConstraintCondensed(script);
    add(cx, constraint, false);
}


class TypeConstraintProp : public TypeConstraint
{
public:
    const jsbytecode *pc;

    



    bool assign;
    TypeSet *target;

    
    jsid id;

    TypeConstraintProp(JSScript *script, const jsbytecode *pc,
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
TypeSet::addGetProperty(JSContext *cx, JSScript *script, const jsbytecode *pc,
                        TypeSet *target, jsid id)
{
    add(cx, ArenaNew<TypeConstraintProp>(cx->compartment->types.pool, script, pc, target, id, false));
}

void
TypeSet::addSetProperty(JSContext *cx, JSScript *script, const jsbytecode *pc,
                        TypeSet *target, jsid id)
{
    add(cx, ArenaNew<TypeConstraintProp>(cx->compartment->types.pool, script, pc, target, id, true));
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
    add(cx, ArenaNew<TypeConstraintNewObject>(cx->compartment->types.pool, script, fun, target));
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
    add(cx, ArenaNew<TypeConstraintCall>(cx->compartment->types.pool, site));
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
    add(cx, ArenaNew<TypeConstraintArith>(cx->compartment->types.pool, script, target, other));
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
    add(cx, ArenaNew<TypeConstraintTransformThis>(cx->compartment->types.pool, script, target));
}


class TypeConstraintFilterPrimitive : public TypeConstraint
{
public:
    TypeSet *target;

    
    bool onlyNullVoid;

    TypeConstraintFilterPrimitive(JSScript *script, TypeSet *target, bool onlyNullVoid)
        : TypeConstraint("filter", script), target(target), onlyNullVoid(onlyNullVoid)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type);
};

void
TypeSet::addFilterPrimitives(JSContext *cx, JSScript *script, TypeSet *target, bool onlyNullVoid)
{
    add(cx, ArenaNew<TypeConstraintFilterPrimitive>(cx->compartment->types.pool,
                                                    script, target, onlyNullVoid));
}





class TypeConstraintMonitorRead : public TypeConstraint
{
public:
    TypeSet *target;

    TypeConstraintMonitorRead(JSScript *script, TypeSet *target)
        : TypeConstraint("monitorRead", script), target(target)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type);
};

void
TypeSet::addMonitorRead(JSContext *cx, JSScript *script, TypeSet *target)
{
    add(cx, ArenaNew<TypeConstraintMonitorRead>(cx->compartment->types.pool, script, target));
}





class TypeConstraintGenerator : public TypeConstraint
{
public:
    TypeSet *target;

    TypeConstraintGenerator(JSScript *script, TypeSet *target)
        : TypeConstraint("generator", script), target(target)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type);
};


static inline void
SetForTypes(JSContext *cx, JSScript *script, const AnalyzeState &state, TypeSet *types)
{
    if (state.popped(0).isForEach)
        types->addType(cx, TYPE_UNKNOWN);
    else
        types->addType(cx, TYPE_STRING);

    state.popped(0).types->add(cx,
        ArenaNew<TypeConstraintGenerator>(cx->compartment->types.pool, script, types));
}





void
TypeConstraintSubset::newType(JSContext *cx, TypeSet *source, jstype type)
{
    
    target->addType(cx, type);
}

void
TypeConstraintBaseSubset::newType(JSContext *cx, TypeSet *source, jstype type)
{
    target->addType(cx, type);
}


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
PropertyAccess(JSContext *cx, JSScript *script, const jsbytecode *pc, TypeObject *object,
               bool assign, TypeSet *target, jsid id)
{
    JS_ASSERT_IF(!target, assign);

    
    if (object->unknownProperties) {
        if (!assign)
            target->addType(cx, TYPE_UNKNOWN);
        return;
    }

    
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

    
    if (target) {
        TypeSet *types = object->getProperty(cx, id, assign);
        if (!types)
            return;
        if (assign)
            target->addSubset(cx, script, types);
        else
            types->addMonitorRead(cx, script, target);
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
    if (type == TYPE_UNKNOWN ||
        (!TypeIsObject(type) && !script->compileAndGo)) {
        





        if (assign)
            cx->compartment->types.monitorBytecode(cx, script, pc - script->code);
        else
            target->addType(cx, TYPE_UNKNOWN);
        return;
    }

    TypeObject *object = GetPropertyObject(cx, script, type);
    if (object)
        PropertyAccess(cx, script, pc, object, assign, target, id);
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
        if (object->unknownProperties) {
            target->addType(cx, TYPE_UNKNOWN);
        } else {
            TypeSet *newTypes = object->getProperty(cx, JSID_EMPTY, true);
            if (!newTypes)
                return;
            newTypes->addMonitorRead(cx, script, target);
        }
    } else if (!fun->script) {
        




    } else if (!fun->script->compileAndGo) {
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
    const jsbytecode *pc = callsite->pc;

    if (type == TYPE_UNKNOWN) {
        
        cx->compartment->types.monitorBytecode(cx, script, pc - script->code);
        return;
    }

    if (!TypeIsObject(type))
        return;

    
    TypeObject *object = (TypeObject*) type;
    if (object->unknownProperties) {
        
        cx->compartment->types.monitorBytecode(cx, script, pc - script->code);
        return;
    }
    if (!object->isFunction) {
        



        return;
    }
    TypeFunction *function = object->asFunction();

    if (!function->script) {
        JS_ASSERT(function->handler);

        if (function->isGeneric) {
            if (callsite->argumentCount == 0) {
                
                return;
            }

            





            TypeSet *thisTypes = TypeSet::make(cx, "genericthis");
            if (!thisTypes)
                return;
            callsite->argumentTypes[0]->addTransformThis(cx, script, thisTypes);

            TypeCallsite *newSite = ArenaNew<TypeCallsite>(cx->compartment->types.pool,
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

    
    if (!callee->analyzed)
        AnalyzeScriptTypes(cx, callee);

    
    for (unsigned i = 0; i < callsite->argumentCount && i < nargs; i++) {
        TypeSet *argTypes = callsite->argumentTypes[i];
        TypeSet *types = callee->argTypes(i);
        argTypes->addSubset(cx, script, types);
    }

    
    for (unsigned i = callsite->argumentCount; i < nargs; i++) {
        TypeSet *types = callee->argTypes(i);
        types->addType(cx, TYPE_UNDEFINED);
    }

    
    if (callsite->isNew) {
        callee->typeSetNewCalled(cx);

        



        if (callsite->returnTypes) {
            callee->thisTypes()->addSubset(cx, script, callsite->returnTypes);
            callee->returnTypes()->addFilterPrimitives(cx, script,
                                                       callsite->returnTypes, false);
        }
    } else {
        if (callsite->thisTypes) {
            
            callsite->thisTypes->addSubset(cx, script, callee->thisTypes());
        } else {
            JS_ASSERT(callsite->thisType != TYPE_NULL);
            callee->thisTypes()->addType(cx, callsite->thisType);
        }

        
        if (callsite->returnTypes)
            callee->returnTypes()->addSubset(cx, script, callsite->returnTypes);
    }
}

void
TypeConstraintArith::newType(JSContext *cx, TypeSet *source, jstype type)
{
    







    if (other) {
        





        switch (type) {
          case TYPE_DOUBLE:
            if (other->typeFlags & (TYPE_FLAG_UNDEFINED | TYPE_FLAG_NULL |
                                    TYPE_FLAG_INT32 | TYPE_FLAG_DOUBLE | TYPE_FLAG_BOOLEAN) ||
                other->objectCount != 0) {
                target->addType(cx, TYPE_DOUBLE);
            }
            break;
          case TYPE_STRING:
            target->addType(cx, TYPE_STRING);
            break;
          case TYPE_UNKNOWN:
            target->addType(cx, TYPE_UNKNOWN);
          default:
            if (other->typeFlags & (TYPE_FLAG_UNDEFINED | TYPE_FLAG_NULL |
                                    TYPE_FLAG_INT32 | TYPE_FLAG_BOOLEAN) ||
                other->objectCount != 0) {
                target->addType(cx, TYPE_INT32);
            }
            if (other->typeFlags & TYPE_FLAG_DOUBLE)
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

    if (!script->compileAndGo) {
        target->addType(cx, TYPE_UNKNOWN);
        return;
    }

    TypeObject *object = NULL;
    switch (type) {
      case TYPE_NULL:
      case TYPE_UNDEFINED:
        object = script->getGlobalType();
        break;
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

void
TypeConstraintFilterPrimitive::newType(JSContext *cx, TypeSet *source, jstype type)
{
    if (onlyNullVoid) {
        if (type == TYPE_NULL || type == TYPE_UNDEFINED)
            return;
    } else if (type != TYPE_UNKNOWN && TypeIsPrimitive(type)) {
        return;
    }

    target->addType(cx, type);
}

void
TypeConstraintMonitorRead::newType(JSContext *cx, TypeSet *source, jstype type)
{
    target->addType(cx, type);
}

void
TypeConstraintGenerator::newType(JSContext *cx, TypeSet *source, jstype type)
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





void
TypeConstraintCondensed::newType(JSContext *cx, TypeSet *source, jstype type)
{
    if (script->types) {
        









        return;
    }

    AnalyzeScriptTypes(cx, script);
}

void
TypeConstraintCondensed::newObjectState(JSContext *cx)
{
    if (script->types)
        return;
    AnalyzeScriptTypes(cx, script);
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
    add(cx, ArenaNew<TypeConstraintPushAll>(cx->compartment->types.pool, script, pc));
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
TypeSet::Clone(JSContext *cx, JSScript *script, TypeSet *source, ClonedTypeSet *target)
{
    if (!source) {
        target->typeFlags = TYPE_FLAG_UNKNOWN;
        return;
    }

    if (script && !source->unknown())
        source->add(cx, ArenaNew<TypeConstraintFreeze>(cx->compartment->types.pool, script), false);

    target->typeFlags = source->typeFlags & ~TYPE_FLAG_INTERMEDIATE_SET;
    target->objectCount = source->objectCount;
    if (source->objectCount >= 2) {
        target->objectSet = (TypeObject **) ::js_malloc(sizeof(TypeObject*) * source->objectCount);
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
            
            if (source->objectCount >= 2)
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
TypeSet::getKnownTypeTag(JSContext *cx, JSScript *script)
{
    TypeFlags flags = typeFlags & ~TYPE_FLAG_INTERMEDIATE_SET;
    JSValueType type;

    if (objectCount)
        type = flags ? JSVAL_TYPE_UNKNOWN : JSVAL_TYPE_OBJECT;
    else
        type = GetValueTypeFromTypeFlags(flags);

    if (script && type != JSVAL_TYPE_UNKNOWN)
        add(cx, ArenaNew<TypeConstraintFreezeTypeTag>(cx->compartment->types.pool, script), false);

    return type;
}


static inline ObjectKind
CombineObjectKind(TypeObject *object, ObjectKind kind)
{
    




    if (object->unknownProperties || object->hasSpecialEquality || kind == OBJECT_UNKNOWN)
        return OBJECT_UNKNOWN;

    ObjectKind nkind;
    if (object->isFunction)
        nkind = object->asFunction()->script ? OBJECT_SCRIPTED_FUNCTION : OBJECT_NATIVE_FUNCTION;
    else if (object->isPackedArray)
        nkind = OBJECT_PACKED_ARRAY;
    else if (object->isDenseArray)
        nkind = OBJECT_DENSE_ARRAY;
    else
        nkind = OBJECT_NO_SPECIAL_EQUALITY;

    if (kind == nkind || kind == OBJECT_NONE)
        return nkind;

    if ((kind == OBJECT_PACKED_ARRAY && nkind == OBJECT_DENSE_ARRAY) ||
        (kind == OBJECT_DENSE_ARRAY && nkind == OBJECT_PACKED_ARRAY)) {
        return OBJECT_DENSE_ARRAY;
    }

    return OBJECT_NO_SPECIAL_EQUALITY;
}


class TypeConstraintFreezeObjectKind : public TypeConstraint
{
public:
    TypeObject *object;

    



    ObjectKind *pkind;

    TypeConstraintFreezeObjectKind(TypeObject *object, ObjectKind *pkind, JSScript *script)
        : TypeConstraint("freezeObjectKind", script), object(object), pkind(pkind)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type) {}

    void newObjectState(JSContext *cx)
    {
        ObjectKind nkind = CombineObjectKind(object, *pkind);
        if (nkind != *pkind) {
            *pkind = nkind;
            cx->compartment->types.addPendingRecompile(cx, script);
        }
    }
};





class TypeConstraintFreezeObjectKindSet : public TypeConstraint
{
public:
    ObjectKind kind;

    TypeConstraintFreezeObjectKindSet(ObjectKind kind, JSScript *script)
        : TypeConstraint("freezeObjectKindSet", script), kind(kind)
    {
        JS_ASSERT(kind != OBJECT_NONE);
    }

    void newType(JSContext *cx, TypeSet *source, jstype type)
    {
        if (kind == OBJECT_UNKNOWN) {
            
            return;
        }

        if (type == TYPE_UNKNOWN) {
            kind = OBJECT_UNKNOWN;
        } else if (TypeIsObject(type)) {
            TypeObject *object = (TypeObject *) type;
            ObjectKind nkind = CombineObjectKind(object, kind);

            if (nkind != OBJECT_UNKNOWN) {
                



                TypeSet *elementTypes = object->getProperty(cx, JSID_VOID, false);
                if (!elementTypes)
                    return;
                elementTypes->add(cx,
                    ArenaNew<TypeConstraintFreezeObjectKind>(cx->compartment->types.pool,
                                                             object, &kind, script), false);
            }

            if (nkind == kind) {
                
                return;
            }
            kind = nkind;
        } else {
            return;
        }

        cx->compartment->types.addPendingRecompile(cx, script);
    }
};

ObjectKind
TypeSet::getKnownObjectKind(JSContext *cx, JSScript *script)
{
    ObjectKind kind = OBJECT_NONE;

    if (objectCount >= 2) {
        unsigned objectCapacity = HashSetCapacity(objectCount);
        for (unsigned i = 0; i < objectCapacity; i++) {
            TypeObject *object = objectSet[i];
            if (object)
                kind = CombineObjectKind(object, kind);
        }
    } else if (objectCount == 1) {
        kind = CombineObjectKind((TypeObject *) objectSet, kind);
    } else {
        return OBJECT_UNKNOWN;
    }

    if (kind != OBJECT_UNKNOWN) {
        



        add(cx, ArenaNew<TypeConstraintFreezeObjectKindSet>(cx->compartment->types.pool, kind, script));
    }

    return kind;
}


class TypeConstraintFreezeNonEmpty : public TypeConstraint
{
public:
    bool hasType;

    TypeConstraintFreezeNonEmpty(JSScript *script)
        : TypeConstraint("freezeNonEmpty", script), hasType(false)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type)
    {
        if (hasType)
            return;

        hasType = true;
        cx->compartment->types.addPendingRecompile(cx, script);
    }
};

bool
TypeSet::knownNonEmpty(JSContext *cx, JSScript *script)
{
    if (typeFlags != 0)
        return true;

    add(cx, ArenaNew<TypeConstraintFreezeNonEmpty>(cx->compartment->types.pool, script), false);

    return false;
}





void
TypeCompartment::init(JSContext *cx)
{
    PodZero(this);

    




#ifdef DEBUG
    typeEmpty.name_ = JSID_VOID;
#endif
    typeEmpty.unknownProperties = true;

    if (cx && cx->getRunOptions() & JSOPTION_TYPE_INFERENCE)
        inferenceEnabled = true;

    JS_InitArenaPool(&pool, "typeinfer", 512, 8, NULL);
}

TypeObject *
TypeCompartment::newTypeObject(JSContext *cx, JSScript *script, const char *name,
                               bool isFunction, JSObject *proto)
{
#ifdef DEBUG
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

    TypeObject *object;
    if (isFunction) {
        object = (TypeFunction *) cx->calloc(sizeof(TypeFunction));
        if (!object)
            return NULL;
        new(object) TypeFunction(id, proto);
    } else {
        object = (TypeObject *) cx->calloc(sizeof(TypeObject));
        if (!object)
            return NULL;
        new(object) TypeObject(id, proto);
    }

    TypeObject *&objects = script ? script->typeObjects : this->objects;
    object->next = objects;
    objects = object;

    if (!cx->typeInferenceEnabled())
        object->hasSpecialEquality = true;

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

    TypeObject *res = newTypeObject(cx, script, name, false, proto);
    if (!res)
        return NULL;

    if (isArray) {
        if (!res->unknownProperties)
            res->isDenseArray = res->isPackedArray = true;
        res->initializerArray = true;
    } else {
        res->initializerObject = true;
    }
    res->initializerOffset = offset;

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
UseNewType(JSContext *cx, JSScript *script, jsbytecode *pc)
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

void
TypeCompartment::growPendingArray(JSContext *cx)
{
    unsigned newCapacity = js::Max(unsigned(100), pendingCapacity * 2);
    PendingWork *newArray = (PendingWork *) js_calloc(newCapacity * sizeof(PendingWork));
    if (!newArray) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    memcpy(newArray, pendingArray, pendingCount * sizeof(PendingWork));
    js_free(pendingArray);

    pendingArray = newArray;
    pendingCapacity = newCapacity;
}

bool
TypeCompartment::dynamicCall(JSContext *cx, JSObject *callee,
                             const js::CallArgs &args, bool constructing)
{
    unsigned nargs = callee->getFunctionPrivate()->nargs;
    JSScript *script = callee->getFunctionPrivate()->script();

    if (constructing) {
        script->typeSetNewCalled(cx);
    } else {
        jstype type = GetValueType(cx, args.thisv());
        if (!script->typeSetThis(cx, type))
            return false;
    }

    




    unsigned arg = 0;
    for (; arg < args.argc() && arg < nargs; arg++) {
        if (!script->typeSetArgument(cx, arg, args[arg]))
            return false;
    }

    
    for (; arg < nargs; arg++) {
        if (!script->typeSetArgument(cx, arg, UndefinedValue()))
            return false;
    }

    return true;
}

bool
TypeCompartment::dynamicPush(JSContext *cx, JSScript *script, uint32 offset, jstype type)
{
    if (script->types) {
        




        js::types::TypeSet *pushed = script->types->pushed(offset, 0);
        if (pushed->hasType(type))
            return true;
    } else {
        
        js::types::TypeResult *result, **presult = &script->typeResults;
        while (*presult) {
            result = *presult;
            if (result->offset == offset && result->type == type) {
                if (presult != &script->typeResults) {
                    
                    *presult = result->next;
                    result->next = script->typeResults;
                    script->typeResults = result;
                }
                return true;
            }
            presult = &result->next;
        }
    }

    AutoEnterTypeInference enter(cx);

    InferSpew(ISpewOps, "externalType: monitorResult #%u:%05u: %s",
               script->id(), offset, TypeString(type));

    TypeResult *result = (TypeResult *) cx->calloc(sizeof(TypeResult));
    if (!result) {
        setPendingNukeTypes(cx);
        return checkPendingRecompiles(cx);
    }

    result->offset = offset;
    result->type = type;
    result->next = script->typeResults;
    script->typeResults = result;

    if (script->types) {
        TypeSet *pushed = script->types->pushed(offset, 0);
        pushed->addType(cx, type);
    } else if (script->analyzed) {
        
        AnalyzeScriptTypes(cx, script);
    }

    







    jsbytecode *pc = script->code + offset;
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
            if (!global->unknownProperties) {
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
            if (GET_SLOTNO(pc) < script->nfixed) {
                TypeSet *types = script->localTypes(GET_SLOTNO(pc));
                types->addType(cx, type);
            }
            break;

          case JSOP_INCARG:
          case JSOP_DECARG:
          case JSOP_ARGINC:
          case JSOP_ARGDEC: {
            TypeSet *types = script->argTypes(GET_SLOTNO(pc));
            types->addType(cx, type);
            break;
          }

          default:;
        }
    }

    return checkPendingRecompiles(cx);
}

bool
TypeCompartment::processPendingRecompiles(JSContext *cx)
{
    
    Vector<JSScript*> *pending = pendingRecompiles;
    pendingRecompiles = NULL;

    for (unsigned i = 0; i < pending->length(); i++) {
#ifdef JS_METHODJIT
        JSScript *script = (*pending)[i];
        mjit::Recompiler recompiler(cx, script);
        if (!recompiler.recompile()) {
            pendingNukeTypes = true;
            cx->free(pending);
            return nukeTypes(cx);
        }
#endif
    }

    cx->free(pending);
    return true;
}

void
TypeCompartment::setPendingNukeTypes(JSContext *cx)
{
    if (!pendingNukeTypes) {
        js_ReportOutOfMemory(cx);
        pendingNukeTypes = true;
    }
}

bool
TypeCompartment::nukeTypes(JSContext *cx)
{
    










    JS_ASSERT(pendingNukeTypes);
    if (pendingRecompiles) {
        cx->free(pendingRecompiles);
        pendingRecompiles = NULL;
    }

    
    *((int*)0) = 0;

    return true;
}

void
TypeCompartment::addPendingRecompile(JSContext *cx, JSScript *script)
{
    if (!script->jitNormal && !script->jitCtor) {
        
        return;
    }

    if (!pendingRecompiles) {
        pendingRecompiles = (Vector<JSScript*>*) cx->calloc(sizeof(Vector<JSScript*>));
        if (!pendingRecompiles) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
        new(pendingRecompiles) Vector<JSScript*>(cx);
    }

    for (unsigned i = 0; i < pendingRecompiles->length(); i++) {
        if (script == (*pendingRecompiles)[i])
            return;
    }

    if (!pendingRecompiles->append(script)) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    recompilations++;
}

bool
TypeCompartment::dynamicAssign(JSContext *cx, JSObject *obj, jsid id, const Value &rval)
{
    if (obj->isWith())
        obj = js_UnwrapWithObject(cx, obj);

    jstype rvtype = GetValueType(cx, rval);
    TypeObject *object = obj->getType();

    if (object->unknownProperties)
        return true;

    id = MakeTypeId(cx, id);

    





    JSOp op = JSOp(*cx->regs->pc);
    if (id == id___proto__(cx) || (op == JSOP_SETELEM && !JSID_IS_VOID(id)))
        return cx->markTypeObjectUnknownProperties(object);

    AutoEnterTypeInference enter(cx);

    TypeSet *assignTypes = object->getProperty(cx, id, true);
    if (!assignTypes || assignTypes->hasType(rvtype))
        return cx->compartment->types.checkPendingRecompiles(cx);

    InferSpew(ISpewOps, "externalType: monitorAssign %s %s: %s",
              object->name(), TypeIdString(id), TypeString(rvtype));
    assignTypes->addType(cx, rvtype);

    return cx->compartment->types.checkPendingRecompiles(cx);
}

void
TypeCompartment::monitorBytecode(JSContext *cx, JSScript *script, uint32 offset)
{
    if (script->types->monitored(offset))
        return;

    




    JSOp op = JSOp(script->code[offset]);
    switch (op) {
      case JSOP_SETNAME:
      case JSOP_SETGNAME:
      case JSOP_SETXMLNAME:
      case JSOP_SETCONST:
      case JSOP_SETELEM:
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
        script->types->addType(cx, offset, 0, TYPE_UNKNOWN);
        break;
      default:
        TypeFailure(cx, "Monitoring unknown bytecode: %s", js_CodeNameTwo[op]);
    }

    InferSpew(ISpewOps, "addMonitorNeeded: #%u:%05u", script->id(), offset);

    script->types->setMonitored(offset);

    if (script->hasJITCode())
        cx->compartment->types.addPendingRecompile(cx, script);
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
        if (script->types)
            script->types->print(cx, script);
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

    double millis = analysisTime / 1000.0;

    printf("Counts: ");
    for (unsigned count = 0; count < TYPE_COUNT_LIMIT; count++) {
        if (count)
            printf("/");
        printf("%u", typeCounts[count]);
    }
    printf(" (%u over)\n", typeCountOver);

    printf("Recompilations: %u\n", recompilations);
    printf("Time: %.2f ms\n", millis);
}

















static inline bool
NumberTypes(jstype a, jstype b)
{
    return (a == TYPE_INT32 || a == TYPE_DOUBLE) && (b == TYPE_INT32 || b == TYPE_DOUBLE);
}

struct ArrayTableKey
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

bool
TypeCompartment::fixArrayType(JSContext *cx, JSObject *obj)
{
    if (!arrayTypeTable) {
        arrayTypeTable = js_new<ArrayTypeTable>();
        if (!arrayTypeTable || !arrayTypeTable->init()) {
            arrayTypeTable = NULL;
            js_ReportOutOfMemory(cx);
            return false;
        }
    }

    





    JS_ASSERT(obj->isPackedDenseArray());

    unsigned len = obj->getDenseArrayInitializedLength();
    if (len == 0)
        return true;

    jstype type = GetValueType(cx, obj->getDenseArrayElement(0));

    for (unsigned i = 1; i < len; i++) {
        jstype ntype = GetValueType(cx, obj->getDenseArrayElement(i));
        if (ntype != type) {
            if (NumberTypes(type, ntype))
                type = TYPE_DOUBLE;
            else
                return true;
        }
    }

    ArrayTableKey key;
    key.type = type;
    key.proto = obj->getProto();
    ArrayTypeTable::AddPtr p = arrayTypeTable->lookupForAdd(key);

    if (p) {
        obj->setType(p->value);
    } else {
        TypeObject *objType = newTypeObject(cx, NULL, "TableArray", false, obj->getProto());
        if (!objType) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        if (!objType->unknownProperties)
            objType->isDenseArray = objType->isPackedArray = true;
        obj->setType(objType);

        if (!cx->addTypePropertyId(objType, JSID_VOID, type))
            return false;

        if (!arrayTypeTable->relookupOrAdd(p, key, objType)) {
            js_ReportOutOfMemory(cx);
            return false;
        }
    }

    return true;
}







struct ObjectTableKey
{
    jsid *ids;
    uint32 nslots;
    JSObject *proto;

    typedef JSObject * Lookup;

    static inline uint32 hash(JSObject *obj) {
        return (uint32) (JSID_BITS(obj->lastProperty()->id) ^
                         obj->slotSpan() ^
                         ((uint32)(size_t)obj->getProto() >> 2));
    }

    static inline bool match(const ObjectTableKey &v, JSObject *obj) {
        if (obj->slotSpan() != v.nslots || obj->getProto() != v.proto)
            return false;
        const Shape *shape = obj->lastProperty();
        while (!JSID_IS_EMPTY(shape->id)) {
            if (shape->id != v.ids[shape->slot])
                return false;
            shape = shape->previous();
        }
        return true;
    }
};

struct ObjectTableEntry
{
    TypeObject *object;
    Shape *newShape;
    jstype *types;
};

bool
TypeCompartment::fixObjectType(JSContext *cx, JSObject *obj)
{
    if (!objectTypeTable) {
        objectTypeTable = js_new<ObjectTypeTable>();
        if (!objectTypeTable || !objectTypeTable->init()) {
            objectTypeTable = NULL;
            js_ReportOutOfMemory(cx);
            return false;
        }
    }

    





    JS_ASSERT(obj->isObject());

    if (obj->slotSpan() == 0 || obj->inDictionaryMode())
        return true;

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
                        while (!JSID_IS_EMPTY(shape->id)) {
                            if (shape->slot == i) {
                                if (!cx->addTypePropertyId(p->value.object, shape->id, TYPE_DOUBLE))
                                    return false;
                                break;
                            }
                            shape = shape->previous();
                        }
                    }
                } else {
                    return true;
                }
            }
        }

        obj->setTypeAndShape(p->value.object, p->value.newShape);
    } else {
        




        JSObject *xobj = NewBuiltinClassInstance(cx, &js_ObjectClass,
                                                 (gc::FinalizeKind) obj->finalizeKind());
        if (!xobj) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        AutoObjectRooter xvr(cx, xobj);

        TypeObject *objType = newTypeObject(cx, NULL, "TableObject", false, obj->getProto());
        if (!objType) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        xobj->setType(objType);

        jsid *ids = (jsid *) cx->calloc(obj->slotSpan() * sizeof(jsid));
        if (!ids)
            return false;

        jstype *types = (jstype *) cx->calloc(obj->slotSpan() * sizeof(jstype));
        if (!types)
            return false;

        const Shape *shape = baseShape;
        while (!JSID_IS_EMPTY(shape->id)) {
            ids[shape->slot] = shape->id;
            types[shape->slot] = GetValueType(cx, obj->getSlot(shape->slot));
            if (!cx->addTypePropertyId(objType, shape->id, types[shape->slot]))
                return false;
            shape = shape->previous();
        }

        
        for (unsigned i = 0; i < obj->slotSpan(); i++) {
            if (!js_DefineNativeProperty(cx, xobj, ids[i], UndefinedValue(), NULL, NULL,
                                         JSPROP_ENUMERATE, 0, 0, NULL, 0)) {
                return false;
            }
        }
        JS_ASSERT(!xobj->inDictionaryMode());
        const Shape *newShape = xobj->lastProperty();

        ObjectTableKey key;
        key.ids = ids;
        key.nslots = obj->slotSpan();
        key.proto = obj->getProto();
        JS_ASSERT(ObjectTableKey::match(key, obj));

        ObjectTableEntry entry;
        entry.object = objType;
        entry.newShape = (Shape *) newShape;
        entry.types = types;

        p = objectTypeTable->lookupForAdd(obj);
        if (!objectTypeTable->add(p, key, entry)) {
            js_ReportOutOfMemory(cx);
            return false;
        }

        obj->setTypeAndShape(objType, newShape);
    }

    return true;
}





void
TypeObject::storeToInstances(JSContext *cx, Property *base)
{
    TypeObject *object = instanceList;
    while (object) {
        Property *p =
            HashSetLookup<jsid,Property,Property>(object->propertySet, object->propertyCount, base->id);
        if (p)
            base->ownTypes.addBaseSubset(cx, object, &p->types);
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
             p->ownTypes.addBaseSubset(cx, this, &base->types);
         obj = obj->getProto();
     }
}

void
TypeObject::splicePrototype(JSContext *cx, JSObject *proto)
{
    JS_ASSERT(!this->proto);

    



    JS_ASSERT(this != &cx->compartment->types.typeEmpty);

    this->proto = proto;
    this->instanceNext = proto->getType()->instanceList;
    proto->getType()->instanceList = this;

    if (!cx->typeInferenceEnabled())
        return;

    AutoEnterTypeInference enter(cx);

    




    if (propertyCount >= 2) {
        unsigned capacity = HashSetCapacity(propertyCount);
        for (unsigned i = 0; i < capacity; i++) {
            Property *prop = propertySet[i];
            if (prop)
                getFromPrototypes(cx, prop);
        }
    } else if (propertyCount == 1) {
        Property *prop = (Property *) propertySet;
        getFromPrototypes(cx, prop);
    }

    JS_ALWAYS_TRUE(cx->compartment->types.checkPendingRecompiles(cx));
}

bool
TypeObject::addProperty(JSContext *cx, jsid id, Property **pprop)
{
    JS_ASSERT(!*pprop);
    Property *base = (Property *) cx->calloc(sizeof(Property));
    if (!base) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return false;
    }
    new(base) Property(id);

    *pprop = base;

    InferSpew(ISpewOps, "typeSet: T%p property %s %s",
              &base->types, name(), TypeIdString(id));
    InferSpew(ISpewOps, "typeSet: T%p own property %s %s",
              &base->ownTypes, name(), TypeIdString(id));

    base->ownTypes.addBaseSubset(cx, this, &base->types);

    
    if (instanceList)
        storeToInstances(cx, base);

    
    getFromPrototypes(cx, base);

    return true;
}

static inline void
ObjectStateChange(JSContext *cx, TypeObject *object, bool markingUnknown)
{
    
    TypeSet *elementTypes = object->getProperty(cx, JSID_VOID, false);
    if (!elementTypes)
        return;
    if (markingUnknown) {
        
        object->unknownProperties = true;
    }
    TypeConstraint *constraint = elementTypes->constraintList;
    while (constraint) {
        constraint->newObjectState(cx);
        constraint = constraint->next;
    }
}

void
TypeObject::markNotPacked(JSContext *cx, bool notDense)
{
    JS_ASSERT(cx->compartment->types.inferenceDepth);

    if (notDense) {
        if (!isDenseArray)
            return;
        isDenseArray = false;
    } else if (!isPackedArray) {
        return;
    }
    isPackedArray = false;

    InferSpew(ISpewOps, "%s: %s", notDense ? "NonDenseArray" : "NonPackedArray", name());

    ObjectStateChange(cx, this, false);
}

void
TypeObject::markUnknown(JSContext *cx)
{
    JS_ASSERT(!unknownProperties);

    InferSpew(ISpewOps, "UnknownProperties: %s", name());

    isDenseArray = false;
    isPackedArray = false;
    hasSpecialEquality = true;

    ObjectStateChange(cx, this, true);

    

    TypeObject *instance = instanceList;
    while (instance) {
        if (!instance->unknownProperties)
            instance->markUnknown(cx);
        instance = instance->instanceNext;
    }

    








    if (propertyCount >= 2) {
        unsigned capacity = HashSetCapacity(propertyCount);
        for (unsigned i = 0; i < capacity; i++) {
            Property *prop = propertySet[i];
            if (prop)
                prop->ownTypes.addType(cx, TYPE_UNKNOWN);
        }
    } else if (propertyCount == 1) {
        Property *prop = (Property *) propertySet;
        prop->ownTypes.addType(cx, TYPE_UNKNOWN);
    }
}

void
TypeObject::print(JSContext *cx)
{
    printf("%s : %s", name(), proto ? proto->getType()->name() : "(null)");

    if (unknownProperties)
        printf(" unknown");
    else if (isPackedArray)
        printf(" packed");
    else if (isDenseArray)
        printf(" dense");

    if (propertyCount == 0) {
        printf(" {}\n");
        return;
    }

    printf(" {");

    if (propertyCount >= 2) {
        unsigned capacity = HashSetCapacity(propertyCount);
        for (unsigned i = 0; i < capacity; i++) {
            Property *prop = propertySet[i];
            if (prop) {
                printf("\n    %s:", TypeIdString(prop->id));
                prop->ownTypes.print(cx);
            }
        }
    } else if (propertyCount == 1) {
        Property *prop = (Property *) propertySet;
        printf("\n    %s:", TypeIdString(prop->id));
        prop->ownTypes.print(cx);
    }

    printf("\n}\n");
}





static inline ptrdiff_t
GetJumpOffset(jsbytecode *pc, jsbytecode *pc2)
{
    uint32 type = JOF_OPTYPE(*pc);
    if (JOF_TYPE_IS_EXTENDED_JUMP(type))
        return GET_JUMPX_OFFSET(pc2);
    return GET_JUMP_OFFSET(pc2);
}


static inline bool
BytecodeNoFallThrough(JSOp op)
{
    switch (op) {
      case JSOP_GOTO:
      case JSOP_GOTOX:
      case JSOP_DEFAULT:
      case JSOP_DEFAULTX:
      case JSOP_RETURN:
      case JSOP_STOP:
      case JSOP_RETRVAL:
      case JSOP_THROW:
      case JSOP_TABLESWITCH:
      case JSOP_TABLESWITCHX:
      case JSOP_LOOKUPSWITCH:
      case JSOP_LOOKUPSWITCHX:
      case JSOP_FILTER:
        return true;
      case JSOP_GOSUB:
      case JSOP_GOSUBX:
        
        return false;
      default:
        return false;
    }
}


void
MergeTypes(JSContext *cx, AnalyzeState &state, JSScript *script, uint32 offset)
{
    unsigned targetDepth = state.analysis.getCode(offset).stackDepth;
    JS_ASSERT(state.stackDepth >= targetDepth);
    if (!state.joinTypes[offset]) {
        TypeSet **joinTypes = ArenaArray<TypeSet*>(state.pool, targetDepth);
        if (!joinTypes) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }
        state.joinTypes[offset] = joinTypes;
        for (unsigned i = 0; i < targetDepth; i++)
            joinTypes[i] = state.stack[i].types;
    }
    for (unsigned i = 0; i < targetDepth; i++) {
        if (!state.joinTypes[offset][i])
            state.joinTypes[offset][i] = state.stack[i].types;
        else if (state.stack[i].types && state.joinTypes[offset][i] != state.stack[i].types)
            state.stack[i].types->addSubset(cx, script, state.joinTypes[offset][i]);
    }
}





static inline bool
CheckNextTest(jsbytecode *pc)
{
    jsbytecode *next = pc + analyze::GetBytecodeLength(pc);
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


static bool
AnalyzeBytecode(JSContext *cx, AnalyzeState &state, JSScript *script, uint32 offset)
{
    jsbytecode *pc = script->code + offset;
    JSOp op = (JSOp)*pc;

    InferSpew(ISpewOps, "analyze: #%u:%05u", script->id(), offset);

    



    uint32 stackDepth = state.analysis.getCode(offset).stackDepth;
    if (stackDepth > state.stackDepth) {
#ifdef DEBUG
        



        for (unsigned i = state.stackDepth; i < stackDepth; i++)
            JS_ASSERT(!state.stack[i].isForEach);
#endif
        unsigned ndefs = stackDepth - state.stackDepth;
        memset(&state.stack[state.stackDepth], 0, ndefs * sizeof(AnalyzeStateStack));
    }
    state.stackDepth = stackDepth;

    



    if (state.joinTypes[offset]) {
        MergeTypes(cx, state, script, offset);
        for (unsigned i = 0; i < stackDepth; i++)
            state.stack[i].types = state.joinTypes[offset][i];
    }

    TypeObject *initializer = NULL;

    unsigned defCount = analyze::GetDefCount(script, offset);
    TypeSet *pushed = ArenaArray<TypeSet>(cx->compartment->types.pool, defCount);
    if (!pushed)
        return false;

    JS_ASSERT(!script->types->pushedArray[offset]);
    script->types->pushedArray[offset] = pushed;

    PodZero(pushed, defCount);

    for (unsigned i = 0; i < defCount; i++) {
        pushed[i].setIntermediate();
        InferSpew(ISpewOps, "typeSet: T%p pushed%u #%u:%05u", &pushed[i], i, script->id(), offset);
    }

    
    switch (op) {

        
      case JSOP_POP:
      case JSOP_NOP:
      case JSOP_TRACE:
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
        if (script->compileAndGo) {
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
        
        state.popped(0).types->addSubset(cx, script, &pushed[0]);
        break;

      case JSOP_DUP:
        state.popped(0).types->addSubset(cx, script, &pushed[0]);
        state.popped(0).types->addSubset(cx, script, &pushed[1]);
        break;

      case JSOP_DUP2:
        state.popped(1).types->addSubset(cx, script, &pushed[0]);
        state.popped(0).types->addSubset(cx, script, &pushed[1]);
        state.popped(1).types->addSubset(cx, script, &pushed[2]);
        state.popped(0).types->addSubset(cx, script, &pushed[3]);
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

        





        uint64 start = cx->compartment->types.currentTime();
        JSObject *obj;
        JSProperty *prop;
        js_LookupPropertyWithFlags(cx, script->global, id,
                                   JSRESOLVE_QUALIFIED, &obj, &prop);
        cx->compartment->types.analysisTime -= (cx->compartment->types.currentTime() - start);

        
        PropertyAccess(cx, script, pc, script->getGlobalType(),
                       false, &pushed[0], id);

        if (op == JSOP_CALLGLOBAL || op == JSOP_CALLGNAME)
            pushed[1].addType(cx, TYPE_UNKNOWN);

        if (CheckNextTest(pc))
            pushed[0].addType(cx, TYPE_UNDEFINED);
        break;
      }

      case JSOP_NAME:
      case JSOP_CALLNAME:
        
        if (op == JSOP_CALLNAME)
            pushed[1].addType(cx, TYPE_UNKNOWN);
        break;

      case JSOP_BINDGNAME:
      case JSOP_BINDNAME:
        break;

      case JSOP_SETGNAME: {
        jsid id = GetAtomId(cx, script, pc, 0);
        PropertyAccess(cx, script, pc, script->getGlobalType(),
                       true, state.popped(0).types, id);
        state.popped(0).types->addSubset(cx, script, &pushed[0]);
        break;
      }

      case JSOP_SETNAME:
      case JSOP_SETCONST:
        cx->compartment->types.monitorBytecode(cx, script, offset);
        state.popped(0).types->addSubset(cx, script, &pushed[0]);
        break;

      case JSOP_GETXPROP:
        pushed[0].addType(cx, TYPE_UNKNOWN);
        break;

      case JSOP_INCGNAME:
      case JSOP_DECGNAME:
      case JSOP_GNAMEINC:
      case JSOP_GNAMEDEC: {
        jsid id = GetAtomId(cx, script, pc, 0);
        PropertyAccess(cx, script, pc, script->getGlobalType(), true, NULL, id);
        PropertyAccess(cx, script, pc, script->getGlobalType(), false, &pushed[0], id);
        break;
      }

      case JSOP_INCNAME:
      case JSOP_DECNAME:
      case JSOP_NAMEINC:
      case JSOP_NAMEDEC:
        cx->compartment->types.monitorBytecode(cx, script, offset);
        break;

      case JSOP_GETFCSLOT:
      case JSOP_CALLFCSLOT: {
        unsigned index = GET_UINT16(pc);
        TypeSet *types = script->upvarTypes(index);
        types->addSubset(cx, script, &pushed[0]);
        if (op == JSOP_CALLFCSLOT)
            pushed[1].addType(cx, TYPE_UNDEFINED);
        break;
      }

      case JSOP_GETUPVAR_DBG:
      case JSOP_CALLUPVAR_DBG:
        pushed[0].addType(cx, TYPE_UNKNOWN);
        if (op == JSOP_CALLUPVAR_DBG)
            pushed[1].addType(cx, TYPE_UNDEFINED);
        break;

      case JSOP_GETARG:
      case JSOP_SETARG:
      case JSOP_CALLARG: {
        TypeSet *types = script->argTypes(GET_ARGNO(pc));
        types->addSubset(cx, script, &pushed[0]);
        if (op == JSOP_SETARG)
            state.popped(0).types->addSubset(cx, script, types);
        if (op == JSOP_CALLARG)
            pushed[1].addType(cx, TYPE_UNDEFINED);
        break;
      }

      case JSOP_INCARG:
      case JSOP_DECARG:
      case JSOP_ARGINC:
      case JSOP_ARGDEC: {
        TypeSet *types = script->argTypes(GET_ARGNO(pc));
        types->addArith(cx, script, types);
        types->addSubset(cx, script, &pushed[0]);
        break;
      }

      case JSOP_ARGSUB:
        pushed[0].addType(cx, TYPE_UNKNOWN);
        break;

      case JSOP_GETLOCAL:
      case JSOP_SETLOCAL:
      case JSOP_SETLOCALPOP:
      case JSOP_CALLLOCAL: {
        uint32 local = GET_SLOTNO(pc);
        TypeSet *types = local < script->nfixed ? script->localTypes(local) : NULL;

        if (op != JSOP_SETLOCALPOP) {
            if (types)
                types->addSubset(cx, script, &pushed[0]);
            else
                pushed[0].addType(cx, TYPE_UNKNOWN);
        }
        if (op == JSOP_CALLLOCAL)
            pushed[1].addType(cx, TYPE_UNDEFINED);

        if (op == JSOP_SETLOCAL || op == JSOP_SETLOCALPOP) {
            if (types)
                state.popped(0).types->addSubset(cx, script, types);
        } else {
            



            if (state.analysis.localHasUseBeforeDef(local) ||
                !state.analysis.localDefined(local, pc)) {
                pushed[0].addType(cx, TYPE_UNDEFINED);
            }
        }

        break;
      }

      case JSOP_INCLOCAL:
      case JSOP_DECLOCAL:
      case JSOP_LOCALINC:
      case JSOP_LOCALDEC: {
        uint32 local = GET_SLOTNO(pc);
        TypeSet *types = local < script->nfixed ? script->localTypes(local) : NULL;
        if (types) {
            types->addArith(cx, script, types);
            types->addSubset(cx, script, &pushed[0]);
        } else {
            pushed[0].addType(cx, TYPE_UNKNOWN);
        }
        break;
      }

      case JSOP_ARGUMENTS:
      case JSOP_ARGCNT:
        pushed[0].addType(cx, TYPE_UNKNOWN);
        break;

      case JSOP_SETPROP:
      case JSOP_SETMETHOD: {
        jsid id = GetAtomId(cx, script, pc, 0);
        state.popped(1).types->addSetProperty(cx, script, pc, state.popped(0).types, id);
        state.popped(0).types->addSubset(cx, script, &pushed[0]);
        break;
      }

      case JSOP_GETPROP:
      case JSOP_CALLPROP: {
        jsid id = GetAtomId(cx, script, pc, 0);
        state.popped(0).types->addGetProperty(cx, script, pc, &pushed[0], id);

        if (op == JSOP_CALLPROP)
            state.popped(0).types->addFilterPrimitives(cx, script, &pushed[1], true);
        if (CheckNextTest(pc))
            pushed[0].addType(cx, TYPE_UNDEFINED);
        break;
      }

      case JSOP_INCPROP:
      case JSOP_DECPROP:
      case JSOP_PROPINC:
      case JSOP_PROPDEC: {
        jsid id = GetAtomId(cx, script, pc, 0);
        state.popped(0).types->addGetProperty(cx, script, pc, &pushed[0], id);
        state.popped(0).types->addSetProperty(cx, script, pc, NULL, id);
        break;
      }

      case JSOP_GETTHISPROP: {
        jsid id = GetAtomId(cx, script, pc, 0);

        
        TypeSet *newTypes = TypeSet::make(cx, "thisprop");
        if (!newTypes)
            return false;
        script->thisTypes()->addTransformThis(cx, script, newTypes);
        newTypes->addGetProperty(cx, script, pc, &pushed[0], id);

        if (CheckNextTest(pc))
            pushed[0].addType(cx, TYPE_UNDEFINED);
        break;
      }

      case JSOP_GETARGPROP: {
        TypeSet *types = script->argTypes(GET_ARGNO(pc));

        jsid id = GetAtomId(cx, script, pc, SLOTNO_LEN);
        types->addGetProperty(cx, script, pc, &pushed[0], id);

        if (CheckNextTest(pc))
            pushed[0].addType(cx, TYPE_UNDEFINED);
        break;
      }

      case JSOP_GETLOCALPROP: {
        uint32 local = GET_SLOTNO(pc);
        TypeSet *types = local < script->nfixed ? script->localTypes(local) : NULL;
        if (types) {
            jsid id = GetAtomId(cx, script, pc, SLOTNO_LEN);
            types->addGetProperty(cx, script, pc, &pushed[0], id);
        } else {
            pushed[0].addType(cx, TYPE_UNKNOWN);
        }

        if (CheckNextTest(pc))
            pushed[0].addType(cx, TYPE_UNDEFINED);
        break;
      }

      case JSOP_GETELEM:
      case JSOP_CALLELEM:
        



        state.popped(1).types->addGetProperty(cx, script, pc, &pushed[0], JSID_VOID);

        if (op == JSOP_CALLELEM)
            state.popped(1).types->addFilterPrimitives(cx, script, &pushed[1], true);
        if (CheckNextTest(pc))
            pushed[0].addType(cx, TYPE_UNDEFINED);
        break;

      case JSOP_SETELEM:
        state.popped(2).types->addSetProperty(cx, script, pc, state.popped(0).types, JSID_VOID);
        state.popped(0).types->addSubset(cx, script, &pushed[0]);
        break;

      case JSOP_INCELEM:
      case JSOP_DECELEM:
      case JSOP_ELEMINC:
      case JSOP_ELEMDEC:
        state.popped(1).types->addGetProperty(cx, script, pc, &pushed[0], JSID_VOID);
        state.popped(1).types->addSetProperty(cx, script, pc, NULL, JSID_VOID);
        break;

      case JSOP_LENGTH:
        
        state.popped(0).types->addGetProperty(cx, script, pc, &pushed[0], id_length(cx));
        break;

      case JSOP_THIS:
        script->thisTypes()->addTransformThis(cx, script, &pushed[0]);
        break;

      case JSOP_RETURN:
      case JSOP_SETRVAL:
        if (script->fun)
            state.popped(0).types->addSubset(cx, script, script->returnTypes());
        break;

      case JSOP_ADD:
        state.popped(0).types->addArith(cx, script, &pushed[0], state.popped(1).types);
        state.popped(1).types->addArith(cx, script, &pushed[0], state.popped(0).types);
        break;

      case JSOP_SUB:
      case JSOP_MUL:
      case JSOP_MOD:
      case JSOP_DIV:
        state.popped(0).types->addArith(cx, script, &pushed[0]);
        state.popped(1).types->addArith(cx, script, &pushed[0]);
        break;

      case JSOP_NEG:
      case JSOP_POS:
        state.popped(0).types->addArith(cx, script, &pushed[0]);
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
        if (op == JSOP_LAMBDA || op == JSOP_LAMBDA_FC)
            res = &pushed[0];
        else if (op == JSOP_DEFLOCALFUN || op == JSOP_DEFLOCALFUN_FC)
            res = script->localTypes(GET_SLOTNO(pc));

        if (res) {
            if (script->compileAndGo)
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
        
        unsigned argCount = analyze::GetUseCount(script, offset) - 2;
        TypeCallsite *callsite = ArenaNew<TypeCallsite>(cx->compartment->types.pool,
                                                        cx, script, pc, op == JSOP_NEW, argCount);
        if (!callsite || (argCount && !callsite->argumentTypes)) {
            cx->compartment->types.setPendingNukeTypes(cx);
            break;
        }
        callsite->thisTypes = state.popped(argCount).types;
        callsite->returnTypes = &pushed[0];

        for (unsigned i = 0; i < argCount; i++)
            callsite->argumentTypes[i] = state.popped(argCount - 1 - i).types;

        state.popped(argCount + 1).types->addCall(cx, callsite);
        break;
      }

      case JSOP_NEWINIT:
      case JSOP_NEWARRAY:
      case JSOP_NEWOBJECT:
        if (script->compileAndGo) {
            bool isArray = (op == JSOP_NEWARRAY || (op == JSOP_NEWINIT && pc[1] == JSProto_Array));
            initializer = script->getTypeInitObject(cx, pc, isArray);
            if (!initializer)
                return false;
            pushed[0].addType(cx, (jstype) initializer);
        } else {
            pushed[0].addType(cx, TYPE_UNKNOWN);
        }
        break;

      case JSOP_ENDINIT:
        break;

      case JSOP_INITELEM:
        initializer = state.popped(2).initializer;
        JS_ASSERT((initializer != NULL) == script->compileAndGo);
        if (initializer) {
            pushed[0].addType(cx, (jstype) initializer);
            if (!initializer->unknownProperties) {
                




                TypeSet *types = initializer->getProperty(cx, JSID_VOID, true);
                if (!types)
                    return false;
                if (state.hasGetSet)
                    types->addType(cx, TYPE_UNKNOWN);
                else if (state.hasHole)
                    initializer->markNotPacked(cx, false);
                else
                    state.popped(0).types->addSubset(cx, script, types);
            }
        } else {
            pushed[0].addType(cx, TYPE_UNKNOWN);
        }
        state.hasGetSet = false;
        state.hasHole = false;
        break;

      case JSOP_GETTER:
      case JSOP_SETTER:
        state.hasGetSet = true;
        break;

      case JSOP_HOLE:
        state.hasHole = true;
        break;

      case JSOP_INITPROP:
      case JSOP_INITMETHOD:
        initializer = state.popped(1).initializer;
        JS_ASSERT((initializer != NULL) == script->compileAndGo);
        if (initializer) {
            pushed[0].addType(cx, (jstype) initializer);
            if (!initializer->unknownProperties) {
                jsid id = GetAtomId(cx, script, pc, 0);
                TypeSet *types = initializer->getProperty(cx, id, true);
                if (!types)
                    return false;
                if (id == id___proto__(cx) || id == id_prototype(cx))
                    cx->compartment->types.monitorBytecode(cx, script, offset);
                else if (state.hasGetSet)
                    types->addType(cx, TYPE_UNKNOWN);
                else
                    state.popped(0).types->addSubset(cx, script, types);
            }
        } else {
            pushed[0].addType(cx, TYPE_UNKNOWN);
        }
        state.hasGetSet = false;
        JS_ASSERT(!state.hasHole);
        break;

      case JSOP_ENTERWITH:
      case JSOP_ENTERBLOCK:
        




        break;

      case JSOP_ITER:
        





        state.popped(0).types->addSubset(cx, script, &pushed[0]);
        break;

      case JSOP_MOREITER:
        state.popped(0).types->addSubset(cx, script, &pushed[0]);
        pushed[1].addType(cx, TYPE_BOOLEAN);
        break;

      case JSOP_FORGNAME: {
        jsid id = GetAtomId(cx, script, pc, 0);
        TypeObject *global = script->getGlobalType();
        if (!global->unknownProperties) {
            TypeSet *types = global->getProperty(cx, id, true);
            if (!types)
                return false;
            SetForTypes(cx, script, state, types);
        }
        break;
      }

      case JSOP_FORNAME:
        cx->compartment->types.monitorBytecode(cx, script, offset);
        break;

      case JSOP_FORLOCAL: {
        uint32 local = GET_SLOTNO(pc);
        TypeSet *types = local < script->nfixed ? script->localTypes(local) : NULL;
        if (types)
            SetForTypes(cx, script, state, types);
        break;
      }

      case JSOP_FORARG: {
        TypeSet *types = script->argTypes(GET_ARGNO(pc));
        SetForTypes(cx, script, state, types);
        break;
      }

      case JSOP_FORELEM:
        state.popped(0).types->addSubset(cx, script, &pushed[0]);
        pushed[1].addType(cx, TYPE_UNKNOWN);
        break;

      case JSOP_FORPROP:
      case JSOP_ENUMELEM:
      case JSOP_ENUMCONSTELEM:
        cx->compartment->types.monitorBytecode(cx, script, offset);
        break;

      case JSOP_ARRAYPUSH: {
        TypeSet *types = state.stack[GET_SLOTNO(pc) - script->nfixed].types;
        types->addSetProperty(cx, script, pc, state.popped(0).types, JSID_VOID);
        break;
      }

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
        state.popped(0).types->addSubset(cx, script, &pushed[0]);
        break;

      case JSOP_CASE:
      case JSOP_CASEX:
        state.popped(1).types->addSubset(cx, script, &pushed[0]);
        break;

      case JSOP_UNBRAND:
        state.popped(0).types->addSubset(cx, script, &pushed[0]);
        break;

      case JSOP_GENERATOR:
        if (script->fun) {
            if (script->compileAndGo) {
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
        state.popped(0).types->addSubset(cx, script, &pushed[0]);
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
        
        state.popped(0).types->addSubset(cx, script, &pushed[0]);
        break;

      case JSOP_ENDFILTER:
        state.popped(1).types->addSubset(cx, script, &pushed[0]);
        break;

      case JSOP_DEFSHARP:
        break;

      case JSOP_USESHARP:
        pushed[0].addType(cx, TYPE_UNKNOWN);
        break;

      case JSOP_CALLEE:
        if (script->compileAndGo)
            pushed[0].addType(cx, (jstype) script->fun->getType());
        else
            pushed[0].addType(cx, TYPE_UNKNOWN);
        break;

      case JSOP_TABLESWITCH:
      case JSOP_TABLESWITCHX: {
        jsbytecode *pc2 = pc;
        unsigned jmplen = (op == JSOP_TABLESWITCH) ? JUMP_OFFSET_LEN : JUMPX_OFFSET_LEN;
        unsigned defaultOffset = offset + GetJumpOffset(pc, pc2);
        pc2 += jmplen;
        jsint low = GET_JUMP_OFFSET(pc2);
        pc2 += JUMP_OFFSET_LEN;
        jsint high = GET_JUMP_OFFSET(pc2);
        pc2 += JUMP_OFFSET_LEN;

        MergeTypes(cx, state, script, defaultOffset);

        for (jsint i = low; i <= high; i++) {
            unsigned targetOffset = offset + GetJumpOffset(pc, pc2);
            if (targetOffset != offset)
                MergeTypes(cx, state, script, targetOffset);
            pc2 += jmplen;
        }
        break;
      }

      case JSOP_LOOKUPSWITCH:
      case JSOP_LOOKUPSWITCHX: {
        jsbytecode *pc2 = pc;
        unsigned jmplen = (op == JSOP_LOOKUPSWITCH) ? JUMP_OFFSET_LEN : JUMPX_OFFSET_LEN;
        unsigned defaultOffset = offset + GetJumpOffset(pc, pc2);
        pc2 += jmplen;
        unsigned npairs = GET_UINT16(pc2);
        pc2 += UINT16_LEN;

        MergeTypes(cx, state, script, defaultOffset);

        while (npairs) {
            pc2 += INDEX_LEN;
            unsigned targetOffset = offset + GetJumpOffset(pc, pc2);
            MergeTypes(cx, state, script, targetOffset);
            pc2 += jmplen;
            npairs--;
        }
        break;
      }

      case JSOP_TRY: {
        JSTryNote *tn = script->trynotes()->vector;
        JSTryNote *tnlimit = tn + script->trynotes()->length;
        for (; tn < tnlimit; tn++) {
            unsigned startOffset = script->main - script->code + tn->start;
            if (startOffset == offset + 1) {
                unsigned catchOffset = startOffset + tn->length;
                if (tn->kind != JSTRY_ITER)
                    MergeTypes(cx, state, script, catchOffset);
            }
        }
        break;
      }

      default:
        TypeFailure(cx, "Unknown bytecode: %s", js_CodeNameTwo[op]);
    }

    

    if (op == JSOP_DUP) {
        state.stack[stackDepth] = state.stack[stackDepth - 1];
        state.stackDepth = stackDepth + 1;
    } else if (op == JSOP_DUP2) {
        state.stack[stackDepth]     = state.stack[stackDepth - 2];
        state.stack[stackDepth + 1] = state.stack[stackDepth - 1];
        state.stackDepth = stackDepth + 2;
    } else {
        unsigned nuses = analyze::GetUseCount(script, offset);
        unsigned ndefs = analyze::GetDefCount(script, offset);
        memset(&state.stack[stackDepth - nuses], 0, ndefs * sizeof(AnalyzeStateStack));
        state.stackDepth = stackDepth - nuses + ndefs;
    }

    for (unsigned i = 0; i < defCount; i++)
        state.popped(defCount -1 - i).types = &pushed[i];

    switch (op) {
      case JSOP_ITER: {
        uintN flags = pc[1];
        if (flags & JSITER_FOREACH)
            state.popped(0).isForEach = true;
        break;
      }

      case JSOP_NEWINIT:
      case JSOP_NEWARRAY:
      case JSOP_NEWOBJECT:
      case JSOP_INITELEM:
      case JSOP_INITPROP:
      case JSOP_INITMETHOD:
        state.popped(0).initializer = initializer;
        break;

      default:;
    }

    
    uint32 type = JOF_TYPE(js_CodeSpec[op].format);
    if (type == JOF_JUMP || type == JOF_JUMPX) {
        unsigned targetOffset = offset + GetJumpOffset(pc, pc);
        MergeTypes(cx, state, script, targetOffset);
    }

    return true;
}

void
AnalyzeScriptTypes(JSContext *cx, JSScript *script)
{
    JS_ASSERT(!script->types && !script->isUncachedEval);

    analyze::Script analysis;
    analysis.analyze(cx, script);

    AnalyzeState state(cx, analysis);

    unsigned length = sizeof(TypeScript)
        + (script->length * sizeof(TypeScript*));
    unsigned char *cursor = (unsigned char *) cx->calloc(length);

    if (analysis.failed() || !script->ensureVarTypes(cx) || !state.init(script) || !cursor) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return;
    }

    if (script->analyzed) {
        




        cx->compartment->types.addPendingRecompile(cx, script);
    }

    TypeScript *types = (TypeScript *) cursor;
    script->types = types;
    script->analyzed = true;
#ifdef DEBUG
    types->script = script;
#endif

    cursor += sizeof(TypeScript);
    types->pushedArray = (TypeSet **) cursor;

    if (script->calledWithNew)
        AnalyzeScriptNew(cx, script);

    unsigned offset = 0;
    while (offset < script->length) {
        analyze::Bytecode *code = analysis.maybeCode(offset);

        jsbytecode *pc = script->code + offset;
        analyze::UntrapOpcode untrap(cx, script, pc);

        if (code && !AnalyzeBytecode(cx, state, script, offset)) {
            cx->compartment->types.setPendingNukeTypes(cx);
            return;
        }

        offset += analyze::GetBytecodeLength(pc);
    }

    




    TypeResult *result = script->typeResults;
    while (result) {
        TypeSet *pushed = script->types->pushed(result->offset);
        pushed->addType(cx, result->type);
        result = result->next;
    }
}

void
AnalyzeScriptNew(JSContext *cx, JSScript *script)
{
    JS_ASSERT(script->calledWithNew && script->fun);

    




    if (script->fun->getType()->unknownProperties ||
        script->fun->isFunctionPrototype() ||
        !script->compileAndGo) {
        script->thisTypes()->addType(cx, TYPE_UNKNOWN);
        return;
    }

    TypeFunction *funType = script->fun->getType()->asFunction();
    TypeSet *prototypeTypes = funType->getProperty(cx, id_prototype(cx), false);
    if (!prototypeTypes)
        return;
    prototypeTypes->addNewObject(cx, script, funType, script->thisTypes());
}





#ifdef DEBUG

void
PrintBytecode(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    unsigned offset = pc - script->code;

    JSOp op = (JSOp)*pc;
    JS_ASSERT(op < JSOP_LIMIT);

    const JSCodeSpec *cs = &js_CodeSpec[op];
    const char *name = js_CodeNameTwo[op];

    uint32 type = JOF_TYPE(cs->format);
    switch (type) {
      case JOF_BYTE:
      case JOF_TABLESWITCH:
      case JOF_TABLESWITCHX:
      case JOF_LOOKUPSWITCH:
      case JOF_LOOKUPSWITCHX:
        printf("%s", name);
        break;

      case JOF_JUMP:
      case JOF_JUMPX: {
        ptrdiff_t off = GetJumpOffset(pc, pc);
        printf("%s %u", name, unsigned(offset + off));
        break;
      }

      case JOF_ATOM: {
        if (op == JSOP_DOUBLE) {
            printf("%s", name);
        } else {
            jsid id = GetAtomId(cx, script, pc, 0);
            if (JSID_IS_STRING(id))
                printf("%s %s", name, TypeIdString(id));
            else
                printf("%s (index)", name);
        }
        break;
      }

      case JOF_OBJECT:
        printf("%s (object)", name);
        break;

      case JOF_REGEXP:
        printf("%s (regexp)", name);
        break;

      case JOF_UINT16PAIR:
        printf("%s %d %d", name, GET_UINT16(pc), GET_UINT16(pc + UINT16_LEN));
        break;

      case JOF_UINT16:
        printf("%s %d", name, GET_UINT16(pc));
        break;

      case JOF_QARG:
        printf("%s %d", name, GET_ARGNO(pc));
        break;

      case JOF_GLOBAL:
        printf("%s %s", name, TypeIdString(GetGlobalId(cx, script, pc)));
        break;

      case JOF_LOCAL:
        printf("%s %d", name, GET_SLOTNO(pc));
        break;

      case JOF_SLOTATOM: {
        jsid id = GetAtomId(cx, script, pc, SLOTNO_LEN);

        printf("%s %d %s", name, GET_SLOTNO(pc), TypeIdString(id));
        break;
      }

      case JOF_SLOTOBJECT:
        printf("%s %u (object)", name, GET_SLOTNO(pc));
        break;

      case JOF_UINT24:
        JS_ASSERT(op == JSOP_UINT24 || op == JSOP_NEWARRAY);
        printf("%s %d", name, (jsint)GET_UINT24(pc));
        break;

      case JOF_UINT8:
        printf("%s %d", name, (jsint)pc[1]);
        break;

      case JOF_INT8:
        printf("%s %d", name, (jsint)GET_INT8(pc));
        break;

      case JOF_INT32:
        printf("%s %d", name, (jsint)GET_INT32(pc));
        break;

      default:
        JS_NOT_REACHED("Unknown opcode type");
    }
}

#endif

void
TypeScript::print(JSContext *cx, JSScript *script)
{
    TypeCompartment *compartment = &script->compartment->types;

    



    for (unsigned offset = 0; offset < script->length; offset++) {
        TypeSet *array = pushed(offset);
        if (!array)
            continue;

        unsigned defCount = analyze::GetDefCount(script, offset);
        if (!defCount)
            continue;

        for (unsigned i = 0; i < defCount; i++) {
            TypeSet *types = &array[i];

            
            unsigned typeCount = types->objectCount ? 1 : 0;
            for (jstype type = TYPE_UNDEFINED; type <= TYPE_STRING; type++) {
                if (types->typeFlags & (1 << type))
                    typeCount++;
            }

            




            if (types->typeFlags & TYPE_FLAG_DOUBLE) {
                JS_ASSERT(types->typeFlags & TYPE_FLAG_INT32);
                typeCount--;
            }

            if ((types->typeFlags & TYPE_FLAG_UNKNOWN) ||
                typeCount > TypeCompartment::TYPE_COUNT_LIMIT) {
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
        printf("\n    local%u:", i);
        script->localTypes(i)->print(cx);
    }
    for (unsigned i = 0; i < script->bindings.countUpvars(); i++) {
        printf("\n    upvar%u:", i);
        script->upvarTypes(i)->print(cx);
    }
    printf("\n");

    for (unsigned offset = 0; offset < script->length; offset++) {
        TypeSet *array = pushed(offset);
        if (!array)
            continue;

        printf("#%u:%05u:  ", script->id(), offset);
        PrintBytecode(cx, script, script->code + offset);
        printf("\n");

        unsigned defCount = analyze::GetDefCount(script, offset);
        for (unsigned i = 0; i < defCount; i++) {
            printf("  type %d:", i);
            array[i].print(cx);
            printf("\n");
        }

        if (monitored(offset))
            printf("  monitored\n");
    }

    printf("\n");

#endif 

}

} } 





js::types::TypeFunction *
JSContext::newTypeFunction(const char *name, JSObject *proto)
{
    return (js::types::TypeFunction *) compartment->types.newTypeObject(this, NULL, name, true, proto);
}

js::types::TypeObject *
JSContext::newTypeObject(const char *name, JSObject *proto)
{
    return compartment->types.newTypeObject(this, NULL, name, false, proto);
}

js::types::TypeObject *
JSContext::newTypeObject(const char *base, const char *postfix, JSObject *proto, bool isFunction)
{
    char *name = NULL;
#ifdef DEBUG
    unsigned len = strlen(base) + strlen(postfix) + 5;
    name = (char *)alloca(len);
    JS_snprintf(name, len, "%s:%s", base, postfix);
#endif
    return compartment->types.newTypeObject(this, NULL, name, isFunction, proto);
}









static inline bool
IgnorePushed(JSOp op, unsigned index)
{
    switch (op) {
      
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

      default:
        return false;
    }
}

bool
JSScript::makeVarTypes(JSContext *cx)
{
    JS_ASSERT(!varTypes);

    unsigned nargs = fun ? fun->nargs : 0;
    unsigned count = 2 + nargs + nfixed + bindings.countUpvars();
    varTypes = (js::types::TypeSet *) cx->calloc(sizeof(js::types::TypeSet) * count);
    if (!varTypes)
        return false;

#ifdef DEBUG
    InferSpew(js::types::ISpewOps, "typeSet: T%p return #%u", returnTypes(), id());
    InferSpew(js::types::ISpewOps, "typeSet: T%p this #%u", thisTypes(), id());
    for (unsigned i = 0; i < nargs; i++)
        InferSpew(js::types::ISpewOps, "typeSet: T%p arg%u #%u", argTypes(i), i, id());
    for (unsigned i = 0; i < nfixed; i++)
        InferSpew(js::types::ISpewOps, "typeSet: T%p local%u #%u", localTypes(i), i, id());
    for (unsigned i = 0; i < bindings.countUpvars(); i++)
        InferSpew(js::types::ISpewOps, "typeSet: T%p upvar%u #%u", upvarTypes(i), i, id());
#endif

    return true;
}

bool
JSScript::typeSetFunction(JSContext *cx, JSFunction *fun)
{
    JS_ASSERT(cx->typeInferenceEnabled());

    char *name = NULL;
#ifdef DEBUG
    name = (char *) alloca(10);
    JS_snprintf(name, 10, "#%u", id());
#endif
    js::types::TypeFunction *type = cx->newTypeFunction(name, fun->getProto())->asFunction();
    if (!type)
        return false;

    if (!fun->setTypeAndUniqueShape(cx, type))
        return false;
    type->script = this;
    this->fun = fun;

    return true;
}

#ifdef DEBUG

void
JSScript::typeCheckBytecode(JSContext *cx, const jsbytecode *pc, const js::Value *sp)
{
    if (!types)
        return;

    int defCount = js::analyze::GetDefCount(this, pc - code);
    js::types::TypeSet *array = types->pushed(pc - code);

    for (int i = 0; i < defCount; i++) {
        const js::Value &val = sp[-defCount + i];
        js::types::TypeSet *types = &array[i];
        if (IgnorePushed(JSOp(*pc), i))
            continue;

        js::types::jstype type = js::types::GetValueType(cx, val);

        if (!js::types::TypeSetMatches(cx, types, type)) {
            js::types::TypeFailure(cx, "Missing type at #%u:%05u pushed %u: %s",
                                   id(), pc - code, i, js::types::TypeString(type));
        }

        if (js::types::TypeIsObject(type)) {
            JS_ASSERT(val.isObject());
            JSObject *obj = &val.toObject();
            js::types::TypeObject *object = (js::types::TypeObject *) type;

            if (object->unknownProperties) {
                JS_ASSERT(!object->isDenseArray);
                continue;
            }

            
            JS_ASSERT_IF(object->isPackedArray, object->isDenseArray);
            if (object->isDenseArray) {
                if (!obj->isDenseArray() ||
                    (object->isPackedArray && !obj->isPackedDenseArray())) {
                    js::types::TypeFailure(cx, "Object not %s array at #%u:%05u popped %u: %s",
                        object->isPackedArray ? "packed" : "dense",
                        id(), pc - code, i, object->name());
                }
            }
        }
    }
}

#endif





void
JSObject::makeNewType(JSContext *cx)
{
    JS_ASSERT(!newType);

    js::types::TypeObject *type = cx->newTypeObject(getType()->name(), "new", this);
    if (!type)
        return;

    if (cx->typeInferenceEnabled() && !getType()->unknownProperties) {
        js::types::AutoEnterTypeInference enter(cx);

        
        js::types::TypeSet *types = getType()->getProperty(cx, JSID_EMPTY, true);
        if (types)
            types->addType(cx, (js::types::jstype) type);

        if (!cx->compartment->types.checkPendingRecompiles(cx))
            return;
    }

    newType = type;
    setDelegate();
}





namespace js {
namespace types {

void
types::TypeObject::trace(JSTracer *trc)
{
    JS_ASSERT(!marked);

    



    if (trc->context->runtime->gcMarkAndSweep)
        marked = true;

#ifdef DEBUG
    gc::MarkId(trc, name_, "type_name");
#endif

    if (propertyCount >= 2) {
        unsigned capacity = HashSetCapacity(propertyCount);
        for (unsigned i = 0; i < capacity; i++) {
            Property *prop = propertySet[i];
            if (prop)
                gc::MarkId(trc, prop->id, "type_prop");
        }
    } else if (propertyCount == 1) {
        Property *prop = (Property *) propertySet;
        gc::MarkId(trc, prop->id, "type_prop");
    }

    if (emptyShapes) {
        int count = gc::FINALIZE_OBJECT_LAST - gc::FINALIZE_OBJECT0 + 1;
        for (int i = 0; i < count; i++) {
            if (emptyShapes[i])
                Shape::trace(trc, emptyShapes[i]);
        }
    }

    if (proto)
        gc::MarkObject(trc, *proto, "type_proto");
}






void
CondenseSweepTypeSet(JSContext *cx, TypeCompartment *compartment,
                     HashSet<JSScript*> *pcondensed, TypeSet *types)
{
    






    JS_ASSERT(!(types->typeFlags & TYPE_FLAG_INTERMEDIATE_SET));

    if (types->objectCount >= 2) {
        bool removed = false;
        unsigned objectCapacity = HashSetCapacity(types->objectCount);
        for (unsigned i = 0; i < objectCapacity; i++) {
            TypeObject *object = types->objectSet[i];
            if (object && !object->marked) {
                







                if (object->unknownProperties)
                    types->objectSet[i] = &compartment->typeEmpty;
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
            ::js_free(oldArray);
        }
    } else if (types->objectCount == 1) {
        TypeObject *object = (TypeObject*) types->objectSet;
        if (!object->marked) {
            if (object->unknownProperties) {
                types->objectSet = (TypeObject**) &compartment->typeEmpty;
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

        TypeObject *object = constraint->baseSubset();
        if (object) {
            




            if (object->marked) {
                constraint->next = types->constraintList;
                types->constraintList = constraint;
            } else {
                ::js_free(constraint);
            }
            constraint = next;
            continue;
        }

        



        JSScript *script = constraint->script;
        if (script->isCachedEval ||
            (script->u.object && IsAboutToBeFinalized(cx, script->u.object)) ||
            (script->fun && IsAboutToBeFinalized(cx, script->fun))) {
            if (constraint->condensed())
                ::js_free(constraint);
            constraint = next;
            continue;
        }

        if (pcondensed) {
            HashSet<JSScript*>::AddPtr p = pcondensed->lookupForAdd(script);
            if (!p) {
                if (pcondensed->add(p, script))
                    types->addCondensed(cx, script);
                else
                    compartment->setPendingNukeTypes(cx);
            }
        }

        if (constraint->condensed())
            ::js_free(constraint);
        constraint = next;
    }

    if (pcondensed)
        pcondensed->clear();
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

static void
CondenseTypeObjectList(JSContext *cx, TypeCompartment *compartment, TypeObject *objects)
{
    HashSet<JSScript *> condensed(cx), *pcondensed = &condensed;
    if (!condensed.init()) {
        compartment->setPendingNukeTypes(cx);
        pcondensed = NULL;
    }

    TypeObject *object = objects;
    while (object) {
        if (!object->marked) {
            



            object = object->next;
            continue;
        }

        PruneInstanceObjects(object);

        
        if (object->propertyCount >= 2) {
            unsigned capacity = HashSetCapacity(object->propertyCount);
            for (unsigned i = 0; i < capacity; i++) {
                Property *prop = object->propertySet[i];
                if (prop) {
                    CondenseSweepTypeSet(cx, compartment, pcondensed, &prop->types);
                    CondenseSweepTypeSet(cx, compartment, pcondensed, &prop->ownTypes);
                }
            }
        } else if (object->propertyCount == 1) {
            Property *prop = (Property *) object->propertySet;
            CondenseSweepTypeSet(cx, compartment, pcondensed, &prop->types);
            CondenseSweepTypeSet(cx, compartment, pcondensed, &prop->ownTypes);
        }

        object = object->next;
    }
}

void
TypeCompartment::condense(JSContext *cx)
{
    PruneInstanceObjects(&typeEmpty);

    CondenseTypeObjectList(cx, this, objects);
}

static void
DestroyProperty(JSContext *cx, Property *prop)
{
    prop->types.destroy(cx);
    prop->ownTypes.destroy(cx);
    cx->free(prop);
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
                cx->free(object->emptyShapes);
            *pobject = object->next;

            if (object->propertyCount >= 2) {
                unsigned capacity = HashSetCapacity(object->propertyCount);
                for (unsigned i = 0; i < capacity; i++) {
                    Property *prop = object->propertySet[i];
                    if (prop)
                        DestroyProperty(cx, prop);
                }
                ::js_free(object->propertySet);
            } else if (object->propertyCount == 1) {
                Property *prop = (Property *) object->propertySet;
                DestroyProperty(cx, prop);
            }

            cx->free(object);
        }
    }
}

void
TypeCompartment::sweep(JSContext *cx)
{
    if (typeEmpty.marked) {
        typeEmpty.marked = false;
    } else if (typeEmpty.emptyShapes) {
        cx->free(typeEmpty.emptyShapes);
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
            if (!entry.object->marked || !entry.newShape->marked())
                remove = true;
            for (unsigned i = 0; !remove && i < key.nslots; i++) {
                if (JSID_IS_STRING(key.ids[i])) {
                    JSString *str = JSID_TO_STRING(key.ids[i]);
                    if (!JSString::isStatic(str) && !str->asCell()->isMarked())
                        remove = true;
                }
                if (TypeIsObject(entry.types[i]) && !((TypeObject *)entry.types[i])->marked)
                    remove = true;
            }

            if (remove) {
                cx->free(key.ids);
                cx->free(entry.types);
                e.removeFront();
            }
        }
    }

    SweepTypeObjectList(cx, objects);
}

TypeCompartment::~TypeCompartment()
{
    if (pendingArray)
        js_free(pendingArray);

    if (arrayTypeTable)
        js_delete<ArrayTypeTable>(arrayTypeTable);

    if (objectTypeTable)
        js_delete<ObjectTypeTable>(objectTypeTable);
}

} } 

void
JSScript::condenseTypes(JSContext *cx)
{
    js::types::CondenseTypeObjectList(cx, &compartment->types, typeObjects);

    if (varTypes) {
        js::HashSet<JSScript *> condensed(cx), *pcondensed = &condensed;
        if (!condensed.init()) {
            compartment->types.setPendingNukeTypes(cx);
            pcondensed = NULL;
        }

        unsigned num = 2 + nfixed + (fun ? fun->nargs : 0) + bindings.countUpvars();

        if (isCachedEval ||
            (u.object && IsAboutToBeFinalized(cx, u.object)) ||
            (fun && IsAboutToBeFinalized(cx, fun))) {
            for (unsigned i = 0; i < num; i++)
                varTypes[i].destroy(cx);
            cx->free(varTypes);
            varTypes = NULL;
        } else {
            for (unsigned i = 0; i < num; i++)
                js::types::CondenseSweepTypeSet(cx, &compartment->types, pcondensed, &varTypes[i]);
        }
    }

    js::types::TypeResult **presult = &typeResults;
    while (*presult) {
        js::types::TypeResult *result = *presult;
        if (js::types::TypeIsObject(result->type)) {
            js::types::TypeObject *object = (js::types::TypeObject *) result->type;
            if (!object->marked) {
                if (!object->unknownProperties) {
                    *presult = result->next;
                    cx->free(result);
                    continue;
                } else {
                    result->type = (js::types::jstype) &compartment->types.typeEmpty;
                }
            }
        }
        presult = &result->next;
    }
}

void
JSScript::sweepTypes(JSContext *cx)
{
    SweepTypeObjectList(cx, typeObjects);

    if (types && !compartment->types.inferenceDepth) {
        cx->free(types);
        types = NULL;
    }
}
