






































#ifndef FUNCTION_H
#define FUNCTION_H

#include "Module.h"
#include "nsTArray.h"
#include "prlink.h"
#include "ffi.h"

namespace mozilla {
namespace ctypes {


enum ErrorNum {
#define MSG_DEF(name, number, count, exception, format) \
  name = number,
#include "ctypes.msg"
#undef MSG_DEF
  CTYPESERR_LIMIT
};

const JSErrorFormatString*
GetErrorMessage(void* userRef, const char* locale, const uintN errorNumber);

struct Type
{
  ffi_type mFFIType;
  TypeCode mType;
};

struct Value
{
  void* mData;
  union {
    PRInt8   mInt8;
    PRInt16  mInt16;
    PRInt32  mInt32;
    PRInt64  mInt64;
    PRUint8  mUint8;
    PRUint16 mUint16;
    PRUint32 mUint32;
    PRUint64 mUint64;
    float    mFloat;
    double   mDouble;
    void*    mPointer;
  } mValue;
};

class Function
{
public:
  Function();

  Function*& Next() { return mNext; }

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
