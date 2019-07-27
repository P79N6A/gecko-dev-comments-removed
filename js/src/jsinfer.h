







#ifndef jsinfer_h
#define jsinfer_h

#include "mozilla/MemoryReporting.h"
#include "mozilla/TypedEnum.h"

#include "jsalloc.h"
#include "jsfriendapi.h"
#include "jstypes.h"

#include "ds/IdValuePair.h"
#include "ds/LifoAlloc.h"
#include "gc/Barrier.h"
#include "gc/Marking.h"
#include "jit/IonTypes.h"
#include "js/Utility.h"
#include "js/Vector.h"

namespace js {

class TypeDescr;

class TaggedProto
{
  public:
    static JSObject * const LazyProto;

    TaggedProto() : proto(nullptr) {}
    explicit TaggedProto(JSObject *proto) : proto(proto) {}

    uintptr_t toWord() const { return uintptr_t(proto); }

    bool isLazy() const {
        return proto == LazyProto;
    }
    bool isObject() const {
        
        return uintptr_t(proto) > uintptr_t(TaggedProto::LazyProto);
    }
    JSObject *toObject() const {
        JS_ASSERT(isObject());
        return proto;
    }
    JSObject *toObjectOrNull() const {
        JS_ASSERT(!proto || isObject());
        return proto;
    }
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
    static bool poisoned(const TaggedProto &v) { return IsPoisonedPtr(v.raw()); }
};

template <> struct GCMethods<TaggedProto>
{
    static TaggedProto initial() { return TaggedProto(); }
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
    inline bool isLazy() const { return value()->isLazy(); }
    inline bool isObject() const { return value()->isObject(); }
    inline JSObject *toObject() const { return value()->toObject(); }
    inline JSObject *toObjectOrNull() const { return value()->toObjectOrNull(); }
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














enum ExecutionMode {
    
    SequentialExecution,

    



    ParallelExecution,

    




    



    DefinitePropertiesAnalysis,

    



    ArgumentsUsageAnalysis
};

inline const char *
ExecutionModeString(ExecutionMode mode)
{
    switch (mode) {
      case SequentialExecution:
        return "SequentialExecution";
      case ParallelExecution:
        return "ParallelExecution";
      case DefinitePropertiesAnalysis:
        return "DefinitePropertiesAnalysis";
      case ArgumentsUsageAnalysis:
        return "ArgumentsUsageAnalysis";
      default:
        MOZ_CRASH("Invalid ExecutionMode");
    }
}





static const unsigned NumExecutionModes = ParallelExecution + 1;

template <ExecutionMode mode>
struct ExecutionModeTraits
{
};

template <> struct ExecutionModeTraits<SequentialExecution>
{
    typedef JSContext * ContextType;
    typedef ExclusiveContext * ExclusiveContextType;

    static inline JSContext *toContextType(ExclusiveContext *cx);
};

template <> struct ExecutionModeTraits<ParallelExecution>
{
    typedef ForkJoinContext * ContextType;
    typedef ForkJoinContext * ExclusiveContextType;

    static inline ForkJoinContext *toContextType(ForkJoinContext *cx) { return cx; }
};

namespace jit {
    struct IonScript;
    class IonAllocPolicy;
    class TempAllocator;
}

namespace types {

struct TypeZone;
class TypeSet;
struct TypeObjectKey;






class Type
{
    uintptr_t data;
    explicit Type(uintptr_t data) : data(data) {}

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

    bool isMagicArguments() const {
        return primitive() == JSVAL_TYPE_MAGIC;
    }

    bool isSomeObject() const {
        return data == JSVAL_TYPE_OBJECT || data > JSVAL_TYPE_UNKNOWN;
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

    bool isObjectUnchecked() const {
        return data > JSVAL_TYPE_UNKNOWN;
    }

    inline TypeObjectKey *objectKey() const;

    

    bool isSingleObject() const {
        return isObject() && !!(data & 1);
    }

    inline JSObject *singleObject() const;
    inline JSObject *singleObjectNoBarrier() const;

    

    bool isTypeObject() const {
        return isObject() && !(data & 1);
    }

    inline TypeObject *typeObject() const;
    inline TypeObject *typeObjectNoBarrier() const;

    bool operator == (Type o) const { return data == o.data; }
    bool operator != (Type o) const { return data != o.data; }

    static inline Type UndefinedType() { return Type(JSVAL_TYPE_UNDEFINED); }
    static inline Type NullType()      { return Type(JSVAL_TYPE_NULL); }
    static inline Type BooleanType()   { return Type(JSVAL_TYPE_BOOLEAN); }
    static inline Type Int32Type()     { return Type(JSVAL_TYPE_INT32); }
    static inline Type DoubleType()    { return Type(JSVAL_TYPE_DOUBLE); }
    static inline Type StringType()    { return Type(JSVAL_TYPE_STRING); }
    static inline Type SymbolType()    { return Type(JSVAL_TYPE_SYMBOL); }
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

    static js::ThingRootKind rootKind() { return js::THING_ROOT_TYPE; }
};


inline Type GetValueType(const Value &val);






inline Type GetMaybeOptimizedOutValueType(const Value &val);

















class TypeConstraint
{
public:
    
    TypeConstraint *next;

    TypeConstraint()
        : next(nullptr)
    {}

    
    virtual const char *kind() = 0;

    
    virtual void newType(JSContext *cx, TypeSet *source, Type type) = 0;

    



    virtual void newPropertyState(JSContext *cx, TypeSet *source) {}

    




    virtual void newObjectState(JSContext *cx, TypeObject *object) {}

    



    virtual bool sweep(TypeZone &zone, TypeConstraint **res) = 0;
};


enum MOZ_ENUM_TYPE(uint32_t) {
    TYPE_FLAG_UNDEFINED =   0x1,
    TYPE_FLAG_NULL      =   0x2,
    TYPE_FLAG_BOOLEAN   =   0x4,
    TYPE_FLAG_INT32     =   0x8,
    TYPE_FLAG_DOUBLE    =  0x10,
    TYPE_FLAG_STRING    =  0x20,
    TYPE_FLAG_SYMBOL    =  0x40,
    TYPE_FLAG_LAZYARGS  =  0x80,
    TYPE_FLAG_ANYOBJECT = 0x100,

    
    TYPE_FLAG_PRIMITIVE = TYPE_FLAG_UNDEFINED | TYPE_FLAG_NULL | TYPE_FLAG_BOOLEAN |
                          TYPE_FLAG_INT32 | TYPE_FLAG_DOUBLE | TYPE_FLAG_STRING |
                          TYPE_FLAG_SYMBOL,

    
    TYPE_FLAG_OBJECT_COUNT_MASK   = 0x3e00,
    TYPE_FLAG_OBJECT_COUNT_SHIFT  = 9,
    TYPE_FLAG_OBJECT_COUNT_LIMIT  =
        TYPE_FLAG_OBJECT_COUNT_MASK >> TYPE_FLAG_OBJECT_COUNT_SHIFT,

    
    TYPE_FLAG_UNKNOWN             = 0x00004000,

    
    TYPE_FLAG_BASE_MASK           = 0x000041ff,

    

    




    TYPE_FLAG_NON_DATA_PROPERTY = 0x00008000,

    
    TYPE_FLAG_NON_WRITABLE_PROPERTY = 0x00010000,

    
    TYPE_FLAG_NON_CONSTANT_PROPERTY = 0x00020000,

    








    TYPE_FLAG_DEFINITE_MASK       = 0xfffc0000,
    TYPE_FLAG_DEFINITE_SHIFT      = 18
};
typedef uint32_t TypeFlags;


enum MOZ_ENUM_TYPE(uint32_t) {
    
    OBJECT_FLAG_FROM_ALLOCATION_SITE  = 0x1,

    



    OBJECT_FLAG_NURSERY_PROTO         = 0x2,

    



    OBJECT_FLAG_SETS_MARKED_UNKNOWN   = 0x4,

    
    OBJECT_FLAG_PROPERTY_COUNT_MASK   = 0xfff8,
    OBJECT_FLAG_PROPERTY_COUNT_SHIFT  = 3,
    OBJECT_FLAG_PROPERTY_COUNT_LIMIT  =
        OBJECT_FLAG_PROPERTY_COUNT_MASK >> OBJECT_FLAG_PROPERTY_COUNT_SHIFT,

    
    OBJECT_FLAG_SPARSE_INDEXES        = 0x00010000,

    
    OBJECT_FLAG_NON_PACKED            = 0x00020000,

    



    OBJECT_FLAG_LENGTH_OVERFLOW       = 0x00040000,

    
    OBJECT_FLAG_ITERATED              = 0x00080000,

    
    OBJECT_FLAG_REGEXP_FLAGS_SET      = 0x00100000,

    



    OBJECT_FLAG_RUNONCE_INVALIDATED   = 0x00200000,

    



    OBJECT_FLAG_PRE_TENURE            = 0x00400000,

    
    OBJECT_FLAG_COPY_ON_WRITE         = 0x00800000,

    



    OBJECT_FLAG_UNKNOWN_PROPERTIES    = 0x01000000,

    
    OBJECT_FLAG_DYNAMIC_MASK          = 0x01ff0000,

    
    OBJECT_FLAG_UNKNOWN_MASK =
        OBJECT_FLAG_DYNAMIC_MASK
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

    TypeSet()
      : flags(0), objectSet(nullptr)
    {}

    void print();

    
    inline bool hasType(Type type) const;

    TypeFlags baseFlags() const { return flags & TYPE_FLAG_BASE_MASK; }
    bool unknown() const { return !!(flags & TYPE_FLAG_UNKNOWN); }
    bool unknownObject() const { return !!(flags & (TYPE_FLAG_UNKNOWN | TYPE_FLAG_ANYOBJECT)); }
    bool empty() const { return !baseFlags() && !baseObjectCount(); }

    bool hasAnyFlag(TypeFlags flags) const {
        JS_ASSERT((flags & TYPE_FLAG_BASE_MASK) == flags);
        return !!(baseFlags() & flags);
    }

    bool nonDataProperty() const {
        return flags & TYPE_FLAG_NON_DATA_PROPERTY;
    }
    bool nonWritableProperty() const {
        return flags & TYPE_FLAG_NON_WRITABLE_PROPERTY;
    }
    bool nonConstantProperty() const {
        return flags & TYPE_FLAG_NON_CONSTANT_PROPERTY;
    }
    bool definiteProperty() const { return flags & TYPE_FLAG_DEFINITE_MASK; }
    unsigned definiteSlot() const {
        JS_ASSERT(definiteProperty());
        return (flags >> TYPE_FLAG_DEFINITE_SHIFT) - 1;
    }

    
    static TemporaryTypeSet *unionSets(TypeSet *a, TypeSet *b, LifoAlloc *alloc);
    
    static TemporaryTypeSet *intersectSets(TemporaryTypeSet *a, TemporaryTypeSet *b, LifoAlloc *alloc);

    
    void addType(Type type, LifoAlloc *alloc);

    
    typedef Vector<Type, 1, SystemAllocPolicy> TypeList;
    bool enumerateTypes(TypeList *list);

    




    inline unsigned getObjectCount() const;
    inline TypeObjectKey *getObject(unsigned i) const;
    inline JSObject *getSingleObject(unsigned i) const;
    inline TypeObject *getTypeObject(unsigned i) const;
    inline JSObject *getSingleObjectNoBarrier(unsigned i) const;
    inline TypeObject *getTypeObjectNoBarrier(unsigned i) const;

    
    inline const Class *getObjectClass(unsigned i) const;

    bool canSetDefinite(unsigned slot) {
        
        return (slot + 1) <= (unsigned(TYPE_FLAG_DEFINITE_MASK) >> TYPE_FLAG_DEFINITE_SHIFT);
    }
    void setDefinite(unsigned slot) {
        JS_ASSERT(canSetDefinite(slot));
        flags |= ((slot + 1) << TYPE_FLAG_DEFINITE_SHIFT);
        JS_ASSERT(definiteSlot() == slot);
    }

    
    bool mightBeMIRType(jit::MIRType type);

    



    bool isSubset(TypeSet *other);

    



    bool objectsAreSubset(TypeSet *other);

    
    bool addTypesToConstraint(JSContext *cx, TypeConstraint *constraint);

    
    TemporaryTypeSet *clone(LifoAlloc *alloc) const;
    bool clone(LifoAlloc *alloc, TemporaryTypeSet *result) const;

    
    TemporaryTypeSet *filter(LifoAlloc *alloc, bool filterUndefined, bool filterNull) const;
    
    TemporaryTypeSet *cloneObjectsOnly(LifoAlloc *alloc);
    TemporaryTypeSet *cloneWithoutObjects(LifoAlloc *alloc);

    
    static void readBarrier(const TypeSet *types);

  protected:
    uint32_t baseObjectCount() const {
        return (flags & TYPE_FLAG_OBJECT_COUNT_MASK) >> TYPE_FLAG_OBJECT_COUNT_SHIFT;
    }
    inline void setBaseObjectCount(uint32_t count);

    void clearObjects();
};


class ConstraintTypeSet : public TypeSet
{
  public:
    
    TypeConstraint *constraintList;

    ConstraintTypeSet() : constraintList(nullptr) {}

    



    void addType(ExclusiveContext *cx, Type type);

    
    bool addConstraint(JSContext *cx, TypeConstraint *constraint, bool callExisting = true);

    inline void sweep(JS::Zone *zone, bool *oom);
};

class StackTypeSet : public ConstraintTypeSet
{
  public:
};

class HeapTypeSet : public ConstraintTypeSet
{
    inline void newPropertyState(ExclusiveContext *cx);

  public:
    
    inline void setNonDataProperty(ExclusiveContext *cx);
    inline void setNonDataPropertyIgnoringConstraints(); 

    
    inline void setNonWritableProperty(ExclusiveContext *cx);

    
    inline void setNonConstantProperty(ExclusiveContext *cx);
};

class CompilerConstraintList;

CompilerConstraintList *
NewCompilerConstraintList(jit::TempAllocator &alloc);

class TemporaryTypeSet : public TypeSet
{
  public:
    TemporaryTypeSet() {}
    TemporaryTypeSet(LifoAlloc *alloc, Type type);

    TemporaryTypeSet(uint32_t flags, TypeObjectKey **objectSet) {
        this->flags = flags;
        this->objectSet = objectSet;
    }

    








    
    jit::MIRType getKnownMIRType();

    bool isMagicArguments() { return getKnownMIRType() == jit::MIRType_MagicOptimizedArguments; }

    
    bool maybeObject() { return unknownObject() || baseObjectCount() > 0; }

    




    bool objectOrSentinel() {
        TypeFlags flags = TYPE_FLAG_UNDEFINED | TYPE_FLAG_NULL | TYPE_FLAG_ANYOBJECT;
        if (baseFlags() & (~flags & TYPE_FLAG_BASE_MASK))
            return false;

        return hasAnyFlag(TYPE_FLAG_ANYOBJECT) || baseObjectCount() > 0;
    }

    
    bool hasObjectFlags(CompilerConstraintList *constraints, TypeObjectFlags flags);

    
    const Class *getKnownClass();

    
    enum ForAllResult {
        EMPTY=1,                
        ALL_TRUE,               
        ALL_FALSE,              
        MIXED,                  
                                
                                
    };

    


    ForAllResult forAllClasses(bool (*func)(const Class *clasp));

    
    JSObject *getCommonPrototype();

    
    Scalar::Type getTypedArrayType();

    
    bool isDOMClass();

    
    bool maybeCallable();

    
    bool maybeEmulatesUndefined();

    
    JSObject *getSingleton();

    
    bool propertyNeedsBarrier(CompilerConstraintList *constraints, jsid id);

    



    bool filtersType(const TemporaryTypeSet *other, Type type) const;

    enum DoubleConversion {
        
        AlwaysConvertToDoubles,

        
        MaybeConvertToDoubles,

        
        DontConvertToDoubles,

        
        AmbiguousDoubleConversion
    };

    



    DoubleConversion convertDoubleElements(CompilerConstraintList *constraints);
};

bool
AddClearDefiniteGetterSetterForPrototypeChain(JSContext *cx, TypeObject *type, HandleId id);

bool
AddClearDefiniteFunctionUsesInScript(JSContext *cx, TypeObject *type,
                                     JSScript *script, JSScript *calleeScript);


inline bool isInlinableCall(jsbytecode *pc);


struct Property
{
    
    HeapId id;

    
    HeapTypeSet types;

    explicit Property(jsid id)
      : id(id)
    {}

    Property(const Property &o)
      : id(o.id.get()), types(o.types)
    {}

    static uint32_t keyBits(jsid id) { return uint32_t(JSID_BITS(id)); }
    static jsid getKey(Property *p) { return p->id; }
};







































class TypeNewScript
{
  public:
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

  private:
    
    
    
    HeapPtrFunction fun;

    
    
    
    
    
    static const uint32_t PRELIMINARY_OBJECT_COUNT = 20;
    JSObject **preliminaryObjects;

    
    
    
    
    HeapPtrObject templateObject_;

    
    
    
    
    
    
    
    
    Initializer *initializerList;

    
    
    
    
    
    HeapPtrShape initializedShape_;

    
    
    HeapPtrTypeObject initializedType_;

  public:
    TypeNewScript() { mozilla::PodZero(this); }
    ~TypeNewScript() {
        js_free(preliminaryObjects);
        js_free(initializerList);
    }

    static inline void writeBarrierPre(TypeNewScript *newScript);
    static void writeBarrierPost(TypeNewScript *newScript, void *addr) {}

    bool analyzed() const {
        if (preliminaryObjects) {
            JS_ASSERT(!templateObject());
            JS_ASSERT(!initializerList);
            JS_ASSERT(!initializedShape());
            JS_ASSERT(!initializedType());
            return false;
        }
        JS_ASSERT(templateObject());
        return true;
    }

    JSObject *templateObject() const {
        return templateObject_;
    }

    Shape *initializedShape() const {
        return initializedShape_;
    }

    TypeObject *initializedType() const {
        return initializedType_;
    }

    void trace(JSTracer *trc);
    void sweep(FreeOp *fop);

    void registerNewObject(JSObject *res);
    void unregisterNewObject(JSObject *res);
    bool maybeAnalyze(JSContext *cx, TypeObject *type, bool *regenerate, bool force = false);

    void rollbackPartiallyInitializedObjects(JSContext *cx, TypeObject *type);

    static void make(JSContext *cx, TypeObject *type, JSFunction *fun);
};






















struct TypeObject : gc::BarrieredCell<TypeObject>
{
  private:
    
    const Class *clasp_;

    
    HeapPtrObject proto_;

    




    HeapPtrObject singleton_;

  public:

    const Class *clasp() const {
        return clasp_;
    }

    void setClasp(const Class *clasp) {
        JS_ASSERT(singleton());
        clasp_ = clasp;
    }

    TaggedProto proto() const {
        return TaggedProto(proto_);
    }

    JSObject *singleton() const {
        return singleton_;
    }

    
    HeapPtrObject &protoRaw() { return proto_; }
    HeapPtrObject &singletonRaw() { return singleton_; }

    void setProto(JSContext *cx, TaggedProto proto);
    void setProtoUnchecked(TaggedProto proto) {
        proto_ = proto.raw();
    }

    void initSingleton(JSObject *singleton) {
        singleton_ = singleton;
    }

    



    static const size_t LAZY_SINGLETON = 1;
    bool lazy() const { return singleton() == (JSObject *) LAZY_SINGLETON; }

  private:
    
    TypeObjectFlags flags_;

    




    HeapPtrTypeNewScript newScript_;

  public:

    TypeObjectFlags flags() const {
        return flags_;
    }

    void addFlags(TypeObjectFlags flags) {
        flags_ |= flags;
    }

    void clearFlags(TypeObjectFlags flags) {
        flags_ &= ~flags;
    }

    TypeNewScript *newScript() {
        return newScript_;
    }

    void setNewScript(TypeNewScript *newScript);

  private:
    
































    Property **propertySet;
  public:

    
    HeapPtrFunction interpretedFunction;

#if JS_BITS_PER_WORD == 32
    uint32_t padding;
#endif

    inline TypeObject(const Class *clasp, TaggedProto proto, TypeObjectFlags initialFlags);

    bool hasAnyFlags(TypeObjectFlags flags) {
        JS_ASSERT((flags & OBJECT_FLAG_DYNAMIC_MASK) == flags);
        return !!(this->flags() & flags);
    }
    bool hasAllFlags(TypeObjectFlags flags) {
        JS_ASSERT((flags & OBJECT_FLAG_DYNAMIC_MASK) == flags);
        return (this->flags() & flags) == flags;
    }

    bool unknownProperties() {
        JS_ASSERT_IF(flags() & OBJECT_FLAG_UNKNOWN_PROPERTIES,
                     hasAllFlags(OBJECT_FLAG_DYNAMIC_MASK));
        return !!(flags() & OBJECT_FLAG_UNKNOWN_PROPERTIES);
    }

    bool shouldPreTenure() {
        return hasAnyFlags(OBJECT_FLAG_PRE_TENURE) && !unknownProperties();
    }

    bool hasTenuredProto() const {
        return !(flags() & OBJECT_FLAG_NURSERY_PROTO);
    }

    gc::InitialHeap initialHeap(CompilerConstraintList *constraints);

    bool canPreTenure() {
        
        
        
        
        if (unknownProperties())
            return false;
        return fromAllocationSite() || newScript();
    }

    bool fromAllocationSite() {
        return flags() & OBJECT_FLAG_FROM_ALLOCATION_SITE;
    }

    void setShouldPreTenure(ExclusiveContext *cx) {
        JS_ASSERT(canPreTenure());
        setFlags(cx, OBJECT_FLAG_PRE_TENURE);
    }

    



    inline HeapTypeSet *getProperty(ExclusiveContext *cx, jsid id);

    
    inline HeapTypeSet *maybeGetProperty(jsid id);

    inline unsigned getPropertyCount();
    inline Property *getProperty(unsigned i);

    

    void updateNewPropertyTypes(ExclusiveContext *cx, jsid id, HeapTypeSet *types);
    bool addDefiniteProperties(ExclusiveContext *cx, Shape *shape);
    bool matchDefiniteProperties(HandleObject obj);
    void addPrototype(JSContext *cx, TypeObject *proto);
    void addPropertyType(ExclusiveContext *cx, jsid id, Type type);
    void addPropertyType(ExclusiveContext *cx, jsid id, const Value &value);
    void markPropertyNonData(ExclusiveContext *cx, jsid id);
    void markPropertyNonWritable(ExclusiveContext *cx, jsid id);
    void markStateChange(ExclusiveContext *cx);
    void setFlags(ExclusiveContext *cx, TypeObjectFlags flags);
    void markUnknown(ExclusiveContext *cx);
    void maybeClearNewScriptOnOOM();
    void clearNewScript(ExclusiveContext *cx);
    bool isPropertyNonData(jsid id);
    bool isPropertyNonWritable(jsid id);

    void print();

    inline void clearProperties();
    inline void sweep(FreeOp *fop, bool *oom);

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const;

    




    void finalize(FreeOp *fop) {}

    static inline ThingRootKind rootKind() { return THING_ROOT_TYPE_OBJECT; }

    static inline uint32_t offsetOfClasp() {
        return offsetof(TypeObject, clasp_);
    }

    static inline uint32_t offsetOfProto() {
        return offsetof(TypeObject, proto_);
    }

  private:
    inline uint32_t basePropertyCount() const;
    inline void setBasePropertyCount(uint32_t count);

    static void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(TypeObject, proto_) == offsetof(js::shadow::TypeObject, proto));
    }
};







struct TypeObjectWithNewScriptEntry
{
    ReadBarrieredTypeObject object;

    
    JSFunction *newFunction;

    TypeObjectWithNewScriptEntry(TypeObject *object, JSFunction *newFunction)
      : object(object), newFunction(newFunction)
    {}

    struct Lookup {
        const Class *clasp;
        TaggedProto hashProto;
        TaggedProto matchProto;
        JSFunction *newFunction;

        Lookup(const Class *clasp, TaggedProto proto, JSFunction *newFunction)
          : clasp(clasp), hashProto(proto), matchProto(proto), newFunction(newFunction)
        {}

#ifdef JSGC_GENERATIONAL
        



        Lookup(const Class *clasp, TaggedProto hashProto, TaggedProto matchProto, JSFunction *newFunction)
            : clasp(clasp), hashProto(hashProto), matchProto(matchProto), newFunction(newFunction)
        {}
#endif

    };

    static inline HashNumber hash(const Lookup &lookup);
    static inline bool match(const TypeObjectWithNewScriptEntry &key, const Lookup &lookup);
    static void rekey(TypeObjectWithNewScriptEntry &k, const TypeObjectWithNewScriptEntry& newKey) { k = newKey; }
};
typedef HashSet<TypeObjectWithNewScriptEntry,
                TypeObjectWithNewScriptEntry,
                SystemAllocPolicy> TypeObjectWithNewScriptSet;


bool
UseNewType(JSContext *cx, JSScript *script, jsbytecode *pc);

bool
UseNewTypeForClone(JSFunction *fun);





bool
ArrayPrototypeHasIndexedProperty(CompilerConstraintList *constraints, JSScript *script);


bool
TypeCanHaveExtraIndexedProperties(CompilerConstraintList *constraints, TemporaryTypeSet *types);


class TypeScript
{
    friend class ::JSScript;

    
    StackTypeSet typeArray_[1];

  public:
    
    StackTypeSet *typeArray() const {
        
        JS_STATIC_ASSERT(sizeof(TypeScript) ==
            sizeof(typeArray_) + offsetof(TypeScript, typeArray_));
        return const_cast<StackTypeSet *>(typeArray_);
    }

    static inline size_t SizeIncludingTypeArray(size_t arraySize) {
        
        JS_STATIC_ASSERT(sizeof(TypeScript) ==
            sizeof(StackTypeSet) + offsetof(TypeScript, typeArray_));
        return offsetof(TypeScript, typeArray_) + arraySize * sizeof(StackTypeSet);
    }

    static inline unsigned NumTypeSets(JSScript *script);

    static inline StackTypeSet *ThisTypes(JSScript *script);
    static inline StackTypeSet *ArgTypes(JSScript *script, unsigned i);

    
    static inline StackTypeSet *BytecodeTypes(JSScript *script, jsbytecode *pc);

    template <typename TYPESET>
    static inline TYPESET *BytecodeTypes(JSScript *script, jsbytecode *pc, uint32_t *bytecodeMap,
                                         uint32_t *hint, TYPESET *typeArray);

    
    static inline TypeObject *InitObject(JSContext *cx, JSScript *script, jsbytecode *pc,
                                         JSProtoKey kind);

    






    static inline void Monitor(JSContext *cx, JSScript *script, jsbytecode *pc,
                               const js::Value &val);
    static inline void Monitor(JSContext *cx, const js::Value &rval);

    
    static inline void MonitorAssign(JSContext *cx, HandleObject obj, jsid id);

    
    static inline void SetThis(JSContext *cx, JSScript *script, Type type);
    static inline void SetThis(JSContext *cx, JSScript *script, const js::Value &value);
    static inline void SetArgument(JSContext *cx, JSScript *script, unsigned arg, Type type);
    static inline void SetArgument(JSContext *cx, JSScript *script, unsigned arg,
                                   const js::Value &value);

    




    static bool FreezeTypeSets(CompilerConstraintList *constraints, JSScript *script,
                               TemporaryTypeSet **pThisTypes,
                               TemporaryTypeSet **pArgTypes,
                               TemporaryTypeSet **pBytecodeTypes);

    static void Purge(JSContext *cx, HandleScript script);

    static void Sweep(FreeOp *fop, JSScript *script, bool *oom);
    void destroy();

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const {
        return mallocSizeOf(this);
    }

#ifdef DEBUG
    void printTypes(JSContext *cx, HandleScript script) const;
#endif
};

void
FillBytecodeTypeMap(JSScript *script, uint32_t *bytecodeMap);

JSObject *
GetOrFixupCopyOnWriteObject(JSContext *cx, HandleScript script, jsbytecode *pc);

JSObject *
GetCopyOnWriteObject(JSScript *script, jsbytecode *pc);

class RecompileInfo;




bool
FinishCompilation(JSContext *cx, HandleScript script, ExecutionMode executionMode,
                  CompilerConstraintList *constraints, RecompileInfo *precompileInfo);



void
FinishDefinitePropertiesAnalysis(JSContext *cx, CompilerConstraintList *constraints);

struct ArrayTableKey;
typedef HashMap<ArrayTableKey,
                ReadBarrieredTypeObject,
                ArrayTableKey,
                SystemAllocPolicy> ArrayTypeTable;

struct ObjectTableKey;
struct ObjectTableEntry;
typedef HashMap<ObjectTableKey,ObjectTableEntry,ObjectTableKey,SystemAllocPolicy> ObjectTypeTable;

struct AllocationSiteKey;
typedef HashMap<AllocationSiteKey,
                ReadBarrieredTypeObject,
                AllocationSiteKey,
                SystemAllocPolicy> AllocationSiteTable;

class HeapTypeSetKey;


struct TypeObjectKey
{
    static intptr_t keyBits(TypeObjectKey *obj) { return (intptr_t) obj; }
    static TypeObjectKey *getKey(TypeObjectKey *obj) { return obj; }

    static TypeObjectKey *get(JSObject *obj) {
        JS_ASSERT(obj);
        return (TypeObjectKey *) (uintptr_t(obj) | 1);
    }
    static TypeObjectKey *get(TypeObject *obj) {
        JS_ASSERT(obj);
        return (TypeObjectKey *) obj;
    }

    bool isTypeObject() {
        return (uintptr_t(this) & 1) == 0;
    }
    bool isSingleObject() {
        return (uintptr_t(this) & 1) != 0;
    }

    inline TypeObject *asTypeObject();
    inline JSObject *asSingleObject();

    inline TypeObject *asTypeObjectNoBarrier();
    inline JSObject *asSingleObjectNoBarrier();

    const Class *clasp();
    TaggedProto proto();
    bool hasTenuredProto();
    JSObject *singleton();
    TypeNewScript *newScript();

    bool unknownProperties();
    bool hasFlags(CompilerConstraintList *constraints, TypeObjectFlags flags);
    void watchStateChangeForInlinedCall(CompilerConstraintList *constraints);
    void watchStateChangeForTypedArrayData(CompilerConstraintList *constraints);
    HeapTypeSetKey property(jsid id);
    void ensureTrackedProperty(JSContext *cx, jsid id);

    TypeObject *maybeType();
};









class HeapTypeSetKey
{
    friend struct TypeObjectKey;

    
    TypeObjectKey *object_;
    jsid id_;

    
    HeapTypeSet *maybeTypes_;

  public:
    HeapTypeSetKey()
      : object_(nullptr), id_(JSID_EMPTY), maybeTypes_(nullptr)
    {}

    TypeObjectKey *object() const { return object_; }
    jsid id() const { return id_; }
    HeapTypeSet *maybeTypes() const { return maybeTypes_; }

    bool instantiate(JSContext *cx);

    void freeze(CompilerConstraintList *constraints);
    jit::MIRType knownMIRType(CompilerConstraintList *constraints);
    bool nonData(CompilerConstraintList *constraints);
    bool nonWritable(CompilerConstraintList *constraints);
    bool isOwnProperty(CompilerConstraintList *constraints);
    bool knownSubset(CompilerConstraintList *constraints, const HeapTypeSetKey &other);
    JSObject *singleton(CompilerConstraintList *constraints);
    bool needsBarrier(CompilerConstraintList *constraints);
    bool constant(CompilerConstraintList *constraints, Value *valOut);
    bool couldBeConstant(CompilerConstraintList *constraints);
};







class CompilerOutput
{
    
    
    JSScript *script_;
    ExecutionMode mode_ : 2;

    
    bool pendingInvalidation_ : 1;

    
    
    uint32_t sweepIndex_ : 29;

  public:
    static const uint32_t INVALID_SWEEP_INDEX = (1 << 29) - 1;

    CompilerOutput()
      : script_(nullptr), mode_(SequentialExecution),
        pendingInvalidation_(false), sweepIndex_(INVALID_SWEEP_INDEX)
    {}

    CompilerOutput(JSScript *script, ExecutionMode mode)
      : script_(script), mode_(mode),
        pendingInvalidation_(false), sweepIndex_(INVALID_SWEEP_INDEX)
    {}

    JSScript *script() const { return script_; }
    inline ExecutionMode mode() const { return mode_; }

    inline jit::IonScript *ion() const;

    bool isValid() const {
        return script_ != nullptr;
    }
    void invalidate() {
        script_ = nullptr;
    }

    void setPendingInvalidation() {
        pendingInvalidation_ = true;
    }
    bool pendingInvalidation() {
        return pendingInvalidation_;
    }

    void setSweepIndex(uint32_t index) {
        if (index >= INVALID_SWEEP_INDEX)
            MOZ_CRASH();
        sweepIndex_ = index;
    }
    void invalidateSweepIndex() {
        sweepIndex_ = INVALID_SWEEP_INDEX;
    }
    uint32_t sweepIndex() {
        JS_ASSERT(sweepIndex_ != INVALID_SWEEP_INDEX);
        return sweepIndex_;
    }
};

class RecompileInfo
{
    uint32_t outputIndex;

  public:
    explicit RecompileInfo(uint32_t outputIndex = uint32_t(-1))
      : outputIndex(outputIndex)
    {}

    bool operator == (const RecompileInfo &o) const {
        return outputIndex == o.outputIndex;
    }
    CompilerOutput *compilerOutput(TypeZone &types) const;
    CompilerOutput *compilerOutput(JSContext *cx) const;
    bool shouldSweep(TypeZone &types);
};


struct TypeCompartment
{
    

    
    unsigned scriptCount;

    
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

    
    void print(JSContext *cx, bool force);

    





    TypeObject *newTypeObject(ExclusiveContext *cx, const Class *clasp, Handle<TaggedProto> proto,
                              TypeObjectFlags initialFlags = 0);

    
    TypeObject *addAllocationSiteTypeObject(JSContext *cx, AllocationSiteKey key);

    
    void markSetsUnknown(JSContext *cx, TypeObject *obj);

    void clearTables();
    void sweep(FreeOp *fop);
    void finalizeObjects();

    void addSizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf,
                                size_t *allocationSiteTables,
                                size_t *arrayTypeTables,
                                size_t *objectTypeTables);
};

void FixRestArgumentsType(ExclusiveContext *cxArg, JSObject *obj);

struct TypeZone
{
    JS::Zone                     *zone_;

    
    static const size_t TYPE_LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 8 * 1024;
    js::LifoAlloc                typeLifoAlloc;

    




    Vector<CompilerOutput> *compilerOutputs;

    
    Vector<RecompileInfo> *pendingRecompiles;

    explicit TypeZone(JS::Zone *zone);
    ~TypeZone();

    JS::Zone *zone() const { return zone_; }

    void sweep(FreeOp *fop, bool releaseTypes, bool *oom);
    void clearAllNewScriptsOnOOM();

    
    void addPendingRecompile(JSContext *cx, const RecompileInfo &info);
    void addPendingRecompile(JSContext *cx, JSScript *script);

    void processPendingRecompiles(FreeOp *fop);
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

inline const char * InferSpewColorReset() { return nullptr; }
inline const char * InferSpewColor(TypeConstraint *constraint) { return nullptr; }
inline const char * InferSpewColor(TypeSet *types) { return nullptr; }
inline void InferSpew(SpewChannel which, const char *fmt, ...) {}
inline const char * TypeString(Type type) { return nullptr; }
inline const char * TypeObjectString(TypeObject *type) { return nullptr; }

#endif


MOZ_NORETURN void TypeFailure(JSContext *cx, const char *fmt, ...);

} 
} 

#endif 
