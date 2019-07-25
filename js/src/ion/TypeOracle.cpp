








































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

    BinaryTypes res;
    res.lhsTypes = script->analysis()->poppedTypes(pc, 1);
    res.rhsTypes = script->analysis()->poppedTypes(pc, 0);
    res.outTypes = script->analysis()->pushedTypes(pc, 0);
    return res;
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
    if ((js_CodeSpec[op].format & JOF_INCDEC) || op == JSOP_NEG) {
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
    return TypeScript::ThisTypes(script);
}

void
TypeInferenceOracle::getNewTypesAtJoinPoint(JSScript *script, jsbytecode *pc, Vector<MIRType> &slotTypes)
{
    ScriptAnalysis *analysis = script->analysis();

    
    
    
    
    
    
    
    
    if (!analysis->jumpTarget(pc))
        return;

    if (const SlotValue *newv = analysis->newValues(pc)) {
        while (newv->slot) {
            if (newv->value.kind() != SSAValue::PHI ||
                newv->value.phiOffset() != uint32_t(pc - script->code) ||
                !analysis->trackSlot(newv->slot)) {
                newv++;
                continue;
            }

            types::TypeSet *types = analysis->getValueTypes(newv->value);
            slotTypes[newv->slot] = getMIRType(types);
            newv++;
        }
    }
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
TypeInferenceOracle::elementReadIsDense(JSScript *script, jsbytecode *pc)
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
TypeInferenceOracle::elementReadIsPacked(JSScript *script, jsbytecode *pc)
{
    types::TypeSet *types = script->analysis()->poppedTypes(pc, 1);
    return !types->hasObjectFlags(cx, types::OBJECT_FLAG_NON_PACKED_ARRAY);
}

bool
TypeInferenceOracle::elementWriteIsDense(JSScript *script, jsbytecode *pc)
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
    
    
    types::TypeSet *types = script->analysis()->poppedTypes(pc, 2);
    return types->propertyNeedsBarrier(cx, id);
}

TypeSet *
TypeInferenceOracle::getCallTarget(JSScript *caller, uint32 argc, jsbytecode *pc)
{
    JS_ASSERT(caller == this->script);
    JS_ASSERT(JSOp(*pc) == JSOP_CALL || JSOp(*pc) == JSOP_NEW);

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
    JS_ASSERT(JSOp(*pc) == JSOP_CALL);

    Bytecode *code = caller->analysis()->maybeCode(pc);
    if (code->monitoredTypes || code->monitoredTypesReturn || caller->analysis()->typeBarriers(cx, pc))
        return false;
    return true;
}

bool
TypeInferenceOracle::canEnterInlinedScript(JSScript *inlineScript)
{
    return inlineScript->hasAnalysis() && inlineScript->analysis()->ranInference() &&
        inlineScript->analysis()->inlineable() && !inlineScript->analysis()->usesScopeChain();
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
TypeInferenceOracle::globalPropertyTypeSet(JSScript *script, jsbytecode *pc, jsid id)
{
    TypeObject *type = script->global()->getType(cx);
    if (type->unknownProperties())
        return NULL;

    return type->getProperty(cx, id, false);
}

