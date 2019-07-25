






































#include "FileReaderSync.h"

#include "nsIDOMFile.h"

#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jstypedarray.h"
#include "nsJSUtils.h"
#include "xpcprivate.h"

#include "Exceptions.h"
#include "File.h"
#include "FileReaderSyncPrivate.h"
#include "WorkerInlines.h"

#define FUNCTION_FLAGS \
  JSPROP_ENUMERATE

USING_WORKERS_NAMESPACE

using mozilla::dom::workers::exceptions::ThrowFileExceptionForCode;
using js::ArrayBuffer;

namespace {

inline bool
EnsureSucceededOrThrow(JSContext* aCx, nsresult rv)
{
  if (NS_SUCCEEDED(rv)) {
    return true;
  }

  intN code = rv == NS_ERROR_FILE_NOT_FOUND ?
              FILE_NOT_FOUND_ERR :
              FILE_NOT_READABLE_ERR;
  ThrowFileExceptionForCode(aCx, code);
  return false;
}

inline nsIDOMBlob*
GetDOMBlobFromJSObject(JSContext* aCx, JSObject* aObj) {
  JSClass* classPtr = NULL;

  if (aObj) {
    nsIDOMBlob* blob = file::GetDOMBlobFromJSObject(aCx, aObj);
    if (blob) {
      return blob;
    }

    classPtr = JS_GET_CLASS(aCx, aObj);
  }

  JS_ReportErrorNumber(aCx, js_GetErrorMessage, NULL, JSMSG_UNEXPECTED_TYPE,
                       classPtr ? classPtr->name : "Object", "not a Blob.");
  return NULL;
}

class FileReaderSync
{
  
  FileReaderSync();
  ~FileReaderSync();

  static JSClass sClass;
  static JSFunctionSpec sFunctions[];

public:
  static JSObject*
  InitClass(JSContext* aCx, JSObject* aObj)
  {
    return JS_InitClass(aCx, aObj, NULL, &sClass, Construct, 0,
                        NULL, sFunctions, NULL, NULL);
  }

  static FileReaderSyncPrivate*
  GetPrivate(JSContext* aCx, JSObject* aObj)
  {
    if (aObj) {
      JSClass* classPtr = JS_GET_CLASS(aCx, aObj);
      if (classPtr == &sClass) {
        FileReaderSyncPrivate* fileReader =
          GetJSPrivateSafeish<FileReaderSyncPrivate>(aCx, aObj);
        return fileReader;
      }
    }
    return NULL;
  }

private:
  static FileReaderSyncPrivate*
  GetInstancePrivate(JSContext* aCx, JSObject* aObj, const char* aFunctionName)
  {
    JSClass* classPtr = NULL;

    if (aObj) {
      FileReaderSyncPrivate* fileReader = GetPrivate(aCx, aObj);
      if (fileReader) {
        return fileReader;
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
    JSObject* obj = JS_NewObject(aCx, &sClass, NULL, NULL);
    if (!obj) {
      return false;
    }

    FileReaderSyncPrivate* fileReader = new FileReaderSyncPrivate();
    NS_ADDREF(fileReader);

    if (!SetJSPrivateSafeish(aCx, obj, fileReader)) {
      NS_RELEASE(fileReader);
      return false;
    }

    JS_SET_RVAL(aCx, aVp, OBJECT_TO_JSVAL(obj));
    return true;
  }

  static void
  Finalize(JSContext* aCx, JSObject* aObj)
  {
    JS_ASSERT(JS_GET_CLASS(aCx, aObj) == &sClass);
    FileReaderSyncPrivate* fileReader =
      GetJSPrivateSafeish<FileReaderSyncPrivate>(aCx, aObj);
    NS_IF_RELEASE(fileReader);
  }

  static JSBool
  ReadAsArrayBuffer(JSContext* aCx, uintN aArgc, jsval* aVp)
  {
    JSObject* obj = JS_THIS_OBJECT(aCx, aVp);

    FileReaderSyncPrivate* fileReader =
      GetInstancePrivate(aCx, obj, "readAsArrayBuffer");
    if (!fileReader) {
      return false;
    }

    JSObject* jsBlob;
    if (!JS_ConvertArguments(aCx, aArgc, JS_ARGV(aCx, aVp), "o", &jsBlob)) {
      return false;
    }

    nsIDOMBlob* blob = GetDOMBlobFromJSObject(aCx, jsBlob);
    if (!blob) {
      return false;
    }

    PRUint64 blobSize;
    nsresult rv = blob->GetSize(&blobSize);
    if (!EnsureSucceededOrThrow(aCx, rv)) {
      return false;
    }

    JSObject* jsArrayBuffer = js_CreateArrayBuffer(aCx, blobSize);
    if (!jsArrayBuffer) {
      return false;
    }

    JSUint32 bufferLength = JS_GetArrayBufferByteLength(jsArrayBuffer);
    uint8* arrayBuffer = JS_GetArrayBufferData(jsArrayBuffer);

    rv = fileReader->ReadAsArrayBuffer(blob, bufferLength, arrayBuffer);
    if (!EnsureSucceededOrThrow(aCx, rv)) {
      return false;
    }

    JS_SET_RVAL(aCx, aVp, OBJECT_TO_JSVAL(jsArrayBuffer));
    return true;
  }

  static JSBool
  ReadAsDataURL(JSContext* aCx, uintN aArgc, jsval* aVp)
  {
    JSObject* obj = JS_THIS_OBJECT(aCx, aVp);

    FileReaderSyncPrivate* fileReader =
      GetInstancePrivate(aCx, obj, "readAsDataURL");
    if (!fileReader) {
      return false;
    }

    JSObject* jsBlob;
    if (!JS_ConvertArguments(aCx, aArgc, JS_ARGV(aCx, aVp), "o", &jsBlob)) {
      return false;
    }

    nsIDOMBlob* blob = GetDOMBlobFromJSObject(aCx, jsBlob);
    if (!blob) {
      return false;
    }

    nsString blobText;
    nsresult rv = fileReader->ReadAsDataURL(blob, blobText);
    if (!EnsureSucceededOrThrow(aCx, rv)) {
      return false;
    }

    JSString* jsBlobText = JS_NewUCStringCopyN(aCx, blobText.get(),
                                               blobText.Length());
    if (!jsBlobText) {
      return false;
    }

    JS_SET_RVAL(aCx, aVp, STRING_TO_JSVAL(jsBlobText));
    return true;
  }

  static JSBool
  ReadAsBinaryString(JSContext* aCx, uintN aArgc, jsval* aVp)
  {
    JSObject* obj = JS_THIS_OBJECT(aCx, aVp);

    FileReaderSyncPrivate* fileReader =
      GetInstancePrivate(aCx, obj, "readAsBinaryString");
    if (!fileReader) {
      return false;
    }

    JSObject* jsBlob;
    if (!JS_ConvertArguments(aCx, aArgc, JS_ARGV(aCx, aVp), "o", &jsBlob)) {
      return false;
    }

    nsIDOMBlob* blob = GetDOMBlobFromJSObject(aCx, jsBlob);
    if (!blob) {
      return false;
    }

    nsString blobText;
    nsresult rv = fileReader->ReadAsBinaryString(blob, blobText);
    if (!EnsureSucceededOrThrow(aCx, rv)) {
      return false;
    }

    JSString* jsBlobText = JS_NewUCStringCopyN(aCx, blobText.get(),
                                               blobText.Length());
    if (!jsBlobText) {
      return false;
    }

    JS_SET_RVAL(aCx, aVp, STRING_TO_JSVAL(jsBlobText));
    return true;
  }

  static JSBool
  ReadAsText(JSContext* aCx, uintN aArgc, jsval* aVp)
  {
    JSObject* obj = JS_THIS_OBJECT(aCx, aVp);

    FileReaderSyncPrivate* fileReader =
      GetInstancePrivate(aCx, obj, "readAsText");
    if (!fileReader) {
      return false;
    }

    JSObject* jsBlob;
    JSString* jsEncoding = JS_GetEmptyString(JS_GetRuntime(aCx));
    if (!JS_ConvertArguments(aCx, aArgc, JS_ARGV(aCx, aVp), "o/S", &jsBlob,
                             &jsEncoding)) {
      return false;
    }

    nsDependentJSString encoding;
    if (!encoding.init(aCx, jsEncoding)) {
      return false;
    }

    nsIDOMBlob* blob = GetDOMBlobFromJSObject(aCx, jsBlob);
    if (!blob) {
      return false;
    }

    nsString blobText;
    nsresult rv = fileReader->ReadAsText(blob, encoding, blobText);
    if (!EnsureSucceededOrThrow(aCx, rv)) {
      return false;
    }

    JSString* jsBlobText = JS_NewUCStringCopyN(aCx, blobText.get(),
                                               blobText.Length());
    if (!jsBlobText) {
      return false;
    }

    JS_SET_RVAL(aCx, aVp, STRING_TO_JSVAL(jsBlobText));
    return true;
  }
};

JSClass FileReaderSync::sClass = {
  "FileReaderSync",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Finalize,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

JSFunctionSpec FileReaderSync::sFunctions[] = {
  JS_FN("readAsArrayBuffer", ReadAsArrayBuffer, 1, FUNCTION_FLAGS),
  JS_FN("readAsBinaryString", ReadAsBinaryString, 1, FUNCTION_FLAGS),
  JS_FN("readAsText", ReadAsText, 1, FUNCTION_FLAGS),
  JS_FN("readAsDataURL", ReadAsDataURL, 1, FUNCTION_FLAGS),
  JS_FS_END
};

} 

BEGIN_WORKERS_NAMESPACE

namespace filereadersync {

bool
InitClass(JSContext* aCx, JSObject* aGlobal)
{
  return !!FileReaderSync::InitClass(aCx, aGlobal);
}

} 

END_WORKERS_NAMESPACE
