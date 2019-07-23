











#ifndef CHROME_COMMON_CHROME_PLUGIN_API_H__
#define CHROME_COMMON_CHROME_PLUGIN_API_H__

#include "base/basictypes.h"

#ifndef STDCALL
#ifdef WIN32
#define STDCALL __stdcall
#else
#define STDCALL
#endif 
#endif 

#ifdef __cplusplus
extern "C" {
#endif



#define CP_MAJOR_VERSION 0
#define CP_MINOR_VERSION 9
#define CP_VERSION       ((CP_MAJOR_VERSION << 8) | (CP_MINOR_VERSION))

#define CP_GET_MAJOR_VERSION(version) ((version & 0xff00) >> 8)
#define CP_GET_MINOR_VERSION(version) (version & 0x00ff)

typedef unsigned char CPBool;


typedef enum {
  CP_PROCESS_BROWSER = 0,
  CP_PROCESS_PLUGIN,
  CP_PROCESS_RENDERER,
} CPProcessType;


typedef enum {
  
  CPERR_SUCCESS = 0,

  
  CPERR_IO_PENDING = -1,

  
  CPERR_FAILURE = -2,

  
  CPERR_INVALID_VERSION = -3,

  
  CPERR_CANCELLED = -4,

  
  CPERR_INVALID_PARAMETER = -5,
} CPError;


typedef enum {
  
  CPRESPONSEINFO_HTTP_STATUS = 0,

  
  
  CPRESPONSEINFO_HTTP_RAW_HEADERS = 1,
} CPResponseInfoType;


typedef struct _CPID_t {
  int unused;
} CPID_t;
typedef struct _CPID_t* CPID;






typedef uint32 CPBrowsingContext;


typedef enum {
  
  
  
  CPBROWSINGCONTEXT_DATA_DIR_PTR = 0,

  
  
  CPBROWSINGCONTEXT_UI_LOCALE_PTR = 1,
} CPBrowsingContextInfoType;


typedef struct _CPRequest {
  void* pdata;  
  const char* url;  
  const char* method;  
  CPBrowsingContext context;  
} CPRequest;

typedef enum {
  CPREQUESTLOAD_NORMAL = 0,

  
  CPREQUESTLOAD_VALIDATE_CACHE = 1 << 0,

  
  CPREQUESTLOAD_BYPASS_CACHE = 1 << 1,

  
  
  CPREQUESTLOAD_PREFERRING_CACHE = 1 << 2,

  
  
  CPREQUESTLOAD_ONLY_FROM_CACHE = 1 << 3,

  
  
  CPREQUESTLOAD_DISABLE_CACHE = 1 << 4,

  
  CPREQUESTLOAD_DISABLE_INTERCEPT = 1 << 5,

  
  
  
  CPREQUESTLOAD_SYNCHRONOUS = 1 << 20,
} CPRequestLoadFlags;






typedef CPError (STDCALL *CPP_ShutdownFunc)(void);


typedef CPBool (STDCALL *CPP_ShouldInterceptRequestFunc)(CPRequest* request);




typedef void (STDCALL *CPP_HtmlDialogClosedFunc)(
    void* plugin_context, const char* json_retval);




typedef CPError (STDCALL *CPP_HandleCommandFunc)(
    CPBrowsingContext context, int command, void* command_data);








typedef CPError (STDCALL *CPB_HandleCommandFunc)(
    CPID id, CPBrowsingContext context, int command, void* command_data);









typedef void (STDCALL *CPB_EnableRequestInterceptFunc)(
    CPID id, const char** schemes, uint32 num_schemes);





typedef CPError (STDCALL *CPB_CreateRequestFunc)(
    CPID id, CPBrowsingContext context, const char* method, const char* url,
    CPRequest** request);





typedef CPError (STDCALL *CPB_GetCookiesFunc)(
    CPID id, CPBrowsingContext context, const char* url, char** cookies);



typedef void* (STDCALL *CPB_AllocFunc)(uint32 size);


typedef void (STDCALL *CPB_FreeFunc)(void* memory);









typedef void (STDCALL *CPB_SetKeepProcessAliveFunc)(CPID id,
                                                    CPBool keep_alive);








typedef CPError (STDCALL *CPB_ShowHtmlDialogModalFunc)(
    CPID id, CPBrowsingContext context, const char* url, int width, int height,
    const char* json_arguments, char** json_retval);





typedef CPError (STDCALL *CPB_ShowHtmlDialogFunc)(
    CPID id, CPBrowsingContext context, const char* url, int width, int height,
    const char* json_arguments, void* plugin_context);


typedef CPBrowsingContext (STDCALL *CPB_GetBrowsingContextFromNPPFunc)(
    struct _NPP* npp);





typedef int (STDCALL *CPB_GetBrowsingContextInfoFunc)(
    CPID id, CPBrowsingContext context, CPBrowsingContextInfoType type,
    void* buf, uint32 buf_size);




typedef CPError (STDCALL *CPB_GetCommandLineArgumentsFunc)(
    CPID id, CPBrowsingContext context, const char* url, char** arguments);






typedef CPError (STDCALL *CPB_AddUICommandFunc)(CPID id, int command);











typedef CPError (STDCALL *CPR_StartRequestFunc)(CPRequest* request);




typedef void (STDCALL *CPR_EndRequestFunc)(CPRequest* request, CPError reason);







typedef void (STDCALL *CPR_SetExtraRequestHeadersFunc)(CPRequest* request,
                                                       const char* headers);



typedef void (STDCALL *CPR_SetRequestLoadFlagsFunc)(CPRequest* request,
                                                    uint32 flags);






typedef void (STDCALL *CPR_AppendDataToUploadFunc)(
    CPRequest* request, const char* bytes, int bytes_len);










typedef CPError (STDCALL *CPR_AppendFileToUploadFunc)(
    CPRequest* request, const char* filepath, uint64 offset, uint64 length);





typedef int (STDCALL *CPR_GetResponseInfoFunc)(
    CPRequest* request, CPResponseInfoType type,
    void* buf, uint32 buf_size);






typedef int (STDCALL *CPR_ReadFunc)(
    CPRequest* request, void* buf, uint32 buf_size);








typedef void (STDCALL *CPRR_ReceivedRedirectFunc)(CPRequest* request,
                                                  const char* new_url);





typedef void (STDCALL *CPRR_StartCompletedFunc)(CPRequest* request,
                                                CPError result);






typedef void (STDCALL *CPRR_ReadCompletedFunc)(CPRequest* request,
                                               int bytes_read);



typedef void (STDCALL *CPRR_UploadProgressFunc)(CPRequest* request,
                                                uint64 position,
                                                uint64 size);






typedef CPBool (STDCALL *CPB_IsPluginProcessRunningFunc)(CPID id);


typedef CPProcessType (STDCALL *CPB_GetProcessTypeFunc)(CPID id);




typedef CPError (STDCALL *CPB_SendMessageFunc)(CPID id,
                                               const void *data,
                                               uint32 data_len);





typedef CPError (STDCALL *CPB_SendSyncMessageFunc)(CPID id,
                                                   const void *data,
                                                   uint32 data_len,
                                                   void **retval,
                                                   uint32 *retval_len);



typedef CPError (STDCALL *CPB_PluginThreadAsyncCallFunc)(CPID id,
                                                         void (*func)(void *),
                                                         void *user_data);




typedef CPError (STDCALL *CPB_OpenFileDialogFunc)(CPID id,
                                                  CPBrowsingContext context,
                                                  bool multiple_files,
                                                  const char *title,
                                                  const char *filter,
                                                  void *user_data);


typedef void (STDCALL *CPP_OnMessageFunc)(void *data, uint32 data_len);


typedef void (STDCALL *CPP_OnSyncMessageFunc)(void *data, uint32 data_len,
                                              void **retval,
                                              uint32 *retval_len);



typedef void (STDCALL *CPP_OnFileDialogResultFunc)(void *data,
                                                   const char **files,
                                                   uint32 files_len);





typedef struct _CPRequestFuncs {
  uint16 size;
  CPR_SetExtraRequestHeadersFunc set_extra_request_headers;
  CPR_SetRequestLoadFlagsFunc set_request_load_flags;
  CPR_AppendDataToUploadFunc append_data_to_upload;
  CPR_StartRequestFunc start_request;
  CPR_EndRequestFunc end_request;
  CPR_GetResponseInfoFunc get_response_info;
  CPR_ReadFunc read;
  CPR_AppendFileToUploadFunc append_file_to_upload;
} CPRequestFuncs;





typedef struct _CPResponseFuncs {
  uint16 size;
  CPRR_ReceivedRedirectFunc received_redirect;
  CPRR_StartCompletedFunc start_completed;
  CPRR_ReadCompletedFunc read_completed;
  CPRR_UploadProgressFunc upload_progress;
} CPResponseFuncs;





typedef struct _CPPluginFuncs {
  uint16 size;
  uint16 version;
  CPRequestFuncs* request_funcs;
  CPResponseFuncs* response_funcs;
  CPP_ShutdownFunc shutdown;
  CPP_ShouldInterceptRequestFunc should_intercept_request;
  CPP_OnMessageFunc on_message;
  CPP_HtmlDialogClosedFunc html_dialog_closed;
  CPP_HandleCommandFunc handle_command;
  CPP_OnSyncMessageFunc on_sync_message;
  CPP_OnFileDialogResultFunc on_file_dialog_result;
} CPPluginFuncs;





typedef struct _CPBrowserFuncs {
  uint16 size;
  uint16 version;
  CPRequestFuncs* request_funcs;
  CPResponseFuncs* response_funcs;
  CPB_EnableRequestInterceptFunc enable_request_intercept;
  CPB_CreateRequestFunc create_request;
  CPB_GetCookiesFunc get_cookies;
  CPB_AllocFunc alloc;
  CPB_FreeFunc free;
  CPB_SetKeepProcessAliveFunc set_keep_process_alive;
  CPB_ShowHtmlDialogModalFunc show_html_dialog_modal;
  CPB_ShowHtmlDialogFunc show_html_dialog;
  CPB_IsPluginProcessRunningFunc is_plugin_process_running;
  CPB_GetProcessTypeFunc get_process_type;
  CPB_SendMessageFunc send_message;
  CPB_GetBrowsingContextFromNPPFunc get_browsing_context_from_npp;
  CPB_GetBrowsingContextInfoFunc get_browsing_context_info;
  CPB_GetCommandLineArgumentsFunc get_command_line_arguments;
  CPB_AddUICommandFunc add_ui_command;
  CPB_HandleCommandFunc handle_command;
  CPB_SendSyncMessageFunc send_sync_message;
  CPB_PluginThreadAsyncCallFunc plugin_thread_async_call;
  CPB_OpenFileDialogFunc open_file_dialog;
} CPBrowserFuncs;
















typedef CPError (STDCALL *CP_VersionNegotiateFunc)(
  uint16 min_version, uint16 max_version, uint16 *selected_version);








typedef CPError (STDCALL *CP_InitializeFunc)(
    CPID id, const CPBrowserFuncs* bfuncs, CPPluginFuncs* pfuncs);

#ifdef __cplusplus
} 
#endif

#endif
