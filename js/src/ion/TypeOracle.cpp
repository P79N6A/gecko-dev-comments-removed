






#include "TypeOracle.h"

#include "Ion.h"
#include "IonSpewer.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsanalyze.h"

using namespace js;
using namespace js::ion;
using namespace js::types;
using namespace js::analyze;

bool
TypeInferenceOracle::init(JSContext *cx, JSScript *script, bool inlinedCall)
{
    AutoEnterAnalysis enter(cx);

    this->cx = cx;
    this->script_.init(script);

    if (inlinedCall) {
        JS_ASSERT_IF(script->length != 1, script->analysis()->ranInference());
        return script->ensureRanInference(cx);
    }

    Vector<JSScript*> seen(cx);
    return analyzeTypesForInlinableCallees(cx, script, seen);
}

bool
TypeInferenceOracle::analyzeTypesForInlinableCallees(JSContext *cx, JSScript *script,
                                                     Vector<JSScript*> &seen)
{
    
    if (seen.length() > 0 && script->getUseCount() < js_IonOptions.usesBeforeInlining())
        return true;

    for (size_t i = 0; i < seen.length(); i++) {
        if (seen[i] == script)
            return true;
    }
    if (!seen.append(script))
        return false;

    if (!script->ensureRanInference(cx))
        return false;

    ScriptAnalysis *analysis = script->analysis();
    JS_ASSERT(analysis->ranInference());

    for (jsbytecode *pc = script->code;
         pc < script->code + script->length;
         pc += GetBytecodeLength(pc))
    {
        if (!(js_CodeSpec[*pc].format & JOF_INVOKE))
            continue;

        if (!analysis->maybeCode(pc))
            continue;

        uint32_t argc = GET_ARGC(pc);

        StackTypeSet *calleeTypes = analysis->poppedTypes(pc, argc + 1);
        if (!analyzeTypesForInlinableCallees(cx, calleeTypes, seen))
            return false;

        
        
        if (*pc == JSOP_FUNCALL || *pc == JSOP_FUNAPPLY) {
            StackTypeSet *thisTypes = analysis->poppedTypes(pc, argc);
            if (!analyzeTypesForInlinableCallees(cx, thisTypes, seen))
                return false;
        }
    }

    return true;
}

bool
TypeInferenceOracle::analyzeTypesForInlinableCallees(JSContext *cx, StackTypeSet *calleeTypes,
                                                     Vector<JSScript*> &seen)
{
    if (calleeTypes->unknownObject())
        return true;

    size_t count = calleeTypes->getObjectCount();
    for (size_t i = 0; i < count; i++) {
        JSScript *script;
        if (JSObject *singleton = calleeTypes->getSingleObject(i)) {
            if (!singleton->isFunction() || !singleton->toFunction()->hasScript())
                continue;
            script = singleton->toFunction()->nonLazyScript();
        } else if (TypeObject *type = calleeTypes->getTypeObject(i)) {
            JSFunction *fun = type->interpretedFunction;
            if (!fun || !fun->hasScript())
                continue;
            script = fun->nonLazyScript();
        } else {
            continue;
        }

        if (!analyzeTypesForInlinableCallees(cx, script, seen))
            return false;

        
        
        if (calleeTypes->unknownObject() || calleeTypes->getObjectCount() != count)
            break;
    }

    return true;
}

MIRType
GetMIRType(JSValueType type)
{
    
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

MIRType
TypeInferenceOracle::getMIRType(StackTypeSet *types)
{
    return GetMIRType(types->getKnownTypeTag());
}

MIRType
TypeInferenceOracle::getMIRType(HeapTypeSet *types)
{
    return GetMIRType(types->getKnownTypeTag(cx));
}

TypeOracle::UnaryTypes
TypeInferenceOracle::unaryTypes(UnrootedScript script, jsbytecode *pc)
{
    JS_ASSERT(script == this->script());

    UnaryTypes res;
    res.inTypes = script->analysis()->poppedTypes(pc, 0);
    res.outTypes = script->analysis()->pushedTypes(pc, 0);
    return res;
}

TypeOracle::BinaryTypes
TypeInferenceOracle::binaryTypes(UnrootedScript script, jsbytecode *pc)
{
    JS_ASSERT(script == this->script());

    JSOp op = (JSOp)*pc;

    BinaryTypes res;
    if (op == JSOP_NEG || op == JSOP_POS) {
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

TypeOracle::Unary
TypeInferenceOracle::unaryOp(UnrootedScript script, jsbytecode *pc)
{
    JS_ASSERT(script == this->script());

    Unary res;
    res.ival = getMIRType(script->analysis()->poppedTypes(pc, 0));
    res.rval = getMIRType(script->analysis()->pushedTypes(pc, 0));
    return res;
}

TypeOracle::Binary
TypeInferenceOracle::binaryOp(UnrootedScript script, jsbytecode *pc)
{
    JS_ASSERT(script == this->script());

    JSOp op = (JSOp)*pc;

    Binary res;
    if (op == JSOP_NEG || op == JSOP_POS) {
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

StackTypeSet *
TypeInferenceOracle::thisTypeSet(UnrootedScript script)
{
    JS_ASSERT(script == this->script());
    return TypeScript::ThisTypes(script);
}

bool
TypeInferenceOracle::getOsrTypes(jsbytecode *osrPc, Vector<MIRType> &slotTypes)
{
    JS_ASSERT(JSOp(*osrPc) == JSOP_LOOPENTRY);
    JS_ASSERT(script()->code < osrPc);
    JS_ASSERT(osrPc < script()->code + script()->length);

    Vector<types::StackTypeSet *> slotTypeSets(cx);
    if (!slotTypeSets.resize(TotalSlots(script())))
        return false;

    for (uint32_t slot = ThisSlot(); slot < TotalSlots(script()); slot++)
        slotTypeSets[slot] = TypeScript::SlotTypes(script(), slot);

    jsbytecode *pc = script()->code;
    ScriptAnalysis *analysis = script()->analysis();

    
    
    for (;;) {
        Bytecode *opinfo = analysis->maybeCode(pc);
        if (opinfo) {
            if (opinfo->jumpTarget) {
                
                if (const SlotValue *newv = analysis->newValues(pc)) {
                    while (newv->slot) {
                        if (newv->slot < TotalSlots(script()))
                            slotTypeSets[newv->slot] = analysis->getValueTypes(newv->value);
                        newv++;
                    }
                }
            }

            if (BytecodeUpdatesSlot(JSOp(*pc))) {
                uint32_t slot = GetBytecodeSlot(script(), pc);
                if (analysis->trackSlot(slot))
                    slotTypeSets[slot] = analysis->pushedTypes(pc, 0);
            }
        }

        if (pc == osrPc)
            break;

        pc += GetBytecodeLength(pc);
    }

    JS_ASSERT(pc == osrPc);

    
    
    JS_ASSERT(ThisSlot() == 1);
    JS_ASSERT(ArgSlot(0) == 2);

#ifdef DEBUG
    uint32_t stackDepth = analysis->getCode(osrPc).stackDepth;
#endif

    if (script()->function()) {
        JS_ASSERT(slotTypes.length() == TotalSlots(script()) + stackDepth);

        for (size_t i = ThisSlot(); i < TotalSlots(script()); i++)
            slotTypes[i] = getMIRType(slotTypeSets[i]);
    } else {
        JS_ASSERT(slotTypes.length() == TotalSlots(script()) + stackDepth - 1);

        for (size_t i = ArgSlot(0); i < TotalSlots(script()); i++)
            slotTypes[i - 1] = getMIRType(slotTypeSets[i]);
    }

    return true;
}

StackTypeSet *
TypeInferenceOracle::parameterTypeSet(UnrootedScript script, size_t index)
{
    JS_ASSERT(script == this->script());
    return TypeScript::ArgTypes(script, index);
}

StackTypeSet *
TypeInferenceOracle::propertyRead(UnrootedScript script, jsbytecode *pc)
{
    return script->analysis()->pushedTypes(pc, 0);
}

StackTypeSet *
TypeInferenceOracle::propertyReadBarrier(HandleScript script, jsbytecode *pc)
{
    if (script->analysis()->typeBarriers(cx, pc))
        return script->analysis()->bytecodeTypes(pc);
    return NULL;
}

bool
TypeInferenceOracle::propertyReadIdempotent(HandleScript script, jsbytecode *pc, HandleId id)
{
    if (script->analysis()->getCode(pc).notIdempotent)
        return false;

    if (id != IdToTypeId(id))
        return false;

    StackTypeSet *types = script->analysis()->poppedTypes(pc, 0);
    if (!types || types->unknownObject())
        return false;

    for (unsigned i = 0; i < types->getObjectCount(); i++) {
        if (types->getSingleObject(i))
            return false;

        if (TypeObject *obj = types->getTypeObject(i)) {
            if (obj->unknownProperties())
                return false;

            
            HeapTypeSet *propertyTypes = obj->getProperty(cx, id, false);
            if (!propertyTypes || propertyTypes->isOwnProperty(cx, obj, true))
                return false;
        }
    }

    return true;
}

bool
TypeInferenceOracle::propertyReadAccessGetter(UnrootedScript script, jsbytecode *pc)
{
    return script->analysis()->getCode(pc).accessGetter;
}

bool
TypeInferenceOracle::inObjectIsDenseNativeWithoutExtraIndexedProperties(HandleScript script, jsbytecode *pc)
{
    
    StackTypeSet *id = script->analysis()->poppedTypes(pc, 1);
    StackTypeSet *obj = script->analysis()->poppedTypes(pc, 0);

    JSValueType idType = id->getKnownTypeTag();
    if (idType != JSVAL_TYPE_INT32 && idType != JSVAL_TYPE_DOUBLE)
        return false;

    Class *clasp = obj->getKnownClass();
    if (!clasp || !clasp->isNative())
        return false;

    return !types::TypeCanHaveExtraIndexedProperties(cx, obj);
}

bool
TypeInferenceOracle::inArrayIsPacked(UnrootedScript script, jsbytecode *pc)
{
    StackTypeSet *types = script->analysis()->poppedTypes(pc, 0);
    return !types->hasObjectFlags(cx, types::OBJECT_FLAG_NON_PACKED);
}

bool
TypeInferenceOracle::elementReadIsDenseNative(RawScript script, jsbytecode *pc)
{
    
    StackTypeSet *obj = script->analysis()->poppedTypes(pc, 1);
    StackTypeSet *id = script->analysis()->poppedTypes(pc, 0);

    JSValueType idType = id->getKnownTypeTag();
    if (idType != JSVAL_TYPE_INT32 && idType != JSVAL_TYPE_DOUBLE)
        return false;

    if (obj->hasType(types::Type::StringType()))
        return false;

    Class *clasp = obj->getKnownClass();
    return clasp && clasp->isNative();
}

bool
TypeInferenceOracle::elementReadIsTypedArray(RawScript script, jsbytecode *pc, int *arrayType)
{
    
    StackTypeSet *obj = script->analysis()->poppedTypes(pc, 1);
    StackTypeSet *id = script->analysis()->poppedTypes(pc, 0);

    JSValueType idType = id->getKnownTypeTag();
    if (idType != JSVAL_TYPE_INT32 && idType != JSVAL_TYPE_DOUBLE)
        return false;

    if (obj->hasType(types::Type::StringType()))
        return false;

    *arrayType = obj->getTypedArrayType();
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
TypeInferenceOracle::elementReadIsString(UnrootedScript script, jsbytecode *pc)
{
    
    StackTypeSet *value = script->analysis()->poppedTypes(pc, 1);
    StackTypeSet *id = script->analysis()->poppedTypes(pc, 0);

    if (value->getKnownTypeTag() != JSVAL_TYPE_STRING)
        return false;

    JSValueType idType = id->getKnownTypeTag();
    if (idType != JSVAL_TYPE_INT32 && idType != JSVAL_TYPE_DOUBLE)
        return false;

    
    
    
    StackTypeSet *pushed = script->analysis()->pushedTypes(pc, 0);
    if (pushed->getKnownTypeTag() != JSVAL_TYPE_STRING)
        return false;

    return true;
}

bool
TypeInferenceOracle::elementReadShouldAlwaysLoadDoubles(UnrootedScript script, jsbytecode *pc)
{
    StackTypeSet *types = script->analysis()->poppedTypes(pc, 1);
    types::StackTypeSet::DoubleConversion conversion = types->convertDoubleElements(cx);
    return conversion == StackTypeSet::AlwaysConvertToDoubles;
}

bool
TypeInferenceOracle::elementReadHasExtraIndexedProperty(UnrootedScript script, jsbytecode *pc)
{
    StackTypeSet *obj = script->analysis()->poppedTypes(pc, 1);
    return types::TypeCanHaveExtraIndexedProperties(cx, obj);
}

bool
TypeInferenceOracle::elementReadIsPacked(UnrootedScript script, jsbytecode *pc)
{
    StackTypeSet *types = script->analysis()->poppedTypes(pc, 1);
    return !types->hasObjectFlags(cx, types::OBJECT_FLAG_NON_PACKED);
}

void
TypeInferenceOracle::elementReadGeneric(UnrootedScript script, jsbytecode *pc, bool *cacheable, bool *monitorResult, bool *intIndex)
{
    MIRType obj = getMIRType(script->analysis()->poppedTypes(pc, 1));
    MIRType id = getMIRType(script->analysis()->poppedTypes(pc, 0));

    *cacheable = (obj == MIRType_Object &&
                  (id == MIRType_Value || id == MIRType_Int32 || id == MIRType_String));

    *intIndex = id == MIRType_Int32;

    
    
    if (*cacheable && id == MIRType_Int32 && script->analysis()->getCode(pc).nonNativeGetElement)
        *cacheable = false;

    if (*cacheable)
        *monitorResult = (id == MIRType_String || script->analysis()->getCode(pc).getStringElement);
    else
        *monitorResult = true;
}

bool
TypeInferenceOracle::elementWriteIsDenseNative(HandleScript script, jsbytecode *pc)
{
    return elementWriteIsDenseNative(script->analysis()->poppedTypes(pc, 2),
                                     script->analysis()->poppedTypes(pc, 1));
}

bool
TypeInferenceOracle::elementWriteIsDenseNative(StackTypeSet *obj, StackTypeSet *id)
{
    
    JSValueType idType = id->getKnownTypeTag();
    if (idType != JSVAL_TYPE_INT32 && idType != JSVAL_TYPE_DOUBLE)
        return false;

    Class *clasp = obj->getKnownClass();
    if (!clasp || !clasp->isNative())
        return false;

    return obj->convertDoubleElements(cx) != StackTypeSet::AmbiguousDoubleConversion;
}

bool
TypeInferenceOracle::elementWriteIsTypedArray(RawScript script, jsbytecode *pc, int *arrayType)
{
    return elementWriteIsTypedArray(script->analysis()->poppedTypes(pc, 2),
                                    script->analysis()->poppedTypes(pc, 1),
                                    arrayType);
}

bool
TypeInferenceOracle::elementWriteIsTypedArray(StackTypeSet *obj, StackTypeSet *id, int *arrayType)
{
    
    JSValueType idType = id->getKnownTypeTag();
    if (idType != JSVAL_TYPE_INT32 && idType != JSVAL_TYPE_DOUBLE)
        return false;

    *arrayType = obj->getTypedArrayType();
    if (*arrayType == TypedArray::TYPE_MAX)
        return false;

    return true;
}

bool
TypeInferenceOracle::elementWriteNeedsDoubleConversion(UnrootedScript script, jsbytecode *pc)
{
    StackTypeSet *types = script->analysis()->poppedTypes(pc, 2);
    types::StackTypeSet::DoubleConversion conversion = types->convertDoubleElements(cx);
    return conversion == StackTypeSet::AlwaysConvertToDoubles ||
           conversion == StackTypeSet::MaybeConvertToDoubles;
}

bool
TypeInferenceOracle::elementWriteHasExtraIndexedProperty(RawScript script, jsbytecode *pc)
{
    StackTypeSet *obj = script->analysis()->poppedTypes(pc, 2);

    if (obj->hasObjectFlags(cx, types::OBJECT_FLAG_LENGTH_OVERFLOW))
        return true;

    return types::TypeCanHaveExtraIndexedProperties(cx, obj);
}

bool
TypeInferenceOracle::elementWriteIsPacked(RawScript script, jsbytecode *pc)
{
    StackTypeSet *types = script->analysis()->poppedTypes(pc, 2);
    return !types->hasObjectFlags(cx, types::OBJECT_FLAG_NON_PACKED);
}

bool
TypeInferenceOracle::setElementHasWrittenHoles(UnrootedScript script, jsbytecode *pc)
{
    return script->analysis()->getCode(pc).arrayWriteHole;
}

MIRType
TypeInferenceOracle::elementWrite(UnrootedScript script, jsbytecode *pc)
{
    StackTypeSet *objTypes = DropUnrooted(script)->analysis()->poppedTypes(pc, 2);
    MIRType elementType = MIRType_None;
    unsigned count = objTypes->getObjectCount();

    for (unsigned i = 0; i < count; i++) {
        if (objTypes->getSingleObject(i))
            return MIRType_None;

        if (TypeObject *object = objTypes->getTypeObject(i)) {
            if (object->unknownProperties())
                return MIRType_None;

            types::HeapTypeSet *elementTypes = object->getProperty(cx, JSID_VOID, false);
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

    return elementType;
}

bool
TypeInferenceOracle::arrayResultShouldHaveDoubleConversion(UnrootedScript script, jsbytecode *pc)
{
    types::StackTypeSet::DoubleConversion conversion =
        script->analysis()->pushedTypes(pc, 0)->convertDoubleElements(cx);
    return conversion == types::StackTypeSet::AlwaysConvertToDoubles;
}

bool
TypeInferenceOracle::canInlineCalls()
{
    return script()->analysis()->hasFunctionCalls();
}

bool
TypeInferenceOracle::propertyWriteCanSpecialize(UnrootedScript script, jsbytecode *pc)
{
    return !script->analysis()->getCode(pc).monitoredTypes;
}

bool
TypeInferenceOracle::propertyWriteNeedsBarrier(UnrootedScript script, jsbytecode *pc, RawId id)
{
    StackTypeSet *types = DropUnrooted(script)->analysis()->poppedTypes(pc, 1);
    return types->propertyNeedsBarrier(cx, id);
}

bool
TypeInferenceOracle::elementWriteNeedsBarrier(UnrootedScript script, jsbytecode *pc)
{
    
    
    StackTypeSet *types = DropUnrooted(script)->analysis()->poppedTypes(pc, 2);
    return types->propertyNeedsBarrier(cx, JSID_VOID);
}

StackTypeSet *
TypeInferenceOracle::getCallTarget(UnrootedScript caller, uint32_t argc, jsbytecode *pc)
{
    JS_ASSERT(caller == this->script());
    JS_ASSERT(js_CodeSpec[*pc].format & JOF_INVOKE);

    ScriptAnalysis *analysis = script()->analysis();
    return analysis->poppedTypes(pc, argc + 1);
}

StackTypeSet *
TypeInferenceOracle::getCallArg(UnrootedScript script, uint32_t argc, uint32_t arg, jsbytecode *pc)
{
    JS_ASSERT(argc >= arg);
    
    
    return script->analysis()->poppedTypes(pc, argc - arg);
}

StackTypeSet *
TypeInferenceOracle::getCallReturn(UnrootedScript script, jsbytecode *pc)
{
    return script->analysis()->pushedTypes(pc, 0);
}

bool
TypeInferenceOracle::canInlineCall(HandleScript caller, jsbytecode *pc)
{
    JS_ASSERT(types::IsInlinableCall(pc));

    JSOp op = JSOp(*pc);
    Bytecode *code = caller->analysis()->maybeCode(pc);

    
    
    if (op != JSOP_FUNAPPLY && code->monitoredTypes)
        return false;
    return true;
}

types::TypeBarrier*
TypeInferenceOracle::callArgsBarrier(HandleScript caller, jsbytecode *pc)
{
    JS_ASSERT(types::IsInlinableCall(pc));
    return caller->analysis()->typeBarriers(cx, pc);
}

bool
TypeInferenceOracle::canEnterInlinedFunction(RawScript caller, jsbytecode *pc, RawFunction target)
{
    AssertCanGC();
    RootedScript targetScript(cx, target->nonLazyScript());

    
    if (targetScript->length == 1) {
        if (!targetScript->ensureRanInference(cx))
            return false;
    }

    if (!targetScript->hasAnalysis() ||
        !targetScript->analysis()->ranInference() ||
        !targetScript->analysis()->ranSSA())
    {
        return false;
    }

    if (!targetScript->analysis()->ionInlineable())
        return false;

    if (targetScript->needsArgsObj())
        return false;

    if (!targetScript->compileAndGo)
        return false;

    if (targetScript->analysis()->usesScopeChain())
        return false;

    types::TypeObject *targetType = target->getType(cx);
    if (!targetType || targetType->unknownProperties())
        return false;

    JSOp op = JSOp(*pc);
    TypeSet *returnTypes = TypeScript::ReturnTypes(targetScript);
    TypeSet *callReturn = getCallReturn(caller, pc);
    if (op == JSOP_NEW) {
        if (!returnTypes->isSubsetIgnorePrimitives(callReturn))
            return false;
    } else {
        if (!returnTypes->isSubset(callReturn))
            return false;
    }

    
    HeapTypeSet::WatchObjectStateChange(cx, targetType);
    return true;
}

HeapTypeSet *
TypeInferenceOracle::globalPropertyWrite(UnrootedScript script, jsbytecode *pc, jsid id,
                                         bool *canSpecialize)
{
    *canSpecialize = !script->analysis()->getCode(pc).monitoredTypes;
    if (!*canSpecialize)
        return NULL;

    return globalPropertyTypeSet(DropUnrooted(script), pc, id);
}

StackTypeSet *
TypeInferenceOracle::returnTypeSet(UnrootedScript script, jsbytecode *pc, types::StackTypeSet **barrier)
{
    if (script->analysis()->getCode(pc).monitoredTypesReturn)
        *barrier = script->analysis()->bytecodeTypes(pc);
    else
        *barrier = NULL;
    return script->analysis()->pushedTypes(pc, 0);
}

StackTypeSet *
TypeInferenceOracle::aliasedVarBarrier(UnrootedScript script, jsbytecode *pc, types::StackTypeSet **barrier)
{
    *barrier = script->analysis()->bytecodeTypes(pc);
    return script->analysis()->pushedTypes(pc, 0);
}

HeapTypeSet *
TypeInferenceOracle::globalPropertyTypeSet(UnrootedScript script, jsbytecode *pc, jsid id)
{
    TypeObject *type = DropUnrooted(script)->global().getType(cx);
    if (!type || type->unknownProperties())
        return NULL;

    return type->getProperty(cx, id, false);
}

LazyArgumentsType
TypeInferenceOracle::isArgumentObject(types::StackTypeSet *obj)
{
    if (obj->isMagicArguments())
        return DefinitelyArguments;
    if (obj->hasAnyFlag(TYPE_FLAG_LAZYARGS))
        return MaybeArguments;
    return NotArguments;
}

LazyArgumentsType
TypeInferenceOracle::propertyReadMagicArguments(UnrootedScript script, jsbytecode *pc)
{
    StackTypeSet *obj = script->analysis()->poppedTypes(pc, 0);
    return isArgumentObject(obj);
}

LazyArgumentsType
TypeInferenceOracle::elementReadMagicArguments(UnrootedScript script, jsbytecode *pc)
{
    StackTypeSet *obj = script->analysis()->poppedTypes(pc, 1);
    return isArgumentObject(obj);
}

LazyArgumentsType
TypeInferenceOracle::elementWriteMagicArguments(UnrootedScript script, jsbytecode *pc)
{
    StackTypeSet *obj = script->analysis()->poppedTypes(pc, 2);
    return isArgumentObject(obj);
}
