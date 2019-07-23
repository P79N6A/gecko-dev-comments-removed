






































#ifndef FUNCTION_H
#define FUNCTION_H

#include "Library.h"
#include "nsIXPCScriptable.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "prlink.h"
#include "jsapi.h"
#include "ffi.h"

namespace mozilla {
namespace ctypes {

struct Type
{
  ffi_type mFFIType;
  PRUint16 mType;
};

struct Value
{
  void* mData;
  union {
    int8_t   mInt8;
    int16_t  mInt16;
    int32_t  mInt32;
    int64_t  mInt64;
    uint8_t  mUint8;
    uint16_t mUint16;
    uint32_t mUint32;
    uint64_t mUint64;
    float    mFloat;
    double   mDouble;
    void*    mPointer;
  } mValue;
};

class Function : public nsIXPCScriptable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSCRIPTABLE

  Function();

  nsresult Init(JSContext* aContext, Library* aLibrary, PRFuncPtr aFunc, PRUint16 aCallType, jsval aResultType, const nsTArray<jsval>& aArgTypes);

private:
  ~Function();

  PRBool Execute(JSContext* aContext, PRUint32 aArgc, jsval* aArgv, jsval* aValue);

protected:
  
  nsRefPtr<Library> mLibrary;

  PRFuncPtr mFunc;

  ffi_abi mCallType;
  Type mResultType;
  nsAutoTArray<Type, 16> mArgTypes;
  nsAutoTArray<ffi_type*, 16> mFFITypes;

  ffi_cif mCIF;
};

}
}

#endif
