











































#include "chrome/common/ipc_message_macros.h"



IPC_BEGIN_MESSAGES(DevToolsClient)

  
  IPC_MESSAGE_CONTROL1(DevToolsClientMsg_RpcMessage,
                       std::string  )

IPC_END_MESSAGES(DevToolsClient)





IPC_BEGIN_MESSAGES(DevToolsAgent)

  
  IPC_MESSAGE_CONTROL0(DevToolsAgentMsg_Attach)

  
  IPC_MESSAGE_CONTROL0(DevToolsAgentMsg_Detach)

  
  IPC_MESSAGE_CONTROL1(DevToolsAgentMsg_RpcMessage,
                       std::string  )

  
  
  
  
  IPC_MESSAGE_CONTROL1(DevToolsAgentMsg_DebuggerCommand,
                       std::string  )

  
  IPC_MESSAGE_CONTROL2(DevToolsAgentMsg_InspectElement,
                       int ,
                       int )

IPC_END_MESSAGES(DevToolsAgent)
