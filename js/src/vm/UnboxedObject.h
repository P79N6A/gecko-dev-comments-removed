





#ifndef vm_UnboxedObject_h
#define vm_UnboxedObject_h

#include "jsgc.h"
#include "jsinfer.h"
#include "jsobj.h"

namespace js {



static inline size_t
UnboxedTypeSize(JSValueType type)
{
    switch (type) {
      case JSVAL_TYPE_BOOLEAN: return 1;
      case JSVAL_TYPE_INT32:   return 4;
      case JSVAL_TYPE_DOUBLE:  return 8;
      case JSVAL_TYPE_STRING:  return sizeof(void *);
      case JSVAL_TYPE_OBJECT:  return sizeof(void *);
      default:                 return 0;
    }
}

static inline bool
UnboxedTypeNeedsPreBarrier(JSValueType type)
{
    return type == JSVAL_TYPE_STRING || type == JSVAL_TYPE_OBJECT;
}


class UnboxedLayout : public mozilla::LinkedListElement<UnboxedLayout>
{
  public:
    struct Property {
        PropertyName *name;
        uint32_t offset;
        JSValueType type;

        Property()
          : name(nullptr), offset(UINT32_MAX), type(JSVAL_TYPE_MAGIC)
        {}
    };

    typedef Vector<Property, 0, SystemAllocPolicy> PropertyVector;

  private:
    
    PropertyVector properties_;

    
    size_t size_;

    
    TypeNewScript *newScript_;

    
    
    int32_t *traceList_;

  public:
    UnboxedLayout(const PropertyVector &properties, size_t size)
      : size_(size), newScript_(nullptr), traceList_(nullptr)
    {
        properties_.appendAll(properties);
    }

    ~UnboxedLayout() {
        js_delete(newScript_);
        js_free(traceList_);
    }

    const PropertyVector &properties() const {
        return properties_;
    }

    TypeNewScript *newScript() const {
        return newScript_;
    }

    void setNewScript(TypeNewScript *newScript, bool writeBarrier = true);

    const int32_t *traceList() const {
        return traceList_;
    }

    void setTraceList(int32_t *traceList) {
        traceList_ = traceList;
    }

    const Property *lookup(JSAtom *atom) const {
        for (size_t i = 0; i < properties_.length(); i++) {
            if (properties_[i].name == atom)
                return &properties_[i];
        }
        return nullptr;
    }

    const Property *lookup(jsid id) const {
        if (JSID_IS_STRING(id))
            return lookup(JSID_TO_ATOM(id));
        return nullptr;
    }

    size_t size() const {
        return size_;
    }

    inline gc::AllocKind getAllocKind() const;

    void trace(JSTracer *trc);

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf);
};





class UnboxedPlainObject : public JSObject
{
    
    uint8_t data_[1];

  public:
    static const Class class_;

    static bool obj_lookupProperty(JSContext *cx, HandleObject obj,
                                   HandleId id, MutableHandleObject objp,
                                   MutableHandleShape propp);

    static bool obj_defineProperty(JSContext *cx, HandleObject obj, HandleId id, HandleValue v,
                                   PropertyOp getter, StrictPropertyOp setter, unsigned attrs);

    static bool obj_getProperty(JSContext *cx, HandleObject obj, HandleObject receiver,
                                HandleId id, MutableHandleValue vp);

    static bool obj_setProperty(JSContext *cx, HandleObject obj, HandleId id,
                                MutableHandleValue vp, bool strict);

    static bool obj_getOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id,
                                             MutableHandle<JSPropertyDescriptor> desc);

    static bool obj_setPropertyAttributes(JSContext *cx, HandleObject obj,
                                          HandleId id, unsigned *attrsp);

    static bool obj_deleteProperty(JSContext *cx, HandleObject obj, HandleId id, bool *succeeded);

    static bool obj_enumerate(JSContext *cx, HandleObject obj, AutoIdVector &properties);
    static bool obj_watch(JSContext *cx, HandleObject obj, HandleId id, HandleObject callable);

    const UnboxedLayout &layout() const {
        return group()->unboxedLayout();
    }

    uint8_t *data() {
        return &data_[0];
    }

    bool setValue(JSContext *cx, const UnboxedLayout::Property &property, const Value &v);
    Value getValue(const UnboxedLayout::Property &property);

    bool convertToNative(JSContext *cx);

    static UnboxedPlainObject *create(JSContext *cx, HandleObjectGroup group, NewObjectKind newKind);

    static void trace(JSTracer *trc, JSObject *object);

    static size_t offsetOfData() {
        return offsetof(UnboxedPlainObject, data_[0]);
    }
};




bool
TryConvertToUnboxedLayout(JSContext *cx, Shape *templateShape,
                          ObjectGroup *group, PreliminaryObjectArray *objects);

inline gc::AllocKind
UnboxedLayout::getAllocKind() const
{
    return gc::GetGCObjectKindForBytes(UnboxedPlainObject::offsetOfData() + size());
}

} 

#endif 
