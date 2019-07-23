




































#include "tmIPCModule.h"
#include "tmTransaction.h"
#include "tmTransactionManager.h"





static ipcModuleMethods gTransMethods =
{
    IPC_MODULE_METHODS_VERSION,
    tmIPCModule::Init,
    tmIPCModule::Shutdown,
    tmIPCModule::HandleMsg
};

static ipcModuleEntry gTransModuleEntry[] =
{
    { TRANSACTION_MODULE_ID, &gTransMethods }
};

IPC_IMPL_GETMODULES(TransactionModule, gTransModuleEntry)

static const nsID kTransModuleID = TRANSACTION_MODULE_ID;




tmTransactionManager *tmIPCModule::tm;




void
tmIPCModule::Init() {
  if (!tm)
    InitInternal();
}

void
tmIPCModule::Shutdown() {
  if (tm) {
    delete tm;
    tm = nsnull;
  }
}


void
tmIPCModule::HandleMsg(ipcClientHandle client, const nsID &target, 
                       const void *data, PRUint32 dataLen) {

  
  if (!tm && (InitInternal() < 0))
    return;

  
  tmTransaction *trans = new tmTransaction();

  
  if (trans) {
    if(NS_SUCCEEDED(trans->Init(IPC_GetClientID(client),  
                                TM_INVALID_ID,            
                                TM_INVALID,               
                                TM_INVALID,               
                                (PRUint8 *)data,          
                                dataLen))) {              
      
      tm->HandleTransaction(trans);
    }
    else
      delete trans;
  }
}





void
tmIPCModule::SendMsg(PRUint32 aDestClientIPCID, tmTransaction *aTransaction) {

  IPC_SendMsg(aDestClientIPCID,
              kTransModuleID,
              (void *)aTransaction->GetRawMessage(),
              aTransaction->GetRawMessageLength());
}




PRInt32
tmIPCModule::InitInternal() {

  tm = new tmTransactionManager();
  if (tm)
    return tm->Init();
  return -1;
}

