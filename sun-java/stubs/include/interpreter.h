



































#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#include "prtypes.h"

PR_BEGIN_EXTERN_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bool.h"
#include "jni.h"

typedef struct execenv ExecEnv;

typedef void JavaStack;

typedef void JavaFrame; 

struct Hjava_lang_ClassLoader;

JRI_PUBLIC_API(JHandle *)
ArrayAlloc(int32_t, int32_t);

JRI_PUBLIC_API(JavaFrame *)
CompiledFramePrev(JavaFrame *, JavaFrame *);

JRI_PUBLIC_API(JavaStack *)
CreateNewJavaStack(ExecEnv *, JavaStack *);

JRI_PUBLIC_API(bool_t)
ExecuteJava(unsigned char *, ExecEnv *);

JRI_PUBLIC_API(ClassClass *)
FindClassFromClass(struct execenv *, char *, bool_t, ClassClass *);

JRI_PUBLIC_API(ClassClass *)
FindLoadedClass(char *, struct Hjava_lang_ClassLoader *);

JRI_PUBLIC_API(void)
PrintToConsole(const char *);

JRI_PUBLIC_API(bool_t)
VerifyClassAccess(ClassClass *, ClassClass *, bool_t);

JRI_PUBLIC_API(bool_t)
VerifyFieldAccess(ClassClass *, ClassClass *, int, bool_t);

JRI_PUBLIC_API(long)
do_execute_java_method(ExecEnv *, void *, char *, char *,
                       struct methodblock *, bool_t, ...);

JRI_PUBLIC_API(long)
do_execute_java_method_vararg(ExecEnv *, void *, char *, char *,
                              struct methodblock *, bool_t, va_list,
                              long *, bool_t);

JRI_PUBLIC_API(HObject *)
execute_java_constructor_vararg(struct execenv *, char *, ClassClass *,
                                char *, va_list);

JRI_PUBLIC_API(HObject *)
execute_java_constructor(ExecEnv *, char *, ClassClass *, char *, ...);

JRI_PUBLIC_API(bool_t)
is_subclass_of(ClassClass *, ClassClass *, ExecEnv *);

JRI_PUBLIC_API(HObject*)
newobject(ClassClass *, unsigned char *, struct execenv *);

JRI_PUBLIC_API(int32_t)
sizearray(int32_t, int32_t);

PR_END_EXTERN_C

#endif 
