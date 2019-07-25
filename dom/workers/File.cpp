






































#include "File.h"

#include "nsIDOMFile.h"

#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsfriendapi.h"
#include "nsCOMPtr.h"
#include "nsJSUtils.h"
#include "nsStringGlue.h"
#include "xpcprivate.h"
#include "xpcquickstubs.h"

#include "Exceptions.h"
#include "WorkerInlines.h"
#include "WorkerPrivate.h"

#define PROPERTY_FLAGS \
  JSPROP_ENUMERATE | JSPROP_SHARED

USING_WORKERS_NAMESPACE

using mozilla::dom::workers::exceptions::ThrowFileExceptionForCode;

namespace {

class Blob
{
  
  Blob();
  ~Blob();

  static JSClass sClass;
  static JSPropertySpec sProperties[];
  static JSFunctionSpec sFunctions[];

public:
  static JSObject*
  InitClass(JSContext* aCx, JSObject* aObj)
  {
    return JS_InitClass(aCx, aObj, NULL, &sClass, Construct, 0, sProperties,
                        sFunctions, NULL, NULL);
  }

  static JSObject*
  Create(JSContext* aCx, nsIDOMBlob* aBlob)
  {
    JS_ASSERT(SameCOMIdentity(static_cast<nsISupports*>(aBlob), aBlob));

    JSObject* obj = JS_NewObject(aCx, &sClass, NULL, NULL);
    if (obj) {
      if (!JS_SetPrivate(aCx, obj, aBlob)) {
        return NULL;
      }
      NS_ADDREF(aBlob);
    }
    return obj;
  }

  static nsIDOMBlob*
  GetPrivate(JSContext* aCx, JSObject* aObj);

private:
  static nsIDOMBlob*
  GetInstancePrivate(JSContext* aCx, JSObject* aObj, const char* aFunctionName)
  {
    JSClass* classPtr = NULL;

    if (aObj) {
      nsIDOMBlob* blob = GetPrivate(aCx, aObj);
      if (blob) {
        return blob;
      }

      classPtr = JS_GET_CLASS(aCx, aObj);
    }

    JS_ReportErrorNumber(aCx, js_GetErrorMessage, NULL,
                         JSMSG_INCOMPATIBLE_PROTO, sClass.name, aFunctionName,
                         classPtr ? classPtr->name : "Object");
    return NULL;
  }

  static JSBool
  Construct(JSContext* aCx, uintN aArgc, jsval* aVp)
  {
    JS_ReportErrorNumber(aCx, js_GetErrorMessage, NULL, JSMSG_WRONG_CONSTRUCTOR,
                         sClass.name);
    return false;
  }

  static void
  Finalize(JSContext* aCx, JSObject* aObj)
  {
    JS_ASSERT(JS_GET_CLASS(aCx, aObj) == &sClass);

    nsIDOMBlob* blob = GetPrivate(aCx, aObj);
    NS_IF_RELEASE(blob);
  }

  static JSBool
  GetSize(JSContext* aCx, JSObject* aObj, jsid aIdval, jsval* aVp)
  {
    nsIDOMBlob* blob = GetInstancePrivate(aCx, aObj, "size");
    if (!blob) {
      return false;
    }

    PRUint64 size;
    if (NS_FAILED(blob->GetSize(&size))) {
      ThrowFileExceptionForCode(aCx, FILE_NOT_READABLE_ERR);
    }

    if (!JS_NewNumberValue(aCx, jsdouble(size), aVp)) {
      return false;
    }

    return true;
  }

  static JSBool
  GetType(JSContext* aCx, JSObject* aObj, jsid aIdval, jsval* aVp)
  {
    nsIDOMBlob* blob = GetInstancePrivate(aCx, aObj, "type");
    if (!blob) {
      return false;
    }

    nsString type;
    if (NS_FAILED(blob->GetType(type))) {
      ThrowFileExceptionForCode(aCx, FILE_NOT_READABLE_ERR);
    }

    JSString* jsType = JS_NewUCStringCopyN(aCx, type.get(), type.Length());
    if (!jsType) {
      return false;
    }

    *aVp = STRING_TO_JSVAL(jsType);

    return true;
  }

  static JSBool
  MozSlice(JSContext* aCx, uintN aArgc, jsval* aVp)
  {
    JSObject* obj = JS_THIS_OBJECT(aCx, aVp);

    nsIDOMBlob* blob = GetInstancePrivate(aCx, obj, "mozSlice");
    if (!blob) {
      return false;
    }

    jsdouble start = 0, end = 0;
    JSString* jsContentType = JS_GetEmptyString(JS_GetRuntime(aCx));
    if (!JS_ConvertArguments(aCx, aArgc, JS_ARGV(aCx, aVp), "/IIS", &start,
                             &end, &jsContentType)) {
      return false;
    }

    nsDependentJSString contentType;
    if (!contentType.init(aCx, jsContentType)) {
      return false;
    }

    PRUint8 optionalArgc = aArgc;
    nsCOMPtr<nsIDOMBlob> rtnBlob;
    if (NS_FAILED(blob->MozSlice(xpc_qsDoubleToUint64(start),
                                 xpc_qsDoubleToUint64(end),
                                 contentType, optionalArgc,
                                 getter_AddRefs(rtnBlob)))) {
      ThrowFileExceptionForCode(aCx, FILE_NOT_READABLE_ERR);
      return false;
    }

    JSObject* rtnObj = file::CreateBlob(aCx, rtnBlob);
    if (!rtnObj) {
      return false;
    }

    JS_SET_RVAL(aCx, aVp, OBJECT_TO_JSVAL(rtnObj));
    return true;
  }
};

JSClass Blob::sClass = {
  "Blob",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Finalize,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

JSPropertySpec Blob::sProperties[] = {
  { "size", 0, PROPERTY_FLAGS, GetSize, js_GetterOnlyPropertyStub },
  { "type", 0, PROPERTY_FLAGS, GetType, js_GetterOnlyPropertyStub },
  { 0, 0, 0, NULL, NULL }
};

JSFunctionSpec Blob::sFunctions[] = {
  JS_FN("mozSlice", MozSlice, 1, JSPROP_ENUMERATE),
  JS_FS_END
};

class File : public Blob
{
  
  File();
  ~File();

  static JSClass sClass;
  static JSPropertySpec sProperties[];

public:
  static JSObject*
  InitClass(JSContext* aCx, JSObject* aObj, JSObject* aParentProto)
  {
    return JS_InitClass(aCx, aObj, aParentProto, &sClass, Construct, 0,
                        sProperties, NULL, NULL, NULL);
  }

  static JSObject*
  Create(JSContext* aCx, nsIDOMFile* aFile)
  {
    JS_ASSERT(SameCOMIdentity(static_cast<nsISupports*>(aFile), aFile));

    JSObject* obj = JS_NewObject(aCx, &sClass, NULL, NULL);
    if (obj) {
      if (!JS_SetPrivate(aCx, obj, aFile)) {
        return NULL;
      }
      NS_ADDREF(aFile);
    }
    return obj;
  }

  static nsIDOMFile*
  GetPrivate(JSContext* aCx, JSObject* aObj)
  {
    if (aObj) {
      JSClass* classPtr = JS_GET_CLASS(aCx, aObj);
      if (classPtr == &sClass) {
        nsISupports* priv = static_cast<nsISupports*>(JS_GetPrivate(aCx, aObj));
        nsCOMPtr<nsIDOMFile> file = do_QueryInterface(priv);
        JS_ASSERT_IF(priv, file);
        return file;
      }
    }
    return NULL;
  }

  static JSClass*
  Class()
  {
    return &sClass;
  }

private:
  static nsIDOMFile*
  GetInstancePrivate(JSContext* aCx, JSObject* aObj, const char* aFunctionName)
  {
    JSClass* classPtr = NULL;

    if (aObj) {
      nsIDOMFile* file = GetPrivate(aCx, aObj);
      if (file) {
        return file;
      }
      classPtr = JS_GET_CLASS(aCx, aObj);
    }

    JS_ReportErrorNumber(aCx, js_GetErrorMessage, NULL,
                         JSMSG_INCOMPATIBLE_PROTO, sClass.name, aFunctionName,
                         classPtr ? classPtr->name : "Object");
    return NULL;
  }

  static JSBool
  Construct(JSContext* aCx, uintN aArgc, jsval* aVp)
  {
    JS_ReportErrorNumber(aCx, js_GetErrorMessage, NULL, JSMSG_WRONG_CONSTRUCTOR,
                         sClass.name);
    return false;
  }

  static void
  Finalize(JSContext* aCx, JSObject* aObj)
  {
    JS_ASSERT(JS_GET_CLASS(aCx, aObj) == &sClass);

    nsIDOMFile* file = GetPrivate(aCx, aObj);
    NS_IF_RELEASE(file);
  }

  static JSBool
  GetMozFullPath(JSContext* aCx, JSObject* aObj, jsid aIdval, jsval* aVp)
  {
    nsIDOMFile* file = GetInstancePrivate(aCx, aObj, "mozFullPath");
    if (!file) {
      return false;
    }

    nsString fullPath;

    if (GetWorkerPrivateFromContext(aCx)->UsesSystemPrincipal() &&
        NS_FAILED(file->GetMozFullPathInternal(fullPath))) {
      ThrowFileExceptionForCode(aCx, FILE_NOT_READABLE_ERR);
      return false;
    }

    JSString* jsFullPath = JS_NewUCStringCopyN(aCx, fullPath.get(),
                                               fullPath.Length());
    if (!jsFullPath) {
      return false;
    }

    *aVp = STRING_TO_JSVAL(jsFullPath);
    return true;
  }

  static JSBool
  GetName(JSContext* aCx, JSObject* aObj, jsid aIdval, jsval* aVp)
  {
    nsIDOMFile* file = GetInstancePrivate(aCx, aObj, "name");
    if (!file) {
      return false;
    }

    nsString name;
    if (NS_FAILED(file->GetName(name))) {
      name.Truncate();
    }

    JSString* jsName = JS_NewUCStringCopyN(aCx, name.get(), name.Length());
    if (!jsName) {
      return false;
    }

    *aVp = STRING_TO_JSVAL(jsName);
    return true;
  }
};

JSClass File::sClass = {
  "File",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Finalize,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

JSPropertySpec File::sProperties[] = {
  { "name", 0, PROPERTY_FLAGS, GetName, js_GetterOnlyPropertyStub },
  { "mozFullPath", 0, PROPERTY_FLAGS, GetMozFullPath,
    js_GetterOnlyPropertyStub },
  { 0, 0, 0, NULL, NULL }
};

nsIDOMBlob*
Blob::GetPrivate(JSContext* aCx, JSObject* aObj)
{
  if (aObj) {
    JSClass* classPtr = JS_GET_CLASS(aCx, aObj);
    if (classPtr == &sClass || classPtr == File::Class()) {
      nsISupports* priv = static_cast<nsISupports*>(JS_GetPrivate(aCx, aObj));
      nsCOMPtr<nsIDOMBlob> blob = do_QueryInterface(priv);
      JS_ASSERT_IF(priv, blob);
      return blob;
    }
  }
  return NULL;
}

} 

BEGIN_WORKERS_NAMESPACE

namespace file {

JSObject*
CreateBlob(JSContext* aCx, nsIDOMBlob* aBlob)
{
  return Blob::Create(aCx, aBlob);
}

bool
InitClasses(JSContext* aCx, JSObject* aGlobal)
{
  JSObject* blobProto = Blob::InitClass(aCx, aGlobal);
  return blobProto && File::InitClass(aCx, aGlobal, blobProto);
}

nsIDOMBlob*
GetDOMBlobFromJSObject(JSContext* aCx, JSObject* aObj)
{
  return Blob::GetPrivate(aCx, aObj);
}

JSObject*
CreateFile(JSContext* aCx, nsIDOMFile* aFile)
{
  return File::Create(aCx, aFile);
}

nsIDOMFile*
GetDOMFileFromJSObject(JSContext* aCx, JSObject* aObj)
{
  return File::GetPrivate(aCx, aObj);
}

} 

END_WORKERS_NAMESPACE
