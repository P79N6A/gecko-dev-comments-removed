



#include "base/shared_memory.h"
#include "chrome/common/ipc_message_macros.h"
#include "webkit/glue/webcursor.h"




IPC_BEGIN_MESSAGES(PluginProcess)
  
  
  
  IPC_MESSAGE_CONTROL1(PluginProcessMsg_CreateChannel,
                       bool )

  
  
  IPC_MESSAGE_CONTROL1(PluginProcessMsg_PluginMessage,
                       std::vector<uint8> )

  
  
  
  IPC_MESSAGE_CONTROL0(PluginProcessMsg_AskBeforeShutdown)

  
  
  IPC_MESSAGE_CONTROL0(PluginProcessMsg_Shutdown)

IPC_END_MESSAGES(PluginProcess)





IPC_BEGIN_MESSAGES(PluginProcessHost)
  
  IPC_MESSAGE_CONTROL1(PluginProcessHostMsg_ChannelCreated,
                       std::wstring )

  IPC_SYNC_MESSAGE_CONTROL0_1(PluginProcessHostMsg_GetPluginFinderUrl,
                              std::string )

  IPC_MESSAGE_CONTROL0(PluginProcessHostMsg_ShutdownRequest)

  
  
  IPC_MESSAGE_CONTROL1(PluginProcessHostMsg_PluginMessage,
                       std::vector<uint8> )

  
  
  IPC_SYNC_MESSAGE_CONTROL1_1(PluginProcessHostMsg_PluginSyncMessage,
                              std::vector<uint8> ,
                              std::vector<uint8> )

  
  
  IPC_SYNC_MESSAGE_CONTROL2_1(PluginProcessHostMsg_GetCookies,
                              int32 ,
                              GURL ,
                              std::string )

  
  
  
  IPC_SYNC_MESSAGE_CONTROL1_2(PluginProcessHostMsg_ResolveProxy,
                              GURL ,
                              int ,
                              std::string )

#if defined(OS_WIN)
  
  IPC_SYNC_MESSAGE_CONTROL1_1(PluginProcessHostMsg_CreateWindow,
                              HWND ,
                              HWND )

  
  IPC_MESSAGE_CONTROL2(PluginProcessHostMsg_PluginWindowDestroyed,
                       HWND ,
                       HWND )

  IPC_MESSAGE_ROUTED3(PluginProcessHostMsg_DownloadUrl,
                      std::string ,
                      int ,
                      HWND )
#endif

IPC_END_MESSAGES(PluginProcessHost)





IPC_BEGIN_MESSAGES(Plugin)
  
  
  
  IPC_SYNC_MESSAGE_CONTROL1_1(PluginMsg_CreateInstance,
                              std::string ,
                              int )

  
  
  
  IPC_SYNC_MESSAGE_CONTROL1_0(PluginMsg_DestroyInstance,
                              int )

  IPC_SYNC_MESSAGE_CONTROL0_1(PluginMsg_GenerateRouteID,
                             int )

  
  IPC_SYNC_MESSAGE_ROUTED1_1(PluginMsg_Init,
                             PluginMsg_Init_Params,
                             bool )

  
  IPC_SYNC_MESSAGE_ROUTED1_0(PluginMsg_Paint,
                             gfx::Rect )

  
  
  IPC_MESSAGE_ROUTED0(PluginMsg_DidPaint)

  IPC_SYNC_MESSAGE_ROUTED0_2(PluginMsg_Print,
                             base::SharedMemoryHandle ,
                             size_t )

  IPC_SYNC_MESSAGE_ROUTED0_2(PluginMsg_GetPluginScriptableObject,
                             int ,
                             intptr_t )

  IPC_SYNC_MESSAGE_ROUTED1_0(PluginMsg_DidFinishLoadWithReason,
                             int )

  
  
  
  
  IPC_MESSAGE_ROUTED4(PluginMsg_UpdateGeometry,
                      gfx::Rect ,
                      gfx::Rect ,
                      TransportDIB::Id ,
                      TransportDIB::Id )

  IPC_SYNC_MESSAGE_ROUTED0_0(PluginMsg_SetFocus)

  IPC_SYNC_MESSAGE_ROUTED1_2(PluginMsg_HandleEvent,
                             NPEvent ,
                             bool ,
                             WebCursor )

  IPC_SYNC_MESSAGE_ROUTED2_0(PluginMsg_WillSendRequest,
                             int ,
                             GURL )

  IPC_SYNC_MESSAGE_ROUTED1_1(PluginMsg_DidReceiveResponse,
                             PluginMsg_DidReceiveResponseParams,
                             bool )

  IPC_SYNC_MESSAGE_ROUTED3_0(PluginMsg_DidReceiveData,
                             int ,
                             std::vector<char> ,
                             int )

  IPC_SYNC_MESSAGE_ROUTED1_0(PluginMsg_DidFinishLoading,
                             int )

  IPC_SYNC_MESSAGE_ROUTED1_0(PluginMsg_DidFail,
                             int )

  IPC_MESSAGE_ROUTED5(PluginMsg_SendJavaScriptStream,
                      std::string ,
                      std::wstring ,
                      bool ,
                      bool ,
                      intptr_t )

  IPC_MESSAGE_ROUTED2(PluginMsg_DidReceiveManualResponse,
                      std::string ,
                      PluginMsg_DidReceiveResponseParams)

  IPC_MESSAGE_ROUTED1(PluginMsg_DidReceiveManualData,
                      std::vector<char> )

  IPC_MESSAGE_ROUTED0(PluginMsg_DidFinishManualLoading)

  IPC_MESSAGE_ROUTED0(PluginMsg_DidManualLoadFail)

  IPC_MESSAGE_ROUTED0(PluginMsg_InstallMissingPlugin)

  IPC_SYNC_MESSAGE_ROUTED1_0(PluginMsg_HandleURLRequestReply,
                             PluginMsg_URLRequestReply_Params)

  IPC_SYNC_MESSAGE_ROUTED3_0(PluginMsg_URLRequestRouted,
                             std::string ,
                             bool ,
                             intptr_t )
IPC_END_MESSAGES(Plugin)






IPC_BEGIN_MESSAGES(PluginHost)
  
  
  
  IPC_SYNC_MESSAGE_ROUTED1_0(PluginHostMsg_SetWindow,
                             gfx::NativeViewId )

#if defined(OS_WIN)
  
  
  
  
  IPC_SYNC_MESSAGE_ROUTED1_0(PluginHostMsg_SetWindowlessPumpEvent,
                             HANDLE )
#endif

  IPC_MESSAGE_ROUTED1(PluginHostMsg_URLRequest,
                      PluginHostMsg_URLRequest_Params)

  IPC_SYNC_MESSAGE_ROUTED1_0(PluginHostMsg_CancelResource,
                             int )

  IPC_MESSAGE_ROUTED1(PluginHostMsg_InvalidateRect,
                      gfx::Rect )

  IPC_SYNC_MESSAGE_ROUTED1_2(PluginHostMsg_GetWindowScriptNPObject,
                             int ,
                             bool ,
                             intptr_t )

  IPC_SYNC_MESSAGE_ROUTED1_2(PluginHostMsg_GetPluginElement,
                             int ,
                             bool ,
                             intptr_t )

  IPC_MESSAGE_ROUTED3(PluginHostMsg_SetCookie,
                      GURL ,
                      GURL ,
                      std::string )

  IPC_SYNC_MESSAGE_ROUTED2_1(PluginHostMsg_GetCookies,
                             GURL ,
                             GURL ,
                             std::string )

  
  
  
  IPC_SYNC_MESSAGE_ROUTED4_1(PluginHostMsg_ShowModalHTMLDialog,
                              GURL ,
                              int ,
                              int ,
                              std::string ,
                              std::string )

  IPC_MESSAGE_ROUTED1(PluginHostMsg_MissingPluginStatus,
                      int )

  IPC_SYNC_MESSAGE_ROUTED0_1(PluginHostMsg_GetCPBrowsingContext,
                             uint32 )

  IPC_MESSAGE_ROUTED0(PluginHostMsg_CancelDocumentLoad)

  IPC_MESSAGE_ROUTED5(PluginHostMsg_InitiateHTTPRangeRequest,
                      std::string ,
                      std::string ,
                      intptr_t    ,
                      bool        ,
                      intptr_t    )

IPC_END_MESSAGES(PluginHost)





IPC_BEGIN_MESSAGES(NPObject)
  IPC_SYNC_MESSAGE_ROUTED0_0(NPObjectMsg_Release)

  IPC_SYNC_MESSAGE_ROUTED1_1(NPObjectMsg_HasMethod,
                             NPIdentifier_Param ,
                             bool )

  IPC_SYNC_MESSAGE_ROUTED3_2(NPObjectMsg_Invoke,
                             bool ,
                             NPIdentifier_Param ,
                             std::vector<NPVariant_Param> ,
                             NPVariant_Param ,
                             bool )

  IPC_SYNC_MESSAGE_ROUTED1_1(NPObjectMsg_HasProperty,
                             NPIdentifier_Param ,
                             bool )

  IPC_SYNC_MESSAGE_ROUTED1_2(NPObjectMsg_GetProperty,
                             NPIdentifier_Param ,
                             NPVariant_Param ,
                             bool )

  IPC_SYNC_MESSAGE_ROUTED2_1(NPObjectMsg_SetProperty,
                             NPIdentifier_Param ,
                             NPVariant_Param ,
                             bool )

  IPC_SYNC_MESSAGE_ROUTED1_1(NPObjectMsg_RemoveProperty,
                             NPIdentifier_Param ,
                             bool )

  IPC_SYNC_MESSAGE_ROUTED0_0(NPObjectMsg_Invalidate)

  IPC_SYNC_MESSAGE_ROUTED0_2(NPObjectMsg_Enumeration,
                             std::vector<NPIdentifier_Param> ,
                             bool )

  IPC_SYNC_MESSAGE_ROUTED2_2(NPObjectMsg_Evaluate,
                             std::string ,
                             bool ,
                             NPVariant_Param ,
                             bool )

  IPC_SYNC_MESSAGE_ROUTED1_0(NPObjectMsg_SetException,
                             std::string )

IPC_END_MESSAGES(NPObject)
