





#include "builtin/TypeRepresentation.h"

#include "mozilla/HashFunctions.h"

#include "jscntxt.h"
#include "jsnum.h"
#include "jsutil.h"

#include "builtin/TypedObject.h"
#include "js/HashTable.h"
#include "vm/Runtime.h"
#include "vm/StringBuffer.h"

#include "jsgcinlines.h"
#include "jsobjinlines.h"

using namespace js;
using namespace mozilla;




const Class TypeRepresentation::class_ = {
    "TypeRepresentation",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_RESERVED_SLOTS(JS_TYPEREPR_SLOTS),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    obj_finalize,
    nullptr,        
    nullptr,        
    nullptr,        
    obj_trace,
};




bool
TypeRepresentationHasher::match(TypeRepresentation *key1,
                                TypeRepresentation *key2)
{
    if (key1->kind() != key2->kind())
        return false;

    switch (key1->kind()) {
      case TypeDescr::Scalar:
        return matchScalars(key1->asScalar(), key2->asScalar());

      case TypeDescr::Reference:
        return matchReferences(key1->asReference(), key2->asReference());

      case TypeDescr::X4:
        return matchX4s(key1->asX4(), key2->asX4());

      case TypeDescr::Struct:
        return matchStructs(key1->asStruct(), key2->asStruct());

      case TypeDescr::SizedArray:
        return matchSizedArrays(key1->asSizedArray(),
                                key2->asSizedArray());

      case TypeDescr::UnsizedArray:
        return matchUnsizedArrays(key1->asUnsizedArray(),
                                  key2->asUnsizedArray());
    }

    MOZ_ASSUME_UNREACHABLE("Invalid kind");
}

bool
TypeRepresentationHasher::matchScalars(ScalarTypeRepresentation *key1,
                                       ScalarTypeRepresentation *key2)
{
    return key1->type() == key2->type();
}

bool
TypeRepresentationHasher::matchReferences(ReferenceTypeRepresentation *key1,
                                          ReferenceTypeRepresentation *key2)
{
    return key1->type() == key2->type();
}

bool
TypeRepresentationHasher::matchX4s(X4TypeRepresentation *key1,
                                   X4TypeRepresentation *key2)
{
    return key1->type() == key2->type();
}

bool
TypeRepresentationHasher::matchStructs(StructTypeRepresentation *key1,
                                       StructTypeRepresentation *key2)
{
    if (key1->fieldCount() != key2->fieldCount())
        return false;

    for (size_t i = 0; i < key1->fieldCount(); i++) {
        if (key1->field(i).propertyName != key2->field(i).propertyName)
            return false;

        if (key1->field(i).typeRepr != key2->field(i).typeRepr)
            return false;
    }

    return true;
}

bool
TypeRepresentationHasher::matchSizedArrays(SizedArrayTypeRepresentation *key1,
                                           SizedArrayTypeRepresentation *key2)
{
    
    return key1->element() == key2->element() &&
           key1->length() == key2->length();
}

bool
TypeRepresentationHasher::matchUnsizedArrays(UnsizedArrayTypeRepresentation *key1,
                                             UnsizedArrayTypeRepresentation *key2)
{
    
    return key1->element() == key2->element();
}

HashNumber
TypeRepresentationHasher::hash(TypeRepresentation *key) {
    switch (key->kind()) {
      case TypeDescr::Scalar:
        return hashScalar(key->asScalar());

      case TypeDescr::Reference:
        return hashReference(key->asReference());

      case TypeDescr::X4:
        return hashX4(key->asX4());

      case TypeDescr::Struct:
        return hashStruct(key->asStruct());

      case TypeDescr::UnsizedArray:
        return hashUnsizedArray(key->asUnsizedArray());

      case TypeDescr::SizedArray:
        return hashSizedArray(key->asSizedArray());
    }

    MOZ_ASSUME_UNREACHABLE("Invalid kind");
}

HashNumber
TypeRepresentationHasher::hashScalar(ScalarTypeRepresentation *key)
{
    return HashGeneric(key->kind(), key->type());
}

HashNumber
TypeRepresentationHasher::hashReference(ReferenceTypeRepresentation *key)
{
    return HashGeneric(key->kind(), key->type());
}

HashNumber
TypeRepresentationHasher::hashX4(X4TypeRepresentation *key)
{
    return HashGeneric(key->kind(), key->type());
}

HashNumber
TypeRepresentationHasher::hashStruct(StructTypeRepresentation *key)
{
    HashNumber hash = HashGeneric(key->kind());
    for (HashNumber i = 0; i < key->fieldCount(); i++) {
        hash = AddToHash(hash, key->field(i).propertyName.get());
        hash = AddToHash(hash, key->field(i).typeRepr);
    }
    return hash;
}

HashNumber
TypeRepresentationHasher::hashSizedArray(SizedArrayTypeRepresentation *key)
{
    return HashGeneric(key->kind(), key->element(), key->length());
}

HashNumber
TypeRepresentationHasher::hashUnsizedArray(UnsizedArrayTypeRepresentation *key)
{
    return HashGeneric(key->kind(), key->element());
}




TypeRepresentation::TypeRepresentation(TypeDescr::Kind kind, bool opaque)
  : kind_(kind),
    opaque_(opaque)
{}

SizedTypeRepresentation::SizedTypeRepresentation(SizedTypeDescr::Kind kind,
                                                 bool opaque,
                                                 size_t size,
                                                 size_t align)
  : TypeRepresentation(kind, opaque),
    size_(size),
    alignment_(align)
{}

ScalarTypeRepresentation::ScalarTypeRepresentation(ScalarTypeDescr::Type type)
  : SizedTypeRepresentation(TypeDescr::Scalar,
                            false,
                            ScalarTypeDescr::size(type),
                            ScalarTypeDescr::alignment(type)),
    type_(type)
{
}

static size_t X4Sizes[] = {
#define X4_SIZE(_kind, _type, _name)                        \
    sizeof(_type) * 4,
    JS_FOR_EACH_X4_TYPE_REPR(X4_SIZE) 0
#undef X4_SIZE
};

X4TypeRepresentation::X4TypeRepresentation(X4TypeDescr::Type type)
  : SizedTypeRepresentation(X4TypeDescr::X4, false, X4Sizes[type], X4Sizes[type]),
    type_(type)
{
}

ReferenceTypeRepresentation::ReferenceTypeRepresentation(ReferenceTypeDescr::Type type)
  : SizedTypeRepresentation(TypeDescr::Reference, true, 0, 1),
    type_(type)
{
    switch (type) {
      case ReferenceTypeDescr::TYPE_ANY:
        size_ = sizeof(js::HeapValue);
        alignment_ = MOZ_ALIGNOF(js::HeapValue);
        break;

      case ReferenceTypeDescr::TYPE_OBJECT:
      case ReferenceTypeDescr::TYPE_STRING:
        size_ = sizeof(js::HeapPtrObject);
        alignment_ = MOZ_ALIGNOF(js::HeapPtrObject);
        break;
    }
}

SizedArrayTypeRepresentation::SizedArrayTypeRepresentation(SizedTypeRepresentation *element,
                                                           size_t length)
  : SizedTypeRepresentation(TypeDescr::SizedArray, element->opaque(),
                            element->size() * length, element->alignment()),
    element_(element),
    length_(length)
{
}

UnsizedArrayTypeRepresentation::UnsizedArrayTypeRepresentation(SizedTypeRepresentation *element)
  : TypeRepresentation(TypeDescr::UnsizedArray, element->opaque()),
    element_(element)
{
}

static inline size_t alignTo(size_t address, size_t align) {
    JS_ASSERT(IsPowerOfTwo(align));
    return (address + align - 1) & -align;
}

StructField::StructField(size_t index,
                         PropertyName *propertyName,
                         SizedTypeRepresentation *typeRepr,
                         size_t offset)
  : index(index),
    propertyName(propertyName),
    typeRepr(typeRepr),
    offset(offset)
{}

StructTypeRepresentation::StructTypeRepresentation()
  : SizedTypeRepresentation(TypeDescr::Struct, false, 0, 1),
    fieldCount_(0) 
{
    
}

bool
StructTypeRepresentation::init(JSContext *cx,
                               AutoPropertyNameVector &names,
                               AutoObjectVector &typeReprOwners)
{
    JS_ASSERT(names.length() == typeReprOwners.length());
    fieldCount_ = names.length();

    
    
    
    
    uint32_t totalSize = 0;

    
    alignment_ = 1;
    opaque_ = false;

    for (size_t i = 0; i < names.length(); i++) {
        SizedTypeRepresentation *fieldTypeRepr =
            fromOwnerObject(*typeReprOwners[i])->asSized();

        if (fieldTypeRepr->opaque())
            opaque_ = true;

        uint32_t alignedSize = alignTo(totalSize, fieldTypeRepr->alignment());
        if (alignedSize < totalSize) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                 JSMSG_TYPEDOBJECT_TOO_BIG);
            return false;
        }

        new(fields() + i) StructField(i, names[i],
                                      fieldTypeRepr, alignedSize);
        alignment_ = js::Max(alignment_, fieldTypeRepr->alignment());

        uint32_t incrementedSize = alignedSize + fieldTypeRepr->size();
        if (incrementedSize < alignedSize) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                 JSMSG_TYPEDOBJECT_TOO_BIG);
            return false;
        }

        totalSize = incrementedSize;
    }

    uint32_t alignedSize = alignTo(totalSize, alignment_);
    if (alignedSize < totalSize) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_TYPEDOBJECT_TOO_BIG);
        return false;
    }

    size_ = alignedSize;
    return true;
}




JSObject *
TypeRepresentation::addToTableOrFree(JSContext *cx,
                                     TypeRepresentationHash::AddPtr &p)
{
    JS_ASSERT(!ownerObject_);
    Rooted<GlobalObject*> global(cx, cx->global());
    JSCompartment *comp = cx->compartment();

    
    if (!comp->typeReprs.relookupOrAdd(p, this, this)) {
        js_ReportOutOfMemory(cx);
        js_free(this); 
        return nullptr;
    }

    RootedObject objectProto(cx, global->getOrCreateObjectPrototype(cx));
    if (!objectProto)
        return nullptr;

    
    
    
    
    RootedObject ownerObject(cx);
    ownerObject = NewObjectWithGivenProto(cx, &class_, objectProto,
                                          cx->global(), TenuredObject);
    if (!ownerObject) {
        comp->typeReprs.remove(this);
        js_free(this);
        return nullptr;
    }
    ownerObject->setPrivate(this);
    ownerObject->initReservedSlot(JS_TYPEREPR_SLOT_KIND, Int32Value(kind()));
    ownerObject_.init(ownerObject);
    return &*ownerObject;
}

namespace js {
class TypeRepresentationHelper {
  public:
    template<typename D, typename T>
    static JSObject *CreateSimple(JSContext *cx, typename D::Type type) {
        JSCompartment *comp = cx->compartment();

        TypeRepresentationHash::AddPtr p;
        {
            T sample(type);
            p = comp->typeReprs.lookupForAdd(&sample);
        }
        if (p)
            return (*p)->ownerObject();

        
        T *ptr = (T *) cx->malloc_(sizeof(T));
        if (!ptr)
            return nullptr;
        new(ptr) T(type);

        return ptr->addToTableOrFree(cx, p);
    }
};
} 


JSObject *
ScalarTypeRepresentation::Create(JSContext *cx,
                                 ScalarTypeDescr::Type type)
{
    return TypeRepresentationHelper::CreateSimple<ScalarTypeDescr,
                                                  ScalarTypeRepresentation>(cx, type);
}


JSObject *
X4TypeRepresentation::Create(JSContext *cx,
                             X4TypeDescr::Type type)
{
    return TypeRepresentationHelper::CreateSimple<X4TypeDescr,
                                                  X4TypeRepresentation>(cx, type);
}


JSObject *
ReferenceTypeRepresentation::Create(JSContext *cx,
                                    ReferenceTypeDescr::Type type)
{
    JSCompartment *comp = cx->compartment();

    TypeRepresentationHash::AddPtr p;
    {
        ReferenceTypeRepresentation sample(type);
        p = comp->typeReprs.lookupForAdd(&sample);
    }
    if (p)
        return (*p)->ownerObject();

    
    ReferenceTypeRepresentation *ptr =
        (ReferenceTypeRepresentation *) cx->malloc_(
            sizeof(ReferenceTypeRepresentation));
    if (!ptr)
        return nullptr;
    new(ptr) ReferenceTypeRepresentation(type);

    return ptr->addToTableOrFree(cx, p);
}


JSObject *
SizedArrayTypeRepresentation::Create(JSContext *cx,
                                     SizedTypeRepresentation *element,
                                     size_t length)
{
    JSCompartment *comp = cx->compartment();

    
    
    
    int32_t temp;
    if (!SafeMul(element->size(), length, &temp)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_TYPEDOBJECT_TOO_BIG);
        return nullptr;
    }

    TypeRepresentationHash::AddPtr p;
    {
        SizedArrayTypeRepresentation sample(element, length);
        p = comp->typeReprs.lookupForAdd(&sample);
    }
    if (p)
        return (*p)->ownerObject();

    
    SizedArrayTypeRepresentation *ptr =
        (SizedArrayTypeRepresentation *) cx->malloc_(
            sizeof(SizedArrayTypeRepresentation));
    if (!ptr)
        return nullptr;
    new(ptr) SizedArrayTypeRepresentation(element, length);

    return ptr->addToTableOrFree(cx, p);
}



JSObject *
UnsizedArrayTypeRepresentation::Create(JSContext *cx,
                                       SizedTypeRepresentation *element)
{
    JSCompartment *comp = cx->compartment();

    TypeRepresentationHash::AddPtr p;
    {
        UnsizedArrayTypeRepresentation sample(element);
        p = comp->typeReprs.lookupForAdd(&sample);
    }
    if (p)
        return (*p)->ownerObject();

    
    UnsizedArrayTypeRepresentation *ptr =
        (UnsizedArrayTypeRepresentation *) cx->malloc_(
            sizeof(UnsizedArrayTypeRepresentation));
    if (!ptr)
        return nullptr;
    new(ptr) UnsizedArrayTypeRepresentation(element);

    return ptr->addToTableOrFree(cx, p);
}


JSObject *
StructTypeRepresentation::Create(JSContext *cx,
                                 AutoPropertyNameVector &names,
                                 AutoObjectVector &typeReprOwners)
{
    size_t count = names.length();
    JSCompartment *comp = cx->compartment();

    
    size_t size = sizeof(StructTypeRepresentation) + count * sizeof(StructField);
    StructTypeRepresentation *ptr =
        (StructTypeRepresentation *) cx->malloc_(size);
    new(ptr) StructTypeRepresentation();
    if (!ptr->init(cx, names, typeReprOwners))
        return nullptr;

    TypeRepresentationHash::AddPtr p = comp->typeReprs.lookupForAdd(ptr);
    if (p) {
        js_free(ptr); 
        return (*p)->ownerObject();
    }

    return ptr->addToTableOrFree(cx, p);
}




void
TypeRepresentation::mark(JSTracer *trace)
{
    
    
    
    
    gc::MarkObject(trace, &ownerObject_, "typeRepresentation_ownerObject");
}

 void
TypeRepresentation::obj_trace(JSTracer *trace, JSObject *object)
{
    fromOwnerObject(*object)->traceFields(trace);
}

void
TypeRepresentation::traceFields(JSTracer *trace)
{
    mark(trace); 

    switch (kind()) {
      case TypeDescr::Scalar:
      case TypeDescr::Reference:
      case TypeDescr::X4:
        break;

      case TypeDescr::Struct:
        asStruct()->traceStructFields(trace);
        break;

      case TypeDescr::SizedArray:
        asSizedArray()->traceSizedArrayFields(trace);
        break;

      case TypeDescr::UnsizedArray:
        asUnsizedArray()->traceUnsizedArrayFields(trace);
        break;
    }
}

void
StructTypeRepresentation::traceStructFields(JSTracer *trace)
{
    for (size_t i = 0; i < fieldCount(); i++) {
        gc::MarkString(trace, &fields()[i].propertyName, "typerepr_field_propertyName");
        fields()[i].typeRepr->mark(trace);
    }
}

void
SizedArrayTypeRepresentation::traceSizedArrayFields(JSTracer *trace)
{
    this->mark(trace);
    element_->mark(trace);
}

void
UnsizedArrayTypeRepresentation::traceUnsizedArrayFields(JSTracer *trace)
{
    this->mark(trace);
    element_->mark(trace);
}




 void
TypeRepresentation::obj_finalize(js::FreeOp *fop, JSObject *object)
{
    JSCompartment *comp = object->compartment();
    TypeRepresentation *typeRepr = fromOwnerObject(*object);
    comp->typeReprs.remove(typeRepr);
    js_free(typeRepr);
}




template<typename V>
static void
visitReferences(SizedTypeRepresentation *repr,
                uint8_t *mem,
                V& visitor)
{
    if (repr->transparent())
        return;

    switch (repr->kind()) {
      case TypeDescr::Scalar:
      case TypeDescr::X4:
        return;

      case TypeDescr::Reference:
        visitor.visitReference(repr->asReference(), mem);
        return;

      case TypeDescr::SizedArray:
      {
        SizedArrayTypeRepresentation *arrayRepr = repr->asSizedArray();
        SizedTypeRepresentation *elementRepr = arrayRepr->element();
        for (size_t i = 0; i < arrayRepr->length(); i++) {
            visitReferences(elementRepr, mem, visitor);
            mem += elementRepr->size();
        }
        return;
      }

      case TypeDescr::UnsizedArray:
      {
        MOZ_ASSUME_UNREACHABLE("Only Sized Type representations");
      }

      case TypeDescr::Struct:
      {
        StructTypeRepresentation *structRepr = repr->asStruct();
        for (size_t i = 0; i < structRepr->fieldCount(); i++) {
            const StructField &f = structRepr->field(i);
            visitReferences(f.typeRepr, mem + f.offset, visitor);
        }
        return;
      }
    }

    MOZ_ASSUME_UNREACHABLE("Invalid type repr kind");
}




namespace js {
class MemoryInitVisitor {
    const JSRuntime *rt_;

  public:
    MemoryInitVisitor(const JSRuntime *rt)
      : rt_(rt)
    {}

    void visitReference(ReferenceTypeRepresentation *repr, uint8_t *mem);
};
} 

void
js::MemoryInitVisitor::visitReference(ReferenceTypeRepresentation *repr, uint8_t *mem)
{
    switch (repr->type()) {
      case ReferenceTypeDescr::TYPE_ANY:
      {
        js::HeapValue *heapValue = reinterpret_cast<js::HeapValue *>(mem);
        heapValue->init(UndefinedValue());
        return;
      }

      case ReferenceTypeDescr::TYPE_OBJECT:
      {
        js::HeapPtrObject *objectPtr =
            reinterpret_cast<js::HeapPtrObject *>(mem);
        objectPtr->init(nullptr);
        return;
      }

      case ReferenceTypeDescr::TYPE_STRING:
      {
        js::HeapPtrString *stringPtr =
            reinterpret_cast<js::HeapPtrString *>(mem);
        stringPtr->init(rt_->emptyString);
        return;
      }
    }

    MOZ_ASSUME_UNREACHABLE("Invalid kind");
}

void
SizedTypeRepresentation::initInstance(const JSRuntime *rt,
                                      uint8_t *mem,
                                      size_t length)
{
    JS_ASSERT(length >= 1);

    MemoryInitVisitor visitor(rt);

    
    memset(mem, 0, size());
    if (opaque())
        visitReferences(this, mem, visitor);

    
    uint8_t *target = mem;
    for (size_t i = 1; i < length; i++) {
        target += size();
        memcpy(target, mem, size());
    }
}




namespace js {
class MemoryTracingVisitor {
    JSTracer *trace_;

  public:

    MemoryTracingVisitor(JSTracer *trace)
      : trace_(trace)
    {}

    void visitReference(ReferenceTypeRepresentation *repr, uint8_t *mem);
};
} 

void
js::MemoryTracingVisitor::visitReference(ReferenceTypeRepresentation *repr, uint8_t *mem)
{
    switch (repr->type()) {
      case ReferenceTypeDescr::TYPE_ANY:
      {
        js::HeapValue *heapValue = reinterpret_cast<js::HeapValue *>(mem);
        gc::MarkValue(trace_, heapValue, "reference-val");
        return;
      }

      case ReferenceTypeDescr::TYPE_OBJECT:
      {
        js::HeapPtrObject *objectPtr =
            reinterpret_cast<js::HeapPtrObject *>(mem);
        if (*objectPtr)
            gc::MarkObject(trace_, objectPtr, "reference-obj");
        return;
      }

      case ReferenceTypeDescr::TYPE_STRING:
      {
        js::HeapPtrString *stringPtr =
            reinterpret_cast<js::HeapPtrString *>(mem);
        if (*stringPtr)
            gc::MarkString(trace_, stringPtr, "reference-str");
        return;
      }
    }

    MOZ_ASSUME_UNREACHABLE("Invalid kind");
}

void
SizedTypeRepresentation::traceInstance(JSTracer *trace,
                                       uint8_t *mem,
                                       size_t length)
{
    MemoryTracingVisitor visitor(trace);

    for (size_t i = 0; i < length; i++) {
        visitReferences(this, mem, visitor);
        mem += size();
    }
}




const StructField *
StructTypeRepresentation::fieldNamed(jsid id) const
{
    if (!JSID_IS_ATOM(id))
        return nullptr;

    uint32_t unused;
    JSAtom *atom = JSID_TO_ATOM(id);
    AutoThreadSafeAccess ts(atom);

    if (atom->isIndex(&unused))
        return nullptr;

    PropertyName *name = atom->asPropertyName();

    for (size_t i = 0; i < fieldCount(); i++) {
        if (field(i).propertyName.get() == name)
            return &field(i);
    }
    return nullptr;
}

 bool
TypeRepresentation::isOwnerObject(JSObject &obj)
{
    return obj.getClass() == &class_;
}

 TypeRepresentation *
TypeRepresentation::fromOwnerObject(JSObject &obj)
{
    JS_ASSERT(obj.getClass() == &class_);
    return (TypeRepresentation*) obj.getPrivate();
}


