







































#ifndef jsobj_h___
#define jsobj_h___








#include "jsapi.h"
#include "jshash.h" 
#include "jspubtd.h"
#include "jsprvtd.h"
#include "jslock.h"
#include "jsvalue.h"
#include "jsvector.h"

namespace js {

class JSProxyHandler;
class AutoPropDescArrayRooter;

namespace mjit {
class Compiler;
}

static inline PropertyOp
CastAsPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(PropertyOp, object);
}

static inline JSPropertyOp
CastAsJSPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(JSPropertyOp, object);
}

inline JSObject *
CastAsObject(PropertyOp op)
{
    return JS_FUNC_TO_DATA_PTR(JSObject *, op);
}

inline Value
CastAsObjectJsval(PropertyOp op)
{
    return ObjectOrNullValue(CastAsObject(op));
}

} 





struct PropDesc {
    friend class js::AutoPropDescArrayRooter;

    PropDesc();

  public:
    
    bool initialize(JSContext* cx, jsid id, const js::Value &v);

    
    bool isAccessorDescriptor() const {
        return hasGet || hasSet;
    }

    
    bool isDataDescriptor() const {
        return hasValue || hasWritable;
    }

    
    bool isGenericDescriptor() const {
        return !isAccessorDescriptor() && !isDataDescriptor();
    }

    bool configurable() const {
        return (attrs & JSPROP_PERMANENT) == 0;
    }

    bool enumerable() const {
        return (attrs & JSPROP_ENUMERATE) != 0;
    }

    bool writable() const {
        return (attrs & JSPROP_READONLY) == 0;
    }

    JSObject* getterObject() const {
        return get.isUndefined() ? NULL : &get.toObject();
    }
    JSObject* setterObject() const {
        return set.isUndefined() ? NULL : &set.toObject();
    }

    const js::Value &getterValue() const {
        return get;
    }
    const js::Value &setterValue() const {
        return set;
    }

    js::PropertyOp getter() const {
        return js::CastAsPropertyOp(getterObject());
    }
    js::PropertyOp setter() const {
        return js::CastAsPropertyOp(setterObject());
    }

    static void traceDescriptorArray(JSTracer* trc, JSObject* obj);
    static void finalizeDescriptorArray(JSContext* cx, JSObject* obj);

    js::Value pd;
    jsid id;
    js::Value value, get, set;

    
    uint8 attrs;

    
    bool hasGet : 1;
    bool hasSet : 1;
    bool hasValue : 1;
    bool hasWritable : 1;
    bool hasEnumerable : 1;
    bool hasConfigurable : 1;
};

namespace js {

typedef Vector<PropDesc, 1> PropDescArray;

} 

struct JSObjectMap {
    static JS_FRIEND_DATA(const JSObjectMap) sharedNonNative;

    uint32 shape;       
    uint32 slotSpan;    

    explicit JSObjectMap(uint32 shape) : shape(shape), slotSpan(0) {}
    JSObjectMap(uint32 shape, uint32 slotSpan) : shape(shape), slotSpan(slotSpan) {}

    enum { INVALID_SHAPE = 0x8fffffff, SHAPELESS = 0xffffffff };

    bool isNative() const { return this != &sharedNonNative; }

  private:
    
    JSObjectMap(JSObjectMap &);
    void operator=(JSObjectMap &);
};








extern JS_FRIEND_API(JSBool)
js_LookupProperty(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                  JSProperty **propp);

extern JSBool
js_DefineProperty(JSContext *cx, JSObject *obj, jsid id, const js::Value *value,
                  js::PropertyOp getter, js::PropertyOp setter, uintN attrs);

extern JSBool
js_GetProperty(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSBool
js_SetProperty(JSContext *cx, JSObject *obj, jsid id, js::Value *vp, JSBool strict);

extern JSBool
js_GetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);

extern JSBool
js_SetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);

extern JSBool
js_DeleteProperty(JSContext *cx, JSObject *obj, jsid id, js::Value *rval, JSBool strict);

extern JS_FRIEND_API(JSBool)
js_Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
             js::Value *statep, jsid *idp);

extern JSType
js_TypeOf(JSContext *cx, JSObject *obj);

struct NativeIterator;

const uint32 JS_INITIAL_NSLOTS = 3;








const uint32 JSSLOT_PRIVATE = 0;

struct JSFunction;































struct JSObject {
    



    friend class js::TraceRecorder;

    










    union {
        js::Shape       *lastProp;
        JSObjectMap     *map;
    };

    js::Class           *clasp;

  private:
    inline void setLastProperty(const js::Shape *shape);
    inline void removeLastProperty();

#ifdef DEBUG
    void checkShapeConsistency();
#endif

  public:
    inline const js::Shape *lastProperty() const;

    inline js::Shape **nativeSearch(jsid id, bool adding = false);
    inline const js::Shape *nativeLookup(jsid id);

    inline bool nativeContains(jsid id);
    inline bool nativeContains(const js::Shape &shape);

    enum {
        DELEGATE        = 0x01,
        SYSTEM          = 0x02,
        NOT_EXTENSIBLE  = 0x04,
        BRANDED         = 0x08,
        GENERIC         = 0x10,
        METHOD_BARRIER  = 0x20,
        INDEXED         =  0x40,
        OWN_SHAPE       =  0x80,
        BOUND_FUNCTION  = 0x100
    };

    



    enum {
        NSLOTS_BITS     = 29,
        NSLOTS_LIMIT    = JS_BIT(NSLOTS_BITS)
    };

    uint32      flags;                      
    uint32      objShape;                   

    JSObject    *proto;                     
    JSObject    *parent;                    
    js::Value   *dslots;                    

    
    js::EmptyShape *emptyShape;

    js::Value   fslots[JS_INITIAL_NSLOTS];  
#ifdef JS_THREADSAFE
    JSTitle     title;
#endif

    






    inline bool canProvideEmptyShape(js::Class *clasp);
    inline js::EmptyShape *getEmptyShape(JSContext *cx, js::Class *aclasp);

    bool isNative() const       { return map->isNative(); }

    js::Class *getClass() const { return clasp; }
    JSClass *getJSClass() const { return Jsvalify(clasp); }

    bool hasClass(const js::Class *c) const {
        return c == clasp;
    }

    const js::ObjectOps *getOps() const {
        return &getClass()->ops;
    }

    inline void trace(JSTracer *trc);

    uint32 shape() const {
        JS_ASSERT(objShape != JSObjectMap::INVALID_SHAPE);
        return objShape;
    }

    bool isDelegate() const     { return !!(flags & DELEGATE); }
    void setDelegate()          { flags |= DELEGATE; }

    bool isBoundFunction() const { return !!(flags & BOUND_FUNCTION); }

    static void setDelegateNullSafe(JSObject *obj) {
        if (obj)
            obj->setDelegate();
    }

    bool isSystem() const       { return !!(flags & SYSTEM); }
    void setSystem()            { flags |= SYSTEM; }

    




    bool branded()              { return !!(flags & BRANDED); }

    bool brand(JSContext *cx, uint32 slot, js::Value v);
    bool unbrand(JSContext *cx);

    bool generic()              { return !!(flags & GENERIC); }
    void setGeneric()           { flags |= GENERIC; }

  private:
    void generateOwnShape(JSContext *cx);

    void setOwnShape(uint32 s)  { flags |= OWN_SHAPE; objShape = s; }
    void clearOwnShape()        { flags &= ~OWN_SHAPE; objShape = map->shape; }

  public:
    inline bool nativeEmpty() const;

    bool hasOwnShape() const    { return !!(flags & OWN_SHAPE); }

    void setMap(JSObjectMap *amap) {
        JS_ASSERT(!hasOwnShape());
        map = amap;
        objShape = map->shape;
    }

    void setSharedNonNativeMap() {
        setMap(const_cast<JSObjectMap *>(&JSObjectMap::sharedNonNative));
    }

    void deletingShapeChange(JSContext *cx, const js::Shape &shape);
    bool methodShapeChange(JSContext *cx, const js::Shape &shape);
    bool methodShapeChange(JSContext *cx, uint32 slot);
    void protoShapeChange(JSContext *cx);
    void shadowingShapeChange(JSContext *cx, const js::Shape &shape);
    bool globalObjectOwnShapeChange(JSContext *cx);

    void extensibleShapeChange(JSContext *cx) {
        
        generateOwnShape(cx);
    }

    







































    bool hasMethodBarrier()     { return !!(flags & METHOD_BARRIER); }
    void setMethodBarrier()     { flags |= METHOD_BARRIER; }

    





    bool brandedOrHasMethodBarrier() { return !!(flags & (BRANDED | METHOD_BARRIER)); }

    




    bool methodReadBarrier(JSContext *cx, const js::Shape &shape, js::Value *vp);

    






    bool methodWriteBarrier(JSContext *cx, const js::Shape &shape, const js::Value &v);
    bool methodWriteBarrier(JSContext *cx, uint32 slot, const js::Value &v);

    bool isIndexed() const          { return !!(flags & INDEXED); }
    void setIndexed()               { flags |= INDEXED; }

    




    inline bool inDictionaryMode() const;

    inline uint32 propertyCount() const;

    inline bool hasPropertyTable() const;

    uint32 numSlots(void) const {
        return dslots ? dslots[-1].toPrivateUint32() : uint32(JS_INITIAL_NSLOTS);
    }

    size_t slotsAndStructSize(uint32 nslots) const;
    size_t slotsAndStructSize() const { return slotsAndStructSize(numSlots()); }

  private:
    static size_t slotsToDynamicWords(size_t nslots) {
        JS_ASSERT(nslots > JS_INITIAL_NSLOTS);
        return nslots + 1 - JS_INITIAL_NSLOTS;
    }

    static size_t dynamicWordsToSlots(size_t nwords) {
        JS_ASSERT(nwords > 1);
        return nwords - 1 + JS_INITIAL_NSLOTS;
    }

  public:
    bool allocSlots(JSContext *cx, size_t nslots);
    bool growSlots(JSContext *cx, size_t nslots);
    void shrinkSlots(JSContext *cx, size_t nslots);

    















    bool ensureInstanceReservedSlots(JSContext *cx, size_t nreserved);

    




    bool ensureClassReservedSlotsForEmptyObject(JSContext *cx);

    inline bool ensureClassReservedSlots(JSContext *cx);

    uint32 slotSpan() const { return map->slotSpan; }

    bool containsSlot(uint32 slot) const { return slot < slotSpan(); }

    js::Value& getSlotRef(uintN slot) {
        return (slot < JS_INITIAL_NSLOTS)
               ? fslots[slot]
               : (JS_ASSERT(slot < dslots[-1].toPrivateUint32()),
                  dslots[slot - JS_INITIAL_NSLOTS]);
    }

    const js::Value &getSlot(uintN slot) const {
        return (slot < JS_INITIAL_NSLOTS)
               ? fslots[slot]
               : (JS_ASSERT(slot < dslots[-1].toPrivateUint32()),
                  dslots[slot - JS_INITIAL_NSLOTS]);
    }

    void setSlot(uintN slot, const js::Value &value) {
        if (slot < JS_INITIAL_NSLOTS) {
            fslots[slot] = value;
        } else {
            JS_ASSERT(slot < dslots[-1].toPrivateUint32());
            dslots[slot - JS_INITIAL_NSLOTS] = value;
        }
    }

    inline const js::Value &lockedGetSlot(uintN slot) const;
    inline void lockedSetSlot(uintN slot, const js::Value &value);

    





    inline js::Value getSlotMT(JSContext *cx, uintN slot);
    inline void setSlotMT(JSContext *cx, uintN slot, const js::Value &value);

    inline js::Value getReservedSlot(uintN index) const;

    
    inline void updateShape(JSContext *cx);
    inline void updateFlags(const js::Shape *shape, bool isDefinitelyAtom = false);

    
    inline void extend(JSContext *cx, const js::Shape *shape, bool isDefinitelyAtom = false);

    JSObject *getProto() const  { return proto; }
    void clearProto()           { proto = NULL; }

    void setProto(JSObject *newProto) {
#ifdef DEBUG
        for (JSObject *obj = newProto; obj; obj = obj->getProto())
            JS_ASSERT(obj != this);
#endif
        setDelegateNullSafe(newProto);
        proto = newProto;
    }

    JSObject *getParent() const {
        return parent;
    }

    void clearParent() {
        parent = NULL;
    }

    void setParent(JSObject *newParent) {
#ifdef DEBUG
        for (JSObject *obj = newParent; obj; obj = obj->getParent())
            JS_ASSERT(obj != this);
#endif
        setDelegateNullSafe(newParent);
        parent = newParent;
    }

    JSObject *getGlobal() const;

    bool isGlobal() const {
        return !!(getClass()->flags & JSCLASS_IS_GLOBAL);
    }

    void *getPrivate() const {
        JS_ASSERT(getClass()->flags & JSCLASS_HAS_PRIVATE);
        return *(void **)&fslots[JSSLOT_PRIVATE];
    }

    void setPrivate(void *data) {
        JS_ASSERT(getClass()->flags & JSCLASS_HAS_PRIVATE);
        *(void **)&fslots[JSSLOT_PRIVATE] = data;
    }


    



  private:
    





    bool sealOrFreeze(JSContext *cx, bool freeze = false);

  public:
    bool isExtensible() const { return !(flags & NOT_EXTENSIBLE); }
    bool preventExtensions(JSContext *cx, js::AutoIdVector *props);
    
    
    inline bool seal(JSContext *cx) { return sealOrFreeze(cx); }
    
    bool freeze(JSContext *cx) { return sealOrFreeze(cx, true); }
        
    



  private:
    static const uint32 JSSLOT_PRIMITIVE_THIS = JSSLOT_PRIVATE;

  public:
    inline const js::Value &getPrimitiveThis() const;
    inline void setPrimitiveThis(const js::Value &pthis);

    



    
    static const uint32 JSSLOT_ARRAY_LENGTH = JSSLOT_PRIVATE;

    static const uint32 JSSLOT_DENSE_ARRAY_CAPACITY = JSSLOT_PRIVATE + 1;

    
    
    
    inline void staticAssertArrayLengthIsInPrivateSlot();

  public:
    static const uint32 DENSE_ARRAY_CLASS_RESERVED_SLOTS = 3;

    inline uint32 getArrayLength() const;
    inline void setArrayLength(uint32 length);

    inline uint32 getDenseArrayCapacity() const;
    inline void setDenseArrayCapacity(uint32 capacity);

    inline const js::Value &getDenseArrayElement(uint32 i) const;
    inline js::Value *addressOfDenseArrayElement(uint32 i);
    inline void setDenseArrayElement(uint32 i, const js::Value &v);

    inline js::Value *getDenseArrayElements() const;   
    bool growDenseArrayElements(JSContext *cx, uint32 oldcap, uint32 newcap);
    bool ensureDenseArrayElements(JSContext *cx, uint32 newcap);
    bool shrinkDenseArrayElements(JSContext *cx, uint32 newcap);
    inline void freeDenseArrayElements(JSContext *cx);

    inline void voidDenseOnlyArraySlots();  

    JSBool makeDenseArraySlow(JSContext *cx);

    



  private:
    




















    static const uint32 JSSLOT_ARGS_DATA   = JSSLOT_PRIVATE + 2;

  public:
    
    static const uint32 JSSLOT_ARGS_LENGTH = JSSLOT_PRIVATE + 1;
    static const uint32 ARGS_CLASS_RESERVED_SLOTS = 2;
    static const uint32 ARGS_FIRST_FREE_SLOT = JSSLOT_PRIVATE + ARGS_CLASS_RESERVED_SLOTS + 1;

    
    static const uint32 ARGS_LENGTH_OVERRIDDEN_BIT = 0x1;
    static const uint32 ARGS_PACKED_BITS_COUNT = 1;

    


    inline void setArgsLength(uint32 argc);

    



    inline uint32 getArgsInitialLength() const;

    inline void setArgsLengthOverridden();
    inline bool isArgsLengthOverridden() const;

    inline js::ArgumentsData *getArgsData() const;
    inline void setArgsData(js::ArgumentsData *data);

    inline const js::Value &getArgsCallee() const;
    inline void setArgsCallee(const js::Value &callee);

    inline const js::Value &getArgsElement(uint32 i) const;
    inline js::Value *addressOfArgsElement(uint32 i) const;
    inline void setArgsElement(uint32 i, const js::Value &v);

  private:
    



    static const uint32 JSSLOT_CALL_CALLEE = JSSLOT_PRIVATE + 1;
    static const uint32 JSSLOT_CALL_ARGUMENTS = JSSLOT_PRIVATE + 2;

  public:
    
    static const uint32 CALL_RESERVED_SLOTS = 2;

    inline JSObject &getCallObjCallee() const;
    inline JSFunction *getCallObjCalleeFunction() const; 
    inline void setCallObjCallee(JSObject &callee);

    inline const js::Value &getCallObjArguments() const;
    inline void setCallObjArguments(const js::Value &v);

    



    static const uint32 JSSLOT_DATE_UTC_TIME = JSSLOT_PRIVATE;

    




    static const uint32 JSSLOT_DATE_COMPONENTS_START = JSSLOT_PRIVATE + 1;

    static const uint32 JSSLOT_DATE_LOCAL_TIME = JSSLOT_PRIVATE + 1;
    static const uint32 JSSLOT_DATE_LOCAL_YEAR = JSSLOT_PRIVATE + 2;
    static const uint32 JSSLOT_DATE_LOCAL_MONTH = JSSLOT_PRIVATE + 3;
    static const uint32 JSSLOT_DATE_LOCAL_DATE = JSSLOT_PRIVATE + 4;
    static const uint32 JSSLOT_DATE_LOCAL_DAY = JSSLOT_PRIVATE + 5;
    static const uint32 JSSLOT_DATE_LOCAL_HOURS = JSSLOT_PRIVATE + 6;
    static const uint32 JSSLOT_DATE_LOCAL_MINUTES = JSSLOT_PRIVATE + 7;
    static const uint32 JSSLOT_DATE_LOCAL_SECONDS = JSSLOT_PRIVATE + 8;

    static const uint32 DATE_CLASS_RESERVED_SLOTS = 9;

    inline const js::Value &getDateUTCTime() const;
    inline void setDateUTCTime(const js::Value &pthis);

    



  private:
    friend struct JSFunction;
    friend class js::mjit::Compiler;

    



    static const uint32 JSSLOT_FLAT_CLOSURE_UPVARS = JSSLOT_PRIVATE + 1;

    



    static const uint32 JSSLOT_FUN_METHOD_ATOM = JSSLOT_PRIVATE + 1;
    static const uint32 JSSLOT_FUN_METHOD_OBJ  = JSSLOT_PRIVATE + 2;

    static const uint32 JSSLOT_BOUND_FUNCTION_THIS       = JSSLOT_PRIVATE + 1;
    static const uint32 JSSLOT_BOUND_FUNCTION_ARGS_COUNT = JSSLOT_PRIVATE + 2;

  public:
    static const uint32 FUN_CLASS_RESERVED_SLOTS = 2;

    inline JSFunction *getFunctionPrivate() const;

    inline js::Value *getFlatClosureUpvars() const;
    inline js::Value getFlatClosureUpvar(uint32 i) const;
    inline void setFlatClosureUpvars(js::Value *upvars);

    inline bool hasMethodObj(const JSObject& obj) const;
    inline void setMethodObj(JSObject& obj);

    inline bool initBoundFunction(JSContext *cx, const js::Value &thisArg,
                                  const js::Value *args, uintN argslen);

    inline JSObject *getBoundFunctionTarget() const;
    inline const js::Value &getBoundFunctionThis() const;
    inline const js::Value *getBoundFunctionArguments(uintN &argslen) const;

    



  private:
    static const uint32 JSSLOT_REGEXP_LAST_INDEX = JSSLOT_PRIVATE + 1;

  public:
    static const uint32 REGEXP_CLASS_RESERVED_SLOTS = 1;

    inline const js::Value &getRegExpLastIndex() const;
    inline void setRegExpLastIndex(const js::Value &v);
    inline void setRegExpLastIndex(jsdouble d);
    inline void zeroRegExpLastIndex();

    



    inline NativeIterator *getNativeIterator() const;
    inline void setNativeIterator(NativeIterator *);

    



    






  private:
    static const uint32 JSSLOT_NAME_PREFIX          = JSSLOT_PRIVATE;       
    static const uint32 JSSLOT_NAME_URI             = JSSLOT_PRIVATE + 1;   

    static const uint32 JSSLOT_NAMESPACE_DECLARED   = JSSLOT_PRIVATE + 2;

    static const uint32 JSSLOT_QNAME_LOCAL_NAME     = JSSLOT_PRIVATE + 2;

  public:
    static const uint32 NAMESPACE_CLASS_RESERVED_SLOTS = 3;
    static const uint32 QNAME_CLASS_RESERVED_SLOTS     = 3;

    inline jsval getNamePrefix() const;
    inline void setNamePrefix(jsval prefix);

    inline jsval getNameURI() const;
    inline void setNameURI(jsval uri);

    inline jsval getNamespaceDeclared() const;
    inline void setNamespaceDeclared(jsval decl);

    inline jsval getQNameLocalName() const;
    inline void setQNameLocalName(jsval decl);

    



    inline js::JSProxyHandler *getProxyHandler() const;
    inline const js::Value &getProxyPrivate() const;
    inline void setProxyPrivate(const js::Value &priv);

    


    inline JSObject *getWithThis() const;
    inline void setWithThis(JSObject *thisp);

    


    inline bool isCallable();

    
    inline void initCommon(js::Class *aclasp, JSObject *proto, JSObject *parent,
                           JSContext *cx);
    inline void init(js::Class *aclasp, JSObject *proto, JSObject *parent,
                     JSContext *cx);
    inline void init(js::Class *aclasp, JSObject *proto, JSObject *parent,
                     void *priv, JSContext *cx);
    inline void init(js::Class *aclasp, JSObject *proto, JSObject *parent,
                     const js::Value &privateSlotValue, JSContext *cx);

    inline void finish(JSContext *cx);

    



    inline void initSharingEmptyShape(js::Class *clasp,
                                      JSObject *proto,
                                      JSObject *parent,
                                      const js::Value &privateSlotValue,
                                      JSContext *cx);
    inline void initSharingEmptyShape(js::Class *clasp,
                                      JSObject *proto,
                                      JSObject *parent,
                                      void *priv,
                                      JSContext *cx);

    inline bool hasSlotsArray() const { return !!dslots; }

    
    inline void freeSlotsArray(JSContext *cx);

    inline bool hasProperty(JSContext *cx, jsid id, bool *foundp, uintN flags = 0);

    bool allocSlot(JSContext *cx, uint32 *slotp);
    void freeSlot(JSContext *cx, uint32 slot);

    bool reportReadOnly(JSContext* cx, jsid id, uintN report = JSREPORT_ERROR);
    bool reportNotConfigurable(JSContext* cx, jsid id, uintN report = JSREPORT_ERROR);
    bool reportNotExtensible(JSContext *cx, uintN report = JSREPORT_ERROR);

  private:
    js::Shape *getChildProperty(JSContext *cx, js::Shape *parent, js::Shape &child);

    






    const js::Shape *addPropertyInternal(JSContext *cx, jsid id,
                                         js::PropertyOp getter, js::PropertyOp setter,
                                         uint32 slot, uintN attrs,
                                         uintN flags, intN shortid,
                                         js::Shape **spp);

    bool toDictionaryMode(JSContext *cx);

  public:
    
    const js::Shape *addProperty(JSContext *cx, jsid id,
                                 js::PropertyOp getter, js::PropertyOp setter,
                                 uint32 slot, uintN attrs,
                                 uintN flags, intN shortid);

    
    const js::Shape *addDataProperty(JSContext *cx, jsid id, uint32 slot, uintN attrs) {
        JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
        return addProperty(cx, id, NULL, NULL, slot, attrs, 0, 0);
    }

    
    const js::Shape *putProperty(JSContext *cx, jsid id,
                                 js::PropertyOp getter, js::PropertyOp setter,
                                 uint32 slot, uintN attrs,
                                 uintN flags, intN shortid);

    
    const js::Shape *changeProperty(JSContext *cx, const js::Shape *shape, uintN attrs, uintN mask,
                                    js::PropertyOp getter, js::PropertyOp setter);

    
    bool removeProperty(JSContext *cx, jsid id);

    
    void clear(JSContext *cx);

    JSBool lookupProperty(JSContext *cx, jsid id, JSObject **objp, JSProperty **propp) {
        JSLookupPropOp op = getOps()->lookupProperty;
        return (op ? op : js_LookupProperty)(cx, this, id, objp, propp);
    }

    JSBool defineProperty(JSContext *cx, jsid id, const js::Value &value,
                          js::PropertyOp getter = js::PropertyStub,
                          js::PropertyOp setter = js::PropertyStub,
                          uintN attrs = JSPROP_ENUMERATE) {
        js::DefinePropOp op = getOps()->defineProperty;
        return (op ? op : js_DefineProperty)(cx, this, id, &value, getter, setter, attrs);
    }

    JSBool getProperty(JSContext *cx, jsid id, js::Value *vp) {
        js::PropertyIdOp op = getOps()->getProperty;
        return (op ? op : js_GetProperty)(cx, this, id, vp);
    }

    JSBool setProperty(JSContext *cx, jsid id, js::Value *vp, JSBool strict) {
        js::StrictPropertyIdOp op = getOps()->setProperty;
        return (op ? op : js_SetProperty)(cx, this, id, vp, strict);
    }

    JSBool getAttributes(JSContext *cx, jsid id, uintN *attrsp) {
        JSAttributesOp op = getOps()->getAttributes;
        return (op ? op : js_GetAttributes)(cx, this, id, attrsp);
    }

    JSBool setAttributes(JSContext *cx, jsid id, uintN *attrsp) {
        JSAttributesOp op = getOps()->setAttributes;
        return (op ? op : js_SetAttributes)(cx, this, id, attrsp);
    }

    JSBool deleteProperty(JSContext *cx, jsid id, js::Value *rval, JSBool strict) {
        js::StrictPropertyIdOp op = getOps()->deleteProperty;
        return (op ? op : js_DeleteProperty)(cx, this, id, rval, strict);
    }

    JSBool enumerate(JSContext *cx, JSIterateOp iterop, js::Value *statep, jsid *idp) {
        js::NewEnumerateOp op = getOps()->enumerate;
        return (op ? op : js_Enumerate)(cx, this, iterop, statep, idp);
    }

    JSType typeOf(JSContext *cx) {
        JSTypeOfOp op = getOps()->typeOf;
        return (op ? op : js_TypeOf)(cx, this);
    }

    JSObject *wrappedObject(JSContext *cx) const;

    
    JSObject *thisObject(JSContext *cx) {
        JSObjectOp op = getOps()->thisObject;
        return op ? op(cx, this) : this;
    }

    static bool thisObject(JSContext *cx, const js::Value &v, js::Value *vp);

    inline void dropProperty(JSContext *cx, JSProperty *prop);

    JS_FRIEND_API(JSCompartment *) getCompartment(JSContext *cx);

    inline JSObject *getThrowTypeError() const;

    const js::Shape *defineBlockVariable(JSContext *cx, jsid id, intN index);

    void swap(JSObject *obj);

    inline bool canHaveMethodBarrier() const;

    inline bool isArguments() const;
    inline bool isNormalArguments() const;
    inline bool isStrictArguments() const;
    inline bool isArray() const;
    inline bool isDenseArray() const;
    inline bool isSlowArray() const;
    inline bool isNumber() const;
    inline bool isBoolean() const;
    inline bool isString() const;
    inline bool isPrimitive() const;
    inline bool isDate() const;
    inline bool isFunction() const;
    inline bool isObject() const;
    inline bool isWith() const;
    inline bool isBlock() const;
    inline bool isStaticBlock() const;
    inline bool isClonedBlock() const;
    inline bool isCall() const;
    inline bool isRegExp() const;
    inline bool isXML() const;
    inline bool isXMLId() const;
    inline bool isNamespace() const;
    inline bool isQName() const;

    inline bool isProxy() const;
    inline bool isObjectProxy() const;
    inline bool isFunctionProxy() const;

    JS_FRIEND_API(bool) isWrapper() const;
    JS_FRIEND_API(JSObject *) unwrap(uintN *flagsp = NULL);

    inline void initArrayClass();
};

JS_STATIC_ASSERT(offsetof(JSObject, fslots) % sizeof(js::Value) == 0);
JS_STATIC_ASSERT(sizeof(JSObject) % JS_GCTHING_ALIGN == 0);

#define JSSLOT_START(clasp) (((clasp)->flags & JSCLASS_HAS_PRIVATE)           \
                             ? JSSLOT_PRIVATE + 1                             \
                             : JSSLOT_PRIVATE)

#define JSSLOT_FREE(clasp)  (JSSLOT_START(clasp)                              \
                             + JSCLASS_RESERVED_SLOTS(clasp))






#define MAX_DSLOTS_LENGTH   (~size_t(0) / sizeof(js::Value) - 1)
#define MAX_DSLOTS_LENGTH32 (~uint32(0) / sizeof(js::Value) - 1)

#define OBJ_CHECK_SLOT(obj,slot) JS_ASSERT((obj)->containsSlot(slot))

#ifdef JS_THREADSAFE






#define THREAD_IS_RUNNING_GC(rt, thread)                                      \
    ((rt)->gcRunning && (rt)->gcThread == (thread))

#define CX_THREAD_IS_RUNNING_GC(cx)                                           \
    THREAD_IS_RUNNING_GC((cx)->runtime, (cx)->thread)

#endif 

inline void
OBJ_TO_INNER_OBJECT(JSContext *cx, JSObject *&obj)
{
    if (JSObjectOp op = obj->getClass()->ext.innerObject)
        obj = op(cx, obj);
}

inline void
OBJ_TO_OUTER_OBJECT(JSContext *cx, JSObject *&obj)
{
    if (JSObjectOp op = obj->getClass()->ext.outerObject)
        obj = op(cx, obj);
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

extern js::Class js_ObjectClass;
extern js::Class js_WithClass;
extern js::Class js_BlockClass;

inline bool JSObject::isObject() const { return getClass() == &js_ObjectClass; }
inline bool JSObject::isWith() const   { return getClass() == &js_WithClass; }
inline bool JSObject::isBlock() const  { return getClass() == &js_BlockClass; }
















static const uint32 JSSLOT_BLOCK_DEPTH = JSSLOT_PRIVATE + 1;

inline bool
JSObject::isStaticBlock() const
{
    return isBlock() && !getProto();
}

inline bool
JSObject::isClonedBlock() const
{
    return isBlock() && !!getProto();
}

static const uint32 JSSLOT_WITH_THIS = JSSLOT_PRIVATE + 2;

#define OBJ_BLOCK_COUNT(cx,obj)                                               \
    (obj)->propertyCount()
#define OBJ_BLOCK_DEPTH(cx,obj)                                               \
    (obj)->getSlot(JSSLOT_BLOCK_DEPTH).toInt32()
#define OBJ_SET_BLOCK_DEPTH(cx,obj,depth)                                     \
    (obj)->setSlot(JSSLOT_BLOCK_DEPTH, Value(Int32Value(depth)))









extern JS_REQUIRES_STACK JSObject *
js_NewWithObject(JSContext *cx, JSObject *proto, JSObject *parent, jsint depth);

inline JSObject *
js_UnwrapWithObject(JSContext *cx, JSObject *withobj)
{
    JS_ASSERT(withobj->getClass() == &js_WithClass);
    return withobj->getProto();
}







extern JSObject *
js_NewBlockObject(JSContext *cx);

extern JSObject *
js_CloneBlockObject(JSContext *cx, JSObject *proto, JSStackFrame *fp);

extern JS_REQUIRES_STACK JSBool
js_PutBlockObject(JSContext *cx, JSBool normalUnwind);

JSBool
js_XDRBlockObject(JSXDRState *xdr, JSObject **objp);

struct JSSharpObjectMap {
    jsrefcount  depth;
    jsatomid    sharpgen;
    JSHashTable *table;
};

#define SHARP_BIT       ((jsatomid) 1)
#define BUSY_BIT        ((jsatomid) 2)
#define SHARP_ID_SHIFT  2
#define IS_SHARP(he)    (uintptr_t((he)->value) & SHARP_BIT)
#define MAKE_SHARP(he)  ((he)->value = (void *) (uintptr_t((he)->value)|SHARP_BIT))
#define IS_BUSY(he)     (uintptr_t((he)->value) & BUSY_BIT)
#define MAKE_BUSY(he)   ((he)->value = (void *) (uintptr_t((he)->value)|BUSY_BIT))
#define CLEAR_BUSY(he)  ((he)->value = (void *) (uintptr_t((he)->value)&~BUSY_BIT))

extern JSHashEntry *
js_EnterSharpObject(JSContext *cx, JSObject *obj, JSIdArray **idap,
                    jschar **sp);

extern void
js_LeaveSharpObject(JSContext *cx, JSIdArray **idap);





extern void
js_TraceSharpMap(JSTracer *trc, JSSharpObjectMap *map);

extern JSBool
js_HasOwnPropertyHelper(JSContext *cx, JSLookupPropOp lookup, uintN argc,
                        js::Value *vp);

extern JSBool
js_HasOwnProperty(JSContext *cx, JSLookupPropOp lookup, JSObject *obj, jsid id,
                  JSObject **objp, JSProperty **propp);

extern JSBool
js_NewPropertyDescriptorObject(JSContext *cx, jsid id, uintN attrs,
                               const js::Value &getter, const js::Value &setter,
                               const js::Value &value, js::Value *vp);

extern JSBool
js_PropertyIsEnumerable(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

#ifdef OLD_GETTER_SETTER_METHODS
JS_FRIEND_API(JSBool) js_obj_defineGetter(JSContext *cx, uintN argc, js::Value *vp);
JS_FRIEND_API(JSBool) js_obj_defineSetter(JSContext *cx, uintN argc, js::Value *vp);
#endif

extern JSObject *
js_InitObjectClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitClass(JSContext *cx, JSObject *obj, JSObject *parent_proto,
             js::Class *clasp, js::Native constructor, uintN nargs,
             JSPropertySpec *ps, JSFunctionSpec *fs,
             JSPropertySpec *static_ps, JSFunctionSpec *static_fs);




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
js_PopulateObject(JSContext *cx, JSObject *newborn, JSObject *props);




extern JSBool
js_GetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key,
                  JSObject **objp);

extern JSBool
js_SetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key,
                  JSObject *cobj, JSObject *prototype);





extern JSBool
js_FindClassObject(JSContext *cx, JSObject *start, JSProtoKey key,
                   js::Value *vp, js::Class *clasp = NULL);

extern JSObject *
js_ConstructObject(JSContext *cx, js::Class *clasp, JSObject *proto,
                   JSObject *parent, uintN argc, js::Value *argv);

extern JSObject *
js_NewInstance(JSContext *cx, JSObject *callee);

extern jsid
js_CheckForStringIndex(jsid id);







extern void
js_PurgeScopeChainHelper(JSContext *cx, JSObject *obj, jsid id);

inline void
js_PurgeScopeChain(JSContext *cx, JSObject *obj, jsid id)
{
    if (obj->isDelegate())
        js_PurgeScopeChainHelper(cx, obj, id);
}





extern const js::Shape *
js_AddNativeProperty(JSContext *cx, JSObject *obj, jsid id,
                     js::PropertyOp getter, js::PropertyOp setter, uint32 slot,
                     uintN attrs, uintN flags, intN shortid);






extern const js::Shape *
js_ChangeNativePropertyAttrs(JSContext *cx, JSObject *obj,
                             const js::Shape *shape, uintN attrs, uintN mask,
                             js::PropertyOp getter, js::PropertyOp setter);

extern JSBool
js_DefineOwnProperty(JSContext *cx, JSObject *obj, jsid id,
                     const js::Value &descriptor, JSBool *bp);




const uintN JSDNP_CACHE_RESULT = 1; 
const uintN JSDNP_DONT_PURGE   = 2; 
const uintN JSDNP_SET_METHOD   = 4; 


const uintN JSDNP_UNQUALIFIED  = 8; 










extern JSBool
js_DefineNativeProperty(JSContext *cx, JSObject *obj, jsid id, const js::Value &value,
                        js::PropertyOp getter, js::PropertyOp setter, uintN attrs,
                        uintN flags, intN shortid, JSProperty **propp,
                        uintN defineHow = 0);






extern int
js_LookupPropertyWithFlags(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                           JSObject **objp, JSProperty **propp);







inline bool
js_IsCacheableNonGlobalScope(JSObject *obj)
{
    extern JS_FRIEND_DATA(js::Class) js_CallClass;
    extern JS_FRIEND_DATA(js::Class) js_DeclEnvClass;
    JS_ASSERT(obj->getParent());

    js::Class *clasp = obj->getClass();
    bool cacheable = (clasp == &js_CallClass ||
                      clasp == &js_BlockClass ||
                      clasp == &js_DeclEnvClass);

    JS_ASSERT_IF(cacheable, !obj->getOps()->lookupProperty);
    return cacheable;
}




extern js::PropertyCacheEntry *
js_FindPropertyHelper(JSContext *cx, jsid id, JSBool cacheResult,
                      JSObject **objp, JSObject **pobjp, JSProperty **propp);





extern JS_FRIEND_API(JSBool)
js_FindProperty(JSContext *cx, jsid id, JSObject **objp, JSObject **pobjp,
                JSProperty **propp);

extern JS_REQUIRES_STACK JSObject *
js_FindIdentifierBase(JSContext *cx, JSObject *scopeChain, jsid id);

extern JSObject *
js_FindVariableScope(JSContext *cx, JSFunction **funp);














const uintN JSGET_CACHE_RESULT      = 1; 
const uintN JSGET_METHOD_BARRIER    = 0; 
const uintN JSGET_NO_METHOD_BARRIER = 2; 







extern JSBool
js_NativeGet(JSContext *cx, JSObject *obj, JSObject *pobj, const js::Shape *shape, uintN getHow,
             js::Value *vp);

extern JSBool
js_NativeSet(JSContext *cx, JSObject *obj, const js::Shape *shape, bool added,
             js::Value *vp);

extern JSBool
js_GetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uint32 getHow, js::Value *vp);

extern bool
js_GetPropertyHelperWithShape(JSContext *cx, JSObject *obj, jsid id, uint32 getHow,
                              js::Value *vp, const js::Shape **shapeOut, JSObject **holderOut);

extern JSBool
js_GetOwnPropertyDescriptor(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSBool
js_GetMethod(JSContext *cx, JSObject *obj, jsid id, uintN getHow, js::Value *vp);







extern JS_FRIEND_API(bool)
js_CheckUndeclaredVarAssignment(JSContext *cx, JSString *propname);

extern JSBool
js_SetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uintN defineHow,
                     js::Value *vp, JSBool strict);





extern JSBool
js_SetNativeAttributes(JSContext *cx, JSObject *obj, js::Shape *shape,
                       uintN attrs);

namespace js {

extern JSBool
DefaultValue(JSContext *cx, JSObject *obj, JSType hint, Value *vp);

extern JSBool
CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
            js::Value *vp, uintN *attrsp);

} 

extern bool
js_IsDelegate(JSContext *cx, JSObject *obj, const js::Value &v);





extern JS_FRIEND_API(JSBool)
js_GetClassPrototype(JSContext *cx, JSObject *scope, JSProtoKey protoKey,
                     JSObject **protop, js::Class *clasp = NULL);

extern JSBool
js_SetClassPrototype(JSContext *cx, JSObject *ctor, JSObject *proto,
                     uintN attrs);





extern JSBool
js_PrimitiveToObject(JSContext *cx, js::Value *vp);





extern JSBool
js_ValueToObjectOrNull(JSContext *cx, const js::Value &v, JSObject **objp);





extern JSObject *
js_ValueToNonNullObject(JSContext *cx, const js::Value &v);

extern JSBool
js_TryValueOf(JSContext *cx, JSObject *obj, JSType type, js::Value *rval);

extern JSBool
js_TryMethod(JSContext *cx, JSObject *obj, JSAtom *atom,
             uintN argc, js::Value *argv, js::Value *rval);

extern JSBool
js_XDRObject(JSXDRState *xdr, JSObject **objp);

extern void
js_TraceObject(JSTracer *trc, JSObject *obj);

extern void
js_PrintObjectSlotName(JSTracer *trc, char *buf, size_t bufsize);

extern void
js_ClearNative(JSContext *cx, JSObject *obj);

extern bool
js_GetReservedSlot(JSContext *cx, JSObject *obj, uint32 index, js::Value *vp);

extern bool
js_SetReservedSlot(JSContext *cx, JSObject *obj, uint32 index, const js::Value &v);

extern JSBool
js_CheckPrincipalsAccess(JSContext *cx, JSObject *scopeobj,
                         JSPrincipals *principals, JSAtom *caller);


extern JSBool
js_CheckContentSecurityPolicy(JSContext *cx);


extern JSObject *
js_GetWrappedObject(JSContext *cx, JSObject *obj);


extern const char *
js_ComputeFilename(JSContext *cx, JSStackFrame *caller,
                   JSPrincipals *principals, uintN *linenop);

extern JSBool
js_ReportGetterOnlyAssignment(JSContext *cx);

extern JS_FRIEND_API(JSBool)
js_GetterOnlyPropertyStub(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

#ifdef DEBUG
JS_FRIEND_API(void) js_DumpChars(const jschar *s, size_t n);
JS_FRIEND_API(void) js_DumpString(JSString *str);
JS_FRIEND_API(void) js_DumpAtom(JSAtom *atom);
JS_FRIEND_API(void) js_DumpObject(JSObject *obj);
JS_FRIEND_API(void) js_DumpValue(const js::Value &val);
JS_FRIEND_API(void) js_DumpId(jsid id);
JS_FRIEND_API(void) js_DumpStackFrame(JSContext *cx, JSStackFrame *start = NULL);
bool IsSaneThisObject(JSObject &obj);
#endif

extern uintN
js_InferFlags(JSContext *cx, uintN defaultFlags);


JSBool
js_Object(JSContext *cx, uintN argc, js::Value *vp);


namespace js {

extern bool
SetProto(JSContext *cx, JSObject *obj, JSObject *proto, bool checkForCycles);

}

namespace js {

extern JSString *
obj_toStringHelper(JSContext *cx, JSObject *obj);

}
#endif 
