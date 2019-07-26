



#ifndef _CPR_TIMERS_H_
#define _CPR_TIMERS_H_

#include "cpr_types.h"
#include "cpr_ipc.h"

__BEGIN_DECLS




typedef void *cprTimer_t;















typedef struct {
    const char *expiredTimerName;
    uint16_t expiredTimerId;
    void *usrData;
} cprCallBackTimerMsg_t;















void cprSleep(uint32_t duration);
























cprTimer_t cprCreateTimer(const char * name,
                          uint16_t applicationTimerId,
                          uint16_t applicationMsgId,
                          cprMsgQueue_t callBackMsgQueue);

















cprRC_t cprStartTimer(cprTimer_t timer, uint32_t duration, void *data);















boolean cprIsTimerRunning(cprTimer_t timer);














cprRC_t cprDestroyTimer(cprTimer_t timer);














cprRC_t cprCancelTimer(cprTimer_t timer);
















cprRC_t cprUpdateTimer(cprTimer_t timer, uint32_t duration);

__END_DECLS

#endif
