




































#ifndef __FORMAT_H__
#define __FORMAT_H__

#include "npapi.h"

typedef enum
{
  action_invalid = 0,
  action_npn_version,
  action_npn_get_url_notify,
  action_npn_get_url,
  action_npn_post_url_notify,
  action_npn_post_url,
  action_npn_request_read,
  action_npn_new_stream,
  action_npn_write,
  action_npn_destroy_stream,
  action_npn_status,
  action_npn_user_agent,
  action_npn_mem_alloc,
  action_npn_mem_free,
  action_npn_mem_flush,
  action_npn_reload_plugins,
  action_npn_get_java_env,
  action_npn_get_java_peer,
  action_npn_get_value,
  action_npn_set_value,
  action_npn_invalidate_rect,
  action_npn_invalidate_region,
  action_npn_force_redraw,

  action_npp_new,
  action_npp_destroy,
  action_npp_set_window,
  action_npp_new_stream,
  action_npp_destroy_stream,
  action_npp_stream_as_file,
  action_npp_write_ready,
  action_npp_write,
  action_npp_print,
  action_npp_handle_event,
  action_npp_url_notify,
  action_npp_get_java_class,
  action_npp_get_value,
  action_npp_set_value
} NPAPI_Action;

struct LogArgumentStruct
{
  DWORD dwArg;
  int iLength;
  void * pData;

  LogArgumentStruct()
  {
    iLength = 0;
    pData = NULL;
  }

  ~LogArgumentStruct()
  {
    if(pData != NULL)
      delete [] pData;
    iLength = 0;
  }
};

struct LogItemStruct
{
  NPAPI_Action action;
  LogArgumentStruct arg1;
  LogArgumentStruct arg2;
  LogArgumentStruct arg3;
  LogArgumentStruct arg4;
  LogArgumentStruct arg5;
  LogArgumentStruct arg6;
  LogArgumentStruct arg7;

  LogItemStruct(){}
  ~LogItemStruct(){}
};

char * FormatNPAPIError(int iError);
char * FormatNPAPIReason(int iReason);
char * FormatNPPVariable(NPPVariable var);
char * FormatNPNVariable(NPNVariable var);
BOOL FormatPCHARArgument(char * szBuf, int iLength, LogArgumentStruct * parg);
BOOL FormatBOOLArgument(char * szBuf, int iLength, LogArgumentStruct * parg);
BOOL FormatPBOOLArgument(char * szBuf, int iLength, LogArgumentStruct * parg);
LogItemStruct * makeLogItemStruct(NPAPI_Action action, 
                                  DWORD dw1, DWORD dw2, DWORD dw3, DWORD dw4,
                                  DWORD dw5, DWORD dw6, DWORD dw7, BOOL bShort = FALSE);
void freeLogItemStruct(LogItemStruct * lis);
int formatLogItem(LogItemStruct * plis, char * szOutput, BOOL bDOSStyle = FALSE);

#endif 
