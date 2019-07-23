




































#ifndef ipcModuleReg_h__
#define ipcModuleReg_h__

#include "ipcModule.h"









void IPC_InitModuleReg(const char *exePath);





void IPC_ShutdownModuleReg();




ipcModuleMethods *IPC_GetModuleByTarget(const nsID &target);




void IPC_NotifyModulesClientUp(ipcClient *);
void IPC_NotifyModulesClientDown(ipcClient *);

#endif 
