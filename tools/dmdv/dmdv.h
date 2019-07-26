












































#ifndef __DMDV_H
#define __DMDV_H

#include "valgrind/valgrind.h"





typedef
   enum {
      VG_USERREQ__DMDV_REPORT = VG_USERREQ_TOOL_BASE('D','M'),
      VG_USERREQ__DMDV_UNREPORT,
      VG_USERREQ__DMDV_CHECK_REPORTING
   } Vg_DMDVClientRequest;




#define VALGRIND_DMDV_REPORT(_qzz_addr,_qzz_len,_qzz_name)       \
    VALGRIND_DO_CLIENT_REQUEST_EXPR(0 /* default return */,      \
                            VG_USERREQ__DMDV_REPORT,             \
                            (_qzz_addr), (_qzz_len), (_qzz_name), 0, 0)


#define VALGRIND_DMDV_UNREPORT(_qzz_addr)                        \
    VALGRIND_DO_CLIENT_REQUEST_EXPR(0 /* default return */,      \
                            VG_USERREQ__DMDV_UNREPORT,           \
                            (_qzz_addr), 0, 0, 0, 0)


#define VALGRIND_DMDV_CHECK_REPORTING                            \
    VALGRIND_DO_CLIENT_REQUEST_EXPR(0 /* default return */,      \
                            VG_USERREQ__DMDV_CHECK_REPORTING,    \
                            0, 0, 0, 0, 0)

#endif

