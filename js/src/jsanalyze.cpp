






































#include "jsanalyze.h"
#include "jsautooplen.h"
#include "jscompartment.h"
#include "jscntxt.h"

#include "jsinferinlines.h"
#include "jsobjinlines.h"










namespace js {
namespace types {

void
TypeCompartment::init()
{
    PodZero(this);

#ifdef DEBUG
    emptyObject.name_ = JSID_VOID;
#endif
    emptyObject.unknownProperties = true;
}

types::TypeObject *
TypeCompartment::newTypeObject(JSContext *cx, JSScript *script, const char *name,
                               bool isFunction, JSObject *proto)
{
#ifdef DEBUG
#if 1 
    static unsigned nameCount = 0;
    unsigned len = strlen(name) + 15;
    char *newName = (char *) alloca(len);
    JS_snprintf(newName, len, "%u:%s", ++nameCount, name);
    name = newName;
#endif
    jsid id = ATOM_TO_JSID(js_Atomize(cx, name, strlen(name), ATOM_PINNED));
#else
    jsid id = JSID_VOID;
#endif

    TypeObject *object;
    if (isFunction) {
        object = (TypeFunction *) cx->calloc(sizeof(TypeFunction));
        new(object) TypeFunction(id, proto);
    } else {
        object = (TypeObject *) cx->calloc(sizeof(TypeObject));
        new(object) TypeObject(id, proto);
    }

#ifdef JS_TYPE_INFERENCE
    TypeObject *&objects = script ? script->typeObjects : this->objects;
    object->next = objects;
    objects = object;
#else
    object->next = this->objects;
    this->objects = object;
#endif

    return object;
}

#ifdef JS_TYPE_INFERENCE
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
    if (isArray)
        res->initializerArray = true;
    else
        res->initializerObject = true;
    res->initializerOffset = offset;

    return res;
}
#endif

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

void
TypeObject::splicePrototype(JSContext *cx, JSObject *proto)
{
    JS_ASSERT(!this->proto);
    this->proto = proto;
    this->instanceNext = proto->getType()->instanceList;
    proto->getType()->instanceList = this;

    





#ifdef JS_TYPE_INFERENCE
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
JSContext::newTypeObject(const char *base, const char *postfix, JSObject *proto)
{
    char *name = NULL;
#ifdef DEBUG
    unsigned len = strlen(base) + strlen(postfix) + 5;
    name = (char *)alloca(len);
    JS_snprintf(name, len, "%s:%s", base, postfix);
#endif
    return compartment->types.newTypeObject(this, NULL, name, false, proto);
}

void
JSObject::makeNewType(JSContext *cx)
{
    JS_ASSERT(!newType);
    setDelegate();
    newType = cx->newTypeObject(getType()->name(), "new", this);

#ifdef JS_TYPE_INFERENCE
    if (!getType()->unknownProperties) {
        
        js::types::TypeSet *types = getType()->getProperty(cx, JSID_EMPTY, true);
        cx->compartment->types.addDynamicType(cx, types, (js::types::jstype) newType);
    }
#endif
}





namespace js {
namespace types {

void
types::TypeObject::trace(JSTracer *trc)
{
    JS_ASSERT(!marked);

    



    if (trc->context->runtime->gcMarkAndSweep)
        marked = true;

    if (emptyShapes) {
        int count = gc::FINALIZE_OBJECT_LAST - gc::FINALIZE_OBJECT0 + 1;
        for (int i = 0; i < count; i++) {
            if (emptyShapes[i])
                emptyShapes[i]->trace(trc);
        }
    }

    if (proto)
        gc::MarkObject(trc, *proto, "type_proto");
}

#ifdef JS_TYPE_INFERENCE






void
CondenseSweepTypeSet(JSContext *cx, HashSet<JSScript*> &condensed, TypeSet *types)
{
    if (types->objectCount >= 2) {
        bool removed = false;
        unsigned objectCapacity = HashSetCapacity(types->objectCount);
        for (unsigned i = 0; i < objectCapacity; i++) {
            TypeObject *object = types->objectSet[i];
            if (object && !object->marked) {
                removed = true;
                types->objectSet[i] = NULL;
            }
        }
        if (removed) {
            
            TypeObject **oldArray = types->objectSet;
            types->objectSet = NULL;
            types->objectCount = 0;
            for (unsigned i = 0; i < objectCapacity; i++) {
                TypeObject *object = oldArray[i];
                if (object) {
                    TypeObject *&entry = HashSetInsert<TypeObject *,TypeObject,TypeObjectKey>
                        (cx, types->objectSet, types->objectCount, object);
                    entry = object;
                }
            }
            cx->free(oldArray);
        }
    } else if (types->objectCount == 1) {
        TypeObject *object = (TypeObject*) types->objectSet;
        if (!object->marked) {
            types->objectSet = NULL;
            types->objectCount = 0;
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
                cx->free(constraint);
            }
            constraint = next;
            continue;
        }

        





        JSScript *script = constraint->script;
        if (script->isCachedEval ||
            (script->u.object && IsAboutToBeFinalized(cx, script->u.object)) ||
            (script->fun && IsAboutToBeFinalized(cx, script->fun))) {
            if (constraint->condensed())
                cx->free(constraint);
            constraint = next;
            continue;
        }

        HashSet<JSScript*>::AddPtr p =
            condensed.lookupForAdd(script);
        if (!p) {
            if (!condensed.add(p, script))
                JS_NOT_REACHED("FIXME");
            types->addCondensed(cx, script);
        }

        if (constraint->condensed())
            cx->free(constraint);
        constraint = next;
    }

    condensed.clear();
}

void
CondenseTypeObjectList(JSContext *cx, TypeObject *objects)
{
    HashSet<JSScript *> condensed(cx);
    if (!condensed.init())
        JS_NOT_REACHED("FIXME");

    TypeObject *object = objects;
    while (object) {
        if (object->propertyCount >= 2) {
            unsigned capacity = HashSetCapacity(object->propertyCount);
            for (unsigned i = 0; i < capacity; i++) {
                Property *prop = object->propertySet[i];
                if (prop) {
                    CondenseSweepTypeSet(cx, condensed, &prop->types);
                    CondenseSweepTypeSet(cx, condensed, &prop->ownTypes);
                }
            }
        } else if (object->propertyCount == 1) {
            Property *prop = (Property *) object->propertySet;
            CondenseSweepTypeSet(cx, condensed, &prop->types);
            CondenseSweepTypeSet(cx, condensed, &prop->ownTypes);
        }
        object = object->next;
    }
}

static void
DestroyTypeSet(JSContext *cx, const TypeSet &types)
{
    if (types.objectCount >= 2)
        cx->free(types.objectSet);
}

static void
DestroyProperty(JSContext *cx, Property *prop)
{
    DestroyTypeSet(cx, prop->types);
    DestroyTypeSet(cx, prop->ownTypes);
    cx->free(prop);
}

#endif 

void
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

#ifdef JS_TYPE_INFERENCE
            if (object->propertyCount >= 2) {
                unsigned capacity = HashSetCapacity(object->propertyCount);
                for (unsigned i = 0; i < capacity; i++) {
                    Property *prop = object->propertySet[i];
                    if (prop)
                        DestroyProperty(cx, prop);
                }
                cx->free(object->propertySet);
            } else if (object->propertyCount == 1) {
                Property *prop = (Property *) object->propertySet;
                DestroyProperty(cx, prop);
            }
#endif

            cx->free(object);
        }
    }
}

} } 

void
JSScript::condenseTypes(JSContext *cx)
{
#ifdef JS_TYPE_INFERENCE
    js::types::CondenseTypeObjectList(cx, typeObjects);

    if (varTypes) {
        js::HashSet<JSScript *> condensed(cx);
        if (!condensed.init())
            JS_NOT_REACHED("FIXME");

        unsigned num = 2 + nfixed + (fun ? fun->nargs : 0) + bindings.countUpvars();
        for (unsigned i = 0; i < num; i++)
            js::types::CondenseSweepTypeSet(cx, condensed, &varTypes[i]);
    }
#endif
}

void
JSScript::sweepTypes(JSContext *cx)
{
#ifdef JS_TYPE_INFERENCE
    SweepTypeObjectList(cx, typeObjects);

    if (types)
        js::types::DestroyScriptTypes(cx, this);
#endif
}

namespace js {
namespace analyze {





Script::Script()
{
    PodZero(this);
    JS_InitArenaPool(&pool, "script_analyze", 256, 8, NULL);
}

Script::~Script()
{
    JS_FinishArenaPool(&pool);
}





bool
Bytecode::mergeDefines(JSContext *cx, Script *script, bool initial,
                       unsigned newDepth, uint32 *newArray, unsigned newCount)
{
    if (initial) {
        



        stackDepth = newDepth;
        defineArray = newArray;
        defineCount = newCount;
        return true;
    }

    



    if (analyzed) {
#ifdef DEBUG
        









        JS_ASSERT(stackDepth == newDepth);
        for (unsigned i = 0; i < defineCount; i++) {
            bool found = false;
            for (unsigned j = 0; j < newCount; j++) {
                if (newArray[j] == defineArray[i])
                    found = true;
            }
            JS_ASSERT(found);
        }
#endif
    } else {
        JS_ASSERT(stackDepth == newDepth);
        bool owned = false;
        for (unsigned i = 0; i < defineCount; i++) {
            bool found = false;
            for (unsigned j = 0; j < newCount; j++) {
                if (newArray[j] == defineArray[i])
                    found = true;
            }
            if (!found) {
                




                if (!owned) {
                    uint32 *reallocArray = ArenaArray<uint32>(script->pool, defineCount);
                    if (!reallocArray) {
                        script->setOOM(cx);
                        return false;
                    }
                    memcpy(reallocArray, defineArray, defineCount * sizeof(uint32));
                    defineArray = reallocArray;
                    owned = true;
                }

                
                defineArray[i--] = defineArray[--defineCount];
            }
        }
    }

    return true;
}





inline bool
Script::addJump(JSContext *cx, unsigned offset,
                unsigned *currentOffset, unsigned *forwardJump,
                unsigned stackDepth, uint32 *defineArray, unsigned defineCount)
{
    JS_ASSERT(offset < script->length);

    Bytecode *&code = codeArray[offset];
    bool initial = (code == NULL);
    if (initial) {
        code = ArenaNew<Bytecode>(pool, this, offset);
        if (!code) {
            setOOM(cx);
            return false;
        }
    }

    if (!code->mergeDefines(cx, this, initial, stackDepth, defineArray, defineCount))
        return false;
    code->jumpTarget = true;

    if (offset < *currentOffset) {
        
        if (!code->analyzed) {
            if (*forwardJump == 0)
                *forwardJump = *currentOffset;
            *currentOffset = offset;
        }
    } else if (offset > *forwardJump) {
        *forwardJump = offset;
    }

    return true;
}

inline void
Script::setLocal(uint32 local, uint32 offset)
{
    JS_ASSERT(local < localCount());
    JS_ASSERT(offset != LOCAL_CONDITIONALLY_DEFINED);

    












    JS_ASSERT(locals[local] == LOCAL_CONDITIONALLY_DEFINED ||
              locals[local] == offset || offset == LOCAL_USE_BEFORE_DEF);

    locals[local] = offset;
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
Script::analyze(JSContext *cx, JSScript *script)
{
    JS_ASSERT(script && !codeArray && !locals);
    this->script = script;

    unsigned length = script->length;
    unsigned nfixed = localCount();

    codeArray = ArenaArray<Bytecode*>(pool, length);
    locals = ArenaArray<uint32>(pool, nfixed);

    if (!codeArray || !locals) {
        setOOM(cx);
        return;
    }

    PodZero(codeArray, length);

    for (unsigned i = 0; i < nfixed; i++)
        locals[i] = LOCAL_CONDITIONALLY_DEFINED;

    




    if (script->usesEval || cx->compartment->debugMode) {
        for (uint32 i = 0; i < nfixed; i++)
            setLocal(i, LOCAL_USE_BEFORE_DEF);
    }

    for (uint32 i = 0; i < script->nClosedVars; i++) {
        uint32 slot = script->getClosedVar(i);
        if (slot < nfixed)
            setLocal(slot, LOCAL_USE_BEFORE_DEF);
    }

    



    if (cx->compartment->debugMode)
        usesRval = true;

    




    unsigned forwardJump = 0;

    



    unsigned forwardCatch = 0;

    
    Bytecode *startcode = ArenaNew<Bytecode>(pool, this, 0);
    if (!startcode) {
        setOOM(cx);
        return;
    }

    startcode->stackDepth = 0;
    codeArray[0] = startcode;

    unsigned offset, nextOffset = 0;
    while (nextOffset < length) {
        offset = nextOffset;

        JS_ASSERT(forwardCatch <= forwardJump);

        
        if (forwardJump && forwardJump == offset)
            forwardJump = 0;
        if (forwardCatch && forwardCatch == offset)
            forwardCatch = 0;

        Bytecode *code = maybeCode(offset);
        jsbytecode *pc = script->code + offset;

        UntrapOpcode untrap(cx, script, pc);

        JSOp op = (JSOp)*pc;
        JS_ASSERT(op < JSOP_LIMIT);

        
        unsigned successorOffset = offset + GetBytecodeLength(pc);

        



        nextOffset = successorOffset;

        if (!code) {
            
            continue;
        }

        if (code->analyzed) {
            
            continue;
        }

        code->analyzed = true;

        if (forwardCatch)
            code->inTryBlock = true;

        if (untrap.trap)
            code->safePoint = true;

        unsigned stackDepth = code->stackDepth;
        uint32 *defineArray = code->defineArray;
        unsigned defineCount = code->defineCount;

        if (!forwardJump) {
            








            for (unsigned i = 0; i < defineCount; i++) {
                uint32 local = defineArray[i];
                JS_ASSERT_IF(locals[local] != LOCAL_CONDITIONALLY_DEFINED &&
                             locals[local] != LOCAL_USE_BEFORE_DEF,
                             locals[local] <= offset);
                if (locals[local] == LOCAL_CONDITIONALLY_DEFINED)
                    setLocal(local, offset);
            }
            defineArray = NULL;
            defineCount = 0;
        }

        unsigned nuses = GetUseCount(script, offset);
        unsigned ndefs = GetDefCount(script, offset);

        JS_ASSERT(stackDepth >= nuses);
        stackDepth -= nuses;
        stackDepth += ndefs;

        switch (op) {

          case JSOP_SETRVAL:
          case JSOP_POPV:
            usesRval = true;
            break;

          case JSOP_NAME:
          case JSOP_CALLNAME:
          case JSOP_BINDNAME:
          case JSOP_SETNAME:
          case JSOP_DELNAME:
          case JSOP_INCNAME:
          case JSOP_DECNAME:
          case JSOP_NAMEINC:
          case JSOP_NAMEDEC:
          case JSOP_FORNAME:
            usesScope = true;
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

            if (!addJump(cx, defaultOffset, &nextOffset, &forwardJump,
                         stackDepth, defineArray, defineCount)) {
                return;
            }
            getCode(defaultOffset).switchTarget = true;
            getCode(defaultOffset).safePoint = true;

            for (jsint i = low; i <= high; i++) {
                unsigned targetOffset = offset + GetJumpOffset(pc, pc2);
                if (targetOffset != offset) {
                    if (!addJump(cx, targetOffset, &nextOffset, &forwardJump,
                                 stackDepth, defineArray, defineCount)) {
                        return;
                    }
                }
                getCode(targetOffset).switchTarget = true;
                getCode(targetOffset).safePoint = true;
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

            if (!addJump(cx, defaultOffset, &nextOffset, &forwardJump,
                         stackDepth, defineArray, defineCount)) {
                return;
            }
            getCode(defaultOffset).switchTarget = true;
            getCode(defaultOffset).safePoint = true;

            while (npairs) {
                pc2 += INDEX_LEN;
                unsigned targetOffset = offset + GetJumpOffset(pc, pc2);
                if (!addJump(cx, targetOffset, &nextOffset, &forwardJump,
                             stackDepth, defineArray, defineCount)) {
                    return;
                }
                getCode(targetOffset).switchTarget = true;
                getCode(targetOffset).safePoint = true;
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

                    
                    if (catchOffset > forwardCatch)
                        forwardCatch = catchOffset;

                    if (tn->kind != JSTRY_ITER) {
                        if (!addJump(cx, catchOffset, &nextOffset, &forwardJump,
                                     stackDepth, defineArray, defineCount)) {
                            return;
                        }
                        getCode(catchOffset).exceptionEntry = true;
                        getCode(catchOffset).safePoint = true;
                    }
                }
            }
            break;
          }

          case JSOP_GETLOCAL:
            




            if (pc[JSOP_GETLOCAL_LENGTH] != JSOP_POP) {
                uint32 local = GET_SLOTNO(pc);
                if (local < nfixed && !localDefined(local, offset))
                    setLocal(local, LOCAL_USE_BEFORE_DEF);
            }
            break;

          case JSOP_CALLLOCAL:
          case JSOP_INCLOCAL:
          case JSOP_DECLOCAL:
          case JSOP_LOCALINC:
          case JSOP_LOCALDEC:
          case JSOP_GETLOCALPROP: {
            uint32 local = GET_SLOTNO(pc);
            if (local < nfixed && !localDefined(local, offset))
                setLocal(local, LOCAL_USE_BEFORE_DEF);
            break;
          }

          case JSOP_SETLOCAL:
          case JSOP_FORLOCAL: {
            uint32 local = GET_SLOTNO(pc);

            






            if (local < nfixed && locals[local] == LOCAL_CONDITIONALLY_DEFINED) {
                if (forwardJump) {
                    
                    uint32 *newArray = ArenaArray<uint32>(pool, defineCount + 1);
                    if (!newArray) {
                        setOOM(cx);
                        return;
                    }
                    if (defineCount)
                        memcpy(newArray, defineArray, defineCount * sizeof(uint32));
                    defineArray = newArray;
                    defineArray[defineCount++] = local;
                } else {
                    
                    setLocal(local, offset);
                }
            }
            break;
          }

          default:
            break;
        }

        uint32 type = JOF_TYPE(js_CodeSpec[op].format);

        
        if (type == JOF_JUMP || type == JOF_JUMPX) {
            
            unsigned newStackDepth = stackDepth;

            switch (op) {
              case JSOP_OR:
              case JSOP_AND:
              case JSOP_ORX:
              case JSOP_ANDX:
                




                stackDepth--;
                break;

              case JSOP_CASE:
              case JSOP_CASEX:
                
                newStackDepth--;
                break;

              default:;
            }

            unsigned targetOffset = offset + GetJumpOffset(pc, pc);
            if (!addJump(cx, targetOffset, &nextOffset, &forwardJump,
                         newStackDepth, defineArray, defineCount)) {
                return;
            }
        }

        
        if (!BytecodeNoFallThrough(op)) {
            JS_ASSERT(successorOffset < script->length);

            Bytecode *&nextcode = codeArray[successorOffset];
            bool initial = (nextcode == NULL);

            if (initial) {
                nextcode = ArenaNew<Bytecode>(pool, this, successorOffset);
                if (!nextcode) {
                    setOOM(cx);
                    return;
                }
            }

            if (type == JOF_JUMP || type == JOF_JUMPX)
                nextcode->jumpFallthrough = true;

            if (!nextcode->mergeDefines(cx, this, initial, stackDepth,
                                        defineArray, defineCount)) {
                return;
            }

            
            if (type == JOF_JUMP || type == JOF_JUMPX)
                nextcode->jumpTarget = true;
            else
                nextcode->fallthrough = true;
        }
    }

    JS_ASSERT(!failed());
    JS_ASSERT(forwardJump == 0 && forwardCatch == 0);
}





LifetimeScript::LifetimeScript()
    : analysis(NULL), script(NULL), fun(NULL), codeArray(NULL)
{
    JS_InitArenaPool(&pool, "script_liverange", 256, 8, NULL);
}

LifetimeScript::~LifetimeScript()
{
    JS_FinishArenaPool(&pool);
}

bool
LifetimeScript::analyze(JSContext *cx, analyze::Script *analysis, JSScript *script, JSFunction *fun)
{
    JS_ASSERT(analysis->hasAnalyzed() && !analysis->failed());

    this->analysis = analysis;
    this->script = script;
    this->fun = fun;

    codeArray = ArenaArray<LifetimeBytecode>(pool, script->length);
    if (!codeArray)
        return false;
    PodZero(codeArray, script->length);

    if (script->nfixed) {
        locals = ArenaArray<LifetimeVariable>(pool, script->nfixed);
        if (!locals)
            return false;
        PodZero(locals, script->nfixed);
    } else {
        locals = NULL;
    }

    if (fun && fun->nargs) {
        args = ArenaArray<LifetimeVariable>(pool, fun->nargs);
        if (!args)
            return false;
        PodZero(args, fun->nargs);
    } else {
        args = NULL;
    }

    PodZero(&thisVar);

    saved = ArenaArray<LifetimeVariable*>(pool, script->nfixed + (fun ? fun->nargs : 0));
    savedCount = 0;

    uint32 offset = script->length - 1;
    while (offset < script->length) {
        Bytecode *code = analysis->maybeCode(offset);
        if (!code) {
            offset--;
            continue;
        }

        UntrapOpcode untrap(cx, script, script->code + offset);

        if (codeArray[offset].loopBackedge) {
            





            unsigned backedge = codeArray[offset].loopBackedge;
            for (unsigned i = 0; i < script->nfixed; i++) {
                if (locals[i].lifetime && !extendVariable(cx, locals[i], offset, backedge))
                    return false;
            }
            for (unsigned i = 0; fun && i < fun->nargs; i++) {
                if (args[i].lifetime && !extendVariable(cx, args[i], offset, backedge))
                    return false;
            }
        }

        jsbytecode *pc = script->code + offset;
        JSOp op = (JSOp) *pc;

        switch (op) {
          case JSOP_GETARG:
          case JSOP_CALLARG:
          case JSOP_INCARG:
          case JSOP_DECARG:
          case JSOP_ARGINC:
          case JSOP_ARGDEC:
          case JSOP_GETARGPROP: {
            unsigned arg = GET_ARGNO(pc);
            if (!analysis->argEscapes(arg)) {
                if (!addVariable(cx, args[arg], offset))
                    return false;
            }
            break;
          }

          case JSOP_SETARG: {
            unsigned arg = GET_ARGNO(pc);
            if (!analysis->argEscapes(arg))
                killVariable(cx, args[arg], offset);
            break;
          }

          case JSOP_GETLOCAL:
          case JSOP_CALLLOCAL:
          case JSOP_INCLOCAL:
          case JSOP_DECLOCAL:
          case JSOP_LOCALINC:
          case JSOP_LOCALDEC:
          case JSOP_GETLOCALPROP: {
            unsigned local = GET_SLOTNO(pc);
            if (!analysis->localEscapes(local)) {
                if (!addVariable(cx, locals[local], offset))
                    return false;
            }
            break;
          }

          case JSOP_SETLOCAL:
          case JSOP_SETLOCALPOP:
          case JSOP_DEFLOCALFUN: {
            unsigned local = GET_SLOTNO(pc);
            if (!analysis->localEscapes(local))
                killVariable(cx, locals[local], offset);
            break;
          }

          case JSOP_THIS:
          case JSOP_GETTHISPROP:
            if (!addVariable(cx, thisVar, offset))
                return false;
            break;

          case JSOP_IFEQ:
          case JSOP_IFEQX:
          case JSOP_IFNE:
          case JSOP_IFNEX:
          case JSOP_OR:
          case JSOP_ORX:
          case JSOP_AND:
          case JSOP_ANDX:
          case JSOP_GOTO:
          case JSOP_GOTOX: {
            uint32 targetOffset = offset + GetJumpOffset(pc, pc);
            if (targetOffset < offset) {
                JSOp nop = JSOp(script->code[targetOffset]);
                if (nop == JSOP_GOTO || nop == JSOP_GOTOX) {
                    
                    jsbytecode *target = script->code + targetOffset;
                    targetOffset = targetOffset + GetJumpOffset(target, target);
                } else {
                    
                    JS_ASSERT(nop == JSOP_TRACE || nop == JSOP_NOTRACE);
                    codeArray[targetOffset].loopBackedge = offset;
                    break;
                }
            }
            for (unsigned i = 0; i < savedCount; i++) {
                LifetimeVariable &var = *saved[i];
                JS_ASSERT(!var.lifetime && var.saved);
                if (!var.savedEnd) {
                    






                    var.savedEnd = offset;
                }
                if (var.live(targetOffset)) {
                    



                    var.lifetime = ArenaNew<Lifetime>(pool, offset, var.saved);
                    if (!var.lifetime)
                        return false;
                    var.saved = NULL;
                    saved[i--] = saved[--savedCount];
                }
            }
            break;
          }

          case JSOP_LOOKUPSWITCH:
          case JSOP_LOOKUPSWITCHX:
          case JSOP_TABLESWITCH:
          case JSOP_TABLESWITCHX:
            
            for (unsigned i = 0; i < savedCount; i++) {
                LifetimeVariable &var = *saved[i];
                var.lifetime = ArenaNew<Lifetime>(pool, offset, var.saved);
                if (!var.lifetime)
                    return false;
                var.saved = NULL;
                saved[i--] = saved[--savedCount];
            }
            savedCount = 0;
            break;

          default:;
        }

        offset--;
    }

    return true;
}

#ifdef DEBUG
void
LifetimeScript::dumpVariable(LifetimeVariable &var)
{
    Lifetime *segment = var.lifetime ? var.lifetime : var.saved;
    while (segment) {
        printf(" (%u,%u%s)", segment->start, segment->end, segment->loopTail ? ",tail" : "");
        segment = segment->next;
    }
    printf("\n");
}
#endif 

inline bool
LifetimeScript::addVariable(JSContext *cx, LifetimeVariable &var, unsigned offset)
{
    if (var.lifetime) {
        JS_ASSERT(offset < var.lifetime->start);
        var.lifetime->start = offset;
    } else {
        if (var.saved) {
            
            for (unsigned i = 0; i < savedCount; i++) {
                if (saved[i] == &var) {
                    JS_ASSERT(savedCount);
                    saved[i--] = saved[--savedCount];
                    break;
                }
            }
        }
        var.lifetime = ArenaNew<Lifetime>(pool, offset, var.saved);
        if (!var.lifetime)
            return false;
        var.saved = NULL;
    }
    return true;
}

inline void
LifetimeScript::killVariable(JSContext *cx, LifetimeVariable &var, unsigned offset)
{
    if (!var.lifetime)
        return;
    JS_ASSERT(offset < var.lifetime->start);

    




    var.lifetime->start = offset;

    var.saved = var.lifetime;
    var.savedEnd = 0;
    var.lifetime = NULL;

    saved[savedCount++] = &var;
}

inline bool
LifetimeScript::extendVariable(JSContext *cx, LifetimeVariable &var, unsigned start, unsigned end)
{
    JS_ASSERT(var.lifetime);
    var.lifetime->start = start;

    Lifetime *segment = var.lifetime;
    if (segment->start >= end)
        return true;
    while (segment->next && segment->next->start < end)
        segment = segment->next;
    if (segment->end >= end)
        return true;

    Lifetime *tail = ArenaNew<Lifetime>(pool, end, segment->next);
    if (!tail)
        return false;
    tail->start = segment->end;
    tail->loopTail = true;
    segment->next = tail;

    return true;
}

} 
} 
