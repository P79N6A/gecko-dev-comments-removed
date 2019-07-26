






#ifndef jsobj_h___
#define jsobj_h___









#include "jsapi.h"
#include "jsatom.h"
#include "jsclass.h"
#include "jsfriendapi.h"

#include "gc/Barrier.h"
#include "gc/Heap.h"

#include "vm/ObjectImpl.h"
#include "vm/String.h"

ForwardDeclareJS(Object);

namespace JS {
struct ObjectsExtraSizes;
}

namespace js {

class AutoPropDescArrayRooter;
class BaseProxyHandler;
class CallObject;
struct GCMarker;
struct NativeIterator;
ForwardDeclare(Shape);
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
DeleteProperty(JSContext *cx, HandleObject obj, HandlePropertyName name, MutableHandleValue rval, JSBool strict);

extern JSBool
DeleteElement(JSContext *cx, HandleObject obj, uint32_t index, MutableHandleValue rval, JSBool strict);

extern JSBool
DeleteSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid, MutableHandleValue rval, JSBool strict);

extern JSBool
DeleteGeneric(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue rval, JSBool strict);

} 


extern JSBool
DefaultValue(JSContext *cx, HandleObject obj, JSType hint, MutableHandleValue vp);

extern Class ArrayClass;
extern Class ArrayBufferClass;
extern Class BlockClass;
extern Class BooleanClass;
extern Class CallableObjectClass;
extern Class DataViewClass;
extern Class DateClass;
extern Class ErrorClass;
extern Class ElementIteratorClass;
extern Class GeneratorClass;
extern Class IntlClass;
extern Class JSONClass;
extern Class MapIteratorClass;
extern Class MathClass;
extern Class ModuleClass;
extern Class NumberClass;
extern Class NormalArgumentsObjectClass;
extern Class ObjectClass;
extern Class ProxyClass;
extern Class RegExpClass;
extern Class RegExpStaticsClass;
extern Class SetIteratorClass;
extern Class StopIterationClass;
extern Class StringClass;
extern Class StrictArgumentsObjectClass;
extern Class WeakMapClass;
extern Class WithClass;

class ArgumentsObject;
class ArrayBufferObject;
class BlockObject;
class BooleanObject;
class ClonedBlockObject;
class DataViewObject;
class DebugScopeObject;
class DeclEnvObject;
class ElementIteratorObject;
class GlobalObject;
class MapObject;
class MapIteratorObject;
class Module;
class NestedScopeObject;
class NewObjectCache;
class NormalArgumentsObject;
class NumberObject;
class PropertyIteratorObject;
class ScopeObject;
class SetObject;
class SetIteratorObject;
class StaticBlockObject;
class StrictArgumentsObject;
class StringObject;
class RegExpObject;
class WithObject;

}  











class JSObject : public js::ObjectImpl
{
  private:
    friend class js::Shape;
    friend struct js::GCMarker;
    friend class  js::NewObjectCache;

    
    static js::types::TypeObject *makeLazyType(JSContext *cx, js::HandleObject obj);

  public:
    



    static bool setLastProperty(JSContext *cx, JS::HandleObject obj, js::HandleShape shape);

    
    inline void setLastPropertyInfallible(js::RawShape shape);

    



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

  public:
    inline bool nativeEmpty() const;

    bool shadowingShapeChange(JSContext *cx, const js::Shape &shape);

    



    inline bool isIndexed() const;

    inline uint32_t propertyCount() const;

    inline bool hasShapeTable() const;

    void sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf, JS::ObjectsExtraSizes *sizes);

    bool hasIdempotentProtoChain() const;

    static const uint32_t MAX_FIXED_SLOTS = 16;

  public:

    

    
    inline bool isFixedSlot(size_t slot);

    
    inline size_t dynamicSlotIndex(size_t slot);

    
    inline const js::HeapSlot *getRawSlots();

    




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

    inline void nativeSetSlot(unsigned slot, const js::Value &value);
    static inline void nativeSetSlotWithType(JSContext *cx, js::HandleObject, js::Shape *shape,
                                             const js::Value &value);

    inline const js::Value &getReservedSlot(unsigned index) const;
    inline js::HeapSlot &getReservedSlotRef(unsigned index);
    inline void initReservedSlot(unsigned index, const js::Value &v);
    inline void setReservedSlot(unsigned index, const js::Value &v);

    



    static inline bool setSingletonType(JSContext *cx, js::HandleObject obj);

    inline js::types::TypeObject* getType(JSContext *cx);

    const js::HeapPtr<js::types::TypeObject> &typeFromGC() const {
        
        return type_;
    }

    











    inline JSObject *getProto() const;
    using js::ObjectImpl::getTaggedProto;
    static inline bool getProto(JSContext *cx, js::HandleObject obj,
                                js::MutableHandleObject protop);

    inline void setType(js::types::TypeObject *newType);

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

    inline js::GlobalObject &global() const;
    using js::ObjectImpl::compartment;

    
    static inline bool clearType(JSContext *cx, js::HandleObject obj);
    static bool clearParent(JSContext *cx, js::HandleObject obj);

    



  private:
    enum ImmutabilityType { SEAL, FREEZE };

    





    static bool sealOrFreeze(JSContext *cx, js::HandleObject obj, ImmutabilityType it);

    static bool isSealedOrFrozen(JSContext *cx, js::HandleObject obj, ImmutabilityType it, bool *resultp);

    static inline unsigned getSealedOrFrozenAttributes(unsigned attrs, ImmutabilityType it);

  public:
    bool preventExtensions(JSContext *cx);

    
    static inline bool seal(JSContext *cx, js::HandleObject obj) { return sealOrFreeze(cx, obj, SEAL); }
    
    static inline bool freeze(JSContext *cx, js::HandleObject obj) { return sealOrFreeze(cx, obj, FREEZE); }

    static inline bool isSealed(JSContext *cx, js::HandleObject obj, bool *resultp) {
        return isSealedOrFrozen(cx, obj, SEAL, resultp);
    }
    static inline bool isFrozen(JSContext *cx, js::HandleObject obj, bool *resultp) {
        return isSealedOrFrozen(cx, obj, FREEZE, resultp);
    }

    

    inline bool ensureElements(JSContext *cx, unsigned cap);
    bool growElements(JSContext *cx, unsigned cap);
    bool growElements(js::Allocator *alloc, unsigned cap);
    void shrinkElements(JSContext *cx, unsigned cap);
    inline void setDynamicElements(js::ObjectElements *header);

    inline uint32_t getDenseCapacity();
    inline void setDenseInitializedLength(uint32_t length);
    inline void ensureDenseInitializedLength(JSContext *cx, unsigned index, unsigned extra);
    inline void setDenseElement(unsigned idx, const js::Value &val);
    inline void initDenseElement(unsigned idx, const js::Value &val);
    inline void setDenseElementMaybeConvertDouble(unsigned idx, const js::Value &val);
    static inline void setDenseElementWithType(JSContext *cx, js::HandleObject obj,
                                               unsigned idx, const js::Value &val);
    static inline void initDenseElementWithType(JSContext *cx, js::HandleObject obj,
                                                unsigned idx, const js::Value &val);
    static inline void setDenseElementHole(JSContext *cx, js::HandleObject obj, unsigned idx);
    static inline void removeDenseElementForSparseIndex(JSContext *cx, js::HandleObject obj,
                                                        unsigned idx);
    inline void copyDenseElements(unsigned dstStart, const js::Value *src, unsigned count);
    inline void initDenseElements(unsigned dstStart, const js::Value *src, unsigned count);
    inline void moveDenseElements(unsigned dstStart, unsigned srcStart, unsigned count);
    inline void moveDenseElementsUnbarriered(unsigned dstStart, unsigned srcStart, unsigned count);
    inline bool shouldConvertDoubleElements();
    inline void setShouldConvertDoubleElements();

    
    inline void markDenseElementsNotPacked(JSContext *cx);

    






    enum EnsureDenseResult { ED_OK, ED_FAILED, ED_SPARSE };
    inline EnsureDenseResult ensureDenseElements(JSContext *cx, unsigned index, unsigned extra);
    inline EnsureDenseResult parExtendDenseElements(js::Allocator *alloc, js::Value *v,
                                                    uint32_t extra);
    template<typename CONTEXT>
    inline EnsureDenseResult extendDenseElements(CONTEXT *cx, unsigned requiredCapacity, unsigned extra);

    
    static bool sparsifyDenseElement(JSContext *cx, js::HandleObject obj, unsigned index);

    
    static bool sparsifyDenseElements(JSContext *cx, js::HandleObject obj);

    
    static const unsigned MIN_SPARSE_INDEX = 1000;

    



    static const unsigned SPARSE_DENSITY_RATIO = 8;

    



    bool willBeSparseElements(unsigned requiredCapacity, unsigned newElementsHint);

    



    static EnsureDenseResult maybeDensifySparseElements(JSContext *cx, js::HandleObject obj);

    
    inline uint32_t getArrayLength() const;
    static inline void setArrayLength(JSContext *cx, js::HandleObject obj, uint32_t length);
    inline void setArrayLengthInt32(uint32_t length);

  public:
    



    static const uint32_t JSSLOT_DATE_UTC_TIME = 0;
    static const uint32_t JSSLOT_DATE_TZA = 1;

    




    static const uint32_t JSSLOT_DATE_COMPONENTS_START = 2;

    static const uint32_t JSSLOT_DATE_LOCAL_TIME    = JSSLOT_DATE_COMPONENTS_START + 0;
    static const uint32_t JSSLOT_DATE_LOCAL_YEAR    = JSSLOT_DATE_COMPONENTS_START + 1;
    static const uint32_t JSSLOT_DATE_LOCAL_MONTH   = JSSLOT_DATE_COMPONENTS_START + 2;
    static const uint32_t JSSLOT_DATE_LOCAL_DATE    = JSSLOT_DATE_COMPONENTS_START + 3;
    static const uint32_t JSSLOT_DATE_LOCAL_DAY     = JSSLOT_DATE_COMPONENTS_START + 4;
    static const uint32_t JSSLOT_DATE_LOCAL_HOURS   = JSSLOT_DATE_COMPONENTS_START + 5;
    static const uint32_t JSSLOT_DATE_LOCAL_MINUTES = JSSLOT_DATE_COMPONENTS_START + 6;
    static const uint32_t JSSLOT_DATE_LOCAL_SECONDS = JSSLOT_DATE_COMPONENTS_START + 7;

    static const uint32_t DATE_CLASS_RESERVED_SLOTS = JSSLOT_DATE_LOCAL_SECONDS + 1;

    inline const js::Value &getDateUTCTime() const;
    inline void setDateUTCTime(const js::Value &pthis);

    



    friend class JSFunction;

    inline JSFunction *toFunction();
    inline const JSFunction *toFunction() const;

  public:
    



    static const uint32_t ITER_CLASS_NFIXED_SLOTS = 1;

    


    inline bool isCallable();

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
    static js::RawShape getChildProperty(JSContext *cx, JS::HandleObject obj,
                                         js::HandleShape parent, js::StackShape &child);

  protected:
    






    static js::RawShape addPropertyInternal(JSContext *cx,
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
    
    static js::RawShape addProperty(JSContext *cx, JS::HandleObject, JS::HandleId id,
                                    JSPropertyOp getter, JSStrictPropertyOp setter,
                                    uint32_t slot, unsigned attrs, unsigned flags,
                                    int shortid, bool allowDictionary = true);

    
    js::RawShape addDataProperty(JSContext *cx, jsid id_, uint32_t slot, unsigned attrs) {
        JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
        JS::RootedObject self(cx, this);
        JS::RootedId id(cx, id_);
        return addProperty(cx, self, id, NULL, NULL, slot, attrs, 0, 0);
    }

    js::RawShape addDataProperty(JSContext *cx, js::HandlePropertyName name, uint32_t slot, unsigned attrs) {
        JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
        JS::RootedObject self(cx, this);
        JS::RootedId id(cx, NameToId(name));
        return addProperty(cx, self, id, NULL, NULL, slot, attrs, 0, 0);
    }

    
    static js::RawShape putProperty(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                                    JSPropertyOp getter, JSStrictPropertyOp setter,
                                    uint32_t slot, unsigned attrs,
                                    unsigned flags, int shortid);
    static js::RawShape putProperty(JSContext *cx, JS::HandleObject obj,
                                    js::PropertyName *name,
                                    JSPropertyOp getter, JSStrictPropertyOp setter,
                                    uint32_t slot, unsigned attrs,
                                    unsigned flags, int shortid)
    {
        JS::RootedId id(cx, js::NameToId(name));
        return putProperty(cx, obj, id, getter, setter, slot, attrs, flags, shortid);
    }

    
    static js::RawShape changeProperty(JSContext *cx, js::HandleObject obj,
                                       js::HandleShape shape, unsigned attrs, unsigned mask,
                                       JSPropertyOp getter, JSStrictPropertyOp setter);

    static inline bool changePropertyAttributes(JSContext *cx, js::HandleObject obj,
                                                js::HandleShape shape, unsigned attrs);

    
    bool removeProperty(JSContext *cx, jsid id);

    
    static void clear(JSContext *cx, js::HandleObject obj);

    static inline JSBool lookupGeneric(JSContext *cx, js::HandleObject obj,
                                       js::HandleId id,
                                       js::MutableHandleObject objp, js::MutableHandleShape propp);
    static inline JSBool lookupProperty(JSContext *cx, js::HandleObject obj,
                                        js::PropertyName *name,
                                        js::MutableHandleObject objp, js::MutableHandleShape propp);
    static inline JSBool lookupElement(JSContext *cx, js::HandleObject obj,
                                       uint32_t index,
                                       js::MutableHandleObject objp, js::MutableHandleShape propp);
    static inline JSBool lookupSpecial(JSContext *cx, js::HandleObject obj,
                                       js::SpecialId sid,
                                       js::MutableHandleObject objp, js::MutableHandleShape propp);

    static inline JSBool defineGeneric(JSContext *cx, js::HandleObject obj,
                                       js::HandleId id, js::HandleValue value,
                                       JSPropertyOp getter = JS_PropertyStub,
                                       JSStrictPropertyOp setter = JS_StrictPropertyStub,
                                       unsigned attrs = JSPROP_ENUMERATE);
    static inline JSBool defineProperty(JSContext *cx, js::HandleObject obj,
                                        js::PropertyName *name, js::HandleValue value,
                                        JSPropertyOp getter = JS_PropertyStub,
                                        JSStrictPropertyOp setter = JS_StrictPropertyStub,
                                        unsigned attrs = JSPROP_ENUMERATE);

    static inline JSBool defineElement(JSContext *cx, js::HandleObject obj,
                                       uint32_t index, js::HandleValue value,
                                       JSPropertyOp getter = JS_PropertyStub,
                                       JSStrictPropertyOp setter = JS_StrictPropertyStub,
                                       unsigned attrs = JSPROP_ENUMERATE);
    static inline JSBool defineSpecial(JSContext *cx, js::HandleObject obj,
                                       js::SpecialId sid, js::HandleValue value,
                                       JSPropertyOp getter = JS_PropertyStub,
                                       JSStrictPropertyOp setter = JS_StrictPropertyStub,
                                       unsigned attrs = JSPROP_ENUMERATE);

    static inline JSBool getGeneric(JSContext *cx, js::HandleObject obj,
                                    js::HandleObject receiver,
                                    js::HandleId id, js::MutableHandleValue vp);
    static inline JSBool getGenericNoGC(JSContext *cx, JSObject *obj, JSObject *receiver,
                                        jsid id, js::Value *vp);
    static inline JSBool getProperty(JSContext *cx, js::HandleObject obj,
                                     js::HandleObject receiver,
                                     js::PropertyName *name, js::MutableHandleValue vp);
    static inline JSBool getPropertyNoGC(JSContext *cx, JSObject *obj, JSObject *receiver,
                                         js::PropertyName *name, js::Value *vp);
    static inline JSBool getElement(JSContext *cx, js::HandleObject obj,
                                    js::HandleObject receiver,
                                    uint32_t index, js::MutableHandleValue vp);
    static inline JSBool getElementNoGC(JSContext *cx, JSObject *obj, JSObject *receiver,
                                        uint32_t index, js::Value *vp);
    

    static inline JSBool getElementIfPresent(JSContext *cx, js::HandleObject obj,
                                             js::HandleObject receiver, uint32_t index,
                                             js::MutableHandleValue vp, bool *present);
    static inline JSBool getSpecial(JSContext *cx, js::HandleObject obj,
                                    js::HandleObject receiver, js::SpecialId sid,
                                    js::MutableHandleValue vp);

    static inline JSBool setGeneric(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                                    js::HandleId id,
                                    js::MutableHandleValue vp, JSBool strict);
    static inline JSBool setProperty(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                                     js::PropertyName *name,
                                     js::MutableHandleValue vp, JSBool strict);
    static inline JSBool setElement(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                                    uint32_t index,
                                    js::MutableHandleValue vp, JSBool strict);
    static inline JSBool setSpecial(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                                    js::SpecialId sid,
                                    js::MutableHandleValue vp, JSBool strict);

    static JSBool nonNativeSetProperty(JSContext *cx, js::HandleObject obj,
                                       js::HandleId id, js::MutableHandleValue vp, JSBool strict);
    static JSBool nonNativeSetElement(JSContext *cx, js::HandleObject obj,
                                      uint32_t index, js::MutableHandleValue vp, JSBool strict);

    static inline JSBool getGenericAttributes(JSContext *cx, js::HandleObject obj,
                                              js::HandleId id, unsigned *attrsp);
    static inline JSBool getPropertyAttributes(JSContext *cx, js::HandleObject obj,
                                               js::PropertyName *name, unsigned *attrsp);
    static inline JSBool getElementAttributes(JSContext *cx, js::HandleObject obj,
                                              uint32_t index, unsigned *attrsp);
    static inline JSBool getSpecialAttributes(JSContext *cx, js::HandleObject obj,
                                              js::SpecialId sid, unsigned *attrsp);

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
                                      js::MutableHandleValue rval, bool strict);
    static inline bool deleteElement(JSContext *cx, js::HandleObject obj,
                                     uint32_t index,
                                     js::MutableHandleValue rval, bool strict);
    static inline bool deleteSpecial(JSContext *cx, js::HandleObject obj,
                                     js::HandleSpecialId sid,
                                     js::MutableHandleValue rval, bool strict);
    static bool deleteByValue(JSContext *cx, js::HandleObject obj,
                              const js::Value &property, js::MutableHandleValue rval, bool strict);

    static inline bool enumerate(JSContext *cx, JS::HandleObject obj, JSIterateOp iterop,
                                 JS::MutableHandleValue statep, JS::MutableHandleId idp);
    static inline bool defaultValue(JSContext *cx, js::HandleObject obj,
                                    JSType hint, js::MutableHandleValue vp);
    static inline JSObject *thisObject(JSContext *cx, js::HandleObject obj);

    static bool thisObject(JSContext *cx, const js::Value &v, js::Value *vp);

    static bool swap(JSContext *cx, JS::HandleObject a, JS::HandleObject b);

    inline void initArrayClass();

    



























    
    inline bool isArray() const;
    inline bool isArguments() const;
    inline bool isArrayBuffer() const;
    inline bool isDataView() const;
    inline bool isDate() const;
    inline bool isElementIterator() const;
    inline bool isError() const;
    inline bool isFunction() const;
    inline bool isGenerator() const;
    inline bool isGlobal() const;
    inline bool isMapIterator() const;
    inline bool isModule() const;
    inline bool isObject() const;
    inline bool isPrimitive() const;
    inline bool isPropertyIterator() const;
    inline bool isProxy() const;
    inline bool isRegExp() const;
    inline bool isRegExpStatics() const;
    inline bool isScope() const;
    inline bool isScript() const;
    inline bool isSetIterator() const;
    inline bool isStopIteration() const;
    inline bool isTypedArray() const;
    inline bool isWeakMap() const;

    
    inline bool isBlock() const;
    inline bool isCall() const;
    inline bool isDeclEnv() const;
    inline bool isNestedScope() const;
    inline bool isWith() const;
    inline bool isClonedBlock() const;
    inline bool isStaticBlock() const;

    
    inline bool isBoolean() const;
    inline bool isNumber() const;
    inline bool isString() const;

    
    inline bool isNormalArguments() const;
    inline bool isStrictArguments() const;

    
    inline bool isDebugScope() const;
    inline bool isWrapper() const;
    inline bool isFunctionProxy() const;
    inline bool isCrossCompartmentWrapper() const;

    inline js::ArgumentsObject &asArguments();
    inline js::ArrayBufferObject &asArrayBuffer();
    inline const js::ArgumentsObject &asArguments() const;
    inline js::BlockObject &asBlock();
    inline js::BooleanObject &asBoolean();
    inline js::CallObject &asCall();
    inline js::ClonedBlockObject &asClonedBlock();
    inline js::DataViewObject &asDataView();
    inline js::DeclEnvObject &asDeclEnv();
    inline js::DebugScopeObject &asDebugScope();
    inline js::GlobalObject &asGlobal();
    inline js::MapObject &asMap();
    inline js::MapIteratorObject &asMapIterator();
    inline js::Module &asModule();
    inline js::NestedScopeObject &asNestedScope();
    inline js::NormalArgumentsObject &asNormalArguments();
    inline js::NumberObject &asNumber();
    inline js::PropertyIteratorObject &asPropertyIterator();
    inline const js::PropertyIteratorObject &asPropertyIterator() const;
    inline js::RegExpObject &asRegExp();
    inline js::ScopeObject &asScope();
    inline js::SetObject &asSet();
    inline js::SetIteratorObject &asSetIterator();
    inline js::StrictArgumentsObject &asStrictArguments();
    inline js::StaticBlockObject &asStaticBlock();
    inline js::StringObject &asString();
    inline js::WithObject &asWith();

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

#define JSSLOT_FREE(clasp)  JSCLASS_RESERVED_SLOTS(clasp)

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
js_GetClassObject(JSContext *cx, js::RawObject obj, JSProtoKey key,
                  js::MutableHandleObject objp);





extern JSProtoKey
js_IdentifyClassPrototype(JSObject *obj);





bool
js_FindClassObject(JSContext *cx, JSProtoKey protoKey, js::MutableHandleValue vp,
                   js::Class *clasp = NULL);





extern js::RawShape
js_AddNativeProperty(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                     JSPropertyOp getter, JSStrictPropertyOp setter, uint32_t slot,
                     unsigned attrs, unsigned flags, int shortid);

extern JSBool
js_DefineOwnProperty(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                     const JS::Value &descriptor, JSBool *bp);

namespace js {





enum NewObjectKind {
    
    GenericObject,

    




    SingletonObject,

    




    MaybeSingletonObject
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




const unsigned DNP_CACHE_RESULT = 1;   
const unsigned DNP_DONT_PURGE   = 2;   
const unsigned DNP_UNQUALIFIED  = 4;   


const unsigned DNP_SKIP_TYPE    = 8;   




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


const unsigned JSGET_CACHE_RESULT = 1; 







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
GetOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id, PropertyDescriptor *desc);

bool
GetOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp);

bool
NewPropertyDescriptorObject(JSContext *cx, const PropertyDescriptor *desc, MutableHandleValue vp);

extern JSBool
GetMethod(JSContext *cx, HandleObject obj, HandleId id, unsigned getHow, MutableHandleValue vp);

inline bool
GetMethod(JSContext *cx, HandleObject obj, PropertyName *name, unsigned getHow, MutableHandleValue vp)
{
    Rooted<jsid> id(cx, NameToId(name));
    return GetMethod(cx, obj, id, getHow, vp);
}





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

inline void
DestroyIdArray(FreeOp *fop, JSIdArray *ida);

extern bool
GetFirstArgumentAsObject(JSContext *cx, unsigned argc, Value *vp, const char *method,
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
