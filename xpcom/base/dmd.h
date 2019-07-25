












































#ifndef __DMD_H
#define __DMD_H

#include "valgrind/valgrind.h"





typedef
   enum { 
      VG_USERREQ__DMD_REPORT = VG_USERREQ_TOOL_BASE('D','M'),
      VG_USERREQ__DMD_UNREPORT,
      VG_USERREQ__DMD_CHECK_REPORTING
   } Vg_DMDClientRequest;




#define VALGRIND_DMD_REPORT(_qzz_addr,_qzz_len,_qzz_name)        \
    VALGRIND_DO_CLIENT_REQUEST_EXPR(0 /* default return */,      \
                            VG_USERREQ__DMD_REPORT,              \
                            (_qzz_addr), (_qzz_len), (_qzz_name), 0, 0)


#define VALGRIND_DMD_UNREPORT(_qzz_addr)                         \
    VALGRIND_DO_CLIENT_REQUEST_EXPR(0 /* default return */,      \
                            VG_USERREQ__DMD_UNREPORT,            \
                            (_qzz_addr), 0, 0, 0, 0)


#define VALGRIND_DMD_CHECK_REPORTING                             \
    VALGRIND_DO_CLIENT_REQUEST_EXPR(0 /* default return */,      \
                            VG_USERREQ__DMD_CHECK_REPORTING,     \
                            0, 0, 0, 0, 0)

#endif

