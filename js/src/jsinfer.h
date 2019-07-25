








































#ifndef jsinfer_h___
#define jsinfer_h___

#include "jsarena.h"
#include "jstl.h"
#include "jsprvtd.h"
#include "jsvalue.h"

#ifndef _MSC_VER
#include <sys/time.h>
#endif

namespace js {
    struct CallArgs;
namespace analyze {
    struct Bytecode;
    class Script;
} }

namespace js {
namespace types {


struct TypeSet;
struct TypeCallsite;
struct TypeObject;
struct TypeFunction;
struct TypeCompartment;
struct ClonedTypeSet;






typedef jsword jstype;


const jstype TYPE_UNDEFINED = 1;
const jstype TYPE_NULL      = 2;
const jstype TYPE_BOOLEAN   = 3;
const jstype TYPE_INT32     = 4;
const jstype TYPE_DOUBLE    = 5;
const jstype TYPE_STRING    = 6;





const jstype TYPE_UNKNOWN = 7;






static inline bool
TypeIsPrimitive(jstype type)
{
    JS_ASSERT(type);
    return type < TYPE_UNKNOWN;
}

static inline bool
TypeIsObject(jstype type)
{
    JS_ASSERT(type);
    return type > TYPE_UNKNOWN;
}


inline jstype GetValueType(JSContext *cx, const Value &val);






























class TypeConstraint
{
public:
#ifdef DEBUG
    const char *kind_;
    const char *kind() const { return kind_; }
#else
    const char *kind() const { return NULL; }
#endif

    
    TypeConstraint *next;

    




    JSScript *script;

    TypeConstraint(const char *kind, JSScript *script)
        : next(NULL), script(script)
    {
        JS_ASSERT(script);
#ifdef DEBUG
        this->kind_ = kind;
#endif
    }

    
    virtual void newType(JSContext *cx, TypeSet *source, jstype type) = 0;

    




    virtual void newObjectState(JSContext *cx) {}

    





    virtual bool condensed() { return false; }

    




    virtual TypeObject * baseSubset() { return NULL; }
};
















enum ObjectKind {
    OBJECT_NONE,
    OBJECT_UNKNOWN,
    OBJECT_PACKED_ARRAY,
    OBJECT_DENSE_ARRAY,
    OBJECT_SCRIPTED_FUNCTION,
    OBJECT_NATIVE_FUNCTION,
    OBJECT_NO_SPECIAL_EQUALITY
};


enum {
    TYPE_FLAG_UNDEFINED = 1 << TYPE_UNDEFINED,
    TYPE_FLAG_NULL      = 1 << TYPE_NULL,
    TYPE_FLAG_BOOLEAN   = 1 << TYPE_BOOLEAN,
    TYPE_FLAG_INT32     = 1 << TYPE_INT32,
    TYPE_FLAG_DOUBLE    = 1 << TYPE_DOUBLE,
    TYPE_FLAG_STRING    = 1 << TYPE_STRING,

    TYPE_FLAG_UNKNOWN   = 1 << TYPE_UNKNOWN,

    
    TYPE_FLAG_INTERMEDIATE_SET = 0x1000
};


typedef uint32 TypeFlags;


struct TypeSet
{
    
    TypeFlags typeFlags;

    
    TypeObject **objectSet;
    unsigned objectCount;

    
    TypeConstraint *constraintList;

    TypeSet()
        : typeFlags(0), objectSet(NULL), objectCount(0), constraintList(NULL)
    {}

    void print(JSContext *cx);

    void setIntermediate() { typeFlags |= TYPE_FLAG_INTERMEDIATE_SET; }

    inline void destroy(JSContext *cx);

    
    inline bool hasType(jstype type);

    bool unknown() { return typeFlags & TYPE_FLAG_UNKNOWN; }

    



    inline void addType(JSContext *cx, jstype type);

    
    void addTypeSet(JSContext *cx, ClonedTypeSet *types);

    
    inline void add(JSContext *cx, TypeConstraint *constraint, bool callExisting = true);
    void addSubset(JSContext *cx, JSScript *script, TypeSet *target);
    void addGetProperty(JSContext *cx, JSScript *script, const jsbytecode *pc,
                        TypeSet *target, jsid id);
    void addSetProperty(JSContext *cx, JSScript *script, const jsbytecode *pc,
                        TypeSet *target, jsid id);
    void addNewObject(JSContext *cx, JSScript *script, TypeFunction *fun, TypeSet *target);
    void addCall(JSContext *cx, TypeCallsite *site);
    void addArith(JSContext *cx, JSScript *script,
                  TypeSet *target, TypeSet *other = NULL);
    void addTransformThis(JSContext *cx, JSScript *script, TypeSet *target);
    void addFilterPrimitives(JSContext *cx, JSScript *script,
                             TypeSet *target, bool onlyNullVoid);
    void addMonitorRead(JSContext *cx, JSScript *script, TypeSet *target);

    void addBaseSubset(JSContext *cx, TypeObject *object, TypeSet *target);
    void addCondensed(JSContext *cx, JSScript *script);

    



    static inline TypeSet* make(JSContext *cx, const char *name);

    






    
    JSValueType getKnownTypeTag(JSContext *cx, JSScript *script);

    
    ObjectKind getKnownObjectKind(JSContext *cx, JSScript *script);

    
    bool hasUnknownProperties(JSContext *cx, JSScript *script);

    
    bool knownNonEmpty(JSContext *cx, JSScript *script);

    
    void pushAllTypes(JSContext *cx, JSScript *script, const jsbytecode *pc);

    



    static void Clone(JSContext *cx, JSScript *script, TypeSet *source, ClonedTypeSet *target);

  private:
    inline void markUnknown(JSContext *cx);
};


struct ClonedTypeSet
{
    TypeFlags typeFlags;
    TypeObject **objectSet;
    unsigned objectCount;
};


struct Property
{
    
    jsid id;

    
    TypeSet types;

    
    TypeSet ownTypes;

    Property(jsid id)
        : id(id)
    {}

    static uint32 keyBits(jsid id) { return (uint32) JSID_BITS(id); }
    static jsid getKey(Property *p) { return p->id; }
};


struct TypeObject
{
#ifdef DEBUG
    
    jsid name_;
#endif

    
    JSObject *proto;

    
    js::EmptyShape **emptyShapes;

    
    bool isFunction;

    
    bool marked;

    



    bool initializerObject;
    bool initializerArray;
    uint32 initializerOffset;

    










    uint32 contribution;
    static const uint32 CONTRIBUTION_LIMIT = 20000;

    




    Property **propertySet;
    unsigned propertyCount;

    
    TypeObject *instanceList;

    
    TypeObject *instanceNext;

    
    TypeObject *next;

    






    bool unknownProperties;

    
    bool isDenseArray;

    
    bool isPackedArray;

    
    bool hasSpecialEquality;

    TypeObject() {}

    
    inline TypeObject(jsid id, JSObject *proto);

    
    TypeFunction* asFunction()
    {
        JS_ASSERT(isFunction);
        return (TypeFunction *) this;
    }

    






    inline bool canProvideEmptyShape(js::Class *clasp);
    inline js::EmptyShape *getEmptyShape(JSContext *cx, js::Class *aclasp,
                                          unsigned kind);

    





    inline TypeSet *getProperty(JSContext *cx, jsid id, bool assign);

    inline const char * name();

    
    void splicePrototype(JSContext *cx, JSObject *proto);

    

    bool addProperty(JSContext *cx, jsid id, Property **pprop);
    void addPrototype(JSContext *cx, TypeObject *proto);
    void markNotPacked(JSContext *cx, bool notDense);
    void markUnknown(JSContext *cx);
    void storeToInstances(JSContext *cx, Property *base);
    void getFromPrototypes(JSContext *cx, Property *base);

    void print(JSContext *cx);
    void trace(JSTracer *trc);
};






struct TypeFunction : public TypeObject
{
    
    JSTypeHandler handler;

    
    JSScript *script;

    




    bool isGeneric;

    inline TypeFunction(jsid id, JSObject *proto);
};






struct TypeCallsite
{
    JSScript *script;
    const jsbytecode *pc;

    
    bool isNew;

    
    TypeSet **argumentTypes;
    unsigned argumentCount;

    
    TypeSet *thisTypes;

    
    jstype thisType;

    
    TypeSet *returnTypes;

    inline TypeCallsite(JSContext *cx, JSScript *script, const jsbytecode *pc,
                        bool isNew, unsigned argumentCount);

    
    inline bool forceThisTypes(JSContext *cx);

    
    inline TypeObject* getInitObject(JSContext *cx, bool isArray);

    inline bool compileAndGo();
};






struct TypeResult
{
    



    uint32 offset;

    
    jstype type;

    
    TypeResult *next;
};


struct TypeScript
{
#ifdef DEBUG
    JSScript *script;
#endif

    



    TypeSet **pushedArray;

    
    void print(JSContext *cx, JSScript *script);

    inline bool monitored(uint32 offset);
    inline void setMonitored(uint32 offset);

    inline TypeSet *pushed(uint32 offset);
    inline TypeSet *pushed(uint32 offset, uint32 index);

    inline void addType(JSContext *cx, uint32 offset, uint32 index, jstype type);
};


void AnalyzeScriptTypes(JSContext *cx, JSScript *script);


void AnalyzeScriptNew(JSContext *cx, JSScript *script);

struct ArrayTableKey;
typedef HashMap<ArrayTableKey,TypeObject*,ArrayTableKey,SystemAllocPolicy> ArrayTypeTable;

struct ObjectTableKey;
struct ObjectTableEntry;
typedef HashMap<ObjectTableKey,ObjectTableEntry,ObjectTableKey,SystemAllocPolicy> ObjectTypeTable;


struct TypeCompartment
{
    
    TypeObject *objects;

    
    bool inferenceEnabled;

    
    unsigned inferenceDepth;
    uint64_t inferenceStartTime;

    
    JSArenaPool pool;

    
    unsigned scriptCount;

    
    TypeObject typeEmpty;

    



    bool pendingNukeTypes;

    




    bool typesNuked;

    
    Vector<JSScript*> *pendingRecompiles;

    

    ArrayTypeTable *arrayTypeTable;
    ObjectTypeTable *objectTypeTable;

    bool fixArrayType(JSContext *cx, JSObject *obj);
    bool fixObjectType(JSContext *cx, JSObject *obj);

    

    
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

    

    



    uint64_t analysisTime;

    
    static const unsigned TYPE_COUNT_LIMIT = 4;
    unsigned typeCounts[TYPE_COUNT_LIMIT];
    unsigned typeCountOver;

    
    unsigned recompilations;

    void init(JSContext *cx);
    ~TypeCompartment();

    uint64 currentTime()
    {
#if 0
        timeval current;
        gettimeofday(&current, NULL);
        return current.tv_sec * (uint64_t) 1000000 + current.tv_usec;
#else
        
        return 0;
#endif
    }

    
    inline void addPending(JSContext *cx, TypeConstraint *constraint, TypeSet *source, jstype type);
    void growPendingArray(JSContext *cx);

    
    inline void resolvePending(JSContext *cx);

    
    void print(JSContext *cx, JSCompartment *compartment);

    
    TypeObject *newTypeObject(JSContext *cx, JSScript *script,
                              const char *name, bool isFunction, JSObject *proto);

    
    TypeObject *newInitializerTypeObject(JSContext *cx, JSScript *script,
                                         uint32 offset, bool isArray);

    



    bool dynamicPush(JSContext *cx, JSScript *script, uint32 offset, jstype type);
    bool dynamicAssign(JSContext *cx, JSObject *obj, jsid id, const Value &rval);
    bool dynamicCall(JSContext *cx, JSObject *callee, const CallArgs &args, bool constructing);

    inline bool checkPendingRecompiles(JSContext *cx);

    bool nukeTypes(JSContext *cx);
    bool processPendingRecompiles(JSContext *cx);

    
    void setPendingNukeTypes(JSContext *cx);

    
    void addPendingRecompile(JSContext *cx, JSScript *script);

    
    void monitorBytecode(JSContext *cx, JSScript *script, uint32 offset);

    void condense(JSContext *cx);
    void sweep(JSContext *cx);
};

enum SpewChannel {
    ISpewOps,      
    ISpewResult,   
    SPEW_COUNT
};

#ifdef DEBUG

void InferSpew(SpewChannel which, const char *fmt, ...);
const char * TypeString(jstype type);


bool TypeHasProperty(JSContext *cx, TypeObject *obj, jsid id, const Value &value);

#else

inline void InferSpew(SpewChannel which, const char *fmt, ...) {}
inline const char * TypeString(jstype type) { return NULL; }

#endif


void TypeFailure(JSContext *cx, const char *fmt, ...);

} 
} 

static JS_ALWAYS_INLINE js::types::TypeObject *
Valueify(JSTypeObject *jstype) { return (js::types::TypeObject*) jstype; }

static JS_ALWAYS_INLINE js::types::TypeFunction *
Valueify(JSTypeFunction *jstype) { return (js::types::TypeFunction*) jstype; }

static JS_ALWAYS_INLINE js::types::TypeCallsite *
Valueify(JSTypeCallsite *jssite) { return (js::types::TypeCallsite*) jssite; }

#endif 
