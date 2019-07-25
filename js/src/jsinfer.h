








































#ifndef jsinfer_h___
#define jsinfer_h___

#include "jsarena.h"
#include "jstl.h"
#include "jsprvtd.h"
#include "jsvalue.h"

namespace js {
    class CallArgs;
}

namespace js {
namespace types {


class TypeSet;
struct TypeCallsite;
struct TypeObject;
struct TypeFunction;
struct TypeCompartment;
struct ClonedTypeSet;






typedef jsuword jstype;


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

    



    virtual void newPropertyState(JSContext *cx, TypeSet *source) {}

    



    virtual void newObjectState(JSContext *cx, TypeObject *object) {}

    




    virtual void slotsReallocation(JSContext *cx) {}

    





    virtual bool condensed() { return false; }

    





    virtual TypeObject * persistentObject() { return NULL; }
};


enum {
    TYPE_FLAG_UNDEFINED = 1 << TYPE_UNDEFINED,
    TYPE_FLAG_NULL      = 1 << TYPE_NULL,
    TYPE_FLAG_BOOLEAN   = 1 << TYPE_BOOLEAN,
    TYPE_FLAG_INT32     = 1 << TYPE_INT32,
    TYPE_FLAG_DOUBLE    = 1 << TYPE_DOUBLE,
    TYPE_FLAG_STRING    = 1 << TYPE_STRING,

    TYPE_FLAG_UNKNOWN   = 1 << TYPE_UNKNOWN,

    
    TYPE_FLAG_INTERMEDIATE_SET    = 0x0100,

    

    
    TYPE_FLAG_OWN_PROPERTY        = 0x0200,

    




    TYPE_FLAG_CONFIGURED_PROPERTY = 0x0400,

    




    TYPE_FLAG_DEFINITE_PROPERTY   = 0x0800,

    
    TYPE_FLAG_DEFINITE_MASK       = 0xf000,
    TYPE_FLAG_DEFINITE_SHIFT      = 12,

    
    TYPE_FLAG_BASE_MASK           = 0xffffff00
};
typedef uint32 TypeFlags;


enum {
    






    OBJECT_FLAG_UNKNOWN_MASK = uint32(-1),

    



    OBJECT_FLAG_NON_DENSE_ARRAY = 1 << 0,

    
    OBJECT_FLAG_NON_PACKED_ARRAY = 1 << 1,

    
    OBJECT_FLAG_UNINLINEABLE = 1 << 2,

    
    OBJECT_FLAG_SPECIAL_EQUALITY = 1 << 3,

    
    OBJECT_FLAG_ITERATED = 1 << 4
};
typedef uint32 TypeObjectFlags;


class TypeSet
{
    
    TypeFlags typeFlags;

    
    TypeObject **objectSet;
    unsigned objectCount;

  public:

    
    TypeConstraint *constraintList;

    TypeSet()
        : typeFlags(0), objectSet(NULL), objectCount(0), constraintList(NULL)
    {}

    void print(JSContext *cx);

    inline void destroy(JSContext *cx);

    
    inline bool hasType(jstype type);

    TypeFlags baseFlags() { return typeFlags & ~TYPE_FLAG_BASE_MASK; }
    bool hasAnyFlag(TypeFlags flags) { return typeFlags & flags; }
    bool unknown() { return typeFlags & TYPE_FLAG_UNKNOWN; }

    bool isDefiniteProperty() { return typeFlags & TYPE_FLAG_DEFINITE_PROPERTY; }
    unsigned definiteSlot() {
        JS_ASSERT(isDefiniteProperty());
        return typeFlags >> TYPE_FLAG_DEFINITE_SHIFT;
    }

    



    inline void addType(JSContext *cx, jstype type);

    
    void addTypeSet(JSContext *cx, ClonedTypeSet *types);

    
    inline void setOwnProperty(JSContext *cx, bool configured);

    




    inline unsigned getObjectCount();
    inline TypeObject *getObject(unsigned i);

    
    inline TypeObject *getSingleObject();

    bool intermediate() { return typeFlags & TYPE_FLAG_INTERMEDIATE_SET; }
    void setIntermediate() { JS_ASSERT(!typeFlags); typeFlags = TYPE_FLAG_INTERMEDIATE_SET; }
    void setOwnProperty(bool configurable) {
        typeFlags |= TYPE_FLAG_OWN_PROPERTY;
        if (configurable)
            typeFlags |= TYPE_FLAG_CONFIGURED_PROPERTY;
    }
    void setDefinite(unsigned slot) {
        JS_ASSERT(slot <= (TYPE_FLAG_DEFINITE_MASK >> TYPE_FLAG_DEFINITE_SHIFT));
        typeFlags |= TYPE_FLAG_DEFINITE_PROPERTY | (slot << TYPE_FLAG_DEFINITE_SHIFT);
    }

    
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
    void addSubsetBarrier(JSContext *cx, JSScript *script, const jsbytecode *pc, TypeSet *target);

    void addBaseSubset(JSContext *cx, TypeObject *object, TypeSet *target);
    bool addCondensed(JSContext *cx, JSScript *script);

    



    static inline TypeSet* make(JSContext *cx, const char *name);

    






    
    void addFreeze(JSContext *cx);

    
    JSValueType getKnownTypeTag(JSContext *cx);

    
    bool hasObjectFlags(JSContext *cx, TypeObjectFlags flags);
    static bool HasObjectFlags(JSContext *cx, TypeObject *object, TypeObjectFlags flags);

    
    static void WatchObjectReallocation(JSContext *cx, JSObject *object);

    






    bool isOwnProperty(JSContext *cx, bool configurable);

    
    bool hasUnknownProperties(JSContext *cx);

    
    bool knownNonEmpty(JSContext *cx);

    
    JSObject *getSingleton(JSContext *cx);

    
    void pushAllTypes(JSContext *cx, JSScript *script, const jsbytecode *pc);

    



    static void Clone(JSContext *cx, TypeSet *source, ClonedTypeSet *target);

    
    typedef HashSet<JSScript *,
                    DefaultHasher<JSScript *>,
                    SystemAllocPolicy> ScriptSet;

    static bool
    CondenseSweepTypeSet(JSContext *cx, JSCompartment *compartment,
                         ScriptSet &condensed, TypeSet *types);

  private:
    inline void markUnknown(JSContext *cx);
};


struct ClonedTypeSet
{
    TypeFlags typeFlags;
    TypeObject **objectSet;
    unsigned objectCount;
};












class TypeIntermediate
{
  public:
    
    TypeIntermediate *next;

    TypeIntermediate() : next(NULL) {}

    
    virtual void replay(JSContext *cx, JSScript *script) = 0;

    
    virtual bool sweep(JSContext *cx, JSCompartment *compartment) = 0;

    
    virtual bool hasDynamicResult(uint32 offset, jstype type) { return false; }
};








struct TypeBarrier
{
    
    TypeBarrier *next;

    
    TypeSet *target;

    



    jstype type;
};


struct Property
{
    
    jsid id;

    
    TypeSet types;

    Property(jsid id)
        : id(id)
    {}

    static uint32 keyBits(jsid id) { return (uint32) JSID_BITS(id); }
    static jsid getKey(Property *p) { return p->id; }
};





struct TypeNewScript
{
    JSScript *script;

    
     unsigned finalizeKind;

    
    const Shape *shape;

    







    struct Initializer {
        enum Kind {
            SETPROP,
            FRAME_PUSH,
            FRAME_POP,
            DONE
        } kind;
        uint32 offset;
        Initializer(Kind kind, uint32 offset)
          : kind(kind), offset(offset)
        {}
    };
    Initializer *initializerList;
};


struct TypeObject
{
#ifdef DEBUG
    
    jsid name_;
#endif

    
    JSObject *proto;

    
    js::EmptyShape **emptyShapes;

    
    TypeObjectFlags flags;

    
    bool isFunction;

    
    bool marked;

    




    TypeNewScript *newScript;

    



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

    
    JSObject *singleton;

    TypeObject() {}

    
    inline TypeObject(jsid id, JSObject *proto);

    
    TypeFunction* asFunction()
    {
        JS_ASSERT(isFunction);
        return (TypeFunction *) this;
    }

    bool unknownProperties() { return flags == OBJECT_FLAG_UNKNOWN_MASK; }
    bool hasAnyFlags(TypeObjectFlags flags) { return (this->flags & flags) != 0; }
    bool hasAllFlags(TypeObjectFlags flags) { return (this->flags & flags) == flags; }

    






    inline bool canProvideEmptyShape(js::Class *clasp);
    inline js::EmptyShape *getEmptyShape(JSContext *cx, js::Class *aclasp,
                                          unsigned kind);

    





    inline TypeSet *getProperty(JSContext *cx, jsid id, bool assign);

    inline const char * name();

    
    void splicePrototype(JSContext *cx, JSObject *proto);

    inline unsigned getPropertyCount();
    inline Property *getProperty(unsigned i);

    

    bool addProperty(JSContext *cx, jsid id, Property **pprop);
    bool addDefiniteProperties(JSContext *cx, JSObject *obj);
    void addPrototype(JSContext *cx, TypeObject *proto);
    void setFlags(JSContext *cx, TypeObjectFlags flags);
    void markUnknown(JSContext *cx);
    void clearNewScript(JSContext *cx);
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

    inline bool hasGlobal();
};

struct ArrayTableKey;
typedef HashMap<ArrayTableKey,TypeObject*,ArrayTableKey,SystemAllocPolicy> ArrayTypeTable;

struct ObjectTableKey;
struct ObjectTableEntry;
typedef HashMap<ObjectTableKey,ObjectTableEntry,ObjectTableKey,SystemAllocPolicy> ObjectTypeTable;


struct TypeCompartment
{
    
    TypeObject *objects;

    
    bool inferenceEnabled;

    
    unsigned scriptCount;

    
    TypeObject typeEmpty;

    



    bool pendingNukeTypes;

    




    bool typesNuked;

    
    Vector<JSScript*> *pendingRecompiles;

    




    unsigned recompilations;
    unsigned frameExpansions;

    




    JSScript *compiledScript;

    

    ArrayTypeTable *arrayTypeTable;
    ObjectTypeTable *objectTypeTable;

    void fixArrayType(JSContext *cx, JSObject *obj);
    void fixObjectType(JSContext *cx, JSObject *obj);

    

    



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

    

    
    static const unsigned TYPE_COUNT_LIMIT = 4;
    unsigned typeCounts[TYPE_COUNT_LIMIT];
    unsigned typeCountOver;

    void init(JSContext *cx);
    ~TypeCompartment();

    
    inline void addPending(JSContext *cx, TypeConstraint *constraint, TypeSet *source, jstype type);
    void growPendingArray(JSContext *cx);

    
    inline void resolvePending(JSContext *cx);

    
    void print(JSContext *cx, JSCompartment *compartment);

    
    TypeObject *newTypeObject(JSContext *cx, JSScript *script,
                              const char *base, const char *postfix,
                              bool isFunction, bool isArray, JSObject *proto);

    
    TypeObject *newInitializerTypeObject(JSContext *cx, JSScript *script,
                                         uint32 offset, bool isArray);

    



    void dynamicPush(JSContext *cx, JSScript *script, uint32 offset, jstype type);
    void dynamicCall(JSContext *cx, JSObject *callee, const CallArgs &args, bool constructing);

    void nukeTypes(JSContext *cx);
    void processPendingRecompiles(JSContext *cx);

    
    void setPendingNukeTypes(JSContext *cx);

    
    void addPendingRecompile(JSContext *cx, JSScript *script);

    
    void monitorBytecode(JSContext *cx, JSScript *script, uint32 offset);

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
