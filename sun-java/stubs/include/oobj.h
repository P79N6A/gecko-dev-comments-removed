



































#ifndef _OOBJ_H_
#define _OOBJ_H_

#include <stddef.h>

#include "typedefs.h"
#include "bool.h"
#include "jriext.h"

typedef void Hjava_lang_Class;
typedef Hjava_lang_Class ClassClass;

typedef void Hjava_lang_Object;
typedef Hjava_lang_Object HObject;
typedef Hjava_lang_Object JHandle;

struct methodblock;

JRI_PUBLIC_API(void)
MakeClassSticky(ClassClass *cb);

#endif 
