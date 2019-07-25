








































#ifndef js_ion_type_oracle_h__
#define js_ion_type_oracle_h__

#include "jsscript.h"

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
    MIRType_Slots       
};

class TypeOracle
{
  public:
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
    virtual Unary unaryOp(JSScript *script, jsbytecode *pc) = 0;
    virtual Binary binaryOp(JSScript *script, jsbytecode *pc) = 0;
    virtual types::TypeSet *thisTypeSet(JSScript *script) { return NULL; }
    virtual types::TypeSet *parameterTypeSet(JSScript *script, size_t index) { return NULL; }
    virtual types::TypeSet *propertyRead(JSScript *script, jsbytecode *pc,
                                         types::TypeSet **barrier) {
        *barrier = NULL;
        return NULL;
    }
};

class DummyOracle : public TypeOracle
{
  public:
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

    Unary unaryOp(JSScript *script, jsbytecode *pc);
    Binary binaryOp(JSScript *script, jsbytecode *pc);
    types::TypeSet *thisTypeSet(JSScript *script);
    types::TypeSet *parameterTypeSet(JSScript *script, size_t index);
    types::TypeSet *propertyRead(JSScript *script, jsbytecode *pc, types::TypeSet **barrier);
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

