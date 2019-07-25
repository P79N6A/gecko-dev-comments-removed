






#ifndef MapObject_h__
#define MapObject_h__

#include "jsapi.h"
#include "jscntxt.h"
#include "jsobj.h"

#include "js/HashTable.h"

namespace js {









class HashableValue {
    RelocatableValue value;

  public:
    struct Hasher {
        typedef HashableValue Lookup;
        static HashNumber hash(const Lookup &v) { return v.hash(); }
        static bool match(const HashableValue &k, const Lookup &l) { return k.equals(l); }
    };

    HashableValue() : value(UndefinedValue()) {}

    bool setValue(JSContext *cx, const Value &v);
    HashNumber hash() const;
    bool equals(const HashableValue &other) const;
    HashableValue mark(JSTracer *trc) const;

    struct StackRoot {
        StackRoot(JSContext *cx, HashableValue *pv) : valueRoot(cx, (Value*) &pv->value) {}
        RootValue valueRoot;
    };
};

typedef HashMap<HashableValue,
                RelocatableValue,
                HashableValue::Hasher,
                RuntimeAllocPolicy> ValueMap;
typedef HashSet<HashableValue,
                HashableValue::Hasher,
                RuntimeAllocPolicy> ValueSet;

class MapObject : public JSObject {
  public:
    static JSObject *initClass(JSContext *cx, JSObject *obj);
    static Class class_;
  private:
    typedef ValueMap Data;
    static JSFunctionSpec methods[];
    ValueMap *getData() { return static_cast<ValueMap *>(getPrivate()); }
    static void mark(JSTracer *trc, JSObject *obj);
    static void finalize(FreeOp *fop, JSObject *obj);
    static JSBool construct(JSContext *cx, unsigned argc, Value *vp);
    static JSBool size(JSContext *cx, unsigned argc, Value *vp);
    static JSBool get(JSContext *cx, unsigned argc, Value *vp);
    static JSBool has(JSContext *cx, unsigned argc, Value *vp);
    static JSBool set(JSContext *cx, unsigned argc, Value *vp);
    static JSBool delete_(JSContext *cx, unsigned argc, Value *vp);
};

class SetObject : public JSObject {
  public:
    static JSObject *initClass(JSContext *cx, JSObject *obj);
    static Class class_;
  private:
    typedef ValueSet Data;
    static JSFunctionSpec methods[];
    ValueSet *getData() { return static_cast<ValueSet *>(getPrivate()); }
    static void mark(JSTracer *trc, JSObject *obj);
    static void finalize(FreeOp *fop, JSObject *obj);
    static JSBool construct(JSContext *cx, unsigned argc, Value *vp);
    static JSBool size(JSContext *cx, unsigned argc, Value *vp);
    static JSBool has(JSContext *cx, unsigned argc, Value *vp);
    static JSBool add(JSContext *cx, unsigned argc, Value *vp);
    static JSBool delete_(JSContext *cx, unsigned argc, Value *vp);
};

} 

extern JSObject *
js_InitMapClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitSetClass(JSContext *cx, JSObject *obj);

#endif  
