







#ifndef jsinfer_h___
#define jsinfer_h___

#include "mozilla/Attributes.h"

#include "jsalloc.h"
#include "jsfriendapi.h"
#include "jsprvtd.h"

#include "ds/LifoAlloc.h"
#include "gc/Barrier.h"
#include "gc/Heap.h"
#include "js/HashTable.h"
#include "js/Vector.h"

namespace JS {
struct TypeInferenceSizes;
}

namespace js {

class CallObject;

namespace mjit {
    struct JITScript;
}

namespace types {


struct TypeObjectKey {
    static intptr_t keyBits(TypeObjectKey *obj) { return (intptr_t) obj; }
    static TypeObjectKey *getKey(TypeObjectKey *obj) { return obj; }
};






class Type
{
    uintptr_t data;
    Type(uintptr_t data) : data(data) {}

  public:

    uintptr_t raw() const { return data; }

    bool isPrimitive() const {
        return data < JSVAL_TYPE_OBJECT;
    }

    bool isPrimitive(JSValueType type) const {
        JS_ASSERT(type < JSVAL_TYPE_OBJECT);
        return (uintptr_t) type == data;
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

    inline TypeObjectKey *objectKey() const;

    

    bool isSingleObject() const {
        return isObject() && !!(data & 1);
    }

    inline JSObject *singleObject() const;

    

    bool isTypeObject() const {
        return isObject() && !(data & 1);
    }

    inline TypeObject *typeObject() const;

    bool operator == (Type o) const { return data == o.data; }
    bool operator != (Type o) const { return data != o.data; }

    static inline Type UndefinedType() { return Type(JSVAL_TYPE_UNDEFINED); }
    static inline Type NullType()      { return Type(JSVAL_TYPE_NULL); }
    static inline Type BooleanType()   { return Type(JSVAL_TYPE_BOOLEAN); }
    static inline Type Int32Type()     { return Type(JSVAL_TYPE_INT32); }
    static inline Type DoubleType()    { return Type(JSVAL_TYPE_DOUBLE); }
    static inline Type StringType()    { return Type(JSVAL_TYPE_STRING); }
    static inline Type MagicArgType()  { return Type(JSVAL_TYPE_MAGIC); }
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
    
    TypeConstraint *next;

    TypeConstraint()
        : next(NULL)
    {}

    
    virtual const char *kind() = 0;

    
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

    

    



    TYPE_FLAG_PURGED              = 0x00020000,

    




    TYPE_FLAG_CONSTRAINTS_PURGED  = 0x00040000,

    

    



    TYPE_FLAG_PROPAGATED_PROPERTY = 0x00080000,

    
    TYPE_FLAG_OWN_PROPERTY        = 0x00100000,

    




    TYPE_FLAG_CONFIGURED_PROPERTY = 0x00200000,

    




    TYPE_FLAG_DEFINITE_PROPERTY   = 0x00400000,

    
    TYPE_FLAG_DEFINITE_MASK       = 0x0f000000,
    TYPE_FLAG_DEFINITE_SHIFT      = 24
};
typedef uint32_t TypeFlags;


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

    
    OBJECT_FLAG_NON_DOM               = 0x00080000,

    
    OBJECT_FLAG_UNINLINEABLE          = 0x00100000,

    
    OBJECT_FLAG_SPECIAL_EQUALITY      = 0x00200000,

    
    OBJECT_FLAG_ITERATED              = 0x00400000,

    
    OBJECT_FLAG_REGEXP_FLAGS_SET      = 0x00800000,

    
    OBJECT_FLAG_DYNAMIC_MASK          = 0x00ff0000,

    



    OBJECT_FLAG_UNKNOWN_PROPERTIES    = 0x80000000,

    
    OBJECT_FLAG_UNKNOWN_MASK =
        OBJECT_FLAG_DYNAMIC_MASK
      | OBJECT_FLAG_UNKNOWN_PROPERTIES
      | OBJECT_FLAG_SETS_MARKED_UNKNOWN
};
typedef uint32_t TypeObjectFlags;

class StackTypeSet;
class HeapTypeSet;


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

    inline void sweep(JSCompartment *compartment);
    inline size_t computedSizeOfExcludingThis();

    
    inline bool hasType(Type type);

    TypeFlags baseFlags() const { return flags & TYPE_FLAG_BASE_MASK; }
    bool unknown() const { return !!(flags & TYPE_FLAG_UNKNOWN); }
    bool unknownObject() const { return !!(flags & (TYPE_FLAG_UNKNOWN | TYPE_FLAG_ANYOBJECT)); }

    bool empty() const { return !baseFlags() && !baseObjectCount(); }

    bool hasAnyFlag(TypeFlags flags) const {
        JS_ASSERT((flags & TYPE_FLAG_BASE_MASK) == flags);
        return !!(baseFlags() & flags);
    }

    bool ownProperty(bool configurable) const {
        return flags & (configurable ? TYPE_FLAG_CONFIGURED_PROPERTY : TYPE_FLAG_OWN_PROPERTY);
    }
    bool definiteProperty() const { return flags & TYPE_FLAG_DEFINITE_PROPERTY; }
    unsigned definiteSlot() const {
        JS_ASSERT(definiteProperty());
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

    bool constraintsPurged() { return !!(flags & TYPE_FLAG_CONSTRAINTS_PURGED); }
    void setConstraintsPurged() { flags |= TYPE_FLAG_CONSTRAINTS_PURGED; }

    bool purged() { return !!(flags & TYPE_FLAG_PURGED); }
    void setPurged() { flags |= TYPE_FLAG_PURGED | TYPE_FLAG_CONSTRAINTS_PURGED; }

    inline StackTypeSet *toStackTypeSet();
    inline HeapTypeSet *toHeapTypeSet();

    inline void addTypesToConstraint(JSContext *cx, TypeConstraint *constraint);
    inline void add(JSContext *cx, TypeConstraint *constraint, bool callExisting = true);

  protected:
    uint32_t baseObjectCount() const {
        return (flags & TYPE_FLAG_OBJECT_COUNT_MASK) >> TYPE_FLAG_OBJECT_COUNT_SHIFT;
    }
    inline void setBaseObjectCount(uint32_t count);

    inline void clearObjects();
};








class StackTypeSet : public TypeSet
{
  public:

    



    static StackTypeSet *make(JSContext *cx, const char *name);

    

    void addSubset(JSContext *cx, TypeSet *target);
    void addGetProperty(JSContext *cx, JSScript *script, jsbytecode *pc,
                        StackTypeSet *target, jsid id);
    void addSetProperty(JSContext *cx, JSScript *script, jsbytecode *pc,
                        StackTypeSet *target, jsid id);
    void addSetElement(JSContext *cx, JSScript *script, jsbytecode *pc,
                       StackTypeSet *objectTypes, StackTypeSet *valueTypes);
    void addCall(JSContext *cx, TypeCallsite *site);
    void addArith(JSContext *cx, JSScript *script, jsbytecode *pc,
                  TypeSet *target, TypeSet *other = NULL);
    void addTransformThis(JSContext *cx, JSScript *script, TypeSet *target);
    void addPropagateThis(JSContext *cx, JSScript *script, jsbytecode *pc,
                          Type type, StackTypeSet *types = NULL);
    void addSubsetBarrier(JSContext *cx, JSScript *script, jsbytecode *pc, TypeSet *target);

    








    
    JSValueType getKnownTypeTag();

    bool isMagicArguments() { return getKnownTypeTag() == JSVAL_TYPE_MAGIC; }

    
    bool hasObjectFlags(JSContext *cx, TypeObjectFlags flags);

    



    int getTypedArrayType();

    
    JSObject *getSingleton();

    
    bool propertyNeedsBarrier(JSContext *cx, jsid id);
};







class HeapTypeSet : public TypeSet
{
  public:

    

    void addSubset(JSContext *cx, TypeSet *target);
    void addGetProperty(JSContext *cx, JSScript *script, jsbytecode *pc,
                        StackTypeSet *target, jsid id);
    void addCallProperty(JSContext *cx, JSScript *script, jsbytecode *pc, jsid id);
    void addFilterPrimitives(JSContext *cx, TypeSet *target);
    void addSubsetBarrier(JSContext *cx, JSScript *script, jsbytecode *pc, TypeSet *target);

    

    
    void addFreeze(JSContext *cx);


    




    static void WatchObjectStateChange(JSContext *cx, TypeObject *object);

    
    static bool HasObjectFlags(JSContext *cx, TypeObject *object, TypeObjectFlags flags);

    






    bool isOwnProperty(JSContext *cx, TypeObject *object, bool configurable);

    
    bool knownNonEmpty(JSContext *cx);

    
    bool knownSubset(JSContext *cx, TypeSet *other);

    
    JSObject *getSingleton(JSContext *cx);

    



    bool needsBarrier(JSContext *cx);
};

inline StackTypeSet *
TypeSet::toStackTypeSet()
{
    JS_ASSERT(constraintsPurged());
    return (StackTypeSet *) this;
}

inline HeapTypeSet *
TypeSet::toHeapTypeSet()
{
    JS_ASSERT(!constraintsPurged());
    return (HeapTypeSet *) this;
}







struct TypeResult
{
    uint32_t offset;
    Type type;
    TypeResult *next;

    TypeResult(uint32_t offset, Type type)
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

    
    HeapTypeSet types;

    inline Property(jsid id);
    inline Property(const Property &o);

    static uint32_t keyBits(jsid id) { return uint32_t(JSID_BITS(id)); }
    static jsid getKey(Property *p) { return p->id; }
};











struct TypeNewScript
{
    HeapPtrFunction fun;

    
    gc::AllocKind allocKind;

    



    HeapPtrShape  shape;

    







    struct Initializer {
        enum Kind {
            SETPROP,
            FRAME_PUSH,
            FRAME_POP,
            DONE
        } kind;
        uint32_t offset;
        Initializer(Kind kind, uint32_t offset)
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

    










    uint32_t contribution;
    static const uint32_t CONTRIBUTION_LIMIT = 2000;

    




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

    





    inline HeapTypeSet *getProperty(JSContext *cx, jsid id, bool own);

    
    inline HeapTypeSet *maybeGetProperty(JSContext *cx, jsid id);

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

    inline void clearProperties();
    inline void sweep(FreeOp *fop);

    inline size_t computedSizeOfExcludingThis();

    void sizeOfExcludingThis(TypeInferenceSizes *sizes, JSMallocSizeOfFun mallocSizeOf);

    




    void finalize(FreeOp *fop) {}

    static inline void writeBarrierPre(TypeObject *type);
    static inline void writeBarrierPost(TypeObject *type, void *addr);
    static inline void readBarrier(TypeObject *type);

    static inline ThingRootKind rootKind() { return THING_ROOT_TYPE_OBJECT; }

  private:
    inline uint32_t basePropertyCount() const;
    inline void setBasePropertyCount(uint32_t count);

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
typedef HashSet<ReadBarriered<TypeObject>, TypeObjectEntry, SystemAllocPolicy> TypeObjectSet;


bool
UseNewType(JSContext *cx, JSScript *script, jsbytecode *pc);


bool
UseNewTypeForInitializer(JSContext *cx, JSScript *script, jsbytecode *pc, JSProtoKey key);





bool
ArrayPrototypeHasIndexedProperty(JSContext *cx, JSScript *script);






struct TypeCallsite
{
    JSScript *script;
    jsbytecode *pc;

    
    bool isNew;

    
    unsigned argumentCount;
    StackTypeSet **argumentTypes;

    
    StackTypeSet *thisTypes;

    
    StackTypeSet *returnTypes;

    inline TypeCallsite(JSContext *cx, JSScript *script, jsbytecode *pc,
                        bool isNew, unsigned argumentCount);
};


class TypeScript
{
    friend struct ::JSScript;

    
    analyze::ScriptAnalysis *analysis;

  public:
    
    TypeResult *dynamicList;

    




    HeapTypeSet *propertyReadTypes;

    
    TypeSet *typeArray() { return (TypeSet *) (uintptr_t(this) + sizeof(TypeScript)); }

    static inline unsigned NumTypeSets(JSScript *script);

    static inline HeapTypeSet  *ReturnTypes(JSScript *script);
    static inline StackTypeSet *ThisTypes(JSScript *script);
    static inline StackTypeSet *ArgTypes(JSScript *script, unsigned i);
    static inline StackTypeSet *LocalTypes(JSScript *script, unsigned i);

    
    static inline StackTypeSet *SlotTypes(JSScript *script, unsigned slot);

#ifdef DEBUG
    
    static void CheckBytecode(JSContext *cx, JSScript *script, jsbytecode *pc, const js::Value *sp);
#endif

    
    static inline TypeObject *StandardType(JSContext *cx, JSScript *script, JSProtoKey kind);

    
    static inline TypeObject *InitObject(JSContext *cx, JSScript *script, jsbytecode *pc, JSProtoKey kind);

    



    static inline void MonitorOverflow(JSContext *cx, JSScript *script, jsbytecode *pc);
    static inline void MonitorString(JSContext *cx, JSScript *script, jsbytecode *pc);
    static inline void MonitorUnknown(JSContext *cx, JSScript *script, jsbytecode *pc);

    static inline void GetPcScript(JSContext *cx, JSScript **script, jsbytecode **pc);
    static inline void MonitorOverflow(JSContext *cx);
    static inline void MonitorString(JSContext *cx);
    static inline void MonitorUnknown(JSContext *cx);

    






    static inline void Monitor(JSContext *cx, JSScript *script, jsbytecode *pc,
                               const js::Value &val);
    static inline void Monitor(JSContext *cx, const js::Value &rval);

    
    static inline void MonitorAssign(JSContext *cx, JSObject *obj, jsid id);

    
    static inline void SetThis(JSContext *cx, JSScript *script, Type type);
    static inline void SetThis(JSContext *cx, JSScript *script, const js::Value &value);
    static inline void SetLocal(JSContext *cx, JSScript *script, unsigned local, Type type);
    static inline void SetLocal(JSContext *cx, JSScript *script, unsigned local, const js::Value &value);
    static inline void SetArgument(JSContext *cx, JSScript *script, unsigned arg, Type type);
    static inline void SetArgument(JSContext *cx, JSScript *script, unsigned arg, const js::Value &value);

    static void AddFreezeConstraints(JSContext *cx, JSScript *script);
    static void Purge(JSContext *cx, JSScript *script);

    static void Sweep(FreeOp *fop, JSScript *script);
    void destroy();
};

struct ArrayTableKey;
typedef HashMap<ArrayTableKey,ReadBarriered<TypeObject>,ArrayTableKey,SystemAllocPolicy> ArrayTypeTable;

struct ObjectTableKey;
struct ObjectTableEntry;
typedef HashMap<ObjectTableKey,ObjectTableEntry,ObjectTableKey,SystemAllocPolicy> ObjectTypeTable;

struct AllocationSiteKey;
typedef HashMap<AllocationSiteKey,ReadBarriered<TypeObject>,AllocationSiteKey,SystemAllocPolicy> AllocationSiteTable;







struct CompilerOutput
{
    JSScript *script;
    bool constructing : 1;
    bool barriers : 1;
    bool pendingRecompilation : 1;
    uint32_t chunkIndex:29;

    CompilerOutput();

    bool isJM() const { return true; }

    mjit::JITScript * mjit() const;

    bool isValid() const;

    void setPendingRecompilation() {
        pendingRecompilation = true;
    }
    void invalidate() {
        script = NULL;
    }
};

struct RecompileInfo
{
    static const uint32_t NoCompilerRunning = uint32_t(-1);
    uint32_t outputIndex;

    RecompileInfo()
      : outputIndex(NoCompilerRunning)
    {
    }

    bool operator == (const RecompileInfo &o) const {
        return outputIndex == o.outputIndex;
    }
    CompilerOutput *compilerOutput(TypeCompartment &types) const;
    CompilerOutput *compilerOutput(JSContext *cx) const;
};


struct TypeCompartment
{
    

    



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

    
    bool inferenceEnabled;

    



    bool pendingNukeTypes;

    
    unsigned scriptCount;

    
    Vector<CompilerOutput> *constrainedOutputs;

    
    Vector<RecompileInfo> *pendingRecompiles;

    




    unsigned recompilations;
    unsigned frameExpansions;

    




    RecompileInfo compiledInfo;

    
    AllocationSiteTable *allocationSiteTable;

    

    ArrayTypeTable *arrayTypeTable;
    ObjectTypeTable *objectTypeTable;

    void fixArrayType(JSContext *cx, JSObject *obj);
    void fixObjectType(JSContext *cx, JSObject *obj);

    

    
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
                              JSProtoKey kind, JSObject *proto,
                              bool unknown = false, bool isDOM = false);

    
    TypeObject *newAllocationSiteTypeObject(JSContext *cx, AllocationSiteKey key);

    void nukeTypes(FreeOp *fop);
    void processPendingRecompiles(FreeOp *fop);

    
    void setPendingNukeTypes(JSContext *cx);
    void setPendingNukeTypesNoReport();

    
    void addPendingRecompile(JSContext *cx, const RecompileInfo &info);
    void addPendingRecompile(JSContext *cx, JSScript *script, jsbytecode *pc);

    
    void monitorBytecode(JSContext *cx, JSScript *script, uint32_t offset,
                         bool returnOnly = false);

    
    void markSetsUnknown(JSContext *cx, TypeObject *obj);

    void sweep(FreeOp *fop);
    void sweepCompilerOutputs(FreeOp *fop, bool discardConstraints);

    void maybePurgeAnalysis(JSContext *cx, bool force = false);

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


MOZ_NORETURN void TypeFailure(JSContext *cx, const char *fmt, ...);

} 
} 

#endif 
