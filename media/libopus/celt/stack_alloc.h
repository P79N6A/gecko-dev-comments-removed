






























#ifndef STACK_ALLOC_H
#define STACK_ALLOC_H

#if (!defined (VAR_ARRAYS) && !defined (USE_ALLOCA) && !defined (NONTHREADSAFE_PSEUDOSTACK))
#error "Opus requires one of VAR_ARRAYS, USE_ALLOCA, or NONTHREADSAFE_PSEUDOSTACK be defined to select the temporary allocation mode."
#endif

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


#ifdef ENABLE_VALGRIND

#include <valgrind/memcheck.h>
#define OPUS_CHECK_ARRAY(ptr, len) VALGRIND_CHECK_MEM_IS_DEFINED(ptr, len*sizeof(*ptr))
#define OPUS_CHECK_VALUE(value) VALGRIND_CHECK_VALUE_IS_DEFINED(value)
#define OPUS_CHECK_ARRAY_COND(ptr, len) VALGRIND_CHECK_MEM_IS_DEFINED(ptr, len*sizeof(*ptr))
#define OPUS_CHECK_VALUE_COND(value) VALGRIND_CHECK_VALUE_IS_DEFINED(value)
#define OPUS_PRINT_INT(value) do {fprintf(stderr, #value " = %d at %s:%d\n", value, __FILE__, __LINE__);}while(0)
#define OPUS_FPRINTF fprintf

#else

static inline int _opus_false(void) {return 0;}
#define OPUS_CHECK_ARRAY(ptr, len) _opus_false()
#define OPUS_CHECK_VALUE(value) _opus_false()
#define OPUS_PRINT_INT(value) do{}while(0)
#define OPUS_FPRINTF (void)

#endif


#endif 
