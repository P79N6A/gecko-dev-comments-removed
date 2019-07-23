






































#ifndef FUNCTION_H
#define FUNCTION_H

#include "CTypes.h"
#include "nsTArray.h"
#include "prlink.h"

namespace mozilla {
namespace ctypes {

enum FunctionSlot
{
  SLOT_FUNCTION = 0,
  SLOT_LIBRARYOBJ = 1
  
};

struct Type
{
  ffi_type mFFIType;
  JSObject* mType;
};

struct AutoValue
{
  AutoValue() : mData(NULL) { }

  ~AutoValue()
  {
    delete static_cast<char*>(mData);
  }

  bool SizeToType(JSContext* cx, JSObject* type)
  {
    size_t size = CType::GetSize(cx, type);
    mData = new char[size];
    if (mData)
      memset(mData, 0, size);
    return mData != NULL;
  }

  void* mData;
};

class Function
{
public:
  Function();

  Function*& Next() { return mNext; }
  void Trace(JSTracer *trc);

  static JSObject* Create(JSContext* aContext, JSObject* aLibrary, PRFuncPtr aFunc, const char* aName, jsval aCallType, jsval aResultType, jsval* aArgTypes, uintN aArgLength);
  static JSBool Call(JSContext* cx, uintN argc, jsval* vp);

  ~Function();

private:
  bool Init(JSContext* aContext, PRFuncPtr aFunc, jsval aCallType, jsval aResultType, jsval* aArgTypes, uintN aArgLength);
  bool Execute(JSContext* cx, PRUint32 argc, jsval* vp);

protected:
  PRFuncPtr mFunc;

  ffi_abi mCallType;
  Type mResultType;
  nsAutoTArray<Type, 16> mArgTypes;
  nsAutoTArray<ffi_type*, 16> mFFITypes;

  ffi_cif mCIF;

  Function* mNext;
};

}
}

#endif
