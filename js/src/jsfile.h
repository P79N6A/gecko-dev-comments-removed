






































#ifndef _jsfile_h__
#define _jsfile_h__

#if JS_HAS_FILE_OBJECT

#include "jsobj.h"

extern JS_PUBLIC_API(JSObject*)
js_InitFileClass(JSContext *cx, JSObject* obj);

extern JS_PUBLIC_API(JSObject*)
js_NewFileObject(JSContext *cx, char *bytes);

extern JSClass js_FileClass;

#endif 
#endif 
