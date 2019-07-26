
































extern "C" {
#include <stdlib.h> 
#include <unwind.h> 
}
#include "stacktrace.h"

_START_GOOGLE_NAMESPACE_

typedef struct {
  void **result;
  int max_depth;
  int skip_count;
  int count;
} trace_arg_t;



static _Unwind_Reason_Code nop_backtrace(struct _Unwind_Context *uc, void *opq) {
  return _URC_NO_REASON;
}





static bool ready_to_run = false;
class StackTraceInit {
 public:
   StackTraceInit() {
     
     _Unwind_Backtrace(nop_backtrace, NULL);
     ready_to_run = true;
   }
};

static StackTraceInit module_initializer;  

static _Unwind_Reason_Code GetOneFrame(struct _Unwind_Context *uc, void *opq) {
  trace_arg_t *targ = (trace_arg_t *) opq;

  if (targ->skip_count > 0) {
    targ->skip_count--;
  } else {
    targ->result[targ->count++] = (void *) _Unwind_GetIP(uc);
  }

  if (targ->count == targ->max_depth)
    return _URC_END_OF_STACK;

  return _URC_NO_REASON;
}


int GetStackTrace(void** result, int max_depth, int skip_count) {
  if (!ready_to_run)
    return 0;

  trace_arg_t targ;

  skip_count += 1;         

  targ.result = result;
  targ.max_depth = max_depth;
  targ.skip_count = skip_count;
  targ.count = 0;

  _Unwind_Backtrace(GetOneFrame, &targ);

  return targ.count;
}

_END_GOOGLE_NAMESPACE_
