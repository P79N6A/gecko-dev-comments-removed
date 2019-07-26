






































#ifndef MISC_APP_TASK_H
#define MISC_APP_TASK_H

extern cprMsgQueue_t s_misc_msg_queue;

extern cpr_status_e MiscAppTaskSendMsg(uint32_t cmd, cprBuffer_t buf,
                                       uint16_t len);
extern void MiscAppTask(void *arg);

#endif 
