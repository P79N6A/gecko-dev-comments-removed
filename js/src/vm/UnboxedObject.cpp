





#include "vm/UnboxedObject.h"

#include "jit/JitCommon.h"
#include "jit/Linker.h"

#include "jsobjinlines.h"

#include "vm/Shape-inl.h"

using mozilla::ArrayLength;
using mozilla::DebugOnly;
using mozilla::PodCopy;
using mozilla::UniquePtr;

using namespace js;





void
UnboxedLayout::trace(JSTracer* trc)
{
    for (size_t i = 0; i < properties_.length(); i++)
        TraceManuallyBarrieredEdge(trc, &properties_[i].name, "unboxed_layout_name");

    if (newScript())
        newScript()->trace(trc);

    if (nativeGroup_)
        TraceEdge(trc, &nativeGroup_, "unboxed_layout_nativeGroup");

    if (nativeShape_)
        TraceEdge(trc, &nativeShape_, "unboxed_layout_nativeShape");

    if (replacementNewGroup_)
        TraceEdge(trc, &replacementNewGroup_, "unboxed_layout_replacementNewGroup");

    if (constructorCode_)
        TraceEdge(trc, &constructorCode_, "unboxed_layout_constructorCode");
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
UnboxedLayout::setNewScript(TypeNewScript* newScript, bool writeBarrier )
{
    if (newScript_ && writeBarrier)
        TypeNewScript::writeBarrierPre(newScript_);
    newScript_ = newScript;
}



static const uintptr_t CLEAR_CONSTRUCTOR_CODE_TOKEN = 0x1;

 bool
UnboxedLayout::makeConstructorCode(JSContext* cx, HandleObjectGroup group)
{
    using namespace jit;

    if (!cx->compartment()->ensureJitCompartmentExists(cx))
        return false;

    UnboxedLayout& layout = group->unboxedLayout();
    MOZ_ASSERT(!layout.constructorCode());

    UnboxedPlainObject* templateObject = UnboxedPlainObject::create(cx, group, TenuredObject);
    if (!templateObject)
        return false;

    JitContext jitContext(cx, nullptr);

    MacroAssembler masm;

    Register propertiesReg, newKindReg;
#ifdef JS_CODEGEN_X86
    propertiesReg = eax;
    newKindReg = ecx;
    masm.loadPtr(Address(masm.getStackPointer(), sizeof(void*)), propertiesReg);
    masm.loadPtr(Address(masm.getStackPointer(), 2 * sizeof(void*)), newKindReg);
#else
    propertiesReg = IntArgReg0;
    newKindReg = IntArgReg1;
#endif

    MOZ_ASSERT(propertiesReg.volatile_());
    MOZ_ASSERT(newKindReg.volatile_());

    AllocatableGeneralRegisterSet regs(GeneralRegisterSet::All());
    regs.take(propertiesReg);
    regs.take(newKindReg);
    Register object = regs.takeAny(), scratch1 = regs.takeAny(), scratch2 = regs.takeAny();

    LiveGeneralRegisterSet savedNonVolatileRegisters = SavedNonVolatileRegisters(regs);
    for (GeneralRegisterForwardIterator iter(savedNonVolatileRegisters); iter.more(); ++iter)
        masm.Push(*iter);

    
    if (ScratchDoubleReg.volatile_())
        masm.push(ScratchDoubleReg);

    Label failure, tenuredObject, allocated;
    masm.branch32(Assembler::NotEqual, newKindReg, Imm32(GenericObject), &tenuredObject);
    masm.branchTest32(Assembler::NonZero, AbsoluteAddress(group->addressOfFlags()),
                      Imm32(OBJECT_FLAG_PRE_TENURE), &tenuredObject);

    
    masm.createGCObject(object, scratch1, templateObject, gc::DefaultHeap, &failure,
                         false);

    masm.jump(&allocated);
    masm.bind(&tenuredObject);

    
    masm.createGCObject(object, scratch1, templateObject, gc::TenuredHeap, &failure,
                         false);

    
    
    Label postBarrier;
    for (size_t i = 0; i < layout.properties().length(); i++) {
        const UnboxedLayout::Property& property = layout.properties()[i];
        if (property.type == JSVAL_TYPE_OBJECT) {
            Address valueAddress(propertiesReg, i * sizeof(IdValuePair) + offsetof(IdValuePair, value));
            Label notObject;
            masm.branchTestObject(Assembler::NotEqual, valueAddress, &notObject);
            Register valueObject = masm.extractObject(valueAddress, scratch1);
            masm.branchPtrInNurseryRange(Assembler::Equal, valueObject, scratch2, &postBarrier);
            masm.bind(&notObject);
        }
    }

    masm.jump(&allocated);
    masm.bind(&postBarrier);

    masm.mov(ImmPtr(cx->runtime()), scratch1);
    masm.setupUnalignedABICall(2, scratch2);
    masm.passABIArg(scratch1);
    masm.passABIArg(object);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void*, PostWriteBarrier));

    masm.bind(&allocated);

    ValueOperand valueOperand;
#ifdef JS_NUNBOX32
    valueOperand = ValueOperand(scratch1, scratch2);
#else
    valueOperand = ValueOperand(scratch1);
#endif

    Label failureStoreOther, failureStoreObject;

    for (size_t i = 0; i < layout.properties().length(); i++) {
        const UnboxedLayout::Property& property = layout.properties()[i];
        Address valueAddress(propertiesReg, i * sizeof(IdValuePair) + offsetof(IdValuePair, value));
        Address targetAddress(object, UnboxedPlainObject::offsetOfData() + property.offset);

        masm.loadValue(valueAddress, valueOperand);

        if (property.type == JSVAL_TYPE_OBJECT) {
            HeapTypeSet* types = group->maybeGetProperty(IdToTypeId(NameToId(property.name)));

            Label notObject;
            masm.branchTestObject(Assembler::NotEqual, valueOperand,
                                  types->mightBeMIRType(MIRType_Null) ? &notObject : &failureStoreObject);

            Register payloadReg = masm.extractObject(valueOperand, scratch1);

            if (!types->hasType(TypeSet::AnyObjectType())) {
                Register scratch = (payloadReg == scratch1) ? scratch2 : scratch1;
                masm.guardObjectType(payloadReg, types, scratch, &failureStoreObject);
            }

            masm.storeUnboxedProperty(targetAddress, JSVAL_TYPE_OBJECT,
                                      TypedOrValueRegister(MIRType_Object,
                                                           AnyRegister(payloadReg)), nullptr);

            if (notObject.used()) {
                Label done;
                masm.jump(&done);
                masm.bind(&notObject);
                masm.branchTestNull(Assembler::NotEqual, valueOperand, &failureStoreOther);
                masm.storeUnboxedProperty(targetAddress, JSVAL_TYPE_OBJECT, NullValue(), nullptr);
                masm.bind(&done);
            }
        } else {
            masm.storeUnboxedProperty(targetAddress, property.type,
                                      ConstantOrRegister(valueOperand), &failureStoreOther);
        }
    }

    Label done;
    masm.bind(&done);

    if (object != ReturnReg)
        masm.movePtr(object, ReturnReg);

    
    if (ScratchDoubleReg.volatile_())
        masm.pop(ScratchDoubleReg);
    for (GeneralRegisterBackwardIterator iter(savedNonVolatileRegisters); iter.more(); ++iter)
        masm.Pop(*iter);

    masm.abiret();

    masm.bind(&failureStoreOther);

    
    
    
    masm.initUnboxedObjectContents(object, templateObject);

    masm.bind(&failure);

    masm.movePtr(ImmWord(0), object);
    masm.jump(&done);

    masm.bind(&failureStoreObject);

    
    
    
    
    {
        Label isObject;
        masm.branchTestObject(Assembler::Equal, valueOperand, &isObject);
        masm.branchTestNull(Assembler::NotEqual, valueOperand, &failureStoreOther);
        masm.bind(&isObject);
    }

    
    masm.initUnboxedObjectContents(object, templateObject);

    masm.movePtr(ImmWord(CLEAR_CONSTRUCTOR_CODE_TOKEN), object);
    masm.jump(&done);

    Linker linker(masm);
    AutoFlushICache afc("UnboxedObject");
    JitCode* code = linker.newCode<NoGC>(cx, OTHER_CODE);
    if (!code)
        return false;

    layout.setConstructorCode(code);
    return true;
}

void
UnboxedLayout::detachFromCompartment()
{
    if (isInList())
        remove();
}

static inline bool
SetUnboxedValue(ExclusiveContext* cx, JSObject* unboxedObject, jsid id,
                uint8_t* p, JSValueType type, const Value& v)
{
    switch (type) {
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
            
            
            
            AddTypePropertyId(cx, unboxedObject, id, v);

            
            
            
            JSObject* obj = v.toObjectOrNull();
            if (IsInsideNursery(v.toObjectOrNull()) && !IsInsideNursery(unboxedObject)) {
                JSRuntime* rt = cx->asJSContext()->runtime();
                rt->gc.storeBuffer.putWholeCellFromMainThread(unboxedObject);
            }

            *reinterpret_cast<PreBarrieredObject*>(p) = obj;
            return true;
        }
        return false;

      default:
        MOZ_CRASH("Invalid type for unboxed value");
    }
}

static inline Value
GetUnboxedValue(uint8_t* p, JSValueType type)
{
    switch (type) {
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





bool
UnboxedPlainObject::setValue(ExclusiveContext* cx, const UnboxedLayout::Property& property,
                             const Value& v)
{
    uint8_t* p = &data_[property.offset];
    return SetUnboxedValue(cx, this, NameToId(property.name), p, property.type, v);
}

Value
UnboxedPlainObject::getValue(const UnboxedLayout::Property& property)
{
    uint8_t* p = &data_[property.offset];
    return GetUnboxedValue(p, property.type);
}

void
UnboxedPlainObject::trace(JSTracer* trc, JSObject* obj)
{
    if (obj->as<UnboxedPlainObject>().expando_) {
        TraceManuallyBarrieredEdge(trc,
            reinterpret_cast<NativeObject**>(&obj->as<UnboxedPlainObject>().expando_),
            "unboxed_expando");
    }

    const UnboxedLayout& layout = obj->as<UnboxedPlainObject>().layoutDontCheckGeneration();
    const int32_t* list = layout.traceList();
    if (!list)
        return;

    uint8_t* data = obj->as<UnboxedPlainObject>().data();
    while (*list != -1) {
        HeapPtrString* heap = reinterpret_cast<HeapPtrString*>(data + *list);
        TraceEdge(trc, heap, "unboxed_string");
        list++;
    }
    list++;
    while (*list != -1) {
        HeapPtrObject* heap = reinterpret_cast<HeapPtrObject*>(data + *list);
        if (*heap)
            TraceEdge(trc, heap, "unboxed_object");
        list++;
    }

    
    MOZ_ASSERT(*(list + 1) == -1);
}

 UnboxedExpandoObject*
UnboxedPlainObject::ensureExpando(JSContext* cx, Handle<UnboxedPlainObject*> obj)
{
    if (obj->expando_)
        return obj->expando_;

    UnboxedExpandoObject* expando = NewObjectWithGivenProto<UnboxedExpandoObject>(cx, NullPtr());
    if (!expando)
        return nullptr;

    
    
    
    
    if (IsInsideNursery(expando) && !IsInsideNursery(obj))
        cx->runtime()->gc.storeBuffer.putWholeCellFromMainThread(obj);

    obj->expando_ = expando;
    return expando;
}

bool
UnboxedPlainObject::containsUnboxedOrExpandoProperty(ExclusiveContext* cx, jsid id) const
{
    if (layout().lookup(id))
        return true;

    if (maybeExpando() && maybeExpando()->containsShapeOrElement(cx, id))
        return true;

    return false;
}

static bool
PropagatePropertyTypes(JSContext* cx, jsid id, ObjectGroup* oldGroup, ObjectGroup* newGroup)
{
    HeapTypeSet* typeProperty = oldGroup->maybeGetProperty(id);
    TypeSet::TypeList types;
    if (!typeProperty->enumerateTypes(&types)) {
        ReportOutOfMemory(cx);
        return false;
    }
    MOZ_ASSERT(!types.empty());
    for (size_t j = 0; j < types.length(); j++)
        AddTypePropertyId(cx, newGroup, nullptr, id, types[j]);
    return true;
}

 bool
UnboxedLayout::makeNativeGroup(JSContext* cx, ObjectGroup* group)
{
    AutoEnterAnalysis enter(cx);

    UnboxedLayout& layout = group->unboxedLayout();
    Rooted<TaggedProto> proto(cx, group->proto());

    MOZ_ASSERT(!layout.nativeGroup());

    
    
    
    
    
    
    RootedObjectGroup replacementNewGroup(cx);
    if (layout.newScript()) {
        MOZ_ASSERT(!layout.isArray());

        replacementNewGroup = ObjectGroupCompartment::makeGroup(cx, &PlainObject::class_, proto);
        if (!replacementNewGroup)
            return false;

        PlainObject* templateObject = NewObjectWithGroup<PlainObject>(cx, replacementNewGroup,
                                                                      layout.getAllocKind(),
                                                                      TenuredObject);
        if (!templateObject)
            return false;

        for (size_t i = 0; i < layout.properties().length(); i++) {
            const UnboxedLayout::Property& property = layout.properties()[i];
            if (!templateObject->addDataProperty(cx, NameToId(property.name), i, JSPROP_ENUMERATE))
                return false;
            MOZ_ASSERT(templateObject->slotSpan() == i + 1);
            MOZ_ASSERT(!templateObject->inDictionaryMode());
        }

        TypeNewScript* replacementNewScript =
            TypeNewScript::makeNativeVersion(cx, layout.newScript(), templateObject);
        if (!replacementNewScript)
            return false;

        replacementNewGroup->setNewScript(replacementNewScript);
        gc::TraceTypeNewScript(replacementNewGroup);

        group->clearNewScript(cx, replacementNewGroup);
    }

    const Class* clasp = layout.isArray() ? &ArrayObject::class_ : &PlainObject::class_;
    size_t nfixed = layout.isArray() ? 0 : gc::GetGCKindSlots(layout.getAllocKind());

    if (layout.isArray()) {
        
        
        
        if (!NewDenseEmptyArray(cx))
            return false;
    }

    RootedShape shape(cx, EmptyShape::getInitialShape(cx, clasp, proto, nfixed, 0));
    if (!shape)
        return false;

    MOZ_ASSERT_IF(layout.isArray(), !shape->isEmptyShape() && shape->slotSpan() == 0);

    
    for (size_t i = 0; i < layout.properties().length(); i++) {
        const UnboxedLayout::Property& property = layout.properties()[i];

        StackShape unrootedChild(shape->base()->unowned(), NameToId(property.name), i,
                                 JSPROP_ENUMERATE, 0);
        RootedGeneric<StackShape*> child(cx, &unrootedChild);
        shape = cx->compartment()->propertyTree.getChild(cx, shape, *child);
        if (!shape)
            return false;
    }

    ObjectGroup* nativeGroup =
        ObjectGroupCompartment::makeGroup(cx, clasp, proto,
                                          group->flags() & OBJECT_FLAG_DYNAMIC_MASK);
    if (!nativeGroup)
        return false;

    
    if (layout.isArray()) {
        if (!PropagatePropertyTypes(cx, JSID_VOID, group, nativeGroup))
            return false;
    } else {
        for (size_t i = 0; i < layout.properties().length(); i++) {
            const UnboxedLayout::Property& property = layout.properties()[i];
            jsid id = NameToId(property.name);
            if (!PropagatePropertyTypes(cx, id, group, nativeGroup))
                return false;

            HeapTypeSet* nativeProperty = nativeGroup->maybeGetProperty(id);
            if (nativeProperty->canSetDefinite(i))
                nativeProperty->setDefinite(i);
        }
    }

    layout.nativeGroup_ = nativeGroup;
    layout.nativeShape_ = shape;
    layout.replacementNewGroup_ = replacementNewGroup;

    nativeGroup->setOriginalUnboxedGroup(group);

    group->markStateChange(cx);

    return true;
}

 bool
UnboxedPlainObject::convertToNative(JSContext* cx, JSObject* obj)
{
    const UnboxedLayout& layout = obj->as<UnboxedPlainObject>().layout();
    UnboxedExpandoObject* expando = obj->as<UnboxedPlainObject>().maybeExpando();

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

    obj->setGroup(layout.nativeGroup());
    obj->as<PlainObject>().setLastPropertyMakeNative(cx, layout.nativeShape());

    for (size_t i = 0; i < values.length(); i++)
        obj->as<PlainObject>().initSlotUnchecked(i, values[i]);

    if (expando) {
        
        
        
        
        gc::AutoSuppressGC suppress(cx);

        Vector<jsid> ids(cx);
        for (Shape::Range<NoGC> r(expando->lastProperty()); !r.empty(); r.popFront()) {
            if (!ids.append(r.front().propid()))
                return false;
        }
        for (size_t i = 0; i < expando->getDenseInitializedLength(); i++) {
            if (!expando->getDenseElement(i).isMagic(JS_ELEMENTS_HOLE)) {
                if (!ids.append(INT_TO_JSID(i)))
                    return false;
            }
        }
        ::Reverse(ids.begin(), ids.end());

        RootedPlainObject nobj(cx, &obj->as<PlainObject>());
        Rooted<UnboxedExpandoObject*> nexpando(cx, expando);
        RootedId id(cx);
        Rooted<PropertyDescriptor> desc(cx);
        for (size_t i = 0; i < ids.length(); i++) {
            id = ids[i];
            if (!GetOwnPropertyDescriptor(cx, nexpando, id, &desc))
                return false;
            ObjectOpResult result;
            if (!StandardDefineProperty(cx, nobj, id, desc, result))
                return false;
            MOZ_ASSERT(result.ok());
        }
    }

    return true;
}


UnboxedPlainObject*
UnboxedPlainObject::create(ExclusiveContext* cx, HandleObjectGroup group, NewObjectKind newKind)
{
    MOZ_ASSERT(group->clasp() == &class_);
    gc::AllocKind allocKind = group->unboxedLayout().getAllocKind();

    UnboxedPlainObject* res =
        NewObjectWithGroup<UnboxedPlainObject>(cx, group, allocKind, newKind);
    if (!res)
        return nullptr;

    
    res->initExpando();

    
    
    const int32_t* list = res->layout().traceList();
    if (list) {
        uint8_t* data = res->data();
        while (*list != -1) {
            HeapPtrString* heap = reinterpret_cast<HeapPtrString*>(data + *list);
            heap->init(cx->names().empty);
            list++;
        }
        list++;
        while (*list != -1) {
            HeapPtrObject* heap = reinterpret_cast<HeapPtrObject*>(data + *list);
            heap->init(nullptr);
            list++;
        }
        
        MOZ_ASSERT(*(list + 1) == -1);
    }

    return res;
}

 JSObject*
UnboxedPlainObject::createWithProperties(ExclusiveContext* cx, HandleObjectGroup group,
                                         NewObjectKind newKind, IdValuePair* properties)
{
    MOZ_ASSERT(newKind == GenericObject || newKind == TenuredObject);

    UnboxedLayout& layout = group->unboxedLayout();

    if (layout.constructorCode()) {
        MOZ_ASSERT(cx->isJSContext());

        typedef JSObject* (*ConstructorCodeSignature)(IdValuePair*, NewObjectKind);
        ConstructorCodeSignature function =
            reinterpret_cast<ConstructorCodeSignature>(layout.constructorCode()->raw());

        JSObject* obj;
        {
            JS::AutoSuppressGCAnalysis nogc;
            obj = reinterpret_cast<JSObject*>(CALL_GENERATED_2(function, properties, newKind));
        }
        if (obj > reinterpret_cast<JSObject*>(CLEAR_CONSTRUCTOR_CODE_TOKEN))
            return obj;

        if (obj == reinterpret_cast<JSObject*>(CLEAR_CONSTRUCTOR_CODE_TOKEN))
            layout.setConstructorCode(nullptr);
    }

    UnboxedPlainObject* obj = UnboxedPlainObject::create(cx, group, newKind);
    if (!obj)
        return nullptr;

    for (size_t i = 0; i < layout.properties().length(); i++) {
        if (!obj->setValue(cx, layout.properties()[i], properties[i].value))
            return NewPlainObjectWithProperties(cx, properties, layout.properties().length(), newKind);
    }

#ifndef JS_CODEGEN_NONE
    if (cx->isJSContext() &&
        !layout.constructorCode() &&
        cx->asJSContext()->runtime()->jitSupportsFloatingPoint)
    {
        if (!UnboxedLayout::makeConstructorCode(cx->asJSContext(), group))
            return nullptr;
    }
#endif

    return obj;
}

 bool
UnboxedPlainObject::obj_lookupProperty(JSContext* cx, HandleObject obj,
                                       HandleId id, MutableHandleObject objp,
                                       MutableHandleShape propp)
{
    if (obj->as<UnboxedPlainObject>().containsUnboxedOrExpandoProperty(cx, id)) {
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
UnboxedPlainObject::obj_defineProperty(JSContext* cx, HandleObject obj, HandleId id,
                                       Handle<JSPropertyDescriptor> desc,
                                       ObjectOpResult& result)
{
    const UnboxedLayout& layout = obj->as<UnboxedPlainObject>().layout();

    if (const UnboxedLayout::Property* property = layout.lookup(id)) {
        if (!desc.getter() && !desc.setter() && desc.attributes() == JSPROP_ENUMERATE) {
            
            if (obj->as<UnboxedPlainObject>().setValue(cx, *property, desc.value()))
                return true;
        }

        
        
        if (!convertToNative(cx, obj))
            return false;

        return DefineProperty(cx, obj, id, desc, result) &&
               result.checkStrict(cx, obj, id);
    }

    
    Rooted<UnboxedExpandoObject*> expando(cx, ensureExpando(cx, obj.as<UnboxedPlainObject>()));
    if (!expando)
        return false;

    
    AddTypePropertyId(cx, obj, id, desc.value());

    return DefineProperty(cx, expando, id, desc, result);
}

 bool
UnboxedPlainObject::obj_hasProperty(JSContext* cx, HandleObject obj, HandleId id, bool* foundp)
{
    if (obj->as<UnboxedPlainObject>().containsUnboxedOrExpandoProperty(cx, id)) {
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
UnboxedPlainObject::obj_getProperty(JSContext* cx, HandleObject obj, HandleObject receiver,
                                    HandleId id, MutableHandleValue vp)
{
    const UnboxedLayout& layout = obj->as<UnboxedPlainObject>().layout();

    if (const UnboxedLayout::Property* property = layout.lookup(id)) {
        vp.set(obj->as<UnboxedPlainObject>().getValue(*property));
        return true;
    }

    if (UnboxedExpandoObject* expando = obj->as<UnboxedPlainObject>().maybeExpando()) {
        if (expando->containsShapeOrElement(cx, id)) {
            RootedObject nexpando(cx, expando);
            RootedObject nreceiver(cx, (obj == receiver) ? expando : receiver.get());
            return GetProperty(cx, nexpando, nreceiver, id, vp);
        }
    }

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        vp.setUndefined();
        return true;
    }

    return GetProperty(cx, proto, receiver, id, vp);
}

 bool
UnboxedPlainObject::obj_setProperty(JSContext* cx, HandleObject obj, HandleId id, HandleValue v,
                                    HandleValue receiver, ObjectOpResult& result)
{
    const UnboxedLayout& layout = obj->as<UnboxedPlainObject>().layout();

    if (const UnboxedLayout::Property* property = layout.lookup(id)) {
        if (receiver.isObject() && obj == &receiver.toObject()) {
            if (obj->as<UnboxedPlainObject>().setValue(cx, *property, v))
                return result.succeed();

            if (!convertToNative(cx, obj))
                return false;
            return SetProperty(cx, obj, id, v, receiver, result);
        }

        return SetPropertyByDefining(cx, obj, id, v, receiver, false, result);
    }

    if (UnboxedExpandoObject* expando = obj->as<UnboxedPlainObject>().maybeExpando()) {
        if (expando->containsShapeOrElement(cx, id)) {
            
            AddTypePropertyId(cx, obj, id, v);

            RootedObject nexpando(cx, expando);
            RootedValue nreceiver(cx, (receiver.isObject() && obj == &receiver.toObject())
                                      ? ObjectValue(*expando)
                                      : receiver);
            return SetProperty(cx, nexpando, id, v, nreceiver, result);
        }
    }

    return SetPropertyOnProto(cx, obj, id, v, receiver, result);
}

 bool
UnboxedPlainObject::obj_getOwnPropertyDescriptor(JSContext* cx, HandleObject obj, HandleId id,
                                                 MutableHandle<JSPropertyDescriptor> desc)
{
    const UnboxedLayout& layout = obj->as<UnboxedPlainObject>().layout();

    if (const UnboxedLayout::Property* property = layout.lookup(id)) {
        desc.value().set(obj->as<UnboxedPlainObject>().getValue(*property));
        desc.setAttributes(JSPROP_ENUMERATE);
        desc.object().set(obj);
        return true;
    }

    if (UnboxedExpandoObject* expando = obj->as<UnboxedPlainObject>().maybeExpando()) {
        if (expando->containsShapeOrElement(cx, id)) {
            RootedObject nexpando(cx, expando);
            if (!GetOwnPropertyDescriptor(cx, nexpando, id, desc))
                return false;
            if (desc.object() == nexpando)
                desc.object().set(obj);
            return true;
        }
    }

    desc.object().set(nullptr);
    return true;
}

 bool
UnboxedPlainObject::obj_deleteProperty(JSContext* cx, HandleObject obj, HandleId id,
                                       ObjectOpResult& result)
{
    if (!convertToNative(cx, obj))
        return false;
    return DeleteProperty(cx, obj, id, result);
}

 bool
UnboxedPlainObject::obj_watch(JSContext* cx, HandleObject obj, HandleId id, HandleObject callable)
{
    if (!convertToNative(cx, obj))
        return false;
    return WatchProperty(cx, obj, id, callable);
}

 bool
UnboxedPlainObject::obj_enumerate(JSContext* cx, HandleObject obj, AutoIdVector& properties)
{
    UnboxedExpandoObject* expando = obj->as<UnboxedPlainObject>().maybeExpando();

    
    if (expando) {
        for (size_t i = 0; i < expando->getDenseInitializedLength(); i++) {
            if (!expando->getDenseElement(i).isMagic(JS_ELEMENTS_HOLE)) {
                if (!properties.append(INT_TO_JSID(i)))
                    return false;
            }
        }
    }

    const UnboxedLayout::PropertyVector& unboxed = obj->as<UnboxedPlainObject>().layout().properties();
    for (size_t i = 0; i < unboxed.length(); i++) {
        if (!properties.append(NameToId(unboxed[i].name)))
            return false;
    }

    if (expando) {
        Vector<jsid> ids(cx);
        for (Shape::Range<NoGC> r(expando->lastProperty()); !r.empty(); r.popFront()) {
            if (!ids.append(r.front().propid()))
                return false;
        }
        ::Reverse(ids.begin(), ids.end());
        if (!properties.append(ids.begin(), ids.length()))
            return false;
    }

    return true;
}

const Class UnboxedExpandoObject::class_ = {
    "UnboxedExpandoObject",
    JSCLASS_IMPLEMENTS_BARRIERS
};

const Class UnboxedPlainObject::class_ = {
    js_Object_str,
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





 bool
UnboxedArrayObject::convertToNative(JSContext* cx, JSObject* obj)
{
    const UnboxedLayout& layout = obj->as<UnboxedArrayObject>().layout();

    if (!layout.nativeGroup()) {
        if (!UnboxedLayout::makeNativeGroup(cx, obj->group()))
            return false;
    }

    size_t length = obj->as<UnboxedArrayObject>().length();
    size_t initlen = obj->as<UnboxedArrayObject>().initializedLength();

    AutoValueVector values(cx);
    for (size_t i = 0; i < initlen; i++) {
        if (!values.append(obj->as<UnboxedArrayObject>().getElement(i)))
            return false;
    }

    obj->setGroup(layout.nativeGroup());

    ArrayObject* aobj = &obj->as<ArrayObject>();
    aobj->setLastPropertyMakeNative(cx, layout.nativeShape());

    
    
    if (!aobj->ensureElements(cx, Max<size_t>(initlen, 1)))
        return false;

    MOZ_ASSERT(!aobj->getDenseInitializedLength());
    aobj->setDenseInitializedLength(initlen);
    aobj->initDenseElements(0, values.begin(), initlen);
    aobj->setLengthInt32(length);

    return true;
}

 UnboxedArrayObject*
UnboxedArrayObject::create(ExclusiveContext* cx, HandleObjectGroup group, uint32_t length,
                           NewObjectKind newKind)
{
    MOZ_ASSERT(length <= MaximumCapacity);

    MOZ_ASSERT(group->clasp() == &class_);
    uint32_t elementSize = UnboxedTypeSize(group->unboxedLayout().elementType());
    uint32_t nbytes = offsetOfInlineElements() + elementSize * length;

    UnboxedArrayObject* res;
    if (nbytes <= JSObject::MAX_BYTE_SIZE) {
        gc::AllocKind allocKind = gc::GetGCObjectKindForBytes(nbytes);

        res = NewObjectWithGroup<UnboxedArrayObject>(cx, group, allocKind, newKind);
        if (!res)
            return nullptr;
        res->setInlineElements();

        size_t capacity = (GetGCKindBytes(allocKind) - offsetOfInlineElements()) / elementSize;
        res->setCapacityIndex(exactCapacityIndex(capacity));
    } else {
        UniquePtr<uint8_t[], JS::FreePolicy> elements(
            cx->zone()->pod_malloc<uint8_t>(length * elementSize));
        if (!elements)
            return nullptr;

        res = NewObjectWithGroup<UnboxedArrayObject>(cx, group, gc::AllocKind::OBJECT0, newKind);
        if (!res)
            return nullptr;

        res->elements_ = elements.release();
        res->setCapacityIndex(CapacityMatchesLengthIndex);
    }

    res->setLength(length);
    res->setInitializedLength(0);
    return res;
}

bool
UnboxedArrayObject::setElement(ExclusiveContext* cx, size_t index, const Value& v)
{
    MOZ_ASSERT(index < initializedLength());
    uint8_t* p = elements() + index * elementSize();
    return SetUnboxedValue(cx, this, JSID_VOID, p, elementType(), v);
}

bool
UnboxedArrayObject::initElement(ExclusiveContext* cx, size_t index, const Value& v)
{
    MOZ_ASSERT(index < initializedLength());
    uint8_t* p = elements() + index * elementSize();
    if (UnboxedTypeNeedsPreBarrier(elementType()))
        *reinterpret_cast<void**>(p) = nullptr;
    return SetUnboxedValue(cx, this, JSID_VOID, p, elementType(), v);
}

Value
UnboxedArrayObject::getElement(size_t index)
{
    MOZ_ASSERT(index < initializedLength());
    uint8_t* p = elements() + index * elementSize();
    return GetUnboxedValue(p, elementType());
}

 void
UnboxedArrayObject::trace(JSTracer* trc, JSObject* obj)
{
    JSValueType type = obj->as<UnboxedArrayObject>().elementType();
    if (!UnboxedTypeNeedsPreBarrier(type))
        return;

    MOZ_ASSERT(obj->as<UnboxedArrayObject>().elementSize() == sizeof(uintptr_t));
    size_t initlen = obj->as<UnboxedArrayObject>().initializedLength();
    void** elements = reinterpret_cast<void**>(obj->as<UnboxedArrayObject>().elements());

    switch (type) {
      case JSVAL_TYPE_OBJECT:
        for (size_t i = 0; i < initlen; i++) {
            HeapPtrObject* heap = reinterpret_cast<HeapPtrObject*>(elements + i);
            if (*heap)
                TraceEdge(trc, heap, "unboxed_object");
        }
        break;

      case JSVAL_TYPE_STRING:
        for (size_t i = 0; i < initlen; i++) {
            HeapPtrString* heap = reinterpret_cast<HeapPtrString*>(elements + i);
            TraceEdge(trc, heap, "unboxed_string");
        }
        break;

      default:
        MOZ_CRASH();
    }
}

 void
UnboxedArrayObject::objectMoved(JSObject* obj, const JSObject* old)
{
    UnboxedArrayObject& dst = obj->as<UnboxedArrayObject>();
    const UnboxedArrayObject& src = old->as<UnboxedArrayObject>();

    
    if (src.hasInlineElements())
        dst.setInlineElements();
}

 void
UnboxedArrayObject::finalize(FreeOp* fop, JSObject* obj)
{
    if (!obj->as<UnboxedArrayObject>().hasInlineElements())
        js_free(obj->as<UnboxedArrayObject>().elements());
}





 const uint32_t
UnboxedArrayObject::CapacityArray[] = {
    UINT32_MAX, 
    0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 13, 16, 17, 18, 20, 24, 26, 32, 34, 36, 48, 52, 64, 68,
    128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288,
    1048576, 2097152, 3145728, 4194304, 5242880, 6291456, 7340032, 8388608, 9437184, 11534336,
    13631488, 15728640, 17825792, 20971520, 24117248, 27262976, 31457280, 35651584, 40894464,
    46137344, 52428800, 59768832, MaximumCapacity
};

static const uint32_t
Pow2CapacityIndexes[] = {
    1,  
    2,  
    4,  
    8,  
    13, 
    19, 
    24, 
    26, 
    27, 
    28, 
    29, 
    30, 
    31, 
    32, 
    33, 
    34, 
    35, 
    36, 
    37, 
    38, 
    39  
};

static const uint32_t MebiCapacityIndex = 39;

 uint32_t
UnboxedArrayObject::chooseCapacityIndex(uint32_t capacity, uint32_t length)
{
    
    
    

    
    MOZ_ASSERT(mozilla::ArrayLength(CapacityArray) - 1 <= CapacityMask >> CapacityShift);

    
    MOZ_ASSERT(capacity <= MaximumCapacity);

    static const uint32_t Mebi = 1024 * 1024;

    if (capacity <= Mebi) {
        capacity = mozilla::RoundUpPow2(capacity);

        
        
        if (length >= capacity && capacity > (length / 3) * 2)
            return CapacityMatchesLengthIndex;

        if (capacity < MinimumDynamicCapacity)
            capacity = MinimumDynamicCapacity;

        uint32_t bit = mozilla::FloorLog2Size(capacity);
        MOZ_ASSERT(capacity == uint32_t(1 << bit));
        MOZ_ASSERT(bit <= 20);
        MOZ_ASSERT(mozilla::ArrayLength(Pow2CapacityIndexes) == 21);

        uint32_t index = Pow2CapacityIndexes[bit];
        MOZ_ASSERT(CapacityArray[index] == capacity);

        return index;
    }

    MOZ_ASSERT(CapacityArray[MebiCapacityIndex] == Mebi);

    for (uint32_t i = MebiCapacityIndex + 1;; i++) {
        if (CapacityArray[i] >= capacity)
            return i;
    }

    MOZ_CRASH("Invalid capacity");
}

 uint32_t
UnboxedArrayObject::exactCapacityIndex(uint32_t capacity)
{
    for (size_t i = CapacityMatchesLengthIndex + 1; i < ArrayLength(CapacityArray); i++) {
        if (CapacityArray[i] == capacity)
            return i;
    }
    MOZ_CRASH();
}

bool
UnboxedArrayObject::growElements(ExclusiveContext* cx, size_t cap)
{
    
    
    MOZ_ASSERT(cap <= MaximumCapacity);

    uint32_t oldCapacity = capacity();
    uint32_t newCapacityIndex = chooseCapacityIndex(cap, length());
    uint32_t newCapacity = computeCapacity(newCapacityIndex, length());

    MOZ_ASSERT(oldCapacity < cap);
    MOZ_ASSERT(cap <= newCapacity);

    
    JS_STATIC_ASSERT(MaximumCapacity < UINT32_MAX / sizeof(double));

    uint8_t* newElements;
    if (hasInlineElements()) {
        newElements = cx->zone()->pod_malloc<uint8_t>(newCapacity * elementSize());
        if (!newElements)
            return false;
        js_memcpy(newElements, elements(), initializedLength() * elementSize());
    } else {
        newElements = cx->zone()->pod_realloc<uint8_t>(elements(), oldCapacity * elementSize(),
                                                       newCapacity * elementSize());
        if (!newElements)
            return false;
    }

    elements_ = newElements;
    setCapacityIndex(newCapacityIndex);

    return true;
}

void
UnboxedArrayObject::shrinkElements(ExclusiveContext* cx, size_t cap)
{
    if (hasInlineElements())
        return;

    uint32_t oldCapacity = capacity();
    uint32_t newCapacityIndex = chooseCapacityIndex(cap, 0);
    uint32_t newCapacity = computeCapacity(newCapacityIndex, 0);

    MOZ_ASSERT(cap < oldCapacity);
    MOZ_ASSERT(cap <= newCapacity);

    if (newCapacity >= oldCapacity)
        return;

    uint8_t* newElements =
        cx->zone()->pod_realloc<uint8_t>(elements(), oldCapacity * elementSize(),
                                         newCapacity * elementSize());
    if (!newElements)
        return;

    elements_ = newElements;
    setCapacityIndex(newCapacityIndex);
}

bool
UnboxedArrayObject::containsProperty(JSContext* cx, jsid id)
{
    if (JSID_IS_INT(id) && uint32_t(JSID_TO_INT(id)) < initializedLength())
        return true;
    if (JSID_IS_ATOM(id) && JSID_TO_ATOM(id) == cx->names().length)
        return true;
    return false;
}

 bool
UnboxedArrayObject::obj_lookupProperty(JSContext* cx, HandleObject obj,
                                       HandleId id, MutableHandleObject objp,
                                       MutableHandleShape propp)
{
    if (obj->as<UnboxedArrayObject>().containsProperty(cx, id)) {
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
UnboxedArrayObject::obj_defineProperty(JSContext* cx, HandleObject obj, HandleId id,
                                       Handle<JSPropertyDescriptor> desc,
                                       ObjectOpResult& result)
{
    if (JSID_IS_INT(id) && !desc.getter() && !desc.setter() && desc.attributes() == JSPROP_ENUMERATE) {
        UnboxedArrayObject* nobj = &obj->as<UnboxedArrayObject>();

        uint32_t index = JSID_TO_INT(id);
        if (index < nobj->initializedLength()) {
            if (nobj->setElement(cx, index, desc.value()))
                return result.succeed();
        } else if (index == nobj->initializedLength() && index < MaximumCapacity) {
            if (nobj->initializedLength() == nobj->capacity()) {
                if (!nobj->growElements(cx, index + 1))
                    return false;
            }
            nobj->setInitializedLength(index + 1);
            if (nobj->initElement(cx, index, desc.value())) {
                if (nobj->length() <= index)
                    nobj->setLength(index + 1);
                return result.succeed();
            }
            nobj->setInitializedLength(index);
        }
    }

    if (!convertToNative(cx, obj))
        return false;

    return DefineProperty(cx, obj, id, desc, result);
}

 bool
UnboxedArrayObject::obj_hasProperty(JSContext* cx, HandleObject obj, HandleId id, bool* foundp)
{
    if (obj->as<UnboxedArrayObject>().containsProperty(cx, id)) {
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
UnboxedArrayObject::obj_getProperty(JSContext* cx, HandleObject obj, HandleObject receiver,
                                    HandleId id, MutableHandleValue vp)
{
    if (obj->as<UnboxedArrayObject>().containsProperty(cx, id)) {
        if (JSID_IS_INT(id))
            vp.set(obj->as<UnboxedArrayObject>().getElement(JSID_TO_INT(id)));
        else
            vp.set(Int32Value(obj->as<UnboxedArrayObject>().length()));
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
UnboxedArrayObject::obj_setProperty(JSContext* cx, HandleObject obj, HandleId id, HandleValue v,
                                    HandleValue receiver, ObjectOpResult& result)
{
    if (obj->as<UnboxedArrayObject>().containsProperty(cx, id)) {
        if (receiver.isObject() && obj == &receiver.toObject()) {
            if (JSID_IS_INT(id)) {
                if (obj->as<UnboxedArrayObject>().setElement(cx, JSID_TO_INT(id), v))
                    return result.succeed();
            } else {
                uint32_t len;
                if (!CanonicalizeArrayLengthValue(cx, v, &len))
                    return false;
                if (len <= INT32_MAX) {
                    UnboxedArrayObject* nobj = &obj->as<UnboxedArrayObject>();
                    nobj->setLength(len);
                    if (len < nobj->initializedLength()) {
                        nobj->setInitializedLength(len);
                        nobj->shrinkElements(cx, len);
                    }
                }
            }

            if (!convertToNative(cx, obj))
                return false;
            return SetProperty(cx, obj, id, v, receiver, result);
        }

        return SetPropertyByDefining(cx, obj, id, v, receiver, false, result);
    }

    return SetPropertyOnProto(cx, obj, id, v, receiver, result);
}

 bool
UnboxedArrayObject::obj_getOwnPropertyDescriptor(JSContext* cx, HandleObject obj, HandleId id,
                                                 MutableHandle<JSPropertyDescriptor> desc)
{
    if (obj->as<UnboxedArrayObject>().containsProperty(cx, id)) {
        if (JSID_IS_INT(id)) {
            desc.value().set(obj->as<UnboxedArrayObject>().getElement(JSID_TO_INT(id)));
            desc.setAttributes(JSPROP_ENUMERATE);
        } else {
            desc.value().set(Int32Value(obj->as<UnboxedArrayObject>().length()));
            desc.setAttributes(JSPROP_PERMANENT);
        }
        desc.object().set(obj);
        return true;
    }

    desc.object().set(nullptr);
    return true;
}

 bool
UnboxedArrayObject::obj_deleteProperty(JSContext* cx, HandleObject obj, HandleId id,
                                       ObjectOpResult& result)
{
    if (!convertToNative(cx, obj))
        return false;
    return DeleteProperty(cx, obj, id, result);
}

 bool
UnboxedArrayObject::obj_watch(JSContext* cx, HandleObject obj, HandleId id, HandleObject callable)
{
    if (!convertToNative(cx, obj))
        return false;
    return WatchProperty(cx, obj, id, callable);
}

 bool
UnboxedArrayObject::obj_enumerate(JSContext* cx, HandleObject obj, AutoIdVector& properties)
{
    for (size_t i = 0; i < obj->as<UnboxedArrayObject>().initializedLength(); i++) {
        if (!properties.append(INT_TO_JSID(i)))
            return false;
    }
    return properties.append(NameToId(cx->names().length));
}

const Class UnboxedArrayObject::class_ = {
    "Array",
    Class::NON_NATIVE |
    JSCLASS_IMPLEMENTS_BARRIERS |
    0 ,
    nullptr,        
    nullptr,        
    nullptr,        
    nullptr,        
    nullptr,        
    nullptr,        
    nullptr,        
    nullptr,        
    UnboxedArrayObject::finalize,
    nullptr,        
    nullptr,        
    nullptr,        
    UnboxedArrayObject::trace,
    JS_NULL_CLASS_SPEC,
    {
        nullptr,    
        nullptr,    
        false,      
        nullptr,    
        UnboxedArrayObject::objectMoved
    },
    {
        UnboxedArrayObject::obj_lookupProperty,
        UnboxedArrayObject::obj_defineProperty,
        UnboxedArrayObject::obj_hasProperty,
        UnboxedArrayObject::obj_getProperty,
        UnboxedArrayObject::obj_setProperty,
        UnboxedArrayObject::obj_getOwnPropertyDescriptor,
        UnboxedArrayObject::obj_deleteProperty,
        UnboxedArrayObject::obj_watch,
        nullptr,   
        nullptr,   
        UnboxedArrayObject::obj_enumerate,
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
CombineUnboxedTypes(const Value& value, JSValueType* existing)
{
    JSValueType type = value.isDouble() ? JSVAL_TYPE_DOUBLE : value.extractNonDoubleType();

    if (*existing == JSVAL_TYPE_MAGIC || *existing == type || UnboxedTypeIncludes(type, *existing)) {
        *existing = type;
        return true;
    }
    if (UnboxedTypeIncludes(*existing, type))
        return true;
    return false;
}



static bool
PropertiesAreSuperset(const UnboxedLayout::PropertyVector& properties, UnboxedLayout* layout)
{
    for (size_t i = 0; i < layout->properties().length(); i++) {
        const UnboxedLayout::Property& layoutProperty = layout->properties()[i];
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

static bool
CombinePlainObjectProperties(PlainObject* obj, Shape* templateShape,
                             UnboxedLayout::PropertyVector& properties)
{
    
    
    
    
    
    
    MOZ_ASSERT(gc::GetGCKindSlots(obj->asTenured().getAllocKind()) >=
               Min(NativeObject::MAX_FIXED_SLOTS, templateShape->slotSpan()));

    if (obj->lastProperty() != templateShape || obj->hasDynamicElements()) {
        
        
        return false;
    }

    for (size_t i = 0; i < templateShape->slotSpan(); i++) {
        Value val = obj->getSlot(i);

        JSValueType& existing = properties[i].type;
        if (!CombineUnboxedTypes(val, &existing))
            return false;
    }

    return true;
}

static bool
CombineArrayObjectElements(ExclusiveContext* cx, ArrayObject* obj, JSValueType* elementType)
{
    if (obj->inDictionaryMode() ||
        obj->lastProperty()->propid() != AtomToId(cx->names().length) ||
        !obj->lastProperty()->previous()->isEmptyShape() ||
        !obj->getDenseInitializedLength())
    {
        
        
        return false;
    }

    for (size_t i = 0; i < obj->getDenseInitializedLength(); i++) {
        Value val = obj->getDenseElement(i);

        
        if (val.isMagic(JS_ELEMENTS_HOLE))
            return false;

        if (!CombineUnboxedTypes(val, elementType))
            return false;
    }

    return true;
}

static size_t
ComputePlainObjectLayout(ExclusiveContext* cx, Shape* templateShape,
                         UnboxedLayout::PropertyVector& properties)
{
    
    for (Shape::Range<NoGC> r(templateShape); !r.empty(); r.popFront()) {
        size_t slot = r.front().slot();
        MOZ_ASSERT(!properties[slot].name);
        properties[slot].name = JSID_TO_ATOM(r.front().propid())->asPropertyName();
    }

    
    uint32_t offset = 0;

    
    
    
    
    
    UnboxedLayout* bestExisting = nullptr;
    for (UnboxedLayout* existing = cx->compartment()->unboxedLayouts.getFirst();
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
            const UnboxedLayout::Property& existingProperty = bestExisting->properties()[i];
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

    
    return offset;
}

static bool
SetLayoutTraceList(ExclusiveContext* cx, UnboxedLayout* layout)
{
    
    Vector<int32_t, 8, SystemAllocPolicy> objectOffsets, stringOffsets;
    for (size_t i = 0; i < layout->properties().length(); i++) {
        const UnboxedLayout::Property& property = layout->properties()[i];
        MOZ_ASSERT(property.offset != UINT32_MAX);
        if (property.type == JSVAL_TYPE_OBJECT) {
            if (!objectOffsets.append(property.offset))
                return false;
        } else if (property.type == JSVAL_TYPE_STRING) {
            if (!stringOffsets.append(property.offset))
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
        int32_t* traceList = cx->zone()->pod_malloc<int32_t>(entries.length());
        if (!traceList)
            return false;
        PodCopy(traceList, entries.begin(), entries.length());
        layout->setTraceList(traceList);
    }

    return true;
}

static inline Value
NextValue(const AutoValueVector& values, size_t* valueCursor)
{
    return values[(*valueCursor)++];
}

static bool
GetValuesFromPreliminaryArrayObject(ArrayObject* obj, AutoValueVector& values)
{
    if (!values.append(Int32Value(obj->length())))
        return false;
    if (!values.append(Int32Value(obj->getDenseInitializedLength())))
        return false;
    for (size_t i = 0; i < obj->getDenseInitializedLength(); i++) {
        if (!values.append(obj->getDenseElement(i)))
            return false;
    }
    return true;
}

void
UnboxedArrayObject::fillAfterConvert(ExclusiveContext* cx,
                                     const AutoValueVector& values, size_t* valueCursor)
{
    MOZ_ASSERT(CapacityArray[1] == 0);
    setCapacityIndex(1);
    setInitializedLength(0);
    setInlineElements();

    setLength(NextValue(values, valueCursor).toInt32());

    int32_t initlen = NextValue(values, valueCursor).toInt32();

    if (!growElements(cx, initlen))
        CrashAtUnhandlableOOM("UnboxedArrayObject::fillAfterConvert");
    setInitializedLength(initlen);

    for (size_t i = 0; i < size_t(initlen); i++)
        JS_ALWAYS_TRUE(initElement(cx, i, NextValue(values, valueCursor)));
}

static bool
GetValuesFromPreliminaryPlainObject(PlainObject* obj, AutoValueVector& values)
{
    for (size_t i = 0; i < obj->slotSpan(); i++) {
        if (!values.append(obj->getSlot(i)))
            return false;
    }
    return true;
}

void
UnboxedPlainObject::fillAfterConvert(ExclusiveContext* cx,
                                     const AutoValueVector& values, size_t* valueCursor)
{
    initExpando();
    memset(data(), 0, layout().size());
    for (size_t i = 0; i < layout().properties().length(); i++)
        JS_ALWAYS_TRUE(setValue(cx, layout().properties()[i], NextValue(values, valueCursor)));
}

bool
js::TryConvertToUnboxedLayout(ExclusiveContext* cx, Shape* templateShape,
                              ObjectGroup* group, PreliminaryObjectArray* objects)
{
    
    
#ifdef NIGHTLY_BUILD
    if (templateShape) {
        if (!getenv("JS_OPTION_USE_UNBOXED_OBJECTS")) {
            if (!group->runtimeFromAnyThread()->options().unboxedObjects())
                return true;
        }
    } else {
        if (!group->runtimeFromAnyThread()->options().unboxedArrays())
            return true;
    }
#else
    return true;
#endif

    MOZ_ASSERT_IF(templateShape, !templateShape->getObjectFlags());

    bool isArray = !templateShape;

    const JS::RuntimeOptions& options = group->runtimeFromAnyThread()->options();
    if (isArray ? !options.unboxedArrays() : !options.unboxedObjects())
        return true;

    if (group->runtimeFromAnyThread()->isSelfHostingGlobal(cx->global()))
        return true;

    if (!isArray && templateShape->slotSpan() == 0)
        return true;

    UnboxedLayout::PropertyVector properties;
    if (!isArray) {
        if (!properties.appendN(UnboxedLayout::Property(), templateShape->slotSpan()))
            return false;
    }
    JSValueType elementType = JSVAL_TYPE_MAGIC;

    size_t objectCount = 0;
    for (size_t i = 0; i < PreliminaryObjectArray::COUNT; i++) {
        JSObject* obj = objects->get(i);
        if (!obj)
            continue;

        objectCount++;

        if (isArray) {
            if (!CombineArrayObjectElements(cx, &obj->as<ArrayObject>(), &elementType))
                return true;
        } else {
            if (!CombinePlainObjectProperties(&obj->as<PlainObject>(), templateShape, properties))
                return true;
        }
    }

    size_t layoutSize = 0;
    if (isArray) {
        
        
        if (UnboxedTypeSize(elementType) == 0)
            return true;
    } else {
        if (objectCount <= 1) {
            
            
            
            
            return true;
        }

        for (size_t i = 0; i < templateShape->slotSpan(); i++) {
            
            
            
            if (UnboxedTypeSize(properties[i].type) == 0)
                return true;
        }

        layoutSize = ComputePlainObjectLayout(cx, templateShape, properties);

        
        if (sizeof(JSObject) + layoutSize > JSObject::MAX_BYTE_SIZE)
            return true;
    }

    UniquePtr<UnboxedLayout, JS::DeletePolicy<UnboxedLayout> > layout;
    layout.reset(group->zone()->new_<UnboxedLayout>());
    if (!layout)
        return false;

    if (isArray) {
        layout->initArray(elementType);
    } else {
        if (!layout->initProperties(properties, layoutSize))
            return false;

        
        cx->compartment()->unboxedLayouts.insertFront(layout.get());

        if (!SetLayoutTraceList(cx, layout.get()))
            return false;
    }

    
    
    
    

    
    const Class* clasp = isArray ? &UnboxedArrayObject::class_ : &UnboxedPlainObject::class_;
    Shape* newShape = EmptyShape::getInitialShape(cx, clasp, group->proto(), 0);
    if (!newShape) {
        cx->recoverFromOutOfMemory();
        return false;
    }

    
    
    AutoValueVector values(cx);
    for (size_t i = 0; i < PreliminaryObjectArray::COUNT; i++) {
        JSObject* obj = objects->get(i);
        if (!obj)
            continue;

        if (isArray) {
            if (!GetValuesFromPreliminaryArrayObject(&obj->as<ArrayObject>(), values))
                return false;
        } else {
            if (!GetValuesFromPreliminaryPlainObject(&obj->as<PlainObject>(), values))
                return false;
        }
    }

    if (TypeNewScript* newScript = group->newScript())
        layout->setNewScript(newScript);

    for (size_t i = 0; i < PreliminaryObjectArray::COUNT; i++) {
        if (JSObject* obj = objects->get(i))
            obj->as<NativeObject>().setLastPropertyMakeNonNative(newShape);
    }

    group->setClasp(clasp);
    group->setUnboxedLayout(layout.get());

    size_t valueCursor = 0;
    for (size_t i = 0; i < PreliminaryObjectArray::COUNT; i++) {
        JSObject* obj = objects->get(i);
        if (!obj)
            continue;

        if (isArray)
            obj->as<UnboxedArrayObject>().fillAfterConvert(cx, values, &valueCursor);
        else
            obj->as<UnboxedPlainObject>().fillAfterConvert(cx, values, &valueCursor);
    }

    MOZ_ASSERT(valueCursor == values.length());
    layout.release();
    return true;
}
