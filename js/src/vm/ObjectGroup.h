





#ifndef vm_ObjectGroup_h
#define vm_ObjectGroup_h

#include "jsbytecode.h"
#include "jsfriendapi.h"

#include "ds/IdValuePair.h"
#include "gc/Barrier.h"
#include "vm/TypeInference.h"

namespace js {

class TypeDescr;
class UnboxedLayout;

class PreliminaryObjectArrayWithTemplate;
class TypeNewScript;
class HeapTypeSet;
class AutoClearTypeInferenceStateOnOOM;
class CompilerConstraintList;




class TaggedProto
{
  public:
    static JSObject * const LazyProto;

    TaggedProto() : proto(nullptr) {}
    explicit TaggedProto(JSObject* proto) : proto(proto) {}

    uintptr_t toWord() const { return uintptr_t(proto); }

    bool isLazy() const {
        return proto == LazyProto;
    }
    bool isObject() const {
        
        return uintptr_t(proto) > uintptr_t(TaggedProto::LazyProto);
    }
    JSObject* toObject() const {
        MOZ_ASSERT(isObject());
        return proto;
    }
    JSObject* toObjectOrNull() const {
        MOZ_ASSERT(!proto || isObject());
        return proto;
    }
    JSObject* raw() const { return proto; }

    bool operator ==(const TaggedProto& other) { return proto == other.proto; }
    bool operator !=(const TaggedProto& other) { return proto != other.proto; }

  private:
    JSObject* proto;
};

template <>
struct RootKind<TaggedProto>
{
    static ThingRootKind rootKind() { return THING_ROOT_OBJECT; }
};

template <> struct GCMethods<const TaggedProto>
{
    static TaggedProto initial() { return TaggedProto(); }
};

template <> struct GCMethods<TaggedProto>
{
    static TaggedProto initial() { return TaggedProto(); }
};

template<class Outer>
class TaggedProtoOperations
{
    const TaggedProto* value() const {
        return static_cast<const Outer*>(this)->extract();
    }

  public:
    uintptr_t toWord() const { return value()->toWord(); }
    inline bool isLazy() const { return value()->isLazy(); }
    inline bool isObject() const { return value()->isObject(); }
    inline JSObject* toObject() const { return value()->toObject(); }
    inline JSObject* toObjectOrNull() const { return value()->toObjectOrNull(); }
    JSObject* raw() const { return value()->raw(); }
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
    const TaggedProto* extract() const {
        return static_cast<const Rooted<TaggedProto>*>(this)->address();
    }
};





inline Handle<TaggedProto>
AsTaggedProto(HandleObject obj)
{
    static_assert(sizeof(JSObject*) == sizeof(TaggedProto),
                  "TaggedProto must be binary compatible with JSObject");
    return Handle<TaggedProto>::fromMarkedLocation(
            reinterpret_cast<TaggedProto const*>(obj.address()));
}

namespace gc {
void MergeCompartments(JSCompartment* source, JSCompartment* target);
}





enum NewObjectKind {
    
    GenericObject,

    




    SingletonObject,

    




    TenuredObject
};






















class ObjectGroup : public gc::TenuredCell
{
    friend void gc::MergeCompartments(JSCompartment* source, JSCompartment* target);

    
    const Class* clasp_;

    
    HeapPtrObject proto_;

    
    JSCompartment* compartment_;

  public:

    const Class* clasp() const {
        return clasp_;
    }

    void setClasp(const Class* clasp) {
        clasp_ = clasp;
    }

    TaggedProto proto() const {
        return TaggedProto(proto_);
    }

    
    HeapPtrObject& protoRaw() { return proto_; }

    void setProto(TaggedProto proto);
    void setProtoUnchecked(TaggedProto proto);

    bool singleton() const {
        return flagsDontCheckGeneration() & OBJECT_FLAG_SINGLETON;
    }

    bool lazy() const {
        bool res = flagsDontCheckGeneration() & OBJECT_FLAG_LAZY_SINGLETON;
        MOZ_ASSERT_IF(res, singleton());
        return res;
    }

    JSCompartment* compartment() const {
        return compartment_;
    }

  private:
    
    ObjectGroupFlags flags_;

    
    enum AddendumKind {
        Addendum_None,

        
        
        Addendum_InterpretedFunction,

        
        
        Addendum_NewScript,

        
        Addendum_PreliminaryObjects,

        
        
        
        Addendum_UnboxedLayout,

        
        
        
        Addendum_OriginalUnboxedGroup,

        
        Addendum_TypeDescr
    };

    
    
    void* addendum_;

    void setAddendum(AddendumKind kind, void* addendum, bool writeBarrier = true);

    AddendumKind addendumKind() const {
        return (AddendumKind)
            ((flags_ & OBJECT_FLAG_ADDENDUM_MASK) >> OBJECT_FLAG_ADDENDUM_SHIFT);
    }

    TypeNewScript* newScriptDontCheckGeneration() const {
        if (addendumKind() == Addendum_NewScript)
            return reinterpret_cast<TypeNewScript*>(addendum_);
        return nullptr;
    }

    UnboxedLayout* maybeUnboxedLayoutDontCheckGeneration() const {
        if (addendumKind() == Addendum_UnboxedLayout)
            return reinterpret_cast<UnboxedLayout*>(addendum_);
        return nullptr;
    }

    TypeNewScript* anyNewScript();
    void detachNewScript(bool writeBarrier, ObjectGroup* replacement);

    ObjectGroupFlags flagsDontCheckGeneration() const {
        return flags_;
    }

  public:

    ObjectGroupFlags flags() {
        maybeSweep(nullptr);
        return flagsDontCheckGeneration();
    }

    void addFlags(ObjectGroupFlags flags) {
        maybeSweep(nullptr);
        flags_ |= flags;
    }

    void clearFlags(ObjectGroupFlags flags) {
        maybeSweep(nullptr);
        flags_ &= ~flags;
    }

    TypeNewScript* newScript() {
        maybeSweep(nullptr);
        return newScriptDontCheckGeneration();
    }

    void setNewScript(TypeNewScript* newScript) {
        setAddendum(Addendum_NewScript, newScript);
    }

    PreliminaryObjectArrayWithTemplate* maybePreliminaryObjects() {
        maybeSweep(nullptr);
        return maybePreliminaryObjectsDontCheckGeneration();
    }

    PreliminaryObjectArrayWithTemplate* maybePreliminaryObjectsDontCheckGeneration() {
        if (addendumKind() == Addendum_PreliminaryObjects)
            return reinterpret_cast<PreliminaryObjectArrayWithTemplate*>(addendum_);
        return nullptr;
    }

    void setPreliminaryObjects(PreliminaryObjectArrayWithTemplate* preliminaryObjects) {
        setAddendum(Addendum_PreliminaryObjects, preliminaryObjects);
    }

    void detachPreliminaryObjects() {
        MOZ_ASSERT(maybePreliminaryObjects());
        setAddendum(Addendum_None, nullptr);
    }

    UnboxedLayout* maybeUnboxedLayout() {
        maybeSweep(nullptr);
        return maybeUnboxedLayoutDontCheckGeneration();
    }

    UnboxedLayout& unboxedLayoutDontCheckGeneration() const {
        MOZ_ASSERT(addendumKind() == Addendum_UnboxedLayout);
        return *maybeUnboxedLayoutDontCheckGeneration();
    }

    UnboxedLayout& unboxedLayout() {
        maybeSweep(nullptr);
        return unboxedLayoutDontCheckGeneration();
    }

    void setUnboxedLayout(UnboxedLayout* layout) {
        setAddendum(Addendum_UnboxedLayout, layout);
    }

    ObjectGroup* maybeOriginalUnboxedGroup() const {
        if (addendumKind() == Addendum_OriginalUnboxedGroup)
            return reinterpret_cast<ObjectGroup*>(addendum_);
        return nullptr;
    }

    void setOriginalUnboxedGroup(ObjectGroup* group) {
        setAddendum(Addendum_OriginalUnboxedGroup, group);
    }

    TypeDescr* maybeTypeDescr() {
        
        
        if (addendumKind() == Addendum_TypeDescr)
            return reinterpret_cast<TypeDescr*>(addendum_);
        return nullptr;
    }

    TypeDescr& typeDescr() {
        MOZ_ASSERT(addendumKind() == Addendum_TypeDescr);
        return *maybeTypeDescr();
    }

    void setTypeDescr(TypeDescr* descr) {
        setAddendum(Addendum_TypeDescr, descr);
    }

    JSFunction* maybeInterpretedFunction() {
        
        
        if (addendumKind() == Addendum_InterpretedFunction)
            return reinterpret_cast<JSFunction*>(addendum_);
        return nullptr;
    }

    void setInterpretedFunction(JSFunction* fun) {
        setAddendum(Addendum_InterpretedFunction, fun);
    }

    class Property
    {
      public:
        
        
        
        HeapId id;

        
        HeapTypeSet types;

        explicit Property(jsid id)
          : id(id)
        {}

        Property(const Property& o)
          : id(o.id.get()), types(o.types)
        {}

        static uint32_t keyBits(jsid id) { return uint32_t(JSID_BITS(id)); }
        static jsid getKey(Property* p) { return p->id; }
    };

  private:
    








































    Property** propertySet;
  public:

    inline ObjectGroup(const Class* clasp, TaggedProto proto, JSCompartment* comp,
                       ObjectGroupFlags initialFlags);

    inline bool hasAnyFlags(ObjectGroupFlags flags) {
        MOZ_ASSERT((flags & OBJECT_FLAG_DYNAMIC_MASK) == flags);
        return !!(this->flags() & flags);
    }

    bool hasAllFlags(ObjectGroupFlags flags) {
        MOZ_ASSERT((flags & OBJECT_FLAG_DYNAMIC_MASK) == flags);
        return (this->flags() & flags) == flags;
    }

    bool hasAllFlagsDontCheckGeneration(ObjectGroupFlags flags) {
        MOZ_ASSERT((flags & OBJECT_FLAG_DYNAMIC_MASK) == flags);
        return (this->flagsDontCheckGeneration() & flags) == flags;
    }

    bool unknownProperties() {
        MOZ_ASSERT_IF(flags() & OBJECT_FLAG_UNKNOWN_PROPERTIES,
                      hasAllFlags(OBJECT_FLAG_DYNAMIC_MASK));
        return !!(flags() & OBJECT_FLAG_UNKNOWN_PROPERTIES);
    }

    bool unknownPropertiesDontCheckGeneration() {
        MOZ_ASSERT_IF(flagsDontCheckGeneration() & OBJECT_FLAG_UNKNOWN_PROPERTIES,
                      hasAllFlagsDontCheckGeneration(OBJECT_FLAG_DYNAMIC_MASK));
        return !!(flagsDontCheckGeneration() & OBJECT_FLAG_UNKNOWN_PROPERTIES);
    }

    bool shouldPreTenure() {
        return hasAnyFlags(OBJECT_FLAG_PRE_TENURE) && !unknownProperties();
    }

    gc::InitialHeap initialHeap(CompilerConstraintList* constraints);

    bool canPreTenure() {
        return !unknownProperties();
    }

    bool fromAllocationSite() {
        return flags() & OBJECT_FLAG_FROM_ALLOCATION_SITE;
    }

    void setShouldPreTenure(ExclusiveContext* cx) {
        MOZ_ASSERT(canPreTenure());
        setFlags(cx, OBJECT_FLAG_PRE_TENURE);
    }

    



    inline HeapTypeSet* getProperty(ExclusiveContext* cx, JSObject* obj, jsid id);

    
    inline HeapTypeSet* maybeGetProperty(jsid id);

    




    inline unsigned getPropertyCount();
    inline Property* getProperty(unsigned i);

    

    void updateNewPropertyTypes(ExclusiveContext* cx, JSObject* obj, jsid id, HeapTypeSet* types);
    bool addDefiniteProperties(ExclusiveContext* cx, Shape* shape);
    bool matchDefiniteProperties(HandleObject obj);
    void markPropertyNonData(ExclusiveContext* cx, JSObject* obj, jsid id);
    void markPropertyNonWritable(ExclusiveContext* cx, JSObject* obj, jsid id);
    void markStateChange(ExclusiveContext* cx);
    void setFlags(ExclusiveContext* cx, ObjectGroupFlags flags);
    void markUnknown(ExclusiveContext* cx);
    void maybeClearNewScriptOnOOM();
    void clearNewScript(ExclusiveContext* cx, ObjectGroup* replacement = nullptr);
    bool isPropertyNonData(jsid id);
    bool isPropertyNonWritable(jsid id);

    void print();

    inline void clearProperties();
    void maybeSweep(AutoClearTypeInferenceStateOnOOM* oom);
    void traceChildren(JSTracer* trc);

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

    void finalize(FreeOp* fop);
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

    const ObjectGroupFlags* addressOfFlags() const {
        return &flags_;
    }

    
    
    static inline int32_t addendumOriginalUnboxedGroupValue() {
        return Addendum_OriginalUnboxedGroup << OBJECT_FLAG_ADDENDUM_SHIFT;
    }

    inline uint32_t basePropertyCount();

  private:
    inline void setBasePropertyCount(uint32_t count);

    static void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(ObjectGroup, proto_) == offsetof(js::shadow::ObjectGroup, proto));
    }

  public:
    
    static bool useSingletonForClone(JSFunction* fun);

    
    static bool useSingletonForNewObject(JSContext* cx, JSScript* script, jsbytecode* pc);

    
    static bool useSingletonForAllocationSite(JSScript* script, jsbytecode* pc,
                                              JSProtoKey key);
    static bool useSingletonForAllocationSite(JSScript* script, jsbytecode* pc,
                                              const Class* clasp);

    

    static ObjectGroup* defaultNewGroup(ExclusiveContext* cx, const Class* clasp,
                                        TaggedProto proto,
                                        JSObject* associated = nullptr);
    static ObjectGroup* lazySingletonGroup(ExclusiveContext* cx, const Class* clasp,
                                           TaggedProto proto);

    static void setDefaultNewGroupUnknown(JSContext* cx, const js::Class* clasp, JS::HandleObject obj);

#ifdef DEBUG
    static bool hasDefaultNewGroup(JSObject* proto, const Class* clasp, ObjectGroup* group);
#endif

    

    
    
    static void fixArrayGroup(ExclusiveContext* cx, ArrayObject* obj);

    
    static void fixRestArgumentsGroup(ExclusiveContext* cx, ArrayObject* obj);

    
    static JSObject* newPlainObject(ExclusiveContext* cx,
                                    IdValuePair* properties, size_t nproperties,
                                    NewObjectKind newKind);

    

    
    
    static ObjectGroup* allocationSiteGroup(JSContext* cx, JSScript* script, jsbytecode* pc,
                                            JSProtoKey key);

    
    static ObjectGroup* callingAllocationSiteGroup(JSContext* cx, JSProtoKey key);

    
    static bool
    setAllocationSiteObjectGroup(JSContext* cx, HandleScript script, jsbytecode* pc,
                                 HandleObject obj, bool singleton);

    static ArrayObject* getOrFixupCopyOnWriteObject(JSContext* cx, HandleScript script,
                                                    jsbytecode* pc);
    static ArrayObject* getCopyOnWriteObject(JSScript* script, jsbytecode* pc);

    
    static bool findAllocationSite(JSContext* cx, ObjectGroup* group,
                                   JSScript** script, uint32_t* offset);

  private:
    static ObjectGroup* defaultNewGroup(JSContext* cx, JSProtoKey key);
    static void setGroupToHomogenousArray(ExclusiveContext* cx, JSObject* obj,
                                          TypeSet::Type type);
};


class ObjectGroupCompartment
{
    friend class ObjectGroup;

    struct NewEntry;
    typedef HashSet<NewEntry, NewEntry, SystemAllocPolicy> NewTable;
    class NewTableRef;

    
    NewTable* defaultNewTable;
    NewTable* lazyTable;

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

    
    
    
    
    
    
    
    
    
    ArrayObjectTable* arrayObjectTable;
    PlainObjectTable* plainObjectTable;

    struct AllocationSiteKey;
    typedef HashMap<AllocationSiteKey,
                    ReadBarrieredObjectGroup,
                    AllocationSiteKey,
                    SystemAllocPolicy> AllocationSiteTable;

    
    AllocationSiteTable* allocationSiteTable;

  public:
    ObjectGroupCompartment();
    ~ObjectGroupCompartment();

    void removeDefaultNewGroup(const Class* clasp, TaggedProto proto, JSObject* associated);
    void replaceDefaultNewGroup(const Class* clasp, TaggedProto proto, JSObject* associated,
                                ObjectGroup* group);

    static ObjectGroup* makeGroup(ExclusiveContext* cx, const Class* clasp,
                                  Handle<TaggedProto> proto,
                                  ObjectGroupFlags initialFlags = 0);

    void addSizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf,
                                size_t* allocationSiteTables,
                                size_t* arrayGroupTables,
                                size_t* plainObjectGroupTables,
                                size_t* compartmentTables);

    void clearTables();

    void sweep(FreeOp* fop);

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
    void checkNewTableAfterMovingGC(NewTable* table);
#endif

    void sweepNewTable(NewTable* table);
    void fixupNewTableAfterMovingGC(NewTable* table);

    static void newTablePostBarrier(ExclusiveContext* cx, NewTable* table,
                                    const Class* clasp, TaggedProto proto, JSObject* associated);
};

PlainObject*
NewPlainObjectWithProperties(ExclusiveContext* cx, IdValuePair* properties, size_t nproperties,
                             NewObjectKind newKind);

} 

#endif 
