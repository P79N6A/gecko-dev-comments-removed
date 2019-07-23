




































#ifndef ipcCommandModule_h__
#define ipcCommandModule_h__

#include "ipcm.h" 

class ipcClient;
class ipcMessage;

void IPCM_HandleMsg(ipcClient *, const ipcMessage *);

#endif 
