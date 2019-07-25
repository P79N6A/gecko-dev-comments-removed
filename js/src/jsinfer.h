








































#ifndef jsinfer_h___
#define jsinfer_h___

#include "jsalloc.h"
#include "jsarena.h"
#include "jscell.h"
#include "jstl.h"
#include "jsprvtd.h"
#include "jsvalue.h"
#include "jshashtable.h"

namespace js {
    class CallArgs;
    namespace analyze {
        class ScriptAnalysis;
    }
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

    
    TYPE_FLAG_OBJECT_COUNT_MASK   = 0xff00,
    TYPE_FLAG_OBJECT_COUNT_SHIFT  = 8,
    TYPE_FLAG_OBJECT_COUNT_LIMIT  =
        TYPE_FLAG_OBJECT_COUNT_MASK >> TYPE_FLAG_OBJECT_COUNT_SHIFT,

    
    TYPE_FLAG_UNKNOWN             = 0x00010000,

    
    TYPE_FLAG_BASE_MASK           = 0x000100ff,

    

    



    TYPE_FLAG_PROPAGATED_PROPERTY = 0x00020000,

    
    TYPE_FLAG_OWN_PROPERTY        = 0x00040000,

    




    TYPE_FLAG_CONFIGURED_PROPERTY = 0x00080000,

    




    TYPE_FLAG_DEFINITE_PROPERTY   = 0x00100000,

    
    TYPE_FLAG_DEFINITE_MASK       = 0x0f000000,
    TYPE_FLAG_DEFINITE_SHIFT      = 24
};
typedef uint32 TypeFlags;


enum {
    
    OBJECT_FLAG_FUNCTION              = 0x1,

    
    OBJECT_FLAG_NEW_SCRIPT_CLEARED    = 0x2,

    




    OBJECT_FLAG_NEW_SCRIPT_REGENERATE = 0x4,

    



    OBJECT_FLAG_SETS_MARKED_UNKNOWN   = 0x8,

    
    OBJECT_FLAG_PROPERTY_COUNT_MASK   = 0xfff0,
    OBJECT_FLAG_PROPERTY_COUNT_SHIFT  = 4,
    OBJECT_FLAG_PROPERTY_COUNT_LIMIT  =
        OBJECT_FLAG_PROPERTY_COUNT_MASK >> OBJECT_FLAG_PROPERTY_COUNT_SHIFT,

    



    OBJECT_FLAG_NON_DENSE_ARRAY       = 0x010000,

    
    OBJECT_FLAG_NON_PACKED_ARRAY      = 0x020000,

    
    OBJECT_FLAG_NON_TYPED_ARRAY       = 0x040000,

    
    OBJECT_FLAG_CREATED_ARGUMENTS     = 0x080000,

    
    OBJECT_FLAG_UNINLINEABLE          = 0x100000,

    
    OBJECT_FLAG_SPECIAL_EQUALITY      = 0x200000,

    
    OBJECT_FLAG_ITERATED              = 0x400000,

    
    OBJECT_FLAG_DYNAMIC_MASK          = 0x7f0000,

    



    OBJECT_FLAG_UNKNOWN_PROPERTIES    = 0x800000,

    
    OBJECT_FLAG_UNKNOWN_MASK =
        OBJECT_FLAG_DYNAMIC_MASK
      | OBJECT_FLAG_UNKNOWN_PROPERTIES
      | OBJECT_FLAG_SETS_MARKED_UNKNOWN
};
typedef uint32 TypeObjectFlags;


class TypeSet
{
    
    TypeFlags flags;

    
    TypeObjectKey **objectSet;

  public:

    
    TypeConstraint *constraintList;

    TypeSet()
        : flags(0), objectSet(NULL), constraintList(NULL)
    {}

    void print(JSContext *cx);

    inline void sweep(JSContext *cx, JSCompartment *compartment);
    inline size_t dynamicSize();

    
    inline bool hasType(Type type);

    TypeFlags baseFlags() { return flags & TYPE_FLAG_BASE_MASK; }
    bool unknown() { return !!(flags & TYPE_FLAG_UNKNOWN); }
    bool unknownObject() { return !!(flags & (TYPE_FLAG_UNKNOWN | TYPE_FLAG_ANYOBJECT)); }

    bool hasAnyFlag(TypeFlags flags) {
        JS_ASSERT((flags & TYPE_FLAG_BASE_MASK) == flags);
        return !!(baseFlags() & flags);
    }

    bool isOwnProperty(bool configurable) {
        return flags & (configurable ? TYPE_FLAG_CONFIGURED_PROPERTY : TYPE_FLAG_OWN_PROPERTY);
    }
    bool isDefiniteProperty() { return flags & TYPE_FLAG_DEFINITE_PROPERTY; }
    unsigned definiteSlot() {
        JS_ASSERT(isDefiniteProperty());
        return flags >> TYPE_FLAG_DEFINITE_SHIFT;
    }

    



    inline void addType(JSContext *cx, Type type);

    
    inline void setOwnProperty(JSContext *cx, bool configured);

    




    inline unsigned getObjectCount();
    inline TypeObjectKey *getObject(unsigned i);
    inline JSObject *getSingleObject(unsigned i);
    inline TypeObject *getTypeObject(unsigned i);

    void setOwnProperty(bool configurable) {
        flags |= TYPE_FLAG_OWN_PROPERTY;
        if (configurable)
            flags |= TYPE_FLAG_CONFIGURED_PROPERTY;
    }
    void setDefinite(unsigned slot) {
        JS_ASSERT(slot <= (TYPE_FLAG_DEFINITE_MASK >> TYPE_FLAG_DEFINITE_SHIFT));
        flags |= TYPE_FLAG_DEFINITE_PROPERTY | (slot << TYPE_FLAG_DEFINITE_SHIFT);
    }

    bool hasPropagatedProperty() { return !!(flags & TYPE_FLAG_PROPAGATED_PROPERTY); }
    void setPropagatedProperty() { flags |= TYPE_FLAG_PROPAGATED_PROPERTY; }

    
    inline void add(JSContext *cx, TypeConstraint *constraint, bool callExisting = true);
    void addSubset(JSContext *cx, TypeSet *target);
    void addGetProperty(JSContext *cx, JSScript *script, jsbytecode *pc,
                        TypeSet *target, jsid id);
    void addSetProperty(JSContext *cx, JSScript *script, jsbytecode *pc,
                        TypeSet *target, jsid id);
    void addCallProperty(JSContext *cx, JSScript *script, jsbytecode *pc, jsid id);
    void addSetElement(JSContext *cx, JSScript *script, jsbytecode *pc,
                       TypeSet *objectTypes, TypeSet *valueTypes);
    void addCall(JSContext *cx, TypeCallsite *site);
    void addArith(JSContext *cx, TypeSet *target, TypeSet *other = NULL);
    void addTransformThis(JSContext *cx, JSScript *script, TypeSet *target);
    void addPropagateThis(JSContext *cx, JSScript *script, jsbytecode *pc, Type type);
    void addFilterPrimitives(JSContext *cx, TypeSet *target, bool onlyNullVoid);
    void addSubsetBarrier(JSContext *cx, JSScript *script, jsbytecode *pc, TypeSet *target);
    void addLazyArguments(JSContext *cx, TypeSet *target);

    



    static TypeSet *make(JSContext *cx, const char *name);

    






    
    void addFreeze(JSContext *cx);

    
    JSValueType getKnownTypeTag(JSContext *cx);

    bool isLazyArguments(JSContext *cx) { return getKnownTypeTag(cx) == JSVAL_TYPE_MAGIC; }

    
    bool hasObjectFlags(JSContext *cx, TypeObjectFlags flags);
    static bool HasObjectFlags(JSContext *cx, TypeObject *object, TypeObjectFlags flags);

    




    static void WatchObjectStateChange(JSContext *cx, TypeObject *object);

    






    bool isOwnProperty(JSContext *cx, TypeObject *object, bool configurable);

    
    bool knownNonEmpty(JSContext *cx);

    



    int getTypedArrayType(JSContext *cx);

    
    JSObject *getSingleton(JSContext *cx, bool freeze = true);

    inline void clearObjects();

  private:
    inline uint32 baseObjectCount() const;
    inline void setBaseObjectCount(uint32 count);
};







struct TypeResult
{
    uint32 offset;
    Type type;
    TypeResult *next;

    TypeResult(uint32 offset, Type type)
        : offset(offset), type(type), next(NULL)
    {}
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

    Property(const Property &o)
        : id(o.id), types(o.types)
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




























struct TypeObject : gc::Cell
{
#ifdef DEBUG
    
    jsid name_;
#if JS_BITS_PER_WORD == 32
    void *padding;
#endif
#endif

    
    JSObject *proto;

    




    JSObject *singleton;

    
    js::EmptyShape **emptyShapes;

    
    TypeObjectFlags flags;

    




    TypeNewScript *newScript;

    










    uint32 contribution;
    static const uint32 CONTRIBUTION_LIMIT = 2000;

    
























    Property **propertySet;

    
    JSScript *functionScript;

    
    inline TypeObject(jsid id, JSObject *proto, bool isFunction, bool unknown);

    bool isFunction() { return !!(flags & OBJECT_FLAG_FUNCTION); }

    bool hasAnyFlags(TypeObjectFlags flags) {
        JS_ASSERT((flags & OBJECT_FLAG_DYNAMIC_MASK) == flags);
        return !!(this->flags & flags);
    }
    bool hasAllFlags(TypeObjectFlags flags) {
        JS_ASSERT((flags & OBJECT_FLAG_DYNAMIC_MASK) == flags);
        return (this->flags & flags) == flags;
    }

    bool unknownProperties() {
        JS_ASSERT_IF(flags & OBJECT_FLAG_UNKNOWN_PROPERTIES,
                     hasAllFlags(OBJECT_FLAG_DYNAMIC_MASK));
        return !!(flags & OBJECT_FLAG_UNKNOWN_PROPERTIES);
    }

    






    inline bool canProvideEmptyShape(js::Class *clasp);
    inline js::EmptyShape *getEmptyShape(JSContext *cx, js::Class *aclasp,
                                          unsigned kind);

    





    inline TypeSet *getProperty(JSContext *cx, jsid id, bool assign);

    
    inline TypeSet *maybeGetProperty(JSContext *cx, jsid id);

    inline const char * name();

    inline unsigned getPropertyCount();
    inline Property *getProperty(unsigned i);

    
    inline void setFlagsFromKey(JSContext *cx, JSProtoKey kind);

    

    bool addProperty(JSContext *cx, jsid id, Property **pprop);
    bool addDefiniteProperties(JSContext *cx, JSObject *obj);
    bool matchDefiniteProperties(JSObject *obj);
    void addPrototype(JSContext *cx, TypeObject *proto);
    void addPropertyType(JSContext *cx, jsid id, Type type);
    void addPropertyType(JSContext *cx, jsid id, const Value &value);
    void addPropertyType(JSContext *cx, const char *name, Type type);
    void addPropertyType(JSContext *cx, const char *name, const Value &value);
    void markPropertyConfigured(JSContext *cx, jsid id);
    void markStateChange(JSContext *cx);
    void setFlags(JSContext *cx, TypeObjectFlags flags);
    void markUnknown(JSContext *cx);
    void clearNewScript(JSContext *cx);
    void getFromPrototypes(JSContext *cx, jsid id, TypeSet *types, bool force = false);

    void print(JSContext *cx);
    void trace(JSTracer *trc, bool weak = false);

    inline void clearProperties();
    inline void sweep(JSContext *cx);

    inline size_t dynamicSize();

    




    void finalize(JSContext *cx) {}

  private:
    inline uint32 basePropertyCount() const;
    inline void setBasePropertyCount(uint32 count);
};


extern TypeObject emptyTypeObject;





extern void
MarkArgumentsCreated(JSContext *cx, JSScript *script);


bool
UseNewType(JSContext *cx, JSScript *script, jsbytecode *pc);






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
    
    analyze::ScriptAnalysis *analysis;

    
    TypeSet *typeArray() { return (TypeSet *) (jsuword(this) + sizeof(TypeScript)); }

    static inline unsigned NumTypeSets(JSScript *script);

    
    TypeResult *dynamicList;

    static inline TypeSet *ReturnTypes(JSScript *script);
    static inline TypeSet *ThisTypes(JSScript *script);
    static inline TypeSet *ArgTypes(JSScript *script, unsigned i);
    static inline TypeSet *LocalTypes(JSScript *script, unsigned i);

    
    static inline TypeSet *SlotTypes(JSScript *script, unsigned slot);

#ifdef DEBUG
    
    static void CheckBytecode(JSContext *cx, JSScript *script, jsbytecode *pc, const js::Value *sp);
#endif

    
    static inline TypeObject *StandardType(JSContext *cx, JSScript *script, JSProtoKey kind);

    
    static inline TypeObject *InitObject(JSContext *cx, JSScript *script, const jsbytecode *pc, JSProtoKey kind);

    



    static inline void MonitorOverflow(JSContext *cx, JSScript *script, jsbytecode *pc);
    static inline void MonitorString(JSContext *cx, JSScript *script, jsbytecode *pc);
    static inline void MonitorUnknown(JSContext *cx, JSScript *script, jsbytecode *pc);

    






    static inline void Monitor(JSContext *cx, JSScript *script, jsbytecode *pc,
                               const js::Value &val);

    
    static inline void MonitorAssign(JSContext *cx, JSScript *script, jsbytecode *pc,
                                     JSObject *obj, jsid id, const js::Value &val);

    
    static inline void SetThis(JSContext *cx, JSScript *script, Type type);
    static inline void SetThis(JSContext *cx, JSScript *script, const js::Value &value);
    static inline void SetLocal(JSContext *cx, JSScript *script, unsigned local, Type type);
    static inline void SetLocal(JSContext *cx, JSScript *script, unsigned local, const js::Value &value);
    static inline void SetArgument(JSContext *cx, JSScript *script, unsigned arg, Type type);
    static inline void SetArgument(JSContext *cx, JSScript *script, unsigned arg, const js::Value &value);

    static void Sweep(JSContext *cx, JSScript *script);
    void destroy();
};

struct ArrayTableKey;
typedef HashMap<ArrayTableKey,TypeObject*,ArrayTableKey,SystemAllocPolicy> ArrayTypeTable;

struct ObjectTableKey;
struct ObjectTableEntry;
typedef HashMap<ObjectTableKey,ObjectTableEntry,ObjectTableKey,SystemAllocPolicy> ObjectTypeTable;

struct AllocationSiteKey;
typedef HashMap<AllocationSiteKey,TypeObject*,AllocationSiteKey,SystemAllocPolicy> AllocationSiteTable;


struct TypeCompartment
{
    
    bool inferenceEnabled;

    
    unsigned scriptCount;

    



    bool pendingNukeTypes;

    
    Vector<JSScript*> *pendingRecompiles;

    




    unsigned recompilations;
    unsigned frameExpansions;

    




    JSScript *compiledScript;

    
    AllocationSiteTable *allocationSiteTable;

    

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

    inline JSCompartment *compartment();

    
    inline void addPending(JSContext *cx, TypeConstraint *constraint, TypeSet *source, Type type);
    void growPendingArray(JSContext *cx);

    
    inline void resolvePending(JSContext *cx);

    
    void print(JSContext *cx);

    





    TypeObject *newTypeObject(JSContext *cx, JSScript *script,
                              const char *base, const char *postfix,
                              JSProtoKey kind, JSObject *proto, bool unknown = false);

    
    TypeObject *newAllocationSiteTypeObject(JSContext *cx, const AllocationSiteKey &key);

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
