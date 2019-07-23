







#include <string>
#include <vector>

#include "build/build_config.h"

#include "base/clipboard.h"
#include "base/file_path.h"
#include "base/gfx/rect.h"
#include "base/gfx/native_widget_types.h"
#include "base/shared_memory.h"
#include "chrome/common/ipc_message_macros.h"
#include "chrome/common/transport_dib.h"
#include "skia/include/SkBitmap.h"
#include "webkit/glue/dom_operations.h"
#include "webkit/glue/webappcachecontext.h"
#include "webkit/glue/webcursor.h"
#include "webkit/glue/webplugin.h"








IPC_BEGIN_MESSAGES(View)
  
  
  IPC_MESSAGE_CONTROL1(ViewMsg_SetNextPageID,
                       int32 )

  
  
  
  IPC_MESSAGE_CONTROL4(ViewMsg_New,
                       gfx::NativeViewId, 
                       ModalDialogEvent, 
                       WebPreferences,
                       int32 )

  
  IPC_MESSAGE_CONTROL3(ViewMsg_SetCacheCapacities,
                       size_t ,
                       size_t ,
                       size_t )

  
  
  
  IPC_MESSAGE_ROUTED1(ViewMsg_CreatingNew_ACK,
                      gfx::NativeViewId )

  
  IPC_MESSAGE_ROUTED0(ViewMsg_Close)

  
  
  
  
  
  IPC_MESSAGE_ROUTED2(ViewMsg_Resize,
                      gfx::Size ,
                      gfx::Rect )

  
  
  IPC_MESSAGE_ROUTED0(ViewMsg_WasHidden)

  
  
  
  
  
  IPC_MESSAGE_ROUTED1(ViewMsg_WasRestored,
                      bool )

  
  
  IPC_MESSAGE_ROUTED0(ViewMsg_CaptureThumbnail)

  
  
  IPC_MESSAGE_ROUTED0(ViewMsg_PaintRect_ACK)

  
  
  IPC_MESSAGE_ROUTED0(ViewMsg_PrintPages)

  
  
  IPC_MESSAGE_ROUTED0(ViewMsg_ScrollRect_ACK)

  
  IPC_MESSAGE_ROUTED0(ViewMsg_HandleInputEvent)

  IPC_MESSAGE_ROUTED0(ViewMsg_MouseCaptureLost)

  
  IPC_MESSAGE_ROUTED1(ViewMsg_SetFocus, bool )

  
  
  IPC_MESSAGE_ROUTED1(ViewMsg_SetInitialFocus, bool )

  
  
  IPC_MESSAGE_ROUTED1(ViewMsg_Navigate, ViewMsg_Navigate_Params)

  IPC_MESSAGE_ROUTED0(ViewMsg_Stop)

  
  
  IPC_MESSAGE_ROUTED4(ViewMsg_LoadAlternateHTMLText,
                      std::string ,
                      bool, 
                      GURL ,
                      std::string )

  
  
  
  
  IPC_MESSAGE_ROUTED1(ViewMsg_StopFinding, bool )

  
  
  IPC_MESSAGE_ROUTED0(ViewMsg_Undo)
  IPC_MESSAGE_ROUTED0(ViewMsg_Redo)
  IPC_MESSAGE_ROUTED0(ViewMsg_Cut)
  IPC_MESSAGE_ROUTED0(ViewMsg_Copy)
  IPC_MESSAGE_ROUTED0(ViewMsg_Paste)
  IPC_MESSAGE_ROUTED1(ViewMsg_Replace, std::wstring)
  IPC_MESSAGE_ROUTED0(ViewMsg_ToggleSpellCheck)
  IPC_MESSAGE_ROUTED0(ViewMsg_Delete)
  IPC_MESSAGE_ROUTED0(ViewMsg_SelectAll)

  
  
  IPC_MESSAGE_ROUTED2(ViewMsg_CopyImageAt,
                      int ,
                      int )

  
  
  
  IPC_MESSAGE_CONTROL1(ViewMsg_VisitedLink_NewTable, base::SharedMemoryHandle)

  
  
  
  IPC_MESSAGE_CONTROL1(ViewMsg_UserScripts_NewScripts, base::SharedMemoryHandle)

  
  IPC_MESSAGE_ROUTED3(ViewMsg_Find,
                      int ,
                      string16 ,
                      WebKit::WebFindOptions)

  
  IPC_MESSAGE_ROUTED2(ViewMsg_Resource_ReceivedResponse,
                      int ,
                      ResourceResponseHead)

  
  
  IPC_MESSAGE_ROUTED3(ViewMsg_Resource_DownloadProgress,
                      int ,
                      int64 ,
                      int64 )

  
  IPC_MESSAGE_ROUTED3(ViewMsg_Resource_UploadProgress,
                      int ,
                      int64 ,
                      int64 )

  
  IPC_MESSAGE_ROUTED2(ViewMsg_Resource_ReceivedRedirect,
                      int ,
                      GURL )

  
  
  IPC_MESSAGE_ROUTED3(ViewMsg_Resource_DataReceived,
                      int ,
                      base::SharedMemoryHandle ,
                      int )

  
  IPC_MESSAGE_ROUTED3(ViewMsg_Resource_RequestComplete,
                      int ,
                      URLRequestStatus ,
                      std::string )

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  IPC_MESSAGE_ROUTED2(ViewMsg_ScriptEvalRequest,
                      std::wstring,  
                      std::wstring  )

  
  
  
  IPC_MESSAGE_ROUTED2(ViewMsg_CSSInsertRequest,
                      std::wstring,  
                      std::string  )

  
  IPC_MESSAGE_ROUTED3(ViewMsg_AddMessageToConsole,
                      string16 ,
                      string16 ,
                      WebKit::WebConsoleMessage::Level )

  
  IPC_MESSAGE_ROUTED0(ViewMsg_DebugAttach)

  
  IPC_MESSAGE_ROUTED0(ViewMsg_DebugDetach)

  
  IPC_MESSAGE_ROUTED1(ViewMsg_DebugBreak,
                      bool  )

  
  IPC_MESSAGE_ROUTED1(ViewMsg_DebugCommand,
                      std::wstring  )

  
  
  
  
  IPC_MESSAGE_ROUTED0(ViewMsg_SetupDevToolsClient)

  
  IPC_MESSAGE_ROUTED1(ViewMsg_Zoom,
                      int )

  
  IPC_MESSAGE_ROUTED1(ViewMsg_InsertText,
                      string16  )

  
  IPC_MESSAGE_ROUTED1(ViewMsg_SetPageEncoding,
                      std::wstring )

  
  IPC_MESSAGE_ROUTED2(ViewMsg_InspectElement,
                      int  ,
                      int  )

  
  IPC_MESSAGE_ROUTED0(ViewMsg_ShowJavaScriptConsole)

  
  IPC_MESSAGE_ROUTED1(ViewMsg_ReservePageIDRange,
                      int  )

  
  IPC_MESSAGE_ROUTED1(ViewMsg_FormFill,
                      FormData )

  
  
  IPC_MESSAGE_ROUTED1(ViewMsg_FillPasswordForm,
                      PasswordFormDomManager::FillData )

  
  IPC_MESSAGE_ROUTED3(ViewMsg_DragTargetDragEnter,
                      WebDropData ,
                      gfx::Point ,
                      gfx::Point )
  IPC_MESSAGE_ROUTED2(ViewMsg_DragTargetDragOver,
                      gfx::Point ,
                      gfx::Point )
  IPC_MESSAGE_ROUTED0(ViewMsg_DragTargetDragLeave)
  IPC_MESSAGE_ROUTED2(ViewMsg_DragTargetDrop,
                      gfx::Point ,
                      gfx::Point )

  IPC_MESSAGE_ROUTED1(ViewMsg_UploadFile, ViewMsg_UploadFile_Params)

  
  
  IPC_MESSAGE_ROUTED3(ViewMsg_DragSourceEndedOrMoved,
                      gfx::Point ,
                      gfx::Point ,
                      bool )

  
  IPC_MESSAGE_ROUTED0(ViewMsg_DragSourceSystemDragEnded)

  
  
  
  IPC_MESSAGE_ROUTED1(ViewMsg_AllowBindings,
                      int )

  
  
  IPC_MESSAGE_ROUTED2(ViewMsg_SetDOMUIProperty,
                      std::string ,
                      std::string )

  
  
  
  
  
  
  
  
  
  
  
  IPC_MESSAGE_ROUTED1(ViewMsg_ImeSetInputMode,
                      bool )

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  IPC_MESSAGE_ROUTED5(ViewMsg_ImeSetComposition,
                      int, 
                      int, 
                      int, 
                      int, 
                      std::wstring  )

  
  IPC_MESSAGE_ROUTED1(ViewMsg_UpdateWebPreferences, WebPreferences)

  
  
  
  
  IPC_MESSAGE_ROUTED0(ViewMsg_FindReplyACK)

  
  
  IPC_MESSAGE_ROUTED0(ViewMsg_UpdateTargetURL_ACK)

  
  IPC_MESSAGE_ROUTED1(ViewMsg_SetAltErrorPageURL, GURL)

  
  IPC_MESSAGE_ROUTED0(ViewMsg_InstallMissingPlugin)

  
  IPC_MESSAGE_CONTROL0(ViewMsg_PurgePluginListCache)

  IPC_MESSAGE_ROUTED1(ViewMsg_RunFileChooserResponse,
                      std::vector<FilePath> )

  
  IPC_MESSAGE_ROUTED0(ViewMsg_EnableViewSourceMode)

  IPC_MESSAGE_ROUTED2(ViewMsg_UpdateBackForwardListCount,
                      int ,
                      int )

  
  IPC_SYNC_MESSAGE_ROUTED1_1(ViewMsg_GetAccessibilityInfo,
                             webkit_glue::WebAccessibility::InParams
                             ,
                             webkit_glue::WebAccessibility::OutParams
                             )

  
  
  
  IPC_MESSAGE_ROUTED2(ViewMsg_ClearAccessibilityInfo,
                      int  ,
                      bool )

  
  
  IPC_MESSAGE_ROUTED1(ViewMsg_GetAllSavableResourceLinksForCurrentPage,
                      GURL )

  
  
  IPC_MESSAGE_ROUTED3(ViewMsg_GetSerializedHtmlDataForCurrentPageWithLocalLinks,
                      std::vector<GURL> ,
                      std::vector<FilePath> ,
                      FilePath )

  
  
  IPC_MESSAGE_ROUTED1(ViewMsg_GetApplicationInfo, int32 )

  
  
  IPC_MESSAGE_ROUTED3(ViewMsg_DownloadImage,
                      int ,
                      GURL ,
                      int 

)

  
  
  
  IPC_MESSAGE_ROUTED0(ViewMsg_CantFocus)

  
  
  
  IPC_MESSAGE_ROUTED0(ViewMsg_ShouldClose)

  
  
  IPC_MESSAGE_ROUTED2(ViewMsg_ClosePage,
                      int ,
                      int )

  
  
  IPC_MESSAGE_CONTROL0(ViewMsg_GetCacheResourceStats)

  
  IPC_MESSAGE_CONTROL0(ViewMsg_GetRendererHistograms)

  
  IPC_MESSAGE_ROUTED0(ViewMsg_ThemeChanged)

  
  
  IPC_MESSAGE_ROUTED1(ViewMsg_Repaint,
                      gfx::Size )

  
  IPC_MESSAGE_ROUTED3(ViewMsg_HandleMessageFromExternalHost,
                      std::string ,
                      std::string ,
                      std::string )

  
  
  
  
  IPC_MESSAGE_ROUTED0(ViewMsg_DisassociateFromPopupCount)

  
  
  
  IPC_MESSAGE_CONTROL3(AppCacheMsg_AppCacheSelected,
                       int ,
                       int ,
                       int64 )

  
  
  IPC_MESSAGE_ROUTED4(ViewMsg_AutofillSuggestions,
                      int64 ,
                      int ,
                      std::vector<std::wstring> ,
                      int )

  
  
  
  IPC_MESSAGE_ROUTED1(ViewMsg_PopupNotificationVisiblityChanged,
                      bool )

  
  IPC_MESSAGE_ROUTED1(ViewMsg_RequestAudioPacket,
                      int )

  
  
  
  IPC_MESSAGE_ROUTED3(ViewMsg_NotifyAudioStreamCreated,
                      int ,
                      base::SharedMemoryHandle ,
                      int )

  
  
  IPC_MESSAGE_ROUTED3(ViewMsg_NotifyAudioStreamStateChanged,
                      int ,
                      AudioOutputStream::State ,
                      int 
)

  IPC_MESSAGE_ROUTED3(ViewMsg_NotifyAudioStreamVolume,
                      int ,
                      double ,
                      double )

  
  
  IPC_MESSAGE_ROUTED0(ViewMsg_MoveOrResizeStarted)

  
  IPC_MESSAGE_ROUTED2(ViewMsg_ExtensionResponse,
                      int ,
                      std::string )

  
  
  
  
  IPC_MESSAGE_CONTROL2(ViewMsg_ExtensionHandleConnect,
                       int ,
                       std::string )

  
  IPC_MESSAGE_CONTROL2(ViewMsg_ExtensionHandleMessage,
                       std::string ,
                       int )

  
  IPC_MESSAGE_CONTROL2(ViewMsg_ExtensionHandleEvent,
                       std::string ,
                       std::string )

  
  IPC_MESSAGE_CONTROL1(ViewMsg_Extension_SetFunctionNames,
                       std::vector<std::string>)

  
  
  
  
  
  
  
  
  IPC_MESSAGE_ROUTED1(ViewMsg_SetTextDirection,
                      int )

  
  IPC_MESSAGE_ROUTED0(ViewMsg_ClearFocusedNode)

  
  
  IPC_MESSAGE_ROUTED1(ViewMsg_SetBackground,
                      SkBitmap )
IPC_END_MESSAGES(View)






IPC_BEGIN_MESSAGES(ViewHost)
  
  
  
  
  IPC_SYNC_MESSAGE_CONTROL2_2(ViewHostMsg_CreateWindow,
                              int ,
                              bool ,
                              int ,
                              ModalDialogEvent )

  
  
  
  IPC_SYNC_MESSAGE_CONTROL2_1(ViewHostMsg_CreateWidget,
                              int ,
                              bool ,
                              int )

  
  
  
  
  
  
  
  IPC_MESSAGE_ROUTED5(ViewHostMsg_ShowView,
                      int ,
                      WindowOpenDisposition ,
                      gfx::Rect ,
                      bool ,
                      GURL )

  IPC_MESSAGE_ROUTED2(ViewHostMsg_ShowWidget,
                      int ,
                      gfx::Rect )

  
  
  IPC_SYNC_MESSAGE_ROUTED0_0(ViewHostMsg_RunModal)

  IPC_MESSAGE_CONTROL1(ViewHostMsg_UpdatedCacheStats,
                       WebKit::WebCache::UsageStats )

  
  
  IPC_MESSAGE_ROUTED0(ViewHostMsg_RenderViewReady)

  
  
  IPC_MESSAGE_ROUTED0(ViewHostMsg_RenderViewGone)

  
  
  
  
  IPC_MESSAGE_ROUTED0(ViewHostMsg_Close)

  
  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_RequestMove,
                      gfx::Rect )

  
  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_FrameNavigate,
                      ViewHostMsg_FrameNavigate_Params)

  
  
  IPC_MESSAGE_ROUTED2(ViewHostMsg_UpdateState,
                      int32 ,
                      std::string )

  
  
  
  IPC_MESSAGE_ROUTED2(ViewHostMsg_UpdateTitle, int32, std::wstring)

  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_UpdateEncoding,
                      std::wstring )

  
  
  IPC_MESSAGE_ROUTED2(ViewHostMsg_UpdateTargetURL, int32, GURL)

  
  
  
  IPC_MESSAGE_ROUTED0(ViewHostMsg_DidStartLoading)

  
  
  IPC_MESSAGE_ROUTED0(ViewHostMsg_DidStopLoading)

  
  
  
  
  IPC_MESSAGE_ROUTED4(ViewHostMsg_DidLoadResourceFromMemoryCache,
                      GURL ,
                      std::string  ,
                      std::string  ,
                      std::string  )

  
  IPC_MESSAGE_ROUTED2(ViewHostMsg_DidStartProvisionalLoadForFrame,
                      bool ,
                      GURL )

  
  IPC_MESSAGE_ROUTED4(ViewHostMsg_DidFailProvisionalLoadWithError,
                      bool ,
                      int ,
                      GURL ,
                      bool 

 )

  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_PaintRect,
                      ViewHostMsg_PaintRect_Params)

  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_ScrollRect,
                      ViewHostMsg_ScrollRect_Params)

  
  
  
  
  IPC_MESSAGE_ROUTED0(ViewHostMsg_HandleInputEvent_ACK)

  IPC_MESSAGE_ROUTED0(ViewHostMsg_Focus)
  IPC_MESSAGE_ROUTED0(ViewHostMsg_Blur)

  
  IPC_SYNC_MESSAGE_ROUTED1_1(ViewHostMsg_GetWindowRect,
                             gfx::NativeViewId ,
                             gfx::Rect )

  IPC_MESSAGE_ROUTED1(ViewHostMsg_SetCursor, WebCursor)
  
  
  
  
  
  
  IPC_MESSAGE_ROUTED5(ViewHostMsg_Find_Reply,
                      int ,
                      int ,
                      gfx::Rect ,
                      int ,
                      bool )

  
  IPC_MESSAGE_ROUTED2(ViewHostMsg_RequestResource,
                      int ,
                      ViewHostMsg_Resource_Request)

  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_CancelRequest,
                      int )

  
  IPC_SYNC_MESSAGE_ROUTED2_1(ViewHostMsg_SyncLoad,
                             int ,
                             ViewHostMsg_Resource_Request,
                             SyncLoadResult)

  
  
  IPC_MESSAGE_CONTROL3(ViewHostMsg_SetCookie,
                       GURL ,
                       GURL ,
                       std::string )

  
  IPC_SYNC_MESSAGE_CONTROL2_1(ViewHostMsg_GetCookies,
                              GURL ,
                              GURL ,
                              std::string )

  
  IPC_SYNC_MESSAGE_CONTROL1_1(ViewHostMsg_GetPlugins,
                              bool ,
                              std::vector<WebPluginInfo> )

  
  
  IPC_SYNC_MESSAGE_CONTROL3_2(ViewHostMsg_GetPluginPath,
                              GURL ,
                              std::string ,
                              std::string ,
                              FilePath ,
                              std::string )

  
  IPC_SYNC_MESSAGE_CONTROL0_1(ViewHostMsg_GetDataDir,
                              std::wstring )

  
  
  IPC_MESSAGE_CONTROL2(ViewHostMsg_PluginMessage,
                       FilePath ,
                       std::vector<uint8> )

  
  
  IPC_SYNC_MESSAGE_CONTROL2_1(ViewHostMsg_PluginSyncMessage,
                              FilePath ,
                              std::vector<uint8> ,
                              std::vector<uint8> )

  
  IPC_SYNC_MESSAGE_ROUTED1_2(ViewHostMsg_SpellCheck,
                             std::wstring ,
                             int ,
                             int )

  
  IPC_MESSAGE_ROUTED2(ViewHostMsg_DownloadUrl,
                      GURL ,
                      GURL )

  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_GoToEntryAtOffset,
                      int )

  IPC_SYNC_MESSAGE_ROUTED4_2(ViewHostMsg_RunJavaScriptMessage,
                             std::wstring ,
                             std::wstring ,
                             GURL         ,
                             int          ,
                             bool         ,
                             std::wstring )

  
  
  IPC_MESSAGE_CONTROL3(ViewHostMsg_PageContents, GURL, int32, std::wstring)

  
  
  IPC_MESSAGE_ROUTED3(ViewHostMsg_Thumbnail,
                      GURL ,
                      ThumbnailScore ,
                      SkBitmap )

  
  IPC_MESSAGE_ROUTED2(ViewHostMsg_UpdateFavIconURL,
                      int32 ,
                      GURL )

  
  
  IPC_MESSAGE_ROUTED0(ViewHostMsg_PasteFromSelectionClipboard)

  
  
  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_ContextMenu, ContextMenuParams)

  
  IPC_MESSAGE_ROUTED3(ViewHostMsg_OpenURL,
                      GURL ,
                      GURL ,
                      WindowOpenDisposition )

  IPC_MESSAGE_ROUTED1(ViewHostMsg_DidContentsPreferredWidthChange,
                      int )

  
  
  
  
  
  IPC_MESSAGE_ROUTED2(ViewHostMsg_DomOperationResponse,
                      std::string  ,
                      int  )

  
  
  IPC_MESSAGE_ROUTED2(ViewHostMsg_DOMUISend,
                      std::string  ,
                      std::string  )

  
  IPC_MESSAGE_ROUTED3(ViewHostMsg_ForwardMessageToExternalHost,
                      std::string  ,
                      std::string  ,
                      std::string  )

  
  
  
  
  IPC_SYNC_MESSAGE_CONTROL4_2(ViewHostMsg_OpenChannelToPlugin,
                              GURL ,
                              std::string ,
                              std::string ,
                              std::wstring ,
                              std::wstring ,
                              FilePath )

  

  
  IPC_MESSAGE_CONTROL1(ViewHostMsg_ClipboardWriteObjectsAsync,
      Clipboard::ObjectMap )
  
  
  
  IPC_SYNC_MESSAGE_CONTROL1_0(ViewHostMsg_ClipboardWriteObjectsSync,
      Clipboard::ObjectMap )
  IPC_SYNC_MESSAGE_CONTROL1_1(ViewHostMsg_ClipboardIsFormatAvailable,
                              std::string ,
                              bool )
  IPC_SYNC_MESSAGE_CONTROL0_1(ViewHostMsg_ClipboardReadText,
                              string16 )
  IPC_SYNC_MESSAGE_CONTROL0_1(ViewHostMsg_ClipboardReadAsciiText,
                              std::string )
  IPC_SYNC_MESSAGE_CONTROL0_2(ViewHostMsg_ClipboardReadHTML,
                              string16 ,
                              GURL )

#if defined(OS_WIN)
  
  
  IPC_SYNC_MESSAGE_CONTROL1_0(ViewHostMsg_LoadFont,
                              LOGFONT )
#endif  

  
  
  
  IPC_SYNC_MESSAGE_CONTROL1_1(ViewHostMsg_GetScreenInfo,
                              gfx::NativeViewId ,
                              WebKit::WebScreenInfo )

  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_SetTooltipText,
                      std::wstring )

  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_SelectionChanged,
                      std::string )

  
  
  IPC_MESSAGE_ROUTED3(ViewHostMsg_RunFileChooser,
                      bool ,
                      string16 ,
                      FilePath )

  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_PasswordFormsSeen,
                      std::vector<PasswordForm> )

  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_AutofillFormSubmitted,
                      AutofillForm )

  
  
  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_StartDragging,
                      WebDropData )

  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_UpdateDragCursor,
                      bool )

  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_TakeFocus, bool )

  
  
  IPC_MESSAGE_ROUTED3(ViewHostMsg_PageHasOSDD,
                      int32 ,
                      GURL ,
                      bool )

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  IPC_MESSAGE_ROUTED2(ViewHostMsg_ImeUpdateStatus,
                      ViewHostMsg_ImeControl, 
                      gfx::Rect )

  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_InspectElement_Reply,
                      int )

  
  
  IPC_MESSAGE_ROUTED2(ViewHostMsg_DidGetPrintedPagesCount,
                      int ,
                      int )

  
  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_DidPrintPage,
                      ViewHostMsg_DidPrintPage_Params )

  
  IPC_SYNC_MESSAGE_ROUTED0_1(ViewHostMsg_GetDefaultPrintSettings,
                             ViewMsg_Print_Params )

#if defined(OS_WIN)
  
  
  
  
  IPC_SYNC_MESSAGE_ROUTED3_1(ViewHostMsg_ScriptedPrint,
                             gfx::NativeViewId ,
                             int ,
                             int ,
                             ViewMsg_PrintPages_Params 
)
#endif  

  
  
  IPC_MESSAGE_ROUTED3(ViewHostMsg_AddMessageToConsole,
                      std::wstring, 
                      int32, 
                      std::wstring )

  
  IPC_MESSAGE_ROUTED0(ViewHostMsg_DidDebugAttach)

  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_DebuggerOutput,
                      std::wstring )

  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_ForwardToDevToolsClient,
                      IPC::Message )

  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_ForwardToDevToolsAgent,
                      IPC::Message )

  IPC_MESSAGE_ROUTED2(ViewHostMsg_ToolsAgentMsg,
                      int, 
                      std::wstring  )

  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_UserMetricsRecordAction,
                      std::wstring )

  
  IPC_MESSAGE_CONTROL1(ViewHostMsg_RendererHistograms, std::vector<std::string>)

  
  
  IPC_MESSAGE_CONTROL1(ViewHostMsg_DnsPrefetch,
                      std::vector<std::string> )

  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_MissingPluginStatus,
                      int )

  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_CrashedPlugin,
                      FilePath )

  
  IPC_MESSAGE_ROUTED0(ViewHostMsg_JSOutOfMemory)

  
  
  
  IPC_SYNC_MESSAGE_ROUTED2_2(ViewHostMsg_RunBeforeUnloadConfirm,
                             GURL,        
                             std::wstring ,
                             bool         ,
                             std::wstring )

  IPC_MESSAGE_ROUTED3(ViewHostMsg_SendCurrentPageAllSavableResourceLinks,
                      std::vector<GURL> ,
                      std::vector<GURL> ,
                      std::vector<GURL> )

  IPC_MESSAGE_ROUTED3(ViewHostMsg_SendSerializedHtmlData,
                      GURL ,
                      std::string ,
                      int32 )

  IPC_SYNC_MESSAGE_ROUTED4_1(ViewHostMsg_ShowModalHTMLDialog,
                             GURL ,
                             int ,
                             int ,
                             std::string ,
                             std::string )

  IPC_MESSAGE_ROUTED2(ViewHostMsg_DidGetApplicationInfo,
                      int32 ,
                      webkit_glue::WebApplicationInfo)

  
  
  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_ShouldClose_ACK,
                      bool )

  
  
  IPC_MESSAGE_ROUTED2(ViewHostMsg_ClosePage_ACK,
                      int ,
                      int )

  IPC_MESSAGE_ROUTED4(ViewHostMsg_DidDownloadImage,
                      int ,
                      GURL ,
                      bool ,
                      SkBitmap )

  
  IPC_SYNC_MESSAGE_CONTROL1_1(ViewHostMsg_GetMimeTypeFromExtension,
                              FilePath::StringType ,
                              std::string )
  IPC_SYNC_MESSAGE_CONTROL1_1(ViewHostMsg_GetMimeTypeFromFile,
                              FilePath ,
                              std::string )
  IPC_SYNC_MESSAGE_CONTROL1_1(ViewHostMsg_GetPreferredExtensionForMimeType,
                              std::string ,
                              FilePath::StringType )

  
  
  IPC_SYNC_MESSAGE_CONTROL0_1(ViewHostMsg_GetCPBrowsingContext,
                              uint32 )

  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_DataReceived_ACK,
                      int )

  
  IPC_MESSAGE_ROUTED3(ViewHostMsg_DidRedirectProvisionalLoad,
                      int ,
                      GURL ,
                      GURL )

  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_DownloadProgress_ACK,
                      int )

  
  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_UploadProgress_ACK,
                      int )

  
  
  IPC_SYNC_MESSAGE_ROUTED1_1(ViewHostMsg_DuplicateSection,
                             base::SharedMemoryHandle ,
                             base::SharedMemoryHandle )

  
  
  IPC_MESSAGE_CONTROL1(ViewHostMsg_ResourceTypeStats,
                       WebKit::WebCache::ResourceTypeStats)

  
  
  IPC_MESSAGE_CONTROL1(ViewHostMsg_SuddenTerminationChanged,
                       bool )

  
  IPC_SYNC_MESSAGE_ROUTED1_1(ViewHostMsg_GetRootWindowRect,
                             gfx::NativeViewId ,
                             gfx::Rect )

  
  IPC_MESSAGE_CONTROL3(AppCacheMsg_ContextCreated,
                       WebAppCacheContext::ContextType,
                       int ,
                       int )

  
  IPC_MESSAGE_CONTROL1(AppCacheMsg_ContextDestroyed,
                       int )

  
  
  
  
  
  
  
  
  
  
  
  IPC_MESSAGE_CONTROL5(AppCacheMsg_SelectAppCache,
                       int ,
                       int ,
                       GURL  ,
                       int64 ,
                       GURL  )

  
  
  IPC_SYNC_MESSAGE_ROUTED1_1(ViewHostMsg_GetRootWindowResizerRect,
                             gfx::NativeViewId ,
                             gfx::Rect )

  
  IPC_MESSAGE_ROUTED4(ViewHostMsg_QueryFormFieldAutofill,
                      std::wstring ,
                      std::wstring ,
                      int64 ,
                      int )

  
  
  IPC_MESSAGE_ROUTED2(ViewHostMsg_RemoveAutofillEntry,
                      std::wstring ,
                      std::wstring )

  
  
  
  IPC_SYNC_MESSAGE_CONTROL1_2(ViewHostMsg_ResolveProxy,
                              GURL ,
                              int ,
                              std::string )

  
  IPC_MESSAGE_ROUTED2(ViewHostMsg_CreateAudioStream,
                      int ,
                      ViewHostMsg_Audio_CreateStream)

  
  
  IPC_MESSAGE_ROUTED2(ViewHostMsg_NotifyAudioPacketReady,
                      int ,
                      size_t )

  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_StartAudioStream,
                      int )

  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_CloseAudioStream,
                      int )

  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_GetAudioVolume,
                      int )

  
  
  IPC_MESSAGE_ROUTED3(ViewHostMsg_SetAudioVolume,
                      int ,
                      double ,
                      double )

  
  
  IPC_MESSAGE_ROUTED3(ViewHostMsg_ExtensionRequest,
                      std::string ,
                      std::string ,
                      int )

  
  IPC_MESSAGE_CONTROL1(ViewHostMsg_ExtensionAddListener,
                       std::string )

  
  IPC_MESSAGE_CONTROL1(ViewHostMsg_ExtensionRemoveListener,
                       std::string )

#if defined(OS_MACOSX)
  
  
  
  IPC_SYNC_MESSAGE_CONTROL1_1(ViewHostMsg_AllocTransportDIB,
                              size_t, 
                              TransportDIB::Handle )

  
  
  
  IPC_MESSAGE_CONTROL1(ViewHostMsg_FreeTransportDIB,
                       TransportDIB::Id )
#endif

  
  
  
  IPC_SYNC_MESSAGE_CONTROL2_1(ViewHostMsg_CreateDedicatedWorker,
                              GURL ,
                              int ,
                              int )

  
  
  IPC_MESSAGE_CONTROL1(ViewHostMsg_ForwardToWorker,
                       IPC::Message )

  
  
  
  IPC_SYNC_MESSAGE_CONTROL2_1(ViewHostMsg_OpenChannelToExtension,
                              int ,
                              std::string ,
                              int )

  
  
  IPC_MESSAGE_ROUTED2(ViewHostMsg_ExtensionPostMessage,
                      int ,
                      std::string )

  
  IPC_MESSAGE_ROUTED1(ViewHostMsg_ShowPopup,
                      ViewHostMsg_ShowPopup_Params)

IPC_END_MESSAGES(ViewHost)
