








































#ifndef jsinfer_h___
#define jsinfer_h___

#include <sys/time.h>
#include "jsarena.h"
#include "jstl.h"
#include "jsprvtd.h"





namespace js { namespace analyze {
    struct Bytecode;
    class Script;
} }

namespace js {
namespace types {


struct TypeSet;
struct VariableSet;
struct TypeCallsite;
struct TypeObject;
struct TypeFunction;
struct TypeCompartment;






typedef jsword jstype;


const jstype TYPE_UNDEFINED = 1;
const jstype TYPE_NULL      = 2;
const jstype TYPE_BOOLEAN   = 3;
const jstype TYPE_INT32     = 4;
const jstype TYPE_DOUBLE    = 5;
const jstype TYPE_STRING    = 6;





const jstype TYPE_UNKNOWN = 7;


enum {
    TYPE_FLAG_UNDEFINED = 1 << TYPE_UNDEFINED,
    TYPE_FLAG_NULL      = 1 << TYPE_NULL,
    TYPE_FLAG_BOOLEAN   = 1 << TYPE_BOOLEAN,
    TYPE_FLAG_INT32     = 1 << TYPE_INT32,
    TYPE_FLAG_DOUBLE    = 1 << TYPE_DOUBLE,
    TYPE_FLAG_STRING    = 1 << TYPE_STRING,

    TYPE_FLAG_UNKNOWN   = 1 << TYPE_UNKNOWN,

    TYPE_FLAG_OBJECT   = 0x1000
};


typedef uint32 TypeFlags;






static inline bool
TypeIsPrimitive(jstype type)
{
    JS_ASSERT(type && type != TYPE_UNKNOWN);
    return type < TYPE_UNKNOWN;
}

static inline bool
TypeIsObject(jstype type)
{
    JS_ASSERT(type && type != TYPE_UNKNOWN);
    return type > TYPE_UNKNOWN;
}


inline jstype GetValueType(JSContext *cx, const Value &val);


void PrintType(JSContext *cx, jstype type, bool newline = true);





class TypeConstraint
{
public:
#ifdef JS_TYPES_DEBUG_SPEW
    static unsigned constraintCount;
    unsigned id;
    const char *kind;
#endif

    
    TypeConstraint *next;

    TypeConstraint(const char *_kind) : next(NULL)
    {
#ifdef JS_TYPES_DEBUG_SPEW
        id = ++constraintCount;
        kind = _kind;
#endif
    }

    
    virtual void newType(JSContext *cx, TypeSet *source, jstype type) = 0;
};


struct TypeSet
{
#ifdef JS_TYPES_DEBUG_SPEW
    static unsigned typesetCount;
    unsigned id;
#endif

    
    TypeFlags typeFlags;

    
    TypeObject **objectSet;
    unsigned objectCount;

    
    TypeConstraint *constraintList;

#ifdef DEBUG
    
    JSArenaPool *pool;
#endif

    TypeSet(JSArenaPool *pool)
        : typeFlags(0), objectSet(NULL), objectCount(0), constraintList(NULL)
    {
        setPool(pool);
    }

    void setPool(JSArenaPool *pool)
    {
#ifdef DEBUG
        this->pool = pool;
#ifdef JS_TYPES_DEBUG_SPEW
        this->id = ++typesetCount;
#endif
#endif
    }

    void print(JSContext *cx, FILE *out);

    
    inline bool hasType(jstype type);

    



    inline void addType(JSContext *cx, jstype type);

    
    void addSubset(JSContext *cx, JSArenaPool &pool, TypeSet *target);
    void addGetProperty(JSContext *cx, analyze::Bytecode *code, TypeSet *target, jsid id);
    void addSetProperty(JSContext *cx, analyze::Bytecode *code, TypeSet *target, jsid id);
    void addGetElem(JSContext *cx, analyze::Bytecode *code, TypeSet *object, TypeSet *target);
    void addSetElem(JSContext *cx, analyze::Bytecode *code, TypeSet *object, TypeSet *target);
    void addCall(JSContext *cx, TypeCallsite *site);
    void addArith(JSContext *cx, JSArenaPool &pool, analyze::Bytecode *code,
                  TypeSet *target, TypeSet *other = NULL);
    void addTransformThis(JSContext *cx, JSArenaPool &pool, TypeSet *target);
    void addFilterPrimitives(JSContext *cx, JSArenaPool &pool, TypeSet *target, bool onlyNullVoid);
    void addMonitorRead(JSContext *cx, JSArenaPool &pool, analyze::Bytecode *code, TypeSet *target);

    
    void addFreeze(JSContext *cx, JSArenaPool &pool, analyze::Bytecode *code);
    void addFreezeProp(JSContext *cx, JSArenaPool &pool, analyze::Bytecode *code, jsid id);
    void addFreezeElem(JSContext *cx, JSArenaPool &pool, analyze::Bytecode *code, TypeSet *object);

    



    static inline TypeSet* make(JSContext *cx, JSArenaPool &pool, const char *name);

  private:
    inline void add(JSContext *cx, TypeConstraint *constraint, bool callExisting = true);
};






struct TypeStack
{
    




    TypeStack *mergedGroup;

    
    int id;

    
    unsigned stackDepth;

    
    TypeStack *innerStack;

    
    TypeSet types;

    
    bool hasMerged;

    
    bool boundWith;

    
    bool isForEach;

    
    jsid letVariable;

    
    VariableSet *scopeVars;
    analyze::Script *scopeScript;

    
    inline TypeStack* group();

    
    inline void setInnerStack(TypeStack *inner);

    
    static void merge(JSContext *cx, TypeStack *one, TypeStack *two);
};






struct TypeCallsite
{
    
    analyze::Bytecode *code;

    
    bool isNew;

    
    TypeSet **argumentTypes;
    unsigned argumentCount;

    
    TypeSet *thisTypes;

    
    jstype thisType;

    
    TypeSet *returnTypes;

    inline TypeCallsite(analyze::Bytecode *code, bool isNew, unsigned argumentCount);

    
    inline void forceThisTypes(JSContext *cx);
    inline void forceReturnTypes(JSContext *cx);

    
    inline TypeObject* getInitObject(JSContext *cx, bool isArray);

    
    inline JSArenaPool & pool();
};


struct Variable
{
    



    jsid id;

    
    Variable *next;

    




    TypeSet types;

    Variable(JSArenaPool *pool, jsid id)
        : id(id), next(NULL), types(pool)
    {}
};


struct VariableSet
{
#ifdef JS_TYPES_DEBUG_SPEW
    jsid name;
#endif

    
    Variable *variables;

    



    VariableSet **propagateSet;
    unsigned propagateCount;

    JSArenaPool *pool;

    VariableSet(JSArenaPool *pool)
        : variables(NULL), propagateSet(NULL), propagateCount(NULL), pool(pool)
    {
        JS_ASSERT(pool);
    }

    
    inline TypeSet* getVariable(JSContext *cx, jsid id);

    





    bool addPropagate(JSContext *cx, VariableSet *target, bool excludePrototype);

    void print(JSContext *cx, FILE *out);
};


struct TypeObject
{
    




    jsid name;

    
    bool isFunction;

    



    bool monitored;

    




    VariableSet propertySet;
    bool propertiesFilled;

    
    TypeObject *next;

    



    bool hasObjectPropagation;
    bool hasArrayPropagation;

    




    bool isInitObject;

    
    TypeObject(JSContext *cx, JSArenaPool *pool, jsid id);

    
    bool addPropagate(JSContext *cx, TypeObject *target, bool excludePrototype = true);

    
    TypeFunction* asFunction()
    {
        if (isFunction) {
            return (TypeFunction*) this;
        } else {
            JS_NOT_REACHED("Object is not a function");
            return NULL;
        }
    }

    JSArenaPool & pool() { return *propertySet.pool; }

    
    inline VariableSet& properties(JSContext *cx);

    
    inline TypeSet* indexTypes(JSContext *cx);

    void print(JSContext *cx, FILE *out);

    



    void setMonitored(JSContext *cx);
};


struct TypeFunction : public TypeObject
{
    
    JSTypeHandler handler;

    
    JSScript *script;

    



    TypeObject *prototypeObject;

    
    TypeObject *newObject;

    



    TypeSet returnTypes;

    



    bool isBuiltin;

    




    bool isGeneric;

    TypeFunction(JSContext *cx, JSArenaPool *pool, jsid id);

    
    inline TypeObject* getNewObject(JSContext *cx);

    
    TypeObject* prototype(JSContext *cx)
    {
        
        properties(cx);
        return prototypeObject;
    }

    void fillProperties(JSContext *cx);
};

inline VariableSet&
TypeObject::properties(JSContext *cx)
{
    if (!propertiesFilled) {
        propertiesFilled = true;
        if (isFunction)
            asFunction()->fillProperties(cx);
    }
    return propertySet;
}






enum FixedTypeObjectName
{
    
    TYPE_OBJECT_OBJECT,
    TYPE_OBJECT_FUNCTION,
    TYPE_OBJECT_ARRAY,
    TYPE_OBJECT_FUNCTION_PROTOTYPE,

    TYPE_OBJECT_FUNCTION_LAST = TYPE_OBJECT_FUNCTION_PROTOTYPE,

    
    TYPE_OBJECT_OBJECT_PROTOTYPE,
    TYPE_OBJECT_ARRAY_PROTOTYPE,
    TYPE_OBJECT_NEW_BOOLEAN,
    TYPE_OBJECT_NEW_DATE,
    TYPE_OBJECT_NEW_ERROR,
    TYPE_OBJECT_NEW_ITERATOR,
    TYPE_OBJECT_NEW_NUMBER,
    TYPE_OBJECT_NEW_STRING,
    TYPE_OBJECT_NEW_PROXY,
    TYPE_OBJECT_NEW_REGEXP,
    TYPE_OBJECT_NEW_ARRAYBUFFER,
    TYPE_OBJECT_MAGIC,   
    TYPE_OBJECT_GETSET,  

    TYPE_OBJECT_BASE_LAST = TYPE_OBJECT_GETSET,

    
    TYPE_OBJECT_NEW_XML,
    TYPE_OBJECT_NEW_QNAME,
    TYPE_OBJECT_NEW_NAMESPACE,
    TYPE_OBJECT_ARGUMENTS,
    TYPE_OBJECT_NOSUCHMETHOD,
    TYPE_OBJECT_NOSUCHMETHOD_ARGUMENTS,
    TYPE_OBJECT_PROPERTY_DESCRIPTOR,
    TYPE_OBJECT_KEY_VALUE_PAIR,

    TYPE_OBJECT_MONITOR_LAST = TYPE_OBJECT_KEY_VALUE_PAIR,

    
    TYPE_OBJECT_REGEXP_MATCH_ARRAY,
    TYPE_OBJECT_STRING_SPLIT_ARRAY,
    TYPE_OBJECT_UNKNOWN_ARRAY,
    TYPE_OBJECT_CLONE_ARRAY,
    TYPE_OBJECT_PROPERTY_ARRAY,
    TYPE_OBJECT_NAMESPACE_ARRAY,
    TYPE_OBJECT_JSON_ARRAY,
    TYPE_OBJECT_REFLECT_ARRAY,

    TYPE_OBJECT_ARRAY_LAST = TYPE_OBJECT_REFLECT_ARRAY,

    
    TYPE_OBJECT_UNKNOWN_OBJECT,
    TYPE_OBJECT_CLONE_OBJECT,
    TYPE_OBJECT_JSON_STRINGIFY,
    TYPE_OBJECT_JSON_REVIVE,
    TYPE_OBJECT_JSON_OBJECT,
    TYPE_OBJECT_REFLECT_OBJECT,
    TYPE_OBJECT_XML_SETTINGS,

    
    TYPE_OBJECT_REGEXP_STATICS,
    TYPE_OBJECT_CALL,
    TYPE_OBJECT_DECLENV,
    TYPE_OBJECT_SHARP_ARRAY,
    TYPE_OBJECT_WITH,
    TYPE_OBJECT_BLOCK,
    TYPE_OBJECT_NULL_CLOSURE,
    TYPE_OBJECT_PROPERTY_ITERATOR,
    TYPE_OBJECT_SCRIPT,

    TYPE_OBJECT_FIXED_LIMIT
};

extern const char* const fixedTypeObjectNames[];


struct TypeCompartment
{
    



    JSArenaPool pool;
    TypeObject *objects;

    TypeObject *fixedTypeObjects[TYPE_OBJECT_FIXED_LIMIT];

    
    unsigned scriptCount;

    
    bool interpreting;

    
    static const unsigned GETID_COUNT = 2;
    char *scratchBuf[GETID_COUNT];
    unsigned scratchLen[GETID_COUNT];

    
    TypeObject *globalObject;

    struct IdHasher
    {
        typedef jsid Lookup;
        static uint32 hashByte(uint32 hash, uint8 byte) {
            hash = (hash << 4) + byte;
            uint32 x = hash & 0xF0000000L;
            if (x)
                hash ^= (x >> 24);
            return hash & ~x;
        }
        static uint32 hash(jsid id) {
            
            uint32 hash = 0, v = uint32(JSID_BITS(id));
            hash = hashByte(hash, v & 0xff);
            v >>= 8;
            hash = hashByte(hash, v & 0xff);
            v >>= 8;
            hash = hashByte(hash, v & 0xff);
            v >>= 8;
            return hashByte(hash, v & 0xff);
        }
        static bool match(jsid id0, jsid id1) {
            return id0 == id1;
        }
    };

    
    typedef HashMap<jsid, TypeObject*, IdHasher, SystemAllocPolicy> ObjectNameTable;
    ObjectNameTable *objectNameTable;

    

    
    struct PendingWork
    {
        TypeConstraint *constraint;
        TypeSet *source;
        jstype type;
    };

    



    PendingWork *pendingArray;
    unsigned pendingCount;
    unsigned pendingCapacity;

    
    bool resolving;

    

    
    FILE *out;

    



    bool warnings;

    



    bool ignoreWarnings;

    



    uint64_t analysisTime;

    
    unsigned recompilations;

    
    static const unsigned TYPE_COUNT_LIMIT = 4;
    unsigned typeCounts[TYPE_COUNT_LIMIT];
    unsigned typeCountOver;

    void init();
    ~TypeCompartment();

    uint64 currentTime()
    {
        timeval current;
        gettimeofday(&current, NULL);
        return current.tv_sec * (uint64_t) 1000000 + current.tv_usec;
    }

    TypeObject *makeFixedTypeObject(JSContext *cx, FixedTypeObjectName which);

    
    inline void addPending(JSContext *cx, TypeConstraint *constraint, TypeSet *source, jstype type);
    void growPendingArray();

    
    inline void resolvePending(JSContext *cx);

    void print(JSContext *cx, JSCompartment *compartment);

    
    TypeObject *getTypeObject(JSContext *cx, js::analyze::Script *script,
                              const char *name, bool isFunction);

    



    void addDynamicType(JSContext *addCx, TypeSet *types, jstype type,
                        const char *format, ...);

    
    inline void monitorBytecode(analyze::Bytecode *code);

    
    inline void recompileScript(analyze::Bytecode *code);
};

} 
} 

static JS_ALWAYS_INLINE js::types::TypeObject *
Valueify(JSTypeObject *jstype) { return (js::types::TypeObject*) jstype; }

static JS_ALWAYS_INLINE js::types::TypeFunction *
Valueify(JSTypeFunction *jstype) { return (js::types::TypeFunction*) jstype; }

static JS_ALWAYS_INLINE js::types::TypeCallsite *
Valueify(JSTypeCallsite *jssite) { return (js::types::TypeCallsite*) jssite; }

#endif 
