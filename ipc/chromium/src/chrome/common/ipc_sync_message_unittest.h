



#include "chrome/common/ipc_message_macros.h"

IPC_BEGIN_MESSAGES(Test)
  IPC_SYNC_MESSAGE_CONTROL0_0(SyncChannelTestMsg_NoArgs)

  IPC_SYNC_MESSAGE_CONTROL0_1(SyncChannelTestMsg_AnswerToLife,
                              int )

  IPC_SYNC_MESSAGE_CONTROL1_1(SyncChannelTestMsg_Double,
                              int ,
                              int )

  
  IPC_SYNC_MESSAGE_CONTROL0_1(Msg_C_0_1, bool)

  
  IPC_SYNC_MESSAGE_CONTROL0_2(Msg_C_0_2, bool, int)

  
  IPC_SYNC_MESSAGE_CONTROL0_3(Msg_C_0_3, bool, int, std::string)

  
  IPC_SYNC_MESSAGE_CONTROL1_1(Msg_C_1_1, int, bool)

  
  IPC_SYNC_MESSAGE_CONTROL1_2(Msg_C_1_2, bool, bool, int)

  
  IPC_SYNC_MESSAGE_CONTROL1_3(Msg_C_1_3, int, std::string, int, bool)

  
  IPC_SYNC_MESSAGE_CONTROL2_1(Msg_C_2_1, int, bool, bool)

  
  IPC_SYNC_MESSAGE_CONTROL2_2(Msg_C_2_2, bool, int, bool, int)

  
  IPC_SYNC_MESSAGE_CONTROL2_3(Msg_C_2_3, int, bool, std::string, int, bool)

  
  IPC_SYNC_MESSAGE_CONTROL3_1(Msg_C_3_1, int, bool, std::string, bool)

  
  
  IPC_SYNC_MESSAGE_CONTROL3_2(Msg_C_3_2, std::string, bool, int, bool, int)

  
  
  IPC_SYNC_MESSAGE_CONTROL3_3(Msg_C_3_3, int, std::string, bool, std::string,
                              int, bool)


  

  
  IPC_SYNC_MESSAGE_ROUTED0_1(Msg_R_0_1, bool)

  
  IPC_SYNC_MESSAGE_ROUTED0_2(Msg_R_0_2, bool, int)

  
  IPC_SYNC_MESSAGE_ROUTED0_3(Msg_R_0_3, bool, int, std::string)

  
  IPC_SYNC_MESSAGE_ROUTED1_1(Msg_R_1_1, int, bool)

  
  IPC_SYNC_MESSAGE_ROUTED1_2(Msg_R_1_2, bool, bool, int)

  
  IPC_SYNC_MESSAGE_ROUTED1_3(Msg_R_1_3, int, std::string, int, bool)

  
  IPC_SYNC_MESSAGE_ROUTED2_1(Msg_R_2_1, int, bool, bool)

  
  IPC_SYNC_MESSAGE_ROUTED2_2(Msg_R_2_2, bool, int, bool, int)

  
  IPC_SYNC_MESSAGE_ROUTED2_3(Msg_R_2_3, int, bool, std::string, int, bool)

  
  IPC_SYNC_MESSAGE_ROUTED3_1(Msg_R_3_1, int, bool, std::string, bool)

  
  
  IPC_SYNC_MESSAGE_ROUTED3_2(Msg_R_3_2, std::string, bool, int, bool, int)

  
  
  IPC_SYNC_MESSAGE_ROUTED3_3(Msg_R_3_3, int, std::string, bool, std::string,
                             int, bool)

IPC_END_MESSAGES(TestMsg)
