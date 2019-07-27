





#include "vm/UnboxedObject.h"

#include "jsinferinlines.h"
#include "jsobjinlines.h"

#include "vm/Shape-inl.h"

using mozilla::ArrayLength;
using mozilla::DebugOnly;
using mozilla::PodCopy;

using namespace js;





void
UnboxedLayout::trace(JSTracer *trc)
{
    for (size_t i = 0; i < properties_.length(); i++)
        MarkStringUnbarriered(trc, &properties_[i].name, "unboxed_layout_name");

    if (newScript())
        newScript()->trace(trc);
}

size_t
UnboxedLayout::sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf)
{
    return mallocSizeOf(this)
         + properties_.sizeOfExcludingThis(mallocSizeOf)
         + (newScript() ? newScript()->sizeOfIncludingThis(mallocSizeOf) : 0)
         + mallocSizeOf(traceList());
}

void
UnboxedLayout::setNewScript(types::TypeNewScript *newScript, bool writeBarrier )
{
    if (newScript_ && writeBarrier)
        types::TypeNewScript::writeBarrierPre(newScript_);
    newScript_ = newScript;
}





bool
UnboxedPlainObject::setValue(JSContext *cx, const UnboxedLayout::Property &property, const Value &v)
{
    uint8_t *p = &data_[property.offset];

    switch (property.type) {
      case JSVAL_TYPE_BOOLEAN:
        if (v.isBoolean()) {
            *p = v.toBoolean();
            return true;
        }
        return false;

      case JSVAL_TYPE_INT32:
        if (v.isInt32()) {
            *reinterpret_cast<int32_t*>(p) = v.toInt32();
            return true;
        }
        return false;

      case JSVAL_TYPE_DOUBLE:
        if (v.isNumber()) {
            *reinterpret_cast<double*>(p) = v.toNumber();
            return true;
        }
        return false;

      case JSVAL_TYPE_STRING:
        if (v.isString()) {
            *reinterpret_cast<HeapPtrString*>(p) = v.toString();
            return true;
        }
        return false;

      case JSVAL_TYPE_OBJECT:
        if (v.isObjectOrNull()) {
            
            
            
            types::AddTypePropertyId(cx, this, NameToId(property.name), v);

            *reinterpret_cast<HeapPtrObject*>(p) = v.toObjectOrNull();
            return true;
        }
        return false;

      default:
        MOZ_CRASH("Invalid type for unboxed value");
    }
}

Value
UnboxedPlainObject::getValue(const UnboxedLayout::Property &property)
{
    uint8_t *p = &data_[property.offset];

    switch (property.type) {
      case JSVAL_TYPE_BOOLEAN:
        return BooleanValue(*p != 0);

      case JSVAL_TYPE_INT32:
        return Int32Value(*reinterpret_cast<int32_t*>(p));

      case JSVAL_TYPE_DOUBLE:
        return DoubleValue(*reinterpret_cast<double*>(p));

      case JSVAL_TYPE_STRING:
        return StringValue(*reinterpret_cast<JSString**>(p));

      case JSVAL_TYPE_OBJECT:
        return ObjectOrNullValue(*reinterpret_cast<JSObject**>(p));

      default:
        MOZ_CRASH("Invalid type for unboxed value");
    }
}

void
UnboxedPlainObject::trace(JSTracer *trc, JSObject *obj)
{
    const UnboxedLayout &layout = obj->as<UnboxedPlainObject>().layout();
    const int32_t *list = layout.traceList();
    if (!list)
        return;

    uint8_t *data = obj->as<UnboxedPlainObject>().data();
    while (*list != -1) {
        HeapPtrString *heap = reinterpret_cast<HeapPtrString *>(data + *list);
        MarkString(trc, heap, "unboxed_string");
        list++;
    }
    list++;
    while (*list != -1) {
        HeapPtrObject *heap = reinterpret_cast<HeapPtrObject *>(data + *list);
        if (*heap)
            MarkObject(trc, heap, "unboxed_object");
        list++;
    }

    
    MOZ_ASSERT(*(list + 1) == -1);
}

bool
UnboxedPlainObject::convertToNative(JSContext *cx)
{
    
    
    
    type()->clearNewScript(cx);

    
    if (!is<UnboxedPlainObject>())
        return true;

    Rooted<UnboxedPlainObject *> obj(cx, this);
    Rooted<TaggedProto> proto(cx, getTaggedProto());

    size_t nfixed = gc::GetGCKindSlots(obj->layout().getAllocKind());

    AutoValueVector values(cx);
    RootedShape shape(cx, EmptyShape::getInitialShape(cx, &PlainObject::class_, proto,
                                                      getMetadata(), getParent(), nfixed,
                                                      lastProperty()->getObjectFlags()));
    if (!shape)
        return false;

    for (size_t i = 0; i < obj->layout().properties().length(); i++) {
        const UnboxedLayout::Property &property = obj->layout().properties()[i];

        if (!values.append(obj->getValue(property)))
            return false;

        StackShape unrootedChild(shape->base()->unowned(), NameToId(property.name), i,
                                 JSPROP_ENUMERATE, 0);
        RootedGeneric<StackShape*> child(cx, &unrootedChild);
        shape = cx->compartment()->propertyTree.getChild(cx, shape, *child);
        if (!shape)
            return false;
    }

    if (!SetClassAndProto(cx, obj, &PlainObject::class_, proto))
        return false;

    RootedNativeObject nobj(cx, &obj->as<PlainObject>());
    NativeObject::setLastPropertyMakeNative(cx, nobj, shape);

    for (size_t i = 0; i < values.length(); i++)
        nobj->initSlotUnchecked(i, values[i]);

    return true;
}


UnboxedPlainObject *
UnboxedPlainObject::create(JSContext *cx, HandleTypeObject type, NewObjectKind newKind)
{
    MOZ_ASSERT(type->clasp() == &class_);
    gc::AllocKind allocKind = type->unboxedLayout().getAllocKind();

    UnboxedPlainObject *res = NewObjectWithType<UnboxedPlainObject>(cx, type, cx->global(),
                                                                    allocKind, newKind);
    if (!res)
        return nullptr;

    
    
    const int32_t *list = res->layout().traceList();
    if (list) {
        uint8_t *data = res->data();
        while (*list != -1) {
            HeapPtrString *heap = reinterpret_cast<HeapPtrString *>(data + *list);
            heap->init(cx->names().empty);
            list++;
        }
        list++;
        while (*list != -1) {
            HeapPtrObject *heap = reinterpret_cast<HeapPtrObject *>(data + *list);
            heap->init(nullptr);
            list++;
        }
        
        MOZ_ASSERT(*(list + 1) == -1);
    }

    return res;
}

 bool
UnboxedPlainObject::obj_lookupGeneric(JSContext *cx, HandleObject obj,
                                      HandleId id, MutableHandleObject objp,
                                      MutableHandleShape propp)
{
    if (obj->as<UnboxedPlainObject>().layout().lookup(id)) {
        MarkNonNativePropertyFound<CanGC>(propp);
        objp.set(obj);
        return true;
    }

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        objp.set(nullptr);
        propp.set(nullptr);
        return true;
    }

    return LookupProperty(cx, proto, id, objp, propp);
}

 bool
UnboxedPlainObject::obj_lookupProperty(JSContext *cx, HandleObject obj,
                                       HandlePropertyName name,
                                       MutableHandleObject objp,
                                       MutableHandleShape propp)
{
    RootedId id(cx, NameToId(name));
    return obj_lookupGeneric(cx, obj, id, objp, propp);
}

 bool
UnboxedPlainObject::obj_lookupElement(JSContext *cx, HandleObject obj,
                                      uint32_t index, MutableHandleObject objp,
                                      MutableHandleShape propp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return obj_lookupGeneric(cx, obj, id, objp, propp);
}

 bool
UnboxedPlainObject::obj_defineGeneric(JSContext *cx, HandleObject obj, HandleId id, HandleValue v,
                                      PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    if (!obj->as<UnboxedPlainObject>().convertToNative(cx))
        return false;

    return DefineProperty(cx, obj, id, v, getter, setter, attrs);
}

 bool
UnboxedPlainObject::obj_defineProperty(JSContext *cx, HandleObject obj,
                                       HandlePropertyName name, HandleValue v,
                                       PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    Rooted<jsid> id(cx, NameToId(name));
    return obj_defineGeneric(cx, obj, id, v, getter, setter, attrs);
}

 bool
UnboxedPlainObject::obj_defineElement(JSContext *cx, HandleObject obj, uint32_t index, HandleValue v,
                                      PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return obj_defineGeneric(cx, obj, id, v, getter, setter, attrs);
}

 bool
UnboxedPlainObject::obj_getGeneric(JSContext *cx, HandleObject obj, HandleObject receiver,
                                   HandleId id, MutableHandleValue vp)
{
    const UnboxedLayout &layout = obj->as<UnboxedPlainObject>().layout();

    if (const UnboxedLayout::Property *property = layout.lookup(id)) {
        vp.set(obj->as<UnboxedPlainObject>().getValue(*property));
        return true;
    }

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        vp.setUndefined();
        return true;
    }

    return GetProperty(cx, proto, receiver, id, vp);
}

 bool
UnboxedPlainObject::obj_getProperty(JSContext *cx, HandleObject obj, HandleObject receiver,
                                    HandlePropertyName name, MutableHandleValue vp)
{
    RootedId id(cx, NameToId(name));
    return obj_getGeneric(cx, obj, receiver, id, vp);
}

 bool
UnboxedPlainObject::obj_getElement(JSContext *cx, HandleObject obj, HandleObject receiver,
                                   uint32_t index, MutableHandleValue vp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return obj_getGeneric(cx, obj, receiver, id, vp);
}

 bool
UnboxedPlainObject::obj_setGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                   MutableHandleValue vp, bool strict)
{
    const UnboxedLayout &layout = obj->as<UnboxedPlainObject>().layout();

    if (const UnboxedLayout::Property *property = layout.lookup(id)) {
        if (obj->as<UnboxedPlainObject>().setValue(cx, *property, vp))
            return true;

        if (!obj->as<UnboxedPlainObject>().convertToNative(cx))
            return false;
        return SetProperty(cx, obj, obj, id, vp, strict);
    }

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        if (!obj->as<UnboxedPlainObject>().convertToNative(cx))
            return false;
        return SetProperty(cx, obj, obj, id, vp, strict);
    }

    return SetProperty(cx, proto, obj, id, vp, strict);
}

 bool
UnboxedPlainObject::obj_setProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                    MutableHandleValue vp, bool strict)
{
    RootedId id(cx, NameToId(name));
    return obj_setGeneric(cx, obj, id, vp, strict);
}

 bool
UnboxedPlainObject::obj_setElement(JSContext *cx, HandleObject obj, uint32_t index,
                                   MutableHandleValue vp, bool strict)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return obj_setGeneric(cx, obj, id, vp, strict);
}

 bool
UnboxedPlainObject::obj_getOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id,
                                                 MutableHandle<JSPropertyDescriptor> desc)
{
    const UnboxedLayout &layout = obj->as<UnboxedPlainObject>().layout();

    if (const UnboxedLayout::Property *property = layout.lookup(id)) {
        desc.value().set(obj->as<UnboxedPlainObject>().getValue(*property));
        desc.setAttributes(JSPROP_ENUMERATE);
        desc.object().set(obj);
        return true;
    }

    desc.object().set(nullptr);
    return true;
}

 bool
UnboxedPlainObject::obj_setGenericAttributes(JSContext *cx, HandleObject obj,
                                             HandleId id, unsigned *attrsp)
{
    if (!obj->as<UnboxedPlainObject>().convertToNative(cx))
        return false;
    return SetPropertyAttributes(cx, obj, id, attrsp);
}

 bool
UnboxedPlainObject::obj_deleteGeneric(JSContext *cx, HandleObject obj, HandleId id, bool *succeeded)
{
    if (!obj->as<UnboxedPlainObject>().convertToNative(cx))
        return false;
    return DeleteProperty(cx, obj, id, succeeded);
}

 bool
UnboxedPlainObject::obj_watch(JSContext *cx, HandleObject obj, HandleId id, HandleObject callable)
{
    if (!obj->as<UnboxedPlainObject>().convertToNative(cx))
        return false;
    return WatchProperty(cx, obj, id, callable);
}

 bool
UnboxedPlainObject::obj_enumerate(JSContext *cx, HandleObject obj, AutoIdVector &properties)
{
    const UnboxedLayout::PropertyVector &unboxed = obj->as<UnboxedPlainObject>().layout().properties();
    for (size_t i = 0; i < unboxed.length(); i++) {
        if (!properties.append(NameToId(unboxed[i].name)))
            return false;
    }
    return true;
}

const Class UnboxedPlainObject::class_ = {
    "Object",
    Class::NON_NATIVE | JSCLASS_IMPLEMENTS_BARRIERS,
    nullptr,        
    nullptr,        
    nullptr,        
    nullptr,        
    nullptr,        
    nullptr,        
    nullptr,        
    nullptr,        
    nullptr,        
    nullptr,        
    nullptr,        
    UnboxedPlainObject::trace,
    JS_NULL_CLASS_SPEC,
    JS_NULL_CLASS_EXT,
    {
        UnboxedPlainObject::obj_lookupGeneric,
        UnboxedPlainObject::obj_lookupProperty,
        UnboxedPlainObject::obj_lookupElement,
        UnboxedPlainObject::obj_defineGeneric,
        UnboxedPlainObject::obj_defineProperty,
        UnboxedPlainObject::obj_defineElement,
        UnboxedPlainObject::obj_getGeneric,
        UnboxedPlainObject::obj_getProperty,
        UnboxedPlainObject::obj_getElement,
        UnboxedPlainObject::obj_setGeneric,
        UnboxedPlainObject::obj_setProperty,
        UnboxedPlainObject::obj_setElement,
        UnboxedPlainObject::obj_getOwnPropertyDescriptor,
        UnboxedPlainObject::obj_setGenericAttributes,
        UnboxedPlainObject::obj_deleteGeneric,
        UnboxedPlainObject::obj_watch,
        nullptr,   
        nullptr,   
        UnboxedPlainObject::obj_enumerate,
        nullptr, 
    }
};





static bool
UnboxedTypeIncludes(JSValueType supertype, JSValueType subtype)
{
    if (supertype == JSVAL_TYPE_DOUBLE && subtype == JSVAL_TYPE_INT32)
        return true;
    if (supertype == JSVAL_TYPE_OBJECT && subtype == JSVAL_TYPE_NULL)
        return true;
    return false;
}

bool
js::TryConvertToUnboxedLayout(JSContext *cx, Shape *templateShape,
                              types::TypeObject *type, types::PreliminaryObjectArray *objects)
{
    if (!cx->runtime()->options().unboxedObjects())
        return true;

    if (templateShape->slotSpan() == 0)
        return true;

    UnboxedLayout::PropertyVector properties;
    if (!properties.appendN(UnboxedLayout::Property(), templateShape->slotSpan()))
        return false;

    size_t objectCount = 0;
    for (size_t i = 0; i < types::PreliminaryObjectArray::COUNT; i++) {
        JSObject *obj = objects->get(i);
        if (!obj)
            continue;

        objectCount++;

        
        
        
        MOZ_ASSERT(gc::GetGCKindSlots(obj->asTenured().getAllocKind()) ==
                   NativeObject::MAX_FIXED_SLOTS);

        if (obj->as<PlainObject>().lastProperty() != templateShape ||
            obj->as<PlainObject>().hasDynamicElements())
        {
            
            
            return true;
        }

        for (size_t i = 0; i < templateShape->slotSpan(); i++) {
            Value val = obj->as<PlainObject>().getSlot(i);

            JSValueType &existing = properties[i].type;
            JSValueType type = val.isDouble() ? JSVAL_TYPE_DOUBLE : val.extractNonDoubleType();

            if (existing == JSVAL_TYPE_MAGIC || existing == type || UnboxedTypeIncludes(type, existing))
                existing = type;
            else if (!UnboxedTypeIncludes(existing, type))
                return true;
        }
    }

    if (objectCount <= 1) {
        
        
        return true;
    }

    for (size_t i = 0; i < templateShape->slotSpan(); i++) {
        
        
        
        if (UnboxedTypeSize(properties[i].type) == 0)
            return true;
    }

    
    for (Shape::Range<NoGC> r(templateShape); !r.empty(); r.popFront()) {
        size_t slot = r.front().slot();
        MOZ_ASSERT(!properties[slot].name);
        properties[slot].name = JSID_TO_ATOM(r.front().propid())->asPropertyName();
    }

    
    
    uint32_t offset = 0;

    static const size_t typeSizes[] = { 8, 4, 1 };

    Vector<int32_t, 8, SystemAllocPolicy> objectOffsets, stringOffsets;

    DebugOnly<size_t> addedProperties = 0;
    for (size_t i = 0; i < ArrayLength(typeSizes); i++) {
        size_t size = typeSizes[i];
        for (size_t j = 0; j < templateShape->slotSpan(); j++) {
            JSValueType type = properties[j].type;
            if (UnboxedTypeSize(type) == size) {
                if (type == JSVAL_TYPE_OBJECT) {
                    if (!objectOffsets.append(offset))
                        return false;
                } else if (type == JSVAL_TYPE_STRING) {
                    if (!stringOffsets.append(offset))
                        return false;
                }
                addedProperties++;
                properties[j].offset = offset;
                offset += size;
            }
        }
    }
    MOZ_ASSERT(addedProperties == templateShape->slotSpan());

    
    if (sizeof(JSObject) + offset > JSObject::MAX_BYTE_SIZE)
        return true;

    UnboxedLayout *layout = type->zone()->new_<UnboxedLayout>(properties, offset);
    if (!layout)
        return false;

    
    if (!objectOffsets.empty() || !stringOffsets.empty()) {
        Vector<int32_t, 8, SystemAllocPolicy> entries;
        if (!entries.appendAll(stringOffsets) ||
            !entries.append(-1) ||
            !entries.appendAll(objectOffsets) ||
            !entries.append(-1) ||
            !entries.append(-1))
        {
            return false;
        }
        int32_t *traceList = type->zone()->pod_malloc<int32_t>(entries.length());
        if (!traceList)
            return false;
        PodCopy(traceList, entries.begin(), entries.length());
        layout->setTraceList(traceList);
    }

    
    
    
    
    

    
    Shape *newShape = EmptyShape::getInitialShape(cx, &UnboxedPlainObject::class_,
                                                  type->proto(),
                                                  templateShape->getObjectMetadata(),
                                                  templateShape->getObjectParent(),
                                                  templateShape->getObjectFlags());
    if (!newShape) {
        cx->clearPendingException();
        return false;
    }

    
    
    Vector<Value, 0, SystemAllocPolicy> values;
    if (!values.reserve(objectCount * templateShape->slotSpan()))
        return false;
    for (size_t i = 0; i < types::PreliminaryObjectArray::COUNT; i++) {
        if (!objects->get(i))
            continue;

        RootedNativeObject obj(cx, &objects->get(i)->as<NativeObject>());
        for (size_t j = 0; j < templateShape->slotSpan(); j++)
            values.infallibleAppend(obj->getSlot(j));

        
        NativeObject::clear(cx, obj);

        obj->setLastPropertyMakeNonNative(newShape);
    }

    if (types::TypeNewScript *newScript = type->newScript())
        layout->setNewScript(newScript);

    type->setClasp(&UnboxedPlainObject::class_);
    type->setUnboxedLayout(layout);

    size_t valueCursor = 0;
    for (size_t i = 0; i < types::PreliminaryObjectArray::COUNT; i++) {
        if (!objects->get(i))
            continue;
        UnboxedPlainObject *obj = &objects->get(i)->as<UnboxedPlainObject>();
        memset(obj->data(), 0, layout->size());
        for (size_t j = 0; j < templateShape->slotSpan(); j++) {
            Value v = values[valueCursor++];
            JS_ALWAYS_TRUE(obj->setValue(cx, properties[j], v));
        }
    }

    MOZ_ASSERT(valueCursor == values.length());
    return true;
}
