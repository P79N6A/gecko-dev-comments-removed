





#include "vm/UnboxedObject.h"

#include "jsobjinlines.h"

#include "vm/Shape-inl.h"

using mozilla::ArrayLength;
using mozilla::DebugOnly;
using mozilla::PodCopy;
using mozilla::UniquePtr;

using namespace js;





void
UnboxedLayout::trace(JSTracer *trc)
{
    for (size_t i = 0; i < properties_.length(); i++)
        MarkStringUnbarriered(trc, &properties_[i].name, "unboxed_layout_name");

    if (newScript())
        newScript()->trace(trc);

    if (nativeGroup_)
        MarkObjectGroup(trc, &nativeGroup_, "unboxed_layout_nativeGroup");

    if (nativeShape_)
        MarkShape(trc, &nativeShape_, "unboxed_layout_nativeShape");

    if (replacementNewGroup_)
        MarkObjectGroup(trc, &replacementNewGroup_, "unboxed_layout_replacementNewGroup");
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
UnboxedLayout::setNewScript(TypeNewScript *newScript, bool writeBarrier )
{
    if (newScript_ && writeBarrier)
        TypeNewScript::writeBarrierPre(newScript_);
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
            MOZ_ASSERT(!IsInsideNursery(v.toString()));
            *reinterpret_cast<PreBarrieredString*>(p) = v.toString();
            return true;
        }
        return false;

      case JSVAL_TYPE_OBJECT:
        if (v.isObjectOrNull()) {
            
            
            
            AddTypePropertyId(cx, this, NameToId(property.name), v);

            
            
            
            JSObject *obj = v.toObjectOrNull();
            if (IsInsideNursery(v.toObjectOrNull()) && !IsInsideNursery(this))
                cx->runtime()->gc.storeBuffer.putWholeCellFromMainThread(this);

            *reinterpret_cast<PreBarrieredObject*>(p) = obj;
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
    const UnboxedLayout &layout = obj->as<UnboxedPlainObject>().layoutDontCheckGeneration();
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
UnboxedLayout::makeNativeGroup(JSContext *cx, ObjectGroup *group)
{
    AutoEnterAnalysis enter(cx);

    UnboxedLayout &layout = group->unboxedLayout();
    Rooted<TaggedProto> proto(cx, group->proto());

    MOZ_ASSERT(!layout.nativeGroup());

    
    
    
    
    
    
    RootedObjectGroup replacementNewGroup(cx);
    if (layout.newScript()) {
        replacementNewGroup = ObjectGroupCompartment::makeGroup(cx, &PlainObject::class_, proto);
        if (!replacementNewGroup)
            return false;

        PlainObject *templateObject = NewObjectWithGroup<PlainObject>(cx, replacementNewGroup,
                                                                      cx->global(), layout.getAllocKind(),
                                                                      MaybeSingletonObject);
        if (!templateObject)
            return false;

        for (size_t i = 0; i < layout.properties().length(); i++) {
            const UnboxedLayout::Property &property = layout.properties()[i];
            if (!templateObject->addDataProperty(cx, NameToId(property.name), i, JSPROP_ENUMERATE))
                return false;
            MOZ_ASSERT(templateObject->slotSpan() == i + 1);
            MOZ_ASSERT(!templateObject->inDictionaryMode());
        }

        TypeNewScript *replacementNewScript =
            TypeNewScript::makeNativeVersion(cx, layout.newScript(), templateObject);
        if (!replacementNewScript)
            return false;

        replacementNewGroup->setNewScript(replacementNewScript);
        gc::TraceTypeNewScript(replacementNewGroup);

        group->clearNewScript(cx, replacementNewGroup);
    }

    size_t nfixed = gc::GetGCKindSlots(layout.getAllocKind());
    RootedShape shape(cx, EmptyShape::getInitialShape(cx, &PlainObject::class_, proto,
                                                      cx->global(), nullptr, nfixed, 0));
    if (!shape)
        return false;

    for (size_t i = 0; i < layout.properties().length(); i++) {
        const UnboxedLayout::Property &property = layout.properties()[i];

        StackShape unrootedChild(shape->base()->unowned(), NameToId(property.name), i,
                                 JSPROP_ENUMERATE, 0);
        RootedGeneric<StackShape*> child(cx, &unrootedChild);
        shape = cx->compartment()->propertyTree.getChild(cx, shape, *child);
        if (!shape)
            return false;
    }

    ObjectGroup *nativeGroup =
        ObjectGroupCompartment::makeGroup(cx, &PlainObject::class_, proto,
                                          group->flags() & OBJECT_FLAG_DYNAMIC_MASK);
    if (!nativeGroup)
        return false;

    
    for (size_t i = 0; i < layout.properties().length(); i++) {
        const UnboxedLayout::Property &property = layout.properties()[i];
        jsid id = NameToId(property.name);

        HeapTypeSet *typeProperty = group->maybeGetProperty(id);
        TypeSet::TypeList types;
        if (!typeProperty->enumerateTypes(&types))
            return false;
        MOZ_ASSERT(!types.empty());
        for (size_t j = 0; j < types.length(); j++)
            AddTypePropertyId(cx, nativeGroup, id, types[j]);
        HeapTypeSet *nativeProperty = nativeGroup->maybeGetProperty(id);
        if (nativeProperty->canSetDefinite(i))
            nativeProperty->setDefinite(i);
    }

    layout.nativeGroup_ = nativeGroup;
    layout.nativeShape_ = shape;
    layout.replacementNewGroup_ = replacementNewGroup;

    nativeGroup->setOriginalUnboxedGroup(group);

    return true;
}

 bool
UnboxedPlainObject::convertToNative(JSContext *cx, JSObject *obj)
{
    const UnboxedLayout &layout = obj->as<UnboxedPlainObject>().layout();

    if (!layout.nativeGroup()) {
        if (!UnboxedLayout::makeNativeGroup(cx, obj->group()))
            return false;

        
        if (obj->is<PlainObject>())
            return true;
    }

    AutoValueVector values(cx);
    for (size_t i = 0; i < layout.properties().length(); i++) {
        if (!values.append(obj->as<UnboxedPlainObject>().getValue(layout.properties()[i])))
            return false;
    }

    uint32_t objectFlags = obj->lastProperty()->getObjectFlags();
    RootedObject metadata(cx, obj->getMetadata());

    obj->setGroup(layout.nativeGroup());
    obj->as<PlainObject>().setLastPropertyMakeNative(cx, layout.nativeShape());

    for (size_t i = 0; i < values.length(); i++)
        obj->as<PlainObject>().initSlotUnchecked(i, values[i]);

    if (objectFlags) {
        RootedObject objRoot(cx, obj);
        if (!obj->setFlags(cx, objectFlags))
            return false;
        obj = objRoot;
    }

    if (metadata) {
        RootedObject objRoot(cx, obj);
        RootedObject metadataRoot(cx, metadata);
        if (!setMetadata(cx, objRoot, metadataRoot))
            return false;
    }

    return true;
}


UnboxedPlainObject *
UnboxedPlainObject::create(JSContext *cx, HandleObjectGroup group, NewObjectKind newKind)
{
    MOZ_ASSERT(group->clasp() == &class_);
    gc::AllocKind allocKind = group->unboxedLayout().getAllocKind();

    UnboxedPlainObject *res = NewObjectWithGroup<UnboxedPlainObject>(cx, group, cx->global(),
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
UnboxedPlainObject::obj_lookupProperty(JSContext *cx, HandleObject obj,
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
UnboxedPlainObject::obj_defineProperty(JSContext *cx, HandleObject obj, HandleId id, HandleValue v,
                                       PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    if (!convertToNative(cx, obj))
        return false;

    return DefineProperty(cx, obj, id, v, getter, setter, attrs);
}

 bool
UnboxedPlainObject::obj_hasProperty(JSContext *cx, HandleObject obj, HandleId id, bool *foundp)
{
    if (obj->as<UnboxedPlainObject>().layout().lookup(id)) {
        *foundp = true;
        return true;
    }

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        *foundp = false;
        return true;
    }

    return HasProperty(cx, proto, id, foundp);
}

 bool
UnboxedPlainObject::obj_getProperty(JSContext *cx, HandleObject obj, HandleObject receiver,
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
UnboxedPlainObject::obj_setProperty(JSContext *cx, HandleObject obj, HandleObject receiver,
                                    HandleId id, MutableHandleValue vp, bool strict)
{
    const UnboxedLayout &layout = obj->as<UnboxedPlainObject>().layout();

    if (const UnboxedLayout::Property *property = layout.lookup(id)) {
        if (obj == receiver) {
            if (obj->as<UnboxedPlainObject>().setValue(cx, *property, vp))
                return true;

            if (!convertToNative(cx, obj))
                return false;
            return SetProperty(cx, obj, receiver, id, vp, strict);
        }

        return SetPropertyByDefining(cx, obj, receiver, id, vp, strict, false);
    }

    return SetPropertyOnProto(cx, obj, receiver, id, vp, strict);
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
UnboxedPlainObject::obj_deleteProperty(JSContext *cx, HandleObject obj, HandleId id,
                                       bool *succeeded)
{
    if (!convertToNative(cx, obj))
        return false;
    return DeleteProperty(cx, obj, id, succeeded);
}

 bool
UnboxedPlainObject::obj_watch(JSContext *cx, HandleObject obj, HandleId id, HandleObject callable)
{
    if (!convertToNative(cx, obj))
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
        UnboxedPlainObject::obj_lookupProperty,
        UnboxedPlainObject::obj_defineProperty,
        UnboxedPlainObject::obj_hasProperty,
        UnboxedPlainObject::obj_getProperty,
        UnboxedPlainObject::obj_setProperty,
        UnboxedPlainObject::obj_getOwnPropertyDescriptor,
        UnboxedPlainObject::obj_deleteProperty,
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



static bool
PropertiesAreSuperset(const UnboxedLayout::PropertyVector &properties, UnboxedLayout *layout)
{
    for (size_t i = 0; i < layout->properties().length(); i++) {
        const UnboxedLayout::Property &layoutProperty = layout->properties()[i];
        bool found = false;
        for (size_t j = 0; j < properties.length(); j++) {
            if (layoutProperty.name == properties[j].name) {
                found = (layoutProperty.type == properties[j].type);
                break;
            }
        }
        if (!found)
            return false;
    }
    return true;
}

bool
js::TryConvertToUnboxedLayout(JSContext *cx, Shape *templateShape,
                              ObjectGroup *group, PreliminaryObjectArray *objects)
{
    if (!cx->runtime()->options().unboxedObjects())
        return true;

    if (templateShape->slotSpan() == 0)
        return true;

    UnboxedLayout::PropertyVector properties;
    if (!properties.appendN(UnboxedLayout::Property(), templateShape->slotSpan()))
        return false;

    size_t objectCount = 0;
    for (size_t i = 0; i < PreliminaryObjectArray::COUNT; i++) {
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

    
    
    
    
    
    UnboxedLayout *bestExisting = nullptr;
    for (UnboxedLayout *existing = cx->compartment()->unboxedLayouts.getFirst();
         existing;
         existing = existing->getNext())
    {
        if (PropertiesAreSuperset(properties, existing)) {
            if (!bestExisting ||
                existing->properties().length() > bestExisting->properties().length())
            {
                bestExisting = existing;
            }
        }
    }
    if (bestExisting) {
        for (size_t i = 0; i < bestExisting->properties().length(); i++) {
            const UnboxedLayout::Property &existingProperty = bestExisting->properties()[i];
            for (size_t j = 0; j < templateShape->slotSpan(); j++) {
                if (existingProperty.name == properties[j].name) {
                    MOZ_ASSERT(existingProperty.type == properties[j].type);
                    properties[j].offset = existingProperty.offset;
                }
            }
        }
        offset = bestExisting->size();
    }

    
    
    static const size_t typeSizes[] = { 8, 4, 1 };

    for (size_t i = 0; i < ArrayLength(typeSizes); i++) {
        size_t size = typeSizes[i];
        for (size_t j = 0; j < templateShape->slotSpan(); j++) {
            if (properties[j].offset != UINT32_MAX)
                continue;
            JSValueType type = properties[j].type;
            if (UnboxedTypeSize(type) == size) {
                offset = JS_ROUNDUP(offset, size);
                properties[j].offset = offset;
                offset += size;
            }
        }
    }

    
    if (sizeof(JSObject) + offset > JSObject::MAX_BYTE_SIZE)
        return true;

    UniquePtr<UnboxedLayout, JS::DeletePolicy<UnboxedLayout> > layout;
    layout.reset(group->zone()->new_<UnboxedLayout>(properties, offset));
    if (!layout)
        return false;

    cx->compartment()->unboxedLayouts.insertFront(layout.get());

    
    Vector<int32_t, 8, SystemAllocPolicy> objectOffsets, stringOffsets;
    for (size_t i = 0; i < templateShape->slotSpan(); i++) {
        MOZ_ASSERT(properties[i].offset != UINT32_MAX);
        JSValueType type = properties[i].type;
        if (type == JSVAL_TYPE_OBJECT) {
            if (!objectOffsets.append(properties[i].offset))
                return false;
        } else if (type == JSVAL_TYPE_STRING) {
            if (!stringOffsets.append(properties[i].offset))
                return false;
        }
    }

    
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
        int32_t *traceList = group->zone()->pod_malloc<int32_t>(entries.length());
        if (!traceList)
            return false;
        PodCopy(traceList, entries.begin(), entries.length());
        layout->setTraceList(traceList);
    }

    
    
    
    
    

    
    Shape *newShape = EmptyShape::getInitialShape(cx, &UnboxedPlainObject::class_,
                                                  group->proto(),
                                                  templateShape->getObjectParent(),
                                                  templateShape->getObjectMetadata(),
                                                  templateShape->getObjectFlags());
    if (!newShape) {
        cx->clearPendingException();
        return false;
    }

    
    
    Vector<Value, 0, SystemAllocPolicy> values;
    if (!values.reserve(objectCount * templateShape->slotSpan()))
        return false;
    for (size_t i = 0; i < PreliminaryObjectArray::COUNT; i++) {
        if (!objects->get(i))
            continue;

        RootedNativeObject obj(cx, &objects->get(i)->as<NativeObject>());
        for (size_t j = 0; j < templateShape->slotSpan(); j++)
            values.infallibleAppend(obj->getSlot(j));

        
        NativeObject::clear(cx, obj);

        obj->setLastPropertyMakeNonNative(newShape);
    }

    if (TypeNewScript *newScript = group->newScript())
        layout->setNewScript(newScript);

    group->setClasp(&UnboxedPlainObject::class_);
    group->setUnboxedLayout(layout.get());

    size_t valueCursor = 0;
    for (size_t i = 0; i < PreliminaryObjectArray::COUNT; i++) {
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
    layout.release();
    return true;
}
