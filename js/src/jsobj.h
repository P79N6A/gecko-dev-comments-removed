







































#ifndef jsobj_h___
#define jsobj_h___









#include "jsapi.h"
#include "jshash.h"
#include "jspubtd.h"
#include "jsprvtd.h"
#include "jslock.h"
#include "jsvalue.h"
#include "jsvector.h"
#include "jscell.h"

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

static inline StrictPropertyOp
CastAsStrictPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(StrictPropertyOp, object);
}

static inline JSPropertyOp
CastAsJSPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(JSPropertyOp, object);
}

static inline JSStrictPropertyOp
CastAsJSStrictPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(JSStrictPropertyOp, object);
}

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





struct PropDesc {
    



    js::Value pd;

    js::Value value, get, set;

    
    uint8 attrs;

    
    bool hasGet : 1;
    bool hasSet : 1;
    bool hasValue : 1;
    bool hasWritable : 1;
    bool hasEnumerable : 1;
    bool hasConfigurable : 1;

    friend class js::AutoPropDescArrayRooter;

    PropDesc();

  public:
    
    bool initialize(JSContext* cx, const js::Value &v);

    
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
    js::StrictPropertyOp setter() const {
        return js::CastAsStrictPropertyOp(setterObject());
    }
};

typedef Vector<PropDesc, 1> PropDescArray;

void
MeterEntryCount(uintN count);

} 

struct JSObjectMap : public js::gc::Cell {
    mutable uint32 shape;  
    uint32 slotSpan;       

    static JS_FRIEND_DATA(JSObjectMap) sharedNonNative;

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
                  js::PropertyOp getter, js::StrictPropertyOp setter, uintN attrs);

extern JSBool
js_GetProperty(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, js::Value *vp);

inline JSBool
js_GetProperty(JSContext *cx, JSObject *obj, jsid id, js::Value *vp)
{
    return js_GetProperty(cx, obj, obj, id, vp);
}

namespace js {

extern JSBool
GetPropertyDefault(JSContext *cx, JSObject *obj, jsid id, const Value &def, Value *vp);

} 

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

namespace js {

struct NativeIterator;
class RegExp;

}

struct JSFunction;

namespace nanojit {
class ValidateWriter;
}





































struct JSObject : js::gc::Cell {
    





    friend class js::TraceRecorder;
    friend class nanojit::ValidateWriter;
    friend class GetPropCompiler;

    














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
        DELEGATE                  =  0x01,
        SYSTEM                    =  0x02,
        NOT_EXTENSIBLE            =  0x04,
        BRANDED                   =  0x08,
        GENERIC                   =  0x10,
        METHOD_BARRIER            =  0x20,
        INDEXED                   =  0x40,
        OWN_SHAPE                 =  0x80,
        BOUND_FUNCTION            = 0x100,
        HAS_EQUALITY              = 0x200,
        METHOD_THRASH_COUNT_MASK  = 0xc00,
        METHOD_THRASH_COUNT_SHIFT =    10,
        METHOD_THRASH_COUNT_MAX   = METHOD_THRASH_COUNT_MASK >> METHOD_THRASH_COUNT_SHIFT
    };

    



    enum {
        NSLOTS_BITS     = 29,
        NSLOTS_LIMIT    = JS_BIT(NSLOTS_BITS)
    };

    uint32      flags;                      
    uint32      objShape;                   

    
    js::EmptyShape **emptyShapes;

    JSObject    *proto;                     
    JSObject    *parent;                    
    void        *privateData;               
    jsuword     capacity;                   
    js::Value   *slots;                     


    






    inline bool canProvideEmptyShape(js::Class *clasp);
    inline js::EmptyShape *getEmptyShape(JSContext *cx, js::Class *aclasp,
                                          unsigned kind);

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
    void clearDelegate()        { flags &= ~DELEGATE; }

    bool isBoundFunction() const { return !!(flags & BOUND_FUNCTION); }

    static void setDelegateNullSafe(JSObject *obj) {
        if (obj)
            obj->setDelegate();
    }

    bool isSystem() const       { return !!(flags & SYSTEM); }
    void setSystem()            { flags |= SYSTEM; }

    




    bool branded()              { return !!(flags & BRANDED); }

    




    bool brand(JSContext *cx);
    bool unbrand(JSContext *cx);

    bool generic()              { return !!(flags & GENERIC); }
    void setGeneric()           { flags |= GENERIC; }

    uintN getMethodThrashCount() const {
        return (flags & METHOD_THRASH_COUNT_MASK) >> METHOD_THRASH_COUNT_SHIFT;
    }

    void setMethodThrashCount(uintN count) {
        JS_ASSERT(count <= METHOD_THRASH_COUNT_MAX);
        flags = (flags & ~METHOD_THRASH_COUNT_MASK) | (count << METHOD_THRASH_COUNT_SHIFT);
    }

    bool hasSpecialEquality() const { return !!(flags & HAS_EQUALITY); }
    void assertSpecialEqualitySynced() const {
        JS_ASSERT(!!clasp->ext.equality == hasSpecialEquality());
    }

    
    inline void syncSpecialEquality();

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

    
    void initCall(JSContext *cx, const js::Bindings &bindings, JSObject *parent);
    void initClonedBlock(JSContext *cx, JSObject *proto, JSStackFrame *priv);
    void setBlockOwnShape(JSContext *cx);

    void deletingShapeChange(JSContext *cx, const js::Shape &shape);
    const js::Shape *methodShapeChange(JSContext *cx, const js::Shape &shape);
    bool methodShapeChange(JSContext *cx, uint32 slot);
    void protoShapeChange(JSContext *cx);
    void shadowingShapeChange(JSContext *cx, const js::Shape &shape);
    bool globalObjectOwnShapeChange(JSContext *cx);
    void watchpointOwnShapeChange(JSContext *cx) { generateOwnShape(cx); }

    void extensibleShapeChange(JSContext *cx) {
        
        generateOwnShape(cx);
    }

    







































    bool hasMethodBarrier()     { return !!(flags & METHOD_BARRIER); }
    void setMethodBarrier()     { flags |= METHOD_BARRIER; }

    





    bool brandedOrHasMethodBarrier() { return !!(flags & (BRANDED | METHOD_BARRIER)); }

    




    const js::Shape *methodReadBarrier(JSContext *cx, const js::Shape &shape, js::Value *vp);

    






    const js::Shape *methodWriteBarrier(JSContext *cx, const js::Shape &shape, const js::Value &v);
    bool methodWriteBarrier(JSContext *cx, uint32 slot, const js::Value &v);

    bool isIndexed() const          { return !!(flags & INDEXED); }
    void setIndexed()               { flags |= INDEXED; }

    




    inline bool inDictionaryMode() const;

    inline uint32 propertyCount() const;

    inline bool hasPropertyTable() const;

     unsigned finalizeKind() const;

    uint32 numSlots() const { return capacity; }

    size_t slotsAndStructSize(uint32 nslots) const;
    size_t slotsAndStructSize() const { return slotsAndStructSize(numSlots()); }

    inline js::Value* fixedSlots() const;
    inline size_t numFixedSlots() const;

    static inline size_t getFixedSlotOffset(size_t slot);

  public:
    
    static const uint32 SLOT_CAPACITY_MIN = 8;

    bool allocSlots(JSContext *cx, size_t nslots);
    bool growSlots(JSContext *cx, size_t nslots);
    void shrinkSlots(JSContext *cx, size_t nslots);

    bool ensureSlots(JSContext *cx, size_t nslots) {
        if (numSlots() < nslots)
            return growSlots(cx, nslots);
        return true;
    }

    















    bool ensureInstanceReservedSlots(JSContext *cx, size_t nreserved);

    



    js::Value *getSlots() const {
        return slots;
    }

    




    bool ensureClassReservedSlotsForEmptyObject(JSContext *cx);

    inline bool ensureClassReservedSlots(JSContext *cx);

    uint32 slotSpan() const { return map->slotSpan; }

    bool containsSlot(uint32 slot) const { return slot < slotSpan(); }

    js::Value& getSlotRef(uintN slot) {
        JS_ASSERT(slot < capacity);
        return slots[slot];
    }

    js::Value &nativeGetSlotRef(uintN slot) {
        JS_ASSERT(isNative());
        JS_ASSERT(containsSlot(slot));
        return getSlotRef(slot);
    }

    const js::Value &getSlot(uintN slot) const {
        JS_ASSERT(slot < capacity);
        return slots[slot];
    }

    const js::Value &nativeGetSlot(uintN slot) const {
        JS_ASSERT(isNative());
        JS_ASSERT(containsSlot(slot));
        return getSlot(slot);
    }

    void setSlot(uintN slot, const js::Value &value) {
        JS_ASSERT(slot < capacity);
        slots[slot] = value;
    }

    void nativeSetSlot(uintN slot, const js::Value &value) {
        JS_ASSERT(isNative());
        JS_ASSERT(containsSlot(slot));
        return setSlot(slot, value);
    }

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

    JS_FRIEND_API(JSObject *) getGlobal() const;

    bool isGlobal() const {
        return !!(getClass()->flags & JSCLASS_IS_GLOBAL);
    }

    void *getPrivate() const {
        JS_ASSERT(getClass()->flags & JSCLASS_HAS_PRIVATE);
        return privateData;
    }

    void setPrivate(void *data) {
        JS_ASSERT(getClass()->flags & JSCLASS_HAS_PRIVATE);
        privateData = data;
    }


    



  private:
    enum ImmutabilityType { SEAL, FREEZE };

    





    bool sealOrFreeze(JSContext *cx, ImmutabilityType it);

  public:
    bool isExtensible() const { return !(flags & NOT_EXTENSIBLE); }
    bool preventExtensions(JSContext *cx, js::AutoIdVector *props);

    
    inline bool seal(JSContext *cx) { return sealOrFreeze(cx, SEAL); }
    
    bool freeze(JSContext *cx) { return sealOrFreeze(cx, FREEZE); }
        
    



  private:
    static const uint32 JSSLOT_PRIMITIVE_THIS = 0;

  public:
    inline const js::Value &getPrimitiveThis() const;
    inline void setPrimitiveThis(const js::Value &pthis);

  private:
    
    static const uint32 JSSLOT_STRING_LENGTH = 1;

    




    const js::Shape *assignInitialStringShape(JSContext *cx);

  public:
    static const uint32 STRING_RESERVED_SLOTS = 2;

    inline size_t getStringLength() const;

    inline bool initString(JSContext *cx, JSString *str);

    



    inline uint32 getArrayLength() const;
    inline void setArrayLength(uint32 length);

    inline uint32 getDenseArrayCapacity();
    inline js::Value* getDenseArrayElements();
    inline const js::Value &getDenseArrayElement(uintN idx);
    inline js::Value* addressOfDenseArrayElement(uintN idx);
    inline void setDenseArrayElement(uintN idx, const js::Value &val);
    inline void shrinkDenseArrayElements(JSContext *cx, uintN cap);

    






    enum EnsureDenseResult { ED_OK, ED_FAILED, ED_SPARSE };
    inline EnsureDenseResult ensureDenseArrayElements(JSContext *cx, uintN index, uintN extra);

    



    bool willBeSparseDenseArray(uintN requiredCapacity, uintN newElementsHint);

    JSBool makeDenseArraySlow(JSContext *cx);

    



  private:
    













































    static const uint32 JSSLOT_ARGS_DATA = 1;

  public:
    
    static const uint32 JSSLOT_ARGS_LENGTH = 0;
    static const uint32 ARGS_CLASS_RESERVED_SLOTS = 2;
    static const uint32 ARGS_FIRST_FREE_SLOT = ARGS_CLASS_RESERVED_SLOTS + 1;

    
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
    inline js::Value *getArgsElements() const;
    inline js::Value *addressOfArgsElement(uint32 i);
    inline void setArgsElement(uint32 i, const js::Value &v);

  private:
    










    static const uint32 JSSLOT_CALL_CALLEE = 0;
    static const uint32 JSSLOT_CALL_ARGUMENTS = 1;

  public:
    
    static const uint32 CALL_RESERVED_SLOTS = 2;

    
    inline bool callIsForEval() const;

    
    inline JSStackFrame *maybeCallObjStackFrame() const;

    



    inline JSObject *getCallObjCallee() const;
    inline JSFunction *getCallObjCalleeFunction() const; 
    inline void setCallObjCallee(JSObject *callee);

    inline const js::Value &getCallObjArguments() const;
    inline void setCallObjArguments(const js::Value &v);

    
    inline const js::Value &callObjArg(uintN i) const;
    inline js::Value &callObjArg(uintN i);

    
    inline const js::Value &callObjVar(uintN i) const;
    inline js::Value &callObjVar(uintN i);

    



    static const uint32 JSSLOT_DATE_UTC_TIME = 0;

    




    static const uint32 JSSLOT_DATE_COMPONENTS_START = 1;

    static const uint32 JSSLOT_DATE_LOCAL_TIME = 1;
    static const uint32 JSSLOT_DATE_LOCAL_YEAR = 2;
    static const uint32 JSSLOT_DATE_LOCAL_MONTH = 3;
    static const uint32 JSSLOT_DATE_LOCAL_DATE = 4;
    static const uint32 JSSLOT_DATE_LOCAL_DAY = 5;
    static const uint32 JSSLOT_DATE_LOCAL_HOURS = 6;
    static const uint32 JSSLOT_DATE_LOCAL_MINUTES = 7;
    static const uint32 JSSLOT_DATE_LOCAL_SECONDS = 8;

    static const uint32 DATE_CLASS_RESERVED_SLOTS = 9;

    inline const js::Value &getDateUTCTime() const;
    inline void setDateUTCTime(const js::Value &pthis);

    



  private:
    friend struct JSFunction;
    friend class js::mjit::Compiler;

    



    static const uint32 JSSLOT_FLAT_CLOSURE_UPVARS = 0;

    




    static const uint32 JSSLOT_FUN_METHOD_ATOM = 0;
    static const uint32 JSSLOT_FUN_METHOD_OBJ  = 1;

    static const uint32 JSSLOT_BOUND_FUNCTION_THIS       = 0;
    static const uint32 JSSLOT_BOUND_FUNCTION_ARGS_COUNT = 1;

  public:
    static const uint32 FUN_CLASS_RESERVED_SLOTS = 2;

    inline JSFunction *getFunctionPrivate() const;

    inline js::Value *getFlatClosureUpvars() const;
    inline js::Value getFlatClosureUpvar(uint32 i) const;
    inline js::Value &getFlatClosureUpvar(uint32 i);
    inline void setFlatClosureUpvars(js::Value *upvars);

    inline bool hasMethodObj(const JSObject& obj) const;
    inline void setMethodObj(JSObject& obj);

    inline bool initBoundFunction(JSContext *cx, const js::Value &thisArg,
                                  const js::Value *args, uintN argslen);

    inline JSObject *getBoundFunctionTarget() const;
    inline const js::Value &getBoundFunctionThis() const;
    inline const js::Value *getBoundFunctionArguments(uintN &argslen) const;

    



  private:
    static const uint32 JSSLOT_REGEXP_LAST_INDEX = 0;
    static const uint32 JSSLOT_REGEXP_SOURCE = 1;
    static const uint32 JSSLOT_REGEXP_GLOBAL = 2;
    static const uint32 JSSLOT_REGEXP_IGNORE_CASE = 3;
    static const uint32 JSSLOT_REGEXP_MULTILINE = 4;
    static const uint32 JSSLOT_REGEXP_STICKY = 5;

    




    const js::Shape *assignInitialRegExpShape(JSContext *cx);

  public:
    static const uint32 REGEXP_CLASS_RESERVED_SLOTS = 6;

    inline const js::Value &getRegExpLastIndex() const;
    inline void setRegExpLastIndex(const js::Value &v);
    inline void setRegExpLastIndex(jsdouble d);
    inline void zeroRegExpLastIndex();

    inline void setRegExpSource(JSString *source);
    inline void setRegExpGlobal(bool global);
    inline void setRegExpIgnoreCase(bool ignoreCase);
    inline void setRegExpMultiline(bool multiline);
    inline void setRegExpSticky(bool sticky);

    inline bool initRegExp(JSContext *cx, js::RegExp *re);

    



    inline js::NativeIterator *getNativeIterator() const;
    inline void setNativeIterator(js::NativeIterator *);

    



    inline JSScript *getScript() const;

    



    






  private:
    static const uint32 JSSLOT_NAME_PREFIX          = 0;   
    static const uint32 JSSLOT_NAME_URI             = 1;   

    static const uint32 JSSLOT_NAMESPACE_DECLARED   = 2;

    static const uint32 JSSLOT_QNAME_LOCAL_NAME     = 2;

  public:
    static const uint32 NAMESPACE_CLASS_RESERVED_SLOTS = 3;
    static const uint32 QNAME_CLASS_RESERVED_SLOTS     = 3;

    inline JSLinearString *getNamePrefix() const;
    inline jsval getNamePrefixVal() const;
    inline void setNamePrefix(JSLinearString *prefix);
    inline void clearNamePrefix();

    inline JSLinearString *getNameURI() const;
    inline jsval getNameURIVal() const;
    inline void setNameURI(JSLinearString *uri);

    inline jsval getNamespaceDeclared() const;
    inline void setNamespaceDeclared(jsval decl);

    inline JSLinearString *getQNameLocalName() const;
    inline jsval getQNameLocalNameVal() const;
    inline void setQNameLocalName(JSLinearString *name);

    



    inline js::JSProxyHandler *getProxyHandler() const;
    inline const js::Value &getProxyPrivate() const;
    inline void setProxyPrivate(const js::Value &priv);
    inline const js::Value &getProxyExtra() const;
    inline void setProxyExtra(const js::Value &extra);

    


    inline JSObject *getWithThis() const;
    inline void setWithThis(JSObject *thisp);

    


    inline bool isCallable();

    
    void init(JSContext *cx, js::Class *aclasp, JSObject *proto, JSObject *parent,
              void *priv, bool useHoles);

    inline void finish(JSContext *cx);
    JS_ALWAYS_INLINE void finalize(JSContext *cx);

    



    inline bool initSharingEmptyShape(JSContext *cx,
                                      js::Class *clasp,
                                      JSObject *proto,
                                      JSObject *parent,
                                      void *priv,
                                       unsigned kind);

    inline bool hasSlotsArray() const;

    
    inline void freeSlotsArray(JSContext *cx);

    
    inline void revertToFixedSlots(JSContext *cx);

    inline bool hasProperty(JSContext *cx, jsid id, bool *foundp, uintN flags = 0);

    








    bool allocSlot(JSContext *cx, uint32 *slotp);
    bool freeSlot(JSContext *cx, uint32 slot);

  public:
    bool reportReadOnly(JSContext* cx, jsid id, uintN report = JSREPORT_ERROR);
    bool reportNotConfigurable(JSContext* cx, jsid id, uintN report = JSREPORT_ERROR);
    bool reportNotExtensible(JSContext *cx, uintN report = JSREPORT_ERROR);

    





    bool callMethod(JSContext *cx, jsid id, uintN argc, js::Value *argv, js::Value *vp);

  private:
    js::Shape *getChildProperty(JSContext *cx, js::Shape *parent, js::Shape &child);

    






    const js::Shape *addPropertyInternal(JSContext *cx, jsid id,
                                         js::PropertyOp getter, js::StrictPropertyOp setter,
                                         uint32 slot, uintN attrs,
                                         uintN flags, intN shortid,
                                         js::Shape **spp);

    bool toDictionaryMode(JSContext *cx);

  public:
    
    const js::Shape *addProperty(JSContext *cx, jsid id,
                                 js::PropertyOp getter, js::StrictPropertyOp setter,
                                 uint32 slot, uintN attrs,
                                 uintN flags, intN shortid);

    
    const js::Shape *addDataProperty(JSContext *cx, jsid id, uint32 slot, uintN attrs) {
        JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
        return addProperty(cx, id, NULL, NULL, slot, attrs, 0, 0);
    }

    
    const js::Shape *putProperty(JSContext *cx, jsid id,
                                 js::PropertyOp getter, js::StrictPropertyOp setter,
                                 uint32 slot, uintN attrs,
                                 uintN flags, intN shortid);

    
    const js::Shape *changeProperty(JSContext *cx, const js::Shape *shape, uintN attrs, uintN mask,
                                    js::PropertyOp getter, js::StrictPropertyOp setter);

    
    bool removeProperty(JSContext *cx, jsid id);

    
    void clear(JSContext *cx);

    JSBool lookupProperty(JSContext *cx, jsid id, JSObject **objp, JSProperty **propp) {
        js::LookupPropOp op = getOps()->lookupProperty;
        return (op ? op : js_LookupProperty)(cx, this, id, objp, propp);
    }

    JSBool defineProperty(JSContext *cx, jsid id, const js::Value &value,
                          js::PropertyOp getter = js::PropertyStub,
                          js::StrictPropertyOp setter = js::StrictPropertyStub,
                          uintN attrs = JSPROP_ENUMERATE) {
        js::DefinePropOp op = getOps()->defineProperty;
        return (op ? op : js_DefineProperty)(cx, this, id, &value, getter, setter, attrs);
    }

    JSBool getProperty(JSContext *cx, JSObject *receiver, jsid id, js::Value *vp) {
        js::PropertyIdOp op = getOps()->getProperty;
        return (op ? op : (js::PropertyIdOp)js_GetProperty)(cx, this, receiver, id, vp);
    }

    JSBool getProperty(JSContext *cx, jsid id, js::Value *vp) {
        return getProperty(cx, this, id, vp);
    }

    JSBool setProperty(JSContext *cx, jsid id, js::Value *vp, JSBool strict) {
        js::StrictPropertyIdOp op = getOps()->setProperty;
        return (op ? op : js_SetProperty)(cx, this, id, vp, strict);
    }

    JSBool getAttributes(JSContext *cx, jsid id, uintN *attrsp) {
        js::AttributesOp op = getOps()->getAttributes;
        return (op ? op : js_GetAttributes)(cx, this, id, attrsp);
    }

    JSBool setAttributes(JSContext *cx, jsid id, uintN *attrsp) {
        js::AttributesOp op = getOps()->setAttributes;
        return (op ? op : js_SetAttributes)(cx, this, id, attrsp);
    }

    JSBool deleteProperty(JSContext *cx, jsid id, js::Value *rval, JSBool strict) {
        js::DeleteIdOp op = getOps()->deleteProperty;
        return (op ? op : js_DeleteProperty)(cx, this, id, rval, strict);
    }

    JSBool enumerate(JSContext *cx, JSIterateOp iterop, js::Value *statep, jsid *idp) {
        js::NewEnumerateOp op = getOps()->enumerate;
        return (op ? op : js_Enumerate)(cx, this, iterop, statep, idp);
    }

    JSType typeOf(JSContext *cx) {
        js::TypeOfOp op = getOps()->typeOf;
        return (op ? op : js_TypeOf)(cx, this);
    }

    
    JSObject *thisObject(JSContext *cx) {
        JSObjectOp op = getOps()->thisObject;
        return op ? op(cx, this) : this;
    }

    static bool thisObject(JSContext *cx, const js::Value &v, js::Value *vp);

    inline JSCompartment *getCompartment() const;

    inline JSObject *getThrowTypeError() const;

    JS_FRIEND_API(JSObject *) clone(JSContext *cx, JSObject *proto, JSObject *parent);
    JS_FRIEND_API(bool) copyPropertiesFrom(JSContext *cx, JSObject *obj);
    bool swap(JSContext *cx, JSObject *other);

    const js::Shape *defineBlockVariable(JSContext *cx, jsid id, intN index);

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
    inline bool isScript() const;
    inline bool isXML() const;
    inline bool isXMLId() const;
    inline bool isNamespace() const;
    inline bool isQName() const;
    inline bool isWeakMap() const;

    inline bool isProxy() const;
    inline bool isObjectProxy() const;
    inline bool isFunctionProxy() const;

    JS_FRIEND_API(bool) isWrapper() const;
    JS_FRIEND_API(JSObject *) unwrap(uintN *flagsp = NULL);

    inline void initArrayClass();
};


JS_STATIC_ASSERT(sizeof(JSObject) % sizeof(js::Value) == 0);

inline js::Value*
JSObject::fixedSlots() const {
    return (js::Value*) (jsuword(this) + sizeof(JSObject));
}

inline bool
JSObject::hasSlotsArray() const { return this->slots != fixedSlots(); }

 inline size_t
JSObject::getFixedSlotOffset(size_t slot) {
    return sizeof(JSObject) + (slot * sizeof(js::Value));
}

struct JSObject_Slots2 : JSObject { js::Value fslots[2]; };
struct JSObject_Slots4 : JSObject { js::Value fslots[4]; };
struct JSObject_Slots8 : JSObject { js::Value fslots[8]; };
struct JSObject_Slots12 : JSObject { js::Value fslots[12]; };
struct JSObject_Slots16 : JSObject { js::Value fslots[16]; };

#define JSSLOT_FREE(clasp)  JSCLASS_RESERVED_SLOTS(clasp)

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
















static const uint32 JSSLOT_BLOCK_DEPTH = 0;
static const uint32 JSSLOT_BLOCK_FIRST_FREE_SLOT = JSSLOT_BLOCK_DEPTH + 1;

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

static const uint32 JSSLOT_WITH_THIS = 1;

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
js_HasOwnPropertyHelper(JSContext *cx, js::LookupPropOp lookup, uintN argc,
                        js::Value *vp);

extern JSBool
js_HasOwnProperty(JSContext *cx, js::LookupPropOp lookup, JSObject *obj, jsid id,
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

namespace js {
JSObject *
DefineConstructorAndPrototype(JSContext *cx, JSObject *obj, JSProtoKey key, JSAtom *atom,
                              JSObject *protoProto, Class *clasp,
                              Native constructor, uintN nargs,
                              JSPropertySpec *ps, JSFunctionSpec *fs,
                              JSPropertySpec *static_ps, JSFunctionSpec *static_fs);
}

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
js_CreateThisForFunctionWithProto(JSContext *cx, JSObject *callee, JSObject *proto);


extern JSObject *
js_CreateThisForFunction(JSContext *cx, JSObject *callee);


extern JSObject *
js_CreateThis(JSContext *cx, JSObject *callee);

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
                     js::PropertyOp getter, js::StrictPropertyOp setter, uint32 slot,
                     uintN attrs, uintN flags, intN shortid);






extern const js::Shape *
js_ChangeNativePropertyAttrs(JSContext *cx, JSObject *obj,
                             const js::Shape *shape, uintN attrs, uintN mask,
                             js::PropertyOp getter, js::StrictPropertyOp setter);

extern JSBool
js_DefineOwnProperty(JSContext *cx, JSObject *obj, jsid id,
                     const js::Value &descriptor, JSBool *bp);




const uintN JSDNP_CACHE_RESULT = 1; 
const uintN JSDNP_DONT_PURGE   = 2; 
const uintN JSDNP_SET_METHOD   = 4; 


const uintN JSDNP_UNQUALIFIED  = 8; 








extern JSBool
js_DefineNativeProperty(JSContext *cx, JSObject *obj, jsid id, const js::Value &value,
                        js::PropertyOp getter, js::StrictPropertyOp setter, uintN attrs,
                        uintN flags, intN shortid, JSProperty **propp,
                        uintN defineHow = 0);






extern int
js_LookupPropertyWithFlags(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                           JSObject **objp, JSProperty **propp);





static const uintN JSRESOLVE_INFER = 0xffff;

extern JS_FRIEND_DATA(js::Class) js_CallClass;
extern JS_FRIEND_DATA(js::Class) js_DeclEnvClass;

namespace js {






static inline bool
IsCacheableNonGlobalScope(JSObject *obj)
{
    JS_ASSERT(obj->getParent());

    js::Class *clasp = obj->getClass();
    bool cacheable = (clasp == &js_CallClass ||
                      clasp == &js_BlockClass ||
                      clasp == &js_DeclEnvClass);

    JS_ASSERT_IF(cacheable, !obj->getOps()->lookupProperty);
    return cacheable;
}

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
             bool strict, js::Value *vp);

extern JSBool
js_GetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uint32 getHow, js::Value *vp);

extern bool
js_GetPropertyHelperWithShape(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id,
                              uint32 getHow, js::Value *vp,
                              const js::Shape **shapeOut, JSObject **holderOut);

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





extern JSObject *
HasNativeMethod(JSObject *obj, jsid methodid, Native native);

extern bool
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

namespace js {





extern JSObject *
ToObjectSlow(JSContext *cx, js::Value *vp);

JS_ALWAYS_INLINE JSObject *
ToObject(JSContext *cx, js::Value *vp)
{
    if (vp->isObject())
        return &vp->toObject();
    return ToObjectSlow(cx, vp);
}

}





extern JSObject *
js_ValueToNonNullObject(JSContext *cx, const js::Value &v);

extern JSBool
js_TryValueOf(JSContext *cx, JSObject *obj, JSType type, js::Value *rval);

extern JSBool
js_XDRObject(JSXDRState *xdr, JSObject **objp);

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
js_CheckContentSecurityPolicy(JSContext *cx, JSObject *scopeObj);


extern const char *
js_ComputeFilename(JSContext *cx, JSStackFrame *caller,
                   JSPrincipals *principals, uintN *linenop);

extern JSBool
js_ReportGetterOnlyAssignment(JSContext *cx);

extern JS_FRIEND_API(JSBool)
js_GetterOnlyPropertyStub(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp);

#ifdef DEBUG
JS_FRIEND_API(void) js_DumpChars(const jschar *s, size_t n);
JS_FRIEND_API(void) js_DumpString(JSString *str);
JS_FRIEND_API(void) js_DumpAtom(JSAtom *atom);
JS_FRIEND_API(void) js_DumpObject(JSObject *obj);
JS_FRIEND_API(void) js_DumpValue(const js::Value &val);
JS_FRIEND_API(void) js_DumpId(jsid id);
JS_FRIEND_API(void) js_DumpStackFrame(JSContext *cx, JSStackFrame *start = NULL);
#endif

extern uintN
js_InferFlags(JSContext *cx, uintN defaultFlags);


JSBool
js_Object(JSContext *cx, uintN argc, js::Value *vp);


namespace js {

extern bool
SetProto(JSContext *cx, JSObject *obj, JSObject *proto, bool checkForCycles);

extern JSString *
obj_toStringHelper(JSContext *cx, JSObject *obj);

enum EvalType { INDIRECT_EVAL, DIRECT_EVAL };











extern bool
EvalKernel(JSContext *cx, uintN argc, js::Value *vp, EvalType evalType, JSStackFrame *caller,
           JSObject *scopeobj);





extern bool
IsBuiltinEvalForScope(JSObject *scopeChain, const js::Value &v);


extern bool
IsAnyBuiltinEval(JSFunction *fun);

}

#endif 
