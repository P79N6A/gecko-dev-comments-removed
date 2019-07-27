



#ifndef _CPR_DEBUG_H_
#define _CPR_DEBUG_H_

#include "cpr_types.h"
#include "cpr_stdio.h"

__BEGIN_DECLS





extern int32_t cprInfo;

#define CPR_INFO if (cprInfo) notice_msg
#define CPR_ERROR err_msg

__END_DECLS

#endif
