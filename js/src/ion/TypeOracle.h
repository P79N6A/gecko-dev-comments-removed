








































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
    MIRType_Value,
    MIRType_Any,        
    MIRType_None,       
    MIRType_Slots,      
    MIRType_Elements,   
    MIRType_StackFrame  
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
    virtual void getNewTypesAtJoinPoint(JSScript *script, jsbytecode *pc, Vector<MIRType> &slotTypes) { }
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
    virtual types::TypeSet *globalPropertyWrite(JSScript *script, jsbytecode *pc,
                                                jsid id, bool *canSpecialize) {
        *canSpecialize = true;
        return NULL;
    }
    virtual types::TypeSet *returnTypeSet(JSScript *script, jsbytecode *pc, types::TypeSet **barrier) {
        *barrier = NULL;
        return NULL;
    }
    virtual bool elementReadIsDense(JSScript *script, jsbytecode *pc) {
        return false;
    }
    virtual bool elementReadIsPacked(JSScript *script, jsbytecode *pc) {
        return false;
    }
    virtual bool setElementHasWrittenHoles(JSScript *script, jsbytecode *pc) {
        return true;
    }
    virtual bool elementWriteIsDense(JSScript *script, jsbytecode *pc) {
        return false;
    }
    virtual bool elementWriteIsPacked(JSScript *script, jsbytecode *pc) {
        return false;
    }
    virtual bool propertyWriteCanSpecialize(JSScript *script, jsbytecode *pc) {
        return true;
    }
    virtual bool propertyWriteNeedsBarrier(JSScript *script, jsbytecode *pc, jsid id) {
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
        JS_ASSERT(JSOp(*pc) == JSOP_CALL);
        return NULL;
    }

    virtual bool canEnterInlinedScript(JSScript *callee) {
        return false;
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
    void getNewTypesAtJoinPoint(JSScript *script, jsbytecode *pc, Vector<MIRType> &slotTypes);
    types::TypeSet *parameterTypeSet(JSScript *script, size_t index);
    types::TypeSet *globalPropertyTypeSet(JSScript *script, jsbytecode *pc, jsid id);
    types::TypeSet *propertyRead(JSScript *script, jsbytecode *pc);
    types::TypeSet *propertyReadBarrier(JSScript *script, jsbytecode *pc);
    types::TypeSet *globalPropertyWrite(JSScript *script, jsbytecode *pc, jsid id, bool *canSpecialize);
    types::TypeSet *returnTypeSet(JSScript *script, jsbytecode *pc, types::TypeSet **barrier);
    types::TypeSet *getCallTarget(JSScript *caller, uint32 argc, jsbytecode *pc);
    bool elementReadIsDense(JSScript *script, jsbytecode *pc);
    bool elementReadIsPacked(JSScript *script, jsbytecode *pc);
    bool elementWriteIsDense(JSScript *script, jsbytecode *pc);
    bool elementWriteIsPacked(JSScript *script, jsbytecode *pc);
    bool setElementHasWrittenHoles(JSScript *script, jsbytecode *pc);
    bool propertyWriteCanSpecialize(JSScript *script, jsbytecode *pc);
    bool propertyWriteNeedsBarrier(JSScript *script, jsbytecode *pc, jsid id);
    MIRType elementWrite(JSScript *script, jsbytecode *pc);
    bool arrayPrototypeHasIndexedProperty();
    bool canInlineCalls();
    bool canEnterInlinedScript(JSScript *inlineScript);
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
    case MIRType_StackFrame:
      return "StackFrame";
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

} 
} 

#endif 

