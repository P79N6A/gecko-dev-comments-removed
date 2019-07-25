








































#ifndef jsinfer_h___
#define jsinfer_h___

#include "jsalloc.h"
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
struct TypeCompartment;


struct TypeObjectKey {
    static intptr_t keyBits(TypeObjectKey *obj) { return (intptr_t) obj; }
    static TypeObjectKey *getKey(TypeObjectKey *obj) { return obj; }
};






class Type
{
    jsuword data;
    Type(jsuword data) : data(data) {}

  public:

    jsuword raw() const { return data; }

    bool isPrimitive() const {
        return data < JSVAL_TYPE_OBJECT;
    }

    bool isPrimitive(JSValueType type) const {
        JS_ASSERT(type < JSVAL_TYPE_OBJECT);
        return type == (JSValueType) data;
    }

    JSValueType primitive() const {
        JS_ASSERT(isPrimitive());
        return (JSValueType) data;
    }

    bool isAnyObject() const {
        return data == JSVAL_TYPE_OBJECT;
    }

    bool isUnknown() const {
        return data == JSVAL_TYPE_UNKNOWN;
    }

    

    bool isObject() const {
        JS_ASSERT(!isAnyObject() && !isUnknown());
        return data > JSVAL_TYPE_UNKNOWN;
    }

    TypeObjectKey *objectKey() const {
        JS_ASSERT(isObject());
        return (TypeObjectKey *) data;
    }

    

    bool isSingleObject() const {
        return isObject() && !!(data & 1);
    }

    JSObject *singleObject() const {
        JS_ASSERT(isSingleObject());
        return (JSObject *) (data ^ 1);
    }

    

    bool isTypeObject() const {
        return isObject() && !(data & 1);
    }

    TypeObject *typeObject() const {
        JS_ASSERT(isTypeObject());
        return (TypeObject *) data;
    }

    bool operator == (Type o) const { return data == o.data; }
    bool operator != (Type o) const { return data != o.data; }

    static inline Type UndefinedType() { return Type(JSVAL_TYPE_UNDEFINED); }
    static inline Type NullType()      { return Type(JSVAL_TYPE_NULL); }
    static inline Type BooleanType()   { return Type(JSVAL_TYPE_BOOLEAN); }
    static inline Type Int32Type()     { return Type(JSVAL_TYPE_INT32); }
    static inline Type DoubleType()    { return Type(JSVAL_TYPE_DOUBLE); }
    static inline Type StringType()    { return Type(JSVAL_TYPE_STRING); }
    static inline Type LazyArgsType()  { return Type(JSVAL_TYPE_MAGIC); }
    static inline Type AnyObjectType() { return Type(JSVAL_TYPE_OBJECT); }
    static inline Type UnknownType()   { return Type(JSVAL_TYPE_UNKNOWN); }

    static inline Type PrimitiveType(JSValueType type) {
        JS_ASSERT(type < JSVAL_TYPE_UNKNOWN);
        return Type(type);
    }

    static inline Type ObjectType(JSObject *obj);
    static inline Type ObjectType(TypeObject *obj);
    static inline Type ObjectType(TypeObjectKey *obj);
};


inline Type GetValueType(JSContext *cx, const Value &val);

































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

    TypeConstraint(const char *kind)
        : next(NULL)
    {
#ifdef DEBUG
        this->kind_ = kind;
#endif
    }

    
    virtual void newType(JSContext *cx, TypeSet *source, Type type) = 0;

    



    virtual void newPropertyState(JSContext *cx, TypeSet *source) {}

    




    virtual void newObjectState(JSContext *cx, TypeObject *object, bool force) {}

    




    virtual TypeObject * persistentObject() { return NULL; }

    virtual size_t allocatedSize() { return 0; }
};


enum {
    TYPE_FLAG_UNDEFINED =  0x1,
    TYPE_FLAG_NULL      =  0x2,
    TYPE_FLAG_BOOLEAN   =  0x4,
    TYPE_FLAG_INT32     =  0x8,
    TYPE_FLAG_DOUBLE    = 0x10,
    TYPE_FLAG_STRING    = 0x20,
    TYPE_FLAG_LAZYARGS  = 0x40,
    TYPE_FLAG_ANYOBJECT = 0x80,

    TYPE_FLAG_UNKNOWN   = 0x100,

    
    TYPE_FLAG_INTERMEDIATE_SET    = 0x0200,

    

    
    TYPE_FLAG_OWN_PROPERTY        = 0x0400,

    




    TYPE_FLAG_CONFIGURED_PROPERTY = 0x0800,

    
    TYPE_FLAG_HAS_PERSISTENT_CONSTRAINTS = 0x1000,

    




    TYPE_FLAG_DEFINITE_PROPERTY   = 0x08000,

    
    TYPE_FLAG_DEFINITE_MASK       = 0xf0000,
    TYPE_FLAG_DEFINITE_SHIFT      = 16,

    
    TYPE_FLAG_BASE_MASK           = 0xffffffff ^ ((TYPE_FLAG_UNKNOWN << 1) - 1)
};
typedef uint32 TypeFlags;


enum {
    



    OBJECT_FLAG_NON_DENSE_ARRAY = 1 << 0,

    
    OBJECT_FLAG_NON_PACKED_ARRAY = 1 << 1,

    
    OBJECT_FLAG_NON_TYPED_ARRAY = 1 << 2,

    
    OBJECT_FLAG_CREATED_ARGUMENTS = 1 << 3,

    
    OBJECT_FLAG_UNINLINEABLE = 1 << 4,

    
    OBJECT_FLAG_SPECIAL_EQUALITY = 1 << 5,

    
    OBJECT_FLAG_ITERATED = 1 << 6,

    




    OBJECT_FLAG_DETERMINED_MASK =
        OBJECT_FLAG_NON_DENSE_ARRAY |
        OBJECT_FLAG_NON_PACKED_ARRAY |
        OBJECT_FLAG_NON_TYPED_ARRAY |
        OBJECT_FLAG_SPECIAL_EQUALITY,

    
    OBJECT_FLAG_UNKNOWN_MASK = uint32(-1)
};
typedef uint32 TypeObjectFlags;


class TypeSet
{
    
    TypeFlags typeFlags;

    
    TypeObjectKey **objectSet;
    unsigned objectCount;

  public:

    
    TypeConstraint *constraintList;

    TypeSet()
        : typeFlags(0), objectSet(NULL), objectCount(0), constraintList(NULL)
    {}

    void print(JSContext *cx);

    inline bool sweep(JSContext *cx);
    inline void destroy();
    size_t dynamicSize();

    
    inline bool hasType(Type type);

    TypeFlags baseFlags() { return typeFlags & ~TYPE_FLAG_BASE_MASK; }
    bool hasAnyFlag(TypeFlags flags) { return !!(typeFlags & flags); }
    bool unknown() { return !!(typeFlags & TYPE_FLAG_UNKNOWN); }
    bool unknownObject() { return !!(typeFlags & (TYPE_FLAG_UNKNOWN | TYPE_FLAG_ANYOBJECT)); }

    bool isDefiniteProperty() { return typeFlags & TYPE_FLAG_DEFINITE_PROPERTY; }
    unsigned definiteSlot() {
        JS_ASSERT(isDefiniteProperty());
        return typeFlags >> TYPE_FLAG_DEFINITE_SHIFT;
    }

    



    inline void addType(JSContext *cx, Type type);

    
    inline void setOwnProperty(JSContext *cx, bool configured);

    




    inline unsigned getObjectCount();
    inline TypeObjectKey *getObject(unsigned i);
    inline JSObject *getSingleObject(unsigned i);
    inline TypeObject *getTypeObject(unsigned i);

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
    void addSubset(JSContext *cx, TypeSet *target);
    void addGetProperty(JSContext *cx, JSScript *script, jsbytecode *pc,
                        TypeSet *target, jsid id);
    void addSetProperty(JSContext *cx, JSScript *script, jsbytecode *pc,
                        TypeSet *target, jsid id);
    void addCallProperty(JSContext *cx, JSScript *script, jsbytecode *pc, jsid id);
    void addNewObject(JSContext *cx, TypeObject *fun, TypeSet *target);
    void addCall(JSContext *cx, TypeCallsite *site);
    void addArith(JSContext *cx, TypeSet *target, TypeSet *other = NULL);
    void addTransformThis(JSContext *cx, JSScript *script, TypeSet *target);
    void addPropagateThis(JSContext *cx, JSScript *script, jsbytecode *pc, Type type);
    void addFilterPrimitives(JSContext *cx, TypeSet *target, bool onlyNullVoid);
    void addSubsetBarrier(JSContext *cx, JSScript *script, jsbytecode *pc, TypeSet *target);
    void addLazyArguments(JSContext *cx, TypeSet *target);

    void addBaseSubset(JSContext *cx, TypeObject *object, TypeSet *target);

    



    static TypeSet *make(JSContext *cx, const char *name);

    






    
    void addFreeze(JSContext *cx);

    
    JSValueType getKnownTypeTag(JSContext *cx);

    bool isLazyArguments(JSContext *cx) { return getKnownTypeTag(cx) == JSVAL_TYPE_MAGIC; }

    
    bool hasObjectFlags(JSContext *cx, TypeObjectFlags flags);
    static bool HasObjectFlags(JSContext *cx, TypeObject *object, TypeObjectFlags flags);

    
    static void WatchObjectReallocation(JSContext *cx, JSObject *object);

    






    bool isOwnProperty(JSContext *cx, bool configurable);

    
    bool knownNonEmpty(JSContext *cx);

    



    int getTypedArrayType(JSContext *cx);

    
    JSObject *getSingleton(JSContext *cx, bool freeze = true);

    static bool
    SweepTypeSet(JSContext *cx, JSCompartment *compartment, TypeSet *types);

  private:
    void clearObjects();
};












class TypeIntermediate
{
  public:
    
    TypeIntermediate *next;

    TypeIntermediate() : next(NULL) {}

    
    virtual void replay(JSContext *cx, JSScript *script) = 0;

    
    virtual bool sweep(JSContext *cx, JSCompartment *compartment) = 0;

    
    virtual bool hasDynamicResult(uint32 offset, Type type) { return false; }

    virtual size_t allocatedSize() = 0;
};




















































struct TypeBarrier
{
    
    TypeBarrier *next;

    
    TypeSet *target;

    



    Type type;

    TypeBarrier(TypeSet *target, Type type)
        : next(NULL), target(target), type(type)
    {}
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

    




    JSObject *singleton;

    
    js::EmptyShape **emptyShapes;

    
    TypeObjectFlags flags;

    
    bool isFunction;

    
    bool marked;

    
    bool newScriptCleared;

    



    bool setsMarkedUnknown;

    




    TypeNewScript *newScript;

    



    JSProtoKey initializerKey;
    uint32 initializerOffset;

    










    uint32 contribution;
    static const uint32 CONTRIBUTION_LIMIT = 2000;

    
























    Property **propertySet;
    unsigned propertyCount;

    
    TypeObject *instanceList;

    
    TypeObject *instanceNext;

    
    TypeObject *next;

    
    JSScript *functionScript;

    TypeObject() {}

    
    inline TypeObject(jsid id, JSObject *proto, bool isFunction);

    bool unknownProperties() { return flags == OBJECT_FLAG_UNKNOWN_MASK; }
    bool hasAnyFlags(TypeObjectFlags flags) { return (this->flags & flags) != 0; }
    bool hasAllFlags(TypeObjectFlags flags) { return (this->flags & flags) == flags; }

    






    inline bool canProvideEmptyShape(js::Class *clasp);
    inline js::EmptyShape *getEmptyShape(JSContext *cx, js::Class *aclasp,
                                          unsigned kind);

    





    inline TypeSet *getProperty(JSContext *cx, jsid id, bool assign);

    inline bool hasProperty(JSContext *cx, jsid id);

    inline const char * name();

    inline unsigned getPropertyCount();
    inline Property *getProperty(unsigned i);

    
    inline void setFlagsFromKey(JSContext *cx, JSProtoKey key);

    

    bool addProperty(JSContext *cx, jsid id, Property **pprop);
    bool addDefiniteProperties(JSContext *cx, JSObject *obj);
    void addPrototype(JSContext *cx, TypeObject *proto);
    void addPropertyType(JSContext *cx, jsid id, Type type);
    void addPropertyType(JSContext *cx, jsid id, const Value &value);
    void addPropertyType(JSContext *cx, const char *name, Type type);
    void addPropertyType(JSContext *cx, const char *name, const Value &value);
    void markPropertyConfigured(JSContext *cx, jsid id);
    void aliasProperties(JSContext *cx, jsid first, jsid second);
    void markSlotReallocation(JSContext *cx);
    void setFlags(JSContext *cx, TypeObjectFlags flags);
    void markUnknown(JSContext *cx);
    void clearNewScript(JSContext *cx);
    void storeToInstances(JSContext *cx, Property *base);
    void getFromPrototypes(JSContext *cx, Property *base);

    void print(JSContext *cx);
    void trace(JSTracer *trc);
};


bool
UseNewType(JSContext *cx, JSScript *script, jsbytecode *pc);





void
CheckNewScriptProperties(JSContext *cx, TypeObject *type, JSScript *script);






struct TypeCallsite
{
    JSScript *script;
    jsbytecode *pc;

    
    bool isNew;

    
    TypeSet **argumentTypes;
    unsigned argumentCount;

    
    TypeSet *thisTypes;

    
    TypeSet *returnTypes;

    inline TypeCallsite(JSContext *cx, JSScript *script, jsbytecode *pc,
                        bool isNew, unsigned argumentCount);
};


struct TypeScript
{
    inline JSScript *script();

    
    TypeSet *typeArray;
    inline unsigned numTypeSets();

    
    TypeObject *typeObjects;

    
    TypeIntermediate *intermediateList;
    void addIntermediate(TypeIntermediate *type) {
        type->next = intermediateList;
        intermediateList = type;
    }

    
    inline bool ensureTypeArray(JSContext *cx);

    inline TypeSet *bytecodeTypes(const jsbytecode *pc);
    inline TypeSet *returnTypes();
    inline TypeSet *thisTypes();
    inline TypeSet *argTypes(unsigned i);
    inline TypeSet *localTypes(unsigned i);
    inline TypeSet *upvarTypes(unsigned i);

    
    inline TypeSet *slotTypes(unsigned slot);

  private:
    bool makeTypeArray(JSContext *cx);
  public:

#ifdef DEBUG
    
    void checkBytecode(JSContext *cx, jsbytecode *pc, const js::Value *sp);
#endif

    
    inline TypeObject *standardType(JSContext *cx, JSProtoKey key);

    
    inline TypeObject *initObject(JSContext *cx, const jsbytecode *pc, JSProtoKey key);

    



    inline void monitorOverflow(JSContext *cx, jsbytecode *pc);
    inline void monitorString(JSContext *cx, jsbytecode *pc);
    inline void monitorUnknown(JSContext *cx, jsbytecode *pc);

    






    inline void monitor(JSContext *cx, jsbytecode *pc, const js::Value &val);

    
    inline void monitorAssign(JSContext *cx, jsbytecode *pc,
                              JSObject *obj, jsid id, const js::Value &val);

    
    inline void setThis(JSContext *cx, Type type);
    inline void setThis(JSContext *cx, const js::Value &value);
    inline void setNewCalled(JSContext *cx);
    inline void setLocal(JSContext *cx, unsigned local, Type type);
    inline void setLocal(JSContext *cx, unsigned local, const js::Value &value);
    inline void setArgument(JSContext *cx, unsigned arg, Type type);
    inline void setArgument(JSContext *cx, unsigned arg, const js::Value &value);
    inline void setUpvar(JSContext *cx, unsigned upvar, const js::Value &value);

    void trace(JSTracer *trc);
    void sweep(JSContext *cx);
    void finalizeObjects();
    void destroy();
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

    



    TypeObject typeLazyArguments;

    



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
        Type type;
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

    
    inline void addPending(JSContext *cx, TypeConstraint *constraint, TypeSet *source, Type type);
    void growPendingArray(JSContext *cx);

    
    inline void resolvePending(JSContext *cx);

    
    void print(JSContext *cx, JSCompartment *compartment);

    





    TypeObject *newTypeObject(JSContext *cx, JSScript *script,
                              const char *base, const char *postfix,
                              JSProtoKey key, JSObject *proto);

    
    TypeObject *newInitializerTypeObject(JSContext *cx, JSScript *script,
                                         uint32 offset, JSProtoKey key);

    void nukeTypes(JSContext *cx);
    void processPendingRecompiles(JSContext *cx);

    
    void setPendingNukeTypes(JSContext *cx);

    
    void addPendingRecompile(JSContext *cx, JSScript *script);

    
    void monitorBytecode(JSContext *cx, JSScript *script, uint32 offset,
                         bool returnOnly = false);

    
    void markSetsUnknown(JSContext *cx, TypeObject *obj);

    void sweep(JSContext *cx);
    void finalizeObjects();
};

enum SpewChannel {
    ISpewOps,      
    ISpewResult,   
    SPEW_COUNT
};

#ifdef DEBUG

const char * InferSpewColorReset();
const char * InferSpewColor(TypeConstraint *constraint);
const char * InferSpewColor(TypeSet *types);

void InferSpew(SpewChannel which, const char *fmt, ...);
const char * TypeString(Type type);


bool TypeHasProperty(JSContext *cx, TypeObject *obj, jsid id, const Value &value);

#else

inline const char * InferSpewColorReset() { return NULL; }
inline const char * InferSpewColor(TypeConstraint *constraint) { return NULL; }
inline const char * InferSpewColor(TypeSet *types) { return NULL; }
inline void InferSpew(SpewChannel which, const char *fmt, ...) {}
inline const char * TypeString(Type type) { return NULL; }

#endif


void TypeFailure(JSContext *cx, const char *fmt, ...);

} 
} 

#endif 
