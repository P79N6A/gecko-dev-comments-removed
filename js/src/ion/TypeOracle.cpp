






#include "TypeOracle.h"

#include "IonSpewer.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsanalyze.h"

using namespace js;
using namespace js::ion;
using namespace js::types;
using namespace js::analyze;

bool
TypeInferenceOracle::init(JSContext *cx, JSScript *script)
{
    this->cx = cx;
    this->script = script;
    return script->ensureRanInference(cx);
}

MIRType
TypeInferenceOracle::getMIRType(TypeSet *types)
{
    
    JSValueType type = types->getKnownTypeTag(cx);
    switch (type) {
      case JSVAL_TYPE_UNDEFINED:
        return MIRType_Undefined;
      case JSVAL_TYPE_NULL:
        return MIRType_Null;
      case JSVAL_TYPE_BOOLEAN:
        return MIRType_Boolean;
      case JSVAL_TYPE_INT32:
        return MIRType_Int32;
      case JSVAL_TYPE_DOUBLE:
        return MIRType_Double;
      case JSVAL_TYPE_STRING:
        return MIRType_String;
      case JSVAL_TYPE_OBJECT:
        return MIRType_Object;
      case JSVAL_TYPE_MAGIC:
        return MIRType_Magic;
      default:
        return MIRType_Value;
    }
}

TypeOracle::UnaryTypes
TypeInferenceOracle::unaryTypes(JSScript *script, jsbytecode *pc)
{
    JS_ASSERT(script == this->script);

    UnaryTypes res;
    res.inTypes = script->analysis()->poppedTypes(pc, 0);
    res.outTypes = script->analysis()->pushedTypes(pc, 0);
    return res;
}

TypeOracle::BinaryTypes
TypeInferenceOracle::binaryTypes(JSScript *script, jsbytecode *pc)
{
    JS_ASSERT(script == this->script);

    JSOp op = (JSOp)*pc;

    BinaryTypes res;
    if ((js_CodeSpec[op].format & JOF_INCDEC) || op == JSOP_NEG || op == JSOP_POS) {
        res.lhsTypes = script->analysis()->poppedTypes(pc, 0);
        res.rhsTypes = NULL;
        res.outTypes = script->analysis()->pushedTypes(pc, 0);
    } else {
        res.lhsTypes = script->analysis()->poppedTypes(pc, 1);
        res.rhsTypes = script->analysis()->poppedTypes(pc, 0);
        res.outTypes = script->analysis()->pushedTypes(pc, 0);
    }
    return res;
}

TypeOracle::BinaryTypes
TypeInferenceOracle::incslot(JSScript *script, jsbytecode *pc)
{
    JSOp op = JSOp(*pc);
    unsigned index = GET_SLOTNO(pc);
 
    TypeSet *types = NULL;
    if (js_CodeSpec[op].type() == JOF_LOCAL) {
        if (script->analysis()->trackSlot(LocalSlot(script, index)))
            return binaryTypes(script, pc);
        types = TypeScript::LocalTypes(script, index);
    } else {
        if (script->analysis()->trackSlot(ArgSlot(index)))
            return binaryTypes(script, pc);
        types = TypeScript::ArgTypes(script, index);
    }

    BinaryTypes b;
    b.lhsTypes = types;
    b.rhsTypes = NULL;
    b.outTypes = script->analysis()->pushedTypes(pc, 0);
    return b;
}

TypeOracle::Unary
TypeInferenceOracle::unaryOp(JSScript *script, jsbytecode *pc)
{
    JS_ASSERT(script == this->script);

    Unary res;
    res.ival = getMIRType(script->analysis()->poppedTypes(pc, 0));
    res.rval = getMIRType(script->analysis()->pushedTypes(pc, 0));
    return res;
}

TypeOracle::Binary
TypeInferenceOracle::binaryOp(JSScript *script, jsbytecode *pc)
{
    JS_ASSERT(script == this->script);
    
    JSOp op = (JSOp)*pc;

    Binary res;
    if ((js_CodeSpec[op].format & JOF_INCDEC) || op == JSOP_NEG || op == JSOP_POS) {
        res.lhs = getMIRType(script->analysis()->poppedTypes(pc, 0));
        res.rhs = MIRType_Int32;
        res.rval = getMIRType(script->analysis()->pushedTypes(pc, 0));
    } else {
        res.lhs = getMIRType(script->analysis()->poppedTypes(pc, 1));
        res.rhs = getMIRType(script->analysis()->poppedTypes(pc, 0));
        res.rval = getMIRType(script->analysis()->pushedTypes(pc, 0));
    }
    return res;
}

TypeSet *
TypeInferenceOracle::thisTypeSet(JSScript *script)
{
    JS_ASSERT(script == this->script);
    TypeSet *thisTypes = TypeScript::ThisTypes(script);

    if (thisTypes)
        thisTypes->addFreeze(cx);

    return thisTypes;
}

bool
TypeInferenceOracle::getOsrTypes(jsbytecode *osrPc, Vector<MIRType> &slotTypes)
{
    JS_ASSERT(JSOp(*osrPc) == JSOP_LOOPENTRY);
    JS_ASSERT(script->code < osrPc);
    JS_ASSERT(osrPc < script->code + script->length);

    Vector<types::TypeSet *> slotTypeSets(cx);
    if (!slotTypeSets.resize(TotalSlots(script)))
        return false;

    for (uint32_t slot = ThisSlot(); slot < TotalSlots(script); slot++)
        slotTypeSets[slot] = TypeScript::SlotTypes(script, slot);

    jsbytecode *pc = script->code;
    ScriptAnalysis *analysis = script->analysis();

    
    
    while (pc < osrPc) {
        Bytecode *opinfo = analysis->maybeCode(pc);
        if (opinfo) {
            if (opinfo->jumpTarget) {
                
                if (const SlotValue *newv = analysis->newValues(pc)) {
                    while (newv->slot) {
                        if (newv->slot < TotalSlots(script))
                            slotTypeSets[newv->slot] = analysis->getValueTypes(newv->value);
                        newv++;
                    }
                }
            }

            if (BytecodeUpdatesSlot(JSOp(*pc))) {
                uint32_t slot = GetBytecodeSlot(script, pc);
                if (analysis->trackSlot(slot))
                    slotTypeSets[slot] = analysis->pushedTypes(pc, 0);
            }
        }

        pc += GetBytecodeLength(pc);
    }

    JS_ASSERT(pc == osrPc);

    
    
    JS_ASSERT(ThisSlot() == 1);
    JS_ASSERT(ArgSlot(0) == 2);

#ifdef DEBUG
    uint32_t stackDepth = analysis->getCode(osrPc).stackDepth;
#endif

    if (script->function()) {
        JS_ASSERT(slotTypes.length() == TotalSlots(script) + stackDepth);

        for (size_t i = ThisSlot(); i < TotalSlots(script); i++)
            slotTypes[i] = getMIRType(slotTypeSets[i]);
    } else {
        JS_ASSERT(slotTypes.length() == TotalSlots(script) + stackDepth - 1);

        for (size_t i = ArgSlot(0); i < TotalSlots(script); i++)
            slotTypes[i - 1] = getMIRType(slotTypeSets[i]);
    }

    return true;
}

TypeSet *
TypeInferenceOracle::parameterTypeSet(JSScript *script, size_t index)
{
    JS_ASSERT(script == this->script);
    TypeSet *argTypes = TypeScript::ArgTypes(script, index);
    if (argTypes)
        argTypes->addFreeze(cx);

    return argTypes;
}

TypeSet *
TypeInferenceOracle::propertyRead(JSScript *script, jsbytecode *pc)
{
    return script->analysis()->pushedTypes(pc, 0);
}

TypeSet *
TypeInferenceOracle::propertyReadBarrier(JSScript *script, jsbytecode *pc)
{
    if (script->analysis()->typeBarriers(cx, pc))
        return script->analysis()->bytecodeTypes(pc);
    return NULL;
}

bool
TypeInferenceOracle::propertyReadIdempotent(JSScript *script, jsbytecode *pc, HandleId id)
{
    if (script->analysis()->getCode(pc).notIdempotent)
        return false;

    if (id.get() != MakeTypeId(cx, id))
        return false;

    TypeSet *types = script->analysis()->poppedTypes(pc, 0);
    if (!types || types->unknownObject())
        return false;

    for (unsigned i = 0; i < types->getObjectCount(); i++) {
        if (types->getSingleObject(i))
            return false;

        if (TypeObject *obj = types->getTypeObject(i)) {
            if (obj->unknownProperties())
                return false;

            
            TypeSet *propertyTypes = obj->getProperty(cx, id, false);
            if (!propertyTypes || propertyTypes->isOwnProperty(cx, obj, true))
                return false;
        }
    }

    types->addFreeze(cx);
    return true;
}

bool
TypeInferenceOracle::elementReadIsDenseArray(JSScript *script, jsbytecode *pc)
{
    
    types::TypeSet *obj = script->analysis()->poppedTypes(pc, 1);
    types::TypeSet *id = script->analysis()->poppedTypes(pc, 0);

    JSValueType objType = obj->getKnownTypeTag(cx);
    if (objType != JSVAL_TYPE_OBJECT)
        return false;

    JSValueType idType = id->getKnownTypeTag(cx);
    if (idType != JSVAL_TYPE_INT32 && idType != JSVAL_TYPE_DOUBLE)
        return false;

    return !obj->hasObjectFlags(cx, types::OBJECT_FLAG_NON_DENSE_ARRAY);
}

bool
TypeInferenceOracle::elementReadIsTypedArray(JSScript *script, jsbytecode *pc, int *arrayType)
{
    
    types::TypeSet *obj = script->analysis()->poppedTypes(pc, 1);
    types::TypeSet *id = script->analysis()->poppedTypes(pc, 0);

    JSValueType objType = obj->getKnownTypeTag(cx);
    if (objType != JSVAL_TYPE_OBJECT)
        return false;

    JSValueType idType = id->getKnownTypeTag(cx);
    if (idType != JSVAL_TYPE_INT32 && idType != JSVAL_TYPE_DOUBLE)
        return false;

    if (obj->hasObjectFlags(cx, types::OBJECT_FLAG_NON_TYPED_ARRAY))
        return false;

    *arrayType = obj->getTypedArrayType(cx);
    if (*arrayType == TypedArray::TYPE_MAX)
        return false;

    JS_ASSERT(*arrayType >= 0 && *arrayType < TypedArray::TYPE_MAX);

    
    
    
    
    types::TypeSet *result = propertyRead(script, pc);
    if (*arrayType == TypedArray::TYPE_FLOAT32 || *arrayType == TypedArray::TYPE_FLOAT64) {
        if (!result->hasType(types::Type::DoubleType()))
            return false;
    } else {
        if (!result->hasType(types::Type::Int32Type()))
            return false;
    }

    return true;
}

bool
TypeInferenceOracle::elementReadIsString(JSScript *script, jsbytecode *pc)
{
    
    types::TypeSet *value = script->analysis()->poppedTypes(pc, 1);
    types::TypeSet *id = script->analysis()->poppedTypes(pc, 0);

    if (value->getKnownTypeTag(cx) != JSVAL_TYPE_STRING)
        return false;

    if (id->getKnownTypeTag(cx) != JSVAL_TYPE_INT32)
        return false;

    types::TypeSet *pushed = script->analysis()->pushedTypes(pc, 0);
    if (!pushed->hasType(types::Type::StringType()))
        return false;

    return true;
}

bool
TypeInferenceOracle::elementReadIsPacked(JSScript *script, jsbytecode *pc)
{
    types::TypeSet *types = script->analysis()->poppedTypes(pc, 1);
    return !types->hasObjectFlags(cx, types::OBJECT_FLAG_NON_PACKED_ARRAY);
}

void
TypeInferenceOracle::elementReadGeneric(JSScript *script, jsbytecode *pc, bool *cacheable, bool *monitorResult)
{
    MIRType obj = getMIRType(script->analysis()->poppedTypes(pc, 1));
    MIRType id = getMIRType(script->analysis()->poppedTypes(pc, 0));

    *cacheable = (obj == MIRType_Object &&
                  (id == MIRType_Value || id == MIRType_Int32 || id == MIRType_String));
    if (*cacheable)
        *monitorResult = (id == MIRType_String || script->analysis()->getCode(pc).getStringElement);
    else
        *monitorResult = true;
}

bool
TypeInferenceOracle::elementWriteIsDenseArray(JSScript *script, jsbytecode *pc)
{
    
    types::TypeSet *obj = script->analysis()->poppedTypes(pc, 2);
    types::TypeSet *id = script->analysis()->poppedTypes(pc, 1);

    JSValueType objType = obj->getKnownTypeTag(cx);
    if (objType != JSVAL_TYPE_OBJECT)
        return false;

    JSValueType idType = id->getKnownTypeTag(cx);
    if (idType != JSVAL_TYPE_INT32 && idType != JSVAL_TYPE_DOUBLE)
        return false;

    return !obj->hasObjectFlags(cx, types::OBJECT_FLAG_NON_DENSE_ARRAY);
}

bool
TypeInferenceOracle::elementWriteIsTypedArray(JSScript *script, jsbytecode *pc, int *arrayType)
{
    
    types::TypeSet *obj = script->analysis()->poppedTypes(pc, 2);
    types::TypeSet *id = script->analysis()->poppedTypes(pc, 1);

    JSValueType objType = obj->getKnownTypeTag(cx);
    if (objType != JSVAL_TYPE_OBJECT)
        return false;

    JSValueType idType = id->getKnownTypeTag(cx);
    if (idType != JSVAL_TYPE_INT32 && idType != JSVAL_TYPE_DOUBLE)
        return false;

    if (obj->hasObjectFlags(cx, types::OBJECT_FLAG_NON_TYPED_ARRAY))
        return false;

    *arrayType = obj->getTypedArrayType(cx);
    if (*arrayType == TypedArray::TYPE_MAX)
        return false;

    return true;
}

bool
TypeInferenceOracle::elementWriteIsPacked(JSScript *script, jsbytecode *pc)
{
    types::TypeSet *types = script->analysis()->poppedTypes(pc, 2);
    return !types->hasObjectFlags(cx, types::OBJECT_FLAG_NON_PACKED_ARRAY);
}

bool
TypeInferenceOracle::setElementHasWrittenHoles(JSScript *script, jsbytecode *pc)
{
    return script->analysis()->getCode(pc).arrayWriteHole;
}

MIRType
TypeInferenceOracle::elementWrite(JSScript *script, jsbytecode *pc)
{
    types::TypeSet *objTypes = script->analysis()->poppedTypes(pc, 2);
    MIRType elementType = MIRType_None;
    unsigned count = objTypes->getObjectCount();

    for (unsigned i = 0; i < count; i++) {
        if (objTypes->getSingleObject(i))
            return MIRType_None;

        if (TypeObject *object = objTypes->getTypeObject(i)) {
            types::TypeSet *elementTypes = object->getProperty(cx, JSID_VOID, false);
            if (!elementTypes)
                return MIRType_None;

            MIRType type = getMIRType(elementTypes);
            if (type == MIRType_None)
                return MIRType_None;

            if (elementType == MIRType_None)
                elementType = type;
            else if (elementType != type)
                return MIRType_None;
        }
    }

    
    objTypes->addFreeze(cx);
    return elementType;
}

bool
TypeInferenceOracle::arrayPrototypeHasIndexedProperty()
{
    return ArrayPrototypeHasIndexedProperty(cx, script);
}

bool
TypeInferenceOracle::canInlineCalls()
{
    return script->analysis()->hasFunctionCalls();
}

bool
TypeInferenceOracle::propertyWriteCanSpecialize(JSScript *script, jsbytecode *pc)
{
    return !script->analysis()->getCode(pc).monitoredTypes;
}

bool
TypeInferenceOracle::propertyWriteNeedsBarrier(JSScript *script, jsbytecode *pc, jsid id)
{
    types::TypeSet *types = script->analysis()->poppedTypes(pc, 1);
    return types->propertyNeedsBarrier(cx, id);
}

bool
TypeInferenceOracle::elementWriteNeedsBarrier(JSScript *script, jsbytecode *pc)
{
    
    
    types::TypeSet *types = script->analysis()->poppedTypes(pc, 2);
    return types->propertyNeedsBarrier(cx, JSID_VOID);
}

TypeSet *
TypeInferenceOracle::getCallTarget(JSScript *caller, uint32 argc, jsbytecode *pc)
{
    JS_ASSERT(caller == this->script);
    JS_ASSERT(js_CodeSpec[*pc].format & JOF_INVOKE && JSOp(*pc) != JSOP_EVAL);

    ScriptAnalysis *analysis = script->analysis();
    return analysis->poppedTypes(pc, argc + 1);
}

TypeSet *
TypeInferenceOracle::getCallArg(JSScript *script, uint32 argc, uint32 arg, jsbytecode *pc)
{
    JS_ASSERT(argc >= arg);
    
    
    return script->analysis()->poppedTypes(pc, argc - arg);
}

TypeSet *
TypeInferenceOracle::getCallReturn(JSScript *script, jsbytecode *pc)
{
    return script->analysis()->pushedTypes(pc, 0);
}

bool
TypeInferenceOracle::canInlineCall(JSScript *caller, jsbytecode *pc)
{
    JS_ASSERT(types::IsInlinableCall(pc));

    Bytecode *code = caller->analysis()->maybeCode(pc);
    if (code->monitoredTypes || code->monitoredTypesReturn || caller->analysis()->typeBarriers(cx, pc))
        return false;
    return true;
}

bool
TypeInferenceOracle::canEnterInlinedFunction(JSFunction *target)
{
    JSScript *script = target->script();
    if (!script->hasAnalysis() || !script->analysis()->ranInference())
        return false;

    if (!script->analysis()->inlineable())
        return false;

    if (script->analysis()->usesScopeChain())
        return false;

    if (target->getType(cx)->unknownProperties())
        return false;

    
    TypeSet::WatchObjectStateChange(cx, target->getType(cx));
    return true;
}

TypeSet *
TypeInferenceOracle::globalPropertyWrite(JSScript *script, jsbytecode *pc, jsid id,
                                         bool *canSpecialize)
{
    *canSpecialize = !script->analysis()->getCode(pc).monitoredTypes;
    if (!*canSpecialize)
        return NULL;

    return globalPropertyTypeSet(script, pc, id);
}

TypeSet *
TypeInferenceOracle::returnTypeSet(JSScript *script, jsbytecode *pc, types::TypeSet **barrier)
{
    if (script->analysis()->getCode(pc).monitoredTypesReturn)
        *barrier = script->analysis()->bytecodeTypes(pc);
    else
        *barrier = NULL;
    return script->analysis()->pushedTypes(pc, 0);
}

TypeSet *
TypeInferenceOracle::aliasedVarBarrier(JSScript *script, jsbytecode *pc, types::TypeSet **barrier)
{
    *barrier = script->analysis()->bytecodeTypes(pc);
    return script->analysis()->pushedTypes(pc, 0);
}

TypeSet *
TypeInferenceOracle::globalPropertyTypeSet(JSScript *script, jsbytecode *pc, jsid id)
{
    TypeObject *type = script->global().getType(cx);
    if (type->unknownProperties())
        return NULL;

    return type->getProperty(cx, id, false);
}

LazyArgumentsType
TypeInferenceOracle::isArgumentObject(types::TypeSet *obj)
{
    if (obj->isMagicArguments(cx))
        return DefinitelyArguments;
    if (obj->hasAnyFlag(TYPE_FLAG_LAZYARGS))
        return MaybeArguments;
    return NotArguments;
}

LazyArgumentsType
TypeInferenceOracle::propertyReadMagicArguments(JSScript *script, jsbytecode *pc)
{
    types::TypeSet *obj = script->analysis()->poppedTypes(pc, 0);
    return isArgumentObject(obj);
}

LazyArgumentsType
TypeInferenceOracle::elementReadMagicArguments(JSScript *script, jsbytecode *pc)
{
    types::TypeSet *obj = script->analysis()->poppedTypes(pc, 1);
    return isArgumentObject(obj);
}

LazyArgumentsType
TypeInferenceOracle::elementWriteMagicArguments(JSScript *script, jsbytecode *pc)
{
    types::TypeSet *obj = script->analysis()->poppedTypes(pc, 2);
    return isArgumentObject(obj);
}
