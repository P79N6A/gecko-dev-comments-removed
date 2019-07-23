




































#ifndef ipcModule_h__
#define ipcModule_h__

#include "nsID.h"







typedef class ipcClient *ipcClientHandle;








#define IPC_MODULE_METHODS_VERSION (1<<16) // 1.0




struct ipcModuleMethods
{
    
    
    
    
    PRUint32 version;

    
    
    
    void (* init) (void);

    
    
    
    void (* shutdown) (void);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    void (* handleMsg) (ipcClientHandle client,
                        const nsID     &target,
                        const void     *data,
                        PRUint32        dataLen);

    
    
    
    void (* clientUp) (ipcClientHandle client);

    
    
    
    void (* clientDown) (ipcClientHandle client);
};








#define IPC_DAEMON_METHODS_VERSION (1<<16) // 1.0




typedef PRBool (* ipcClientEnumFunc)       (void *closure, ipcClientHandle client, PRUint32 clientID);
typedef PRBool (* ipcClientNameEnumFunc)   (void *closure, ipcClientHandle client, const char *name);
typedef PRBool (* ipcClientTargetEnumFunc) (void *closure, ipcClientHandle client, const nsID &target);




struct ipcDaemonMethods
{
    PRUint32 version;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    PRStatus (* dispatchMsg) (ipcClientHandle client, 
                              const nsID     &target,
                              const void     *data,
                              PRUint32        dataLen);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    PRStatus (* sendMsg) (ipcClientHandle client,
                          const nsID     &target,
                          const void     *data,
                          PRUint32        dataLen);

    
    
    
    
    ipcClientHandle (* getClientByID) (PRUint32 clientID);

    
    
    
    
    
    ipcClientHandle (* getClientByName) (const char *name);

    
    
    
    void (* enumClients) (ipcClientEnumFunc func, void *closure);

    
    
    
    PRUint32 (* getClientID) (ipcClientHandle client);

    
    
    
    
    PRBool (* clientHasName)     (ipcClientHandle client, const char *name);
    PRBool (* clientHasTarget)   (ipcClientHandle client, const nsID &target);
    void   (* enumClientNames)   (ipcClientHandle client, ipcClientNameEnumFunc func, void *closure);
    void   (* enumClientTargets) (ipcClientHandle client, ipcClientTargetEnumFunc func, void *closure);
};





struct ipcModuleEntry
{
    
    
    
    nsID target;

    
    
    
    ipcModuleMethods *methods;
};



#define IPC_EXPORT extern "C" NS_EXPORT











typedef int (* ipcGetModulesFunc) (const ipcDaemonMethods *methods, const ipcModuleEntry **entries);

#endif 
