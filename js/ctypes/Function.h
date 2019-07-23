






































#ifndef FUNCTION_H
#define FUNCTION_H

#include "Library.h"
#include "nsIXPCScriptable.h"
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
