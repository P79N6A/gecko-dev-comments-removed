






#ifndef jsobj_h___
#define jsobj_h___









#include "jsapi.h"
#include "jsatom.h"
#include "jsclass.h"
#include "jsfriendapi.h"
#include "jsinfer.h"
#include "jspubtd.h"
#include "jsprvtd.h"
#include "jslock.h"

#include "gc/Barrier.h"
#include "gc/Heap.h"

#include "vm/ObjectImpl.h"
#include "vm/String.h"

namespace js {

class AutoPropDescArrayRooter;
class BaseProxyHandler;
class CallObject;
struct GCMarker;
struct NativeIterator;

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






extern JS_FRIEND_API(JSBool)
LookupProperty(JSContext *cx, HandleObject obj, HandleId id, MutableHandleObject objp,
               MutableHandleShape propp);

inline bool
LookupProperty(JSContext *cx, HandleObject obj, PropertyName *name,
               MutableHandleObject objp, MutableHandleShape propp)
{
    Rooted<jsid> id(cx, NameToId(name));
    return LookupProperty(cx, obj, id, objp, propp);
}

extern JS_FRIEND_API(JSBool)
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
extern Class JSONClass;
extern Class MapIteratorClass;
extern Class MathClass;
extern Class NumberClass;
extern Class NormalArgumentsObjectClass;
extern Class ObjectClass;
extern Class ProxyClass;
extern Class RegExpClass;
extern Class RegExpStaticsClass;
extern Class SetIteratorClass;
extern Class SlowArrayClass;
extern Class StopIterationClass;
extern Class StringClass;
extern Class StrictArgumentsObjectClass;
extern Class WeakMapClass;
extern Class WithClass;
extern Class XMLFilterClass;

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











struct JSObject : public js::ObjectImpl
{
  private:
    friend struct js::Shape;
    friend struct js::GCMarker;
    friend class  js::NewObjectCache;

    
    js::types::TypeObject *makeLazyType(JSContext *cx);

  public:
    



    bool setLastProperty(JSContext *cx, js::Shape *shape);

    
    inline void setLastPropertyInfallible(js::Shape *shape);

    
    static inline JSObject *create(JSContext *cx,
                                   js::gc::AllocKind kind,
                                   js::HandleShape shape,
                                   js::HandleTypeObject type,
                                   js::HeapSlot *slots);

    
    static inline JSObject *createDenseArray(JSContext *cx,
                                             js::gc::AllocKind kind,
                                             js::HandleShape shape,
                                             js::HandleTypeObject type,
                                             uint32_t length);

    




    inline void removeLastProperty(JSContext *cx);
    inline bool canRemoveLastProperty();

    



    bool setSlotSpan(JSContext *cx, uint32_t span);

    
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

    bool generateOwnShape(JSContext *cx, js::Shape *newShape = NULL) {
        return replaceWithNewEquivalentShape(cx, lastProperty(), newShape);
    }

  private:
    js::Shape *replaceWithNewEquivalentShape(JSContext *cx, js::Shape *existingShape,
                                             js::Shape *newShape = NULL);

    enum GenerateShape {
        GENERATE_NONE,
        GENERATE_SHAPE
    };

    bool setFlag(JSContext *cx,  uint32_t flag,
                 GenerateShape generateShape = GENERATE_NONE);

  public:
    inline bool nativeEmpty() const;

    bool shadowingShapeChange(JSContext *cx, const js::Shape &shape);

    
    inline bool isIndexed() const;

    inline uint32_t propertyCount() const;

    inline bool hasShapeTable() const;

    inline size_t computedSizeOfThisSlotsElements() const;

    inline void sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf,
                                    size_t *slotsSize, size_t *elementsSize,
                                    size_t *miscSize) const;

    bool hasIdempotentProtoChain() const;

    static const uint32_t MAX_FIXED_SLOTS = 16;

  public:

    

    
    inline bool isFixedSlot(size_t slot);

    
    inline size_t dynamicSlotIndex(size_t slot);

    
    inline const js::HeapSlot *getRawSlots();

    




    bool growSlots(JSContext *cx, uint32_t oldCount, uint32_t newCount);
    void shrinkSlots(JSContext *cx, uint32_t oldCount, uint32_t newCount);

    bool hasDynamicSlots() const { return slots != NULL; }

  protected:
    inline bool updateSlotsForSpan(JSContext *cx, size_t oldSpan, size_t newSpan);

  public:
    



    inline void prepareSlotRangeForOverwrite(size_t start, size_t end);
    inline void prepareElementRangeForOverwrite(size_t start, size_t end);

    void rollbackProperties(JSContext *cx, uint32_t slotSpan);

    inline void nativeSetSlot(unsigned slot, const js::Value &value);
    inline void nativeSetSlotWithType(JSContext *cx, js::Shape *shape, const js::Value &value);

    inline const js::Value &getReservedSlot(unsigned index) const;
    inline js::HeapSlot &getReservedSlotRef(unsigned index);
    inline void initReservedSlot(unsigned index, const js::Value &v);
    inline void setReservedSlot(unsigned index, const js::Value &v);

    



    static inline bool setSingletonType(JSContext *cx, js::HandleObject obj);

    inline js::types::TypeObject *getType(JSContext *cx);

    const js::HeapPtr<js::types::TypeObject> &typeFromGC() const {
        
        return type_;
    }

    inline void setType(js::types::TypeObject *newType);

    js::types::TypeObject *getNewType(JSContext *cx, JSFunction *fun = NULL,
                                      bool isDOM = false);

#ifdef DEBUG
    bool hasNewType(js::types::TypeObject *newType);
#endif

    




    inline bool setIteratedSingleton(JSContext *cx);

    



    bool setNewTypeUnknown(JSContext *cx);

    
    bool splicePrototype(JSContext *cx, JSObject *proto);

    



    bool shouldSplicePrototype(JSContext *cx);

    

























    
    inline JSObject *getParent() const;
    static bool setParent(JSContext *cx, js::HandleObject obj, js::HandleObject newParent);

    




    inline JSObject *enclosingScope();

    inline js::GlobalObject &global() const;

    
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
    void shrinkElements(JSContext *cx, unsigned cap);
    inline void setDynamicElements(js::ObjectElements *header);

    



    bool allocateSlowArrayElements(JSContext *cx);

    inline uint32_t getArrayLength() const;
    inline void setArrayLength(JSContext *cx, uint32_t length);

    inline uint32_t getDenseArrayCapacity();
    inline void setDenseArrayLength(uint32_t length);
    inline void setDenseArrayInitializedLength(uint32_t length);
    inline void ensureDenseArrayInitializedLength(JSContext *cx, unsigned index, unsigned extra);
    inline void setDenseArrayElement(unsigned idx, const js::Value &val);
    inline void initDenseArrayElement(unsigned idx, const js::Value &val);
    inline void setDenseArrayElementWithType(JSContext *cx, unsigned idx, const js::Value &val);
    inline void initDenseArrayElementWithType(JSContext *cx, unsigned idx, const js::Value &val);
    inline void copyDenseArrayElements(unsigned dstStart, const js::Value *src, unsigned count);
    inline void initDenseArrayElements(unsigned dstStart, const js::Value *src, unsigned count);
    inline void moveDenseArrayElements(unsigned dstStart, unsigned srcStart, unsigned count);
    inline void moveDenseArrayElementsUnbarriered(unsigned dstStart, unsigned srcStart, unsigned count);
    inline bool denseArrayHasInlineSlots() const;

    
    inline void markDenseArrayNotPacked(JSContext *cx);

    






    enum EnsureDenseResult { ED_OK, ED_FAILED, ED_SPARSE };
    inline EnsureDenseResult ensureDenseArrayElements(JSContext *cx, unsigned index, unsigned extra);

    



    bool willBeSparseDenseArray(unsigned requiredCapacity, unsigned newElementsHint);

    static bool makeDenseArraySlow(JSContext *cx, js::HandleObject obj);

    




    bool arrayGetOwnDataElement(JSContext *cx, size_t i, js::Value *vp);

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

    



    friend struct JSFunction;

    inline JSFunction *toFunction();
    inline const JSFunction *toFunction() const;

  public:
    



    static const uint32_t ITER_CLASS_NFIXED_SLOTS = 1;

    



    






  private:
    static const uint32_t JSSLOT_NAME_PREFIX          = 0;   
    static const uint32_t JSSLOT_NAME_URI             = 1;   

    static const uint32_t JSSLOT_NAMESPACE_DECLARED   = 2;

    static const uint32_t JSSLOT_QNAME_LOCAL_NAME     = 2;

  public:
    static const uint32_t NAMESPACE_CLASS_RESERVED_SLOTS = 3;
    static const uint32_t QNAME_CLASS_RESERVED_SLOTS     = 3;

    inline JSLinearString *getNamePrefix() const;
    inline jsval getNamePrefixVal() const;
    inline void setNamePrefix(JSLinearString *prefix);
    inline void clearNamePrefix();

    inline JSLinearString *getNameURI() const;
    inline jsval getNameURIVal() const;
    inline void setNameURI(JSLinearString *uri);

    inline jsval getNamespaceDeclared() const;
    inline void setNamespaceDeclared(jsval decl);

    inline JSAtom *getQNameLocalName() const;
    inline jsval getQNameLocalNameVal() const;
    inline void setQNameLocalName(JSAtom *name);

    


    inline bool isCallable();

    inline void finish(js::FreeOp *fop);
    JS_ALWAYS_INLINE void finalize(js::FreeOp *fop);

    static inline bool hasProperty(JSContext *cx, js::HandleObject obj,
                                   js::HandleId id, bool *foundp, unsigned flags = 0);

    






    bool allocSlot(JSContext *cx, uint32_t *slotp);
    void freeSlot(JSContext *cx, uint32_t slot);

  public:
    static bool reportReadOnly(JSContext *cx, jsid id, unsigned report = JSREPORT_ERROR);
    bool reportNotConfigurable(JSContext* cx, jsid id, unsigned report = JSREPORT_ERROR);
    bool reportNotExtensible(JSContext *cx, unsigned report = JSREPORT_ERROR);

    





    bool callMethod(JSContext *cx, js::HandleId id, unsigned argc, js::Value *argv,
                    js::MutableHandleValue vp);

  private:
    js::Shape *getChildProperty(JSContext *cx, js::Shape *parent, js::StackShape &child);

  protected:
    






    js::Shape *addPropertyInternal(JSContext *cx, jsid id,
                                   JSPropertyOp getter, JSStrictPropertyOp setter,
                                   uint32_t slot, unsigned attrs,
                                   unsigned flags, int shortid, js::Shape **spp,
                                   bool allowDictionary);

  private:
    bool toDictionaryMode(JSContext *cx);

    struct TradeGutsReserved;
    static bool ReserveForTradeGuts(JSContext *cx, JSObject *a, JSObject *b,
                                    TradeGutsReserved &reserved);

    static void TradeGuts(JSContext *cx, JSObject *a, JSObject *b,
                          TradeGutsReserved &reserved);

  public:
    
    js::Shape *addProperty(JSContext *cx, jsid id,
                           JSPropertyOp getter, JSStrictPropertyOp setter,
                           uint32_t slot, unsigned attrs,
                           unsigned flags, int shortid, bool allowDictionary = true);

    
    js::Shape *addDataProperty(JSContext *cx, jsid id, uint32_t slot, unsigned attrs) {
        JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
        return addProperty(cx, id, NULL, NULL, slot, attrs, 0, 0);
    }

    
    js::Shape *putProperty(JSContext *cx, jsid id,
                           JSPropertyOp getter, JSStrictPropertyOp setter,
                           uint32_t slot, unsigned attrs,
                           unsigned flags, int shortid);
    inline js::Shape *
    putProperty(JSContext *cx, js::PropertyName *name,
                JSPropertyOp getter, JSStrictPropertyOp setter,
                uint32_t slot, unsigned attrs, unsigned flags, int shortid) {
        return putProperty(cx, js::NameToId(name), getter, setter, slot, attrs, flags, shortid);
    }

    
    static js::Shape *changeProperty(JSContext *cx, js::HandleObject obj,
                                     js::Shape *shape, unsigned attrs, unsigned mask,
                                     JSPropertyOp getter, JSStrictPropertyOp setter);

    static inline bool changePropertyAttributes(JSContext *cx, js::HandleObject obj,
                                                js::Shape *shape, unsigned attrs);

    
    bool removeProperty(JSContext *cx, jsid id);

    
    void clear(JSContext *cx);

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
    static inline JSBool getProperty(JSContext *cx, js::HandleObject obj,
                                     js::HandleObject receiver,
                                     js::PropertyName *name, js::MutableHandleValue vp);
    static inline JSBool getElement(JSContext *cx, js::HandleObject obj,
                                    js::HandleObject receiver,
                                    uint32_t index, js::MutableHandleValue vp);
    

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
    static inline JSType typeOf(JSContext *cx, js::HandleObject obj);
    static inline JSObject *thisObject(JSContext *cx, js::HandleObject obj);

    static bool thisObject(JSContext *cx, const js::Value &v, js::Value *vp);

    bool swap(JSContext *cx, JSObject *other);

    inline void initArrayClass();

    



























    
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
#if JS_HAS_XML_SUPPORT
    inline bool isNamespace() const;
    inline bool isQName() const;
    inline bool isXML() const;
    inline bool isXMLId() const;
#endif

    
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
    inline js::NestedScopeObject &asNestedScope();
    inline js::NormalArgumentsObject &asNormalArguments();
    inline js::NumberObject &asNumber();
    inline js::PropertyIteratorObject &asPropertyIterator();
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

extern JSBool
js_HasOwnPropertyHelper(JSContext *cx, js::LookupGenericOp lookup, js::HandleObject obj,
                        js::HandleId id, js::MutableHandleValue rval);

extern JSBool
js_HasOwnProperty(JSContext *cx, js::LookupGenericOp lookup, js::HandleObject obj, js::HandleId id,
                  js::MutableHandleObject objp, js::MutableHandleShape propp);

extern JSBool
js_PropertyIsEnumerable(JSContext *cx, js::HandleObject obj, js::HandleId id, js::Value *vp);

extern JSFunctionSpec object_methods[];
extern JSFunctionSpec object_static_methods[];

namespace js {

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



extern JSObject *
js_CreateThisForFunctionWithProto(JSContext *cx, js::HandleObject callee, JSObject *proto);


extern JSObject *
js_CreateThisForFunction(JSContext *cx, js::HandleObject callee, bool newType);


extern JSObject *
js_CreateThis(JSContext *cx, js::Class *clasp, js::HandleObject callee);





extern js::Shape *
js_AddNativeProperty(JSContext *cx, js::HandleObject obj, jsid id,
                     JSPropertyOp getter, JSStrictPropertyOp setter, uint32_t slot,
                     unsigned attrs, unsigned flags, int shortid);

extern JSBool
js_DefineOwnProperty(JSContext *cx, js::HandleObject obj, js::HandleId id,
                     const js::Value &descriptor, JSBool *bp);

namespace js {




const unsigned DNP_CACHE_RESULT = 1;   
const unsigned DNP_DONT_PURGE   = 2;   
const unsigned DNP_UNQUALIFIED  = 4;   


const unsigned DNP_SKIP_TYPE    = 8;   




extern Shape *
DefineNativeProperty(JSContext *cx, HandleObject obj, HandleId id, HandleValue value,
                     PropertyOp getter, StrictPropertyOp setter, unsigned attrs,
                     unsigned flags, int shortid, unsigned defineHow = 0);

inline Shape *
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
LookupNameForSet(JSContext *cx, HandlePropertyName name, HandleObject scopeChain,
                 MutableHandleObject objp);

}

extern JSObject *
js_FindVariableScope(JSContext *cx, JSFunction **funp);


const unsigned JSGET_CACHE_RESULT = 1; 







extern JSBool
js_NativeGet(JSContext *cx, js::Handle<JSObject*> obj, js::Handle<JSObject*> pobj,
             js::Shape *shape, unsigned getHow, js::Value *vp);

extern JSBool
js_NativeSet(JSContext *cx, js::Handle<JSObject*> obj, js::Handle<JSObject*> receiver,
             js::Shape *shape, bool added, bool strict, js::Value *vp);

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
GetOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id, Value *vp);

bool
NewPropertyDescriptorObject(JSContext *cx, const PropertyDescriptor *desc, Value *vp);

extern JSBool
GetMethod(JSContext *cx, HandleObject obj, HandleId id, unsigned getHow, MutableHandleValue vp);

inline bool
GetMethod(JSContext *cx, HandleObject obj, PropertyName *name, unsigned getHow, MutableHandleValue vp)
{
    Rooted<jsid> id(cx, NameToId(name));
    return GetMethod(cx, obj, id, getHow, vp);
}





extern bool
HasDataProperty(JSContext *cx, HandleObject obj, jsid id, Value *vp);

inline bool
HasDataProperty(JSContext *cx, HandleObject obj, PropertyName *name, Value *vp)
{
    return HasDataProperty(cx, obj, NameToId(name), vp);
}

extern JSBool
CheckAccess(JSContext *cx, JSObject *obj, HandleId id, JSAccessMode mode,
            MutableHandleValue v, unsigned *attrsp);

} 

extern bool
js_IsDelegate(JSContext *cx, JSObject *obj, const js::Value &v);





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

} 

extern void
js_GetObjectSlotName(JSTracer *trc, char *buf, size_t bufsize);

extern JSBool
js_ReportGetterOnlyAssignment(JSContext *cx);

extern unsigned
js_InferFlags(JSContext *cx, unsigned defaultFlags);


JSBool
js_Object(JSContext *cx, unsigned argc, js::Value *vp);








extern JS_FRIEND_API(bool)
js_GetClassPrototype(JSContext *cx, JSProtoKey protoKey, js::MutableHandleObject protop,
                     js::Class *clasp = NULL);

namespace js {

extern bool
SetProto(JSContext *cx, HandleObject obj, HandleObject proto, bool checkForCycles);

extern JSString *
obj_toStringHelper(JSContext *cx, JSObject *obj);

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
