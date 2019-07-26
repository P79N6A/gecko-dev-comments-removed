





#ifndef mozilla_dom_TypedArray_h
#define mozilla_dom_TypedArray_h

#include "jsfriendapi.h"
#include "jsapi.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {







template<typename T,
         JSObject* UnboxArray(JSObject*, uint32_t*, T**)>
struct TypedArray_base : AllTypedArraysBase {
  TypedArray_base(JSObject* obj)
  {
    DoInit(obj);
  }

  TypedArray_base() :
    mObj(nullptr)
  {
  }

private:
  T* mData;
  uint32_t mLength;
  JSObject* mObj;

public:
  inline bool Init(JSObject* obj)
  {
    MOZ_ASSERT(!inited());
    DoInit(obj);
    return inited();
  }

  inline bool inited() const {
    return !!mObj;
  }

  inline T *Data() const {
    MOZ_ASSERT(inited());
    return mData;
  }

  inline uint32_t Length() const {
    MOZ_ASSERT(inited());
    return mLength;
  }

  inline JSObject *Obj() const {
    MOZ_ASSERT(inited());
    return mObj;
  }

  inline bool WrapIntoNewCompartment(JSContext* cx)
  {
    return JS_WrapObject(cx, &mObj);
  }

  inline void TraceSelf(JSTracer* trc)
  {
    JS_CallObjectTracer(trc, &mObj, "TypedArray.mObj");
  }

protected:
  inline void DoInit(JSObject* obj)
  {
    mObj = UnboxArray(obj, &mLength, &mData);
  }
};


template<typename T,
         T* GetData(JSObject*),
         JSObject* UnboxArray(JSObject*, uint32_t*, T**),
         JSObject* CreateNew(JSContext*, uint32_t)>
struct TypedArray : public TypedArray_base<T,UnboxArray> {
  TypedArray(JSObject* obj) :
    TypedArray_base<T,UnboxArray>(obj)
  {}

  TypedArray() :
    TypedArray_base<T,UnboxArray>()
  {}

  static inline JSObject*
  Create(JSContext* cx, nsWrapperCache* creator, uint32_t length,
         const T* data = nullptr) {
    JS::Rooted<JSObject*> creatorWrapper(cx);
    Maybe<JSAutoCompartment> ac;
    if (creator && (creatorWrapper = creator->GetWrapperPreserveColor())) {
      ac.construct(cx, creatorWrapper);
    }

    return CreateCommon(cx, creatorWrapper, length, data);
  }

  static inline JSObject*
  Create(JSContext* cx, JS::Handle<JSObject*> creator, uint32_t length,
         const T* data = nullptr) {
    Maybe<JSAutoCompartment> ac;
    if (creator) {
      ac.construct(cx, creator);
    }

    return CreateCommon(cx, creator, length, data);
  }

private:
  static inline JSObject*
  CreateCommon(JSContext* cx, JS::Handle<JSObject*> creator, uint32_t length,
               const T* data) {
    JSObject* obj = CreateNew(cx, length);
    if (!obj) {
      return nullptr;
    }
    if (data) {
      T* buf = static_cast<T*>(GetData(obj));
      memcpy(buf, data, length*sizeof(T));
    }
    return obj;
  }
};

typedef TypedArray<int8_t, JS_GetInt8ArrayData, JS_GetObjectAsInt8Array,
                   JS_NewInt8Array>
        Int8Array;
typedef TypedArray<uint8_t, JS_GetUint8ArrayData,
                   JS_GetObjectAsUint8Array, JS_NewUint8Array>
        Uint8Array;
typedef TypedArray<uint8_t, JS_GetUint8ClampedArrayData,
                   JS_GetObjectAsUint8ClampedArray, JS_NewUint8ClampedArray>
        Uint8ClampedArray;
typedef TypedArray<int16_t, JS_GetInt16ArrayData,
                   JS_GetObjectAsInt16Array, JS_NewInt16Array>
        Int16Array;
typedef TypedArray<uint16_t, JS_GetUint16ArrayData,
                   JS_GetObjectAsUint16Array, JS_NewUint16Array>
        Uint16Array;
typedef TypedArray<int32_t, JS_GetInt32ArrayData,
                   JS_GetObjectAsInt32Array, JS_NewInt32Array>
        Int32Array;
typedef TypedArray<uint32_t, JS_GetUint32ArrayData,
                   JS_GetObjectAsUint32Array, JS_NewUint32Array>
        Uint32Array;
typedef TypedArray<float, JS_GetFloat32ArrayData,
                   JS_GetObjectAsFloat32Array, JS_NewFloat32Array>
        Float32Array;
typedef TypedArray<double, JS_GetFloat64ArrayData,
                   JS_GetObjectAsFloat64Array, JS_NewFloat64Array>
        Float64Array;
typedef TypedArray_base<uint8_t, JS_GetObjectAsArrayBufferView>
        ArrayBufferView;
typedef TypedArray<uint8_t, JS_GetArrayBufferData,
                   JS_GetObjectAsArrayBuffer, JS_NewArrayBuffer>
        ArrayBuffer;

} 
} 

#endif 
