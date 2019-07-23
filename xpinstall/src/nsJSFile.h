




































#ifndef __NS_JSFILE_H__
#define __NS_JSFILE_H__

#include "jsapi.h"
#include "nscore.h"


JSBool JS_DLL_CALLBACK
InstallFileOpDirCreate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpDirGetParent(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpDirRemove(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpDirRename(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpFileCopy(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpFileRemove(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpFileExists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpFileExecute(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpFileGetNativeVersion(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpFileGetDiskSpaceAvailable(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpFileGetModDate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpFileGetSize(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpFileIsDirectory(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpFileIsWritable(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpFileIsFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpFileModDateChanged(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpFileMove(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpFileRename(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpFileWindowsShortcut(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpFileMacAlias(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
InstallFileOpFileUnixLink(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

PRInt32 InitXPFileOpObjectPrototype(JSContext *jscontext, JSObject *global, JSObject **fileOpObjectPrototype);

#endif
