





#ifndef jsobj_h
#define jsobj_h










#include "mozilla/MemoryReporting.h"

#include "jsapi.h"
#include "jsatom.h"
#include "jsclass.h"
#include "jsfriendapi.h"

#include "gc/Barrier.h"
#include "gc/Heap.h"

#include "vm/ObjectImpl.h"
#include "vm/Shape.h"
#include "vm/String.h"

namespace JS {
struct ObjectsExtraSizes;
}

namespace js {

class AutoPropDescArrayRooter;
class BaseProxyHandler;
struct GCMarker;
struct NativeIterator;
class Nursery;
struct StackShape;

namespace mjit { class Compiler; }

inline JSObject *
CastAsObject(PropertyOp op)
{
    return JS_FUNC_TO_DATA_PTR(JSObject *, op);
}

inline JSObject *
CastAsObject(StrictPropertyOp op)
{
    return JS_FUNC_TO_DATA_PTR(JSObject *, op);
}

inline Value
CastAsObjectJsval(PropertyOp op)
{
    return ObjectOrNullValue(CastAsObject(op));
}

inline Value
CastAsObjectJsval(StrictPropertyOp op)
{
    return ObjectOrNullValue(CastAsObject(op));
}







#define JS_PSG(name,getter,flags)                                               \
    {name, 0, (flags) | JSPROP_SHARED | JSPROP_NATIVE_ACCESSORS,                \
     JSOP_WRAPPER((JSPropertyOp)getter), JSOP_NULLWRAPPER}
#define JS_PSGS(name,getter,setter,flags)                                       \
    {name, 0, (flags) | JSPROP_SHARED | JSPROP_NATIVE_ACCESSORS,                \
     JSOP_WRAPPER((JSPropertyOp)getter), JSOP_WRAPPER((JSStrictPropertyOp)setter)}
#define JS_PS_END {0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER}



typedef Vector<PropDesc, 1> PropDescArray;







namespace baseops {






template <AllowGC allowGC>
extern JSBool
LookupProperty(JSContext *cx,
               typename MaybeRooted<JSObject*, allowGC>::HandleType obj,
               typename MaybeRooted<jsid, allowGC>::HandleType id,
               typename MaybeRooted<JSObject*, allowGC>::MutableHandleType objp,
               typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp);

inline bool
LookupProperty(JSContext *cx, HandleObject obj, PropertyName *name,
               MutableHandleObject objp, MutableHandleShape propp)
{
    Rooted<jsid> id(cx, NameToId(name));
    return LookupProperty<CanGC>(cx, obj, id, objp, propp);
}

extern JSBool
LookupElement(JSContext *cx, HandleObject obj, uint32_t index,
              MutableHandleObject objp, MutableHandleShape propp);

extern JSBool
DefineGeneric(JSContext *cx, HandleObject obj, HandleId id, HandleValue value,
               JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs);

inline JSBool
DefineProperty(JSContext *cx, HandleObject obj, PropertyName *name, HandleValue value,
               JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs)
{
    Rooted<jsid> id(cx, NameToId(name));
    return DefineGeneric(cx, obj, id, value, getter, setter, attrs);
}

extern JSBool
DefineElement(JSContext *cx, HandleObject obj, uint32_t index, HandleValue value,
              JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs);

extern JSBool
GetProperty(JSContext *cx, HandleObject obj, HandleObject receiver, HandleId id, MutableHandleValue vp);

extern JSBool
GetPropertyNoGC(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp);

extern JSBool
GetElement(JSContext *cx, HandleObject obj, HandleObject receiver, uint32_t index, MutableHandleValue vp);

inline JSBool
GetProperty(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp)
{
    return GetProperty(cx, obj, obj, id, vp);
}

inline JSBool
GetElement(JSContext *cx, HandleObject obj, uint32_t index, MutableHandleValue vp)
{
    return GetElement(cx, obj, obj, index, vp);
}

extern JSBool
GetPropertyDefault(JSContext *cx, HandleObject obj, HandleId id, HandleValue def, MutableHandleValue vp);

extern JSBool
SetPropertyHelper(JSContext *cx, HandleObject obj, HandleObject receiver, HandleId id,
                  unsigned defineHow, MutableHandleValue vp, JSBool strict);

inline bool
SetPropertyHelper(JSContext *cx, HandleObject obj, HandleObject receiver, PropertyName *name,
                  unsigned defineHow, MutableHandleValue vp, JSBool strict)
{
    Rooted<jsid> id(cx, NameToId(name));
    return SetPropertyHelper(cx, obj, receiver, id, defineHow, vp, strict);
}

extern JSBool
SetElementHelper(JSContext *cx, HandleObject obj, HandleObject Receiver, uint32_t index,
                 unsigned defineHow, MutableHandleValue vp, JSBool strict);

extern JSType
TypeOf(JSContext *cx, HandleObject obj);

extern JSBool
GetAttributes(JSContext *cx, HandleObject obj, HandleId id, unsigned *attrsp);

extern JSBool
SetAttributes(JSContext *cx, HandleObject obj, HandleId id, unsigned *attrsp);

extern JSBool
GetElementAttributes(JSContext *cx, HandleObject obj, uint32_t index, unsigned *attrsp);

extern JSBool
SetElementAttributes(JSContext *cx, HandleObject obj, uint32_t index, unsigned *attrsp);

extern JSBool
DeleteProperty(JSContext *cx, HandleObject obj, HandlePropertyName name, JSBool *succeeded);

extern JSBool
DeleteElement(JSContext *cx, HandleObject obj, uint32_t index, JSBool *succeeded);

extern JSBool
DeleteSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid, JSBool *succeeded);

extern JSBool
DeleteGeneric(JSContext *cx, HandleObject obj, HandleId id, JSBool *succeeded);

} 

extern Class IntlClass;
extern Class JSONClass;
extern Class MathClass;
extern Class ObjectClass;

class ArrayBufferObject;
class GlobalObject;
class MapObject;
class NewObjectCache;
class NormalArgumentsObject;
class SetObject;
class StrictArgumentsObject;

}  











class JSObject : public js::ObjectImpl
{
  private:
    friend class js::Shape;
    friend struct js::GCMarker;
    friend class  js::NewObjectCache;
    friend class js::Nursery;

    
    static js::types::TypeObject *makeLazyType(JSContext *cx, js::HandleObject obj);

  public:
    



    static bool setLastProperty(JSContext *cx, JS::HandleObject obj, js::HandleShape shape);

    
    inline void setLastPropertyInfallible(js::Shape *shape);

    



    static inline JSObject *create(JSContext *cx,
                                   js::gc::AllocKind kind,
                                   js::gc::InitialHeap heap,
                                   js::HandleShape shape,
                                   js::HandleTypeObject type,
                                   js::HeapSlot *extantSlots = NULL);

    
    static inline JSObject *createArray(JSContext *cx,
                                        js::gc::AllocKind kind,
                                        js::gc::InitialHeap heap,
                                        js::HandleShape shape,
                                        js::HandleTypeObject type,
                                        uint32_t length);

    




    inline void removeLastProperty(JSContext *cx);
    inline bool canRemoveLastProperty();

    



    static bool setSlotSpan(JSContext *cx, JS::HandleObject obj, uint32_t span);

    
    static const uint32_t NELEMENTS_LIMIT = JS_BIT(28);

  public:
    inline bool setDelegate(JSContext *cx);

    inline bool isBoundFunction() const;

    inline bool hasSpecialEquality() const;

    inline bool watched() const;
    inline bool setWatched(JSContext *cx);

    
    inline bool isVarObj();
    inline bool setVarObj(JSContext *cx);

    





    inline bool hasUncacheableProto() const;
    inline bool setUncacheableProto(JSContext *cx);

    



    inline bool hadElementsAccess() const;
    inline bool setHadElementsAccess(JSContext *cx);

  public:
    inline bool nativeEmpty() const;

    bool shadowingShapeChange(JSContext *cx, const js::Shape &shape);

    



    inline bool isIndexed() const;

    inline uint32_t propertyCount() const;

    inline bool hasShapeTable() const;

    void sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf, JS::ObjectsExtraSizes *sizes);

    bool hasIdempotentProtoChain() const;

    static const uint32_t MAX_FIXED_SLOTS = 16;

  public:

    

    
    bool isFixedSlot(size_t slot) {
        return slot < numFixedSlots();
    }

    
    size_t dynamicSlotIndex(size_t slot) {
        JS_ASSERT(slot >= numFixedSlots());
        return slot - numFixedSlots();
    }

    




    static bool growSlots(JSContext *cx, js::HandleObject obj, uint32_t oldCount,
                          uint32_t newCount);
    static void shrinkSlots(JSContext *cx, js::HandleObject obj, uint32_t oldCount,
                            uint32_t newCount);

    bool hasDynamicSlots() const { return slots != NULL; }

  protected:
    static inline bool updateSlotsForSpan(JSContext *cx, js::HandleObject obj, size_t oldSpan,
                                          size_t newSpan);

  public:
    



    inline void prepareSlotRangeForOverwrite(size_t start, size_t end);
    inline void prepareElementRangeForOverwrite(size_t start, size_t end);

    void rollbackProperties(JSContext *cx, uint32_t slotSpan);

    inline void nativeSetSlot(uint32_t slot, const js::Value &value);
    static inline void nativeSetSlotWithType(JSContext *cx, js::HandleObject, js::Shape *shape,
                                             const js::Value &value);

    inline const js::Value &getReservedSlot(uint32_t index) const {
        JS_ASSERT(index < JSSLOT_FREE(getClass()));
        return getSlot(index);
    }

    inline js::HeapSlot &getReservedSlotRef(uint32_t index) {
        JS_ASSERT(index < JSSLOT_FREE(getClass()));
        return getSlotRef(index);
    }

    inline void initReservedSlot(uint32_t index, const js::Value &v);
    inline void setReservedSlot(uint32_t index, const js::Value &v);

    



    static inline bool setSingletonType(JSContext *cx, js::HandleObject obj);

    
    inline js::types::TypeObject* getType(JSContext *cx);
    js::types::TypeObject* uninlinedGetType(JSContext *cx);

    const js::HeapPtr<js::types::TypeObject> &typeFromGC() const {
        
        return type_;
    }

    











    bool uninlinedIsProxy() const;
    JSObject *getProto() const {
        JS_ASSERT(!uninlinedIsProxy());
        return js::ObjectImpl::getProto();
    }
    static inline bool getProto(JSContext *cx, js::HandleObject obj,
                                js::MutableHandleObject protop);

    
    inline void setType(js::types::TypeObject *newType);
    void uninlinedSetType(js::types::TypeObject *newType);

    js::types::TypeObject *getNewType(JSContext *cx, js::Class *clasp, JSFunction *fun = NULL);

#ifdef DEBUG
    bool hasNewType(js::Class *clasp, js::types::TypeObject *newType);
#endif

    




    inline bool setIteratedSingleton(JSContext *cx);

    



    static bool setNewTypeUnknown(JSContext *cx, js::Class *clasp, JS::HandleObject obj);

    
    bool splicePrototype(JSContext *cx, js::Class *clasp, js::Handle<js::TaggedProto> proto);

    



    bool shouldSplicePrototype(JSContext *cx);

    

























    
    inline JSObject *getParent() const;
    static bool setParent(JSContext *cx, js::HandleObject obj, js::HandleObject newParent);

    




    inline JSObject *enclosingScope();

    
    inline JSObject *getMetadata() const;
    static bool setMetadata(JSContext *cx, js::HandleObject obj, js::HandleObject newMetadata);

    inline js::GlobalObject &global() const;

    
    static inline bool clearType(JSContext *cx, js::HandleObject obj);
    static bool clearParent(JSContext *cx, js::HandleObject obj);

    



  private:
    enum ImmutabilityType { SEAL, FREEZE };

    





    static bool sealOrFreeze(JSContext *cx, js::HandleObject obj, ImmutabilityType it);

    static bool isSealedOrFrozen(JSContext *cx, js::HandleObject obj, ImmutabilityType it, bool *resultp);

    static inline unsigned getSealedOrFrozenAttributes(unsigned attrs, ImmutabilityType it);

  public:
    
    static inline bool seal(JSContext *cx, js::HandleObject obj) { return sealOrFreeze(cx, obj, SEAL); }
    
    static inline bool freeze(JSContext *cx, js::HandleObject obj) { return sealOrFreeze(cx, obj, FREEZE); }

    static inline bool isSealed(JSContext *cx, js::HandleObject obj, bool *resultp) {
        return isSealedOrFrozen(cx, obj, SEAL, resultp);
    }
    static inline bool isFrozen(JSContext *cx, js::HandleObject obj, bool *resultp) {
        return isSealedOrFrozen(cx, obj, FREEZE, resultp);
    }

    
    static const char *className(JSContext *cx, js::HandleObject obj);

    
    inline bool ensureElements(JSContext *cx, uint32_t cap);
    bool growElements(js::ThreadSafeContext *tcx, uint32_t newcap);
    void shrinkElements(JSContext *cx, uint32_t cap);
    void setDynamicElements(js::ObjectElements *header) {
        JS_ASSERT(!hasDynamicElements());
        elements = header->elements();
        JS_ASSERT(hasDynamicElements());
    }

    uint32_t getDenseCapacity() {
        JS_ASSERT(uninlinedIsNative());
        JS_ASSERT(getElementsHeader()->capacity >= getElementsHeader()->initializedLength);
        return getElementsHeader()->capacity;
    }

    inline void setDenseInitializedLength(uint32_t length);
    inline void ensureDenseInitializedLength(JSContext *cx, uint32_t index, uint32_t extra);
    inline void setDenseElement(uint32_t index, const js::Value &val);
    inline void initDenseElement(uint32_t index, const js::Value &val);
    inline void setDenseElementMaybeConvertDouble(uint32_t index, const js::Value &val);
    static inline void setDenseElementWithType(JSContext *cx, js::HandleObject obj,
                                               uint32_t index, const js::Value &val);
    static inline void initDenseElementWithType(JSContext *cx, js::HandleObject obj,
                                                uint32_t index, const js::Value &val);
    static inline void setDenseElementHole(JSContext *cx, js::HandleObject obj, uint32_t index);
    static inline void removeDenseElementForSparseIndex(JSContext *cx, js::HandleObject obj,
                                                        uint32_t index);
    inline void copyDenseElements(uint32_t dstStart, const js::Value *src, uint32_t count);
    inline void initDenseElements(uint32_t dstStart, const js::Value *src, uint32_t count);
    inline void moveDenseElements(uint32_t dstStart, uint32_t srcStart, uint32_t count);
    inline void moveDenseElementsUnbarriered(uint32_t dstStart, uint32_t srcStart, uint32_t count);

    bool shouldConvertDoubleElements() {
        JS_ASSERT(uninlinedIsNative());
        return getElementsHeader()->shouldConvertDoubleElements();
    }

    inline void setShouldConvertDoubleElements();

    
    inline void markDenseElementsNotPacked(JSContext *cx);

    






    enum EnsureDenseResult { ED_OK, ED_FAILED, ED_SPARSE };
    inline EnsureDenseResult ensureDenseElements(JSContext *cx, uint32_t index, uint32_t extra);
    inline EnsureDenseResult parExtendDenseElements(js::ThreadSafeContext *tcx, js::Value *v,
                                                    uint32_t extra);

    inline EnsureDenseResult extendDenseElements(js::ThreadSafeContext *tcx,
                                                 uint32_t requiredCapacity, uint32_t extra);

    
    static bool sparsifyDenseElement(JSContext *cx, js::HandleObject obj, uint32_t index);

    
    static bool sparsifyDenseElements(JSContext *cx, js::HandleObject obj);

    
    static const uint32_t MIN_SPARSE_INDEX = 1000;

    



    static const unsigned SPARSE_DENSITY_RATIO = 8;

    



    bool willBeSparseElements(uint32_t requiredCapacity, uint32_t newElementsHint);

    



    static EnsureDenseResult maybeDensifySparseElements(JSContext *cx, js::HandleObject obj);

  public:
    



    static const uint32_t ITER_CLASS_NFIXED_SLOTS = 1;

    


    bool isCallable() {
        return getClass()->isCallable();
    }

    inline void finish(js::FreeOp *fop);
    JS_ALWAYS_INLINE void finalize(js::FreeOp *fop);

    static inline bool hasProperty(JSContext *cx, js::HandleObject obj,
                                   js::HandleId id, bool *foundp, unsigned flags = 0);

    






    static bool allocSlot(JSContext *cx, JS::HandleObject obj, uint32_t *slotp);
    void freeSlot(uint32_t slot);

  public:
    static bool reportReadOnly(JSContext *cx, jsid id, unsigned report = JSREPORT_ERROR);
    bool reportNotConfigurable(JSContext* cx, jsid id, unsigned report = JSREPORT_ERROR);
    bool reportNotExtensible(JSContext *cx, unsigned report = JSREPORT_ERROR);

    





    bool callMethod(JSContext *cx, js::HandleId id, unsigned argc, js::Value *argv,
                    js::MutableHandleValue vp);

  private:
    static js::Shape *getChildProperty(JSContext *cx, JS::HandleObject obj,
                                         js::HandleShape parent, js::StackShape &child);

  protected:
    






    static js::Shape *addPropertyInternal(JSContext *cx,
                                            JS::HandleObject obj, JS::HandleId id,
                                            JSPropertyOp getter, JSStrictPropertyOp setter,
                                            uint32_t slot, unsigned attrs,
                                            unsigned flags, int shortid, js::Shape **spp,
                                            bool allowDictionary);

  private:
    struct TradeGutsReserved;
    static bool ReserveForTradeGuts(JSContext *cx, JSObject *a, JSObject *b,
                                    TradeGutsReserved &reserved);

    static void TradeGuts(JSContext *cx, JSObject *a, JSObject *b,
                          TradeGutsReserved &reserved);

  public:
    
    static js::Shape *addProperty(JSContext *cx, JS::HandleObject, JS::HandleId id,
                                    JSPropertyOp getter, JSStrictPropertyOp setter,
                                    uint32_t slot, unsigned attrs, unsigned flags,
                                    int shortid, bool allowDictionary = true);

    
    js::Shape *addDataProperty(JSContext *cx, jsid id_, uint32_t slot, unsigned attrs) {
        JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
        JS::RootedObject self(cx, this);
        JS::RootedId id(cx, id_);
        return addProperty(cx, self, id, NULL, NULL, slot, attrs, 0, 0);
    }

    js::Shape *addDataProperty(JSContext *cx, js::HandlePropertyName name, uint32_t slot, unsigned attrs) {
        JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
        JS::RootedObject self(cx, this);
        JS::RootedId id(cx, NameToId(name));
        return addProperty(cx, self, id, NULL, NULL, slot, attrs, 0, 0);
    }

    
    static js::Shape *putProperty(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                                    JSPropertyOp getter, JSStrictPropertyOp setter,
                                    uint32_t slot, unsigned attrs,
                                    unsigned flags, int shortid);
    static js::Shape *putProperty(JSContext *cx, JS::HandleObject obj,
                                    js::PropertyName *name,
                                    JSPropertyOp getter, JSStrictPropertyOp setter,
                                    uint32_t slot, unsigned attrs,
                                    unsigned flags, int shortid)
    {
        JS::RootedId id(cx, js::NameToId(name));
        return putProperty(cx, obj, id, getter, setter, slot, attrs, flags, shortid);
    }

    
    static js::Shape *changeProperty(JSContext *cx, js::HandleObject obj,
                                       js::HandleShape shape, unsigned attrs, unsigned mask,
                                       JSPropertyOp getter, JSStrictPropertyOp setter);

    static inline bool changePropertyAttributes(JSContext *cx, js::HandleObject obj,
                                                js::HandleShape shape, unsigned attrs);

    
    bool removeProperty(JSContext *cx, jsid id);

    
    static void clear(JSContext *cx, js::HandleObject obj);

    static JSBool lookupGeneric(JSContext *cx, js::HandleObject obj, js::HandleId id,
                                js::MutableHandleObject objp, js::MutableHandleShape propp)
    {
        



        js::LookupGenericOp op = obj->getOps()->lookupGeneric;
        if (op)
            return op(cx, obj, id, objp, propp);
        return js::baseops::LookupProperty<js::CanGC>(cx, obj, id, objp, propp);
    }

    static JSBool lookupProperty(JSContext *cx, js::HandleObject obj, js::PropertyName *name,
                                 js::MutableHandleObject objp, js::MutableHandleShape propp)
    {
        JS::RootedId id(cx, js::NameToId(name));
        return lookupGeneric(cx, obj, id, objp, propp);
    }

    static JSBool lookupElement(JSContext *cx, js::HandleObject obj, uint32_t index,
                                js::MutableHandleObject objp, js::MutableHandleShape propp)
    {
        js::LookupElementOp op = obj->getOps()->lookupElement;
        return (op ? op : js::baseops::LookupElement)(cx, obj, index, objp, propp);
    }

    static JSBool lookupSpecial(JSContext *cx, js::HandleObject obj, js::SpecialId sid,
                                js::MutableHandleObject objp, js::MutableHandleShape propp)
    {
        JS::RootedId id(cx, SPECIALID_TO_JSID(sid));
        return lookupGeneric(cx, obj, id, objp, propp);
    }

    static JSBool defineGeneric(JSContext *cx, js::HandleObject obj,
                                       js::HandleId id, js::HandleValue value,
                                       JSPropertyOp getter = JS_PropertyStub,
                                       JSStrictPropertyOp setter = JS_StrictPropertyStub,
                                       unsigned attrs = JSPROP_ENUMERATE)
    {
        JS_ASSERT(!(attrs & JSPROP_NATIVE_ACCESSORS));
        js::DefineGenericOp op = obj->getOps()->defineGeneric;
        return (op ? op : js::baseops::DefineGeneric)(cx, obj, id, value, getter, setter, attrs);
    }

    static JSBool defineProperty(JSContext *cx, js::HandleObject obj,
                                 js::PropertyName *name, js::HandleValue value,
                                 JSPropertyOp getter = JS_PropertyStub,
                                 JSStrictPropertyOp setter = JS_StrictPropertyStub,
                                 unsigned attrs = JSPROP_ENUMERATE)
    {
        JS::RootedId id(cx, js::NameToId(name));
        return defineGeneric(cx, obj, id, value, getter, setter, attrs);
    }

    static JSBool defineElement(JSContext *cx, js::HandleObject obj,
                                uint32_t index, js::HandleValue value,
                                JSPropertyOp getter = JS_PropertyStub,
                                JSStrictPropertyOp setter = JS_StrictPropertyStub,
                                unsigned attrs = JSPROP_ENUMERATE)
    {
        js::DefineElementOp op = obj->getOps()->defineElement;
        return (op ? op : js::baseops::DefineElement)(cx, obj, index, value, getter, setter, attrs);
    }

    static JSBool defineSpecial(JSContext *cx, js::HandleObject obj,
                                js::SpecialId sid, js::HandleValue value,
                                JSPropertyOp getter = JS_PropertyStub,
                                JSStrictPropertyOp setter = JS_StrictPropertyStub,
                                unsigned attrs = JSPROP_ENUMERATE)
    {
        JS::RootedId id(cx, SPECIALID_TO_JSID(sid));
        return defineGeneric(cx, obj, id, value, getter, setter, attrs);
    }

    static JSBool getGeneric(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                             js::HandleId id, js::MutableHandleValue vp)
    {
        js::GenericIdOp op = obj->getOps()->getGeneric;
        if (op) {
            if (!op(cx, obj, receiver, id, vp))
                return false;
        } else {
            if (!js::baseops::GetProperty(cx, obj, receiver, id, vp))
                return false;
        }
        return true;
    }

    static JSBool getGenericNoGC(JSContext *cx, JSObject *obj, JSObject *receiver,
                                 jsid id, js::Value *vp)
    {
        js::GenericIdOp op = obj->getOps()->getGeneric;
        if (op)
            return false;
        return js::baseops::GetPropertyNoGC(cx, obj, receiver, id, vp);
    }

    static JSBool getProperty(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                              js::PropertyName *name, js::MutableHandleValue vp)
    {
        JS::RootedId id(cx, js::NameToId(name));
        return getGeneric(cx, obj, receiver, id, vp);
    }

    static JSBool getPropertyNoGC(JSContext *cx, JSObject *obj, JSObject *receiver,
                                         js::PropertyName *name, js::Value *vp)
    {
        return getGenericNoGC(cx, obj, receiver, js::NameToId(name), vp);
    }

    static inline JSBool getElement(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                                    uint32_t index, js::MutableHandleValue vp);
    static inline JSBool getElementNoGC(JSContext *cx, JSObject *obj, JSObject *receiver,
                                        uint32_t index, js::Value *vp);

    

    static inline JSBool getElementIfPresent(JSContext *cx, js::HandleObject obj,
                                             js::HandleObject receiver, uint32_t index,
                                             js::MutableHandleValue vp, bool *present);

    static JSBool getSpecial(JSContext *cx, js::HandleObject obj,
                             js::HandleObject receiver, js::SpecialId sid,
                             js::MutableHandleValue vp)
    {
        JS::RootedId id(cx, SPECIALID_TO_JSID(sid));
        return getGeneric(cx, obj, receiver, id, vp);
    }

    static JSBool setGeneric(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                             js::HandleId id, js::MutableHandleValue vp, JSBool strict)
    {
        if (obj->getOps()->setGeneric)
            return nonNativeSetProperty(cx, obj, id, vp, strict);
        return js::baseops::SetPropertyHelper(cx, obj, receiver, id, 0, vp, strict);
    }

    static JSBool setProperty(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                              js::PropertyName *name,
                              js::MutableHandleValue vp, JSBool strict)
    {
        JS::RootedId id(cx, js::NameToId(name));
        return setGeneric(cx, obj, receiver, id, vp, strict);
    }

    static JSBool setElement(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                             uint32_t index, js::MutableHandleValue vp, JSBool strict)
    {
        if (obj->getOps()->setElement)
            return nonNativeSetElement(cx, obj, index, vp, strict);
        return js::baseops::SetElementHelper(cx, obj, receiver, index, 0, vp, strict);
    }

    static JSBool setSpecial(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                             js::SpecialId sid, js::MutableHandleValue vp, JSBool strict)
    {
        JS::RootedId id(cx, SPECIALID_TO_JSID(sid));
        return setGeneric(cx, obj, receiver, id, vp, strict);
    }


    static JSBool nonNativeSetProperty(JSContext *cx, js::HandleObject obj,
                                       js::HandleId id, js::MutableHandleValue vp, JSBool strict);
    static JSBool nonNativeSetElement(JSContext *cx, js::HandleObject obj,
                                      uint32_t index, js::MutableHandleValue vp, JSBool strict);

    static JSBool getGenericAttributes(JSContext *cx, js::HandleObject obj,
                                       js::HandleId id, unsigned *attrsp)
    {
        js::GenericAttributesOp op = obj->getOps()->getGenericAttributes;
        return (op ? op : js::baseops::GetAttributes)(cx, obj, id, attrsp);
    }

    static JSBool getPropertyAttributes(JSContext *cx, js::HandleObject obj,
                                        js::PropertyName *name, unsigned *attrsp)
    {
        JS::RootedId id(cx, js::NameToId(name));
        return getGenericAttributes(cx, obj, id, attrsp);
    }

    static inline JSBool getElementAttributes(JSContext *cx, js::HandleObject obj,
                                              uint32_t index, unsigned *attrsp);

    static JSBool getSpecialAttributes(JSContext *cx, js::HandleObject obj,
                                       js::SpecialId sid, unsigned *attrsp)
    {
        JS::RootedId id(cx, SPECIALID_TO_JSID(sid));
        return getGenericAttributes(cx, obj, id, attrsp);
    }

    static inline JSBool setGenericAttributes(JSContext *cx, js::HandleObject obj,
                                              js::HandleId id, unsigned *attrsp);
    static inline JSBool setPropertyAttributes(JSContext *cx, js::HandleObject obj,
                                               js::PropertyName *name, unsigned *attrsp);
    static inline JSBool setElementAttributes(JSContext *cx, js::HandleObject obj,
                                              uint32_t index, unsigned *attrsp);
    static inline JSBool setSpecialAttributes(JSContext *cx, js::HandleObject obj,
                                              js::SpecialId sid, unsigned *attrsp);

    static inline bool deleteProperty(JSContext *cx, js::HandleObject obj,
                                      js::HandlePropertyName name,
                                      JSBool *succeeded);
    static inline bool deleteElement(JSContext *cx, js::HandleObject obj,
                                     uint32_t index, JSBool *succeeded);
    static inline bool deleteSpecial(JSContext *cx, js::HandleObject obj,
                                     js::HandleSpecialId sid, JSBool *succeeded);
    static bool deleteByValue(JSContext *cx, js::HandleObject obj,
                              const js::Value &property, JSBool *succeeded);

    static bool enumerate(JSContext *cx, JS::HandleObject obj, JSIterateOp iterop,
                                 JS::MutableHandleValue statep, JS::MutableHandleId idp)
    {
        JSNewEnumerateOp op = obj->getOps()->enumerate;
        return (op ? op : JS_EnumerateState)(cx, obj, iterop, statep, idp);
    }

    static bool defaultValue(JSContext *cx, js::HandleObject obj, JSType hint,
                             js::MutableHandleValue vp)
    {
        JSConvertOp op = obj->getClass()->convert;
        bool ok;
        if (op == JS_ConvertStub)
            ok = js::DefaultValue(cx, obj, hint, vp);
        else
            ok = op(cx, obj, hint, vp);
        JS_ASSERT_IF(ok, vp.isPrimitive());
        return ok;
    }

    static JSObject *thisObject(JSContext *cx, js::HandleObject obj)
    {
        JSObjectOp op = obj->getOps()->thisObject;
        return op ? op(cx, obj) : obj;
    }

    static bool thisObject(JSContext *cx, const js::Value &v, js::Value *vp);

    static bool swap(JSContext *cx, JS::HandleObject a, JS::HandleObject b);

    inline void initArrayClass();

    



























    template <class T>
    inline bool is() const { return getClass() == &T::class_; }

    template <class T>
    T &as() {
        JS_ASSERT(is<T>());
        return *static_cast<T *>(this);
    }

    template <class T>
    const T &as() const {
        JS_ASSERT(is<T>());
        return *static_cast<const T *>(this);
    }

    
    inline bool isObject()           const { return hasClass(&js::ObjectClass); }
    using js::ObjectImpl::isProxy;

    
    inline bool isWrapper()                 const;
    inline bool isFunctionProxy()           const { return hasClass(&js::FunctionProxyClass); }
    inline bool isCrossCompartmentWrapper() const;

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_OBJECT; }

#ifdef DEBUG
    void dump();
#endif

  private:
    static void staticAsserts() {
        MOZ_STATIC_ASSERT(sizeof(JSObject) == sizeof(js::shadow::Object),
                          "shadow interface must match actual interface");
        MOZ_STATIC_ASSERT(sizeof(JSObject) == sizeof(js::ObjectImpl),
                          "JSObject itself must not have any fields");
        MOZ_STATIC_ASSERT(sizeof(JSObject) % sizeof(js::Value) == 0,
                          "fixed slots after an object must be aligned");
    }

    JSObject() MOZ_DELETE;
    JSObject(const JSObject &other) MOZ_DELETE;
    void operator=(const JSObject &other) MOZ_DELETE;
};






static JS_ALWAYS_INLINE bool
operator==(const JSObject &lhs, const JSObject &rhs)
{
    return &lhs == &rhs;
}

static JS_ALWAYS_INLINE bool
operator!=(const JSObject &lhs, const JSObject &rhs)
{
    return &lhs != &rhs;
}

struct JSObject_Slots2 : JSObject { js::Value fslots[2]; };
struct JSObject_Slots4 : JSObject { js::Value fslots[4]; };
struct JSObject_Slots8 : JSObject { js::Value fslots[8]; };
struct JSObject_Slots12 : JSObject { js::Value fslots[12]; };
struct JSObject_Slots16 : JSObject { js::Value fslots[16]; };

static inline bool
js_IsCallable(const js::Value &v)
{
    return v.isObject() && v.toObject().isCallable();
}

inline JSObject *
GetInnerObject(JSContext *cx, js::HandleObject obj)
{
    if (JSObjectOp op = obj->getClass()->ext.innerObject)
        return op(cx, obj);
    return obj;
}

inline JSObject *
GetOuterObject(JSContext *cx, js::HandleObject obj)
{
    if (JSObjectOp op = obj->getClass()->ext.outerObject)
        return op(cx, obj);
    return obj;
}

class JSValueArray {
  public:
    jsval *array;
    size_t length;

    JSValueArray(jsval *v, size_t c) : array(v), length(c) {}
};

class ValueArray {
  public:
    js::Value *array;
    size_t length;

    ValueArray(js::Value *v, size_t c) : array(v), length(c) {}
};

namespace js {

template <AllowGC allowGC>
extern JSBool
HasOwnProperty(JSContext *cx, LookupGenericOp lookup,
               typename MaybeRooted<JSObject*, allowGC>::HandleType obj,
               typename MaybeRooted<jsid, allowGC>::HandleType id,
               typename MaybeRooted<JSObject*, allowGC>::MutableHandleType objp,
               typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp);

bool
IsStandardClassResolved(JSObject *obj, js::Class *clasp);

void
MarkStandardClassInitializedNoProto(JSObject *obj, js::Class *clasp);

} 




extern const char js_watch_str[];
extern const char js_unwatch_str[];
extern const char js_hasOwnProperty_str[];
extern const char js_isPrototypeOf_str[];
extern const char js_propertyIsEnumerable_str[];

#ifdef OLD_GETTER_SETTER_METHODS
extern const char js_defineGetter_str[];
extern const char js_defineSetter_str[];
extern const char js_lookupGetter_str[];
extern const char js_lookupSetter_str[];
#endif

extern JSBool
js_PopulateObject(JSContext *cx, js::HandleObject newborn, js::HandleObject props);




extern bool
js_GetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key,
                  js::MutableHandleObject objp);





extern JSProtoKey
js_IdentifyClassPrototype(JSObject *obj);





bool
js_FindClassObject(JSContext *cx, JSProtoKey protoKey, js::MutableHandleValue vp,
                   js::Class *clasp = NULL);





extern js::Shape *
js_AddNativeProperty(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                     JSPropertyOp getter, JSStrictPropertyOp setter, uint32_t slot,
                     unsigned attrs, unsigned flags, int shortid);

namespace js {

extern JSBool
DefineOwnProperty(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                  JS::HandleValue descriptor, JSBool *bp);

extern JSBool
DefineOwnProperty(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                  const js::PropertyDescriptor &descriptor, JSBool *bp);





enum NewObjectKind {
    
    GenericObject,

    




    SingletonObject,

    




    MaybeSingletonObject,

    




    TenuredObject
};

inline gc::InitialHeap
GetInitialHeap(NewObjectKind newKind, const Class *clasp)
{
    if (clasp->finalize || newKind != GenericObject)
        return gc::TenuredHeap;
    return gc::DefaultHeap;
}



extern JSObject *
CreateThisForFunctionWithProto(JSContext *cx, js::HandleObject callee, JSObject *proto,
                               NewObjectKind newKind = GenericObject);


extern JSObject *
CreateThisForFunction(JSContext *cx, js::HandleObject callee, bool newType);


extern JSObject *
CreateThis(JSContext *cx, js::Class *clasp, js::HandleObject callee);

extern JSObject *
CloneObject(JSContext *cx, HandleObject obj, Handle<js::TaggedProto> proto, HandleObject parent);




const unsigned DNP_DONT_PURGE   = 1;   
const unsigned DNP_UNQUALIFIED  = 2;   


const unsigned DNP_SKIP_TYPE    = 4;   




extern bool
DefineNativeProperty(JSContext *cx, HandleObject obj, HandleId id, HandleValue value,
                     PropertyOp getter, StrictPropertyOp setter, unsigned attrs,
                     unsigned flags, int shortid, unsigned defineHow = 0);

inline bool
DefineNativeProperty(JSContext *cx, HandleObject obj, PropertyName *name, HandleValue value,
                     PropertyOp getter, StrictPropertyOp setter, unsigned attrs,
                     unsigned flags, int shortid, unsigned defineHow = 0)
{
    Rooted<jsid> id(cx, NameToId(name));
    return DefineNativeProperty(cx, obj, id, value, getter, setter, attrs, flags,
                                shortid, defineHow);
}




extern bool
LookupPropertyWithFlags(JSContext *cx, HandleObject obj, HandleId id, unsigned flags,
                        js::MutableHandleObject objp, js::MutableHandleShape propp);

inline bool
LookupPropertyWithFlags(JSContext *cx, HandleObject obj, PropertyName *name, unsigned flags,
                        js::MutableHandleObject objp, js::MutableHandleShape propp)
{
    Rooted<jsid> id(cx, NameToId(name));
    return LookupPropertyWithFlags(cx, obj, id, flags, objp, propp);
}









extern bool
DefineProperty(JSContext *cx, js::HandleObject obj,
               js::HandleId id, const PropDesc &desc, bool throwError,
               bool *rval);

bool
DefineProperties(JSContext *cx, HandleObject obj, HandleObject props);





extern bool
ReadPropertyDescriptors(JSContext *cx, HandleObject props, bool checkAccessors,
                        AutoIdVector *ids, AutoPropDescArrayRooter *descs);





static const unsigned RESOLVE_INFER = 0xffff;


extern bool
LookupName(JSContext *cx, HandlePropertyName name, HandleObject scopeChain,
           MutableHandleObject objp, MutableHandleObject pobjp, MutableHandleShape propp);

extern bool
LookupNameNoGC(JSContext *cx, PropertyName *name, JSObject *scopeChain,
               JSObject **objp, JSObject **pobjp, Shape **propp);








extern bool
LookupNameWithGlobalDefault(JSContext *cx, HandlePropertyName name, HandleObject scopeChain,
                            MutableHandleObject objp);

}

extern JSObject *
js_FindVariableScope(JSContext *cx, JSFunction **funp);







extern JSBool
js_NativeGet(JSContext *cx, js::Handle<JSObject*> obj, js::Handle<JSObject*> pobj,
             js::Handle<js::Shape*> shape, unsigned getHow, js::MutableHandle<js::Value> vp);

extern JSBool
js_NativeSet(JSContext *cx, js::Handle<JSObject*> obj, js::Handle<JSObject*> receiver,
             js::Handle<js::Shape*> shape, bool strict, js::MutableHandleValue vp);

namespace js {

bool
GetPropertyHelper(JSContext *cx, HandleObject obj, HandleId id, uint32_t getHow, MutableHandleValue vp);

inline bool
GetPropertyHelper(JSContext *cx, HandleObject obj, PropertyName *name, uint32_t getHow, MutableHandleValue vp)
{
    RootedId id(cx, NameToId(name));
    return GetPropertyHelper(cx, obj, id, getHow, vp);
}

bool
LookupPropertyPure(JSObject *obj, jsid id, JSObject **objp, Shape **propp);

bool
GetPropertyPure(JSObject *obj, jsid id, Value *vp);

inline bool
GetPropertyPure(JSObject *obj, PropertyName *name, Value *vp)
{
    return GetPropertyPure(obj, NameToId(name), vp);
}

bool
GetOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id, PropertyDescriptor *desc);

bool
GetOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp);

bool
NewPropertyDescriptorObject(JSContext *cx, const PropertyDescriptor *desc, MutableHandleValue vp);





extern bool
HasDataProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp);

inline bool
HasDataProperty(JSContext *cx, JSObject *obj, PropertyName *name, Value *vp)
{
    return HasDataProperty(cx, obj, NameToId(name), vp);
}

extern JSBool
CheckAccess(JSContext *cx, JSObject *obj, HandleId id, JSAccessMode mode,
            MutableHandleValue v, unsigned *attrsp);

extern bool
IsDelegate(JSContext *cx, HandleObject obj, const Value &v, bool *result);

} 





extern JSBool
js_PrimitiveToObject(JSContext *cx, js::Value *vp);

extern JSBool
js_ValueToObjectOrNull(JSContext *cx, const js::Value &v, JS::MutableHandleObject objp);


extern JSObject *
js_ValueToNonNullObject(JSContext *cx, const js::Value &v);

namespace js {






extern JSObject *
ToObjectSlow(JSContext *cx, HandleValue vp, bool reportScanStack);


JS_ALWAYS_INLINE JSObject *
ToObject(JSContext *cx, HandleValue vp)
{
    if (vp.isObject())
        return &vp.toObject();
    return ToObjectSlow(cx, vp, false);
}


JS_ALWAYS_INLINE JSObject *
ToObjectFromStack(JSContext *cx, HandleValue vp)
{
    if (vp.isObject())
        return &vp.toObject();
    return ToObjectSlow(cx, vp, true);
}

extern JSObject *
CloneObjectLiteral(JSContext *cx, HandleObject parent, HandleObject srcObj);

} 

extern void
js_GetObjectSlotName(JSTracer *trc, char *buf, size_t bufsize);

extern JSBool
js_ReportGetterOnlyAssignment(JSContext *cx);

extern unsigned
js_InferFlags(JSContext *cx, unsigned defaultFlags);









extern bool
js_GetClassPrototype(JSContext *cx, JSProtoKey protoKey, js::MutableHandleObject protop,
                     js::Class *clasp = NULL);

namespace js {

extern bool
SetClassAndProto(JSContext *cx, HandleObject obj,
                 Class *clasp, Handle<TaggedProto> proto, bool checkForCycles);

extern JSObject *
NonNullObject(JSContext *cx, const Value &v);

extern const char *
InformalValueTypeName(const Value &v);

extern bool
GetFirstArgumentAsObject(JSContext *cx, const CallArgs &args, const char *method,
                         MutableHandleObject objp);


extern bool
Throw(JSContext *cx, jsid id, unsigned errorNumber);

extern bool
Throw(JSContext *cx, JSObject *obj, unsigned errorNumber);














extern bool
CheckDefineProperty(JSContext *cx, HandleObject obj, HandleId id, HandleValue value,
                    PropertyOp getter, StrictPropertyOp setter, unsigned attrs);

}  

#endif 
