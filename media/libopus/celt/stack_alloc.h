







































#ifndef STACK_ALLOC_H
#define STACK_ALLOC_H

#ifdef USE_ALLOCA
# ifdef WIN32
#  include <malloc.h>
# else
#  ifdef HAVE_ALLOCA_H
#   include <alloca.h>
#  else
#   include <stdlib.h>
#  endif
# endif
#endif






































#if defined(VAR_ARRAYS)

#define VARDECL(type, var)
#define ALLOC(var, size, type) type var[size]
#define SAVE_STACK
#define RESTORE_STACK
#define ALLOC_STACK

#elif defined(USE_ALLOCA)

#define VARDECL(type, var) type *var

# ifdef WIN32
#  define ALLOC(var, size, type) var = ((type*)_alloca(sizeof(type)*(size)))
# else
#  define ALLOC(var, size, type) var = ((type*)alloca(sizeof(type)*(size)))
# endif

#define SAVE_STACK
#define RESTORE_STACK
#define ALLOC_STACK

#else

#ifdef CELT_C
char *global_stack=0;
#else
extern char *global_stack;
#endif 

#ifdef ENABLE_VALGRIND

#include <valgrind/memcheck.h>

#ifdef CELT_C
char *global_stack_top=0;
#else
extern char *global_stack_top;
#endif 

#define ALIGN(stack, size) ((stack) += ((size) - (long)(stack)) & ((size) - 1))
#define PUSH(stack, size, type) (VALGRIND_MAKE_MEM_NOACCESS(stack, global_stack_top-stack),ALIGN((stack),sizeof(type)/sizeof(char)),VALGRIND_MAKE_MEM_UNDEFINED(stack, ((size)*sizeof(type)/sizeof(char))),(stack)+=(2*(size)*sizeof(type)/sizeof(char)),(type*)((stack)-(2*(size)*sizeof(type)/sizeof(char))))
#define RESTORE_STACK ((global_stack = _saved_stack),VALGRIND_MAKE_MEM_NOACCESS(global_stack, global_stack_top-global_stack))
#define ALLOC_STACK char *_saved_stack; ((global_stack = (global_stack==0) ? ((global_stack_top=opus_alloc_scratch(GLOBAL_STACK_SIZE*2)+(GLOBAL_STACK_SIZE*2))-(GLOBAL_STACK_SIZE*2)) : global_stack),VALGRIND_MAKE_MEM_NOACCESS(global_stack, global_stack_top-global_stack)); _saved_stack = global_stack;

#else

#define ALIGN(stack, size) ((stack) += ((size) - (long)(stack)) & ((size) - 1))
#define PUSH(stack, size, type) (ALIGN((stack),sizeof(type)/sizeof(char)),(stack)+=(size)*(sizeof(type)/sizeof(char)),(type*)((stack)-(size)*(sizeof(type)/sizeof(char))))
#define RESTORE_STACK (global_stack = _saved_stack)
#define ALLOC_STACK char *_saved_stack; (global_stack = (global_stack==0) ? opus_alloc_scratch(GLOBAL_STACK_SIZE) : global_stack); _saved_stack = global_stack;

#endif 

#include "os_support.h"
#define VARDECL(type, var) type *var
#define ALLOC(var, size, type) var = PUSH(global_stack, size, type)
#define SAVE_STACK char *_saved_stack = global_stack;

#endif 

#endif 
