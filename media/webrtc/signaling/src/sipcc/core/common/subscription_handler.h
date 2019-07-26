






































#ifndef __SUB_HANDLER_H__
#define __SUB_HANDLER_H__

boolean sub_hndlr_isAlertingBLFState(int inst);
    
boolean sub_hndlr_isInUseBLFState(int inst); 

boolean sub_hndlr_isAvailable(); 

void sub_hndlr_start(); 

void sub_hndlr_stop(); 

void sub_hndlr_controlBLFButtons(boolean state);

void sub_hndlr_NotifyBLFStatus(int requestId, int status, int appId); 

#endif

