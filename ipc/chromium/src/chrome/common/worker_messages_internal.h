



#include "base/string16.h"
#include "chrome/common/ipc_message_macros.h"
#include "googleurl/src/gurl.h"





IPC_BEGIN_MESSAGES(WorkerProcess)
  IPC_MESSAGE_CONTROL2(WorkerProcessMsg_CreateWorker,
                       GURL  ,
                       int  )
IPC_END_MESSAGES(WorkerProcess)













IPC_BEGIN_MESSAGES(Worker)
  IPC_MESSAGE_ROUTED3(WorkerMsg_StartWorkerContext,
                      GURL ,
                      string16  ,
                      string16  )

  IPC_MESSAGE_ROUTED0(WorkerMsg_TerminateWorkerContext)

  IPC_MESSAGE_ROUTED1(WorkerMsg_PostMessageToWorkerContext,
                      string16  )

  IPC_MESSAGE_ROUTED0(WorkerMsg_WorkerObjectDestroyed)
IPC_END_MESSAGES(Worker)





IPC_BEGIN_MESSAGES(WorkerHost)
  IPC_MESSAGE_ROUTED1(WorkerHostMsg_PostMessageToWorkerObject,
                      string16  )

  IPC_MESSAGE_ROUTED3(WorkerHostMsg_PostExceptionToWorkerObject,
                      string16  ,
                      int  ,
                      string16  )

  IPC_MESSAGE_ROUTED6(WorkerHostMsg_PostConsoleMessageToWorkerObject,
                      int  ,
                      int  ,
                      int  ,
                      string16  ,
                      int  ,
                      string16  )

  IPC_MESSAGE_ROUTED1(WorkerHostMsg_ConfirmMessageFromWorkerObject,
                      bool )

  IPC_MESSAGE_ROUTED1(WorkerHostMsg_ReportPendingActivity,
                      bool )

  IPC_MESSAGE_ROUTED0(WorkerHostMsg_WorkerContextDestroyed)
IPC_END_MESSAGES(WorkerHost)
