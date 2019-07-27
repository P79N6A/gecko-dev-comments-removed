





#ifndef vm_ObjectGroup_h
#define vm_ObjectGroup_h

#include "jsbytecode.h"
#include "jsfriendapi.h"

#include "ds/IdValuePair.h"
#include "gc/Barrier.h"

namespace js {

class TypeDescr;
class UnboxedLayout;

namespace types {

class Type;
class TypeNewScript;
class HeapTypeSet;
struct Property;
class AutoClearTypeInferenceStateOnOOM;
class CompilerConstraintList;

} 




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
        MOZ_ASSERT(isObject());
        return proto;
    }
    JSObject *toObjectOrNull() const {
        MOZ_ASSERT(!proto || isObject());
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


enum : uint32_t {
    
    OBJECT_FLAG_FROM_ALLOCATION_SITE  = 0x1,

    

    
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






















class ObjectGroup : public gc::TenuredCell
{
    
    const Class *clasp_;

    
    HeapPtrObject proto_;

    




    HeapPtrObject singleton_;

  public:

    const Class *clasp() const {
        return clasp_;
    }

    void setClasp(const Class *clasp) {
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

    void setProto(TaggedProto proto);
    void setProtoUnchecked(TaggedProto proto);

    void initSingleton(JSObject *singleton) {
        singleton_ = singleton;
    }

    



    static const size_t LAZY_SINGLETON = 1;
    bool lazy() const { return singleton() == (JSObject *) LAZY_SINGLETON; }

  private:
    
    ObjectGroupFlags flags_;

    
    enum AddendumKind {
        Addendum_None,

        
        
        Addendum_InterpretedFunction,

        
        
        Addendum_NewScript,

        
        
        
        Addendum_UnboxedLayout,

        
        Addendum_TypeDescr
    };

    
    
    void *addendum_;

    void setAddendum(AddendumKind kind, void *addendum, bool writeBarrier = true);

    AddendumKind addendumKind() const {
        return (AddendumKind)
            ((flags_ & OBJECT_FLAG_ADDENDUM_MASK) >> OBJECT_FLAG_ADDENDUM_SHIFT);
    }

    types::TypeNewScript *newScriptDontCheckGeneration() const {
        if (addendumKind() == Addendum_NewScript)
            return reinterpret_cast<types::TypeNewScript *>(addendum_);
        return nullptr;
    }

    UnboxedLayout *maybeUnboxedLayoutDontCheckGeneration() const {
        if (addendumKind() == Addendum_UnboxedLayout)
            return reinterpret_cast<UnboxedLayout *>(addendum_);
        return nullptr;
    }

    types::TypeNewScript *anyNewScript();
    void detachNewScript(bool writeBarrier);

  public:

    ObjectGroupFlags flags() {
        maybeSweep(nullptr);
        return flags_;
    }

    void addFlags(ObjectGroupFlags flags) {
        maybeSweep(nullptr);
        flags_ |= flags;
    }

    void clearFlags(ObjectGroupFlags flags) {
        maybeSweep(nullptr);
        flags_ &= ~flags;
    }

    types::TypeNewScript *newScript() {
        maybeSweep(nullptr);
        return newScriptDontCheckGeneration();
    }

    void setNewScript(types::TypeNewScript *newScript) {
        setAddendum(Addendum_NewScript, newScript);
    }

    UnboxedLayout *maybeUnboxedLayout() {
        maybeSweep(nullptr);
        return maybeUnboxedLayoutDontCheckGeneration();
    }

    UnboxedLayout &unboxedLayout() {
        MOZ_ASSERT(addendumKind() == Addendum_UnboxedLayout);
        return *maybeUnboxedLayout();
    }

    void setUnboxedLayout(UnboxedLayout *layout) {
        setAddendum(Addendum_UnboxedLayout, layout);
    }

    TypeDescr *maybeTypeDescr() {
        
        
        if (addendumKind() == Addendum_TypeDescr)
            return reinterpret_cast<TypeDescr *>(addendum_);
        return nullptr;
    }

    TypeDescr &typeDescr() {
        MOZ_ASSERT(addendumKind() == Addendum_TypeDescr);
        return *maybeTypeDescr();
    }

    void setTypeDescr(TypeDescr *descr) {
        setAddendum(Addendum_TypeDescr, descr);
    }

    JSFunction *maybeInterpretedFunction() {
        
        
        if (addendumKind() == Addendum_InterpretedFunction)
            return reinterpret_cast<JSFunction *>(addendum_);
        return nullptr;
    }

    void setInterpretedFunction(JSFunction *fun) {
        setAddendum(Addendum_InterpretedFunction, fun);
    }

  private:
    
    
    
    
    
    types::Property **propertySet;
  public:

    inline ObjectGroup(const Class *clasp, TaggedProto proto, ObjectGroupFlags initialFlags);

    inline bool hasAnyFlags(ObjectGroupFlags flags) {
        MOZ_ASSERT((flags & OBJECT_FLAG_DYNAMIC_MASK) == flags);
        return !!(this->flags() & flags);
    }
    bool hasAllFlags(ObjectGroupFlags flags) {
        MOZ_ASSERT((flags & OBJECT_FLAG_DYNAMIC_MASK) == flags);
        return (this->flags() & flags) == flags;
    }

    bool unknownProperties() {
        MOZ_ASSERT_IF(flags() & OBJECT_FLAG_UNKNOWN_PROPERTIES,
                      hasAllFlags(OBJECT_FLAG_DYNAMIC_MASK));
        return !!(flags() & OBJECT_FLAG_UNKNOWN_PROPERTIES);
    }

    bool shouldPreTenure() {
        return hasAnyFlags(OBJECT_FLAG_PRE_TENURE) && !unknownProperties();
    }

    gc::InitialHeap initialHeap(types::CompilerConstraintList *constraints);

    bool canPreTenure() {
        return !unknownProperties();
    }

    bool fromAllocationSite() {
        return flags() & OBJECT_FLAG_FROM_ALLOCATION_SITE;
    }

    void setShouldPreTenure(ExclusiveContext *cx) {
        MOZ_ASSERT(canPreTenure());
        setFlags(cx, OBJECT_FLAG_PRE_TENURE);
    }

    



    inline types::HeapTypeSet *getProperty(ExclusiveContext *cx, jsid id);

    
    inline types::HeapTypeSet *maybeGetProperty(jsid id);

    inline unsigned getPropertyCount();
    inline types::Property *getProperty(unsigned i);

    

    void updateNewPropertyTypes(ExclusiveContext *cx, jsid id, types::HeapTypeSet *types);
    bool addDefiniteProperties(ExclusiveContext *cx, Shape *shape);
    bool matchDefiniteProperties(HandleObject obj);
    void markPropertyNonData(ExclusiveContext *cx, jsid id);
    void markPropertyNonWritable(ExclusiveContext *cx, jsid id);
    void markStateChange(ExclusiveContext *cx);
    void setFlags(ExclusiveContext *cx, ObjectGroupFlags flags);
    void markUnknown(ExclusiveContext *cx);
    void maybeClearNewScriptOnOOM();
    void clearNewScript(ExclusiveContext *cx);
    bool isPropertyNonData(jsid id);
    bool isPropertyNonWritable(jsid id);

    void print();

    inline void clearProperties();
    void maybeSweep(types::AutoClearTypeInferenceStateOnOOM *oom);

  private:
#ifdef DEBUG
    bool needsSweep();
#endif

    uint32_t generation() {
        return (flags_ & OBJECT_FLAG_GENERATION_MASK) >> OBJECT_FLAG_GENERATION_SHIFT;
    }

  public:
    void setGeneration(uint32_t generation) {
        MOZ_ASSERT(generation <= (OBJECT_FLAG_GENERATION_MASK >> OBJECT_FLAG_GENERATION_SHIFT));
        flags_ &= ~OBJECT_FLAG_GENERATION_MASK;
        flags_ |= generation << OBJECT_FLAG_GENERATION_SHIFT;
    }

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const;

    void finalize(FreeOp *fop);
    void fixupAfterMovingGC() {}

    static inline ThingRootKind rootKind() { return THING_ROOT_OBJECT_GROUP; }

    static inline uint32_t offsetOfClasp() {
        return offsetof(ObjectGroup, clasp_);
    }

    static inline uint32_t offsetOfProto() {
        return offsetof(ObjectGroup, proto_);
    }

    static inline uint32_t offsetOfAddendum() {
        return offsetof(ObjectGroup, addendum_);
    }

    static inline uint32_t offsetOfFlags() {
        return offsetof(ObjectGroup, flags_);
    }

  private:
    inline uint32_t basePropertyCount();
    inline void setBasePropertyCount(uint32_t count);

    static void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(ObjectGroup, proto_) == offsetof(js::shadow::ObjectGroup, proto));
    }

  public:
    
    static bool useSingletonForClone(JSFunction *fun);

    
    static bool useSingletonForNewObject(JSContext *cx, JSScript *script, jsbytecode *pc);

    
    static bool useSingletonForAllocationSite(JSScript *script, jsbytecode *pc,
                                              JSProtoKey key);
    static bool useSingletonForAllocationSite(JSScript *script, jsbytecode *pc,
                                              const Class *clasp);

    

    static ObjectGroup *defaultNewGroup(ExclusiveContext *cx, const Class *clasp,
                                        TaggedProto proto,
                                        JSObject *associated = nullptr);
    static ObjectGroup *lazySingletonGroup(ExclusiveContext *cx, const Class *clasp,
                                           TaggedProto proto);

    static void setDefaultNewGroupUnknown(JSContext *cx, const js::Class *clasp, JS::HandleObject obj);

#ifdef DEBUG
    static bool hasDefaultNewGroup(JSObject *proto, const Class *clasp, ObjectGroup *group);
#endif

    

    
    
    static void fixArrayGroup(ExclusiveContext *cx, ArrayObject *obj);
    static void fixPlainObjectGroup(ExclusiveContext *cx, PlainObject *obj);

    
    static void fixRestArgumentsGroup(ExclusiveContext *cx, ArrayObject *obj);

    static PlainObject *newPlainObject(JSContext *cx, IdValuePair *properties, size_t nproperties);

    

    
    
    static ObjectGroup *allocationSiteGroup(JSContext *cx, JSScript *script, jsbytecode *pc,
                                            JSProtoKey key);

    
    static ObjectGroup *callingAllocationSiteGroup(JSContext *cx, JSProtoKey key);

    
    static bool
    setAllocationSiteObjectGroup(JSContext *cx, HandleScript script, jsbytecode *pc,
                                 HandleObject obj, bool singleton);

    static ArrayObject *getOrFixupCopyOnWriteObject(JSContext *cx, HandleScript script,
                                                    jsbytecode *pc);
    static ArrayObject *getCopyOnWriteObject(JSScript *script, jsbytecode *pc);

    
    static bool findAllocationSiteForType(JSContext *cx, types::Type type,
                                          JSScript **script, uint32_t *offset);

  private:
    static ObjectGroup *defaultNewGroup(JSContext *cx, JSProtoKey key);
    static void setGroupToHomogenousArray(ExclusiveContext *cx, JSObject *obj, types::Type type);
};


class ObjectGroupCompartment
{
    friend class ObjectGroup;

    struct NewEntry;
    typedef HashSet<NewEntry, NewEntry, SystemAllocPolicy> NewTable;
    class NewTableRef;

    
    NewTable *defaultNewTable;
    NewTable *lazyTable;

    struct ArrayObjectKey;
    typedef HashMap<ArrayObjectKey,
                    ReadBarrieredObjectGroup,
                    ArrayObjectKey,
                    SystemAllocPolicy> ArrayObjectTable;

    struct PlainObjectKey;
    struct PlainObjectEntry;
    typedef HashMap<PlainObjectKey,
                    PlainObjectEntry,
                    PlainObjectKey,
                    SystemAllocPolicy> PlainObjectTable;

    
    
    
    
    
    
    
    
    
    ArrayObjectTable *arrayObjectTable;
    PlainObjectTable *plainObjectTable;

    struct AllocationSiteKey;
    typedef HashMap<AllocationSiteKey,
                    ReadBarrieredObjectGroup,
                    AllocationSiteKey,
                    SystemAllocPolicy> AllocationSiteTable;

    
    AllocationSiteTable *allocationSiteTable;

  public:
    ObjectGroupCompartment();
    ~ObjectGroupCompartment();

    void removeDefaultNewGroup(const Class *clasp, TaggedProto proto, JSObject *associated);
    void replaceDefaultNewGroup(const Class *clasp, TaggedProto proto, JSObject *associated,
                                ObjectGroup *group);

    static ObjectGroup *makeGroup(ExclusiveContext *cx, const Class *clasp,
                                  Handle<TaggedProto> proto,
                                  ObjectGroupFlags initialFlags = 0);

    void addSizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf,
                                size_t *allocationSiteTables,
                                size_t *arrayGroupTables,
                                size_t *plainObjectGroupTables,
                                size_t *compartmentTables);

    void clearTables();

    void sweep(FreeOp *fop);

#ifdef JSGC_HASH_TABLE_CHECKS
    void checkTablesAfterMovingGC() {
        checkNewTableAfterMovingGC(defaultNewTable);
        checkNewTableAfterMovingGC(lazyTable);
    }
#endif

    void fixupTablesAfterMovingGC() {
        fixupNewTableAfterMovingGC(defaultNewTable);
        fixupNewTableAfterMovingGC(lazyTable);
    }

  private:
#ifdef JSGC_HASH_TABLE_CHECKS
    void checkNewTableAfterMovingGC(NewTable *table);
#endif

    void sweepNewTable(NewTable *table);
    void fixupNewTableAfterMovingGC(NewTable *table);

    static void newTablePostBarrier(ExclusiveContext *cx, NewTable *table,
                                    const Class *clasp, TaggedProto proto, JSObject *associated);
    static void updatePlainObjectEntryTypes(ExclusiveContext *cx, PlainObjectEntry &entry,
                                            IdValuePair *properties, size_t nproperties);
};

} 

#endif 
