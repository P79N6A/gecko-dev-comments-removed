





#ifndef shell_jsheaptools_h
#define shell_jsheaptools_h

#ifdef DEBUG

#include "jsapi.h"

JSBool FindReferences(JSContext *cx, unsigned argc, jsval *vp);

#endif 

#endif 
