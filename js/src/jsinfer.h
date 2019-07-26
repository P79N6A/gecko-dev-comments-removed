







#ifndef jsinfer_h
#define jsinfer_h

#include "mozilla/MemoryReporting.h"

#include "jsalloc.h"
#include "jsfriendapi.h"

#include "ds/IdValuePair.h"
#include "ds/LifoAlloc.h"
#include "gc/Barrier.h"
#include "gc/Marking.h"
#include "js/Utility.h"
#include "js/Vector.h"

namespace js {

class TypeRepresentation;

class TaggedProto
{
  public:
    TaggedProto() : proto(NULL) {}
    TaggedProto(JSObject *proto) : proto(proto) {}

    uintptr_t toWord() const { return uintptr_t(proto); }

    inline bool isLazy() const;
    inline bool isObject() const;
    inline JSObject *toObject() const;
    inline JSObject *toObjectOrNull() const;
    JSObject *raw() const { return proto; }

    bool operator ==(const TaggedProto &other) { return proto == other.proto; }
    bool operator !=(const TaggedProto &other) { return proto != other.proto; }

  private:
    JSObject *proto;
};

template <>
struct RootKind<TaggedProto>
{
    static ThingRootKind rootKind() { return THING_ROOT_OBJECT; }
};

template <> struct GCMethods<const TaggedProto>
{
    static TaggedProto initial() { return TaggedProto(); }
    static ThingRootKind kind() { return THING_ROOT_OBJECT; }
    static bool poisoned(const TaggedProto &v) { return IsPoisonedPtr(v.raw()); }
};

template <> struct GCMethods<TaggedProto>
{
    static TaggedProto initial() { return TaggedProto(); }
    static ThingRootKind kind() { return THING_ROOT_OBJECT; }
    static bool poisoned(const TaggedProto &v) { return IsPoisonedPtr(v.raw()); }
};

template<class Outer>
class TaggedProtoOperations
{
    const TaggedProto *value() const {
        return static_cast<const Outer*>(this)->extract();
    }

  public:
    uintptr_t toWord() const { return value()->toWord(); }
    inline bool isLazy() const;
    inline bool isObject() const;
    inline JSObject *toObject() const;
    inline JSObject *toObjectOrNull() const;
    JSObject *raw() const { return value()->raw(); }
};

template <>
class HandleBase<TaggedProto> : public TaggedProtoOperations<Handle<TaggedProto> >
{
    friend class TaggedProtoOperations<Handle<TaggedProto> >;
    const TaggedProto * extract() const {
        return static_cast<const Handle<TaggedProto>*>(this)->address();
    }
};

template <>
class RootedBase<TaggedProto> : public TaggedProtoOperations<Rooted<TaggedProto> >
{
    friend class TaggedProtoOperations<Rooted<TaggedProto> >;
    const TaggedProto *extract() const {
        return static_cast<const Rooted<TaggedProto> *>(this)->address();
    }
};

class CallObject;

namespace jit {
    struct IonScript;
}

namespace analyze {
    class ScriptAnalysis;
}

namespace types {

class TypeCallsite;
class TypeCompartment;
class TypeSet;


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


inline Type GetValueType(const Value &val);



































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

    
    TYPE_FLAG_PRIMITIVE = TYPE_FLAG_UNDEFINED | TYPE_FLAG_NULL | TYPE_FLAG_BOOLEAN |
                          TYPE_FLAG_INT32 | TYPE_FLAG_DOUBLE | TYPE_FLAG_STRING,

    
    TYPE_FLAG_OBJECT_COUNT_MASK   = 0x1f00,
    TYPE_FLAG_OBJECT_COUNT_SHIFT  = 8,
    TYPE_FLAG_OBJECT_COUNT_LIMIT  =
        TYPE_FLAG_OBJECT_COUNT_MASK >> TYPE_FLAG_OBJECT_COUNT_SHIFT,

    
    TYPE_FLAG_UNKNOWN             = 0x00010000,

    
    TYPE_FLAG_BASE_MASK           = 0x000100ff,

    













    TYPE_FLAG_STACK_SET           = 0x00020000,
    TYPE_FLAG_HEAP_SET            = 0x00040000,

    

    



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

    
    OBJECT_FLAG_ADDENDUM_CLEARED      = 0x2,

    




    OBJECT_FLAG_NEW_SCRIPT_REGENERATE = 0x4,

    



    OBJECT_FLAG_SETS_MARKED_UNKNOWN   = 0x8,

    
    OBJECT_FLAG_PROPERTY_COUNT_MASK   = 0xfff0,
    OBJECT_FLAG_PROPERTY_COUNT_SHIFT  = 4,
    OBJECT_FLAG_PROPERTY_COUNT_LIMIT  =
        OBJECT_FLAG_PROPERTY_COUNT_MASK >> OBJECT_FLAG_PROPERTY_COUNT_SHIFT,

    
    OBJECT_FLAG_SPARSE_INDEXES        = 0x00010000,

    
    OBJECT_FLAG_NON_PACKED            = 0x00020000,

    



    OBJECT_FLAG_LENGTH_OVERFLOW       = 0x00040000,

    



    
    OBJECT_FLAG_ITERATED              = 0x00100000,

    
    OBJECT_FLAG_REGEXP_FLAGS_SET      = 0x00200000,

    
    OBJECT_FLAG_EMULATES_UNDEFINED    = 0x00400000,

    



    OBJECT_FLAG_RUNONCE_INVALIDATED   = 0x00800000,

    
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
class TemporaryTypeSet;


class TypeSet
{
  protected:
    
    TypeFlags flags;

    
    TypeObjectKey **objectSet;

  public:

    
    TypeConstraint *constraintList;

    TypeSet()
      : flags(0), objectSet(NULL), constraintList(NULL)
    {}

    void print();

    inline void sweep(JS::Zone *zone);

    
    inline bool hasType(Type type) const;

    TypeFlags baseFlags() const { return flags & TYPE_FLAG_BASE_MASK; }
    bool unknown() const { return !!(flags & TYPE_FLAG_UNKNOWN); }
    bool unknownObject() const { return !!(flags & (TYPE_FLAG_UNKNOWN | TYPE_FLAG_ANYOBJECT)); }

    bool empty() const { return !baseFlags() && !baseObjectCount(); }
    bool noConstraints() const { return constraintList == NULL; }

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

    
    static TemporaryTypeSet *unionSets(TypeSet *a, TypeSet *b, LifoAlloc *alloc);

    



    inline void addType(ExclusiveContext *cx, Type type);

    
    inline void setOwnProperty(ExclusiveContext *cx, bool configured);

    




    inline unsigned getObjectCount() const;
    inline TypeObjectKey *getObject(unsigned i) const;
    inline JSObject *getSingleObject(unsigned i) const;
    inline TypeObject *getTypeObject(unsigned i) const;
    inline bool getTypeOrSingleObject(JSContext *cx, unsigned i, TypeObject **obj) const;

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

    bool isStackSet() {
        return flags & TYPE_FLAG_STACK_SET;
    }
    bool isHeapSet() {
        return flags & TYPE_FLAG_HEAP_SET;
    }

    



    bool isSubset(TypeSet *other);

    
    void addTypesToConstraint(JSContext *cx, TypeConstraint *constraint);

    
    void add(JSContext *cx, TypeConstraint *constraint, bool callExisting = true);

    inline StackTypeSet *toStackSet();
    inline HeapTypeSet *toHeapSet();

    



    TemporaryTypeSet *clone(LifoAlloc *alloc) const;

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
    StackTypeSet() { flags |= TYPE_FLAG_STACK_SET; }

    
    void addSubset(JSContext *cx, StackTypeSet *target);
};

class HeapTypeSet : public TypeSet
{
  public:
    HeapTypeSet() { flags |= TYPE_FLAG_HEAP_SET; }

    
    void addSubset(JSContext *cx, HeapTypeSet *target);

    
    void addFreeze(JSContext *cx);

    




    static void WatchObjectStateChange(JSContext *cx, TypeObject *object);

    
    static bool HasObjectFlags(JSContext *cx, TypeObject *object, TypeObjectFlags flags);

    






    bool isOwnProperty(JSContext *cx, TypeObject *object, bool configurable);

    
    bool knownNonEmpty(JSContext *cx);

    
    bool knownSubset(JSContext *cx, HeapTypeSet *other);

    
    JSObject *getSingleton(JSContext *cx);

    



    bool needsBarrier(JSContext *cx);

    
    JSValueType getKnownTypeTag(JSContext *cx);
};

class TemporaryTypeSet : public TypeSet
{
  public:
    TemporaryTypeSet() {}
    TemporaryTypeSet(Type type);

    TemporaryTypeSet(uint32_t flags, TypeObjectKey **objectSet) {
        this->flags = flags;
        this->objectSet = objectSet;
        JS_ASSERT(!isStackSet() && !isHeapSet());
    }

    
    bool addObject(TypeObjectKey *key, LifoAlloc *alloc);

    








    
    JSValueType getKnownTypeTag();

    
    bool mightBeType(JSValueType type);

    bool isMagicArguments() { return getKnownTypeTag() == JSVAL_TYPE_MAGIC; }

    
    bool maybeObject() { return unknownObject() || baseObjectCount() > 0; }

    




    bool objectOrSentinel() {
        TypeFlags flags = TYPE_FLAG_UNDEFINED | TYPE_FLAG_NULL | TYPE_FLAG_ANYOBJECT;
        if (baseFlags() & (~flags & TYPE_FLAG_BASE_MASK))
            return false;

        return hasAnyFlag(TYPE_FLAG_ANYOBJECT) || baseObjectCount() > 0;
    }

    
    bool hasObjectFlags(JSContext *cx, TypeObjectFlags flags);

    
    const Class *getKnownClass();

    
    JSObject *getCommonPrototype();

    
    int getTypedArrayType();

    
    bool isDOMClass();

    
    bool maybeCallable();

    
    JSObject *getSingleton();

    
    bool propertyNeedsBarrier(JSContext *cx, jsid id);

    



    bool filtersType(const TemporaryTypeSet *other, Type type) const;

    enum DoubleConversion {
        
        AlwaysConvertToDoubles,

        
        MaybeConvertToDoubles,

        
        DontConvertToDoubles,

        
        AmbiguousDoubleConversion
    };

    



    DoubleConversion convertDoubleElements(JSContext *cx);
};

inline StackTypeSet *
TypeSet::toStackSet()
{
    JS_ASSERT(isStackSet());
    return (StackTypeSet *) this;
}

inline HeapTypeSet *
TypeSet::toHeapSet()
{
    JS_ASSERT(isHeapSet());
    return (HeapTypeSet *) this;
}

bool
AddClearDefiniteGetterSetterForPrototypeChain(JSContext *cx, TypeObject *type, jsid id);

void
AddClearDefiniteFunctionUsesInScript(JSContext *cx, TypeObject *type,
                                     JSScript *script, JSScript *calleeScript);







struct TypeResult
{
    uint32_t offset;
    Type type;
    TypeResult *next;

    TypeResult(uint32_t offset, Type type)
        : offset(offset), type(type), next(NULL)
    {}
};


inline bool isInlinableCall(jsbytecode *pc);



























































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

    Property(jsid id)
      : id(id)
    {}

    Property(const Property &o)
      : id(o.id.get()), types(o.types)
    {}

    static uint32_t keyBits(jsid id) { return uint32_t(JSID_BITS(id)); }
    static jsid getKey(Property *p) { return p->id; }
};

struct TypeNewScript;
struct TypeTypedObject;

struct TypeObjectAddendum
{
    enum Kind {
        NewScript,
        TypedObject
    };

    TypeObjectAddendum(Kind kind);

    const Kind kind;

    bool isNewScript() {
        return kind == NewScript;
    }

    TypeNewScript *asNewScript() {
        JS_ASSERT(isNewScript());
        return (TypeNewScript*) this;
    }

    bool isTypedObject() {
        return kind == TypedObject;
    }

    TypeTypedObject *asTypedObject() {
        JS_ASSERT(isTypedObject());
        return (TypeTypedObject*) this;
    }

    static inline void writeBarrierPre(TypeObjectAddendum *type);

    static void writeBarrierPost(TypeObjectAddendum *newScript, void *addr) {}
};











struct TypeNewScript : public TypeObjectAddendum
{
    TypeNewScript();

    HeapPtrFunction fun;

    
    gc::AllocKind allocKind;

    



    HeapPtrShape  shape;

    








    struct Initializer {
        enum Kind {
            SETPROP,
            SETPROP_FRAME,
            DONE
        } kind;
        uint32_t offset;
        Initializer(Kind kind, uint32_t offset)
          : kind(kind), offset(offset)
        {}
    };
    Initializer *initializerList;

    static inline void writeBarrierPre(TypeNewScript *newScript);
};

struct TypeTypedObject : public TypeObjectAddendum
{
    TypeTypedObject(TypeRepresentation *repr);

    TypeRepresentation *const typeRepr;
};




























struct TypeObject : gc::BarrieredCell<TypeObject>
{
    
    const Class *clasp;

    
    HeapPtrObject proto;

    




    HeapPtrObject singleton;

    



    static const size_t LAZY_SINGLETON = 1;
    bool lazy() const { return singleton == (JSObject *) LAZY_SINGLETON; }

    
    TypeObjectFlags flags;

    









    HeapPtr<TypeObjectAddendum> addendum;

    bool hasNewScript() {
        return addendum && addendum->isNewScript();
    }

    TypeNewScript *newScript() {
        return addendum->asNewScript();
    }

    bool hasTypedObject() {
        return addendum && addendum->isTypedObject();
    }

    TypeTypedObject *typedObject() {
        return addendum->asTypedObject();
    }

    






    bool addTypedObjectAddendum(JSContext *cx, TypeRepresentation *repr);

    





























    Property **propertySet;

    
    HeapPtrFunction interpretedFunction;

#if JS_BITS_PER_WORD == 32
    uint32_t padding;
#endif

    inline TypeObject(const Class *clasp, TaggedProto proto, bool isFunction, bool unknown);

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

    





    inline HeapTypeSet *getProperty(ExclusiveContext *cx, jsid id, bool own);

    
    inline HeapTypeSet *maybeGetProperty(ExclusiveContext *cx, jsid id);

    inline unsigned getPropertyCount();
    inline Property *getProperty(unsigned i);

    
    inline int getTypedArrayType();

    



    

    

    bool addProperty(ExclusiveContext *cx, jsid id, Property **pprop);
    bool addDefiniteProperties(ExclusiveContext *cx, JSObject *obj);
    bool matchDefiniteProperties(HandleObject obj);
    void addPrototype(JSContext *cx, TypeObject *proto);
    void addPropertyType(ExclusiveContext *cx, jsid id, Type type);
    void addPropertyType(ExclusiveContext *cx, jsid id, const Value &value);
    void addPropertyType(ExclusiveContext *cx, const char *name, Type type);
    void addPropertyType(ExclusiveContext *cx, const char *name, const Value &value);
    void markPropertyConfigured(ExclusiveContext *cx, jsid id);
    void markStateChange(ExclusiveContext *cx);
    void setFlags(ExclusiveContext *cx, TypeObjectFlags flags);
    void markUnknown(ExclusiveContext *cx);
    void clearAddendum(ExclusiveContext *cx);
    void clearNewScriptAddendum(ExclusiveContext *cx);
    void clearTypedObjectAddendum(ExclusiveContext *cx);
    void getFromPrototypes(JSContext *cx, jsid id, HeapTypeSet *types, bool force = false);

    void print();

    inline void clearProperties();
    inline void sweep(FreeOp *fop);

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf);

    




    void finalize(FreeOp *fop) {}

    static inline ThingRootKind rootKind() { return THING_ROOT_TYPE_OBJECT; }

  private:
    inline uint32_t basePropertyCount() const;
    inline void setBasePropertyCount(uint32_t count);

    static void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(TypeObject, proto) == offsetof(js::shadow::TypeObject, proto));
    }
};





struct TypeObjectEntry : DefaultHasher<ReadBarriered<TypeObject> >
{
    struct Lookup {
        const Class *clasp;
        TaggedProto proto;

        Lookup(const Class *clasp, TaggedProto proto) : clasp(clasp), proto(proto) {}
    };

    static inline HashNumber hash(const Lookup &lookup);
    static inline bool match(TypeObject *key, const Lookup &lookup);
};
typedef HashSet<ReadBarriered<TypeObject>, TypeObjectEntry, SystemAllocPolicy> TypeObjectSet;


bool
UseNewType(JSContext *cx, JSScript *script, jsbytecode *pc);

bool
UseNewTypeForClone(JSFunction *fun);





bool
ArrayPrototypeHasIndexedProperty(JSContext *cx, HandleScript script);


bool
TypeCanHaveExtraIndexedProperties(JSContext *cx, TemporaryTypeSet *types);


class TypeScript
{
    friend class ::JSScript;

    
    analyze::ScriptAnalysis *analysis;

    



    uint32_t *bytecodeMap;

  public:
    
    TypeResult *dynamicList;

    




    HeapTypeSet *propertyReadTypes;

    
    TypeSet *typeArray() const { return (TypeSet *) (uintptr_t(this) + sizeof(TypeScript)); }

    static inline unsigned NumTypeSets(JSScript *script);

    static inline StackTypeSet *ThisTypes(JSScript *script);
    static inline StackTypeSet *ArgTypes(JSScript *script, unsigned i);

    
    static inline StackTypeSet *BytecodeTypes(JSScript *script, jsbytecode *pc);

    
    static inline TypeObject *StandardType(JSContext *cx, JSProtoKey kind);

    
    static inline TypeObject *InitObject(JSContext *cx, JSScript *script, jsbytecode *pc,
                                         JSProtoKey kind);

    



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

    
    static inline void MonitorAssign(JSContext *cx, HandleObject obj, jsid id);

    
    static inline void SetThis(JSContext *cx, JSScript *script, Type type);
    static inline void SetThis(JSContext *cx, JSScript *script, const js::Value &value);
    static inline void SetArgument(JSContext *cx, JSScript *script, unsigned arg, Type type);
    static inline void SetArgument(JSContext *cx, JSScript *script, unsigned arg,
                                   const js::Value &value);

    static void AddFreezeConstraints(JSContext *cx, JSScript *script);
    static void Purge(JSContext *cx, HandleScript script);

    static void Sweep(FreeOp *fop, JSScript *script);
    void destroy();

#ifdef DEBUG
    void printTypes(JSContext *cx, HandleScript script) const;
#endif
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
    enum Kind {
        Ion,
        ParallelIon
    };

    JSScript *script;

    
    
    
    unsigned kindInt : 2;
    bool pendingRecompilation : 1;

    CompilerOutput();

    Kind kind() const { return static_cast<Kind>(kindInt); }
    void setKind(Kind k) { kindInt = k; }

    jit::IonScript *ion() const;

    bool isValid() const;

    void setPendingRecompilation() {
        pendingRecompilation = true;
    }
    void invalidate() {
        script = NULL;
    }
    bool isInvalidated() const {
        return script == NULL;
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

    
    unsigned scriptCount;

    
    Vector<CompilerOutput> *constrainedOutputs;

    
    Vector<RecompileInfo> *pendingRecompiles;

    




    RecompileInfo compiledInfo;

    
    AllocationSiteTable *allocationSiteTable;

    

    ArrayTypeTable *arrayTypeTable;
    ObjectTypeTable *objectTypeTable;

  private:
    void setTypeToHomogenousArray(ExclusiveContext *cx, JSObject *obj, Type type);

  public:
    void fixArrayType(ExclusiveContext *cx, JSObject *obj);
    void fixObjectType(ExclusiveContext *cx, JSObject *obj);
    void fixRestArgumentsType(ExclusiveContext *cx, JSObject *obj);

    JSObject *newTypedObject(JSContext *cx, IdValuePair *properties, size_t nproperties);

    TypeCompartment();
    ~TypeCompartment();

    inline JSCompartment *compartment();

    
    inline void addPending(JSContext *cx, TypeConstraint *constraint, TypeSet *source, Type type);
    bool growPendingArray(JSContext *cx);

    
    inline void resolvePending(JSContext *cx);

    
    void print(JSContext *cx, bool force);

    





    TypeObject *newTypeObject(ExclusiveContext *cx, const Class *clasp, Handle<TaggedProto> proto,
                              bool unknown = false);

    
    TypeObject *addAllocationSiteTypeObject(JSContext *cx, AllocationSiteKey key);

    void processPendingRecompiles(FreeOp *fop);

    
    void setPendingNukeTypes(ExclusiveContext *cx);

    
    void addPendingRecompile(JSContext *cx, const RecompileInfo &info);
    void addPendingRecompile(JSContext *cx, JSScript *script);

    
    void markSetsUnknown(JSContext *cx, TypeObject *obj);

    void sweep(FreeOp *fop);
    void sweepShapes(FreeOp *fop);
    void sweepCompilerOutputs(FreeOp *fop, bool discardConstraints);

    void finalizeObjects();
};

void FixRestArgumentsType(ExclusiveContext *cxArg, JSObject *obj);

struct TypeZone
{
    JS::Zone                     *zone_;

    
    static const size_t TYPE_LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 8 * 1024;
    js::LifoAlloc                typeLifoAlloc;

    



    bool                         pendingNukeTypes;

    
    bool                         inferenceEnabled;

    TypeZone(JS::Zone *zone);
    ~TypeZone();
    void init(JSContext *cx);

    JS::Zone *zone() const { return zone_; }

    void sweep(FreeOp *fop, bool releaseTypes);

    
    void setPendingNukeTypes();

    void nukeTypes(FreeOp *fop);
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
