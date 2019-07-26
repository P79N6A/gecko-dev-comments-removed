





#ifndef shell_jsheaptools_h
#define shell_jsheaptools_h

#ifdef DEBUG

#include "js/TypeDecls.h"

bool FindReferences(JSContext *cx, unsigned argc, JS::Value *vp);

#endif 

#endif 
