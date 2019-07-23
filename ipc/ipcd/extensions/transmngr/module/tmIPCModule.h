




































#ifndef _tmIPCModule_H_
#define _tmIPCModule_H_

#include "ipcModuleUtil.h"
#include "tmUtils.h"


class tmTransaction;
class tmTransactionManager;











class tmIPCModule
{
public:

  
  

  


  static void Shutdown();

  


  static void Init();

  



  static void HandleMsg(ipcClientHandle client,
                        const nsID     &target,
                        const void     *data,
                        PRUint32        dataLen);

  
  

  


  static void SendMsg(PRUint32 aDestClientIPCID, tmTransaction *aTransaction);

protected:

  






  static PRInt32 InitInternal();

  static tmTransactionManager *tm;

};

#endif


