



































#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: error.c,v $ $Revision: 1.9 $ $Date: 2008/05/17 03:44:39 $";
#endif 








#ifndef BASE_H
#include "base.h"
#endif 
#include <limits.h> 
#include <string.h> 

#define NSS_MAX_ERROR_STACK_COUNT 16 /* error codes */








struct stack_header_str {
  PRUint16 space;
  PRUint16 count;
};

struct error_stack_str {
  struct stack_header_str header;
  PRInt32 stack[1];
};
typedef struct error_stack_str error_stack;










#define INVALID_TPD_INDEX UINT_MAX
static PRUintn error_stack_index = INVALID_TPD_INDEX;








static PRCallOnceType error_call_once;






static PRStatus
error_once_function ( void)
{
  return PR_NewThreadPrivateIndex(&error_stack_index, PR_Free);
}









static error_stack *
error_get_my_stack ( void)
{
  PRStatus st;
  error_stack *rv;
  PRUintn new_size;
  PRUint32 new_bytes;
  error_stack *new_stack;

  if( INVALID_TPD_INDEX == error_stack_index ) {
    st = PR_CallOnce(&error_call_once, error_once_function);
    if( PR_SUCCESS != st ) {
      return (error_stack *)NULL;
    }
  }

  rv = (error_stack *)PR_GetThreadPrivate(error_stack_index);
  if( (error_stack *)NULL == rv ) {
    
    new_size = 16;
  } else if( rv->header.count == rv->header.space  &&
             rv->header.count  < NSS_MAX_ERROR_STACK_COUNT ) {
    
    new_size = PR_MIN( rv->header.space * 2, NSS_MAX_ERROR_STACK_COUNT);
  } else {
    
    return rv;
  }

  new_bytes = (new_size * sizeof(PRInt32)) + sizeof(error_stack);
  
  new_stack = PR_Calloc(1, new_bytes);
  
  if( (error_stack *)NULL != new_stack ) {
    if( (error_stack *)NULL != rv ) {
	(void)nsslibc_memcpy(new_stack,rv,rv->header.space);
    }
    new_stack->header.space = new_size;
  }

  
  PR_SetThreadPrivate(error_stack_index, new_stack);
  return new_stack;
}































NSS_IMPLEMENT PRInt32
NSS_GetError ( void)
{
  error_stack *es = error_get_my_stack();

  if( (error_stack *)NULL == es ) {
    return NSS_ERROR_NO_MEMORY; 
  }

  if( 0 == es->header.count ) {
    return 0;
  }

  return es->stack[ es->header.count-1 ];
}



















NSS_IMPLEMENT PRInt32 *
NSS_GetErrorStack ( void)
{
  error_stack *es = error_get_my_stack();

  if( (error_stack *)NULL == es ) {
    return (PRInt32 *)NULL;
  }

  
  es->stack[ es->header.count ] = 0;

  return es->stack;
}









NSS_IMPLEMENT void
nss_SetError ( PRUint32 error)
{
  error_stack *es;

  if( 0 == error ) {
    nss_ClearErrorStack();
    return;
  }

  es = error_get_my_stack();
  if( (error_stack *)NULL == es ) {
    
    return;
  }

  if (es->header.count < es->header.space) {
    es->stack[ es->header.count++ ] = error;
  } else {
    memmove(es->stack, es->stack + 1, 
		(es->header.space - 1) * (sizeof es->stack[0]));
    es->stack[ es->header.space - 1 ] = error;
  }
  return;
}







NSS_IMPLEMENT void
nss_ClearErrorStack ( void)
{
  error_stack *es = error_get_my_stack();
  if( (error_stack *)NULL == es ) {
    
    return;
  }

  es->header.count = 0;
  es->stack[0] = 0;
  return;
}







NSS_IMPLEMENT void
nss_DestroyErrorStack ( void)
{
  if( INVALID_TPD_INDEX != error_stack_index ) {
    PR_SetThreadPrivate(error_stack_index, NULL);
  }
  return;
}
