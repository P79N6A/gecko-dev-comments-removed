
























#ifndef _UNWIND_H
#define _UNWIND_H


#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif





typedef enum
  {
    _URC_NO_REASON = 0,
    _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
    _URC_FATAL_PHASE2_ERROR = 2,
    _URC_FATAL_PHASE1_ERROR = 3,
    _URC_NORMAL_STOP = 4,
    _URC_END_OF_STACK = 5,
    _URC_HANDLER_FOUND = 6,
    _URC_INSTALL_CONTEXT = 7,
    _URC_CONTINUE_UNWIND = 8
  }
_Unwind_Reason_Code;

typedef int _Unwind_Action;

#define _UA_SEARCH_PHASE	1
#define _UA_CLEANUP_PHASE	2
#define _UA_HANDLER_FRAME	4
#define _UA_FORCE_UNWIND	8

struct _Unwind_Context;		
struct _Unwind_Exception;	

typedef void (*_Unwind_Exception_Cleanup_Fn) (_Unwind_Reason_Code,
					      struct _Unwind_Exception *);

typedef _Unwind_Reason_Code (*_Unwind_Stop_Fn) (int, _Unwind_Action,
						uint64_t,
						struct _Unwind_Exception *,
						struct _Unwind_Context *,
						void *);





struct _Unwind_Exception
  {
    uint64_t exception_class;
    _Unwind_Exception_Cleanup_Fn exception_cleanup;
    unsigned long private_1;
    unsigned long private_2;
  } __attribute__((__aligned__));

extern _Unwind_Reason_Code _Unwind_RaiseException (struct _Unwind_Exception *);
extern _Unwind_Reason_Code _Unwind_ForcedUnwind (struct _Unwind_Exception *,
						 _Unwind_Stop_Fn, void *);
extern void _Unwind_Resume (struct _Unwind_Exception *);
extern void _Unwind_DeleteException (struct _Unwind_Exception *);
extern unsigned long _Unwind_GetGR (struct _Unwind_Context *, int);
extern void _Unwind_SetGR (struct _Unwind_Context *, int, unsigned long);
extern unsigned long _Unwind_GetIP (struct _Unwind_Context *);
extern unsigned long _Unwind_GetIPInfo (struct _Unwind_Context *, int *);
extern void _Unwind_SetIP (struct _Unwind_Context *, unsigned long);
extern unsigned long _Unwind_GetLanguageSpecificData (struct _Unwind_Context*);
extern unsigned long _Unwind_GetRegionStart (struct _Unwind_Context *);

#ifdef _GNU_SOURCE



typedef _Unwind_Reason_Code (*_Unwind_Trace_Fn) (struct _Unwind_Context *,
						 void *);



# define _UA_END_OF_STACK	16




extern _Unwind_Reason_Code
	  _Unwind_Resume_or_Rethrow (struct _Unwind_Exception *);



extern unsigned long _Unwind_GetBSP (struct _Unwind_Context *);



extern unsigned long _Unwind_GetCFA (struct _Unwind_Context *);


extern unsigned long _Unwind_GetDataRelBase (struct _Unwind_Context *);


extern unsigned long _Unwind_GetTextRelBase (struct _Unwind_Context *);







extern _Unwind_Reason_Code _Unwind_Backtrace (_Unwind_Trace_Fn, void *);







extern void *_Unwind_FindEnclosingFunction (void *);




#endif 

#ifdef __cplusplus
};
#endif

#endif
