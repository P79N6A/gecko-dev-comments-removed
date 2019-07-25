








































#ifndef jsinfer_h___
#define jsinfer_h___

#include "jsalloc.h"
#include "jscell.h"
#include "jsfriendapi.h"
#include "jsprvtd.h"

#include "ds/LifoAlloc.h"
#include "gc/Barrier.h"
#include "js/HashTable.h"

namespace js {
namespace types {


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
        return (jsuword) type == data;
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

    



    OBJECT_FLAG_NON_DENSE_ARRAY       = 0x00010000,

    
    OBJECT_FLAG_NON_PACKED_ARRAY      = 0x00020000,

    
    OBJECT_FLAG_NON_TYPED_ARRAY       = 0x00040000,

    
    OBJECT_FLAG_CREATED_ARGUMENTS     = 0x00080000,

    
    OBJECT_FLAG_UNINLINEABLE          = 0x00100000,

    
    OBJECT_FLAG_SPECIAL_EQUALITY      = 0x00200000,

    
    OBJECT_FLAG_ITERATED              = 0x00400000,

    
    OBJECT_FLAG_REENTRANT_FUNCTION    = 0x00800000,

    
    OBJECT_FLAG_REGEXP_FLAGS_SET      = 0x01000000,

    
    OBJECT_FLAG_DYNAMIC_MASK          = 0x01ff0000,

    



    OBJECT_FLAG_UNKNOWN_PROPERTIES    = 0x80000000,

    
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

    TypeFlags baseFlags() const { return flags & TYPE_FLAG_BASE_MASK; }
    bool unknown() const { return !!(flags & TYPE_FLAG_UNKNOWN); }
    bool unknownObject() const { return !!(flags & (TYPE_FLAG_UNKNOWN | TYPE_FLAG_ANYOBJECT)); }

    bool empty() const { return !baseFlags() && !baseObjectCount(); }

    bool hasAnyFlag(TypeFlags flags) const {
        JS_ASSERT((flags & TYPE_FLAG_BASE_MASK) == flags);
        return !!(baseFlags() & flags);
    }

    bool isOwnProperty(bool configurable) const {
        return flags & (configurable ? TYPE_FLAG_CONFIGURED_PROPERTY : TYPE_FLAG_OWN_PROPERTY);
    }
    bool isDefiniteProperty() const { return flags & TYPE_FLAG_DEFINITE_PROPERTY; }
    unsigned definiteSlot() const {
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

    enum FilterKind {
        FILTER_ALL_PRIMITIVES,
        FILTER_NULL_VOID,
        FILTER_VOID
    };

    
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
    void addPropagateThis(JSContext *cx, JSScript *script, jsbytecode *pc,
                          Type type, TypeSet *types = NULL);
    void addFilterPrimitives(JSContext *cx, TypeSet *target, FilterKind filter);
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

    
    bool knownSubset(JSContext *cx, TypeSet *other);

    



    int getTypedArrayType(JSContext *cx);

    
    JSObject *getSingleton(JSContext *cx, bool freeze = true);

    
    bool hasGlobalObject(JSContext *cx, JSObject *global);

    inline void clearObjects();

    



    bool needsBarrier(JSContext *cx);

    
    bool propertyNeedsBarrier(JSContext *cx, jsid id);

  private:
    uint32 baseObjectCount() const {
        return (flags & TYPE_FLAG_OBJECT_COUNT_MASK) >> TYPE_FLAG_OBJECT_COUNT_SHIFT;
    }
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

    



    JSObject *singleton;
    jsid singletonId;

    TypeBarrier(TypeSet *target, Type type, JSObject *singleton, jsid singletonId)
        : next(NULL), target(target), type(type),
          singleton(singleton), singletonId(singletonId)
    {}
};


struct Property
{
    
    HeapId id;

    
    TypeSet types;

    inline Property(jsid id);
    inline Property(const Property &o);

    static uint32 keyBits(jsid id) { return (uint32) JSID_BITS(id); }
    static jsid getKey(Property *p) { return p->id; }
};











struct TypeNewScript
{
    HeapPtrFunction fun;

    
    gc::AllocKind allocKind;

    



    HeapPtr<const Shape> shape;

    







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

    static inline void writeBarrierPre(TypeNewScript *newScript);
    static inline void writeBarrierPost(TypeNewScript *newScript, void *addr);
};




























struct TypeObject : gc::Cell
{
    
    HeapPtrObject proto;

    




    HeapPtrObject singleton;

    



    static const size_t LAZY_SINGLETON = 1;
    bool lazy() const { return singleton == (JSObject *) LAZY_SINGLETON; }

    
    TypeObjectFlags flags;

    










    uint32 contribution;
    static const uint32 CONTRIBUTION_LIMIT = 2000;

    




    HeapPtr<TypeNewScript> newScript;

    





























    Property **propertySet;

    
    HeapPtrFunction interpretedFunction;

#if JS_BITS_PER_WORD == 32
    void *padding;
#endif

    inline TypeObject(JSObject *proto, bool isFunction, bool unknown);

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

    





    inline TypeSet *getProperty(JSContext *cx, jsid id, bool assign);

    
    inline TypeSet *maybeGetProperty(JSContext *cx, jsid id);

    inline unsigned getPropertyCount();
    inline Property *getProperty(unsigned i);

    
    inline void setFlagsFromKey(JSContext *cx, JSProtoKey kind);

    



    inline JSObject *getGlobal();

    

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

    inline void clearProperties();
    inline void sweep(JSContext *cx);

    inline size_t dynamicSize();

    




    void finalize(JSContext *cx, bool background) {}

    static inline void writeBarrierPre(TypeObject *type);
    static inline void writeBarrierPost(TypeObject *type, void *addr);

  private:
    inline uint32 basePropertyCount() const;
    inline void setBasePropertyCount(uint32 count);

    static void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(TypeObject, proto) == offsetof(js::shadow::TypeObject, proto));
    }
};





struct TypeObjectEntry
{
    typedef JSObject *Lookup;

    static inline HashNumber hash(JSObject *base);
    static inline bool match(TypeObject *key, JSObject *lookup);
};
typedef HashSet<TypeObject *, TypeObjectEntry, SystemAllocPolicy> TypeObjectSet;





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






































struct TypeScriptNesting
{
    







    JSScript *parent;

    
    JSScript *children;

    
    JSScript *next;

    
    JSObject *activeCall;

    







    const Value *argArray;
    const Value *varArray;

    
    uint32 activeFrames;

    TypeScriptNesting() { PodZero(this); }
    ~TypeScriptNesting();
};


bool CheckScriptNesting(JSContext *cx, JSScript *script);


void NestingPrologue(JSContext *cx, StackFrame *fp);
void NestingEpilogue(StackFrame *fp);


class TypeScript
{
    friend struct ::JSScript;

    
    analyze::ScriptAnalysis *analysis;

    




    static const size_t GLOBAL_MISSING_SCOPE = 0x1;

    
    HeapPtr<GlobalObject> global;

  public:

    
    TypeScriptNesting *nesting;

    
    TypeResult *dynamicList;

    TypeScript() {
        this->global = (js::GlobalObject *) GLOBAL_MISSING_SCOPE;
    }

    bool hasScope() { return size_t(global.get()) != GLOBAL_MISSING_SCOPE; }

    
    TypeSet *typeArray() { return (TypeSet *) (jsuword(this) + sizeof(TypeScript)); }

    static inline unsigned NumTypeSets(JSScript *script);

    static bool SetScope(JSContext *cx, JSScript *script, JSObject *scope);

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
    inline void trace(JSTracer *trc);
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
    bool growPendingArray(JSContext *cx);

    
    inline void resolvePending(JSContext *cx);

    
    void print(JSContext *cx, bool force);

    





    TypeObject *newTypeObject(JSContext *cx, JSScript *script,
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
const char * TypeObjectString(TypeObject *type);


bool TypeHasProperty(JSContext *cx, TypeObject *obj, jsid id, const Value &value);

#else

inline const char * InferSpewColorReset() { return NULL; }
inline const char * InferSpewColor(TypeConstraint *constraint) { return NULL; }
inline const char * InferSpewColor(TypeSet *types) { return NULL; }
inline void InferSpew(SpewChannel which, const char *fmt, ...) {}
inline const char * TypeString(Type type) { return NULL; }
inline const char * TypeObjectString(TypeObject *type) { return NULL; }

#endif


void TypeFailure(JSContext *cx, const char *fmt, ...);

} 
} 

namespace JS {
    template<> class AnchorPermitted<js::types::TypeObject *> { };
}

#endif 
