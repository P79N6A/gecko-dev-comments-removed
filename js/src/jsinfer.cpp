






































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

#include <zlib.h>

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

#ifdef DEBUG
unsigned TypeSet::typesetCount = 0;
unsigned TypeConstraint::constraintCount = 0;
#endif

static const char *js_CodeNameTwo[] = {
#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format) \
    name,
#include "jsopcode.tbl"
#undef OPDEF
};





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
        if (strstr(env, "dynamic"))
            active[ISpewDynamic] = true;
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
TypeObjectString(TypeObject *object)
{
    return TypeIdString(object->name);
}

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
        return TypeIdString(object->name);
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

#endif

void TypeFailure(JSContext *cx, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stdout, "[infer failure] ");
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    va_end(ap);

    cx->compartment->types.finish(cx, cx->compartment);

    fflush(stdout);
    *((int*)NULL) = 0;  
}





inline void
TypeSet::add(JSContext *cx, TypeConstraint *constraint, bool callExisting)
{
    InferSpew(ISpewOps, "addConstraint: T%u C%u %s",
              id(), constraint->id(), constraint->kind());

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
    if (typeFlags == 0) {
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

    if (typeFlags & TYPE_FLAG_OBJECT) {
        printf(" object[%u]", objectCount);

        if (objectCount >= 2) {
            unsigned objectCapacity = HashSetCapacity(objectCount);
            for (unsigned i = 0; i < objectCapacity; i++) {
                TypeObject *object = objectSet[i];
                if (object)
                    printf(" %s", TypeIdString(object->name));
            }
        } else if (objectCount == 1) {
            TypeObject *object = (TypeObject*) objectSet;
            printf(" %s", TypeIdString(object->name));
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

    void newType(JSContext *cx, TypeSet *source, jstype type);
};

void
TypeSet::addSubset(JSContext *cx, JSArenaPool &pool, TypeSet *target)
{
    JS_ASSERT(this->pool == &pool);
    add(cx, ArenaNew<TypeConstraintSubset>(pool, target));
}


class TypeConstraintProp : public TypeConstraint
{
public:
    
    analyze::Bytecode *code;

    



    bool assign;
    TypeSet *target;

    
    jsid id;

    TypeConstraintProp(analyze::Bytecode *code, TypeSet *target, jsid id, bool assign)
        : TypeConstraint("prop"), code(code), assign(assign), target(target), id(id)
    {
        JS_ASSERT(code);
        JS_ASSERT(id == MakeTypeId(id));

        
        JS_ASSERT_IF(!target, assign);
    }

    void newType(JSContext *cx, TypeSet *source, jstype type);
};

void
TypeSet::addGetProperty(JSContext *cx, analyze::Bytecode *code, TypeSet *target, jsid id)
{
    JS_ASSERT(this->pool == &code->pool());
    add(cx, ArenaNew<TypeConstraintProp>(code->pool(), code, target, id, false));
}

void
TypeSet::addSetProperty(JSContext *cx, analyze::Bytecode *code, TypeSet *target, jsid id)
{
    JS_ASSERT(this->pool == &code->pool());
    add(cx, ArenaNew<TypeConstraintProp>(code->pool(), code, target, id, true));
}





class TypeConstraintElem : public TypeConstraint
{
public:
    
    analyze::Bytecode *code;

    
    TypeSet *object;

    
    TypeSet *target;

    
    bool assign;

    TypeConstraintElem(analyze::Bytecode *code, TypeSet *object, TypeSet *target, bool assign)
        : TypeConstraint("elem"), code(code), object(object), target(target), assign(assign)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type);
};

void
TypeSet::addGetElem(JSContext *cx, analyze::Bytecode *code, TypeSet *object, TypeSet *target)
{
    JS_ASSERT(this->pool == &code->pool());
    add(cx, ArenaNew<TypeConstraintElem>(code->pool(), code, object, target, false));
}

void
TypeSet::addSetElem(JSContext *cx, analyze::Bytecode *code, TypeSet *object, TypeSet *target)
{
    JS_ASSERT(this->pool == &code->pool());
    add(cx, ArenaNew<TypeConstraintElem>(code->pool(), code, object, target, true));
}






class TypeConstraintCall : public TypeConstraint
{
public:
    
    TypeCallsite *callsite;

    TypeConstraintCall(TypeCallsite *callsite)
        : TypeConstraint("call"), callsite(callsite)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type);
};

void
TypeSet::addCall(JSContext *cx, TypeCallsite *site)
{
    JS_ASSERT(this->pool == &site->pool());
    add(cx, ArenaNew<TypeConstraintCall>(site->pool(), site));
}


class TypeConstraintArith : public TypeConstraint
{
public:
    
    analyze::Bytecode *code;

    
    TypeSet *target;

    
    TypeSet *other;

    TypeConstraintArith(analyze::Bytecode *code, TypeSet *target, TypeSet *other)
        : TypeConstraint("arith"), code(code), target(target), other(other)
    {
        JS_ASSERT(code);
        JS_ASSERT(target);
    }

    void newType(JSContext *cx, TypeSet *source, jstype type);
};

void
TypeSet::addArith(JSContext *cx, JSArenaPool &pool, analyze::Bytecode *code,
                  TypeSet *target, TypeSet *other)
{
    JS_ASSERT(this->pool == &pool);
    add(cx, ArenaNew<TypeConstraintArith>(pool, code, target, other));
}


class TypeConstraintTransformThis : public TypeConstraint
{
public:
    TypeSet *target;

    TypeConstraintTransformThis(TypeSet *target)
        : TypeConstraint("transformthis"), target(target)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type);
};

void
TypeSet::addTransformThis(JSContext *cx, JSArenaPool &pool, TypeSet *target)
{
    JS_ASSERT(this->pool == &pool);
    add(cx, ArenaNew<TypeConstraintTransformThis>(pool, target));
}


class TypeConstraintFilterPrimitive : public TypeConstraint
{
public:
    TypeSet *target;

    
    bool onlyNullVoid;

    TypeConstraintFilterPrimitive(TypeSet *target, bool onlyNullVoid)
        : TypeConstraint("filter"), target(target), onlyNullVoid(onlyNullVoid)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type);
};

void
TypeSet::addFilterPrimitives(JSContext *cx, JSArenaPool &pool, TypeSet *target, bool onlyNullVoid)
{
    JS_ASSERT(this->pool == &pool);
    add(cx, ArenaNew<TypeConstraintFilterPrimitive>(pool, target, onlyNullVoid));
}





class TypeConstraintMonitorRead : public TypeConstraint
{
public:
    analyze::Bytecode *code;
    TypeSet *target;

    TypeConstraintMonitorRead(analyze::Bytecode *code, TypeSet *target)
        : TypeConstraint("monitorRead"), code(code), target(target)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type);
};

void
TypeSet::addMonitorRead(JSContext *cx, JSArenaPool &pool, analyze::Bytecode *code, TypeSet *target)
{
    JS_ASSERT(this->pool == &pool);
    add(cx, ArenaNew<TypeConstraintMonitorRead>(pool, code, target));
}





class TypeConstraintGenerator : public TypeConstraint
{
public:
    TypeSet *target;

    TypeConstraintGenerator(TypeSet *target)
        : TypeConstraint("generator"), target(target)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type);
};


static inline void
SetForTypes(JSContext *cx, const analyze::Script::AnalyzeState &state,
            analyze::Bytecode *code, TypeSet *types)
{
    JS_ASSERT(code->stackDepth == state.stackDepth);
    if (state.popped(0).isForEach)
        types->addType(cx, TYPE_UNKNOWN);
    else
        types->addType(cx, TYPE_STRING);

    code->popped(0)->add(cx, ArenaNew<TypeConstraintGenerator>(code->pool(), types));
}





void
TypeConstraintSubset::newType(JSContext *cx, TypeSet *source, jstype type)
{
    
    target->addType(cx, type);
}


static inline TypeObject *
GetPropertyObject(JSContext *cx, jstype type)
{
    if (TypeIsObject(type))
        return (TypeObject*) type;

    



    switch (type) {

      case TYPE_INT32:
      case TYPE_DOUBLE:
        if (!cx->globalObject->getReservedSlot(JSProto_Number).isObject())
            js_InitNumberClass(cx, cx->globalObject);
        return cx->getFixedTypeObject(TYPE_OBJECT_NEW_NUMBER);

      case TYPE_BOOLEAN:
        if (!cx->globalObject->getReservedSlot(JSProto_Boolean).isObject())
            js_InitBooleanClass(cx, cx->globalObject);
        return cx->getFixedTypeObject(TYPE_OBJECT_NEW_BOOLEAN);

      case TYPE_STRING:
        if (!cx->globalObject->getReservedSlot(JSProto_String).isObject())
            js_InitStringClass(cx, cx->globalObject);
        return cx->getFixedTypeObject(TYPE_OBJECT_NEW_STRING);

      default:
        
        return NULL;
    }
}





static inline void
PropertyAccess(JSContext *cx, analyze::Bytecode *code, TypeObject *object,
               bool assign, TypeSet *target, jsid id)
{
    JS_ASSERT_IF(!target, assign);

    
    if (object->unknownProperties) {
        if (!assign)
            target->addType(cx, TYPE_UNKNOWN);
        return;
    }

    
    if (assign && id == id_prototype(cx)) {
        cx->compartment->types.monitorBytecode(cx, code);
        return;
    }

    
    if (id == id___proto__(cx) || id == id_constructor(cx) || id == id_caller(cx)) {
        if (assign)
            cx->compartment->types.monitorBytecode(cx, code);
        else
            target->addType(cx, TYPE_UNKNOWN);
        return;
    }

    
    if (assign && !JSID_IS_VOID(id))
        cx->markTypeArrayNotPacked(object, true, false);

    
    if (target) {
        TypeSet *types = object->getProperty(cx, id, assign);
        if (assign)
            target->addSubset(cx, code->pool(), types);
        else
            types->addMonitorRead(cx, *object->pool, code, target);
    } else {
        TypeSet *readTypes = object->getProperty(cx, id, false);
        TypeSet *writeTypes = object->getProperty(cx, id, true);
        if (code->hasIncDecOverflow)
            writeTypes->addType(cx, TYPE_DOUBLE);
        readTypes->addArith(cx, *object->pool, code, writeTypes);
    }
}

void
TypeConstraintProp::newType(JSContext *cx, TypeSet *source, jstype type)
{
    if (type == TYPE_UNKNOWN) {
        





        if (assign)
            cx->compartment->types.monitorBytecode(cx, code);
        else
            target->addType(cx, TYPE_UNKNOWN);
        return;
    }

    TypeObject *object = GetPropertyObject(cx, type);
    if (object)
        PropertyAccess(cx, code, object, assign, target, id);
}

void
TypeConstraintElem::newType(JSContext *cx, TypeSet *source, jstype type)
{
    switch (type) {
      case TYPE_UNDEFINED:
      case TYPE_BOOLEAN:
      case TYPE_NULL:
      case TYPE_INT32:
      case TYPE_DOUBLE:
        





        if (assign)
            object->addSetProperty(cx, code, target, JSID_VOID);
        else
            object->addGetProperty(cx, code, target, JSID_VOID);
        break;
      default:
        



        if (assign)
            cx->compartment->types.monitorBytecode(cx, code);
        else
            target->addType(cx, TYPE_UNKNOWN);
    }
};

class TypeConstraintNewObject : public TypeConstraint
{
    TypeSet *target;

  public:
    TypeConstraintNewObject(TypeSet *target)
        : TypeConstraint("newObject"), target(target)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type)
    {
        if (type == TYPE_UNKNOWN) {
            target->addType(cx, TYPE_UNKNOWN);
            return;
        }

        
        if (TypeIsObject(type)) {
            TypeObject *object = (TypeObject *) type;
            target->addType(cx, (jstype) object->getNewObject(cx));
        }
    }
};

void
TypeConstraintCall::newType(JSContext *cx, TypeSet *source, jstype type)
{
    if (type == TYPE_UNKNOWN) {
        
        cx->compartment->types.monitorBytecode(cx, callsite->code);
        return;
    }

    
    TypeFunction *function = NULL;
    if (TypeIsObject(type)) {
        TypeObject *object = (TypeObject*) type;
        if (object->isFunction) {
            function = (TypeFunction*) object;
        } else {
            
            cx->compartment->types.monitorBytecode(cx, callsite->code);
        }
    }
    if (!function)
        return;

    JSArenaPool &pool = callsite->pool();

    if (!function->script) {
        JS_ASSERT(function->handler);

        if (function->handler == JS_TypeHandlerDynamic) {
            
            if (callsite->returnTypes)
                callsite->returnTypes->addType(cx, TYPE_UNKNOWN);
        } else if (function->isGeneric) {
            if (callsite->argumentCount == 0) {
                
                return;
            }

            





            TypeSet *thisTypes = TypeSet::make(cx, pool, "genericthis");
            callsite->argumentTypes[0]->addTransformThis(cx, pool, thisTypes);

            TypeCallsite *newSite = ArenaNew<TypeCallsite>(pool, callsite->code, callsite->isNew,
                                                           callsite->argumentCount - 1);
            newSite->thisTypes = thisTypes;
            newSite->returnTypes = callsite->returnTypes;
            for (unsigned i = 0; i < callsite->argumentCount - 1; i++)
                newSite->argumentTypes[i] = callsite->argumentTypes[i + 1];

            function->handler(cx, (JSTypeFunction*)function, (JSTypeCallsite*)newSite);
        } else {
            
            function->handler(cx, (JSTypeFunction*)function, (JSTypeCallsite*)callsite);
        }

        




        if (callsite->isNew && callsite->returnTypes &&
            function != cx->getFixedTypeObject(TYPE_OBJECT_OBJECT) &&
            function != cx->getFixedTypeObject(TYPE_OBJECT_FUNCTION) &&
            function != cx->getFixedTypeObject(TYPE_OBJECT_ARRAY)) {
            if (!function->prototypeObject)
                function->getProperty(cx, id_prototype(cx), false);
            TypeObject *object = function->prototypeObject->getNewObject(cx);
            if (callsite->returnTypes)
                callsite->returnTypes->addType(cx, (jstype) object);
        }

        return;
    }

    analyze::Script *script = function->script->analysis;

    
    for (unsigned i = 0; i < callsite->argumentCount; i++) {
        TypeSet *argTypes = callsite->argumentTypes[i];
        jsid id = script->getArgumentId(i);

        if (!JSID_IS_VOID(id)) {
            TypeSet *types = script->getVariable(cx, id);
            argTypes->addSubset(cx, pool, types);
        } else {
            




        }
    }

    
    for (unsigned i = callsite->argumentCount; i < script->argCount(); i++) {
        jsid id = script->getArgumentId(i);
        TypeSet *types = script->getVariable(cx, id);
        types->addType(cx, TYPE_UNDEFINED);
    }

    
    if (callsite->isNew) {
        
        if (function->unknownProperties) {
            script->thisTypes.addType(cx, TYPE_UNKNOWN);
        } else {
            TypeSet *prototypeTypes = function->getProperty(cx, id_prototype(cx), false);
            prototypeTypes->add(cx,
                ArenaNew<TypeConstraintNewObject>(*function->pool, &script->thisTypes));
        }

        



        if (callsite->returnTypes) {
            script->thisTypes.addSubset(cx, script->pool, callsite->returnTypes);
            function->returnTypes.addFilterPrimitives(cx, *function->pool,
                                                      callsite->returnTypes, false);
        }
    } else {
        if (callsite->thisTypes) {
            
            callsite->thisTypes->addSubset(cx, pool, &script->thisTypes);
        } else {
            JS_ASSERT(callsite->thisType != TYPE_NULL);
            script->thisTypes.addType(cx, callsite->thisType);
        }

        
        if (callsite->returnTypes)
            function->returnTypes.addSubset(cx, *function->pool, callsite->returnTypes);
    }

    
    if (!script->hasAnalyzed())
        script->analyze(cx);
}

void
TypeConstraintArith::newType(JSContext *cx, TypeSet *source, jstype type)
{
    if (other) {
        





        switch (type) {
          case TYPE_UNDEFINED:
          case TYPE_NULL:
          case TYPE_INT32:
          case TYPE_BOOLEAN:
            
            if (other->typeFlags & (TYPE_FLAG_UNDEFINED | TYPE_FLAG_NULL |
                                    TYPE_FLAG_INT32 | TYPE_FLAG_BOOLEAN))
                target->addType(cx, TYPE_INT32);
            if (other->typeFlags & TYPE_FLAG_DOUBLE)
                target->addType(cx, TYPE_DOUBLE);
            break;
          case TYPE_DOUBLE:
            if (other->typeFlags & (TYPE_FLAG_UNDEFINED | TYPE_FLAG_NULL |
                                    TYPE_FLAG_INT32 | TYPE_FLAG_DOUBLE | TYPE_FLAG_BOOLEAN))
                target->addType(cx, TYPE_DOUBLE);
            break;
          case TYPE_STRING:
            target->addType(cx, TYPE_STRING);
            break;
          default:
            



            target->addType(cx, TYPE_UNKNOWN);
            break;
        }
    } else {
        
        switch (type) {
          case TYPE_UNDEFINED:
          case TYPE_NULL:
          case TYPE_INT32:
          case TYPE_BOOLEAN:
            target->addType(cx, TYPE_INT32);
            break;
          case TYPE_DOUBLE:
            target->addType(cx, TYPE_DOUBLE);
            break;
          default:
            target->addType(cx, TYPE_UNKNOWN);
            break;
        }
    }
}

void
TypeConstraintTransformThis::newType(JSContext *cx, TypeSet *source, jstype type)
{
    if (type == TYPE_UNKNOWN || TypeIsObject(type)) {
        target->addType(cx, type);
        return;
    }

    
    TypeObject *object = NULL;
    switch (type) {
      case TYPE_NULL:
      case TYPE_UNDEFINED:
        object = cx->getGlobalTypeObject();
        break;
      case TYPE_INT32:
      case TYPE_DOUBLE:
        object = cx->getFixedTypeObject(TYPE_OBJECT_NEW_NUMBER);
        break;
      case TYPE_BOOLEAN:
        object = cx->getFixedTypeObject(TYPE_OBJECT_NEW_BOOLEAN);
        break;
      case TYPE_STRING:
        object = cx->getFixedTypeObject(TYPE_OBJECT_NEW_STRING);
        break;
      default:
        JS_NOT_REACHED("Bad type");
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
    if (type == (jstype) cx->getFixedTypeObject(TYPE_OBJECT_GETSET)) {
        target->addType(cx, TYPE_UNKNOWN);
        return;
    }

    target->addType(cx, type);
}

void
TypeConstraintGenerator::newType(JSContext *cx, TypeSet *source, jstype type)
{
    if (type == TYPE_UNKNOWN)
        target->addType(cx, TYPE_UNKNOWN);

    if (type == (jstype) cx->getFixedTypeObject(TYPE_OBJECT_NEW_ITERATOR) ||
        type == (jstype) cx->getFixedTypeObject(TYPE_OBJECT_NEW_GENERATOR)) {
        target->addType(cx, TYPE_UNKNOWN);
    }
}


class TypeConstraintPossiblyPacked : public TypeConstraint
{
public:
    TypeConstraintPossiblyPacked() : TypeConstraint("possiblyPacked") {}

    void newType(JSContext *cx, TypeSet *source, jstype type)
    {
        if (type != TYPE_UNKNOWN && TypeIsObject(type)) {
            TypeObject *object = (TypeObject *) type;
            object->possiblePackedArray = true;
        }
    }
};









class TypeConstraintFreezeTypeTag : public TypeConstraint
{
public:
    JSScript *script;

    



    bool typeUnknown;

    TypeConstraintFreezeTypeTag(JSScript *script)
        : TypeConstraint("freezeTypeTag"),
          script(script), typeUnknown(false)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type)
    {
        if (typeUnknown)
            return;

        if (type != TYPE_UNKNOWN && TypeIsObject(type)) {
            
            if (source->objectCount >= 2) {
                JS_ASSERT(source->typeFlags == TYPE_FLAG_OBJECT);
                return;
            }
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
      case TYPE_FLAG_OBJECT:
        return JSVAL_TYPE_OBJECT;
      default:
        return JSVAL_TYPE_UNKNOWN;
    }
}

JSValueType
TypeSet::getKnownTypeTag(JSContext *cx, JSScript *script)
{
    JSValueType type = GetValueTypeFromTypeFlags(typeFlags);

    if (script && type != JSVAL_TYPE_UNKNOWN) {
        JS_ASSERT(this->pool == &script->analysis->pool);
        add(cx, ArenaNew<TypeConstraintFreezeTypeTag>(script->analysis->pool, script), false);
    }

    return type;
}


static inline ObjectKind
CombineObjectKind(TypeObject *object, ObjectKind kind)
{
    






    if (object->isPackedArray && !object->possiblePackedArray) {
        InferSpew(ISpewDynamic, "Possible unpacked array: %s", TypeIdString(object->name));
        object->isPackedArray = false;
    }

    ObjectKind nkind;
    if (object->isFunction)
        nkind = object->asFunction()->script ? OBJECT_SCRIPTED_FUNCTION : OBJECT_NATIVE_FUNCTION;
    else if (object->isPackedArray)
        nkind = OBJECT_PACKED_ARRAY;
    else if (object->isDenseArray)
        nkind = OBJECT_DENSE_ARRAY;
    else
        nkind = OBJECT_UNKNOWN;

    if (kind == OBJECT_UNKNOWN || nkind == OBJECT_UNKNOWN)
        return OBJECT_UNKNOWN;

    if (kind == nkind || kind == OBJECT_NONE)
        return nkind;

    if ((kind == OBJECT_PACKED_ARRAY && nkind == OBJECT_DENSE_ARRAY) ||
        (kind == OBJECT_DENSE_ARRAY && nkind == OBJECT_PACKED_ARRAY)) {
        return OBJECT_DENSE_ARRAY;
    }

    return OBJECT_UNKNOWN;
}


class TypeConstraintFreezeArray : public TypeConstraint
{
public:
    



    ObjectKind *pkind;

    JSScript *script;

    TypeConstraintFreezeArray(ObjectKind *pkind, JSScript *script)
        : TypeConstraint("freezeArray"),
          pkind(pkind), script(script)
    {
        JS_ASSERT(*pkind == OBJECT_PACKED_ARRAY || *pkind == OBJECT_DENSE_ARRAY);
    }

    void newType(JSContext *cx, TypeSet *source, jstype type) {}

    void arrayNotPacked(JSContext *cx, bool notDense)
    {
        if (*pkind == OBJECT_UNKNOWN) {
            
            return;
        }

        JS_ASSERT(*pkind == OBJECT_PACKED_ARRAY || *pkind == OBJECT_DENSE_ARRAY);

        if (!notDense && *pkind == OBJECT_DENSE_ARRAY) {
            
            return;
        }

        cx->compartment->types.addPendingRecompile(cx, script);
    }
};





class TypeConstraintFreezeObjectKind : public TypeConstraint
{
public:
    ObjectKind kind;
    JSScript *script;

    TypeConstraintFreezeObjectKind(ObjectKind kind, JSScript *script)
        : TypeConstraint("freezeObjectKind"),
          kind(kind), script(script)
    {}

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

            if (nkind != OBJECT_UNKNOWN &&
                (kind == OBJECT_PACKED_ARRAY || kind == OBJECT_DENSE_ARRAY)) {
                




                TypeSet *elementTypes = object->getProperty(cx, JSID_VOID, false);
                elementTypes->add(cx,
                    ArenaNew<TypeConstraintFreezeArray>(*object->pool, &kind, script), false);
            }

            if (nkind == kind) {
                
                return;
            }
            kind = nkind;

            cx->compartment->types.addPendingRecompile(cx, script);
        }
    }
};

ObjectKind
TypeSet::getKnownObjectKind(JSContext *cx, JSScript *script)
{
    JS_ASSERT(this->pool == &script->analysis->pool);

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
    }

    if (kind != OBJECT_UNKNOWN) {
        



        add(cx, ArenaNew<TypeConstraintFreezeObjectKind>(script->analysis->pool,
                                                         kind, script), true);
    }

    return kind;
}





class TypeConstraintFreezeGetSet : public TypeConstraint
{
public:
    JSScript *script;
    bool hasGetSet;

    TypeConstraintFreezeGetSet(JSScript *script)
        : TypeConstraint("freezeGetSet"),
          script(script), hasGetSet(false)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type)
    {
        if (hasGetSet)
            return;

        if (type != TYPE_UNKNOWN) {
            if (!TypeIsObject(type))
                return;
            if (cx->getFixedTypeObject(TYPE_OBJECT_GETSET) != (TypeObject *) type)
                return;
        }

        hasGetSet = true;

        cx->compartment->types.addPendingRecompile(cx, script);
    }
};

bool
TypeSet::hasGetterSetter(JSContext *cx, JSScript *script)
{
    TypeObject *getset = cx->getFixedTypeObject(TYPE_OBJECT_GETSET);

    if (objectCount >= 2) {
        unsigned objectCapacity = HashSetCapacity(objectCount);
        for (unsigned i = 0; i < objectCapacity; i++) {
            if (getset == objectSet[i])
                return true;
        }
    } else if (objectCount == 1) {
        if (getset == (TypeObject *) objectSet)
            return true;
    }

    add(cx, ArenaNew<TypeConstraintFreezeGetSet>(script->analysis->pool, script), true);

    return false;
}













const char * const fixedTypeObjectNames[] = {
    "Object",
    "Function",
    "Array",
    "Function:prototype",
    "#EmptyFunction",
    "Object:prototype",
    "Array:prototype",
    "Boolean:prototype:new",
    "Number:prototype:new",
    "String:prototype:new",
    "RegExp:prototype:new",
    "Iterator:prototype:new",
    "Generator:prototype:new",
    "ArrayBuffer:prototype:new",
    "#XML",
    "#Arguments",
    "#NoSuchMethod",
    "#NoSuchMethodArguments",
    "#PropertyDescriptor",
    "#KeyValuePair",
    "#JSON",
    "#Proxy",
    "#RegExpMatchArray",
    "#StringSplitArray",
    "#UnknownArray",
    "#CloneArray",
    "#PropertyArray",
    "#ReflectArray",
    "#UnknownObject",
    "#CloneObject",
    "#ReflectObject",
    "#XMLSettings",
    "#GetSet",
    "#RegExpStatics",
    "#Call",
    "#DeclEnv",
    "#SharpArray",
    "#With",
    "#Block",
    "#NullClosure",
    "#PropertyIterator",
    "#Script"
};

JS_STATIC_ASSERT(JS_ARRAY_LENGTH(fixedTypeObjectNames) == TYPE_OBJECT_FIXED_LIMIT);

void
TypeCompartment::init()
{
    PodZero(this);

    JS_InitArenaPool(&pool, "typeinfer", 512, 8, NULL);

    objectNameTable = new ObjectNameTable();
#ifdef DEBUG
    bool success =
#endif
        objectNameTable->init();
    JS_ASSERT(success);
}

TypeCompartment::~TypeCompartment()
{
    
    JS_FinishArenaPool(&pool);

    delete objectNameTable;
    JS_ASSERT(!pendingRecompiles);
}

void 
TypeCompartment::growPendingArray()
{
    pendingCapacity = js::Max(unsigned(100), pendingCapacity * 2);
    PendingWork *oldArray = pendingArray;
    pendingArray = ArenaArray<PendingWork>(pool, pendingCapacity);
    memcpy(pendingArray, oldArray, pendingCount * sizeof(PendingWork));
}

TypeObject *
TypeCompartment::makeFixedTypeObject(JSContext *cx, FixedTypeObjectName which)
{
    const char *name = fixedTypeObjectNames[which];
    switch (which) {

      case TYPE_OBJECT_OBJECT:
      case TYPE_OBJECT_FUNCTION:
      case TYPE_OBJECT_ARRAY:
        return cx->getTypeFunction(name);
      case TYPE_OBJECT_FUNCTION_PROTOTYPE: {
        TypeObject *proto = cx->getFixedTypeObject(TYPE_OBJECT_OBJECT_PROTOTYPE);
        return cx->getTypeFunctionHandler(name, JS_TypeHandlerVoid, proto);
      }
      case TYPE_OBJECT_EMPTY_FUNCTION:
        return cx->getTypeFunction(name, NULL);

      case TYPE_OBJECT_OBJECT_PROTOTYPE:
        return getTypeObject(cx, NULL, name, false, NULL);
      case TYPE_OBJECT_ARRAY_PROTOTYPE:
        return cx->getTypeObject(name, NULL);
      case TYPE_OBJECT_NEW_BOOLEAN:
        return cx->getTypeObject(name, cx->getTypeObject("Boolean:prototype", NULL));
      case TYPE_OBJECT_NEW_NUMBER:
        return cx->getTypeObject(name, cx->getTypeObject("Number:prototype", NULL));
      case TYPE_OBJECT_NEW_STRING:
        return cx->getTypeObject(name, cx->getTypeObject("String:prototype", NULL));
      case TYPE_OBJECT_NEW_REGEXP:
        return cx->getTypeObject(name, cx->getTypeObject("RegExp:prototype", NULL));
      case TYPE_OBJECT_NEW_ITERATOR:
        return cx->getTypeObject(name, cx->getTypeObject("Iterator:prototype", NULL));
      case TYPE_OBJECT_NEW_GENERATOR:
        return cx->getTypeObject(name, cx->getTypeObject("Generator:prototype", NULL));
      case TYPE_OBJECT_NEW_ARRAYBUFFER:
        return cx->getTypeObject(name, cx->getTypeObject("ArrayBuffer:prototype", NULL));

      case TYPE_OBJECT_XML:
      case TYPE_OBJECT_ARGUMENTS:
      case TYPE_OBJECT_NOSUCHMETHOD:
      case TYPE_OBJECT_NOSUCHMETHOD_ARGUMENTS:
      case TYPE_OBJECT_PROPERTY_DESCRIPTOR:
      case TYPE_OBJECT_KEY_VALUE_PAIR:
      case TYPE_OBJECT_JSON:
      case TYPE_OBJECT_PROXY: {
        TypeObject *object = getTypeObject(cx, NULL, name, false, NULL);
        cx->markTypeObjectUnknownProperties(object);
        return object;
      }

      case TYPE_OBJECT_REGEXP_MATCH_ARRAY:
      case TYPE_OBJECT_STRING_SPLIT_ARRAY:
      case TYPE_OBJECT_UNKNOWN_ARRAY:
      case TYPE_OBJECT_CLONE_ARRAY:
      case TYPE_OBJECT_PROPERTY_ARRAY:
      case TYPE_OBJECT_REFLECT_ARRAY:
        return cx->getTypeObject(name, cx->getFixedTypeObject(TYPE_OBJECT_ARRAY_PROTOTYPE));

      case TYPE_OBJECT_UNKNOWN_OBJECT:
      case TYPE_OBJECT_CLONE_OBJECT:
      case TYPE_OBJECT_REFLECT_OBJECT:
      case TYPE_OBJECT_XML_SETTINGS:
        return cx->getTypeObject(name, NULL);

      case TYPE_OBJECT_GETSET:
      case TYPE_OBJECT_REGEXP_STATICS:
      case TYPE_OBJECT_CALL:
      case TYPE_OBJECT_DECLENV:
      case TYPE_OBJECT_SHARP_ARRAY:
      case TYPE_OBJECT_WITH:
      case TYPE_OBJECT_BLOCK:
      case TYPE_OBJECT_NULL_CLOSURE:
      case TYPE_OBJECT_PROPERTY_ITERATOR:
      case TYPE_OBJECT_SCRIPT:
        return getTypeObject(cx, NULL, name, false, NULL);

      default:
        JS_NOT_REACHED("Unknown fixed object");
        return NULL;
    }
}

TypeObject *
TypeCompartment::getTypeObject(JSContext *cx, analyze::Script *script, const char *name,
                               bool isFunction, TypeObject *prototype)
{
#ifdef JS_TYPE_INFERENCE
    jsid id = ATOM_TO_JSID(js_Atomize(cx, name, strlen(name), 0));

    JSArenaPool &pool = script ? script->pool : this->pool;

    



    ObjectNameTable::AddPtr p = objectNameTable->lookupForAdd(id);
    if (p) {
        js::types::TypeObject *object = p->value;
        JS_ASSERT(object->isFunction == isFunction);
        JS_ASSERT(object->prototype == prototype);
        JS_ASSERT(object->pool == &pool);
        return object;
    }

    js::types::TypeObject *object;
    if (isFunction)
        object = ArenaNew<TypeFunction>(pool, cx, &pool, id, prototype);
    else
        object = ArenaNew<TypeObject>(pool, cx, &pool, id, prototype);

    TypeObject *&objects = script ? script->objects : this->objects;
    object->next = objects;
    objects = object;

    objectNameTable->add(p, id, object);
    return object;
#else
    return NULL;
#endif
}

void
TypeCompartment::addDynamicType(JSContext *cx, TypeSet *types, jstype type)
{
    JS_ASSERT(!types->hasType(type));

    interpreting = false;
    uint64_t startTime = currentTime();

    types->addType(cx, type);

    uint64_t endTime = currentTime();
    analysisTime += (endTime - startTime);
    interpreting = true;

    if (hasPendingRecompiles())
        processPendingRecompiles(cx);
}

void
TypeCompartment::addDynamicPush(JSContext *cx, analyze::Bytecode &code,
                                unsigned index, jstype type)
{
    js::types::TypeSet *types = code.pushed(index);
    JS_ASSERT(!types->hasType(type));

    InferSpew(ISpewDynamic, "MonitorResult: #%u:%05u %u: %s",
              code.script->id, code.offset, index, TypeString(type));

    interpreting = false;
    uint64_t startTime = currentTime();

    types->addType(cx, type);

    





    JSOp op = JSOp(code.script->getScript()->code[code.offset]);
    const JSCodeSpec *cs = &js_CodeSpec[op];
    if (cs->format & (JOF_INC | JOF_DEC)) {
        JS_ASSERT(!code.hasIncDecOverflow);
        code.hasIncDecOverflow = true;

        
        analyze::Script::AnalyzeState state;
        code.script->analyzeTypes(cx, &code, state);
    }

    uint64_t endTime = currentTime();
    analysisTime += (endTime - startTime);
    interpreting = true;

    if (hasPendingRecompiles())
        processPendingRecompiles(cx);
}

void
TypeCompartment::processPendingRecompiles(JSContext *cx)
{
    





    JS_ASSERT(pendingRecompiles);
    for (unsigned i = 0; i < pendingRecompiles->length(); i++) {
#ifdef JS_METHODJIT
        JSScript *script = (*pendingRecompiles)[i];
        mjit::Recompiler recompiler(cx, script);
        if (!recompiler.recompile())
            JS_NOT_REACHED("Recompilation failed!");
#endif
    }
    delete pendingRecompiles;
    pendingRecompiles = NULL;
}

void
TypeCompartment::addPendingRecompile(JSContext *cx, JSScript *script)
{
    if (!script->jitNormal && !script->jitCtor) {
        
        return;
    }

    if (!pendingRecompiles)
        pendingRecompiles = new Vector<JSScript*>(ContextAllocPolicy(cx));

    for (unsigned i = 0; i < pendingRecompiles->length(); i++) {
        if (script == (*pendingRecompiles)[i])
            return;
    }

    recompilations++;
    pendingRecompiles->append(script);
}

void
TypeCompartment::dynamicAssign(JSContext *cx, JSObject *obj, jsid id, const Value &rval)
{
    jstype rvtype = GetValueType(cx, rval);
    TypeObject *object = obj->getTypeObject();

    if (object->unknownProperties)
        return;

    TypeSet *assignTypes;

    




    if (obj->isCall() || obj->isBlock()) {
        
        while (!obj->isCall())
            obj = obj->getParent();
        analyze::Script *script = obj->getCallObjCalleeFunction()->script()->analysis;
        JS_ASSERT(!script->isEval());

        assignTypes = script->getVariable(cx, id);
    } else {
        id = MakeTypeId(id);

        if (!JSID_IS_VOID(id) && id != id_prototype(cx) && id != id___proto__(cx)) {
            




            cx->markTypeObjectUnknownProperties(object);
            if (hasPendingRecompiles())
                processPendingRecompiles(cx);
            return;
        }

        assignTypes = object->getProperty(cx, id, true);

        



        if (id == id___proto__(cx))
            cx->markTypeObjectUnknownProperties(object);
    }

    if (assignTypes->hasType(rvtype))
        return;

    InferSpew(ISpewDynamic, "MonitorAssign: %s %s: %s",
              TypeIdString(object->name), TypeIdString(id), TypeString(rvtype));
    addDynamicType(cx, assignTypes, rvtype);
}

void
TypeCompartment::monitorBytecode(JSContext *cx, analyze::Bytecode *code)
{
    if (code->monitorNeeded)
        return;

    




    JSOp op = JSOp(code->script->getScript()->code[code->offset]);
    switch (op) {
      case JSOP_SETNAME:
      case JSOP_SETGNAME:
      case JSOP_SETELEM:
      case JSOP_SETPROP:
      case JSOP_SETMETHOD:
      case JSOP_INITPROP:
      case JSOP_INITMETHOD:
      case JSOP_FORPROP:
      case JSOP_FORNAME:
      case JSOP_ENUMELEM:
      case JSOP_DEFFUN:
      case JSOP_DEFFUN_FC:
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
      case JSOP_SETCALL:
      case JSOP_EVAL:
      case JSOP_FUNCALL:
      case JSOP_FUNAPPLY:
      case JSOP_NEW:
        code->setFixed(cx, 0, TYPE_UNKNOWN);
        break;
      default:
        TypeFailure(cx, "Monitoring unknown bytecode: %s", js_CodeNameTwo[op]);
    }

    InferSpew(ISpewOps, "addMonitorNeeded: #%u:%05u", code->script->id, code->offset);

    code->monitorNeeded = true;

    JSScript *script = code->script->getScript();
    if (script->hasJITCode())
        cx->compartment->types.addPendingRecompile(cx, script);
}

void
TypeCompartment::finish(JSContext *cx, JSCompartment *compartment)
{
    JS_ASSERT(this == &compartment->types);

    if (!InferSpewActive(ISpewResult) || JS_CLIST_IS_EMPTY(&compartment->scripts))
        return;

    for (JSScript *script = (JSScript *)compartment->scripts.next;
         &script->links != &compartment->scripts;
         script = (JSScript *)script->links.next) {
        if (script->analysis)
            script->analysis->finish(cx);
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





void
TypeStack::merge(JSContext *cx, TypeStack *one, TypeStack *two)
{
    JS_ASSERT((one == NULL) == (two == NULL));

    if (!one)
        return;

    one = one->group();
    two = two->group();

    
    if (one == two)
        return;

    
    JS_ASSERT(one->types.typeFlags == 0 && one->types.constraintList == NULL);
    JS_ASSERT(two->types.typeFlags == 0 && two->types.constraintList == NULL);

    
    if (one->innerStack)
        merge(cx, one->innerStack, two->innerStack);

    InferSpew(ISpewOps, "merge: T%u T%u", one->types.id(), two->types.id());

    
    PodZero(one);
    one->mergedGroup = two;
}





TypeObject::TypeObject(JSContext *cx, JSArenaPool *pool, jsid name, TypeObject *prototype)
    : name(name), isFunction(false), propertySet(NULL), propertyCount(0),
      prototype(prototype), instanceList(NULL), instanceNext(NULL), newObject(NULL),
      pool(pool), next(NULL), unknownProperties(false),
      isDenseArray(false), isPackedArray(false), possiblePackedArray(false)
{
    InferSpew(ISpewOps, "newObject: %s", TypeIdString(name));

    if (prototype) {
        if (prototype == cx->compartment->types.fixedTypeObjects[TYPE_OBJECT_ARRAY_PROTOTYPE])
            isDenseArray = isPackedArray = true;
        if (prototype->unknownProperties)
            unknownProperties = true;
        instanceNext = prototype->instanceList;
        prototype->instanceList = this;
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
            base->ownTypes.addSubset(cx, *pool, &p->types);
        if (object->instanceList)
            object->storeToInstances(cx, base);
        object = object->instanceNext;
    }
}

void
TypeObject::addProperty(JSContext *cx, jsid id, Property *&base)
{
    JS_ASSERT(!base);
    base = ArenaNew<Property>(*pool, pool, id);

    InferSpew(ISpewOps, "addProperty: %s %s T%u own T%u",
              TypeIdString(name), TypeIdString(id), base->types.id(), base->ownTypes.id());

    base->ownTypes.addSubset(cx, *pool, &base->types);

    if (unknownProperties) {
        




        base->ownTypes.addType(cx, TYPE_UNKNOWN);
    }

    
    if (instanceList)
        storeToInstances(cx, base);

    
    TypeObject *object = prototype;
    while (object) {
        Property *p =
            HashSetLookup<jsid,Property,Property>(object->propertySet, object->propertyCount, id);
        if (p)
            p->ownTypes.addSubset(cx, *object->pool, &base->types);
        object = object->prototype;
    }

    



    if (!isFunction || asFunction()->isBuiltin || id != id_prototype(cx))
        return;

    TypeFunction *function = asFunction();
    JS_ASSERT(!function->prototypeObject);

    const char *baseName = js_GetStringBytes(JSID_TO_ATOM(name));
    unsigned len = strlen(baseName) + 15;
    char *prototypeName = (char *)alloca(len);
    JS_snprintf(prototypeName, len, "%s:prototype", baseName);
    function->prototypeObject = cx->getTypeObject(prototypeName, NULL);

    base->ownTypes.addType(cx, (jstype) function->prototypeObject);
}

void
TypeObject::markUnknown(JSContext *cx)
{
    JS_ASSERT(!unknownProperties);
    unknownProperties = true;

    cx->markTypeArrayNotPacked(this, true, false);

    








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

    

    TypeObject *instance = instanceList;
    while (instance) {
        if (!instance->unknownProperties)
            instance->markUnknown(cx);
        instance = instance->instanceNext;
    }
}

TypeObject *
TypeObject::getNewObject(JSContext *cx)
{
    if (newObject)
        return newObject;
    const char *baseName = js_GetStringBytes(JSID_TO_ATOM(name));
    unsigned len = strlen(baseName) + 10;
    char *newName = (char *)alloca(len);
    JS_snprintf(newName, len, "%s:new", baseName);
    newObject = cx->getTypeObject(newName, this);
    return newObject;
}

void
TypeObject::print(JSContext *cx)
{
    printf("%s : %s", TypeIdString(name), prototype ? TypeIdString(prototype->name) : "(null)");

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





TypeFunction::TypeFunction(JSContext *cx, JSArenaPool *pool, jsid name, TypeObject *prototype)
    : TypeObject(cx, pool, name, prototype), handler(NULL), script(NULL),
      prototypeObject(NULL), returnTypes(pool),
      isBuiltin(false), isGeneric(false)
{
    isFunction = true;
    InferSpew(ISpewOps, "newFunction: %s return T%u", TypeIdString(name), returnTypes.id());
}





} } 





static inline bool
IgnorePopped(JSOp op, unsigned index)
{
    switch (op) {
      case JSOP_LEAVEBLOCK:
      case JSOP_LEAVEBLOCKEXPR:
         



        return true;

      case JSOP_FORNAME:
      case JSOP_FORLOCAL:
      case JSOP_FORGLOBAL:
      case JSOP_FORARG:
      case JSOP_FORPROP:
      case JSOP_FORELEM:
      case JSOP_MOREITER:
      case JSOP_ENDITER:
        
        return true;
      case JSOP_ENUMELEM:
        return (index == 1 || index == 2);

      
      case JSOP_SETNAME:
      case JSOP_SETGNAME:
        return (index == 1);
      case JSOP_GETXPROP:
      case JSOP_DUP:
        return true;
      case JSOP_SETXMLNAME:
        return (index == 1 || index == 2);

      case JSOP_ENDFILTER:
        
        return (index == 0);

      case JSOP_LEAVEWITH:
        
        return true;

      case JSOP_RETSUB:
      case JSOP_POPN:
        
        return true;

      default:
        return false;
    }
}

void
JSScript::typeCheckBytecode(JSContext *cx, const jsbytecode *pc, const js::Value *sp)
{
    if (analysis->failed())
        return;

    js::analyze::Bytecode &code = analysis->getCode(pc);
    JS_ASSERT(code.analyzed);

    int useCount = js::analyze::GetUseCount(this, code.offset);
    JSOp op = (JSOp) *pc;

    if (!useCount)
        return;

    js::types::TypeStack *stack = code.inStack->group();
    for (int i = 0; i < useCount; i++) {
        const js::Value &val = sp[-1 - i];
        js::types::TypeSet *types = &stack->types;
        bool ignore = val.isMagic(JS_ARRAY_HOLE) || stack->ignoreTypeTag || IgnorePopped(op, i);

        stack = stack->innerStack ? stack->innerStack->group() : NULL;

        if (ignore)
            continue;

        js::types::jstype type = js::types::GetValueType(cx, val);
        if (!types->hasType(type)) {
            js::types::TypeFailure(cx, "Missing type at #%u:%05u popped %u: %s",
                                   analysis->id, code.offset, i, js::types::TypeString(type));
            return;
        }

        if (js::types::TypeIsObject(type)) {
            JS_ASSERT(val.isObject());
            JSObject *obj = &val.toObject();
            js::types::TypeObject *object = (js::types::TypeObject *) type;

            if (object->unknownProperties) {
                JS_ASSERT(!object->isDenseArray);
                continue;
            }

            



            if (((object->prototype != NULL) != (obj->getProto() != NULL)) ||
                (object->prototype && object->prototype != obj->getProto()->getTypeObject())) {
                jsid protoName = object->prototype ? object->prototype->name : JSID_VOID;
                jsid needName = obj->getProto() ? obj->getProto()->getTypeObject()->name : JSID_VOID;
                js::types::TypeFailure(cx, "Wrong prototype %s for %s at #%u:%05u popped %u: need %s",
                                       js::types::TypeIdString(protoName),
                                       js::types::TypeIdString(object->name),
                                       analysis->id, code.offset, i,
                                       js::types::TypeIdString(needName));
            }

            
            JS_ASSERT_IF(object->isPackedArray, object->isDenseArray);
            if (object->isDenseArray) {
                if (!obj->isDenseArray() ||
                    (object->isPackedArray && !obj->isPackedDenseArray())) {
                    js::types::TypeFailure(cx, "Object not %s array at #%u:%05u popped %u: %s",
                        object->isPackedArray ? "packed" : "dense",
                        analysis->id, code.offset, i,
                        js::types::TypeIdString(object->name));
                }
            }
        }
    }
}

namespace js {
namespace analyze {

using namespace types;

void
Script::addVariable(JSContext *cx, jsid id, types::Variable *&var)
{
    JS_ASSERT(!var);
    var = ArenaNew<types::Variable>(pool, &pool, id);

    InferSpew(ISpewOps, "addVariable: #%lu %s T%u",
              this->id, TypeIdString(id), var->types.id());
}

inline Bytecode*
Script::parentCode()
{
    return parent ? &parent->analysis->getCode(parentpc) : NULL;
}

inline Script*
Script::evalParent()
{
    Script *script = this;
    while (script->parent && !script->fun)
        script = script->parent->analysis;
    return script;
}

void
Script::setFunction(JSContext *cx, JSFunction *fun)
{
    JS_ASSERT(!this->fun);
    this->fun = fun;

    
    if (script->isEmpty())
        function()->returnTypes.addType(cx, TYPE_UNDEFINED);

    








    if (fun->hasLocalNames())
        localNames = fun->getLocalNameArray(cx, &pool);

    
    if (fun->atom)
        getVariable(cx, ATOM_TO_JSID(fun->atom))->addType(cx, (jstype) fun->getTypeObject());
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






static Script * const SCOPE_GLOBAL = (Script *) 0x1;
static Script *
SearchScope(JSContext *cx, Script *script, TypeStack *stack, jsid id)
{
    
    while (true) {
        
        while (stack) {
            stack = stack->group();
            if (stack->boundWith) {
                
                return NULL;
            }
            if (stack->letVariable == id) {
                
                return script->evalParent();
            }
            stack = stack->innerStack;
        }

        if (script->isEval()) {
            
            JS_ASSERT(!script->variableCount);
            stack = script->parentCode()->inStack;
            script = script->parent->analysis;
            continue;
        }

        if (!script->parent)
            break;

        
        if (id == id_arguments(cx) && script->fun) {
            TypeSet *types = script->getVariable(cx, id);
            types->addType(cx, (jstype) cx->getFixedTypeObject(TYPE_OBJECT_ARGUMENTS));
            return script;
        }

        
        if (script->fun && id == ATOM_TO_JSID(script->fun->atom)) {
            TypeSet *types = script->getVariable(cx, id);
            types->addType(cx, (jstype) script->function());
            return script;
        }

        unsigned nargs = script->argCount();
        for (unsigned i = 0; i < nargs; i++) {
            if (id == script->getArgumentId(i))
                return script;
        }

        unsigned nfixed = script->getScript()->nfixed;
        for (unsigned i = 0; i < nfixed; i++) {
            if (id == script->getLocalId(i, NULL))
                return script;
        }

        stack = script->parentCode()->inStack;
        script = script->parent->analysis;
    }

    return SCOPE_GLOBAL;
}

static inline jsid
GetAtomId(JSContext *cx, Script *script, const jsbytecode *pc, unsigned offset)
{
    unsigned index = js_GetIndexFromBytecode(cx, script->getScript(), (jsbytecode*) pc, offset);
    return MakeTypeId(ATOM_TO_JSID(script->getScript()->getAtom(index)));
}

static inline jsid
GetGlobalId(JSContext *cx, Script *script, const jsbytecode *pc)
{
    unsigned index = GET_SLOTNO(pc);
    return MakeTypeId(ATOM_TO_JSID(script->getScript()->getGlobalAtom(index)));
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


static inline Script *
GetUpvarVariable(JSContext *cx, Bytecode *code, unsigned index, jsid *id)
{
    JSUpvarArray *uva = code->script->getScript()->upvars();

    JS_ASSERT(index < uva->length);
    js::UpvarCookie cookie = uva->vector[index];
    uint16 level = code->script->getScript()->staticLevel - cookie.level();
    uint16 slot = cookie.slot();

    
    Bytecode *newCode = code;
    while (newCode->script->getScript()->staticLevel != level)
        newCode = newCode->script->parentCode();

    Script *newScript = newCode->script;

    



    if (!newScript->fun)
        *id = newScript->getLocalId(newScript->getScript()->nfixed + slot, newCode);
    else if (slot < newScript->argCount())
        *id = newScript->getArgumentId(slot);
    else if (slot == UpvarCookie::CALLEE_SLOT)
        *id = ATOM_TO_JSID(newScript->fun->atom);
    else
        *id = newScript->getLocalId(slot - newScript->argCount(), newCode);

    JS_ASSERT(!JSID_IS_VOID(*id));
    return newScript->evalParent();
}


class TypeConstraintToString : public TypeConstraint
{
  public:
    TypeSet *target;

    TypeConstraintToString(TypeSet *_target)
        : TypeConstraint("tostring"), target(_target)
    {}

    void newType(JSContext *cx, TypeSet *source, jstype type)
    {
        if (TypeIsObject(type))
            target->addType(cx, TYPE_STRING);
        else
            target->addType(cx, type);
    }
};





static inline void
CheckNextTest(JSContext *cx, Bytecode *code, jsbytecode *pc)
{
    jsbytecode *next = pc + GetBytecodeLength(pc);
    switch ((JSOp)*next) {
      case JSOP_IFEQ:
      case JSOP_IFNE:
      case JSOP_NOT:
      case JSOP_TYPEOF:
      case JSOP_TYPEOFEXPR:
        code->pushed(0)->addType(cx, TYPE_UNDEFINED);
        break;
      default:
        break;
    }
}


static inline void
MergePushed(JSContext *cx, JSArenaPool &pool, Bytecode *code, unsigned num, TypeSet *types)
{
    types->addSubset(cx, pool, code->pushed(num));
}

void
Script::analyzeTypes(JSContext *cx, Bytecode *code, AnalyzeState &state)
{
    unsigned offset = code->offset;

    JS_ASSERT(code->analyzed);
    jsbytecode *pc = script->code + offset;
    JSOp op = (JSOp)*pc;

    InferSpew(ISpewOps, "analyze: #%u:%05u", id, offset);

    if (code->stackDepth > state.stackDepth && state.stack) {
#ifdef DEBUG
        



        for (unsigned i = state.stackDepth; i < code->stackDepth; i++) {
            JS_ASSERT(!state.stack[i].isForEach);
            JS_ASSERT(!state.stack[i].hasDouble);
            JS_ASSERT(!state.stack[i].scope);
        }
#endif
        unsigned ndefs = code->stackDepth - state.stackDepth;
        memset(&state.stack[state.stackDepth], 0, ndefs * sizeof(AnalyzeStateStack));
    }
    state.stackDepth = code->stackDepth;

    
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
      case JSOP_LOOKUPSWITCH:
      case JSOP_LOOKUPSWITCHX:
      case JSOP_TABLESWITCH:
      case JSOP_TABLESWITCHX:
      case JSOP_DEFCONST:
      case JSOP_LEAVEWITH:
      case JSOP_LEAVEBLOCK:
      case JSOP_RETRVAL:
      case JSOP_ENDITER:
      case JSOP_TRY:
      case JSOP_THROWING:
      case JSOP_GOSUB:
      case JSOP_GOSUBX:
      case JSOP_RETSUB:
      case JSOP_CONDSWITCH:
      case JSOP_DEFAULT:
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
      case JSOP_FORELEM:
      case JSOP_DEBUGGER:
        break;

        
      case JSOP_VOID:
      case JSOP_PUSH:
        code->setFixed(cx, 0, TYPE_UNDEFINED);
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
        
        code->setFixed(cx, 0, TYPE_INT32);
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
        code->setFixed(cx, 0, TYPE_BOOLEAN);
        break;
      case JSOP_DOUBLE:
        code->setFixed(cx, 0, TYPE_DOUBLE);
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
        code->setFixed(cx, 0, TYPE_STRING);
        break;
      case JSOP_NULL:
        code->setFixed(cx, 0, TYPE_NULL);
        break;
      case JSOP_REGEXP:
        code->setFixed(cx, 0, (jstype) cx->getFixedTypeObject(TYPE_OBJECT_NEW_REGEXP));
        break;

      case JSOP_STOP:
        
        if (fun)
            function()->returnTypes.addType(cx, TYPE_UNDEFINED);
        break;

      case JSOP_OR:
      case JSOP_ORX:
      case JSOP_AND:
      case JSOP_ANDX:
        
        code->popped(0)->addSubset(cx, pool, code->pushed(0));
        break;

      case JSOP_DUP:
        MergePushed(cx, pool, code, 0, code->popped(0));
        MergePushed(cx, pool, code, 1, code->popped(0));
        break;

      case JSOP_DUP2:
        MergePushed(cx, pool, code, 0, code->popped(1));
        MergePushed(cx, pool, code, 1, code->popped(0));
        MergePushed(cx, pool, code, 2, code->popped(1));
        MergePushed(cx, pool, code, 3, code->popped(0));
        break;

      case JSOP_GETGLOBAL:
      case JSOP_CALLGLOBAL:
      case JSOP_GETGNAME:
      case JSOP_CALLGNAME:
      case JSOP_NAME:
      case JSOP_CALLNAME: {
        
        jsid id;
        Script *scope;

        switch (op) {
          case JSOP_GETGLOBAL:
          case JSOP_CALLGLOBAL:
            id = GetGlobalId(cx, this, pc);
            scope = SCOPE_GLOBAL;
            break;
          case JSOP_GETGNAME:
          case JSOP_CALLGNAME:
            id = GetAtomId(cx, this, pc, 0);
            scope = SCOPE_GLOBAL;
            break;
          default:
            id = GetAtomId(cx, this, pc, 0);
            scope = SearchScope(cx, this, code->inStack, id);
            break;
        }

        if (scope == SCOPE_GLOBAL) {
            



            uint64_t startTime = cx->compartment->types.currentTime();
            JSObject *obj;
            JSProperty *prop;
            js_LookupPropertyWithFlags(cx, cx->globalObject, id,
                                       JSRESOLVE_QUALIFIED, &obj, &prop);
            uint64_t endTime = cx->compartment->types.currentTime();
            cx->compartment->types.analysisTime -= (endTime - startTime);

            
            PropertyAccess(cx, code, cx->getGlobalTypeObject(), false, code->pushed(0), id);
        } else if (scope) {
            
            TypeSet *types = scope->getVariable(cx, id);
            types->addSubset(cx, scope->pool, code->pushed(0));
        } else {
            
            code->setFixed(cx, 0, TYPE_UNKNOWN);
        }

        if (op == JSOP_CALLGLOBAL || op == JSOP_CALLGNAME || op == JSOP_CALLNAME)
            code->setFixed(cx, 1, scope ? TYPE_UNDEFINED : TYPE_UNKNOWN);
        CheckNextTest(cx, code, pc);
        break;
      }

      case JSOP_BINDGNAME:
      case JSOP_BINDNAME:
        
        break;

      case JSOP_SETGNAME:
      case JSOP_SETNAME: {
        jsid id = GetAtomId(cx, this, pc, 0);

        const AnalyzeStateStack &stack = state.popped(1);
        if (stack.scope == SCOPE_GLOBAL) {
            PropertyAccess(cx, code, cx->getGlobalTypeObject(), true, code->popped(0), id);
        } else if (stack.scope) {
            TypeSet *types = stack.scope->getVariable(cx, id);
            code->popped(0)->addSubset(cx, pool, types);
        } else {
            cx->compartment->types.monitorBytecode(cx, code);
        }

        MergePushed(cx, pool, code, 0, code->popped(0));
        break;
      }

      case JSOP_GETXPROP: {
        jsid id = GetAtomId(cx, this, pc, 0);

        const AnalyzeStateStack &stack = state.popped(0);
        if (stack.scope == SCOPE_GLOBAL) {
            PropertyAccess(cx, code, cx->getGlobalTypeObject(), false, code->pushed(0), id);
        } else if (stack.scope) {
            TypeSet *types = stack.scope->getVariable(cx, id);
            types->addSubset(cx, stack.scope->pool, code->pushed(0));
        } else {
            code->setFixed(cx, 0, TYPE_UNKNOWN);
        }

        break;
      }

      case JSOP_INCNAME:
      case JSOP_DECNAME:
      case JSOP_NAMEINC:
      case JSOP_NAMEDEC: {
        jsid id = GetAtomId(cx, this, pc, 0);

        Script *scope = SearchScope(cx, this, code->inStack, id);
        if (scope == SCOPE_GLOBAL) {
            PropertyAccess(cx, code, cx->getGlobalTypeObject(), true, NULL, id);
            PropertyAccess(cx, code, cx->getGlobalTypeObject(), false, code->pushed(0), id);
        } else if (scope) {
            TypeSet *types = scope->getVariable(cx, id);
            types->addSubset(cx, scope->pool, code->pushed(0));
            types->addArith(cx, scope->pool, code, types);
            if (code->hasIncDecOverflow)
                types->addType(cx, TYPE_DOUBLE);
        } else {
            cx->compartment->types.monitorBytecode(cx, code);
        }

        break;
      }

      case JSOP_SETGLOBAL:
      case JSOP_SETCONST: {
        






        jsid id = (op == JSOP_SETGLOBAL) ? GetGlobalId(cx, this, pc) : GetAtomId(cx, this, pc, 0);
        TypeSet *types = cx->getGlobalTypeObject()->getProperty(cx, id, true);
        code->popped(0)->addSubset(cx, pool, types);
        MergePushed(cx, pool, code, 0, code->popped(0));
        break;
      }

      case JSOP_INCGLOBAL:
      case JSOP_DECGLOBAL:
      case JSOP_GLOBALINC:
      case JSOP_GLOBALDEC: {
        jsid id = GetGlobalId(cx, this, pc);
        TypeSet *types = cx->getGlobalTypeObject()->getProperty(cx, id, true);
        types->addArith(cx, cx->compartment->types.pool, code, types);
        MergePushed(cx, cx->compartment->types.pool, code, 0, types);
        if (code->hasIncDecOverflow)
            types->addType(cx, TYPE_DOUBLE);
        break;
      }

      case JSOP_INCGNAME:
      case JSOP_DECGNAME:
      case JSOP_GNAMEINC:
      case JSOP_GNAMEDEC: {
        jsid id = GetAtomId(cx, this, pc, 0);
        PropertyAccess(cx, code, cx->getGlobalTypeObject(), true, NULL, id);
        PropertyAccess(cx, code, cx->getGlobalTypeObject(), false, code->pushed(0), id);
        break;
      }

      case JSOP_GETUPVAR:
      case JSOP_CALLUPVAR:
      case JSOP_GETFCSLOT:
      case JSOP_CALLFCSLOT: {
        unsigned index = GET_UINT16(pc);

        jsid id = JSID_VOID;
        Script *newScript = GetUpvarVariable(cx, code, index, &id);
        TypeSet *types = newScript->getVariable(cx, id);

        MergePushed(cx, newScript->pool, code, 0, types);
        if (op == JSOP_CALLUPVAR || op == JSOP_CALLFCSLOT)
            code->setFixed(cx, 1, TYPE_UNDEFINED);
        break;
      }

      case JSOP_GETARG:
      case JSOP_SETARG:
      case JSOP_CALLARG: {
        jsid id = getArgumentId(GET_ARGNO(pc));

        if (!JSID_IS_VOID(id)) {
            TypeSet *types = getVariable(cx, id);

            MergePushed(cx, pool, code, 0, types);
            if (op == JSOP_SETARG)
                code->popped(0)->addSubset(cx, pool, types);
        } else {
            code->setFixed(cx, 0, TYPE_UNKNOWN);
        }

        if (op == JSOP_CALLARG)
            code->setFixed(cx, 1, TYPE_UNDEFINED);
        break;
      }

      case JSOP_INCARG:
      case JSOP_DECARG:
      case JSOP_ARGINC:
      case JSOP_ARGDEC: {
        jsid id = getArgumentId(GET_ARGNO(pc));
        TypeSet *types = getVariable(cx, id);

        types->addArith(cx, pool, code, types);
        MergePushed(cx, pool, code, 0, types);
        if (code->hasIncDecOverflow)
            types->addType(cx, TYPE_DOUBLE);
        break;
      }

      case JSOP_ARGSUB:
        code->setFixed(cx, 0, TYPE_UNKNOWN);
        break;

      case JSOP_GETLOCAL:
      case JSOP_SETLOCAL:
      case JSOP_SETLOCALPOP:
      case JSOP_CALLLOCAL: {
        uint32 local = GET_SLOTNO(pc);
        jsid id = getLocalId(local, code);

        TypeSet *types;
        JSArenaPool *pool;
        if (!JSID_IS_VOID(id)) {
            types = evalParent()->getVariable(cx, id);
            pool = &evalParent()->pool;
        } else {
            types = getStackTypes(GET_SLOTNO(pc), code);
            pool = &this->pool;
        }

        if (op != JSOP_SETLOCALPOP) {
            MergePushed(cx, *pool, code, 0, types);
            if (op == JSOP_CALLLOCAL)
                code->setFixed(cx, 1, TYPE_UNDEFINED);
        }

        if (op == JSOP_SETLOCAL || op == JSOP_SETLOCALPOP) {
            state.clearLocal(local);
            if (state.popped(0).isConstant)
                state.addConstLocal(local, state.popped(0).isZero);

            code->popped(0)->addSubset(cx, this->pool, types);
        } else {
            





            if (localHasUseBeforeDef(local) || !localDefined(local, pc))
                code->pushed(0)->addType(cx, TYPE_UNDEFINED);
        }

        break;
      }

      case JSOP_INCLOCAL:
      case JSOP_DECLOCAL:
      case JSOP_LOCALINC:
      case JSOP_LOCALDEC: {
        uint32 local = GET_SLOTNO(pc);
        jsid id = getLocalId(local, code);

        state.clearLocal(local);

        TypeSet *types = evalParent()->getVariable(cx, id);
        types->addArith(cx, evalParent()->pool, code, types);
        MergePushed(cx, evalParent()->pool, code, 0, types);

        if (code->hasIncDecOverflow)
            types->addType(cx, TYPE_DOUBLE);
        break;
      }

      case JSOP_ARGUMENTS: {
        



        JS_ASSERT(fun);
        TypeSet *types = getVariable(cx, id_arguments(cx));
        types->addType(cx, (jstype) cx->getFixedTypeObject(TYPE_OBJECT_ARGUMENTS));
        MergePushed(cx, pool, code, 0, types);
        break;
      }

      case JSOP_ARGCNT: {
        JS_ASSERT(fun);
        TypeSet *types = getVariable(cx, id_arguments(cx));
        types->addType(cx, (jstype) cx->getFixedTypeObject(TYPE_OBJECT_ARGUMENTS));

        types->addGetProperty(cx, code, code->pushed(0), id_length(cx));
        break;
      }

      case JSOP_SETPROP:
      case JSOP_SETMETHOD: {
        jsid id = GetAtomId(cx, this, pc, 0);
        code->popped(1)->addSetProperty(cx, code, code->popped(0), id);

        MergePushed(cx, pool, code, 0, code->popped(0));
        break;
      }

      case JSOP_GETPROP:
      case JSOP_CALLPROP: {
        jsid id = GetAtomId(cx, this, pc, 0);
        code->popped(0)->addGetProperty(cx, code, code->pushed(0), id);

        if (op == JSOP_CALLPROP)
            code->popped(0)->addFilterPrimitives(cx, pool, code->pushed(1), true);
        CheckNextTest(cx, code, pc);
        break;
      }

      case JSOP_INCPROP:
      case JSOP_DECPROP:
      case JSOP_PROPINC:
      case JSOP_PROPDEC: {
        jsid id = GetAtomId(cx, this, pc, 0);
        code->popped(0)->addGetProperty(cx, code, code->pushed(0), id);
        code->popped(0)->addSetProperty(cx, code, NULL, id);
        break;
      }

      case JSOP_GETTHISPROP: {
        jsid id = GetAtomId(cx, this, pc, 0);

        
        TypeSet *newTypes = TypeSet::make(cx, pool, "thisprop");
        thisTypes.addTransformThis(cx, pool, newTypes);
        newTypes->addGetProperty(cx, code, code->pushed(0), id);

        CheckNextTest(cx, code, pc);
        break;
      }

      case JSOP_GETARGPROP: {
        jsid id = getArgumentId(GET_ARGNO(pc));
        TypeSet *types = getVariable(cx, id);

        jsid propid = GetAtomId(cx, this, pc, SLOTNO_LEN);
        types->addGetProperty(cx, code, code->pushed(0), propid);

        CheckNextTest(cx, code, pc);
        break;
      }

      case JSOP_GETLOCALPROP: {
        jsid id = getLocalId(GET_SLOTNO(pc), code);
        TypeSet *types = evalParent()->getVariable(cx, id);

        jsid propid = GetAtomId(cx, this, pc, SLOTNO_LEN);
        types->addGetProperty(cx, code, code->pushed(0), propid);

        CheckNextTest(cx, code, pc);
        break;
      }

      case JSOP_GETELEM:
      case JSOP_CALLELEM:
        code->popped(0)->addGetElem(cx, code, code->popped(1), code->pushed(0));

        CheckNextTest(cx, code, pc);

        if (op == JSOP_CALLELEM)
            code->popped(1)->addFilterPrimitives(cx, pool, code->pushed(1), true);
        break;

      case JSOP_SETELEM:
        if (state.popped(1).isZero) {
            







            code->popped(2)->add(cx, ArenaNew<TypeConstraintPossiblyPacked>(pool));
        }

        code->popped(1)->addSetElem(cx, code, code->popped(2), code->popped(0));
        MergePushed(cx, pool, code, 0, code->popped(0));
        break;

      case JSOP_INCELEM:
      case JSOP_DECELEM:
      case JSOP_ELEMINC:
      case JSOP_ELEMDEC:
        code->popped(0)->addGetElem(cx, code, code->popped(1), code->pushed(0));
        code->popped(0)->addSetElem(cx, code, code->popped(1), NULL);
        break;

      case JSOP_LENGTH:
        
        code->popped(0)->addGetProperty(cx, code, code->pushed(0), id_length(cx));
        break;

      case JSOP_THIS:
        thisTypes.addTransformThis(cx, pool, code->pushed(0));

        
        if (!fun)
            code->setFixed(cx, 0, (jstype) cx->getGlobalTypeObject());
        break;

      case JSOP_RETURN:
      case JSOP_SETRVAL:
        if (fun)
            code->popped(0)->addSubset(cx, pool, &function()->returnTypes);
        break;

      case JSOP_ADD:
        code->popped(0)->addArith(cx, pool, code, code->pushed(0), code->popped(1));
        code->popped(1)->addArith(cx, pool, code, code->pushed(0), code->popped(0));
        break;

      case JSOP_SUB:
      case JSOP_MUL:
      case JSOP_MOD:
      case JSOP_DIV:
        code->popped(0)->addArith(cx, pool, code, code->pushed(0));
        code->popped(1)->addArith(cx, pool, code, code->pushed(0));
        if (op == JSOP_DIV) {
            
            if (!state.popped(0).isConstant)
                code->setFixed(cx, 0, TYPE_DOUBLE);
        }
        break;

      case JSOP_NEG:
      case JSOP_POS:
        code->popped(0)->addArith(cx, pool, code, code->pushed(0));
        break;

      case JSOP_LAMBDA:
      case JSOP_LAMBDA_FC:
      case JSOP_DEFFUN:
      case JSOP_DEFFUN_FC:
      case JSOP_DEFLOCALFUN:
      case JSOP_DEFLOCALFUN_FC: {
        unsigned funOffset = 0;
        if (op == JSOP_DEFLOCALFUN || op == JSOP_DEFLOCALFUN_FC)
            funOffset = SLOTNO_LEN;

        unsigned off = (op == JSOP_DEFLOCALFUN || op == JSOP_DEFLOCALFUN_FC) ? SLOTNO_LEN : 0;
        JSObject *obj = GetScriptObject(cx, script, pc, off);
        TypeFunction *function = obj->getTypeObject()->asFunction();

        
        function->script->analysis->parent = script;
        function->script->analysis->parentpc = pc;

        TypeSet *res = NULL;

        if (op == JSOP_LAMBDA || op == JSOP_LAMBDA_FC) {
            res = code->pushed(0);
        } else if (op == JSOP_DEFLOCALFUN || op == JSOP_DEFLOCALFUN_FC) {
            jsid id = getLocalId(GET_SLOTNO(pc), code);
            res = evalParent()->getVariable(cx, id);
        } else {
            
            JSAtom *atom = obj->getFunctionPrivate()->atom;
            JS_ASSERT(atom);
            jsid id = ATOM_TO_JSID(atom);
            if (parent) {
                if (this->fun) {
                    



                    res = getVariable(cx, id);
                } else {
                    
                    Script *scope = SearchScope(cx, parent->analysis, parentCode()->inStack, id);
                    if (scope == SCOPE_GLOBAL) {
                        res = cx->getGlobalTypeObject()->getProperty(cx, id, true);
                    } else if (scope) {
                        res = scope->getVariable(cx, id);
                        res->addType(cx, TYPE_UNKNOWN);
                    }
                }
            } else {
                
                res = cx->getGlobalTypeObject()->getProperty(cx, id, true);
            }
        }

        if (res)
            res->addType(cx, (jstype) function);
        else
            cx->compartment->types.monitorBytecode(cx, code);
        break;
      }

      case JSOP_CALL:
      case JSOP_SETCALL:
      case JSOP_EVAL:
      case JSOP_FUNCALL:
      case JSOP_FUNAPPLY:
      case JSOP_NEW: {
        
        unsigned argCount = GetUseCount(script, offset) - 2;
        TypeCallsite *callsite = ArenaNew<TypeCallsite>(pool, code, op == JSOP_NEW, argCount);
        callsite->thisTypes = code->popped(argCount);
        callsite->returnTypes = code->pushed(0);

        for (unsigned i = 0; i < argCount; i++) {
            TypeSet *argTypes = code->popped(argCount - 1 - i);
            callsite->argumentTypes[i] = argTypes;
        }

        code->popped(argCount + 1)->addCall(cx, callsite);
        break;
      }

      case JSOP_NEWINIT:
      case JSOP_NEWARRAY:
      case JSOP_NEWOBJECT: {
        TypeObject *object;
        if (op == JSOP_NEWARRAY || (op == JSOP_NEWINIT && pc[1] == JSProto_Array)) {
            object = code->initArray;
            jsbytecode *next = pc + GetBytecodeLength(pc);
            if (JSOp(*next) != JSOP_ENDINIT)
                object->possiblePackedArray = true;
        } else {
            object = code->initObject;
        }

        code->pushed(0)->addType(cx, (jstype) object);
        break;
      }

      case JSOP_ENDINIT:
        break;

      case JSOP_INITELEM: {
        TypeObject *object = code->initObject;
        JS_ASSERT(object);

        code->pushed(0)->addType(cx, (jstype) object);

        TypeSet *types;
        if (state.popped(1).hasDouble) {
            Value val = DoubleValue(state.popped(1).doubleValue);
            jsid id;
            if (!js_InternNonIntElementId(cx, NULL, val, &id))
                JS_NOT_REACHED("Bad");
            types = object->getProperty(cx, id, true);
        } else {
            types = object->getProperty(cx, JSID_VOID, true);
        }

        if (state.hasGetSet)
            types->addType(cx, (jstype) cx->getFixedTypeObject(TYPE_OBJECT_GETSET));
        else if (state.hasHole)
            cx->markTypeArrayNotPacked(object, false);
        else
            code->popped(0)->addSubset(cx, pool, types);
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
        TypeObject *object = code->initObject;
        JS_ASSERT(object);

        code->pushed(0)->addType(cx, (jstype) object);

        jsid id = GetAtomId(cx, this, pc, 0);
        TypeSet *types = object->getProperty(cx, id, true);

        if (id == id___proto__(cx) || id == id_prototype(cx))
            cx->compartment->types.monitorBytecode(cx, code);
        else if (state.hasGetSet)
            types->addType(cx, (jstype) cx->getFixedTypeObject(TYPE_OBJECT_GETSET));
        else
            code->popped(0)->addSubset(cx, pool, types);
        state.hasGetSet = false;
        JS_ASSERT(!state.hasHole);
        break;
      }

      case JSOP_ENTERWITH:
        




        code->pushedArray[0].group()->boundWith = true;
        break;

      case JSOP_ENTERBLOCK: {
        JSObject *obj = GetScriptObject(cx, script, pc, 0);
        unsigned defCount = GetDefCount(script, offset);

        const Shape *shape = obj->lastProperty();
        for (unsigned i = 0; i < defCount; i++) {
            code->pushedArray[i].group()->letVariable = shape->id;
            shape = shape->previous();
        }
        break;
      }

      case JSOP_ITER:
        





        MergePushed(cx, pool, code, 0, code->popped(0));
        code->pushedArray[0].group()->ignoreTypeTag = true;
        break;

      case JSOP_MOREITER:
        code->pushedArray[0].group()->ignoreTypeTag = true;
        MergePushed(cx, pool, code, 0, code->popped(0));
        code->setFixed(cx, 1, TYPE_BOOLEAN);
        break;

      case JSOP_FORNAME: {
        jsid id = GetAtomId(cx, this, pc, 0);
        Script *scope = SearchScope(cx, this, code->inStack, id);

        if (scope == SCOPE_GLOBAL)
            SetForTypes(cx, state, code, cx->getGlobalTypeObject()->getProperty(cx, id, true));
        else if (scope)
            SetForTypes(cx, state, code, scope->getVariable(cx, id));
        else
            cx->compartment->types.monitorBytecode(cx, code);
        break;
      }

      case JSOP_FORGLOBAL: {
        jsid id = GetGlobalId(cx, this, pc);
        SetForTypes(cx, state, code, cx->getGlobalTypeObject()->getProperty(cx, id, true));
        break;
      }

      case JSOP_FORLOCAL: {
        jsid id = getLocalId(GET_SLOTNO(pc), code);
        JS_ASSERT(!JSID_IS_VOID(id));

        SetForTypes(cx, state, code, evalParent()->getVariable(cx, id));
        break;
      }

      case JSOP_FORARG: {
        jsid id = getArgumentId(GET_ARGNO(pc));
        JS_ASSERT(!JSID_IS_VOID(id));

        SetForTypes(cx, state, code, getVariable(cx, id));
        break;
      }

      case JSOP_FORPROP:
      case JSOP_ENUMELEM:
        cx->compartment->types.monitorBytecode(cx, code);
        break;

      case JSOP_ARRAYPUSH: {
        TypeSet *types = getStackTypes(GET_SLOTNO(pc), code);
        types->addSetProperty(cx, code, code->popped(0), JSID_VOID);
        break;
      }

      case JSOP_THROW:
        
        break;

      case JSOP_FINALLY:
        
        break;

      case JSOP_EXCEPTION:
        code->setFixed(cx, 0, TYPE_UNKNOWN);
        break;

      case JSOP_DEFVAR:
        





        if (parent && !fun) {
            jsid id = GetAtomId(cx, this, pc, 0);
            Script *scope = SearchScope(cx, parent->analysis, parentCode()->inStack, id);
            if (scope && scope != SCOPE_GLOBAL) {
                TypeSet *types = scope->getVariable(cx, id);
                types->addType(cx, TYPE_UNKNOWN);
            }
        }
        break;

      case JSOP_DELPROP:
      case JSOP_DELELEM:
      case JSOP_DELNAME:
        
        code->setFixed(cx, 0, TYPE_BOOLEAN);
        break;

      case JSOP_LEAVEBLOCKEXPR:
        MergePushed(cx, pool, code, 0, code->popped(0));
        break;

      case JSOP_CASE:
        MergePushed(cx, pool, code, 0, code->popped(1));
        break;

      case JSOP_UNBRAND:
        MergePushed(cx, pool, code, 0, code->popped(0));
        break;

      case JSOP_GENERATOR:
        if (fun) {
            TypeObject *object = cx->getFixedTypeObject(TYPE_OBJECT_NEW_GENERATOR);
            function()->returnTypes.addType(cx, (jstype) object);
        }
        break;

      case JSOP_YIELD:
        code->setFixed(cx, 0, TYPE_UNKNOWN);
        break;

      case JSOP_CALLXMLNAME:
        code->setFixed(cx, 1, TYPE_UNKNOWN);
        
      case JSOP_XMLNAME:
        code->setFixed(cx, 0, TYPE_UNKNOWN);
        break;

      case JSOP_SETXMLNAME:
        cx->compartment->types.monitorBytecode(cx, code);
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
        code->setFixed(cx, 0, (jstype) cx->getFixedTypeObject(TYPE_OBJECT_XML));
        break;

      case JSOP_FILTER:
        
        MergePushed(cx, pool, code, 0, code->popped(0));
        code->pushedArray[0].group()->boundWith = true;

        
        break;

      case JSOP_ENDFILTER:
        MergePushed(cx, pool, code, 0, code->popped(1));
        break;

      case JSOP_DEFSHARP: {
        



        JS_ASSERT(code->inStack);
        TypeSet *value = &code->inStack->group()->types;

        
        char name[24];
        JS_snprintf(name, sizeof(name), "#%d:%d",
                    GET_UINT16(pc), GET_UINT16(pc + UINT16_LEN));
        JSAtom *atom = js_Atomize(cx, name, strlen(name), ATOM_PINNED);
        jsid id = ATOM_TO_JSID(atom);

        TypeSet *types = evalParent()->getVariable(cx, id);
        value->addSubset(cx, pool, types);
        break;
      }

      case JSOP_USESHARP: {
        char name[24];
        JS_snprintf(name, sizeof(name), "#%d:%d",
                    GET_UINT16(pc), GET_UINT16(pc + UINT16_LEN));
        JSAtom *atom = js_Atomize(cx, name, strlen(name), ATOM_PINNED);
        jsid id = ATOM_TO_JSID(atom);

        TypeSet *types = evalParent()->getVariable(cx, id);
        MergePushed(cx, evalParent()->pool, code, 0, types);
        break;
      }

      case JSOP_CALLEE:
        code->setFixed(cx, 0, (jstype) function());
        break;

      default:
        TypeFailure(cx, "Unknown bytecode: %s", js_CodeNameTwo[op]);
    }

    

    if (!state.stack)
        return;

    if (op == JSOP_DUP) {
        state.stack[code->stackDepth] = state.stack[code->stackDepth - 1];
        state.stackDepth = code->stackDepth + 1;
    } else if (op == JSOP_DUP2) {
        state.stack[code->stackDepth]     = state.stack[code->stackDepth - 2];
        state.stack[code->stackDepth + 1] = state.stack[code->stackDepth - 1];
        state.stackDepth = code->stackDepth + 2;
    } else {
        unsigned nuses = GetUseCount(script, offset);
        unsigned ndefs = GetDefCount(script, offset);
        memset(&state.stack[code->stackDepth - nuses], 0, ndefs * sizeof(AnalyzeStateStack));
        state.stackDepth = code->stackDepth - nuses + ndefs;
    }

    switch (op) {
      case JSOP_BINDGNAME: {
        AnalyzeStateStack &stack = state.popped(0);
        stack.scope = SCOPE_GLOBAL;
        break;
      }

      case JSOP_BINDNAME: {
        jsid id = GetAtomId(cx, this, pc, 0);
        AnalyzeStateStack &stack = state.popped(0);
        stack.scope = SearchScope(cx, this, code->inStack, id);
        break;
      }

      case JSOP_ITER: {
        uintN flags = pc[1];
        if (flags & JSITER_FOREACH)
            state.popped(0).isForEach = true;
        break;
      }

      case JSOP_DOUBLE: {
        AnalyzeStateStack &stack = state.popped(0);
        stack.hasDouble = true;
        stack.doubleValue = GetScriptConst(cx, script, pc).toDouble();
        break;
      }

      case JSOP_ZERO:
        state.popped(0).isZero = true;
        
      case JSOP_ONE:
      case JSOP_INT8:
      case JSOP_INT32:
      case JSOP_UINT16:
      case JSOP_UINT24:
        state.popped(0).isConstant = true;
        break;

      case JSOP_GETLOCAL:
        if (state.maybeLocalConst(GET_SLOTNO(pc), false)) {
            state.popped(0).isConstant = true;
            if (state.maybeLocalConst(GET_SLOTNO(pc), true))
                state.popped(0).isZero = true;
        }
        break;

      default:;
    }
}





#ifdef DEBUG

void
Bytecode::print(JSContext *cx)
{
    jsbytecode *pc = script->getScript()->code + offset;

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

      case JOF_QARG: {
        jsid id = script->getArgumentId(GET_ARGNO(pc));
        printf("%s %s", name, TypeIdString(id));
        break;
      }

      case JOF_GLOBAL:
        printf("%s %s", name, TypeIdString(GetGlobalId(cx, script, pc)));
        break;

      case JOF_LOCAL:
        if ((op != JSOP_ARRAYPUSH) && (analyzed || (GET_SLOTNO(pc) < script->getScript()->nfixed))) {
            jsid id = script->getLocalId(GET_SLOTNO(pc), this);
            printf("%s %d %s", name, GET_SLOTNO(pc), TypeIdString(id));
        } else {
            printf("%s %d", name, GET_SLOTNO(pc));
        }
        break;

      case JOF_SLOTATOM: {
        jsid id = GetAtomId(cx, script, pc, SLOTNO_LEN);

        jsid slotid = JSID_VOID;
        if (op == JSOP_GETARGPROP)
            slotid = script->getArgumentId(GET_ARGNO(pc));
        if (op == JSOP_GETLOCALPROP && (analyzed || (GET_SLOTNO(pc) < script->getScript()->nfixed)))
            slotid = script->getLocalId(GET_SLOTNO(pc), this);

        printf("%s %u %s %s", name, GET_SLOTNO(pc), TypeIdString(slotid), TypeIdString(id));
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
Script::finish(JSContext *cx)
{
    if (failed() || !codeArray)
        return;

    TypeCompartment *compartment = &script->compartment->types;

    



    for (unsigned offset = 0; offset < script->length; offset++) {
        Bytecode *code = codeArray[offset];
        if (!code || !code->analyzed)
            continue;

        unsigned useCount = GetUseCount(script, offset);
        if (!useCount)
            continue;

        TypeStack *stack = code->inStack->group();
        for (unsigned i = 0; i < useCount; i++) {
            TypeSet *types = &stack->types;

            
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

            stack = stack->innerStack ? stack->innerStack->group() : NULL;
        }
    }

#ifdef DEBUG

    if (parent) {
        if (fun)
            printf("Function");
        else
            printf("Eval");

        printf(" #%u @%u\n", id, parent->analysis->id);
    } else {
        printf("Main #%u:\n", id);
    }

    if (!codeArray) {
        printf("(unused)\n");
        return;
    }

    
    printf("defines:");
    for (unsigned i = 0; i < localCount(); i++) {
        if (locals[i] != LOCAL_USE_BEFORE_DEF && locals[i] != LOCAL_CONDITIONALLY_DEFINED)
            printf(" %s@%u", TypeIdString(getLocalId(i, NULL)), locals[i]);
    }
    printf("\n");

    printf("locals:");

    if (variableCount >= 2) {
        unsigned capacity = HashSetCapacity(variableCount);
        for (unsigned i = 0; i < capacity; i++) {
            Variable *var = variableSet[i];
            if (var) {
                printf("\n    %s:", TypeIdString(var->id));
                var->types.print(cx);
            }
        }
    } else if (variableCount == 1) {
        Variable *var = (Variable *) variableSet;
        printf("\n    %s:", TypeIdString(var->id));
        var->types.print(cx);
    }
    printf("\n");

    int id_count = 0;

    for (unsigned offset = 0; offset < script->length; offset++) {
        Bytecode *code = codeArray[offset];
        if (!code)
            continue;

        printf("#%u:%05u:  ", id, offset);
        code->print(cx);
        printf("\n");

        if (code->defineCount) {
            printf("  defines:");
            for (unsigned i = 0; i < code->defineCount; i++) {
                uint32 local = code->defineArray[i];
                printf(" %s", TypeIdString(getLocalId(local, NULL)));
            }
            printf("\n");
        }

        TypeStack *stack;
        unsigned useCount = GetUseCount(script, offset);
        if (useCount) {
            printf("  use:");
            stack = code->inStack->group();
            for (unsigned i = 0; i < useCount; i++) {
                if (!stack->id)
                    stack->id = ++id_count;
                printf(" %d", stack->id);
                stack = stack->innerStack ? stack->innerStack->group() : NULL;
            }
            printf("\n");

            
            stack = code->inStack->group();
            for (unsigned i = 0; i < useCount; i++) {
                if (!IgnorePopped((JSOp)script->code[offset], i)) {
                    if (stack->types.typeFlags == 0)
                        printf("  missing stack: %d\n", stack->id);
                }
                stack = stack->innerStack ? stack->innerStack->group() : NULL;
            }
        }

        unsigned defCount = GetDefCount(script, offset);
        if (defCount) {
            printf("  def:");
            for (unsigned i = 0; i < defCount; i++) {
                stack = code->pushedArray[i].group();
                if (!stack->id)
                    stack->id = ++id_count;
                printf(" %d", stack->id);
            }
            printf("\n");
            for (unsigned i = 0; i < defCount; i++) {
                stack = code->pushedArray[i].group();
                printf("  type %d:", stack->id);
                stack->types.print(cx);
                printf("\n");
            }
        }

        if (code->monitorNeeded)
            printf("  monitored\n");
    }

    printf("\n");

    TypeObject *object = objects;
    while (object) {
        object->print(cx);
        object = object->next;
    }

#endif 

}

} } 





namespace js {

static inline void
TraceObjectList(JSTracer *trc, types::TypeObject *objects)
{
    types::TypeObject *object = objects;
    while (object) {
        gc::MarkString(trc, JSID_TO_STRING(object->name), "type_object_name");

        if (object->propertyCount >= 2) {
            unsigned capacity = types::HashSetCapacity(object->propertyCount);
            for (unsigned i = 0; i < capacity; i++) {
                types::Property *prop = object->propertySet[i];
                if (prop && !JSID_IS_VOID(prop->id))
                    gc::MarkString(trc, JSID_TO_STRING(prop->id), "type_property");
            }
        } else if (object->propertyCount == 1) {
            types::Property *prop = (types::Property *) object->propertySet;
            if (!JSID_IS_VOID(prop->id))
                gc::MarkString(trc, JSID_TO_STRING(prop->id), "type_property");
        }

        object = object->next;
    }
}

void
analyze::Script::trace(JSTracer *trc)
{
    if (fun) {
        JS_SET_TRACING_NAME(trc, "type_script");
        gc::Mark(trc, fun);
    }

    TraceObjectList(trc, objects);

    if (variableCount >= 2) {
        unsigned capacity = types::HashSetCapacity(variableCount);
        for (unsigned i = 0; i < capacity; i++) {
            types::Variable *var = variableSet[i];
            if (var)
                gc::MarkString(trc, JSID_TO_STRING(var->id), "type_property");
        }
    } else if (variableCount == 1) {
        types::Variable *var = (types::Variable *) variableSet;
        gc::MarkString(trc, JSID_TO_STRING(var->id), "type_property");
    }

    unsigned nameCount = script->nfixed + argCount();
    for (unsigned i = 0; i < nameCount; i++) {
        if (localNames[i]) {
            JSAtom *atom = JS_LOCAL_NAME_TO_ATOM(localNames[i]);
            gc::MarkString(trc, ATOM_TO_STRING(atom), "type_script_local");
        }
    }
}

void
types::TypeCompartment::trace(JSTracer *trc)
{
    TraceObjectList(trc, objects);
}

} 
