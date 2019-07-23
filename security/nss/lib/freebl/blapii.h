




































#ifndef _BLAPII_H_
#define _BLAPII_H_

#include "blapit.h"

SEC_BEGIN_PROTOS

#if defined(XP_UNIX) && !defined(NO_CHECK_FORK)

extern PRBool parentForkedAfterC_Initialize;

#define SKIP_AFTER_FORK(x) if (!parentForkedAfterC_Initialize) x

#else

#define SKIP_AFTER_FORK(x) x

#endif

SEC_END_PROTOS

#endif 

