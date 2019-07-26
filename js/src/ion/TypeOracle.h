








































#ifndef js_ion_type_oracle_h__
#define js_ion_type_oracle_h__

#include "jsscript.h"
#include "IonTypes.h"

namespace js {
namespace ion {




enum MIRType
{
    MIRType_Undefined,
    MIRType_Null,
    MIRType_Boolean,
    MIRType_Int32,
    MIRType_Double,
    MIRType_String,
    MIRType_Object,
    MIRType_Magic,
    MIRType_Value,
    MIRType_Any,        
    MIRType_None,       
    MIRType_Slots,      
    MIRType_Elements,   
    MIRType_UpvarSlots, 
    MIRType_StackFrame, 
    MIRType_ArgObj      
};

enum LazyArgumentsType {
    MaybeArguments = 0,
    DefinitelyArguments,
    NotArguments
};

class TypeOracle
{
  public:
    struct UnaryTypes {
        types::TypeSet *inTypes;
        types::TypeSet *outTypes;
    };

    struct BinaryTypes {
        types::TypeSet *lhsTypes;
        types::TypeSet *rhsTypes;
        types::TypeSet *outTypes;
    };

    struct Unary {
        MIRType ival;
        MIRType rval;
    };
    struct Binary {
        MIRType lhs;
        MIRType rhs;
        MIRType rval;
    };

  public:
    virtual UnaryTypes unaryTypes(JSScript *script, jsbytecode *pc) = 0;
    virtual BinaryTypes binaryTypes(JSScript *script, jsbytecode *pc) = 0;
    virtual Unary unaryOp(JSScript *script, jsbytecode *pc) = 0;
    virtual Binary binaryOp(JSScript *script, jsbytecode *pc) = 0;
    virtual types::TypeSet *thisTypeSet(JSScript *script) { return NULL; }
    virtual bool getOsrTypes(jsbytecode *osrPc, Vector<MIRType> &slotTypes) { return true; }
    virtual types::TypeSet *parameterTypeSet(JSScript *script, size_t index) { return NULL; }
    virtual types::TypeSet *globalPropertyTypeSet(JSScript *script, jsbytecode *pc, jsid id) {
        return NULL;
    }
    virtual types::TypeSet *propertyRead(JSScript *script, jsbytecode *pc) {
        return NULL;
    }
    virtual types::TypeSet *propertyReadBarrier(JSScript *script, jsbytecode *pc) {
        return NULL;
    }
    virtual bool propertyReadIdempotent(JSScript *script, jsbytecode *pc, HandleId id) {
        return false;
    }
    virtual types::TypeSet *globalPropertyWrite(JSScript *script, jsbytecode *pc,
                                                jsid id, bool *canSpecialize) {
        *canSpecialize = true;
        return NULL;
    }
    virtual types::TypeSet *returnTypeSet(JSScript *script, jsbytecode *pc, types::TypeSet **barrier) {
        *barrier = NULL;
        return NULL;
    }
    virtual bool elementReadIsDenseArray(JSScript *script, jsbytecode *pc) {
        return false;
    }
    virtual bool elementReadIsTypedArray(JSScript *script, jsbytecode *pc, int *arrayType) {
        return false;
    }
    virtual bool elementReadIsString(JSScript *script, jsbytecode *pc) {
        return false;
    }
    virtual bool elementReadIsPacked(JSScript *script, jsbytecode *pc) {
        return false;
    }
    virtual void elementReadGeneric(JSScript *script, jsbytecode *pc, bool *cacheable, bool *monitorResult) {
        *cacheable = false;
        *monitorResult = true;
    }
    virtual bool setElementHasWrittenHoles(JSScript *script, jsbytecode *pc) {
        return true;
    }
    virtual bool elementWriteIsDenseArray(JSScript *script, jsbytecode *pc) {
        return false;
    }
    virtual bool elementWriteIsTypedArray(JSScript *script, jsbytecode *pc, int *arrayType) {
        return false;
    }
    virtual bool elementWriteIsPacked(JSScript *script, jsbytecode *pc) {
        return false;
    }
    virtual bool propertyWriteCanSpecialize(JSScript *script, jsbytecode *pc) {
        return true;
    }
    virtual bool elementWriteNeedsBarrier(JSScript *script, jsbytecode *pc) {
        return true;
    }
    virtual MIRType elementWrite(JSScript *script, jsbytecode *pc) {
        return MIRType_None;
    }
    virtual bool arrayPrototypeHasIndexedProperty() {
        return true;
    }
    virtual bool canInlineCalls() {
        return false;
    }

    
    virtual types::TypeSet *getCallTarget(JSScript *caller, uint32 argc, jsbytecode *pc) {
        
        JS_ASSERT(js_CodeSpec[*pc].format & JOF_INVOKE && JSOp(*pc) != JSOP_EVAL);
        return NULL;
    }
    virtual types::TypeSet *getCallArg(JSScript *script, uint32 argc, uint32 arg, jsbytecode *pc) {
        return NULL;
    }
    virtual types::TypeSet *getCallReturn(JSScript *script, jsbytecode *pc) {
        return NULL;
    }
    virtual bool canInlineCall(JSScript *caller, jsbytecode *pc) {
        return false;
    }
    virtual bool canEnterInlinedFunction(JSFunction *callee) {
        return false;
    }
    virtual MIRType aliasedVarType(JSScript *script, jsbytecode *pc)  {
        return MIRType_Value;
    }

    virtual LazyArgumentsType isArgumentObject(types::TypeSet *obj) {
        return MaybeArguments;
    }
    virtual LazyArgumentsType propertyReadMagicArguments(JSScript *script, jsbytecode *pc) {
        return MaybeArguments;
    }
    virtual LazyArgumentsType elementReadMagicArguments(JSScript *script, jsbytecode *pc) {
        return MaybeArguments;
    }
    virtual LazyArgumentsType elementWriteMagicArguments(JSScript *script, jsbytecode *pc) {
        return MaybeArguments;
    }
    virtual Binary incslot(JSScript *script, jsbytecode *pc) {
        return binaryOp(script, pc);
    }
};

class DummyOracle : public TypeOracle
{
  public:
    UnaryTypes unaryTypes(JSScript *script, jsbytecode *pc) {
        UnaryTypes u;
        u.inTypes = NULL;
        u.outTypes = NULL;
        return u;
    }
    BinaryTypes binaryTypes(JSScript *script, jsbytecode *pc) {
        BinaryTypes b;
        b.lhsTypes = NULL;
        b.rhsTypes = NULL;
        b.outTypes = NULL;
        return b;
    }
    Unary unaryOp(JSScript *script, jsbytecode *pc) {
        Unary u;
        u.ival = MIRType_Int32;
        u.rval = MIRType_Int32;
        return u;
    }
    Binary binaryOp(JSScript *script, jsbytecode *pc) {
        Binary b;
        b.lhs = MIRType_Int32;
        b.rhs = MIRType_Int32;
        b.rval = MIRType_Int32;
        return b;
    }
};

class TypeInferenceOracle : public TypeOracle
{
    JSContext *cx;
    JSScript *script;

    MIRType getMIRType(types::TypeSet *types);

  public:
    TypeInferenceOracle() : cx(NULL), script(NULL) {}

    bool init(JSContext *cx, JSScript *script);

    UnaryTypes unaryTypes(JSScript *script, jsbytecode *pc);
    BinaryTypes binaryTypes(JSScript *script, jsbytecode *pc);
    Unary unaryOp(JSScript *script, jsbytecode *pc);
    Binary binaryOp(JSScript *script, jsbytecode *pc);
    types::TypeSet *thisTypeSet(JSScript *script);
    bool getOsrTypes(jsbytecode *osrPc, Vector<MIRType> &slotTypes);
    types::TypeSet *parameterTypeSet(JSScript *script, size_t index);
    types::TypeSet *globalPropertyTypeSet(JSScript *script, jsbytecode *pc, jsid id);
    types::TypeSet *propertyRead(JSScript *script, jsbytecode *pc);
    types::TypeSet *propertyReadBarrier(JSScript *script, jsbytecode *pc);
    bool propertyReadIdempotent(JSScript *script, jsbytecode *pc, HandleId id);
    types::TypeSet *globalPropertyWrite(JSScript *script, jsbytecode *pc, jsid id, bool *canSpecialize);
    types::TypeSet *returnTypeSet(JSScript *script, jsbytecode *pc, types::TypeSet **barrier);
    types::TypeSet *getCallTarget(JSScript *caller, uint32 argc, jsbytecode *pc);
    types::TypeSet *getCallArg(JSScript *caller, uint32 argc, uint32 arg, jsbytecode *pc);
    types::TypeSet *getCallReturn(JSScript *caller, jsbytecode *pc);
    bool elementReadIsDenseArray(JSScript *script, jsbytecode *pc);
    bool elementReadIsTypedArray(JSScript *script, jsbytecode *pc, int *atype);
    bool elementReadIsString(JSScript *script, jsbytecode *pc);
    bool elementReadIsPacked(JSScript *script, jsbytecode *pc);
    void elementReadGeneric(JSScript *script, jsbytecode *pc, bool *cacheable, bool *monitorResult);
    bool elementWriteIsDenseArray(JSScript *script, jsbytecode *pc);
    bool elementWriteIsTypedArray(JSScript *script, jsbytecode *pc, int *arrayType);
    bool elementWriteIsPacked(JSScript *script, jsbytecode *pc);
    bool setElementHasWrittenHoles(JSScript *script, jsbytecode *pc);
    bool propertyWriteCanSpecialize(JSScript *script, jsbytecode *pc);
    bool elementWriteNeedsBarrier(JSScript *script, jsbytecode *pc);
    MIRType elementWrite(JSScript *script, jsbytecode *pc);
    bool arrayPrototypeHasIndexedProperty();
    bool canInlineCalls();
    bool canInlineCall(JSScript *caller, jsbytecode *pc);
    bool canEnterInlinedFunction(JSFunction *callee);
    MIRType aliasedVarType(JSScript *script, jsbytecode *pc);

    LazyArgumentsType isArgumentObject(types::TypeSet *obj);
    LazyArgumentsType propertyReadMagicArguments(JSScript *script, jsbytecode *pc);
    LazyArgumentsType elementReadMagicArguments(JSScript *script, jsbytecode *pc);
    LazyArgumentsType elementWriteMagicArguments(JSScript *script, jsbytecode *pc);

    Binary incslot(JSScript *script, jsbytecode *pc);
};

static inline MIRType
MIRTypeFromValueType(JSValueType type)
{
    switch (type) {
      case JSVAL_TYPE_DOUBLE:
        return MIRType_Double;
      case JSVAL_TYPE_INT32:
        return MIRType_Int32;
      case JSVAL_TYPE_UNDEFINED:
        return MIRType_Undefined;
      case JSVAL_TYPE_STRING:
        return MIRType_String;
      case JSVAL_TYPE_BOOLEAN:
        return MIRType_Boolean;
      case JSVAL_TYPE_NULL:
        return MIRType_Null;
      case JSVAL_TYPE_OBJECT:
        return MIRType_Object;
      case JSVAL_TYPE_MAGIC:
        return MIRType_Magic;
      case JSVAL_TYPE_UNKNOWN:
        return MIRType_Value;
      default:
        JS_NOT_REACHED("unexpected jsval type");
        return MIRType_None;
    }
}

static inline JSValueType
ValueTypeFromMIRType(MIRType type)
{
  switch (type) {
    case MIRType_Undefined:
      return JSVAL_TYPE_UNDEFINED;
    case MIRType_Null:
      return JSVAL_TYPE_NULL;
    case MIRType_Boolean:
      return JSVAL_TYPE_BOOLEAN;
    case MIRType_Int32:
      return JSVAL_TYPE_INT32;
    case MIRType_Double:
      return JSVAL_TYPE_DOUBLE;
    case MIRType_String:
      return JSVAL_TYPE_STRING;
    case MIRType_Magic:
      return JSVAL_TYPE_MAGIC;
    default:
      JS_ASSERT(type == MIRType_Object);
      return JSVAL_TYPE_OBJECT;
  }
}

static inline JSValueTag
MIRTypeToTag(MIRType type)
{
    return JSVAL_TYPE_TO_TAG(ValueTypeFromMIRType(type));
}

static inline const char *
StringFromMIRType(MIRType type)
{
  switch (type) {
    case MIRType_Undefined:
      return "Undefined";
    case MIRType_Null:
      return "Null";
    case MIRType_Boolean:
      return "Bool";
    case MIRType_Int32:
      return "Int32";
    case MIRType_Double:
      return "Double";
    case MIRType_String:
      return "String";
    case MIRType_Object:
      return "Object";
    case MIRType_Magic:
      return "Magic";
    case MIRType_Value:
      return "Value";
    case MIRType_Any:
      return "Any";
    case MIRType_None:
      return "None";
    case MIRType_Slots:
      return "Slots";
    case MIRType_Elements:
      return "Elements";
    case MIRType_UpvarSlots:
      return "UpvarSlots";
    case MIRType_StackFrame:
      return "StackFrame";
    case MIRType_ArgObj:
      return "ArgumentsObject";
    default:
      JS_NOT_REACHED("Unknown MIRType.");
      return "";
  }
}

static inline bool
IsNumberType(MIRType type)
{
    return type == MIRType_Int32 || type == MIRType_Double;
}

static inline bool
IsNullOrUndefined(MIRType type)
{
    return type == MIRType_Null || type == MIRType_Undefined;
}

} 
} 

#endif 

