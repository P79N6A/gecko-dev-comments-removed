

















































































#ifndef _r_macros_h
#define _r_macros_h


#define PROTO_LIST(a) a

#ifndef WIN32
#ifndef __GNUC__
#define __FUNCTION__ "unknown"
#endif
#endif

#ifdef R_TRACE_ERRORS
#ifdef WIN32
#define REPORT_ERROR_(caller,a) printf("%s: error %d at %s:%d (function %s)\n", \
	caller,a,__FILE__,__LINE__,__FUNCTION__)
#else
#define REPORT_ERROR_(caller,a) fprintf(stderr,"%s: error %d at %s:%d (function %s)\n", \
	caller,a,__FILE__,__LINE__,__FUNCTION__)
#endif
#else
#define REPORT_ERROR_(caller,a)
#endif

#ifndef ERETURN
#define ERETURN(a) do {int _r=a; if(!_r) _r=-1; REPORT_ERROR_("ERETURN",_r); return(_r);} while(0)
#endif

#ifndef ABORT
#define ABORT(a) do { int _r=a; if(!_r) _r=-1; REPORT_ERROR_("ABORT",_r); _status=_r; goto abort;} while(0)
#endif

#ifndef FREE
#define FREE(a) if(a) free(a)
#endif
#ifndef MIN
#define MIN(a,b) ((a)>(b))?(b):(a)
#endif

#ifndef MAX
#define MAX(a,b) ((b)>(a))?(b):(a)
#endif

#ifdef DEBUG
#define DBG(a) debug a
int debug(int cls, char *format,...);
#else
#define DBG(a)
#endif

#define UNIMPLEMENTED do { fprintf(stderr,"%s:%d Function %s unimplemented\n",__FILE__,__LINE__,__FUNCTION__); abort(); } while(0)

#include "r_memory.h"

#endif
