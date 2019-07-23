






































#ifndef NSNATIVEMETHOD_H
#define NSNATIVEMETHOD_H

#include "nsNativeTypes.h"
#include "nsIXPCScriptable.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "prlink.h"
#include "jsapi.h"
#include "ffi.h"

struct nsNativeType
{
  ffi_type mType;
  PRUint16 mNativeType;
};

struct nsNativeValue
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

class nsNativeMethod : public nsIXPCScriptable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSCRIPTABLE

  nsNativeMethod();

  nsresult Init(JSContext* aContext, nsNativeTypes* aLibrary, PRFuncPtr aFunc, PRUint16 aCallType, jsval aResultType, const nsTArray<jsval>& aArgTypes);

private:
  ~nsNativeMethod();

  PRBool Execute(JSContext* aContext, PRUint32 aArgc, jsval* aArgv, jsval* aValue);

protected:
  
  nsRefPtr<nsNativeTypes> mLibrary;

  PRFuncPtr mFunc;

  ffi_abi mCallType;
  nsNativeType mResultType;
  nsAutoTArray<nsNativeType, 16> mArgTypes;
  nsAutoTArray<ffi_type*, 16> mFFITypes;

  ffi_cif mCIF;
};

#endif
