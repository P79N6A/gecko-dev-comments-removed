







#ifndef vm_TypeInference_h
#define vm_TypeInference_h

#include "mozilla/MemoryReporting.h"

#include "jsalloc.h"
#include "jsfriendapi.h"
#include "jstypes.h"

#include "ds/IdValuePair.h"
#include "ds/LifoAlloc.h"
#include "gc/Barrier.h"
#include "gc/Marking.h"
#include "jit/IonTypes.h"
#include "js/UbiNode.h"
#include "js/Utility.h"
#include "js/Vector.h"

namespace js {

namespace jit {
    struct IonScript;
    class JitAllocPolicy;
    class TempAllocator;
}

class TaggedProto;
struct TypeZone;
class TypeConstraint;
class TypeNewScript;
class CompilerConstraintList;
class HeapTypeSetKey;














enum : uint32_t {
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

    
    TYPE_FLAG_OBJECT_COUNT_MASK     = 0x3e00,
    TYPE_FLAG_OBJECT_COUNT_SHIFT    = 9,
    TYPE_FLAG_OBJECT_COUNT_LIMIT    = 7,
    TYPE_FLAG_DOMOBJECT_COUNT_LIMIT =
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


enum : uint32_t {
    
    OBJECT_FLAG_FROM_ALLOCATION_SITE  = 0x1,

    
    OBJECT_FLAG_SINGLETON             = 0x2,

    



    OBJECT_FLAG_LAZY_SINGLETON        = 0x4,

    
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

    



    OBJECT_FLAG_TYPED_OBJECT_NEUTERED = 0x00400000,

    



    OBJECT_FLAG_PRE_TENURE            = 0x00800000,

    
    OBJECT_FLAG_COPY_ON_WRITE         = 0x01000000,

    
    OBJECT_FLAG_NEW_SCRIPT_CLEARED    = 0x02000000,

    



    OBJECT_FLAG_UNKNOWN_PROPERTIES    = 0x04000000,

    
    OBJECT_FLAG_DYNAMIC_MASK          = 0x07ff0000,

    
    OBJECT_FLAG_ADDENDUM_MASK         = 0x38000000,
    OBJECT_FLAG_ADDENDUM_SHIFT        = 27,

    
    
    OBJECT_FLAG_GENERATION_MASK       = 0x40000000,
    OBJECT_FLAG_GENERATION_SHIFT      = 30,
};
typedef uint32_t ObjectGroupFlags;

class StackTypeSet;
class HeapTypeSet;
class TemporaryTypeSet;
































class TypeSet
{
  public:
    
    
    class ObjectKey {
      public:
        static intptr_t keyBits(ObjectKey* obj) { return (intptr_t) obj; }
        static ObjectKey* getKey(ObjectKey* obj) { return obj; }

        static inline ObjectKey* get(JSObject* obj);
        static inline ObjectKey* get(ObjectGroup* group);

        bool isGroup() {
            return (uintptr_t(this) & 1) == 0;
        }
        bool isSingleton() {
            return (uintptr_t(this) & 1) != 0;
        }

        inline ObjectGroup* group();
        inline JSObject* singleton();

        inline ObjectGroup* groupNoBarrier();
        inline JSObject* singletonNoBarrier();

        const Class* clasp();
        TaggedProto proto();
        TypeNewScript* newScript();

        bool unknownProperties();
        bool hasFlags(CompilerConstraintList* constraints, ObjectGroupFlags flags);
        bool hasStableClassAndProto(CompilerConstraintList* constraints);
        void watchStateChangeForInlinedCall(CompilerConstraintList* constraints);
        void watchStateChangeForTypedArrayData(CompilerConstraintList* constraints);
        void watchStateChangeForUnboxedConvertedToNative(CompilerConstraintList* constraints);
        HeapTypeSetKey property(jsid id);
        void ensureTrackedProperty(JSContext* cx, jsid id);

        ObjectGroup* maybeGroup();
    };

    
    
    
    class Type
    {
        friend class TypeSet;

        uintptr_t data;
        explicit Type(uintptr_t data) : data(data) {}

      public:

        uintptr_t raw() const { return data; }

        bool isPrimitive() const {
            return data < JSVAL_TYPE_OBJECT;
        }

        bool isPrimitive(JSValueType type) const {
            MOZ_ASSERT(type < JSVAL_TYPE_OBJECT);
            return (uintptr_t) type == data;
        }

        JSValueType primitive() const {
            MOZ_ASSERT(isPrimitive());
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
            MOZ_ASSERT(!isAnyObject() && !isUnknown());
            return data > JSVAL_TYPE_UNKNOWN;
        }

        bool isObjectUnchecked() const {
            return data > JSVAL_TYPE_UNKNOWN;
        }

        inline ObjectKey* objectKey() const;

        

        bool isSingleton() const {
            return isObject() && !!(data & 1);
        }
        bool isSingletonUnchecked() const {
            return isObjectUnchecked() && !!(data & 1);
        }

        inline JSObject* singleton() const;
        inline JSObject* singletonNoBarrier() const;

        

        bool isGroup() const {
            return isObject() && !(data & 1);
        }
        bool isGroupUnchecked() const {
            return isObjectUnchecked() && !(data & 1);
        }

        inline ObjectGroup* group() const;
        inline ObjectGroup* groupNoBarrier() const;

        bool operator == (Type o) const { return data == o.data; }
        bool operator != (Type o) const { return data != o.data; }

        static ThingRootKind rootKind() { return THING_ROOT_TYPE; }
    };

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
        MOZ_ASSERT(type < JSVAL_TYPE_UNKNOWN);
        return Type(type);
    }

    static inline Type ObjectType(JSObject* obj);
    static inline Type ObjectType(ObjectGroup* group);
    static inline Type ObjectType(ObjectKey* key);

    static const char* NonObjectTypeString(Type type);

#ifdef DEBUG
    static const char* TypeString(Type type);
    static const char* ObjectGroupString(ObjectGroup* group);
#else
    static const char* TypeString(Type type) { return nullptr; }
    static const char* ObjectGroupString(ObjectGroup* group) { return nullptr; }
#endif

  protected:
    
    TypeFlags flags;

    
    ObjectKey** objectSet;

  public:

    TypeSet()
      : flags(0), objectSet(nullptr)
    {}

    void print(FILE* fp = stderr);

    
    inline bool hasType(Type type) const;

    TypeFlags baseFlags() const { return flags & TYPE_FLAG_BASE_MASK; }
    bool unknown() const { return !!(flags & TYPE_FLAG_UNKNOWN); }
    bool unknownObject() const { return !!(flags & (TYPE_FLAG_UNKNOWN | TYPE_FLAG_ANYOBJECT)); }
    bool empty() const { return !baseFlags() && !baseObjectCount(); }

    bool hasAnyFlag(TypeFlags flags) const {
        MOZ_ASSERT((flags & TYPE_FLAG_BASE_MASK) == flags);
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
        MOZ_ASSERT(definiteProperty());
        return (flags >> TYPE_FLAG_DEFINITE_SHIFT) - 1;
    }

    
    static TemporaryTypeSet* unionSets(TypeSet* a, TypeSet* b, LifoAlloc* alloc);
    
    static TemporaryTypeSet* intersectSets(TemporaryTypeSet* a, TemporaryTypeSet* b, LifoAlloc* alloc);
    




    static TemporaryTypeSet* removeSet(TemporaryTypeSet* a, TemporaryTypeSet* b, LifoAlloc* alloc);

    
    void addType(Type type, LifoAlloc* alloc);

    
    typedef Vector<Type, 1, SystemAllocPolicy> TypeList;
    template <class TypeListT> bool enumerateTypes(TypeListT* list) const;

    




    inline unsigned getObjectCount() const;
    inline ObjectKey* getObject(unsigned i) const;
    inline JSObject* getSingleton(unsigned i) const;
    inline ObjectGroup* getGroup(unsigned i) const;
    inline JSObject* getSingletonNoBarrier(unsigned i) const;
    inline ObjectGroup* getGroupNoBarrier(unsigned i) const;

    
    inline const Class* getObjectClass(unsigned i) const;

    bool canSetDefinite(unsigned slot) {
        
        return (slot + 1) <= (unsigned(TYPE_FLAG_DEFINITE_MASK) >> TYPE_FLAG_DEFINITE_SHIFT);
    }
    void setDefinite(unsigned slot) {
        MOZ_ASSERT(canSetDefinite(slot));
        flags |= ((slot + 1) << TYPE_FLAG_DEFINITE_SHIFT);
        MOZ_ASSERT(definiteSlot() == slot);
    }

    
    bool mightBeMIRType(jit::MIRType type);

    



    bool isSubset(const TypeSet* other) const;

    



    bool objectsAreSubset(TypeSet* other);

    
    bool equals(const TypeSet* other) const {
        return this->isSubset(other) && other->isSubset(this);
    }

    bool objectsIntersect(const TypeSet* other) const;

    
    bool addTypesToConstraint(JSContext* cx, TypeConstraint* constraint);

    
    TemporaryTypeSet* clone(LifoAlloc* alloc) const;
    bool clone(LifoAlloc* alloc, TemporaryTypeSet* result) const;

    
    TemporaryTypeSet* filter(LifoAlloc* alloc, bool filterUndefined, bool filterNull) const;
    
    TemporaryTypeSet* cloneObjectsOnly(LifoAlloc* alloc);
    TemporaryTypeSet* cloneWithoutObjects(LifoAlloc* alloc);

    
    static void readBarrier(const TypeSet* types);

  protected:
    uint32_t baseObjectCount() const {
        return (flags & TYPE_FLAG_OBJECT_COUNT_MASK) >> TYPE_FLAG_OBJECT_COUNT_SHIFT;
    }
    inline void setBaseObjectCount(uint32_t count);

    void clearObjects();

  public:
    static inline Type GetValueType(const Value& val);

    static inline bool IsUntrackedValue(const Value& val);

    
    
    
    static inline Type GetMaybeUntrackedValueType(const Value& val);

    static void MarkTypeRoot(JSTracer* trc, Type* v, const char* name);
    static void MarkTypeUnbarriered(JSTracer* trc, Type* v, const char* name);
    static bool IsTypeMarked(Type* v);
    static bool IsTypeAllocatedDuringIncremental(Type v);
    static bool IsTypeAboutToBeFinalized(Type* v);
};





class TypeConstraint
{
public:
    
    TypeConstraint* next;

    TypeConstraint()
        : next(nullptr)
    {}

    
    virtual const char* kind() = 0;

    
    virtual void newType(JSContext* cx, TypeSet* source, TypeSet::Type type) = 0;

    



    virtual void newPropertyState(JSContext* cx, TypeSet* source) {}

    




    virtual void newObjectState(JSContext* cx, ObjectGroup* group) {}

    



    virtual bool sweep(TypeZone& zone, TypeConstraint** res) = 0;
};







class AutoClearTypeInferenceStateOnOOM
{
    Zone* zone;
    bool oom;

  public:
    explicit AutoClearTypeInferenceStateOnOOM(Zone* zone)
      : zone(zone), oom(false)
    {}

    ~AutoClearTypeInferenceStateOnOOM();

    void setOOM() {
        oom = true;
    }
};


class ConstraintTypeSet : public TypeSet
{
  public:
    
    TypeConstraint* constraintList;

    ConstraintTypeSet() : constraintList(nullptr) {}

    



    void addType(ExclusiveContext* cx, Type type);

    
    bool addConstraint(JSContext* cx, TypeConstraint* constraint, bool callExisting = true);

    inline void sweep(JS::Zone* zone, AutoClearTypeInferenceStateOnOOM& oom);
};

class StackTypeSet : public ConstraintTypeSet
{
  public:
};

class HeapTypeSet : public ConstraintTypeSet
{
    inline void newPropertyState(ExclusiveContext* cx);

  public:
    
    inline void setNonDataProperty(ExclusiveContext* cx);

    
    inline void setNonWritableProperty(ExclusiveContext* cx);

    
    inline void setNonConstantProperty(ExclusiveContext* cx);
};

CompilerConstraintList*
NewCompilerConstraintList(jit::TempAllocator& alloc);

class TemporaryTypeSet : public TypeSet
{
  public:
    TemporaryTypeSet() {}
    TemporaryTypeSet(LifoAlloc* alloc, Type type);

    TemporaryTypeSet(uint32_t flags, ObjectKey** objectSet) {
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

    
    bool hasObjectFlags(CompilerConstraintList* constraints, ObjectGroupFlags flags);

    
    const Class* getKnownClass(CompilerConstraintList* constraints);

    
    enum ForAllResult {
        EMPTY=1,                
        ALL_TRUE,               
        ALL_FALSE,              
        MIXED,                  
                                
                                
    };

    


    ForAllResult forAllClasses(CompilerConstraintList* constraints,
                               bool (*func)(const Class* clasp));

    



    bool getCommonPrototype(CompilerConstraintList* constraints, JSObject** proto);

    
    Scalar::Type getTypedArrayType(CompilerConstraintList* constraints);

    
    Scalar::Type getSharedTypedArrayType(CompilerConstraintList* constraints);

    
    bool isDOMClass(CompilerConstraintList* constraints);

    
    bool maybeCallable(CompilerConstraintList* constraints);

    
    bool maybeEmulatesUndefined(CompilerConstraintList* constraints);

    
    JSObject* maybeSingleton();

    
    bool propertyNeedsBarrier(CompilerConstraintList* constraints, jsid id);

    



    bool filtersType(const TemporaryTypeSet* other, Type type) const;

    enum DoubleConversion {
        
        AlwaysConvertToDoubles,

        
        MaybeConvertToDoubles,

        
        DontConvertToDoubles,

        
        AmbiguousDoubleConversion
    };

    



    DoubleConversion convertDoubleElements(CompilerConstraintList* constraints);
};

bool
AddClearDefiniteGetterSetterForPrototypeChain(JSContext* cx, ObjectGroup* group, HandleId id);

bool
AddClearDefiniteFunctionUsesInScript(JSContext* cx, ObjectGroup* group,
                                     JSScript* script, JSScript* calleeScript);






class PreliminaryObjectArray
{
  public:
    static const uint32_t COUNT = 20;

  private:
    
    
    JSObject* objects[COUNT];

  public:
    PreliminaryObjectArray() {
        mozilla::PodZero(this);
    }

    void registerNewObject(JSObject* res);

    JSObject* get(size_t i) const {
        MOZ_ASSERT(i < COUNT);
        return objects[i];
    }

    bool full() const;
    void sweep();
};

class PreliminaryObjectArrayWithTemplate : public PreliminaryObjectArray
{
    HeapPtrShape shape_;

  public:
    explicit PreliminaryObjectArrayWithTemplate(Shape* shape)
      : shape_(shape)
    {}

    Shape* shape() {
        return shape_;
    }

    void maybeAnalyze(ExclusiveContext* cx, ObjectGroup* group, bool force = false);

    void trace(JSTracer* trc);

    static void writeBarrierPre(PreliminaryObjectArrayWithTemplate* preliminaryObjects);
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
    
    HeapPtrFunction function_;

    
    
    PreliminaryObjectArray* preliminaryObjects;

    
    
    
    
    
    
    HeapPtrPlainObject templateObject_;

    
    
    
    
    
    
    
    
    Initializer* initializerList;

    
    
    
    
    
    HeapPtrShape initializedShape_;

    
    
    HeapPtrObjectGroup initializedGroup_;

  public:
    TypeNewScript() { mozilla::PodZero(this); }
    ~TypeNewScript() {
        js_delete(preliminaryObjects);
        js_free(initializerList);
    }

    static void writeBarrierPre(TypeNewScript* newScript);

    bool analyzed() const {
        return preliminaryObjects == nullptr;
    }

    PlainObject* templateObject() const {
        return templateObject_;
    }

    Shape* initializedShape() const {
        return initializedShape_;
    }

    ObjectGroup* initializedGroup() const {
        return initializedGroup_;
    }

    JSFunction* function() const {
        return function_;
    }

    void trace(JSTracer* trc);
    void sweep();

    void registerNewObject(PlainObject* res);
    bool maybeAnalyze(JSContext* cx, ObjectGroup* group, bool* regenerate, bool force = false);

    bool rollbackPartiallyInitializedObjects(JSContext* cx, ObjectGroup* group);

    static void make(JSContext* cx, ObjectGroup* group, JSFunction* fun);
    static TypeNewScript* makeNativeVersion(JSContext* cx, TypeNewScript* newScript,
                                            PlainObject* templateObject);

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const;
};


inline bool isInlinableCall(jsbytecode* pc);

bool
ClassCanHaveExtraProperties(const Class* clasp);





bool
ArrayPrototypeHasIndexedProperty(CompilerConstraintList* constraints, JSScript* script);


bool
TypeCanHaveExtraIndexedProperties(CompilerConstraintList* constraints, TemporaryTypeSet* types);


class TypeScript
{
    friend class ::JSScript;

    
    StackTypeSet typeArray_[1];

  public:
    
    StackTypeSet* typeArray() const {
        
        JS_STATIC_ASSERT(sizeof(TypeScript) ==
                         sizeof(typeArray_) + offsetof(TypeScript, typeArray_));
        return const_cast<StackTypeSet*>(typeArray_);
    }

    static inline size_t SizeIncludingTypeArray(size_t arraySize) {
        
        JS_STATIC_ASSERT(sizeof(TypeScript) ==
            sizeof(StackTypeSet) + offsetof(TypeScript, typeArray_));
        return offsetof(TypeScript, typeArray_) + arraySize * sizeof(StackTypeSet);
    }

    static inline unsigned NumTypeSets(JSScript* script);

    static inline StackTypeSet* ThisTypes(JSScript* script);
    static inline StackTypeSet* ArgTypes(JSScript* script, unsigned i);

    
    static inline StackTypeSet* BytecodeTypes(JSScript* script, jsbytecode* pc);

    template <typename TYPESET>
    static inline TYPESET* BytecodeTypes(JSScript* script, jsbytecode* pc, uint32_t* bytecodeMap,
                                         uint32_t* hint, TYPESET* typeArray);

    






    static inline void Monitor(JSContext* cx, JSScript* script, jsbytecode* pc,
                               const js::Value& val);
    static inline void Monitor(JSContext* cx, const js::Value& rval);

    
    static inline void MonitorAssign(JSContext* cx, HandleObject obj, jsid id);

    
    static inline void SetThis(JSContext* cx, JSScript* script, TypeSet::Type type);
    static inline void SetThis(JSContext* cx, JSScript* script, const js::Value& value);
    static inline void SetArgument(JSContext* cx, JSScript* script, unsigned arg,
                                   TypeSet::Type type);
    static inline void SetArgument(JSContext* cx, JSScript* script, unsigned arg,
                                   const js::Value& value);

    




    static bool FreezeTypeSets(CompilerConstraintList* constraints, JSScript* script,
                               TemporaryTypeSet** pThisTypes,
                               TemporaryTypeSet** pArgTypes,
                               TemporaryTypeSet** pBytecodeTypes);

    static void Purge(JSContext* cx, HandleScript script);

    void destroy();

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const {
        return mallocSizeOf(this);
    }

#ifdef DEBUG
    void printTypes(JSContext* cx, HandleScript script) const;
#endif
};

void
FillBytecodeTypeMap(JSScript* script, uint32_t* bytecodeMap);

class RecompileInfo;




bool
FinishCompilation(JSContext* cx, HandleScript script, CompilerConstraintList* constraints,
                  RecompileInfo* precompileInfo);



void
FinishDefinitePropertiesAnalysis(JSContext* cx, CompilerConstraintList* constraints);









class HeapTypeSetKey
{
    friend class TypeSet::ObjectKey;

    
    TypeSet::ObjectKey* object_;
    jsid id_;

    
    HeapTypeSet* maybeTypes_;

  public:
    HeapTypeSetKey()
      : object_(nullptr), id_(JSID_EMPTY), maybeTypes_(nullptr)
    {}

    TypeSet::ObjectKey* object() const { return object_; }
    jsid id() const { return id_; }
    HeapTypeSet* maybeTypes() const { return maybeTypes_; }

    bool instantiate(JSContext* cx);

    void freeze(CompilerConstraintList* constraints);
    jit::MIRType knownMIRType(CompilerConstraintList* constraints);
    bool nonData(CompilerConstraintList* constraints);
    bool nonWritable(CompilerConstraintList* constraints);
    bool isOwnProperty(CompilerConstraintList* constraints, bool allowEmptyTypesForGlobal = false);
    bool knownSubset(CompilerConstraintList* constraints, const HeapTypeSetKey& other);
    JSObject* singleton(CompilerConstraintList* constraints);
    bool needsBarrier(CompilerConstraintList* constraints);
    bool constant(CompilerConstraintList* constraints, Value* valOut);
    bool couldBeConstant(CompilerConstraintList* constraints);
};







class CompilerOutput
{
    
    
    JSScript* script_;

    
    bool pendingInvalidation_ : 1;

    
    
    uint32_t sweepIndex_ : 31;

  public:
    static const uint32_t INVALID_SWEEP_INDEX = static_cast<uint32_t>(1 << 31) - 1;

    CompilerOutput()
      : script_(nullptr),
        pendingInvalidation_(false), sweepIndex_(INVALID_SWEEP_INDEX)
    {}

    explicit CompilerOutput(JSScript* script)
      : script_(script),
        pendingInvalidation_(false), sweepIndex_(INVALID_SWEEP_INDEX)
    {}

    JSScript* script() const { return script_; }

    inline jit::IonScript* ion() const;

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
    uint32_t sweepIndex() {
        MOZ_ASSERT(sweepIndex_ != INVALID_SWEEP_INDEX);
        return sweepIndex_;
    }
};

class RecompileInfo
{
    
    
    uint32_t outputIndex : 31;

    
    
    uint32_t generation : 1;

  public:
    RecompileInfo(uint32_t outputIndex, uint32_t generation)
      : outputIndex(outputIndex), generation(generation)
    {}

    RecompileInfo()
      : outputIndex(JS_BITMASK(31)), generation(0)
    {}

    CompilerOutput* compilerOutput(TypeZone& types) const;
    CompilerOutput* compilerOutput(JSContext* cx) const;
    bool shouldSweep(TypeZone& types);
};

typedef Vector<RecompileInfo, 0, SystemAllocPolicy> RecompileInfoVector;

struct AutoEnterAnalysis;

struct TypeZone
{
    JS::Zone* zone_;

    
    static const size_t TYPE_LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 8 * 1024;
    LifoAlloc typeLifoAlloc;

    
    uint32_t generation : 1;

    




    typedef Vector<CompilerOutput, 4, SystemAllocPolicy> CompilerOutputVector;
    CompilerOutputVector* compilerOutputs;

    
    
    LifoAlloc sweepTypeLifoAlloc;

    
    
    CompilerOutputVector* sweepCompilerOutputs;

    
    
    bool sweepReleaseTypes;

    
    AutoEnterAnalysis* activeAnalysis;

    explicit TypeZone(JS::Zone* zone);
    ~TypeZone();

    JS::Zone* zone() const { return zone_; }

    void beginSweep(FreeOp* fop, bool releaseTypes, AutoClearTypeInferenceStateOnOOM& oom);
    void endSweep(JSRuntime* rt);
    void clearAllNewScriptsOnOOM();

    
    void addPendingRecompile(JSContext* cx, const RecompileInfo& info);
    void addPendingRecompile(JSContext* cx, JSScript* script);

    void processPendingRecompiles(FreeOp* fop, RecompileInfoVector& recompiles);
};

enum SpewChannel {
    ISpewOps,      
    ISpewResult,   
    SPEW_COUNT
};

#ifdef DEBUG

const char * InferSpewColorReset();
const char * InferSpewColor(TypeConstraint* constraint);
const char * InferSpewColor(TypeSet* types);

void InferSpew(SpewChannel which, const char* fmt, ...);


bool ObjectGroupHasProperty(JSContext* cx, ObjectGroup* group, jsid id, const Value& value);

#else

inline const char * InferSpewColorReset() { return nullptr; }
inline const char * InferSpewColor(TypeConstraint* constraint) { return nullptr; }
inline const char * InferSpewColor(TypeSet* types) { return nullptr; }
inline void InferSpew(SpewChannel which, const char* fmt, ...) {}

#endif


MOZ_NORETURN MOZ_COLD void TypeFailure(JSContext* cx, const char* fmt, ...);


void
PrintTypes(JSContext* cx, JSCompartment* comp, bool force);

} 



namespace JS {
namespace ubi {
template<> struct Concrete<js::ObjectGroup> : TracerConcrete<js::ObjectGroup> { };
}
}

#endif 
