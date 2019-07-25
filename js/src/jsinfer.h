








































#ifndef jsinfer_h___
#define jsinfer_h___

#include "jsarena.h"
#include "jstl.h"
#include "jsprvtd.h"
#include "jsvalue.h"

#ifndef _MSC_VER
#include <sys/time.h>
#endif



namespace js { namespace analyze {
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





class TypeConstraint
{
public:
#ifdef DEBUG
    static unsigned constraintCount;
    unsigned id_;
    const char *kind_;

    unsigned id() const { return id_; }
    const char *kind() const { return kind_; }
#else
    unsigned id() const { return 0; }
    const char *kind() const { return NULL; }
#endif

    
    TypeConstraint *next;

    TypeConstraint(const char *kind) : next(NULL)
    {
#ifdef DEBUG
        this->id_ = ++constraintCount;
        this->kind_ = kind;
#endif
    }

    
    virtual void newType(JSContext *cx, TypeSet *source, jstype type) = 0;

    



    virtual void arrayNotPacked(JSContext *cx, bool notDense) {}
};














enum ObjectKind {
    OBJECT_NONE,
    OBJECT_UNKNOWN,
    OBJECT_PACKED_ARRAY,
    OBJECT_DENSE_ARRAY,
    OBJECT_SCRIPTED_FUNCTION,
    OBJECT_NATIVE_FUNCTION
};


struct TypeSet
{
#ifdef DEBUG
    static unsigned typesetCount;
    unsigned id_;

    
    JSArenaPool *pool;

    unsigned id() const { return id_; }
#else
    unsigned id() const { return 0; }
#endif

    
    TypeFlags typeFlags;

    
    TypeObject **objectSet;
    unsigned objectCount;

    
    TypeConstraint *constraintList;

    TypeSet(JSArenaPool *pool)
        : typeFlags(0), objectSet(NULL), objectCount(0), constraintList(NULL)
    {
        setPool(pool);
    }

    void setPool(JSArenaPool *pool)
    {
#if defined DEBUG && defined JS_TYPE_INFERENCE
        this->id_ = ++typesetCount;
        this->pool = pool;
#endif
    }

    void print(JSContext *cx);

    
    inline bool hasType(jstype type);

    



    inline void addType(JSContext *cx, jstype type);

    
    inline void add(JSContext *cx, TypeConstraint *constraint, bool callExisting = true);
    void addSubset(JSContext *cx, JSArenaPool &pool, TypeSet *target);
    void addGetProperty(JSContext *cx, analyze::Bytecode *code, TypeSet *target, jsid id);
    void addSetProperty(JSContext *cx, analyze::Bytecode *code, TypeSet *target, jsid id);
    void addGetElem(JSContext *cx, analyze::Bytecode *code, TypeSet *object, TypeSet *target);
    void addSetElem(JSContext *cx, analyze::Bytecode *code, TypeSet *object, TypeSet *target);
    void addNewObject(JSContext *cx, JSArenaPool &pool, TypeSet *target);
    void addCall(JSContext *cx, TypeCallsite *site);
    void addArith(JSContext *cx, JSArenaPool &pool, analyze::Bytecode *code,
                  TypeSet *target, TypeSet *other = NULL);
    void addTransformThis(JSContext *cx, JSArenaPool &pool, TypeSet *target);
    void addFilterPrimitives(JSContext *cx, JSArenaPool &pool, TypeSet *target, bool onlyNullVoid);
    void addMonitorRead(JSContext *cx, JSArenaPool &pool, analyze::Bytecode *code, TypeSet *target);

    



    static inline TypeSet* make(JSContext *cx, JSArenaPool &pool, const char *name);

    

    




    JSValueType getKnownTypeTag(JSContext *cx, JSScript *script);

    
    ObjectKind getKnownObjectKind(JSContext *cx, JSScript *script);

    
    bool knownNonEmpty(JSContext *cx, JSScript *script);
};






struct TypeStack
{
    




    TypeStack *mergedGroup;

    
    TypeStack *innerStack;

    
    TypeSet types;

    




    jsid letVariable;
    bool boundWith;

    



    bool ignoreTypeTag;

#ifdef DEBUG
    
    int id;
#endif

    
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

    




    TypeSet types;

    Variable(JSArenaPool *pool, jsid id)
        : id(id), types(pool)
    {}

    static uint32 keyBits(jsid id) { return (uint32) JSID_BITS(id); }
    static jsid getKey(Variable *v) { return v->id; }
};


struct Property
{
    
    jsid id;

    
    TypeSet types;

    
    TypeSet ownTypes;

    Property(JSArenaPool *pool, jsid id)
        : id(id), types(pool), ownTypes(pool)
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

    




    Property **propertySet;
    unsigned propertyCount;

    
    TypeObject *instanceList;

    
    TypeObject *instanceNext;

    



    JSArenaPool *pool;
    TypeObject *next;

    
    bool unknownProperties;

    
    bool isDenseArray;

    
    bool isPackedArray;

    




    bool possiblePackedArray;

    TypeObject() {}

    
    inline TypeObject(JSArenaPool *pool, jsid id, JSObject *proto);

    
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

    
    void splicePrototype(JSObject *proto);

    

    void addPrototype(JSContext *cx, TypeObject *proto);
    void addProperty(JSContext *cx, jsid id, Property *&prop);
    void markUnknown(JSContext *cx);
    void storeToInstances(JSContext *cx, Property *base);

    void print(JSContext *cx);
    void trace(JSTracer *trc);
};


struct TypeFunction : public TypeObject
{
    
    JSTypeHandler handler;

    
    JSScript *script;

    



    TypeSet returnTypes;

    




    bool isGeneric;

    inline TypeFunction(JSArenaPool *pool, jsid id, JSObject *proto);
};


struct TypeCompartment
{
    



    JSArenaPool pool;
    TypeObject *objects;

    
    unsigned scriptCount;

    
    bool interpreting;

    
    TypeObject emptyObject;

    
    TypeObject *typeGetSet;

    
    Vector<JSScript*> *pendingRecompiles;

    

    
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

    void init();
    ~TypeCompartment();

    uint64 currentTime()
    {
#ifndef _MSC_VER
        timeval current;
        gettimeofday(&current, NULL);
        return current.tv_sec * (uint64_t) 1000000 + current.tv_usec;
#else
        
        return 0;
#endif
    }

    
    inline void addPending(JSContext *cx, TypeConstraint *constraint, TypeSet *source, jstype type);
    void growPendingArray();

    
    inline void resolvePending(JSContext *cx);

    
    void finish(JSContext *cx, JSCompartment *compartment);

    
    TypeObject *newTypeObject(JSContext *cx, analyze::Script *script,
                              const char *name, bool isFunction, JSObject *proto);

    



    void addDynamicType(JSContext *cx, TypeSet *types, jstype type);
    void addDynamicPush(JSContext *cx, analyze::Bytecode &code, unsigned index, jstype type);
    void dynamicAssign(JSContext *cx, JSObject *obj, jsid id, const Value &rval);

    inline bool hasPendingRecompiles() { return pendingRecompiles != NULL; }
    void processPendingRecompiles(JSContext *cx);
    void addPendingRecompile(JSContext *cx, JSScript *script);

    
    void monitorBytecode(JSContext *cx, analyze::Bytecode *code);

    void sweep(JSContext *cx);
};

enum SpewChannel {
    ISpewDynamic,  
    ISpewOps,      
    ISpewResult,   
    SPEW_COUNT
};

#ifdef DEBUG

void InferSpew(SpewChannel which, const char *fmt, ...);
const char * TypeString(jstype type);

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
